#include <iostream>
#include <fstream>
#include <cstdlib>
#include "stdlib.h"
#include "stdio.h"
#include "osmium_import.hpp"
#include "output.hpp"
#include "output_geojson.hpp"
#include "import_paths.hpp"
#include "helper.hpp"
#include "graph.hpp"
#include "checks.hpp"

int compare(const void * a_p, const void * b_p) {
    size_t a = *(size_t *) a_p;
    size_t b = *(size_t *) b_p;
    if(a < b) {
        return -1;
    }
    if(a == b) {
        return 0;
    }
    return 1;
}

/**
 * Print all sizes of polygons to one file
 */
void getStatsOnPolygons(char * input, char * output_s) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, input);
    std::vector<size_t> sizes(paths.size());
    std::ofstream output(output_s);
    size_t i = 0;
    for(const auto& p:paths) {
        sizes[i++] = p.size();
    }
    sort(sizes.begin(), sizes.end());
    for(i = 0; i < paths.size(); i++) {
        output << std::to_string(sizes[i]) << "\n";
    }
}

/**
 * Run graph creation for a given polygon file
 */
void testGraphCreation(char * filename, size_t node_count) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, filename);
    Graph g(paths, node_count);
}

int main(int argc, char ** argv) {
    if (argc < 3) return 10;

    getStatsOnPolygons(argv[1], argv[2]);
}
