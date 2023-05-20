#ifndef GAME_H
#define GAME_H

#include "common.h"
#include <HandmadeMath.h>

enum Game_Mode {
    GAME_MODE_WORLD,
    GAME_MODE_EDITOR,
};

struct Game_Button {
    int half_transition_count;
    bool ended_down;
};

union Game_Controller_Input {
    Game_Button buttons[4];
    struct {
        Game_Button move_up;
        Game_Button move_down;
        Game_Button move_left;
        Game_Button move_right;

        Game_Button terminator;
    };
};

struct Game_Input {
    f32 delta;
    Game_Controller_Input controllers[5];
};

struct Game_State {
    Game_Mode game_mode;

    HMM_Vec2 player_p;
    HMM_Vec2 player_dp;
};

#endif // GAME_H
