#include "output_paths.hpp"
#include <iostream>
#include "output.hpp"

#define LENGTH(T, l) (sizeof(T) * (l))

void PathsOutput::outputPathStart(const ClipperLib::Path& path) {
    this->lastPathBegin = out.tellp();
    this->sizeOfCurrentPath = 0;
    out.write((char *) &(this->sizeOfCurrentPath), LENGTH(this->sizeOfCurrentPath, 1));
    this->outputPoint(path.front());
}

template void PathsOutput::outputPathMiddle<true>(const ClipperLib::Path& path);
template void PathsOutput::outputPathMiddle<false>(const ClipperLib::Path& path);

template<bool reverse> void PathsOutput::outputPathMiddle(const ClipperLib::Path& path) {
    if(reverse) {
        for(int i=path.size() - 2; i > 0; i--) {
            this->outputPoint(path[i]);
        }
    } else {
        this->sizeOfCurrentPath += path.size() - 1;
        out.write((char *) &(path[1]), LENGTH(path[1], path.size() - 1));
    }
}

void PathsOutput::outputPathEnd() {
    // Jump back to beginning to set path length
    this->lastEndOfPath = out.tellp();
    out.seekp(this->lastPathBegin);
    out.write((char *) &(this->sizeOfCurrentPath), LENGTH(this->sizeOfCurrentPath, 1));
    out.seekp(this->lastEndOfPath);
}

void PathsOutput::outputPoint(const ClipperLib::IntPoint& p) {
    this->sizeOfCurrentPath++;
    out.write((char *) &p, LENGTH(p, 1));
}
