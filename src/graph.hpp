#pragma once
#include <clipper.hpp>
#include "outline_holder.hpp"

struct Node {
    ClipperLib::IntPoint position;
    size_t edge_offset;
    bool onWater;
};
using Node = struct Node;

struct NodeCH {
    ClipperLib::IntPoint position;
    size_t edge_offset;
    size_t priority;
};
using NodeCH = struct NodeCH;

struct Edge {
    size_t dest;
    size_t length;
};
using Edge = struct Edge;

struct EdgeCH {
    size_t dest;
    size_t length;
    size_t p1, p2; // Edge index of first and part
};
using EdgeCH = struct EdgeCH;

struct PathData {
    ClipperLib::Path path;
    size_t heap_accesses;
    size_t length;
};
using PathData = struct PathData;

class Graph {
    public:
        Graph(OutlineHolder& outline_holder, size_t N);
        Graph(const char * filename);
        void output(const char * filename);
        void output(std::ostream& out);
        void output_geojson(const char * filename);
        void output_geojson(std::ostream& out);
        size_t getIndex(size_t x, size_t y);
        static size_t distance(const Node& a, const Node& b);
        size_t getNearestNode(const ClipperLib::IntPoint& x);
        PathData getPathDijkstra(const ClipperLib::IntPoint& from, const ClipperLib::IntPoint& to);
        PathData getPathAStar(const ClipperLib::IntPoint& from, const ClipperLib::IntPoint& to);

    private:
        void generateGraph(OutlineHolder& outline_holder, size_t N);
        void generateCH();
        void addEdgeIfNodeExists(long long x, long long y, const Node& node);
        std::vector<Edge> edges;
        std::vector<Node> nodes;
        //std::vector<size_t> node_map; // Maps from normal node index to CH node index
        std::vector<EdgeCH> edges_ch;
        std::vector<NodeCH> nodes_ch;
        size_t pointsInX;
        size_t pointsInY;
};
