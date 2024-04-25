#ifndef SHADER_H
#define SHADER_H

#include "SDL.h" // IWYU pragma: keep //clangd

Uint8 shader_create_program(const char *vert_src, const char *frag_src,
                            int *status, char *log, size_t log_size);

#endif
