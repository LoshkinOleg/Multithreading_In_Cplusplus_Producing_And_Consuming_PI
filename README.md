# Multithreading In C++: Approximating PI
This is the repository containing code used in the blogpost [Multithreading in C++: Approximating PI]() and an empty exercice file for you to implement it yourself (exercice.h).
This project uses Sergey Yagovtsev's excellent [easy_profiler](http://github.com/yse/easy_profiler) library.

## Usage
1. Make an [out-of-source CMake build](https://cprieto.com/posts/2016/10/cmake-out-of-source-build.html) under "<sourceDir>/build".
2. Disable "USE_WORKING_IMPLEMENTATION" in CMake's GUI application if you wish to start writing an implementation yourself.
3. If you're on Windows, run "moveDlls.bat" or manually move "/thridparty/easy_profiler/bin/easy_profiler.dll" to "/build/Application/bin/Debug/" and "/build/Application/bin/Release/".
4. Launch the generated VS solution (or other IDE you're using) and set "Application" to be the default project.
5. Write your own implementation in "/Application/include/exercise.h".