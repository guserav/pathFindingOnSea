from build import backend
import flask
import os
import re
from werkzeug.utils import secure_filename
app = flask.Flask(__name__)
app.secret_key = b'_5#y2L"F4Q8z\n\xec]/'

DATA_DIR='data'
OUTLINES_DATA='outlines.bin'
GRAPH_GEOJSON='graph-{:n}.json'
GRAPH_DATA='graph-{:n}.bin'
GRAPH_DATA_RE=re.compile('graph-([0-9]+).bin')
LAST_PATH_GEOJSON = ""

def createDirectory(name):
    os.makedirs(name, exist_ok=True)

@app.route('/')
def index():
    return flask.redirect(flask.url_for('show_map'))

@app.route('/calculateOutlines')
def calculateOutlines():
    INPUT_DIR='input'
    createDirectory(INPUT_DIR)
    files=os.listdir(INPUT_DIR)
    source = flask.request.args.get('filename', None)
    if source:
        filename = os.path.join(INPUT_DIR,secure_filename(source))
        if not os.path.exists(filename):
            flask.flash("Couldn't find {}".format(filename))
        else:
            basename = os.path.basename(filename)
            name, ext = os.path.splitext(basename)
            outdir = os.path.join(DATA_DIR, name)
            createDirectory(outdir)
            geojsonOut = os.path.join(outdir, "geojson.json")
            dataOut = os.path.join(outdir, OUTLINES_DATA)
            backend.parseOSMdata(filename, geojsonOut, dataOut)
            return flask.redirect(flask.url_for('generateGraph'))

    return flask.render_template('calculateOutlines.html', files=files)

def checkDataDir(name):
    path = os.path.join(DATA_DIR, name)
    contours = False
    graphs = []
    if os.path.isdir(path):
        contoursFile = os.path.join(path, OUTLINES_DATA)
        if os.path.exists(contoursFile):
            contours = True
        graphsMatches = [GRAPH_DATA_RE.fullmatch(x) for x in os.listdir(path)]
        graphs = [m.group(1) for m in graphsMatches if m]
    return contours, graphs

@app.route('/generateGraph')
def generateGraph():
    createDirectory(DATA_DIR)
    data=[x for x in os.listdir(DATA_DIR) if checkDataDir(x)[0]]

    source = flask.request.args.get('data_source', None)
    size = int(flask.request.args.get('node_count', 0))
    if source and size > 0:
        sourceFile = os.path.join(DATA_DIR, source, OUTLINES_DATA)
        graphFile = os.path.join(DATA_DIR, source, GRAPH_DATA.format(size))
        graph = backend.Graph(sourceFile, size)
        graph.output(graphFile)
        if size <= 1000:
            geojsonFile = os.path.join(DATA_DIR, source, GRAPH_GEOJSON.format(size))
            graph.output_geojson(geojsonFile)

    return flask.render_template('generateGraph.html', data=data)

@app.route('/calculateDistance/<x1>/<y1>/<x2>/<y2>')
def calculateDistance(x1, y1, x2, y2):
    return flask.jsonify({'distance': backend.calculate_distance(float(x1), float(y1), float(x2), float(y2))}), 200

@app.route('/findPath/<data_source>/<x1>/<y1>/<x2>/<y2>')
def getPath(data_source, x1, y1, x2, y2):
    global LAST_PATH_GEOJSON
    data_source = data_source.split(";")
    data = data_source[0]
    node_count = int(data_source[1])
    graph = backend.Graph(os.path.join(DATA_DIR, data, GRAPH_DATA.format(node_count)))
    path_data = graph.find_path(float(x1), float(y1), float(x2), float(y2))
    LAST_PATH_GEOJSON = path_data['geojson']
    return flask.jsonify(path_data), 200

@app.route('/lastPath.json')
def lastPathGeojson():
    global LAST_PATH_GEOJSON
    response = app.response_class(
        response=LAST_PATH_GEOJSON,
        status=200,
        mimetype='application/json'
    )
    return response

@app.route('/data/json/<data_source>')
def get_json_for_data(data_source):
    data_source = data_source.split(";")
    data = data_source[0]
    node_count = int(data_source[1])
    path = os.path.join(DATA_DIR, data, GRAPH_GEOJSON.format(node_count))
    if os.path.exists(path):
        return flask.send_from_directory(os.path.join(DATA_DIR, data), GRAPH_GEOJSON.format(node_count), cache_timeout=1)
    else:
        return app.response_class(response="", status=404)


@app.route('/map')
def show_map():
    createDirectory(DATA_DIR)
    data={x: checkDataDir(x) for x in os.listdir(DATA_DIR) if checkDataDir(x)[1]}
    return flask.render_template('map.html', data=data)
