#include "outline_holder.hpp"
#include "helper.hpp"
#include <cassert>
#include <iostream>

void calculateOutlines(const ClipperLib::Path& path, ClipperLib::IntRect& rect) {
    rect.left = path.front().X;
    rect.right = path.front().X;
    rect.bottom = path.front().Y;
    rect.top = path.front().Y;

    for(const auto& point : path) {
        if(point.X < rect.left) {
            rect.left = point.X;
        } else if(point.X > rect.right){
            rect.right = point.X;
        }
        if(point.Y < rect.bottom) {
            rect.bottom = point.Y;
        } else if(point.Y > rect.top){
            rect.top = point.Y;
        }
    }
}

template <class T>
void calculateOutlines(const T& paths, ClipperLib::IntRect& outline) {
    calculateOutlines(paths.front(), outline);
    for(auto p = paths.begin()++; p != paths.end(); p++) {
        ClipperLib::IntRect currentOutlines;
        calculateOutlines(*p, currentOutlines);
        if(currentOutlines.left < outline.left) {
            outline.left = currentOutlines.left;
        }
        if(currentOutlines.right > outline.right) {
            outline.right = currentOutlines.right;
        }
        if(currentOutlines.top > outline.top) {
            outline.top = currentOutlines.top;
        }
        if(currentOutlines.bottom < outline.bottom) {
            outline.bottom = currentOutlines.bottom;
        }
    }
}

inline bool intervalIntersection(ClipperLib::cInt a, ClipperLib::cInt b, ClipperLib::cInt x, ClipperLib::cInt y) {
    assert(a <= b);
    assert(x <= y);
    if (b < x) {
        return false;
    }
    if (y < a) {
        return false;
    }
    return true;
}

inline bool rectRectIntersect(const ClipperLib::IntRect& a, const ClipperLib::IntRect& b) {
    return intervalIntersection(a.left, a.right, b.left, b.right) && intervalIntersection(a.bottom, a.top, b.bottom, b.top);
}


OutlineHolder::OutlineHolder(const std::list<ClipperLib::Path>& polygons, bool initboundingBoxes):polygons(polygons), boundingBoxes(initboundingBoxes?polygons.size() : 0) {
    if(initboundingBoxes) {
        size_t i = 0;
        for(auto it = polygons.begin(); it != polygons.end(); it++) {
            calculateOutlines(*it, boundingBoxes[i]);
            i++;
        }
    }
}

bool OutlineHolderSimple::isPointInWater(const ClipperLib::IntPoint& p) {
    size_t i = 0;
    for(auto it = polygons.begin(); it != polygons.end(); it++) {
        if (isPointInRectangle(boundingBoxes[i], p)) {
            if (ClipperLib::PointInPolygon(p, *it)) {
                return false;
            }
        }
        i++;
    }
    return true;
}

/**
 * Get Path that represents the given rectangle
 */
ClipperLib::Path getRect(ClipperLib::cInt left, ClipperLib::cInt right, ClipperLib::cInt top, ClipperLib::cInt bottom) {
    assert(left <= right);
    assert(bottom <= top);
    ClipperLib::Path rect(4);
    rect[0] = ClipperLib::IntPoint(left, top);
    rect[1] = ClipperLib::IntPoint(right, top);
    rect[2] = ClipperLib::IntPoint(right, bottom);
    rect[3] = ClipperLib::IntPoint(left, bottom);
    return rect;
}

