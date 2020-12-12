#include <iostream>
#include "stdlib.h"
#include "stdio.h"
#include "osmium_import.hpp"
#include "output.hpp"


int main(int argc, char ** argv) {
    if (argc < 2) return 10;
    NullStream null;
    importAndOutputFile(argv[1], std::cout, null);
}
