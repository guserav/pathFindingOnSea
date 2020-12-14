#pragma once

#include <clipper.hpp>
#include <istream>
#include <list>
#include <ostream>

namespace paths_import {
    void readIn(std::list<ClipperLib::Path>& paths, std::istream& input);
    void readIn(std::list<ClipperLib::Path>& paths, const char * filename);
}
