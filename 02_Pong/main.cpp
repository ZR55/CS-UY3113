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

/* enums */
enum AppStatus {RUNNING, TERMINATED};

/* constants */
// The size of our literal game window
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

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
constexpr float PADDLE_WIDTH = 1.0f;
constexpr float PADDLE_HEIGHT = 630 / 498.0f;
constexpr glm::vec3 PADDLE_INIT_SCALE = glm::vec3(PADDLE_WIDTH, PADDLE_HEIGHT, 0.0f);
constexpr char BALL_SPRITE_FILEPATH[] = "assets/ball.png";
constexpr float BALL_SIZE = 0.3f;
constexpr glm::vec3 BALL_INIT_SCALE = glm::vec3(BALL_SIZE, BALL_SIZE, 0.0f);

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

//constexpr float ROT_INCREMENT_WATERMELON = 1.0f;

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
//float g_theta_watermelon = 0.0f,
//g_theta_bin_apple = 10.0f,
//g_theta_apple = 0.0f;

float g_paddle_left_x = -3.0f,
g_paddle_right_x = 3.0f,
g_paddle_y = 0.0f;

//float g_bin_size = BIN_SIZE;

glm::mat4 g_view_matrix,
g_paddle_left_matrix,
g_paddle_right_matrix,
g_ball_matrix,
g_projection_matrix;

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
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_game_status = TERMINATED;
        }
    }
}

void update() {
    // delta time
    float tick = SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = tick - g_previous_tick;
    g_previous_tick = tick;

    // game logic - accumulators
    //g_theta_watermelon += 1.5f * delta_time;
    //g_rotation_watermelon.z += ROT_INCREMENT_WATERMELON * delta_time;
    //g_theta_bin_apple += 1.7f * delta_time;
    //g_theta_apple += 4.5f * delta_time;

    // model matrix reset
    g_paddle_left_matrix = glm::mat4(1.0f);
    g_paddle_right_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);

    // transformation
    g_paddle_left_matrix = glm::translate(g_paddle_left_matrix, glm::vec3(g_paddle_left_x, g_paddle_y, 0.f));
    g_ball_matrix = glm::scale(g_ball_matrix, BALL_INIT_SCALE);
    //if (g_watermelon_status == ESCAPING) {
    //    // watermelon move in circle and rotate
    //    glm::vec3 watermelon_translation_vector = glm::vec3(glm::cos(g_theta_watermelon) * 2.5, glm::sin(g_theta_watermelon) * 2.5, 0.0f);
    //    g_paddle_right_matrix = glm::translate(g_paddle_right_matrix, watermelon_translation_vector);
    //    g_paddle_right_matrix = glm::rotate(g_paddle_right_matrix, g_rotation_watermelon.z, glm::vec3(0.0f, 0.0f, -1.0f));
    //    // bin moves in circle, chasing watermelon with slightly faster speed
    //    glm::vec3 bin_translation_vector = glm::vec3(glm::cos(g_theta_bin_apple) * 2.5, glm::sin(g_theta_bin_apple) * 2.5, 0.0f);
    //    g_ball_matrix = glm::translate(g_ball_matrix, bin_translation_vector);
    //    g_ball_matrix = glm::scale(g_ball_matrix, BIN_INIT_SCALE);
    //    // apple moves up and down inside of the bin
    //    glm::vec3 apple_translation_vector = bin_translation_vector + glm::vec3(0.0f, glm::sin(g_theta_apple) * 0.3, 0.0f);
    //    //g_apple_matrix = glm::translate(g_apple_matrix, bin_translation_vector);
    //    g_paddle_left_matrix = glm::translate(g_paddle_left_matrix, apple_translation_vector);

    //    // check if watermelon is caught
    //    if (watermelon_translation_vector.x + FRUIT_WIDTH / 2 <= bin_translation_vector.x + BIN_SIZE / 2 - 0.5f &&
    //        watermelon_translation_vector.x - FRUIT_WIDTH / 2 >= bin_translation_vector.x - BIN_SIZE / 2 + 0.5f &&
    //        watermelon_translation_vector.y + WATERMELON_HEIGHT / 2 <= bin_translation_vector.y + BIN_SIZE / 2 - 0.2f &&
    //        watermelon_translation_vector.y - WATERMELON_HEIGHT / 2 >= bin_translation_vector.y - BIN_SIZE / 2 + 0.2f) {
    //        g_watermelon_status = CAUGHT;

    //        std::cout << "CAUGHT!!!!!!" << '\n';

    //        g_bin_x = bin_translation_vector.x;
    //        g_bin_y = bin_translation_vector.y;
    //        g_apple_x = apple_translation_vector.x;
    //        g_apple_y = apple_translation_vector.y;
    //        g_watermelon_x = watermelon_translation_vector.x;
    //        g_watermelon_y = watermelon_translation_vector.y;
    //    }
    //}
    //else {
    //    // the bin with watermelon and apple move to the center
    //    float back_to_origin_time = 1000.f;
    //    float bin_increment_x = (0 - g_bin_x) / back_to_origin_time,
    //        bin_increment_y = (0 - g_bin_y) / back_to_origin_time,
    //        apple_increment_x = (0 - g_apple_x) / back_to_origin_time,
    //        apple_increment_y = (0 - g_apple_y) / back_to_origin_time,
    //        watermelon_increment_x = (0 - g_watermelon_x) / back_to_origin_time,
    //        watermelon_increment_y = (0 - g_watermelon_y) / back_to_origin_time;
    //    g_bin_x += bin_increment_x;
    //    g_bin_y += bin_increment_y;
    //    g_apple_x += apple_increment_x;
    //    g_apple_y += apple_increment_y;
    //    g_watermelon_x += watermelon_increment_x;
    //    g_watermelon_y += watermelon_increment_y;
    //    g_bin_size += 0.0005f;

    //    if (g_bin_size >= 6.f) {
    //        g_bin_size = 6.f;
    //    }

    //    g_ball_matrix = glm::translate(g_ball_matrix, glm::vec3(g_bin_x, g_bin_y, 0.f));

    //    // the bin scales up once it gets to the origin
    //    if (g_bin_x <= 0.05f && g_bin_y <= 0.05f) {
    //        g_ball_matrix = glm::scale(g_ball_matrix, glm::vec3(g_bin_size, g_bin_size, 0.f));
    //    }
    //    else {
    //        g_ball_matrix = glm::scale(g_ball_matrix, BIN_INIT_SCALE);
    //    }

    //    g_paddle_right_matrix = glm::translate(g_paddle_right_matrix, glm::vec3(g_watermelon_x, g_watermelon_y, 0.f));

    //    g_paddle_left_matrix = glm::translate(g_paddle_left_matrix, glm::vec3(g_apple_x, g_apple_y, 0.f));
    //}



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