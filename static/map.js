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

var backgroundSource = new ol.source.Vector({});

var p1 = [-10,10];
var p2 = [-10,-10];
var pMap1 = new ol.Feature({geometry: new ol.geom.Point(ol.proj.transform(p2, 'EPSG:4326', 'EPSG:3857')), name: 'Start'});
var pMap2 = new ol.Feature({geometry: new ol.geom.Point(ol.proj.transform(p2, 'EPSG:4326', 'EPSG:3857')), name: 'End'});
var pointCollection = new ol.Collection([pMap1, pMap2]);

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
        backgroundLayer, //for the graph and path
        startEndLayer
    ],
    view: new ol.View({
        center: [0, 0],
        zoom: 1
    })
});

var setPoint1 = true;
const distanceDisplay = document.querySelector('#distance');
const lengthDisplay = document.querySelector('#length');
const startDisplay = document.querySelector('#start');
startDisplay.textContent = p1;
const endDisplay = document.querySelector('#end');
endDisplay.textContent = p2;
const data_source = document.querySelector('#data_source');
map.on('singleclick', function(e){
    var newCords = ol.proj.transform(e.coordinate, e.map.getView().getProjection(), 'EPSG:4326')
    console.log(newCords);
    if (setPoint1) {
        p1 = newCords;
        startDisplay.textContent = p1;
        pMap1.setGeometry(new ol.geom.Point(e.coordinate));
    } else {
        p2 = newCords;
        endDisplay.textContent = p2;
        pMap2.setGeometry(new ol.geom.Point(e.coordinate));
    }
    fetch('/findPath/' + data_source.selectedOptions[0].value + '/' + p1[0] + '/' + p1[1] + '/' + p2[0] + '/' + p2[1]).then(function(response) {
        response.text().then(function(text) {
            data = JSON.parse(text);
            distanceDisplay.textContent = data['distance'];
            lengthDisplay.textContent = data['length'];
            var vectorSource = new ol.source.Vector({
                url: '/lastPath.json',
                format: new ol.format.GeoJSON({dataProjection: 'EPSG:4326', featureProjection: 'EPSG:3857'}),
            });
            var vectorLayer = new ol.layer.Vector({
                source: vectorSource,
                style: styleFunction,
            });
            map.getLayers().setAt(1, vectorLayer);
        });
    });
    setPoint1 = !setPoint1;
});

function reloadBackground() {
    var vectorSource = new ol.source.Vector({
        url: '/data/json/' + data_source.selectedOptions[0].value,
        format: new ol.format.GeoJSON({dataProjection: 'EPSG:4326', featureProjection: 'EPSG:3857'}),
    });
    var vectorLayer = new ol.layer.Vector({
        source: vectorSource,
        style: styleFunction,
    });
    map.getLayers().setAt(1, vectorLayer);
}
