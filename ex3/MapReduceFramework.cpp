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
#define ERR_JOIN "system error: Error in pthread_join function!"
#define ERR_MUTEX_LOCK "system error: Error in locking mutex function!"
#define ERR_MUTEX_UNLOCK "system error: Error in unlocking mutex function!"
#define ERR_DESTROY "system error: Error in pthread_mutex_destroy function!"
#define ERR_SEM_INIT "system error: Error in sem_init function!"
#define ERR_SEM_DESTROY "system error: Error in sem_destroy function!"
#define INIT_ATOMIC 0
#define UNDEFINED 0.0f
#define FINISH 100.0f
#define SUCCESS 0
#define SHUFFLE_THREAD 0

typedef struct JobContext JobContext;

typedef struct Thread {
    int id{};
    JobContext *_job{};
    IntermediateVec *_vec{};
    pthread_t *_thread{};
    pthread_mutex_t _e2Mutex{};
    bool joined = false;
} Thread;


struct JobContext {
    JobState *_state{};
    int _threadsNum{};
    MapReduceClient *_client{};
    InputVec *_inputVec{};
    OutputVec *_outputVec{};
    //Atomic counter for the number of pairs in total.
    std::atomic<int> _numPair{INIT_ATOMIC};
    //Atomic counter for the number of pairs of the input that have been mapped so far.
    std::atomic<int> _numMap{INIT_ATOMIC};
    //Atomic counter for the number of shuffled pairs so far.
    std::atomic<int> _numShuffle{INIT_ATOMIC};
    //Atomic counter for the number of reduced pairs so far.
    std::atomic<int> _numReduce{INIT_ATOMIC};
    Barrier _barrier = Barrier(_threadsNum);
    Thread *_threads{};
    std::vector<IntermediateVec> _vectorVectors{};
    //Mutex for emit3 function
    pthread_mutex_t _e3Mutex{};
    //Mutex for reduce phase.
    pthread_mutex_t reduceMutex{};
    //Mutex for mapping phase.
    pthread_mutex_t mapMutex{};
    //Mutex for switching between stages.
    pthread_mutex_t switchMutex{};
    //Mutex for waitForJob function.
    pthread_mutex_t waitMutex{};
    sem_t semaphore{};
    bool waitCalled = false;
};

//----------------- Functions Declarations -----------------//

void *mapSortShuffleReduce(void *context);
void mapPhase(void *context);
bool sortHelper(IntermediatePair first, IntermediatePair second);
void sortPhase(void *context);
void shufflePhase(void *context);
void reducePhase(void *context);
static void system_library_exit(const std::string &msg);
void releaseAll(JobHandle);

//----------------- Functions Implementations -----------------//

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
                            int multiThreadLevel) {
    //Making a new JobContext
	auto job = new JobContext;
    job->_state = new JobState{UNDEFINED_STAGE, UNDEFINED};
    job->_threadsNum = multiThreadLevel;
    job->_client = const_cast<MapReduceClient *>(&client);
    job->_inputVec = const_cast<InputVec *>(&inputVec);
    job->_outputVec = &outputVec;
    job->_barrier = Barrier(multiThreadLevel);
    job->_threads = new Thread[multiThreadLevel];
    job->_e3Mutex = PTHREAD_MUTEX_INITIALIZER;
    job->reduceMutex = PTHREAD_MUTEX_INITIALIZER;
    job->mapMutex = PTHREAD_MUTEX_INITIALIZER;
    job->switchMutex = PTHREAD_MUTEX_INITIALIZER;
    job->waitMutex = PTHREAD_MUTEX_INITIALIZER;
    if (inputVec.empty())
    {
        job->_state->percentage = FINISH;
        job->_state->stage = REDUCE_STAGE;
    } else {
    	//Initializing and creating the threads.
        for (int i = 0; i < job->_threadsNum; ++i) {
            job->_threads[i].id = i;
            job->_threads[i]._job = job;
            job->_threads[i]._vec = new IntermediateVec;
            job->_threads[i]._thread = new pthread_t;
            job->_threads[i]._e2Mutex = PTHREAD_MUTEX_INITIALIZER;
        }
        if (sem_init(&job->semaphore, 0, 0) != SUCCESS) {
            system_library_exit(ERR_SEM_INIT);
        }
        for (int i = 0; i < multiThreadLevel; ++i) {
            if (pthread_create(job->_threads[i]._thread, nullptr, mapSortShuffleReduce, &job->_threads[i]) != SUCCESS) {
                system_library_exit(ERR_CREATE);
            }
        }
    }
    return job;
}

/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * @param job returned by startMapReduceFramework
 */
void waitForJob(JobHandle job) {
    auto jobC = (JobContext *) job;
    //Check the current job as "waited" because we cannot call waitForJob twice for the same job.
    if (jobC->waitCalled)
    {
        return;
    }
    if (pthread_mutex_lock(&jobC->waitMutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_LOCK);
    }
    //Join all the threads.
    for (int i = 0; i < jobC->_threadsNum; ++i) {
        if (!jobC->_threads[i].joined && pthread_join(*jobC->_threads[i]._thread, nullptr) != SUCCESS) {
            system_library_exit(ERR_JOIN);
        }
        jobC->_threads[i].joined = true;
    }
    jobC->waitCalled = true;
    if (pthread_mutex_unlock(&jobC->waitMutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_UNLOCK);
    }
}

