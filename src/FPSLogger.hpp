//
// Created by Andrei Ghita on 26.03.2025.
//

#ifndef FPSLOGGER_HPP
#define FPSLOGGER_HPP
#include <chrono>

class FPSLogger {
public:
    void logFrame();

    void printMetrics() const;
private:
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    std::vector<double> frameTimes;

};


#endif //FPSLOGGER_HPP
