cmake_minimum_required(VERSION 3.15)
project(CG50Wordle C)

include(GenerateG3A)
include(Fxconv)
find_package(Gint 2.1 REQUIRED)

add_executable(wordle src/main.c)
target_compile_options(wordle PRIVATE -Wall -Wextra -Os)
target_link_libraries(wordle Gint::Gint)

generate_g3a(
    TARGET wordle
    OUTPUT "WORDLE.g3a"
    NAME "WORDLE"
    VERSION "01.01.0000"
    ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png
)
