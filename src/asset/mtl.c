#include "mtl.h"

#include <stdio.h>
#include <stdlib.h>

#include "util/log.h"
#include "asset/image.h"

char *strsep(char **stringp, const char *delim);

static int parse_line(ilA_mtl *mtl, char *line, char *error)
{
    char *word;
    int col = 0;

#define next_word strsep(&line, " ")
    word = next_word;
    if (strcmp(word, "newmtl") == 0) {
        mtl->cur = calloc(1, sizeof(ilA_mtl));
        mtl->cur->name = strdup(next_word);
        HASH_ADD_KEYPTR(hh, mtl, mtl->cur->name, strlen(mtl->cur->name), mtl->cur);
    } else if (strcmp(word, "Ka") == 0) {
        mtl->ambient[0] = strtof(next_word, NULL);
        mtl->ambient[1] = strtof(next_word, NULL);
        mtl->ambient[2] = strtof(next_word, NULL);
    } else if (strcmp(word, "Kd") == 0) {
        mtl->diffuse[0] = strtof(next_word, NULL);
        mtl->diffuse[1] = strtof(next_word, NULL);
        mtl->diffuse[2] = strtof(next_word, NULL);
    } else if (strcmp(word, "Ks") == 0) {
        mtl->specular[0] = strtof(next_word, NULL);
        mtl->specular[1] = strtof(next_word, NULL);
        mtl->specular[2] = strtof(next_word, NULL);
    } else if (strcmp(word, "Ns") == 0) {
        mtl->specular[3] = strtof(next_word, NULL);
    } else if (strcmp(word, "Tr") == 0 || strcmp(word, "d") == 0) {
        mtl->transparency = strtof(next_word, NULL);
    } else if (strcmp(word, "#") != 0 && *word != 0) {
        snprintf(error, 1024, "Unknown command \"%s\"", word);
        col = 1;
    }
#undef next
    return col;
}

ilA_mtl *ilA_mesh_parseMtl(const char *filename, const char *data, size_t length)
{
    ilA_mtl *mtl = calloc(1, sizeof(ilA_mtl));
    char *str = strndup(data, length), *saveptr = str, *ptr, error[1024], colstr[8];
    int line = 0, col;

    while ((ptr = strsep(&saveptr, "\n"))) {
        line++;
        col = parse_line(mtl, ptr, error);
        if (col > 0) {
            snprintf(colstr, 8, "%i", col);
            il_log_real(filename, line, colstr, 2, "%s", error);
        }
    }
    free(str);
    return mtl;
}

void ilA_mtl_free(ilA_mtl *self)
{
    ilA_mtl *node, *tmp;
    HASH_ITER(hh, self, node, tmp) {
        HASH_DELETE(hh, self, node);
        if (node->name) {
            free(node->name);
        }
        if (node->diffuse_map) {
            ilA_img_free(node->diffuse_map);
        }
        if (node->specular_map) {
            ilA_img_free(node->specular_map);
        }
        if (node->specular_highlight_map) {
            ilA_img_free(node->specular_highlight_map);
        }
        free(node);
    }
    free(self);
}

