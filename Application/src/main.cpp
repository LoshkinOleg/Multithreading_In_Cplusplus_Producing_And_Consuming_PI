#include <cassert>
#include <cassert>

#include <easy/profiler.h> // Can't use it, else there's data races on the profiler's context, lol

#if USE_WORKING_IMPLEMENTATION
#include "workingImplementation.h"
#else
#include "exercise.h"
#endif // USE_WORKING_IMPLEMENTATION

// https://begriffs.com/posts/2020-03-23-concurrent-programming.html

// https://www.youtube.com/watch?v=Dt51GebwNR0

// Idea: message passing (easier coherency) for next blogpost
// another blogpost: shared memory (more performant), multi processes linked with pipes

// Demonstrate data race

// Approximate PI with a shared random engine, first with mutex, then with seeding offset: idea: strictly equivelent PI approximations

// Explain mutexes, condition variables (not a variable, "thread wait queue"), semaphore: sleeping threads and notifying threads

// Explain deadlocks (and exception when using cv's), livelocks

// Code a heisenbug

// lock-free -> blocking-free -> "non-blocking" concurrency

// implement arbiter: transaction-based

// producer consumer problem

// https://en.wikipedia.org/wiki/Bailey%E2%80%93Borwein%E2%80%93Plouffe_formula

// Useful: https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables
// "Don't wait without a condition", lost wakeup and spurious wakeup

// Idea for images: stalker's anomalies, victorian banquet?

#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>
#include <semaphore>
#include <iostream>
#include <string>
#include <vector>
#include <random>

#include "digitsOfPi.h" // Fabrice Bellard's implementation of the Bailey-Borwein-Plouffe formula allowing to compute an arbitrary digit of PI.

constexpr const unsigned int MS_SLEEP_TIME{25}; // The amount of mean milliseconds to sleep for in each block of code profiled by the easy_profiler to more easilly see the order of execution.
constexpr const unsigned int MS_SLEEP_TIME_DEVIATION{5}; // Threads will sleep for 0 to 5 milliseconds, just to fuck with the compiler.

char buffer = 0;			// Single char buffer we're using to transfer digits of PI between threads. Used in all functions.
std::string toPrint = "";	// Final string that will be written to the console once all the Produce'ing and Consume'ing has been done. Used in all functions.
std::mutex m;				// Mutex used to protect buffer. Used in MutexOnly, SimpleCV and Lockstep functions.
std::condition_variable cv_producer; // Used in SimpleCV and Lockstep functions.
std::condition_variable cv_consumer; // Used in SimpleCV and Lockstep functions.
bool produced = false;		// Used in Lockstep function.
std::vector<std::thread> threads; // Used to hold the threads for all but the SingleThreaded functions.
std::default_random_engine e; // Using a random delay to mess with the compiler so that it's unable to predict the order of execution of threads.
std::uniform_int_distribution<unsigned int> d(MS_SLEEP_TIME - MS_SLEEP_TIME_DEVIATION, MS_SLEEP_TIME + MS_SLEEP_TIME_DEVIATION);

// Note:: don't explicitly call .lock() and .unlock() on unique_locks: the ctors and dtors do that themselves.

// Resets all values to default between the runs of the functions defined below.
void Reset()
{
	std::scoped_lock<std::mutex> lck(m); // Needs to be a unique lock since we're putting the thread to sleep.

	threads.clear();
	buffer = 0;
	toPrint.clear();
	toPrint.resize(1024); // 1024 is just an arbitrary value to make sure the string doesn't request a heap allocation once the algorithms are running.
	produced = false;
}

// Single threaded algorithm.
void SingleThreaded_Producer(const size_t pos)
{

	buffer = std::to_string(GetNthPiDigit(pos))[0];
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

void SingleThreaded_Consumer()
{

	toPrint += buffer;
	toPrint += '\n';
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

// NOTE: fuck, you need to protect easy_profiler's context, else you'll end up with a data race.

// Unchecked multithreading. Data race AND not synchronized. Producer and Consumer are running concurrently and are both accessing buffer.
void NoMutex_Producer(const size_t pos)
{

	// std::scoped_lock<std::mutex> lck(m);
	// EASY_THREAD_SCOPE("NoMutex_Producer thread");
	// EASY_BLOCK("NoMutex_Producer", profiler::colors::Green);


	buffer = std::to_string(GetNthPiDigit(pos))[0];
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e))); // Note: this technically causes a data race but in this particular case, it only plays in our favor: we want randomess.

	// EASY_END_BLOCK;
}

void NoMutex_Consumer()
{

	// std::scoped_lock<std::mutex> lck(m);
	// EASY_THREAD_SCOPE("NoMutex_Consumer thread");
	// EASY_BLOCK("NoMutex_Consumer", profiler::colors::Green100);

	toPrint += buffer;
	toPrint += '\n';
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));

	// EASY_END_BLOCK;
}

