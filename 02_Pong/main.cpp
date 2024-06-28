/**
* Author: Rui Zhang
* Assignment: Pong Clone
* Date due: 2024-06-29, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "glm/ext.hpp"

/* enums */
enum AppStatus {RUNNING, GAMEOVER, TERMINATED};
enum GameMode {ONE, TWO};
enum Winner {PLAYER1, PLAYER2};

/* constants */
// The size of our literal game window
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float WIDTH_BOUND = 4.5f,
HEIGHT_BOUND = 3.6f;

// general
constexpr float BG_RED = 255 / 255.0f,
BG_BLUE = 186 / 255.0f,
BG_GREEN = 186 / 255.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr float HANGING_OFFSET = 0.01f;

// textures
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float PADDLE_WIDTH = 1.0f;
constexpr char PADDLE_LEFT_SPRITE_FILEPATH[] = "assets/left.png";
constexpr float PADDLE_LEFT_HEIGHT = 324 / 157.0f * PADDLE_WIDTH;
constexpr char PADDLE_RIGHT_SPRITE_FILEPATH[] = "assets/right.png";
constexpr float PADDLE_RIGHT_HEIGHT = 327 / 141.0f * PADDLE_WIDTH;
constexpr glm::vec3 PADDLE_LEFT_INIT_SCALE = glm::vec3(PADDLE_WIDTH, PADDLE_LEFT_HEIGHT, 0.0f);
constexpr glm::vec3 PADDLE_RIGHT_INIT_SCALE = glm::vec3(PADDLE_WIDTH, PADDLE_RIGHT_HEIGHT, 0.0f);
constexpr glm::vec3 PADDLE_LEFT_INIT_POS = glm::vec3(-WIDTH_BOUND, 0.0f, 0.0f);
constexpr glm::vec3 PADDLE_RIGHT_INIT_POS = glm::vec3(WIDTH_BOUND, 0.0f, 0.0f);
constexpr char BALL_SPRITE_FILEPATH[] = "assets/bomb.png";
constexpr float BALL_WIDTH = 0.5f;
constexpr float BALL_HEIGHT = 720.0f / 851 * BALL_WIDTH;
constexpr glm::vec3 BALL_INIT_SCALE = glm::vec3(BALL_WIDTH, BALL_HEIGHT, 0.0f);
constexpr float ROT_INCREMENT = 8.0f;

constexpr char PLAYER_ONE_SPRITE_FILEPATH[] = "assets/player1.png";
constexpr char PLAYER_TWO_SPRITE_FILEPATH[] = "assets/player2.png";
constexpr float SIGN_HEIGHT = 1.0f;
constexpr float SIGN1_WIDTH = 326 / 221.0f * SIGN_HEIGHT;
constexpr float SIGN2_WIDTH = 401 / 221.0f * SIGN_HEIGHT;
constexpr glm::vec3 PLAYER_ONE_INIT_POS = glm::vec3(-1.5f, 3.0f, 0.0f);
constexpr glm::vec3 PLAYER_ONE_INIT_SCALE = glm::vec3(SIGN1_WIDTH, SIGN_HEIGHT, 0.0f);
constexpr glm::vec3 PLAYER_TWO_INIT_POS = glm::vec3(1.5f, 3.0f, 0.0f);
constexpr glm::vec3 PLAYER_TWO_INIT_SCALE = glm::vec3(SIGN2_WIDTH, SIGN_HEIGHT, 0.0f);

constexpr char WINNER_ONE_SPRITE_FILEPATH[] = "assets/won1.png";
constexpr char WINNER_TWO_SPRITE_FILEPATH[] = "assets/won2.png";
constexpr float WIN_SIGN_SIZE = 5.0f;
constexpr glm::vec3 WIN_SIGN_INIT_SCALE = glm::vec3(WIN_SIGN_SIZE, WIN_SIGN_SIZE, 0.0f);


constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

