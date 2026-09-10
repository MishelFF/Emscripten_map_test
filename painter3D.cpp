#include <SDL2/SDL_ttf.h>
#include "painter3D.h"

static const char* kVertexShaderSrc = R"(#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform bool uUseUniformColor;
uniform vec4 uColor;

out vec3 vNormal;
out vec4 vColor;

void main() {
    vNormal = mat3(uModel) * aNormal;
    vColor = uUseUniformColor ? uColor : aColor;
    gl_Position = uMVP * vec4(aPosition, 1.0);
}
)";
static const char* kFragmentShaderSrc = R"(#version 300 es
precision mediump float;

in vec3 vNormal;
in vec4 vColor;

uniform vec3 uLightDir; 

out vec4 fragColor;

void main() {
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    float ambient = 0.3;
    float lighting = ambient + diff * 0.7;

    fragColor = vec4(vColor.rgb * lighting, vColor.a);
}
)";
static const char* kLabelVertexShaderSrc = R"(#version 300 es
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 uViewProj;
uniform mat4 uView;
uniform vec3 uWorldPos;
uniform vec2 uSize;

out vec2 vUV;

void main() {
    // right/up камеры — из столбцов view-матрицы, чтобы квад был перпендикулярен направлению взгляда
    vec3 right = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 up    = vec3(uView[0][1], uView[1][1], uView[2][1]);

    vec3 worldOffset = uWorldPos + right * aPos.x * uSize.x + up * aPos.y * uSize.y;
    gl_Position = uViewProj * vec4(worldOffset, 1.0);
    vUV = aUV;
}
)";

static const char* kLabelFragmentShaderSrc = R"(#version 300 es
precision mediump float;

in vec2 vUV;
uniform sampler2D uTex;
uniform vec4 uTextColor;

out vec4 fragColor;

void main() {
    vec4 texel = texture(uTex, vUV);
    fragColor = vec4(uTextColor.rgb, texel.a * uTextColor.a);
}
)";

static const char* kLineVertexShaderSrc = R"(#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in float aArcLength;
layout(location = 2) in float aOffset;

uniform mat4 uMVP;

out float vArcLength;
out float vOffset;

void main() {
    vArcLength = aArcLength;
    vOffset = aOffset;
    gl_Position = uMVP * vec4(aPosition, 1.0);
}
)";

static const char* kLineFragmentShaderSrc = R"(#version 300 es
precision mediump float;

in float vArcLength;
in float vOffset;

uniform vec4 uColor;
uniform int uStyle;
uniform float uHalfWidth;
uniform float uDashLength;
uniform float uGapLength;

out vec4 fragColor;

bool insideDot(float local, float dotCenter) {
    float du = local - dotCenter;
    float dv = vOffset;
    return sqrt(du * du + dv * dv) <= uHalfWidth;
}

float periodForStyle(int style) {
    float dotSpan = uHalfWidth * 2.0 + uGapLength;
    if (style == 1) return uDashLength + uGapLength;                    // dash
    if (style == 2) return dotSpan;                                     // dot
    if (style == 3) return uDashLength + uGapLength + dotSpan;          // dash-dot
    if (style == 4) return uDashLength + uGapLength + dotSpan * 2.0;    // dash-dot-dot
    return uDashLength + uGapLength;
}

void main() {
    if (uStyle == 0) {
        fragColor = uColor;
        return;
    }

    float period = periodForStyle(uStyle);
    float local = mod(vArcLength, period);

    if (uStyle == 1) {
        if (local > uDashLength) discard;

    } else if (uStyle == 2) {
        float center = floor(vArcLength / period + 0.5) * period;
        if (!insideDot(vArcLength, center)) discard;

    } else if (uStyle == 3) {
        float dotCenter = uDashLength + uGapLength + uHalfWidth;
        bool inDash = local <= uDashLength;
        bool inDot  = insideDot(local, dotCenter);
        if (!(inDash || inDot)) discard;

    } else if (uStyle == 4) {
        float dot1Center = uDashLength + uGapLength + uHalfWidth;
        float dot2Center = dot1Center + uHalfWidth * 2.0 + uGapLength;
        bool inDash = local <= uDashLength;
        bool inDot1 = insideDot(local, dot1Center);
        bool inDot2 = insideDot(local, dot2Center);
        if (!(inDash || inDot1 || inDot2)) discard;
    }

    fragColor = uColor;
}
)";
constexpr float kDashLength = 0.015f; // длина штриха
constexpr float kGapLength  = 0.005f; // длина разрыва

Painter3D::~Painter3D()
{
    shutdown();
}

void Painter3D::init()
{
    initShaders();
    initLabelShaders();
    initLineShaders();
    initBillboardQuad();  
}
GLuint Painter3D::compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        SDL_Log("Shader compile error (%s): %s", type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}  

