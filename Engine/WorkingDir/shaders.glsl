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

uniform sampler2D uAlbedoTexture;

layout(location = 0) out vec4 oColor;

void main() {
	oColor = texture(uAlbedoTexture, vTexCoord);
}

#endif
#endif


#ifdef FORWARD_SHADING

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangents;
layout(location=4) in vec3 aBiTangents;


struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3			color;
	 vec3			direction;
	 vec3			position;
	 float 			intensity;
};

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
 	int 			uLightCount;
 	Light			uLight[16];
};

layout(binding = 1, std140) uniform LocalParms
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPos;
out vec3 vNormals;
out vec3 vViewDir;
out mat3 TBN;

void main() {

	gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPos, 1.0);

	vPos = vec3(uWorldMatrix * vec4(aPos,1.0));
	vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
	vTexCoord = aTexCoord;

	vViewDir = uCameraPos - aPos;

	vec3 T = normalize(vec3(uWorldMatrix * vec4(aTangents,   0.0)));
    vec3 B = normalize(vec3(uWorldMatrix * vec4(aBiTangents, 0.0)));
    vec3 N = normalize(vec3(uWorldMatrix * vec4(vNormals,    0.0)));

    TBN = mat3(T,B,N);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3			color;
	 vec3			direction;
	 vec3			position;
	 float 			intensity;

};


//---------------------------Function declaration--------------------------------------
vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir, vec2 texCoords);
vec3 CalculatePointLight(Light light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec2 texCoords);


layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
	int 			uLightCount;
	Light			uLight[16];
};

in vec2 vTexCoord;
in vec3 vPos;
in vec3 vNormals;
in vec3 vViewDir;
in mat3 TBN;

uniform sampler2D uAlbedoTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oLight;

void main() {

	vec3 result = vec3(0.0,0.0,0.0);

	vec3 normals = normalize(TBN * vNormals);

	for(int i = 0; i < uLightCount; ++i)
	{			
		if(uLight[i].type == 0)
			result += CalculateDirectionalLight(uLight[i], vNormals, normalize(vViewDir), vTexCoord);
		else{
			if(uLight[i].intensity > 0){
			result += CalculatePointLight(uLight[i], vNormals, vPos, normalize(vViewDir), vTexCoord);
			} 
		}
	}

	oColor 		= vec4(result,1.0) + texture(uAlbedoTexture, vTexCoord) * 0.2;
	oNormals 	= vec4(normals, 1.0);
	oAlbedo		= texture(uAlbedoTexture, vTexCoord);
	oLight		= vec4(result, 1.0);
	
	gl_FragDepth = gl_FragCoord.z - 0.1;
}


vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir, vec2 texCoords){
	vec3 ambient = light.color;
    // Diffuse
    //vec3 fake_lightDir = normalize(light.lightPos - frag_pos);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = ambient *  diff;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + view_dir); 
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 0.0) * 0.01;

    vec3 specular = vec3(0);
    specular = diffuse * spec;
    // Final Calculation     
    return (diffuse + specular) * light.intensity;
}

vec3 CalculatePointLight(Light light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec2 texCoords)
{
    // Ambient
    vec3 ambient = light.color;
    // diffuse shadi
    vec3 lightDir = normalize(light.position - frag_pos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = ambient * diff;
     // Specul
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(view_dir, reflectDir), 0.0), 0.0) * 0.01;//    vec3 specular = vec3(0);
    vec3 specular = ambient * spec;
    // attenuati
    float distance = length(light.position - frag_pos);
    float attenuation = 1/distance;      
	return (diffuse + specular) * attenuation;
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangents;
layout(location=4) in vec3 aBiTangents;




layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
 	int 			uLightCount;
};

layout(binding = 1, std140) uniform LocalParms
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPos;
out vec3 vNormals;
out vec3 vViewDir;
out mat3 TBN;

