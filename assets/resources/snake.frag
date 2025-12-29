uniform vec4 baseColor;
uniform float time;

void main()
{
    vec2 uv = gl_TexCoord[0].xy - vec2(0.5);
    float dist = length(uv);

    float intensity = smoothstep(0.5, 0.0, dist);
    float pulse = 0.9 + 0.1 * sin(time * 2.0);

    intensity *= pulse;

    vec4 texColor = baseColor;
    vec4 glow = vec4(texColor.rgb * intensity, texColor.a * intensity * 0.5);

    gl_FragColor = texColor + glow;
}