#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include <Windows.h>

// openGL handling
#include <glad/glad.h>
#include <GLFW/glfw3.h> 
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3native.h>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "common.h"

#include "game.cpp"


const int WIDTH = 1600;
const int HEIGHT = 900;

LARGE_INTEGER win32_clock_frequency;

LARGE_INTEGER win32_get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

static float win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    return (float)(end.QuadPart - start.QuadPart) / (float)win32_clock_frequency.QuadPart;
}

HMM_Vec2 win32_get_draw_rect(HWND hwnd) {
    HMM_Vec2 result;
    RECT rc;
    GetClientRect(hwnd, &rc);
    result.width = (float)(rc.right - rc.left);
    result.height = (float)(rc.bottom - rc.top);
    return result;
}

void win32_process_pending_messages(HWND hwnd, Game_Controller_Input *keyboard) {
    MSG msg;
    while (PeekMessageA(&msg, hwnd, NULL, NULL, PM_REMOVE)) {
        switch (msg.message) {
        case WM_KEYUP:
        case WM_KEYDOWN: {
            bool is_down = ((1l << 31) & msg.lParam) == 0;
            bool was_down = ((1l << 30) & msg.lParam) == 1;

            switch (msg.wParam) {
            case 'W':
            case VK_UP:
                keyboard->move_up.ended_down = is_down;
                break;
            case 'A':
            case VK_LEFT:
                keyboard->move_left.ended_down = is_down;
                break;
            case 'S':
            case VK_DOWN:
                keyboard->move_down.ended_down = is_down;
                break;
            case 'D':
            case VK_RIGHT:
                keyboard->move_right.ended_down = is_down;
                break;
            }
        } break;

        default:
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
       }
    }
}

struct Texture {
    unsigned int id;
    int width;
    int height;
};

Texture GL_load_texture(char *texture_path) {
    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nchannels;
    unsigned char *data = stbi_load(texture_path, &width, &height, &nchannels, 4);
    if (!data) {
        printf("Failed to load texture %s\n", texture_path);
    }
 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);

    Texture texture{};
    texture.id = texture_id;
    texture.width = width;
    texture.height = height;
    return texture;
}

