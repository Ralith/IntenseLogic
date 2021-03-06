#include "context.h"

#include "util/log.h"
#include "util/assert.h"
#include "util/timer.h"
#include "graphics/glutil.h"
#include "common/event.h"
#include "graphics/stage.h"
#include "graphics/graphics.h"

static GLvoid error_cb(GLenum source, GLenum type, GLuint id, GLenum severity,
                       GLsizei length, const GLchar* message, GLvoid* user);
void ilG_registerInputBackend(ilG_context *ctx);

void context_cons(void *obj)
{
    ilG_context *self = obj;
    if (!ilG_context_type.registry) {
        ilG_context_type.registry = ilE_registry_new();
        ilE_registry_forward(ilG_context_type.registry, il_registry);
    }
    self->base.registry = ilE_registry_new();
    ilE_registry_forward(self->base.registry, ilG_context_type.registry);
    self->contextMajor = 3;
#ifdef __APPLE__
    self->contextMinor = 2;
    self->forwardCompat = 1;
#else
    self->contextMinor = 1;
#endif
    self->profile = ILG_CONTEXT_NONE;
#ifdef DEBUG
    self->debugContext = 1;
#endif
    self->experimental = 1;
    self->startWidth = 800;
    self->startHeight = 600;
    self->initialTitle = "IntenseLogic";
}

static void context_des(void *obj)
{
    ilG_context *self = obj;
    size_t i;

    self->complete = 0; // hopefully prevents use after free
    free(self->texunits);
    IL_FREE(self->positionables);
    for (i = 0; i < self->lights.length; i++) {
        il_unref(self->lights.data[i]);
    }
    IL_FREE(self->lights);
    for (i = 0; i < self->stages.length; i++) {
        il_unref(self->stages.data[i]);
    }
    IL_FREE(self->stages);
    glDeleteFramebuffers(1, &self->framebuffer);
    glDeleteTextures(5, &self->fbtextures[0]);
}

il_type ilG_context_type = {
    .typeclasses = NULL,
    .storage = NULL,
    .constructor = context_cons,
    .destructor = context_des,
    .copy = NULL,
    .name = "il.graphics.context",
    .size = sizeof(ilG_context),
    .registry = NULL,
    .parent = NULL
};

void ilG_context_hint(ilG_context *self, enum ilG_context_hint hint, int param)
{
#define HINT(v, f) case v: self->f = param; break;
    switch (hint) {
        HINT(ILG_CONTEXT_MAJOR, contextMajor)
        HINT(ILG_CONTEXT_MINOR, contextMinor)
        HINT(ILG_CONTEXT_FORWARD_COMPAT, forwardCompat)
        HINT(ILG_CONTEXT_PROFILE, profile)
        HINT(ILG_CONTEXT_DEBUG, debugContext)
        HINT(ILG_CONTEXT_EXPERIMENTAL, experimental)
        HINT(ILG_CONTEXT_WIDTH, startWidth)
        HINT(ILG_CONTEXT_HEIGHT, startHeight)
        HINT(ILG_CONTEXT_HDR, hdr)
        default:
        il_error("Invalid hint");
    }
}

int ilG_context_build(ilG_context *self)
{
    if (self->complete) {
        il_error("Context already complete");
        return 0;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, self->contextMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, self->contextMinor);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, self->forwardCompat? GL_TRUE : GL_FALSE);
    switch (self->profile) {
        case ILG_CONTEXT_NONE:
        break;
        case ILG_CONTEXT_CORE:
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        break;
        case ILG_CONTEXT_COMPAT:
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        break;
        default:
        il_error("Invalid profile");
        return 0;
    }
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, self->debugContext? GL_TRUE : GL_FALSE);
    if (!(self->window = glfwCreateWindow(self->startWidth, self->startHeight, self->initialTitle, NULL, NULL))) { // TODO: allow context sharing + monitor specification
        il_error("glfwOpenWindow() failed - are you sure you have OpenGL 3.1?");
        return 0;
    }
    ilG_registerInputBackend(self);
    glfwSetWindowUserPointer(self->window, self);
    ilG_context_makeCurrent(self);
    glfwSwapInterval(0);
    glewExperimental = self->experimental? GL_TRUE : GL_FALSE; // TODO: find out why IL crashes without this
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        il_fatal("glewInit() failed: %s", glewGetErrorString(err));
    }
    il_log("Using GLEW %s", glewGetString(GLEW_VERSION));

