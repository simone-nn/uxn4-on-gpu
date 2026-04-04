#include <chrono>
#include <iostream>
#include <cstdint>
#include <vector>

int main() {
    const uint32_t K = 100;
    const uint32_t J = 1000;
    const uint32_t INNER = 1000;
    const uint32_t dataCount = K * J * INNER;
    
    // Allocate and fill input buffer
    std::vector<uint32_t> inputs(dataCount);
    for (uint32_t i = 0; i < dataCount; i++) {
        inputs[i] = i;
    }

    volatile uint32_t result = 0;
    uint32_t temp;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (uint32_t k = 0; k < K; k++) {
        for (uint32_t j = 0; j < J; j++) {
            temp = 0;
            for (uint32_t i = 0; i < INNER; i++) {
                uint32_t readValue = inputs[(k * J + j) * INNER + i];
            #ifdef WITH_MEM
                temp += int((i * j) ^ (i + j) ^ readValue);
            #else
                temp += int((i * j) ^ (i + j));
            #endif
                temp = (temp * 1103515245 + 12345) & 0x7FFFFFFF;
            }
            result += temp;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    
    result += temp;
    asm volatile("" : "+r"(temp) : : "memory");

    double cpuTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "CPU Time: " << cpuTimeMs << " ms\n";
    std::cout << "Result: " << result << "\n";
    
    return 0;
}