//
// Created by sarah on 13/05/2020.
//
#include "MapReduceFramework.h"
#include "Barrier.h"
#include <atomic>
#include <cstdlib>
#include <pthread.h>
#include <vector>
#include <iostream>
#include <semaphore.h>

#define FAILED 1
#define SYSTEM_ERROR "system error: "


class JobContext;

/*vector of Intermediate Pairs */
typedef std::vector<IntermediatePair> IntermediateVec;
/*struct that contain related data for each thread*/
typedef struct {
    //pointer for th job
    JobContext *jobContext;
    //thread's id
    int id;
    //Intermediate vector, holds the output of the map phase
    IntermediateVec intermediateVec;
}ThreadContext;

/* class which includes all the parametrs relevant for the job*/
class JobContext
{
public:
    //client
    const MapReduceClient &client;
    //input vector
    const InputVec  &inputVec;
    //output
    OutputVec  &outputVec;
    //current state of the job
    JobState state;
    //the number of worker threads for the job
    int numberOfThreads;
    //list of all threads
    pthread_t *threads;
    // list of mutexs for each thread, used when accessing intermediateVec 
    pthread_mutex_t *mutexs;
    // vector of threads contexts
    std::vector<ThreadContext> threads_contexts;
    // vector for K2 keys in the map produced in shuffle phase
    std::vector<K2*> K2_keys;
    // map for Intermediate keys (K2) and  v2 values (vector)
    IntermediateMap shuffledMap;
    //barrier
    Barrier barrier;
    bool terminated = false;
    bool finish_map = false;
    bool finish_shuffle = false;
    int mapped_input = 0;
    int reduced_pairs = 0;
    // mutex /atomic variables
    std::atomic<int> map_counter;
    std::atomic<int> totalPairs;
    std::atomic<int> reduce_counter;
    int shuffle_counter = 0;
    pthread_mutex_t reduce_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t reduceConter_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t outputMutex = PTHREAD_MUTEX_INITIALIZER;

    JobContext(const MapReduceClient& client,const InputVec& inputVec,
               OutputVec& outputVec,int multiThreadLevel):
            client(client),inputVec(inputVec),outputVec(outputVec),
            numberOfThreads(multiThreadLevel),threads_contexts(multiThreadLevel),
            barrier(numberOfThreads),map_counter(0),totalPairs(0),reduce_counter(0)
    {

        threads = new pthread_t[numberOfThreads];
        if (!threads){
            std::cerr<< SYSTEM_ERROR<< "memory alloc failed"<<std::endl;
            exit(FAILED);
        }
        mutexs = new pthread_mutex_t[numberOfThreads];
        if (!mutexs){
            std::cerr<< SYSTEM_ERROR<< "memory alloc failed"<<std::endl;
            exit(FAILED);
        }
        for(int i = 0 ; i < numberOfThreads; ++i){
            mutexs[i] = PTHREAD_MUTEX_INITIALIZER;
        }
        state = {UNDEFINED_STAGE, 0};

    }
    ~JobContext(){
        delete [] threads;
        threads = nullptr;
        for(int i = 0 ; i < numberOfThreads; ++i){
            pthread_mutex_destroy(&mutexs[i]);
        }
        delete[] mutexs;
        mutexs = nullptr;
        threads_contexts.clear();
        shuffledMap.clear();
        K2_keys.clear();
        if(pthread_mutex_destroy(&reduceConter_mutex) != 0){
            std::cerr<<SYSTEM_ERROR<<"error on pthread_mutex_destroy"<<std::endl;
            exit(FAILED);
        }
       if (pthread_mutex_destroy(&reduce_mutex)!= 0){
           std::cerr<<SYSTEM_ERROR<<"error on pthread_mutex_destroy"<<std::endl;
           exit(FAILED);
       }
        if(pthread_mutex_destroy(&Mutex)){
            std::cerr<<SYSTEM_ERROR<<"error on pthread_mutex_destroy"<<std::endl;
            exit(FAILED);
       }
        if(pthread_mutex_destroy(&outputMutex)){
            std::cerr<<SYSTEM_ERROR<<"error on pthread_mutex_destroy"<<std::endl;
            exit(FAILED);
        }
    }
};

