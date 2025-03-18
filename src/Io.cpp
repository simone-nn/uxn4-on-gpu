#include "Io.hpp"

#include <iostream>

#define KEY_A      0x01
#define KEY_B      0x02
#define KEY_SELECT 0x04
#define KEY_HOME   0x08
#define KEY_UP     0x10
#define KEY_DOWN   0x20
#define KEY_LEFT   0x40
#define KEY_RIGHT  0x80

std::unordered_map<int, uint8_t> keymap;

void keyboardInit() {
    // A
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_LEFT_CONTROL), KEY_A});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_RIGHT_CONTROL), KEY_A});
    // B
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_LEFT_ALT), KEY_B});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_RIGHT_ALT), KEY_B});
    // Select
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_LEFT_SHIFT), KEY_SELECT});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_RIGHT_SHIFT), KEY_SELECT});
    // Home
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_HOME), KEY_HOME});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_ENTER), KEY_HOME});
    // Up
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_UP), KEY_UP});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_W), KEY_UP});
    // Down
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_DOWN), KEY_DOWN});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_S), KEY_DOWN});
    // Left
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_LEFT), KEY_LEFT});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_A), KEY_LEFT});
    // Right
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_RIGHT), KEY_RIGHT});
    keymap.insert({glfwGetKeyScancode(GLFW_KEY_D), KEY_RIGHT});
}


// --- --- GLFW Event Callback Functions --- ---
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // button = 0 when left click, button = 1 when right click
    // todo this function sucks
    mouse.used = true;
    mouse.mouse1 = false; mouse.mouse2 = false; mouse.mouse3 = false;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:   mouse.mouse1 = true; break;
        case GLFW_MOUSE_BUTTON_RIGHT:  mouse.mouse2 = true; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: mouse.mouse3 = true; break;
        default: break;
    }
}

void cursorPositionCallback(GLFWwindow* window, double x, double y) {
    // values are from 0 to WIDTH, HEIGHT;  0,0 top left
    mouse.cursor_x = static_cast<uint16_t>(x);
    mouse.cursor_y = static_cast<uint16_t>(y);
}

void keyboardPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_RELEASE) { return; }
    keyboard.used = true;
    if (auto pKey = glfwGetKeyName(key, scancode)) keyboard.key = *pKey;
    if (keymap.contains(scancode)) {keyboard.button = keymap.at(scancode);}
}

void mouseToUxnMemory(UxnMemory* memory) {
    // always copy mouse data
    to_uxn_mem2(mouse.cursor_x, &memory->shared.dev[0x92]);
    to_uxn_mem2(mouse.cursor_y, &memory->shared.dev[0x94]);
    if (mouse.used) {
        int uxn_button = 0;
        if (mouse.mouse1) uxn_button += 1;
        if (mouse.mouse2) uxn_button += 2;
        if (mouse.mouse3) uxn_button += 4;
        to_uxn_mem(uxn_button, &memory->shared.dev[0x96]);
        mouse.used = false;
    } else {
        to_uxn_mem(0, &memory->shared.dev[0x96]);
    }
}

void keyboardToUxnMemory(UxnMemory* memory) {
    if (keyboard.used) {
        to_uxn_mem(keyboard.button, &memory->shared.dev[0x82]);
        to_uxn_mem(keyboard.key, &memory->shared.dev[0x83]);
        keyboard.used = false;
    }
}
