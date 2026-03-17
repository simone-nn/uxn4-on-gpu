#ifndef UXN_H
#define UXN_H
#include <glm/glm.hpp>
#include "Resource.hpp"
#include "Console.hpp"
#include "EventQueue.hpp"
#include <string>
#include <array>
#include <chrono>

// Uxn sizes
#define UXN_RAM_SIZE 65536
#define UXN_STACK_SIZE 256
#define UXN_DEV_SIZE 256
// Uxn deviceFlags
#define DEO_FLAG         0x001
#define DEO_CONSOLE_FLAG 0x002
#define DEO_CERROR_FLAG  0x004
#define DEO_SCREENW_FLAG 0x008
#define DEO_SCREENH_FLAG 0x010
#define DEI_CONSOLE_FLAG 0x020
#define DRAW_PIXEL_FLAG  0x100
#define DRAW_SPRITE_FLAG 0x200

typedef struct uxn_memory {
    struct shared {
        uint16_t pc;
        uint8_t dev[UXN_DEV_SIZE];
        uint16_t flags;
        uint8_t halt;
    } shared;
    struct _private {
        uint8_t ram[UXN_RAM_SIZE];
        uint8_t wst[UXN_STACK_SIZE];  // working stack
        uint8_t pWst;
        uint8_t rst[UXN_STACK_SIZE];  // return stack
        uint8_t pRst;
    } _private;
    uxn_memory();
} UxnMemory;

enum class uxn_device: uint8_t {
    System = 0x0,
    Console = 0x10,
    Screen = 0x20,
    Audio = 0x30,
    Controller = 0x80,
    Mouse = 0x90,
    File = 0xA0,
    Datetime = 0xC0,
    Null = 0xff, // no device selected
};
constexpr std::array CALLBACK_DEVICES = {
    uxn_device::Screen,
    uxn_device::Console,
    uxn_device::Controller,
    uxn_device::Mouse,
};

char8_t from_uxn_mem(const uint8_t* p);

char16_t from_uxn_mem2(const uint8_t* p);

void to_uxn_mem(uint8_t c, uint8_t* p);

void to_uxn_mem2(uint16_t c, uint8_t* p);

bool mask(uint8_t x, uint8_t mask);

class Uxn {
public:
    UxnMemory* memory;
    Console* console;
    std::unordered_map<uxn_device,uint16_t> deviceCallbackVectors;
    bool debug{false};

    explicit Uxn(const char* program_path, Console *console, EventQueue *gpuEventQueue);

    ~Uxn();

    void reset();

    void outputToFile(const char* output_file_name, bool showRAM) const;

    void prepareCallback(uxn_device callback);

    void setDatetime() const;

    void handleUxnIO();

    void printBuffer();

    [[nodiscard]]
    bool programTerminated() const;

    [[nodiscard]]
    bool maskFlag(uint16_t mask) const;

    [[nodiscard]]
    glm::vec4 getBackgroundColor() const;
private:
    UxnMemory* original_memory;
    std::string program_path;
    std::vector<char> program_rom;
    std::string console_buffer;
    std::string cerror_buffer;
    EventQueue *gpuEventQueue;
};

#endif //UXN_H
