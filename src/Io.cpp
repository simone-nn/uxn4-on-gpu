#include "Io.hpp"


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
    //todo keyboardToUxnMemory
}
