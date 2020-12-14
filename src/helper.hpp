#pragma once
#include <clipper.hpp>
#include <list>

#define PRECISION 1e5

#define toString(x) (x / PRECISION)
#define LENGTH(T, l) (sizeof(T) * (l))


template<typename T>
inline ClipperLib::cInt toInt(T x) {
    return x * PRECISION;
}

bool checkForJumpCrossing(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b);
void checkForJumpCrossing(std::list<ClipperLib::Path>& paths);
