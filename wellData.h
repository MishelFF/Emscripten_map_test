#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <emscripten.h>
#include "point.h"


struct fontSprite { 
    unsigned int font,code,color,center_color;
};

struct WellDevRec  { double dn; double dw;double nw;double ng;int hourWorkInput;int hourWorkOutput;};

struct WellInfo { 
    std::string NC,clust;
    PointD coord,bot; 
    std::vector<fontSprite> sprites;
    std::vector<std::string> labels;
    WellDevRec current_dev;
    int NCColor;
    int typeRef;
    int stateRef;
};
class WellData{
public:
    int loadWellsFile(const char* filename);
    std::size_t size(){return wells.size();};
    const WellInfo& operator[](size_t index) const{return wells[index];}
    WellInfo& operator[](size_t index) {return wells[index];}
    auto begin() const { return wells.begin(); }
    auto end() const { return wells.end(); }
private:
    std::vector<WellInfo> wells;

};