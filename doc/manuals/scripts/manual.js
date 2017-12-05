(function (w) {

var items = [
    [
        "Index",
        [
            ["Index", "index.html", "images/Index.png"],
            ["About", "About.html", "images/Calf Studio Gear.jpg"],
            ["Controls", "Controls.html", "images/Calf - Controls.jpg"],
            ["Calf Rack", "Calf.html", "images/Calf.jpg"]
        ],
        "icons/index.png"
    ],
    [
        "Synthesizer",
        [
            ["(Organ)", "Organ.html", "images/Calf - Organ - Sound Processor.jpg"],
            ["Monosynth", "Monosynth.html", "images/Calf - Monosynth - Audio Path.jpg"],
            ["(Fluidsynth)", "Fluidsynth.html", "images/Calf - Fluidsynth.jpg"],
            ["(Wavetable)", "Wavetable.html", "images/Calf - Wavetable.jpg"],
        ],
        "icons/synthesizer.png"
    ],
    [
        "Modulation",
        [
            ["(Multi Chorus)", "Multi Chorus.html", "images/Calf - Multi Chorus.jpg"],
            ["Phaser", "Phaser.html", "images/Calf - Phaser.jpg"],
            ["Flanger", "Flanger.html", "images/Calf - Flanger.jpg"],
            ["(Rotary Speaker)", "Rotary Speaker.html", "images/Calf - Rotary Speaker.jpg"],
            ["Pulsator", "Pulsator.html", "images/Calf - Pulsator.jpg"],
            ["Ring Modulator", "Ring Modulator.html", "images/Calf - Ring Modulator.jpg"],
        ],
        "icons/modulation.png"
    ],
    [
        "Delay",
        [
            ["Reverb", "Reverb.html", "images/Calf - Reverb.jpg"],
            ["Vintage Delay", "Vintage Delay.html", "images/Calf - Vintage Delay.jpg"],
            ["Compensation Delay Line", "Compensation Delay Line.html", "images/Calf - Compensation Delay Line.jpg"],
        ],
        "icons/delay.png"
    ],
    [
        "Dynamics",
        [
            ["Compressor", "Compressor.html", "images/Calf - Compressor.jpg"],
            ["Sidechain Compressor", "Sidechain Compressor.html", "images/Calf - Sidechain Compressor.jpg"],
            ["Multiband Compressor", "Multiband Compressor.html", "images/Calf - Multiband Compressor.jpg"],
            ["Mono Compressor", "Mono Compressor.html", "images/Calf - Mono Compressor.jpg"],
            ["Deesser", "Deesser.html", "images/Calf - Deesser.jpg"],
            ["Gate", "Gate.html", "images/Calf - Gate.jpg"],
            ["Sidechain Gate", "Sidechain Gate.html", "images/Calf - Sidechain Gate.jpg"],
            ["Multiband Gate", "Multiband Gate.html", "images/Calf - Multiband Gate.jpg"],
            ["Limiter", "Limiter.html", "images/Calf - Limiter.jpg"],
            ["Multiband Limiter", "Multiband Limiter.html", "images/Calf - Multiband Limiter.jpg"],
            ["Sidechain Limiter", "Sidechain Limiter.html", "images/Calf - Sidechain Limiter.jpg"],
            ["Transient Designer", "Transient Designer.html", "images/Calf - Transient Designer.jpg"]
        ],
        "icons/dynamics.png"
    ],
    [
        "Filters",
        [
            ["Filter", "Filter.html", "images/Calf - Filter.jpg"],
            ["Filterclavier", "Filterclavier.html", "images/Calf - Filterclavier.jpg"],
            ["Envelope Filter", "Envelope Filter.html", "images/Calf - Envelope Filter.jpg"],
            ["Emphasis", "Emphasis.html", "images/Calf - Emphasis.jpg"],
            ["Equalizer 5 Band", "Equalizer 5 Band.html", "images/Calf - Equalizer 5 Band.jpg"],
            ["Equalizer 8 Band", "Equalizer 8 Band.html", "images/Calf - Equalizer 8 Band.jpg"],
            ["Equalizer 12 Band", "Equalizer 12 Band.html", "images/Calf - Equalizer 12 Band.jpg"],
            ["(Equalizer 30 Band)", "Equalizer 30 Band.html", "images/Calf - Equalizer 30 Band.jpg"],
            ["(Vocoder)", "Vocoder.html", "images/Calf - Vocoder.jpg"],
        ],
        "icons/filters.png"
    ],
    [
        "Distortion",
        [
            ["(Saturator)", "Saturator.html", "images/Calf - Saturator.jpg"],
            ["Exciter", "Exciter.html", "images/Calf - Exciter.jpg"],
            ["Bass Enhancer", "Bass Enhancer.html", "images/Calf - Bass Enhancer.jpg"],
            ["Tape Simulator", "Tape Simulator.html", "images/Calf - Tape Simulator.jpg"],
            ["Vinyl", "Vinyl.html", "images/Calf - Vinyl.jpg"],
            ["Crusher", "Crusher.html", "images/Calf - Crusher.jpg"],
        ],
        "icons/distortion.png"
    ],
    [
        "Tools",
        [
            ["Mono Input", "Mono Input.html", "images/Calf - Mono Input.jpg"],
            ["Stereo Tools", "Stereo Tools.html", "images/Calf - Stereo Tools.jpg"],
            ["(Haas Stereo Enhancer)", "Haas Stereo Enhancer.html", "images/Calf - Haas Stereo Enhancer.jpg"],
            ["Analyzer", "Analyzer.html", "images/Calf - Analyzer.jpg"],
            ["(X-Over 2 Band)", "X-Over 2 Band.html", "images/Calf - X-Over 2 Band.jpg"],
            ["(X-Over 3 Band)", "X-Over 3 Band.html", "images/Calf - X-Over 3 Band.jpg"],
            ["(X-Over 4 Band)", "X-Over 4 Band.html", "images/Calf - X-Over 4 Band.jpg"],
        ],
        "icons/tools.png"
    ]
];
    
function init () {
    // Build menu structure
    var menu = element("ul", {"id":"menu"}, document.body);
    for(var i = 0; i < items.length; i++) {
        var entry = element("li", {}, menu);
        var submenu = element("ul", {"class":"submenu", "id":"menu_" + items[i][0]}, entry);
        var icon = element("img", {"src":"images/" + items[i][2], "alt":items[i][0], "class":"micon", "id": items[i][0]}, entry);
        var span = element("span", {}, entry);
        span.innerHTML = items[i][0];
        var index = getID("index");
        for(var j = 0; j < items[i][1].length; j++) {
            if (!j) {
                submenu.innerHTML = "<li><h3>" + items[i][0] + "</h3></li>";
                if (index) {
                    var h3 = element("h3");
                    h3.innerHTML = "<img src='images/" + items[i][2] + "' class='iicon'/>" + items[i][0];
                    index.appendChild(h3);
                    var iul = element("ul", {}, index);
                }
            }
            var li = element("li", {}, submenu);
            if (index && items[i][1][j][2]) {
                var ili = element("li", {}, iul);
                var l = element("a", {href:items[i][1][j][1]}, ili);
                var span = element("span", {}, l);
                span.innerHTML = items[i][1][j][0];
                var img = element("img", {alt:items[i][1][j][0], src:items[i][1][j][2]});
                l.insertBefore(img, l.firstChild);
                if (items[i][1][j][0].substr(0,1) == "(")
                    ili.classList.add("inactive");
            }
            var attr = {title:items[i][1][j][0], href:items[i][1][j][1]};
            if (items[i][1][j][0][0] == "(") attr["class"] = "inactive";
            var a = element("a", attr, li);
            a.innerHTML = items[i][1][j][0];
            if (i >= items.length - 1)
                element("br", {clear:'all'}, index);
        }
    }
};
function element (type, attr, parent) {
    var o = document.createElement(type);
    if (attr) {
        for (var i in attr) {
            if (attr.hasOwnProperty(i))
                o.setAttribute(i, attr[i]);
        }
    }
    if (parent)
        parent.appendChild(o);
    return o;
}
function getID (id) {
    return document.getElementById(id);
}

document.addEventListener("DOMContentLoaded", init);

})(this);
