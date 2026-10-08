#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderNoise::NoiseGS)
(
    layout(triangles) in;
    layout(triangle_strip, max_vertices = 3) out;

    layout(location = 0) in vec3 inUVW[];
    layout(location = 0) out vec3 outUVW;

    layout(push_constant) uniform constants
    {
        uint layers;
        uint frequency_worley;
        uint frequency_perlin;
        uint seed;
    }
    Settings;

    void main()
    {
        for (int vertex = 0; vertex < 3; vertex++)
        {
            gl_Position = gl_in[vertex].gl_Position;
            gl_Layer = int(inUVW[vertex].z);
            outUVW = inUVW[vertex];
            outUVW.z /= Settings.layers;
            EmitVertex();
        }

        EndPrimitive();
    }
);

ShaderCodeDefinition(GShaderUtils::NoiseCommon)
(
    const uint UI0 = 1597334673U;
    const uint UI1 = 3812015801U;
    const uvec2 UI2 = uvec2(UI0, UI1);
    const uvec3 UI3 = uvec3(UI0, UI1, 2798796415U);
    const float UIF = (1.0 / float(0xffffffffU));

    uint NOISE_SEED = 2798796415U;

    float remap(float orig, float old_min, float old_max, float new_min, float new_max)
    {
        return new_min + (((orig - old_min) / (old_max - old_min)) * (new_max - new_min));
    }

    float noise(float p)
    {
        return fract(sin(p + 1.951) * 43758.5453);
    }

    float noise(vec2 p)
    {
        return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    }

    vec2 noise2(vec2 p)
    {
        ivec2 n = ivec2(p.x * ivec2(3, 37) + p.y * ivec2(311, 113));
        n = (n << 13) ^ n;
        n = n * (n * n * 15731 + 789221) + int(NOISE_SEED);
        return -1.0 + 2.0 * vec2(n & ivec2(0x0fffffff)) / float(0x0fffffff);
    }

    vec3 noise3(vec3 p)
    {
        ivec3 n = ivec3(p.x * ivec3(3, 37, 64) + p.y * ivec3(311, 113, 235) + p.z * ivec3(567, 256, 56));
        n = (n << 13) ^ n;
        n = n * (n * n * 15731 + 789221) + int(NOISE_SEED);
        return -1.0 + 2.0 * vec3(n & ivec3(0x0fffffff)) / float(0x0fffffff);
    }

    vec2 pcg2d(vec2 p)
    {
        uvec2 v = uvec2(p) * 1664525u;

        v.x += v.y * 1664525u;
        v.y += v.x * 1664525u;

        v = v ^ (v >> 16u);

        v.x += v.y * 1664525u;
        v.y += v.x * 1664525u;

        v = v ^ (v >> 16u);

        return vec2(v) / float(0x0fffffff);
    }

    vec3 pcg3d(vec3 p)
    {
        uvec3 i = uvec3(floatBitsToUint(p)) * 1664525u + 1013904223u;

        i.x += i.y * i.z;
        i.y += i.z * i.x;
        i.z += i.x * i.y;

        i ^= i >> 16u;

        i.x += i.y * i.z;
        i.y += i.z * i.x;
        i.z += i.x * i.y;

        return fract(vec3(uintBitsToFloat(i)) / vec3(0x0fffffff));
    }

    float worley(vec2 p0, float freq)
    {
        float min_d = 1.0;
        const vec2 i = floor(p0 * freq);
        const vec2 f = fract(p0 * freq);

        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                vec2 p1 = vec2(x, y);
                vec2 p2 = abs(noise2(mod(i + p1, freq)));
                vec2 diff = p2 + p1 - f;

                min_d = min(min_d, dot(diff, diff));
            }
        }

        return sqrt(min_d);
    }

    float worley(vec3 p0, float freq)
    {
        float min_d = 1.0;
        const vec3 i = floor(p0 * freq);
        const vec3 f = fract(p0 * freq);

        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                for (int z = -1; z <= 1; z++) {
                    vec3 p1 = vec3(x, y, z);
                    vec3 p2 = abs(noise3(mod(i + p1, freq)));
                    vec3 diff = p2 + p1 - f;

                    min_d = min(min_d, dot(diff, diff));
                }
            }
        }

        return sqrt(min_d);
    }

    vec4 permute(vec4 x)
    {
        return mod(((x * 34.0) + 1.0) * x, 289.0);
    }

    vec4 taylorInvSqrt(vec4 r) 
    { 
        return 1.79284291400159 - 0.85373472095314 * r; 
    }

    vec2 fade(vec2 t)
    {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    vec3 fade(vec3 t) 
    { 
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); 
    }

    float perlin(vec2 x0, float freq)
    {
        vec4 Pi = floor(x0.xyxy * freq) + vec4(0.0, 0.0, 1.0, 1.0);
        vec4 Pf = fract(x0.xyxy * freq) - vec4(0.0, 0.0, 1.0, 1.0);
        Pi = mod(Pi, 289.0);

        vec4 ix = Pi.xzxz;
        vec4 iy = Pi.yyww;
        vec4 fx = Pf.xzxz;
        vec4 fy = Pf.yyww;

        vec4 i = permute(permute(ix) + iy);
        vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0;
        vec4 gy = abs(gx) - 0.5;
        vec4 tx = floor(gx + 0.5);
        gx = gx - tx;

        vec2 g00 = vec2(gx.x, gy.x);
        vec2 g10 = vec2(gx.y, gy.y);
        vec2 g01 = vec2(gx.z, gy.z);
        vec2 g11 = vec2(gx.w, gy.w);

        vec4 norm = 1.79284291400159 - 0.85373472095314 * vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
        g00 *= norm.x;
        g01 *= norm.y;
        g10 *= norm.z;
        g11 *= norm.w;

        float n00 = dot(g00, vec2(fx.x, fy.x));
        float n10 = dot(g10, vec2(fx.y, fy.y));
        float n01 = dot(g01, vec2(fx.z, fy.z));
        float n11 = dot(g11, vec2(fx.w, fy.w));

        vec2 fade_xy = fade(Pf.xy);
        vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
        float n_xy = mix(n_x.x, n_x.y, fade_xy.y);

        return 2.3 * n_xy;
    }

    float perlin(vec3 x0, float freq)
    {
        vec3 Pi0 = floor(x0 * freq);
        vec3 Pi1 = Pi0 + vec3(1.0);
        Pi0 = mod(Pi0, 289.0);
        Pi1 = mod(Pi1, 289.0);
        vec3 Pf0 = fract(x0 * freq);
        vec3 Pf1 = Pf0 - vec3(1.0);
        vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
        vec4 iy = vec4(Pi0.yy, Pi1.yy);
        vec4 iz0 = Pi0.zzzz;
        vec4 iz1 = Pi1.zzzz;

        vec4 ixy = permute(permute(ix) + iy);
        vec4 ixy0 = permute(ixy + iz0);
        vec4 ixy1 = permute(ixy + iz1);

        vec4 gx0 = ixy0 / 7.0;
        vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
        gx0 = fract(gx0);
        vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
        vec4 sz0 = step(gz0, vec4(0.0));
        gx0 -= sz0 * (step(0.0, gx0) - 0.5);
        gy0 -= sz0 * (step(0.0, gy0) - 0.5);

        vec4 gx1 = ixy1 / 7.0;
        vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
        gx1 = fract(gx1);
        vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
        vec4 sz1 = step(gz1, vec4(0.0));
        gx1 -= sz1 * (step(0.0, gx1) - 0.5);
        gy1 -= sz1 * (step(0.0, gy1) - 0.5);

        vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
        vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
        vec3 g010 = vec3(gx0.z, gy0.z, gz0.z);
        vec3 g110 = vec3(gx0.w, gy0.w, gz0.w);
        vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
        vec3 g101 = vec3(gx1.y, gy1.y, gz1.y);
        vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
        vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);

        vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
        g000 *= norm0.x;
        g010 *= norm0.y;
        g100 *= norm0.z;
        g110 *= norm0.w;
        vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
        g001 *= norm1.x;
        g011 *= norm1.y;
        g101 *= norm1.z;
        g111 *= norm1.w;

        float n000 = dot(g000, Pf0);
        float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
        float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
        float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
        float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
        float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
        float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
        float n111 = dot(g111, Pf1);

        vec3 fade_xyz = fade(Pf0);
        vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
        vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
        float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
        return 2.2 * n_xyz;
    }

    float fbm_worley(vec2 x, float f, uint n)
    {
        const mat2 rot = mat2(0.00, 0.80, 0.60, -0.80);

        float v = 0.0, a = 0.5;
        for (int i = 0; i < n; i++) {
            v += a * worley(x, f);
            x = rot * x;
            f *= 2.0;
            a *= 0.5;
        }

        return v;
    }

    float fbm_worley(vec3 x, float f, uint n)
    {
        const mat3 rot = mat3(0.00, 0.80, 0.60, -0.80, 0.36, -0.48, -0.60, -0.48, 0.64);

        float v = 0.0, a = 0.5, w = 0.0;
        for (int i = 0; i < n; i++) 
        {
            v += a * worley(x, f);
            x = rot * x;
            f *= 2.0;
            w += a;
            a *= a;
        }

        return v / w;
    }

    float fbm_perlin(vec2 x, float f, uint n)
    {
        const mat2 rot = mat2(0.00, 0.80, 0.60, -0.80);

        float v = 0.0, a = 0.5, w = 0.0;
        for (int i = 0; i < n; i++)
        {
            v += a * perlin(x, f);
            x = rot * x;
            f *= 2.0;
            w += a;
            a *= a;
        }

        return clamp(v / w, -1.0, 1.0) * 0.5 + 0.5;
    }

    float fbm_perlin(vec3 x, float f, uint n)
    {
        const mat3 rot = mat3(0.00, 0.80, 0.60, -0.80, 0.36, -0.48, -0.60, -0.48, 0.64);

        float v = 0.0, a = 0.5, w = 0.0;
        for (int i = 0; i < n; i++)
        {
            v += a * perlin(x, f);
            x = rot * x;
            f *= 2.0;
            w += a;
            a *= a;
        }

        return clamp(v / w, -1.0, 1.0) * 0.5 + 0.5;
    }
);