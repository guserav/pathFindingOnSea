#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include "helper.hpp"
#include "output_geojson.hpp"

#define AREABOUND_WEST -180.
#define AREABOUND_EAST 180.
#define AREABOUND_NORTH 85.
#define AREABOUND_SOUTH -85.
const ClipperLib::IntRect world_boundary = {
    .left = toInt(AREABOUND_WEST),
    .top = toInt(AREABOUND_NORTH),
    .right = toInt(AREABOUND_EAST),
    .bottom = toInt(AREABOUND_SOUTH)
};

/*
 * Scale N to a square to determine how many points to actually place
 */
float pointsForSquare(size_t N) {
    return N * (AREABOUND_EAST - AREABOUND_WEST) / (AREABOUND_NORTH - AREABOUND_WEST);
}

Graph::Graph(OutlineHolder& outline_holder, size_t N) {
    generateGraph(outline_holder, N);
    generateCH();
}

void Graph::generateGraph(OutlineHolder& outline_holder, size_t N) {
    pointsInX = std::sqrt(pointsForSquare(N));
    pointsInY = N / pointsInX;
    N = pointsInX * pointsInY;
    float distanceX = (AREABOUND_EAST - AREABOUND_WEST) / pointsInX;
    float distanceY = (AREABOUND_NORTH - AREABOUND_SOUTH) / pointsInY;

    std::cerr << "DistanceX: " << distanceX << std::endl;
    std::cerr << "DistanceY: " << distanceY << std::endl;
    std::cerr << "pointsInX: " << pointsInX << std::endl;
    std::cerr << "pointsInY: " << pointsInY << std::endl;

    nodes.resize(N + 1);
    std::cerr << "Done calculating Outlines" << std::endl;
    for(size_t x = 0; x < pointsInX; x++) {
        const float curX = AREABOUND_WEST + x * distanceX + distanceX / 2;
        for(size_t y = 0; y < pointsInY; y++) {
            const float curY = AREABOUND_SOUTH + y * distanceY + distanceY / 2;
            const size_t index = getIndex(x, y);
            std::cerr << "\r" << index;
            Node& node = nodes.at(index);
            node.position.X = toInt(curX);
            node.position.Y = toInt(curY);
            node.onWater = outline_holder.isPointInWater(node.position);
        }
    }
    std::cerr << "\nDone generating Points" << std::endl;

    for(long long x = 0; x < pointsInX; x++) {
        for(long long y = 0; y < pointsInY; y++) {
            const size_t index = getIndex(x, y);
            std::cerr << "\r" << index;
            Node& node = nodes.at(index);
            node.edge_offset = edges.size();
            if(node.onWater) {
                for(long long i = -1; i <= 1; i++) {
                    for(long long j = -1; j <= 1; j++) {
                        if(i|j) {
                            addEdgeIfNodeExists(x + i, y + j, node);
                        }
                    }
                }
            }
        }
    }
    std::cerr << "\nDone generating Edges: " << edges.size() << std::endl;
    nodes.back().edge_offset = edges.size();
    nodes.back().onWater = false;
}

struct EDStruct {
    size_t index;
    long value;
};

int compareEDStruct(const void * a, const void * b) {
    return ((EDStruct * )a)->value - ((EDStruct * )b)->value;
}

