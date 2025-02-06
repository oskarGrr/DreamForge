#version 450

layout (location = 0) out vec4 fragColor;

layout (location = 0) in vec3 inColor;

layout( push_constant ) uniform constants
{
	float DF_IncTime;
	uint DF_width;
	uint DF_height;
	float mousePosX;
	float mousePosY;
}Input;

vec2 iMouse = vec2(Input.mousePosX, Input.mousePosY);
vec2 iResolution = vec2(Input.DF_width, Input.DF_height);
float iTime = Input.DF_IncTime;
vec2 fragCoord = gl_FragCoord.xy;


void main()
{
	vec2 uv = fragCoord / iResolution;

	vec3 col = vec3(uv.x);

	fragColor = vec4(col, 1.0);
}