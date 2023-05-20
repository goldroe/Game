#include "game.h"

void game_update_and_render(Game_Input *input, Game_State *state) {
    Game_Controller_Input *keyboard = &input->controllers[0];
    if (keyboard->move_up.ended_down) {
        state->player_p.y += 10.0f;
    }
    if (keyboard->move_down.ended_down) {
        state->player_p.y -= 10.0f;
    }
    if (keyboard->move_left.ended_down) {
        state->player_p.x -= 10.0f;
    } 
    if (keyboard->move_right.ended_down) {
        state->player_p.x += 10.0f;
    }
}
