$(document).ready(function () {
    var items = [
        [
            "Basics",
            [
                ["Index", "index.html", false],
                ["Controls", "Controls.html", "images/Calf - Controls.png"],
                ["Calf Rack", "Calf.html", "images/Calf.png"]
            ]
        ],
        [
            "Synthesizer",
            [
                
            ]
        ],
        [
            "Delay FX",
            [
                ["(Vintage Delay)", "Vintage Delay.html", "images/Calf - Vintage Delay.png"],
                ["(Reverb)", "Reverb.html", "images/Calf - Reverb.png"],
                ["(Multichorus)", "Multichorus.html", "images/Calf - Multichorus.png"],
                ["(Flanger)", "Flanger.html", "images/Calf - Flanger.png"],
                ["(Phaser)", "Phaser.html", "images/Calf - Phaser.png"]
            ]
        ],
        [
            "Dynamics",
            [
                ["Compressor", "Compressor.html", "images/Calf - Compressor.png"],
                ["Sidechain Compressor", "Sidechain Compressor.html", "images/Calf - Sidechain Compressor.png"],
                ["Multiband Compressor", "Multiband Compressor.html", "images/Calf - Multiband Compressor.png"],
                ["Deesser", "Deesser.html", "images/Calf - Deesser.png"]
            ]
        ],
        [
            "Filter",
            [
                ["Filter", "Filter.html", "images/Calf - Filter.png"],
                ["Equalizer 5 Band", "Equalizer5band.html", "images/Calf - Equalizer 5 Band.png"],
                ["Equalizer 8 Band", "Equalizer8band.html", "images/Calf - Equalizer 8 Band.png"],
                ["Equalizer 12 Band", "Equalizer12band.html", "images/Calf - Equalizer 12 Band.png"],
            ]
        ],
        [
            "Other",
            [
                ["Pulsator", "Pulsator.html", "images/Calf - Pulsator.png"],
                ["(Saturator)", "Saturator.html", "images/Calf - Saturator.png"],
                ["(Exciter)", "Exciter.html", "images/Calf - Exciter.png"],
                ["(Bass Enhancer)", "Bass Enhancer.html", "images/Calf - Bass Enhancer.png"]
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
                menu += "<img src=\"" + items[i][1][j][2] + "\" alt=\"" + items[i][1][j][0] + "\">";
//                menu += "</a>";
            }
            menu += "<a href=\"" + items[i][1][j][1] + "\" title=\"" + items[i][1][j][0] + "\">" + items[i][1][j][0] + "</a></li>";
        }
        menu +="</ul><br clear='all'/></div>";
    }
    header += menu + "<br clear='all'></div>";
    $("body").prepend("<div class='headerbg'>" + header + "</div>");
    $(".index").prepend(menu);
    $("h2").append("&nbsp;&laquo;").prepend("&raquo;&nbsp;");
    
});
