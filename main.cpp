#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <emscripten.h>
#include "gpc.h"
#include "miniz.h"


#include <iostream>
#include <emscripten/emscripten.h>
#include "miniz.h"
#include "fieldmodel.h"
#include "point.h"
#include "painter2D.h"
#include "painter3D.h"

bool g_FilesLoaded = false;
extern "C" {

EMSCRIPTEN_KEEPALIVE
void process_uploaded_zip(uint8_t *zip_buffer, int zip_size)
{
    std::cout << "Архив получен. Размер: " << zip_size << " байт. Начинаем распаковку..." << std::endl;

    // Инициализируем архив из буфера в памяти
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_mem(&zip_archive, zip_buffer, zip_size, 0))
    {
        std::cerr << "Ошибка: Не удалось прочитать ZIP-архив из памяти!" << std::endl;
        return;
    }

    // Получаем количество файлов в архиве
    mz_uint num_files = mz_zip_reader_get_num_files(&zip_archive);
    std::cout << "Файлов в архиве: " << num_files << std::endl;

    // Перебираем каждый файл в архиве
    for (mz_uint i = 0; i < num_files; i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            std::cerr << "Не удалось получить информацию о файле под индексом " << i << std::endl;
            continue;
        }

        // Пропускаем директории
        if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
        {
            std::cout << "Директория: " << file_stat.m_filename << " (пропускаем или создаем при необходимости)" << std::endl;
            continue;
        }

        std::cout << "Распаковка: " << file_stat.m_filename << " (" << file_stat.m_uncomp_size << " байт)" << std::endl;

       
        size_t uncompressed_size;
        void *pUncomp_data = mz_zip_reader_extract_to_heap(&zip_archive, i, &uncompressed_size, 0);

        if (!pUncomp_data)
        {
            std::cerr << "Ошибка распаковки файла: " << file_stat.m_filename << std::endl;
            continue;
        }

        // Записываем файл в виртуальную файловую систему Emscripten (MEMFS)
        // Теперь этот файл будет доступен для fopen() по его имени
        std::ofstream out_file(file_stat.m_filename, std::ios::binary);
        if (out_file.is_open())
        {
            out_file.write(reinterpret_cast<const char *>(pUncomp_data), uncompressed_size);
            out_file.close();
            std::cout << "Успешно сохранен в виртуальную ФС: " << file_stat.m_filename << std::endl;
        }
        else
        {
            std::cerr << "Не удалось сохранить файл на виртуальный диск!" << std::endl;
        }

        // Освобождаем память кучи miniz, выделенную под этот конкретный файл
        mz_free(pUncomp_data);
    }

    mz_zip_reader_end(&zip_archive);
    std::cout << "Распаковка завершена! Все файлы теперь доступны в среде WASM." << std::endl;
    g_FilesLoaded=true;
}


}

struct MouseState { 
    Point pos,click_pos,last_pos; 
    bool leftButtonDown; 
    bool rightButtonDown; 
};

enum class AppMode { Mode2D, Mode3D, ModeNone };
AppMode currentMode = AppMode::Mode2D;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext glContext = nullptr;

    MouseState mouseState;  
    std::unique_ptr<Painter2D> painter2D;
    std::unique_ptr<Painter3D> painter3D;
void move_image(int dx,int dy) {
    if (currentMode==AppMode ::Mode3D){painter3D->mouseMove(dx,dy);}
    else {painter2D->changeImageOffset(dx,dy);}
}
void scale_image(int dx,int dy) {
    if (currentMode==AppMode ::Mode3D){
        painter3D->mouseScale(dy/100.0);
    }
    else painter2D->changeScale(dy/100.0);
}
void reset_image() {
    if (currentMode==AppMode ::Mode3D){painter3D->resetCamera();}
}

