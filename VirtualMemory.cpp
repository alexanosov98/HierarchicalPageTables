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



// Function to calculate cyclical distance
int findCyclicalDistance(uint64_t currentPage, uint64_t targetPage) {
    uint64_t distance1 = NUM_PAGES - std::abs(int64_t(targetPage) - int64_t(currentPage));
    uint64_t distance2 = std::abs(int64_t(targetPage) - int64_t(currentPage));
    uint64_t distance = distance1 < distance2 ? distance1 : distance2;
    return distance;
}

void clearFrameAndFathersLink(word_t& evictedFrame, word_t& evictedFather, bool isLeaf = false){
    word_t val;
    for (int i = 0; i < PAGE_SIZE; i++){
        if (!isLeaf){
            PMwrite (evictedFrame * PAGE_SIZE + i, 0);
        }
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
evictedFather, int& evictedPageNum, uint64_t& originalFrame, bool &emptyFound) {
    //Flag for the case that we found an empty frame
    if (maxCyclicalDistance == NUM_PAGES){
        return;
    }
    //Checks if the current frame is empty, if so returns it.
    if (depth < TABLES_DEPTH && ((uint64_t )frameIndex) != originalFrame) {
        word_t testVal;
        bool found = true;
        for (uint64_t i = 0; i < PAGE_SIZE; i++) {
            PMread(frameIndex * PAGE_SIZE + i, &testVal);
            if (testVal != 0) {
                if (testVal > *maxOccupiedFrame){
                    *maxOccupiedFrame = testVal;
                }
                found = false;
                break;
            }
        }
        if (found){
            maxCyclicalDistance = NUM_PAGES;
            evictedFrame = frameIndex;
            evictedFather = myFather;
            evictedPageNum = NUM_PAGES;
            return;
        }
    }
    //Base case: when we reach the maximum depth, calculate the distance.
    if (depth == TABLES_DEPTH) {
        int distance = findCyclicalDistance ( currentVirtualAddress, targetPage);
        if (distance > maxCyclicalDistance)
        {
            maxCyclicalDistance = distance;
            evictedFrame = frameIndex;
            evictedFather = myFather;
            evictedPageNum = currentVirtualAddress;
        }
        return;
    }
    //Traverse each entry in the current table
    word_t val;
    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        PMread(frameIndex * PAGE_SIZE + i, &val);
        if (val != 0) {
            if (val > *maxOccupiedFrame){
                *maxOccupiedFrame = val;
            }
            currentVirtualAddress = (currentVirtualAddress << OFFSET_WIDTH) + i; //todo MIGHT NEED TO  CHANGE THIS
            findPageToEvict(val, depth + 1, targetPage, evictedFrame,
                            maxCyclicalDistance, currentVirtualAddress,
                            maxOccupiedFrame, frameIndex, evictedFather,
                            evictedPageNum, originalFrame, emptyFound);
            currentVirtualAddress = (currentVirtualAddress - i) >> OFFSET_WIDTH;
        }
    }
}

/*
 * Returns the new frame address, evicts an existing frame if needed
 */
word_t findNewFrame(uint64_t desirablePageNum, uint64_t currFrame){
    word_t maxOccupiedFrame = 0;
    bool emptyFound = false;
    word_t evictedFrame;
    int maxCyclicalDistance = -1;
    word_t evictedFather = 0;
    uint64_t currentVirtualAddress = 0;
    int evictedPageNum = 0;
    findPageToEvict(0, 0, desirablePageNum, evictedFrame,
                    maxCyclicalDistance, currentVirtualAddress,
                    &maxOccupiedFrame, 0, evictedFather, evictedPageNum, currFrame, emptyFound);
    //Found an empty Frame
    if (evictedPageNum == NUM_PAGES){
        clearFrameAndFathersLink(evictedFrame, evictedFather);
        return evictedFrame;
    }
    if (maxOccupiedFrame + 1 < NUM_FRAMES){
        return ++maxOccupiedFrame;
    }
    PMevict (evictedFrame, evictedPageNum);
    clearFrameAndFathersLink(evictedFrame, evictedFather, true);
    return evictedFrame;
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
            //No a leaf.
            if (treeDepth != TABLES_DEPTH - 1) {
                //If the next layer is a table, fill it with zeros.
                for (int i = 0; i < PAGE_SIZE; i++) {
                    PMwrite(newFrameNum * PAGE_SIZE + i, 0);
                }
            }
            //Is a leaf
            else{
                PMrestore(newFrameNum, virtualAddress >> OFFSET_WIDTH);
            }
            //Link the previous table to the new table
            PMwrite(fatherFrame * PAGE_SIZE + nextPageOffset, newFrameNum);
            currFrameNum = newFrameNum;
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
    uint64_t finalOffset = virtualAddress % PAGE_SIZE;
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