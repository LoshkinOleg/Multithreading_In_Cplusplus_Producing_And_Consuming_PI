#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
#include <random>

#include <easy/profiler.h> // Used on Windows builds.

#include "digitsOfPi.h" // Fabrice Bellard's implementation of the Bailey-Borwein-Plouffe formula allowing to compute an arbitrary digit of PI.

constexpr const size_t FIRST_DIGIT = 0; // Firist digit of PI to print.
constexpr const size_t LAST_DIGIT = 6; // Last digit of PI to print.

// Structure the functions will use to pass around digits of PI.
struct PieceOfPi
{
	size_t producerId = 0; // Index of the producer thread that has generated the digit.
	size_t consumerId = 0; // Index of the consumer thread that has consumed the digit.
	char digit = 0; // The digit to consume.

	// Use to get a string description of the PieceOfPi.
	inline std::string ToString() const
	{
		std::string str = "{ digit: ";
		str += digit;
		str += "; producerId: " + std::to_string(producerId);
		str += "; consumerId: " + std::to_string(consumerId);
		str += " }";
		return str;
	}
};

// Puts the current thread to sleep for a random amount of time to mess with the compiler and CPU to make the code more unpredictable.
void MessWithCompiler()
{
	static std::default_random_engine e;
	static std::uniform_int_distribution<unsigned int> d(0, 25);

	std::this_thread::sleep_for(std::chrono::milliseconds(d(e)));
}

PieceOfPi buffer = {}; // Structure the functions will use to pass around digits of PI. Used both by producers and consumers. This resource needs to be protected.
std::string toPrint = ""; // Where consumers will write the result of the producer. This resource needs to be protected.
size_t iteration = FIRST_DIGIT; // The index of the next digit of PI to compute. This resource needs to be protected.

void SingleThreaded_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Yellow);

    // TODO: set the digit to next digit of PI using GetNthPiDigit(iteration). Set producerId to id. At the end, increment iteration.
}

void SingleThreaded_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Yellow100);

    // TODO: set consumerId to id. Put buffer.ToString into toPrint.
}

void NoMutex_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Blue);

    // TODO: set the digit to next digit of PI using GetNthPiDigit(iteration). Set producerId to id. At the end, increment iteration.

	// MessWithCompiler(); // Uncomment this once everything is working and you should see a data race.

    // TODO: increment iteration here.
}

void NoMutex_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Blue100);

    // TODO: set consumerId to id. Put buffer.ToString into toPrint.
}

std::mutex m; // A mutex. Used to make critical operations atomic.

void MutexOnly_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Green);

    // TODO: lock m.

    // TODO: set the digit to next digit of PI using GetNthPiDigit(iteration). Set producerId to id.

    // TODO: increment iteration here.
}

void MutexOnly_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Green100);

	// MessWithCompiler(); // Uncomment this once everything is working and you should see issues with the order of execution.

    // TODO: lock m.

    // TODO: set consumerId to id. Put buffer.ToString into toPrint.
}

std::condition_variable cv_producer; // A condition variable. Used for signaling events and can be used to synchronize things.
std::condition_variable cv_consumer;
bool produced = false; // Not technically required but is required practically. Used as a predicate to prevent spurious wakeups.

void CV_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Red);

	MessWithCompiler(); // Everything should be working despite these.

    // TODO: lock m and wait for cv_producer with produced as predicate. cv_producer should wake up when produced becomes false.

	MessWithCompiler(); // Everything should be working despite these.

    // TODO: set consumerId to id. Put buffer.ToString into toPrint.

	MessWithCompiler(); // Everything should be working despite these.
	
    // TODO: increment iteration here.

	MessWithCompiler(); // Everything should be working despite these.

    // TODO: notify a consumer. The consumer expects produced to be true.

	MessWithCompiler(); // Everything should be working despite these.
}

void CV_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Red100);

	MessWithCompiler(); // Everything should be working despite these.
	
    // TODO: lock m and wait for cv_consumer with produced as predicate. cv_consumer should wake up when produced becomes true.

	MessWithCompiler(); // Everything should be working despite these.

    // TODO: set consumerId to id. Put buffer.ToString into toPrint.

	MessWithCompiler(); // Everything should be working despite these.

    // TODO: notify a producer. The producer expects produced to be false.

	MessWithCompiler(); // Everything should be working despite these.
}