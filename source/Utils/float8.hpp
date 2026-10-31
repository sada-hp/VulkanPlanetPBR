#pragma once

struct ufloat8 {
    uint8_t bits;

    static ufloat8 from_float(float f) {
        ufloat8 result{ 0 };
        if (f <= 0.0f) return result; 

        uint32_t f_bits = *reinterpret_cast<uint32_t*>(&f);

        int32_t f_exp = ((f_bits >> 23) & 0xFF) - 127;
        uint32_t f_mant = f_bits & 0x7FFFFF;

        int32_t new_exp = f_exp + 7;

        if (new_exp >= 15) {
            result.bits = 0xFF;
        }
        else if (new_exp <= 0) {
            result.bits = 0x00;
        }
        else {
            uint8_t e_bits = static_cast<uint8_t>(new_exp & 0xF);
            uint8_t m_bits = static_cast<uint8_t>((f_mant >> 19) & 0xF);
            result.bits = (e_bits << 4) | m_bits;
        }
        return result;
    }

    float to_float() const 
    {
        if (bits == 0) return 0.0f;

        uint8_t e_bits = (bits >> 4) & 0xF;
        uint8_t m_bits = bits & 0xF;

        int32_t exp = static_cast<int32_t>(e_bits) - 7;
        float mantissa = 1.0f + (static_cast<float>(m_bits) / 16.0f);

        return std::ldexp(mantissa, exp);
    }
};