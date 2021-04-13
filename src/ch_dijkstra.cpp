#include "graph.hpp"
#include <cassert>

#define DISCOVERED_FROM 0
#define DISCOVERED_TO 1

struct NodeCHDijkstraData {
    size_t distanceHere[2];
    size_t prev[2];
    size_t prevEdge[2]; // Edge in previous graph used
};
using NodeCHDijkstraData = struct NodeCHDijkstraData;

struct HeapElemCHDijkstra {
    size_t distanceHere;
    size_t sortValue;
    size_t prev;
    size_t prevEdge; // Edge in previous graph used
    size_t node;
    uint8_t from;
};
using HeapElemCHDijkstra = struct HeapElemCHDijkstra;

bool compareCHDijkstra(const HeapElemCHDijkstra& a, const HeapElemCHDijkstra& b) {
    return a.sortValue > b.sortValue;
}

void addEdge(const std::vector<Node>& nodes, const std::vector<EdgeCH>& edges_ch, const EdgeCH& edge, ClipperLib::Path& p) {
    if(edge.p1 != SIZE_MAX) {
        addEdge(nodes, edges_ch, edges_ch[edge.p2], p);
        p.push_back(nodes[edges_ch[edge.p1].dest].position);
        addEdge(nodes, edges_ch, edges_ch[edge.p1], p);
    }
}

void addPath(const std::vector<Node>& nodes, const std::vector<EdgeCH>& edges_ch, const std::vector<NodeCHDijkstraData>& dijkstraData, size_t current, ClipperLib::Path& p, const size_t goal, size_t from) {
    p.push_back(nodes[current].position);
    while(current != goal) {
        size_t edge = dijkstraData[current].prevEdge[from];
        assert(edge != SIZE_MAX);
        current = dijkstraData[current].prev[from];
        addEdge(nodes, edges_ch, edges_ch[edge], p);
        p.push_back(nodes[current].position);
    }
}

PathData Graph::getPathCHDijkstra(size_t fromIndex, size_t toIndex) {
    PathData ret{.heap_accesses = 0, .length = 0};

    std::vector<NodeCHDijkstraData> dijkstraData(nodes_ch.size(), {
            .distanceHere = {SIZE_MAX, SIZE_MAX},
            .prev = {SIZE_MAX, SIZE_MAX},
            .prevEdge = {SIZE_MAX, SIZE_MAX},
            });
    dijkstraData[fromIndex].prev[DISCOVERED_FROM] = fromIndex;
    dijkstraData[toIndex].prev[DISCOVERED_TO] = toIndex;
    std::vector<HeapElemCHDijkstra> dijkstraHeap;
    std::make_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
    dijkstraHeap.push_back({.distanceHere = 0, .prev = fromIndex, .prevEdge = SIZE_MAX, .node = fromIndex, .from = DISCOVERED_FROM});
    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
    dijkstraHeap.push_back({.distanceHere = 0, .prev = toIndex, .prevEdge = SIZE_MAX, .node = toIndex, .from = DISCOVERED_TO});
    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
    size_t currentBest = SIZE_MAX;
    size_t currentBestIndex = SIZE_MAX;

    while(dijkstraHeap.size()) {
        ret.heap_accesses++;
        std::pop_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
        HeapElemCHDijkstra current = dijkstraHeap.back();
        dijkstraHeap.pop_back();


        NodeCHDijkstraData& node = dijkstraData[current.node];
        if(current.distanceHere < node.distanceHere[current.from]) {
            node.distanceHere[current.from] = current.distanceHere;
            node.prev[current.from] = current.prev;
            node.prevEdge[current.from] = current.prevEdge;
            if(node.prev[0] != SIZE_MAX && node.prev[1] != SIZE_MAX) {
                const size_t sum = node.distanceHere[0] + node.distanceHere[1];
                if(sum < currentBest) {
                    currentBest = sum;
                    currentBestIndex = current.node;
                }
                if(currentBest < current.sortValue) { // This breaking condition is inspired from https://i11www.iti.kit.edu/_media/teaching/theses/da-columbus-12.pdf Odly enough I was required to flip the comparison to get a correct lenght. But it seems to terminate in an awfull time (No time to fix it)
                    break;
                }
            }
            size_t currentToNode = fromIndex;
            if(DISCOVERED_FROM == current.from) {
                currentToNode = toIndex;
            }

            for(long i = nodes_ch[current.node + 1].edge_offset - 1; i >= nodes_ch[current.node].edge_offset; i--) {
                const EdgeCH& edge = edges_ch[i];
                size_t dist = current.distanceHere + edge.length;
                const NodeCHDijkstraData& otherNode = dijkstraData[edge.dest];

                bool correctDirection = nodes_ch[current.node].priority < nodes_ch[edge.dest].priority;
                if(!correctDirection) break; // Edges are sorted in first block of lower priority then higher
                if(correctDirection && dist < otherNode.distanceHere[current.from]) {
                    dijkstraHeap.push_back({
                            .distanceHere = dist,
                            .sortValue = dist + distance(nodes[edge.dest], nodes[currentToNode]),
                            .prev = current.node,
                            .prevEdge = i,
                            .node = edge.dest,
                            .from = current.from});
                    std::push_heap(dijkstraHeap.begin(), dijkstraHeap.end(), &compareCHDijkstra);
                }
            }
        }
    }

    if(currentBestIndex != SIZE_MAX) {
        ret.length = currentBest;
        ClipperLib::Path p1, p2;
        addPath(nodes, edges_ch, dijkstraData, currentBestIndex, p1, fromIndex, 0);
        if(SIZE_MAX != dijkstraData[currentBestIndex].prevEdge[1]) {
            addEdge(nodes, edges_ch, edges_ch[dijkstraData[currentBestIndex].prevEdge[1]], p2);
            addPath(nodes, edges_ch, dijkstraData, dijkstraData[currentBestIndex].prev[1], p2, toIndex, 1);
        }
        std::reverse(p1.begin(), p1.end());
        p1.reserve(p1.size() + p2.size());
        for(const auto& i : p2) {
            p1.push_back(i);
        }
        ret.path = p1;
    }
    return ret;
}
