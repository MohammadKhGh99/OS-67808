//
// Created by sarahhegazi on 16/06/2020.
//

#include <algorithm>
#include <cmath>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}


/**
 * split the virtual address to get offsets in each frame in the tree
 * @param virtualAddress page virtual address
 * @param addressArray offsets array
 */
void splitVirtualAddress(uint64_t virtualAddress, uint64_t* addressArray){
    for(word_t i = TABLES_DEPTH; i >= 0; --i){
        addressArray[i] = virtualAddress % PAGE_SIZE;
        virtualAddress = virtualAddress / PAGE_SIZE;
    }
}
/**
 * finds the number of maximum frame in use
 * @param curFrame number of current frame we check
 * @param depth depth in page tree
 * @return number of maximum frame in use
 */
word_t findMaxFrame(word_t curFrame, int depth){
    word_t tmpFrame = curFrame;
    word_t frameNum;
    for(int i = 0; i < PAGE_SIZE; ++i){
        PMread(curFrame*PAGE_SIZE+i,&frameNum);
        tmpFrame= std::max(frameNum,tmpFrame);
        if(frameNum != 0 && depth > 1){
            tmpFrame = std::max(tmpFrame, findMaxFrame(frameNum, depth - 1));
        }
    }
    return tmpFrame;
}

/**
 * calculate the cyclical Distance between current page we are checking the page we want to swap in
 * @param current_page virtual current page we are checking
 * @param page_swapped_in virtual page to swap in
 * @return cyclical Distance as described in the exercise
 */
uint64_t cyclicalDistance(uint64_t current_page, uint64_t page_swapped_in){
    uint64_t  diff = page_swapped_in - current_page;
    if (diff > (NUM_PAGES - diff)){
        return NUM_PAGES - diff;
    }
    return diff;
}

/**
 * check if newPageToEvict is better page to evict than curPageToEvict
 * @param swapped_in_page virtual page to swap in
 * @param curPageToEvict page we decided to evict
 * @param newPageToEvict candidate page to evict
 * @return true if newPage ToEvict is better, otherwise false
 */
bool changePageToEvict(word_t &swapped_in_page, word_t &curPageToEvict, word_t &newPageToEvict){
    if(curPageToEvict == -1){
        return true;
    }
    uint64_t distance1 = cyclicalDistance(curPageToEvict, swapped_in_page);
    uint64_t distance2 = cyclicalDistance(newPageToEvict, swapped_in_page);
    return (distance1 < distance2);
}
/**
 * search for frame (empty or to frame to evict) in curFrame subtree
 * @param page_swapped_in virtual page number we want to swap in memory
 * @param curPageToEvict page number to evict(if needed)
 * @param curFrameToEvict frame number that we will evict
 * @param pointerToRemove reference to curFrameToEvict table from its parent
 * @param pointerCurFrame pointer to current frame we are checking
 * @param curFrame current frame we are checking
 * @param pageBase start of page we are looking through
 * @param depth depth in table tree
 * @param frameInPath frame that is already in our path and can't use
 * @return true if we find frame to evict, false other wise
 */
bool searchForFrame(word_t page_swapped_in, word_t &curPageToEvict, word_t &curFrameToEvict, word_t &pointerToRemove
        , word_t pointerCurFrame, word_t curFrame, word_t pageBase, int depth, word_t frameInPath){
    word_t frame;
    word_t base;
    word_t ptrToFrame;
    bool frameEmpty = true;
    for (int i = 0; i<PAGE_SIZE; ++i){
        ptrToFrame = curFrame*PAGE_SIZE+i;
        PMread(ptrToFrame,&frame);
        if(frame != 0){
            frameEmpty = false;
            if(depth > 1){
                if(!searchForFrame(page_swapped_in,curPageToEvict,curFrameToEvict,pointerToRemove,
                        ptrToFrame,frame,pageBase+i*std::pow(PAGE_SIZE,depth-1),depth-1,frameInPath)){
                    return false;
                }
            }
            else{
                //reached the final level of the tree and need to find with page/frame to evict
                base = pageBase + i;
                if(changePageToEvict(page_swapped_in,curPageToEvict,base)){
                    curPageToEvict = base;
                    curFrameToEvict = frame;
                    pointerToRemove = curFrame*PAGE_SIZE+i;
                }
            }
        }
    }
    if(frameEmpty){
        if (curFrame == frameInPath){
            return true;
        }
        //found empty frame, no need to evict but we have to remove the reference to this table from its parent
        curPageToEvict = -1;
        curFrameToEvict = curFrame;
        pointerToRemove = pointerCurFrame;
        return false;
    }
    return true;
}
/**
 * find a frame we want to read/ write from/to it
 * @param swapped_in_page virtual page we to read/write
 * @param frameInPath frame already in path
 * @return number of frame in physical memory where data in the page swapped in
 */
word_t findFrame(word_t swapped_in_page, word_t frameInPath){
    word_t pageToEvict = -1;
    word_t frameToEvict = -1;
    word_t pointerToRemove = -1;
    word_t  ptrCurFrame = -1 ,curFrame = 0, pageBase = 0;
    bool findPage = searchForFrame(swapped_in_page,pageToEvict,frameToEvict,pointerToRemove,ptrCurFrame,
                                   curFrame,pageBase,TABLES_DEPTH,frameInPath);
    if(findPage){
        PMevict(frameToEvict,pageToEvict);
    }
    PMwrite(pointerToRemove,0);
    return frameToEvict;
}
/**
 * translate the virtual address in read/write operation
 * @param virtualAddress address to translate
 * @param physicalAddress address in physical memory
 */
void translateToPhysicalAddress(uint64_t virtualAddress, uint64_t &physicalAddress) {
    uint64_t AddressArray[TABLES_DEPTH + 1] = {0};
    word_t pageIndex = virtualAddress / PAGE_SIZE;
    splitVirtualAddress(virtualAddress, AddressArray);
    word_t word = 0;
    word_t frame = 0;
    for (int i = 0; i < TABLES_DEPTH; ++i) {
        PMread(word * PAGE_SIZE + AddressArray[i], &frame);
        if (frame == 0) {
            frame = findMaxFrame(0, TABLES_DEPTH) + 1;
            if (frame == NUM_FRAMES) {
                frame = findFrame(pageIndex, word);
            }
            PMwrite(word * PAGE_SIZE + AddressArray[i], frame);
            // frame of table we need to clear table before adding new addresses
            if (i < TABLES_DEPTH- 1) {
                clearTable(frame);
            }
                // frame of page (last level in tree) we need to restore page from disk
            else {
                PMrestore(frame, pageIndex);
            }
        }
        word = frame;
    }
    physicalAddress = frame * PAGE_SIZE + AddressArray[TABLES_DEPTH];
}


void VMinitialize() {
    clearTable(0);
}

int VMread(uint64_t virtualAddress, word_t* value) {
    if (!((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) == 0)) { // illegal address
        return 0;
    }

    uint64_t physicalAddress;
    translateToPhysicalAddress(virtualAddress,physicalAddress);
    
    PMread(physicalAddress, value); //reads value from page
    return 1;
}

int VMwrite(uint64_t virtualAddress, word_t value) {
    if (!((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) == 0)) { // illegal address
        return 0;
    }

    uint64_t physicalAddress;
    translateToPhysicalAddress(virtualAddress,physicalAddress);

    PMwrite(physicalAddress,value); //writes value at page

    return 1;
}