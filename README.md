# Multithreading in C++: Producing and Consuming PI
This repository contains the code used in the blogpost [Multithreading in C++: Producing and Consuming PI](). It demonstrates how to use the multithreading facilities of C++ to synchronize multiple threads of the program using std::mutex, std::condition_variable and std::counting_semaphore.

Note that C++20 is needed to run the semaphore code.

## Windows users
I suggest using Visual Studio 2019 or greater to run this project. Use CMake's GUI to generate the visual studio solution.

## Linux users
This project is working on Ubuntu Focal Fossa using VSCode as the IDE with it's CMake Tools extension. Note that MSVC defines the working directory as "/build", you'll need to do the same for your IDE manually, otherwise easy_profiler won't be able to output profiling results to "/build/profilerOutputs" and the assert in main will trigger!

On VSCode you can do so by defining "cwd": "${workspaceFolder}/build" in launch.json instead of the default value of "Application/src".

Note that on Linux, easy_profiler is known to have issues when profiling multithreaded programs.
