/**
* Author: Rui Zhang
* Assignment: Rise of the AI
* Date due: 2024-07-27, 11:59pm
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
#define PLATFORM_COUNT 11
#define ENEMY_COUNT 3
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
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
#include "Map.h"

// ----- STRUCTS AND ENUMS ----- //
struct GameState
{
    Map *map;
    Entity* player;
//    Entity* platforms;
    Entity* enemies;

    Mix_Music* bgm;
    Mix_Chunk* jump_sfx;
};

enum AppStatus { RUNNING, TERMINATED };

// ----- CONSTANTS ----- //
constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char PLAYERSHEET_FILEPATH[] = "assets/rabbit.png",
TILESET_FILEPATH[] = "assets/tileset.png",
VULTURESHEET_FILEPATH[] = "assets/vulture_static.png",
FOXSHEET_FILEPATH[] = "assets/fox_static.png",
HUNTERSHEET_FILEPATH[] = "assets/hunter_static.png";

constexpr char BGM_FILEPATH[] = "assets/dooblydoo.mp3",
SFX_FILEPATH[] = "assets/bounce.wav";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

constexpr float PLATFORM_OFFSET = 5.0f;

// ----- VARIABLES ----- //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

AppStatus g_app_status = RUNNING;

unsigned int LEVEL_1_DATA[] = {
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    2, 2, 1, 1, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2
};

GLuint load_texture(const char* filepath);

void initialise();
void process_input();
void update();
void render();
void shutdown();

// ----- GENERAL FUNCTIONS ----- //
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
    // ----- GENERAL STUFF ----- //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Hello, AI!",
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
    // ----- VIDEO STUFF ----- //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ————— MAP SET-UP ————— //
    GLuint map_texture_id = load_texture(TILESET_FILEPATH);
    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, .5f, 4, 1);

    // ----- PLATFORM ----- //
//    GLuint platform_texture_id = load_texture(TILESET_FILEPATH);
//
//    g_game_state.platforms = new Entity[PLATFORM_COUNT];
//
//    for (int i = 0; i < PLATFORM_COUNT; i++)
//    {
//        g_game_state.platforms[i] = Entity(platform_texture_id, 0.0f, 0.4f, 1.0f, PLATFORM);
//        g_game_state.platforms[i].set_position(glm::vec3(i - PLATFORM_OFFSET, -3.0f, 0.0f));
//        g_game_state.platforms[i].update(0.0f, NULL, NULL, 0);
//    }


    // ------ PLAYER ------//
    GLuint player_texture_id = load_texture(PLAYERSHEET_FILEPATH);

    int player_walking_animation[4][4] =
    {
    { 4, 5, 6, 7 },  // for George to move to the left,
    { 12, 13, 14, 15 }, // for George to move to the right,
    { 0, 1, 2, 3 }, // for George to move upwards,
    { 8, 9, 10, 11 }   // for George to move downwards
    };

    glm::vec3 acceleration = glm::vec3(0.0f, -4.905f, 0.0f);

    g_game_state.player = new Entity(
        player_texture_id,         // texture id
        3.0f,                      // speed
        acceleration,              // acceleration
        3.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        0.9f,                      // width
        0.9f,                       // height
        PLAYER
    );
    g_game_state.player->set_position(glm::vec3(4.5f, 0.0f, 0.0f));

    // Jumping
    g_game_state.player->set_jumping_power(3.0f);

    g_game_state.enemies = new Entity[ENEMY_COUNT];
    GLuint vulture_texture_id = load_texture(VULTURESHEET_FILEPATH);
    GLuint fox_texture_id = load_texture(FOXSHEET_FILEPATH);
    GLuint hunter_texture_id = load_texture(HUNTERSHEET_FILEPATH);
    
    // ----- VULTURE ----- //
    g_game_state.enemies[0] = Entity(vulture_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, WALKER, IDLE);
    g_game_state.enemies[0].set_position(glm::vec3(4.0f, 2.5f, 0.0f));
    g_game_state.enemies[0].set_movement(glm::vec3(-1.0f,0.f,0.f));

    // ----- FOX ----- //

    g_game_state.enemies[1] = Entity(fox_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, GUARD, IDLE);
    g_game_state.enemies[1].set_position(glm::vec3(-2.0f, 2.0f, 0.0f));

    // ----- HUNTER ----- //
    g_game_state.enemies[2] = Entity(hunter_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, WALKER, IDLE);
    g_game_state.enemies[2].set_position(glm::vec3(0.0f, -0.0f, 0.0f));


    // ----- AUDIO STUFF ----- //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_game_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4);

    g_game_state.jump_sfx = Mix_LoadWAV(SFX_FILEPATH);

    // ----- GENERAL STUFF ----- //
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

            case SDLK_SPACE:
                // Jump
                if (g_game_state.player->get_collided_bottom())
                {
                    g_game_state.player->jump();
                    Mix_PlayChannel(-1, g_game_state.jump_sfx, 0);
                }
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
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, ENEMY_COUNT, g_game_state.map);

        for (int i = 0; i < ENEMY_COUNT; i++)
            g_game_state.enemies[i].update(FIXED_TIMESTEP,
                g_game_state.player,
                NULL, 0,
                g_game_state.map);

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
    
    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, 0.0f, 0.0f));
}

void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glClear(GL_COLOR_BUFFER_BIT);

    g_game_state.player->render(&g_shader_program);

//    for (int i = 0; i < PLATFORM_COUNT; i++)
//        g_game_state.platforms[i].render(&g_shader_program);
    for (int i = 0; i < ENEMY_COUNT; i++)
        g_game_state.enemies[i].render(&g_shader_program);
    g_game_state.map->render(&g_shader_program);

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

//    delete[] g_game_state.platforms;
    delete[] g_game_state.enemies;
    delete    g_game_state.player;
    delete    g_game_state.map;
    Mix_FreeChunk(g_game_state.jump_sfx);
    Mix_FreeMusic(g_game_state.bgm);
}

// ----- GAME LOOP ----- //
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