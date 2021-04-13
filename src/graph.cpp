#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
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

ClipperLib::IntPoint getRandomPoint() {
    float x = rand() / (RAND_MAX * 1.);
    x *= AREABOUND_EAST - AREABOUND_WEST;
    x += AREABOUND_WEST;
    float y = rand() / (RAND_MAX * 1.);
    y *= AREABOUND_NORTH - AREABOUND_SOUTH;
    y += AREABOUND_SOUTH;
    return {toInt(x), toInt(y)};
}

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
    pointsInX = std::sqrt(N);
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
        size_t from;
        size_t destination;
        size_t hop_node; //find edge in tmpFinalEdges
        size_t edge_index1, edge_index2;
    };
    std::vector<std::vector<TmpEdge>> tmpRemainingEdges(N);
    std::vector<std::vector<TmpEdge>> tmpFinalEdges(N);
    for(size_t i = 0; i < N; i++) {
        nodes_ch[i].position = nodes[i].position;
        const size_t edges_start = nodes[i].edge_offset;
        const size_t edges_stop = nodes[i + 1].edge_offset;
        const size_t edge_count = edges_stop - edges_start;
        if (edge_count) {
            auto& curEdges = tmpRemainingEdges[i];
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
    std::vector<size_t> neighbourMap(N, SIZE_MAX);

    size_t remainingNodes = N;
    size_t currentPriority = 1;  // priority 0 marks nodes not yet removed
    for(size_t i = 0; i < nodes_ch.size() - 1; i++) {
        if(tmpRemainingEdges[i].size() + tmpFinalEdges[i].size() < 2) { // No point in expanding nodes with only 1 connected node
            remainingNodes--;
            nodes_ch[i].priority = currentPriority;
            if(tmpRemainingEdges[i].size() > 0) {
                assert(1 == tmpRemainingEdges[i].size());
                auto& ownRemainingEdges = tmpRemainingEdges[i];
                auto& ownFinalEdges = tmpFinalEdges[i];
                ownFinalEdges.push_back(ownRemainingEdges.front());
                ownRemainingEdges.clear();
                const auto& edge = ownFinalEdges.front();
                auto& neighbourRemainingEdges = tmpRemainingEdges[edge.destination];
                auto& neighbourFinalEdges = tmpFinalEdges[edge.destination];
                for(size_t j = 0; j < neighbourRemainingEdges.size(); j++) {
                    const auto& otherEdge = neighbourRemainingEdges[j];
                    if(otherEdge.destination == i) {
                        neighbourFinalEdges.push_back(otherEdge);
                        neighbourRemainingEdges.erase(neighbourRemainingEdges.begin() + j);
                        j--;
                    }
                }
            }
        } else if(!nodes[i].onWater){
            // This property allows us to just mark them as priority 1 and don't think about them later on
            throw std::runtime_error("All None water nodes should have no edges");
        }
    }
    std::vector<size_t> edges_to_remove; // vector holding indexes of edges to remove from the current neighbour
    std::vector<TmpEdge> edges_to_add;
    while(remainingNodes) {
        std::vector<bool> visitedIndependece(N, false);
        currentPriority++;
        std::vector<EDStruct> indexesForED(remainingNodes);
        size_t curIndexInEDVector = 0;
        for(size_t i = 0; i < N; i++) {
            if(nodes_ch[i].priority == 0) {
                long edge_count = 0;
                for(size_t j = 0; j < tmpRemainingEdges[i].size(); j++) {
                    assert(0 == nodes_ch[tmpRemainingEdges[i][j].destination].priority);
                    edge_count++;
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
        for(size_t i = 0; i <= remainingNodes / 2; i++) {
            const size_t index = indexesForED[i].index;
            if(!visitedIndependece[index]) {
                // Node is marked by not marking at as true
                currentSetSize++;
                for(auto& e : tmpRemainingEdges[index]) {
                    visitedIndependece[e.destination] = true;
                    // No need to check if the destination is part of the graph as this will be done later anyway
                }
            }
        }
        // Only build independent set in lower half of ED nodes to avoid working on nodes with a huge amount of neighbours for as long as possible
        for(size_t i = 1 + remainingNodes / 2; i < remainingNodes; i++) {
            const size_t index = indexesForED[i].index;
            visitedIndependece[index] = true;
        }
        for(size_t i = 0; i < N; i++) {
            auto& curNode = nodes_ch[i];
            assert(SIZE_MAX == neighbourMap[i]);
            if(!visitedIndependece[i] && curNode.priority == 0) {
                auto& currentFinalEdges = tmpFinalEdges[i];
                auto& currentRemainingEdges = tmpRemainingEdges[i];
                const size_t edgeCountPreviouslyInFinal = currentFinalEdges.size();
                currentFinalEdges.insert(currentFinalEdges.end(), currentRemainingEdges.begin(), currentRemainingEdges.end());
                edges_to_add.clear();
                ONLY_DEBUG(
                    size_t neighbourCount = 0;
                    for(size_t j = 0; j < tmpRemainingEdges[i].size(); j++) {
                        assert(0 == nodes_ch[tmpRemainingEdges[i][j].destination].priority);
                        neighbourCount++;
                    }
                    assert(neighbourCount == currentRemainingEdges.size());
                    for(size_t j = 0; j < currentRemainingEdges.size(); j++) {
                        size_t other = currentRemainingEdges[j].destination;
                        bool found = false;
                        for(size_t k = 0; k < j; k++) {
                            if(currentRemainingEdges[k].destination == other) {
                                found = true;
                                break;
                            }
                        }
                        assert(!found);
                    }
                )
                // TODO maybe do an qsort on the neighbours based of the index for faster finding later
                ONLY_DEBUG(
                    size_t totalNumberOfEdgesAdded =0;
                    struct DebugEdge {
                        size_t from;
                        size_t to;
                    };
                    std::vector<DebugEdge> edges_between_neighbours;
                )
                for(size_t j = 0; j < currentRemainingEdges.size(); j++) {
                    neighbourMap[currentRemainingEdges[j].destination] = j;
                }
                for(size_t j = 0; j < currentRemainingEdges.size(); j++) {
                    edges_to_remove.clear();
                    size_t currentNeighbour = currentRemainingEdges[j].destination;
                    struct {
                        size_t currentLength = SIZE_MAX;
                        size_t edge_here = SIZE_MAX; // Stores the edge to get directly from j to neighbour k
                        bool overThisNode = false;
                        size_t e1_i = SIZE_MAX;
                    } dijkstraData[currentRemainingEdges.size()];
                    assert(dijkstraData[0].currentLength == SIZE_MAX);
                    for(size_t e1_i = 0; e1_i < tmpRemainingEdges[currentNeighbour].size(); e1_i++) {
                        const auto& edge1 = tmpRemainingEdges[currentNeighbour][e1_i];
                        if(edge1.hop_node == i) continue; // We can't use shortcut edges that we added
                        const bool overThisNode = (edge1.destination == i);
                        if(overThisNode) {
                            edges_to_remove.push_back(e1_i);
                            for(size_t n = 0; n < currentRemainingEdges.size(); n++) {
                                const auto& edge2 = currentRemainingEdges[n];
                                if(edge2.hop_node == i) continue; // We can't use shortcut edges that we added
                                const size_t length = edge1.length + edge2.length;
                                if(length < dijkstraData[n].currentLength) {
                                    dijkstraData[n].currentLength = length;
                                    dijkstraData[n].overThisNode = true;
                                    dijkstraData[n].e1_i = edges_to_remove.size();
                                }
                            }
                        } else if(neighbourMap[edge1.destination] < SIZE_MAX) {
                            const size_t n = neighbourMap[edge1.destination];
                            dijkstraData[n].edge_here = e1_i - edges_to_remove.size(); // We need to take edges that are deleted into account
                            if(edge1.length <= dijkstraData[n].currentLength) {
                                dijkstraData[n].currentLength = edge1.length;
                                dijkstraData[n].overThisNode = false;
                                dijkstraData[n].e1_i = SIZE_MAX;
                            }
                        }
                    }
                    const size_t from = currentRemainingEdges[j].destination;
                    auto& finalEdgesOfNeighbour = tmpFinalEdges[from];
                    auto& remainingEdgesOfNeighbour = tmpRemainingEdges[from];
                    for(size_t k = 0; k < currentRemainingEdges.size(); k++) {
                        if(k != j && dijkstraData[k].overThisNode) {
                            const size_t to = currentRemainingEdges[k].destination;
                            assert(dijkstraData[k].e1_i <= edges_to_remove.size());
                            assert(from != to);
                            assert(to != i);
                            assert(from != i);
                            // Only required to add the one edge as the other direction is done from the other neighbour
                            TmpEdge newEdge = {
                                .length = dijkstraData[k].currentLength,
                                .from = from,
                                .destination = to,
                                .hop_node = dijkstraData[k].edge_here, // misuse hop_node for now to store the edge to replace by adding this edge
                                .edge_index1 = finalEdgesOfNeighbour.size() + edges_to_remove.size() - dijkstraData[k].e1_i,
                                .edge_index2 = k + edgeCountPreviouslyInFinal
                            };
                            edges_to_add.push_back(newEdge);
                            ONLY_DEBUG(edges_between_neighbours.push_back({from, to});)
                            ONLY_DEBUG(totalNumberOfEdgesAdded++;)
                            //finalEdgesOfNeighbour.push_back(remainingEdgesOfNeighbour[dijkstraData[k].e1_i]);
                            assert(newEdge.edge_index1 < finalEdgesOfNeighbour.size() + edges_to_remove.size());
                            assert(newEdge.edge_index2 < currentFinalEdges.size());
                        }
                    }
                    for(long k = edges_to_remove.size() - 1; k >= 0; k--) {
                        assert(edges_to_remove[k] < remainingEdgesOfNeighbour.size());
                        assert(i == remainingEdgesOfNeighbour[edges_to_remove[k]].destination);
                        assert(i != remainingEdgesOfNeighbour[edges_to_remove[k]].hop_node);
                        finalEdgesOfNeighbour.push_back(remainingEdgesOfNeighbour[edges_to_remove[k]]);
                        remainingEdgesOfNeighbour.erase(remainingEdgesOfNeighbour.begin() + edges_to_remove[k]);
                    }
                    ONLY_DEBUG(
                        for(long k = remainingEdgesOfNeighbour.size() - 1; k >= 0; k--) {
                            if(i == remainingEdgesOfNeighbour[k].destination) {
                                remainingEdgesOfNeighbour.erase(remainingEdgesOfNeighbour.begin() + k);
                                k--;
                                assert(0);
                            }
                        }
                    )
                }
                for(size_t j = 0; j < currentRemainingEdges.size(); j++) {
                    neighbourMap[currentRemainingEdges[j].destination] = SIZE_MAX;
                }
                for(auto& newEdge : edges_to_add) {
                    auto& remainingEdgesOfNeighbour = tmpRemainingEdges[newEdge.from];
                    if(newEdge.hop_node < SIZE_MAX) {
                        assert(newEdge.destination == remainingEdgesOfNeighbour[newEdge.hop_node].destination);
                        remainingEdgesOfNeighbour[newEdge.hop_node] = newEdge;
                        remainingEdgesOfNeighbour[newEdge.hop_node].hop_node = i;
                    } else {
                        newEdge.hop_node = i;
                        remainingEdgesOfNeighbour.push_back(newEdge);
                    }
                }
                ONLY_DEBUG(
                    assert(totalNumberOfEdgesAdded == edges_between_neighbours.size());
                    assert(totalNumberOfEdgesAdded == edges_to_add.size());
                    for(auto& e1 : edges_between_neighbours) {
                        bool found = false;
                        for(auto& e2 : edges_between_neighbours) {
                            if(e1.from == e2.to && e1.to == e2.from) {
                                found = true;
                                break;
                            }
                        }
                        assert(found);
                    }
                    if(totalNumberOfEdgesAdded % 2) {
                        assert(0);
                    }
                )
                // Remove node from current Graph
                curNode.priority = currentPriority;
                remainingNodes--;
                std::cerr << remainingNodes << "                   " << currentRemainingEdges.size() << "                   \r";
                currentRemainingEdges.clear();
            }
        }
    }
    std::cerr << "\nDone contraction: " << std::endl;
    size_t edgeCount = 0;
    for(const auto& edges : tmpFinalEdges) {
        edgeCount += edges.size();
    }
    for(const auto& edges : tmpRemainingEdges) {
        assert(0 == edges.size());
    }

    edges_ch.reserve(edgeCount);
    for(size_t i = 0; i < N; i++) {
        nodes_ch[i].edge_offset = edges_ch.size();
        for(const auto& e : tmpFinalEdges[i]) {
            EdgeCH newEdge = {
                .dest = e.destination,
                .length = e.length,
                .p1 = SIZE_MAX,
                .p2 = SIZE_MAX,
            };
            assert(newEdge.length != 0);
            assert(newEdge.length != SIZE_MAX);
            assert(i != e.destination);
            edges_ch.push_back(newEdge);
        }
    }
    for(size_t i = 0; i < N; i++) {
        for(size_t j = 0; j < tmpFinalEdges[i].size(); j++) {
            const auto& tmpE = tmpFinalEdges[i][j];
            auto& e = edges_ch[j + nodes_ch[i].edge_offset];
            if(SIZE_MAX != tmpE.edge_index1) {
                e.p1 = tmpE.edge_index1 + nodes_ch[i].edge_offset;
                e.p2 = tmpE.edge_index2 + nodes_ch[tmpE.hop_node].edge_offset;
                assert(SIZE_MAX != e.p1);
                assert(SIZE_MAX != e.p2);
            }
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
    nodes_ch.resize(nodes_count);
    input.read((char *) nodes_ch.data(), LENGTH(NodeCH, nodes_count));

    input.read((char *) &pointsInX, LENGTH(pointsInX, 1));
    input.read((char *) &pointsInY, LENGTH(pointsInY, 1));

    size_t edges_count = 0;
    input.read((char *) &edges_count, LENGTH(edges_count, 1));
    edges.resize(edges_count);
    input.read((char *) edges.data(), LENGTH(Edge, edges_count));

    size_t edges_ch_count = 0;
    input.read((char *) &edges_ch_count, LENGTH(edges_ch_count, 1));
    edges_ch.resize(edges_ch_count);
    input.read((char *) edges_ch.data(), LENGTH(EdgeCH, edges_ch_count));
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
    out.write((char *) nodes_ch.data(), LENGTH(NodeCH, nodes_count));

    out.write((char *) &pointsInX, LENGTH(pointsInX, 1));
    out.write((char *) &pointsInY, LENGTH(pointsInY, 1));

    size_t edges_count = edges.size();
    out.write((char *) &edges_count, LENGTH(edges_count, 1));
    out.write((char *) edges.data(), LENGTH(Edge, edges_count));

    size_t edges_ch_count = edges_ch.size();
    out.write((char *) &edges_ch_count, LENGTH(edges_ch_count, 1));
    out.write((char *) edges_ch.data(), LENGTH(EdgeCH, edges_ch_count));
}

void Graph::output_geojson(const char * filename) {
    std::ofstream out(filename, std::ios_base::binary);
    output_geojsonCH(out);
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

void Graph::output_geojsonCH(std::ostream& out) {
    geojson_output::outputPathsStart(out);
    bool first = true;
    for(size_t i = 0; i < nodes.size() - 1; i++) {
        if(!first) out << ",";
        first = false;
        geojson_output::outputFeaturePoint(out, nodes_ch[i].position);
        for(size_t j = nodes_ch[i].edge_offset; j < nodes_ch[i+1].edge_offset; j++) {
            out << ",";
            geojson_output::outputLine(out, nodes_ch[i].position, nodes_ch[edges_ch[j].dest].position);
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

PathData findPath(Graph* graph, float x1, float y1, float x2, float y2, int algorithm) {
    ClipperLib::IntPoint a_i{toInt(x1), toInt(y1)};
    ClipperLib::IntPoint b_i{toInt(x2), toInt(y2)};
    const size_t a = graph->getNearestNode(a_i);
    const size_t b = graph->getNearestNode(b_i);
    PathData p;
    auto start = std::chrono::high_resolution_clock::now();
    switch (algorithm) {
        case ALGORITHM_DIJKSTRA:
            p = graph->getPathDijkstra(a, b);
            break;
        case ALGORITHM_A_STAR:
            p = graph->getPathAStar(a, b);
            break;
        case ALGORITHM_CH_DIJSKTRA:
            p = graph->getPathCHDijkstra(a, b);
            break;
        default:
            throw std::runtime_error("Unknown Algorithm");
    }
    auto stop = std::chrono::high_resolution_clock::now();
    p.duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    p.distance = calculate_distance(a, b);
    return p;
}
