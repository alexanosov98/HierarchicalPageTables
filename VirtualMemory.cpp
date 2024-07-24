#include <algorithm>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define OFFSET_MASK & ((1LL << OFFSET_WIDTH) - 1)
#define PM_PAGE_NUM_MASK >> OFFSET_WIDTH
#define SUCCESS 1;
#define FAILURE 0;


void VMinitialize() {
    for (word_t i = 0; i < NUM_FRAMES; i++){
        for (word_t j = 0; j < PAGE_SIZE; j++){
            PMwrite (i*PAGE_SIZE + j, 0);
        }
    }
}

int countBits(uint64_t binaryNumber) {
    int count = 0;
    while (binaryNumber > 0) {
        count++;
        binaryNumber >>= 1; // Right shift the number by 1 bit
    }
    return count;
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

/*
 * Returns in decimal
 */
uint64_t getNextPageOffset(uint64_t address, int depth) {
    return binaryToDecimal(address >> ((TABLES_DEPTH - depth) * OFFSET_WIDTH) OFFSET_MASK);
}


word_t getMaxOccupiedFrame(uint64_t currFrame, word_t& maxOccupiedFrame){
    word_t val;
    for (word_t i = 0; i < NUM_FRAMES; i++){
        if (i != currFrame)
        {
            for (word_t j = 0; j < PAGE_SIZE; j++){
                PMread (i * PAGE_SIZE + j, &val);
                if (val != 0)
                {
                    if (val > maxOccupiedFrame){
                        maxOccupiedFrame = val;
                    }
                }
            }
        }
    }
}

word_t getEmptyFrame(uint64_t currFrame){
    word_t val;
    for (word_t i = 0; i < NUM_FRAMES; i++){
        if (i != currFrame)
        {
            bool frameIsEmpty = true;
            for (word_t j = 0; j < PAGE_SIZE; j++){
                PMread (i * PAGE_SIZE + j, &val);
                if (val != 0)
                {
                    frameIsEmpty = false;
                    break;
                }
            }
            if (frameIsEmpty){
                return i;
            }
        }
    }
    return 0;
}



// Function to calculate cyclical distance
int findCyclicalDistance(uint64_t targetPage, uint64_t currPageNumber) {
    if (targetPage - currPageNumber < NUM_PAGES - (targetPage - currPageNumber)){
        return (int) (targetPage - currPageNumber);
    }
    return (int) (NUM_PAGES - (targetPage - currPageNumber));
}


void evictPage(word_t& evictedFrame, word_t& evictedFather, int& evictedPageNumber){
    PMevict (evictedFrame, evictedPageNumber);
    word_t val;
    for (int i = 0; i < PAGE_SIZE; i++){
        PMwrite (evictedFrame * PAGE_SIZE + i, 0);
        PMread (evictedFather * PAGE_SIZE + i, &val);
        if(val == evictedFrame){
            PMwrite (evictedFather * PAGE_SIZE + i, 0);
        }
    }



}

/**
 * Function to traverse the page table tree and find the closest frame to
 * change
 */

void findPageToEvict(word_t frameIndex, int depth, uint64_t
targetPage, word_t& evictedFrame, int& maxCyclicalDistance, uint64_t&
currentVirtualAddress, word_t* maxOccupiedFrame, word_t myFather, word_t&
evictedFather, int& evictedPageNum) {
    //Base case: when we reach the maximum depth, calculate the distance.
    if (depth == TABLES_DEPTH) {
        int distance = findCyclicalDistance (targetPage, currentVirtualAddress);
        if (distance > maxCyclicalDistance)
        {
            maxCyclicalDistance = distance;
            evictedFrame = frameIndex;
            evictedFather = myFather;
            evictedPageNum = currentVirtualAddress;
        }
        return;
    }
    // Traverse each entry in the current table
    word_t val;
    for (uint64_t i = 0; i < PAGE_SIZE; i++) {

        PMread(frameIndex * PAGE_SIZE + i, &val);
        if (val != 0) {
            currentVirtualAddress = (currentVirtualAddress << 1LL) + i;
            findPageToEvict(val, depth + 1, targetPage, evictedFrame,
                            maxCyclicalDistance, currentVirtualAddress,
                            maxOccupiedFrame, frameIndex, evictedFather, evictedPageNum);
            currentVirtualAddress = (currentVirtualAddress - i) >> 1LL;
        }
    }
}

/*
 * Returns the new frame address, evicts an existing frame if needed
 */
word_t findNewFrame(uint64_t desirablePageNum, uint64_t currFrame){
    word_t maxOccupiedFrame = 0;
    getMaxOccupiedFrame(currFrame, maxOccupiedFrame);
    if (maxOccupiedFrame + 1 < NUM_FRAMES){
        return ++maxOccupiedFrame;
    }
    word_t evictedFrame;
    int maxCyclicalDistance = -1;
    word_t evictedFather = 0;
    uint64_t currentVirtualAddress = 0;
    int evictedPageNum = 0;
    findPageToEvict(0, 0, desirablePageNum, evictedFrame,
                    maxCyclicalDistance, currentVirtualAddress, &maxOccupiedFrame, 0, evictedFather, evictedPageNum);

    evictPage(evictedFrame, evictedFather, evictedPageNum);
    return evictedFrame;
}
#include <iostream>
void printPhysicalMemory1() {
    std::cout << "Physical Memory:" << std::endl;
    for (int frame = 0; frame < NUM_FRAMES; ++frame) {
        std::cout << "Frame " << frame << ": ";
        for (int offset = 0; offset < PAGE_SIZE; ++offset) {
            word_t value;
            PMread(frame * PAGE_SIZE + offset, &value);
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

void reachTheLeaf(uint64_t& currFrameNum, uint64_t virtualAddress, uint64_t PMPageNum){
    int treeDepth = 0;
    word_t newFrameNum = 0;
    uint64_t nextPageOffset = 0;
    word_t readValue = 0;
    uint64_t fatherFrame = currFrameNum;
    //Traverse through the tree.
    while (treeDepth < TABLES_DEPTH) {
        nextPageOffset = getNextPageOffset(virtualAddress, treeDepth);
        PMread(currFrameNum * PAGE_SIZE + nextPageOffset, &readValue);
        //Page table is not in the RAM
        if (readValue == 0) {
            fatherFrame = currFrameNum;
            //Find a new frame to store the Page table
            newFrameNum = findNewFrame(PMPageNum, currFrameNum);
            if (treeDepth != TABLES_DEPTH - 1) {
                //If the next layer is a table, fill it with zeros.
                for (int i = 0; i < PAGE_SIZE; i++) {
                    PMwrite(newFrameNum * PAGE_SIZE + i, 0);
                }
            }
            else{
                PMrestore(newFrameNum, virtualAddress >> OFFSET_WIDTH);
            }
            //Link the previous table to the new table
            PMwrite(fatherFrame * PAGE_SIZE + nextPageOffset, newFrameNum);
            currFrameNum = newFrameNum;
            printPhysicalMemory1();
        }
        //Page table is in the RAM
        else{
            currFrameNum = readValue;
        }
        treeDepth++;


    }
}



int VMread(uint64_t virtualAddress, word_t *value) {
    if(countBits (virtualAddress) > VIRTUAL_ADDRESS_WIDTH){
        return FAILURE;
    }
    uint64_t finalOffset = virtualAddress OFFSET_MASK;
    uint64_t PMPageNum = virtualAddress PM_PAGE_NUM_MASK;
    uint64_t currFrameNum = 0;

    reachTheLeaf(currFrameNum, virtualAddress, PMPageNum);
    //Read the value from the page specified in the VA.
    PMread(currFrameNum * PAGE_SIZE + finalOffset, value);
    return SUCCESS;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if(countBits (virtualAddress) > VIRTUAL_ADDRESS_WIDTH){
        return FAILURE;
    }
    uint64_t finalOffset = virtualAddress OFFSET_MASK;
    uint64_t PMPageNum = virtualAddress PM_PAGE_NUM_MASK;
    uint64_t currFrameNum = 0;

    reachTheLeaf(currFrameNum, virtualAddress, PMPageNum);
    //Write the value to the page specified in the VA.
    //Restore the page from the disk.
    PMrestore (currFrameNum, binaryToDecimal (PMPageNum));
    //Write the value in the PM
    PMwrite(currFrameNum * PAGE_SIZE + finalOffset, value);
    return SUCCESS;
}