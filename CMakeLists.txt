cmake_minimum_required(VERSION 3.21)

project(MapTest LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ОШИБКА 1: Обязательно объявляем путь к шаблону перед его использованием!
# Замените 'minimal_shell.html' на реальное имя вашего файла шаблона
set(HTML_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/minimal_shell.html")

# Список исходных файлов
set(PROJECT_SOURCES
    main.cpp
    gpc.c
    miniz.c
)

add_executable(MapTest ${PROJECT_SOURCES})

# Явно задаем имя выходного файла и суффикс
set_target_properties(MapTest PROPERTIES 
    OUTPUT_NAME "index"
    SUFFIX ".html"
)

# Настройки для Emscripten (SDL2)
target_compile_options(MapTest PRIVATE
    -Wall
    -Wextra
    "-sUSE_SDL=2"
    "-sUSE_SDL_TTF=2"
)

# Формируем путь к папке plast
file(TO_CMAKE_PATH "d:/MF/Em_test/plast/" WINDOWS_SAFE_PLAST_PATH)

target_link_options(MapTest PRIVATE
    "--shell-file=${HTML_SHELL}"
    "-sUSE_SDL=2"
    "-sUSE_SDL_TTF=2"
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sASSERTIONS=1"
    "--embed-file=d:/MF/Em_test/plast@/"
    "-sEXPORTED_RUNTIME_METHODS=[ccall,HEAPU8]"
    "-sEXPORTED_FUNCTIONS=[_process_uploaded_zip,_malloc,_free,_main]"
    "-sASSERTIONS=2"
    "-sSTACK_OVERFLOW_CHECK=2"
    "-sSAFE_HEAP=1"
)


add_custom_command(
    TARGET MapTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/index.html"
        "${CMAKE_BINARY_DIR}/index.js"
        "${CMAKE_BINARY_DIR}/index.wasm"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Copying web build to source directory"
)
