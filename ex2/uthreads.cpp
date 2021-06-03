//
// Created by m7mdg on 03-May-21.
//

#include "uthreads.h"
#include <map>
#include <vector>
#include <queue>
#include <iostream>
#include <sys/time.h>
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
#define ERR_BLOCK_MAIN "You can't blocked the main thread!"
#define ERR_SIGPROCMASK "Error in sigprocmask function!"
#define ERR_TERMINATE "Error in terminating thread!"
#define ERR_MAX "Max number of threads reached, can't create new thread !"
#define ERR_TIMER "Problem with setting the timer!"
#define ERR_SIGEMPTYSET "Error in sigemptyset function !"
#define ERR_SIGACTION "Error in signal action !"
#define ERR_QUANTUM_LEN "Invalid length of a quantum_list !"
#define NO_BLOCK "There is no BLOCKED_THRD threads!"
#define ERR_BLOCK "Couldn't remove from BLOCKED_THRD threads!"
#define NO_READY "There is no READY threads!"
#define ERR_READY "Couldn't remove from READY threads!"
#define ERR_SIGADDSET "Error in sigaddset function!"
#define MTX_BLK 1
#define NO_MTX_BLK 0
#define AFTER_MTX_BLK 2
#define BLOCKED_THRD 1
#define UNBLOCKED_THRD 0

enum STATE
{
	READY, RUNNING, BLOCK
};

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
	char *_stack = new char[STACK_SIZE];
	int _quantum_counter = 0;
	int _tid;
	int _mutex_blocked = NO_MTX_BLK;
	int _blocked = UNBLOCKED_THRD;

public:
	sigjmp_buf sig;

	void (*entry_point)(void);

	Thread(STATE state, int tid, void (*func)(void)) : _state(state), _tid(tid), sig(), entry_point(func)
	{}

	~Thread()
	{ delete[] _stack; }

	STATE get_state()
	{ return _state; }

	void set_state(STATE state)
	{ _state = state; }

	char *get_stack()
	{ return _stack; }

	int get_quantum_counter() const
	{ return _quantum_counter; }

	void inc_quantum_counter()
	{ _quantum_counter++; }

	int get_tid() const
	{ return _tid; }

	int get_mutex_blocked() const
	{ return _mutex_blocked; }

	void set_mutex_blocked(int other)
	{ _mutex_blocked = other; }

	int get_blocked() const
	{ return _blocked; }

	void set_blocked(int other)
	{ _blocked = other; }

	void load_context()
	{ siglongjmp(sig, 1); }

	int save_context()
	{ return sigsetjmp(sig, 1); }

};

int mutex = UNLOCKED;
int quantum_list = 0;
static int quantum_total = 0;
static std::map<int, Thread *> threads;
struct itimerval timer;
struct sigaction sig;
static Thread *running;
static std::deque<Thread *> ready;
static std::vector<Thread *> blocked;
static std::deque<Thread *> mutex_blocked;
int from_mutex_lock = 0;

// Helper Functions Declarations - START

void setup(Thread *thread);

void memory_free();

void block_signals();

void unblock_signals();

void threads_switch(int itself);

int first_mt_id();

int remove_ready(int id);

int remove_blocked(int id);

static void handler(int);

// Helper Functions Declarations - END

/////////////////////////////////////////////////////////////

// Helper Functions Implementations - START


void setup(Thread *thread)
{
	auto sp = (address_t) thread->get_stack() + STACK_SIZE - sizeof(address_t);
	auto pc = (address_t) thread->entry_point;
	sigsetjmp(thread->sig, 1);
	((thread->sig)->__jmpbuf)[JB_SP] = translate_address(sp);
	((thread->sig)->__jmpbuf)[JB_PC] = translate_address(pc);
	if (sigemptyset(&(thread->sig)->__saved_mask) == FAIL)
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGEMPTYSET << std::endl;
		exit(EXIT_FAILURE);
	}
}

void block_signals()
{
	if (sigprocmask(SIG_BLOCK, &sig.sa_mask, nullptr))
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGPROCMASK << std::endl;
		exit(EXIT_FAILURE);
	}
}

void unblock_signals()
{
	if (sigprocmask(SIG_UNBLOCK, &sig.sa_mask, nullptr))
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGPROCMASK << std::endl;
		exit(EXIT_FAILURE);
	}
}


void memory_free()
{
	for (auto &thread: threads)
	{
		delete (thread.second);
	}
	threads.clear();
	ready.clear();
	blocked.clear();
	if (sigemptyset(&sig.sa_mask))
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGEMPTYSET << std::endl;
		exit(EXIT_FAILURE);
	}
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	exit(EXIT_SUCCESS);
}

