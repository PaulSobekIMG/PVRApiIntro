#version 450

layout (location = 0) out lowp vec4    oColour;

void main()
{



        oColour = vec4((gl_FragCoord.x)/800.0, (gl_FragCoord.y)/600.0, 1.0, 1.0);


}



