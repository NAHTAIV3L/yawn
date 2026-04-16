#ifndef EGL_H_
#define EGL_H_
#include <stdlib.h>
#include <stdio.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "gbm-inst.h"

struct egl {
    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;
};

struct egl* egl_init(struct gbm* gbm);
void egl_deinit(struct egl* egl);

#endif // EGL_H_
