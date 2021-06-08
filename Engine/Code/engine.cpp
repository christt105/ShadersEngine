//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "assimp_model_loading.h"
#include "buffer_management.h"

#define BINDING(b) b

float Camera::moveSpeed;

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image, GLenum wrapTex)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapTex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath, GLenum wrapTex)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;
    
    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image, wrapTex);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void Init(App* app)
{
    // Initialize your resources here!
    // - vertex buffers
    VertexV3V2 vertices[] = {
        {   vec3(-0.5f, -0.5f, 0.f), vec2(0.f, 0.f) },
        {   vec3( 0.5f, -0.5f, 0.f), vec2(1.f, 0.f) },
        {   vec3( 0.5f,  0.5f, 0.f), vec2(1.f, 1.f) },
        {   vec3(-0.5f,  0.5f, 0.f), vec2(0.f, 1.f) }
    };

    u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    app->mode = Mode::Mode_Water;

    Camera::moveSpeed = 10.f;
    if (app->mode == Mode::Mode_Water) {
        app->camera.pos = vec3(10.f, 8.f, 12.f);
        app->camera.theta = -130.f;
        app->camera.phi = -15.f;
    }

    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // - element/index buffers
    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // - vaos
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    GLint maxsize;

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAligment);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxsize);
    app->cBuffer = CreateBuffer(maxsize, GL_UNIFORM_BUFFER, GL_STREAM_DRAW);

    // - programs (and retrieve uniform indices)
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uAlbedoTexture");
    texturedGeometryProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedGeometryProgram.vertexInputLayout.attributes.push_back({ 1, 2 });

    app->texturedForwardProgramIdx = LoadProgram(app, "shaders.glsl", "FORWARD_SHADING");
    Program& texturedForwardProgram = app->programs[app->texturedForwardProgramIdx];
    app->texturedMeshProgramIdx_uTexture = glGetUniformLocation(texturedForwardProgram.handle, "uAlbedoTexture");
    texturedForwardProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedForwardProgram.vertexInputLayout.attributes.push_back({ 1, 3 });
    texturedForwardProgram.vertexInputLayout.attributes.push_back({ 2, 2 });
    texturedForwardProgram.vertexInputLayout.attributes.push_back({ 3, 3 });
    texturedForwardProgram.vertexInputLayout.attributes.push_back({ 4, 3 });

    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    app->texturedMeshProgramIdx_uTexture2 = glGetUniformLocation(texturedMeshProgram.handle, "uAlbedoTexture");
    app->texturedMeshProgramIdx_uTexture3 = glGetUniformLocation(texturedMeshProgram.handle, "uNormalTexture");
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 1, 3 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2, 2 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 3, 3 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 4, 3 });

    app->texturedLightProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_LIGHTS");
    Program& texturedLightProgram = app->programs[app->texturedLightProgramIdx];
    app->texturedMeshProgramIdx_uNormals    = glGetUniformLocation(texturedLightProgram.handle, "uNormalsTexture");
    app->texturedMeshProgramIdx_uPosition   = glGetUniformLocation(texturedLightProgram.handle, "uPositionTexture");
    app->texturedMeshProgramIdx_uAlbedo     = glGetUniformLocation(texturedLightProgram.handle, "uAlbedoTexture");
    app->texturedMeshProgramIdx_uDepth      = glGetUniformLocation(texturedLightProgram.handle, "uDepthTexture");
    texturedLightProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedLightProgram.vertexInputLayout.attributes.push_back({ 1, 2 });
    
    app->texturedSphereLightsProgramIdx = LoadProgram(app, "shaders.glsl", "DRAW_LIGHTS");
    Program& texturedSphereLightProgram = app->programs[app->texturedSphereLightsProgramIdx];
    app->texturedLightProgramIdx_uLightColor = glGetUniformLocation(texturedSphereLightProgram.handle, "lightColor");
    app->texturedLightProgramIdx_uViewProjection = glGetUniformLocation(texturedSphereLightProgram.handle, "projectionView");
    app->texturedLightProgramIdx_uModel = glGetUniformLocation(texturedSphereLightProgram.handle, "model");
    texturedSphereLightProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedSphereLightProgram.vertexInputLayout.attributes.push_back({ 1, 2 });
    texturedSphereLightProgram.vertexInputLayout.attributes.push_back({ 2, 3 });

    app->baseModelProgramIdx = LoadProgram(app, "shaders.glsl", "DRAW_BASE_MODEL");
    Program& texturedBaseProgram = app->programs[app->baseModelProgramIdx];
    app->BaseModelProgramIdx_uViewProjection = glGetUniformLocation(texturedBaseProgram.handle, "uWorldViewProjectionMatrix");
    app->BaseModelProgramIdx_uPlane = glGetUniformLocation(texturedBaseProgram.handle, "plane");
    app->BaseModelProgramIdx_uFaceColor = glGetUniformLocation(texturedBaseProgram.handle, "faceColor");
    app->BaseModelProgramIdx_uLightPos = glGetUniformLocation(texturedBaseProgram.handle, "lightPos");
    app->BaseModelProgramIdx_uLightColor = glGetUniformLocation(texturedBaseProgram.handle, "lightColor");
    texturedBaseProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedBaseProgram.vertexInputLayout.attributes.push_back({ 1, 3 });

    app->waterProgramIdx = LoadProgram(app, "shaders.glsl", "WATER_SHADER");
    Program& texturedWaterProgram = app->programs[app->waterProgramIdx];
    app->WaterProgramIdx_uViewProjection = glGetUniformLocation(texturedWaterProgram.handle, "uWorldViewProjectionMatrix");
    app->WaterProgramIdx_uModelMatrix = glGetUniformLocation(texturedWaterProgram.handle, "uWorldMatrix");
    app->WaterProgramIdx_uReflectionTex = glGetUniformLocation(texturedWaterProgram.handle, "reflectionTex");
    app->WaterProgramIdx_uRefractionTex = glGetUniformLocation(texturedWaterProgram.handle, "refractionTex");
    app->WaterProgramIdx_uDudvTex = glGetUniformLocation(texturedWaterProgram.handle, "dudvMap");
    app->WaterProgramIdx_uMoveFactor = glGetUniformLocation(texturedWaterProgram.handle, "move");
    app->WaterProgramIdx_uCameraPos = glGetUniformLocation(texturedWaterProgram.handle, "cameraPos");
    app->WaterProgramIdx_uNormalMapTex = glGetUniformLocation(texturedWaterProgram.handle, "normalMap");
    app->WaterProgramIdx_uLightPos = glGetUniformLocation(texturedWaterProgram.handle, "lightPos");
    app->WaterProgramIdx_uLightColor = glGetUniformLocation(texturedWaterProgram.handle, "lightColor");
    app->WaterProgramIdx_uDepthMap = glGetUniformLocation(texturedWaterProgram.handle, "depthMap");
    app->WaterProgramIdx_uWaveStrength = glGetUniformLocation(texturedWaterProgram.handle, "waveStrength");
    app->WaterProgramIdx_uShineDamper = glGetUniformLocation(texturedWaterProgram.handle, "shineDamper");
    app->WaterProgramIdx_uReflectivity = glGetUniformLocation(texturedWaterProgram.handle, "reflectivity");
    app->WaterProgramIdx_uTiling = glGetUniformLocation(texturedWaterProgram.handle, "tiling");
    texturedWaterProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedWaterProgram.vertexInputLayout.attributes.push_back({ 1, 2 });
    
    // - textures
    app->diceTexIdx     = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx    = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx    = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx   = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx  = LoadTexture2D(app, "color_magenta.png");

    app->normalMapIdx = LoadTexture2D(app, "3/Textures/Normal.png", GL_REPEAT);
    app->bumpMapIdx = LoadTexture2D(app, "3/Textures/Height.png", GL_REPEAT);
    app->albedoMapIdx = LoadTexture2D(app, "3/Textures/Color.png", GL_REPEAT);
    //u32 pat = LoadModel(app, "Patrick/Patrick.obj");


    //u32 cliff = LoadModel(app, "Cliff2/rocks.obj");
    //u32 cliff = LoadModel(app, "weapon/source/bandygun3_notUDIm.fbx");
    //u32 cliff = LoadModel(app, "Cliff/eyeyey/a.obj");
    //u32 cliff = LoadModel(app, "AK47/AK47.obj");
    //u32 cliff = LoadModel(app, "3/Models_OBJ/Terrain_50000.obj");
    //u32 cliff = LoadModel(app, "Cubo/Cube_obj.obj");
    u32 cliff = LoadModel(app, "Plane/Plane.obj");

    app->entities.push_back(Entity(glm::mat4(1.f), cliff));
    /*app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(0.0f, 0.1f, 5.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(0.0f, 0.1f, 10.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(-12.1f, 0.1f, 1.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(12.1f, 0.1f, 1.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(-12.1f, 0.1f, 5.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(12.1f, 0.1f, 5.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(-12.1f, 0.1f, 10.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(12.1f, 0.1f, 10.f)), pat));*/

    app->lights.push_back(Light(LightType::LightType_Directional, vec3(1.f, 1.f, 1.f), vec3(0.89, -1.0, -1.0), vec3(0.f, 10.f,11.5f), 0.9f));
   /* app->lights.push_back(Light(LightType::LightType_Point, vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.f, 2.1f, 1.9f), 2.f));
    app->lights.push_back(Light(LightType::LightType_Point, vec3(1.0, 0.0, .0), vec3(0.0, 1.0, 1.0), vec3(0.f, 2.1f, 5.9f), 5.f));
    app->lights.push_back(Light(LightType::LightType_Point, vec3(0.0, 0.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.f, 3.f, 11.f), 15.f));
    app->lights.push_back(Light(LightType::LightType_Point, vec3(0.0, 1.0, 0.0), vec3(0.0, -1.0, 1.0), vec3(13.f, 2.f, 5.f), 2.f));
    app->lights.push_back(Light(LightType::LightType_Point, vec3(0.1, 0.5, 0.3), vec3(0.0, -1.0, 1.0), vec3(13.f, 6.f, 2.f), 10.f));
    app->lights.push_back(Light(LightType::LightType_Point, vec3(0.5, 0.3, 0.1), vec3(0.0, -1.0, 1.0), vec3(-13.f, 2.f, 11.f), 3.f));
    app->lights.push_back(Light(LightType::LightType_Point, vec3(0.3, 0.1, 0.5), vec3(0.0, -1.0, 1.0), vec3(-13.f, 6.f, 5.f), 10.f));
    app->lights.push_back(Light(LightType::LightType_Point, vec3(0.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(12.f, 2.f, 2.f), 4.f));*/

    app->mode = Mode::Mode_Deferred;
    app->island = LoadModel(app, "WaterScene/volcano.obj");
    app->wTexDudvSelected = app->wTexDudv1 = LoadTexture2D(app, "WaterScene/waterDUDV.png", GL_REPEAT);
    app->wTexDudv2 = LoadTexture2D(app, "WaterScene/waterDUDV2.jpg", GL_REPEAT);
    app->wTexNormalMap = LoadTexture2D(app, "WaterScene/normalMap.png", GL_REPEAT);
    app->water = WaterTile(vec3(4.7f, 2.534f, 2.5f), vec2(4.f, 6.f));

    //Framebuffer
    for (int i = 0; i < (int)FrameBuffer::MAX; ++i) {
        app->framebuffer[(FrameBuffer)i] = 0;
    }

    for (int i = (int)FrameBuffer::FinalRender; i < (int)FrameBuffer::Depth; ++i) {
        glGenTextures(1, &app->framebuffer[(FrameBuffer)i]);
        glBindTexture(GL_TEXTURE_2D, app->framebuffer[(FrameBuffer)i]);
        if (i == (int)FrameBuffer::Position || i == (int)FrameBuffer::Normals) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }
        else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenTextures(1, &app->framebuffer[FrameBuffer::Depth]);
    glBindTexture(GL_TEXTURE_2D, app->framebuffer[FrameBuffer::Depth]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->framebuffer[FrameBuffer::Framebuffer]);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebuffer[FrameBuffer::Framebuffer]);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->framebuffer[FrameBuffer::FinalRender], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->framebuffer[FrameBuffer::Normals], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->framebuffer[FrameBuffer::Albedo], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->framebuffer[FrameBuffer::Light], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, app->framebuffer[FrameBuffer::Position], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->framebuffer[FrameBuffer::Depth], 0);

    CheckFramebufferStatus();

    glDrawBuffers(5, &app->framebuffer[FrameBuffer::FinalRender]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // WATER =============================================================================
    int size[] = { app->displaySize.x, app->displaySize.y };
    glGenTextures(1, &app->wTexReflection);
    glBindTexture(GL_TEXTURE_2D, app->wTexReflection);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &app->wTexRefraction);
    glBindTexture(GL_TEXTURE_2D, app->wTexRefraction);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &app->wDepthReflection);
    glBindTexture(GL_TEXTURE_2D, app->wDepthReflection);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size[0], size[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glGenTextures(1, &app->wDepthRefraction);
    glBindTexture(GL_TEXTURE_2D, app->wDepthRefraction);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size[0], size[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glGenTextures(1, &app->wTexBase);
    glBindTexture(GL_TEXTURE_2D, app->wTexBase);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &app->wDepthBase);
    glBindTexture(GL_TEXTURE_2D, app->wDepthBase);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size[0], size[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->wFboBase);
    glBindFramebuffer(GL_FRAMEBUFFER, app->wFboBase);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->wTexBase, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->wDepthBase, 0);
    CheckFramebufferStatus();
    glDrawBuffers(1, &app->wFboBase);
    glBindFramebuffer(GL_FRAMEBUFFER, NULL);

    glGenFramebuffers(1, &app->wFboReflect);
    glBindFramebuffer(GL_FRAMEBUFFER, app->wFboReflect);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->wTexReflection, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->wDepthReflection, 0);
    CheckFramebufferStatus();
    glDrawBuffers(1, &app->wFboReflect);
    glBindFramebuffer(GL_FRAMEBUFFER, NULL);

    glGenFramebuffers(1, &app->wFboRefract);
    glBindFramebuffer(GL_FRAMEBUFFER, app->wFboRefract);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->wTexRefraction, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->wDepthRefraction, 0);
    CheckFramebufferStatus();
    glDrawBuffers(1, &app->wFboReflect);
    glBindFramebuffer(GL_FRAMEBUFFER, NULL);
}

