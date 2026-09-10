#include <math.h>
#include <vector>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include "colorScale.h"
#include "wellData.h"
#include "contourData.h"
#include "fieldmodel.h"

struct VertexGeom{
    float position[3];
    float normal[3];
};
struct VertexColor{
    float color[4];
};
struct MeshBuffer {
    std::vector<VertexGeom>  geometry;
    std::vector<VertexColor>  colors;
    std::vector<uint32_t>    indices;

    GLuint vao = 0;
    GLuint vboGeom  = 0;   
    GLuint vboColor  = 0;   
    GLuint ebo = 0;

    size_t indexCount=0;
    size_t lastColorCount=0;
    GLenum drawMode=GL_TRIANGLE_STRIP;

    bool geomDirty=true;  
    bool colorDirty=true;
    bool flipWinding = false;   
};
struct WellMeshBuffer {
    MeshBuffer cylinder;   
    MeshBuffer cone;       
};
struct WellLabelTex {
    GLuint texture = 0;
    int texWidth = 0;
    int texHeight = 0;
};
struct WellDrawInfo {
    glm::vec3 headPos;   
    float length;  
    float radius;        
    glm::vec4 color;
    WellLabelTex labelTex;
    glm::vec4 labelColor{0.0f, 0.0f, 0.0f, 1.0f};
    const WellInfo* wellData;
};
struct LabelInfo{
    GLuint labelShaderProgram=0;
    GLuint billboardVBO=0;
    GLuint billboardVAO=0;
    GLint uLabelViewProjLoc = -1;
    GLint uLabelViewLoc = -1;
    GLint uLabelWorldPosLoc = -1;
    GLint uLabelSizeLoc = -1;
    GLint uLabelColorLoc = -1;
    GLint uLabelTexLoc = -1;
};

struct ContourVertex {
    float pos[3];      
    float arcLength;   
    float offset;      
};

struct ContourRange {
    GLint first;
    GLsizei count;
    glm::vec4 color;
    int style;          // 0=solid, 1=dash, 2=dot
    float halfWidth;
};

class Painter3D {
public:
    ~Painter3D();
    void init();
    void changeScale(double delta) {scale+=delta;scale=std::max(0.001,scale);scale=std::min(100.0,scale);}
    void changeImageOffset(int dx, int dy) {imageOffset.x+=dx;imageOffset.y+=dy;}
    int  loadModel();
    void modelToScene(const float& modelX,const float& modelY,const float& modelZ, float& sceneX, float& sceneY, float& sceneZ);
    void prepareScene();
    void destroyScene();
    void setProjectionMatrix(float fovYDegrees, float aspect, float nearPlane, float farPlane);
    void setCam();
    void draw();
    void mouseMove(int dx,int dy);
    void mouseScale(float dy);
    void resetCamera();

private:
    ContourData contours;
    ContourData isolines;
    WellData wells;
    ColorScale paramColorScale;
    FieldModel m_fieldModel;
    double scale=1.0;
    PointD startPoint;
    Point imageOffset={0,0};
    std::array<MeshBuffer,6> meshes;
    GLuint shaderProgram = 0;
    GLint  uMVPLoc = -1;
    GLint  uModelLoc = -1;
    GLint  uLightDirLoc = -1;
    GLint  uUseUniformColorLoc = -1;
    GLint  uColorLoc = -1;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::vec3 lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    float m_camHorizAngle=0.0;
    float m_camVertAngle=0.0;
    float m_camDistance=5.0;
    WellMeshBuffer wellMesh;
    std::vector<WellDrawInfo> wellUnits;
    LabelInfo labelDrawInfo;
    GLuint lineShaderProgram = 0;
    GLuint contourVAO = 0;
    GLuint contourVBO = 0;
    std::vector<ContourRange> contourRanges;
    bool contourGeomDirty = true;
    GLint uLineMVPLoc = -1;
    GLint uLineColorLoc = -1;
    GLint uLineStyleLoc = -1;
    GLint uLineHalfWidthLoc = -1;
    GLint uLineDashLengthLoc = -1;
    GLint uLineGapLengthLoc = -1;
    GLuint compileShader(GLenum type, const char* source);
    bool initShaders();
    bool initLabelShaders();
    bool initLineShaders();
    void prepareSurfaceMesh(const GrdSurface& surf,const GrdSurface& param, const ColorScale& paramColorScale,bool flipWinding, MeshBuffer& mesh);
    void preparSides(const int side, MeshBuffer& mesh);
    void prepareWellUnitMesh(int segments);
    void prepareContourMesh();

    void uploadMeshBuffers(MeshBuffer& mesh);
    void destroyMeshBuffer(MeshBuffer& mesh);
    void shutdown();
    GLuint createTextTexture(const std::vector<std::string>& lines,TTF_Font* font, int& outW, int& outH);
    GLuint createTextTextureLine(const std::string& text, TTF_Font* font, int& outW, int& outH);
    void setWellLabel(WellDrawInfo& w, TTF_Font* font);
    void initBillboardQuad();
    void drawModel();
    void drawWells();
    void drawWellLabels();
    void drawContours();

};