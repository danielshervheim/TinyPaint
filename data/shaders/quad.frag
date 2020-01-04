#version 150 core

out vec4 outColor;

in vec2 vUv;

uniform sampler2D uTexture;

void main()
{
    outColor = texture(uTexture, vUv);  // +vec4(vUv,0.0,0.0)*0.5;
    // outColor = vec4(vUv, 0.0, 1.0);
}