void main() {

	gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPos, 1.0);

	vPos = vec3(uWorldMatrix * vec4(aPos,1.0));
	vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
	vTexCoord = aTexCoord;

	vViewDir = uCameraPos - aPos;

	vec3 T = normalize(vec3(uWorldMatrix * vec4(aTangents,   0.0)));
    vec3 B = normalize(vec3(uWorldMatrix * vec4(aBiTangents, 0.0)));
    vec3 N = normalize(vec3(uWorldMatrix * vec4(vNormals,    0.0)));

    TBN = mat3(T,B,N);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3			color;
	 vec3			direction;
	 vec3			position;
	 float 			intensity;

};


//---------------------------Function declaration--------------------------------------
vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir, vec2 texCoords);
vec3 CalculatePointLight(Light light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec2 texCoords);


layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
	int 			uLightCount;
};

in vec2 vTexCoord;
in vec3 vPos;
in vec3 vNormals;
in vec3 vViewDir;
in mat3 TBN;

uniform sampler2D uAlbedoTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oLight;

void main() {

	vec3 normals = normalize(TBN * vNormals);

	oColor 		= texture(uAlbedoTexture, vTexCoord);
	oNormals 	= vec4(vNormals, 1.0);
	oAlbedo		= texture(uAlbedoTexture, vTexCoord);
	oLight		= vec4(1.0);
	
	gl_FragDepth = gl_FragCoord.z - 0.1;
}


#endif
#endif


#ifdef SHOW_LIGHTS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=2) in vec2 aTexCoord;

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3			color;
	 vec3			direction;
	 vec3			position;
	 float 			intensity;
};

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
 	int 			uLightCount;
 	Light			uLight[16];
};


out vec2 vTexCoord;

void main() {

	gl_Position = vec4(aPos, 1.0);

	vTexCoord = aTexCoord;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3			color;
	 vec3			direction;
	 vec3			position;
	 float 			intensity;

};

struct Textures{
 	sampler2D uAlbedoTexture;
 	sampler2D uPositionTexture;
 	sampler2D uNormalsTexture;
}
//---------------------------Function declaration--------------------------------------
vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir, vec2 texCoords);
vec3 CalculatePointLight(Light light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec2 texCoords);


layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
	int 			uLightCount;
	Light			uLight[16];
	Textures		texts;
};

in vec2 vTexCoord;

layout(location = 0) out vec4 oColor;

void main() {

	vec3 fragPos = texture(texts.uPositionTexture, vTexCoord).rgb;
	vec3 norms = texture(texts.uNormalsTexture, vTexCoord).rgb;
	vec3 diffuseCol = texture(texts.uAlbedoTexture, vTexCoord).rgb;

	vec3 viewDir = normalize(uCameraPos - fragPos);
	vec3 result = vec3(0.0,0.0,0.0);
	
	for(int i = 0; i < uLightCount; ++i)
	{			
		if(uLight[i].type == 0)
			result += CalculateDirectionalLight(uLight[i], norms, normalize(viewDir), vTexCoord);
		else{
			if(uLight[i].intensity > 0){
				result += CalculatePointLight(uLight[i], norms, fragPos, normalize(viewDir), vTexCoord);
			} 
		}
	}

	oColor 	= vec4(result,1.0) + diffuseCol * 0.2;

}


vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir, vec2 texCoords){
	vec3 ambient = light.color;
    // Diffuse
    //vec3 fake_lightDir = normalize(light.lightPos - frag_pos);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = ambient *  diff;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + view_dir); 
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 0.0) * 0.01;

    vec3 specular = vec3(0);
    specular = diffuse * spec;
    // Final Calculation     
    return (diffuse + specular) * light.intensity;
}

vec3 CalculatePointLight(Light light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec2 texCoords)
{
    // Ambient
    vec3 ambient = light.color;
    // diffuse shadi
    vec3 lightDir = normalize(light.position - frag_pos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = ambient * diff;
     // Specul
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(view_dir, reflectDir), 0.0), 0.0) * 0.01;//    vec3 specular = vec3(0);
    vec3 specular = ambient * spec;
    // attenuati
    float distance = length(light.position - frag_pos);
    float attenuation = 1/distance;      
	return (diffuse + specular) * attenuation;
}



#endif
#endif