void threads_switch(int itself)
{
	block_signals();
	Thread *current = running;
	current->set_state(STATE::READY);
	Thread *thread_to_run = ready.front();
	if (thread_to_run != nullptr)
	{
		running = thread_to_run;
	}
	ready.pop_front();
	running->set_state(STATE::RUNNING);

	quantum_total++;
	if (!itself)
	{
		ready.push_back(current);
	}
	running->inc_quantum_counter();
	if (running->get_mutex_blocked() == 2)
	{
		uthread_mutex_lock();
	}
	block_signals();
}

int first_mt_id()
{
	int id = 0;
	for (auto it = threads.begin(); it != threads.end(); id++, it++)
	{
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
		std::cerr << LIB_ERR_MSG << NO_READY << std::endl;
		return FAIL;
	}
	for (auto it = ready.begin(); it != ready.end(); it++)
	{
		if (threads.at(id) == (*it) && (*it)->get_tid() == id)
		{
			ready.erase(it);
			return EXIT_SUCCESS;
		}
	}
	std::cerr << LIB_ERR_MSG << ERR_READY << std::endl;
	return FAIL;
}

int remove_blocked(int id)
{
	if (blocked.empty())
	{
		std::cerr << LIB_ERR_MSG << NO_BLOCK << std::endl;
		return FAIL;
	}
	for (auto it = blocked.begin(); it != blocked.end(); it++)
	{
		if (threads.at(id) == (*it) && (*it)->get_tid() == id)
		{
			blocked.erase(it);
			return SUCCESS;
		}
	}
	std::cerr << LIB_ERR_MSG << ERR_BLOCK << std::endl;
	return FAIL;
}


static void handler(int)
{
	if (setitimer(ITIMER_VIRTUAL, &timer, nullptr))
	{
		std::cerr << SYS_ERR_MSG << ERR_TIMER << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!running->save_context())
	{
		threads_switch(0);
		running->load_context();
	}
}