int main(int argc, char **argv) {
    QueryPerformanceFrequency(&win32_clock_frequency);
    stbi_set_flip_vertically_on_load(true);

    if (!glfwInit()) {
        printf("Could not initialize glfw\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Game", NULL, NULL);

    if (!window) {
        printf("Failed to create window!\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Could not initialize glad\n");
        return -1;
    }

    int wwidth, wheight;
    glfwGetFramebufferSize(window, &wwidth, &wheight);
    glViewport(0, 0, wwidth, wheight);

    glfwSwapInterval(1);

    HWND hwnd = glfwGetWin32Window(window);

    Texture tile_sheet = GL_load_texture("data/jungle.png");
    Texture player_sheet = GL_load_texture("data/player.png");


    float rect_vertices[] = {
        // position
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,

        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };

    unsigned int rect_vao;
    glGenVertexArrays(1, &rect_vao);
    glBindVertexArray(rect_vao);

    unsigned int rect_vbo;
    glGenBuffers(1, &rect_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(rect_vertices), rect_vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    const char *rect_vsource =
        "#version 330 core\n"
        "layout (location = 0) in vec2 a_pos;\n"
        "uniform mat4 wvp;\n"
        "void main() {\n"
        "gl_Position = wvp * vec4(a_pos.x, a_pos.y, 0.0f, 1.0f);\n"
        "}\0";

    const char *rect_fsource =
        "#version 330 core\n"
        "out vec4 frag_color;\n"
        "uniform vec3 our_color;\n"
        "void main() {\n"
        "frag_color = vec4(our_color.r, our_color.g, our_color.b, 1.0);\n"
        "}\0";

    int status = 0;

    unsigned int rect_vshader;
    rect_vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(rect_vshader, 1, &rect_vsource, NULL);
    glCompileShader(rect_vshader);

    glGetShaderiv(rect_vshader, GL_COMPILE_STATUS, &status);
    if (!status) {
        printf("Failed to compile vertex shader!\n");
    }

    unsigned int rect_fshader;
    rect_fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(rect_fshader, 1, &rect_fsource, NULL);
    glCompileShader(rect_fshader);

    glGetShaderiv(rect_fshader, GL_COMPILE_STATUS, &status);
    if (!status) {
        printf("Failed to compile vertex shader!\n");
    }

    unsigned int rect_program;
    rect_program = glCreateProgram();
    glAttachShader(rect_program, rect_vshader);
    glAttachShader(rect_program, rect_fshader);
    glLinkProgram(rect_program);
    glDeleteShader(rect_vshader);
    glDeleteShader(rect_fshader);
    glUseProgram(rect_program);



    float quad_vertices[] = {
        // position     texcoord
        0.0f, 0.0f,     0.0f, 0.0f,
        0.0f, 1.0f,     0.0f, 1.0f,
        1.0f, 1.0f,     1.0f, 1.0f,

        0.0f, 0.0f,     0.0f, 0.0f,
        1.0f, 1.0f,     1.0f, 1.0f,
        1.0f, 0.0f,     1.0f, 0.0f,
   };

    unsigned int quad_vao;
    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    unsigned int quad_vbo;
    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));


    const char *quad_vsource =
        "#version 330 core\n"
        "layout (location = 0) in vec2 a_pos;\n"
        "layout (location = 1) in vec2 a_tex_coord;\n"
        "out vec2 tex_coord;\n"
        "uniform mat4 wvp;\n"
        "void main() {\n"
        "gl_Position = wvp * vec4(a_pos.x, a_pos.y, 0.0, 1.0);\n"
        "tex_coord = a_tex_coord;\n"
        "}\0";

    const char *quad_fsource = 
        "#version 330 core\n"
        "out vec4 frag_color;\n"
        "in vec2 tex_coord;\n"
        "uniform sampler2D our_texture;\n"
        "uniform vec4 atlas_trans;\n"
        "void main() {\n"
        "vec2 coords = tex_coord.xy * atlas_trans.zw + atlas_trans.xy;\n"
        "vec4 tex_color = texture(our_texture, coords);\n"
        "if (tex_color.a < 0.1) discard;\n"
        "frag_color = tex_color;\n"
        "}\0";

    unsigned int quad_vshader;
    quad_vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(quad_vshader, 1, &quad_vsource, NULL);
    glCompileShader(quad_vshader);

    unsigned int quad_fshader;
    quad_fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(quad_fshader, 1, &quad_fsource, NULL);
    glCompileShader(quad_fshader);

    unsigned int quad_program;
    quad_program = glCreateProgram();
    glAttachShader(quad_program, quad_vshader);
    glAttachShader(quad_program, quad_fshader);
    glLinkProgram(quad_program);
    glDeleteShader(quad_vshader);
    glDeleteShader(quad_fshader);
    glUseProgram(quad_program);

    Game_Input game_input{};
    Game_State game_state{};
    
    const s32 tiles_x = 16;
    const s32 tiles_y = 9;
    s32 tiles[tiles_y][tiles_x] = {};
    s32 *tile_map = (s32 *)tiles;
    for (int x = 0, y = 0; x < tiles_x; x++) {
        tile_map[y * tiles_x + x] = 1;
    }

    LARGE_INTEGER last_counter = win32_get_wall_clock();
    while (!glfwWindowShouldClose(window)) {
        Game_Controller_Input *keyboard = &game_input.controllers[0];
        win32_process_pending_messages(hwnd, keyboard); 

        HMM_Vec2 draw_region = win32_get_draw_rect(hwnd);

        game_update_and_render(&game_input, &game_state);

        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(quad_program);
        glBindVertexArray(quad_vao);

        HMM_Vec2 player_size = HMM_V2(100.0f, 100.0f);
        HMM_Vec2 tile_size = HMM_V2(300.0f, 100.0f);

        HMM_Mat4 projection = HMM_Orthographic_RH_ZO(0.0f, draw_region.width, 0.0f, draw_region.height, 0.0f, 100.0f);

        HMM_Mat4 world;
        HMM_Mat4 trans;
        HMM_Mat4 scale;
        HMM_Mat4 wvp;

        // draw tiles
        trans = HMM_Translate(HMM_V3(0.0f, 0.0f, 0.0f));
        scale = HMM_Scale(HMM_V3(tile_size.width, tile_size.height, 1.0f));
        world = trans * scale;
        wvp = projection * world;

        HMM_Vec4 atlas_trans = HMM_V4(0.0f / tile_sheet.width, 176.0f / (float)tile_sheet.height, 48.0f/tile_sheet.width, 16.0f / tile_sheet.height);

        unsigned int wvp_loc = glGetUniformLocation(quad_program, "wvp");
        unsigned int atlas_loc = glGetUniformLocation(quad_program, "atlas_trans");

        glUniformMatrix4fv(wvp_loc, 1, GL_FALSE, (f32 *)&wvp);
        glUniform4fv(atlas_loc, 1, (f32 *)&atlas_trans);

        glBindTexture(GL_TEXTURE_2D, tile_sheet.id);

        
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // draw player
        trans = HMM_Translate(HMM_V3(game_state.player_p.x, game_state.player_p.y, 0.0f));
        scale = HMM_Scale(HMM_V3(player_size.width, player_size.height, 1.0f));
        world = trans * scale;
        wvp = projection * world;

        atlas_trans = HMM_V4(48.0f / player_sheet.width, 0.0f / player_sheet.height, 16.0f / player_sheet.width, 16.0f / player_sheet.height);
        glUniformMatrix4fv(wvp_loc, 1, GL_FALSE, (f32 *)&wvp);
        glUniform4fv(atlas_loc, 1, (f32 *)&atlas_trans);

        glBindTexture(GL_TEXTURE_2D, player_sheet.id);

        glDrawArrays(GL_TRIANGLES, 0, 6);


        // draw rect
        glUseProgram(rect_program);
        glBindVertexArray(rect_vao);

        trans = HMM_Translate(HMM_V3(0.0f, 0.0f, 0.0f));
        scale = HMM_Scale(HMM_V3(100.0f, 5.0f, 1.0f));
        world = trans * scale;
        wvp = projection * world;

        HMM_Vec3 rect_color = HMM_V3(1.0f, 1.0f, 0.0f);
        unsigned int rect_wvp_loc = glGetUniformLocation(rect_program, "wvp");
        unsigned int rect_color_loc = glGetUniformLocation(rect_program, "our_color");
        glUniformMatrix4fv(rect_wvp_loc, 1, GL_FALSE, (f32 *)&wvp);
        glUniform3fv(rect_color_loc, 1, (f32 *)&rect_color);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        trans = HMM_Translate(HMM_V3(0.0f, 100.0f, 0.0f));
        scale = HMM_Scale(HMM_V3(100.0f, 5.0f, 1.0f));
        world = trans * scale;
        wvp = projection * world;
        glUniformMatrix4fv(rect_wvp_loc, 1, GL_FALSE, (f32 *)&wvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        trans = HMM_Translate(HMM_V3(0.0f, 0.0f, 0.0f));
        scale = HMM_Scale(HMM_V3(5.0f, 100.0f, 1.0f));
        world = trans * scale;
        wvp = projection * world;
        glUniformMatrix4fv(rect_wvp_loc, 1, GL_FALSE, (f32 *)&wvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        trans = HMM_Translate(HMM_V3(100.0f - 5.0f, 0.0f, 0.0f));
        scale = HMM_Scale(HMM_V3(5.0f, 100.0f, 1.0f));
        world = trans * scale;
        wvp = projection * world;
        glUniformMatrix4fv(rect_wvp_loc, 1, GL_FALSE, (f32 *)&wvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);


        LARGE_INTEGER end_counter = win32_get_wall_clock();

        float ms_elapsed = 1000.0f * win32_get_seconds_elapsed(last_counter, end_counter);
#if 0
        printf("MS: %f\n", ms_elapsed);
#endif

        last_counter = end_counter;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
