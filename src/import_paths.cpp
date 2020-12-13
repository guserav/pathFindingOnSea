#include "import_paths.hpp"

#define LENGTH(T, l) (sizeof(T) * l)

void paths_import::readIn(std::list<ClipperLib::Path>& paths, std::istream& input) {
    size_t path_size = 0;
    while(EOF != input.peek()) {
        input.read((char *) &path_size, LENGTH(path_size, 1));
        paths.emplace_back(path_size);
        input.read((char *) paths.back().data(), LENGTH(paths.back().front(), path_size));
    }
}
