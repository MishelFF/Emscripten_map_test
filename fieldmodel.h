#pragma once
#include <string>
#include "point.h"

class GrdSurface{
public:
    GrdSurface(const std::string & fileName);
    int nx()const{ return m_nx;};
    int ny()const{ return m_ny;};
    PointD origin()const{return m_origin;}
    PointD space()const{return m_space;}
    float values(int index)const{return m_values[index];}; 
    void minMaxValues(float& minValue,float& maxValue)const{minValue=m_minValue;maxValue=m_maxValue;}   
    void buildPlane(const float level);
    float getZ(const float x, const float y)const;
private:
    int m_nx,m_ny;
    PointD m_origin;
    PointD m_space;
    float m_minValue;
    float m_maxValue;
    std::vector <float> m_values;
};

class FieldModel {
        
public:
    FieldModel();
    ~FieldModel();
    int loadSurfaces();
    PointD getCenter();
    PointD getSize();
    GrdSurface& top(){return *m_topSurface;};
    GrdSurface& bottom(){return *m_bottomSurface;};
    GrdSurface& parameter(){return *m_parameterSurface;};
private:
    GrdSurface *m_topSurface=nullptr;
    GrdSurface *m_bottomSurface=nullptr;
    GrdSurface *m_parameterSurface=nullptr;
};    