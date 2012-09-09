$(document).ready(function () {
    var items = [
        [
            "Basics",
            [
                ["Index", "index.html", "images/Index.png"],
                ["About", "About.html", "images/About.png"],
                ["Controls", "Controls.html", "images/Calf - Controls.png"],
                ["Calf Rack", "Calf.html", "images/Calf.png"]
            ]
        ],
        [
            "Synthesizer",
            [
                ["(Organ)", "Organ.html", "images/Calf - Organ - Tone Generator.png"],
                ["(Monosynth)", "Monosynth.html", "images/Calf - Monosynth - Audio Path.png"],
            ]
        ],
        [
            "Delay FX",
            [
                ["(Vintage Delay)", "Vintage Delay.html", "images/Calf - Vintage Delay.png"],
                ["(Reverb)", "Reverb.html", "images/Calf - Reverb.png"],
                ["(Multi Chorus)", "Multi Chorus.html", "images/Calf - Multi Chorus.png"],
                ["(Flanger)", "Flanger.html", "images/Calf - Flanger.png"],
                ["(Phaser)", "Phaser.html", "images/Calf - Phaser.png"],
            ]
        ],
        [
            "Dynamics",
            [
                ["Compressor", "Compressor.html", "images/Calf - Compressor.png"],
                ["Sidechain Compressor", "Sidechain Compressor.html", "images/Calf - Sidechain Compressor.png"],
                ["Multiband Compressor", "Multiband Compressor.html", "images/Calf - Multiband Compressor.png"],
                ["Deesser", "Deesser.html", "images/Calf - Deesser.png"],
                ["Limiter", "Limiter.html", "images/Calf - Limiter.png"],
                ["Multiband Limiter", "Multiband Limiter.html", "images/Calf - Multiband Limiter.png"],
                ["Gate", "Gate.html", "images/Calf - Gate.png"],
                ["Sidechain Gate", "Sidechain Gate.html", "images/Calf - Sidechain Gate.png"],
                ["Multiband Gate", "Multiband Gate.html", "images/Calf - Multiband Gate.png"]
            ]
        ],
        [
            "Filter",
            [
                ["Filter", "Filter.html", "images/Calf - Filter.png"],
                ["(Filterclavier)", "Filterclavier.html", "images/Calf - Filterclavier.png"],
                ["Equalizer 5 Band", "Equalizer5band.html", "images/Calf - Equalizer 5 Band.png"],
                ["Equalizer 8 Band", "Equalizer8band.html", "images/Calf - Equalizer 8 Band.png"],
                ["Equalizer 12 Band", "Equalizer12band.html", "images/Calf - Equalizer 12 Band.png"],
            ]
        ],
        [
            "Other",
            [
                ["Pulsator", "Pulsator.html", "images/Calf - Pulsator.png"],
                ["(Rotary Speaker)", "Rotary Speaker.html", "images/Calf - Rotary Speaker.png"],
                ["(Saturator)", "Saturator.html", "images/Calf - Saturator.png"],
                ["Exciter", "Exciter.html", "images/Calf - Exciter.png"],
                ["Bass Enhancer", "Bass Enhancer.html", "images/Calf - Bass Enhancer.png"],
                ["Mono Input", "Mono Input.html", "images/Calf - Mono Input.png"],
                ["Stereo Tools", "Stereo Tools.html", "images/Calf - Stereo Tools.png"],
                ["Analyzer", "Analyzer.html", "images/Calf - Analyzer.png"]
            ]
        ]
    ];

    var header = '<div class="header"><img src="images/style_logo.png" align="middle" />';
    var menu = "";
    for(var i = 0; i < items.length; i++) {
        menu += "<div class='menu'><h3>" + items[i][0] + "</h3><ul>";
        for(var j = 0; j < items[i][1].length; j++) {
            menu += "<li>";
            if(items[i][1][j][2]) {
//                menu += "<a class='thickbox' href='" + items[i][1][j][2] + "' title='" + items[i][1][j][0] + "'>";
//                menu += "<a href=\"" + items[i][1][j][1] + "\" title=\"" + items[i][1][j][0] + "\">"
                menu += "<img src=\"" + items[i][1][j][2] + "\" alt=\"" + items[i][1][j][0] + "\">";
//                menu += "</a>";
            }
            cl=""
            if(items[i][1][j][0][0] == "(")
                cl=" class='inactive'"
            menu += "<a" + cl + " href=\"" + items[i][1][j][1] + "\" title=\"" + items[i][1][j][0] + "\">" + items[i][1][j][0] + "</a></li>";
        }
        menu +="<br clear='all'></ul><br clear='all'/></div>";
    }
    header += menu + "<br clear='all'></div>";
    $("body").prepend("<div class='headerbg'>" + header + "</div>");
    $(".index").prepend(menu);
    $("h2").append("&nbsp;&laquo;").prepend("&raquo;&nbsp;");

});
