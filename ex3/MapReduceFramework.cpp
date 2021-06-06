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
#include <algorithm>

#define ERR_CREATE "system error: Error in pthread_create function!"
#define ERR_LOAD "system error: Error in std::atomic load function!"
#define ERR_JOIN "system error: Error in pthread_join function!"
#define ERR_MUTEX "system error: Error in locking or unlocking mutex function!"
#define INIT_ATOMIC 0

//typedef struct ThreadContext ThreadContext;
typedef struct MyThread MyThread;
typedef struct JobContext JobContext;

std::vector<IntermediateVec> vectorVectors;


struct MyThread
{
	pthread_t _thread{};
	typedef struct ThreadContext
	{
		int id{};
		JobContext *_job{};
		IntermediateVec _vec{};
		pthread_mutex_t _mutex{PTHREAD_MUTEX_INITIALIZER};
		InputPair _pair{};
	} ThreadContext;
	ThreadContext _threadContext;
};

typedef MyThread::ThreadContext ThreadContext;

struct JobContext
{
	JobContext(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec) : _client(client),
	_inputVec(inputVec), _outputVec(outputVec)
	{
		for (int i = 0; i < _threadsNum; ++i)
		{
			// todo check this 3 lines - start
			auto temp = MyThread{};
			temp._threadContext = MyThread::ThreadContext{i, this};
			_threads.push_back(&temp);
			// todo - end
		}
	}

	~JobContext()
	{
		delete _state;
		delete _barrier;
		for (auto& it: _threads)
		{
			delete it;
		}
		_threads.clear();
		_keys.clear();
	}

	JobState *_state = new JobState{UNDEFINED_STAGE, 0};  //todo change 0 to 0.0?
	pthread_mutex_t _oMutex{PTHREAD_MUTEX_INITIALIZER};
	pthread_mutex_t _sMutex{PTHREAD_MUTEX_INITIALIZER};
	int _threadsNum{};
	const MapReduceClient &_client;
	const InputVec &_inputVec;
	OutputVec &_outputVec;
	std::atomic<int> _numPair{INIT_ATOMIC};
	std::atomic<int> _numMap{INIT_ATOMIC};
	std::atomic<int> _numMapFinish{INIT_ATOMIC};
	std::atomic<int> _numShuffle{INIT_ATOMIC};
	std::atomic<int> _numReduce{INIT_ATOMIC};
	std::atomic<int> _numReduceFinish{INIT_ATOMIC};
	Barrier *_barrier = new Barrier(_threadsNum);
	std::vector<ThreadContext> _threadContexts;
	std::vector<MyThread *> _threads;
	std::vector<K2*> _keys{};
	IntermediateVec _vec{};
};



static void system_library_exit(const std::string& msg)
{
	std::cerr << msg << std::endl;
	exit(EXIT_FAILURE);
}

//// gets a single pair (K1, V1) and calls emit2(K2,V2, context) any
//// number of times to output (K2, V2) pairs.
//void map(const K1 *key, const V1 *value, void *context)
//{
//
//}

bool sortHelper(IntermediatePair first, IntermediatePair second)
{
	return first.first->operator<(*second.first);
}

void sorting(void *context)
{
	auto contextC = (ThreadContext*) context;
	if (pthread_mutex_lock(&contextC->_mutex) != 0)
		system_library_exit(ERR_MUTEX);
	// todo like this or change to contextC._vec ??
	std::sort(contextC->_job->_threadContexts[contextC->id]._vec.begin(),
			  contextC->_job->_threadContexts[contextC->id]._vec.end(), sortHelper);
	if (pthread_mutex_unlock(&contextC->_mutex) != 0)
	{
		system_library_exit(ERR_MUTEX);
	}
}


// gets a single K2 key and a vector of all its respective V2 values
// calls emit3(K3, V3, context) any number of times (usually once)
// to output (K3, V3) pairs.
void reduce(void *context)  // todo const IntermediateVec *pairs,
{
	auto contextC = (ThreadContext*) context;
	auto job = contextC->_job;
	IntermediateVec vec;
	K2* key = job->_keys.back();
	for (; job->_numReduce < job->_keys.size(); ++job->_numReduce)
	{

		int reduNu = job->_numReduce;
//		job->_client.reduce(job->_keys[reduNu], job.);
		job->_numMapFinish++;
	}
}

