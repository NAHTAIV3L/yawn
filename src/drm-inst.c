#include "drm-inst.h"

drmModeConnector* drm_get_connector(struct drm* drm) {
    drmModeConnector* connector = NULL;
    for (int i = 0; i < drm->resources->count_connectors; i++) {
        connector = drmModeGetConnectorCurrent(drm->fd, drm->resources->connectors[i]);
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

struct drm* drm_init() {
    struct drm* drm = calloc(1, sizeof(*drm));
    drm->fd = -1;
    if (!drm) {
        fprintf(stderr, "Failed to allocate memory for drm\n");
        return NULL;
    }
    {
        drmDevice* devices[64] = { NULL };
        int num_devices = drmGetDevices2(0, devices, sizeof(devices) / sizeof(devices[0]));
        int used_device = -1;
        printf("number of devices: %d\n", num_devices);
        for (int i = 0; i < num_devices; i++) {
            if (!(devices[i]->available_nodes & 1 << DRM_NODE_PRIMARY)) continue;
            drm->fd = open(devices[i]->nodes[DRM_NODE_PRIMARY], O_RDWR | O_CLOEXEC);
            if(drm->fd < 0)
                continue;

            used_device = i;
            break;
        }

        for (int i = 0; i < num_devices; i++) {
            if (i == used_device) continue;
            drmFreeDevice(&(devices[i]));
        }
        drm->device = devices[used_device];
    }
    if (!drm->device) {
        fprintf(stderr, "Failed to find any drm devices\n");
        goto error;
    }

    drmVersionPtr version = drmGetVersion(drm->fd);
    if (version) {
        printf("DRM version: %d.%d.%d\n",
               version->version_major,
               version->version_minor,
               version->version_patchlevel);
        printf("DRM driver: %s\n", version->name);
        drmFreeVersion(version);
    }

    if (drmSetClientCap(drm->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) != 0) {
        fprintf(stderr, "Failed to set universal planes cap\n");
        goto error;
    }

    drm->resources = drmModeGetResources(drm->fd);
    if (!drm->resources) {
        fprintf(stderr, "Failed to get drm resources\n");
        goto error;
    }

    drm->connector = drm_get_connector(drm);
    if (!drm->connector) {
        fprintf(stderr, "Failed to get connector\n");
        goto error;
    }

    drm->mode = drm_connector_get_mode(drm->connector);
    if (!drm->mode) {
        fprintf(stderr, "Failed to get mode for connector\n");
        goto error;
    }

    drmModeEncoder* encoder = drmModeGetEncoder(drm->fd, drm->connector->encoder_id);
    if (!encoder) {
        fprintf(stderr, "Failed to find encoder for connector\n");
        goto error;
    }

    printf("connector type: %s-%u\n",
           drmModeGetConnectorTypeName(drm->connector->connector_type),
           drm->connector->connector_type_id);

    drm->encoder = drmModeGetEncoder(drm->fd, drm->connector->encoder_id);
    if (!drm->encoder) {
        fprintf(stderr, "Failed to find encoder for connector\n");
        goto error;
    }

    drm->crtc = drmModeGetCrtc(drm->fd, drm->encoder->crtc_id);
    if (!drm->crtc) {
        fprintf(stderr, "Failed to get ctrc\n");
        goto error;
    }

    return drm;
 error:
    if (drm->crtc) drmModeFreeCrtc(drm->crtc);
    if (drm->encoder) drmModeFreeEncoder(drm->encoder);
    if (drm->connector) drmModeFreeConnector(drm->connector);
    if (drm->resources) drmModeFreeResources(drm->resources);
    if (drm->device) drmFreeDevice(&drm->device);
    if (drm->fd < 0) close(drm->fd);
    if (drm) free(drm);
    return NULL;
}

void drm_deinit(struct drm* drm) {
    drmModeFreeCrtc(drm->crtc);
    drmModeFreeEncoder(drm->encoder);
    drmModeFreeConnector(drm->connector);
    drmModeFreeResources(drm->resources);
    drmFreeDevice(&drm->device);
    close(drm->fd);
}
