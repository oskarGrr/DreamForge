#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D texSampler;

layout( push_constant ) uniform constants
{
	float DF_IncTime;
	uint DF_width;
	uint DF_height;
	float mousePosX;
	float mousePosY;
}Input;

void main()
{
	outColor = texture(texSampler, fragTexCoord);
}