#version 150 core

out vec4 outColor;

in vec2 vUv;

uniform sampler2D uTexture;

void main()
{
    // Note that gl loads textures upside down, so we flip the y coordinate.
    outColor = texture(uTexture, vec2(vUv.x, 1.0-vUv.y));  // +vec4(vUv,0.0,0.0)*0.5;
    // outColor = vec4(vUv, 0.0, 1.0);
}
