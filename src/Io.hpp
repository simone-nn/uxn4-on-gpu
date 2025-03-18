//
// Created by Andrei Ghita on 18.03.2025.
//

#ifndef IO_HPP
#define IO_HPP
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Uxn.hpp"

inline struct MouseState {
    bool used = false;
    uint16_t cursor_x, cursor_y;
    bool mouse1, mouse2, mouse3;
} mouse;

inline struct KeyboardState {
    bool used = false;
    uint8_t button;
    char8_t key;
} keyboard;

void keyboardInit();

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

void cursorPositionCallback(GLFWwindow* window, double x, double y);

void keyboardPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void mouseToUxnMemory(UxnMemory* memory);

void keyboardToUxnMemory(UxnMemory* memory);

#endif //IO_HPP
