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
pthread_mutex_t semMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t startMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mapMutex = PTHREAD_MUTEX_INITIALIZER;
JobContext* globalJob;
int old;
bool done = false;
Barrier* barrier;
pthread_cond_t cv;

struct MyThread
{
//	MyThread()= default;
	MyThread(int id, JobContext* job): _threadContext(ThreadContext()){
	    _threadContext.id = id;
	    _threadContext._job = job;
	}

	pthread_t _thread{};
	typedef struct ThreadContext
	{
//		ThreadContext()= default;
//		ThreadContext(int id, JobContext* job): id(id), _job(job)
//		{}
		int id{};
		JobContext *_job{};
		IntermediateVec* _vec = new IntermediateVec ;
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
	ThreadContext *_threadContexts;
//	std::vector<ThreadContext> _threadContexts;
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
//	auto job = (JobContext *) context;
    auto contextC = (ThreadContext *) context;

    if (pthread_mutex_lock(&globalJob->_e2Mutex) != 0)
		system_library_exit(ERR_MUTEX);
	// todo like this or change to job._vec ??
    std::cout<<"Sorting#"<<contextC->id<<std::endl;
	std::sort(contextC->_vec->begin(), contextC->_vec->end(), sortHelper);
	if (pthread_mutex_unlock(&globalJob->_e2Mutex) != 0)
	{
		system_library_exit(ERR_MUTEX);
	}
}


// gets a single K2 key and a vector of all its respective V2 values
// calls emit3(K3, V3, context) any number of times (usually once)
// to output (K3, V3) pairs.
void reduce(void *context)  // todo const IntermediateVec *pairs,
{
    auto contextC = (ThreadContext*)context;
    std::cout<<"ReduceThreadID: "<<contextC->id<<std::endl;
	globalJob->_state->stage = REDUCE_STAGE;
	while (!vectorVectors.empty())
	{
		auto cur = &vectorVectors.back();
		globalJob->_client.reduce(cur, context);
		globalJob->_state->percentage = (float) globalJob->_numReduce / (float)globalJob->_numPair;
		vectorVectors.pop_back();
	}
	std::cout<<"AfterReduceID: "<<contextC->id<<std::endl;
}



void* shuffle(void* context)
{
//	auto job = (JobContext *)context;
    auto contextC = (ThreadContext*)context;
    std::cout<<"ShuffleThreadID: "<<contextC->id<<std::endl;
    globalJob->_state->stage = SHUFFLE_STAGE;
    IntermediateVec *toAdd;
    IntermediatePair *curPair;
    while (true)
    {
        toAdd = new IntermediateVec;
        int idxPair = -1;
        for (int i = 0; i < globalJob->_threadsNum; ++i)
        {
            if (!globalJob->_threadContexts[i]._vec->empty())
            {
                if (idxPair == -1 || sortHelper(*curPair, globalJob->_threadContexts[i]._vec->back()))
                {
                    idxPair = i;
                    curPair = &globalJob->_threadContexts[i]._vec->back();
                }
            }
        }
        if (idxPair != -1)
        {
//            IntermediatePair curPair = globalJob->_threadContexts[idxPair]._vec->back();
            globalJob->_threadContexts[idxPair]._vec->pop_back();
            bool flag = false;
            for (int i = 0; i < globalJob->_threadsNum; ++i) {
                flag = false;
                IntermediateVec* curVec = globalJob->_threadContexts[i]._vec;
                while (!curVec->empty() && !flag)
                {
                    if (!sortHelper(*curPair, curVec->back()) && !sortHelper(curVec->back(), *curPair))
                    {
                        toAdd->push_back(curVec->back());
                        curVec->pop_back();
                    }
                    else
                    {
                        flag = true;
                    }
                }
//                globalJob->_threadContexts[i]._vec = curVec;
            }
            toAdd->push_back(*curPair);
            globalJob->_numShuffle += (int)toAdd->size();
            vectorVectors.push_back(*toAdd);
            delete toAdd;
//            toAdd = new IntermediateVec;
//            toAdd.clear();
        }
        else
        {
            delete toAdd;
            break;
        }

    }
    globalJob->_state->percentage = (float)globalJob->_numShuffle / (float) globalJob->_numPair;
    done = true;
    for (int i = 0; i < (int)vectorVectors.size(); ++i) {

    }
	return nullptr;
}

void* mapFunc(void* context)
{
	auto contextC = (ThreadContext*) context;
	std::cout<<"MappingThreadID: "<<contextC->id<<std::endl;
//    std::cout<<"MappingID: "<<globalJob->_threads[contextC->id]->_threadContext.id<<std::endl;
//	auto job = (JobContext*) context;
//	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//	auto typ = typeof(contextC->_job->_inputVec.size());
//    int old;
//    std::cout<<"before"<<old<<std::endl;
//    int i = 0;
    while (true)
    {
//        std::cout<<"Mapping#"<<i<<std::endl;
        if (pthread_mutex_lock(&mapMutex) != 0)
            system_library_exit(ERR_MUTEX);
        if (globalJob->_numMap < (int) globalJob->_inputVec.size())
        {
            old = globalJob->_numMap++;
            InputPair pair = globalJob->_inputVec[old];
            if (old < (int) globalJob->_inputVec.size())
            {
                globalJob->_client.map(pair.first, pair.second, &globalJob->_threadContexts[old]);
            }
            if  (pthread_mutex_unlock(&mapMutex) != 0)
            {
                system_library_exit(ERR_MUTEX);
            }
        }
        else
        {
            if (pthread_mutex_unlock(&mapMutex) != 0)
            {
                system_library_exit(ERR_MUTEX);
            }
            break;
        }
//    i++;
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
int value;
void* mapSortShuffleReduce(void* context)
{
	mapFunc(context);
	sorting(context);
	auto contextC = (ThreadContext*) context;
	barrier->barrier();

	if (contextC->id == 0)
	{
		shuffle(context);
        int count = 0;
        for (auto & vectorVector : vectorVectors) {
            count += (int)vectorVector.size();
        }
        std::cout<<"VectorsNum: "<<count<<std::endl;
        std::cout<<"VectorsNum123: "<<globalJob->_numShuffle<<std::endl;
		std::cout<<"AfterShuffle: "<<contextC->id<<std::endl;
//        if (pthread_mutex_lock(&semMutex) != 0)
//        {
//            system_library_exit(ERR_MUTEX);
//        }
        for (int i = 0; i < globalJob->_threadsNum; ++i) {
            sem_post(&semaphore);

        }
//		pthread_cond_broadcast(&cv);
//        pthread_cond_destroy(&cv);
//        if (pthread_mutex_unlock(&semMutex) != 0)
//        {
//            system_library_exit(ERR_MUTEX);
//        }
//        reduce(context);
	}
	else if (!done)
    {
//	    std::cout<<"Stop! You Number: "<<contextC->id<<std::endl;
//	    globalJob->_threads[contextC->id]->_thread;
//        sem_wait(&semaphore);
//        if (pthread_mutex_lock(&semMutex) != 0)
//        {
//            system_library_exit(ERR_MUTEX);
//        }
//        sem_getvalue(&semaphore, &value);
//        std::cout<<"SemaphoreValBefore: "<<value<<std::endl;
        sem_wait(nullptr);
//        sem_wait(&semaphore);
//        sem_getvalue(&semaphore, &value);
//        std::cout<<"SemaphoreValAfter: "<<value<<std::endl;
//        pthread_cond_wait(&cv, &semMutex);
//        if (pthread_mutex_unlock(&semMutex) != 0)
//        {
//            system_library_exit(ERR_MUTEX);
//        }
//        if (done)
//        {
//
//            sem_post(&semaphore);
//        }
    }
//    {
    reduce(context);
//    }
//    sem_destroy(&semaphore);
    std::cout<<"Done!"<<std::endl;
	pthread_exit(nullptr);
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
	globalJob = new JobContext (client, inputVec, outputVec);
//	auto job = new JobContext(client, inputVec, outputVec);
	globalJob->_threadsNum = multiThreadLevel;
	globalJob->_state->stage = MAP_STAGE;
	barrier = new Barrier(multiThreadLevel);
	if (sem_init(&semaphore, 0, multiThreadLevel - 1) != 0)
	{
		system_library_exit(ERR_SEM);
	}
//	globalJob->_barrier = new Barrier(multiThreadLevel);
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    globalJob->_threadContexts = new ThreadContext[multiThreadLevel];

	for (int i = 0; i < multiThreadLevel; ++i)
    {
        if (pthread_mutex_lock(&mutex) != 0)
        {
            system_library_exit(ERR_MUTEX);
        }
        globalJob->_threads.push_back(new MyThread(i, globalJob));
        globalJob->_threadContexts[i] = globalJob->_threads.back()->_threadContext;
//        globalJob->_threadContexts.push_back(globalJob->_threads.back()->_threadContext);
        if (pthread_mutex_unlock(&mutex) != 0)
        {
            system_library_exit(ERR_MUTEX);
        }
    }

	for (int i = 0; i < multiThreadLevel; ++i)
	{
		if (pthread_mutex_lock(&mutex) != 0)
		{
			system_library_exit(ERR_MUTEX);
		}
//		globalJob->_threads.push_back(new MyThread(i, globalJob));
//		globalJob->_threadContexts.push_back(globalJob->_threads.back()->_threadContext);

		if (pthread_create(&globalJob->_threads[i]->_thread, nullptr, mapSortShuffleReduce, &globalJob->_threadContexts[i]) != 0)
			system_library_exit(ERR_CREATE);
		if (pthread_mutex_unlock(&mutex) != 0)
		{
			system_library_exit(ERR_MUTEX);
		}
	}
//	for (int i = 0; i < multiThreadLevel; ++i)
//    {
//	    if (pthread_mutex_lock(&mutex) != 0)
//        {
//	        system_library_exit(ERR_MUTEX);
//        }
//	    if (pthread_join(globalJob->_threads[i]->_thread, nullptr) != 0)
//        {
//	        system_library_exit(ERR_JOIN);
//        }
//        if (pthread_mutex_unlock(&startMutex) != 0)
//        {
//            system_library_exit(ERR_MUTEX);
//        }
//    }
	if (pthread_mutex_unlock(&startMutex) != 0)
	{
		system_library_exit(ERR_MUTEX);
	}
	return globalJob;
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

	jobC->_state = state;

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
//	auto job = (JobContext*) context;
    auto contextC = (ThreadContext*) context;
	if (pthread_mutex_lock(&globalJob->_e2Mutex) != 0)
		system_library_exit(ERR_MUTEX);
//	job->_numPair++;
	contextC->_job->_numPair++;
	contextC->_vec->emplace_back(key, value);
	if (pthread_mutex_unlock(&globalJob->_e2Mutex) != 0)
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
//	auto job = (JobContext *) context;
    auto contextC = (ThreadContext*) context;
	if (pthread_mutex_lock(&globalJob->_e3Mutex) != 0)
		system_library_exit(ERR_MUTEX);
//	job->_numReduce++;
//	job->_outputVec.emplace_back(key, value);
//    globalJob->_outputVec.emplace_back(key, value);
//    globalJob->_numReduce++;
    contextC->_job = globalJob;
	contextC->_job->_outputVec.emplace_back(key, value);
	contextC->_job->_numReduce++;
	if (pthread_mutex_unlock(&globalJob->_e3Mutex) != 0)
		system_library_exit(ERR_MUTEX);
}