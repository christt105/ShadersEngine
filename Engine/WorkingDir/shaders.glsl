///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPos, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main() {
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;

uniform mat4 uWorldMatrix;
uniform mat4 uWorldViewProjectionMatrix;

out vec2 vTexCoord;
out vec3 vNormals;

void main() {

	gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPos, 1.0);
	vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
	vTexCoord = aTexCoord;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vNormals;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main() {
	oColor = texture(uTexture, vTexCoord);
	//oColor = vec4(vNormals, 1.0);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
