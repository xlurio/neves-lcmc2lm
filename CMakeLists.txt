cmake_minimum_required(VERSION 3.31)

project(neves-lcmc2lm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libxmlpp REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

add_executable(
    mcc2lm
    src/main.cpp
    src/schema.cpp
    src/process.cpp
    src/radical.cpp
    src/radical_utf8.cpp
    src/radical_parse.cpp
    src/radical_load.cpp
    src/radical_load_radicals.cpp
    src/radical_load_fallback.cpp
    src/radical_api.cpp
    src/radical_load_radicals.cpp
    src/radical_load_fallback.cpp
    include/mcc2lm/ngram.hpp)

target_include_directories(mcc2lm PUBLIC include)

target_link_libraries(mcc2lm libxmlpp::libxmlpp SQLite::SQLite3 Threads::Threads)