void CheckFramebufferStatus()
{
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        switch (framebufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                      ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                    ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:       ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default:                                            ELOG("Unknown franebuffer status error | %i", framebufferStatus);
        }
    }
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);

    ImGui::Separator();

    ImGui::Text("OpenGL Info");
    ImGui::Text((const char*)glGetString(GL_VENDOR));
    ImGui::Text((const char*)glGetString(GL_RENDERER));
    ImGui::Text((const char*)glGetString(GL_VERSION));
    ImGui::Text((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    ImGui::Separator();

    ImGui::Text("Mode");
    ImGui::PushID("##mode");
    if (ImGui::BeginCombo("Type", ModeToString(app->mode).c_str())) {
        for (int i = 0; i < (int)Mode::Mode_Count; ++i) {
            if (ImGui::Selectable(ModeToString((Mode)i).c_str(), app->mode == i))
                app->mode = (Mode)i;
        }
        ImGui::EndCombo();
    }

    ImGui::Checkbox("Show sphere lights", &app->showSpheres);
    ImGui::PopID();

    ImGui::Separator();

    ImGui::Text("Camera");
    ImGui::PushID("##camera");
    if (ImGui::BeginCombo("Type", Camera::CameraModeToString(app->camera.mode).c_str())) {
        if (ImGui::Selectable("Orbit", app->camera.mode == Camera::CameraMode::ORBIT)) app->camera.mode = Camera::CameraMode::ORBIT;
        if (ImGui::Selectable("FPS", app->camera.mode == Camera::CameraMode::FPS)) { app->camera.mode = Camera::CameraMode::FPS; app->camera.phi = -10.f; app->camera.theta = -90.f;}
        ImGui::EndCombo();
    }
    if (app->camera.mode == Camera::CameraMode::ORBIT) {
        ImGui::DragFloat("DistanceToOrigin", &app->camera.distanceToOrigin, 0.15f);
        ImGui::SliderFloat("phi", &app->camera.phi, 0.1f, 179.f, "%.1f");
        ImGui::DragFloat("theta", &app->camera.theta);
    }
    else {
        ImGui::DragFloat3("Position", &app->camera.pos.x);
        ImGui::DragFloat("MoveSpeed", &Camera::moveSpeed, 0.05f);
    }
    ImGui::PopID();

    if (app->mode == Mode::Mode_Water) {
        ImGui::Separator();
        ImGui::PushID("Water");

        ImGui::Text("Water");
        ImGui::DragFloat("Speed", &app->wMoveSpeed, 0.001f);
        ImGui::DragFloat("Wave Strength", &app->wuWaveStrength, 0.01f);
        ImGui::DragFloat("Shine Damper", &app->wuShineDamper, 0.5f);
        ImGui::DragFloat("Reflectivity", &app->wuReflectivity, 0.001f);
        ImGui::DragFloat("Tiling", &app->tiling, 0.5f);
        bool isFirst = (app->wTexDudvSelected == app->wTexDudv1);
        if (ImGui::Checkbox("Use DUDV map 2", &isFirst)) {
            app->wTexDudvSelected = (app->wTexDudvSelected == app->wTexDudv1) ? app->wTexDudv2 : app->wTexDudv1;
        }

        ImGui::PopID();
    }

    ImGui::Separator();

    ImGui::Text("Lights");
    ImGui::PushID("Lights");
    if (app->mode == Mode::Mode_Water) {
        ImGui::DragFloat3("Position", glm::value_ptr(app->wLigthPos));
        ImGui::ColorEdit3("Color", glm::value_ptr(app->wLigthColor));
    }
    else {
        if (ImGui::CollapsingHeader("Edit")) {
            for (int i = 0; i < app->lights.size(); ++i) {
                ImGui::PushID(i);
                if (app->lights[i].type == 0) { //Directional
                    ImGui::DragFloat3("direction", glm::value_ptr(app->lights[i].direction), 0.01f);
                    ImGui::DragFloat("intensity", &app->lights[i].radius, 0.01f);
                }
                else {
                    ImGui::DragFloat3("position", glm::value_ptr(app->lights[i].position), 0.01f);
                    ImGui::DragFloat("radius", &app->lights[i].radius, 0.01f);
                }
                ImGui::DragFloat3("color", glm::value_ptr(app->lights[i].color), 0.01f);
                ImGui::PopID();
                ImGui::NewLine();
            }
        }
    }
    ImGui::PopID();

    ImGui::Separator();

    ImGui::Text("Target render");
    if (app->mode == Mode::Mode_Water) {
        static int sel = 0;
        static const char* wtargets[] = { "Final", "Reflection", "Refract" };
        if (ImGui::BeginCombo("Target", wtargets[sel])) {
            for (int i = 0; i < 3; ++i)
                if (ImGui::Selectable(wtargets[i])) sel = i;
            ImGui::EndCombo();
        }

        switch (sel)
        {
        case 0:
            ImGui::Image((ImTextureID)app->wTexBase, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
            break;
        case 1:
            ImGui::Image((ImTextureID)app->wTexReflection, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
            break;
        case 2:
            ImGui::Image((ImTextureID)app->wTexRefraction, ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
            break;
        default:
            break;
        }
    }
    else {
        static int sel = (int)FrameBuffer::FinalRender;
        if (ImGui::BeginCombo("Target", FrameBufferToString((FrameBuffer)sel).c_str())) {
            for (int i = (int)FrameBuffer::FinalRender; i < (int)FrameBuffer::MAX; ++i)
                if (ImGui::Selectable(FrameBufferToString((FrameBuffer)i).c_str())) sel = i;
            ImGui::EndCombo();
        }

        ImGui::Image((ImTextureID)app->framebuffer[(FrameBuffer)sel], ImVec2(ImGui::GetWindowWidth(), app->displaySize.y * ImGui::GetWindowWidth() / app->displaySize.x), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
    }

    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    if (app->camera.mode == Camera::CameraMode::ORBIT) {
        if (app->input.mouseButtons[0] == ButtonState::BUTTON_PRESSED) {
            app->camera.theta += app->input.mouseDelta.x * app->deltaTime * 20.f;
            app->camera.phi -= app->input.mouseDelta.y * app->deltaTime * 20.f;
            app->camera.phi = std::max(0.1f, std::min(app->camera.phi, 179.9f));
        }

        if (app->input.zDelta != 0.f) {
            app->camera.distanceToOrigin -= app->input.zDelta * 20.f * app->deltaTime;
            app->camera.distanceToOrigin = std::max(app->camera.distanceToOrigin, 1.f);
            app->input.zDelta = 0.f;
        }
    }
    else {
        if (app->input.keys[Key::K_W] == ButtonState::BUTTON_PRESSED) {
            app->camera.pos += app->camera.front * Camera::moveSpeed * app->deltaTime;
        }
        if (app->input.keys[Key::K_A] == ButtonState::BUTTON_PRESSED) {
            app->camera.pos -= app->camera.right * Camera::moveSpeed * app->deltaTime;
        }
        if (app->input.keys[Key::K_S] == ButtonState::BUTTON_PRESSED) {
            app->camera.pos -= app->camera.front * Camera::moveSpeed * app->deltaTime;
        }
        if (app->input.keys[Key::K_D] == ButtonState::BUTTON_PRESSED) {
            app->camera.pos += app->camera.right * Camera::moveSpeed * app->deltaTime;
        }
        if (app->input.keys[Key::K_R] == ButtonState::BUTTON_PRESSED) {
            app->camera.pos += app->camera.up * Camera::moveSpeed * app->deltaTime;
        }
        if (app->input.keys[Key::K_F] == ButtonState::BUTTON_PRESSED) {
            app->camera.pos -= app->camera.up * Camera::moveSpeed * app->deltaTime;
        }

        if (app->input.mouseButtons[0] == ButtonState::BUTTON_PRESSED) {
            app->camera.theta += app->input.mouseDelta.x * app->deltaTime * 20.f;
            app->camera.phi -= app->input.mouseDelta.y * app->deltaTime * 20.f;
            app->camera.phi = std::max(-89.9f, std::min(app->camera.phi, 89.9f));
        }
    }

    for (u64 i = 0ULL; i < app->programs.size(); ++i) {
        Program& program = app->programs[i];
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
        if (currentTimestamp > program.lastWriteTimestamp) {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = currentTimestamp;
        }
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program) {
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;

    GLuint vaoHandle = 0;

    //Create a new vao for this submesh/program
    {
        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i) {
            bool attributeWasLinked = false;

            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j) {
                if (program.vertexInputLayout.attributes[i].location != submesh.vertexBufferLayout.attributes[j].location)
                    continue;
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
            assert(attributeWasLinked);
        }

        glBindVertexArray(0);
    }

    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void Render(App* app)
{
    // - clear the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebuffer[FrameBuffer::Framebuffer]);
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    switch (app->mode)
    {
    case Mode_TexturedQuad: {
        glBindFramebuffer(GL_FRAMEBUFFER, NULL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniform1i(app->programUniformTexture, 0);
        glActiveTexture(GL_TEXTURE0);
        GLuint textureHandle = app->textures[app->diceTexIdx].handle;
        glBindTexture(GL_TEXTURE_2D, textureHandle);

        const Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
        glUseProgram(programTexturedGeometry.handle);

        renderQuad();
        break;
    }
    case Mode_Forward: {
        Program& texturedMeshProgram = app->programs[app->texturedForwardProgramIdx];
        glUseProgram(texturedMeshProgram.handle);

        MapBuffer(app->cBuffer, GL_WRITE_ONLY);

        app->globlaParamsOffset = app->cBuffer.head;

        PushVec3(app->cBuffer, app->camera.pos);

        PushUInt(app->cBuffer, app->lights.size());
        for (auto& light : app->lights)
        {
            AlignHead(app->cBuffer, sizeof(glm::vec4));

            PushUInt(app->cBuffer, light.type);
            PushVec3(app->cBuffer, light.color);
            PushVec3(app->cBuffer, light.direction);
            PushVec3(app->cBuffer, light.position);
            float constant = 0.5f;
            float linear = 0.5f;
            float quadratic = 1.f;

            PushFloat(app->cBuffer, light.radius);
            PushFloat(app->cBuffer, linear);
            PushFloat(app->cBuffer, quadratic);
           

        }
        app->globalParamsSize = app->cBuffer.head - app->globlaParamsOffset;

        for (auto& e : app->entities) {

            Model& model = app->models[e.model];
            Mesh& mesh = app->meshes[model.meshIdx];

            glm::mat4 viewMat = app->camera.GetViewMatrix({ app->displaySize.x, app->displaySize.y });

            AlignHead(app->cBuffer, app->uniformBlockAligment);
            e.localParamsOffset = app->cBuffer.head;
            PushMat4(app->cBuffer, e.mat);
            PushMat4(app->cBuffer, viewMat);
            
           

            e.localParamsSize = app->cBuffer.head - e.localParamsOffset;

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cBuffer.handle, app->globlaParamsOffset, app->globalParamsSize);
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cBuffer.handle, e.localParamsOffset, e.localParamsSize);

            for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshmaterial = app->materials[submeshMaterialIdx];
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshmaterial.albedoTextureIdx].handle);
                glUniform1i(app->texturedMeshProgramIdx_uTexture, 0);

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)submesh.indexOffset);
            }
        }
        UnmapBuffer(app->cBuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, NULL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
        glUseProgram(programTexturedGeometry.handle);

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(app->programUniformTexture, 0);
        GLuint textureHandle = app->framebuffer[FrameBuffer::FinalRender];
        glBindTexture(GL_TEXTURE_2D, textureHandle);

        renderQuad();

        break;
    }
    case Mode::Mode_Deferred: {
        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        glUseProgram(texturedMeshProgram.handle);

        MapBuffer(app->cBuffer, GL_WRITE_ONLY);

        app->globlaParamsOffset = app->cBuffer.head;

        PushVec3(app->cBuffer, app->camera.pos);

        PushUInt(app->cBuffer, app->lights.size());

        app->globalParamsSize = app->cBuffer.head - app->globlaParamsOffset;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->textures[app->albedoMapIdx].handle);
        glUniform1i(app->texturedMeshProgramIdx_uTexture2, 0);

        ////manual
        //glActiveTexture(GL_TEXTURE1);
        //glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uhasNormalMap"), 1);
        //glBindTexture(GL_TEXTURE_2D, app->textures[app->normalMapIdx].handle);
        //glUniform1i(app->texturedMeshProgramIdx_uTexture3, 1);

        //glActiveTexture(GL_TEXTURE2);
        //glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uhasBumpMap"), 1);
        //glBindTexture(GL_TEXTURE_2D, app->textures[app->bumpMapIdx].handle);
        //glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uBumpTexture"), 2);

        for (auto& e : app->entities) {

            Model& model = app->models[e.model];
            Mesh& mesh = app->meshes[model.meshIdx];

            glm::mat4 viewMat = app->camera.GetViewMatrix({ app->displaySize.x, app->displaySize.y });

            AlignHead(app->cBuffer, app->uniformBlockAligment);
            e.localParamsOffset = app->cBuffer.head;
            PushMat4(app->cBuffer, e.mat);
            PushMat4(app->cBuffer, viewMat);
            e.localParamsSize = app->cBuffer.head - e.localParamsOffset;

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cBuffer.handle, app->globlaParamsOffset, app->globalParamsSize);
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cBuffer.handle, e.localParamsOffset, e.localParamsSize);
           
            for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);


                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshmaterial = app->materials[submeshMaterialIdx];
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshmaterial.albedoTextureIdx].handle);
                glUniform1i(app->texturedMeshProgramIdx_uTexture2, 0);

                //automatic
                glActiveTexture(GL_TEXTURE2);
                glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uhasBumpMap"), submeshmaterial.hasBumpText);
                if (submeshmaterial.hasBumpText) {
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshmaterial.bumpTextureIdx].handle);
                    glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uBumpTexture"), 2);
                }

                glUniform1i(glGetUniformLocation(texturedMeshProgram.handle, "uhasNormalMap"), submeshmaterial.hasNormalText);
                glActiveTexture(GL_TEXTURE1);
                if (submeshmaterial.hasNormalText) {
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshmaterial.normalsTextureIdx].handle);
                    glUniform1i(app->texturedMeshProgramIdx_uTexture3, 1);
                }

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)submesh.indexOffset);
            }
        }
       

        glBindFramebuffer(GL_FRAMEBUFFER, NULL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(app->programs[app->texturedLightProgramIdx].handle);

        glUniform1i(app->texturedMeshProgramIdx_uPosition, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->framebuffer[FrameBuffer::Position]);
        glUniform1i(app->texturedMeshProgramIdx_uNormals, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, app->framebuffer[FrameBuffer::Normals]);
        glUniform1i(app->texturedMeshProgramIdx_uAlbedo, 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, app->framebuffer[FrameBuffer::Albedo]);
        glUniform1i(app->texturedMeshProgramIdx_uDepth, 3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, app->framebuffer[FrameBuffer::Depth]);

        AlignHead(app->cBuffer, app->uniformBlockAligment);

        app->globlaParamsOffset = app->cBuffer.head;

        PushVec3(app->cBuffer, app->camera.pos);
        PushUInt(app->cBuffer, app->lights.size());

        for (auto& light : app->lights)
        {
            AlignHead(app->cBuffer, sizeof(vec4));

            PushUInt(app->cBuffer, light.type);
            PushVec3(app->cBuffer, light.color);
            PushVec3(app->cBuffer, light.direction);
            PushVec3(app->cBuffer, light.position);

            float constant = 0.5f;
            float linear = 0.5f;
            float quadratic = 1.f;

            PushFloat(app->cBuffer, light.radius);
            PushFloat(app->cBuffer, linear);
            PushFloat(app->cBuffer, quadratic);
        }
        app->globalParamsSize = app->cBuffer.head - app->globlaParamsOffset;

        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cBuffer.handle, app->globlaParamsOffset, app->globalParamsSize);

        renderQuad();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, app->framebuffer[FrameBuffer::Framebuffer]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (app->showSpheres) {
            glUseProgram(app->programs[app->texturedSphereLightsProgramIdx].handle);
            glUniformMatrix4fv(app->texturedLightProgramIdx_uViewProjection, 1, GL_FALSE, glm::value_ptr(app->camera.GetViewMatrix(app->displaySize)));

            for (unsigned int i = 0; i < app->lights.size(); ++i) {
                AlignHead(app->cBuffer, app->uniformBlockAligment);

                int localParamsOffset = app->cBuffer.head;
                glm::mat4 mat = glm::mat4(1.f);
                mat = glm::translate(mat, app->lights[i].position);
                mat = glm::scale(mat, vec3(app->lights[i].radius));
                PushMat4(app->cBuffer, mat);
                PushVec3(app->cBuffer, app->lights[i].color);
                int localParamsSize = app->cBuffer.head - localParamsOffset;

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cBuffer.handle, localParamsOffset, localParamsSize);

                if (app->lights[i].type == LightType_Point)
                    renderSphere();
                else
                    renderCube();
            }
        }
        UnmapBuffer(app->cBuffer);
        break;
    }
    case Mode::Mode_Water: {
        //REFLECTION
        {
            glBindFramebuffer(GL_FRAMEBUFFER, app->wFboReflect);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glEnable(GL_DEPTH_TEST);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glEnable(GL_CLIP_DISTANCE0);

            glClearColor(0.2f, 0.2f, 0.2f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Program& texturedMeshProgram = app->programs[app->baseModelProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            Camera reflectionCamera = app->camera;
            reflectionCamera.pos.y -= 2.f * (app->camera.pos.y - app->water.pos.y);
            reflectionCamera.phi = -reflectionCamera.phi;

            Model& model = app->models[app->island];
            Mesh& mesh = app->meshes[model.meshIdx];

            glm::mat4 viewMat = reflectionCamera.GetViewMatrix({ app->displaySize.x, app->displaySize.y });
            glUniformMatrix4fv(app->BaseModelProgramIdx_uViewProjection, 1, GL_FALSE, glm::value_ptr(viewMat));

            glUniform4f(app->BaseModelProgramIdx_uPlane, 0.f, 1.f, 0.f, -app->water.pos.y);

            glUniform3fv(app->BaseModelProgramIdx_uLightPos, 1, glm::value_ptr(app->wLigthPos));
            glUniform3fv(app->BaseModelProgramIdx_uLightColor, 1, glm::value_ptr(app->wLigthColor));

            for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshmaterial = app->materials[submeshMaterialIdx];
                glUniform3fv(app->BaseModelProgramIdx_uFaceColor, 1, glm::value_ptr(submeshmaterial.albedo));

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)submesh.indexOffset);
            }
        }

        //REFRACTION
        {
            glBindFramebuffer(GL_FRAMEBUFFER, app->wFboRefract);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glEnable(GL_DEPTH_TEST);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glEnable(GL_CLIP_DISTANCE0);

            glClearColor(0.2f, 0.2f, 0.2f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Program& texturedMeshProgram = app->programs[app->baseModelProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            Model& model = app->models[app->island];
            Mesh& mesh = app->meshes[model.meshIdx];

            glm::mat4 viewMat = app->camera.GetViewMatrix({ app->displaySize.x, app->displaySize.y });
            glUniformMatrix4fv(app->BaseModelProgramIdx_uViewProjection, 1, GL_FALSE, glm::value_ptr(viewMat));

            glUniform4f(app->BaseModelProgramIdx_uPlane, 0.f, -1.f, 0.f, app->water.pos.y);

            for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshmaterial = app->materials[submeshMaterialIdx];
                glUniform3fv(app->BaseModelProgramIdx_uFaceColor, 1, glm::value_ptr(submeshmaterial.albedo));

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)submesh.indexOffset);
            }
        }

        //BASE
        {
            glBindFramebuffer(GL_FRAMEBUFFER, app->wFboBase);
            glDisable(GL_CLIP_DISTANCE0);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glEnable(GL_DEPTH_TEST);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            glClearColor(0.2f, 0.2f, 0.2f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 viewMat = app->camera.GetViewMatrix({ app->displaySize.x, app->displaySize.y });
            glUniformMatrix4fv(app->BaseModelProgramIdx_uViewProjection, 1, GL_FALSE, glm::value_ptr(viewMat));

            Model& model = app->models[app->island];
            Mesh& mesh = app->meshes[model.meshIdx];
            Program& texturedMeshProgram = app->programs[app->baseModelProgramIdx];

            for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshmaterial = app->materials[submeshMaterialIdx];
                glUniform3fv(app->BaseModelProgramIdx_uFaceColor, 1, glm::value_ptr(submeshmaterial.albedo));

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)submesh.indexOffset);
            }

            //WATER
            glUseProgram(app->programs[app->waterProgramIdx].handle);

            glUniformMatrix4fv(app->WaterProgramIdx_uViewProjection, 1, GL_FALSE, glm::value_ptr(viewMat));
            glUniformMatrix4fv(app->WaterProgramIdx_uModelMatrix, 1, GL_FALSE, glm::value_ptr(app->water.mat));

            app->wMove += app->wMoveSpeed * app->deltaTime;
            glUniform1f(app->WaterProgramIdx_uMoveFactor, app->wMove);
            glUniform3fv(app->WaterProgramIdx_uCameraPos, 1, glm::value_ptr(app->camera.pos));
            glUniform3fv(app->WaterProgramIdx_uLightPos, 1, glm::value_ptr(app->wLigthPos));
            glUniform3fv(app->WaterProgramIdx_uLightColor, 1, glm::value_ptr(app->wLigthColor));
            glUniform1f(app->WaterProgramIdx_uShineDamper, app->wuShineDamper);
            glUniform1f(app->WaterProgramIdx_uTiling, app->tiling);
            glUniform1f(app->WaterProgramIdx_uWaveStrength, app->wuWaveStrength);
            glUniform1f(app->WaterProgramIdx_uReflectivity, app->wuReflectivity);

            glUniform1i(app->WaterProgramIdx_uReflectionTex, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->wTexReflection);
            glUniform1i(app->WaterProgramIdx_uRefractionTex, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, app->wTexRefraction);
            glUniform1i(app->WaterProgramIdx_uDudvTex, 2);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, app->textures[app->wTexDudvSelected].handle);
            glUniform1i(app->WaterProgramIdx_uNormalMapTex, 3);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, app->textures[app->wTexNormalMap].handle);
            glUniform1i(app->WaterProgramIdx_uDepthMap, 4);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, app->wDepthRefraction);

            app->water.Render();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, NULL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
        glUseProgram(programTexturedGeometry.handle);

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(app->programUniformTexture, 0);
        glBindTexture(GL_TEXTURE_2D, app->wTexBase);

        renderQuad();
        break;
    }
    default:
        break;
    }
}