/* global variables */
// general
SDL_Window* g_display_window = nullptr;
AppStatus g_game_status = RUNNING;
GameMode g_game_mode = TWO;
Winner g_game_winner;
ShaderProgram g_shader_program = ShaderProgram();

float g_previous_tick = 0.0f;

// texture
GLuint g_paddle_left_texture_id,
g_paddle_right_texture_id,
g_ball_texture_id[],
g_player1_texture_id,
g_player2_texture_id,
g_winner1_texture_id,
g_winner2_texture_id;

// objects
float g_paddle_speed = 2.0f;
float g_ball_speed = 1.0f;

glm::mat4 g_view_matrix,
g_paddle_left_matrix,
g_paddle_right_matrix,
g_ball_matrix,
g_player1_matrix,
g_player2_matrix,
g_winner1_matrix,
g_winner2_matrix,
g_projection_matrix;

glm::vec3 g_paddle_left_position = glm::vec3(0.0f);
glm::vec3 g_paddle_right_position = glm::vec3(0.0f);
glm::vec3 g_ball_position = glm::vec3(0.0f);
glm::vec3 g_paddle_left_movement = glm::vec3(0.0f);
glm::vec3 g_paddle_left_movement_one = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 g_paddle_right_movement = glm::vec3(0.0f);
glm::vec3 g_ball_movement = glm::vec3(2.0f, 1.7f, 0.0f);
glm::vec3 g_ball_rotation = glm::vec3(0.0f);


GLuint load_texture(const char* filepath) {
    // load the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL) {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // generate and bind a texture ID to image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // set texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // release file from memory and return texture id
    stbi_image_free(image);

    return textureID;
}

