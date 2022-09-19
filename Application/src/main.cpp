#include <cassert>
#include <cassert>
#include <vector>
#include <numeric>

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

constexpr const unsigned int MS_SLEEP_TIME{5}; // The amount of mean milliseconds to sleep for in each block of code profiled by the easy_profiler to more easilly see the order of execution.
constexpr const unsigned int MS_SLEEP_TIME_DEVIATION{5}; // Threads will sleep for 0 to 5 milliseconds, just to fuck with the compiler.
constexpr const size_t START_OF_PI_RANGE_TO_PRINT = 1; // Starting at first decimal point of PI (Fabrice Bellard's implementation doesn't accept position 0).
constexpr const size_t END_OF_PI_RANGE_TO_PRINT = 6;   // Last PI digit to print.

struct PieceOfPi
{
	char digit = 0;
	size_t producerId = 0;
	size_t consumerId = 0;

	std::string ToString() const
	{
		// str::string str = "{ digit: " + digit;
		std::string str = "{ digit: ";
		str += digit;
		str += "; producerId: " + std::to_string(producerId);
		str += "; consumerId: " + std::to_string(consumerId);
		str += " }";
		return str;
	}
};

PieceOfPi buffer = {};			// Single char buffer we're using to transfer digits of PI between threads. Used in all functions.
size_t iteration = START_OF_PI_RANGE_TO_PRINT;
std::string toPrint = "";	// Final string that will be written to the console once all the Produce'ing and Consume'ing has been done. Used in all functions.
std::mutex m;				// Mutex used to protect buffer. Used in MutexOnly, SimpleCV and Lockstep functions.
std::condition_variable cv_producer; // Used in SimpleCV and Lockstep functions.
std::condition_variable cv_consumer; // Used in SimpleCV and Lockstep functions.
bool produced = false;		// Used in Lockstep function.
std::vector<std::thread> threads; // Used to hold the threads for all but the SingleThreaded functions.
std::default_random_engine e; // Using a random delay to mess with the compiler so that it's unable to predict the order of execution of threads.
std::uniform_int_distribution<unsigned int> d(MS_SLEEP_TIME - MS_SLEEP_TIME_DEVIATION, MS_SLEEP_TIME + MS_SLEEP_TIME_DEVIATION);
std::vector<size_t> iterations(END_OF_PI_RANGE_TO_PRINT - START_OF_PI_RANGE_TO_PRINT + 1); // Random list of indices to digits of PI. Used in main to prevent the program from accidentally producing coherent results (kicking off threads in order can, but is not guaranteed to, produce a correct sequential result).

// Note:: don't explicitly call .lock() and .unlock() on unique_locks: the ctors and dtors do that themselves.

// Resets all values to default between the runs of the functions defined below.
void Reset()
{
	// Randomly start off iterations to avoid accidentally producing coherent results.
	std::iota(iterations.begin(), iterations.end(), START_OF_PI_RANGE_TO_PRINT);
	std::shuffle(iterations.begin(), iterations.end(), e);
	iteration = START_OF_PI_RANGE_TO_PRINT;

	threads.clear();
	buffer = {};
	toPrint.clear();
	toPrint.resize(10 * 1024); // 1024 is just an arbitrary value to make sure the string doesn't request a heap allocation once the algorithms are running.
	produced = false;
}

// Single threaded algorithm.
void SingleThreaded_Producer(const size_t id)
{

	buffer.digit = std::to_string(GetNthPiDigit(iteration++))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

void SingleThreaded_Consumer(const size_t id)
{
	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: ";
	toPrint += buffer.ToString();
	toPrint += "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

// NOTE: fuck, you need to protect easy_profiler's context, else you'll end up with a data race.

// Unchecked multithreading. Data race AND not synchronized. Producer and Consumer are running concurrently and are both accessing buffer.
void NoMutex_Producer(const size_t id)
{

	// std::scoped_lock<std::mutex> lck(m);
	// EASY_THREAD_SCOPE("NoMutex_Producer thread");
	// EASY_BLOCK("NoMutex_Producer", profiler::colors::Green);

	// NOTE: why is this not causing a data race?...

	buffer.digit = std::to_string(GetNthPiDigit(iteration++))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e))); // Note: this technically causes a data race but in this particular case, it only plays in our favor: we want randomess.

	// EASY_END_BLOCK;
}

