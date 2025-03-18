#include <iostream>
#include <fstream>
#include "Uxn.hpp"
#include "DeviceController.hpp"

char8_t from_uxn_mem(const glm::uint* p) {
    return static_cast<char8_t>(*p);
}

char16_t from_uxn_mem2(const glm::uint* p) {
    return static_cast<char16_t>((*p << 8) + p[1]);
}

void to_uxn_mem(char8_t c, glm::uint* p) {
    *p = static_cast<glm::uint>(c);
}

void to_uxn_mem2(char16_t c, glm::uint* p) {
    // big endian
    p[0] = static_cast<glm::uint>((c >> 8) & 0xff);
    p[1] = static_cast<glm::uint>(c & 0xff);
}

uxn_memory::uxn_memory() = default;

Uxn::Uxn(const char *program_path, Console *console) {
    this->console = console;
    this->program_path = std::string(program_path);
    this->program_rom = readFile(program_path);

    if (program_rom.size() + 0x0100 * sizeof(glm::uint) > UXN_RAM_SIZE) {
        throw std::runtime_error("uxn program is bigger than uxn ram!");
    }

    // Padding the opcodes from 8-bit to 32-bit
    auto program = std::vector<glm::uint>(program_rom.size());
    for (size_t i = 0; i < program_rom.size(); i++) {
        to_uxn_mem(program_rom[i], &program[i]);
    }

    this->memory = new UxnMemory();
    // copy the program into memory
    memcpy(memory->_private.ram + 0x0100, program.data(), program.size() * sizeof(glm::uint));
    // set the program counter to where the program starts from
    memory->shared.pc = 0x0100;

    this->original_memory = new UxnMemory();
    memcpy(original_memory, memory, sizeof(UxnMemory));
}

Uxn::~Uxn() {
    delete memory;
    delete original_memory;
}

void Uxn::reset() {
    memory = original_memory;
}

void Uxn::outputToFile(const char* output_file_name, bool showRAM) const {
    #define printValue(i, arr) if (arr[i]!=0x00000000) { outFile << "[0x" << i << "]: 0x" << arr[i] << "\n"; }

    std::ofstream outFile(output_file_name, std::ios::out | std::ios::app);
    // Check if the file opened successfully
    if (!outFile) {
        std::string message = "Failed to open file: ";
        message += output_file_name;
        throw std::runtime_error(message);
    }
    outFile << std::hex;
    outFile << "---Uxn Memory:---\n";
    outFile << "Program Counter: 0x" << memory->shared.pc;
    if (memory->shared.pc != 0) {
        glm::uint instr = from_uxn_mem(&memory->_private.ram[memory->shared.pc - 1]);
        outFile << " prev instruction:" << instr;
    }
    outFile << "\nFlags: 0x" << memory->shared.flags << "\n";
    if (showRAM) {
        outFile << "--RAM:--\n";
        for (int i = 0; i < UXN_RAM_SIZE; ++i) {
            printValue(i, memory->_private.ram);
        }
    }
    outFile << "--Working Stack:--\n";
    outFile << "wst pointer: " << memory->_private.pWst << "\n";
    for (int i = 0; i < UXN_STACK_SIZE; ++i) {
        printValue(i, memory->_private.wst);
    }
    outFile << "--Return Stack:--\n";
    outFile << "rst pointer: " << memory->_private.pRst << "\n";
    for (int i = 0; i < UXN_STACK_SIZE; ++i) {
        printValue(i, memory->_private.rst);
    }
    outFile << "--Device Data:--\n";
    for (int i = 0; i < UXN_DEV_SIZE; ++i) {
        printValue(i, memory->shared.dev);
    }
    outFile << std::dec << "\n\n";

    outFile.close();
    std::cout << "[Log] Memory to file: " << output_file_name << "\n";
}

