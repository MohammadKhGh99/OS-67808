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
#define ERR_SEM "system error: Error in sem_init function!"
#define INIT_ATOMIC 0

//typedef struct ThreadContext ThreadContext;
typedef struct MyThread MyThread;
typedef struct JobContext JobContext;

std::vector<IntermediateVec> vectorVectors;
sem_t semaphore;
pthread_mutex_t startMutex = PTHREAD_MUTEX_INITIALIZER;

struct MyThread
{
//	MyThread()= default;
	MyThread(int id, JobContext* job): _threadContext(ThreadContext(id, job)){}

	pthread_t _thread{};
	typedef struct ThreadContext
	{
//		ThreadContext()= default;
		ThreadContext(int id, JobContext* job): id(id), _job(job)
		{}
		int id{};
		JobContext *_job{};
		IntermediateVec _vec{};
		// todo another vector?
//		InputPair _pair{};
	} ThreadContext;
	ThreadContext _threadContext;
};

typedef MyThread::ThreadContext ThreadContext;

struct JobContext
{
	JobContext(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec) : _client(client),
	_inputVec(inputVec), _outputVec(outputVec), _numPair(0), _numMap(0), _numMapFinish(0), _numShuffle(0),
	_numReduce(0), _numReduceFinish(0)
	{

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
//		_keys.clear();
		pthread_mutex_destroy(&_e3Mutex);
		pthread_mutex_destroy(&_e2Mutex);

	}

	JobState *_state = new JobState{UNDEFINED_STAGE, 0};  //todo change 0 to 0.0?
	pthread_mutex_t _e3Mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t _e2Mutex = PTHREAD_MUTEX_INITIALIZER;
	int _threadsNum{};
	const MapReduceClient &_client;
	const InputVec &_inputVec;
	OutputVec &_outputVec;
	std::atomic<int> _numPair;
	std::atomic<int> _numMap;
	std::atomic<int> _numMapFinish;
	std::atomic<int> _numShuffle;
	std::atomic<int> _numReduce;
	std::atomic<int> _numReduceFinish;
	Barrier *_barrier = new Barrier(_threadsNum);
	std::vector<ThreadContext> _threadContexts;
	std::vector<MyThread *> _threads{};
//	std::vector<K2*> _keys{};
	IntermediateVec _vec{};
};



static void system_library_exit(const std::string& msg)
{
	std::cerr << msg << std::endl;
	exit(EXIT_FAILURE);
}

void freeAll()
{
	sem_destroy(&semaphore);

}

bool sortHelper(IntermediatePair first, IntermediatePair second)
{
	return first.first->operator<(*second.first);
}

void sorting(void *context)
{
	auto job = (JobContext *) context;
	if (pthread_mutex_lock(&job->_e2Mutex) != 0)
		system_library_exit(ERR_MUTEX);
	// todo like this or change to job._vec ??

	std::sort(job->_vec.begin(), job->_vec.end(), sortHelper);
	if (pthread_mutex_unlock(&job->_e2Mutex) != 0)
	{
		system_library_exit(ERR_MUTEX);
	}
}


// gets a single K2 key and a vector of all its respective V2 values
// calls emit3(K3, V3, context) any number of times (usually once)
// to output (K3, V3) pairs.
void reduce(void *context)  // todo const IntermediateVec *pairs,
{
	auto contextC = (JobContext *) context;
	contextC->_state->stage = REDUCE_STAGE;
	sem_wait(&semaphore);
	while (!vectorVectors.empty())
	{
		sem_wait(&semaphore);
		auto cur = &vectorVectors.back();
		contextC->_client.reduce(cur, contextC);
		contextC->_state->percentage = contextC->_numReduce / (float)contextC->_numPair;
		vectorVectors.pop_back();
	}
}



void* shuffle(void* context)
{
	auto job = (JobContext *)context;
    job->_state->stage = SHUFFLE_STAGE;
    while (true)
    {
        IntermediateVec toAdd;
        int idxPair = -1;
        IntermediatePair pair;
        for (int i = 0; i < job->_threadsNum; ++i)
        {
            if (!job->_threadContexts[i]._vec.empty())
            {
                if (idxPair == -1 || sortHelper(pair, job->_threadContexts[i]._vec.back()))
                {
                    idxPair = i;
                    pair = job->_threadContexts[i]._vec.back();
                }
            }
        }
        if (idxPair != -1)
        {
            IntermediatePair curPair = job->_threadContexts[idxPair]._vec.back();
            job->_threadContexts[idxPair]._vec.pop_back();

            int flag = 0;
            while (!flag)
            {
                flag = 1;
                for (const auto& it: job->_threadContexts)
                {
                    IntermediateVec curVec = it._vec;
                    if (!curVec.empty() && !sortHelper(curPair, curVec.back()) && !sortHelper(curVec.back(), curPair))
                    {
                        toAdd.push_back(curVec.back());
                        curVec.pop_back();
                        flag = 0;
                    }
                }
            }
            toAdd.push_back(curPair);
            job->_numShuffle += toAdd.size();
            vectorVectors.push_back(toAdd);
            toAdd.clear();
        }
        else
        {
            break;
        }

    }
    job->_state->percentage = job->_numShuffle / (float) job->_numPair;
	return nullptr;
}