void poll_mouse() {
     SDL_Event event;
    // Poll all pending events for the current frame
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            // 1. Mouse Movement
            case SDL_MOUSEMOTION:
                mouseState.pos.x = event.motion.x;
                mouseState.pos.y = event.motion.y;
                if (mouseState.leftButtonDown) {
                    move_image(event.motion.x-mouseState.last_pos.x,event.motion.y-mouseState.last_pos.y);
                    mouseState.last_pos = {event.motion.x, event.motion.y};
                }
                if (mouseState.rightButtonDown) {
                    scale_image(event.motion.x-mouseState.last_pos.x,event.motion.y-mouseState.last_pos.y);
                    mouseState.last_pos = {event.motion.x, event.motion.y};
                }
                break;

            // 2. Mouse Click Press
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouseState.leftButtonDown = true;
                }
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    mouseState.rightButtonDown = true;
                }
                mouseState.click_pos = {event.button.x, event.button.y};
                mouseState.last_pos = {event.button.x, event.button.y};
                break;

            // 3. Mouse Click Release
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouseState.leftButtonDown = false;
                    move_image(event.button.x-mouseState.last_pos.x,event.button.y-mouseState.last_pos.y);
                }
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    mouseState.rightButtonDown = false;
                    scale_image(event.button.x-mouseState.last_pos.x,event.button.y-mouseState.last_pos.y);
                }
                break;

            // 4. Mouse Wheel / Scroll
            case SDL_MOUSEWHEEL:
                scale_image(0,event.wheel.y*5);
                break;
        }
    }
}

void main_loop() {
    poll_mouse();    
    if (currentMode == AppMode::Mode2D) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        painter2D->draw(renderer);
        SDL_RenderPresent(renderer);
    }
    else if(currentMode == AppMode::Mode3D) painter3D->draw();
    if(g_FilesLoaded) {
        std::cerr << "Start load model " << std::endl;
        if (currentMode == AppMode::Mode2D) painter2D->loadModel();
        else painter3D->loadModel();       
        g_FilesLoaded = false; 
    }
 }   

void initMode2D()
{
    window = SDL_CreateWindow("App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1800, 1800, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    painter2D=std::make_unique<Painter2D>();
    painter2D->loadModel();
    painter2D->prepareScene();
}
void doneMode2D()
{
    painter2D.reset();
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window)   { SDL_DestroyWindow(window); window = nullptr; }
}

void initMode3D()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    window = SDL_CreateWindow("App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,1800, 1800, SDL_WINDOW_OPENGL);
    glContext = SDL_GL_CreateContext(window);
    if (glContext == NULL) {
        std::cerr << "Error create 3D context: " << SDL_GetError() << std::endl;
        return;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    painter3D = std::make_unique<Painter3D>();
    painter3D->init();
    painter3D->loadModel();
    painter3D->prepareScene();
    painter3D->setProjectionMatrix(45.0f, 1800.0f / 1800.0f, 0.1f, 100.0f);
    painter3D->setCam();
}

void doneMode3D()
{
    painter3D.reset();
    if (glContext) { SDL_GL_DeleteContext(glContext); glContext = nullptr; }
    if (window)    { SDL_DestroyWindow(window); window = nullptr; }
}

extern "C" {
EMSCRIPTEN_KEEPALIVE
void process_change_model(){
    if (currentMode == AppMode::Mode2D)
    {
        currentMode = AppMode::ModeNone;
        doneMode2D();
        EM_ASM({
            resetCanvasElement();
        });        
        std::cerr << "2D Mode done " << std::endl;
        initMode3D();
        std::cerr << "3D Mode init " << std::endl;
        currentMode = AppMode::Mode3D;
    }
    else if (currentMode == AppMode::Mode3D){
        currentMode = AppMode::ModeNone;
        doneMode3D();
        EM_ASM({
            resetCanvasElement();
        });
        std::cerr << "3D Mode done " << std::endl;
        initMode2D();
        std::cerr << "2D Mode init " << std::endl;
        currentMode = AppMode::Mode2D;
    }
    else{
        initMode2D();
        currentMode = AppMode::Mode2D;
    }
}

}

int main(int argc, char** argv)
{
  
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    currentMode = AppMode::Mode3D;
    currentMode = AppMode::Mode2D;
    if (currentMode == AppMode::Mode2D) initMode2D();
    else initMode3D();
    std::cerr << "Set Main Loop " << std::endl;
    SDL_version version;
    SDL_GetVersion(&version);

    std::cerr <<"SDL "<<version.major<<"."<<version.minor<<" ."<<version.patch<< std::endl;
    emscripten_set_main_loop(main_loop, 0, 1);
    if (currentMode == AppMode::Mode2D) doneMode2D();
    else doneMode3D();

    TTF_Quit();
    SDL_Quit();
    return 0;
}