/**
 * this function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job the JobContext to update the state argument with its state.
 * @param state the state to update.
 */
void getJobState(JobHandle job, JobState *state) {
    auto jobC = (JobContext *) job;
    if (pthread_mutex_lock(&jobC->switchMutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_LOCK);
    }
    //Updating the current state's percentage depending on the current stage.
    if (jobC->_state->stage == MAP_STAGE)
    {
        jobC->_state->percentage = (float) 100 * jobC->_numMap / (float) jobC->_inputVec->size();
    } else if (jobC->_state->stage == SHUFFLE_STAGE)
    {
        jobC->_state->percentage = (float) 100 * jobC->_numShuffle / (float) jobC->_numPair;
    } else if (jobC->_state->stage == REDUCE_STAGE)
    {
        jobC->_state->percentage = (float) 100 * jobC->_numReduce / (float) jobC->_numShuffle;
    }
    state->percentage = jobC->_state->percentage;
    state->stage = jobC->_state->stage;
    if (pthread_mutex_unlock(&jobC->switchMutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_UNLOCK);
    }
}

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished. After this
 * function is called the job handle will be invalid.
 * @param job the JobContext to close.
 */
void closeJobHandle(JobHandle job) {
	//Waiting for all the threads to finish their jobs.
    waitForJob(job);
    //Releasing all the resources.
    releaseAll(job);
}

/**
 * This function produces a (K2*, V2*) pair.
 * @param key intermediary element.
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit2(K2 *key, V2 *value, void *context) {
    auto contextC = (Thread *) context;
    if (pthread_mutex_lock(&contextC->_e2Mutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_LOCK);
    }
    contextC->_job->_numPair++;
    contextC->_vec->emplace_back(key, value);
    if (pthread_mutex_unlock(&contextC->_e2Mutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_UNLOCK);
    }
}

/**
 * This function creates (K3*, V3*)pair
 * @param key intermediary element
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit3(K3 *key, V3 *value, void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    if (pthread_mutex_lock(&job->_e3Mutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_LOCK);
    }
    contextC->_job->_outputVec->emplace_back(key, value);
    if (pthread_mutex_unlock(&job->_e3Mutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_UNLOCK);
    }
}

/**
 * This function starts all the MapReduce process, when creating a thread this function called.
 * @param context the context of the thread that has been created.
 * @return nullptr when finishing the process.
 */
void *mapSortShuffleReduce(void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    //Start Map Phase process.
    mapPhase(context);
    //Start Sort Phase process.
    sortPhase(context);
    //All the threads waiting in the barrier for all the threads to finish their work.
    job->_barrier.barrier();

    if (contextC->id == SHUFFLE_THREAD) {
    	//Start Shuffle Phase process for thread 0.
        shufflePhase(context);
        for (int i = 0; i < job->_threadsNum - 1; ++i) {
            if (sem_post(&job->semaphore) != SUCCESS) {
                system_library_exit(ERR_SEM_INIT);
            }
        }
    }else if (job->_threadsNum > 1) {
        if (sem_wait(&job->semaphore) != SUCCESS) {
            system_library_exit(ERR_SEM_INIT);
        }
    }
    //Start Reduce Phase process.
    reducePhase(context);
    //Finish the process of the current thread, we could use pthread_exit, but we think that it is the same when
    // using join to wait for other threads' return\exit value.
    return nullptr;
}

/**
 * This function do the Map Phase process.
 * @param context the context of the thread.
 */
void mapPhase(void *context)
{
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    if (job->_state->stage == UNDEFINED_STAGE) {
        job->_state->stage = MAP_STAGE;
    }
    int size = job->_inputVec->size();
    while (job->_numMap < size) {
        if (pthread_mutex_lock(&job->mapMutex) != SUCCESS) {
            system_library_exit(ERR_MUTEX_LOCK);
        }
        if (job->_numMap < size) {
            auto pair = job->_inputVec->at(job->_numMap);
            job->_numMap++;
            job->_client->map(pair.first, pair.second, contextC);
        }
        if (pthread_mutex_unlock(&job->mapMutex) != SUCCESS) {
            system_library_exit(ERR_MUTEX_UNLOCK);
        }
    }
}

/**
 * This function help with the sorting phase process, compares two pairs.
 * @param first the first pair to compare.
 * @param second the second pair to compare.
 * @return if the first pair is smaller than the second.
 */
bool sortHelper(IntermediatePair first, IntermediatePair second) {
    return first.first->operator<(*second.first);
}

/**
 * This function do the Sort Phase process.
 * @param context the context of the current thread.
 */
