


/* 
	Credit to:
    Created by Kishi
    Video URL: https://youtu.be/f4s1h2YETNY
    ShaderToy URL: https://www.shadertoy.com/view/mtyGWy
*/






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

//https://iquilezles.org/articles/palettes/
vec3 palette( float t ) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

void main() {
    vec2 uv = (fragCoord * 2.0 - iResolution.xy) / iResolution.y;
    vec2 uv0 = uv;
    vec3 finalColor = vec3(0.0);
    
    for (float i = 0.0; i < 4.0; i++) {
        uv = fract(uv * 1.5) - 0.5;

        float d = length(uv) * exp(-length(uv0));

        vec3 col = palette(length(uv0) + i*.4 + iTime*.4);

        d = sin(d*8. + iTime)/8.;
        d = abs(d);

        d = pow(0.01 / d, 1.2);

        finalColor += col * d;
    }
        
    fragColor = vec4(finalColor, 1.0);
}