void WaterTile::Render() const
{
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;

    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f, 0.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
             1.0f, 0.0f,  1.0f, 1.0f, 1.0f,
             1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void renderQuad()
{
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;

    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void renderCube()
{
    static unsigned int cubeVAO = 0;
    static unsigned int cubeVBO = 0;
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderSphere()
{
    static unsigned int sphereVAO = 0;
    static unsigned int indexCount;

    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        float stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

void CheckOpenGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    std::string _source;
    std::string _type;
    std::string _severity;

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        _source = "API";
        break;

    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        _source = "WINDOW SYSTEM";
        break;

    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        _source = "SHADER COMPILER";
        break;

    case GL_DEBUG_SOURCE_THIRD_PARTY:
        _source = "THIRD PARTY";
        break;

    case GL_DEBUG_SOURCE_APPLICATION:
        _source = "APPLICATION";
        break;

    case GL_DEBUG_SOURCE_OTHER:
        _source = "UNKNOWN";
        break;

    default:
        _source = "UNKNOWN";
        break;
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        _type = "ERROR";
        break;

    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        _type = "DEPRECATED BEHAVIOR";
        break;

    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        _type = "UDEFINED BEHAVIOR";
        break;

    case GL_DEBUG_TYPE_PORTABILITY:
        _type = "PORTABILITY";
        break;

    case GL_DEBUG_TYPE_PERFORMANCE:
        _type = "PERFORMANCE";
        break;

    case GL_DEBUG_TYPE_OTHER:
        _type = "OTHER";
        break;

    case GL_DEBUG_TYPE_MARKER:
        _type = "MARKER";
        break;

    default:
        _type = "UNKNOWN";
        break;
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        _severity = "HIGH";
        break;

    case GL_DEBUG_SEVERITY_MEDIUM:
        _severity = "MEDIUM";
        break;

    case GL_DEBUG_SEVERITY_LOW:
        _severity = "LOW";
        break;

    case GL_DEBUG_SEVERITY_NOTIFICATION:
        _severity = "NOTIFICATION";
        break;

    default:
        _severity = "UNKNOWN";
        break;
    }

    printf("%d: %s of %s severity, raised from %s: %s\n",
        id, _type.c_str(), _severity.c_str(), _source.c_str(), message);
}

std::string FrameBufferToString(FrameBuffer fb)
{
    switch (fb)
    {
    case FrameBuffer::Framebuffer:
        return "Framebuffer";
    case FrameBuffer::FinalRender:
        return "Final Render";
    case FrameBuffer::Albedo:
        return "Albedo";
    case FrameBuffer::Normals:
        return "Normals";
    case FrameBuffer::Light:
        return "Lights";
    case FrameBuffer::Depth:
        return "Depth";
    }
    return std::string("Unknown");
}
