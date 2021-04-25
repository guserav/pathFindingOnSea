#pragma once

#include <clipper.hpp>
#include <list>

class OutlineHolder {
    public:
        OutlineHolder(const std::list<ClipperLib::Path>& polygons, bool initboundingBoxes = true);

        virtual bool isPointInWater(const ClipperLib::IntPoint& p) = 0;

    protected:
        const std::list<ClipperLib::Path>& polygons;
        std::vector<ClipperLib::IntRect> boundingBoxes;
};

class OutlineHolderSimple : public OutlineHolder {
    public:
        OutlineHolderSimple(const std::list<ClipperLib::Path>& polygons, long p1 = 0, long p2 = 0): OutlineHolder(polygons) {};
        virtual bool isPointInWater(const ClipperLib::IntPoint& p);
};

class RectangleOutlineHolder : public OutlineHolder {
    public:
        RectangleOutlineHolder(const std::list<ClipperLib::Path>& polygons);
        virtual bool isPointInWater(const ClipperLib::IntPoint& p);
    protected:
        std::vector<ClipperLib::Paths> smallerPolygons;
        std::vector<ClipperLib::IntRect> smallerPolyBoundaries;
};

class OutlineTree {
    public:
        OutlineTree(const std::list<ClipperLib::Path>& source, long max_count = 500, long max_region_size = -1, bool inX = true);
        ~OutlineTree();
        bool containsPoint(const ClipperLib::IntPoint& p);
    private:
        template<bool inX> void init(const std::list<ClipperLib::Path>& source, const ClipperLib::IntRect& boundary, long max_count, long max_region_size);
        bool inX;
        ClipperLib::IntRect boundary;
        OutlineTree * low;
        OutlineTree * high;
        std::vector<ClipperLib::Path> polygons;
        std::vector<ClipperLib::IntRect> boundaries;
};

class TreeOutlineHolder : public OutlineHolder {
    public:
        TreeOutlineHolder(const std::list<ClipperLib::Path>& polygons, long max_count = 500, long max_region_size = -1);
        virtual bool isPointInWater(const ClipperLib::IntPoint& p);
    protected:
        OutlineTree tree_root;
};

