cmake_minimum_required(VERSION 3.21)

project(MapTest LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Список исходных файлов
set(PROJECT_SOURCES
    hw.cpp
    gpc.c
)

add_executable(MapTest ${PROJECT_SOURCES})
set_target_properties(MapTest PROPERTIES SUFFIX ".html")

# Настройки для Emscripten (SDL2 + Embed Files)
    target_compile_options(MapTest PRIVATE
        -Wall
        -Wextra
        "-sUSE_SDL=2"
        "-sUSE_SDL_TTF=2"
    )

    # Формируем путь к папке plast относительно текущего файла CMakeLists.txt
    file(TO_CMAKE_PATH "d:/MF/Em_test/plast/" WINDOWS_SAFE_PLAST_PATH)
    target_link_options(MapTest PRIVATE
        "-sUSE_SDL=2"
        "-sUSE_SDL_TTF=2"
        "-sWASM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sASSERTIONS=1"
        # Магия упаковки: берем папку RESOURCES_DIR и монтируем её контент в корень /
        "--embed-file d:\\MF\\Em_test\\plast@/"
    )
