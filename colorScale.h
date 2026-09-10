#pragma once
#include <SDL2/SDL.h>


struct ColorScaleItem  { double value; SDL_Color color; };

SDL_Color GetSDLColor(const int color);

class ColorScale {
public:
    int loadColorScale(const char* filename);
    SDL_Color getDiscreteColor(const double value)const;
    void clear() {m_items.clear();}
    int size(){return m_items.size();}
    void addLevel(ColorScaleItem item){m_items.push_back(item);}
    const ColorScaleItem& operator[](size_t index) const{return m_items[index];}

private:
    std::vector<ColorScaleItem> m_items; 
};