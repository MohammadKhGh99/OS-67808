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
#define ERR_MUTEX_LOCK "system error: Error in locking mutex function!"
#define ERR_MUTEX_UNLOCK "system error: Error in unlocking mutex function!"
#define ERR_SEM "system error: Error in sem_init function!"
#define INIT_ATOMIC 0

typedef struct JobContext JobContext;

typedef struct Thread {
    int id{};
    JobContext *_job{};
    IntermediateVec *_vec = new IntermediateVec;
    pthread_t *_thread = new pthread_t;
    bool joined = false;
} Thread;


struct JobContext {
    JobState *_state = new JobState{UNDEFINED_STAGE, 0.0};  //todo change 0 to 0.0?
    pthread_mutex_t _e3Mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t _e2Mutex = PTHREAD_MUTEX_INITIALIZER;
    int _threadsNum{};
    MapReduceClient *_client{};
    InputVec *_inputVec{};
    OutputVec *_outputVec{};
    std::atomic<int> _numPair{INIT_ATOMIC};
    std::atomic<int> _numMap{INIT_ATOMIC};
//    std::atomic<int> _numMapFinish{0};
    std::atomic<int> _numShuffle{INIT_ATOMIC};
    std::atomic<int> _numReduce{INIT_ATOMIC};
    std::atomic<int> _numReduceFinish{INIT_ATOMIC};
    Barrier *_barrier{};
    Thread *_threads{};
//    IntermediateVec *_vec{};
    std::vector<IntermediateVec *> _vectorVectors{};

    pthread_mutex_t reduceMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mapMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t switchStage = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;
    sem_t semaphore{};
    std::atomic<uint64_t> atomic{INIT_ATOMIC};
    bool waitCalled = false;
    int i = 0;
};


static void system_library_exit(const std::string &msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

//void freeAll() {
//    sem_destroy(&semaphore);
//
//}

bool sortHelper(IntermediatePair first, IntermediatePair second) {
    return first.first->operator<(*second.first);
}

void sortPhase(void *context) {
    auto contextC = (Thread *) context;
//    auto job = contextC->_job;
    if (!contextC->_vec->empty())
    {
        std::sort(contextC->_vec->begin(), contextC->_vec->end(), sortHelper);
    }
//    if (!job->_vec->empty())
//    {
//        std::sort(job->_vec->begin(), job->_vec->end(), sortHelper);
//    }

}


void reducePhase(void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    if (job->_state->stage == MAP_STAGE || job->_state->stage == SHUFFLE_STAGE) {
        job->_state->stage = REDUCE_STAGE;
        job->_state->percentage = 0.0;
    }
//    std::cout << "ReduceThreadID: " << contextC->id << std::endl;
    int size = job->_numShuffle;
    while (job->_numReduce < size) {
        if (pthread_mutex_lock(&job->reduceMutex) != 0) {
            system_library_exit(ERR_MUTEX_LOCK);
        }
        if (!job->_vectorVectors.empty() && job->_numReduce < size) {
            job->_client->reduce(job->_vectorVectors.back(), contextC);
            job->_numReduce += job->_vectorVectors.back()->size();
//            job->_state->percentage = 100 * (float) job->_numReduce / (float) size;
            job->_vectorVectors.pop_back();
        }
        if (pthread_mutex_unlock(&job->reduceMutex) != 0) {
            system_library_exit(ERR_MUTEX_UNLOCK);
        }
    }
//    std::cout << "AfterReduceID: " << contextC->id << std::endl;
}


void shufflePhase(void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
//	std::cout << "ShuffleThreadID: " << contextC->id << std::endl;
    if (job->_state->stage == MAP_STAGE) {
        job->_state->percentage = 0.0;
        job->_state->stage = SHUFFLE_STAGE;
    }
//    int size = job->_numPair;
//    while (!job->_vec->empty())
//    {
//        auto toAdd = new IntermediateVec;
//        IntermediatePair large = job->_vec->back();
//        while (!job->_vec->empty() && !sortHelper(job->_vec->back(), large) && !sortHelper(large, job->_vec->back()))
//        {
//            toAdd->push_back(job->_vec->back());
//            job->_vec->pop_back();
//            job->_numShuffle++;
//            job->_state->percentage = (float) 100 * job->_numShuffle / (float) size;
//        }
//        job->_vectorVectors.push_back(toAdd);
//    }
    while (true) {
        auto toAdd = new IntermediateVec;
        IntermediatePair curPair;
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
                flag = false;
                IntermediateVec *curVec = job->_threads[i]._vec;
                while (!curVec->empty() && !flag) {
                    if (!sortHelper(curPair, curVec->back()) && !sortHelper(curVec->back(), curPair)) {
                        toAdd->push_back(curVec->back());
                        curVec->pop_back();
                        job->_numShuffle++;
//                        job->_state->percentage = (float) 100 * job->_numShuffle / (float) job->_numPair;
                    } else {
                        flag = true;
                    }
                }
            }
            job->_vectorVectors.push_back(toAdd);
        } else {
            break;
        }
    }
}