#ifndef __APPLE__
    if (!GLEW_VERSION_3_1) {
        il_error("GL version 3.1 is required, you have %s: crashes are on you", glGetString(GL_VERSION));
    } else {
        il_log("OpenGL Version %s", glGetString(GL_VERSION));
    }
#endif

    IL_GRAPHICS_TESTERROR("Unknown");
    if (self->debugContext && GLEW_ARB_debug_output) {
        glDebugMessageCallbackARB((GLDEBUGPROCARB)&error_cb, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        il_log("ARB_debug_output present, enabling advanced errors");
        IL_GRAPHICS_TESTERROR("glDebugMessageCallbackARB()");
    } else {
        il_log("ARB_debug_output missing");
    }
    GLint num_texunits;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &num_texunits);
    ilG_testError("glGetIntegerv");
    self->texunits = calloc(sizeof(unsigned), num_texunits);
    self->num_texunits = num_texunits;
    glGenFramebuffers(1, &self->framebuffer);
    glGenTextures(5, &self->fbtextures[0]);
    ilG_testError("Unable to generate framebuffer");
    self->complete = 1;
    return 1;
}

int ilG_context_resize(ilG_context *self, int w, int h, const char *title)
{
    if (!self->complete) {
        il_error("Resizing incomplete context");
        return 0;
    }

    // GL setup
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    IL_GRAPHICS_TESTERROR("Error setting up screen");

    self->width = w;
    self->height = h;
    if (self->title) {
        free(self->title);
    }
    self->title = strdup(title);
    glBindFramebuffer(GL_FRAMEBUFFER, self->framebuffer);
    glBindTexture(GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_DEPTH]);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_DEPTH], 0);
    ilG_testError("Unable to create depth buffer");
    glBindTexture(GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_ACCUM]);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, w, h, 0, GL_RGBA, self->hdr? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_ACCUM], 0);
    ilG_testError("Unable to create accumulation buffer");
    glBindTexture(GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_NORMAL]);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_NORMAL], 0);
    ilG_testError("Unable to create normal buffer");
    glBindTexture(GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_DIFFUSE]);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_DIFFUSE], 0);
    ilG_testError("Unable to create diffuse buffer");
    glBindTexture(GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_SPECULAR]);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_RECTANGLE, self->fbtextures[ILG_CONTEXT_SPECULAR], 0);
    ilG_testError("Unable to create specular buffer");
    // completeness testing
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char *status_str;
        switch(status) {
            case GL_FRAMEBUFFER_UNDEFINED:                      status_str = "GL_FRAMEBUFFER_UNDEFINED";                        break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          status_str = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";            break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  status_str = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";    break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         status_str = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";           break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         status_str = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";           break;
            case GL_FRAMEBUFFER_UNSUPPORTED:                    status_str = "GL_FRAMEBUFFER_UNSUPPORTED";                      break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         status_str = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";           break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:       status_str = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";         break;
            default:                                            status_str = "???";                                             break;
        }
        il_error("Unable to create framebuffer for context: %s", status_str);
        return 0;
    }
    self->valid = 1;
    return 1;
}

void ilG_context_makeCurrent(ilG_context *self)
{
    glfwMakeContextCurrent(self->window);
}

void ilG_context_addStage(ilG_context* self, ilG_stage* stage, int num)
{
    il_return_on_fail(stage);
    stage = il_ref(stage);
    if (num < 0) {
        IL_APPEND(self->stages, stage);
        return;
    }
    IL_INSERT(self->stages, (size_t)num, stage);
}

void ilG_context_clearStages(ilG_context *self)
{
    self->stages.length = 0;
}

