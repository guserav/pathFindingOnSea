#include "checks.hpp"
#include "helper.hpp"
#include <iostream>
#include "import_paths.hpp"

constexpr size_t BUCKETS = 20;


void checkMaximumError(const char * file) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, file);
    checkMaximumError(paths);
}

float getErrorDistance(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b) {
    const ClipperLib::IntPoint midSphere = getMidpoint(a, b);
    const ClipperLib::IntPoint midCart = {(a.X + b.X) / 2, (a.Y + b.Y) / 2};
    return calculate_distance(midSphere, midCart);
}

void checkMaximumError(std::list<ClipperLib::Path> paths) {
    float min = 1e10;
    float avg = 0;
    float avg_2 = 0;
    size_t c = 0;
    float max = 0;
    size_t buckets[BUCKETS] = {0};
    for(const auto& p:paths) {
        for(size_t i = 1; i < p.size(); i++) {
            const ClipperLib::IntPoint& a = p[i-1];
            const ClipperLib::IntPoint& b = p[i];
            float distance = getErrorDistance(a, b);
            c++;
            avg += distance;
            avg_2 += distance * distance;
            long long bucket = std::log(distance);
            if(bucket < 0) {
                bucket = 0;
            } else {
                bucket += 1;
            }
            if(bucket >= BUCKETS) {
                bucket = BUCKETS - 1;
            }
            buckets[bucket]++;
            if(min > distance) {
                min = distance;
            }
            if(max < distance) {
                max = distance;
            }
        }
    }
    std::cerr << "Maximum distance in meter: " << max << std::endl;
    std::cerr << "Minimum distance in meter: " << min << std::endl;
    avg /= c;
    avg_2 /= c;
    std::cerr << "Average distance in meter: " << avg << std::endl;
    std::cerr << "varianz in meter: " <<  avg_2 - (avg * avg) << std::endl;
    for(size_t i = 0; i < BUCKETS; i++) {
        std::cerr << "Below " << std::pow(10, i) << ": " << buckets[i] << std::endl;
    }
}
