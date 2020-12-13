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

#include "output.hpp"
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
