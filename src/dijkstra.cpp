#include "graph.hpp"

struct NodeDijkstraData {
    size_t distanceHere;
    size_t prev;
};
using NodeDijkstraData = struct NodeDijkstraData;

struct HeapElemDijkstra {
    size_t distanceHere;
    size_t prev;
    size_t node;
};
using HeapElemDijkstra = struct HeapElemDijkstra;

bool compareDijkstra(const HeapElemDijkstra& a, const HeapElemDijkstra& b) {
    return a.distanceHere > b.distanceHere;
}

PathData Graph::getPathDijkstra(const ClipperLib::IntPoint& from, const ClipperLib::IntPoint& to) {
    PathData ret{.heap_accesses = 0, .length = 0};
    size_t fromIndex = getNearestNode(from);
    size_t toIndex = getNearestNode(to);

    std::vector<NodeDijkstraData> dijkstraData(nodes.size(), {.distanceHere = SIZE_MAX, .prev = 0});
    std::vector<HeapElemDijkstra> dijkstraHeap;
    std::make_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareDijkstra);
    dijkstraHeap.push_back({.distanceHere = 0, .prev = fromIndex, .node = fromIndex});
    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareDijkstra);

    while(dijkstraHeap.size()) {
        ret.heap_accesses++;
        std::pop_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareDijkstra);
        HeapElemDijkstra current = dijkstraHeap.back();
        dijkstraHeap.pop_back();

        NodeDijkstraData& node = dijkstraData[current.node];
        if(current.distanceHere < node.distanceHere) {
            node.distanceHere = current.distanceHere;
            node.prev = current.prev;

            for(size_t i = nodes[current.node].edge_offset; i < nodes[current.node + 1].edge_offset; i++) {
                const Edge& edge = edges[i];
                size_t dist = current.distanceHere + edge.length;
                const NodeDijkstraData& otherNode = dijkstraData[edge.dest];

                if(dist < otherNode.distanceHere) {
                    dijkstraHeap.push_back({.distanceHere = dist, .prev = current.node, .node = edge.dest});
                    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareDijkstra);
                }
            }
            if(current.node == toIndex){
                ret.length = current.distanceHere;
                break;
            }
        }
    }

    size_t current = toIndex;
    if(dijkstraData[toIndex].distanceHere != SIZE_MAX) {
        ret.path.emplace_back(nodes[current].position);
        while(current != fromIndex) {
            const NodeDijkstraData& c = dijkstraData[current];
            current = c.prev;
            ret.path.emplace_back(nodes[current].position);
        }
    }
    // The path is reverse but we only want to display it so it doesn't matter
    return ret;
}
