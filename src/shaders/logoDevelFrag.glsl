#version 330 core

precision highp float;
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 extraOutput;

uniform float iTime;
uniform vec4 iRect;
uniform float iFPS;
uniform vec4 iMouse;
uniform int iFrame;
uniform int iPass;
uniform sampler2D iPreviousImage;
uniform sampler2D iBloomImage;

const int ONLY_LEDS_PASS = 0;
const int SCENE_PASS = 1;
const int POST_PASS = 2;
bool onlyLeds = iPass == ONLY_LEDS_PASS;

const int nLeds = 172;
const int minLogoLed = 64;
const int maxLogoLed = 167;

layout(std140) uniform TrophyDefinition {
    int _nLedsWouldNotWorkThatWay;
    vec4 ledPosition[nLeds];
};

struct RGB { // last value is only for alignment, never used.
    uint r, g, b, _;
};

layout(std140) uniform LogoStateBuffer {
    RGB ledColor[nLeds];
    vec2 _logoCenter;
    vec2 logoSize;
    int options;
};

// QM took these as great examples of their kind, might make the logoCenter above superfluid
vec2 logoCenter = vec2(ledPosition[127].x, ledPosition[131].y);
float freeScale = 1.3;

#define hasOption(index) (options & (1 << (8 * index))) != 0
bool maskOnlyPixels = hasOption(0);
bool hidePixels = hasOption(1);
bool hideCrosshair = hasOption(2);
bool hideOrigin = hasOption(3);

vec2 iResolution = iRect.zw;
float aspectRatio = iResolution.x / iResolution.y;

const float pi = 3.14159265;
const float rad = pi / 180.;
const float tau = 2. * pi;
const vec4 c = vec4(1., 0., -1., .5);
#define clampVec3(x) clamp(x, vec3(0), vec3(1))
const vec2 epsilon = c.xz * 0.0005;

// red border to distinguish... also, because we're so evil
const vec3 borderDark = vec3(0.4, 0.2, 0.2);
const vec3 borderLight = vec3(0.8, 0.3, 0.3);

vec3 to_vec(RGB rgb) {
    return vec3(rgb.r, rgb.g, rgb.b) / 255.;
}

mat2 rot2D(float angle) {
    float c = cos(2*pi*angle);
    float s = sin(2*pi*angle);
    return mat2(c, -s, +s, c);
}

float sdfCircle(vec2 p, vec2 center, float radius) {
    return length(p - center) - radius;
}

float sdfMouseCrosshair() {
    vec2 dist = abs(gl_FragCoord.xy - iMouse.xy - c.ww);
    return exp(-5. * min(dist.x, dist.y));
}

float sdTriangleFrame(vec2 p, float w, float h) {
    p.x = abs(p.x);
    float m = h * w / sqrt(w*w + 4.0*h*h);
    float hypoDist = max((p.x * h + p.y * w) / sqrt(w*w + h*h), -p.y *w/h);
    float d = hypoDist;
    return abs(d - 1.);
}

float sdLine(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);
    return length(pa - ba * h);
}

float sdBar(vec2 p, vec2 a, vec2 b) {
    // fat and overlong version of the sdLine
    vec2 to = b - a;
    vec2 ortho = cross(vec3(to.x, to.y, 0), c.yyx).xy;
    float d = 1e6;
    for (float shift = -0.02; shift <= 0.02; shift+=0.01) {
        d = min(d, sdLine(p + shift * ortho, a - 100. * to, b + 100 * to));
    }
    return d;
}

float slideIn(float t, float duration, float stay) {
    float d2 = duration/2;
    // center time around [-dur/2, +dur/2], starting at -dur/2;
    t = mod(t, duration) - d2;
    //return sin(2.*pi/duration * t); // super simple first test
    // return -2 * (smoothstep(-duration, 0, t) + smoothstep(0, duration, t) - 1.);
    return -2 * (smoothstep(-duration, 0, t + stay/2) + smoothstep(0, duration, t - stay/2) - 1.);
}

float possin(float x) {
    return 0.5 + 0.5 * sin(x);
}

float flickers(float t, float offset, float threshold) {
    // just some amalgam of trigonometric functions to look a _bit_ less predictable
    float r = possin(t)
    * possin(0.3 * t - offset)
    * possin(3.1 * t - 0.1 + 0.3 * offset)
    * possin(2.86 * t - 0.15 + 7. * offset)
    * possin(9.11 * t - offset * t);
    return sqrt(sqrt(r)) > threshold ? 1. : 0.;
}

