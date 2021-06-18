#include <math.h>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

// ------------------ Functions Declarations (Start) ------------------ //

void findBestFrame(uint64_t table, uint64_t index, word_t *path, uint64_t *emptyPage, uint64_t *array, int depth = 0,
                   uint64_t frame = 0, uint64_t page = 0, uint64_t parent = 0);

int distance(uint64_t firstPage, uint64_t secondPage);

uint64_t convertVirtualToPhysical(uint64_t virtualAddress);

// ------------------ Functions Declarations (End) ------------------ //

void clearTable(uint64_t frameIndex)
{
	for (uint64_t i = 0; i < PAGE_SIZE; ++i)
	{
		PMwrite(frameIndex * PAGE_SIZE + i, 0);
	}
}

/**
 * Initialize the virtual memory
 */
void VMinitialize()
{
	clearTable(0);
}

/**
 * reads a word from the given virtual address and puts its content in *value.
 * @param virtualAddress a word to read.
 * @param value variable to put the content of the virtual address in it.
 * @return 1 on success, 0 on failure (if the address cannot be mapped to a physical address for any reason).
 */
int VMread(uint64_t virtualAddress, word_t *value)
{
	if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
	{
		return 0;
	}
	uint64_t newAddress = convertVirtualToPhysical(virtualAddress);
	PMread(newAddress, value);
	return 1;
}

/**
 * writes a word to the given virtual address.
 * @param virtualAddress virtual address to write to.
 * @param value a value to write.
 * @return 1 on success, 0 on failure (if the address cannot be mapped to a physical address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
	if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
	{
		return 0;
	}
	uint64_t newAddress = convertVirtualToPhysical(virtualAddress);
	PMwrite(newAddress, value);
	return 1;
}

// ------------------ Helping Functions Implementations (Start) ------------------ //

uint64_t convertVirtualToPhysical(uint64_t virtualAddress)
{
    uint64_t tempAddress = virtualAddress;
    uint64_t tempFrame = tempAddress % (int) pow(2, OFFSET_WIDTH);
    bool flag = false;
    word_t path[TABLES_DEPTH + 1] = {0};  // todo {0} added
//	path[0] = 0; todo commented
    for (int i = 0; i < TABLES_DEPTH; ++i)
    {
        int x = pow(2, OFFSET_WIDTH * (TABLES_DEPTH - i));
        int j = (int) tempAddress / x;
        tempAddress %= x;
        PMread(PAGE_SIZE * path[i] + j, &path[i + 1]);
        if (path[i + 1] == 0)
        {
            uint64_t array[4], emptyPage = 0;
            array[3] = 0, array[1] = virtualAddress / PAGE_SIZE;
            ////////////////////////////////
            findBestFrame(virtualAddress / PAGE_SIZE, i + 1, path, &emptyPage, array);
            ////////////////////////////////
            uint64_t tmp = array[3] + 1;
            uint64_t toReturn = emptyPage;
            uint64_t notEmpty = tmp < NUM_FRAMES ? tmp : array[2];
            toReturn = emptyPage ? toReturn : notEmpty;
            if (!emptyPage && tmp < NUM_FRAMES)
            {
                toReturn = tmp;
                if (i + 1 != TABLES_DEPTH)
                {
                    clearTable(toReturn);
                }
            }
            else if (!emptyPage && tmp >= NUM_FRAMES)
            {
                toReturn = array[2];
                PMwrite(array[0], 0);
                PMevict(toReturn, array[1]);
                if (i + 1 != TABLES_DEPTH)
                {
                    clearTable(toReturn);
                }
            }
            path[i + 1] = (word_t) toReturn;
            PMwrite(PAGE_SIZE * path[i] + j, toReturn);
            flag = true;
        }
    }
    if (flag)
    {
        PMrestore(path[TABLES_DEPTH], virtualAddress / PAGE_SIZE);
    }
    return PAGE_SIZE * path[TABLES_DEPTH] + tempFrame;
}

void findBestFrame(uint64_t table, uint64_t index, word_t *path, uint64_t *emptyPage, uint64_t *array, int depth,
                   uint64_t frame, uint64_t page, uint64_t parent)
{
	int tmpEmptyPage = 1;
	array[3] = array[3] < frame ? frame : array[3];
	if (depth != TABLES_DEPTH)
	{
		for (int i = 0; i < PAGE_SIZE; ++i)
		{
			word_t value;
			PMread(PAGE_SIZE * frame + i, &value);
			if (value)
			{
				tmpEmptyPage = 0;
                findBestFrame(table, index, path, emptyPage, array, depth + 1, value, PAGE_SIZE * page + i,
                              PAGE_SIZE * frame + i);
				if ((*emptyPage) == 0)
				{
					continue;
				}
				return;
			}
		}
		if (tmpEmptyPage == 1)
		{
			// Finding Empty Page.
			bool flag = false;
			for (uint64_t i = 0; i < index; ++i)
			{
				if ((uint64_t) path[i] == frame)
				{
					(*emptyPage) = 0;
					flag = true;
					break;
				}
			}
			if (!flag)
			{
				(*emptyPage) = frame;
				PMwrite(parent, 0);
			}
		}
		return;
	}
	if (distance(table, page) > distance(table, array[1]))
	{
		array[0] = parent, array[1] = page, array[2] = frame;
	}
}

int distance(uint64_t firstPage, uint64_t secondPage)
{
	int x = abs((int) (firstPage - secondPage));
	int y = NUM_PAGES - x;
	return x >= y ? y : x;
}

// ------------------ Helping Functions Implementations (End) ------------------ //