/* runs the reduce phase of the algorithm , produce the output*/
void reduce(void* context){
    ThreadContext* tc = (ThreadContext*)context;
    JobContext* jc = tc->jobContext;
    int total_keys = jc->K2_keys.size();
    pthread_mutex_lock(&jc->reduceConter_mutex);
    int cur_value = jc->reduce_counter++;
    pthread_mutex_unlock(&jc->reduceConter_mutex);
    int curVecSize;
    std::vector<V2 *> curVec;
    while((cur_value < total_keys)){
        auto key = jc->K2_keys[cur_value];
        if(pthread_mutex_lock(&jc->reduce_mutex) != 0){std::cerr<< SYSTEM_ERROR<< "error on mutex lock";exit(FAILED);}
        curVec = jc->shuffledMap[key];
        jc->shuffledMap.erase(key);
        if(pthread_mutex_unlock(&jc->reduce_mutex) != 0){std::cerr<<SYSTEM_ERROR << "error on mutex unlock";exit(FAILED);}
        curVecSize = curVec.size();
        jc->client.reduce(key,curVec,tc);
        if(pthread_mutex_lock(&jc->Mutex) != 0){
            std::cerr<<SYSTEM_ERROR<<"error on mutex lock"<<std::endl;
            exit(FAILED);}
        jc->reduced_pairs += curVecSize;
        jc->state.stage = REDUCE_STAGE;
        jc->state.percentage = 100*jc->reduced_pairs/float(jc->totalPairs);
        if(pthread_mutex_unlock(&jc->Mutex) != 0){
            std::cerr <<SYSTEM_ERROR<< "error on mutex unlock"<<std::endl;
            exit(FAILED);}
        pthread_mutex_lock(&jc->reduceConter_mutex);
        cur_value = jc->reduce_counter++;
        pthread_mutex_unlock(&jc->reduceConter_mutex);
        curVec.clear();
    }
}

/**
 * run mapping algorithm described in the exercise
 */
void map_phase(void* context){
    ThreadContext *tc = (ThreadContext*) context;
    JobContext* jc = tc->jobContext;
    const MapReduceClient& client = jc->client;
    std::atomic<int>& map_counter = jc->map_counter;
    int inputSize = jc->inputVec.size();
    InputPair next_pair;
    int old_value = map_counter++;

    while(old_value < inputSize ) {
        next_pair = jc->inputVec.at(old_value);
        pthread_mutex_lock(&jc->mutexs[tc->id]);
        client.map(next_pair.first, next_pair.second, tc);//tc is the context used in emit2
        pthread_mutex_unlock(&jc->mutexs[tc->id]);
        if (pthread_mutex_lock(&jc->Mutex) != 0) {
            std::cerr<<SYSTEM_ERROR<<"error on mutex lock"<<std::endl;
            exit(FAILED);
        }
        jc->state.stage= MAP_STAGE;
        jc->mapped_input++;
        jc->state.percentage = (jc->mapped_input/ (float) jc->inputVec.size() * 100);

        if (pthread_mutex_unlock(&jc->Mutex) != 0) {
            std::cerr<<SYSTEM_ERROR<<"error on mutex unlock"<<std::endl;
            exit(FAILED);
        }
        old_value = map_counter++;
    }
    jc->barrier.barrier();
    reduce(tc);
    return;
}

void emit2 (K2* key, V2* value, void* context)
{
    ThreadContext *tc = (ThreadContext*) context;
    JobContext* jc = tc->jobContext;
    tc->intermediateVec.push_back(IntermediatePair(key,value));
    jc->totalPairs++;
}

void emit3 (K3* key, V3* value, void* context){
    ThreadContext* tc = (ThreadContext*)context;
    if (pthread_mutex_lock(&tc->jobContext->outputMutex)!=0)
    {
        std::cerr<<SYSTEM_ERROR<<"error on mutex lock"<<std::endl;;
        exit(FAILED);}
    tc->jobContext->outputVec.push_back(OutputPair(key,value));
    if (pthread_mutex_unlock(&tc->jobContext->outputMutex)!=0)
    {
        std::cerr<<SYSTEM_ERROR<<"error on mutex unlock"<<std::endl;
        exit(FAILED);}
}

