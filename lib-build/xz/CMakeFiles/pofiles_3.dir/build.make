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

# Utility rule file for pofiles_3.

# Include any custom commands dependencies for this target.
include CMakeFiles/pofiles_3.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/pofiles_3.dir/progress.make

CMakeFiles/pofiles_3: da.gmo

da.gmo: /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/po/da.po
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating da.gmo"
	cd /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz && /opt/homebrew/bin/msgfmt -o /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/da.gmo /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/po/da.po

pofiles_3: CMakeFiles/pofiles_3
pofiles_3: da.gmo
pofiles_3: CMakeFiles/pofiles_3.dir/build.make
.PHONY : pofiles_3

# Rule to build all files generated by this target.
CMakeFiles/pofiles_3.dir/build: pofiles_3
.PHONY : CMakeFiles/pofiles_3.dir/build

CMakeFiles/pofiles_3.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/pofiles_3.dir/cmake_clean.cmake
.PHONY : CMakeFiles/pofiles_3.dir/clean

CMakeFiles/pofiles_3.dir/depend:
	cd /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles/pofiles_3.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/pofiles_3.dir/depend

