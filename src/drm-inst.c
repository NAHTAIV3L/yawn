#include "drm-inst.h"
#include "udev.h"

PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = NULL;

EGLint egl_attrs[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_NONE
};

drmModeConnector* drm_get_connector(struct drm_device* device) {
    drmModeConnector* connector = NULL;
    for (int i = 0; i < device->resources->count_connectors; i++) {
        connector = drmModeGetConnectorCurrent(device->fd, device->resources->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED) {
            return connector;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }
    return NULL;
}

drmModeModeInfo* drm_connector_get_mode(drmModeConnector* connector) {
    for (int i = 0; i < connector->count_modes; i++) {
        if (connector->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
            return &connector->modes[i];
        }
    }
    return NULL;
}

void drm_connector_init(struct drm_connector* connector, size_t connector_idx) {

    connector->connector = drmModeGetConnectorCurrent(connector->device->fd,
                                                      connector->device->resources->connectors[connector_idx]);
    if (!connector->connector) {
        fprintf(stderr, "Failed to get connector: %s, %lu\n", connector->device->path, connector_idx);
        return;
    }

    printf("connector %lu type: %s-%u\n",
           connector_idx,
           drmModeGetConnectorTypeName(connector->connector->connector_type),
           connector->connector->connector_type_id);

    connector->mode = drm_connector_get_mode(connector->connector);
    if (!connector->mode) {
        fprintf(stderr, "Failed to get mode for connector: %s, %lu\n",
                connector->device->path, connector_idx);
        return;
    }


    connector->encoder = drmModeGetEncoder(connector->device->fd, connector->connector->encoder_id);
    if (!connector->encoder) {
        fprintf(stderr, "Failed to find encoder for connector: %s, %lu\n",
                connector->device->path, connector_idx);
        return;
    }

    connector->crtc = drmModeGetCrtc(connector->device->fd, connector->encoder->crtc_id);
    if (!connector->crtc) {
        fprintf(stderr, "Failed to get ctrc for connector: %s, %lu\n",
                connector->device->path, connector_idx);
        return;
    }

    connector->gbm_surface = gbm_surface_create(connector->device->gbm_device,
                                                connector->mode->hdisplay,
                                                connector->mode->vdisplay,
                                                GBM_FORMAT_XRGB8888,
                                                GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!connector->gbm_surface) {
        fprintf(stderr, "Failed to create gbm surface for connector: %s, %lu",
                connector->device->path, connector_idx);
        return;
    }
    connector->egl_surface = eglCreateWindowSurface(connector->device->egl_display,
                                                    connector->device->egl_config,
                                                    (EGLNativeWindowType)connector->gbm_surface,
                                                    NULL);
	if (connector->egl_surface == EGL_NO_SURFACE) {
		fprintf(stderr, "Failed to create EGL Surface\n");
        goto error;
	}
    connector->valid = true;
    return;
 error:
    connector->valid = false;
    if (connector->egl_surface) eglDestroySurface(connector->device->egl_display, connector->egl_surface);
    if (connector->gbm_surface) gbm_surface_destroy(connector->gbm_surface);
    if (connector->crtc) drmModeFreeCrtc(connector->crtc);
    if (connector->encoder) drmModeFreeEncoder(connector->encoder);
    if (connector->connector) drmModeFreeConnector(connector->connector);
}

void drm_connector_deinit(struct drm_connector* connector) {
    if (connector->valid) {
        gbm_surface_destroy(connector->gbm_surface);
        drmModeFreeCrtc(connector->crtc);
        drmModeFreeEncoder(connector->encoder);
        drmModeFreeConnector(connector->connector);
    }
}

int drm_device_init(struct drm_device* device, char* path) {
    device->fd = open(path, O_RDWR | O_CLOEXEC);
    if(device->fd < 0) {
        fprintf(stderr, "Failed to open card: %s\n", path);
        return 0;
    }
    device->path = path;

    drmVersionPtr version = drmGetVersion(device->fd);
    if (version) {
        printf("DRM version: %d.%d.%d\n",
               version->version_major,
               version->version_minor,
               version->version_patchlevel);
        printf("DRM driver: %s\n", version->name);
        drmFreeVersion(version);
    }

    if (drmSetClientCap(device->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) != 0) {
        fprintf(stderr, "Failed to set universal planes cap\n");
        goto error;
    }

    device->resources = drmModeGetResources(device->fd);
    if (!device->resources) {
        fprintf(stderr, "Failed to get drm resources\n");
        goto error;
    }

    device->connectors = calloc(device->resources->count_connectors, sizeof(*device->connectors));
    if (!device->connectors) {
        fprintf(stderr, "Failed to allocate memory for connectors\n");
        goto error;
    }
    device->num_connectors = device->resources->count_connectors;

    device->gbm_device = gbm_create_device(device->fd);
    if (!device->gbm_device) {
        fprintf(stderr, "Failed to create gbm device: %s\n", device->path);
        goto error;
    }

    device->egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, device->gbm_device, NULL);
    if (device->egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to open egl display: %s\n", device->path);
        goto error;
    }

    EGLint egl_major, egl_minor;
    if (eglInitialize(device->egl_display, &egl_major, &egl_minor) != EGL_TRUE) {
        fprintf(stderr, "Failed to initalize EGL: %s\n", device->path);
        goto error;
    }
    fprintf(stderr, "EGL version %u.%u\n", egl_major, egl_minor);
	eglBindAPI(EGL_OPENGL_ES_API);

	EGLint num_configs;
	if (eglChooseConfig(device->egl_display, egl_attrs, &device->egl_config, 1, &num_configs) != EGL_TRUE) {
		fprintf(stderr, "Failed to choose EGL Config: %d, %s\n", eglGetError(), device->path);
        goto error;
	}
	device->egl_context = eglCreateContext(device->egl_display, device->egl_config,
                                           EGL_NO_CONTEXT, NULL);
	if (device->egl_context == EGL_NO_CONTEXT) {
		fprintf(stderr, "Failed to create EGL Context: %x, %s\n", eglGetError(), device->path);
        goto error;
	}

    for (size_t i = 0; i < device->num_connectors; i++) {
        device->connectors[i].device = device;
        drm_connector_init(&device->connectors[i], i);
    }
    device->initalized = 1;

    return 1;
 error:
    if (device->egl_context) eglDestroyContext(device->egl_display, device->egl_context);
    if (device->egl_display) eglTerminate(device->egl_display);
    if (device->gbm_device) gbm_device_destroy(device->gbm_device);
    if (device->resources) drmModeFreeResources(device->resources);
    if (device->path) free(path);
    if (device->fd >= 0) close(device->fd);
    device->num_connectors = 0;
    device->initalized = 0;
    return 0;
}

