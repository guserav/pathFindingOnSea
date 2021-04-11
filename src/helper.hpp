#pragma once
#include <clipper.hpp>
#include <list>
#include <cmath>

#define PRECISION 1e5

#define toString(x) (x / PRECISION)
#define LENGTH(T, l) (sizeof(T) * (l))

#ifndef NDEBUG
    #define ONLY_DEBUG(x)
#else
    #define ONLY_DEBUG(x) x
#endif


template<typename T>
inline ClipperLib::cInt toInt(T x) {
    return x * PRECISION;
}

inline float toRad(ClipperLib::cInt x) {
    return x * M_PI/(180 * PRECISION);
}

inline ClipperLib::cInt fromRad(float x) {
    return x * (180 * PRECISION) / M_PI;
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

inline ClipperLib::IntPoint getMidpoint(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b) {
    const float a_Lat = toRad(a.Y);
    const float b_Lat = toRad(b.Y);
    const float latDiff = toRad(b.Y - a.Y);
    const float lonDiff = toRad(b.X - a.X);

    const float Bx = std::cos(b_Lat) * std::cos(lonDiff);
    const float By = std::cos(b_Lat) * std::sin(lonDiff);
    const float c_lat = std::atan2(std::sin(a_Lat) + std::sin(b_Lat),
                std::sqrt( (std::cos(a_Lat)+Bx)*(std::cos(a_Lat)+Bx) + By*By));
    const float c_lon = toRad(a.X) + std::atan2(By, std::cos(a_Lat) + Bx);
    return {fromRad(c_lon), fromRad(c_lat)};
}

inline bool isPointInRectangle(const ClipperLib::IntRect& r, const ClipperLib::IntPoint& p) {
    if(p.X < r.left) return false;
    if(p.X > r.right) return false;
    if(p.Y < r.bottom) return false;
    if(p.Y > r.top) return false;
    return true;
}


bool checkForJumpCrossing(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b);
bool checkForJumpCrossing(std::list<ClipperLib::Path>& paths);
