#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include "stdlib.h"
#include "stdio.h"
#include "osmium_import.hpp"
#include "output.hpp"
#include "output_geojson.hpp"
#include "import_paths.hpp"
#include "helper.hpp"
#include "graph.hpp"
#include "checks.hpp"

#define AREABOUND_WEST -180.
#define AREABOUND_EAST 180.
#define AREABOUND_NORTH 85.
#define AREABOUND_SOUTH -85.

int compare(const void * a_p, const void * b_p) {
    size_t a = *(size_t *) a_p;
    size_t b = *(size_t *) b_p;
    if(a < b) {
        return -1;
    }
    if(a == b) {
        return 0;
    }
    return 1;
}

/**
 * Print all sizes of polygons to one file
 */
void getStatsOnPolygons(char * input, char * output_s) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, input);
    std::vector<size_t> sizes(paths.size());
    std::ofstream output(output_s);
    size_t i = 0;
    for(const auto& p:paths) {
        sizes[i++] = p.size();
    }
    sort(sizes.begin(), sizes.end());
    for(i = 0; i < paths.size(); i++) {
        output << std::to_string(sizes[i]) << "\n";
    }
}

/**
 * Print all edge errors
 */
void getErrorDistances(char * input, char * output_s) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, input);
    std::ofstream output(output_s);
    size_t totalSize = 0;
    for(const auto& p:paths) {
        if(p.size() > 0) {
            totalSize += p.size() - 1;
        }
    }
    std::vector<float> sizes(totalSize);

    size_t j = 0;
    for(const auto& p:paths) {
        for(size_t i = 1; i < p.size(); i++) {
            const ClipperLib::IntPoint& a = p[i-1];
            const ClipperLib::IntPoint& b = p[i];
            sizes[j++] = getErrorDistance(a, b);
        }
    }
    std::cerr << "About to write out " << j << " " << totalSize << std::endl;
    sort(sizes.begin(), sizes.end());
    for(size_t i = 0; i < totalSize; i++) {
        output << std::to_string(sizes[i]) << "\n";
    }
}

void getEdgeLengths(char * filename, char * output) {
    Graph g(filename);
    g.printEdgeLengths(output);
}

void findPath(char* filename, float x1, float y1, float x2, float y2, int algorithm) {
    Graph graph(filename);
    PathData p = findPath(&graph, x1, y1, x2, y2, algorithm);
    std::cout << "Path Length: " << p.length << std::endl;
    std::cout << "Distance over air: " << p.distance << std::endl;
    std::cout << "Heap Accesses: " << p.heap_accesses << std::endl;
    std::cout << "Time taken: " << p.duration << std::endl;
}

/**
 * Run graph creation for a given polygon file
 */
void testGraphCreation(char * filename, size_t node_count) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, filename);
    if(node_count < 2000) {
        OutlineHolderSimple outline_holder(paths);
        Graph g(outline_holder, node_count);
    } else {
        TreeOutlineHolder outline_holder(paths);
        Graph g(outline_holder, node_count);
    }
}

template<class T>
struct BenchmarkParam {
    T start;
    T mult;
    T end;
};

/**
 * Run graph creation for a given polygon file
 */
