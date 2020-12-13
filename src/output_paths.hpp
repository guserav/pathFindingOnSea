#pragma once
#include <ostream>
#include <clipper.hpp>

class PathsOutput {
    public:
        PathsOutput(std::ostream& out): out(out) {};

        void outputPathStart(const ClipperLib::Path& path);
        template<bool reverse> void outputPathMiddle(const ClipperLib::Path& path);
        void outputPathEnd();

    private:
        void outputPoint(const ClipperLib::IntPoint& p);
        std::ostream& out;
        std::streampos begin;
        std::streampos lastPathBegin;
        std::streampos lastEndOfPath;
        size_t sizeOfCurrentPath;
};
