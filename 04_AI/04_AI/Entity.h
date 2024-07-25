#ifndef ENTITY_H
#define ENTITY_H

#include "Map.h"
#include "glm/glm.hpp"
#include "ShaderProgram.h"
enum EntityType { PLATFORM, PLAYER, ENEMY  };
enum AIType     { WALKER, GUARD, FLYER, SHOOTER, BULLET, NOTYPE };
enum AIState    { WALKING, IDLE, ATTACKING, NOSTATE };


enum AnimationDirection { LEFT, RIGHT, UP, DOWN };
enum EnemyAnimation { MOVE_LEFT, MOVE_RIGHT, DIE_LEFT, DIE_RIGHT };

//enum GameResult {NONE, WIN, LOSE};

class Entity
{
private:
    bool m_is_active = true;
    
    int m_animation[4][4]; // 4x4 array for walking animations

    
    EntityType m_entity_type;
    AIType     m_ai_type;
    AIState    m_ai_state;
    // ————— TRANSFORMATIONS ————— //
    glm::vec3 m_movement;
    glm::vec3 m_position;
    glm::vec3 m_scale;
//    glm::vec3 m_rotation;
//    glm::vec3 m_rotation_direction;
    glm::vec3 m_rotation_center;
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;

    glm::mat4 m_model_matrix;

    float     m_speed,
              m_jumping_power,
    m_rotation_theta = 179.8f;
    
    bool m_is_jumping = false;

    // ————— TEXTURES ————— //
    GLuint    m_texture_id;

    // ————— ANIMATION ————— //
    int m_animation_cols;
    int m_animation_frames,
        m_animation_index,
        m_animation_rows;

    int* m_animation_indices = nullptr;
    float m_animation_time = 0.0f;

    float m_width = 1.0f,
          m_height = 1.0f;
    // ————— COLLISIONS ————— //
    int m_enemy_count = 0;
    bool m_collided_top    = false;
    bool m_collided_bottom = false;
    bool m_collided_left   = false;
    bool m_collided_right  = false;
    
    bool m_map_collided_top    = false;
    bool m_map_collided_bottom = false;
    bool m_map_collided_left   = false;
    bool m_map_collided_right  = false;
    

public:
    // ————— STATIC VARIABLES ————— //
    static constexpr int SECONDS_PER_FRAME = 6;
    static constexpr int ANIMATION_ARRAY_LENGTH = 4;
    static constexpr float ROT_INCREMENT = 1.0f;
    
//    static bool shooter_is_active;
//    GameResult game_result = NONE;

    // ————— METHODS ————— //
    Entity();
    Entity(GLuint texture_id, float speed, glm::vec3 acceleration, float jump_power, int animation[4][4], float animation_time,
        int animation_frames, int animation_index, int animation_cols,
           int animation_rows, float width, float height, EntityType EntityType, AIType AIType, AIState AIState);
    Entity(GLuint texture_id, float speed, float width, float height, EntityType EntityType); // Simpler constructor
    Entity(GLuint texture_id, float speed, float width, float height, EntityType EntityType, AIType AIType, AIState AIState); // AI constructor
    ~Entity();

