#include "graph.hpp"
#include <cmath>
#include <iostream>
#include "helper.hpp"
#include "output_geojson.hpp"

#define AREABOUND_WEST -180.
#define AREABOUND_EAST 180.
#define AREABOUND_NORTH 85.
#define AREABOUND_SOUTH -85.

/*
 * Scale N to a square to determine how many points to actually place
 */
float pointsForSquare(size_t N) {
    return N * (AREABOUND_EAST - AREABOUND_WEST) / (AREABOUND_NORTH - AREABOUND_WEST);
}

bool isPointInRectangle(const ClipperLib::IntRect& r, const ClipperLib::IntPoint& p) {
    if(p.X < r.left) return false;
    if(p.X > r.right) return false;
    if(p.Y < r.bottom) return false;
    if(p.Y > r.top) return false;
    return true;
}

bool isPointInWater(const std::list<ClipperLib::Path>& polygons, const std::vector<ClipperLib::IntRect>& outlines, const ClipperLib::IntPoint& p) {
    size_t i = 0;
    for(auto it = polygons.begin(); it != polygons.end(); it++) {
        if (isPointInRectangle(outlines[i], p)) {
            if (ClipperLib::PointInPolygon(p, *it)) {
                return false;
            }
        }
        i++;
    }
    return true;
}

void calculateOutlines(const ClipperLib::Path& path, ClipperLib::IntRect& rect) {
    rect.left = path.front().X;
    rect.right = path.front().X;
    rect.bottom = path.front().Y;
    rect.top = path.front().Y;

    for(const auto& point : path) {
        if(point.X < rect.left) {
            rect.left = point.X;
        } else if(point.X > rect.right){
            rect.right = point.X;
        }
        if(point.Y < rect.bottom) {
            rect.bottom = point.Y;
        } else if(point.Y > rect.top){
            rect.top = point.Y;
        }
    }
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
    std::vector<ClipperLib::IntRect> boundingBoxes(polygons.size());
    size_t i = 0;
    for(auto it = polygons.begin(); it != polygons.end(); it++) {
        calculateOutlines(*it, boundingBoxes[i]);
        i++;
    }
    std::cerr << "Done calculating Outlines" << std::endl;
    for(size_t x = 0; x < pointsInX; x++) {
        float curX = AREABOUND_WEST + x * distanceX + distanceX / 2;
        for(size_t y = 0; y < pointsInY; y++) {
            float curY = AREABOUND_SOUTH + y * distanceY + distanceY / 2;
            size_t index = x * pointsInY + y;
            std::cerr << "\r" << index;
            Node& node = nodes.at(index);
            node.position.X = toInt(curX);
            node.position.Y = toInt(curY);
            node.onWater = isPointInWater(polygons, boundingBoxes, node.position);
        }
    }
    std::cerr << "\nDone generating Points" << std::endl;

    for(size_t x = 0; x < pointsInX; x++) {
        for(size_t y = 0; y < pointsInY; y++) {
            size_t index = x * pointsInY + y;
            std::cerr << "\r" << index;
            Node& node = nodes.at(index);
            node.edge_offset = edges.size();
            if(node.onWater) {
                for(int i = -1; i <= 1; i++) {
                    for(int j = -1; j <= 1; j++) {
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

void Graph::addEdgeIfNodeExists(size_t x, size_t y, const Node& node) {
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
    long long distX = a.position.X - b.position.X;
    long long distY = a.position.Y - b.position.Y;
    return std::sqrt(distX*distX + distY*distY);
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
