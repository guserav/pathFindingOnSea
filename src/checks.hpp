#pragma once

#include <clipper.hpp>
#include <list>

void checkMaximumError(const char * file);
void checkMaximumError(std::list<ClipperLib::Path> paths);
