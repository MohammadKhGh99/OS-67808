#include <math.h>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

// ------------------ Functions Declarations (Start) ------------------ //
word_t frameReturner(uint64_t virtualAddress, word_t *path, int index);
void frameFinding(uint64_t table, uint64_t index, word_t *path, uint64_t *emptyPage, uint64_t *array, int depth = 0,
				  uint64_t frame = 0, uint64_t page = 0, uint64_t parent = 0);
int distance(uint64_t firstPage, uint64_t secondPage);
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
	/////////////////////////////
	uint64_t newAddress, frame = virtualAddress, offset = frame % (int) pow(2, OFFSET_WIDTH);
	bool flag = false;
	word_t path[TABLES_DEPTH + 1];
	path[0] = 0;
	for (int i = 0; i < TABLES_DEPTH; ++i)
	{
		int x = pow(2, OFFSET_WIDTH * (TABLES_DEPTH - i));  // todo change to double or not?
		int j = frame / x;
		frame %= x;
		PMread(PAGESIZE * path[i] + j, &path[i + 1]);
		if (path[i + 1] == 0) //todo did you mean this or I have to add "!" ?
		{
			PMwrite(PAGE_SIZE * path[i] + j, frameReturner(virtualAddress, path, i + 1));
			flag = true;
		}
	}
	if (flag)
	{
		PMrestore(path[TABLES_DEPTH], virtualAddress / PAGE_SIZE);
	}
	newAddress = PAGE_SIZE * path[TABLES_DEPTH] + offset;
	////////////////////////////
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
	////////////////////////
	uint64_t newAddress, frame = virtualAddress, offset = frame % (int) pow(2, OFFSET_WIDTH);
	bool flag = false;
	word_t path[TABLES_DEPTH + 1];
	path[0] = 0;
	for (int i = 0; i < TABLES_DEPTH; ++i)
	{
		int x = pow(2, OFFSET_WIDTH * (TABLES_DEPTH - i));  // todo change to double or not?
		int j = frame / x;
		frame %= x;
		PMread(PAGESIZE * path[i] + j, &path[i + 1]);
		if (path[i + 1] == 0) //todo did you mean this or I have to add "!" ?
		{
			PMwrite(PAGE_SIZE * path[i] + j, frameReturner(virtualAddress, path, i + 1));
			flag = true;
		}
	}
	if (flag)
	{
		PMrestore(path[TABLES_DEPTH], virtualAddress / PAGE_SIZE);
	}
	newAddress = PAGE_SIZE * path[TABLES_DEPTH] + offset;
	////////////////////////
	PMwrite(newAddress, value);
	return 1;
}

// ------------------ Helping Functions (Start) ------------------ //

word_t frameReturner(uint64_t virtualAddress, word_t *path, int index)
{
	uint64_t array[4], toReturn, emptyPage = 0;
	array[3] = 0, array[1] = virtualAddress / PAGE_SIZE;  // todo nice?
	////////////////////////////////
	frameFinding(virtualAddress / PAGE_SIZE, index, path, &emptyPage, array);
	////////////////////////////////
	uint64_t tmp = array[3] + 1;
	if (emptyPage)
	{
		toReturn = emptyPage;
	}
	else if (tmp < NUM_FRAMES)
	{
		toReturn = tmp;
		if (index != TABLES_DEPTH)
		{
			clearTable(toReturn);
		}
	}
	else
	{
		toReturn = array[2];
		PMwrite(array[0], 0);
		PMevict(toReturn, array[1]);
		if (index != TABLES_DEPTH)
		{
			clearTable(toReturn);
		}
	}
	path[index] = (word_t) toReturn;
	return (word_t) toReturn;
}

void frameFinding(uint64_t table, uint64_t index, word_t *path, uint64_t *emptyPage, uint64_t *array, int depth,
				  uint64_t frame, uint64_t page, uint64_t parent)
{
//	int firstResult, secondResult,
	int tmpEmptyPage = 1;
	array[3] = array[3] < frame ? frame : array[3];
	if (depth != TABLES_DEPTH)
	{
		tmpEmptyPage = depth != 0 ? 1 : 0;
		for (int i = 0; i < PAGE_SIZE; ++i)
		{
			word_t value;
			PMread(PAGE_SIZE * frame + i, &value);
			if (value)  // todo != 0 or not?
			{
				tmpEmptyPage = 0;
				frameFinding(table, index, path, emptyPage, array, depth + 1, value, PAGE_SIZE * page + i, PAGE_SIZE
																										   * frame + i);
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
			for (int i = 0; i < index; ++i)
			{
				if (path[i] == frame)
				{
					(*emptyPage) = 0;
					return;
				}
			}
			(*emptyPage) = frame;
			PMwrite(parent, 0);
		}
	}
	if (distance(table, page) > distance(table, array[1]))
	{
		array[0] = parent, array[1] = page, array[2] = frame;
	}
//	return;
}

int distance(uint64_t firstPage, uint64_t secondPage)
{
	int x = abs((int) (firstPage - secondPage));
	int y = NUM_PAGES - x;
	return x >= y ? y : x;
}

// ------------------ Helping Functions (End) ------------------ //