void Graph::generateCH() {
    // node_map.resize(nodes.size(), SIZE_MAX); // Maps from normal node index to CH node index
    const size_t N = nodes.size() - 1;
    nodes_ch.resize(N + 1, {.position = {0,0}, .edge_offset = SIZE_MAX, .priority = 0});
    struct TmpEdge {
        size_t length;
        size_t destination;
        size_t hop_node; //find edge in tmpEdges
        size_t edge_index1, edge_index2;
    };
    std::vector<std::vector<TmpEdge>> tmpEdges(N);
    for(size_t i = 0; i < N; i++) {
        nodes_ch[i].position = nodes[i].position;
        const size_t edges_start = nodes[i].edge_offset;
        const size_t edges_stop = nodes[i + 1].edge_offset;
        const size_t edge_count = edges_stop - edges_start;
        if (edge_count) {
            auto& curEdges = tmpEdges[i];
            curEdges.resize(edge_count);
            for(int j = 0; j < edge_count; j++) {
                Edge& origEdge = edges[edges_start + j];
                curEdges[j] = {
                    .length = origEdge.length,
                    .destination = origEdge.dest,
                    .hop_node = SIZE_MAX,
                    .edge_index1 = SIZE_MAX,
                    .edge_index2 = SIZE_MAX
                };
            }
        }
    }

    size_t remainingNodes = N;
    size_t currentPriority = 1;  // priority 0 marks nodes not yet removed
    for(size_t i = 0; i < nodes_ch.size() - 1; i++) {
        if(tmpEdges[i].size() < 2) { // No point in expanding nodes with only 1 connected node
            remainingNodes--;
            nodes_ch[i].priority = currentPriority;
        } else if(!nodes[i].onWater){
            // This property allows us to just mark them as priority 1 and don't think about them later on
            throw std::runtime_error("All None water nodes should have no edges");
        }
    }
    while(remainingNodes) {
        std::vector<bool> visitedIndependece(N, false);
        currentPriority++;
        std::vector<EDStruct> indexesForED(remainingNodes);
        size_t curIndexInEDVector = 0;
        for(size_t i = 0; i < N; i++) {
            if(nodes_ch[i].priority == 0) {
                long edge_count = 0;
                for(size_t j = 0; j < tmpEdges[i].size(); j++) {
                    if(0 == nodes_ch[tmpEdges[i][j].destination].priority) {
                        edge_count++;
                    }
                }
                indexesForED[curIndexInEDVector++] = {
                    .index = i,
                    .value = (edge_count * (edge_count - 1)) / 2 - edge_count,
                };
            }
        }

        assert(curIndexInEDVector == remainingNodes);
        qsort(indexesForED.data(), remainingNodes, sizeof(indexesForED[0]), compareEDStruct);
        size_t currentSetSize = 0;
        for(size_t i = 0; i < remainingNodes; i++) {
            const size_t index = indexesForED[i].index;
            if(!visitedIndependece[index]) {
                // Node is marked by not marking at as true
                currentSetSize++;
                for(auto& e : tmpEdges[index]) {
                    visitedIndependece[e.destination] = true;
                    // No need to check if the destination is part of the graph as this will be done later anyway
                }
            }
        }
        for(size_t i = 0; i < N; i++) {
            auto& curNode = nodes_ch[i];
            if(!visitedIndependece[i] && curNode.priority == 0) {
                size_t neighbourCount = 0;
                for(size_t j = 0; j < tmpEdges[i].size(); j++) {
                    if(0 == nodes_ch[tmpEdges[i][j].destination].priority) {
                        neighbourCount++;
                    }
                }
                struct {
                    size_t index;
                } neighbours[neighbourCount];
                size_t cNeighbour = 0;
                for(size_t j = 0; j < tmpEdges[i].size(); j++) {
                    size_t other = tmpEdges[i][j].destination;
                    if(0 == nodes_ch[other].priority) {
                        bool found = false;
                        for(size_t k = 0; k < cNeighbour; k++) {
                            if(neighbours[k].index == other) {
                                found = true;
                                break;
                            }
                        }
                        if(!found) {
                            neighbours[cNeighbour++] = {.index = other};
                        } else {
                            neighbourCount--;
                        }
                    }
                }
                assert(neighbourCount == cNeighbour);
                // TODO maybe do an qsort on the neighbours based of the index for faster finding later
                for(size_t j = 0; j < neighbourCount; j++) {
                    size_t currentNeighbour = neighbours[j].index;
                    struct {
                        size_t currentLength = SIZE_MAX;
                        bool alternative = false;
                        bool overThisNode = false;
                        size_t e1_i = SIZE_MAX;
                        size_t e2_i = SIZE_MAX;
                    } dijkstraData[neighbourCount];
                    assert(dijkstraData[0].currentLength == SIZE_MAX);
                    for(size_t e1_i = 0; e1_i < tmpEdges[currentNeighbour].size(); e1_i++) {
                        const auto& edge1 = tmpEdges[currentNeighbour][e1_i];
                        const bool overThisNode = (edge1.destination == i);
                        if(0 == nodes_ch[edge1.destination].priority) {
                            for(size_t e2_i = 0; e2_i < tmpEdges[edge1.destination].size(); e2_i++) {
                                const auto& edge2 = tmpEdges[edge1.destination][e2_i];
                                if(0 == nodes_ch[edge2.destination].priority) {
                                    const size_t length = edge1.length + edge2.length;
                                    size_t n = 0;
                                    for(; n < neighbourCount; n++) {
                                        if(neighbours[n].index == edge2.destination) {
                                            break;
                                        }
                                    }
                                    if(n != neighbourCount) {
                                        if(length < dijkstraData[n].currentLength) {
                                            dijkstraData[n].currentLength = length;
                                            dijkstraData[n].alternative = false;
                                            dijkstraData[n].overThisNode = overThisNode;
                                            if(overThisNode) {
                                                dijkstraData[n].e1_i = e1_i;
                                                dijkstraData[n].e2_i = e2_i;
                                            }
                                        } else if(dijkstraData[n].currentLength == length) {
                                            dijkstraData[n].alternative = true;
                                            if(overThisNode) {
                                                dijkstraData[n].overThisNode = true;
                                                dijkstraData[n].e1_i = e1_i;
                                                dijkstraData[n].e2_i = e2_i;
                                            }
                                        }
                                    }
                                }
                            }
                            size_t n = 0;
                            for(; n < neighbourCount; n++) {
                                if(neighbours[n].index == edge1.destination) {
                                    break;
                                }
                            }
                            if(n != neighbourCount) {
                                if(edge1.length <= dijkstraData[n].currentLength) {
                                    dijkstraData[n].currentLength = edge1.length;
                                    dijkstraData[n].alternative = false; // Technically not correct for == case but functionally the same
                                    dijkstraData[n].overThisNode = false;
                                    dijkstraData[n].e1_i = SIZE_MAX;
                                    dijkstraData[n].e2_i = SIZE_MAX;
                                }
                            }
                        }
                    }
                    for(size_t k = 0; k < neighbourCount; k++) {
                        if(k != j && dijkstraData[k].overThisNode && !dijkstraData[k].alternative) {
                            const size_t from = neighbours[j].index;
                            const size_t to = neighbours[k].index;
                            auto& edgesOfNeighbour = tmpEdges[from];
                            assert(dijkstraData[k].e1_i < edgesOfNeighbour.size());
                            assert(dijkstraData[k].e2_i < tmpEdges[i].size());
                            assert(from != to);
                            // Only required to add the one edge as the other direction is done from the other neighbour
                            TmpEdge newEdge = {
                                .length = dijkstraData[k].currentLength,
                                .destination = to,
                                .hop_node = i,
                                .edge_index1 = dijkstraData[k].e1_i,
                                .edge_index2 = dijkstraData[k].e2_i
                            };
                            edgesOfNeighbour.push_back(newEdge);
                        }
                    }
                }
                // Remove node from current Graph
                curNode.priority = currentPriority;
                remainingNodes--;
            }
        }
    }
    std::cerr << "\nDone contraction: " << std::endl;
    size_t edgeCount = 0;
    for(const auto& edges : tmpEdges) {
        edgeCount += edges.size();
    }

    edges_ch.reserve(edgeCount);
    for(size_t i = 0; i < N; i++) {
        nodes_ch[i].edge_offset = edges_ch.size();
        for(const auto& e : tmpEdges[i]) {
            EdgeCH newEdge = {
                .dest = e.destination,
                .length = e.length,
            };
            edges_ch.push_back(newEdge);
        }
    }
    for(size_t i = 0; i < N; i++) {
        for(size_t j = 0; j < tmpEdges[i].size(); j++) {
            const auto& tmpE = tmpEdges[i][j];
            auto& e = edges_ch[j + nodes_ch[i].edge_offset];
            e.p1 = tmpE.edge_index1 + nodes_ch[i].edge_offset;
            e.p2 = tmpE.edge_index2 + nodes_ch[tmpE.hop_node].edge_offset;
        }
    }

    std::cerr << "\nDone building adjacency vector: " << edges_ch.size() << std::endl;
    nodes_ch.back().edge_offset = edges_ch.size();
    nodes_ch.back().priority = 0;
}

