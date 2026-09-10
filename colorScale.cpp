#include <iterator>
#include <fstream>
#include <sstream>
#include <iostream>
#include <SDL2/SDL.h>
#include "colorScale.h"
#include "strFunctions.h"

SDL_Color GetSDLColor(const int color) {
     SDL_Color sdlColor;
        sdlColor.b = (color >> 16) & 0xFF;
        sdlColor.g = (color >> 8) & 0xFF;
        sdlColor.r = color & 0xFF;
        sdlColor.a = 255;
        return sdlColor;
}
int ColorScale::loadColorScale(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return 1; // Return error code
    }
    clear();
    ColorScaleItem current_item;
    int c=0;
    std::string line,buf;
    while (std::getline(file,line )) {
        std::stringstream ss(line);
        std::getline(ss, buf, ';');
        current_item.value=std::stod(buf);
        std::getline(ss, buf, '\n');
        c=std::stoi(buf);
        current_item.color= GetSDLColor(c);
//        current_item.color.b=c>>16;
//        current_item.color.g=(c>>8)&0xff;
//        current_item.color.r=c&0xff;
        addLevel(current_item);
    }    
    file.close();
    return 0;  
}
SDL_Color ColorScale::getDiscreteColor(const double value)const {
    if(m_items.empty())return SDL_Color{255,255,255,255} ;
    if (value < m_items.front().value) return m_items.front().color;
    auto upper = std::upper_bound(m_items.begin(), m_items.end(), value,[](double v, const ColorScaleItem& item) { return v < item.value; });
    return std::prev(upper)->color;
}