void Uxn::prepareCallback(uxn_device callback) {
    memory->shared.pc = deviceCallbackVectors.at(callback);
    switch (callback) {
        case uxn_device::Console: {
            std::optional<char> c = console->pop();
            if (c.has_value()) {
                to_uxn_mem(c.value(), &memory->shared.dev[0x12]);
            }
        } break;
        default: break;
    }
    // always copy mouse data
    to_uxn_mem2(mouse.cursor_x, &memory->shared.dev[0x92]);
    to_uxn_mem2(mouse.cursor_y, &memory->shared.dev[0x94]);
    int uxn_button = 0;
    if (mouse.mouse1) uxn_button += 1;
    if (mouse.mouse2) uxn_button += 2;
    if (mouse.mouse3) uxn_button += 4;
    to_uxn_mem(uxn_button, &memory->shared.dev[0x96]);

    // copy datetime data
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&now_c);

    to_uxn_mem2(static_cast<uint16_t>(localTime.tm_year + 1900), &memory->shared.dev[0xc0]);
    to_uxn_mem(static_cast<uint8_t>(localTime.tm_mon + 1), &memory->shared.dev[0xc2]);
    to_uxn_mem(static_cast<uint8_t>(localTime.tm_mday), &memory->shared.dev[0xc3]);
    to_uxn_mem(static_cast<uint8_t>(localTime.tm_hour), &memory->shared.dev[0xc4]);
    to_uxn_mem(static_cast<uint8_t>(localTime.tm_min), &memory->shared.dev[0xc5]);
    to_uxn_mem(static_cast<uint8_t>(localTime.tm_sec), &memory->shared.dev[0xc6]);
    to_uxn_mem(static_cast<uint8_t>(localTime.tm_wday), &memory->shared.dev[0xc7]);
    to_uxn_mem2(static_cast<uint16_t>(localTime.tm_yday + 1), &memory->shared.dev[0xc8]);
    to_uxn_mem(localTime.tm_isdst > 0, &memory->shared.dev[0xca]);
}

void Uxn::handleUxnIO() {
    if (maskFlag(DEO_CONSOLE_FLAG)) {
        // console output
        char8_t c = from_uxn_mem(&memory->shared.dev[0x18]);
        console_buffer.push_back(static_cast<char>(c));
        if (c == 0x0a) { printBuffer(); }
        return;
    }
    if (maskFlag(DEO_CERROR_FLAG)) {
        // console error output
        char8_t c = from_uxn_mem(&memory->shared.dev[0x19]);
        cerror_buffer.push_back(static_cast<char>(c));
        if (c == 0x0a) {
            std::cerr << "[ERROR] " << cerror_buffer;
            cerror_buffer.clear();
        }
        return;
    }
    // callbacks
    if (maskFlag(DEO_FLAG)) {
        for (uxn_device device : CALLBACK_DEVICES) {
            if (uint16_t addr = from_uxn_mem2(&memory->shared.dev[static_cast<glm::uint>(device)])) {
                if (!deviceCallbackVectors.contains(device)) {
                    std::cout << "Adding new callback device: " << static_cast<int>(device) << "\n";
                }
                deviceCallbackVectors.insert({device, addr});
                // start the Console class
                if (device == uxn_device::Console) {
                    console->start();
                }
            }
        }
        return;
    }
}

void Uxn::printBuffer() {
    if (debug) {
        std::cout << "[CONSOLE] " << console_buffer;
    } else {
        std::cout << console_buffer;
    }
    console_buffer.clear();
}

bool Uxn::programTerminated() const {
    return static_cast<int8_t>(from_uxn_mem(&memory->shared.dev[0x0f])) != 0;
}

bool Uxn::maskFlag(glm::uint mask) const {
    return (memory->shared.flags & mask) == mask;
}

glm::vec4 Uxn::getBackgroundColor() const {
    constexpr int shift = 12;
    constexpr float f = 8;
    auto r = static_cast<float>((from_uxn_mem2(&memory->shared.dev[0x08]) >> shift) & 0xf);
    auto g = static_cast<float>((from_uxn_mem2(&memory->shared.dev[0x0a]) >> shift) & 0xf);
    auto b = static_cast<float>((from_uxn_mem2(&memory->shared.dev[0x0c]) >> shift) & 0xf);
    return {r / f, g / f, b / f, 1.0f};
}
