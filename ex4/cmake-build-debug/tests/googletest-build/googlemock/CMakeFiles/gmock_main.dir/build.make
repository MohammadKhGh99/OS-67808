# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.19

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /cygdrive/c/Users/m7mdg/AppData/Local/JetBrains/CLion2021.1/cygwin_cmake/bin/cmake.exe

# The command to remove a file.
RM = /cygdrive/c/Users/m7mdg/AppData/Local/JetBrains/CLion2021.1/cygwin_cmake/bin/cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug"

# Include any dependencies generated for this target.
include tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/depend.make

# Include the progress variables for this target.
include tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/progress.make

# Include the compile flags for this target's objects.
include tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/flags.make

tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.o: tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/flags.make
tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.o: tests/googletest-src/googlemock/src/gmock_main.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.o"
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock" && /usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/gmock_main.dir/src/gmock_main.cc.o -c "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-src/googlemock/src/gmock_main.cc"

tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/gmock_main.dir/src/gmock_main.cc.i"
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock" && /usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-src/googlemock/src/gmock_main.cc" > CMakeFiles/gmock_main.dir/src/gmock_main.cc.i

tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/gmock_main.dir/src/gmock_main.cc.s"
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock" && /usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-src/googlemock/src/gmock_main.cc" -o CMakeFiles/gmock_main.dir/src/gmock_main.cc.s

# Object files for target gmock_main
gmock_main_OBJECTS = \
"CMakeFiles/gmock_main.dir/src/gmock_main.cc.o"

# External object files for target gmock_main
gmock_main_EXTERNAL_OBJECTS =

lib/libgmock_maind.a: tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.o
lib/libgmock_maind.a: tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/build.make
lib/libgmock_maind.a: tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../../../lib/libgmock_maind.a"
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock" && $(CMAKE_COMMAND) -P CMakeFiles/gmock_main.dir/cmake_clean_target.cmake
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock" && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gmock_main.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/build: lib/libgmock_maind.a

.PHONY : tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/build

tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/clean:
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock" && $(CMAKE_COMMAND) -P CMakeFiles/gmock_main.dir/cmake_clean.cmake
.PHONY : tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/clean

tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/depend:
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-src/googlemock" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : tests/googletest-build/googlemock/CMakeFiles/gmock_main.dir/depend