Graph::Graph(const char * filename) {
    std::ifstream input(filename, std::ios_base::binary);
    size_t nodes_count = nodes.size();
    input.read((char *) &nodes_count, LENGTH(nodes_count, 1));
    nodes.resize(nodes_count);
    input.read((char *) nodes.data(), LENGTH(Node, nodes_count));

    input.read((char *) &pointsInX, LENGTH(pointsInX, 1));
    input.read((char *) &pointsInY, LENGTH(pointsInY, 1));

    size_t edges_count = edges.size();
    input.read((char *) &edges_count, LENGTH(edges_count, 1));
    edges.resize(edges_count);
    input.read((char *) edges.data(), LENGTH(Edge, edges_count));
}

void Graph::addEdgeIfNodeExists(long long x, long long y, const Node& node) {
    if(x < 0) x = pointsInX - 1;
    if(y < 0) return;
    if(x >= pointsInX) x = 0;
    if(y >= pointsInY) return;
    size_t otherIndex = getIndex(x, y);
    const Node& otherNode = nodes.at(otherIndex);
    if(!otherNode.onWater) return;
    size_t length = distance(node, otherNode);
    edges.push_back({.dest = otherIndex, .length = length});
}

size_t Graph::getIndex(size_t x, size_t y) {
    return x * pointsInY + y;
}

