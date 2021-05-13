//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

#include "assimp_model_loading.h"
#include <map>

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
    u32 localParamsOffset;
    u32 localParamsSize;

    Entity(const glm::mat4& m, u32 mod) : mat(m), model(mod){}
};

enum LightType
{
    LightType_Directional,
    LightType_Point
};

    enum class FrameBuffer {
        Framebuffer,
        FinalRender, Albedo, Normals, Light, 
        Depth,
        MAX
    };

/*struct FrameBuffer {
    Attachment ids[(int)Attachment::MAX];
};*/

struct Light
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
    float intensity;
    Light(const LightType t, const vec3 c, vec3 dir, vec3 pos, float intensity): type(t),color(c),direction(dir),position(pos), intensity(intensity){}
};



struct Camera {
    enum CameraMode {
        FPS,
        ORBIT
    };
    CameraMode mode = ORBIT;
    static std::string CameraModeToString(CameraMode m) {
        switch (m)
        {
        case Camera::FPS:
            return "FPS";
        case Camera::ORBIT:
            return "Orbit";
        }
        return "unknown";
    }

    float distanceToOrigin = 12.f;
    float phi{ 90.f }, theta{ 90.f };
    vec3 pos;
    vec3 front = vec3(0.f, 0.f, -1.f);
    vec3 up = vec3(0.f, 1.f, 0.f);
    vec3 right = vec3(1.f, 0.f, 0.f);

    glm::mat4 GetViewMatrix(const vec2& size) {
        float Phi = glm::radians(phi);
        float Theta = glm::radians(theta);
        if (mode == CameraMode::ORBIT) {
            pos = { distanceToOrigin * sin(Phi) * cos(Theta), distanceToOrigin * cos(Phi), distanceToOrigin * sin(Phi) * sin(Theta) };

            return glm::perspective(glm::radians(60.f), size.x / size.y, 0.1f, 100.f) * glm::lookAt(pos, vec3(0.f), vec3(0.f, 1.f, 0.f));
        }
        else {
            front.x = cos(Theta) * cos(Phi);
            front.y = sin(Phi);
            front.z = sin(Theta) * cos(Phi);
            front = glm::normalize(front);
            // also re-calculate the Right and Up vector
            right = glm::normalize(glm::cross(front, vec3(0.f, 1.f, 0.f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
            up = glm::normalize(glm::cross(right, front));

            return glm::perspective(glm::radians(60.f), size.x / size.y, 0.1f, 100.f) * glm::lookAt(pos, pos + front, up);
        }
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
    std::map<FrameBuffer, u32> framebuffer;
    Buffer cBuffer;
    GLuint globlaParamsOffset;
    GLuint globalParamsSize;
    int uniformBlockAligment;

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

static std::string FrameBufferToString(FrameBuffer fb);