void mapPhase(void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    if (job->_state->stage == UNDEFINED_STAGE) {
        job->_state->percentage = 0.0;
        job->_state->stage = MAP_STAGE;
    }
//    std::cout << "Mapping: " << contextC->id << std::endl;
    int size = job->_inputVec->size();
    while (job->_numMap < size) {
        if (pthread_mutex_lock(&job->mapMutex) != 0) {
            system_library_exit(ERR_MUTEX_LOCK);
        }
        if (job->_numMap < size) {
            auto pair = job->_inputVec->at(job->_numMap);
            job->_numMap++;
//            job->_state->percentage = (float) 100 * job->_numMap / (float) size;
            job->_client->map(pair.first, pair.second, contextC);
        }
        if (pthread_mutex_unlock(&job->mapMutex) != 0) {
            system_library_exit(ERR_MUTEX_UNLOCK);
        }
    }
//    std::cout << "AfterMapping: " << contextC->id << std::endl;
}

void *mapSortShuffleReduce(void *context) {
    auto contextC = (Thread *) context;
    auto job = contextC->_job;
    mapPhase(context);
    sortPhase(context);
    job->_barrier->barrier();

    if (contextC->id == 0) {
        shufflePhase(context);
//        std::cout << "AfterShuffle: " << contextC->id << std::endl;

        for (int i = 0; i < job->_threadsNum - 1; ++i) {
            if (sem_post(&job->semaphore) != 0) {
                system_library_exit(ERR_SEM);
            }
        }
    }else if (job->_threadsNum > 1) {
        if (sem_wait(&job->semaphore) != 0) {
            system_library_exit(ERR_SEM);
        }
    }
    reducePhase(context);
//    std::cout<<"Exiting...  "<<contextC->id<<std::endl;
//    pthread_exit(nullptr);
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
                            int multiThreadLevel) {
    auto job = new JobContext;
    job->_state = new JobState{UNDEFINED_STAGE, 0.0};
    job->_threadsNum = multiThreadLevel;
    job->_client = const_cast<MapReduceClient *>(&client);
    job->_inputVec = const_cast<InputVec *>(&inputVec);
    job->_outputVec = &outputVec;
    job->_barrier = new Barrier(multiThreadLevel);
    job->_threads = new Thread[multiThreadLevel];
//    job->_vec = new IntermediateVec;
    if (inputVec.empty())
    {
        job->_state->percentage = 100.0;
        job->_state->stage = REDUCE_STAGE;
    } else {  //todo remove or not?
        for (int i = 0; i < job->_threadsNum; ++i) {
            job->_threads[i].id = i;
            job->_threads[i]._job = job;
            job->_threads[i]._vec = new IntermediateVec;
            job->_threads[i]._thread = new pthread_t;
        }
        if (sem_init(&job->semaphore, 0, 0) != 0) {
            system_library_exit(ERR_SEM);
        }
        for (int i = 0; i < multiThreadLevel; ++i) {
            if (pthread_create(job->_threads[i]._thread, nullptr, mapSortShuffleReduce,
                               &job->_threads[i]) != 0) {
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
    if (pthread_mutex_lock(&jobC->waitMutex) != 0)
    {
        system_library_exit(ERR_MUTEX_LOCK);
    }
//    if (jobC->waitCalled)
//    {
//        return;
//    }
//    jobC->waitCalled = true;
//    if ()
//    std::cout<<"Waiting..."<<std::endl;
//    int i = 0;
    while (jobC->i < jobC->_threadsNum && !(jobC->_state->stage == REDUCE_STAGE && jobC->_state->percentage == 100.0)) {
//        std::cout<<"ID: "<<jobC->i<<std::endl;
        if (pthread_join(*jobC->_threads[jobC->i]._thread, nullptr) != 0) {
            system_library_exit(ERR_JOIN);
        }
        jobC->_threads[jobC->i].joined = true;  // todo remove ?
        jobC->i++;
    }
    if (pthread_mutex_unlock(&jobC->waitMutex) != 0)
    {
        system_library_exit(ERR_MUTEX_UNLOCK);
    }
}

/**
 * this function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job
 * @param state
 */
void getJobState(JobHandle job, JobState *state) {
    auto jobC = (JobContext *) job;
    if (pthread_mutex_lock(&jobC->switchStage) != 0)
    {
        system_library_exit(ERR_MUTEX_LOCK);
    }
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
    if (pthread_mutex_unlock(&jobC->switchStage) != 0)
    {
        system_library_exit(ERR_MUTEX_UNLOCK);
    }
}

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished. After this
 * function is called the job handle will be invalid.
 * @param job
 */
void closeJobHandle(JobHandle job) {
    waitForJob(job);
    auto jobC =(JobContext*)job;
    (*jobC->_state) = {UNDEFINED_STAGE, 0.0f};
}

/**
 * This function produces a (K2*, V2*) pair.
 * @param key intermediary element.
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit2(K2 *key, V2 *value, void *context) {
    auto contextC = (Thread *) context;
    contextC->_job->_numPair++;
//    contextC->_job->_vec->emplace_back(key, value);
    contextC->_vec->emplace_back(key, value);
}

/**
 * This function creates (K3*, V3*)pair
 * @param key intermediary element
 * @param value intermediary element
 * @param context contains data structure of the thread that created the intermediary element.
 */
void emit3(K3 *key, V3 *value, void *context) {
    auto contextC = (Thread *) context;
    contextC->_job->_outputVec->emplace_back(key, value);
}