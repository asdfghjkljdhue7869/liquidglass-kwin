#version 140

#include "sdf.glsl"

uniform sampler2D texUnit;
uniform sampler2D sharpTex;
uniform vec4 boxA;
uniform vec4 radA;
uniform vec4 boxB;
uniform vec4 radB;
uniform float kBlend;
uniform float dimA;
uniform float dimB;
uniform float opacity;

in vec2 uv;
in vec2 vertex;

out vec4 fragColor;

const float N_FROST = 0.62;
const float N_TINT = 0.12;
const float N_WASH = 0.02;
const float N_DIMK = 0.0;

float smoothMin(float a, float b, float k)
{
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * 0.25;
}

float unionSdf(vec2 p)
{
    float u = smoothMin(sdfRoundedBox(p, boxA.xy, boxA.zw, radA), sdfRoundedBox(p, boxB.xy, boxB.zw, radB), kBlend);
    vec2 lo = min(boxA.xy - boxA.zw, boxB.xy - boxB.zw);
    vec2 hi = max(boxA.xy + boxA.zw, boxB.xy + boxB.zw);
    float fbb = sdfRoundedBox(p, 0.5 * (lo + hi), 0.5 * (hi - lo), vec4(0.0));
    return -smoothMin(-u, -fbb, 6.0);
}

void main(void)
{
    float fa = sdfRoundedBox(vertex, boxA.xy, boxA.zw, radA);
    float fb = sdfRoundedBox(vertex, boxB.xy, boxB.zw, radB);
    float fu = unionSdf(vertex);
    float inUnion = 1.0 - clamp(0.5 + fu / max(fwidth(fu), 0.001), 0.0, 1.0);
    float a = inUnion * smoothstep(-0.5, 0.5, fa) * smoothstep(-0.5, 0.5, fb);
    if (a <= 0.0) {
        discard;
    }
    float xf = smoothstep(-0.5 * kBlend, 0.5 * kBlend, fa - fb);
    vec3 soft = textureLod(texUnit, uv, 0.0).rgb;
    vec3 sharp = textureLod(sharpTex, uv, 0.0).rgb;
    vec3 col = mix(sharp, soft, N_FROST);
    col *= 1.0 - N_DIMK * mix(dimA, dimB, xf);
    float k = pow(1.0 - clamp(-fu / 40.0, 0.0, 1.0), 1.6);
    vec4 bx = xf > 0.5 ? boxB : boxA;
    vec2 q = (vertex - bx.xy) / max(bx.zw, vec2(1.0));
    vec3 wash = vec3(1.00, 0.30, 0.70) * exp(-dot(q - vec2(-0.6, -0.5), q - vec2(-0.6, -0.5)) / 0.8);
    wash += vec3(0.20, 0.90, 1.00) * exp(-dot(q - vec2(0.7, -0.2), q - vec2(0.7, -0.2)) / 0.8);
    wash += vec3(0.60, 0.35, 1.00) * exp(-dot(q - vec2(0.1, 0.8), q - vec2(0.1, 0.8)) / 0.8);
    vec3 rimCol = mix(col, soft, 0.6) + wash * N_TINT;
    col = mix(col, rimCol, 0.70 * k);
    col += wash * N_WASH;
    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, 1.30);
    vec2 g = vec2(unionSdf(vertex + vec2(1.0, 0.0)) - unionSdf(vertex - vec2(1.0, 0.0)),
                  unionSdf(vertex + vec2(0.0, 1.0)) - unionSdf(vertex - vec2(0.0, 1.0)));
    vec2 n = normalize(g + vec2(0.00001));
    float l = dot(n, normalize(vec2(-0.45, -1.0)));
    float tl = pow(max(l, 0.0), 2.0);
    float brr = pow(max(-l, 0.0), 2.0);
    float line = exp(-(fu + 1.0) * (fu + 1.0));
    float spec = line * (0.04 + 0.80 * tl) + line * brr * 0.22;
    fragColor = vec4(col + vec3(spec), 1.0) * (opacity * a);
}

