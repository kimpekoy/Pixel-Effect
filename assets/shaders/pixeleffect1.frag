#version 110

in vec3 Color;


void main()
{
	gl_FragColor.rgb = Color;
	gl_FragColor.a = 1.0;

}