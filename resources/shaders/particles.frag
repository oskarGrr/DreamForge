#version 450

layout(location = 0) in vec4 fragColor;

layout( push_constant ) uniform constants
{
	float timePassed;
	uint  width;
	uint  height;
	float mousePosX;
	float mousePosY;
}Input;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = fragColor;
}