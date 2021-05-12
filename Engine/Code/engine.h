//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

#include "assimp_model_loading.h"

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct VertexBufferAttribute {
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout {
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute {
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout {
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao {
    GLuint handle;
    GLuint programHandle;
};

struct Buffer {
    GLuint  handle;
    GLenum  type;
    u32     size;
    u32     head;
    void*   data;
};

struct Model {
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Submesh {
    VertexBufferLayout	vertexBufferLayout;
    std::vector<float>	vertices;
    std::vector<u32>	indices;
    u32					vertexOffset;
    u32					indexOffset;

    std::vector<Vao>	vaos;
};

struct Mesh {
    std::vector<Submesh>	submeshes;
    GLuint					vertexBufferHandle;
    GLuint					indexBufferHandle;
};

struct Material {
    std::string name;
    vec3 albedo;
    vec3 emissive;
    f32 smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

struct VertexV3V2 {
    vec3 pos;
    vec2 uv;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Patrick,
    Mode_Count
};

struct Entity {
    glm::mat4 mat = glm::mat4(1.0f);
    u32 model;

    Entity(const glm::mat4& m, u32 mod) : mat(m), model(mod){}
};

enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
    Light(const LightType t, const vec3 c, vec3 dir, vec3 pos): type(t),color(c),direction(dir),position(pos){}
};

struct Camera {
    enum CameraMode {
        FPS,
        ORBIT
    };
    CameraMode mode = ORBIT;

    float distanceToOrigin = 12.f;
    float phi{ 90.f }, theta{ 90.f };
    vec3 pos;

    glm::mat4 GetViewMatrix(const vec2& size) {
        // Make sure that: 0 < phi < 3.14
        float Phi = glm::radians(phi);
        float Theta = glm::radians(theta);
        pos = { distanceToOrigin * sin(Phi) * cos(Theta), distanceToOrigin * cos(Phi), distanceToOrigin * sin(Phi) * sin(Theta) };

        return glm::perspective(glm::radians(60.f), size.x / size.y, 0.1f, 100.f) * glm::lookAt(pos, vec3(0.f), vec3(0.f, 1.f, 0.f));
    }
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;
    GLuint texturedMeshProgramIdx_uTexture;
    GLuint texturedMeshProgramIdx_uViewProjection;
    GLuint texturedMeshProgramIdx_uWorldMatrix;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    std::vector<Texture> textures;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
    std::vector<Model> models;
    std::vector<Program> programs;

    std::vector<Entity> entities;

    Camera camera;
    std::vector<Light> lights;

    //Framebuffer
    GLuint framebufferHandle; //TODO struct and enum with attachments
    GLuint depthAttachmentHandle;
    GLuint colorAttachmentHandle;
    GLuint normalsAttachmentHandle;
    GLuint specularAttachmentHandle;
    GLuint lightAttachmentHandle;
};

u32 LoadTexture2D(App* app, const char* filepath);

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

void APIENTRY CheckOpenGLError(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam);