bool Painter3D::initShaders()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);
    if (vs == 0 || fs == 0) {return false;}
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    GLint linked = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
        SDL_Log("Shader link error: %s", log);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (shaderProgram != 0) {
        uMVPLoc = glGetUniformLocation(shaderProgram, "uMVP");
        uModelLoc = glGetUniformLocation(shaderProgram, "uModel");
        uLightDirLoc = glGetUniformLocation(shaderProgram, "uLightDir");
        uUseUniformColorLoc = glGetUniformLocation(shaderProgram, "uUseUniformColor");
        uColorLoc = glGetUniformLocation(shaderProgram, "uColor");
    }
    return shaderProgram != 0;
}

bool Painter3D::initLabelShaders()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, kLabelVertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kLabelFragmentShaderSrc);
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    labelDrawInfo.labelShaderProgram = glCreateProgram();
    glAttachShader(labelDrawInfo.labelShaderProgram, vs);
    glAttachShader(labelDrawInfo.labelShaderProgram, fs);
    glLinkProgram(labelDrawInfo.labelShaderProgram);

    GLint linked = 0;
    glGetProgramiv(labelDrawInfo.labelShaderProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(labelDrawInfo.labelShaderProgram, sizeof(log), nullptr, log);
        SDL_Log("Label shader link error: %s", log);
        glDeleteProgram(labelDrawInfo.labelShaderProgram);
        labelDrawInfo.labelShaderProgram = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (labelDrawInfo.labelShaderProgram != 0) {
        labelDrawInfo.uLabelViewProjLoc     = glGetUniformLocation(labelDrawInfo.labelShaderProgram, "uViewProj");
        labelDrawInfo.uLabelViewLoc         = glGetUniformLocation(labelDrawInfo.labelShaderProgram, "uView");
        labelDrawInfo.uLabelWorldPosLoc     = glGetUniformLocation(labelDrawInfo.labelShaderProgram, "uWorldPos");
        labelDrawInfo.uLabelSizeLoc         = glGetUniformLocation(labelDrawInfo.labelShaderProgram, "uSize");
//        uLabelCameraPosLoc    = glGetUniformLocation(labelShaderProgram, "uCameraPos");
//        uLabelFovYLoc         = glGetUniformLocation(labelShaderProgram, "uFovY");
//        uLabelScreenHeightLoc = glGetUniformLocation(labelShaderProgram, "uScreenHeight");
//        uLabelPixelSizeLoc    = glGetUniformLocation(labelShaderProgram, "uPixelSize");
        labelDrawInfo.uLabelColorLoc        = glGetUniformLocation(labelDrawInfo.labelShaderProgram, "uTextColor");
        labelDrawInfo.uLabelTexLoc          = glGetUniformLocation(labelDrawInfo.labelShaderProgram, "uTex");
        std::cerr << "Labels shaders prepared" << std::endl;
    }

    return labelDrawInfo.labelShaderProgram != 0;
}
bool Painter3D::initLineShaders()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, kLineVertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kLineFragmentShaderSrc);
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }
    lineShaderProgram = glCreateProgram();
    glAttachShader(lineShaderProgram, vs);
    glAttachShader(lineShaderProgram, fs);
    glLinkProgram(lineShaderProgram);
    GLint linked = 0;
    glGetProgramiv(lineShaderProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(lineShaderProgram, sizeof(log), nullptr, log);
        SDL_Log("Line shader link error: %s", log);
        glDeleteProgram(lineShaderProgram);
        lineShaderProgram = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (lineShaderProgram != 0) {
        uLineMVPLoc = glGetUniformLocation(lineShaderProgram, "uMVP");
        uLineColorLoc = glGetUniformLocation(lineShaderProgram, "uColor");
        uLineStyleLoc = glGetUniformLocation(lineShaderProgram, "uStyle");
        uLineHalfWidthLoc = glGetUniformLocation(lineShaderProgram, "uHalfWidth");
        uLineDashLengthLoc = glGetUniformLocation(lineShaderProgram, "uDashLength");
        uLineGapLengthLoc = glGetUniformLocation(lineShaderProgram, "uGapLength");
   }
    return lineShaderProgram != 0;
}

