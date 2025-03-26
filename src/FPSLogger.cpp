#include "FPSLogger.hpp"
#include <iostream>

void FPSLogger::logStart() {
    programStartTime = std::chrono::high_resolution_clock::now();
}

void FPSLogger::logEnd() {
    programEndTime = std::chrono::high_resolution_clock::now();
}

void FPSLogger::logFrame() {
    auto now = std::chrono::high_resolution_clock::now();

    if (lastFrameTime.time_since_epoch().count() != 0) {
        std::chrono::duration<double, std::milli> frameTime = now - lastFrameTime;
        frameTimes.push_back(frameTime.count());
    }

    lastFrameTime = now;
}

void FPSLogger::printMetrics() const {
    // Program duration
    double programDurationMs = 0.0;
    if (programStartTime.time_since_epoch().count() != 0 &&
        programEndTime.time_since_epoch().count() != 0) {
        std::chrono::duration<double, std::milli> duration = programEndTime - programStartTime;
        programDurationMs = duration.count();
        }

    std::cout << "Program duration: " << programDurationMs << " ms\n";

    if (frameTimes.empty()) {
        std::cout << "Not enough frame data to calculate metrics.\n";
        return;
    }

    double total = 0.0;
    double minTime = std::numeric_limits<double>::max();
    double maxTime = std::numeric_limits<double>::min();

    for (double time : frameTimes) {
        total += time;
        if (time < minTime) minTime = time;
        if (time > maxTime) maxTime = time;
    }

    double averageFrameTime = total / static_cast<double>(frameTimes.size());
    double averageFPS = 1000.0 / averageFrameTime;

    std::cout << "Average FPS: " << averageFPS << "\n";
    std::cout << "Minimum frame time: " << minTime << " ms\n";
    std::cout << "Maximum frame time: " << maxTime << " ms\n";
}

