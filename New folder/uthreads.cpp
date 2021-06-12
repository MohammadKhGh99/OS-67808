//
// Created by m7mdg on 03-May-21.
//

#include "uthreads.h"
#include <map>
#include <vector>
#include <queue>
#include <iostream>
#include <sys/time.h>
//#include <unistd.h>
//#include <stdio.h>
#include <setjmp.h>
#include <signal.h>

#define FAIL -1
#define MIC_TO_SEC 1000000
#define SYS_ERR_MSG "system error: "
#define LIB_ERR_MSG "thread library error: "
#define MAIN_THRD 0
#define UNLOCKED -1
#define SUCCESS 0
#define NO_THRD "There is no thread with this id!"
#define MT_THRD "There is no threads!"
#define ERR_UNLOCK "Error in unlocking the mutex while it is unlocked!"
#define ERR_LOCK "Error in locking the mutex another time by the same thread!"
#define ERR_BLOCK_MAIN "You can't block the main thread!"
#define ERR_SIGPROCMASK "Error in sigprocmask function!"
#define ERR_TERMINATE "Error in terminating thread!"
#define ERR_MAX "Max number of threads reached, can't create new thread !"
#define ERR_TIMER "Problem with setting the timer!"
#define ERR_SIGEMPTYSET "Error in sigemptyset function !"
#define ERR_SIGACTION "Error in signal action !"
#define ERR_QUANTUM_LEN "Invalid length of a quantum_list !"
#define NO_BLOCK "There is no BLOCKED threads!"
#define ERR_BLOCK "Couldn't remove from BLOCKED threads!"
#define NO_READY "There is no READY threads!"
#define ERR_READY "Couldn't remove from READY threads!"

enum STATE
{
	READY, RUNNING, BLOCK
};

//todo switch between __x86_64_ and __x86_32_ !!!!!

#ifdef __x86_64__

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

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

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

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

class Thread
{
private:
	STATE _state;
	char *_stack;
	sigjmp_buf _jump;
	int _quantum_counter;
	int _quantum;
	int _tid;
	void (*_f)(void);

public:
	Thread(STATE state, int tid, void (*func)(void)) : _state(state), _jump(), _quantum(), _tid(tid), _f(func)
	{
		_stack = new char[STACK_SIZE];
		address_t sp = (address_t) _stack + STACK_SIZE - sizeof(address_t), pc = (address_t) _f;  //todo
		sigsetjmp(_jump, 1);

		(_jump->__jmpbuf)[JB_SP] = translate_address(sp);
		(_jump->__jmpbuf)[JB_PC] = translate_address(pc);
		sigemptyset(&_jump->__saved_mask);
		_quantum_counter = 0;
	}

	~Thread()
	{ delete[] _stack; }

	int get_quantum() const
	{return _quantum;}

	void set_quantum(int other)
	{_quantum = other;}

	STATE get_state()
	{ return _state; }

	void do_entry()
    {
	    _f();
    }

	void free_stack()
    {
	    delete[](_stack);
    }

	void set_state(STATE state)
	{ _state = state; }

	int get_quantum_counter() const
	{ return _quantum_counter; }

	void set_quantum_counter(int other)
	{
		_quantum_counter = other;
	}

	void inc_quantum_counter()
	{ _quantum_counter++; }

	int get_tid() const
	{ return _tid; }

	void load_context()
	{ siglongjmp(_jump, 1); }

	int save_context()
	{ return sigsetjmp(_jump, 1); }

};

int mutex = UNLOCKED;
int *quantum_list;
static int quantum_counter = 0;
static std::map<int, Thread *> threads;
struct itimerval itimer;
struct sigaction sig;
static Thread *running;
static std::deque<Thread *> ready;
static std::vector<Thread *> block;
static std::vector<Thread *> mutex_blocked;

//static void timer_func(int i)
//{
//	itimer.it_interval.tv_sec = quantum_list[i] / MIC_TO_SEC;
//	itimer.it_value.tv_sec = quantum_list[i] / MIC_TO_SEC;
//	itimer.it_value.tv_usec = quantum_list[i] % MIC_TO_SEC;
//	itimer.it_interval.tv_usec = quantum_list[i] % MIC_TO_SEC;
//	int set_result = setitimer(ITIMER_VIRTUAL, &itimer, nullptr);
//	if (set_result)
//	{
//		std::cerr << SYS_ERR_MSG << "Problem with setting the timer!" << std::endl;  //todo magic msg
//		exit(EXIT_FAILURE);
//	}
//}

void threads_switch()
{
	Thread *current = running;
	current->set_state(STATE::READY);
	Thread *thread_to_run = ready.front();
	if (thread_to_run != nullptr)
	{
		running = thread_to_run;
//		running->set_quantum_counter(current->get_quantum_counter());
	}
	ready.pop_front();
	running->set_state(STATE::RUNNING);
	running->do_entry(); //todo
	running->add_quantum();
	quantum_counter++;
	ready.push_back(current);
}

