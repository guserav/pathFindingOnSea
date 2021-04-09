# Pathfinding on a Sphere
This is a small project to find the shortest sea path between 2 point on earth.

This code is developed for an assignment in a university lecture.


## Structure
The project has 2 components:

### Python C module
The core functionality is written in C++ and exposed via python c bindings to the flask Frontend.

### Flask
Exposes the functionality of the backend through a webinterface to the user.

### Files
As there are quite computational expensive setup calculations to do the code heavily relies on files to cache the result of the previous operation. For this the following file structure is used.
```
├── data        # data cache contains one folder per processed input
│   ├── antarctica-latest.osm
│   │   ├── geojson.json
│   │   ├── graph-1000.bin
│   │   ├── graph-1000.json
│   │   └── outlines.bin
│   └── planet-coastlines       # Stores all data for one input file
│       ├── geojson.json        # The geojson representation of the input coastlines
│       ├── graph-1000.bin      # Graph with ~1000 based of the calculated outlines
│       ├── graph-1000.json     # geojson representation of the graph
│       └── outlines.bin        # Cache of the parsed and merged coastlines
└── input       # input data
    ├── antarctica-latest.osm.pbf
    └── planet-coastlines.pbf
```


## Setup
First make sure the following Dependencies are installed:

### Dependencies
- Python3
- Flask
- Python c bindings (typically in python or python-dev package)
- g++ (Tested with 9.3.0)
- pkg-config
- osmium (with bzip2, zlib, protozero and expat)

### Build and run flask
```
make

# For better error messages:
env FLASK_APP=app.py FLASK_ENV=development flask run
# The output will tell you what site to open in your browser typically http://127.0.0.1:5000/
```

### Generate a graph
Generating a graph needs a couple of steps.
1. Make sure that you inserted the intended input .pbf input files into `input/`
2. Open the webpage. `/` should automatically forward you to the right page to first generate the outlines.
    - Choose the appropriate input file and click the generate button
    - This should take a few moments the progress can be seen in the flask server output
    - After finishing you are automatically taken to the next page
3. On the generate graph page you can then choose the generated outlines file and proceed with generating a graph with ~#number nodes of your choosing. For a start I would recommend 1000
4. After finishing you are taken to the map and can start clicking on it to see if the path finding works.
    - Note: for node counts <= 1000 a geojson is generated that can be shown if wanted.
5. Run benchmark
```
build/a.out benchmark data/planet-coastlines/graph-1000000.bin 1000 10
```

## Libraries used
### Clipper
Clipper is used to check whether a point lies within land or not. It is a library that assumes a flat geometry but because the lines segments are short enough and there is no line crossing the 180 Meridian we can just assume we are on a flat surface and benefit from the fast PointInPolygon check implemented in Clipper.

### Osmium
This library is used to parse the line segments that form the coastlines from the pbf files.

### std::algorithm
Is used for its Quicksort and heap implementation.


## TODO
- [ ] Point In Polygon Goal: 500'000 - 1'000'000
