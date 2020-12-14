#include <iostream>
#include <fstream>
#include "stdlib.h"
#include "stdio.h"
#include "osmium_import.hpp"
#include "output.hpp"
#include "import_paths.hpp"
#include "helper.hpp"
#include "graph.hpp"


int main(int argc, char ** argv) {
    if (argc < 4) return 10;
    std::ifstream input(argv[1], std::ios_base::binary);
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, input);
    checkForJumpCrossing(paths);
    Graph g(paths, 1e3);
    std::ofstream output(argv[2], std::ios_base::binary);
    std::ofstream output_geojson(argv[3]);
    g.output(output);
    g.output_geojson(output_geojson);
}
