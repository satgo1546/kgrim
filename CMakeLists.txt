cmake_minimum_required(VERSION 3.19)
project(kgrim LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core Gui DBus)

qt_standard_project_setup()
qt_add_executable(kgrim kgrim.cpp)
target_link_libraries(kgrim PRIVATE Qt6::Core Qt6::Gui Qt6::DBus)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
endif()
target_compile_definitions(kgrim PRIVATE VERSION="${GIT_VERSION}")

install(TARGETS kgrim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
