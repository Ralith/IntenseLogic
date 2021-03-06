/** @file common/world.h
 * @brief Container for data related to in-game scenes
 */

#ifndef IL_COMMON_WORLD_H
#define IL_COMMON_WORLD_H

#include <stdlib.h>

#include "util/array.h"
#include "common/base.h"

struct il_positionable;
struct ilG_context;

extern il_type il_world_type;

typedef struct il_world {
    il_base base;
    IL_ARRAY(struct il_positionable*,) objects;
    struct ilG_context* context;
    struct ilP_simulation *simulation;
} il_world;

typedef struct il_worldIterator il_worldIterator;

il_world* il_world_new();

/** Adds a positionable to a world */
void il_world_add(il_world*, struct il_positionable*);

/** Iterates the positionables in a world. Pass a pointer to NULL for first iteration */
struct il_positionable * il_world_iterate(il_world*,
  il_worldIterator**);

#endif
