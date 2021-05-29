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
	 float			linear;
	 float			quadratic;
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
	 float			linear;
	 float			quadratic;
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
uniform sampler2D normalMap;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oLight;
layout(location = 4) out vec4 oPosition;

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
	oNormals 	= vec4(vNormals, 1.0);
	oAlbedo		= texture(uAlbedoTexture, vTexCoord);
	oLight		= vec4(result, 1.0);
	oPosition   = vec4(vPos, 1.0);
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
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 20.1);
    vec3 specular = light.color * spec;
    // attenuati
    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * distance * distance);      
	return (diffuse + specular) * light.intensity * attenuation;
}

#endif
#endif

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
	unsigned int hasNormalMap;
};

out vec2 vTexCoord;
out vec3 vPos;
out vec3 vNormals;
out vec3 vViewDir;
out mat3 TBN;
out mat3 worldViewMatrix;

void main() {

	gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPos, 1.0);

	vPos = vec3(uWorldMatrix * vec4(aPos,1.0));

	vTexCoord = aTexCoord;
	
	vNormals = mat3(transpose(inverse(uWorldMatrix))) * aNormals;

	vec3 T = normalize(vec3(uWorldMatrix * vec4(aTangents,   0.0)));
    vec3 B = normalize(vec3(uWorldMatrix * vec4(aBiTangents, 0.0)));
    vec3 N = normalize(vec3(uWorldMatrix * vec4(vNormals,    0.0)));

    worldViewMatrix = mat3(uWorldMatrix);

	vViewDir = uCameraPos - vPos;

    TBN = mat3(T,B,N);
	

}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

vec2 reliefMapping(vec2 texCoords, vec3 viewDir);

layout(binding = 0, std140) uniform GlobalParms
{
	vec3 			uCameraPos;
	int 			uLightCount;
};

layout(binding = 1, std140) uniform LocalParms
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
	unsigned int hasNormalMap;
};

in vec2 vTexCoord;
in vec3 vPos;
in vec3 vNormals;
in vec3 vViewDir;
in mat3 TBN;
in mat3 worldViewMatrix;

uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oLight;
layout(location = 4) out vec4 oPosition;

void main() {
	vec3 normals = vec3(0.0);

	vec2 tCoords = vTexCoord;
	if(hasNormalMap == 1){
		
		tCoords = reliefMapping(tCoords, vViewDir);
//		if(tCoords.x >= 1.0 || tCoords.y >= 1.0 || tCoords.x <= 0.0 || tCoords.y <= 0.0)
//			tCoords = vTexCoord;

		normals = texture(uNormalTexture, tCoords).rgb;
        normals = normals * 2.0 - 1.0;
		normals = normalize(inverse(transpose(TBN)) * normals);

	} else{
		normals = normalize(vNormals);
	}


	oColor 		= texture(uAlbedoTexture, vTexCoord); //same as albedo
	oNormals 	= vec4(normals, 1.0);
	oAlbedo		= texture(uAlbedoTexture, tCoords);
	oLight		= vec4(1.0);
	oPosition   = vec4(transpose(TBN) * vPos, 1.0);
	gl_FragDepth = gl_FragCoord.z - 0.1;
}

// Parallax occlusion mapping aka. relief mapping
vec2 reliefMapping(vec2 texCoords, vec3 viewDir)
{
	int numSteps = 32;

	float bumpiness = 0.3;
	// Compute the view ray in texture space
	vec3 rayTexspace = inverse(TBN) * inverse(worldViewMatrix) * viewDir;
	// Increment
	vec3 rayIncrementTexspace;
	rayIncrementTexspace.xy = bumpiness * rayTexspace.xy / abs(rayTexspace.z * textureSize(uNormalTexture,0).x);
	rayIncrementTexspace.z = 1.0/numSteps;
	// Sampling state
	vec3 samplePositionTexspace = vec3(texCoords, 0.0);
	float sampledDepth = 1.0 - texture(uNormalTexture, samplePositionTexspace.xy).r;
	// Linear search
	for (int i = 0; i < numSteps && samplePositionTexspace.z < sampledDepth; ++i)
	{
		samplePositionTexspace += rayIncrementTexspace;
		sampledDepth = 1.0 - texture(uNormalTexture, samplePositionTexspace.xy).r;
	}
    // get depth after and before collision for linear interpolation
    float afterDepth  = samplePositionTexspace.z - sampledDepth;
    float beforeDepth = texture(uNormalTexture, samplePositionTexspace.xy).r - samplePositionTexspace.z + sampledDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = samplePositionTexspace.xy * weight + samplePositionTexspace.xy * (1.0 - weight);

    return finalTexCoords;
}

#endif
#endif


#ifdef SHOW_LIGHTS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoord;

struct Light{
	 unsigned int 	type; // 0: dir, 1: point
	 vec3			color;
	 vec3			direction;
	 vec3			position;
	 float 			intensity;
	 float			linear;
	 float			quadratic;
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

struct Light {
	 unsigned int 	type; // 0: dir, 1: point
	 vec3			color;
	 vec3			direction;
	 vec3			position;
	 float 			intensity;
	 float			linear;
	 float			quadratic;
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

uniform sampler2D uPositionTexture;
uniform sampler2D uNormalsTexture;
uniform sampler2D uAlbedoTexture;
uniform sampler2D uDepthTexture;

in vec2 vTexCoord;

layout(location = 0) out vec4 oColor;

void main() {

	vec3 fragPos = texture(uPositionTexture, vTexCoord).rgb;
	vec3 norms = texture(uNormalsTexture, vTexCoord).rgb;
	vec3 diffuseCol = texture(uAlbedoTexture, vTexCoord).rgb;
	float depth = texture(uDepthTexture, vTexCoord).r;

	vec3 viewDir = normalize(uCameraPos - fragPos);
	vec3 result = vec3(0.0,0.0,0.0);    
	
	for(int i = 0; i < uLightCount; ++i)
	{			
		if (depth < 1.0) {
			if (uLight[i].type == 0) {
				result += CalculateDirectionalLight(uLight[i], norms, viewDir, vTexCoord);
			}
			else if (uLight[i].intensity > 0) {
					result += CalculatePointLight(uLight[i], norms, fragPos, viewDir, vTexCoord);
			}
		}
		else {
			result = vec3(0.2);
		}
	}

	oColor = vec4(result, 1.0) * texture(uAlbedoTexture, vTexCoord);
}

vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 view_dir, vec2 texCoords) {
	vec3 ambient = light.color;
    // Diffuse
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

vec3 CalculatePointLight(Light light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec2 texCoords) {
    // Ambient
    vec3 ambient = light.color;
    // diffuse shadi
    vec3 lightDir = normalize(light.position - frag_pos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = ambient * diff;
     // Specul
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 20.1);
    vec3 specular = light.color * spec;
    // attenuati
    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * distance * distance);      
	return (diffuse + specular) * light.intensity * attenuation;
}

#endif
#endif



#ifdef DRAW_LIGHTS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoord;
layout(location=2) in vec3 aNormals;

uniform mat4 projectionView;

layout(binding = 0, std140) uniform LocalParms
{
	mat4 			model;
 	vec3 			lightColor;
};

void main() {
	gl_Position = projectionView * model * vec4(aPos, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(location = 0) out vec4 oColor;

layout(binding = 0, std140) uniform LocalParms
{
	mat4 			model;
 	vec3 			lightColor;
};

void main() {
	oColor = vec4(lightColor, 0.6);
		gl_FragDepth = gl_FragCoord.z - 0.1;

}

#endif
#endif

// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
