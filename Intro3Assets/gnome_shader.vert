#version 300 es


#if __VERSION__ > 300
layout (location = 0) in highp vec3    inVertex;
layout (location = 1) in highp vec3    inNormal;
layout (location = 2) in highp vec2    inUV;
layout (std140, binding = 0) uniform u {highp float scale;};
layout (location = 3) out highp vec3   norms;
layout(location = 4) out highp vec2 TexCoord;
#else
 in highp vec3    inVertex;
 in highp vec3    inNormal;
 in highp vec2    inUV;
uniform highp float scale;
out highp vec3   norms;
out highp vec2   TexCoord;

#endif

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main()
{
	highp vec4 invertex2 = rotationMatrix(
	#ifdef VULKAN
	vec3(0.0,0.0,1.0),radians(180.0) )*rotationMatrix(vec3(0.0,1.0,0.0), scale)*vec4(inVertex.xyz/47.0f,1.0);    
	#else
		vec3(0.0,0.0,1.0),radians(180.0) )*rotationMatrix(vec3(0.0,1.0,0.0), scale)*vec4(vec3(inVertex.x, -inVertex.y, inVertex.z)/47.0f,1.0);    
	#endif
	invertex2.z = invertex2.z+0.6f;
	#ifdef VULKAN
	invertex2.y = invertex2.y+0.8f;
	#else
	invertex2.y = invertex2.y-0.8f;
	#endif
	TexCoord = inUV;
	norms = vec3(0.0);
        gl_Position = invertex2;
}
