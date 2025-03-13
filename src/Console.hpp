#ifndef CONSOLE_H
#define CONSOLE_H
#include <atomic>
#include <thread>
#include <optional>


class Console {
public:
    std::atomic<bool> running{false};
    std::queue<char> buffer;
    std::thread thread;
    std::mutex bufferMutex;

    void start();
    void stop();
    std::optional<char> pop();

private:
    void run();
};

#endif //CONSOLE_H