void render_stages(const ilE_registry* registry, const char *name, size_t size, const void *data, void * ctx)
{
    (void)registry, (void)name, (void)size, (void)data;
    ilG_context *context = ctx;
    size_t i;
    int width, height;
    struct timeval time, tv;
    struct ilG_frame *iter, *temp, *frame, *last;

    if (!context->complete || !context->valid) {
        il_error("Rendering invalid context");
        return;
    }

    ilG_context_makeCurrent(context);
    glfwGetFramebufferSize(context->window, &width, &height);
    if (width != context->width || height != context->height) {
        ilG_context_resize(context, width, height, context->title);
    }
    glViewport(0, 0, width, height);
    il_debug("Begin render");
    static const GLenum drawbufs[] = {
        GL_COLOR_ATTACHMENT0,   // accumulation
        GL_COLOR_ATTACHMENT1,   // normal
        GL_COLOR_ATTACHMENT2,   // diffuse
        GL_COLOR_ATTACHMENT3    // specular
    };
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, context->framebuffer);
    glDrawBuffers(4, &drawbufs[0]);
    glClearColor(0.39, 0.58, 0.93, 1.0); // cornflower blue
    ilG_testError("glClearColor");
    glClearDepth(1.0);
    ilG_testError("glClearDepth");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (i = 0; i < context->stages.length; i++) {
        il_debug("Rendering stage %s", context->stages.data[i]->name);
        context->stages.data[i]->run(context->stages.data[i]);
    }
    gettimeofday(&time, NULL);
    last = IL_LIST_TAIL(context->frames_head, ll);
    IL_LIST_ITER(context->frames_head, ll, iter, temp) {
        timersub(&time, &iter->start, &tv);
        if (tv.tv_sec > 0) {
            IL_LIST_POPHEAD(context->frames_head, ll, frame);
            timersub(&context->frames_sum, &frame->elapsed, &context->frames_sum);
            context->num_frames--;
            free(frame);
        }
    }
    frame = calloc(1, sizeof(struct ilG_frame));
    frame->start = time;
    if (last) {
        timersub(&time, &last->start, &frame->elapsed);
    } else {
        frame->elapsed = (struct timeval){0,0};
    }
    IL_LIST_APPEND(context->frames_head, ll, frame);
    context->num_frames++;
    timeradd(&frame->elapsed, &context->frames_sum, &context->frames_sum);
    context->frames_average.tv_sec = context->frames_sum.tv_sec / context->num_frames;
    context->frames_average.tv_usec = context->frames_sum.tv_usec / context->num_frames;
    context->frames_average.tv_usec += (context->frames_sum.tv_sec % context->num_frames) * 1000000 / context->num_frames;
}

int ilG_context_setActive(ilG_context *self)
{
    if (!self->complete) {
        il_error("Incomplete context");
        return 0;
    }
    if (!self->valid) {
        il_error("Invalid context");
        return 0;
    }
    if (!self->camera) {
        il_error("No camera");
        return 0;
    }
    if (!self->world) {
        il_error("No world");
        return 0;
    }
    ilE_register(ilG_registry, "tick", ILE_DONTCARE, ILE_MAIN, render_stages, self);
    return 1;
}

static GLvoid error_cb(GLenum source, GLenum type, GLuint id, GLenum severity,
                       GLsizei length, const GLchar* message, GLvoid* user)
{
    (void)id, (void)severity, (void)length, (void)user;
    const char *ssource;
    switch(source) {
        case GL_DEBUG_SOURCE_API_ARB:               ssource="API";              break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:     ssource="Window System";    break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:   ssource="Shader Compiler";  break;
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:       ssource="Third Party";      break;
        case GL_DEBUG_SOURCE_APPLICATION_ARB:       ssource="Application";      break;
        case GL_DEBUG_SOURCE_OTHER_ARB:             ssource="Other";            break;
        default: ssource="???";
    }
    const char *stype;
    switch(type) {
        case GL_DEBUG_TYPE_ERROR_ARB:               stype="Error";                  break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: stype="Deprecated Behaviour";   break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  stype="Undefined Behaviour";    break;
        case GL_DEBUG_TYPE_PORTABILITY_ARB:         stype="Portability";            break;
        case GL_DEBUG_TYPE_PERFORMANCE_ARB:         stype="Performance";            break;
        case GL_DEBUG_TYPE_OTHER_ARB:               stype="Other";                  break;
        default: stype="???";
    }
    const char *sseverity;
    switch(severity) {
        case GL_DEBUG_SEVERITY_HIGH_ARB:    sseverity="HIGH";   break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:  sseverity="MEDIUM"; break;
        case GL_DEBUG_SEVERITY_LOW_ARB:     sseverity="LOW";    break;
        default: sseverity="???";
    }
    il_log("OpenGL %s %s (%s): %s\n", ssource, stype,
            sseverity, message);
}

