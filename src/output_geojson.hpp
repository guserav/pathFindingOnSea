#pragma once
#include <clipper.hpp>
#include <ostream>

namespace geojson_output {
    void outputPoint(std::ostream& out, const ClipperLib::IntPoint& p);
    void outputPathStart(std::ostream& out, const ClipperLib::Path& path);
    template<bool reverse> void outputPathMiddle(std::ostream& out, const ClipperLib::Path& path);
    void outputPathEnd(std::ostream& out);
    void outputPath(std::ostream& out, const ClipperLib::Path& path);
    void outputPathsStart(std::ostream& out);
    void outputPathsEnd(std::ostream& out);
    void outputPaths(std::ostream& out, std::list<ClipperLib::Path>& paths);
    void outputFeaturePoint(std::ostream& out, const ClipperLib::IntPoint& p);
    void outputLine(std::ostream& out, const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b);
};
