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


int main(int argc, char ** argv) {
    if (argc < 2) return 10;

    checkMaximumError(argv[1]);

    //Graph g(argv[1]);
    //ClipperLib::IntPoint from{toInt(-5.1), toInt(42)};
    //ClipperLib::IntPoint to{toInt(-37), toInt(37)};
    //PathData path = g.getPath(from, to);

    //std::ofstream out(argv[2]);
    //geojson_output::outputPathsStart(out);
    //geojson_output::outputPath(out, path.path);
    //geojson_output::outputPathsEnd(out);
    //std::cerr << path.length << std::endl;
}
