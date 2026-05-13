cmake_minimum_required(VERSION 3.16)
project(StudentExpenseManager VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER   OFF CACHE BOOL "" FORCE)
set(LLAMA_SERVER_BUILD   OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/llama.cpp)

set(SOURCES
    src/main.cpp
    src/expense.cpp
    src/storage.cpp
    src/analytics.cpp
    src/ai_advisor.cpp
)

add_executable(expense_tracker ${SOURCES})

target_include_directories(expense_tracker PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/extern/llama.cpp/include
    ${CMAKE_SOURCE_DIR}/extern/llama.cpp/ggml/include
)

target_link_libraries(expense_tracker PRIVATE llama)

if(TARGET ggml)
    target_link_libraries(expense_tracker PRIVATE ggml)
endif()

if(MSVC)
    target_compile_options(expense_tracker PRIVATE /W3)
else()
    target_compile_options(expense_tracker PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/data)
    file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/reports)
message(STATUS "Build configured. Run: cmake --build . to compile.")

# ── Web server ─────────────────────────────────────────────────────────────────

