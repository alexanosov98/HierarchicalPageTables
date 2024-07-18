#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define OFFSET_MASK ((1LL << OFFSET_WIDTH) - 1)

void VMinitialize() {
}

uint64_t binaryToDecimal(uint64_t binaryNumber) {
    unsigned int decimalValue = 0;
    unsigned int base = 1;  // 2^0

    while (binaryNumber > 0) {
        unsigned int lastDigit = binaryNumber % 10;
        decimalValue += lastDigit * base;
        binaryNumber = binaryNumber / 10;
        base = base * 2;
    }

    return decimalValue;
}

uint64_t decimalToBinary(uint64_t decimalNumber) {
    uint64_t binaryValue = 0;
    uint64_t base = 1;  // Start with the least significant bit

    while (decimalNumber > 0) {
        uint64_t lastDigit = decimalNumber % 2;
        binaryValue += lastDigit * base;
        decimalNumber /= 2;
        base *= 10;
    }

    return binaryValue;
}

/*
 * Returns in binary
 */
uint64_t nextPMAddress(uint64_t address, int depth) {
    if (!depth){
        return address << TABLES_DEPTH * OFFSET_WIDTH;
    }
    return binaryToDecimal(address << ((TABLES_DEPTH - depth) * OFFSET_WIDTH) & OFFSET_MASK);
}


int64_t checkForEmptyFrame(){
    word_t val;
    for (uint64_t i = 0; i < NUM_FRAMES; i++){
        PMread(i*PAGE_SIZE, &val);
        if (val == 0){
            return i*PAGE_SIZE;
        }
    }
    return 0;
}


int VMread(uint64_t virtualAddress, word_t *value) {
    uint64_t offset = virtualAddress & OFFSET_MASK;
    int treeDepth = 0;
    uint64_t nextPhysicalAddress = 0;
    word_t frame = 0;

    while (treeDepth <= TABLES_DEPTH){
        uint64_t pageIndex = nextPMAddress(virtualAddress, treeDepth);

        nextPhysicalAddress = frame + pageIndex * PAGE_SIZE;

        PMread(nextPhysicalAddress, &frame); // returns pointer

        if (&frame == nullptr){
            word_t emptyFrameAddress = checkForEmptyFrame();
            if (!emptyFrameAddress){
                //todo remove references to it from its parents
            }

        }
        treeDepth++;
    }

    //Add offset.
    nextPhysicalAddress += binaryToDecimal(offset);
    //Extract the wanted data from the final frame.
    PMread(nextPhysicalAddress, value);
    if (value == nullptr){

    }

}

int VMwrite(uint64_t virtualAddress, word_t value) {
    return 0;
}

