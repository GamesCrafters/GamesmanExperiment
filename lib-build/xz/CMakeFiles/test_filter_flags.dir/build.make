# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

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
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.30.4/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.30.4/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz

# Include any dependencies generated for this target.
include CMakeFiles/test_filter_flags.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/test_filter_flags.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/test_filter_flags.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_filter_flags.dir/flags.make

CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o: CMakeFiles/test_filter_flags.dir/flags.make
CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o: /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/tests/test_filter_flags.c
CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o: CMakeFiles/test_filter_flags.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o -MF CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o.d -o CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o -c /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/tests/test_filter_flags.c

CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/tests/test_filter_flags.c > CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.i

CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/tests/test_filter_flags.c -o CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.s

# Object files for target test_filter_flags
test_filter_flags_OBJECTS = \
"CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o"

# External object files for target test_filter_flags
test_filter_flags_EXTERNAL_OBJECTS =

tests_bin/test_filter_flags: CMakeFiles/test_filter_flags.dir/tests/test_filter_flags.c.o
tests_bin/test_filter_flags: CMakeFiles/test_filter_flags.dir/build.make
tests_bin/test_filter_flags: liblzma.a
tests_bin/test_filter_flags: CMakeFiles/test_filter_flags.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable tests_bin/test_filter_flags"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_filter_flags.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_filter_flags.dir/build: tests_bin/test_filter_flags
.PHONY : CMakeFiles/test_filter_flags.dir/build

CMakeFiles/test_filter_flags.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_filter_flags.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_filter_flags.dir/clean

CMakeFiles/test_filter_flags.dir/depend:
	cd /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles/test_filter_flags.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/test_filter_flags.dir/depend

