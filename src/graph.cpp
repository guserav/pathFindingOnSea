#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include "helper.hpp"
#include "output_geojson.hpp"
#include "outline_holder.hpp"

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

Graph::Graph(std::list<ClipperLib::Path>& polygons, size_t N) {
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
    //OutlineHolderSimple outline_holder(polygons);
    TreeOutlineHolder outline_holder(polygons);
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
