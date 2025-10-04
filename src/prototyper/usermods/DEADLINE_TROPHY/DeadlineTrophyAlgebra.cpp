#include "DeadlineTrophy.h"

namespace DeadlineTrophy {

    Vec2 Vec2::operator+(const Vec2& other) const {
        return {x + other.x, y + other.y};
    }

    Vec2& Vec2::operator+=(const Vec2& other) {
        *this = *this + other;
        return *this;
    }

    Vec2 Vec2::operator-(const Vec2& other) const {
        return {x - other.x, y - other.y};
    }

    Vec2& Vec2::operator-=(const Vec2& other) {
        *this = *this - other;
        return *this;
    }

    Vec2 Vec2::operator-() const {
        return {-x, -y};
    };

    template <typename T>
    Vec2 Vec2::operator*(T factor) const {
        return {static_cast<float>(factor) * x, static_cast<float>(factor) * y};
    }
    template Vec2 Vec2::operator*<float>(float) const;
    template Vec2 Vec2::operator*<double>(double) const;
    template Vec2 Vec2::operator*<int>(int) const;

    template <typename T>
    Vec2& Vec2::operator*=(T factor) {
        *this = *this * factor;
        return *this;
    }
    template Vec2& Vec2::operator*=<float>(float);
    template Vec2& Vec2::operator*=<double>(double);
    template Vec2& Vec2::operator*=<int>(int);

    float Vec2::dot(const Vec2& a, const Vec2 b) {
        return a.x * b.x + a.y * b.y;
    }

    float Vec2::length() const {
        return sqrtf(x * x + y * y);
    }

    Vec2& Vec2::shift(float dx, float dy) {
        x += dx;
        y += dy;
        return *this;
    };

    Vec2& Vec2::rotate(float phi) {
        float c = cos_t(phi);
        float s = sin_t(phi);
        float rotX = c * x - s * y;
        float rotY = s * x + c * y;
        x = rotX;
        y = rotY;
        return *this;
    }

    Vec2& Vec2::scale(float factorX, float factorY) {
        x *= factorX;
        y *= factorY;
        return *this;
    }

    float Vec2::distance(const Vec2& other) const {
        return (other - (*this)).length();
    }

    float Vec2::polarAngleFrom(const Vec2& center, float offsetAngle) const {
        // i.e. the angle between (line to center) and (+x axis), counter-clockwise ascending and within [0, 2pi]
        float theta = atan2f(y - center.y, x - center.x);
        return fmod_t(theta - offsetAngle + M_TWOPI, M_TWOPI);
    }

    Vec2 Vec2::fromParameters(uint8_t x, uint8_t y) {
        // will then reach from [-0.5, +0.496] on both axes
        return {
            (static_cast<float>(x) - 128.f) / 128.f,
            (static_cast<float>(y) - 128.f) / 128.f
        };
    }

    float Coord::sdLine(float x1, float y1, float x2, float y2) const {
        // QM TO-MAYBE-DO: could maybe remove this since I have the Vec2 version, but maybe check sameness & efficiency first
        float px = uv.x - x1;
        float py = uv.y - y1;
        float dx = x2 - x1;
        float dy = y2 - y1;
        float h = constrain((px*dx + py*dy) / (dx*dx + dy*dy), 0.f, 1.f);
        float lx = px - dx * h;
        float ly = py - dy * h;
        return sqrtf(lx*lx + ly*ly);
    }

    float Coord::sdLine(Vec2 p1, Vec2 p2) const {
        Vec2 p = uv - p1;
        Vec2 d = p2 - p1;
        float h = constrain(Vec2::dot(p,d) / Vec2::dot(d,d), 0.f, 1.f);
        Vec2 l = p - h * d;
        return l.length();
    }

    float Coord::gaussAt(Vec2 center, float width, float circleRadius) const {
        float expo = ((uv - center).length() - circleRadius) / width;
        return exp(-sq(expo));
    }

    FloatRgb FloatRgb::fromCRGB(const CRGB& color) {
        return {
            255.f * static_cast<float>(color.r),
            255.f * static_cast<float>(color.g),
            255.f * static_cast<float>(color.b),
        };
    }

    CRGB FloatRgb::toCRGB() const {
        // assumes that the floats are in [0..1]
        return CRGB(
            static_cast<uint8_t>(255.f * r),
            static_cast<uint8_t>(255.f * g),
            static_cast<uint8_t>(255.f * b)
        );
    }

    CHSV FloatRgb::toCHSV() const {
        return rgb2hsv_approximate(toCRGB());
    }

    FloatRgb::operator uint32_t() const {
        uint8_t R = static_cast<uint8_t>(r);
        uint8_t G = static_cast<uint8_t>(g);
        uint8_t B = static_cast<uint8_t>(b);
        return (R << 16) | (G << 8) | B;
    }

    FloatRgb& FloatRgb::scale(float factor) {
        r *= factor;
        g *= factor;
        b *= factor;
        return *this;
    }

    FloatRgb& FloatRgb::grade(float exponent) {
        r = r <= 0. ? 0. : exp(exponent * logf(r));
        g = g <= 0. ? 0. : exp(exponent * logf(g));
        b = b <= 0. ? 0. : exp(exponent * logf(b));
        return *this;
    }

}