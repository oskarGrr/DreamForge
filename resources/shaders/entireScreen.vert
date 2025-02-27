#version 450

layout(location = 0) out vec3 fragColor;

void main()
{
	const vec2 positions[6] = vec2[6]
	(
	    vec2(-1.0, -1.0), 
		vec2(1.0, -1.0), 
		vec2(-1.0, 1.0),

		vec2(1.0, -1.0),
		vec2(1.0, 1.0),
		vec2(-1.0, 1.0)
	);

    gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
	//fragColor = colors[gl_VertexIndex];
}