// Helper Functions Implementations - END


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
	sig.sa_handler = handler;
	if (sigemptyset(&sig.sa_mask) == FAIL)
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGEMPTYSET << std::endl;
		exit(EXIT_FAILURE);
	}
	if (sigaddset(&sig.sa_mask, SIGVTALRM) == FAIL)
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGADDSET << std::endl;
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGVTALRM, &sig, nullptr) == FAIL)
	{
		std::cerr << SYS_ERR_MSG << ERR_SIGACTION << std::endl;
		exit(EXIT_FAILURE);
	}
	quantum_list = quantum_usecs;
	timer.it_value.tv_sec = quantum_list / MIC_TO_SEC;
	timer.it_value.tv_usec = quantum_list % MIC_TO_SEC;
	timer.it_interval.tv_sec = quantum_list / MIC_TO_SEC;
	timer.it_interval.tv_usec = quantum_list % MIC_TO_SEC;
	block_signals();

	auto main_thread = new Thread(STATE::RUNNING, 0, nullptr);
	main_thread->inc_quantum_counter();
	quantum_total++;
	running = main_thread;
	threads.insert(std::pair<int, Thread *>(0, main_thread));
	if (setitimer(ITIMER_VIRTUAL, &timer, nullptr))
	{
		std::cerr << SYS_ERR_MSG << ERR_TIMER << std::endl;
		exit(EXIT_FAILURE);
	}
	unblock_signals();
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
	if (threads.size() + 1 > MAX_THREAD_NUM)
	{
		std::cerr << LIB_ERR_MSG << ERR_MAX << std::endl;
		sigprocmask(SIG_UNBLOCK, &sig.sa_mask, nullptr);
		return FAIL;
	}
	block_signals();
	int next_id = first_mt_id();
	auto next_thread = new Thread(STATE::READY, next_id, f);
	threads.insert(std::pair<int, Thread *>(next_id, next_thread));
	ready.push_back(next_thread);
	setup(next_thread);
	unblock_signals();
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
	block_signals();
	if (threads.find(tid) == threads.end())
	{
		std::cerr << LIB_ERR_MSG << NO_THRD << std::endl;
		unblock_signals();
		return FAIL;
	}
	if (tid == MAIN_THRD)
	{
		unblock_signals();
		exit(EXIT_SUCCESS);
	}
	if (tid == running->get_tid())
	{
		if (running->get_tid() == mutex)
		{
			uthread_mutex_unlock();
		}
		Thread *to_terminate = running;
		threads_switch(1);
		threads.erase(tid);
		delete (to_terminate);
		unblock_signals();
		running->load_context();
		if (setitimer(ITIMER_VIRTUAL, &timer, nullptr))
		{
			std::cerr << SYS_ERR_MSG << ERR_TIMER << std::endl;
			exit(EXIT_FAILURE);
		}
		return SUCCESS;
	}
	if (threads.at(tid)->get_state() == STATE::BLOCK)
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
	unblock_signals();
	return SUCCESS;
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED_THRD state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
	block_signals();
	if (threads.find(tid) == threads.end())
	{
		std::cerr << LIB_ERR_MSG << NO_THRD << std::endl;
		unblock_signals();
		return FAIL;
	}
	if (tid == MAIN_THRD && !from_mutex_lock)
	{
		std::cerr << LIB_ERR_MSG << ERR_BLOCK_MAIN << std::endl;
		return FAIL;
	}
	if (tid == running->get_tid())
	{
		if (sigsetjmp(running->sig, 1) != 1)
		{
			if (!from_mutex_lock)
			{
				running->set_blocked(BLOCKED_THRD);
			}
			blocked.push_back(running);
			threads_switch(1);
			threads.at(tid)->set_state(STATE::BLOCK);
			running->load_context();
		}
	}
	else if (threads.at(tid)->get_state() == STATE::READY)
	{
		if (remove_ready(tid) == FAIL)
		{
			return FAIL;
		}
		threads.at(tid)->set_state(STATE::BLOCK);
		blocked.push_back(threads.at(tid));
	}
	if (threads.at(tid)->get_blocked() == UNBLOCKED_THRD)
	{
		threads.at(tid)->set_blocked(BLOCKED_THRD);
	}
	unblock_signals();
	return SUCCESS;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{

	block_signals();
	if (threads.find(tid) == threads.end())
	{
		std::cerr << LIB_ERR_MSG << NO_THRD << std::endl;
		unblock_signals();
		return FAIL;
	}
	if (threads.at(tid)->get_state() == STATE::BLOCK)
	{
		ready.push_back(threads.at(tid));
	}
	else if (threads.at(tid)->get_state() == STATE::READY)
	{
		int flag = 0;
		for (auto &it : ready)
		{
			if (it->get_tid() == tid)
			{
				flag = 1;
				break;
			}
		}
		if (!flag)
		{
			ready.push_back(threads.at(tid));
		}
	}
	remove_blocked(tid);
	threads.at(tid)->set_state(STATE::READY);
	unblock_signals();
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
	block_signals();
	if (mutex == UNLOCKED)
	{
		mutex = running->get_tid();
		unblock_signals();
		return SUCCESS;
	}
	else  //if (mutex != UNLOCKED)
	{
		if (mutex != running->get_tid())
		{
			Thread *to_block = running;
			to_block->set_mutex_blocked(MTX_BLK);
			mutex_blocked.push_back(to_block);
			unblock_signals();
			from_mutex_lock = 1;
			int result = uthread_block(running->get_tid());
			from_mutex_lock = 0;
			return result;
		}
		else  //if (mutex == running->get_tid())
		{
			std::cerr << LIB_ERR_MSG << ERR_LOCK << std::endl;
			unblock_signals();
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
	block_signals();
	if (mutex == UNLOCKED)
	{
		std::cerr << LIB_ERR_MSG << ERR_UNLOCK << std::endl;
		unblock_signals();
		return FAIL;
	}
	if (mutex != running->get_tid())
	{
		std::cerr << LIB_ERR_MSG << ERR_UNLOCK << std::endl;
		unblock_signals();
		return FAIL;
	}
	if (!mutex_blocked.empty())
	{
		Thread *to_unblock = nullptr;
		for (auto it = mutex_blocked.begin(); it != mutex_blocked.end(); it++)
		{
			if ((*it)->get_blocked() == UNBLOCKED_THRD)
			{
				to_unblock = (*it);
				mutex_blocked.erase(it);
				break;
			}
		}
		if (to_unblock != nullptr)
		{
			to_unblock->set_mutex_blocked(AFTER_MTX_BLK);
			uthread_resume(to_unblock->get_tid());
		}
	}
	mutex = UNLOCKED;
	unblock_signals();
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
	return quantum_total;
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
	block_signals();
	if (threads.find(tid) == threads.end())
	{
		std::cerr << LIB_ERR_MSG << NO_THRD << std::endl;
		unblock_signals();
		return FAIL;
	}
	if (threads.empty())
	{
		std::cerr << LIB_ERR_MSG << MT_THRD << std::endl;
		unblock_signals();
		return FAIL;
	}
	Thread *current = threads.at(tid);
	unblock_signals();
	return current->get_quantum_counter();
}
