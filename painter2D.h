#include <math.h>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include "colorScale.h"
#include "wellData.h"
#include "contourData.h"

struct PatternElement {
    float start;   
    float end;     
    bool isDot;    
};

struct LineMesh2D{
    std::vector<SDL_Point> vertices;
    SDL_Color color;
};
struct Mesh2D {
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
};

class Painter2D {
public:
    void changeScale(double delta) {scale+=delta;scale=std::max(0.001,scale);scale=std::min(100.0,scale); prepareScene();}
    void changeImageOffset(int dx, int dy) {imageOffset.x+=dx;imageOffset.y+=dy; prepareScene();}
    void toScreen(const PointD& dot,int *x,int *y);
    PointD toScreenD(const PointD& dot);
//    static SDL_Color colorFromType(int type);
    void drawPath(const std::vector<LineMesh2D>& conturs, SDL_Renderer* renderer);
    void fillTriStrip( SDL_Renderer* renderer);
    void drawWells( SDL_Renderer* renderer);
    void drawDashedConturs( SDL_Renderer* renderer);
    void draw(SDL_Renderer* renderer);
    int  loadModel();
    void prepareScene();
    void destroyScene();
    void prepareIsoPolygons();
    void prepareConturs();
    void preparePath(const ContourData& conturs,std::vector<LineMesh2D>& meshes);

private:
    ContourData isopolygons;
    ContourData isolines;
    ContourData contours;
    WellData wells;
    ColorScale colorScale;
    PathsD tristrips;
    std::vector<int> tristripsLevels;
    double scale=1.0;
    PointD startPoint;
    Point imageOffset={0,0};
    std::vector<Mesh2D> isopolygonMeshes;
    std::vector<Mesh2D> contourMeshes;
    std::vector<LineMesh2D> isolineMeshes;

};