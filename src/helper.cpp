#include "helper.hpp"
#include <iostream>

bool checkForJumpCrossing(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b) {
    return a.X * b.X < toInt(-50) * toInt(50);
}

void checkForJumpCrossing(std::list<ClipperLib::Path>& paths) {
    for(const ClipperLib::Path& path : paths) {
        for(size_t i = 1; i < path.size(); i++) {
            const ClipperLib::IntPoint& prev = path[i - 1];
            const ClipperLib::IntPoint& now = path[i];
            if(checkForJumpCrossing(prev, now)) {
                std::cout << "These points cross the boundary: (" << toString(prev.X) <<  "," << toString(prev.Y) << ") -> (" << toString(now.X) <<  "," << toString(now.Y) << ")" << std::endl;
            }
        }
    }
}
