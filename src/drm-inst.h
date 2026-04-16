#ifndef DRM_INST_H_
#define DRM_INST_H_
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct drm {
    int fd;
    drmDevice* device;
    drmModeRes* resources;
    drmModeConnector* connector;
    drmModeModeInfo* mode;
    drmModeEncoder* encoder;
    drmModeCrtc* crtc;
};

struct drm* drm_init();
void drm_deinit(struct drm* drm);

#endif // DRM_INST_H_
