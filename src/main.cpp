#include <iostream>
#include <fstream>
#include "stdlib.h"
#include "stdio.h"
#include "osmium_import.hpp"
#include "output.hpp"
#include "import_paths.hpp"


int main(int argc, char ** argv) {
    if (argc < 2) return 10;
    std::ifstream input(argv[1], std::ios_base::binary);
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, input);
    std::cerr << "Paths read " << paths.size() << std::endl;
}
