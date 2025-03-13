#include "Console.hpp"
#include <iostream>
#include <chrono>

void Console::start() {
    if (running.load()) return;
    running.store(true);
    thread = std::thread(&Console::run, this);
}

void Console::stop() {
    if (!running.load()) return;
    //Console waits for one more input before closing
    std::cout << "Press any key to exit...\n"; // workaround
    running.store(false);
    if (thread.joinable()) {
        thread.join();
    }
}

void Console::run() {
    // capture input
    while (running.load()) {
        if (std::cin.peek() != EOF) {
            char ch;
            std::cin.get(ch);
            std::lock_guard lock(bufferMutex);
            buffer.push(ch);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::optional<char> Console::pop() {
    std::lock_guard lock(bufferMutex);
    if (buffer.empty()) {
        return std::nullopt;
    }
    char ch = buffer.front();
    buffer.pop();
    return ch;
}

bool Console::notEmpty() {
    std::lock_guard lock(bufferMutex);
    return !buffer.empty();
}