void main() {
    vec2 uv = (2. * (gl_FragCoord.xy - iRect.xy) - iResolution) / iResolution.y;

    extraOutput.x = 0.;   // signals us that we are in the frame
    extraOutput.y = 0.;
//    extraOutput.zw = uv;

    // border frame
    float uvX = uv.x / aspectRatio;
    if (max(abs(uvX), abs(uv.y)) > 0.993) {
        if (abs(uvX) >= abs(uv.y)) { // vertical frame?
            fragColor.rgb = uvX < 0 ? borderDark : borderLight;
        } else {
            fragColor.rgb = uv.y > 0 ? borderDark : borderLight;
        }
        return;
    }

    uv /= freeScale;

    extraOutput.x = 1.;   // signals us that we are in the frame
    extraOutput.y = -1.;  // means "no LED index" (cf. below)
    fragColor = c.yyyx;
    float d;

    // background: trying out sweet sweet effects
    /*
    d = sdTriangleFrame(16. * (uv - vec2(-0., -0.03)), 0.01, 0.015);
    fragColor.r = 0.3 * exp(-10.*(abs(d - 1.) * fract(-0.3 * iTime)));
    */
    vec3 col = fragColor.rgb;

    float slideLoopSec = 4.;
    float slideY = 9. * slideIn(iTime, slideLoopSec, 0.7);

    // when the big logo is around, less of the silly beam please
    float suppress = smoothstep(0.5, 1, abs(2.1 * slideY));

    // and combining some sweet sweet lines (not what you think!)
    //vec2 lineOrigin = vec2(0., 0.15 * sin(-4.1 * iTime) - 0.03);
    mat2 rotation = rot2D(-0.32 * iTime);
    mat2 antirotation = rot2D(+0.6 * iTime);
    vec2 rotCenter = vec2(0, -0.03) - rotation * vec2(0.4, 0.);
    vec2 lineOrigin = antirotation * vec2(-.7, 0.) + rotCenter;
    vec2 lineTarget = antirotation * vec2(.3, 0.) + rotCenter;
    // oh yeah and why not scale it, while suppressing it?
    d = sdLine(uv * suppress, lineOrigin, lineTarget);
    // "beam" effect: blur closer to b
    float blur = exp(-3. * length(uv - lineTarget));
    col.r = exp(-(50. - 49.9999995*blur)*d);
    col.g = 0.8 * blur * col.r;

    float beamLoopSec = 20.;
    float beamTime = mod(iTime, beamLoopSec);
    suppress *= smoothstep(12., 14., beamTime)
    * (1. - smoothstep(beamLoopSec - 1., beamLoopSec, beamTime));
    col.rg *= suppress;

    fragColor.rgb = max(fragColor.rgb, col);

    // the bright bleu path :) (hint: it's probably an A from deAdline)
    const vec2 triLeft = vec2(-0.6, -0.32);
    const vec2 triRight = vec2(0.2, triLeft.y);
    const vec2 rightTail = vec2(0.52, 0.06);
    const vec2 halfTail = (triRight + rightTail) / 2;
    const vec2 apex = vec2(0, 0.35);
    const vec2 rightOfApex = mix(halfTail, apex, 0.8);
    const vec2 leftOfApex = mix(triLeft, apex, 0.86);
    const vec2 leftEnd = vec2(-0.72, -0.2);
    const vec2 leftReachMax = vec2(-0.4, 0.18);
    const vec2 leftReach = mix(leftEnd, leftReachMax, 0.8);


    d = 1000.;
    vec2 a = triLeft, b = triRight;
    vec2 _uv = uv;
    uv.y = _uv.y + slideY;
    // combine lines
    d = min(d, sdLine(uv, a, b));
    a = rightTail;
    d = min(d, sdLine(uv, a, b));
    a = halfTail;
    b = rightOfApex;
    d = min(d, sdLine(uv, a, b));
    a = triLeft;
    b = leftOfApex;
    d = min(d, sdLine(uv, a, b));
    a = rightOfApex;
    d = min(d, sdLine(uv, a, b));
    a = triLeft;
    b = leftEnd;
    d = min(d, sdLine(uv, a, b));
    a = leftReach;
    d = min(d, sdLine(uv, a, b));

    d = exp(-(16. + 4. * sin(0.3 * iTime))*d);
    float dLogo = d;
    col = dLogo * vec3(0, 0.73, 1.);
    fragColor.r *= (1. - (0.25 + 0.5 * cos(1. - suppress) * 0.5 * sin(3. * iTime)) * dLogo);

    float loopTime = 2. * mod(iTime, slideLoopSec) - slideLoopSec;
    float brighter = exp(-3.3 * abs(loopTime + _uv.y - 1.1) - 0.5);
    vec3 colMore = sqrt(brighter * dLogo) * c.yxx;
    col = max(col, colMore);
    col = pow(col, vec3(1.2));

    uv = _uv;

    vec3 flash = vec3(
        0.85 + 0.15 * floor(5.*sin(iTime*0.9)*sin(iTime*4.2-0.328)*sin(iTime*11.423-0.9)),
        0.65 + 0.35 * floor(9.*sin(iTime*0.73)*sin(iTime*.2-0.73)*sin(iTime*14.302+1.9)),
        0.9 + 0.1 * floor(13.*sin(iTime*0.29)*sin(iTime*14.2-0.328)*sin(iTime*52.-0.55))
    );
    // flash *= (1. - clamp(dLogo, 0, 1)); // don't want the weird Negative colors
    float attenuate = (1. - clamp(dLogo, 0, 1));
    d = sdBar(uv, leftEnd, leftReachMax);
    d = min(d, sdBar(uv, triLeft, leftEnd));
    col.rgb = mix(col.rgb, flash,
                  exp(-30.*d) * flickers(iTime, 0., 1. - 0.01 * mod(iTime, 50.)) * attenuate);
    d = sdBar(uv, triRight, rightTail);
    d = min(d, sdBar(uv, halfTail, apex));
    col.rgb = mix(col.rgb, flash,
                  exp(-30.*d) * flickers(iTime, 0.42, 1. - 0.01 * mod(iTime - 10., 50.)) * attenuate);

    float ledMask = 0.;
    float ledBorder = 0.;

    float dMin = 10000.;
    int pixelIndex = -1;
    vec2 pixelPos = c.yy;
    for (int i = minLogoLed; i <= maxLogoLed; i++) {
        // makeshift, but one could use any origin / scale -- this is about being useful quickly!
        vec2 ledPos = (ledPosition[i].xy / logoSize.xx) - logoCenter;
        d = sdfCircle(uv, ledPos, 0.01);
        if (d < dMin) {
            dMin = d;
            pixelIndex = i;
            pixelPos = ledPos;
        }
        ledBorder += d < 0.008 ? 0. : exp(-200.*d);
        ledMask += d < 0.008 ? 1. : exp(-200.*abs(d - 0.008));
    }
    extraOutput.y = float(pixelIndex);
    extraOutput.zw = logoSize;

    ledMask = clamp(ledMask, 0., 1.);
    if (maskOnlyPixels) {
        fragColor.rgb = mix(vec3(0.), col, ledMask);// col * (0.1 + 0.9 * mask);
    } else {
        fragColor.rgb += col;
    }
    if (!hidePixels) {
        fragColor.rgb = max(fragColor.rgb, c.xxx * ledBorder);
    }

    // Grüne Böbbel in Ursprung & Ecken (zur Orientierung halt)
    const vec2 cornerTL = vec2(-.5, +.3);
    const vec2 cornerBL = vec2(-.5, -.3);
    const vec2 cornerTR = vec2(+.5, +.3);
    const vec2 cornerBR = vec2(+.5, -.3);
    if (!hideOrigin) {
        col = fragColor.rgb;
        col.g = max(col.g, float(length(uv) < 0.01)
            + 0.5 * float(length(uv - cornerTL) < 0.01)
            + 0.5 * float(length(uv - cornerBL) < 0.01)
            + 0.5 * float(length(uv - cornerTR) < 0.01)
            + 0.5 * float(length(uv - cornerBR) < 0.01)
        );
        fragColor.g = col.g;
    }

    if (!hideCrosshair) {
        fragColor.rgb += vec3(0.7, 0.2, 0.4) * sdfMouseCrosshair();
        fragColor = vec4(clampVec3(fragColor.rgb), 1.);
    }

    return;
}
