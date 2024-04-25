#define GLAD_GL_IMPLEMENTATION
#include "../external/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

#include "SDL.h" // IWYU pragma: keep //clangd

#include "shader.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define TILE_SIZE 32
#define LAYER_NUM 6
#define BUFSIZ_TILE_X 61 // ROUND(WINDOW_WIDTH/TILE_SIZE)+1
#define BUFSIZ_TILE_Y 35 // ROUND(WINDOW_HEIGHT/TILE_SIZE)+1

/**
 *  Considering:
 *  - Target resolution: 1920x1080
 *  - Target tile size: 32
 *  - Target layer number: 6
 *  - A buffer of 1 column and 1 row on the edges
 *
 *  Number of tiles on x will be: 1920/32 + 1 = 61
 *  Number of tiles on y will be: 1080/32 + 1 = 35
 *  Total number of tiles/transformations: 61*35*6 = 12810
 */
#define TRANSLATION_NUM 12810

int initialize_sdl(void);
void shutdown_sdl(void);
static SDL_Window *window;
static SDL_GLContext *ctx;
static unsigned int shader_program, VAO, VBO, EBO, texture, instance_VBO;

static const char *vertex_src =
    "#version 320 es\n"
    "precision mediump float;\n"
    "layout (location = 0) in vec4 vertex;\n"
    "layout (location = 1) in vec4 offset;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "   TexCoord = vertex.zw + offset.zw;\n"
    "   gl_Position = vec4(vertex.x + offset.x, vertex.y + offset.y, 0, 1.0);\n"
    "}\0";

static const char *fragment_src =
    "#version 320 es\n"
    "precision mediump float;\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(texture1, TexCoord);\n"
    "}\n\0";

struct vec4 {
    float x, y, z, w;
};
static struct vec4 translations[TRANSLATION_NUM];

float normalize_x(float x, float viewport_width)
{
    return x / viewport_width * 2 - 1;
}

float normalize_y(float y, float viewport_height)
{
    return y / viewport_height * 2 - 1;
}

void initialize_gl(void)
{
    ctx = SDL_GL_CreateContext(window);

    // try to init glad with ogl profile
    const char *platform = SDL_GetPlatform();
    int profile, version;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);
    if (profile == SDL_GL_CONTEXT_PROFILE_ES) {
        version = gladLoadGLES2((GLADloadfunc)SDL_GL_GetProcAddress);
    } else {
        version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    }
    if (!version) {
        SDL_Log("Failed to initialize GLAD (OGL/OGLES) on platform: %s",
                platform);
    }
    SDL_Log("GLAD initialization success.");
    SDL_Log("PLATFORM: %s, PROFILE: %i, VERSION: %i", platform, profile,
            version);

    // vsync off so we can see fps
    SDL_GL_SetSwapInterval(0);

    // enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // create shader program
    int status;
    char info_log[512] = {0};
    shader_program =
        shader_create_program(vertex_src, fragment_src, &status, info_log, 512);
    if (!status)
        SDL_Log("shader error: \n\n%s\n", info_log);
    glUseProgram(shader_program);

    // translations
    int n = 0;
    const int tileset_tile_num_x = 6;
    for (int l = 0; l < LAYER_NUM; l++) {
        for (int x = 0; x < BUFSIZ_TILE_X; x += 1) {
            for (int y = 0; y < BUFSIZ_TILE_Y; y += 1) {
                float offsetx = (float)x / WINDOW_WIDTH * 2 * TILE_SIZE;
                float offsety = (float)y / WINDOW_HEIGHT * 2 * TILE_SIZE;
                float tloffsetx = (float)l / tileset_tile_num_x;
                translations[n] = (struct vec4){offsetx, offsety, tloffsetx, 0};
                n++;
            }
        }
    }

    // create instance vbo
    glGenBuffers(1, &instance_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, instance_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct vec4) * TRANSLATION_NUM,
                 &translations[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind instance_vbo

    // setup vertices/indices of quad
    SDL_FPoint top_right = {normalize_x(TILE_SIZE, WINDOW_WIDTH),
                            normalize_y(TILE_SIZE, WINDOW_HEIGHT)};
    SDL_FPoint bottom_right = {normalize_x(TILE_SIZE, WINDOW_WIDTH),
                               normalize_y(0, WINDOW_HEIGHT)};
    SDL_FPoint bottom_left = {normalize_x(0, WINDOW_WIDTH),
                              normalize_y(0, WINDOW_HEIGHT)};
    SDL_FPoint top_left = {normalize_x(0, WINDOW_WIDTH),
                           normalize_y(TILE_SIZE, WINDOW_HEIGHT)};
    float tloffsetx = 1.0f / 6.0f;
    float vertices[] = {
        // positions                    // texture coords
        top_right.x,    top_right.y,    tloffsetx, 1.0f, // top right
        bottom_right.x, bottom_right.y, tloffsetx, 0.0f, // bottom right
        bottom_left.x,  bottom_left.y,  0.0f,      0.0f, // bottom left
        top_left.x,     top_left.y,     0.0f,      1.0f  // top left
    };
    Uint32 indices[] = {
        0, 1, 3, // first Triangle
        1, 2, 3  // second Triangle
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind vbo

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, instance_VBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind instance vbo
    glVertexAttribDivisor(1, 1);      // create the instances

    // create opengl texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    stbi_set_flip_vertically_on_load(1);

    // load image using sdl's crossplatform api for files
    int width, height, nrChannels;
    Uint8 buffer[900] = {0};
    SDL_RWops *file = SDL_RWFromFile("opengl.png", "r");
    if (file == NULL) {
        SDL_Log("Error opening file");
    }
    size_t i = 0;
    for (; i < 900; i++) {
        if (SDL_RWread(file, &buffer[i], sizeof(Uint8), 1) <= 0) {
            buffer[i] = 0;
            break;
        }
    }
    SDL_RWclose(file);

    // load image data from memory using stbi and create the texture
    Uint8 *data = stbi_load_from_memory(buffer, i, &width, &height, &nrChannels,
                                        STBI_rgb_alpha);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        SDL_Log("Failed to load texture: %s", stbi_failure_reason());
    }
    stbi_image_free(data);
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
    SDL_bool running = SDL_TRUE;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

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

            if (evt.type == SDL_WINDOWEVENT) {
                if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    glViewport(0, 0, evt.window.data1, evt.window.data2);
                }
            }
        }

        glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // shader already set to use
        // ebo and vao already binded
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                                TRANSLATION_NUM);

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

    window = SDL_CreateWindow(
        "SDLGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
        WINDOW_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

void shutdown_sdl(void)
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader_program);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
