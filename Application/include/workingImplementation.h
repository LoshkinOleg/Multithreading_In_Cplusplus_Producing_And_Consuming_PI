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

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	buffer.producerId = id;
	iteration++;
}

void SingleThreaded_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Yellow100);

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: ";
	toPrint += buffer.ToString();
	toPrint += "\n";
}

void NoMutex_Producer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Blue);

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	buffer.producerId = id;

	MessWithCompiler(); // This forces a data race.

	iteration++;
}

void NoMutex_Consumer(const size_t id)
{
	EASY_FUNCTION(profiler::colors::Blue100);

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
}

std::mutex m; // A mutex. Used to make critical operations atomic.

void MutexOnly_Producer(const size_t id)
{
	std::unique_lock<std::mutex> lck(m); // Note: don't explicitly call .lock() and .unlock() on unique_locks: the constructors and destructors do that themselves.

	EASY_FUNCTION(profiler::colors::Green);

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	buffer.producerId = id;

	iteration++;
}

void MutexOnly_Consumer(const size_t id)
{
	MessWithCompiler(); // This forces an order of execution issue.

	std::unique_lock<std::mutex> lck(m);

	EASY_FUNCTION(profiler::colors::Green100);

	buffer.consumerId = id;
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
}

std::condition_variable cv_producer; // A condition variable. Used for signaling events and can be used to synchronize things.
std::condition_variable cv_consumer;
bool produced = false; // Not technically required but is required practically. Used as a predicate to prevent spurious wakeups.

void CV_Producer(const size_t id)
{
	MessWithCompiler(); // Everything still works despite this.
	std::unique_lock<std::mutex> lck(m);
	MessWithCompiler(); // Everything still works despite this.
	cv_producer.wait(lck, []{return !produced;});
	EASY_FUNCTION(profiler::colors::Red);
	MessWithCompiler(); // Everything still works despite this.

	buffer.digit = std::to_string(GetNthPiDigit(iteration))[0];
	MessWithCompiler(); // Everything still works despite this.
	buffer.producerId = id;
	MessWithCompiler(); // Everything still works despite this.
	iteration++;
	MessWithCompiler(); // Everything still works despite this.

	produced = true;
	MessWithCompiler(); // Everything still works despite this.
	cv_consumer.notify_one();
	MessWithCompiler(); // Everything still works despite this.
}

void CV_Consumer(const size_t id)
{
	MessWithCompiler(); // Everything still works despite this.
	std::unique_lock<std::mutex> lck(m);
	MessWithCompiler(); // Everything still works despite this.
	cv_consumer.wait(lck, []{return produced;});
	EASY_FUNCTION(profiler::colors::Red100);
	MessWithCompiler(); // Everything still works despite this.

	buffer.consumerId = id;
	MessWithCompiler(); // Everything still works despite this.
	toPrint += "Consumer has recieved the buffer: " + buffer.ToString() + "\n";
	MessWithCompiler(); // Everything still works despite this.

	produced = false;
	MessWithCompiler(); // Everything still works despite this.
	cv_producer.notify_one();
	MessWithCompiler(); // Everything still works despite this.
}