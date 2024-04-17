#include "SDL.h" // IWYU pragma: keep //clangd
#include "SDL_opengl.h"
#include <SDL_video.h>

int initialize_sdl(void);
void shutdown_sdl(void);
static SDL_Window *window;
static SDL_GLContext *ctx;

void initialize_gl(void)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(1);
    ctx = SDL_GL_CreateContext(window);
}

void display_gl_info(void)
{
    char info[512];
    SDL_snprintf(info, SDL_arraysize(info), "\
            GPU: %s\n\
            GL VERSION: %s",
                 glGetString(GL_RENDERER), glGetString(GL_VERSION));
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "DEBUG", info, window);
}

int main(int argc, char *argv[])
{
    SDL_bool running = SDL_FALSE;

    if (initialize_sdl() < 0)
        shutdown_sdl();

    initialize_gl();
    display_gl_info();

    int frames = 0;
    Uint32 last_frame_time = 0;

    while (running) {
        Uint32 now = SDL_GetTicks64();
        if (now - last_frame_time >= 1000) {
            SDL_Log("fps: %i", frames);
            last_frame_time = now;
            frames = 0;
        }
        frames++;

        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT)
                running = SDL_FALSE;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }
    shutdown_sdl();
    return 0;
}

int initialize_sdl(void)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("SDLGL", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1920, 1080,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

void shutdown_sdl(void)
{
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