    void draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index);
    bool const check_collision(Entity* other) const;
    
    void const check_collision_y(Entity* collidable_entities, int collidable_entity_count);
    void const check_collision_x(Entity* collidable_entities, int collidable_entity_count);
    
    // Overloading our methods to check for only the map
    void const check_collision_y(Map *map);
    void const check_collision_x(Map *map);
    
    void update(float delta_time, Entity *player, Entity *collidable_entities, int collidable_entity_count, Map *map);
    void render(ShaderProgram* program);

    void ai_activate(Entity *player);
    void ai_bullet();
    void ai_guard(Entity *player);
    void ai_fly();
    
    void normalise_movement() { m_movement = glm::normalize(m_movement); }

    void face_left() {
        if (m_entity_type == PLAYER) m_animation_indices = m_animation[LEFT];
        if (m_entity_type == ENEMY) {
            m_animation_indices = m_animation[MOVE_LEFT];
            if (m_ai_type == FLYER) std::cout << "set to face left\n";
//            for (int i = 0; i < 4; i++) std::cout << m_animation_indices[i] << "\n";
        }
    }
    void face_right() {
        if (m_entity_type == PLAYER) m_animation_indices = m_animation[RIGHT];
        if (m_entity_type == ENEMY) 
        {m_animation_indices = m_animation[MOVE_RIGHT];
            if (m_ai_type == FLYER) std::cout << "set to face right\n";
            for (int i = 0; i < 4; i++) std::cout << m_animation_indices[i] << "\n";
        }
    }
    void face_up() { m_animation_indices = m_animation[UP]; }
    void face_down() { m_animation_indices = m_animation[DOWN]; }

    void move_left() { m_movement.x = -1.0f; face_left(); }
    void move_right() { m_movement.x = 1.0f;  face_right(); }
    void move_up() { m_movement.y = 1.0f;  face_up(); }
    void move_down() { m_movement.y = -1.0f; face_down(); }
    
    void die_right() { if (m_entity_type == ENEMY) m_animation_indices = m_animation[DIE_RIGHT]; }
    void die_left() { if (m_entity_type == ENEMY) m_animation_indices = m_animation[DIE_LEFT]; }
    
    void const jump() { m_is_jumping = true; }

    // ————— GETTERS ————— //
    EntityType const get_entity_type()    const { return m_entity_type;   };
    AIType     const get_ai_type()        const { return m_ai_type;       };
    AIState    const get_ai_state()       const { return m_ai_state;      };
    glm::vec3 const get_position()     const { return m_position; }
    glm::vec3 const get_velocity()     const { return m_velocity; }
    glm::vec3 const get_acceleration() const { return m_acceleration; }
    glm::vec3 const get_movement()     const { return m_movement; }
    glm::vec3 const get_scale()        const { return m_scale; }
    GLuint    const get_texture_id()   const { return m_texture_id; }
    float     const get_speed()        const { return m_speed; }
    bool      const get_collided_top() const { return m_collided_top; }
    bool      const get_collided_bottom() const { return m_collided_bottom; }
    bool      const get_collided_right() const { return m_collided_right; }
    bool      const get_collided_left() const { return m_collided_left; }
    int       const get_enemy_count() const { return m_enemy_count; }
    bool      const get_map_collided_top() const { return m_map_collided_top; }
    bool      const get_map_collided_bottom() const { return m_map_collided_bottom; }
    bool      const get_map_collided_right() const { return m_map_collided_right; }
    bool      const get_map_collided_left() const { return m_map_collided_left; }
    bool      const get_activation_status() const { return m_is_active; }
    
    void activate()   { m_is_active = true;  };
    void deactivate() { m_is_active = false; };
    // ————— SETTERS ————— //
    void const set_entity_type(EntityType new_entity_type)  { m_entity_type = new_entity_type;};
    void const set_ai_type(AIType new_ai_type){ m_ai_type = new_ai_type;};
    void const set_ai_state(AIState new_state){ m_ai_state = new_state;};
    void const set_position(glm::vec3 new_position) {
        if (m_ai_type == FLYER) m_rotation_center = new_position;
        else m_position = new_position;
    }
    void const set_velocity(glm::vec3 new_velocity) { m_velocity = new_velocity; }
    void const set_acceleration(glm::vec3 new_acceleration) { m_acceleration = new_acceleration; }
    void const set_movement(glm::vec3 new_movement) { m_movement = new_movement; }
    void const set_scale(glm::vec3 new_scale) { m_scale = new_scale; }
    void const set_texture_id(GLuint new_texture_id) { m_texture_id = new_texture_id; }
    void const set_speed(float new_speed) { m_speed = new_speed; }
    void const set_animation_cols(int new_cols) { m_animation_cols = new_cols; }
    void const set_animation_rows(int new_rows) { m_animation_rows = new_rows; }
    void const set_animation_frames(int new_frames) { m_animation_frames = new_frames; }
    void const set_animation_index(int new_index) { m_animation_index = new_index; }
    void const set_animation_time(float new_time) { m_animation_time = new_time; }
    void const set_jumping_power(float new_jumping_power) { m_jumping_power = new_jumping_power;}
    void const set_width(float new_width) {m_width = new_width; }
    void const set_height(float new_height) {m_height = new_height; }
    void const set_enemy_count(int new_enemy_count) {m_enemy_count = new_enemy_count;}

    // Setter for m_animation
    void set_animation(int animation[4][4])
    {
        if (m_ai_type == FLYER) std::cout << "setting m_animation\n";
        for (int i = 0; i < 4; ++i)
        {
            if (m_ai_type == FLYER) std::cout << i << ": ";
            for (int j = 0; j < 4; ++j)
            {
                m_animation[i][j] = animation[i][j];
                if (m_ai_type == FLYER) std::cout <<m_animation[i][j];
            }
            if (m_ai_type == FLYER) std::cout << "\n";
        }
    }
    
//    // setter for enemy animation
//    void set_enemy_animation(int enemy_animation[4][4])
//    {
//        for (int i = 0; i < 4; ++i)
//        {
//            for (int j = 0; j < 4; ++j)
//            {
//                m_enemy_animation[i][j] = enemy_animation[i][j];
//            }
//        }
//    }
};

#endif // ENTITY_H