int first_mt_id()
{
	int id = 0;  //todo int or long
	for (auto it = threads.begin(); it != threads.end(); id++, it++)
	{
		//todo what??
		if (it->first != id)
		{ break; }
		id = it->first;
	}
	return id;
}

int remove_ready(int id)
{
	if (ready.empty())
	{
		std::cerr<<LIB_ERR_MSG<<NO_READY<<std::endl;
		return FAIL;
	}
	for (auto it = ready.begin(); it != ready.end(); it++)
	{
		if (threads.at(id) == (*it) && (*it)->get_tid() == id)
		{
			delete (*it);
			ready.erase(it);
			return EXIT_SUCCESS;
		}
	}
	std::cerr<<LIB_ERR_MSG<<ERR_READY<<std::endl;
	return FAIL;
}

int remove_blocked(int id)
{
	if (ready.empty())
	{
		std::cerr<<LIB_ERR_MSG<<NO_BLOCK<<std::endl;
		return FAIL;
	}
	for (auto it = block.begin(); it != block.end(); it++)
	{
		if (threads.at(id) == (*it) && (*it)->get_tid() == id)
		{
			delete (*it);
			block.erase(it);
			return EXIT_SUCCESS;
		}
	}
	std::cerr<<LIB_ERR_MSG<<ERR_BLOCK<<std::endl;
	return FAIL;
}

void memory_free()
{
	for (auto &thread: threads)
	{
//	    thread.second->free_stack();
	    delete (thread.second);
	}
	threads.clear();
	ready.clear();
	block.clear();
	if (sigemptyset(&sig.sa_mask))
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGEMPTYSET << std::endl;
		exit(EXIT_FAILURE);
	}
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 0;
	exit(EXIT_SUCCESS);
}

static void handler(int)
{
	if (!running->save_context())
	{
		threads_switch();
		running->load_context();
	}
}

/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum_list in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
	if (quantum_usecs <= 0)
	{
		std::cerr << LIB_ERR_MSG << ERR_QUANTUM_LEN << std::endl;
		return FAIL;
	}

	sig.sa_handler = &handler;
	if (sigaction(SIGVTALRM, &sig, nullptr) < 0)
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGACTION << std::endl;
		exit(EXIT_FAILURE);
	}
	if (sigemptyset(&sig.sa_mask) == -1)
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGEMPTYSET << std::endl;
		exit(EXIT_FAILURE);
	}
	auto main_thread = new Thread(STATE::READY, 0, nullptr);
	main_thread->add_quantum();
	quantum_counter++;
	main_thread->set_quantum(quantum_usecs);
	main_thread->set_state(STATE::RUNNING);
//	main_thread->do_entry(); //todo
	running = main_thread;
	threads.insert(std::pair<int, Thread *>(0, main_thread));
	itimer.it_interval.tv_sec = main_thread->get_quantum() / MIC_TO_SEC;
	itimer.it_value.tv_sec = main_thread->get_quantum() / MIC_TO_SEC;
	itimer.it_value.tv_usec = main_thread->get_quantum() % MIC_TO_SEC;
	itimer.it_interval.tv_usec = main_thread->get_quantum() % MIC_TO_SEC;
	int set_result = setitimer(ITIMER_VIRTUAL, &itimer, nullptr);
	if (set_result)
	{
		std::cerr << SYS_ERR_MSG << ERR_TIMER << std::endl;  //todo magic msg
		exit(EXIT_FAILURE);
	}
	return SUCCESS;
}

/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
	if (sigprocmask(SIG_BLOCK, &sig.sa_mask, nullptr))
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGPROCMASK << std::endl;
		exit(EXIT_FAILURE);
	}
	if (threads.size() + 1 >= MAX_THREAD_NUM)
	{
		std::cerr << LIB_ERR_MSG << ERR_MAX << std::endl;
		return FAIL;
	}
	int next_id = first_mt_id();
//	f();
	auto next_thread = new Thread(STATE::READY, next_id, f);
	threads.insert(std::pair<int, Thread *>(next_id, next_thread));
	ready.push_back(next_thread);
