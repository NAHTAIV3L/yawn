#include "gbm-inst.h"

struct gbm* gbm_init(struct drm* drm) {
    struct gbm* gbm = calloc(1, sizeof(*gbm));
    if (!gbm) {
        fprintf(stderr, "Failed to allocate for gbm\n");
        return NULL;
    }

    gbm->device = gbm_create_device(drm->fd);
    if (!gbm->device) {
        fprintf(stderr, "Failed to create gbm device\n");
        goto error;
    }

    gbm->surface =
        gbm_surface_create(gbm->device,
                           drm->mode->hdisplay,
                           drm->mode->vdisplay,
                           GBM_FORMAT_XRGB8888,
                           GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm->surface) {
        fprintf(stderr, "Failed to open gbm surface");
        goto error;
    }
    return gbm;
 error:
    if (gbm->surface) gbm_surface_destroy(gbm->surface);
    if (gbm->device) gbm_device_destroy(gbm->device);
    if (gbm) free(gbm);
    return NULL;
}

void gbm_deinit(struct gbm* gbm) {
    gbm_surface_destroy(gbm->surface);
    gbm_device_destroy(gbm->device);
    free(gbm);
}
