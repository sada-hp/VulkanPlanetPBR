#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderUtils::UtilsCommon)
(
    float smootherstep(float e0, float e1, float x)
    {
        x = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
        return x * x * x * (x * (6.0 * x - 15.0) + 10.0);
    }

    float inverse_smoothstep(float e0, float e1, float x)
    {
        x = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
        return 0.5 - sin(asin(1.0 - 2.0 * x) / 3.0);
    }

    float ridge_smoothstep(float e0, float e1, float x)
    {
        x = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
        float sm = x * x * (3 - 2 * x);
        float inv = 0.5 - sin(asin(1.0 - 2.0 * x) / 3.0);

        return sm * inv;
    }

    float remap(float orig, float old_min, float old_max, float new_min, float new_max)
    {
        return new_min + (((orig - old_min) / (old_max - old_min)) * (new_max - new_min));
    }

    float saturate(float x)
    {
        return clamp(x, 0.0, 1.0);
    }

    vec2 saturate(vec2 x)
    {
        return clamp(x, 0.0, 1.0);
    }

    vec3 saturate(vec3 x)
    {
        return clamp(x, 0.0, 1.0);
    }

    vec4 saturate(vec4 x)
    {
        return clamp(x, 0.0, 1.0);
    }

    float dampen(float Value, float Factor)
    {
        return saturate(Value - Factor) / (1.0 - Factor);
    }

    float saturateAngle(float x)
    {
        return clamp(x, -1.0, 1.0);
    }

    vec2 saturateAngle(vec2 x)
    {
        return clamp(x, -1.0, 1.0);
    }

    vec3 divide_w(vec4 v)
    {
        return v.xyz / v.w;
    }
);