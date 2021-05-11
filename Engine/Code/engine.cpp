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
#include <iostream>

#include "assimp_model_loading.h"

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

GLuint CreateTexture2DFromImage(Image image)
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;
    
    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
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
    // TODO: Initialize your resources here!
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

    // - programs (and retrieve uniform indices)
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    app->texturedMeshProgramIdx_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");
    app->texturedMeshProgramIdx_uViewProjection = glGetUniformLocation(texturedMeshProgram.handle, "uWorldViewProjectionMatrix");
    app->texturedMeshProgramIdx_uWorldMatrix = glGetUniformLocation(texturedMeshProgram.handle, "uWorldMatrix");
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0, 3 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 1, 3 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2, 2 });
    

    // - textures
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    u32 pat = LoadModel(app, "Patrick/Patrick.obj");

    app->entities.push_back(Entity(glm::mat4(1.f), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(5.f, 0.f, -4.f)), pat));
    app->entities.push_back(Entity(glm::translate(glm::mat4(1.f), vec3(-5.f, 0.f, -2.f)), pat));

    app->lights.push_back(Light(LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.f, -10.f, 0.f)));

    app->mode = Mode::Mode_Patrick;

    //Framebuffer
    glGenTextures(1, &app->colorAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

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

    glDrawBuffers(1, &app->colorAttachmentHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::Text("ImGui Version: %s", ImGui::GetVersion());

    ImGui::Separator();

    ImGui::Text("OpenGL Info");
    ImGui::Text((const char*)glGetString(GL_VENDOR));
    ImGui::Text((const char*)glGetString(GL_RENDERER));
    ImGui::Text((const char*)glGetString(GL_VERSION));
    ImGui::Text((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    ImGui::Separator();

    ImGui::Text("Camera");
    ImGui::DragFloat("DistanceToOrigin", &app->camera.distanceToOrigin, 0.15f);
    ImGui::SliderFloat("phi", &app->camera.phi, 0.1f, 179.f, "%.1f");
    ImGui::DragFloat("theta", &app->camera.theta);

    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    if (app->input.mouseButtons[0] == ButtonState::BUTTON_PRESSED) {
        app->camera.theta   += app->input.mouseDelta.x * app->deltaTime * 20.f;
        app->camera.phi     -= app->input.mouseDelta.y * app->deltaTime * 20.f;
        app->camera.phi      = std::max(0.1f, std::min(app->camera.phi, 179.9f));
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
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // - set the viewport
        glViewport(0, 0, app->displaySize.x, app->displaySize.y);

        // - set the blending state
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
    switch (app->mode)
    {
    case Mode_TexturedQuad:
    {
        // TODO: Draw your textured quad here!

        // - bind the texture into unit 0
        glUniform1i(app->programUniformTexture, 0);
        glActiveTexture(GL_TEXTURE0);
        GLuint textureHandle = app->textures[app->diceTexIdx].handle;
        glBindTexture(GL_TEXTURE_2D, textureHandle);

        // - bind the program
        //   (...and make its texture sample from unit 0)
        const Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
        glUseProgram(programTexturedGeometry.handle);

        // - bind the vao
        glBindVertexArray(app->vao);

        // - glDrawElements() !!!
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindVertexArray(0);
        glUseProgram(0);
    }
    break;
    case Mode_Patrick: {
        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        glUseProgram(texturedMeshProgram.handle);

        for (auto& e : app->entities) {
            Model& model = app->models[e.model];
            Mesh& mesh = app->meshes[model.meshIdx];

            glUniformMatrix4fv(app->texturedMeshProgramIdx_uViewProjection, 1, GL_FALSE, &app->camera.GetViewMatrix({ app->displaySize.x, app->displaySize.y })[0][0]);

            glUniformMatrix4fv(app->texturedMeshProgramIdx_uWorldMatrix, 1, GL_FALSE, glm::value_ptr(e.mat));

            glUniform3f(glGetUniformLocation(texturedMeshProgram.handle, "uCameraPos"), app->camera.pos.x, app->camera.pos.y, app->camera.pos.z);
            //for (auto& light : app->lights)
            //{
                glUniform3f(glGetUniformLocation(texturedMeshProgram.handle, "uLightColor"), app->lights[0].color.x, app->lights[0].color.y, app->lights[0].color.z);
                glUniform3f(glGetUniformLocation(texturedMeshProgram.handle, "uLightPos"), app->lights[0].position.x, app->lights[0].position.y, app->lights[0].position.z);

            //}
                

            for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshmaterial = app->materials[submeshMaterialIdx];
                glActiveTexture(GL_TEXTURE);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshmaterial.albedoTextureIdx].handle);
                glUniform1i(app->texturedMeshProgramIdx_uTexture, 0);

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)submesh.indexOffset);
            }
        }
        break;
    }
    default:
        break;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, app->framebufferHandle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.y, 0, 0, app->displaySize.x, app->displaySize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void CheckOpenGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* userParam)
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
        id, _type.c_str(), _severity.c_str(), _source.c_str(), msg);
}
