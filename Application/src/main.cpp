// https://begriffs.com/posts/2020-03-23-concurrent-programming.html

// https://www.youtube.com/watch?v=Dt51GebwNR0

// Useful: https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables

#include <cassert>
#include <vector>
#include <numeric>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <semaphore>
#include <iostream>
#include <string>
#include <random>

#include <easy/profiler.h> // Can't use it, else there's data races on the profiler's context, lol

#include "digitsOfPi.h" // Fabrice Bellard's implementation of the Bailey-Borwein-Plouffe formula allowing to compute an arbitrary digit of PI.

#if USE_WORKING_IMPLEMENTATION
#include "workingImplementation.h"
#else
#include "exercise.h"
#endif//!USE_WORKING_IMPLEMENTATION

constexpr const unsigned int SLEEP_AVG{10};
constexpr const unsigned int SLEEP_DEVIATION{10};
constexpr const size_t FIRST_DIGIT = 1;
constexpr const size_t LAST_DIGIT = 6;

struct PieceOfPi
{
	size_t producerId = 0;
	size_t consumerId = 0;
	char digit = 0;

	inline std::string ToString() const
	{
		// std::string str = "{ digit: " + digit; // TODO: look into what is exactly going on when you do this.
		std::string str = "{ digit: ";
		str += digit;
		str += "; producerId: " + std::to_string(producerId);
		str += "; consumerId: " + std::to_string(consumerId);
		str += " }";
		return str;
	}
};

PieceOfPi buffer = {};
std::string toPrint = "";
size_t iteration = FIRST_DIGIT;
std::default_random_engine e;
std::uniform_int_distribution<unsigned int> d(SLEEP_AVG - SLEEP_DEVIATION / 2, SLEEP_AVG + SLEEP_DEVIATION / 2);

void SingleThreaded_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Yellow);

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
	iteration++;
}

void SingleThreaded_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Yellow100);

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: ";
	toPrint += buffer.ToString();
	toPrint += "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

void NoMutex_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Blue);

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
	iteration++; // Placed after the long sleeping call on purpose: forces a data race.
}

void NoMutex_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Blue100);

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

std::mutex m;

void MutexOnly_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Green);
	std::unique_lock<std::mutex> lck(m); // Note: don't explicitly call .lock() and .unlock() on unique_locks: the constructors and destructors do that themselves.

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
	iteration++;
}

void MutexOnly_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Green100);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::unique_lock<std::mutex> lck(m);

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

std::condition_variable cv_producer;
std::condition_variable cv_consumer;
bool produced = false;

void CV_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Red);
	std::unique_lock<std::mutex> lck(m);
	cv_producer.wait(lck, []{return !produced;});

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	buffer.producerId = id;
	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
	iteration++;

	produced = true;
	cv_consumer.notify_one();
}

void CV_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Red100);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::unique_lock<std::mutex> lck(m);
	cv_consumer.wait(lck, []{return produced;});

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";

	produced = false;
	cv_producer.notify_one();
}

std::vector<std::thread> threads;
std::vector<size_t> iterations(LAST_DIGIT - FIRST_DIGIT + 1);

void Reset()
{
	std::iota(iterations.begin(), iterations.end(), FIRST_DIGIT);
	std::shuffle(iterations.begin(), iterations.end(), e);
	iteration = FIRST_DIGIT;
	threads.clear();
	buffer = {};
	toPrint.clear();
	toPrint.resize(1024); // 1024 is arbitrary, just to ensure there's no heap allocations.
	produced = false;
}

int main()
{
	EASY_PROFILER_ENABLE;
	e.seed(std::chrono::system_clock::now().time_since_epoch().count());

	std::cout << "Value of a float PI: " << std::to_string(3.141592f) << std::endl;

	Reset();
	std::cout << "Using SingleThreaded functions to generate digits of PI..." << std::endl;
	for (size_t i = FIRST_DIGIT; i <= LAST_DIGIT; i++)
	{
		SingleThreaded_Producer(i);
		SingleThreaded_Consumer(i);
	}
	std::cout << toPrint << std::endl;

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

	Reset();
	std::cout << "Using CV functions to generate digits of PI..." << std::endl;
	for (const auto& index : iterations)
	{
		threads.emplace_back(std::thread(CV_Producer, index));
		threads.emplace_back(std::thread(CV_Consumer, index));
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	std::cout << toPrint << std::endl;

	const auto nrOfBlocksWritten = profiler::dumpBlocksToFile("profilerOutputs/session.prof");
#ifdef BUILD_WITH_EASY_PROFILER
	assert(nrOfBlocksWritten && "Easy profiler has failed to write profiling data to disk!"); // TODO: detect file corruption by using nrOfBlocksWritten to see if all blocks have been written.
#endif //!BUILD_WITH_EASY_PROFILER

	return 0;
}