void drm_device_deinit(struct drm_device* device) {
    if (device->initalized) {
        free(device->path);
        for (size_t i = 0; i < device->num_connectors; i++) {
            drm_connector_deinit(&device->connectors[i]);
        }

        drmModeFreeResources(device->resources);
        gbm_device_destroy(device->gbm_device);
        close(device->fd);
    }
}

struct drm* drm_init() {
    struct drm* drm = calloc(1, sizeof(*drm));
    if (!drm) {
        fprintf(stderr, "Failed to allocate memory for drm\n");
        return NULL;
    }
    struct strings* gpus = udev_get_gpus();
    if (!gpus || !gpus->count)  {
        fprintf(stderr, "Failed to find any drm devices\n");
        goto error;
    }
    eglGetPlatformDisplayEXT =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!eglGetPlatformDisplayEXT) {
        fprintf(stderr, "eglGetPlatformDisplayEXT not supported sorry no wm\n");
        goto error;
    }
    drm->devices = calloc(gpus->count, sizeof(*(drm->devices)));
    if (!drm->devices) {
        fprintf(stderr, "Failed to allocate drm devices\n");
        goto error;
    }
    drm->num_devices = gpus->count;
    for (uint32_t i = 0; i < gpus->count; i++) {
        if (!drm_device_init(&drm->devices[i], gpus->items[i])) {
            fprintf(stderr, "Failed to initalize card: %s\n", gpus->items[i]);
        }
    }
    free(gpus);
    return drm;
 error:
    if (drm) free(drm);
    return NULL;
}

void drm_deinit(struct drm* drm) {
    for (uint32_t i = 0; i < drm->num_devices; i++) {
        drm_device_deinit(&drm->devices[i]);
    }
    free(drm->devices);
    free(drm);
}
