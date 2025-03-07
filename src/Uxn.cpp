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

Uxn::Uxn(const char *program_path) {
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

void Uxn::handleUxnIO() {
    // console
    if (maskFlag(DEO_CONSOLE_FLAG)) {
        // console output
        char8_t c = from_uxn_mem(&memory->shared.dev[0x18]);
        console_buffer.push_back(static_cast<char>(c));
        if (c == 0x0a) {
            std::cout << "[CONSOLE] " << console_buffer;
            console_buffer.clear();
        }
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
    if (maskFlag(DEI_CONSOLE_FLAG)) {
        // todo handle console device read
        //https://wiki.xxiivv.com/site/varvara.html#console
        return;
    }
    // callbacks
    if (maskFlag(DEO_FLAG)) {
        //TODO move callback discovery to shader!!
        for (uxn_device device : CALLBACK_DEVICES) {
            if (uint16_t addr = from_uxn_mem2(&memory->shared.dev[static_cast<glm::uint>(device)])) {
                deviceCallbackVectors.insert({device, addr});
            }
        }
        return;
    }
}

bool Uxn::programTerminated() const {
    return static_cast<int8_t>(from_uxn_mem(&memory->shared.dev[0x0f])) != 0;
}

bool Uxn::maskFlag(glm::uint mask) const {
    return (memory->shared.flags & mask) == mask;
}
