#include <iostream>
#include <fstream>
#include "stdlib.h"
#include "stdio.h"
#include "osmium_import.hpp"
#include "output.hpp"
#include "output_geojson.hpp"
#include "import_paths.hpp"
#include "helper.hpp"
#include "graph.hpp"
#include "checks.hpp"

void testGraphCreation(char * filename, size_t node_count) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, filename);
    size_t distribution[sizeof(size_t) * 8] = {0};
    for(const auto& p:paths) {
        int leading_zeros = __builtin_clzll(p.size() | 1);
        distribution[sizeof(size_t) * 8 - leading_zeros - 1]++;
    }
    for(size_t i = 0; i < sizeof(size_t) * 8; i++) {
        // std::cerr << "2 ^ " << i << ": " << distribution[i] << std::endl;
    }

    Graph g(paths, node_count);
}

int main(int argc, char ** argv) {
    if (argc < 2) return 10;

    testGraphCreation(argv[1], 10000);
}
