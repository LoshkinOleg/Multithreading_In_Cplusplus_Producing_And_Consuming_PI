cmake_minimum_required (VERSION 3.14) # Define minimal CMake version.
project(ApproximatingPi) # Name the visual studio solution.

# Set the C++20 as the C++ standard to use (ISO/IEC 14882).
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure build dir exists. We'll be relying on this for finding relative paths in the code and any external tools.
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/build/")
	message(FATAL_ERROR "Please specify an out-of-source directory 'build/' in the project's root directory. If you don't know what an out-of-source build is, here's a link: https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html")
endif()

file(GLOB_RECURSE app_include Application/include/*.h) # Grab the program's implementations located in the include dir for the purposes of this exercice.
file(GLOB_RECURSE app_src Application/src/*.cpp) # Grab the .cpp that will be using the implementations provided.
add_executable(Application ${app_include} ${app_src})
target_include_directories(Application PRIVATE 
	${PROJECT_SOURCE_DIR}/Application/include/ # Detect own headers.
	${PROJECT_SOURCE_DIR}/thirdparty/easy_profiler/include/ # Open-source and easy to add to a project profiling library. Useful to see if we're actually kicking off new threads or not.
	)
target_link_libraries(Application PRIVATE # PRIVATE is used as <keyword> here since Application is an executable, nothing will depend on it.
	general ${PROJECT_SOURCE_DIR}/thirdparty/easy_profiler/lib/easy_profiler.lib # Linking against the .lib of the easy_profiler dynamic library. This means that the pre-compiled "easy_profiler.dll" will need to be placed besides the Application.exe. Use the moveDlls.bat to do that automatically.
	)

set_target_properties(Application PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/Application/bin") # Output compiled binaries to their own folder.

file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/build/profilerOutputs) # Folder for holding easy_profiler's profiling data. file(MAKE_DIRECTORY <dir>) creates a new specified directiory if it doesn't exist yet.

set(USE_EASY_PROFILER ON CACHE BOOL "Whether to enable profiling with easy_profiler. Generated .prof files will be located under /profilerOutputs/") # set(<define> <default value> CACHE <variable type> <description>) creates a variable interactible in the CMake GUI.
if (USE_EASY_PROFILER) # If the use of easy_profiler is desired, add a global preprocessor definition.
	add_compile_definitions(BUILD_WITH_EASY_PROFILER) # BUILD_WITH_EASY_PROFILER is the define that the library's user must declare when they wish to use easy_profiler.
endif()

set(USE_WORKING_IMPLEMENTATION ON CACHE BOOL "Whether to use the already working implementation of PI approximating functions. Disable to make the program run your own implementations you've written in Application/include/exercice.h .")
if (USE_WORKING_IMPLEMENTATION)
	add_compile_definitions(USE_WORKING_IMPLEMENTATION) # Define used in Application/src/main.cpp to tell what implementation to use, the already working one or your own.
endif()