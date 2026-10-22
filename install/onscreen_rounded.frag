#version 140

#include "sdf.glsl"

uniform sampler2D texUnit;
uniform sampler2D sharpTex;
uniform vec2 halfpixel;
uniform vec4 box;
uniform vec4 cornerRadius;
uniform float opacity;
uniform float kind;

in vec2 uv;
in vec2 vertex;

out vec4 fragColor;

const float BAND = 40.0;
const float LENS_PX = 24.0;
const float DRIFT = 0.12;
const float FROST = 0.55;
const float SAT = 1.30;
const float RIM_FROST = 0.70;
const float RIM_TINT = 0.22;
const float WASH_BASE = 0.04;
const float DITHER = 1.0;
const float MENU_FROST = 0.55;
const float MENU_WASH = 0.03;
const float MENU_DIM = 0.15;
const float TOP_SPEC = 0.80;
const vec2 LIGHT = vec2(-0.45, -1.0);

float gauss(float x)
{
    return exp(-x * x);
}

vec2 edgeNormal(vec2 p, vec2 hs, float r)
{
    vec2 qv = abs(p) - hs + vec2(r);
    vec2 qm = max(qv, vec2(0.0));
    vec2 sg = vec2(p.x >= 0.0 ? 1.0 : -1.0, p.y >= 0.0 ? 1.0 : -1.0);
    if (length(qm) > 0.001) {
        return normalize(qm * sg);
    }
    return (qv.x > qv.y) ? vec2(sg.x, 0.0) : vec2(0.0, sg.y);
}

void main(void)
{
    vec2 p = vertex - box.xy;
    float f = sdfRoundedBox(vertex, box.xy, box.zw, cornerRadius);
    float df = fwidth(f);
    float r = p.x > 0.0
        ? (p.y < 0.0 ? cornerRadius.y : cornerRadius.w)
        : (p.y < 0.0 ? cornerRadius.x : cornerRadius.z);
    vec2 n = edgeNormal(p, box.zw, r);

    float bandE = min(BAND, 0.8 * min(box.z, box.w));
    float lensE = LENS_PX * bandE / BAND;
    float rl = min(max(r, 0.8 * bandE), min(box.z, box.w));
    float fl = sdfRoundedBox(vertex, box.xy, box.zw, vec4(rl));
    vec2 nl = edgeNormal(p, box.zw, rl);
    float t = clamp(-fl / bandE, 0.0, 1.0);
    float k = pow(1.0 - t, 1.6);

    vec2 inward = -nl * vec2(halfpixel.x, -halfpixel.y);
    float cr = textureLod(sharpTex, uv + inward * (lensE * k * (1.0 - DRIFT)), 0.0).r;
    float cg = textureLod(sharpTex, uv + inward * (lensE * k), 0.0).g;
    float cb = textureLod(sharpTex, uv + inward * (lensE * k * (1.0 + DRIFT)), 0.0).b;
    vec3 col = vec3(cr, cg, cb);

    vec3 soft = textureLod(texUnit, uv, 0.0).rgb;
    float isMenu = step(0.5, kind) * (1.0 - step(1.5, kind));
    col = mix(col, soft, mix(FROST, MENU_FROST, isMenu));
    col *= 1.0 - MENU_DIM * isMenu;

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, SAT);

    vec2 q = p / max(box.zw, vec2(1.0));
    vec3 wash = vec3(1.00, 0.30, 0.70) * exp(-dot(q - vec2(-0.6, -0.5), q - vec2(-0.6, -0.5)) / 0.8);
    wash += vec3(0.20, 0.90, 1.00) * exp(-dot(q - vec2(0.7, -0.2), q - vec2(0.7, -0.2)) / 0.8);
    wash += vec3(0.60, 0.35, 1.00) * exp(-dot(q - vec2(0.1, 0.8), q - vec2(0.1, 0.8)) / 0.8);

    vec3 rimCol = mix(col, soft, 0.6) + wash * RIM_TINT;
    col = mix(col, rimCol, RIM_FROST * k);
    col += wash * mix(WASH_BASE, MENU_WASH, isMenu);

    float l = dot(n, normalize(LIGHT));
    float tl = pow(max(l, 0.0), 2.0);
    float brr = pow(max(-l, 0.0), 2.0);
    float line = gauss(f + 1.0);
    float spec = line * (0.04 + TOP_SPEC * tl) + line * brr * 0.22;
    float line2 = gauss((f + 3.0) / 1.2);
    float refl = line2 * brr * 0.45;
    float meniscus = gauss((f + 4.0) / 3.5) * tl * 0.10;
    float fa = (0.35 + 0.45 * abs(l)) * 0.55;
    vec3 prism = vec3(gauss(f + 1.2), gauss(f + 2.0), gauss(f + 2.8)) * fa * 0.38;
    float top = 0.05 * (1.0 - smoothstep(-box.w, box.w * 0.2, p.y));
    vec3 add = vec3(spec + refl + meniscus + top * (1.0 - k)) + prism;

    float h = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    col += vec3((h - 0.5) * DITHER / 255.0);

    fragColor = vec4(col + add, 1.0) * opacity;
    fragColor *= 1.0 - clamp(0.5 + f / df, 0.0, 1.0);
}
