#ifndef GBM_H_
#define GBM_H_
#include <gbm.h>
#include "drm-inst.h"

struct gbm {
    struct gbm_device* device;
    struct gbm_surface* surface;
};

struct gbm* gbm_init(struct drm* drm);
void gbm_deinit(struct gbm* gbm);

#endif // GBM_H_
