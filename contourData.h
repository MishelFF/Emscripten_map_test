#pragma once
#include <emscripten.h>
#include <SDL2/SDL.h>
#include <vector>
#include <stdio.h>
#include "point.h"

typedef std::vector<PointD> PathD;
typedef std::vector<PathD> PathsD;

struct Contour {
    int type;
    int hole=0;
    PathD path;
public:
    void clear(){path.clear();}
    bool empty(){return path.empty();}
    int size() const { return path.size();}
    void addPoint(const PointD& point){path.push_back(point);}
    int getType(){return type;};
    void setType(int t){type=t;};
    PointD* data(){return path.data();}   
    const PointD& operator[](size_t index) const{return path[index];}
};

class ContourData{
    std::vector<Contour> conturs;
public:
    int size() const { return conturs.size();}
    int loadConturs(const char* filename);
    const Contour& operator[](size_t index) const{return conturs[index];}
    Contour& operator[](size_t index) {return conturs[index];}
    static SDL_Color getColorByType(const int type);
    static int getStyleByType(const int type);
    static float getWidthByType(const int type);
};