/*runs the shuffle phase of the algorithm, done by one thread only*/
void shuffle_phase(void* context)
{
    ThreadContext* tc = (ThreadContext*)context;
    JobContext* jc = tc->jobContext;
    auto * toShuffle = new std::vector<IntermediatePair>;
    IntermediateMap& KeyMap = jc->shuffledMap;
   while(true) {
       if (jc->shuffle_counter < jc->totalPairs) {
           for (int i = 0; i < jc->numberOfThreads; ++i) {
               pthread_mutex_lock(&jc->mutexs[i]);//use inter-vector
               if (!jc->threads_contexts[i].intermediateVec.empty()) {
                   toShuffle->swap(jc->threads_contexts[i].intermediateVec);
                   for (auto pair : *toShuffle) {
                       if (!KeyMap.count(pair.first)) { jc->K2_keys.push_back(pair.first); }
                       KeyMap[pair.first].push_back(pair.second);
                   }
                   jc->shuffle_counter += toShuffle->size();
                   toShuffle->clear();
               }
               pthread_mutex_unlock(&jc->mutexs[i]);
           }
       }
           if (pthread_mutex_lock(&jc->Mutex) != 0) {
               std::cerr<<SYSTEM_ERROR<<"error on mutex lock"<<std::endl;
               exit(FAILED);
           }
           if ((jc->state.stage == MAP_STAGE) && (jc->state.percentage == 100)) {
               jc->finish_map = true;
           }
           if (jc->finish_map) {
               jc->state.stage = SHUFFLE_STAGE;
               jc->state.percentage = 100 * jc->shuffle_counter / float(jc->totalPairs);
           }
           if (jc->state.stage == SHUFFLE_STAGE && jc->state.percentage == 100) {
               jc->finish_shuffle = true;
               if (pthread_mutex_unlock(&jc->Mutex) != 0) {
                   std::cerr<<SYSTEM_ERROR<<"error on mutex unlock"<<std::endl;
                   exit(FAILED);
               }
               break;
           }
           if (pthread_mutex_unlock(&jc->Mutex) != 0) {
               std::cerr<<SYSTEM_ERROR<<"error on mutex unlock"<<std::endl;
               exit(FAILED);
           }
   }
    jc->barrier.barrier();
    if(jc->finish_shuffle){
        delete toShuffle;
        reduce(tc);
        return;
    }
}

/* preform  the MapReduce algorithm described in the exercise,
 * it is entry point for all working threads*/
void* Job_handler(void* context) {
    ThreadContext *tc = (ThreadContext *) context;
    JobContext *jc = tc->jobContext;
    // if input vector is empty job is done.
    if (jc->inputVec.empty()){
        //let only first thread update state:
        if (tc->id == 0) {
            pthread_mutex_lock(&jc->Mutex);
            jc->state = {REDUCE_STAGE,100};
            pthread_mutex_unlock(&jc->Mutex);
        }

        pthread_exit(nullptr);
    }
    if (tc->id != jc->numberOfThreads - 1) {
        map_phase(tc);
    }else {
        shuffle_phase(tc);
    }
    pthread_exit(nullptr);
}


JobHandle startMapReduceJob(const MapReduceClient& client,
                            const InputVec& inputVec, OutputVec& outputVec,
                            int multiThreadLevel)
{
    JobContext* jobContext = new JobContext(client,inputVec,outputVec,multiThreadLevel);
    for (int i = 0; i < multiThreadLevel; ++i) {
        jobContext->threads_contexts[i] = ThreadContext {jobContext,i};
    }
    for(int i = 0 ; i < multiThreadLevel ; ++i)
    {
        if(pthread_create(&(jobContext->threads[i]), nullptr, Job_handler, &(jobContext->threads_contexts[i])))
        {
            std::cerr<<SYSTEM_ERROR<<"error with pthread_create"<<std::endl;
            exit(FAILED);
        }
    }
    return (void*) jobContext;
}


void waitForJob(JobHandle job)
{
    JobContext* jc = (JobContext*) job;
    if(jc->terminated){ return;}
    for(int i = 0; i < jc->numberOfThreads ;++i){
        if(pthread_join(jc->threads[i], nullptr) != 0){
            std::cerr<<SYSTEM_ERROR<<"error with pthread_join";
            exit(FAILED);
        }
    }
    jc->terminated = true;
}

void getJobState(JobHandle job, JobState* state)
{
    JobContext* jc = (JobContext*) job;
    if(pthread_mutex_lock(&jc->Mutex) != 0){
        std::cerr<<SYSTEM_ERROR<<"error on mutex lock"<<std::endl;
        exit(FAILED);
    }
    *state = jc->state;
    if(pthread_mutex_unlock(&jc->Mutex) != 0){
        std::cerr<<SYSTEM_ERROR<<"error on mutex unlock"<<std::endl;
        exit(FAILED);
    }
}

void closeJobHandle(JobHandle job){
    waitForJob(job);
    JobContext* jc = (JobContext*) job;
    delete jc;
}
