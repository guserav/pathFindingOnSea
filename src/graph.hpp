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

class Graph {
    public:
        Graph(std::list<ClipperLib::Path>& polygons, size_t N);
        void output(std::ostream& out);
        void output_geojson(std::ostream& out);
        size_t getIndex(size_t x, size_t y);
        static size_t distance(const Node& a, const Node& b);

    private:
        void addEdgeIfNodeExists(size_t x, size_t y, const Node& node);
        std::vector<Edge> edges;
        std::vector<Node> nodes;
        size_t pointsInX;
        size_t pointsInY;
};
