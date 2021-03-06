#include "graph.hpp"

struct NodeAStarData {
    size_t distanceHere;
    size_t prev;
};
using NodeAStarData = struct NodeAStarData;

struct HeapElemAStar {
    size_t distanceHere;
    size_t sortValue;
    size_t prev;
    size_t node;
};
using HeapElemAStar = struct HeapElemAStar;

bool compareAStar(const HeapElemAStar& a, const HeapElemAStar& b) {
    return a.sortValue > b.sortValue;
}

PathData Graph::getPathAStar(size_t fromIndex, size_t toIndex) {
    PathData ret{.heap_accesses = 0, .length = 0};
    Node toNode = nodes[toIndex];

    std::vector<NodeAStarData> astarData(nodes.size(), {.distanceHere = SIZE_MAX, .prev = 0});
    std::vector<HeapElemAStar> astarHeap;
    std::make_heap(astarHeap.begin(), astarHeap.end(), &compareAStar);
    astarHeap.push_back({.distanceHere = 0, .sortValue = 0, .prev = fromIndex, .node = fromIndex});
    std::push_heap(astarHeap.begin(), astarHeap.end(), &compareAStar);

    while(astarHeap.size()) {
        ret.heap_accesses++;
        std::pop_heap(astarHeap.begin(), astarHeap.end(), &compareAStar);
        HeapElemAStar current = astarHeap.back();
        astarHeap.pop_back();

        NodeAStarData& node = astarData[current.node];
        if(current.node == toIndex){
            node.prev = current.prev;
            node.distanceHere = current.distanceHere;
            ret.length = current.distanceHere;
            break;
        }
        if(current.distanceHere < node.distanceHere) {
            node.distanceHere = current.distanceHere;
            node.prev = current.prev;

            for(size_t i = nodes[current.node].edge_offset; i < nodes[current.node + 1].edge_offset; i++) {
                const Edge& edge = edges[i];
                size_t dist = current.distanceHere + edge.length;
                const NodeAStarData& otherNode = astarData[edge.dest];

                if(dist < otherNode.distanceHere) {
                    astarHeap.push_back({.distanceHere = dist, .sortValue = dist + distance(nodes[edge.dest], toNode), .prev = current.node, .node = edge.dest});
                    std::push_heap(astarHeap.begin(), astarHeap.end(), &compareAStar);
                }
            }
        }
    }

    size_t current = toIndex;
    if(astarData[toIndex].distanceHere != SIZE_MAX) {
        ret.path.emplace_back(nodes[current].position);
        while(current != fromIndex) {
            const NodeAStarData& c = astarData[current];
            current = c.prev;
            ret.path.emplace_back(nodes[current].position);
        }
    }
    // The path is reverse but we only want to display it so it doesn't matter
    return ret;
}
