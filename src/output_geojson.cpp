#include "output_geojson.hpp"
#include "helper.hpp"

void geojson_output::outputPoint(std::ostream& out, const ClipperLib::IntPoint& p) {
    out << "\n      [" << toString(p.X) << ", " << toString(p.Y) << "]";
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
