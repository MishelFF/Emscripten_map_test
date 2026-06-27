#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <emscripten.h>
#include "gpc.h"

struct fontSprite { 
    unsigned int font,code,color,center_color;
};

struct PointD { double x; double y; };
struct WellDevRec  { double dn; double dw;double nw;double ng;int hourWorkInput;int hourWorkOutput;};

struct WellInfo { 
    std::string NC,clust;
   
    PointD coord,bot; 
    std::vector<fontSprite> sprites;
    std::vector<std::string> labels;
};

typedef std::vector<PointD> PathD;
typedef std::vector< PathD > PathsD;
typedef std::vector<std::vector<int>> PathType;
typedef std::vector<WellInfo> WellsType;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
    PathsD paths,conts,iso_polygons,iso_paths;
    PathType pathtypes,conttypes,isotypes,isopathtypes;
    PointD startPoint;
    WellsType wells;

bool is_digits(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    
    // Find the first non-whitespace character
    const auto start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return ""; // The string is completely empty or all whitespace
    }

    // Find the last non-whitespace character
    const auto end = str.find_last_not_of(whitespace);
    
    // Extract and return the substring
    return str.substr(start, end - start + 1);
}
int loadWellsFile(WellsType& wells,const char* filename) {
    std::ifstream file(filename);
    std::string line;
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return 1; // Return error code
    }
    WellInfo current_well;
    std::string buf;
    while (std::getline(file,line)){
        std::stringstream ss(line);
        fontSprite current_sprite;
        std::getline(ss, current_well.NC, ';');
        std::getline(ss, current_well.clust, ';');
        std::getline(ss, buf, ';');current_well.coord.x=std::stod(buf);
        std::getline(ss, buf, ';');current_well.coord.y=std::stod(buf);
        std::getline(ss, buf, ';');current_well.bot.x=std::stod(buf);
        std::getline(ss, buf, ';');current_well.bot.y=std::stod(buf);
        std::getline(ss, buf, ';');current_sprite.code=std::stoi(buf);
        std::getline(ss, buf, ';');current_sprite.color=std::stoi(buf);
        std::getline(ss, buf, ';');current_sprite.center_color=std::stoi(buf);
        current_well.sprites.push_back(current_sprite);
        for (int i=0;i<3;i++) {
            std::getline(ss, buf, ';');current_well.labels.push_back(buf);
        }
        WellDevRec current_dev;
        std::getline(ss, buf, ';');current_dev.dn=std::stod(buf);
        std::getline(ss, buf, ';');current_dev.dw=std::stod(buf);
        std::getline(ss, buf, ';');current_dev.nw=std::stod(buf);
        std::getline(ss, buf, ';');current_dev.ng=std::stod(buf);
        wells.push_back(current_well);
    }
    file.close();
    return 0; // Success 
}

