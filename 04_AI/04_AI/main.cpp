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
//#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 11
#define ENEMY_COUNT 4
#define LEVEL1_WIDTH 20
#define LEVEL1_HEIGHT 8


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
#include "Utility.h"

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
enum GameResult { NONE, WIN, LOSE };

// ----- CONSTANTS ----- //
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

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
//constexpr char PLAYERSHEET_FILEPATH[] = "assets/george_0.png",
TILESET_FILEPATH[] = "assets/winterTileSheet1.png",
//TILESET_FILEPATH[] = "assets/tileset.png",
VULTURESHEET_FILEPATH[] = "assets/vulture1.png",
FOXSHEET_FILEPATH[] = "assets/fox.png",
HUNTERSHEET_FILEPATH[] = "assets/hunter_static.png",
BULLETSHEET_FILEPATH[] = "assets/bullet.png",
FONTSHEET_FILEPATH[] = "assets/font1.png";

constexpr char BGM_FILEPATH[] = "assets/theSnowQueen.mp3",
SFX_FILEPATH[] = "assets/snowWalk.mp3";

//constexpr float PLATFORM_OFFSET = 5.0f;

// ----- VARIABLES ----- //
GameState g_game_state;
GameResult g_game_result = NONE;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

AppStatus g_app_status = RUNNING;

unsigned int LEVEL_1_DATA[] = {
    19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19,
    19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19,
    19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19,
    19, 0, 0, 0, 14, 15, 16, 0, 0, 0, 0, 14, 16, 0, 0, 0, 0, 0, 0, 19,
    19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19,
    19, 0, 0, 0, 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 1, 3, 0, 0, 19,
    19, 2, 2, 2, 2, 2, 7, 8, 10, 11, 2, 2, 2, 2, 7, 8, 10, 11, 2, 19,
    19, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 19
    
};

//GLuint load_texture(const char* filepath);
GLuint g_font_texture_id;

bool g_shooter_is_active = true;
int g_current_enemy_count;

float g_message_x = 0.0f,
g_message_y = 0.0f;

void initialise();
void process_input();
void update();
void render();
void shutdown();

// ----- GENERAL FUNCTIONS ----- //
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
    GLuint map_texture_id = Utility::load_texture(TILESET_FILEPATH);
    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, 5, 4);

    // ------ PLAYER ------//
    GLuint player_texture_id = Utility::load_texture(PLAYERSHEET_FILEPATH);

    int player_walking_animation[4][4] =
    {
    { 4, 5, 6, 7 },  // for player to move to the left,
    { 12, 13, 14, 15 }, // for player to move to the right,
    { 0, 1, 2, 3 }, // for player to move upwards,
    { 8, 9, 10, 11 }   // for player to move downwards
    };