void* shuffle(void* context)
{
	auto contextC = (JobContext *)context;
//	if (contextC->id != 0)
//	{
//		return nullptr;
//	}
	while (!contextC->_vec.empty())
	{
		IntermediateVec curVec;
		IntermediatePair cur = contextC->_vec.back();
		contextC->_vec.pop_back();
		curVec.push_back(cur);
		while (!sortHelper(cur, contextC->_vec.back()) && !sortHelper(contextC->_vec.back(), cur))
		{
			curVec.push_back(contextC->_vec.back());
			contextC->_vec.pop_back();
			if (contextC->_vec.empty())
				break;
		}
	}
	return nullptr;
}

void* mapFunc(void* context)
{
	auto contextC = (ThreadContext*) context;
	for (; contextC->_job->_numMap < contextC->_job->_inputVec.size(); ++contextC->_job->_numMap)
	{
		if (pthread_mutex_lock(&contextC->_mutex) != 0)
			system_library_exit(ERR_MUTEX);
		contextC->_pair = contextC->_job->_inputVec[contextC->_job->_numMap];
		contextC->_job->_client.map(contextC->_pair.first, contextC->_pair.second, context);
		contextC->_job->_numMapFinish++;
		if (pthread_mutex_unlock(&contextC->_mutex) != 0)
			system_library_exit(ERR_MUTEX);
	}
	sorting(context);
	contextC->_job->_barrier->barrier();
	sem_t thread0;
	sem_init(&thread0, 0, 0);
//	shuffle(context);
	if (contextC->id == 0)
	{
		shuffle(context);
	}
	reduce(context);
	return nullptr;
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
	auto job = new JobContext(client, inputVec, outputVec);
	job->_threadsNum = multiThreadLevel;
	job->_state->stage = MAP_STAGE;
	for (int i = 0; i < multiThreadLevel; ++i)
	{

		auto f = mapFunc;
		void * context = &job->_threads[i]->_threadContext;
		if (i == multiThreadLevel - 1)
			f = shuffle;

		if (pthread_create(&job->_threads[i]->_thread, nullptr, f, context) != 0)
			system_library_exit(ERR_CREATE);
	}
	return job;
}

/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * @param job returned by startMapReduceFramework
 */
void waitForJob(JobHandle job)
{
	auto jobC = (JobContext*) job;
	for (int i = 0; i < jobC->_threadsNum; ++i)
	{
		int result = pthread_join(jobC->_threads[i]->_thread, nullptr);
		if (result == 0 || result == ESRCH)
			continue;
		else // if (result != ESRCH && result != 0)  todo أبصر !!!!!!!
			system_library_exit(ERR_JOIN);
	}
}

/**
 * this function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job
 * @param state
 */
void getJobState(JobHandle job, JobState *state)
{
	auto jobC = (JobContext*) job;
	if (pthread_mutex_lock(&jobC->_sMutex) != 0)
		system_library_exit(ERR_MUTEX);
	float divide = (float) jobC->_inputVec.size() * 100;
	if (jobC->_state->stage == REDUCE_STAGE)
		divide = (float) jobC->_keys.size() * 100;
	else if (jobC->_state->stage == SHUFFLE_STAGE)
		divide = (float) jobC->_numPair * 100;
	jobC->_state->percentage = (float) (jobC->_numReduceFinish.load()) / divide;
	state = new JobState {jobC->_state->stage, jobC->_state->percentage};  //todo don't know!!!!!!
//	(*state){jobC->_state->stage, jobC->_state->percentage};
	if (pthread_mutex_unlock(&jobC->_sMutex) != 0)
		system_library_exit(ERR_MUTEX);
}

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished. After this
 * function is called the job handle will be invalid.
 * @param job
 */
void closeJobHandle(JobHandle job)
{
	waitForJob(job);
	auto jobC = (JobContext*) job;
	delete jobC;
}

/**
 * This function produces a (K2*, V2*) pair.
 * @param key intermediary element.
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit2(K2 *key, V2 *value, void *context)
{
	auto contextC = (ThreadContext*) context;
	if (pthread_mutex_lock(&contextC->_mutex) != 0)
		system_library_exit(ERR_MUTEX);
	contextC->_job->_numPair++;
	contextC->_vec.emplace_back(key, value);
	if (pthread_mutex_unlock(&contextC->_mutex) != 0)
		system_library_exit(ERR_MUTEX);
}

/**
 * This function creates (K3*, V3*)pair
 * @param key intermediary element
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit3(K3 *key, V3 *value, void *context)
{
	auto contextC = (ThreadContext*) context;
	if (pthread_mutex_lock(&contextC->_job->_oMutex) != 0)
		system_library_exit(ERR_MUTEX);
	contextC->_job->_outputVec.emplace_back(key, value);
	if (pthread_mutex_unlock(&contextC->_job->_oMutex) != 0)
		system_library_exit(ERR_MUTEX);
}