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
include CMakeFiles/TestVirtualMemory.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/TestVirtualMemory.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/TestVirtualMemory.dir/flags.make

CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.o: CMakeFiles/TestVirtualMemory.dir/flags.make
CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.o: ../VirtualMemory.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.o"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.o -c "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/VirtualMemory.cpp"

CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.i"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/VirtualMemory.cpp" > CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.i

CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.s"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/VirtualMemory.cpp" -o CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.s

CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.o: CMakeFiles/TestVirtualMemory.dir/flags.make
CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.o: ../PhysicalMemory.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.o"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.o -c "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/PhysicalMemory.cpp"

CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.i"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/PhysicalMemory.cpp" > CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.i

CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.s"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/PhysicalMemory.cpp" -o CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.s

# Object files for target TestVirtualMemory
TestVirtualMemory_OBJECTS = \
"CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.o" \
"CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.o"

# External object files for target TestVirtualMemory
TestVirtualMemory_EXTERNAL_OBJECTS =

libTestVirtualMemory.a: CMakeFiles/TestVirtualMemory.dir/VirtualMemory.cpp.o
libTestVirtualMemory.a: CMakeFiles/TestVirtualMemory.dir/PhysicalMemory.cpp.o
libTestVirtualMemory.a: CMakeFiles/TestVirtualMemory.dir/build.make
libTestVirtualMemory.a: CMakeFiles/TestVirtualMemory.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libTestVirtualMemory.a"
	$(CMAKE_COMMAND) -P CMakeFiles/TestVirtualMemory.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/TestVirtualMemory.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/TestVirtualMemory.dir/build: libTestVirtualMemory.a

.PHONY : CMakeFiles/TestVirtualMemory.dir/build

CMakeFiles/TestVirtualMemory.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/TestVirtualMemory.dir/cmake_clean.cmake
.PHONY : CMakeFiles/TestVirtualMemory.dir/clean

CMakeFiles/TestVirtualMemory.dir/depend:
	cd "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug" "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/CMakeFiles/TestVirtualMemory.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/TestVirtualMemory.dir/depend