RectangleOutlineHolder::RectangleOutlineHolder(const std::list<ClipperLib::Path>& polygons):OutlineHolder(polygons) {
    std::list<ClipperLib::Paths> newListPolygons;
    for(auto& p : polygons) {
        newListPolygons.push_back({p});
    }
    bool changed = true;
    std::cerr << "Start calculating boundingBoxes" << std::endl;
    while(changed) {
        std::cerr << "\r" << newListPolygons.size();
        changed = false;
        for(auto it = newListPolygons.begin(); it != newListPolygons.end(); it++) {
            size_t size = 0;
            for(auto& p : *it) {
                size += p.size();
            }
            if (size > 1e4) {
                changed = true;
                ClipperLib::IntRect outline;
                calculateOutlines(*it, outline);
                ClipperLib::cInt xMiddle = (outline.left + outline.right) / 2;
                ClipperLib::cInt yMiddle = (outline.top + outline.bottom) / 2;
                ClipperLib::Path newBoundingBoxes[4] = {
                    getRect(outline.left, xMiddle, outline.top, yMiddle),
                    getRect(xMiddle, outline.right, outline.top, yMiddle),
                    getRect(outline.left, xMiddle, yMiddle, outline.bottom),
                    getRect(xMiddle, outline.right, yMiddle, outline.bottom)
                };
                for(size_t i = 0; i < 4; i++) {
                    ClipperLib::Paths newPaths;
                    ClipperLib::Clipper clipper;
                    clipper.AddPaths(*it, ClipperLib::ptSubject, true);
                    clipper.AddPath(newBoundingBoxes[i], ClipperLib::ptClip, true);
                    clipper.Execute(ClipperLib::ctIntersection, newPaths);
                    if(newPaths.size()) {
                        newListPolygons.push_back(newPaths);
                    }
                }
                newListPolygons.erase(it);
                break;
            }
        }
    }
    smallerPolygons.reserve(newListPolygons.size());
    smallerPolyBoundaries.reserve(newListPolygons.size());
    for(auto& p: newListPolygons) {
        ClipperLib::IntRect outline;
        smallerPolygons.push_back(p);
        calculateOutlines(p, outline);
        smallerPolyBoundaries.push_back(outline);
    }
    std::cerr << std::endl;
}

bool RectangleOutlineHolder::isPointInWater(const ClipperLib::IntPoint& p) {
    for(size_t i = 0; i < smallerPolygons.size(); i++) {
        if(isPointInRectangle(smallerPolyBoundaries[i], p)) {
            size_t count_polygons = 0;
            for(auto polygon : smallerPolygons[i] ) {
                if (ClipperLib::PointInPolygon(p, polygon)) {
                    count_polygons++;
                }
            }
            if(count_polygons % 2) {
                return false;
            }
        }
    }
    return true;
}

TreeOutlineHolder::TreeOutlineHolder(const std::list<ClipperLib::Path>& polygons, long max_count, long max_region_size):OutlineHolder(polygons, false), tree_root(polygons, max_count, max_region_size) {}

bool TreeOutlineHolder::isPointInWater(const ClipperLib::IntPoint& p) {
    if(tree_root.containsPoint(p)) {
        return false;
    }
    return true;
}

OutlineTree::OutlineTree(const std::list<ClipperLib::Path>& source, long max_count, long max_region_size, bool inX) {
    ClipperLib::IntRect b;
    calculateOutlines(source, b);
    if(inX) {
        init<true>(source, b, max_count, max_region_size);
    } else {
        init<false>(source, b, max_count, max_region_size);
    }
}


OutlineTree::~OutlineTree() {
    delete low;
    delete high;
}

/**
 * Determine the side(low/high) the given point is on based on the division line and the division direction.
 * returns true for high.
 */
template<bool inX>
bool determineSide(ClipperLib::cInt divisionLine, const ClipperLib::IntPoint& p) {
    if constexpr(inX) {
        return p.X > divisionLine;
    } else {
        return p.Y > divisionLine;
    }
}

/**
 * Calculate point on the division line based on the given parameters
 */
template<bool inX>
ClipperLib::IntPoint getPointOnDivisionLine(ClipperLib::cInt divisionLine, const ClipperLib::IntPoint& p1, const ClipperLib::IntPoint& p2) {
    ClipperLib::IntPoint p;
    if constexpr(inX) {
        p.X = divisionLine;
        p.Y = ((divisionLine - p1.X) * (p2.Y - p1.Y)) / (p2.X - p1.X) + p1.Y;
    } else {
        p.Y = divisionLine;
        p.X = ((divisionLine - p1.Y) * (p2.X - p1.X)) / (p2.Y - p1.Y) + p1.X;
    }
    return p;
}

