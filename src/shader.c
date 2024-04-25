#include "../external/glad.h"
#include "SDL.h" // IWYU pragma: keep //clangd

#include "shader.h"

Uint8 shader_create_program(const char *vert_src, const char *frag_src,
                            int *status, char *log, size_t log_size)
{
    //  vertex shader
    Uint8 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vert_src, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, status);
    if (!*status)
        glGetShaderInfoLog(vertex_shader, log_size, NULL, log);

    // fragment shader
    Uint8 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &frag_src, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, status);
    if (!*status)
        glGetShaderInfoLog(fragment_shader, log_size, NULL, log);

    // shader program
    Uint8 shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, status);
    if (!*status)
        glGetProgramInfoLog(shader_program, log_size, NULL, log);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}