template<class T>
void benchmarkCreate(char * filename, size_t n, size_t warmup, struct BenchmarkParam<long> p_node_count, struct BenchmarkParam<long> p_tree_build) {
    std::list<ClipperLib::Path> paths;
    paths_import::readIn(paths, filename);
    for(long tree_build = p_tree_build.start; tree_build <= p_tree_build.end; tree_build *= p_tree_build.mult) {
        for(size_t node_count = p_node_count.start; node_count <= p_node_count.end; node_count *= p_node_count.mult) {
            long long timeTakenTreeBuild = 0;
            long long timeTakenGraphBuild = 0;
            for(size_t i = 0; i < n; i++) {
                auto start = std::chrono::high_resolution_clock::now();
                T outline_holder(paths);
                auto stop = std::chrono::high_resolution_clock::now();
                long long duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
                timeTakenTreeBuild += duration;

                start = std::chrono::high_resolution_clock::now();
                size_t pointsInX = std::sqrt(node_count);
                size_t pointsInY = node_count / pointsInX;
                size_t N = pointsInX * pointsInY;
                float distanceX = (AREABOUND_EAST - AREABOUND_WEST) / pointsInX;
                float distanceY = (AREABOUND_NORTH - AREABOUND_SOUTH) / pointsInY;

                std::cerr << "Done calculating Outlines" << std::endl;
                for(size_t x = 0; x < pointsInX; x++) {
                    const float curX = AREABOUND_WEST + x * distanceX + distanceX / 2;
                    for(size_t y = 0; y < pointsInY; y++) {
                        const float curY = AREABOUND_SOUTH + y * distanceY + distanceY / 2;
                        Node node;
                        node.position.X = toInt(curX);
                        node.position.Y = toInt(curY);
                        node.onWater = outline_holder.isPointInWater(node.position);
                        std::cerr << "\r" << x * pointsInY + y << ", " << node.onWater;
                    }
                }
                std::cerr << "\nDone generating Points " << i << std::endl;
                stop = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
                timeTakenGraphBuild += duration;
            }
            timeTakenTreeBuild /= n;
            timeTakenGraphBuild /= n;
            std::cout << typeid(T).name() << "$" << node_count << "$" << tree_build << ":" << timeTakenTreeBuild << ";" << timeTakenGraphBuild << std::endl;
        }
    }
}

void benchmark(char * filename, size_t n, size_t warmup) {
    Graph g(filename);
    struct {
        size_t from;
        size_t to;
    } queries[n];
    srand(time(0));
    for(auto& q : queries) {
        ClipperLib::IntPoint p1 = getRandomPoint();
        ClipperLib::IntPoint p2 = getRandomPoint();
        q.from = g.getNearestNode(p1);
        q.to = g.getNearestNode(p2);
    }
    uint8_t algorithms[] = {ALGORITHM_DIJKSTRA, ALGORITHM_A_STAR, ALGORITHM_CH_DIJSKTRA};
    struct {
        long long timeTaken;
        size_t length;
        size_t heap_accesses;
    } results[n - warmup][4];
    for(uint8_t algorithm : algorithms) {
        size_t skip = warmup;
        size_t current = 0;
        for(auto& q : queries) {
            auto start = std::chrono::high_resolution_clock::now();
            PathData p;
            switch (algorithm) {
                case ALGORITHM_DIJKSTRA:
                    p = g.getPathDijkstra(q.from, q.to);
                    break;
                case ALGORITHM_A_STAR:
                    p = g.getPathAStar(q.from, q.to);
                    break;
                case ALGORITHM_CH_DIJSKTRA:
                    p = g.getPathCHDijkstra(q.from, q.to);
                    break;
                default:
                    throw std::runtime_error("Unknown Algorithm");
            }
            auto stop = std::chrono::high_resolution_clock::now();
            long long duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
            if(skip > 0) {
                skip--;
                continue;
            }
            results[current++][algorithm] = {.timeTaken = duration, .length = p.length, .heap_accesses = p.heap_accesses};
        }
    }
    long long timeSum[4] = {0};
    long long heap_accesses_sum[4] = {0};
    long long squaredTimeSum[4] = {0};
    for(size_t i = 0; i < n - warmup; i++) {
        size_t length = results[i][algorithms[0]].length;
        for(auto algorithm:algorithms) {
            if(length > 2 && (length < results[i][algorithm].length - 2 || length > results[i][algorithm].length + 2)) { // Off by one errors when running CH_DIJKSTRA
                std::cerr << "Algorithms reported different length of path" << std::endl;
                std::cout << i << ": " << queries[i + warmup].from << "," << queries[i + warmup].to << ": " << results[i][1].length << "," << results[i][2].length << "," << results[i][3].length << std::endl;
                return;
            }
            timeSum[algorithm] += results[i][algorithm].timeTaken;
            heap_accesses_sum[algorithm] += results[i][algorithm].heap_accesses;
            timeSum[algorithm] += results[i][algorithm].timeTaken * results[i][algorithm].timeTaken;
        }
    }

    long long avgTime[4] = {0};
    long long avgHeap_Access[4] = {0};
    long long varianz[4] = {0};
    for(auto algorithm:algorithms) {
        avgTime[algorithm] = timeSum[algorithm] / (n - warmup);
        avgHeap_Access[algorithm] = heap_accesses_sum[algorithm] / (n - warmup);
        varianz[algorithm] = squaredTimeSum[algorithm] / (n - warmup) - avgTime[algorithm];
    }
    std::cout << "Dijkstra average Time   : " << avgTime[ALGORITHM_DIJKSTRA] << std::endl;
    std::cout << "A* average Time         : " << avgTime[ALGORITHM_A_STAR] << std::endl;
    std::cout << "CH Dijkstra average Time: " << avgTime[ALGORITHM_CH_DIJSKTRA] << std::endl;
    std::cout << "Dijkstra heap_accesses   : " << avgHeap_Access[ALGORITHM_DIJKSTRA] << std::endl;
    std::cout << "A* heap_accesses         : " << avgHeap_Access[ALGORITHM_A_STAR] << std::endl;
    std::cout << "CH Dijkstra heap_accesses: " << avgHeap_Access[ALGORITHM_CH_DIJSKTRA] << std::endl;
}

