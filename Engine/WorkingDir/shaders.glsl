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

const int MAX_LIGHTS = 4;

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3	color;
	 vec3	direction;
	 vec3	position;
};

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 	uCameraPos;
	int 	uLightCount;
	Light	uLight[4];
};

layout(binding = 1, std140) uniform LocalParms
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vNormals;
out vec3 vViewDir;


void main() {

	gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPos, 1.0);
	vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
	vTexCoord = aTexCoord;

	vViewDir = uCameraPos - aPos;

}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

//---------------------------Function declaration--------------------------------------
vec3 CalculateDirectionalLight(vec3 lightPos, vec3 lightColor, vec3 normal, vec3 view_dir, vec2 texCoords);

#define MAX_LIGHTS 20;

 struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3	color;
	 vec3	direction;
	 vec3	position;
};


layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
	int 			uLightCount;
	Light			uLight[4];
};

in vec2 vTexCoord;
in vec3 vNormals;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main() {

	vec3 result = vec3(0.0,0.0,0.0);
	for(int i = 0; i < uLightCount; ++i)
	{			
			result += CalculateDirectionalLight(uLight[0].position, uLight[0].color, normalize(vNormals), normalize(vViewDir), vTexCoord);
	}
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