//	next_thread->do_entry();
	if (sigprocmask(SIG_UNBLOCK, &sig.sa_mask, nullptr))
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGPROCMASK << std::endl;
		exit(EXIT_FAILURE);
	}
	return next_id;
}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
	if (sigprocmask(SIG_BLOCK, &sig.sa_mask, nullptr))
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGPROCMASK << std::endl;
		exit(EXIT_FAILURE);
	}
	if (threads.find(tid) == threads.end())
	{
		std::cerr << LIB_ERR_MSG << NO_THRD << std::endl;
		return FAIL;
	}
	if (tid == MAIN_THRD)
	{
	    memory_free();
	    exit(EXIT_SUCCESS);
	}
	if (tid == running->get_tid())
	{
		Thread *to_terminate = running;
		threads_switch();
		threads.erase(tid);
		to_terminate->free_stack(); //todo
		delete (to_terminate);
		running->load_context();
		exit(EXIT_SUCCESS); //todo
	}
	if (threads.at(tid)->get_state() != STATE::READY)
	{
		if (remove_blocked(tid))
		{
			std::cerr << LIB_ERR_MSG << ERR_TERMINATE << std::endl;
			return FAIL;
		}
	}
	else
	{
		if (remove_ready(tid))
		{
			std::cerr << LIB_ERR_MSG << ERR_TERMINATE << std::endl;
			return FAIL;
		}
	}
	threads.erase(tid);
	if (sigprocmask(SIG_UNBLOCK, &sig.sa_mask, nullptr))
	{
		std::cerr<<SYS_ERR_MSG<<ERR_SIGPROCMASK<<std::endl;
		exit(EXIT_FAILURE);
	}
	return SUCCESS; //todo
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
	if (threads.find(tid) == threads.end()) //todo
	{
		std::cerr<<LIB_ERR_MSG<<NO_THRD<<std::endl;
		return FAIL;
	}
	if (tid == MAIN_THRD)
	{
		std::cerr<<LIB_ERR_MSG<<ERR_BLOCK_MAIN<<std::endl;
		return FAIL;
	}
	if (tid == running->get_tid())
	{
        running->set_state(STATE::BLOCK);
        block.push_back(running);
        threads_switch();
//		return SUCCESS;
	}
	else if (threads.at(tid)->get_state() == STATE::READY)
    {
		for (auto it = ready.begin(); it != ready.end(); it++)
		{
			if ((*it)->get_tid() == threads.at(tid)->get_tid())
			{
				ready.erase(it);
				break;
			}
		}
	    threads.at(tid)->set_state(STATE::BLOCK);
	    block.push_back(threads.at(tid));
    }
	sigprocmask(SIG_UNBLOCK, &sig.sa_mask, nullptr);
	return SUCCESS; //todo


}

// sigprocmask(SIG_BLOCK, &sig, nullptr);
// sigprocmask(SIG_UNBLOCK, &sig, nullptr);

/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
	sigprocmask(SIG_BLOCK, &sig.sa_mask, nullptr);
	if (threads.find(tid) == threads.end())
	{
		std::cerr<<LIB_ERR_MSG<<NO_THRD<<std::endl;
		return FAIL;
	}
	if (threads.at(tid)->get_state() == STATE::BLOCK)
	{
		threads.at(tid)->set_state(STATE::READY);
		for (auto it = block.begin(); it != block.end(); it++)
		{
			if ((*it)->get_tid() == tid)
			{
				block.erase(it);
				break;
			}
		}
		ready.push_back(threads.at(tid));
	}
	sigprocmask(SIG_UNBLOCK, &sig.sa_mask, nullptr);
	return SUCCESS;
}


/*
 * Description: This function tries to acquire a mutex.
 * If the mutex is unlocked, it locks it and returns.
 * If the mutex is already locked by different thread, the thread moves to BLOCK state.
 * In the future when this thread will be back to RUNNING state,
 * it will try again to acquire the mutex.
 * If the mutex is already locked by this thread, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_lock()
{
	if (mutex == UNLOCKED)
	{
		mutex = running->get_tid();
		return SUCCESS;
	}
	else  //if (mutex != UNLOCKED)
	{
		if (mutex != running->get_tid())
		{
			running->set_state(STATE::BLOCK);
			mutex_blocked.push_back(running);
			threads_switch();
//			uthread_block(running->get_tid());
			return SUCCESS;
		}
		else  //if (mutex == running->get_tid())
		{
			std::cerr<<LIB_ERR_MSG<<ERR_LOCK<<std::endl;
			return FAIL;
		}
	}
}


/*
 * Description: This function releases a mutex.
 * If there are blocked threads waiting for this mutex,
 * one of them (no matter which one) moves to READY state.
 * If the mutex is already unlocked, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_unlock()
{
	if (mutex == UNLOCKED)
	{
		std::cerr<<LIB_ERR_MSG<<ERR_UNLOCK<<std::endl;
		return FAIL;
	}
	if (!mutex_blocked.empty())
	{
		Thread *unlocker = mutex_blocked.back();
		unlocker->set_state(STATE::READY);
		mutex_blocked.pop_back();
		ready.push_back(unlocker);
		return SUCCESS;
	}
	return SUCCESS;
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
	return running->get_tid();
}


/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum_list.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum_list starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
	return quantum_counter;
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum_list that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum_list). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
	if (threads.find(tid) == threads.end())
	{
		std::cerr<<LIB_ERR_MSG<<NO_THRD<<std::endl;
		return FAIL;
	}
	if (threads.empty())
	{
		std::cerr<<LIB_ERR_MSG<<MT_THRD<<std::endl;
		return FAIL;
	}
	Thread* current = threads.at(tid);
    if (current->get_state() == STATE::RUNNING)
    {
        quantum_counter++;;
        current->add_quantum();
    }
	return current->get_quantum_counter();
}
