#pragma once
#include <clipper.hpp>
#include <list>
#include <cmath>

#define PRECISION 1e5

#define toString(x) (x / PRECISION)
#define LENGTH(T, l) (sizeof(T) * (l))


template<typename T>
inline ClipperLib::cInt toInt(T x) {
    return x * PRECISION;
}

inline float toRad(ClipperLib::cInt x) {
    return x * M_PI/(180 * PRECISION);
}

inline float calculate_distance(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b) {
    const float R = 6371e3; // metres
    const float a_Lat = toRad(a.Y);
    const float b_Lat = toRad(b.Y);
    const float latDiff = toRad(a.Y - b.Y);
    const float lonDiff = toRad(a.X - b.X);
    const float A = std::sin(latDiff/2) * std::sin(latDiff/2) +
                std::cos(a_Lat) * std::cos(b_Lat) *
                std::sin(lonDiff/2) * std::sin(lonDiff/2);
    const float c = 2 * std::atan2(std::sqrt(A), std::sqrt(1-A));
    const float d = R * c; // in metres
    return d;
}

bool checkForJumpCrossing(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b);
bool checkForJumpCrossing(std::list<ClipperLib::Path>& paths);
