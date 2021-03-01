project(MemoryMon)
cmake_minimum_required(VERSION 3.7)

add_executable(MemoryMon MemoryMon.c)
target_compile_definitions(MemoryMon PRIVATE -D_ISOC99_SOURCE)
