#include "tracker.h"

#include <string.h>
#include <stdlib.h>

#include "common/world.h"
#include "graphics/context.h"
#include "graphics/drawable3d.h"
#include "graphics/material.h"
#include "graphics/texture.h"
#include "util/array.h"
#include "util/log.h"
#include "common/positionable.h"

int ilG_trackPositionable(ilG_context* ctx, il_positionable* self)
{
#define check_null(n) if(!n) {il_error("Null " #n); return 0; }
    // make sure we have valid parameters, several bugs have been caught here
    check_null(ctx);
    check_null(self);
    //check_null(ctx->world);
    //check_null(ctx->world->id);
    check_null(self->drawable);
    check_null(self->material);
    check_null(self->texture);
    check_null(self->drawable->id);
    check_null(self->material->id);
    check_null(self->texture->id);
#undef check_null

    unsigned int i;
    for (i = 0; i < ctx->positionables.length; i++) {
        il_positionable* pos = ctx->positionables.data[i];
        if (pos->drawable->id > self->drawable->id) goto insert;
        if (pos->material->id > self->material->id) goto insert;
        if (pos->texture->id  > self->texture->id)  goto insert;
        continue;
insert:
        IL_INSERT(ctx->positionables, i, self);
        return 1;
    }
    // if we even reach here it means that there are no other drawable-material-texture pairs in the list
    IL_APPEND(ctx->positionables, self);
    return 1;
}

int ilG_untrackPositionable(ilG_context* ctx, il_positionable* positionable)
{
    unsigned int i;
    for (i = 0; i < ctx->positionables.length; i++) {
        if (ctx->positionables.data[i] == positionable) {
            IL_REMOVE(ctx->positionables, i);
            return 0;
        }
    }
    return 1;
}

struct ilG_trackiterator {
    ilG_context* context;
    unsigned int i;
    int initialized;
};

ilG_trackiterator* ilG_trackiterator_new(ilG_context* ctx) 
{
    ilG_trackiterator *iter = calloc(1, sizeof(ilG_trackiterator));
    iter->context = ctx;
    iter->initialized = 0;
    return iter;
}

int ilG_trackIterate(ilG_trackiterator* iter)
{
    if (iter->context->positionables.length == 0) {
        free(iter);
        return 0;
    }
    if (!iter->initialized) {
        iter->initialized = 1;
        return 1;
    }
    iter->i++;
    if (iter->i >= iter->context->positionables.length) {
        free(iter);
        return 0;
    }

    return 1;
}

il_positionable* ilG_trackGetPositionable(ilG_trackiterator* iter)
{
    return iter->context->positionables.data[iter->i];
}

ilG_drawable3d* ilG_trackGetDrawable(ilG_trackiterator* iter)
{
    return iter->context->positionables.data[iter->i]->drawable;
}

ilG_material* ilG_trackGetMaterial(ilG_trackiterator* iter)
{
    return iter->context->positionables.data[iter->i]->material;
}

ilG_texture* ilG_trackGetTexture(ilG_trackiterator* iter)
{
    return iter->context->positionables.data[iter->i]->texture;
}

IL_ARRAY(ilG_drawable3d*, drawable_index) drawable_indices;
IL_ARRAY(ilG_material*, material_index) material_indices;
IL_ARRAY(ilG_texture*, texture_index) texture_indices;

ilG_drawable3d* ilG_drawable3d_fromId(unsigned int id) 
{
    ilG_drawable3d** out;
    IL_INDEX(drawable_indices, id - 1, out);
    if (out) return *out;
    return NULL;
}

ilG_material* ilG_material_fromId(unsigned int id) 
{
    ilG_material** out;
    IL_INDEX(material_indices, id - 1, out);
    if (out) return *out;
    return NULL;
}

ilG_texture* ilG_texture_fromId(unsigned int id)
{
    ilG_texture** out;
    IL_INDEX(texture_indices, id - 1, out);
    if (out) return *out;
    return NULL;
}

void ilG_drawable3d_assignId(ilG_drawable3d* self)
{
    if (self->id) return; // already has an ID???
    self->id = drawable_indices.length+1;
    IL_APPEND(drawable_indices, self);
}

void ilG_material_assignId(ilG_material* self)
{
    if (self->id) return;
    self->id = material_indices.length+1;
    IL_APPEND(material_indices, self);
}

void ilG_texture_assignId(ilG_texture* self)
{
    if (self->id) return;
    self->id = texture_indices.length+1;
    IL_APPEND(texture_indices, self);
}

void ilG_drawable3d_setId(ilG_drawable3d* self, unsigned int id)
{
    self->id = id-1;
    IL_SET(drawable_indices, id, self);
}

void ilG_material_setId(ilG_material* self, unsigned int id)
{
    self->id = id-1;
    IL_SET(material_indices, id, self);
}

void ilG_texture_setId(ilG_texture* self, unsigned int id)
{
    self->id = id-1;
    IL_SET(texture_indices, id, self);
}

