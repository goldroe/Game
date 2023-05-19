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

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glfwSwapInterval(1);

    HWND hwnd = glfwGetWin32Window(window);


    float vertices[] = {
        -0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f,

        -0.5f, -0.5f,
         0.5f,  0.5f,
         0.5f, -0.5f,
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
        "void main() {\n"
        "gl_Position = vec4(a_pos.x, a_pos.y, 0.0, 1.0);\n"
        "}\0";

    unsigned int vshader;
    vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vshader_source, NULL);
    glCompileShader(vshader);

    const char *fshader_source = "#version 330 core\n"
        "out vec4 frag_color;\n"
        "void main() {\n"
        "frag_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n"
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

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    LARGE_INTEGER last_counter = win32_get_wall_clock();

    while (!glfwWindowShouldClose(window)) {
        // glfwPollEvents();
        MSG message;
        while (PeekMessageA(&message, hwnd, NULL, NULL, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);


        LARGE_INTEGER end_counter = win32_get_wall_clock();

        float ms_elapsed = 1000.0f * win32_get_seconds_elapsed(last_counter, end_counter);
#if 1
        printf("MS: %f\n", ms_elapsed);
#endif

        last_counter = end_counter;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
