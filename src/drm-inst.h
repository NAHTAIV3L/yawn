#ifndef DRM_INST_H_
#define DRM_INST_H_
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "types.h"

struct drm_connector {
    drmModeConnector* connector;
    drmModeEncoder* encoder;
};
DEF_ARR_TYPE(struct drm_connector, drm_connectors);

struct drm_crtc {
    int valid;
    struct drm_device* device;
    struct connector_arr* connectors;
    drmModeModeInfo* mode;
    drmModeCrtc* crtc;
    struct gbm_surface* gbm_surface;
    EGLSurface egl_surface;
};
DEF_ARR_TYPE(struct drm_crtc, drm_crtcs);

struct drm_device {
    int initalized;
    char* path;
    int fd;
    drmModeRes* resources;
    struct gbm_device* gbm_device;
    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;

    struct drm_crtcs* crtcs;
};
DEF_ARR_TYPE(struct drm_device, drm_devices);

struct drm {
    struct drm_devices* devices;
};

struct drm* drm_init();
void drm_deinit(struct drm* drm);

#endif // DRM_INST_H_
