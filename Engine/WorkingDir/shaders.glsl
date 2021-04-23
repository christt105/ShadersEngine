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

const int MAX_LIGHTS = 20;

uniform mat4 uWorldMatrix;
uniform mat4 uWorldViewProjectionMatrix;

//uniform int uLightN;
uniform vec3 uCameraPos;

out vec2 vTexCoord;
out vec3 vNormals;
out vec3 vViewDir;

//out int vLightN;

void main() {

	gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPos, 1.0);
	vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
	vTexCoord = aTexCoord;

	vViewDir = uCameraPos - aPos;
	//-------------------------Lights-------------------------------------------
	//vLightN = uLightN;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

//---------------------------Function declaration--------------------------------------
vec3 CalculateDirectionalLight(vec3 lightPos, vec3 lightColor, vec3 normal, vec3 view_dir, vec2 texCoords);

in vec2 vTexCoord;
in vec3 vNormals;
//in int vLightN;
in vec3 vViewDir;

uniform sampler2D uTexture;
uniform vec3 uLightColor;
uniform vec3 uLightPos;

layout(location = 0) out vec4 oColor;

void main() {

	vec3 result = CalculateDirectionalLight(uLightPos, uLightColor, normalize(vNormals), normalize(vViewDir), vTexCoord);

	oColor =  vec4(result,1.0) * texture(uTexture, vTexCoord);
	//oColor = vec4(vNormals, 1.0);
}


vec3 CalculateDirectionalLight(vec3 lightPos, vec3 lightColor, vec3 normal, vec3 view_dir, vec2 texCoords){
	vec3 ambient = lightColor * 0.8;
    // Diffuse
    //vec3 fake_lightDir = normalize(light.lightPos - frag_pos);
    vec3 lightDir = normalize(-lightPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = ambient * 0.2 * diff;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + view_dir); 
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 0.0) * 0.1;

    vec3 specular = vec3(0);
    specular = ambient * 0.1 * spec;
    // Final Calculation     
    return ambient + diffuse + specular;
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
