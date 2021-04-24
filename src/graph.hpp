#pragma once
#include <clipper.hpp>
#include "outline_holder.hpp"

#define ALGORITHM_DIJKSTRA 1
#define ALGORITHM_A_STAR 2
#define ALGORITHM_CH_DIJSKTRA 3


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
    float distance;
    long long duration;
};
using PathData = struct PathData;

ClipperLib::IntPoint getRandomPoint();

class Graph {
    public:
        Graph(OutlineHolder& outline_holder, size_t N);
        Graph(const char * filename);
        void output(const char * filename);
        void output(std::ostream& out);
        void output_geojson(const char * filename);
        void output_geojson(std::ostream& out);
        void output_geojsonCH(std::ostream& out);
        void printEdgeLengths(char * output_s);
        size_t getIndex(size_t x, size_t y);
        static size_t distance(const Node& a, const Node& b);
        size_t getNearestNode(const ClipperLib::IntPoint& x);
        PathData getPathDijkstra(size_t from, size_t to);
        PathData getPathCHDijkstra(size_t from, size_t to);
        PathData getPathAStar(size_t from, size_t to);

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

PathData findPath(Graph* graph, float x1, float y1, float x2, float y2, int algorithm);
