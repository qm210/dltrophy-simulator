#version 330 core

precision highp float;
// out locations equal color attachments, respectively.
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

const int nLeds = 172; // needed hardcode to be used in the uniform layout below

layout(std140) uniform TrophyDefinition {
    int _nLedsWouldNotWorkThatWay;
    vec4 ledPosition[nLeds];
};

struct RGB { // last value is only for alignment, never used.
    uint r, g, b, _;
};

layout(std140) uniform StateBuffer {
    RGB ledColor[nLeds];
    float ledSize, ledGlow;
    float camX, camY, camZ, camFov, camTilt;
    float fogScaling, fogGrading, backgroundSpin;
    float floorLevel, floorGraytone, floorLineBrightness,
          floorSpacingX, floorSpacingZ,
          floorLineWidth, floorExponent, floorGrading;
    float pyramidX, pyramidY, pyramidZ,
          pyramidScale, pyramidHeight,
          pyramidAngle, pyramidAngularVelocity;
    float epoxyPermittivity;
    float blendPreviousMixing;
    float traceMinDistance, traceMaxDistance, traceFixedStep;
    int traceMaxSteps, traceMaxRecursions;
    float ledBlurSamples, ledBlurRadius, ledBlurPrecision,
          ledBlurMixing;
    int options;
};

#define hasOption(index) (options & (1 << (8 * index))) != 0

bool showGrid = hasOption(0);
bool accumulateForever = hasOption(1);
bool noStochasticVariation = hasOption(2);
bool onlyPyramidFrame = hasOption(3);

vec3 to_vec(RGB rgb) {
    return vec3(rgb.r, rgb.g, rgb.b) / 255.;
}

const float pi = 3.14159265;
const float rad = pi / 180.;
const float tau = 2. * pi;
const vec4 c = vec4(1., 0., -1., .5);

const int MISS = -1;
const int LED_MATERIAL = 0;
const int FLOOR_MATERIAL = 1;
const int PYRAMID_MATERIAL = 2;
const int PYRAMID_FRAME_MATERIAL = 3;
const int LED_FRAME_MATERIAL = 4;
const int DEBUG_MATERIAL = 99;

const vec3 borderDark = vec3(0.4);
const vec3 borderLight = vec3(0.6);

vec2 iResolution = iRect.zw;
float aspectRatio = iResolution.x / iResolution.y;

vec3 draw_grid(in vec2 uv) {
    vec3 col = c.yyy;
    float thickness = 3.e-3;
    const float step = 0.1;
    uv = abs(uv);
    if (min(uv.x, uv.y) < 1.5 * thickness) {
        thickness *= 1.5;
        col.g = 0.8;
        col.b = 1.;
    }
    else if (mod(uv.x, step) < thickness ||
             mod(uv.y, step) < thickness) {
        col.g = 0.6;
    }
    return col;
}

// 3D GEOMETRY

#define clampVec3(x) clamp(x, vec3(0), vec3(1))

const vec2 epsilon = c.xz * 0.0005;

mat3 rotateX(float theta) {
    float c = cos(theta * rad);
    float s = sin(theta * rad);
    return mat3(
        vec3(1,  0,  0),
        vec3(0,  c, -s),
        vec3(0, +s,  c)
    );
}

mat3 rotateY(float theta) {
    float c = cos(theta * rad);
    float s = sin(theta * rad);
    return mat3(
        vec3( c, 0, s),
        vec3( 0, 1, 0),
        vec3(-s, 0, c)
    );
}

mat3 rotateZ(float theta) {
    float c = cos(theta * rad);
    float s = sin(theta * rad);
    return mat3(
        vec3( c, -s, 0),
        vec3(+s,  c, 0),
        vec3( 0,  0, 1)
    );
}

