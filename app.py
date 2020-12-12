import flask
import osmium
app = flask.Flask(__name__)

class TestHandler(osmium.SimpleHandler):
    def __init__(self):
        self.tagKeys = []
        self.tagValues = []
        super().__init__()

    def way(self, w):
        try:
            self.foundWay = True
            for t in w.tags:
                key = str(t.k)
                value = str(t.v)
            #for n in w.nodes:
                #pass
        except Exception as e:
            app.logger.debug(e)

    def area(self, a):
        try:
            self.foundArea = True
            for t in a.tags:
                key = str(t.k)
                value = str(t.v)
                if key not in self.tagKeys: self.tagKeys.append(key)
                app.logger.debug("Tag: {}".format(key))
            #app.logger.debug("Area: {}".format(a.num_rings()))
        except Exception as e:
            app.logger.debug(e)

@app.route('/')
def index():
    return flask.redirect(flask.url_for('show_map'))

@app.route('/calculateOutlines/<source>')
def calculateOutlines(source):
    t = TestHandler()
    t.apply_file(source)
    reader = osmium.io.Reader(source)
    header = reader.header()
    return str(t.tagKeys) + "\n" + str(t.tagValues)

@app.route('/map')
def show_map():
    return flask.render_template('map.html')