void initialize() {
    // Initialising
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Assignment01",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    // If the window could not be created, then we should quit the program
    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

    // Create an OpenGL context for an OpenGL window...
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);

    // ...and make it the context we are currently working in
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_paddle_left_matrix = glm::mat4(1.0f);
    g_paddle_right_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    g_player1_matrix = glm::mat4(1.0f);
    g_player2_matrix = glm::mat4(1.0f);
    g_winner1_matrix = glm::mat4(1.0f);
    g_winner2_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_paddle_left_texture_id = load_texture(PADDLE_LEFT_SPRITE_FILEPATH);
    g_paddle_right_texture_id = load_texture(PADDLE_RIGHT_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_player1_texture_id = load_texture(PLAYER_ONE_SPRITE_FILEPATH);
    g_player2_texture_id = load_texture(PLAYER_TWO_SPRITE_FILEPATH);
    g_winner1_texture_id = load_texture(WINNER_ONE_SPRITE_FILEPATH);
    g_winner2_texture_id = load_texture(WINNER_TWO_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    g_paddle_left_movement = glm::vec3(0.0f);
    g_paddle_right_movement = glm::vec3(0.0f);

    // bounce on the right paddle
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
                case SDLK_t:
                    // switch between one and two player mode
                    if (g_game_mode == ONE) g_game_mode = TWO;
                    else {
                        g_game_mode = ONE;
                        //g_paddle_left_movement.y = 1.0f;
                    }
                    break;
                case SDLK_1:
                    break;
                case SDLK_2:
                    break;
                case SDLK_3:
                    break;

                default:
                    break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    // TODO: limit paddle movement.
    if (key_state[SDL_SCANCODE_DOWN])
    {
        g_paddle_right_movement.y = -1.0f;
        //g_animation_indices = g_george_walking[LEFT];
    }
    else if (key_state[SDL_SCANCODE_UP])
    {
        g_paddle_right_movement.y = 1.0f;
        //g_animation_indices = g_george_walking[RIGHT];
    }
    
    if (g_game_mode == TWO) {
        if (key_state[SDL_SCANCODE_W])
        {
            g_paddle_left_movement.y = 1.0f;
            //g_animation_indices = g_george_walking[UP];
        }
        else if (key_state[SDL_SCANCODE_S])
        {
            g_paddle_left_movement.y = -1.0f;
            //g_animation_indices = g_george_walking[DOWN];
        }
    }

    if (glm::length(g_paddle_left_movement) > 1.0f)
    {
        g_paddle_left_movement = glm::normalize(g_paddle_left_movement);
    }
}

void update() {
    if (g_game_status == GAMEOVER) {
        g_winner1_matrix = glm::mat4(1.0f);
        g_winner2_matrix = glm::mat4(1.0f);

        if (g_game_winner == PLAYER1) {
            g_winner1_matrix = glm::scale(g_winner1_matrix, WIN_SIGN_INIT_SCALE);
        }
        else {
            g_winner2_matrix = glm::scale(g_winner2_matrix, WIN_SIGN_INIT_SCALE);

        }
    }
    else if (g_game_status == RUNNING) {
        // delta time
        float tick = SDL_GetTicks() / MILLISECONDS_IN_SECOND;
        float delta_time = tick - g_previous_tick;
        g_previous_tick = tick;

        // game logic - accumulators
        // left paddle
        if (g_game_mode == ONE) {
            if (g_paddle_left_position.y > HEIGHT_BOUND - PADDLE_LEFT_HEIGHT / 2) {
                g_paddle_left_position.y = HEIGHT_BOUND - PADDLE_LEFT_HEIGHT / 2;
                g_paddle_left_movement_one.y *= -1;
            }
            else if (g_paddle_left_position.y < -HEIGHT_BOUND + PADDLE_LEFT_HEIGHT / 2) {
                g_paddle_left_position.y = -HEIGHT_BOUND + PADDLE_LEFT_HEIGHT / 2;
                g_paddle_left_movement_one.y *= -1;
            }
            g_paddle_left_position += g_paddle_left_movement_one * g_paddle_speed * delta_time;
        }
        else {
            if (g_paddle_left_position.y >= HEIGHT_BOUND - PADDLE_LEFT_HEIGHT / 2) {
                g_paddle_left_position.y = HEIGHT_BOUND - PADDLE_LEFT_HEIGHT / 2 - HANGING_OFFSET;
            }
            else if (g_paddle_left_position.y <= -HEIGHT_BOUND + PADDLE_LEFT_HEIGHT / 2) {
                g_paddle_left_position.y = -HEIGHT_BOUND + PADDLE_LEFT_HEIGHT / 2 + HANGING_OFFSET;
            }
            else {
                g_paddle_left_position += g_paddle_left_movement * g_paddle_speed * delta_time;
            }
        }
        // right paddle
        if (g_paddle_right_position.y > HEIGHT_BOUND - PADDLE_RIGHT_HEIGHT / 2) {
            g_paddle_right_position.y = HEIGHT_BOUND - PADDLE_RIGHT_HEIGHT / 2;
        }
        else if (g_paddle_right_position.y < -HEIGHT_BOUND + PADDLE_RIGHT_HEIGHT / 2) {
            g_paddle_right_position.y = -HEIGHT_BOUND + PADDLE_RIGHT_HEIGHT / 2;
        }
        else {
            g_paddle_right_position += g_paddle_right_movement * g_paddle_speed * delta_time;
        }
        //ball
        g_ball_position += g_ball_movement * g_ball_speed * delta_time;
        g_ball_rotation.z += ROT_INCREMENT * delta_time;

        // collision detection
        // up and bottom
        if (g_ball_position.y >= HEIGHT_BOUND) {
            // put it back to the max position to avoid hanging
            g_ball_position.y = HEIGHT_BOUND - BALL_HEIGHT / 2;
            g_ball_movement.y *= -1;
        }
        else if (g_ball_position.y <= -HEIGHT_BOUND) {
            g_ball_position.y = -HEIGHT_BOUND + BALL_HEIGHT / 2;
            g_ball_movement.y *= -1;
        }
        // right side
        if (g_ball_position.x >= PADDLE_RIGHT_INIT_POS.x - PADDLE_WIDTH / 2) {
            // bounce on the right paddle
            if (g_ball_position.y <= g_paddle_right_position.y + PADDLE_RIGHT_HEIGHT / 2 &&
                g_ball_position.y >= g_paddle_right_position.y - PADDLE_RIGHT_HEIGHT / 2) {
                // put the ball back on the paddle   
                g_ball_position.x = PADDLE_RIGHT_INIT_POS.x - PADDLE_WIDTH / 2 - BALL_WIDTH / 2;
                g_ball_movement.x *= -1;
                LOG("BOUNCE RIGHT!!\n\n");
            }
            // right player lose
            else {
                LOG("right lose!!\n\n");
                g_game_status = GAMEOVER;
                g_game_winner = PLAYER1;
            }

        }
        // left side
        if (g_ball_position.x <= PADDLE_LEFT_INIT_POS.x + PADDLE_WIDTH / 2) {
            // bounce on the left paddle
            if (g_ball_position.y <= g_paddle_left_position.y + PADDLE_LEFT_HEIGHT / 2 &&
                g_ball_position.y >= g_paddle_left_position.y - PADDLE_LEFT_HEIGHT / 2) {
                g_ball_position.x = PADDLE_LEFT_INIT_POS.x + PADDLE_WIDTH / 2 + BALL_WIDTH / 2;
                g_ball_movement.x *= -1;
                LOG("BOUNCE LEFT!!\n\n");
            }
            // left player lose
            else {
                g_game_status = GAMEOVER;
                g_game_winner = PLAYER2;
            }

        }

        // model matrix reset
        g_paddle_left_matrix = glm::mat4(1.0f);
        g_paddle_right_matrix = glm::mat4(1.0f);
        g_ball_matrix = glm::mat4(1.0f);
        g_player1_matrix = glm::mat4(1.0f);
        g_player2_matrix = glm::mat4(1.0f);

        // transformation
        g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
        g_ball_matrix = glm::rotate(g_ball_matrix, g_ball_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        g_ball_matrix = glm::scale(g_ball_matrix, BALL_INIT_SCALE);
        g_paddle_left_matrix = glm::translate(g_paddle_left_matrix, PADDLE_LEFT_INIT_POS);
        g_paddle_left_matrix = glm::translate(g_paddle_left_matrix, g_paddle_left_position);
        g_paddle_left_matrix = glm::scale(g_paddle_left_matrix, PADDLE_LEFT_INIT_SCALE);
        g_paddle_right_matrix = glm::translate(g_paddle_right_matrix, PADDLE_RIGHT_INIT_POS);
        g_paddle_right_matrix = glm::translate(g_paddle_right_matrix, g_paddle_right_position);
        g_paddle_right_matrix = glm::scale(g_paddle_right_matrix, PADDLE_LEFT_INIT_SCALE);

        g_player1_matrix = glm::translate(g_player1_matrix, PLAYER_ONE_INIT_POS);
        g_player1_matrix = glm::scale(g_player1_matrix, PLAYER_ONE_INIT_SCALE);

        g_player2_matrix = glm::translate(g_player2_matrix, PLAYER_TWO_INIT_POS);
        g_player2_matrix = glm::scale(g_player2_matrix, PLAYER_TWO_INIT_SCALE);

    }

}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id) {
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    // Quite simply: clear the space in memory holding our colours
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // bind texture
    if (g_game_status == RUNNING) {
        draw_object(g_paddle_left_matrix, g_paddle_left_texture_id);
        draw_object(g_paddle_right_matrix, g_paddle_right_texture_id);
        draw_object(g_player1_matrix, g_player1_texture_id);
        draw_object(g_player2_matrix, g_player2_texture_id);
        draw_object(g_ball_matrix, g_ball_texture_id);

    }
    else if (g_game_status == GAMEOVER) {
        if (g_game_winner == PLAYER1) draw_object(g_winner1_matrix, g_winner1_texture_id);
        else draw_object(g_winner2_matrix, g_winner2_texture_id);
    }

    
    // disable two attribute arrays
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Update a window with whatever OpenGL is rendering
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[])
{
    initialize();

    while (g_game_status == RUNNING || g_game_status == GAMEOVER)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}