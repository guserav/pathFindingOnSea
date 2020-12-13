#include <ostream>
#include <clipper.hpp>

//Class to pass if no ouput is intended
class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(nullptr) {}
    NullStream(const NullStream &) : std::ostream(nullptr) {}
};

template <class T>
const NullStream &operator<<(NullStream &&os, const T &value) {
    return os;
}

void concatenatePathsAndOutput(std::list<ClipperLib::Path>& source, std::ostream& geojson_output, std::ostream& path_output);
