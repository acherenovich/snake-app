uniform vec4 baseColor;
uniform float time;
uniform float pulseSpeed;

void main()
{
    vec2 uv = gl_TexCoord[0].xy - vec2(0.5);
    float dist = length(uv);

    float intensity = 1.0 - smoothstep(0.0, 0.5, dist);
    float minIntensity = 0.7;
    float pulse = minIntensity + (1.0 - minIntensity) * (0.5 + 0.5 * sin(time * pulseSpeed));
    intensity *= pulse;

    float alpha = 0.5 + 0.5 * sin(time * (pulseSpeed / 2.0));
    alpha = clamp(alpha, 0.5, 1.0);

    vec4 finalColor = baseColor * intensity;
    finalColor.a = alpha * intensity;

    gl_FragColor = finalColor;
}