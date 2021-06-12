//
// Created by sarah on 13/04/2020.
//
#include <signal.h>
#include <setjmp.h>
#include "uthreads.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <queue>
#include <vector>
#include <map>

#define MICRO_TO_SECOND 1000000
#define SYS_ERROR "system error:"
#define LIBRARY_ERROR "thread library error:"
#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

enum State {READY, RUNNING, BLOCK};
/*class Thread that represents a user thread */
class Thread{
    private:
    int _tid;
    int _Priority;
    int _quantum_counter;
    State _state;
    char* _stack;
    sigjmp_buf _jump_buf;
    void (*_f)(void);

public:
    Thread(int tid, int Priority , void (*f)(void), State state): _tid(tid), _Priority(Priority), _f(f), _state(state){
        _quantum_counter = 0;
        _stack = new char[STACK_SIZE];
        address_t sp, pc;
        sp = (address_t)_stack + STACK_SIZE - sizeof(address_t);
        pc = (address_t)*f;
        sigsetjmp(_jump_buf, 1);
        (_jump_buf->__jmpbuf)[JB_SP] = translate_address(sp);
        (_jump_buf->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&_jump_buf->__saved_mask);
    }
    ~Thread(){
        delete[] _stack;
    }
    int gettid()const{
        return _tid;
    }
    void setstate(State satae){
        _state = satae;
    }
    void setPriority(int pr){
        _Priority = pr;
    }
    State getstate(){
        return _state;
    }
    int getPriority(){
        return _Priority;
    }
    int getquantum(){
        return _quantum_counter;
    }
    void Incquantum(){
        _quantum_counter++;
    }

    int saveContext()
    {
        int ret_val = sigsetjmp(_jump_buf, 1);
        return ret_val;
    }

    void loadContext()
    {
        siglongjmp(_jump_buf, 1);
    }

};


static std::deque <Thread*> ready_queue;
static std::vector<Thread*> blocked;
static Thread* running_thread;
struct sigaction sa;
struct itimerval timer;
static std::map<int, Thread*> All_threads;
static int quantum_counter = 0;
int *quantum_list;


static void Timer(int pr){
    timer.it_value.tv_sec = quantum_list[pr]/MICRO_TO_SECOND;
    timer.it_value.tv_usec = quantum_list[pr] % MICRO_TO_SECOND;
    timer.it_interval.tv_sec = quantum_list[pr]/MICRO_TO_SECOND;
    timer.it_interval.tv_usec = quantum_list[pr] % MICRO_TO_SECOND;
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        std::cerr<<SYS_ERROR<<"Unable to set timer"<< std::endl;
        exit(1);
    }
}
void switchThreads(){
    Thread *tmp = running_thread;
    tmp->setstate(State::READY);
    //run next thread in ready queue
    Thread *nextTORun = ready_queue.front();
    ready_queue.pop_front();
    if (nextTORun != nullptr){
        running_thread = nextTORun;
    }
    running_thread->setstate(State::RUNNING);
    running_thread->Incquantum();
    quantum_counter++;
    Timer(running_thread->getPriority());
    ready_queue.push_back(tmp);
}
static void TimeHandler(int Signal){
    if (!running_thread->saveContext())
    {
        switchThreads();
        running_thread->loadContext();
    }
}
int get_Next_id(){
   int newId = 0;
    for (std::map<int, Thread*>::iterator iter = All_threads.begin(); iter != All_threads.end();
         newId++, iter++)
    {
        if (iter->first != newId)
        {
            break;
        }
    }
    return newId;
}

