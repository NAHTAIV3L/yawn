#include "egl.h"

PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = NULL;

struct egl* egl_init(struct gbm* gbm) {
    struct egl* egl = malloc(sizeof(struct egl));
    if (!egl) {
        fprintf(stderr, "Failed to allocate memory for egl");
        return NULL;
    }
    eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
        eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!eglGetPlatformDisplayEXT) {
        fprintf(stderr, "eglGetPlatformDisplayEXT not supported sorry no wm\n");
        goto error;
    }

    egl->display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbm->device, NULL);
    if (egl->display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to open egl display");
        goto error;
    }
	EGLint attrs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE
	};

    EGLint major, minor;
    if (eglInitialize(egl->display, &major, &minor) != EGL_TRUE) {
        fprintf(stderr, "Failed to initalize EGL\n");
        goto error;
    }
    fprintf(stderr, "EGL version %u.%u\n", major, minor);
	eglBindAPI(EGL_OPENGL_ES_API);

	EGLint num_configs;
	if (eglChooseConfig(egl->display, attrs, &egl->config, 1, &num_configs) != EGL_TRUE) {
		fprintf(stderr, "Failed to choose EGL Config: %d\n", eglGetError());
        goto error;
	}

    egl->surface = eglCreateWindowSurface(egl->display, egl->config,
                                          (EGLNativeWindowType)gbm->surface, NULL);
	if (egl->surface == EGL_NO_SURFACE) {
		fprintf(stderr, "Failed to create EGL Surface\n");
        goto error;
	}

	egl->context = eglCreateContext(egl->display, egl->config,
                                    EGL_NO_CONTEXT, NULL);
	if (egl->context == EGL_NO_CONTEXT) {
		fprintf(stderr, "Failed to create EGL Context: %x\n", eglGetError());
        goto error;
	}
    if (!eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context)) {
        fprintf(stderr, "Failed eglMakeCurrent\n");
        goto error;
    }
    return egl;
 error:
    if (egl->context) eglDestroyContext(egl->display, egl->context);
    if (egl->surface) eglDestroySurface(egl->display, egl->surface);
    if (egl->display) eglTerminate(egl->display);
    free(egl);
    return NULL;
}

void egl_deinit(struct egl* egl) {
    eglMakeCurrent(egl->display, NULL, NULL, egl->context);
    eglDestroyContext(egl->display, egl->context);
    eglDestroySurface(egl->display, egl->surface);
    eglTerminate(egl->display);
    free(egl);
}
