#pragma once

#include <cmath>
#include <algorithm>

#include "Vectors.h"

// ----------------------
// sRGB → Linear RGB
// ----------------------
inline double srgbToLinear(double c)
{
    c /= 255.0;
    if (c <= 0.04045)
        return c / 12.92;
    else
        return std::pow((c + 0.055) / 1.055, 2.4);
}

// ----------------------
// RGB → XYZ
// ----------------------
inline Vec3 rgbToXYZ(const Vec3& rgb)
{
    double r = srgbToLinear(rgb.x);
    double g = srgbToLinear(rgb.y);
    double b = srgbToLinear(rgb.z);

    return {
        r * 0.4124 + g * 0.3576 + b * 0.1805,
        r * 0.2126 + g * 0.7152 + b * 0.0722,
        r * 0.0193 + g * 0.1192 + b * 0.9505
    };
}

// ----------------------
// XYZ → LAB
// ----------------------
inline double lab_f(double t)
{
    if (t > 0.008856)
        return std::cbrt(t);
    else
        return (7.787 * t) + (16.0 / 116.0);
}

inline Vec3 xyzToLab(const Vec3& xyz)
{
    // D65 reference white
    constexpr double Xn = 0.95047;
    constexpr double Yn = 1.00000;
    constexpr double Zn = 1.08883;

    double x = lab_f(xyz.x / Xn);
    double y = lab_f(xyz.y / Yn);
    double z = lab_f(xyz.z / Zn);

    return {
        (116.0 * y) - 16.0,   // L*
        500.0 * (x - y),      // a*
        200.0 * (y - z)       // b*
    };
}

// ----------------------
// RGB → LAB (combined)
// ----------------------
inline Vec3 rgbToLab(const Vec3& rgb)
{
    return xyzToLab(rgbToXYZ(rgb));
}

// ----------------------
// CIE94 color difference
// ----------------------
inline double deltaE94(const Vec3& lab1, const Vec3& lab2)
{
    double dL = lab1.x - lab2.x;
    double da = lab1.y - lab2.y;
    double db = lab1.z - lab2.z;

    double C1 = std::sqrt(lab1.y * lab1.y + lab1.z * lab1.z);
    double C2 = std::sqrt(lab2.y * lab2.y + lab2.z * lab2.z);

    double dC = C1 - C2;
    double dH_sq = da*da + db*db - dC*dC;

    double kL = 1.0, kC = 1.0, kH = 1.0;
    double K1 = 0.045;
    double K2 = 0.015;

    double SL = 1.0;
    double SC = 1.0 + K1 * C1;
    double SH = 1.0 + K2 * C1;

    double termL = dL / (kL * SL);
    double termC = dC / (kC * SC);
    double termH = std::sqrt(std::max(0.0, dH_sq)) / (kH * SH);

    // Return squared ΔE94 (faster for comparisons)
    return termL*termL + termC*termC + termH*termH;
}