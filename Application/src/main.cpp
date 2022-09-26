#include <vector>
#include <numeric>
#include <iostream>
#include <random>
#include <thread>
#include <cassert>

#include <easy/profiler.h> // Used on Windows builds.

#if USE_WORKING_IMPLEMENTATION
#include "workingImplementation.h"
#else
#include "exercise.h"
#endif//!USE_WORKING_IMPLEMENTATION

// Used to make the order of kicking of producers and consumers unpredictable.
std::vector<std::thread> threads;
std::vector<size_t> iterations(LAST_DIGIT - FIRST_DIGIT + 1);

// Resets all variables to default.
void Reset()
{
	std::iota(iterations.begin(), iterations.end(), FIRST_DIGIT);
	std::shuffle(iterations.begin(), iterations.end(), std::default_random_engine());
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
	assert(nrOfBlocksWritten && "Easy profiler has failed to write profiling data to disk!");
#endif //!BUILD_WITH_EASY_PROFILER

	return 0;
}