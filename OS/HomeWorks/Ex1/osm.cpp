#include "osm.h"
#include <sys/time.h>
#include <math.h>
#include <iostream>

const double SEC_TO_MICRS = 1000000.0;
const double SEC_TO_NANOS = 1000000000.0;
const double SEC_IN_MICRS = 1/SEC_TO_MICRS;
const double MICROS_TO_NANOS = 1000;
double osm_operation_time(unsigned int iterations)
{
    unsigned int number_iterations = 0;
    int operation_var = 0;
    struct timeval start, end, diff;
    // Start the clock!
    gettimeofday(&start, nullptr);
    while (number_iterations < iterations)
    {
        operation_var += 1;
        operation_var += 1;
        operation_var += 1;
        operation_var += 1;
        operation_var += 1;
        number_iterations += 5;
    }
    // Stop the clock!
    gettimeofday(&end,nullptr);
    double time_nano = ((end.tv_sec - start.tv_sec)*SEC_TO_NANOS+((end.tv_usec - start.tv_usec) * MICROS_TO_NANOS));
    return  time_nano/iterations;
}

void empty_function(){};
double osm_function_time(unsigned int iterations)
{
    unsigned int number_iterations = 0;
    struct timeval start, end;
    // Start the clock!
    gettimeofday(&start, nullptr);
    while (number_iterations < iterations)
    {
        empty_function();
        empty_function();
        empty_function();
        empty_function();
        empty_function();
        number_iterations += 5;
    }
    // Stop the clock!
    gettimeofday(&end, nullptr);
    double time_nano = ((end.tv_sec - start.tv_sec)*SEC_TO_NANOS+((end.tv_usec - start.tv_usec) * MICROS_TO_NANOS));
    return  time_nano/iterations;

}
double osm_syscall_time(unsigned int iterations)
{
    unsigned int number_iterations = 0;
    struct timeval start, end;
    // Start the clock!
    gettimeofday(&start, nullptr);
    while (number_iterations < iterations)
    {
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        number_iterations += 5;
    }
    // Stop the clock!
    gettimeofday(&end, nullptr);
    double time_nano = ((end.tv_sec - start.tv_sec)*SEC_TO_NANOS+((end.tv_usec - start.tv_usec) * MICROS_TO_NANOS));
    return  time_nano/iterations;
}

int main(int argc ,char** argv){
	unsigned int iterations= std::stoi(argv[1]);
	double time;
	std::cout<<"operation time:"<<std::endl;
	time=osm_operation_time(iterations);
	std::cout<<time<<std::endl;
	std::cout<<"empty function time: "<<std::endl;
	time=osm_function_time(iterations);
	std::cout<<time<<std::endl;
	std::cout<<"trap function time: "<<std::endl;
	time=osm_syscall_time(iterations);
	std::cout<<time<<std::endl;
	return 0;
}