void sortPhase(void *context) {
    auto contextC = (Thread *) context;
    if (!contextC->_vec->empty())
    {
        std::sort(contextC->_vec->begin(), contextC->_vec->end(), sortHelper);
    }
}

/**
 * This function do the Shuffle Phase process.
 * @param context the context of the current thread, in our case is the Thread 0.
 */
void shufflePhase(void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    if (pthread_mutex_lock(&job->switchMutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_LOCK);
    }
    if (job->_state->stage == MAP_STAGE) {
        job->_state->stage = SHUFFLE_STAGE;
    }
    if (pthread_mutex_unlock(&job->switchMutex) != SUCCESS)
    {
        system_library_exit(ERR_MUTEX_UNLOCK);
    }
    while (true) {
        IntermediateVec toAdd;
        IntermediatePair curPair;
        //Finding the largest key in threads' vectors.
        int idxPair = -1;
        for (int i = 0; i < job->_threadsNum; ++i) {
            if (!job->_threads[i]._vec->empty()) {
                if (idxPair == -1 || sortHelper(curPair, job->_threads[i]._vec->back())) {
                    idxPair = i;
                    curPair = job->_threads[i]._vec->back();
                }
            }
        }
        if (idxPair != -1) {
            bool flag;
            for (int i = 0; i < job->_threadsNum; ++i) {
            	//Obtaining the mutex.
                if (pthread_mutex_lock(&job->_threads[i]._e2Mutex) != SUCCESS)
                {
                    system_library_exit(ERR_MUTEX_LOCK);
                }
                flag = false;
                IntermediateVec *curVec = job->_threads[i]._vec;
                //Finding all the pairs that the keys in them are equal to each other.
                while (!curVec->empty() && !flag) {
                    if (!sortHelper(curPair, curVec->back()) && !sortHelper(curVec->back(), curPair)) {
                        toAdd.push_back(curVec->back());
                        curVec->pop_back();
                        job->_numShuffle++;
                    } else {
                        flag = true;
                    }
                }
                if (pthread_mutex_unlock(&job->_threads[i]._e2Mutex) != SUCCESS)
                {
                    system_library_exit(ERR_MUTEX_UNLOCK);
                }
            }
            job->_vectorVectors.push_back(toAdd);
        } else {
            break;
        }
    }
}

/**
 * This function do the Reduce Phase process.
 * @param context the context of the current thread.
 */
void reducePhase(void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    if (job->_state->stage == MAP_STAGE || job->_state->stage == SHUFFLE_STAGE) {
        job->_state->stage = REDUCE_STAGE;
    }
    int size = job->_numShuffle;
    while (job->_numReduce < size) {
        if (pthread_mutex_lock(&job->reduceMutex) != SUCCESS) {
            system_library_exit(ERR_MUTEX_LOCK);
        }
        if (!job->_vectorVectors.empty() && job->_numReduce < size) {
            job->_client->reduce(&job->_vectorVectors.back(), contextC);
            job->_numReduce += job->_vectorVectors.back().size();
            job->_vectorVectors.pop_back();
        }
        if (pthread_mutex_unlock(&job->reduceMutex) != SUCCESS) {
            system_library_exit(ERR_MUTEX_UNLOCK);
        }
    }
}

/**
 * This function takes an error message depends on the current error and prints it to stderr and
 * exits the program with exit code 1.
 */
static void system_library_exit(const std::string &msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

 /**
  * This function releases all the resources of the current job.
  */
void releaseAll(JobHandle jobC)
{
    auto job = (JobContext*) jobC;
    //freeing the state of the job.
    delete job->_state;
    //freeing the Threads.
    for (int i = 0; i < job->_threadsNum; ++i) {
        job->_threads[i]._vec->clear();
        delete job->_threads[i]._vec;
        delete job->_threads[i]._thread;
        if (pthread_mutex_destroy(&job->_threads[i]._e2Mutex) != SUCCESS)
        {
            system_library_exit(ERR_DESTROY);
        }
    }
    delete [] job->_threads;
    //freeing shuffled vector of vectors.
    for (auto& it : job->_vectorVectors)
    {
        it.clear();
    }
    job->_vectorVectors.clear();
    //destroying mutexes.
    if (pthread_mutex_destroy(&job->_e3Mutex) != SUCCESS)
    {
        system_library_exit(ERR_DESTROY);
    }
    if (pthread_mutex_destroy(&job->reduceMutex) != SUCCESS)
    {
        system_library_exit(ERR_DESTROY);
    }
    if (pthread_mutex_destroy(&job->mapMutex) != SUCCESS)
    {
        system_library_exit(ERR_DESTROY);
    }
    if (pthread_mutex_destroy(&job->switchMutex) != SUCCESS)
    {
        system_library_exit(ERR_DESTROY);
    }
    if (pthread_mutex_destroy(&job->waitMutex) != SUCCESS)
    {
        system_library_exit(ERR_DESTROY);
    }
    if (sem_destroy(&job->semaphore) != SUCCESS)
    {
        system_library_exit(ERR_SEM_DESTROY);
    }
    delete job;
}