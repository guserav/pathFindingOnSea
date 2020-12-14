#include "import_paths.hpp"
#include "helper.hpp"
#include <fstream>

void paths_import::readIn(std::list<ClipperLib::Path>& paths, const char * filename) {
    std::ifstream input(filename, std::ios_base::binary);
    paths_import::readIn(paths, input);
}

void paths_import::readIn(std::list<ClipperLib::Path>& paths, std::istream& input) {
    size_t path_size = 0;
    while(EOF != input.peek()) {
        input.read((char *) &path_size, LENGTH(path_size, 1));
        paths.emplace_back(path_size);
        input.read((char *) paths.back().data(), LENGTH(paths.back().front(), path_size));
        if(!ClipperLib::Orientation(paths.back())) {
            std::reverse(paths.back().begin(), paths.back().end());
        }
    }
}
