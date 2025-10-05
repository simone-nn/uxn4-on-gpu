#ifndef CONSOLE_H
#define CONSOLE_H
#include <atomic>
#include <thread>
#include <optional>
#include <queue>


class Console {
public:
    void start();
    void stop();
    std::optional<char> pop();
    bool notEmpty();

private:
    std::atomic<bool> running{false};
    std::queue<char> buffer;
    std::thread thread;
    std::mutex bufferMutex;

    void run();
};

#endif //CONSOLE_H
