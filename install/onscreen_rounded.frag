#version 140

#include "sdf.glsl"

uniform sampler2D texUnit;
uniform sampler2D sharpTex;
uniform vec2 halfpixel;
uniform vec4 box;
uniform vec4 cornerRadius;
uniform float opacity;
uniform float kind;
uniform float nbrCount;
uniform float bridgeMode;
uniform float nbrKind0;
uniform float nbrK;
uniform vec4 nbrBox0;
uniform vec4 nbrRad0;
uniform vec4 nbrBox1;
uniform vec4 nbrRad1;

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
const float CORNER_CALM = 0.7;
const float CORNER_WASH = 0.7;

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

float smoothMin(float a, float b, float k)
{
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * 0.25;
}

float nbrSdf(vec2 q, vec4 bx, vec4 rd, float rlMin)
{
    vec4 rr = rd;
    if (rlMin > 0.0) {
        float m = min(max(max(max(rd.x, rd.y), max(rd.z, rd.w)), rlMin), min(bx.z, bx.w));
        rr = vec4(m);
    }
    return sdfRoundedBox(q, bx.xy, bx.zw, rr);
}

float unionField(vec2 q, float base, float rlMin)
{
    float u = base;
    vec2 lo = box.xy - box.zw;
    vec2 hi = box.xy + box.zw;
    if (nbrCount > 0.5) {
        u = smoothMin(u, nbrSdf(q, nbrBox0, nbrRad0, rlMin), nbrK);
        lo = min(lo, nbrBox0.xy - nbrBox0.zw);
        hi = max(hi, nbrBox0.xy + nbrBox0.zw);
    }
    if (nbrCount > 1.5) {
        u = smoothMin(u, nbrSdf(q, nbrBox1, nbrRad1, rlMin), nbrK);
        lo = min(lo, nbrBox1.xy - nbrBox1.zw);
        hi = max(hi, nbrBox1.xy + nbrBox1.zw);
    }
    if (nbrCount > 0.5) {
        float fbb = sdfRoundedBox(q, 0.5 * (lo + hi), 0.5 * (hi - lo), vec4(0.0));
        u = -smoothMin(-u, -fbb, 0.5);
    }
    return u;
}

