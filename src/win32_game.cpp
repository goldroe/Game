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

int main(int argc, char **argv) {
    QueryPerformanceFrequency(&win32_clock_frequency);
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

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_set_flip_vertically_on_load(true);

    int width, height, nchannels;
    unsigned char *data = stbi_load("data/jungle.png", &width, &height, &nchannels, 4);
    if (!data) {
        printf("Failed to load texture %s\n", "gold-brick.png");
    }
 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);


    float vertices[] = {
        // position     texcoord
        -0.5f, -0.5f,   0.0f, 0.0f, 
        -0.5f,  0.5f,   0.0f, 1.0f,
         0.5f,  0.5f,   1.0f, 1.0f,

        -0.5f, -0.5f,   0.0f, 0.0f,
         0.5f,  0.5f,   1.0f, 1.0f,
         0.5f, -0.5f,   1.0f, 0.0f,
   };

    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    const char *vshader_source = "#version 330 core\n"
        "layout (location = 0) in vec2 a_pos;\n"
        "layout (location = 1) in vec2 a_tex_coord;\n"
        "out vec2 tex_coord;\n"
        "uniform mat4 wvp;\n"
        "void main() {\n"
        "gl_Position = wvp * vec4(a_pos.x, a_pos.y, 0.0, 1.0);\n"
        "tex_coord = a_tex_coord;\n"
        "}\0";

    unsigned int vshader;
    vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vshader_source, NULL);
    glCompileShader(vshader);

    const char *fshader_source = "#version 330 core\n"
        "out vec4 frag_color;\n"
        "in vec2 tex_coord;\n"
        "uniform sampler2D our_texture;\n"
        "void main() {\n"
        "//frag_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n"
        "frag_color = texture(our_texture, tex_coord);\n"
        "}\0";

    unsigned int fshader;
    fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fshader_source, NULL);
    glCompileShader(fshader);

    unsigned int shader_program;
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vshader);
    glAttachShader(shader_program, fshader);
    glLinkProgram(shader_program);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
    glUseProgram(shader_program);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    Game_Input game_input{};
    Game_State game_state{};

    LARGE_INTEGER last_counter = win32_get_wall_clock();

    while (!glfwWindowShouldClose(window)) {
        Game_Controller_Input *keyboard = &game_input.controllers[0];
        win32_process_pending_messages(hwnd, keyboard); 

        HMM_Vec2 draw_region = win32_get_draw_rect(hwnd);

        game_update_and_render(&game_input, &game_state);

        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(vao);

        HMM_Mat4 world;
        HMM_Mat4 projection;
        HMM_Mat4 trans = HMM_Translate(HMM_V3(game_state.player_p.x, game_state.player_p.y, 0.0f));
        HMM_Mat4 scale = HMM_Scale(HMM_V3(900.0f, 900.0f, 1.0f));
        world = trans * scale;
        projection = HMM_Orthographic_RH_ZO(0.0f, draw_region.width, 0.0f, draw_region.height, 0.0f, 100.0f);
        HMM_Mat4 wvp = projection * world;

        unsigned int wvp_loc = glGetUniformLocation(shader_program, "wvp");
        glUniformMatrix4fv(wvp_loc, 1, GL_FALSE, (f32 *)&wvp);

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