void* mapFunc(void* context)
{
//	auto contextC = (ThreadContext*) context;
	auto job = (JobContext*) context;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//	auto typ = typeof(contextC->_job->_inputVec.size());
    int old;
//    std::cout<<"before"<<old<<std::endl;
    while (true)
    {
        if (pthread_mutex_lock(&mutex) != 0)
            system_library_exit(ERR_MUTEX);
        if (job->_numMap < (int) job->_inputVec.size())
        {
            old = job->_numMap++;
            InputPair pair = job->_inputVec[old];
            if (old < (int) job->_inputVec.size())
            {
                job->_client.map(pair.first, pair.second, &job->_threadContexts[old]);
            }
            if  (pthread_mutex_unlock(&mutex) != 0)
            {
                system_library_exit(ERR_MUTEX);
            }
        }
        else
        {
            if (pthread_mutex_unlock(&mutex) != 0)
            {
                system_library_exit(ERR_MUTEX);
            }
            break;
        }

    }
//	for (; job->_numMap < (int)job->_inputVec.size(); job->_numMap++)
//	{
//	    old = job->_numMap;
//        std::cout<<"#"<<old<<std::endl;
//		if (pthread_mutex_lock(&mutex) != 0)
//			system_library_exit(ERR_MUTEX);
//		// todo like this or switch to [...]
//		InputPair cur = job->_inputVec.at(static_cast<unsigned long>(old));
//		job->_client.map(cur.first, cur.second, &job->_threadContexts[old]);
//		job->_numMapFinish++;
//		job->_state->percentage = job->_numMapFinish / (float) job->_inputVec.size();
//		if (pthread_mutex_unlock(&mutex) != 0)
//			system_library_exit(ERR_MUTEX);
//		std::cout<<"end loop "<<old<<std::endl;
//	}
//    std::cout<<"after old"<<old<<std::endl;
//    std::cout<<"after numMap"<<job->_numMap<<std::endl;
	return nullptr;
}

void* mapSortShuffleReduce(void* context)
{
	mapFunc(context);
	sorting(context);
	auto contextC = (ThreadContext*) context;
	contextC->_job->_barrier->barrier();
	// semaphore
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
	if (pthread_mutex_lock(&startMutex) != 0)
	{
		system_library_exit(ERR_MUTEX);
	}
	auto job = new JobContext(client, inputVec, outputVec);
	job->_threadsNum = multiThreadLevel;
	job->_state->stage = MAP_STAGE;
	if (sem_init(&semaphore, 0, 0) != 0)
	{
		system_library_exit(ERR_SEM);
	}
	job->_barrier = new Barrier(multiThreadLevel);
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	for (int i = 0; i < multiThreadLevel; ++i)
	{
		if (pthread_mutex_lock(&mutex) != 0)
		{
			system_library_exit(ERR_MUTEX);
		}
		job->_threads.push_back(new MyThread(i, job));
		job->_threadContexts.push_back(job->_threads.back()->_threadContext);
		auto f = mapSortShuffleReduce;
		if (pthread_create(&job->_threads[i]->_thread, nullptr, f, job) != 0)
			system_library_exit(ERR_CREATE);
		if (pthread_mutex_unlock(&mutex) != 0)
		{
			system_library_exit(ERR_MUTEX);
		}
	}
	if (pthread_mutex_unlock(&startMutex) != 0)
	{
		system_library_exit(ERR_MUTEX);
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
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	for (int i = 0; i < jobC->_threadsNum; ++i)
	{
		if (pthread_mutex_lock(&mutex) != 0)
		{
			system_library_exit(ERR_MUTEX);
		}
		int result = pthread_join(jobC->_threads[i]->_thread, nullptr);
		if (result == 0 || result == ESRCH)
		{
			if (pthread_mutex_unlock(&mutex) != 0)
			{
				system_library_exit(ERR_MUTEX);
			}
			continue;
		}
		else // if (result != ESRCH && result != 0)  todo أبصر !!!!!!!
		{
			if (pthread_mutex_unlock(&mutex) != 0)
			{
				system_library_exit(ERR_MUTEX);
			}
			system_library_exit(ERR_JOIN);
		}

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
	if (pthread_mutex_lock(&jobC->_e2Mutex) != 0)
		system_library_exit(ERR_MUTEX);
//	float divide = 0.0;
//	if (jobC->_state->stage == REDUCE_STAGE)
//		divide = (float) jobC->_numReduce * 100;
////		divide = (float) jobC->_keys.size() * 100;
//	else if (jobC->_state->stage == SHUFFLE_STAGE)
//		divide = (float) jobC->_numPair * 100;
//	else if (jobC->_state->stage == MAP_STAGE)
//		divide = (float) jobC->_inputVec.size() * 100;
//	jobC->_state->percentage = (float) (jobC->_numReduceFinish.load()) / divide;

	jobC->_state = state;
//	state = new JobState {jobC->_state->stage, jobC->_state->percentage};  //todo don't know!!!!!!
//	(*state){jobC->_state->stage, jobC->_state->percentage};
	if (pthread_mutex_unlock(&jobC->_e2Mutex) != 0)
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
	auto job = (JobContext*) context;
	if (pthread_mutex_lock(&job->_e2Mutex) != 0)
		system_library_exit(ERR_MUTEX);
	job->_numPair++;
	job->_vec.emplace_back(key, value);
	if (pthread_mutex_unlock(&job->_e2Mutex) != 0)
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
	auto job = (JobContext *) context;
	if (pthread_mutex_lock(&job->_e3Mutex) != 0)
		system_library_exit(ERR_MUTEX);
	job->_numReduce++;
	job->_outputVec.emplace_back(key, value);
	if (pthread_mutex_unlock(&job->_e3Mutex) != 0)
		system_library_exit(ERR_MUTEX);
}