uniform sampler2D texture;
uniform vec2 direction; // (1/width, 0) or (0, 1/height)

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    // 9-tap gaussian-ish blur (stable + cheap)
    vec4 c = vec4(0.0);

    c += texture2D(texture, uv - 4.0 * direction) * 0.050;
    c += texture2D(texture, uv - 3.0 * direction) * 0.090;
    c += texture2D(texture, uv - 2.0 * direction) * 0.120;
    c += texture2D(texture, uv - 1.0 * direction) * 0.150;
    c += texture2D(texture, uv)                 * 0.180;
    c += texture2D(texture, uv + 1.0 * direction) * 0.150;
    c += texture2D(texture, uv + 2.0 * direction) * 0.120;
    c += texture2D(texture, uv + 3.0 * direction) * 0.090;
    c += texture2D(texture, uv + 4.0 * direction) * 0.050;

    gl_FragColor = c;
}