#define CHECK_PARAMETER(s, e) \
    if(argc < e + 2) { \
        std::cerr << "Missing parameters" << s << std::endl; \
        return 10; \
    }

int main(int argc, char ** argv) {
    if (argc < 2) return 20;
    std::string task(argv[1]);
    if(task == "stats") {
        CHECK_PARAMETER("Expected: input, output_s", 2);
        getStatsOnPolygons(argv[2], argv[3]);
    } else if(task == "statsError") {
        CHECK_PARAMETER("Expected: input, output_s", 2);
        getErrorDistances(argv[2], argv[3]);
    } else if(task == "statsEdgeLength") {
        CHECK_PARAMETER("Expected: input, output_s", 2);
        getEdgeLengths(argv[2], argv[3]);
    } else if(task == "create") {
        CHECK_PARAMETER("Expected: input, node_count", 2);
        size_t node_count = atoll(argv[3]);
        testGraphCreation(argv[2], node_count);
    } else if(task == "findPath") {
        CHECK_PARAMETER("Expected: input, x1, y1, x2, y2, algorithm", 6);
        float x1 = atof(argv[3]);
        float y1 = atof(argv[4]);
        float x2 = atof(argv[5]);
        float y2 = atof(argv[6]);
        int algorithm = atoi(argv[7]);
        findPath(argv[2], x1, y1, x2, y2, algorithm);
    } else if(task == "benchmark") {
        CHECK_PARAMETER("Expected: input, n, warmup", 3);
        int n = atoi(argv[3]);
        int warmup = atoi(argv[4]);
        if(warmup >= n) {
            std::cerr << "warmup has a nonsense value compare to n" << std::endl;
            return 1;
        }
        benchmark(argv[2], n, warmup);
    } else if(task == "benchmarkCreate") {
        CHECK_PARAMETER("Expected: input, n, warmup, node_count_start, node_count_mult, node_count_end, tree_param_start, tree_param_mult, tree_param_end, class", 10);
        int n = atoi(argv[3]);
        int warmup = atoi(argv[4]);
        struct BenchmarkParam<long> node_count;
        node_count.start = atol(argv[5]);
        node_count.mult = atol(argv[6]);
        node_count.end = atol(argv[7]);
        struct BenchmarkParam<long> tree_param;
        tree_param.start = atol(argv[8]);
        tree_param.mult = atol(argv[9]);
        tree_param.end = atol(argv[10]);
        if(warmup >= n) {
            std::cerr << "warmup has a nonsense value compare to n" << std::endl;
            return 1;
        }
        if(argv[11][0] == 's') {
            benchmarkCreate<OutlineHolderSimple>(argv[2], n, warmup, node_count, tree_param);
        } else if(argv[11][0] == 't') {
            benchmarkCreate<TreeOutlineHolder>(argv[2], n, warmup, node_count, tree_param);
        } else {
            benchmarkCreate<TreeOutlineHolder>(argv[2], n, warmup, node_count, tree_param);
            benchmarkCreate<OutlineHolderSimple>(argv[2], n, warmup, node_count, tree_param);
        }
    } else {
        std::cerr << "Unknown task: " << task << std::endl;
    }

}