int loadPaths(PathsD& paths, PathType& pathtypes,const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return 1; // Return error code
    }

    PathD current_path;
    double x, y;
    int n=0;
    std::string line;
    std::vector<int> t;
    while (std::getline(file,line)) if (!line.empty()){
        std::cerr << line << std::endl;
        size_t pos = 0;size_t start=0;
        pos = line.find_first_of(",;");start=pos+1;
        n=std::stoi(line.substr(0, pos));
        t.clear();
        current_path.clear();
        while (pos< (line.size()-1)) {
            pos = line.find_first_of(",;",start);
            if (pos==std::string::npos) {pos=line.size()-1;}
            std::string subs=trim(line.substr(start, pos-start));
            //std::cerr << subs << std::endl;
            if (is_digits(subs)) {
                t.push_back(std::stoi(subs));
            }
            start=pos+1;
        }
        std::cerr<<"n= " << n <<" type= " << t[0] << std::endl;
        for(int i=0;i<n;i++){
                file >> x >> y;
                PointD pt;pt.x=x;pt.y=y;
                current_path.push_back(pt);
//               std::cerr << "x= "<<x<< "  y="<<y<<std::endl;
        }
        file.ignore(); 
        if (!current_path.empty()) {
                paths.push_back(current_path);
                pathtypes.push_back(t);
        }
    }
    file.close();
    return 0; // Success 
}
void toScreen(const PointD& dot,int *x,int *y) {
    *x=(int)((dot.y-startPoint.y)/10+100);
    *y=(int)(2000-(dot.x-startPoint.x)/10);
} 
void drawPath(const PathsD& paths,const PathType& pathtypes) {
    std::vector<SDL_Point> vertices;
    std::vector<SDL_Vertex> strips;
    for (int i = 0; i < paths.size(); i++) {
        vertices.clear();
        for (int j = 0; j < paths[i].size(); j++) {
            
//            float vy=2000-(paths[i][j].x-startPoint.x)/10;
//            float vx=(paths[i][j].y-startPoint.y)/10+100;
//            vx=(vx>0)?vx:0;
//            vy=(vy>0)?vy:0;
            int vx,vy;toScreen(paths[i][j],&vx,&vy);
            SDL_Point dot={(int)vx,(int) vy};
            vertices.push_back(dot);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 50, 255);
        if (pathtypes[i][0]<2) { SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);}
        if ((pathtypes[i][0]>1)&&(pathtypes[i][0]<4)) { SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);}
        if ((pathtypes[i][0]==11)) { SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);}
        if ((pathtypes[i][0]==10)) { SDL_SetRenderDrawColor(renderer, 255, 128, 0, 255);}
        if ((pathtypes[i][0]>999)) { SDL_SetRenderDrawColor(renderer, 150, 150, 0, 150);}
        if ((pathtypes[i][0]>999)||(pathtypes[i][0]==11)||(pathtypes[i][0]==10)||(pathtypes[i][0]<7)) { 
            SDL_RenderDrawLines(renderer,  vertices.data(), vertices.size());
        }
    }    
}    
void fillPath(const PathsD& paths,const PathType& pathtypes) {
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    for (int i = 0; i < paths.size(); i++) {
        vertices.clear();
        indices.clear();
        for (int j = 0; j < paths[i].size(); j++) {
//            float vy=2000-(paths[i][j].x-startPoint.x)/10;
//            float vx=(paths[i][j].y-startPoint.y)/10+100;
//            vx=(vx>0)?vx:0;
//            vy=(vy>0)?vy:0;
            int vx,vy;toScreen(paths[i][j],&vx,&vy);
            SDL_Vertex dot={{(float)vx, float(vy)}, {(unsigned char)(((pathtypes[i][0]))*20 % 255),210 ,0 ,255},   {0.0f, 0.0f}};
            vertices.push_back(dot);
            if (j>1){indices.push_back(j-2);indices.push_back(j-1);indices.push_back(j);}
            
        }
        SDL_RenderGeometry(renderer, NULL, vertices.data(), vertices.size(), indices.data(), indices.size());
//        vertices.clear();
//        vertices.push_back({{0.0f+100*i, 0.0f}, {(unsigned char)(i*20), 0, 0, 255},   {0.0f, 0.0f}});
//        vertices.push_back({{500.0f+100*i, 000.0f}, {(unsigned char)(i*20), 0, 0, 255},   {0.0f, 0.0f}});
//        vertices.push_back({{250.0f+1000*i, 500.0f}, {(unsigned char)(i*20), 0, 0, 255},   {0.0f, 0.0f}});
//        SDL_RenderGeometry(renderer, NULL, vertices.data(), vertices.size(), NULL, 0);
    }

}
void DrawFilledCircle(SDL_Renderer* renderer, int centreX, int centreY, int radius) {
    int x = radius - 1;
    int y = 0;
    int tx = 1;
    int ty = 1;
    int error = tx - (radius << 1);

    while (x >= y) {
        // Draw horizontal lines across matching symmetrical pairs
        SDL_RenderDrawLine(renderer, centreX - x, centreY - y, centreX + x, centreY - y);
        SDL_RenderDrawLine(renderer, centreX - x, centreY + y, centreX + x, centreY + y);
        SDL_RenderDrawLine(renderer, centreX - y, centreY - x, centreX + y, centreY - x);
        SDL_RenderDrawLine(renderer, centreX - y, centreY + x, centreX + y, centreY + x);

        if (error <= 0) {
            y++;
            error += ty;
            ty += 2;
        }
        if (error > 0) {
            x--;
            tx += 2;
            error += tx - (radius << 1);
        }
    }
}
void renderText(const std::string& message, int x, int y, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer) {
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended (font, message.c_str(), color);
    if (!textSurface) {
        std::cerr << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_Rect destRect = { x, y, textWidth, textHeight };
    SDL_RenderCopy(renderer, textTexture, nullptr, &destRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}
void drawWells(const WellsType& wells) {
    TTF_Font* font = TTF_OpenFont("arial.ttf", 12);
    for (int i = 0; i < wells.size(); i++) {
        int vx,vy;toScreen(wells[i].coord,&vx,&vy);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        DrawFilledCircle(renderer, vx, vy, 4);
        renderText(wells[i].NC, vx+5, vy-18, font, {0, 0, 0, 255}, renderer);
    }  
    TTF_CloseFont(font);
}       
void main_loop() {
//    std::vector<SDL_Vertex> vertices;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    fillPath(iso_polygons,isotypes);
    drawPath(paths,pathtypes);
    drawPath(conts,conttypes);
    drawWells(wells);
    SDL_RenderPresent(renderer);

 }    
int main(int argc, char** argv)
{
    gpc_polygon aPolygon;
    gpc_vertex_list aVertexList;
    gpc_tristrip aTriStrip;
    std::vector<SDL_Point> vertices;

    if (loadPaths(iso_paths, isopathtypes,"/IsoPolygons.txt")) return 1; // Error 
    if (loadPaths(paths, pathtypes,"/Isolines.txt")) return 1; // Error 
    if (loadPaths(conts, conttypes,"/Conturs.txt")) return 1; // Error 
    if (loadWellsFile(wells,"/Wells.txt")) return 1; // Error 
    for (int i = 0; i < pathtypes.size(); i++) {pathtypes[i][0]=1000;}
  //  iso_polygons.clear();
  //  for (int i = 0; i < paths.size(); i++) {
  //      PathD current_path;
  //      Triangulate(paths[i], 0, current_path, true);
  //      iso_polygons.push_back(current_path);
  //  }
    std::cerr << "Input types "<< isopathtypes.size() << std::endl;
    for (int i = 0; i < isopathtypes.size(); i++) {
        std::cerr << "Types "<< i<<"Nums "<<isopathtypes[i].size()<<":";
        for (int j = 0; j < isopathtypes[i].size(); j++)  std::cerr <<isopathtypes[i][j]<<"  "; 
        std::cerr << std::endl;
    }
    iso_polygons.clear();
    int prev_poly=-1;
    aPolygon.num_contours = 0;
    int i=0;
    while( (i < iso_paths.size()))if (isopathtypes[i][0]<50) {
        prev_poly=isopathtypes[i][0];
        while((i < iso_paths.size())&&(isopathtypes[i][0]==prev_poly)){
            aVertexList.num_vertices = iso_paths[i].size();
            aVertexList.vertex = (gpc_vertex*)iso_paths[i].data();
            gpc_add_contour(&aPolygon, &aVertexList, isopathtypes[i][2]);       
            i++;
        }
        gpc_polygon_to_tristrip(&aPolygon,&aTriStrip);
        gpc_free_polygon(&aPolygon);aPolygon.num_contours = 0;
      
        for (int j = 0; j < aTriStrip.num_strips; j++) {
            PathD current_path;
            for (int k = 0; k < aTriStrip.strip[j].num_vertices; k++) {
                double x=aTriStrip.strip[j].vertex[k].x;double y=aTriStrip.strip[j].vertex[k].y;
                if(startPoint.x==0){startPoint={x,y};}else{startPoint.x=std::min(x,startPoint.x);startPoint.y=std::min(y,startPoint.y);}
                current_path.push_back({x,y});
            }
            if (!current_path.empty()) {iso_polygons.push_back(current_path);isotypes.push_back(isopathtypes[i-1]);}
            
        }
        gpc_free_tristrip(&aTriStrip);
    }
    else i++;
    std::cerr << "Isolines polygons "<< iso_polygons.size() << std::endl;
    for (int i = 0; i < iso_polygons.size(); i++) {
        std::cerr << "Isoline"<< i<<"Polygons"<<iso_polygons[i].size() << std::endl;
        for (int j = 0; j < iso_polygons[i].size(); j++)  std::cerr << iso_polygons[i][j].x<< "   "<<iso_polygons[i][j].y << std::endl;
    }
//    return 0;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_CreateWindowAndRenderer(1640, 2000, 0, &window, &renderer);
    emscripten_set_main_loop(main_loop, 0, 1);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}