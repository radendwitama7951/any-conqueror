cmake_minimum_required(VERSION 3.31.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

find_program(MOLD_PATH mold)
if(MOLD_PATH)
  set(CMAKE_LINKER_TYPE $<IF:$<PLATFORM_ID:Android>,LLD,MOLD>)
  message(STATUS "[CMAKE] Set linker to mold")
else()
  set(CMAKE_LINKER_TYPE LLD)
endif()

project("any-conqueror")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/app/")
