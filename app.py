import flask
app = flask.Flask(__name__)

@app.route('/')
def index():
    return flask.redirect(flask.url_for('show_map'))

@app.route('/map')
def show_map():
    return flask.render_template('map.html')