//    int player_walking_animation[4][4] =
//    {
//        { 1, 5, 9, 13 },  // for George to move to the left,
//        { 3, 7, 11, 15 }, // for George to move to the right,
//        { 2, 6, 10, 14 }, // for George to move upwards,
//        { 0, 4, 8, 12 }   // for George to move downwards
//    };


    glm::vec3 gravity = glm::vec3(0.0f, -4.905f, 0.0f);

    g_game_state.player = new Entity(
        player_texture_id,         // texture id
        3.0f,                      // speed
        gravity,              // acceleration
        3.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        0.5f,                      // width
        0.5f,                       // height
        PLAYER,
        NOTYPE,
        NOSTATE
    );
    g_game_state.player->set_position(glm::vec3(7.0f, -4.0f, 0.0f));
    // Jumping
    g_game_state.player->set_jumping_power(4.5f);
    g_game_state.player->set_enemy_count(ENEMY_COUNT);

    g_game_state.enemies = new Entity[ENEMY_COUNT];
    GLuint vulture_texture_id = Utility::load_texture(VULTURESHEET_FILEPATH);
    GLuint fox_texture_id = Utility::load_texture(FOXSHEET_FILEPATH);
    GLuint hunter_texture_id = Utility::load_texture(HUNTERSHEET_FILEPATH);
    GLuint bullet_texture_id = Utility::load_texture(BULLETSHEET_FILEPATH);
    
    int enemy_animation[4][4] =
    {
    { 0, 1, 2, 3 },     // fly left,
    { 4, 5, 6, 7 }, // fly right,
    { 8, 9, 10, 11 },     // die left,
    { 12, 13, 14, 15 }    // die right
    };

    // ----- VULTURE ----- //

    g_game_state.enemies[0] = Entity(vulture_texture_id, -1.0f, glm::vec3(0.0f), 0.0f, enemy_animation, 0.0f, 4, 0, 4, 4, 1.0f, 1.0f, ENEMY, FLYER, IDLE);
    g_game_state.enemies[0].set_position(glm::vec3(8.0f, -0.5f, 0.0f));

    // ----- FOX ----- //

    g_game_state.enemies[1] = Entity(fox_texture_id, 1.0f, gravity, 0.0f, enemy_animation, 0.0f, 4, 0, 4, 4, 1.5f, 1.5f, ENEMY, GUARD, IDLE);
    g_game_state.enemies[1].set_position(glm::vec3(2.0f, -5.0f, 0.0f));

    // ----- HUNTER ----- //
    g_game_state.enemies[2] = Entity(hunter_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, SHOOTER, IDLE);
    g_game_state.enemies[2].set_position(glm::vec3(15.5f, -4.0f, 0.0f));
    
    // ----- BULLET ----- //
    g_game_state.enemies[3] = Entity(bullet_texture_id, 2.0f, 0.3f, 0.3f, ENEMY, BULLET, IDLE);
    g_game_state.enemies[3].set_position(glm::vec3(15.5f, -4.0f, 0.0f));


    // ----- AUDIO STUFF ----- //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_game_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4);

    g_game_state.jump_sfx = Mix_LoadWAV(SFX_FILEPATH);
    
    // ----- FONT -----//
    g_font_texture_id = Utility::load_texture(FONTSHEET_FILEPATH);

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
                if (g_game_state.player->get_map_collided_bottom())
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
    if (g_game_result == NONE) {
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
            g_current_enemy_count = g_game_state.player->get_enemy_count();

            for (int i = 0; i < ENEMY_COUNT; i++) {
                // deactivate bullet if shooter is dead
                Entity *current_enemy = &g_game_state.enemies[i];
                if (current_enemy->get_entity_type() == ENEMY && current_enemy->get_ai_type() == SHOOTER && !current_enemy->get_activation_status()) {
                    g_shooter_is_active = false;
                }
                if (current_enemy->get_entity_type() == ENEMY && current_enemy->get_ai_type() == BULLET && !g_shooter_is_active) {
                    current_enemy->deactivate();
                    g_current_enemy_count--;
                }
                // normal update for enemies
                current_enemy->update(FIXED_TIMESTEP,
                    g_game_state.player,
                    NULL, NULL,
                    g_game_state.map);
            }
            
            // check for lose
            if (!g_game_state.player->get_activation_status()) g_game_result = LOSE;
            
            // check for win
            if (g_current_enemy_count == 0) g_game_result = WIN;


            delta_time -= FIXED_TIMESTEP;
        }

        g_accumulator = delta_time;
        
        // Prevent the camera from showing anything outside of the "edge" of the level
        g_view_matrix = glm::mat4(1.0f);
        
        if (g_game_state.player->get_position().x > LEFT_EDGE && g_game_state.player->get_position().x < RIGHT_EDGE) {
            g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, 3, 0));
        } else if (g_game_state.player->get_position().x <= LEFT_EDGE){
            g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-LEFT_EDGE, 3, 0));
        } else {
            g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-RIGHT_EDGE, 3, 0));
        }
    }

}

void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);

    glClear(GL_COLOR_BUFFER_BIT);

    if (g_game_result == WIN) {
        g_message_x = g_game_state.player->get_position().x - 2.0f;
        if (g_message_x <= LEFT_EDGE) g_message_x = LEFT_EDGE;
        Utility::draw_text(&g_shader_program, g_font_texture_id, "You Won!", 0.5f, -0.05f,
            glm::vec3(g_message_x, g_message_y, 0.0f));
    }
    else if (g_game_result == LOSE) {
        g_message_x = g_game_state.player->get_position().x - 2.0f;
        if (g_message_x <= LEFT_EDGE) g_message_x = LEFT_EDGE;
        Utility::draw_text(&g_shader_program, g_font_texture_id, "You Lost!", 0.5f, -0.05f,
            glm::vec3(g_message_x, g_message_y,  0.0f));

    }

    g_game_state.player->render(&g_shader_program);

    for (int i = 0; i < ENEMY_COUNT; i++)
        g_game_state.enemies[i].render(&g_shader_program);
    
    g_game_state.map->render(&g_shader_program);

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

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
