#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>

#include <cstdio>
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    VMinitialize();
    for (uint64_t i = 0; i < VIRTUAL_MEMORY_SIZE; ++i) {
        int j = i;
        std::cout << "Writing the value " + std::to_string(j) << std::endl;
        VMwrite(i, i);
        printRam();
    }
    std::cout << std::endl;
    printRam();
    std::cout << std::endl;

    for (uint64_t i = 0; i < VIRTUAL_MEMORY_SIZE; ++i) {
        word_t value;
        std::cout << "Reading the value at address: " + std::to_string(i) << std::endl;
        VMread(i, &value);
          std::cout << "the value at address: " + std::to_string(i) + " is " +
                       std::to_string (value) << std::endl;

        assert(uint64_t(value) == i);
    }

    printf("success\n");

    return 0;
}