template<bool inX>
void OutlineTree::init(const std::list<ClipperLib::Path>& source, const ClipperLib::IntRect& boundary, long max_count, long max_region_size) {
    this->inX = inX;
    this->boundary = boundary;
    size_t totalNodeCount = 0;
    for(const auto& p: source) {
        totalNodeCount += p.size();
    }
    if(totalNodeCount < max_count || (max_region_size > 0 && (boundary.right - boundary.left < max_region_size || boundary.top - boundary.bottom < max_region_size))) { // No need to divide further.
        polygons.reserve(source.size());
        polygons.insert(polygons.end(), source.begin(), source.end());
        boundaries.resize(source.size());
        for(size_t i = 0; i < polygons.size(); i++) {
            calculateOutlines(polygons[i], boundaries[i]);
        }
        low = nullptr;
        high = nullptr;
    } else {
        std::list<ClipperLib::Path> lowPolygons, highPolygons;
        ClipperLib::cInt divisionLine;
        if constexpr (inX) {
            divisionLine = (boundary.left + boundary.right) / 2;
        } else {
            divisionLine = (boundary.bottom + boundary.top) / 2;
        }
        for(const auto& current_source: source) {
            assert(source.size());
            bool endInHigher = determineSide<inX>(divisionLine, current_source.back());
            long long lastOnOtherSize = current_source.size() - 2; // Start with second last element as the last is already checked in endInHigher
            for(; lastOnOtherSize >= 0; lastOnOtherSize--) {
                if(endInHigher != determineSide<inX>(divisionLine, current_source[lastOnOtherSize])) {
                    break;
                }
            }
            if(lastOnOtherSize < 0) { // Polygon doesn't cross division line
                if(endInHigher) {
                    highPolygons.push_back(current_source);
                } else {
                    lowPolygons.push_back(current_source);
                }
            } else {
                ClipperLib::IntPoint firstDivisionPoint;
                if(endInHigher != determineSide<inX>(divisionLine, current_source.front())) {
                    endInHigher = !endInHigher; // Need to invert that as we now start adding to the first element which is oriented in the other direction
                    firstDivisionPoint = getPointOnDivisionLine<inX>(divisionLine, current_source.front(), current_source.back());
                    lastOnOtherSize = current_source.size(); // We don't can safely iterate to the end in the later for loop
                    if(endInHigher) {
                        highPolygons.emplace_back(1, firstDivisionPoint);
                    } else {
                        lowPolygons.emplace_back(1, firstDivisionPoint);
                    }
                } else {
                    firstDivisionPoint = getPointOnDivisionLine<inX>(divisionLine, current_source[lastOnOtherSize], current_source[lastOnOtherSize + 1]);
                    lastOnOtherSize++; // Now represents the size to iterate to
                    if(endInHigher) {
                        highPolygons.emplace_back(1, firstDivisionPoint);
                        highPolygons.back().insert(highPolygons.back().end(), current_source.begin() + lastOnOtherSize, current_source.end());
                    } else {
                        lowPolygons.emplace_back(1, firstDivisionPoint);
                        lowPolygons.back().insert(lowPolygons.back().end(), current_source.begin() + lastOnOtherSize, current_source.end());
                    }
                }
                bool currentInHigher = endInHigher;
                auto lastSwitch = current_source.begin();
                if(currentInHigher != determineSide<inX>(divisionLine, current_source.front())) {
                    throw std::runtime_error("This shouldn't have happened. Front has different side then expected.");
                }
                for(size_t i = 0; i < lastOnOtherSize; i++) {
                    if(currentInHigher != determineSide<inX>(divisionLine, current_source[i])) {
                        ClipperLib::IntPoint divisionPoint = getPointOnDivisionLine<inX>(divisionLine, current_source.at(i - 1), current_source.at(i));
                        if(currentInHigher) {
                            highPolygons.back().insert(highPolygons.back().end(), lastSwitch, current_source.begin() + i);
                            highPolygons.back().push_back(divisionPoint);
                            lowPolygons.emplace_back(1, divisionPoint);
                        } else {
                            lowPolygons.back().insert(lowPolygons.back().end(), lastSwitch, current_source.begin() + i);
                            lowPolygons.back().push_back(divisionPoint);
                            highPolygons.emplace_back(1, divisionPoint);
                        }
                        lastSwitch = current_source.begin() + i;
                        currentInHigher = !currentInHigher;
                    }
                }
                if(currentInHigher) {
                    highPolygons.back().insert(highPolygons.back().end(), lastSwitch, current_source.begin() + lastOnOtherSize);
                    highPolygons.back().push_back(firstDivisionPoint);
                } else {
                    lowPolygons.back().insert(lowPolygons.back().end(), lastSwitch, current_source.begin() + lastOnOtherSize);
                    lowPolygons.back().push_back(firstDivisionPoint);
                }
            }
        }
        high = new OutlineTree(highPolygons, max_count, max_region_size, !inX);
        low = new OutlineTree(lowPolygons, max_count, max_region_size, !inX);
    }
}

