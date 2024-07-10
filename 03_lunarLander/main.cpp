/**
* Author: Rui Zhang
* Assignment: Lunar Lander
* Date due: 2024-07-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define GROUND_COUNT 1
#define FOREST_COUNT 2

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
enum AppStatus { RUNNING, TERMINATED };

struct GameState
{
    Entity* player;
    Entity* ground;
    Entity* forests;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char PLAYER_FILEPATH[] = "assets/parachute.png";
constexpr char GROUND_FILEPATH[] = "assets/ground.png";
constexpr char FOREST_FILEPATH[] = "assets/forest.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

constexpr float FOREST_HEIGHT = 2.0f;
constexpr float FOREST_WIDTH = 1280 / 374.0f * FOREST_HEIGHT;
constexpr float GROUND_HEIGHT = 0.48f;
constexpr float GROUND_WIDTH = 360 / 78.0f * GROUND_HEIGHT;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath);

void initialise();
void process_input();
void update();
void render();
void shutdown();

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Luanr Lander Variant",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (context == nullptr)
    {
        LOG("ERROR: Could not create OpenGL context.\n");
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ----- PLAYER ----- //
    GLuint player_texture_id = load_texture(PLAYER_FILEPATH);

    g_game_state.player = new Entity(
        player_texture_id,         // texture id
        1.0f,                      // speed
        glm::vec3(0.0f, -0.1f, 0.0f),   // acceleration
        0.5f,                      // width
        0.5f                       // height
    );

    g_game_state.player->set_position(glm::vec3(0.0f, 2.5f, 0.0f));

    // ––––– FOREST ––––– //
    g_game_state.forests = new Entity[FOREST_COUNT];
    GLuint forest_texture_id = load_texture(FOREST_FILEPATH);

    for (int i = 0; i < FOREST_COUNT; i++) {
        g_game_state.forests[i] = Entity(
            forest_texture_id, // texture id
            0.0f,              // speed
            glm::vec3(0.0f),   // acceleration
            1.0f,              // width
            1.0f               // height
        );

        g_game_state.forests[i].set_position(glm::vec3(-4.0f + 9.0f * i, -3.1f, 0.0f));
        g_game_state.forests[i].update(0.0f, NULL, 0);

        g_game_state.forests[i].set_scale(glm::vec3(FOREST_WIDTH, FOREST_HEIGHT, 0.0f));

    }
    g_game_state.forests[1].set_rotation(glm::vec3(0.0f, glm::radians(180.0f), 0.0f));
    

    // ––––– GROUND ––––– //
    GLuint ground_texture_id = load_texture(GROUND_FILEPATH);

    g_game_state.ground = new Entity(
        ground_texture_id,         // texture id
        0.0f,                      // speed
        glm::vec3(0.0f),   // acceleration
        1.0f,                      // width
        1.0f                       // height
    );

    g_game_state.ground->set_position(glm::vec3(0.5f, -3.5f, 0.0f));
    g_game_state.ground->set_scale(glm::vec3(GROUND_WIDTH, GROUND_HEIGHT, 0.0f));

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_app_status = TERMINATED;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])       g_game_state.player->move_left();
    else if (key_state[SDL_SCANCODE_RIGHT]) g_game_state.player->move_right();

    if (key_state[SDL_SCANCODE_UP])       g_game_state.player->move_up();
    else if (key_state[SDL_SCANCODE_DOWN]) g_game_state.player->move_down();

    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
        g_game_state.player->normalise_movement();
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.forests, FOREST_COUNT);
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.ground, GROUND_COUNT);
        g_game_state.forests[0].update(0.0f, NULL, 0);
        g_game_state.forests[1].update(0.0f, NULL, 0);
        g_game_state.ground->update(0.0f, NULL, 0);

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_game_state.player->render(&g_shader_program);

    for (int i = 0; i < FOREST_COUNT; i++)
        g_game_state.forests[i].render(&g_shader_program);

    g_game_state.ground->render(&g_shader_program);

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete[] g_game_state.forests;
    delete g_game_state.player;
    delete g_game_state.ground;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}