int removeFromReady(int tid){

    std::deque<Thread*>::iterator it;
    if (!ready_queue.empty()){
        for(it = ready_queue.begin() ; it != ready_queue.end() ; it++)
        {
            if ((*it)->gettid() == tid && All_threads.at(tid) == (*it))
            {
                delete(*it);
                ready_queue.erase(it);
                return 0;
            }
        }
    }
    return -1;
}
int removeFromBlocked(int tid){

    std::vector<Thread*>::iterator it;
    if (!ready_queue.empty()){
        for(it = blocked.begin() ; it != blocked.end() ; it++)
        {
            if ((*it)->gettid() == tid && All_threads.at(tid) == (*it))
            {
                delete(*it);
                blocked.erase(it);
                return 0;
            }
        }
    }
    return -1;
}
void FreeMemory(){
    for (auto &All_thread : All_threads) {
        delete(All_thread.second);
    }
    All_threads.clear();
    ready_queue.clear();
    blocked.clear();
    if(sigemptyset(&sa.sa_mask))
    {
        std::cerr << "system error: cannot call sigemptyset" << std::endl;
        exit(1);
    }
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    exit(0);
}
int uthread_init(int *quantum_usecs, int size){
    if ( size <= 0){
        std::cerr<< LIBRARY_ERROR<<"wrong input"<<std::endl;
        return -1;
    }
    quantum_list = quantum_usecs;
    sa.sa_handler = &TimeHandler;
    if(sigemptyset (&sa.sa_mask) == -1)
    {
        std::cerr<<SYS_ERROR<<"signal action error"<< std::endl;
        exit(1);
    }
    if(sigaction(SIGVTALRM,&sa,NULL) < 0)
    {
        std::cerr<<SYS_ERROR<<"signal action error"<< std::endl;
        exit(1);
    }
    Thread* mainthread = new Thread(0,0, nullptr,State::READY);
    mainthread->setstate(State::RUNNING);
    mainthread->Incquantum();
    running_thread = mainthread;
    All_threads.insert(std::pair<int, Thread*>(0, mainthread));
    quantum_counter++;
    Timer(0);
    return 0;
}
int uthread_spawn(void (*f)(void), int priority){
    if (All_threads.size() == MAX_THREAD_NUM){
        std::cerr<< LIBRARY_ERROR<<"can't creat new thread,too many threads"<<std::endl;
        return -1;
    }
    if (sigprocmask(SIG_BLOCK,&sa.sa_mask, NULL)){
        std::cerr<<SYS_ERROR<<"sigprocmask error:cannot call sigprogmask"<< std::endl;
        exit(1);
    }
    int id = get_Next_id();
    Thread* Newthread = new Thread(id,priority,f,State::READY);
    All_threads.insert(std::pair<int,Thread*>(id,Newthread));
    ready_queue.push_back(Newthread);
    if (sigprocmask(SIG_UNBLOCK,&sa.sa_mask, NULL)){
        std::cerr<<SYS_ERROR<<"sigprocmask error:cannot call sigprogmask"<< std::endl;
        exit(1);
    }
    return id;
}

int uthread_change_priority(int tid, int priority){
    if (tid < 0 || tid > MAX_THREAD_NUM){
        std::cerr<< LIBRARY_ERROR<<"wrong thread Id"<<std::endl;
        return -1;
    }
    Thread* thread = All_threads.at(tid);
    thread->setPriority(priority);
    return 0;
}
int uthread_terminate(int tid){
    if (!All_threads.count(tid)){
        std::cerr<< LIBRARY_ERROR<<"wrong thread Id"<<std::endl;
        return -1;
    }
    if (sigprocmask(SIG_BLOCK,&sa.sa_mask, NULL)){
        std::cerr<<SYS_ERROR<<"sigprocmask error:cannot call sigprogmask"<< std::endl;
        exit(1);
    }
    if (tid == 0){
        FreeMemory();
    }
    if(tid == running_thread->gettid()){
        Thread *todelete = running_thread;
        switchThreads();
        delete(todelete);
        All_threads.erase(todelete->gettid());
        running_thread->loadContext();
    }
    if (All_threads.at(tid)->getstate() == State::READY){
        if(removeFromReady(tid))
        {
            std::cerr<< LIBRARY_ERROR<<"cannot delete thread "<<std::endl;
            return -1;
        }
        All_threads.erase(tid);
    } else{
        if(removeFromBlocked(tid))
        {
            std::cerr<< LIBRARY_ERROR<<"cannot delete thread "<<std::endl;
            return -1;
        }
        All_threads.erase(tid);
    }
    if (sigprocmask(SIG_UNBLOCK,&sa.sa_mask, NULL)){
        std::cerr<<SYS_ERROR<<"sigprocmask error:cannot call sigprogmask"<< std::endl;
        exit(1);
    }
    return 0;
}