size_t Graph::distance(const Node& a, const Node& b) {
    return calculate_distance(a.position, b.position);
}

void Graph::output(const char * filename) {
    std::ofstream out(filename, std::ios_base::binary);
    output(out);
}

void Graph::output(std::ostream& out) {
    size_t nodes_count = nodes.size();
    out.write((char *) &nodes_count, LENGTH(nodes_count, 1));
    out.write((char *) nodes.data(), LENGTH(Node, nodes_count));

    out.write((char *) &pointsInX, LENGTH(pointsInX, 1));
    out.write((char *) &pointsInY, LENGTH(pointsInY, 1));

    size_t edges_count = edges.size();
    out.write((char *) &edges_count, LENGTH(edges_count, 1));
    out.write((char *) edges.data(), LENGTH(Edge, edges_count));
}

void Graph::output_geojson(const char * filename) {
    std::ofstream out(filename, std::ios_base::binary);
    output_geojson(out);
}


void Graph::output_geojson(std::ostream& out) {
    geojson_output::outputPathsStart(out);
    bool first = true;
    for(size_t i = 0; i < nodes.size(); i++) {
        if(nodes[i].onWater) {
            if(!first) out << ",";
            first = false;
            geojson_output::outputFeaturePoint(out, nodes[i].position);
            for(size_t j = nodes[i].edge_offset; j < nodes[i+1].edge_offset; j++) {
                out << ",";
                geojson_output::outputLine(out, nodes[i].position, nodes[edges[j].dest].position);
            }
        }
    }
    geojson_output::outputPathsEnd(out);
}

size_t Graph::getNearestNode(const ClipperLib::IntPoint& x) {
    size_t currentBest = 0;
    size_t currentBestDistance = SIZE_MAX;
    for(size_t i = 0; i < nodes.size(); i++) {
        const auto& n = nodes[i];
        if(n.onWater) {
            size_t dist = calculate_distance(x, n.position);
            if(dist < currentBestDistance) {
                currentBest = i;
                currentBestDistance = dist;
            }
        }
    }
    return currentBest;
}
