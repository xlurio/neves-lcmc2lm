project(neves-lcmc2lm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libxmlpp REQUIRED)
find_package(Threads REQUIRED)

add_executable(mcc2lm src/main.cpp)

target_include_directories(mcc2lm include)

target_link_libraries(mcc2lm libxmlpp::libxmlpp Threads::Threads)
