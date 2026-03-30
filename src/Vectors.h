#pragma once

#include <iostream>
#include <cmath>

class Vec3 {
public:
    double x, y, z;

    // Constructors
    Vec3() : x(0.0), y(0.0), z(0.0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
    Vec3(double arr[3]) : x(arr[0]), y(arr[1]), z(arr[2]) {}
    Vec3(float arr[3]) : x(arr[0]), y(arr[1]), z(arr[2]) {}

    // Addition
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    // Subtraction
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    // Scalar multiplication (vector * scalar)
    Vec3 operator*(double scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    // Scalar multiplication (scalar * vector)
    friend Vec3 operator*(double scalar, const Vec3& vec) {
        return Vec3(vec.x * scalar, vec.y * scalar, vec.z * scalar);
    }

    // Compound assignment operators
    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vec3& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    bool isnan() const {
        return std::isnan(x) || std::isnan(y) || std::isnan(z);
    }

    double normSquared() const {
        return x*x + y*y + z*z;
    }
};

inline Vec3 elementwiseMul(const Vec3& a, const Vec3& b) {
    return Vec3 {a.x * b.x, a.y * b.y, a.z * b.z};
}

// Optional: output stream for easy printing
inline std::ostream& operator<<(std::ostream& os, const Vec3& v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}