# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.6.2/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.6.2/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/xaxxon/v8toolkit

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/xaxxon/v8toolkit/Debug

# Include any dependencies generated for this target.
include CMakeFiles/javascript_sample.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/javascript_sample.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/javascript_sample.dir/flags.make

CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o: CMakeFiles/javascript_sample.dir/flags.make
CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o: ../samples/javascript_sample.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/xaxxon/v8toolkit/Debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o"
	/Users/xaxxon/Downloads/clang+llvm-3.9.0-x86_64-apple-darwin/bin/clang++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o -c /Users/xaxxon/v8toolkit/samples/javascript_sample.cpp

CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.i"
	/Users/xaxxon/Downloads/clang+llvm-3.9.0-x86_64-apple-darwin/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/xaxxon/v8toolkit/samples/javascript_sample.cpp > CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.i

CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.s"
	/Users/xaxxon/Downloads/clang+llvm-3.9.0-x86_64-apple-darwin/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/xaxxon/v8toolkit/samples/javascript_sample.cpp -o CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.s

CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.requires:

.PHONY : CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.requires

CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.provides: CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.requires
	$(MAKE) -f CMakeFiles/javascript_sample.dir/build.make CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.provides.build
.PHONY : CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.provides

CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.provides.build: CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o


# Object files for target javascript_sample
javascript_sample_OBJECTS = \
"CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o"

# External object files for target javascript_sample
javascript_sample_EXTERNAL_OBJECTS =

javascript_sample: CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o
javascript_sample: CMakeFiles/javascript_sample.dir/build.make
javascript_sample: libv8toolkit_shared.dylib
javascript_sample: CMakeFiles/javascript_sample.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/xaxxon/v8toolkit/Debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable javascript_sample"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/javascript_sample.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/javascript_sample.dir/build: javascript_sample

.PHONY : CMakeFiles/javascript_sample.dir/build

CMakeFiles/javascript_sample.dir/requires: CMakeFiles/javascript_sample.dir/samples/javascript_sample.cpp.o.requires

.PHONY : CMakeFiles/javascript_sample.dir/requires

CMakeFiles/javascript_sample.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/javascript_sample.dir/cmake_clean.cmake
.PHONY : CMakeFiles/javascript_sample.dir/clean

CMakeFiles/javascript_sample.dir/depend:
	cd /Users/xaxxon/v8toolkit/Debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/xaxxon/v8toolkit /Users/xaxxon/v8toolkit /Users/xaxxon/v8toolkit/Debug /Users/xaxxon/v8toolkit/Debug /Users/xaxxon/v8toolkit/Debug/CMakeFiles/javascript_sample.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/javascript_sample.dir/depend
