#pragma once
#include <clipper.hpp>

struct Node {
    ClipperLib::IntPoint position;
    size_t edge_offset;
    bool onWater;
};
using Node = struct Node;

struct Edge {
    size_t dest;
    size_t length;
};
using Edge = struct Edge;

struct PathData {
    ClipperLib::Path path;
    size_t length;
};
using PathData = struct PathData;

class Graph {
    public:
        Graph(std::list<ClipperLib::Path>& polygons, size_t N);
        Graph(const char * filename);
        void output(const char * filename);
        void output(std::ostream& out);
        void output_geojson(const char * filename);
        void output_geojson(std::ostream& out);
        size_t getIndex(size_t x, size_t y);
        static size_t distance(const Node& a, const Node& b);
        size_t getNearestNode(const ClipperLib::IntPoint& x);
        PathData getPathDijkstra(const ClipperLib::IntPoint& from, const ClipperLib::IntPoint& to);

    private:
        void addEdgeIfNodeExists(long long x, long long y, const Node& node);
        std::vector<Edge> edges;
        std::vector<Node> nodes;
        size_t pointsInX;
        size_t pointsInY;
};
