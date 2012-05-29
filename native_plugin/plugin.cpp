/*
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <ui/FramebufferNativeWindow.h>
#include <ui/EGLUtils.h>

#define RETURN_IF_FALSE(x,r) do {bool b = bool(x); if (!b) return r;} while(0)

using namespace android;

struct PluginFuncs {
    int (*create)(int, int, int, char**, void**);
    int (*chooseEGLConfig)(void*, EGLDisplay, EGLConfig*);
    EGLContext (*createEGLContext)(void*, EGLDisplay, EGLConfig);
    int (*init)(void*);
    int (*render)(void*);
    int (*sizeChanged)(void*, int, int);
    void (*destroy)(void*);

    void reset()
    {
        create = NULL;
        chooseEGLConfig = NULL;
        createEGLContext = NULL;
        init = NULL;
        render = NULL;
        sizeChanged = NULL;
        destroy = NULL;
    }
};

static bool loadPlugin(char *lib, PluginFuncs& funcs)
{
    char path[PATH_MAX];
    const char *error;
    void *handle;

    if (!realpath(lib, path)) {
        printf("Can't resolve absolute path of %s", lib);
        return false;
    }

    printf("Loading plugin %s\n", path);
    handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        printf("Can't dlopen %s: %s", path, dlerror());
        return false;
    }

    // Need create and render, the rest are optional
    dlerror();
    funcs.create = (int (*)(int, int, int, char**, void**))dlsym(handle, "create");
    if ((error = dlerror()) != NULL || funcs.create == NULL)  {
        error = error ? error : "not set: ";
        printf("%s %screate", path, error);
        return false;
    }

    dlerror();
    funcs.render = (int (*)(void*))dlsym(handle, "render");
    if ((error = dlerror()) != NULL || funcs.render == NULL)  {
        error = error ? error : "not set: ";
        printf("%s %srender", path, error);
        return false;
    }

    dlerror();
    funcs.init = (int (*)(void*))dlsym(handle, "init");
    if ((error = dlerror()) != NULL || funcs.init == NULL)  {
        error = error ? error : "not set: ";
        printf("%s %sinit", path, error);
        funcs.init = NULL;
    }

    dlerror();
    funcs.destroy = (void (*)(void*))dlsym(handle, "destroy");
    if ((error = dlerror()) != NULL || funcs.destroy == NULL)  {
        error = error ? error : "not set: ";
        printf("%s %sdestroy", path, error);
        funcs.destroy = NULL;
    }

    dlerror();
    funcs.chooseEGLConfig = (int (*)(void*, EGLDisplay, EGLConfig*))dlsym(handle, "chooseEGLConfig");
    if ((error = dlerror()) != NULL || funcs.chooseEGLConfig == NULL)  {
        error = error ? error : "not set: ";
        printf("%s %schooseEGLConfig", path, error);
        funcs.chooseEGLConfig = NULL;
    }

    dlerror();
    funcs.createEGLContext = (EGLContext (*)(void*, EGLDisplay, EGLConfig))dlsym(handle, "createEGLContext");
    if ((error = dlerror()) != NULL || funcs.createEGLContext == NULL)  {
        error = error ? error : "not set: ";
        printf("%s %screateEGLContext", path, error);
        funcs.createEGLContext = NULL;
    }

    dlerror();
    funcs.sizeChanged = (int (*)(void*, int, int))dlsym(handle, "sizeChanged");
    if ((error = dlerror()) != NULL || funcs.sizeChanged == NULL)  {
        error = error ? error : "not set: ";
        printf("%s %ssizeChanged", path, error);
        funcs.sizeChanged = NULL;
    }

    return true;
}

static bool checkEglError(const char* op, EGLBoolean returnVal = EGL_TRUE) {
    bool ret = true;
    if (returnVal != EGL_TRUE) {
        printf("%s() returned %d\n", op, returnVal);
        ret = false;
    }

    for (EGLint e = eglGetError(); e != EGL_SUCCESS; e = eglGetError()) {
        printf("after %s() eglError %s (0x%x)\n", op, EGLUtils::strerror(e), e);
        ret = false;
    }

    return ret;
}

void printEGLConfiguration(EGLDisplay dpy, EGLConfig config) {

#define X(VAL) {VAL, #VAL}
    struct {EGLint attribute; const char* name;} names[] = {
    X(EGL_BUFFER_SIZE),
    X(EGL_ALPHA_SIZE),
    X(EGL_BLUE_SIZE),
    X(EGL_GREEN_SIZE),
    X(EGL_RED_SIZE),
    X(EGL_DEPTH_SIZE),
    X(EGL_STENCIL_SIZE),
    X(EGL_CONFIG_CAVEAT),
    X(EGL_CONFIG_ID),
    X(EGL_LEVEL),
    X(EGL_MAX_PBUFFER_HEIGHT),
    X(EGL_MAX_PBUFFER_PIXELS),
    X(EGL_MAX_PBUFFER_WIDTH),
    X(EGL_NATIVE_RENDERABLE),
    X(EGL_NATIVE_VISUAL_ID),
    X(EGL_NATIVE_VISUAL_TYPE),
    X(EGL_SAMPLES),
    X(EGL_SAMPLE_BUFFERS),
    X(EGL_SURFACE_TYPE),
    X(EGL_TRANSPARENT_TYPE),
    X(EGL_TRANSPARENT_RED_VALUE),
    X(EGL_TRANSPARENT_GREEN_VALUE),
    X(EGL_TRANSPARENT_BLUE_VALUE),
    X(EGL_BIND_TO_TEXTURE_RGB),
    X(EGL_BIND_TO_TEXTURE_RGBA),
    X(EGL_MIN_SWAP_INTERVAL),
    X(EGL_MAX_SWAP_INTERVAL),
    X(EGL_LUMINANCE_SIZE),
    X(EGL_ALPHA_MASK_SIZE),
    X(EGL_COLOR_BUFFER_TYPE),
    X(EGL_RENDERABLE_TYPE),
    X(EGL_CONFORMANT),
   };
#undef X

    for (size_t j = 0; j < sizeof(names) / sizeof(names[0]); j++) {
        EGLint value = -1;
        EGLint returnVal = eglGetConfigAttrib(dpy, config, names[j].attribute, &value);
        EGLint error = eglGetError();
        if (returnVal && error == EGL_SUCCESS) {
            printf(" %s: ", names[j].name);
            printf("%d (0x%x)", value, value);
        }
    }
    printf("\n");
}

static bool getEglConfig(EGLDisplay dpy, EGLNativeWindowType window,
        PluginFuncs *funcs, void *instance, EGLConfig *config)
{
    int status;
    EGLBoolean ret;
    *config = NULL;

    if (funcs != NULL && funcs->chooseEGLConfig != NULL) {
        status = funcs->chooseEGLConfig(instance, dpy, config);
        RETURN_IF_FALSE(checkEglError("chooseEGLConfig") && status == 0, false);
        if (funcs->chooseEGLConfig != NULL && *config == NULL) {
            printf("Plugin chooseEGLConfig returned NULL, using default\n");
        } else if (*config != NULL) {
            // Don't trust the plugin completely, we're more constrained when
            // using native windows than SurfaceFlinger surfaces
            int format;
            if (window->query(window, NATIVE_WINDOW_FORMAT, &format) < 0) {
                printf("window->query NATIVE_WINDOW_FORMAT failed\n");
                return false;
            }
            // Select something that's "close enough", ensuring correct EGL_NATIVE_VISUAL_ID
            EGLint r, g, b, a, d, nvid, rt, num;

            ret = eglGetConfigAttrib(dpy, *config, EGL_NATIVE_VISUAL_ID, &nvid);
            RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_NATIVE_VISUAL_ID", ret), false);
            if (nvid != format) {
                printf("plugin EGL_NATIVE_VISUAL_ID %04x, need %04x\n", nvid, format);

                ret = eglGetConfigAttrib(dpy, *config, EGL_RENDERABLE_TYPE, &rt);
                RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_RENDERABLE_TYPE", ret), false);

                ret = eglGetConfigAttrib(dpy, *config, EGL_RED_SIZE, &r);
                RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_RED_SIZE", ret), false);

                ret = eglGetConfigAttrib(dpy, *config, EGL_RED_SIZE, &g);
                RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_GREEN_SIZE", ret), false);

                ret = eglGetConfigAttrib(dpy, *config, EGL_RED_SIZE, &b);
                RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_BLUE_SIZE", ret), false);

                ret = eglGetConfigAttrib(dpy, *config, EGL_ALPHA_SIZE, &a);
                RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_ALPHA_SIZE", ret), false);

                ret = eglGetConfigAttrib(dpy, *config, EGL_DEPTH_SIZE, &d);
                RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_DEPTH_SIZE", ret), false);

                EGLint attrs[] = {
                    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                    EGL_NATIVE_VISUAL_ID, format,
                    EGL_RED_SIZE, r,
                    EGL_GREEN_SIZE, g,
                    EGL_BLUE_SIZE, b,
                    EGL_DEPTH_SIZE, d,
                    EGL_RENDERABLE_TYPE, rt,
                    EGL_NONE };

                ret = eglChooseConfig(dpy, attrs, 0, 0, &num);
                RETURN_IF_FALSE(checkEglError("eglGetConfigs", ret), false);
                printf("%d potentially compatible configs\n", num);

                EGLConfig* const configs = (EGLConfig*)malloc(sizeof(EGLConfig) * num);
                if (eglChooseConfig(dpy, attrs, configs, num, &num) == EGL_FALSE) {
                    free(configs);
                    return false;
                }

                EGLConfig alpha = NULL, noalpha = NULL;
                for (int i = 0; i < num ; i++) {
                    EGLint val;
                    ret = eglGetConfigAttrib(dpy, configs[i], EGL_NATIVE_VISUAL_ID, &val);
                    RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_NATIVE_VISUAL_ID", ret), false);
                    if (val != format) {
                        printf("skipping config %p, wrong EGL_NATIVE_VISUAL_ID\n", configs[i]);
                        continue;
                    }
                    ret = eglGetConfigAttrib(dpy, configs[i], EGL_ALPHA_SIZE, &val);
                    RETURN_IF_FALSE(checkEglError("eglGetConfigAttrib EGL_ALPHA_SIZE", ret), false);
                    if (val > 0 && alpha == NULL)
                        alpha = configs[i];
                    if (val == 0 && noalpha == NULL)
                        noalpha = configs[i];
                }
                free(configs);
                printf("alpha %p, noalpha %p\n", alpha, noalpha);
                if (alpha == NULL && noalpha == NULL) {
                    // This is bad news
                    printf("no matching config found\n");
                    *config = NULL;
                } else {
                    // Mimic plugin selection if we have a choice
                    *config = a > 0 ? alpha : noalpha;

                    // No match means we take what we can (one is not NULL)
                    if (*config == NULL)
                        *config = a > 0 ? noalpha : alpha;
                }
            }
        }
    }

    if (*config == NULL) {
        printf("Getting default EGLConfig\n");
        EGLint configAttribs[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };
        ret = !EGLUtils::selectConfigForNativeWindow(dpy, configAttribs, window, config);
        RETURN_IF_FALSE(checkEglError("selectConfigForNativeWindow", ret), false);
    }

    if (*config == NULL) {
        printf("Failed to get EGLConfig\n");
        return false;
    }

    printf("getEglConfig %p\n", *config);
    printEGLConfiguration(dpy, *config);
    return true;
}

static bool createEglSurface(EGLDisplay dpy, EGLNativeWindowType window,
        EGLConfig config, EGLSurface *surface, EGLint *w, EGLint *h)
{
    EGLBoolean ret;

    *surface = eglCreateWindowSurface(dpy, config, window, NULL);
    RETURN_IF_FALSE(checkEglError("eglCreateWindowSurface") || *surface != EGL_NO_SURFACE, false);

    eglQuerySurface(dpy, *surface, EGL_WIDTH, w);
    RETURN_IF_FALSE(checkEglError("eglQuerySurface"), false);
    eglQuerySurface(dpy, *surface, EGL_HEIGHT, h);
    RETURN_IF_FALSE(checkEglError("eglQuerySurface"), false);

    printf("EGLSurface created: %p %d x %d\n", *surface, *w, *h);
    return true;
}

static bool createEglContext(EGLDisplay dpy, PluginFuncs *funcs, void *instance,
        EGLConfig config, EGLSurface surface, EGLContext *context)
{
    *context = EGL_NO_CONTEXT;
    if (funcs != NULL && funcs->createEGLContext != NULL) {
        *context = funcs->createEGLContext(instance, dpy, config);
        RETURN_IF_FALSE(checkEglError("createEGLContext"), false);
        if ((*context == NULL || *context == EGL_NO_CONTEXT) &&
                funcs->createEGLContext != NULL) {
            *context = EGL_NO_CONTEXT;
            printf("Plugin didn't create a context, using default\n");
        }
    }

    if ((*context == NULL || *context == EGL_NO_CONTEXT)) {
        printf("getting default EGLContext\n");
        *context = eglCreateContext(dpy, config, EGL_NO_CONTEXT, NULL);
        RETURN_IF_FALSE(checkEglError("eglCreateContext") || context != EGL_NO_CONTEXT, false);
    }

    if ((*context == NULL || *context == EGL_NO_CONTEXT)) {
        printf("failed to create EGLContext\n");
        return false;
    }

    EGLBoolean ret = eglMakeCurrent(dpy, surface, surface, *context);
    RETURN_IF_FALSE(checkEglError("eglMakeCurrent", ret), -1);

    printf("EGLContext created: %p\n", *context);
    return true;
}

static bool destroyEglContextSurface(EGLDisplay dpy, EGLContext *context,
        EGLSurface *surface)
{
    EGLBoolean ret = eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    RETURN_IF_FALSE(checkEglError("eglMakeCurrent", ret), false);
    printf("made nothing current\n");

    if (*context != EGL_NO_CONTEXT) {
        EGLContext old = *context;
        ret = eglDestroyContext(dpy, *context);
        *context = EGL_NO_CONTEXT;
        RETURN_IF_FALSE(checkEglError("eglDestroyContext", ret), false);
        printf("EGLContext destroyed: %p\n", old);
    }

    if (*surface != EGL_NO_SURFACE) {
        EGLSurface old = *surface;
        EGLBoolean ret = eglDestroySurface(dpy, *surface);
        RETURN_IF_FALSE(checkEglError("eglDestroySurface", ret), false);
        *surface = EGL_NO_SURFACE;
        printf("EGLSurface destroyed: %p\n", old);
    }

    return true;
}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Usage: %s libplugin.so <plugin args>\n", argv[0]);
        return 0;
    }

    void *instance = NULL;
    PluginFuncs funcs;
    int status;
    bool swap;
    EGLBoolean ret;
    EGLDisplay dpy = EGL_NO_DISPLAY;
    sp<FramebufferNativeWindow> window;
    EGLint major = -1, minor = -1, w = -1, h = -1;
    EGLConfig config = {0};
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    // Load plugin, no need to go on if that fails
    funcs.reset();
    RETURN_IF_FALSE(loadPlugin(argv[1], funcs), -1);

    // Init EGL
    RETURN_IF_FALSE(checkEglError("<init>"), -1);
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    RETURN_IF_FALSE(checkEglError("eglGetDisplay") || dpy != EGL_NO_DISPLAY, -1);

    ret = eglInitialize(dpy, &major, &minor);
    RETURN_IF_FALSE(checkEglError("eglInitialize", ret), -1);
    printf("EGL version %d.%d initialized\n", major, minor);

    window = new FramebufferNativeWindow();
    if (window->getDevice() == NULL) {
        printf("no framebuffer\n");
        return -1;
    }

    // XXX: Extremely hacky workaround!
    // FramebufferNativeWindow doesn't initialize the cancelBuffer ptr and
    // a certain EGL implementation calls it if it's nonzero. As it's not
    // implemented in FramebufferNativeWindow, we set it to zero here.
    // Google have changed this in a future release.
    window.get()->cancelBuffer = 0;

    // Create a surface with the default config since we need w & h for init
    RETURN_IF_FALSE(getEglConfig(dpy, window.get(), NULL, instance, &config), -1);
    RETURN_IF_FALSE(createEglSurface(dpy, window.get(), config, &surface, &w, &h), -1);
    RETURN_IF_FALSE(createEglContext(dpy, NULL, instance, config, surface, &context), -1);

    // Initialize plugin
    if ((status = funcs.create(w, h, argc - 1, argv + 1, &instance)) != 0) {
        printf("Plugin create failed, %d", ret);
        return -1;
    }

    // If plugin implements chooseEGLConfig we must recreate the surface
    if (funcs.chooseEGLConfig) {
        RETURN_IF_FALSE(destroyEglContextSurface(dpy, &context, &surface), -1);
        RETURN_IF_FALSE(getEglConfig(dpy, window.get(), &funcs, instance, &config), -1);
        RETURN_IF_FALSE(createEglSurface(dpy, window.get(), config, &surface, &w, &h), -1);
        RETURN_IF_FALSE(createEglContext(dpy, &funcs, instance, config, surface, &context), -1);
    }

    if (funcs.init) {
        printf("Calling plugin init\n");
        if ((status = funcs.init(instance)) != 0) {
            printf("Plugin init failed: %d\n", status);
            return -1;
        }
    }

    printf("Starting render loop\n");

    for (;;) {
        swap = true;
        status = funcs.render(instance);

        if (status < 0) {
            printf("Plugin render error %d\n", status);
            break;
        }

        if (status > 0) {
            if (status == 1) {
                // reinit
                RETURN_IF_FALSE(destroyEglContextSurface(dpy, &context, &surface), -1);

                RETURN_IF_FALSE(getEglConfig(dpy, window.get(), &funcs, instance, &config), -1);
                RETURN_IF_FALSE(createEglSurface(dpy, window.get(), config, &surface, &w, &h), -1);
                RETURN_IF_FALSE(createEglContext(dpy, &funcs, instance, config, surface, &context), -1);
                swap = false;
            } else if (status == 2) {
                printf("plugin done\n");
                break;
            } else if (status == 3) {
                // Everything OK, but no swap needed
                swap = false;
            } else {
                printf("plugin render returned unknown status: %d\n", status);
                break;
            }
        }

        if (swap) {
            eglSwapBuffers(dpy, surface);
            checkEglError("eglSwapBuffers");
        }
    }

    return 0;
}
