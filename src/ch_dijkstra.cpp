#include "graph.hpp"

#define DISCOVERED_FROM 1
#define DISCOVERED_TO 2
#define DISCOVERED_BOTH 3

struct NodeCHDijkstraData {
    size_t distanceHere;
    size_t prev;
    size_t prevEdge; // Edge in previous graph used
    uint8_t from;
};
using NodeCHDijkstraData = struct NodeCHDijkstraData;

struct HeapElemCHDijkstra {
    size_t distanceHere;
    size_t prev;
    size_t prevEdge; // Edge in previous graph used
    size_t node;
    uint8_t from;
};
using HeapElemCHDijkstra = struct HeapElemCHDijkstra;

bool compareCHDijkstra(const HeapElemCHDijkstra& a, const HeapElemCHDijkstra& b) {
    return a.distanceHere > b.distanceHere;
}

void addEdge(const std::vector<Node>& nodes, const std::vector<EdgeCH>& edges_ch, const EdgeCH& edge, ClipperLib::Path& p) {
    if(edge.p1 != SIZE_MAX) {
        addEdge(nodes, edges_ch, edges_ch[edge.p2], p);
        p.push_back(nodes[edges_ch[edge.p1].dest].position);
        addEdge(nodes, edges_ch, edges_ch[edge.p1], p);
    }
}

void addPath(const std::vector<Node>& nodes, const std::vector<EdgeCH>& edges_ch, const std::vector<NodeCHDijkstraData>& dijkstraData, size_t current, ClipperLib::Path& p, size_t goal) {
    p.push_back(nodes[current].position);
    while(current != goal) {
        size_t edge = dijkstraData[current].prevEdge;
        size_t current = dijkstraData[current].prev;
        addEdge(nodes, edges_ch, edges_ch[edge], p);
        p.push_back(nodes[current].position);
    }

}

PathData Graph::getPathCHDijkstra(const ClipperLib::IntPoint& from, const ClipperLib::IntPoint& to) {
    PathData ret{.heap_accesses = 0, .length = 0};
    size_t fromIndex = getNearestNode(from);
    size_t toIndex = getNearestNode(to);

    std::vector<NodeCHDijkstraData> dijkstraData(nodes_ch.size(), {.distanceHere = SIZE_MAX, .prev = SIZE_MAX, .from = 0});
    dijkstraData[fromIndex].from = DISCOVERED_FROM;
    dijkstraData[toIndex].from = DISCOVERED_TO;
    std::vector<HeapElemCHDijkstra> dijkstraHeap;
    std::make_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
    dijkstraHeap.push_back({.distanceHere = 0, .prev = fromIndex, .node = fromIndex, .from = DISCOVERED_FROM});
    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
    dijkstraHeap.push_back({.distanceHere = 0, .prev = toIndex, .node = toIndex, .from = DISCOVERED_TO});
    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
    size_t midPoint = SIZE_MAX;
    size_t otherDistance = SIZE_MAX;
    size_t otherPrev = SIZE_MAX;

    while(dijkstraHeap.size()) {
        ret.heap_accesses++;
        std::pop_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
        HeapElemCHDijkstra current = dijkstraHeap.back();
        dijkstraHeap.pop_back();


        NodeCHDijkstraData& node = dijkstraData[current.node];
        node.from |= current.from;
        if(DISCOVERED_BOTH == node.from) {
            midPoint = current.node;
            otherDistance = current.distanceHere;
            otherPrev = current.prev;
            break;
        }
        if(current.distanceHere < node.distanceHere) {
            node.distanceHere = current.distanceHere;
            node.prev = current.prev;

            for(size_t i = nodes_ch[current.node].edge_offset; i < nodes_ch[current.node + 1].edge_offset; i++) {
                const EdgeCH& edge = edges_ch[i];
                size_t dist = current.distanceHere + edge.length;
                const NodeCHDijkstraData& otherNode = dijkstraData[edge.dest];
                bool otherFoundFromOtherDirection = false;
                if(node.from == DISCOVERED_FROM) {
                    otherFoundFromOtherDirection = (DISCOVERED_TO == otherNode.from);
                } else {
                    otherFoundFromOtherDirection = (DISCOVERED_FROM == otherNode.from);
                }

                bool correctDirection = nodes_ch[current.node].priority < nodes_ch[edge.dest].priority;
                if(correctDirection && (otherFoundFromOtherDirection || dist < otherNode.distanceHere)) {
                    dijkstraHeap.push_back({.distanceHere = dist, .prev = current.node, .node = edge.dest, .from = current.from});
                    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
                }
            }
        }
    }

    if(midPoint != SIZE_MAX) {
        ret.length = otherDistance + dijkstraData[midPoint].distanceHere;
        ClipperLib::Path p1, p2;
        size_t current = midPoint;
        if(DISCOVERED_FROM == dijkstraData[midPoint].from) {
            //addPath(nodes, edges_ch, dijkstraData, midPoint, p1, fromIndex);
        } else {
            //addPath(nodes, edges_ch, dijkstraData, midPoint, p1, toIndex);
        }
        // TODO
        ret.path = p1;
    }
    return ret;
}
