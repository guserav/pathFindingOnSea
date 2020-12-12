#pragma once

#define PRECISION 1e5

#define toString(x) (x / PRECISION)

template<typename T>
inline int toInt(T x) {
    return x * PRECISION;
}
