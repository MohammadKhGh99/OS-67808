#include "VirtualMemory.h"
#include "PhysicalMemory.h"

/**
 * checks if frame number is in current search path
 * @param current_path current search path
 * @param frame_num frame number to check
 * @return true if the frame number in the path, else false
 */
bool isFrameInPath(int current_path[], word_t frame_num)
{
    for(int i  = 0 ;i < TABLES_DEPTH; ++i){
        if(current_path[i] == frame_num){
            return true;
        }
    }
    return false;
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
 * checks if the frame with frame number is empty
 * @param frame_number frame to check
 * @return frame number if found empty, else 0
 */
//TODO consider returning boolean
uint64_t IsFrameEmpty(uint64_t frame_number){
    word_t word = 0;
    bool is_empty_frame = true;
    for(uint64_t row = 0 ; row < PAGE_SIZE; ++row){
        PMread(frame_number*PAGE_SIZE+row, &word);
        if(word != 0){
            is_empty_frame = false;
            break;
        }
    }
    if(is_empty_frame){
        return frame_number;
    }
    return 0;
}

/**
 * get page/frame offset from virtual address
 * @return page/frame offset
 */
uint64_t extractOffset(uint64_t virtual_address)
{
    return virtual_address & ((1 << OFFSET_WIDTH)-1);
}
/**
 *
 * @param current_frame_num number of current frame we are searching
 * @param offset_in_frame  offset in the frame we are searching through
 * @param depth depth in page tree
 * @param max_frame_num  maximal frame number that has been traversed
 * @param empty_frame_num number of empty frame that we encounter will searching
 * @param current_path list of frame number in the current path
 * @param swapIn_page virtual page number we want to swap in memory
 * @param max_cyclic_dist maximal cyclic distance so far
 * @param chosenFrame frame number that has the maximal cyclic distance from swapIn_page
 * @param curVirtualPage virtual page we are accessing in the search
 * @param chosenFramePerant pointer to the address of parent of chosenFrame
 * @param PageToEvict page number to evict(if needed)
 * @param stopSearch indicates that we found a frame and we can stop searching
 */
void searchSubTree(uint64_t current_frame_num, uint64_t offset_in_frame, int depth, word_t* max_frame_num,word_t* empty_frame_num,
                   int current_path[], uint64_t swapIn_page, uint64_t* max_cyclic_dist,uint64_t* chosenFrame, uint64_t curVirtualPage,
                   uint64_t* chosenFramePerant,uint64_t* PageToEvict,bool* stopSearch)
{
    if(*stopSearch){//found frame
        return;
    }
    word_t word = 0 ;
    PMread(current_frame_num*PAGE_SIZE+offset_in_frame,&word);
    if(word > *max_frame_num){
        *max_frame_num = word;
    }
    if(!word){
        return;
    }
    //check if frame pointed by word is empty
    if(!isFrameInPath(current_path, word) && depth < TABLES_DEPTH && IsFrameEmpty(word))
    {
        //remove link from parent frame
        PMwrite(current_frame_num*PAGE_SIZE+offset_in_frame,0);
        *empty_frame_num = word;
        *stopSearch = true;
        return;
    }
    //check if we reached leaves of the farme tree
    if(depth == TABLES_DEPTH){
        //we need to choose a frame to evict based on cyclic distance(but that won't stop the
        // search because we need to check distance from all unempty leaves)
        uint64_t current_distance = cyclicalDistance(curVirtualPage,swapIn_page);
        if(current_distance > *max_cyclic_dist){
            *max_cyclic_dist = current_distance;
            *chosenFrame = word;
            *chosenFramePerant = current_frame_num*PAGE_SIZE+offset_in_frame;
            *PageToEvict = curVirtualPage;
        }
        return;
    }
    for(int row = 0 ;row < PAGE_SIZE;++row){
        searchSubTree((uint64_t) word,row,(depth+1),max_frame_num,empty_frame_num,current_path,
                      swapIn_page,max_cyclic_dist,chosenFrame,((curVirtualPage << OFFSET_WIDTH)+row),chosenFramePerant,
                      PageToEvict,stopSearch);
    }
}

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

/**
 * Searches for a frame to allocate to the page according to the given priorities.
 * @param currentPath list of frame number in the current path
 * @param swapIn_page virtual page number we want to swap in memory
 * @return frame number to allocate new page
 */
word_t searchFrame(int currentPath[], uint64_t swapIn_page)
{
    word_t empty_frame_num = 0, max_frame_num = 0;
    uint64_t max_cyclic_dist = 0 , chosenFrame = 0;
    uint64_t chosenFramePerant = 0 ,PageToEvict = 0,curVirtualPage = 0;
    bool stopSearch = false;
    for(int row = 0 ; row < PAGE_SIZE; ++row)
    {
        searchSubTree(0,row,1,&max_frame_num,&empty_frame_num,currentPath,swapIn_page,&max_cyclic_dist,
                      &chosenFrame,((curVirtualPage << OFFSET_WIDTH)+row),&chosenFramePerant,&PageToEvict,&stopSearch);
    }
    //chose frame according to there priority
    if(empty_frame_num){
        return empty_frame_num;
    }

    if(!(empty_frame_num) && (max_frame_num + 1 < NUM_FRAMES))
    {
        return max_frame_num + 1;
    }
    //all frames are already used, need to swap pages
    //first need to remove link from parent frame
    PMwrite(chosenFramePerant,0);
    PMevict(chosenFrame,PageToEvict);
    for (int i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(chosenFrame*PAGE_SIZE + i, 0);
    }
    return chosenFrame;
}

/**
 * translate the virtual address given in read/write
 * @param current_frame_num number of current frame we are searching
 * @param virtual_address address to translate
 * @param depth current depth
 * @param currentPath list of frame number in the current path
 * @return address in physical memory
 */
uint64_t translateToPhysicalAddress(uint64_t current_frame_num, uint64_t virtual_address, int depth, int currentPath[],bool free)
{
    word_t address_To_return = 0;
    uint64_t  offset_in_frame = extractOffset(virtual_address >> ((TABLES_DEPTH - depth)*OFFSET_WIDTH));
    uint64_t swapIn_page = virtual_address >>OFFSET_WIDTH;

    if(free){
        clearTable(current_frame_num);
    }
    PMread(current_frame_num*PAGE_SIZE+offset_in_frame,&address_To_return);
    if(!address_To_return){
        free = true;
        address_To_return = searchFrame(currentPath,swapIn_page);
        PMwrite(current_frame_num*PAGE_SIZE+offset_in_frame,address_To_return);
    }
    else{free = false;}
    if(depth == TABLES_DEPTH - 1){
        PMrestore(address_To_return,swapIn_page);
        return address_To_return;
    }
    currentPath[depth] = address_To_return;
    return translateToPhysicalAddress((uint64_t) address_To_return, virtual_address,depth+1,currentPath,free);
}

void VMinitialize() {
    clearTable(0);
}


int VMread(uint64_t virtualAddress, word_t* value) {
    if (!((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) == 0)) { // illegal address
        return 0;
    }
    int currentPath[TABLES_DEPTH] = {0};
    uint64_t physicalAdd = translateToPhysicalAddress(0,virtualAddress,0,currentPath,false);
    PMread(physicalAdd*PAGE_SIZE + extractOffset(virtualAddress),  value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if (!((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) == 0)) { // illegal address
        return 0;
    }
    int currentPath[TABLES_DEPTH] = {0};
    uint64_t physicalAdd = translateToPhysicalAddress(0,virtualAddress,0,currentPath,false);
    PMwrite(physicalAdd*PAGE_SIZE + extractOffset(virtualAddress),  value);
    return 1;
}
