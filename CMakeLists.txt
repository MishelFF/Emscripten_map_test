cmake_minimum_required(VERSION 3.21)

project(MapTest LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git  GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

set(HTML_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/minimal_shell.html")

set(PROJECT_SOURCES
    colorScale.cpp  
    contourData.cpp
    wellData.cpp
    painter2D.cpp
    painter3D.cpp
    strFunctions.cpp
    fieldmodel.cpp
    main.cpp
    gpc.c
    miniz.c
)

add_executable(MapTest ${PROJECT_SOURCES})

set_target_properties(MapTest PROPERTIES 
    OUTPUT_NAME "index"
    SUFFIX ".html"
)

# Настройки для Emscripten (SDL2)
target_compile_options(MapTest PRIVATE
    -Wall
    -Wextra
    "-sUSE_SDL=2"
    "-sUSE_WEBGL2=1"
    "-sUSE_SDL_TTF=2"
)

file(TO_CMAKE_PATH "d:/MF/Em_test/plast/" WINDOWS_SAFE_PLAST_PATH)

target_link_options(MapTest PRIVATE
    "--shell-file=${HTML_SHELL}"
    "-sUSE_SDL=2"
    "-sUSE_WEBGL2=1"
    "-sFULL_ES3=1"
    "-sUSE_SDL_TTF=2"
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sASSERTIONS=1"
    "--embed-file=d:/MF/Em_test/plast@/"
    "-sEXPORTED_RUNTIME_METHODS=[ccall,HEAPU8]"
    "-sEXPORTED_FUNCTIONS=[_process_uploaded_zip,_process_change_model,_malloc,_free,_main]"
    "-sASSERTIONS=2"
    "-sSTACK_OVERFLOW_CHECK=2"
    "-sSAFE_HEAP=1"
)
target_link_libraries(MapTest PRIVATE glm::glm)

add_custom_command(
    TARGET MapTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/index.html"
        "${CMAKE_BINARY_DIR}/index.js"
        "${CMAKE_BINARY_DIR}/index.wasm"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Copying web build to source directory"
)
