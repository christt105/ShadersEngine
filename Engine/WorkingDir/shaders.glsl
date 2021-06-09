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
out mat3 vworldMat;

void main() {

	gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPos, 1.0);

	vNormals = mat3(uWorldMatrix) * aNormals;
	vTexCoord = aTexCoord;


	vec3 T = normalize(vec3( uWorldMatrix * vec4(aTangents,   0.0)));
    vec3 B = normalize(vec3( uWorldMatrix * vec4(aBiTangents, 0.0)));
    vec3 N = normalize(vec3( uWorldMatrix * vec4(vNormals,    0.0)));

    TBN = transpose(mat3(T,B,N));
	
	vViewDir =  normalize((uCameraPos - aPos));
	vworldMat = mat3(uWorldMatrix);
	vPos = vec3(uWorldMatrix * vec4(aPos,1.0));
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
vec2 reliefMapping(vec2 texCoords, vec3 viewDir);

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
in mat3 vworldMat;

uniform sampler2D uAlbedoTexture;

uniform unsigned int uhasNormalMap;
uniform sampler2D uNormalTexture;

uniform unsigned int uhasBumpMap;
uniform sampler2D uBumpTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oLight;
layout(location = 4) out vec4 oPosition;

void main() {

	vec3 result = vec3(0.0,0.0,0.0);
	 vec2 tCoords = vTexCoord;

		tCoords = reliefMapping(tCoords, vViewDir);
		if(tCoords.x > 1.0 || tCoords.y > 1.0 || tCoords.x < 0.0 || tCoords.y < 0.0)
			discard;
	

	vec3 normals = vNormals;

		normals = normalize( texture(uNormalTexture, tCoords).rgb);
        normals = normals * 2.0 - 1.0;
		normals = normalize(inverse(TBN) * normals);

	

	for(int i = 0; i < uLightCount; ++i)
	{			
		if(uLight[i].type == 0)
			result += CalculateDirectionalLight(uLight[i], normals,  vViewDir, tCoords);
		else{
			if(uLight[i].intensity > 0){
			result += CalculatePointLight(uLight[i], normals, vPos, vViewDir, tCoords);
			} 
		}
	}

	oColor 		= vec4(result, 1.0) * texture(uAlbedoTexture, tCoords);
	oNormals 	= vec4(normals, 1.0);
	oAlbedo		= texture(uAlbedoTexture, tCoords);
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
    vec3 halfwayDir = normalize(lightDir + TBN * view_dir); 
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
    vec3 lightDir = normalize((TBN) * (light.position - frag_pos));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = ambient * diff;
     // Specul
    vec3 halfwayDir =normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 10.1);
    vec3 specular = light.color * spec;
    // attenuati
    float distance = length(TBN * (light.position - frag_pos));
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * distance * distance);      
	return (diffuse + specular + ambient) * light.intensity * attenuation;
}
vec2 reliefMapping(vec2 texCoords, vec3 viewDir)
{
	float bumpiness = 0.5;
	const float minLayers = 2.0;
	const float maxLayers = 32.0;
	viewDir = normalize(TBN * viewDir);
	
	float numLayers= mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;

    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * bumpiness; 
    vec2 deltaTexCoords = P / numLayers;

	vec2  currentTexCoords     = texCoords;
	float currentDepthMapValue = texture(uBumpTexture, currentTexCoords).r;
	  
	while(currentLayerDepth < currentDepthMapValue)
	{
	    // shift texture coordinates along direction of P
	    currentTexCoords -= deltaTexCoords;
	    // get depthmap value at current texture coordinates
	    currentDepthMapValue = texture(uBumpTexture, currentTexCoords).r;  
	    // get depth of next layer
	    currentLayerDepth += layerDepth;  
	}
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(uBumpTexture, prevTexCoords).r - currentLayerDepth + layerDepth;
 
	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;   
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
};

in vec2 vTexCoord;
in vec3 vPos;
in vec3 vNormals;
in vec3 vViewDir;
in mat3 TBN;
in mat3 worldViewMatrix;


uniform sampler2D uAlbedoTexture;

uniform unsigned int uhasNMap;
uniform sampler2D uNormalTexture;

uniform unsigned int uhasBumpMap;
uniform sampler2D uBumpTexture;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oLight;
layout(location = 4) out vec4 oPosition;

