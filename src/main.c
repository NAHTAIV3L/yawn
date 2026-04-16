#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libseat.h>
#include <gbm.h>
#include <drm_fourcc.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "drm-inst.h"
#include "gbm-inst.h"
#include "egl.h"

void fb_destroy_callback(struct gbm_bo* bo, void* data) {
	int fd = gbm_device_get_fd(gbm_bo_get_device(bo));
	uint32_t *fb_id = data;

    if (fb_id) {
        drmModeRmFB(fd, *fb_id);
    }
	free(fb_id);
}

uint32_t get_fb_id_from_bo(int fd, struct gbm_bo* bo) {
    uint32_t width, height, format,
        strides[4] = {0}, handles[4] = {0},
        offsets[4] = {0}, flags = 0;
    uint32_t* fb_id = gbm_bo_get_user_data(bo);
    if (fb_id) return *fb_id;

    fb_id = calloc(1, sizeof(*fb_id));
    if (!fb_id) {
        fprintf(stderr, "Failed to allocate fd_id\n");
    }

	int ret = -1;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);
	format = gbm_bo_get_format(bo);

    uint64_t modifiers[4] = {0};
    modifiers[0] = gbm_bo_get_modifier(bo);
    int num_planes = gbm_bo_get_plane_count(bo);
    for (int i = 0; i < num_planes; i++) {
        handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
        strides[i] = gbm_bo_get_stride_for_plane(bo, i);
        offsets[i] = gbm_bo_get_offset(bo, i);
        modifiers[i] = modifiers[0];
    }
    if (modifiers[0] && modifiers[0] != DRM_FORMAT_MOD_INVALID) {
        flags = DRM_MODE_FB_MODIFIERS;
        printf("Using modifier %" PRIx64 "\n", modifiers[0]);
    }

    ret = drmModeAddFB2WithModifiers(fd, width, height,
                                     format, handles, strides, offsets,
                                     modifiers, fb_id, flags);

    if (ret) {
        if (flags)
            fprintf(stderr, "Modifiers failed!\n");

		memcpy(handles, (uint32_t [4]){gbm_bo_get_handle(bo).u32,0,0,0}, 16);
		memcpy(strides, (uint32_t [4]){gbm_bo_get_stride(bo),0,0,0}, 16);
		memset(offsets, 0, 16);
		ret = drmModeAddFB2(fd, width, height, format,
                            handles, strides, offsets, fb_id, 0);
	}

	if (ret) {
		uint32_t handle = gbm_bo_get_handle(bo).u32;
		uint32_t bpp = gbm_bo_get_bpp(bo);
		uint8_t depth = bpp;
		uint32_t pitch = gbm_bo_get_stride(bo);

		ret = drmModeAddFB(fd, width, height, depth, bpp, pitch,
				handle, fb_id);

	}

	if (ret) {
		printf("failed to create fb: %s\n", strerror(errno));
		free(fb_id);
		return 0;
	}

	gbm_bo_set_user_data(bo, fb_id, fb_destroy_callback);
    return *fb_id;
}

void page_flip_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data) {
	int *waiting_for_flip = data;
	*waiting_for_flip = 0;
}

int main() {
    struct drm* drm = drm_init();
    if (!drm) {
        return 1;
    }

    struct gbm* gbm = gbm_init(drm);
    if (!gbm) {
        return 1;
    }

    struct egl* egl = egl_init(gbm);
    if (!egl) {
        return 1;
    }

	drmEventContext evctx = {
			.version = 2,
			.page_flip_handler = page_flip_handler,
	};
	fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(drm->fd, &fds);

    struct gbm_bo* bo = NULL;
    eglSwapBuffers(egl->display, egl->surface);
    bo = gbm_surface_lock_front_buffer(gbm->surface);
    uint32_t fb_id = get_fb_id_from_bo(drm->fd, bo);
    if (!fb_id) {
        fprintf(stderr, "Failed to get framebuffer\n");
        return 1;
    }
    int ret;
	ret = drmModeSetCrtc(drm->fd, drm->crtc->crtc_id, fb_id, 0, 0,
			&drm->connector->connector_id, 1, drm->mode);
	if (ret) {
		fprintf(stderr, "Failed to set mode: %s\n", strerror(errno));
		return ret;
	}

    for (int i = 0; i < 300; i++) {

        glClearColor(0.0, 1.0, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        eglSwapBuffers(egl->display, egl->surface);

        struct gbm_bo* next_bo = gbm_surface_lock_front_buffer(gbm->surface);
        fb_id = get_fb_id_from_bo(drm->fd, next_bo);
        if (!fb_id) {
            fprintf(stderr, "Failed to get next framebuffer\n");
            return 1;
        }

        int waiting_for_flip = 1;
		ret = drmModePageFlip(drm->fd, drm->crtc->crtc_id, fb_id,
				DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
        while (waiting_for_flip) {
			ret = select(drm->fd + 1, &fds, NULL, NULL, NULL);
			if (ret < 0) {
				printf("select err: %s\n", strerror(errno));
				return ret;
			} else if (FD_ISSET(0, &fds)) {
				printf("user interrupted!\n");
				return 1;
			}
			drmHandleEvent(drm->fd, &evctx);
        }

        gbm_surface_release_buffer(gbm->surface, bo);
    }

    egl_deinit(egl);
    gbm_deinit(gbm);
    drm_deinit(drm);
    return 0;
}
