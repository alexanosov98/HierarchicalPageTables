#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define OFFSET_MASK ((1LL << OFFSET_WIDTH) - 1)
#define PM_PAGE_NUM_MASK (~OFFSET_MASK & ((1LL << VIRTUAL_ADDRESS_WIDTH) - 1))
#define NOT_IN_RAM 0;
#define SUCCESS 1;
#define FAILURE 0;


void VMinitialize() {
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


word_t checkForEmptyFrame(uint64_t currFrame){
  word_t val;
  for (word_t i = 0; i < NUM_FRAMES; i++){
    if (i != currFrame)
    {
      PMread (i * PAGE_SIZE, &val);
      if (val == 0)
      {
        return i;
      }
    }
  }
}


/*
 * Returns the new frame address, evicts an existing frame if needed
 */
word_t findNewFrame(){
//  if (maxFrameReached + 1 < NUM_FRAMES){
//
//  }
  //todo evict, check MAX FRAME REACHED.
  word_t empty_frame = checkForEmptyFrame(currFrame);
  if(empty_frame != 0){
    return empty_frame;
  }

}




void reachTheLeaf(uint64_t& currFrameNum, uint64_t virtualAddress, uint64_t
PMPageNum){
  int treeDepth = 0;
  word_t newFrameNum = 0;
  uint64_t nextPageOffset = 0;
  word_t readValue = 0;
  word_t maxOccupiedFrame = 0;
  //todo update maxFrameReached
  //Traverse through the tree.
  while (treeDepth < TABLES_DEPTH){
    nextPageOffset = getNextPageOffset (virtualAddress, treeDepth);
    PMread (currFrameNum * PAGE_SIZE + nextPageOffset, &readValue);
    //Page table is not in the RAM
    if (readValue == 0){
      if (maxOccupiedFrame+1 >NUM_FRAMES){ // we are out of frames
        //Find a new frame to store the Page table
        newFrameNum = findNewFrame (PMPageNum, currFrameNum, maxOccupiedFrame);
      }
      else{
        newFrameNum = maxOccupiedFrame+1;
        maxOccupiedFrame ++;
      }
      //If the next layer is a table, fill it with zeros.
      for (int i = 0; i < PAGE_SIZE; i++){
        PMwrite (newFrameNum * PAGE_SIZE + i, 0);
      }
      //Link the previous table to the new table
      PMwrite (currFrameNum * PAGE_SIZE + nextPageOffset, newFrameNum);
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
  uint64_t finalOffset = virtualAddress & OFFSET_MASK;
  uint64_t PMPageNum = virtualAddress & PM_PAGE_NUM_MASK;
  uint64_t currFrameNum = 0;

  reachTheLeaf(currFrameNum, virtualAddress, PMPageNum);
  //Read the value from the page specified in the VA.
  //Restore the page from the disk.
  PMrestore (currFrameNum, binaryToDecimal (PMPageNum));
  //Write the value in the PM
  PMread(currFrameNum * PAGE_SIZE + finalOffset, value);
  return SUCCESS;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
  if(countBits (virtualAddress) > VIRTUAL_ADDRESS_WIDTH){
    return FAILURE;
  }
  uint64_t finalOffset = virtualAddress & OFFSET_MASK;
  uint64_t PMPageNum = virtualAddress & PM_PAGE_NUM_MASK;
  uint64_t currFrameNum = 0;

  reachTheLeaf(currFrameNum, virtualAddress, PMPageNum);
  //Write the value to the page specified in the VA.
  //Restore the page from the disk.
  PMrestore (currFrameNum, binaryToDecimal (PMPageNum));
  //Write the value in the PM
  PMwrite(currFrameNum * PAGE_SIZE + finalOffset, value);
  return SUCCESS;
}