void NoMutex_Consumer(const size_t id)
{

	// std::scoped_lock<std::mutex> lck(m);
	// EASY_THREAD_SCOPE("NoMutex_Consumer thread");
	// EASY_BLOCK("NoMutex_Consumer", profiler::colors::Green100);

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));

	// EASY_END_BLOCK;
}

// Mutex but no condition variable. No data races but still not synchronized. Producer and Consumer are running concurrently but sequentially due to the mutex but it is unknown which one will run first.
void MutexOnly_Producer(const size_t id)
{
	std::unique_lock<std::mutex> lck(m);


	buffer.digit = std::to_string(GetNthPiDigit(iteration++))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

void MutexOnly_Consumer(const size_t id)
{
	std::unique_lock<std::mutex> lck(m);


	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

// Working two thread lockstep. Mutex and condition variable with predicate.
void Lockstep_Producer(const size_t id)
{

	std::unique_lock<std::mutex> lck(m);
	cv_producer.wait(lck, []{return !produced;});

	buffer.digit = std::to_string(GetNthPiDigit(iteration++))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));

	produced = true;
	cv_consumer.notify_one();
}

void Lockstep_Consumer(const size_t id)
{

	std::unique_lock<std::mutex> lck(m);
	cv_consumer.wait(lck, []{return produced;});

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));

	produced = false;
	cv_producer.notify_one();
}

int main()
{
	e.seed(std::chrono::system_clock::now().time_since_epoch().count());
	constexpr const float PI = 3.141593f;				   // Just for printing purposes so that you can check whether the given digits are correct.
	std::cout << "Value of a float PI: " << std::to_string(PI) << std::endl;

	// WARNING: Uncomment ONLY ONE of these blocks at a time, else you'll get data races and the profiler won't be able to output valid profiling data!

	Reset();
	std::cout << "Using SingleThreaded functions to generate digits of PI..." << std::endl;
	for (size_t i = START_OF_PI_RANGE_TO_PRINT; i <= END_OF_PI_RANGE_TO_PRINT; i++)
	{
		SingleThreaded_Producer(i);
		SingleThreaded_Consumer(i);
	}
	std::cout << toPrint << std::endl;

	// Idea: make i's random to mess with the compiler? Cuz right now everything works even without synchronization...

	// NOTE: datarace on profiler's context?

	Reset();
	std::cout << "Using NoMutex functions to generate digits of PI..." << std::endl;
	for (const auto& index : iterations)
	{
		threads.push_back(std::thread(NoMutex_Producer, index));
		threads.push_back(std::thread(NoMutex_Consumer, index));
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	std::cout << toPrint << std::endl;

	Reset();
	std::cout << "Using MutexOnly functions to generate digits of PI..." << std::endl;
	for (const auto& index : iterations)
	{
		threads.push_back(std::thread(MutexOnly_Producer, index));
		threads.push_back(std::thread(MutexOnly_Consumer, index));
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	std::cout << toPrint << std::endl;

	// TODO: in blogpost, first try simple cv, without predicate.

	// Note: this does not result in an ordererd execution!
	Reset();
	std::cout << "Using Lockstep functions to generate digits of PI..." << std::endl;
	for (const auto& index : iterations)
	{
		threads.emplace_back(std::thread(Lockstep_Producer, index));
		threads.emplace_back(std::thread(Lockstep_Consumer, index));
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	std::cout << toPrint << std::endl;

	return 0;
}