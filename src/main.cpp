#include <iostream>
#include <fstream>
#include <cstdlib>
#include "stdlib.h"
#include "stdio.h"
#include "osmium_import.hpp"
#include "output.hpp"
#include "output_geojson.hpp"
#include "import_paths.hpp"
#include "helper.hpp"
#include "graph.hpp"
#include "checks.hpp"

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
    } else {
        std::cerr << "Unknown task: " << task << std::endl;
    }

}
