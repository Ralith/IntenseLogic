#include "geometrypass.h"

#include <sys/time.h>

#include "graphics/tracker.h"
#include "graphics/bindable.h"
#include "graphics/stage.h"
#include "graphics/context.h"
#include "graphics/glutil.h"
#include "graphics/arrayattrib.h"
#include "graphics/drawable3d.h"

static void draw_geometry(ilG_stage *self)
{
    ilG_testError("Unknown");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    ilG_testError("glEnable");
    ilG_testError("Error setting up for draw");

    ilG_context *context = self->context;
    struct timeval tv;
    il_positionable* pos = NULL;
    ilG_trackiterator * iter = ilG_trackiterator_new(context);
    gettimeofday(&tv, NULL);
    //const ilG_bindable *drawable = NULL, *material = NULL, *texture = NULL;

    while (ilG_trackIterate(iter)) {
        pos = ilG_trackGetPositionable(iter);
        context->positionable = pos;

        ilG_drawable3d *drawable = ilG_trackGetDrawable(iter);
        if (ILG_TESTATTR(drawable->attrs, ILG_ARRATTR_ISTRANSPARENT)) {
            continue;
        }
        ilG_bindable_swap(&context->drawableb, (void**)&context->drawable, drawable);
        ilG_bindable_swap(&context->materialb, (void**)&context->material, ilG_trackGetMaterial(iter));
        ilG_bindable_swap(&context->textureb,  (void**)&context->texture,  ilG_trackGetTexture(iter));

        ilG_bindable_action(context->materialb, context->material);
        ilG_bindable_action(context->textureb,  context->texture);
        ilG_bindable_action(context->drawableb, context->drawable);
    }
    ilG_bindable_unbind(context->materialb, context->material);
    ilG_bindable_unbind(context->textureb,  context->texture);
    ilG_bindable_unbind(context->drawableb, context->drawable);
    context->drawable = NULL;
    context->material = NULL;
    context->texture = NULL;
    context->drawableb = NULL;
    context->materialb = NULL;
    context->textureb = NULL;
}

void ilG_geometrypass(ilG_stage* self)
{
    self->run = draw_geometry;
    self->name = "Geometry Pass";
}

