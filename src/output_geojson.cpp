#include "output_geojson.hpp"
#include "helper.hpp"

void geojson_output::outputPoint(std::ostream& out, const ClipperLib::IntPoint& p) {
    out << "\n      [" << toString(p.X) << ", " << toString(p.Y) << "]";
}

void geojson_output::outputFeaturePoint(std::ostream& out, const ClipperLib::IntPoint& p) {
    out << "{\n";
    out << "  \"type\": \"Feature\",\n";
    out << "  \"geometry\": {";
    out << "    \"type\": \"Point\",\n";
    out << "    \"coordinates\": ";
    geojson_output::outputPoint(out, p);
    out << "\n  }\n}";
}

void geojson_output::outputLine(std::ostream& out, const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b) {
    if(checkForJumpCrossing(a, b)) {
        ClipperLib::cInt distX = a.X - b.X;
        ClipperLib::cInt distY = a.Y - b.Y;
        if(a.X < b.X) distX += toInt(360);
        if(a.X > b.X) distX -= toInt(360);
        const float p = 0.1;
        distX *= p;
        distY *= p;
        ClipperLib::IntPoint p1{a.X - distX, a.Y - distY};
        ClipperLib::IntPoint p2{b.X + distX, b.Y + distY};
        geojson_output::outputLine(out, a, p1);
        out << ",";
        geojson_output::outputLine(out, b, p2);
        return;
    }
    out << "{\n";
    out << "  \"type\": \"Feature\",\n";
    out << "  \"geometry\": {";
    out << "    \"type\": \"LineString\",\n";
    out << "    \"coordinates\": [";
    geojson_output::outputPoint(out, a);
    out << ",";
    geojson_output::outputPoint(out, b);
    out << "\n  ]\n  }\n}";
}

void geojson_output::outputPathStart(std::ostream& out, const ClipperLib::Path& path) {
    out << "{\n";
    out << "  \"type\": \"Feature\",\n";
    out << "  \"geometry\": {";
    out << "    \"type\": \"LineString\",\n";
    out << "    \"coordinates\": [";
    outputPoint(out, path[0]);
}

template void geojson_output::outputPathMiddle<true>(std::ostream& out, const ClipperLib::Path& path);
template void geojson_output::outputPathMiddle<false>(std::ostream& out, const ClipperLib::Path& path);
template<bool reverse>
void geojson_output::outputPathMiddle(std::ostream& out, const ClipperLib::Path& path) {
    if constexpr(reverse) {
        for(int i=path.size() - 2; i > 0; i--) {
            out << ",";
            outputPoint(out, path[i]);
        }
    } else {
        for(int i=1; i < path.size(); i++) {
            out << ",";
            outputPoint(out, path[i]);
        }
    }
}

void geojson_output::outputPathEnd(std::ostream& out) {
    out << "    \n]\n";
    out << "  }\n";
    out << "}\n";
}

void geojson_output::outputPath(std::ostream& out, const ClipperLib::Path& path) {
    outputPathStart(out, path);
    outputPathMiddle<false>(out, path);
    outputPathEnd(out);
}

void geojson_output::outputPathsStart(std::ostream& out) {
    out << "{\n\"type\": \"FeatureCollection\",\n \"crs\": {\n \"type\": \"name\",\n \"properties\": {\n \"name\": \"EPSG:4326\"\n }\n },\n \"features\": [\n";
}

void geojson_output::outputPathsEnd(std::ostream& out) {
    out << "]}\n";
}

void geojson_output::outputPaths(std::ostream& out, std::list<ClipperLib::Path>& paths) {
    outputPathsStart(out);
    std::list<ClipperLib::Path>::iterator it = paths.begin();
    outputPath(out, *it);
    for (++it; it != paths.end(); ++it){
        out << ",";
        outputPath(out, *it);
    }
    outputPathsEnd(out);
}