void main(void)
{
    vec2 p = vertex - box.xy;
    float fOwnB = sdfRoundedBox(vertex, box.xy, box.zw, cornerRadius);
    float f = unionField(vertex, fOwnB, 0.0);
    float df = fwidth(f);
    float r = p.x > 0.0
        ? (p.y < 0.0 ? cornerRadius.y : cornerRadius.w)
        : (p.y < 0.0 ? cornerRadius.x : cornerRadius.z);
    vec2 n = edgeNormal(p, box.zw, r);
    if (nbrCount > 0.5 && abs(f) < 6.0) {
        vec2 gx = vec2(1.0, 0.0);
        vec2 gy = vec2(0.0, 1.0);
        float fxp = unionField(vertex + gx, sdfRoundedBox(vertex + gx, box.xy, box.zw, cornerRadius), 0.0);
        float fxm = unionField(vertex - gx, sdfRoundedBox(vertex - gx, box.xy, box.zw, cornerRadius), 0.0);
        float fyp = unionField(vertex + gy, sdfRoundedBox(vertex + gy, box.xy, box.zw, cornerRadius), 0.0);
        float fym = unionField(vertex - gy, sdfRoundedBox(vertex - gy, box.xy, box.zw, cornerRadius), 0.0);
        n = normalize(vec2(fxp - fxm, fyp - fym) + vec2(0.00001));
    }

    float bandE = min(BAND, 0.8 * min(box.z, box.w));
    float lensE = LENS_PX * bandE / BAND;
    float rl = min(max(r, 0.8 * bandE), min(box.z, box.w));
    float fl = unionField(vertex, sdfRoundedBox(vertex, box.xy, box.zw, vec4(rl)), rl);
    vec2 nl = edgeNormal(p, box.zw, rl);
    if (nbrCount > 0.5 && abs(fl) < bandE + 2.0) {
        vec4 rq = vec4(rl);
        float lxp = unionField(vertex + vec2(1.0, 0.0), sdfRoundedBox(vertex + vec2(1.0, 0.0), box.xy, box.zw, rq), rl);
        float lxm = unionField(vertex - vec2(1.0, 0.0), sdfRoundedBox(vertex - vec2(1.0, 0.0), box.xy, box.zw, rq), rl);
        float lyp = unionField(vertex + vec2(0.0, 1.0), sdfRoundedBox(vertex + vec2(0.0, 1.0), box.xy, box.zw, rq), rl);
        float lym = unionField(vertex - vec2(0.0, 1.0), sdfRoundedBox(vertex - vec2(0.0, 1.0), box.xy, box.zw, rq), rl);
        vec2 gl2 = vec2(lxp - lxm, lyp - lym);
        if (dot(gl2, gl2) > 1.0e-6) {
            nl = normalize(gl2);
        }
    }
    float t = clamp(-fl / bandE, 0.0, 1.0);
    float k = pow(1.0 - t, 1.6);
    float dcv = length(box.zw - abs(p));
    float cornerC = 1.0 - smoothstep(0.8 * rl, 2.4 * rl, dcv);
    float cornerK = 1.0 - CORNER_CALM * cornerC;
    float lensC = lensE * (1.0 - CORNER_WASH * cornerC);
    float driftC = DRIFT * cornerK;
    float washC = CORNER_WASH * cornerC * k;

    vec2 inward = -nl * vec2(halfpixel.x, -halfpixel.y);
    float cr = textureLod(sharpTex, uv + inward * (lensC * k * (1.0 - driftC)), 0.0).r;
    float cg = textureLod(sharpTex, uv + inward * (lensC * k), 0.0).g;
    float cb = textureLod(sharpTex, uv + inward * (lensC * k * (1.0 + driftC)), 0.0).b;
    vec3 col = vec3(cr, cg, cb);

    vec3 soft = textureLod(texUnit, uv, 0.0).rgb;
    float isMenu = step(0.5, kind) * (1.0 - step(1.5, kind));
    float fN0 = nbrCount > 0.5 ? nbrSdf(vertex, nbrBox0, nbrRad0, 0.0) : 1.0e4;
    if (bridgeMode > 0.5) {
        float isMenuN = step(0.5, nbrKind0) * (1.0 - step(1.5, nbrKind0));
        isMenu = mix(isMenu, isMenuN, smoothstep(-0.5 * nbrK, 0.5 * nbrK, fOwnB - fN0));
    }
    if (bridgeMode < 0.5 && nbrCount > 0.5) {
        float isMenuW = step(0.5, nbrKind0) * (1.0 - step(1.5, nbrKind0));
        isMenu = mix(isMenu, isMenuW, 0.5 * (1.0 - smoothstep(0.0, 0.5 * nbrK, fN0)));
    }
    col = mix(col, soft, mix(mix(FROST, MENU_FROST, isMenu), 1.0, washC));
    col *= 1.0 - MENU_DIM * isMenu;

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, SAT);

    vec2 q = p / max(box.zw, vec2(1.0));
    vec3 wash = vec3(1.00, 0.30, 0.70) * exp(-dot(q - vec2(-0.6, -0.5), q - vec2(-0.6, -0.5)) / 0.8);
    wash += vec3(0.20, 0.90, 1.00) * exp(-dot(q - vec2(0.7, -0.2), q - vec2(0.7, -0.2)) / 0.8);
    wash += vec3(0.60, 0.35, 1.00) * exp(-dot(q - vec2(0.1, 0.8), q - vec2(0.1, 0.8)) / 0.8);

    vec3 rimCol = mix(col, soft, 0.6) + wash * RIM_TINT;
    col = mix(col, rimCol, RIM_FROST * k * (1.0 - 0.5 * CORNER_CALM * cornerC));
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
    vec3 prism = vec3(gauss(f + 1.2), gauss(f + 2.0), gauss(f + 2.8)) * fa * 0.38 * cornerK;
    float top = 0.05 * (1.0 - smoothstep(-box.w, box.w * 0.2, p.y));
    vec3 add = vec3(spec + refl + meniscus + top * (1.0 - k)) + prism;

    float h = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    col += vec3((h - 0.5) * DITHER / 255.0);

    fragColor = vec4(col + add, 1.0) * opacity;
    fragColor *= 1.0 - clamp(0.5 + f / df, 0.0, 1.0);
    if (bridgeMode > 0.5) {
        float outsideBoth = smoothstep(-0.5, 0.5, fOwnB) * smoothstep(-0.5, 0.5, fN0);
        float closer = 1.0 - smoothstep(-0.5, 0.5, fOwnB - fN0);
        fragColor *= outsideBoth * closer;
    }
}
