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
        graphsMatches = [GRAPH_DATA_RE.fullmatch(x) for x in os.listdir()]
        graphs = [m.group(1) for m in graphsMatches if m]
    return contours, graphs

@app.route('/generateGraph')
def generateGraph():
    createDirectory(DATA_DIR)
    data=[x for x in os.listdir(DATA_DIR) if checkDataDir(x)[0]]
    app.logger.debug(data)
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

@app.route('/map')
def show_map():
    return flask.render_template('map.html')
