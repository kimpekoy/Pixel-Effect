uniform vec3 inColor;
out vec3 Color;

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	Color = inColor;
}
