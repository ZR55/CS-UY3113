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
enum AppStatus {RUNNING, TERMINATED};

/* constants */
// The size of our literal game window
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float WIDTH_BOUND = 4.9f,
HEIGHT_BOUND = 3.6f;

// general
constexpr float BG_RED = 193 / 255.0f,
BG_BLUE = 193 / 255.0f,
BG_GREEN = 225 / 255.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

// textures
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char PADDLE_SPRITE_FILEPATH[] = "assets/paddle.png";
constexpr float PADDLE_WIDTH = 0.3f;
constexpr float PADDLE_HEIGHT = 630 / 108.0f * PADDLE_WIDTH;
constexpr glm::vec3 PADDLE_INIT_SCALE = glm::vec3(PADDLE_WIDTH, PADDLE_HEIGHT, 0.0f);
constexpr glm::vec3 PADDLE_LEFT_INIT_POS = glm::vec3(-WIDTH_BOUND, 0.0f, 0.0f);
constexpr glm::vec3 PADDLE_RIGHT_INIT_POS = glm::vec3(WIDTH_BOUND, 0.0f, 0.0f);
constexpr char BALL_SPRITE_FILEPATH[] = "assets/ball.png";
constexpr float BALL_SIZE = 0.3f;
constexpr glm::vec3 BALL_INIT_SCALE = glm::vec3(BALL_SIZE, BALL_SIZE, 0.0f);
//constexpr float BALL_INIT_SPEED_X = 2.0f;
//constexpr float BALL_INIT_SPEED_Y = 1.7f;

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

/* global variables */
// general
SDL_Window* g_display_window = nullptr;
AppStatus g_game_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

float g_previous_tick = 0.0f;

// texture
GLuint g_paddle_left_texture_id,
g_paddle_right_texture_id,
g_ball_texture_id;

// objects
float g_paddle_speed = 2.0f;
float g_ball_speed = 1.0f;

glm::mat4 g_view_matrix,
g_paddle_left_matrix,
g_paddle_right_matrix,
g_ball_matrix,
g_projection_matrix;

glm::vec3 g_paddle_left_position = glm::vec3(0.0f);
glm::vec3 g_paddle_right_position = glm::vec3(0.0f);
glm::vec3 g_ball_position = glm::vec3(0.0f);
glm::vec3 g_paddle_left_movement = glm::vec3(0.0f);
glm::vec3 g_paddle_right_movement = glm::vec3(0.0f);
glm::vec3 g_ball_movement = glm::vec3(2.0f, 1.7f, 0.0f);


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
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_paddle_left_texture_id = load_texture(PADDLE_SPRITE_FILEPATH);
    g_paddle_right_texture_id = load_texture(PADDLE_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    g_paddle_left_movement = glm::vec3(0.0f);
    g_paddle_right_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_game_status = TERMINATED;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

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

    if (glm::length(g_paddle_left_movement) > 1.0f)
    {
        g_paddle_left_movement = glm::normalize(g_paddle_left_movement);
    }
}

void update() {
    // delta time
    float tick = SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = tick - g_previous_tick;
    g_previous_tick = tick;

    // game logic - accumulators
    g_paddle_left_position += g_paddle_left_movement * g_paddle_speed * delta_time;
    g_paddle_right_position += g_paddle_right_movement * g_paddle_speed * delta_time;
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;
    //std::cout << glm::to_string(g_paddle_right_position) << std::endl;

    // collision detection
    // up and bottom
    if (g_ball_position.y >= HEIGHT_BOUND) {
        // put it back to the max position to avoid hanging
        g_ball_position.y = HEIGHT_BOUND - BALL_SIZE / 2;
        g_ball_movement.y *= -1;
    }
    else if (g_ball_position.y <= -HEIGHT_BOUND) {
        g_ball_position.y = -HEIGHT_BOUND + BALL_SIZE / 2;
        g_ball_movement.y *= -1;
    }
    // with the right paddle
    if (g_ball_position.x >= PADDLE_RIGHT_INIT_POS.x - PADDLE_WIDTH / 2 &&
        g_ball_position.y <= g_paddle_right_position.y + PADDLE_HEIGHT / 2 &&
        g_ball_position.y >= g_paddle_right_position.y - PADDLE_HEIGHT / 2) {
    //if (g_ball_position.x <= PADDLE_RIGHT_INIT_POS.x - PADDLE_WIDTH / 2) {

        // put the ball back on the paddle   
        //g_ball_position.x = PADDLE_RIGHT_INIT_POS.x - PADDLE_WIDTH / 2 - BALL_SIZE / 2;
        g_ball_movement.x *= -1;
        LOG("BOUNCE RIGHT!!\n\n");
    }
    if (g_ball_position.x <= PADDLE_LEFT_INIT_POS.x + PADDLE_WIDTH / 2 &&
        g_ball_position.y <= g_paddle_left_position.y + PADDLE_HEIGHT / 2 &&
        g_ball_position.y >= g_paddle_left_position.y - PADDLE_HEIGHT / 2) {
        g_ball_position.x = PADDLE_LEFT_INIT_POS.x + PADDLE_WIDTH / 2 + BALL_SIZE / 2;
        g_ball_movement.x *= -1;
        LOG("BOUNCE LEFT!!\n\n");

    }

    //float x_distance = fabs(g_flower_position.x - CUP_INIT_POS.x) - ((FLOWER_INIT_SCA.x + CUP_INIT_SCA.x) / 2.0f);
    //float y_distance = fabs(g_flower_position.y - CUP_INIT_POS.y) - ((FLOWER_INIT_SCA.y + CUP_INIT_SCA.y) / 2.0f);
    //std::cout << "x = " << g_ball_position.x << "\n";
    //std::cout << "paddle right BOUND " << PADDLE_RIGHT_INIT_POS.x - PADDLE_WIDTH / 2 << "\n";
    //std::cout << "paddle left BOUND " << PADDLE_LEFT_INIT_POS.x + PADDLE_WIDTH / 2 << "\n";
    std::cout << "y = " << g_ball_position.y << "\n";
    std::cout << "paddle right top bound =  " << g_paddle_right_position.y + PADDLE_HEIGHT / 2 << "\n";
    std::cout << "paddle right bottom bound =  " << g_paddle_right_position.y - PADDLE_HEIGHT / 2 << "\n";



    // model matrix reset
    g_paddle_left_matrix = glm::mat4(1.0f);
    g_paddle_right_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);

    // transformation
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
    g_ball_matrix = glm::scale(g_ball_matrix, BALL_INIT_SCALE);
    g_paddle_left_matrix = glm::translate(g_paddle_left_matrix, PADDLE_LEFT_INIT_POS);
    g_paddle_left_matrix = glm::translate(g_paddle_left_matrix, g_paddle_left_position);
    g_paddle_left_matrix = glm::scale(g_paddle_left_matrix, PADDLE_INIT_SCALE);
    g_paddle_right_matrix = glm::translate(g_paddle_right_matrix, PADDLE_RIGHT_INIT_POS);
    g_paddle_right_matrix = glm::translate(g_paddle_right_matrix, g_paddle_right_position);
    g_paddle_right_matrix = glm::scale(g_paddle_right_matrix, PADDLE_INIT_SCALE);
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
    draw_object(g_paddle_left_matrix, g_paddle_left_texture_id);
    draw_object(g_paddle_right_matrix, g_paddle_right_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);

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

    while (g_game_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}