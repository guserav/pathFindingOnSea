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

class OutlineHolderSimple : OutlineHolder {
    public:
        OutlineHolderSimple(const std::list<ClipperLib::Path>& polygons): OutlineHolder(polygons) {};
        virtual bool isPointInWater(const ClipperLib::IntPoint& p);
};

class RectangleOutlineHolder : OutlineHolder {
    public:
        RectangleOutlineHolder(const std::list<ClipperLib::Path>& polygons);
        virtual bool isPointInWater(const ClipperLib::IntPoint& p);
    protected:
        std::vector<ClipperLib::Paths> smallerPolygons;
        std::vector<ClipperLib::IntRect> smallerPolyBoundaries;
};

class OutlineTree {
    public:
        OutlineTree(const std::list<ClipperLib::Path>& source, bool inX = true);
        ~OutlineTree();
        bool containsPoint(const ClipperLib::IntPoint& p);
    private:
        template<bool inX> void init(const std::list<ClipperLib::Path>& source, const ClipperLib::IntRect& boundary);
        bool inX;
        ClipperLib::IntRect boundary;
        OutlineTree * low;
        OutlineTree * high;
        std::vector<ClipperLib::Path> polygons;
        std::vector<ClipperLib::IntRect> boundaries;
};

class TreeOutlineHolder : OutlineHolder {
    public:
        TreeOutlineHolder(const std::list<ClipperLib::Path>& polygons);
        virtual bool isPointInWater(const ClipperLib::IntPoint& p);
    protected:
        OutlineTree tree_root;
};