void main() {

	vec2 tCoords = vTexCoord;
	
	if(uhasBumpMap == 1){
		tCoords = reliefMapping(tCoords, vViewDir);
	}
	
	vec3 normals = vNormals;

	if(uhasNMap == 0){
		normals = texture(uNormalTexture, tCoords).rgb;
        normals = normals * 2.0 - 1.0;
		normals = normalize(inverse(transpose(TBN)) * normals);
	}


	oColor 		= texture(uAlbedoTexture, tCoords); //same as albedo
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

	float bumpiness = 25;
	// Compute the view ray in texture space
	vec3 rayTexspace = transpose(TBN) * inverse(worldViewMatrix) * viewDir;
	// Increment
	vec3 rayIncrementTexspace;
	rayIncrementTexspace.xy = bumpiness * rayTexspace.xy / abs(rayTexspace.z * textureSize(uBumpTexture,0).x);
	rayIncrementTexspace.z = 1.0/numSteps;
	// Sampling state
	vec3 samplePositionTexspace = vec3(texCoords, 0.0);
	float sampledDepth = 1.0 - texture(uBumpTexture, samplePositionTexspace.xy).r;
	// Linear search
	for (int i = 0; i < numSteps && samplePositionTexspace.z < sampledDepth; ++i)
	{
		samplePositionTexspace += rayIncrementTexspace;
		sampledDepth = 1.0 - texture(uBumpTexture, samplePositionTexspace.xy).r;
	}
    // get depth after and before collision for linear interpolation
    float afterDepth  = samplePositionTexspace.z - sampledDepth;
    float beforeDepth = texture(uBumpTexture, samplePositionTexspace.xy).r - samplePositionTexspace.z + sampledDepth;
 
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

#ifdef DRAW_BASE_MODEL

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormals;

uniform mat4 uWorldViewProjectionMatrix;

uniform vec4 plane = vec4(0.0, 1.0, 0.0, 1.0);

out vec3 FragPos;
out vec3 vNormals;

void main() {
	gl_Position = uWorldViewProjectionMatrix * vec4(aPos, 1.0);
	FragPos = aPos;
	gl_ClipDistance[0] = dot(vec4(aPos, 1.0), plane);
	vNormals = mat3(transpose(inverse(mat4(1.0)))) * aNormals;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(location = 0) out vec4 oColor;

in vec3 FragPos;
in vec3 vNormals;

uniform vec3 faceColor;

//move
uniform vec3 lightPos = vec3(5.0, 15.0, 5.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main() {
	// ambient
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;
	
	vec3 norm = normalize(vNormals);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	vec3 result = max(min((ambient + diffuse), 0.9), 0.3) * faceColor;

	oColor = vec4(result, 1.0);
}

#endif
#endif

#ifdef WATER_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoord;

uniform mat4 uWorldViewProjectionMatrix;
uniform mat4 uWorldMatrix;

uniform vec3 lightPos = vec3(5.0, 15.0, 5.0);
uniform vec3 cameraPos;

uniform float tiling = 15.0;

out vec4 clipSpace;
out vec2 vTexCoord;
out vec3 cameraVector;
out vec3 lightVector;

void main() {
	vec4 worldPosition = uWorldMatrix * vec4(aPos, 1.0);
	clipSpace = uWorldViewProjectionMatrix * worldPosition;
	gl_Position = clipSpace;
	vTexCoord = (aTexCoord / 2.0 + 0.5) * tiling;
	cameraVector = cameraPos - worldPosition.xyz;
	lightVector = worldPosition.xyz - lightPos;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(location = 0) out vec4 oColor;

in vec4 clipSpace;
in vec2 vTexCoord;
in vec3 cameraVector;
in vec3 lightVector;

uniform sampler2D reflectionTex;
uniform sampler2D refractionTex;
uniform sampler2D dudvMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;

uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);

uniform float move = 0.0;
uniform float waveStrength = 0.01;

uniform float shineDamper = 20.0;
uniform float reflectivity = 0.6;

void main() {

	vec2 ndc = (clipSpace.xy/clipSpace.w) / 2.0 + 0.5;
	vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);
	vec2 refractTexCoords = vec2(ndc.x,  ndc.y);

	float near = 0.1;
	float far = 1000.0;
	float depth = texture(depthMap, refractTexCoords).r;
	float floorDistance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));

	depth = gl_FragCoord.z;
	float waterDistance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));
	float waterDepth = floorDistance - waterDistance;

	vec2 distortedTexCoords = texture(dudvMap, vec2(vTexCoord.x + move, vTexCoord.y)).rg * 0.1;
	distortedTexCoords = vTexCoord + vec2(distortedTexCoords.x, distortedTexCoords.y + move);
	vec2 distortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength * clamp(waterDepth / 20.0, 0.0, 1.0);

	reflectTexCoords += distortion;
	reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
	reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);

	refractTexCoords += distortion;
	refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);

	vec4 reflectCol = texture(reflectionTex, reflectTexCoords);
	vec4 refractCol = texture(refractionTex, refractTexCoords);

	vec4 normalMapColor = texture(normalMap, distortedTexCoords);
	vec3 normal = normalize(vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b, normalMapColor.g * 2.0 - 1.0));

	vec3 viewVector = normalize(cameraVector);
	float refractiveFactor = dot(viewVector, vec3(0.0, 1.0, 0.0));
	refractiveFactor = pow(refractiveFactor, 10.0);

	vec3 reflectedLight = reflect(normalize(lightVector), normal);
	float specular = max(dot(reflectedLight, viewVector), 0.0);
	specular = pow(specular, shineDamper);
	vec3 specularHighlights = lightColor * specular * reflectivity * clamp(waterDepth / 5.0, 0.0, 1.0);

	oColor = mix(reflectCol, refractCol, refractiveFactor);
	oColor = mix(oColor, vec4(0.0, 1.0, 1.0, 1.0), 0.2) + vec4(specularHighlights, 0.0);
	oColor.a = clamp(waterDepth / 0.2, 0.0, 1.0);
}

#endif
#endif

// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
