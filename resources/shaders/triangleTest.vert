#version 450

void main()
{
	//just for testing
	const vec2 positions[3] = vec2[3]
	(
	    vec2(0.0f, -0.5f), vec2(0.5f, 0.5f), vec2(-0.5f, 0.5f)
	);

    gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
}