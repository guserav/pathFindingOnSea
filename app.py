from build import backend
import flask
import os
from werkzeug.utils import secure_filename
app = flask.Flask(__name__)
app.secret_key = b'_5#y2L"F4Q8z\n\xec]/'

def createDirectory(name):
    os.makedirs(name, exist_ok=True)

@app.route('/')
def index():
    return flask.redirect(flask.url_for('show_map'))

@app.route('/calculateOutlines')
def calculateOutlines():
    INPUT_DIR='input'
    DATA_DIR='data'
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
            dataOut = os.path.join(outdir, "data.bin")
            backend.parseOSMdata(filename, geojsonOut, dataOut)

    return flask.render_template('calculateOutlines.html', files=files)

@app.route('/generateGraph')
def generateGraph():
    return flask.render_template('generateGraph.html', data=data)

@app.route('/map')
def show_map():
    return flask.render_template('map.html')
