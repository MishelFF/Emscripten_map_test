#include "gpc.h"
#include "point.h"
#include "painter2D.h"
#include <SDL2/SDL_ttf.h>

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
void renderText(const std::string& message, int x, int y, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer,Point& textSize ) {
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
    textSize.x=textWidth;textSize.y=textHeight;
    SDL_Rect destRect = { x, y, textWidth, textHeight };
    SDL_RenderCopy(renderer, textTexture, nullptr, &destRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void Painter2D::toScreen(const PointD& dot,int *x,int *y) {
    PointD result=toScreenD(dot);
    *x=static_cast<int>(result.x);
    *y=static_cast<int>(result.y);
} 
PointD Painter2D::toScreenD(const PointD& dot) {
    return {((dot.y-startPoint.y)*scale/10+100)+imageOffset.x,(1000-(dot.x-startPoint.x)*scale/10)+imageOffset.y};
} 

/*SDL_Color Painter2D::colorFromType(int type){
    SDL_Color result={0, 0, 0, 255};
    if (type<2) { result={0, 0, 200, 255};}
    if (type>1&&type<4) { result={255, 0, 0, 255};}
    if (type==11) { result={255, 0, 0, 255};}
    if (type==10) { result={255, 128, 0, 255};}
    if (type>999) { result={150, 150, 0, 150};}
    return result;
}*/
void Painter2D::preparePath(const ContourData& conturs,std::vector<LineMesh2D>& meshes) {
    std::cerr << "Prepare contur start conturs=" <<conturs.size()<< std::endl;
    for (int i = 0; i < conturs.size(); i++) {
        if ((conturs[i].type>999)||(conturs[i].type==11)||(conturs[i].type==10)||(conturs[i].type<7)) { 
            LineMesh2D mesh;
            mesh.vertices.reserve(conturs[i].size());
            for (int j = 0; j < conturs[i].size(); j++) {
                int vx,vy;toScreen(conturs[i][j],&vx,&vy);
                mesh.vertices.push_back({vx,vy});
            }
            mesh.color=conturs.getColorByType(conturs[i].type);
            meshes.push_back(std::move(mesh));
        }
    }    
}    
void Painter2D::drawPath(const std::vector<LineMesh2D>& conturs, SDL_Renderer* renderer) {
    for (auto contur : conturs){
        SDL_SetRenderDrawColor(renderer, contur.color.r, contur.color.g, contur.color.b, contur.color.a);
        SDL_RenderDrawLines(renderer,  contur.vertices.data(), contur.vertices.size());
    }
}

float periodForStyle(int style, float dashLength, float gapLength, float dotRadius)
{
    float dotSpan = dotRadius * 2.0f + gapLength;
    switch (style) {
        case 1: return dashLength + gapLength;                 // dash
        case 2: return dotSpan;                                // dot
        case 3: return dashLength + gapLength + dotSpan;       // dash-dot
        case 4: return dashLength + gapLength + dotSpan * 2.0f; // dash-dot-dot
        default: return dashLength + gapLength;
    }
}

std::vector<PatternElement> patternElementsForStyle(
    int style, float dashLength, float gapLength, float dotRadius)
{
    std::vector<PatternElement> elems;

    if (style == 1) {
        elems.push_back({0.0f, dashLength, false});

    } else if (style == 2) {
        float center = dotRadius; 
        elems.push_back({center - dotRadius, center + dotRadius, true});
    } else if (style == 3) {
        elems.push_back({0.0f, dashLength, false});
        float dotCenter = dashLength + gapLength + dotRadius;
        elems.push_back({dotCenter - dotRadius, dotCenter + dotRadius, true});
    } else if (style == 4) {
        elems.push_back({0.0f, dashLength, false});
        float dot1Center = dashLength + gapLength + dotRadius;
        elems.push_back({dot1Center - dotRadius, dot1Center + dotRadius, true});
        float dot2Center = dot1Center + dotRadius + gapLength + dotRadius;
        elems.push_back({dot2Center - dotRadius, dot2Center + dotRadius, true});
    }
    return elems;
}

PointD lerp(const PointD& a, const PointD& b, float t)
{
    return PointD{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
}

PointD normalize(const PointD& v)
{
    double len = std::sqrt(v.x*v.x+v.y*v.y);
    if (len < 1e-9) { return PointD{0.0, 0.0};}
    return PointD{v.x/len, v.y/len};
}

std::vector<PatternElement> visibleSubSegments(
    float rangeStart, float rangeEnd, int style,
    float dashLength, float gapLength, float dotRadius)
{
    std::vector<PatternElement> result;

    if (style == 0) {
        result.push_back({rangeStart, rangeEnd, false});
        return result;
    }

    float period = periodForStyle(style, dashLength, gapLength, dotRadius);
    std::vector<PatternElement> localElems = patternElementsForStyle(style, dashLength, gapLength, dotRadius);
    int kStart = static_cast<int>(std::floor(rangeStart / period));
    int kEnd   = static_cast<int>(std::floor(rangeEnd / period));
    for (int k = kStart; k <= kEnd; ++k) {
        float periodOffset = k * period;
        for (const PatternElement& elem : localElems) {
            float absStart = periodOffset + elem.start;
            float absEnd   = periodOffset + elem.end;
            float clippedStart = std::max(absStart, rangeStart);
            float clippedEnd   = std::min(absEnd, rangeEnd);
            if (clippedEnd > clippedStart) {
                result.push_back({clippedStart, clippedEnd, elem.isDot});
            }
        }
    }
    return result;
}
void appendQuad(std::vector<SDL_Vertex>& verts, std::vector<int>& indices,
                             PointD p0, PointD p1, float thickness, SDL_Color color) 
{
    PointD dir = normalize({p1.x - p0.x,p1.y - p0.y});
    PointD perp{-dir.y * thickness * 0.5f, dir.x * thickness * 0.5f};
    int base = static_cast<int>(verts.size());
    verts.push_back({static_cast<float>(p0.x - perp.x), static_cast<float>(p0.y - perp.y), color, {0.0f, 0.0f}});
    verts.push_back({static_cast<float>(p0.x + perp.x), static_cast<float>(p0.y + perp.y), color, {0.0f, 0.0f}});
    verts.push_back({static_cast<float>(p1.x - perp.x), static_cast<float>(p1.y - perp.y), color, {0.0f, 0.0f}});
    verts.push_back({static_cast<float>(p1.x + perp.x), static_cast<float>(p1.y + perp.y), color, {0.0f, 0.0f}});
    indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
    indices.push_back(base + 2); indices.push_back(base + 1); indices.push_back(base + 3);
}

void appendDot(std::vector<SDL_Vertex>& verts, std::vector<int>& indices,
                            PointD center, float radius, SDL_Color color, int segments = 8)
{
    int centerIdx = static_cast<int>(verts.size());
    verts.push_back({static_cast<float>(center.x), static_cast<float>(center.y), color, {0.0f, 0.0f}});
    for (int i = 0; i <= segments; ++i) {
        float theta = (float)i / (float)segments * 2.0f * static_cast<float>(M_PI);
        verts.push_back({static_cast<float>(center.x + cosf(theta) * radius), static_cast<float>(center.y + sinf(theta) * radius), color, {0.0f, 0.0f}});
    }
    for (int i = 0; i < segments; ++i) {
        indices.push_back(centerIdx);
        indices.push_back(centerIdx + 1 + i);
        indices.push_back(centerIdx + 2 + i);
    }
}
void Painter2D::prepareConturs(){
    for (int k=0;k<contours.size();++k){
        int type=contours[k].getType();
        if(type>12&&type<990)continue;
        Mesh2D mesh;
        int style=contours.getStyleByType(type);
        float width=4.0*contours.getWidthByType(type);
        SDL_Color color=contours.getColorByType(type);
        float dashLength=10;
        float gapLength=0.4*dashLength;
        float dotRadius=width/2.0f;
        mesh.vertices.clear();
        mesh.indices.clear();
        if (contours[k].size() < 2) continue;
        float traveled = 0.0f;
        for (size_t i = 0; i + 1 < contours[k].size(); ++i) {
            PointD a=toScreenD(contours[k][i]);
            PointD b=toScreenD(contours[k][i+1]);
            float segLen = std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));
            if (segLen < 1e-6f) continue;
            auto subSegments = visibleSubSegments(traveled, traveled + segLen, style,dashLength, gapLength, dotRadius);
            for (const PatternElement& elem : subSegments) {
                float t0 = (elem.start - traveled) / segLen;
                float t1 = (elem.end - traveled) / segLen;
                PointD p0 = lerp(a, b, t0);
                PointD p1 = lerp(a, b, t1);
                if (elem.isDot) {
                    PointD center = lerp(a, b, (t0 + t1) * 0.5f);
                    appendDot(mesh.vertices, mesh.indices, center, dotRadius, color);
                } else {
                    appendQuad(mesh.vertices, mesh.indices, p0, p1, width, color);
                }
            }
            traveled += segLen;
        }
        contourMeshes.push_back(std::move(mesh));
    }
}

void Painter2D::drawDashedConturs( SDL_Renderer* renderer)
{   
    for (auto mesh:contourMeshes){
        if (mesh.vertices.empty()) return;
        SDL_RenderGeometry(renderer, nullptr,mesh.vertices.data(),static_cast<int>(mesh.vertices.size()),mesh.indices.data(),static_cast<int>(mesh.indices.size()));
    }
}


void Painter2D::prepareIsoPolygons(){
    for (int i = 0; i < tristrips.size(); i++) {
        Mesh2D mesh;
        mesh.vertices.clear();
        mesh.vertices.reserve(tristrips[i].size());
        mesh.indices.clear();
        mesh.indices.resize((tristrips[i]).size()*3);
        int level=tristripsLevels[i];
        if (level>(colorScale.size()-1)) {level=colorScale.size()-1;}
        for (int j = 0; j < tristrips[i].size(); j++) {
            int vx,vy;toScreen(tristrips[i][j],&vx,&vy);
            SDL_Vertex dot={{(float)vx, float(vy)}, colorScale[level].color,   {0.0f, 0.0f}};
            mesh.vertices.push_back(dot);
            if (j>1){mesh.indices.push_back(j-2);mesh.indices.push_back(j-1);mesh.indices.push_back(j);}
        }
        isopolygonMeshes.push_back(std::move(mesh));
    }
}
void  Painter2D::fillTriStrip( SDL_Renderer* renderer) {
    for (auto mesh:isopolygonMeshes){    
        SDL_RenderGeometry(renderer, NULL, mesh.vertices.data(), mesh.vertices.size(), mesh.indices.data(), mesh.indices.size());
    }
}

void Painter2D::drawWells( SDL_Renderer* renderer) {
    TTF_Font* font = TTF_OpenFont("arial.ttf", 12);
    TTF_Font* spritesfont = TTF_OpenFont("FWELLS__.ttf", 30);
    Point textSize;
    for (int i = 0; i < wells.size(); i++) {
        int vx,vy;toScreen(wells[i].coord,&vx,&vy);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
//        DrawFilledCircle(renderer, vx, vy, 4);
        char buf[2]=" ";
        for (int j = wells[i].sprites.size()-1; j >=0 ; j--){ 
            buf[0]=wells[i].sprites[j].code;
            renderText(buf, vx-22, vy-15, spritesfont,  GetSDLColor(wells[i].sprites[j].color), renderer,textSize);
        }
        renderText(wells[i].NC, vx+5, vy-18, font, {0, 0, 0, 255}, renderer,textSize);
        if (wells[i].labels.size()>0) {
            SDL_RenderDrawLine(renderer, vx+15, vy, vx+5+textSize.x, vy);
            for (int j = 0; j < wells[i].labels.size(); j++) renderText(wells[i].labels[j], vx+5, vy+textSize.y*j, font, {0, 0, 0, 255}, renderer,textSize);
        } 
    }
    TTF_CloseFont(spritesfont);
    TTF_CloseFont(font);
}   
void Painter2D::prepareScene(){
    //if (isopolygonMeshes.size()!=0 || contourMeshes.size()!=0 || isolineMeshes.size() !=0 ) 
    destroyScene();
    prepareIsoPolygons();
    preparePath(isolines, isolineMeshes);
    prepareConturs();
}
void Painter2D::destroyScene(){
    isopolygonMeshes.clear();
    contourMeshes.clear();
    isolineMeshes.clear();
}
void Painter2D::draw(SDL_Renderer* renderer){
    fillTriStrip(renderer);
    drawPath(isolineMeshes,renderer);
//    drawPath(contours,renderer);
    drawDashedConturs(renderer);
    drawWells(renderer);
}

int  Painter2D::loadModel()
{
    if (isopolygons.loadConturs("/IsoPolygons.txt")) return 1; // Error 
    if (isolines.loadConturs("/Isolines.txt")) return 1; // Error 
    if (contours.loadConturs("/Conturs.txt")) return 1; // Error 
    if (wells.loadWellsFile("/Wells.txt")) return 1; // Error 
    if (colorScale.loadColorScale("/ColorScale.txt")) return 1; // Error 
    for (int i = 0; i < isolines.size(); i++) {isolines[i].setType(1000);}
    gpc_polygon aPolygon={};
    gpc_vertex_list aVertexList={};
    gpc_tristrip aTriStrip={};
    std::vector<SDL_Point> vertices;
/*     std::cerr << "Input types "<< isopathtypes.size() << std::endl;
   for (int i = 0; i < isopathtypes.size(); i++) {
        std::cerr << "Types "<< i<<"Nums "<<isopathtypes[i].size()<<":";
        for (int j = 0; j < isopathtypes[i].size(); j++)  std::cerr <<isopathtypes[i][j]<<"  "; 
        std::cerr << std::endl;
    }*/
    tristrips.clear();
    tristripsLevels.clear();
    startPoint={0,0};
    int prev_poly=-1;
    aPolygon.num_contours = 0;
    int i=0;
    while( (i < isopolygons.size()))if (isopolygons[i].getType()<50) {
        prev_poly=isopolygons[i].getType();
        while((i < isopolygons.size())&&(isopolygons[i].getType()==prev_poly)){
            aVertexList.num_vertices = isopolygons[i].size();
            aVertexList.vertex = (gpc_vertex*)isopolygons[i].data();
            gpc_add_contour(&aPolygon, &aVertexList, isopolygons[i].hole);       
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
            if (!current_path.empty()) {tristrips.push_back(current_path);tristripsLevels.push_back(isopolygons[i-1].getType());}
            
        }
        gpc_free_tristrip(&aTriStrip);
    }
    else i++;
    std::cerr << "Isolines polygons "<< isopolygons.size() << std::endl;
/*    for (int i = 0; i < iso_polygons.size(); i++) {
        std::cerr << "Isoline"<< i<<"Polygons"<<iso_polygons[i].size() << std::endl;
        for (int j = 0; j < iso_polygons[i].size(); j++)  std::cerr << iso_polygons[i][j].x<< "   "<<iso_polygons[i][j].y << std::endl;
    }*/
    std::cerr << "Model Loaded " << std::endl;
    return 0;
 
}
