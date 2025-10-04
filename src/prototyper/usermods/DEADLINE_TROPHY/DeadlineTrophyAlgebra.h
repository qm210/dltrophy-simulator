#pragma once

namespace DeadlineTrophy {

    struct Vec2 {
        float x;
        float y;

        Vec2 operator+(const Vec2& other) const;
        Vec2& operator+=(const Vec2& other);
        Vec2& operator-=(const Vec2& other);
        Vec2 operator-(const Vec2& other) const;
        Vec2 operator-() const;
        template <typename T> Vec2 operator*(T factor) const;
        template <typename T> Vec2& operator*=(T factor);
        static float dot(const Vec2& a, const Vec2 b);
        float length() const;
        Vec2& shift(float dx, float dy);
        Vec2& rotate(float phi);
        Vec2& scale(float factorX, float factorY);
        float distance(const Vec2& other) const;
        float polarAngleFrom(const Vec2& center, float offsetAngle) const;
        static Vec2 fromParameters(uint8_t x, uint8_t y);
    };

    template <typename T> Vec2 operator*(T factor, const Vec2& vec) {
        return vec * factor;
    }

    struct FloatRgb {
        float r;
        float g;
        float b;

        FloatRgb& scale(float factor);
        FloatRgb& grade(float exponent);
        static FloatRgb fromCRGB(const CRGB& color);
        CRGB toCRGB() const;
        CHSV toCHSV() const;
        operator uint32_t() const;
    };

    struct Coord {
        // indices for matrix
        uint8_t x;
        uint8_t y;
        // normalized coordinates
        Vec2 uv;
        // for reference, pixel index within its segment
        int index;

        float sdLine(float x1, float y1, float x2, float y2) const;
        float sdLine(Vec2 p1, Vec2 p2) const;
        float gaussAt(Vec2 p, float width = 0.1, float circleRadius = 0.) const;

        // helpers for the base (don't make any sense for the Logo)
        // side 0=right, 1=front, 2=left, 3=back
        inline int indexOfSide() const { return index / 16; }
        inline int indexInSide() const { return index % 16; }
    };

}
