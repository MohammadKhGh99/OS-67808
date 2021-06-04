//
// Created by m7mdg on 03-Jun-21.
//

#include "MapReduceFramework.h"
#include "Barrier.h"
#include <atomic>
#include <cstdlib>
#include <pthread.h>
#include <vector>
#include <iostream>
#include <semaphore.h>

#define ERR_CREATE "system error: Error in pthread_create function"
#define ERR_LOAD "system error: Error in std::atomic load function"
#define INIT_ATOMIC 0

typedef struct JobContext
{
	JobContext(const MapReduceClient& client, const InputVec& inputVec, OutputVec& outputVec) : _client(client),
	_inputVec(inputVec), _outputVec(outputVec)
	{}

	JobState *_state = new JobState{UNDEFINED_STAGE, 0};  //todo change 0 to 0.0?
	pthread_mutex_t _oMutex{PTHREAD_MUTEX_INITIALIZER};
	pthread_mutex_t _sMutex{PTHREAD_MUTEX_INITIALIZER};
	int _threadsNum{};
	const MapReduceClient& _client;
	const InputVec& _inputVec;
	OutputVec& _outputVec;
	std::atomic<int> _numPair{INIT_ATOMIC};
	std::atomic<int> _numMap{INIT_ATOMIC};
	std::atomic<int> _numMapFinish{INIT_ATOMIC};
	std::atomic<int> _numShuffle{INIT_ATOMIC};
	std::atomic<int> _numReduce{INIT_ATOMIC};
	std::atomic<int> _numReduceFinish{INIT_ATOMIC};
	Barrier* _barrier = new Barrier(_threadsNum);
} JobContext;

typedef struct MyThread
{
	typedef struct ThreadContext
	{

	} ThreadContext;
} MyThread;

static void system_library_exit(char *msg)
{
	std::cerr<<msg<<std::endl;
	exit(EXIT_FAILURE);
}

// gets a single pair (K1, V1) and calls emit2(K2,V2, context) any
// number of times to output (K2, V2) pairs.
void map(const K1 *key, const V1 *value, void *context)
{

}

// gets a single K2 key and a vector of all its respective V2 values
// calls emit3(K3, V3, context) any number of times (usually once)
// to output (K3, V3) pairs.
void reduce(const IntermediateVec *pairs, void *context)
{

}

/**
 * This function starts running the MapReduce algorithm (with several threads) and returns a JobHandle.
 * @param client The implementation of MapReduceClient or in other words the task that the framework should run.
 * @param inputVec a vector of type std::vector<std::pair<K1*, V1*>>, the input elements.
 * @param outputVec a vector of type std::vector<std::pair<K3*, V3*>>, to which the output elements will be added
 * before returning.
 * @param multiThreadLevel the number of worker threads to be used for running the algorithm.
 * @return The function returns JobHandle that will be used for monitoring the job.
 */
JobHandle startMapReduceJob(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec,
							int multiThreadLevel)
{
	auto jobHandle = new JobContext(client, inputVec, outputVec);
	jobHandle->_threadsNum = multiThreadLevel;
	jobHandle->_state->stage = MAP_STAGE;

}

/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * @param job returned by startMapReduceFramework
 */
void waitForJob(JobHandle job)
{

}

/**
 * this function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job
 * @param state
 */
void getJobState(JobHandle job, JobState *state)
{

}

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished. After this
 * function is called the job handle will be invalid.
 * @param job
 */
void closeJobHandle(JobHandle job)
{

}

/**
 * This function produces a (K2*, V2*) pair.
 * @param key intermediary element.
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit2(K2 *key, V2 *value, void *context)
{

}

/**
 *
 * @param key intermediary element
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit3(K3 *key, V3 *value, void *context)
{

}