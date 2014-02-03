$(document).ready(function () {
    // subpage configuration
    var items = [
        [
            "Index",
            [
                ["Index", "index.html", "images/Index.png"],
                ["About", "About.html", "images/About.png"],
                ["Controls", "Controls.html", "images/Calf - Controls.png"],
                ["Calf Rack", "Calf.html", "images/Calf.png"]
            ],
            "icons/index.png"
        ],
        [
            "Synthesizer",
            [
                ["(Organ)", "Organ.html", "images/Calf - Organ - Sound Processor.png"],
                ["(Monosynth)", "Monosynth.html", "images/Calf - Monosynth - Audio Path.png"],
            ],
            "icons/synthesizer.png"
        ],
        [
            "Modulation",
            [
                ["(Multi Chorus)", "Multi Chorus.html", "images/Calf - Multi Chorus.png"],
                ["(Phaser)", "Phaser.html", "images/Calf - Phaser.png"],
                ["(Flanger)", "Flanger.html", "images/Calf - Flanger.png"],
                ["(Rotary Speaker)", "Rotary Speaker.html", "images/Calf - Rotary Speaker.png"],
                ["Pulsator", "Pulsator.html", "images/Calf - Pulsator.png"],
            ],
            "icons/modulation.png"
        ],
        [
            "Delay",
            [
                ["(Reverb)", "Reverb.html", "images/Calf - Reverb.png"],
                ["(Vintage Delay)", "Vintage Delay.html", "images/Calf - Vintage Delay.png"],
                ["Compensation Delay Line", "Compensation Delay Line.html", "images/Calf - Compensation Delay Line.png"],
            ],
            "icons/delay.png"
        ],
        [
            "Dynamics",
            [
                ["Compressor", "Compressor.html", "images/Calf - Compressor.png"],
                ["Sidechain Compressor", "Sidechain Compressor.html", "images/Calf - Sidechain Compressor.png"],
                ["Multiband Compressor", "Multiband Compressor.html", "images/Calf - Multiband Compressor.png"],
                ["Mono Compressor", "Mono Compressor.html", "images/Calf - Mono Compressor.png"],
                ["Deesser", "Deesser.html", "images/Calf - Deesser.png"],
                ["Gate", "Gate.html", "images/Calf - Gate.png"],
                ["Sidechain Gate", "Sidechain Gate.html", "images/Calf - Sidechain Gate.png"],
                ["Multiband Gate", "Multiband Gate.html", "images/Calf - Multiband Gate.png"],
                ["Limiter", "Limiter.html", "images/Calf - Limiter.png"],
                ["Multiband Limiter", "Multiband Limiter.html", "images/Calf - Multiband Limiter.png"],
                ["Transient Designer", "Transient Designer.html", "images/Calf - Transient Designer.png"]
            ],
            "icons/dynamics.png"
        ],
        [
            "Filters",
            [
                ["Filter", "Filter.html", "images/Calf - Filter.png"],
                ["(Filterclavier)", "Filterclavier.html", "images/Calf - Filterclavier.png"],
                ["Equalizer 5 Band", "Equalizer 5 Band.html", "images/Calf - Equalizer 5 Band.png"],
                ["Equalizer 8 Band", "Equalizer 8 Band.html", "images/Calf - Equalizer 8 Band.png"],
                ["Equalizer 12 Band", "Equalizer 12 Band.html", "images/Calf - Equalizer 12 Band.png"],
                ["(X-Over 2 Band)", "X-Over 2 Band.html", "images/Calf - X-Over 2 Band.png"],
                ["(X-Over 3 Band)", "X-Over 3 Band.html", "images/Calf - X-Over 3 Band.png"],
                ["(X-Over 4 Band)", "X-Over 4 Band.html", "images/Calf - X-Over 4 Band.png"],
            ],
            "icons/filters.png"
        ],
        [
            "Distortion",
            [
                ["(Saturator)", "Saturator.html", "images/Calf - Saturator.png"],
                ["Exciter", "Exciter.html", "images/Calf - Exciter.png"],
                ["Bass Enhancer", "Bass Enhancer.html", "images/Calf - Bass Enhancer.png"],
                ["Tape Simulator", "Tape Simulator.html", "images/Calf - Tape Simulator.png"],
            ],
            "icons/distortion.png"
        ],
        [
            "Tools",
            [
                ["Mono Input", "Mono Input.html", "images/Calf - Mono Input.png"],
                ["Stereo Tools", "Stereo Tools.html", "images/Calf - Stereo Tools.png"],
                ["Analyzer", "Analyzer.html", "images/Calf - Analyzer.png"],
            ],
            "icons/tools.png"
        ]
    ];
    
    // Build menu structure
    var menu = $("<div class='menu'/>");
    $("body").prepend(menu);
    for(var i = 0; i < items.length; i++) {
        var submenu = $("<ul class='submenu' id='menu_" + items[i][0] + "'/>").appendTo(menu);
        var icon = $("<img src='images/" + items[i][2] + "' alt='" + items[i][0] + "' class='micon' id='" + items[i][0] + "'/>").appendTo(menu);
        $("<span>" + items[i][0] + "</span>").appendTo(menu);
        icon.mouseenter(function (e) {
            show_menu("#menu_" + $(this).attr("id"));
        });
        icon.mouseleave(function (e) {
            hide_menu("#menu_" + $(this).attr("id"));
        });
        submenu.mouseenter(function (e) {
            show_menu(this);
        });
        submenu.mouseleave(function (e) {
            hide_menu(this);
        });
        for(var j = 0; j < items[i][1].length; j++) {
            if (!j) {
                $("<li><img class='marrow' src='images/marrow.png'/><h3>" + items[i][0] + "</h3></li>").appendTo(submenu);
                if ($("#index")) {
                    $("<h3><img src='images/" + items[i][2] + "' class='iicon'/>" + items[i][0] + "</h3>").appendTo($("#index"));
                    var iul = $("<ul/>").appendTo($("#index"));
                }
            }
            var li = $("<li/>").appendTo(submenu);
            if ($("#index") && items[i][1][j][2]) {
                var ili = $("<li/>").appendTo(iul);
                var l = $("<a href='" + items[i][1][j][1] + "'><span>" + items[i][1][j][0] + "</span></a>").appendTo(ili);
                $("<img src=\"" + items[i][1][j][2] + "\" alt=\"" + items[i][1][j][0] + "\">").prependTo(l);
            }
            var cl=items[i][1][j][0][0] == "(" ? " class='inactive'" : "";
            var a = $("<a" + cl + " href=\"" + items[i][1][j][1] + "\" title=\"" + items[i][1][j][0] + "\">" + items[i][1][j][0] + "</a>").appendTo(li);
            a.click(function (e) {
                hide_menu("#menu_" + $(this).parent().parent().attr("id"));
            });
            if (i >= items.length - 1)
                $("<br clear='all'/>").appendTo($("#index"));
        }
        submenu.css({
            top: Math.min(icon.position().top + 5, $(window).height() - submenu.outerHeight())
        });
    }
    $(".prettyPhoto").prettyPhoto({
        theme: 'dark_rounded',
        social_tools: '',
        show_title: false,
    });
    $(window).resize(resize);
    resize();
});

var hide_menu = function (id) {
    $(id).clearQueue();
    $(id).stop();
    $(id).animate({
        opacity: 0
    }, 300, function () { $(this).css({display: "none"});});
}
var show_menu = function (id) {
    $(id).clearQueue();
    $(id).stop();
    $(id).css({
        display: "block"
    });
    $(id).animate({
        opacity: 1.0
    }, 300);
}
var resize = function () {
    var width = $(".wrapper").width();
    $("body").css({
        'font-size'   : ((width / 1500 + 0.25) * 1.1) + "em",
        'line-height' : ((width / 1500 + 1) * 1.1) + "em"
    });
}