bool OutlineTree::containsPoint(const ClipperLib::IntPoint& p) {
    if(isPointInRectangle(boundary, p)) {
        if(high != nullptr) {
            if(inX) {
                if(determineSide<true>((boundary.left + boundary.right) / 2, p)) {
                    return high->containsPoint(p);
                } else {
                    return low->containsPoint(p);
                }
            } else {
                if(determineSide<false>((boundary.bottom + boundary.top) / 2, p)) {
                    return high->containsPoint(p);
                } else {
                    return low->containsPoint(p);
                }
            }
        } else {
            for(size_t i = 0; i < polygons.size(); i++) {
                if(isPointInRectangle(boundaries[i], p)) {
                    if(ClipperLib::PointInPolygon(p, polygons[i])) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/*OutlineHolderComplex::OutlineHolderComplex(const std::list<ClipperLib::Path>& polygons, const ClipperLib::IntRect& world_boundary, size_t divisionsInX, size_t divisionsInY) :
    OutlineHolder(polygons),
    world_boundary(world_boundary),
    divisionsInX(divisionsInX),
    divisionsInY(divisionsInY),
    divisionWidth((world_boundary.left - world_boundary.right) / divisionsInX),
    divisionHeight((world_boundary.top - world_boundary.bottom) / divisionsInY),
    boundaries(divisionsInX * divisionsInY)
{
    ClipperLib::IntRect c;
    c.right = world_boundary.left;
    std::cerr << "Start calculating boundingBoxes" << std::endl;
    for(size_t x = 0; x < divisionsInX; x++) {
        c.left = c.right;
        c.right += divisionWidth;
        if(x == divisionsInX - 1) {
            c.right = world_boundary.right;
        }

        c.top = world_boundary.bottom;
        for(size_t y = 0; y < divisionsInY; y++) {
            c.bottom = c.top;
            c.top += divisionHeight;
            if(y == divisionsInY - 1) {
                c.top = world_boundary.top;
            }
            ClipperLib::Clipper clipper;
            size_t i = 0;
            for(auto pIt = polygons.begin(); pIt != polygons.end(); pIt++, i++) {
                if(rectRectIntersect(boundingBoxes[i], c)) {
                    clipper.AddPath(*pIt, ClipperLib::ptSubject, true);
                }
            }
            ClipperLib::Path rect(4);
            rect[0] = ClipperLib::IntPoint(c.left, c.top);
            rect[1] = ClipperLib::IntPoint(c.right, c.top);
            rect[2] = ClipperLib::IntPoint(c.right, c.bottom);
            rect[3] = ClipperLib::IntPoint(c.left, c.bottom);
            clipper.AddPath(rect, ClipperLib::ptClip, true);
            std::cerr << "\r" << getIndex(x, y);
            clipper.Execute(ClipperLib::ctIntersection, boundaries[getIndex(x, y)]);
        }
    }
    std::cerr << std::endl;
}

inline size_t OutlineHolderComplex::getIndex(size_t x, size_t y) {
    return x * divisionsInY + y;
}

bool OutlineHolderComplex::isPointInWater(const ClipperLib::IntPoint& p) {
    size_t x = (p.X - world_boundary.left) / divisionWidth;
    if(x >= divisionsInX) {
        x = divisionsInX - 1;
    }
    size_t y = (p.Y - world_boundary.bottom) / divisionHeight;
    if(y >= divisionsInY) {
        y = divisionsInY - 1;
    }
    size_t count_polygons = 0;
    for(auto polygon : boundaries[getIndex(x, y)]) {
        if (ClipperLib::PointInPolygon(p, polygon)) {
            count_polygons++;
        }
    }
    return (count_polygons % 2) == 0;
} */
