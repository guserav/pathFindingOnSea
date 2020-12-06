var styles = { // TODO
    'LineString': new ol.style.Style({
        stroke: new ol.style.Stroke({
            color: 'green',
            width: 1,
        }),
    }),
    'Start': new ol.style.Style({
        image: new ol.style.Circle({
            radius: 3,
            fill: new ol.style.Fill({color: 'rgba(0,0,255,0.1)'}),
            stroke: new ol.style.Stroke({color:'blue'}),
        })
    }),
    'End': new ol.style.Style({
        image: new ol.style.Circle({
            radius: 3,
            fill: new ol.style.Fill({color: 'rgba(0,255,0,0.1)'}),
            stroke: new ol.style.Stroke({color:'green'}),
        })
    }),
    'default': new ol.style.Style({
        stroke: new ol.style.Stroke({
            color: 'purple',
            width: 1,
        }),
        image: new ol.style.Circle({
            radius: 3,
            fill: new ol.style.Fill({color: 'rgba(255,0,0,0.1)'}),
            stroke: new ol.style.Stroke({color:'red'}),
        })
    }),
}
var styleFunction = function (feature) {
    var ret;
    ret = styles[feature.get('name')];
    if (!ret) {
        ret = styles[feature.getGeometry().getType()];
    }
    if(ret) return ret;
    return styles['default']
};

var backgroundSource = new ol.source.Vector({
    url: '/static/backgroundMap.json',
    format: new ol.format.GeoJSON({dataProjection: 'EPSG:4326', featureProjection: 'EPSG:3857'}),
});

var p1 = new ol.Feature({geometry: new ol.geom.Point([-3e6,3e6]), name: 'Start'});
var p2 = new ol.Feature({geometry: new ol.geom.Point([-1e6,-1e6]), name: 'End'});
var pointCollection = new ol.Collection([p1, p2]);

var startEndSource = new ol.source.Vector({
    features: pointCollection,
    //format: new ol.format.JSONFeature({dataProjection: 'EPSG:4326', featureProjection: 'EPSG:3857'}),
});

var backgroundLayer = new ol.layer.Vector({
    source: backgroundSource,
    style: styleFunction,
});

var startEndLayer = new ol.layer.Vector({
    source: startEndSource,
    style: styleFunction,
});

var map = new ol.Map({
    target: 'map',
    layers: [
        new ol.layer.Tile({
            source: new ol.source.OSM()
        }),
        backgroundLayer,
        startEndLayer
    ],
    view: new ol.View({
        center: [0, 0],
        zoom: 4
    })
});

var setPoint1 = true;
map.on('singleclick', function(e){
    console.log(ol.proj.transform(e.coordinate, e.map.getView().getProjection(), 'EPSG:4326'));
    if (setPoint1) {
        p1.setGeometry(new ol.geom.Point(e.coordinate));
    } else {
        p2.setGeometry(new ol.geom.Point(e.coordinate));
    }
    setPoint1 = !setPoint1;
});

function reloadBackground() {
    var vectorSource = new ol.source.Vector({
        url: '/static/backgroundMap.json',
        format: new ol.format.GeoJSON({dataProjection: 'EPSG:4326', featureProjection: 'EPSG:3857'}),
    });
    var vectorLayer = new ol.layer.Vector({
        source: vectorSource,
        style: styleFunction,
    });
    map.getLayers().setAt(1, vectorLayer);
}
