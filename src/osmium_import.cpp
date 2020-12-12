#include "osmium_import.hpp"
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/visitor.hpp>
#include <clipper.hpp>

#include "output_geojson.hpp"
#include "helper.hpp"

class MyHandler : public osmium::handler::Handler {
    public:
        std::list<ClipperLib::Path> paths;
        void way(const osmium::Way& way) {
            if(!way.tags().has_tag("natural", "coastline")) {
                return;
            }
            paths.push_back(ClipperLib::Path());
            for(const auto& node : way.nodes()) {
                paths.back().emplace_back(toInt(node.lon()), toInt(node.lat()));
            }
        }
};

bool compare(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b) {
    if (a.X < b.X) {
        return true;
    }
    if (a.X > b.X) {
        return false;
    }
    return a.Y < b.Y;
}

struct Marker {
    ClipperLib::Path * pointer = nullptr;
    bool used = false;
};

struct SortElement {
    Marker * pointer;
    ClipperLib::IntPoint point;
};

bool compareSortElements (const SortElement& first, const SortElement& second) {
    return compare(first.point, second.point);
}

Marker * findPoint(const std::vector<SortElement>& e, const ClipperLib::IntPoint& p) {
    size_t l = 0;
    size_t h = e.size() - 1;
    while(l + 1 != h) {
        size_t m = (l + h) / 2;
        if(compare(e[m].point, p)) {
            l = m;
        } else {
            h = m;
        }
    }
    if(!l) {
        h = l;
    }
    // l can't mark the point we know the compare evaluated to true at least one
    if (e[h].point == p) {
        if(!e[h].pointer->used) {
            return e[h].pointer;
        }
    }
    // We assume that only exactly 2 points are matching
    if (h + 1 < e.size()) {
        if (e[h + 1].point == p){
            if (!e[h + 1].pointer->used){
                return e[h + 1].pointer;
            }
        }
    }
    return nullptr;
}

void concatenatePathsAndOutput(std::list<ClipperLib::Path>& source, std::ostream& geojson_output, std::ostream& path_output) {
    std::cerr << "Starting with " << source.size() << " paths\n";
    std::vector<SortElement> sortedByStart(source.size());
    std::vector<SortElement> sortedByEnd(source.size());
    std::vector<Marker> markers(source.size());
    size_t i = 0;
    // Setup wrapper construct to find matching paths faster
    for(ClipperLib::Path& p:source) {
        markers[i] = {.pointer = &p, .used = false};
        sortedByStart[i] = {.pointer = &(markers[i]), .point = p.front()};
        sortedByEnd[i] = {.pointer = &(markers[i]), .point = p.back()};
        i++;
    }
    std::sort(sortedByStart.begin(), sortedByStart.end(), compareSortElements);
    std::sort(sortedByEnd.begin(), sortedByEnd.end(), compareSortElements);
    geojson_output::outputPathsStart(geojson_output);
    bool first = true;
    size_t new_amount = 0;
    for(i = 0; i < markers.size(); i++) {
        std::cerr << "\r" << i;
        if(markers[i].used) continue;
        if(new_amount++) std::cout << ",";
        first = false;
        markers[i].used = true;
        ClipperLib::Path& current = *markers[i].pointer;
        ClipperLib::IntPoint end = current.back();
        geojson_output::outputPathStart(geojson_output, current);
        geojson_output::outputPathMiddle<false>(geojson_output, current);
        while(current.front() != end) {
            Marker * s = findPoint(sortedByStart, end);
            Marker * e = findPoint(sortedByEnd, end);
            if(s) {
                assert(end == s->pointer->front());
                geojson_output::outputPathMiddle<false>(geojson_output, *s->pointer);
                end = s->pointer->back();
                s->used = true;
            } else if (e) {
                assert(end == s->pointer->back());
                geojson_output::outputPathMiddle<true>(geojson_output, *e->pointer);
                end = e->pointer->front();
                e->used = true;
            } else {
                std::cerr << "Couldn't find another path to append" << std::endl;
                throw std::runtime_error("Couldn't find another path to append");
            }
        }
        geojson_output::outputPathEnd(geojson_output);
    }
    geojson_output::outputPathsEnd(geojson_output);
    std::cerr << "\nEnding with " << new_amount << " paths\n";
}


void importAndOutputFile(const char * inputFilename, std::ostream& outputGeojson, std::ostream& outputPaths) {
    osmium::io::File infile(inputFilename);
    auto otypes = osmium::osm_entity_bits::node | osmium::osm_entity_bits::way;
    osmium::io::Reader reader{infile, otypes};

    namespace map = osmium::index::map;
    using index_type = map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
    using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

    index_type index;
    location_handler_type location_handler{index};

    MyHandler handler;
    osmium::apply(reader, location_handler, handler);
    reader.close();
    std::cerr << "Done reading in paths." << std::endl;
    concatenatePathsAndOutput(handler.paths, outputGeojson, outputPaths);

}
