#version 300 es

#if __VERSION__ > 300
layout(binding = 1)uniform sampler2D  sTexture;

layout (location = 3) in highp vec3   norms;
layout (location = 4) in highp vec2   TexCoord;

layout (location = 0) out lowp vec4    oColour;

#else
uniform sampler2D  sTexture;

in highp vec3   norms;
in highp vec2   TexCoord;

out lowp vec4    oColour;
#endif

void main()
{
        oColour = vec4(texture(sTexture,TexCoord))*gl_FragCoord.z;
}

