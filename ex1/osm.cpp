//
// Created by m7mdg on 10-Apr-21.
//

#include "osm.h"
#include <sys/time.h>
#include <iostream>

#define TO_NANO 1000000000.0
#define MIC_NAN 1000.0
#define UNROLLING_CONST 5
#define ERROR -1


/**
 * Time measurement function for a simple arithmetic operation.
 *  returns time in nano-seconds upon success,
 *  and -1 upon failure.
 */
double osm_operation_time(unsigned int iterations)
{
	if (iterations <= 0)
	{
		return ERROR;
	}
	unsigned int diff = iterations % UNROLLING_CONST;
	if (diff != 0)
	{
		iterations += UNROLLING_CONST - diff;
	}
	unsigned int iteration = 0, add = 0;
	struct timeval begin{}, end{};
	if (gettimeofday(&begin, nullptr) != 0)
	{
		exit(EXIT_FAILURE);
	}
	while (iteration < iterations)
	{
		add += 1;
		add += 1;
		add += 1;
		add += 1;
		add += 1;
		iteration += UNROLLING_CONST;
	}
	if (gettimeofday(&end, nullptr) != 0)
	{
		exit(EXIT_FAILURE);
	}
	auto micros = (double) (end.tv_usec - begin.tv_usec), nanos = (double) (end.tv_sec - begin.tv_sec);
	return (nanos * TO_NANO + micros * MIC_NAN) / iterations;
}


void empty_function()
{}

/**
 * Time measurement function for an empty function call.
 *  returns time in nano-seconds upon success,
 *  and -1 upon failure.
 */
double osm_function_time(unsigned int iterations)
{
	if (iterations <= 0)
	{
		return ERROR;
	}
	unsigned int diff = iterations % UNROLLING_CONST;
	if (diff != 0)
	{
		iterations += UNROLLING_CONST - diff;
	}
	unsigned int iteration = 0;
	struct timeval begin{}, end{};
	if (gettimeofday(&begin, nullptr) != 0)
	{
		exit(EXIT_FAILURE);
	}
	while (iteration < iterations)
	{
		empty_function();
		empty_function();
		empty_function();
		empty_function();
		empty_function();
		iteration += UNROLLING_CONST;
	}
	if (gettimeofday(&end, nullptr) != 0)
	{
		exit(EXIT_FAILURE);
	}
//	return ((double)(end.tv_sec - begin.tv_sec) * TO_NANO + ((double)(end.tv_usec - begin.tv_usec) * MIC_NAN)) / iterations;
	auto micros = (double) (end.tv_usec - begin.tv_usec), nanos = (double) (end.tv_sec - begin.tv_sec);
	return (nanos * TO_NANO + micros * MIC_NAN) / iterations;
}


/**
 * Time measurement function for an empty trap into the operating system.
 *  returns time in nano-seconds upon success,
 *  and -1 upon failure.
 */
double osm_syscall_time(unsigned int iterations)
{
	if (iterations <= 0)
	{
		return ERROR;
	}
	unsigned int diff = iterations % UNROLLING_CONST;
	if (diff != 0)
	{
		iterations += UNROLLING_CONST - diff;
	}
	unsigned int iteration = 0;
	struct timeval begin{}, end{};
	if (gettimeofday(&begin, nullptr) != 0)
	{
		exit(EXIT_FAILURE);
	}
	while (iteration < iterations)
	{
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
		iteration += UNROLLING_CONST;
	}
	if (gettimeofday(&end, nullptr) != 0)
	{
		exit(EXIT_FAILURE);
	}
	auto micros = (double) (end.tv_usec - begin.tv_usec), nanos = (double) (end.tv_sec - begin.tv_sec);
	return (nanos * TO_NANO + micros * MIC_NAN) / iterations;
}

int main(int argc, char **argv)
{
	unsigned int iterations = std::stoi(argv[1]);
	double process_time;

	// printing the time of simple operation.
	std::cout << "Operation Time:" << std::endl;
	process_time = osm_operation_time(iterations);
	std::cout << process_time << std::endl;

	// printing the time of empty function.
	std::cout << "Empty Function Time:" << std::endl;
	process_time = osm_function_time(iterations);
	std::cout << process_time << std::endl;

	// printing the time of a trap.
	std::cout << "Trap Time:" << std::endl;
	process_time = osm_syscall_time(iterations);
	std::cout << process_time << std::endl;

	return 0;
}