struct Marched {
    float sd;
    int material;
    vec3 color;
    int ledIndex;
    vec3 normal;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

vec3 advance(Ray ray, float depth) {
    return ray.origin + depth * ray.dir;
}

//// pseudorandomosities ////

float globalSeed = 0.;

uint base_hash(uvec2 p) {
    p = 1103515245U*((p >> 1U)^(p.yx));
    uint h32 = 1103515245U*((p.x)^(p.y>>3U));
    return h32^(h32 >> 16);
}

float hash1(inout float seed) {
    uint n = base_hash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    return float(n)*(1.0/float(0xffffffffU));
}

vec2 hash2(inout float seed) {
    uint n = base_hash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    uvec2 rz = uvec2(n, n*48271U);
    return vec2(rz.xy & uvec2(0x7fffffffU))/float(0x7fffffff);
}

float hash21(vec2 co){
    return fract(sin(dot(co.xy,vec2(1.9898,7.233)))*45758.5433);
}

//////

// inspiri-stolen from https://www.shadertoy.com/view/tsScRK

float starnoise(vec3 rd){
    float c = 0.;
    vec3 p = normalize(rd)*200.;
    for (float i=0.;i<4.;i++)
    {
        vec3 q = fract(p)-.5;
        vec3 id = floor(p);
        float c2 = smoothstep(.5,0.,length(q));
        c2 *= step(hash21(id.xz/id.y),.06-i*i*0.005);
        c += c2;
        p = p*.6+.5*p*mat3(3./5.,0,4./5.,0,1,0,-4./5.,0,3./5.);
    }
    c*=c;
    float g = dot(sin(rd*10.512),cos(rd.yzx*10.512));
    c*=smoothstep(-3.14,-.9,g)*.5+.5*smoothstep(-.3,1.,g);
    return c*c;
}

vec3 background(vec3 rd, vec3 ld) {
    float haze = 0.3 * exp2(-5.*(abs(rd.y)-.2*dot(rd,ld)));
    float stars = 0.;
    float spin = backgroundSpin * iTime;
    for (float i=0.; i < backgroundSpin + 1.5; i+=1.) {
        spin -= 0.01;
        stars += starnoise(rd * rotateZ(spin + 0.01 * hash1(globalSeed)));
    }
    stars *= (1. - min(haze,1.));
    vec3 back = vec3(0.,.1,.7)
        * exp2(-.1*abs(length(rd.xz)/rd.y))
        * max(sign(rd.y),0.);
    return clamp(
            mix(back, vec3(.3,.1, .52), haze) + stars
        , 0., 1.);
}

////

vec3 floorColor = vec3(0., 0.73 - 0.2 * cos(iTime), 0.94 - 0.05 * sin(0.4 * iTime));
const vec3 floorNormal = c.yxy;

Marched sdFloor(vec3 p) {
    float d = dot(p, floorNormal) - floorLevel;
    return Marched(d, FLOOR_MATERIAL, c.xxx, -1, floorNormal);
}

float sdSphere(vec3 p, vec3 center, float radius) {
    return length(p - center) - radius;
}

float sdZCylinder(vec3 p, float ra, float h)
{
    const float eps = 0.00001;
    vec2 d = vec2( length(p.xy)-2.0*ra+eps, abs(p.z) - h );
    return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - eps;
}

float sdLineSegment(vec3 p, vec3 a, vec3 b) {
    vec3 ap = p - a;
    vec3 ab = b - a;
    float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    vec3 closest = a + t * ab;
    return length(p - closest);
}

float sdPyramidFrame(vec3 p) {
    const float b = 0.5;
    vec3 v1 = vec3( b, 0,  b);
    vec3 v2 = vec3(-b, 0,  b);
    vec3 v3 = vec3(-b, 0, -b);
    vec3 v4 = vec3( b, 0, -b);
    float d1 = sdLineSegment(p, v1, v2);
    float d2 = sdLineSegment(p, v2, v3);
    float d3 = sdLineSegment(p, v3, v4);
    float d4 = sdLineSegment(p, v4, v1);

    vec3 apex = vec3(0, pyramidHeight, 0);
    float d5 = sdLineSegment(p, apex, v1);
    float d6 = sdLineSegment(p, apex, v2);
    float d7 = sdLineSegment(p, apex, v3);
    float d8 = sdLineSegment(p, apex, v4);

    float frameDist = min(min(min(d1, d2), min(d3, d4)),
    min(min(d5, d6), min(d7, d8)));

    return frameDist - 0.0025; // <-- frame thickness
}

mat3 pyramidRotation = rotateY(pyramidAngle + pyramidAngularVelocity * iTime);
mat3 pyramidCounterRotation = rotateY(-pyramidAngle - pyramidAngularVelocity * iTime);

// thanks iq https://iquilezles.org/articles/distfunctions/
float sdPyramid( vec3 p )
{
    float h = pyramidHeight;
    float m2 = h*h + 0.25;

    p.xz = abs(p.xz);
    p.xz = (p.z>p.x) ? p.zx : p.xz;
    p.xz -= 0.5;

    vec3 q = vec3( p.z, h*p.y - 0.5*p.x, h*p.x + 0.5*p.y);

    float s = max(-q.x,0.0);
    float t = clamp( (q.y-0.5*p.z)/(m2+0.25), 0.0, 1.0 );
    float a = m2*(q.x+s)*(q.x+s) + q.y*q.y;
    float b = m2*(q.x+0.5*t)*(q.x+0.5*t) + (q.y-m2*t)*(q.y-m2*t);
    float d2 = min(q.y,-q.x*m2-q.y*0.5) > 0.0 ? 0.0 : min(a,b);

    return sqrt( (d2+q.z*q.z)/m2 ) * sign(max(q.z,-p.y));
}

// RAY MARCHING / TRACING

bool updatedHit(inout Marched hit, float sd) {
    if (sd < hit.sd) {
        hit.sd = sd;
        return true;
    }
    return false;
}

Marched sdScene(vec3 p) {
    Marched hit = sdFloor(p);
    float sd;
    bool isCloser;

    p *= pyramidRotation;
    for (int i = 0; i < nLeds; i++) {
        if (i >= 64 && i < 64 + 106) {
            sd = sdZCylinder(p - ledPosition[i].xyz, ledSize * 0.7, ledSize * 0.3);
            if (updatedHit(hit, sd)) {
                hit.ledIndex = i;
                hit.material = LED_FRAME_MATERIAL;
            }
        }
        sd = sdSphere(p, ledPosition[i].xyz, ledSize);
        if (updatedHit(hit, sd)) {
            hit.ledIndex = i;
            hit.material = LED_MATERIAL;
        }
    }
    extraOutput.y = float(hit.ledIndex); // cf. below "extraOutput"
    if (onlyLeds) {
        return hit;
    }

    p -= vec3(pyramidX, pyramidY, pyramidZ);
    p *= 1. / pyramidScale;
    sd = sdPyramidFrame(p);
    if (onlyPyramidFrame) {
        if (updatedHit(hit, sd)) {
            hit.material = PYRAMID_FRAME_MATERIAL;
        }
        return hit;
    }

    sd = sdPyramid(p);
    if (updatedHit(hit, sd)) {
        hit.material = PYRAMID_MATERIAL;
    }

    return hit;
}

vec3 calcNormal(in vec3 p) {
    return normalize(
        epsilon.xyy * sdScene(p + epsilon.xyy).sd +
        epsilon.yyx * sdScene(p + epsilon.yyx).sd +
        epsilon.yxy * sdScene(p + epsilon.yxy).sd +
        epsilon.xxx * sdScene(p + epsilon.xxx).sd
    );
}

Marched analyticalHit(Ray ray) {
    float co = dot(ray.origin, floorNormal);
    float cd = dot(ray.dir, floorNormal);

    Marched hit = Marched(1.e4, MISS, c.xxx, -1, floorNormal);
    if (onlyLeds) {
        return hit;
    }

    if (cd == 0) { // ray parallel to the floor -- can never hit
        return hit;
    }
    float distance = (floorLevel - co) / cd;
    if (distance > 0.) {
        hit.material = FLOOR_MATERIAL;
        hit.sd = distance;
    }
    return hit;
}

Marched marchScene(Ray ray) {
    Marched hit;
    float depth = traceMinDistance;
    vec3 p;

    for(int i = 0; i < traceMaxSteps; i++) {
        p = advance(ray, depth);
        hit = sdScene(p);
        depth += hit.sd;
        if (hit.sd < traceMinDistance) {
            break;
        }
        // behind or below the pyramid there is nothing interesting anymore
        if (depth >= traceMaxDistance || p.y < pyramidY - traceMinDistance) {
            return analyticalHit(ray);
        }
    }

    hit.sd = depth;
    hit.normal = calcNormal(advance(ray, hit.sd));
    return hit;
}

float schlick(float cosine, float ior) {
    float r0 = (1.-ior)/(1.+ior);
    r0 = r0*r0;
    return r0 + (1.-r0)*pow((1.-cosine),5.);
}

bool modifiedRefract(const in vec3 p, const in vec3 normal, const in float ni_over_nt, out vec3 refracted) {
    float dt = dot(p, normal);
    float discriminant = 1. - ni_over_nt*ni_over_nt*(1.-dt*dt);
    if (discriminant > 0.) {
        refracted = ni_over_nt*(p - normal*dt) - normal*sqrt(discriminant);
        return true;
    } else {
        return false;
    }
}

void pyramidScatter(const Ray ray, const Marched hit, out vec3 attenuation, out Ray scattered) {
    // this resembles dielectric part from RIOW
    vec3 normal, refracted;
    vec3 reflected = reflect(ray.dir, hit.normal);
    float ni_over_nt, cosine;

    attenuation = 0.4 * vec3(0.9, 0.7, 0.95);
    if (dot(ray.dir, hit.normal) > 0.) {
        normal = -hit.normal;
        ni_over_nt = epoxyPermittivity;
        cosine = dot(ray.dir, hit.normal);
        cosine = sqrt(
            1. - epoxyPermittivity*epoxyPermittivity*(1.-cosine*cosine)
        );
    } else {
        normal = hit.normal;
        ni_over_nt = 1. / epoxyPermittivity;
        cosine = -dot(ray.dir, hit.normal);
    }

    float reflectProbability = 1.;
    if (modifiedRefract(ray.dir, normal, ni_over_nt, refracted)) {
        reflectProbability = schlick(cosine, epoxyPermittivity);
    }
    vec3 currentPosition = advance(ray, hit.sd);
    scattered = Ray(
        currentPosition,
        hash1(globalSeed) < reflectProbability
            ? reflected : refracted
    );
}

const vec3 debugColor = c.xwy; // Ein schnöde-ulkiges Orange

vec3 opaqueMaterial(Marched hit, vec3 ray) {
    switch (hit.material) {
        case LED_FRAME_MATERIAL:
            return c.yyy;

        case LED_MATERIAL:
            return to_vec(ledColor[hit.ledIndex]);

        case FLOOR_MATERIAL:
            // Synthwave Grid ftw.
            vec2 f = vec2(
                fract(ray.x / floorSpacingX),
                fract(ray.z / floorSpacingZ)
            );
            f = .5 - abs(.5 - f);
            f = max(c.yy, 1. - f + .5 * floorLineWidth);
            f = pow(f, vec2(floorExponent));
            float g = pow(f.x+f.y, 1./floorGrading);
            g = clamp(g, 0., 1.);
            return mix(vec3(floorGraytone), floorLineBrightness * floorColor, g);

        case PYRAMID_MATERIAL:
            float frameSd = sdPyramidFrame(ray);
            vec3 base = mix(c.xxx, vec3(0.15, 0.0, 0.3), pow(frameSd, .15));
            base = pow(base, c.xxx * 1.4);

            vec3 lights = c.yyy;
            for (int i = 0; i < nLeds; i++) {
                float sd = distance(ray, ledPosition[i].xyz);
                float w = exp(-ledGlow * sd);
                lights += w * to_vec(ledColor[i]);
            }
            base += lights / float(nLeds);
            return base;

        case PYRAMID_FRAME_MATERIAL:
            return vec3(0.62, 0.0, 1.);

        default:
            return onlyLeds ? c.yyy : debugColor;
    }
}

Marched traceScene(Ray ray) {
    vec3 col = c.xxx;
    Marched hit;
    Marched direct_hit;
    Ray scatRay;
    int r;

    for (r = 0; r < traceMaxRecursions; r++) {
        hit = marchScene(ray);
        if (r == 0) {
            direct_hit = hit;
        }
        hit.color = opaqueMaterial(hit, advance(ray, hit.sd));
        if (hit.material != PYRAMID_MATERIAL) {
            break;
        }
        if (hit.sd >= traceMaxDistance) {
            break;
        }
        // still here? -> hit the pyramid :) go on scattering, mr. funny boi
        vec3 attenuation;
        pyramidScatter(ray, hit, attenuation, scatRay);
        col *= attenuation * hit.color;
        ray = scatRay;
    }
    return hit;
}

const float goldenPhi = 2.39996323;

vec3 blurredBloomImage(in vec2 st) {
    vec4 result = c.yyyy;
    for (float s = 0.; s < ledBlurSamples; s+= 1.) {
        float r = ledBlurRadius * sqrt((s + 0.5) / ledBlurSamples) * 1./iResolution.y;
        float theta = s * goldenPhi;
        // theta += 0.05 * hash1(globalSeed);
        vec2 offset = r * vec2(cos(theta), sin(theta));
        r *= ledBlurPrecision * 0.01/ledSize;
        float weight = exp(-r * r);
        result.rgb += weight * texture(iBloomImage, st + offset).rgb;
        result.a += weight;
    }
    return mix(
        texture(iBloomImage, st).rgb,
        result.rgb / result.a,
        ledBlurMixing
    );
}

const float postExposure = 1.2;
const vec3 postGamma = vec3(0.8);

void postProcess(inout vec3 col, in vec2 uv, in vec2 st) {
    vec3 bloomImage = blurredBloomImage(st);
    float brightness = 1.5 * dot(bloomImage, vec3(0.2126, 0.7152, 0.0722));
//    brightness = pow(brightness, 0.5);
//    col = mix(col + 0.1 * bloomImage, bloomImage, brightness);
    col = mix(col, bloomImage, brightness);
//    col = 1. - exp(-col * (postExposure + brightness));
//    col = pow(col, postGamma);

    // simple vignette for now.
    float rf = length(uv) * 0.9;
    rf = pow(rf, 4.2) + 1.;
    rf = pow(rf, -1.6);
    col *= clamp(rf, 0., 1.);

    // gamma grading
    // col = pow(col, vec3(1.4));
    // gain
    // col = col * 4.0/(2.5 + col);
}

void main() {
    vec2 uv = (2. * (gl_FragCoord.xy - iRect.xy) - iResolution) / iResolution.y;

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

    extraOutput.x = 1.;   // signals us that we are in the frame
    extraOutput.y = -1.;  // means "no LED index" (cf. below)

    fragColor = c.xxxy;
    vec3 col = fragColor.rgb;

    if (!onlyLeds) {
        uvec2 bits = floatBitsToUint(gl_FragCoord.xy);
        globalSeed = float(base_hash(bits))/float(0xffffffffU);
        globalSeed += iTime;
        if (!noStochasticVariation) {
            uv += hash2(globalSeed) / iResolution;
        }
    }

    vec3 lightDir = normalize(vec3(0,.125+.05*sin(.1*iTime),1));
    vec3 ro = vec3(camX, camY, camZ);
    vec3 rd;
    Ray ray;
    Marched hit;

    rd = normalize(vec3(uv, camFov));
    rd *= rotateX(camTilt);
    fragColor.rgb = background(rd, lightDir);

    col = c.yyy;
    ray = Ray(ro, rd);
    hit = traceScene(ray);

    if (onlyLeds) {
        fragColor.rgb = hit.color;
        return;
    }

    float fogMixing = exp(-fogScaling * pow(abs(hit.sd), fogGrading));
    col = mix(fragColor.rgb, hit.color, fogMixing);

    if (showGrid) {
        vec3 grid = draw_grid(uv);
        col += 0.2 * grid;
    }

    fragColor = vec4(clampVec3(col), 1.);

    fragColor.b += exp(-20. * pow(gl_FragCoord.x - iMouse.z, 2.));

    // blend previous image
    vec2 st = (gl_FragCoord.xy) / (iResolution + iRect.xy);
    vec4 previousImage = texture(iPreviousImage, st);

    if (iPass == POST_PASS) {
        col = previousImage.rgb / previousImage.a;
        col = max(col, fragColor.rgb);
        postProcess(col, uv, st);
        fragColor = vec4(col, 1.);
        return;
    }

    if (accumulateForever) {
        // forever-accumulating uses the alpha channel to count the weight.
        if (iFrame > 0) {
            fragColor += previousImage;
        }
        return;
    }

    fragColor.rgb = mix(fragColor.rgb, previousImage.rgb, blendPreviousMixing);
    fragColor.a = 1.;

    bool clicked = distance(iMouse.zw, gl_FragCoord.xy - iRect.xy) < 1.;
    if (!clicked) {
        // extraOutput.y must have been set somewhere above,
        // but if not even clicked, reset to "nothing clicked".
        extraOutput.y = -1.;
    }
}