struct BillboardVertex {
    float pos[2];  
    float uv[2];
};
void Painter3D::initBillboardQuad()
{
    BillboardVertex verts[4] = {
        {{-0.5f, -0.5f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {1.0f, 0.0f}},
    };
    glGenVertexArrays(1, &(labelDrawInfo.billboardVAO));
    glGenBuffers(1, &(labelDrawInfo.billboardVBO));
    glBindVertexArray(labelDrawInfo.billboardVAO);
    glBindBuffer(GL_ARRAY_BUFFER, labelDrawInfo.billboardVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BillboardVertex),reinterpret_cast<void*>(offsetof(BillboardVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BillboardVertex),reinterpret_cast<void*>(offsetof(BillboardVertex, uv)));
    glBindVertexArray(0);
}

int  Painter3D::loadModel(){
        m_fieldModel.loadSurfaces();
        if (wells.loadWellsFile("/Wells.txt")) return 1; 
        if (paramColorScale.loadColorScale("/ColorScale.txt")) return 1;
        if (contours.loadConturs("/Conturs.txt")) return 1;
        if (isolines.loadConturs("/Isolines.txt")) return 1;
        return 0;        
}

void Painter3D::modelToScene(const float& modelX,const float& modelY,const float& modelZ, float& sceneX, float& sceneY, float& sceneZ){
    PointD planeCenter=m_fieldModel.getCenter();
    PointD planeSize=m_fieldModel.getSize();
    float sz=std::max(planeSize.x,planeSize.y);
    sz=std::max(sz,1.0f);
    float vMin,vMax;m_fieldModel.top().minMaxValues(vMin,vMax);
    sceneX=(modelX-planeCenter.x)*2/sz;
    sceneY=(modelZ-(vMin+vMax)/2)*0.5/(vMax-vMin);
    sceneZ=(modelY-planeCenter.y)*2/sz;
}

void Painter3D::prepareSurfaceMesh(const GrdSurface& surf,const GrdSurface& param, const ColorScale& paramColorScale,bool flipWinding, MeshBuffer& mesh){
    int nx=surf.nx();
    int ny=surf.ny();
    PointD origin=surf.origin();
    PointD space=surf.space();
    int surfSize=nx*ny;
    std::vector<std::array<float, 3>> scenePos(surfSize);
    int index=0;
    for (int j=0; j<ny;++j){
        float y=origin.y+j*space.y;
        for (int i=0; i<nx;++i){
            float sceneX,sceneY,sceneZ;
            modelToScene(origin.x+i*space.x,y,surf.values(index),sceneX,sceneY,sceneZ);
            scenePos[index]={sceneX,sceneY,sceneZ};
            ++index;
        }
    }
    mesh.geometry.clear();
    mesh.geometry.resize(surfSize);
    mesh.colors.clear();
    mesh.colors.resize(surfSize);
    auto at = [nx](int i, int j) { return i + j * nx; };
    for (int j=0; j<ny;++j){
        for (int i=0; i<nx;++i){
            int index = at(i, j);
            const auto& p = scenePos[index];
            std::array<float, 3> pXm = scenePos[at(i > 0 ? i - 1 : i, j)];
            std::array<float, 3> pXp = scenePos[at(i < nx - 1 ? i + 1 : i, j)];
            std::array<float, 3> dX = {pXp[0] - pXm[0], pXp[1] - pXm[1], pXp[2] - pXm[2]};
            std::array<float, 3> pYm = scenePos[at(i, j > 0 ? j - 1 : j)];
            std::array<float, 3> pYp = scenePos[at(i, j < ny - 1 ? j + 1 : j)];
            std::array<float, 3> dY = {pYp[0] - pYm[0], pYp[1] - pYm[1], pYp[2] - pYm[2]};
            float sign = flipWinding ? -1.0f : 1.0f;
            float nx_ = sign*(dX[1] * dY[2] - dX[2] * dY[1]);
            float ny_ = sign*(dX[2] * dY[0] - dX[0] * dY[2]);
            float nz_ = sign*(dX[0] * dY[1] - dX[1] * dY[0]);
            float len = std::sqrt(nx_ * nx_ + ny_ * ny_ + nz_ * nz_);
            if (len > 1e-8f) { nx_ /= len; ny_ /= len; nz_ /= len; }
            else { nx_ = 0.0f; ny_ = 0.0f; nz_ = 1.0f; } 

            mesh.geometry[index] = VertexGeom{ {p[0], p[1], p[2]},{nx_, ny_, nz_}};
            SDL_Color color = paramColorScale.getDiscreteColor(param.values(index));
            mesh.colors[index] = VertexColor{{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}};
        }
    }

    mesh.indices.clear();
    mesh.indices.reserve(static_cast<size_t>(ny - 1) * (nx * 2 + 2));
    for (int j = 0; j < ny - 1; ++j) {
        for (int i = 0; i < nx; ++i) {
                mesh.indices.push_back(at(i, j));
                mesh.indices.push_back(at(i, j + 1));
        }
        if (j < ny - 2) {
                mesh.indices.push_back(at(nx - 1, j + 1));
                mesh.indices.push_back(at(0, j + 1));
        }
    } 
    mesh.indexCount = mesh.indices.size();
    mesh.flipWinding = flipWinding;
    mesh.drawMode = GL_TRIANGLE_STRIP;
    mesh.geomDirty = true;
    mesh.colorDirty = true;
}
void Painter3D::preparSides(const int side, MeshBuffer& mesh){
    int nx=m_fieldModel.top().nx();
    int ny=m_fieldModel.top().ny();
    int n;
    if (side==0||side==2)n=nx; else n=ny;
    mesh.geometry.clear();
    mesh.geometry.resize(2*n);
    mesh.colors.clear();
    mesh.colors.resize(2*n);
    int index=0;
    for (int i=0;i<n;i++){
        for (int j=0;j<2;j++){
            GrdSurface* surf;
            if(j==0)surf=&m_fieldModel.top();
            else surf=&m_fieldModel.bottom();
            PointD origin=surf->origin();
            PointD space=surf->space();
            float x,y,z,normx,normz;
            switch (side){
                case 0:x=origin.x+i*space.x;y=origin.y;z=surf->values(i);normx=0;normz=-1;break;
                case 1:x=origin.x;y=origin.y+i*space.y;z=surf->values(i*nx);normx=-1;normz=0;break;
                case 2:x=origin.x+i*space.x;y=origin.y+(ny-1)*space.y;z=surf->values(i+(ny-1)*nx);normx=0;normz=1;break;
                case 3:x=origin.x+(nx-1)*space.x;y=origin.y+i*space.y;z=surf->values(i*nx+nx-1);normx=1;normz=0;break;
            }

            float sceneX,sceneY,sceneZ;
            modelToScene(x,y,z,sceneX,sceneY,sceneZ);
            mesh.geometry[index] = VertexGeom{ {sceneX,sceneY,sceneZ},{normx,0,normz}};
            mesh.colors[index] = VertexColor{{0.5f, 0.5f, 0.0f,1.0f}};
            index++; 
        }
    }
    mesh.indices.clear();
    mesh.indices.reserve(n*2);
    for(int i=0;i<n;i++){
        mesh.indices.push_back(i*2);
        mesh.indices.push_back(i*2+1);
    }
    mesh.indexCount = mesh.indices.size();
    switch(side){
        case 0:mesh.flipWinding = true;break;
        case 1:mesh.flipWinding = false;break;
        case 2:mesh.flipWinding = false;break;
        case 3:mesh.flipWinding = true;break;
    }
    mesh.drawMode = GL_TRIANGLE_STRIP;
    mesh.geomDirty = true;
    mesh.colorDirty = true;
}

void Painter3D::prepareWellUnitMesh(int segments)
{
    MeshBuffer& cyl = wellMesh.cylinder;
    cyl.geometry.clear();
    cyl.geometry.reserve(segments * 2);
    constexpr float cylRadius = 0.3f; 
    for (int i = 0; i < segments; ++i) {
        float angle = (float)i / (float)segments * glm::two_pi<float>();
        float cx = cylRadius*cosf(angle);
        float cz = cylRadius*sinf(angle);
        cyl.geometry.push_back(VertexGeom{{cx, 0.0f, cz},{cx, 0.0f, cz}});
        cyl.geometry.push_back(VertexGeom{{cx, -1.0f, cz},{cx, 0.0f, cz}});
    }
    cyl.indices.clear();
    cyl.indices.reserve(segments * 2 + 2);
    for (int i = 0; i <= segments; ++i) {
        int index = (i % segments) * 2;
        cyl.indices.push_back(index);       
        cyl.indices.push_back(index + 1);   
    }
    cyl.indexCount = cyl.indices.size();
    cyl.drawMode = GL_TRIANGLE_STRIP;
    cyl.flipWinding = true; 
    cyl.colors.assign(cyl.geometry.size(), VertexColor{{1,1,1,1}});

    MeshBuffer& cone = wellMesh.cone;
    cone.geometry.clear();
    cone.geometry.reserve(segments + 1);
    constexpr float coneLength = 0.07f; 
    for (int i = 0; i < segments; ++i) {
        float angle = (float)i / (float)segments * glm::two_pi<float>();
        float cx = cosf(angle);
        float cz = sinf(angle);
        glm::vec3 n = glm::normalize(glm::vec3(cx, 1.0f * (1.0f / coneLength) * 0.3f, cz));
        cone.geometry.push_back(VertexGeom{{cx, 0.0f, cz},{n.x, n.y, n.z}});
    }
    cone.geometry.push_back(VertexGeom{{0.0f, 0.0f + coneLength, 0.0f},{0.0f, -1.0f, 0.0f}});
    cone.geometry.push_back(VertexGeom{{0.0f, 0.0f, 0.0f},{0.0f, -1.0f, 0.0f}});
    int apexIndex = segments;
    cone.indices.clear();
    cone.indices.reserve(segments * 6);
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        cone.indices.push_back(next);
        cone.indices.push_back(i);
        cone.indices.push_back(apexIndex);
        cone.indices.push_back(i);
        cone.indices.push_back(next);
        cone.indices.push_back(apexIndex+1);
    }
    cone.indexCount = cone.indices.size();
    cone.drawMode = GL_TRIANGLES;
    cone.flipWinding = false; 
    cone.colors.assign(cone.geometry.size(), VertexColor{{1,1,1,1}});
    cyl.geomDirty = true;
    cyl.colorDirty = true;
    cone.geomDirty = true;
    cone.colorDirty = true;
}
void Painter3D::uploadMeshBuffers(MeshBuffer& mesh)
{
    if (mesh.geometry.empty() || mesh.indices.empty()) {return;}
    const bool firstUpload = (mesh.vao == 0);

    if (firstUpload) {
        glGenVertexArrays(1, &mesh.vao);
        glGenBuffers(1, &mesh.vboGeom);
        glGenBuffers(1, &mesh.vboColor);
        glGenBuffers(1, &mesh.ebo);
    }
    glBindVertexArray(mesh.vao);
    if (mesh.geomDirty) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vboGeom);
        glBufferData(GL_ARRAY_BUFFER, mesh.geometry.size() * sizeof(VertexGeom),mesh.geometry.data(),GL_STATIC_DRAW);

        // layout(location = 0) vec3 position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,sizeof(VertexGeom),reinterpret_cast<void*>(offsetof(VertexGeom, position)));
        // layout(location = 1) vec3 normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,sizeof(VertexGeom),reinterpret_cast<void*>(offsetof(VertexGeom, normal)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size() * sizeof(uint32_t),mesh.indices.data(), GL_STATIC_DRAW);
        mesh.indexCount = mesh.indices.size();
        mesh.geomDirty = false;
        // mesh.geometry.clear(); mesh.geometry.shrink_to_fit();
        // mesh.indices.clear();  mesh.indices.shrink_to_fit();
    }

    if (mesh.colorDirty) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vboColor);

        if (firstUpload) {
            glBufferData(GL_ARRAY_BUFFER, mesh.colors.size() * sizeof(VertexColor), mesh.colors.data(), GL_DYNAMIC_DRAW);
            // layout(location = 2) vec4 color
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexColor), nullptr);
        } else if (mesh.colors.size() == mesh.lastColorCount) {
            glBufferSubData(GL_ARRAY_BUFFER, 0,mesh.colors.size() * sizeof(VertexColor), mesh.colors.data());
        } else {
            glBufferData(GL_ARRAY_BUFFER, mesh.colors.size() * sizeof(VertexColor), mesh.colors.data(), GL_DYNAMIC_DRAW);
        }
        mesh.lastColorCount = mesh.colors.size();
        mesh.colorDirty = false;
    }

    glBindVertexArray(0);
}
GLuint Painter3D::createTextTexture(const std::vector<std::string>& lines,TTF_Font* font, int& outW, int& outH)
{
    if (lines.empty()) {return 0;}

    std::vector<SDL_Surface*> lineSurfaces;
    lineSurfaces.reserve(lines.size());
    SDL_Color white{255, 255, 255, 255};
    int maxWidth = 0;
    int totalHeight = 0;

    for (const std::string& line : lines) {
        const char* text = line.empty() ? " " : line.c_str();
        SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, white);
        if (!surf) {
            SDL_Log("TTF_RenderUTF8_Blended failed for line '%s': %s", line.c_str(), TTF_GetError());
            for (SDL_Surface* s : lineSurfaces) SDL_FreeSurface(s);
            return 0;
        }
        maxWidth = std::max(maxWidth, surf->w);
        totalHeight += surf->h;
        lineSurfaces.push_back(surf);
    }
    SDL_Surface* combined = SDL_CreateRGBSurfaceWithFormat(0, maxWidth, totalHeight, 32, SDL_PIXELFORMAT_RGBA32);
    if (!combined) {
        SDL_Log("SDL_CreateRGBSurfaceWithFormat failed: %s", SDL_GetError());
        for (SDL_Surface* s : lineSurfaces) SDL_FreeSurface(s);
        return 0;
    }
    SDL_FillRect(combined, nullptr, SDL_MapRGBA(combined->format, 0, 0, 0, 0)); 
    int yOffset = 0;
    for (SDL_Surface* lineSurf : lineSurfaces) {
        SDL_Rect dst{0, yOffset, lineSurf->w, lineSurf->h};
        SDL_SetSurfaceBlendMode(lineSurf, SDL_BLENDMODE_NONE);
        SDL_BlitSurface(lineSurf, nullptr, combined, &dst);
        yOffset += lineSurf->h;
        SDL_FreeSurface(lineSurf);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, combined->w, combined->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, combined->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    outW = combined->w;
    outH = combined->h;
    SDL_FreeSurface(combined);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
GLuint Painter3D::createTextTextureLine(const std::string& text, TTF_Font* font, int& outW, int& outH)
{
    SDL_Color white{255, 255, 255, 255};
    SDL_Surface* rendered = TTF_RenderUTF8_Blended(font, text.c_str(), white);
    if (!rendered) {
        SDL_Log("TTF_RenderUTF8_Blended failed: %s", TTF_GetError());
        std::cerr << "TTF_RenderUTF8_Blended failed: "<< TTF_GetError() << std::endl;
        return 0;
    }
    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(rendered, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(rendered);
    if (!rgba){std::cerr << "SDL_ConvertSurfaceFormat falure: " << std::endl; return 0;} 
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    outW = rgba->w;
    outH = rgba->h;
    SDL_FreeSurface(rgba);
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cerr << "Labels texture prepared" << std::endl;
    return tex;
    
}

void Painter3D::setWellLabel(WellDrawInfo& w, TTF_Font* font)
{
    if (w.labelTex.texture != 0) {glDeleteTextures(1, &w.labelTex.texture);}
    std::vector<std::string> selectedLabels;
    if (w.wellData->labels.size()>0){
        selectedLabels.push_back(w.wellData->NC);
        selectedLabels.insert(selectedLabels.end(), w.wellData->labels.begin(), w.wellData->labels.end());
        w.labelTex.texture = createTextTexture(selectedLabels, font, w.labelTex.texWidth, w.labelTex.texHeight);
    }
    else w.labelTex.texture = createTextTextureLine(w.wellData->NC, font, w.labelTex.texWidth, w.labelTex.texHeight);
}


void Painter3D::prepareContourMesh()
{
    std::vector<ContourVertex> vertices;
    contourRanges.clear();

    constexpr float kZOffset = 0.02f;
    constexpr float kHalfWidth = 0.003f; 
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    auto handleOneContur=[&](Contour& contour){
       
        int conturType=contour.getType();
        if (conturType>10 && conturType<990) return;
        int n = contour.size();
        if (n < 2) return;
        float khWidth=kHalfWidth*contours.getWidthByType(conturType);
        std::vector<glm::vec3> worldPos(n);
        for (int i = 0; i < n; ++i) {
            const PointD& p = contour[i];
            float z = m_fieldModel.top().getZ(p.x, p.y);
            float sx, sy, sz;
            modelToScene(p.x, p.y, z, sx, sy, sz);
            worldPos[i] = glm::vec3(sx, sy + kZOffset, sz);
        }

        GLint first = static_cast<GLint>(vertices.size());
        float arcLength = 0.0f;

        for (int i = 0; i < n; ++i) {
            // направление: среднее входящего и исходящего сегмента (сглаживание стыка)
            glm::vec3 dir(0.0f);
            if (i > 0) dir += glm::normalize(worldPos[i] - worldPos[i - 1]);
            if (i < n - 1) dir += glm::normalize(worldPos[i + 1] - worldPos[i]);
            if (glm::length(dir) < 1e-6f) dir = glm::vec3(1.0f, 0.0f, 0.0f); // вырожденный случай
            dir = glm::normalize(dir);

            glm::vec3 perp = glm::normalize(glm::cross(dir, up)) * khWidth;

            if (i > 0) {
                arcLength += glm::length(worldPos[i] - worldPos[i - 1]);
            }

            glm::vec3 left = worldPos[i] - perp;
            glm::vec3 right = worldPos[i] + perp;

            vertices.push_back(ContourVertex{{left.x, left.y, left.z}, arcLength, -khWidth});
            vertices.push_back(ContourVertex{{right.x, right.y, right.z}, arcLength, khWidth});
        }

        GLsizei count = static_cast<GLsizei>(vertices.size()) - first;
        SDL_Color lColor=contours.getColorByType(conturType);
        contourRanges.push_back(ContourRange{first, count,{lColor.r/255.0,lColor.g/255.0,lColor.g/255.0,lColor.a/255.0},contours.getStyleByType(conturType), khWidth});
        return;
    };
    for (int c = 0; c < contours.size(); ++c) {
         Contour& contour = contours[c];
        handleOneContur(contour);
    }
    for (int c = 0; c < isolines.size(); ++c) {
        Contour& contour = isolines[c];
        contour.type=1000;
        handleOneContur(contour);
    }
    if (contourVAO == 0) {
        glGenVertexArrays(1, &contourVAO);
        glGenBuffers(1, &contourVBO);
    }

    glBindVertexArray(contourVAO);
    glBindBuffer(GL_ARRAY_BUFFER, contourVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ContourVertex),vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ContourVertex), reinterpret_cast<void*>(offsetof(ContourVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(ContourVertex), reinterpret_cast<void*>(offsetof(ContourVertex, arcLength)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ContourVertex), reinterpret_cast<void*>(offsetof(ContourVertex, offset)));
    glBindVertexArray(0);
}

void Painter3D::prepareScene(){
    destroyScene();
    prepareSurfaceMesh(m_fieldModel.top(),m_fieldModel.parameter(),paramColorScale,false,meshes[0]); 
    uploadMeshBuffers(meshes[0]);   
    prepareSurfaceMesh(m_fieldModel.bottom(),m_fieldModel.parameter(),paramColorScale,true,meshes[1]);    
    uploadMeshBuffers(meshes[1]); 
    for (int i=0; i < 4 ;i++) {
        preparSides(i, meshes[i+2]);  
        uploadMeshBuffers(meshes[i+2]); 
    };
    prepareWellUnitMesh(12);
    uploadMeshBuffers(wellMesh.cylinder); 
    uploadMeshBuffers(wellMesh.cone); 
    for(const WellInfo& w : wells){
        WellDrawInfo oneUnit;
        float ztop=m_fieldModel.top().getZ(w.coord.x,w.coord.y);
        float zbottom=m_fieldModel.bottom().getZ(w.coord.x,w.coord.y);
        oneUnit.radius=0.01f;
        float sceneX,sceneY,sceneZ,sceneYb;
        modelToScene(w.coord.x,w.coord.y,ztop+5,sceneX,sceneY,sceneZ);
        modelToScene(w.coord.x,w.coord.y,zbottom-10,sceneX,sceneYb,sceneZ);
        oneUnit.headPos={sceneX,sceneY+0.05,sceneZ};
        oneUnit.length=(fabs(sceneY-sceneYb));
        oneUnit.color={0.0f,0.0f,0.0f,1.0f};
        if (w.typeRef==1) oneUnit.color=glm::vec4(0.588f, 0.294f, 0.0f, 1.0f);
        if (w.typeRef==2) oneUnit.color=glm::vec4(0.1f, 0.2f, 1.0f, 1.0f);
        oneUnit.wellData=&w;
        wellUnits.push_back(oneUnit);
    }
    TTF_Font* font = TTF_OpenFont("arial.ttf", 12);
    for(WellDrawInfo& w : wellUnits) setWellLabel(w, font);
    TTF_CloseFont(font);
    prepareContourMesh();
}
void Painter3D::destroyMeshBuffer(MeshBuffer& mesh)
{
    if (mesh.vao != 0)      { glDeleteVertexArrays(1, &mesh.vao); mesh.vao = 0; }
    if (mesh.vboGeom != 0)  { glDeleteBuffers(1, &mesh.vboGeom); mesh.vboGeom = 0; }
    if (mesh.vboColor != 0) { glDeleteBuffers(1, &mesh.vboColor); mesh.vboColor = 0; }
    if (mesh.ebo != 0)      { glDeleteBuffers(1, &mesh.ebo); mesh.ebo = 0; }
    mesh.indexCount = 0;
    mesh.geomDirty = true;
    mesh.colorDirty = true;
}

void Painter3D::destroyScene()
{
    for (MeshBuffer& mesh : meshes) {
        destroyMeshBuffer(mesh);
    }
    destroyMeshBuffer(wellMesh.cylinder);
    destroyMeshBuffer(wellMesh.cone);
    for (WellDrawInfo& w : wellUnits) {
        if (w.labelTex.texture != 0) {
            glDeleteTextures(1, &w.labelTex.texture);
            w.labelTex.texture = 0;
        }
    }
    wellUnits.clear();
}

void Painter3D::shutdown()
{
    destroyScene();
    if (labelDrawInfo.billboardVAO != 0) { glDeleteVertexArrays(1, &labelDrawInfo.billboardVAO); labelDrawInfo.billboardVAO = 0; }
    if (labelDrawInfo.billboardVBO != 0) { glDeleteBuffers(1, &labelDrawInfo.billboardVBO); labelDrawInfo.billboardVBO = 0; }
    if (shaderProgram != 0) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
    if (labelDrawInfo.labelShaderProgram != 0) { glDeleteProgram(labelDrawInfo.labelShaderProgram); labelDrawInfo.labelShaderProgram = 0; }
    if (lineShaderProgram != 0) { glDeleteProgram(lineShaderProgram); lineShaderProgram = 0; }
}

void Painter3D::drawWells()
{
    if (shaderProgram == 0 || wellUnits.empty()) return;
    if (wellMesh.cylinder.vao == 0 || wellMesh.cone.vao == 0) return;
    glUseProgram(shaderProgram);
    glUniform3fv(uLightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform1i(uUseUniformColorLoc, GL_TRUE);  
    glEnable(GL_CULL_FACE); 
    for (const WellDrawInfo& w : wellUnits) {
        glm::mat4 wellModel = glm::translate(glm::mat4(1.0f), w.headPos) * glm::scale(glm::mat4(1.0f), glm::vec3(w.radius, w.length, w.radius));
        glm::mat4 mvp = projectionMatrix * viewMatrix * wellModel;
        glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(wellModel));
        glUniform4fv(uColorLoc, 1, glm::value_ptr(w.color));
        glFrontFace(wellMesh.cylinder.flipWinding ? GL_CW : GL_CCW);
        glBindVertexArray(wellMesh.cylinder.vao);
        glDrawElements(wellMesh.cylinder.drawMode,static_cast<GLsizei>(wellMesh.cylinder.indexCount),GL_UNSIGNED_INT, nullptr);
        glFrontFace(wellMesh.cone.flipWinding ? GL_CW : GL_CCW);
        glBindVertexArray(wellMesh.cone.vao);
        glDrawElements(wellMesh.cone.drawMode,static_cast<GLsizei>(wellMesh.cone.indexCount),GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}
void Painter3D::drawModel()
{
    if (shaderProgram == 0) return;
    glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniform3fv(uLightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform1i(uUseUniformColorLoc, GL_FALSE); 
    for (MeshBuffer& mesh : meshes) {
        if (mesh.vao == 0 || mesh.indexCount == 0) continue;
        glFrontFace(mesh.flipWinding ? GL_CW : GL_CCW);
        glBindVertexArray(mesh.vao);
        glDrawElements(mesh.drawMode, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

void Painter3D::drawWellLabels()
{   

    float labelVerticalOffset=0.005;
    float labelWorldHeight=0.025;
    if (labelDrawInfo.labelShaderProgram == 0 || wellUnits.empty()) return;

    glUseProgram(labelDrawInfo.labelShaderProgram);

    glm::mat4 viewProj = projectionMatrix * viewMatrix;
    glUniformMatrix4fv(labelDrawInfo.uLabelViewProjLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniformMatrix4fv(labelDrawInfo.uLabelViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // не пишем в depth buffer — подписи не должны перекрывать друг друга по глубине как непрозрачные объекты

    glBindVertexArray(labelDrawInfo.billboardVAO);

    for (const WellDrawInfo& w : wellUnits) {
        if (w.labelTex.texture == 0) continue;

        glm::vec3 apexPos = w.headPos + glm::vec3(0.0f, (0.07f * w.length), -w.radius);
        apexPos.y += labelVerticalOffset; // небольшой отступ вверх/в сторону, чтобы не наезжать на конус

        float aspect = (w.labelTex.texHeight > 0) ? static_cast<float>(w.labelTex.texWidth) / static_cast<float>(w.labelTex.texHeight) : 1.0f;
        glm::vec2 size(labelWorldHeight * aspect, labelWorldHeight);

        glUniform3fv(labelDrawInfo.uLabelWorldPosLoc, 1, glm::value_ptr(apexPos));
        glUniform2fv(labelDrawInfo.uLabelSizeLoc, 1, glm::value_ptr(size));
        glUniform4fv(labelDrawInfo.uLabelColorLoc, 1, glm::value_ptr(w.labelColor));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, w.labelTex.texture);
        glUniform1i(labelDrawInfo.uLabelTexLoc, 0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Painter3D::drawContours(){
    if (lineShaderProgram == 0 || contourVAO == 0 || contourRanges.empty()) return;
    glUseProgram(lineShaderProgram);
    glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
    glUniformMatrix4fv(uLineMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(uLineDashLengthLoc, kDashLength);
    glUniform1f(uLineGapLengthLoc, kGapLength);
    glBindVertexArray(contourVAO);
    glFrontFace(GL_CCW);
    for (const ContourRange& range : contourRanges) {
        glUniform4fv(uLineColorLoc, 1, glm::value_ptr(range.color));
        glUniform1i(uLineStyleLoc, range.style);
        glUniform1f(uLineHalfWidthLoc, range.halfWidth);
        glDrawArrays(GL_TRIANGLE_STRIP, range.first, range.count);
    }
    glBindVertexArray(0);
    std::cerr << "Conturs draw cycle " <<std::endl;
}

void Painter3D::draw(){
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,1800,1800);
    drawModel();
    drawContours();
    drawWells();
    drawWellLabels();
}

void Painter3D::setProjectionMatrix(float fovYDegrees, float aspect, float nearPlane, float farPlane)
{
    projectionMatrix = glm::perspective(glm::radians(fovYDegrees), aspect, nearPlane, farPlane);
}

void Painter3D::setCam(){
    float x = m_camDistance * cosf(m_camVertAngle) * sinf(m_camHorizAngle);
    float y = m_camDistance * sinf(m_camVertAngle);
    float z = m_camDistance * cosf(m_camVertAngle) * cosf(m_camHorizAngle);
    viewMatrix = glm::lookAt(glm::vec3{x, y, z},glm::vec3{0.0f, 0.0f, 0.0f} , glm::vec3{0.0f, 1.0f, 0.0f});
}
void Painter3D::mouseMove(int dx,int dy){
    constexpr float MaxVertAngle = glm::radians(89.0f);
    m_camHorizAngle+=glm::radians(dx*0.5f);
    m_camVertAngle+=glm::radians(dy*0.5f);
    m_camVertAngle=glm::clamp(m_camVertAngle, -MaxVertAngle, MaxVertAngle);
    setCam();
}
void Painter3D::mouseScale(float dy){
    m_camDistance -= dy * 0.5f;
    m_camDistance = glm::clamp( m_camDistance,1.0f, 50.0f);

    setCam();    
}
void Painter3D::resetCamera()
{
    viewMatrix = glm::mat4(1.0f);    
}