// Mutex but no condition variable. No data races but still not synchronized. Producer and Consumer are running concurrently but sequentially due to the mutex but it is unknown which one will run first.
void MutexOnly_Producer(const size_t pos)
{
	std::unique_lock<std::mutex> lck(m);


	buffer = std::to_string(GetNthPiDigit(pos))[0];
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

void MutexOnly_Consumer()
{
	std::unique_lock<std::mutex> lck(m);


	toPrint += buffer;
	toPrint += '\n';
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

// Mutex and condition variable, but without predicate. No data races and in theory synchronized but victim to lost wakeups and spurious wakeups. Kicking off threads with these functions as arguments results in a deadlock due to Producer's lost wakeup.
void SimpleCV_Producer(const size_t pos)
{
	// The cv.notify_one from the main thread happens somewhere before this point in the code.

	EASY_BLOCK("SimpleCV_Producer", profiler::colors::Yellow);

	std::unique_lock<std::mutex> lck(m); // Using a unique lock instead of a scoped lock because the OS needs to be able to wake up and put the thread to sleep repeatedly. This is why we have spurious wakeups.
	cv_producer.wait(lck);						 // Calling std::condition_variable::wait makes the OS unlock the mutex and puts this thread to sleep (ideally) until the mutex the condition variable is signaled. Once the thread becomes runnable, the OS automatically locks the mutex before the rest of the code is executed.
	// cv.wait waits indefinitely because it has missed the wakeup. This is called a deadlock.

	buffer = std::to_string(GetNthPiDigit(pos))[0];
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e))); // This function misses the wakeup even if you comment this delay out, try it.

	cv_consumer.notify_one();
}

void SimpleCV_Consumer()
{
	// This function is never called because SimpleCV_Producer is perpetually waiting for a wakeup that it has missed.


	std::unique_lock<std::mutex> lck(m);
	cv_consumer.wait(lck);

	toPrint += buffer;
	toPrint += '\n';
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));

	cv_producer.notify_one();
}

// Working two thread lockstep. Mutex and condition variable with predicate.
void Lockstep_Producer(const size_t pos)
{

	std::unique_lock<std::mutex> lck(m);
	cv_producer.wait(lck, []{return !produced;});

	buffer = std::to_string(GetNthPiDigit(pos))[0];
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));

	produced = true;
	cv_consumer.notify_one();
}

void Lockstep_Consumer()
{

	std::unique_lock<std::mutex> lck(m);
	cv_consumer.wait(lck, []{return produced;});

	toPrint += buffer;
	toPrint += '\n';
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));

	produced = false;
	cv_producer.notify_one();
}

int main()
{
	EASY_PROFILER_ENABLE;
	EASY_THREAD_SCOPE("Main thread");

	e.seed(std::chrono::system_clock::now().time_since_epoch().count());
	constexpr const float PI = 3.141593f;				   // Just for printing purposes so that you can check whether the given digits are correct.
	constexpr const size_t START_OF_PI_RANGE_TO_PRINT = 1; // Starting at first decimal point of PI (Fabrice Bellard's implementation doesn't accept position 0).
	constexpr const size_t END_OF_PI_RANGE_TO_PRINT = 6;   // Last PI digit to print.
	std::cout << "Value of a float PI: " << std::to_string(PI) << std::endl;

	// WARNING: Uncomment ONLY ONE of these blocks at a time, else you'll get data races and the profiler won't be able to output valid profiling data!

	Reset();
	std::cout << "Using SingleThreaded functions to generate digits of PI..." << std::endl;
	for (size_t i = START_OF_PI_RANGE_TO_PRINT; i <= END_OF_PI_RANGE_TO_PRINT; i++)
	{
		SingleThreaded_Producer(i);
		SingleThreaded_Consumer();
	}
	std::cout << toPrint;

	// Idea: make i's random to mess with the compiler? Cuz right now everything works even without synchronization...

	// NOTE: datarace on profiler's context?
	Reset();
	std::cout << "Using NoMutex functions to generate digits of PI..." << std::endl;
	for (size_t i = START_OF_PI_RANGE_TO_PRINT; i <= END_OF_PI_RANGE_TO_PRINT; i++)
	{
		threads.push_back(std::thread(NoMutex_Producer, i));
		threads.push_back(std::thread(NoMutex_Consumer));
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	
	std::cout << toPrint;

	Reset();
	std::cout << "Using MutexOnly functions to generate digits of PI..." << std::endl;
	for (size_t i = START_OF_PI_RANGE_TO_PRINT; i <= END_OF_PI_RANGE_TO_PRINT; i++)
	{
		threads.push_back(std::thread(MutexOnly_Producer, i));
		threads.push_back(std::thread(MutexOnly_Consumer));
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	std::cout << toPrint;

	// Note: this probably doesn't result in an ordered execution either?
	// This segment causes a deadlock. See for yourself.
	// Reset();
	// std::cout << "Using SimpleCV functions to generate digits of PI..." << std::endl;
	// for (size_t i = START_OF_PI_RANGE_TO_PRINT; i <= END_OF_PI_RANGE_TO_PRINT; i++)
	// {
	// 	threads.push_back(std::thread(SimpleCV_Producer, i));
	//  cv.notify_one();
	//  EASY_EVENT("Created thread for SimpleCV_Producer and notified it.");
	// 	threads.push_back(std::thread(SimpleCV_Consumer));
	// }
	// for (auto& thread : threads)
	// {
	// 	thread.join();
	// }
	// std::cout << toPrint << std::endl;

	// Note: this does not result in an ordererd execution!
	Reset();
	std::cout << "Using Lockstep functions to generate digits of PI..." << std::endl;
	for (size_t i = START_OF_PI_RANGE_TO_PRINT; i <= END_OF_PI_RANGE_TO_PRINT; i++)
	{
		threads.emplace_back(std::thread(Lockstep_Producer, i));
		threads.emplace_back(std::thread(Lockstep_Consumer));
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	std::cout << toPrint;

	// Output easy_profiler's data.
#if BUILD_WITH_EASY_PROFILER
	// const auto success = profiler::dumpBlocksToFile("profilerOutputs/session.prof");
	// assert(success && "Failed to write profiling data to file.");
#endif

	return 0;
}