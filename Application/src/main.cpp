#include <cassert>
#include <cassert>

#include <easy/profiler.h>

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

#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>
#include <semaphore>
#include <iostream>
#include <string>

#include "digitsOfPi.h"

constexpr const float PI = 3.141593f;
constexpr const size_t START_OF_PI_RANGE_TO_PRINT = 1;
constexpr const size_t END_OF_PI_RANGE_TO_PRINT = 6;

char buffer = 0;
int iteration = START_OF_PI_RANGE_TO_PRINT;
std::mutex m;

void Produce()
{
	try
	{
		buffer = std::to_string(GetNthPiDigit(iteration++))[0];
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		throw e;
	}
}

void Consume()
{
	std::cout << std::to_string(iteration) << "'th digit of PI: " << buffer << std::endl;
}

int main()
{
	EASY_PROFILER_ENABLE;

	std::cout << "Value of a float PI: " << std::to_string(PI) << std::endl;
	for (size_t i = START_OF_PI_RANGE_TO_PRINT; i < END_OF_PI_RANGE_TO_PRINT; i++)
	{
		Produce();
		Consume();
	}

	// Output easy_profiler's data.
#if BUILD_WITH_EASY_PROFILER
	const auto success = profiler::dumpBlocksToFile("profilerOutputs/session.prof");
	// assert(success && "Failed to write profiling data to file.");
#endif

	return 0;
}