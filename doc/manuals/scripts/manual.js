$(document).ready(function () {
    // subpage configuration
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
                ["Trigger", "Trigger.html", "images/Calf - Trigger.png"]
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
            $(".menu").addClass("mover");
            show_menu("#menu_" + $(this).attr("id"), "#" + $(this).attr("id"));
        });
        icon.mouseleave(function (e) {
            $(".menu").removeClass("mover");
            hide_menu("#menu_" + $(this).attr("id"));
        });
        submenu.mouseenter(function (e) {
            show_menu(this, "#" + $(this).attr("id").substr(5));
        });
        submenu.mouseleave(function (e) {
            hide_menu(this);
        });
        resize();
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
                if (items[i][1][j][0].substr(0,1) == "(")
                    ili.addClass("inactive");
            }
            var cl=items[i][1][j][0][0] == "(" ? " class='inactive'" : "";
            var a = $("<a" + cl + " href=\"" + items[i][1][j][1] + "\" title=\"" + items[i][1][j][0] + "\">" + items[i][1][j][0] + "</a>").appendTo(li);
            a.click(function (e) {
                hide_menu("#menu_" + $(this).parent().parent().attr("id"));
            });
            if (i >= items.length - 1)
                $("<br clear='all'/>").appendTo($("#index"));
        }
    }
    $(".prettyPhoto").prettyPhoto({
        theme: 'dark_rounded',
        social_tools: '',
        show_title: false,
    });
    $(window).resize(resize);
    $(window).scroll(scroll);
    resize();
});

var hide_menu = function (id) {
    $(id).clearQueue();
    $(id).stop();
    $(id).animate({
        opacity: 0
    }, 300, function () { $(this).css({display: "none"});});
}
var show_menu = function (id, icon) {
    $(id).clearQueue();
    $(id).stop();
    var w = $(window).innerHeight();
    var t = $(window).scrollTop();
    var y = Math.max(-t, Math.min(w - $(".menu").outerHeight(), 0));
    var t_ = Math.min($(icon).position().top + 5, w - $(id).outerHeight() - y);
    $(id).css({
        display: "block",
        top: t_
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

var scroll = function () {
    var h = $(".menu").outerHeight();
    var w = $(window).innerHeight();
    var t = $(window).scrollTop();
    $(".menu").css("top", Math.max(-t, Math.min(w - h, 0)));
    console.log(t, h, w)
}
