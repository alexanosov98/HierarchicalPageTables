#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define OFFSET_MASK ((1LL << OFFSET_WIDTH) - 1)
#define NOT_IN_RAM 0;


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
 * Returns in decimal
 */
uint64_t getNextPageOffset(uint64_t address, int depth) {
    if (!depth){
        return address << TABLES_DEPTH * OFFSET_WIDTH;
    }
    return binaryToDecimal(address << ((TABLES_DEPTH - depth) * OFFSET_WIDTH) & OFFSET_MASK);
}


word_t checkForEmptyFrame(){
  //todo MAKE SURE WE DONT USE A FRAME WE NEED
    word_t val;
    for (uint64_t i = 0; i < NUM_FRAMES; i++){
        PMread(i*PAGE_SIZE, &val);
        if (val == 0){
            return i*PAGE_SIZE;
        }
    }
    return 0;
}


/*
 * Returns the new frame address, evicts an existing frame if needed
 */
word_t findNewFrame(){
//  if (maxFrameReached + 1 < NUM_FRAMES){
//
//  }
  //todo evict, check MAX FRAME REACHED.
  word_t empty_frame = checkForEmptyFrame();
  if(empty_frame != 0){
    return empty_frame;
  }

}




int VMread(uint64_t virtualAddress, word_t *value) {
}


int VMwrite(uint64_t virtualAddress, word_t value) {

  uint64_t finalOffset = virtualAddress & OFFSET_MASK;
  int treeDepth = 0;

  uint64_t currFrame = 0;
  word_t newFrame = 0;
  uint64_t nextPageOffset = 0;

  word_t readValue = 0;
  uint64_t maxFrameReached = 0;

  //todo update maxFrameReached
  //Traverse through the tree.
  while (treeDepth < TABLES_DEPTH){

    nextPageOffset = getNextPageOffset (virtualAddress, treeDepth);

    PMread (currFrame * PAGE_SIZE + nextPageOffset, &readValue);

    //Page table is not in the RAM but the frame is empty
    if (readValue == 0){
      //Find a new frame to store the Page table
      newFrame = findNewFrame ();
      //If the next layer is a table, fill it with zeros.
      for (int i = 0; i < PAGE_SIZE; i++){
        PMwrite (newFrame * PAGE_SIZE + i, 0);
      }
      //Link the previous table to the new table
      PMwrite (currFrame, newFrame);
      currFrame = newFrame;
    }

    //Page table is in the RAM
    else{
      currFrame = newFrame;
      newFrame = readValue;
    }
    treeDepth++;
  }

  //Write the value in the PM
  PMread (currFrame * PAGE_SIZE + finalOffset, &readValue);
  //PM Address is empty
  if (readValue == 0){
    //Find a new frame to store the Page table
    newFrame = findNewFrame ();
    //Link the previous table to the new table
    PMwrite (currFrame, value);
  }

    //Page table is in the RAM
  else{
    currFrame = newFrame;
    newFrame = readValue;
  }
}

