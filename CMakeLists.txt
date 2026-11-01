cmake_minimum_required(VERSION 3.19)
project(kgrim LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()
qt_add_executable(kgrim kgrim.cpp)
target_link_libraries(kgrim PRIVATE Qt6::Core)

install(TARGETS kgrim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
