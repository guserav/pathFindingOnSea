#include <iostream>
#include "stdlib.h"
#include "stdio.h"
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

#define PRECISION 1e5
template<typename T>
inline int toInt(T x) {
    return x * PRECISION;
}

class MyHandler : public osmium::handler::Handler {
    public:
        std::list<ClipperLib::Path> paths;
        void way(const osmium::Way& way) {
            if(!way.tags().has_tag("natural", "coastline")) {
                std::cerr << way.id() << " doesn't have natural=coastline\n";
                return;
            }
            paths.push_back(ClipperLib::Path());
            for(const auto& node : way.nodes()) {
                paths.back().emplace_back(toInt(node.lon()), toInt(node.lat()));
            }
        }
};

#define toString(x) (x / PRECISION)
void outputPoint(ClipperLib::IntPoint& p) {
    std::cout << "\n      [" << toString(p.X) << ", " << toString(p.Y) << "]";
}

void outputPath(ClipperLib::Path& path) {
    std::cout << "{\n";
    std::cout << "  \"type\": \"Feature\",\n";
    std::cout << "  \"geometry\": {";
    std::cout << "    \"type\": \"LineString\",\n";
    std::cout << "    \"coordinates\": [";
    outputPoint(path[0]);
    for(int i=1; i < path.size(); i++) {
        std::cout << ",";
        outputPoint(path[i]);
    }
    std::cout << "    \n]\n";
    std::cout << "  }\n";
    std::cout << "}\n";
}

void outputPath(std::list<ClipperLib::Path> path) {
    std::cout << "{\n";
    std::cout << "  \"type\": \"Feature\",\n";
    std::cout << "  \"geometry\": {";
    std::cout << "    \"type\": \"LineString\",\n";
    std::cout << "    \"coordinates\": [";
    ClipperLib::IntPoint end = path.front().front();
    outputPoint(end);
    for(auto& other : path) {
        if(other.front() == end) {
            for(int i=1; i < other.size(); i++) {
                std::cout << ",";
                outputPoint(other[i]);
            }
            end = other.back();
        } else if (other.back() == end){
            for(int i= other.size() - 2; i >= 0; i--) {
                std::cout << ",";
                outputPoint(other[i]);
            }
            end = other.front();
        } else {
            std::cerr << "Paths were supposed to match" << std::endl;
            throw std::runtime_error("Paths were supposed to match");
        }
    }
    std::cout << "    \n]\n";
    std::cout << "  }\n";
    std::cout << "}\n";
}

void outputPaths(std::list<ClipperLib::Path>& paths) {
    std::cout << "{\n\"type\": \"FeatureCollection\",\n \"crs\": {\n \"type\": \"name\",\n \"properties\": {\n \"name\": \"EPSG:4326\"\n }\n },\n \"features\": [\n";
    std::list<ClipperLib::Path>::iterator it = paths.begin();
    outputPath(*it);
    for (++it; it != paths.end(); ++it){
        std::cout << ",";
        outputPath(*it);
    }
    std::cout << "]}\n";
}

bool compareBasedOnX (const ClipperLib::Path& first, const ClipperLib::Path& second) {
    return first[0].X < second[0].X;
}

void concatenatePaths(std::list<ClipperLib::Path>& source, std::list<ClipperLib::Path>& dest) {
    std::cerr << "Starting with " << source.size() << " paths\n";
    //source.sort(compareBasedOnX);
    while(source.size()) {
        std::list<ClipperLib::Path> toAdd;
        toAdd.splice(toAdd.end(), source, source.begin());
        ClipperLib::Path& current = toAdd.back();
        ClipperLib::IntPoint end = current.back();
        size_t amountToAdd = 0;
        while(current[0] != end) {
            bool found = false;
            for(auto it = source.begin(); it != source.end(); it++) {
                const ClipperLib::Path& other = *it;
                if(other.front() == end) {
                    end = other.back();
                    found = true;
                } else if (other.back() == end){
                    end = other.front();
                    found = true;
                }
                if(found) {
                    amountToAdd += other.size() - 1; // deduplicating merge point
                    toAdd.splice(toAdd.end(), source, it);
                    std::cerr << "\r" << source.size();
                    break;
                }
            }
            if(!found) {
                std::cerr << "Couldn't find another path to append" << std::endl;
                throw std::runtime_error("Couldn't find another path to append");
            }
        }
        outputPath(toAdd);
        /*current.reserve(current.size() + amountToAdd);
        for(auto& other : toAdd) {
            end = current.back();
            if(other.front() == end) {
                for(int i=1; i < other.size(); i++) {
                    current.push_back(other[i]);
                }
            } else if (other.back() == end){
                for(int i= other.size() - 2; i >= 0; i--) {
                    current.push_back(other[i]);
                }
            } else {
                std::cerr << "Paths were supposed to match" << std::endl;
                throw std::runtime_error("Paths were supposed to match");
            }
        }*/
    }
    std::cerr << "\nNow only " << dest.size() << " paths\n";
}

int main(int argc, char ** argv) {
    if (argc < 2) return 10;
    osmium::io::File infile(argv[1]);
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
    std::list<ClipperLib::Path> concatenatedPaths;
    std::cerr << "Done reading in paths." << std::endl;
    concatenatePaths(handler.paths, concatenatedPaths);
    //outputPaths(concatenatedPaths);

    /*osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonCollector<osmium::area::Assembler>  collector(assembler_config);
    osmium::io::Reader reader1(infile, osmium::osm_entity_bits::relation);
    collector.read_relations(reader1);
    reader1.close();
    std::cerr << "First pass done" << std::endl;
    // pass 2
    index_type index;
    cache_type cache{index};
    cache.ignore_errors();
    MyHandler handler;
    osmium::io::Reader reader2(infile);
    apply(reader2, cache, collector.handler([&handler](
        const osmium::memory::Buffer &&buffer) { osmium::apply(buffer, handler); }));
    reader2.close();
    std::cerr << "Done" << std::endl;*/
}
