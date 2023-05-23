#ifndef GAME_H
#define GAME_H

#include "common.h"
#include <HandmadeMath.h>

struct Texture {
    unsigned int id;
    int width;
    int height;
};

struct Shader {
    unsigned int id;
};

struct Game_Button {
    int half_transition_count;
    bool ended_down;
};

union Game_Controller_Input {
    Game_Button buttons[5];
    struct {
        Game_Button move_up;
        Game_Button move_down;
        Game_Button move_left;
        Game_Button move_right;

        Game_Button editor_key;

        Game_Button terminator;
    };
};

struct Game_Input {
    f32 delta;
    int cursor_x;
    int cursor_y;
    Game_Controller_Input controllers[5];
};

struct Game_State {
    bool editor_mode;

    HMM_Vec2 player_p;
    HMM_Vec2 player_dp;
};

enum Entity_Type {
    ENTITY_NONE,
    ENTITY_PLAYER,
    ENTITY_TILE,
};

struct Entity {
    Entity_Type type;

    HMM_Vec2 position;
    HMM_Vec2 size;
    Texture texture;
};



#endif // GAME_H
