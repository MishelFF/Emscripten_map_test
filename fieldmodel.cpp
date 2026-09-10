#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "fieldmodel.h" 

GrdSurface::GrdSurface(const std::string & fileName){
    if(fileName.empty())return;
    std::ifstream grdInput(fileName);
    if (!grdInput.is_open()) return;
    std::string line;
    std::getline(grdInput, line);
    if(line.compare(0,4,"DSAA")!=0)return;
    grdInput >> m_ny>>m_nx;
    if (m_nx==0||m_ny==0)return;
    if(grdInput.fail()){m_nx=0;m_ny=0;return;}
    double x1,x2,y1,y2;
    grdInput >> y1 >> y2; std::getline(grdInput, line);
    grdInput >> x1 >> x2; std::getline(grdInput, line);
    std::getline(grdInput, line);
    m_origin=PointD{x1,y1}; 
    m_space=PointD{(x2-x1)/(m_nx-1), (y2-y1)/(m_ny-1)};
    m_values.clear();
    m_values.resize(m_nx*m_ny);
    bool flag=true;
    unsigned int index=0;
    while (std::getline(grdInput, line)){  
        std::istringstream grdLine(line);
        float val=0;
        while (grdLine >> val) {
            if(val==9.8e+5)  val=0; 
            int i=index%m_ny;
            int j=index/m_ny;
            int pos=i*m_nx+j;
            m_values[pos]=val;
            if (flag){m_minValue=m_maxValue=val;flag=false;}else{if(val<m_minValue)m_minValue=val;if(val>m_maxValue)m_maxValue=val;}
            ++index;
        }               
    }
    if ((m_values.size()!=m_nx*m_ny)) std::cerr << "Values " << m_values.size() << " nx*ny= " << m_nx*m_ny<< std::endl;
}
 
void GrdSurface::buildPlane(float level){
    int index=0;
    for(int i=0;i<m_ny;i++)
        for(int j=0;j<m_nx;j++) {m_values[index++]=level;}
}

float GrdSurface::getZ( const float x,const float y) const
{
    double fi = (x - m_origin.x) / m_space.x;
    double fj = (y - m_origin.y) / m_space.y;
    if (fi < 0.0) fi=0.0;if (fi > static_cast<float>(m_nx - 1)) fi=static_cast<float>(m_nx - 1);
    if (fj < 0.0) fj=0.0;if (fj > static_cast<float>(m_ny - 1)) fj=static_cast<float>(m_ny - 1);
    int i0 = static_cast<int>(std::floor(fi));
    int j0 = static_cast<int>(std::floor(fj));
    int i1 = std::min(i0 + 1, m_nx - 1);
    int j1 = std::min(j0 + 1, m_ny - 1);
    double tx = fi - i0; 
    double ty = fj - j0; 
    float v00 = values(i0 + j0 * m_nx);
    float v10 = values(i1 + j0 * m_nx);
    float v01 = values(i0 + j1 * m_nx);
    float v11 = values(i1 + j1 * m_nx);
    float vTop    = static_cast<float>(v00 + (v10 - v00) * tx);
    float vBottom = static_cast<float>(v01 + (v11 - v01) * tx);
    float result  = static_cast<float>(vTop + (vBottom - vTop) * ty);
    return result;
}

FieldModel::FieldModel(){

 }
FieldModel::~FieldModel(){
    delete m_topSurface;
    delete m_bottomSurface;
    delete m_parameterSurface;
 }

 int FieldModel::loadSurfaces(){
    m_topSurface=new GrdSurface("/top.grd");
    m_bottomSurface=new GrdSurface("/bottom.grd");
    m_parameterSurface=new GrdSurface("/parameter.grd");
    return 0;
}
PointD FieldModel::getCenter(){
    if(m_topSurface==nullptr){return PointD{0,0};};
    PointD size=getSize();
    return PointD{m_topSurface->origin().x+size.x/2,m_topSurface->origin().y+size.y/2};
}

PointD FieldModel::getSize(){
    if(m_topSurface==nullptr){return PointD{0,0};};
    return PointD{m_topSurface->space().x*m_topSurface->nx(),m_topSurface->space().y*m_topSurface->ny()};
}
