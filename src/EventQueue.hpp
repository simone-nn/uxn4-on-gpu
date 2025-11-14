#pragma once
#include <queue>
#include <optional>
#include <variant>

enum class GPUEventType { Resize};

struct ResizeData { bool isWidth; int value; };

struct GPUEvent {
    GPUEventType type;
    std::variant<ResizeData> data;
};

class EventQueue {
    std::queue<GPUEvent> events;
public:
    void push(GPUEvent event) {
        events.push(std::move(event));
    }
    
    std::optional<GPUEvent> pop() {
        if (events.empty()) return std::nullopt;
        GPUEvent event = std::move(events.front());
        events.pop();
        return event;
    }
};