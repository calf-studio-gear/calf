$(document).ready(function () {
    var items = [
        ["Index", "index.html", false],
        ["Calf", "Calf.html", "images/Calf.png"],
        ["Compressor", "Compressor.html", "images/Calf - Compressor.png"],
        ["Sidechain Compressor", "Sidechain Compressor.html", "images/Calf - Sidechain Compressor.png"],
        ["Multiband Compressor", "Multiband Compressor.html", "images/Calf - Multiband Compressor.png"],
        ["Deesser", "Deesser.html", "images/Calf - Deesser.png"],
        ["Pulsator", "Pulsator.html", "images/Calf - Pulsator.png"],
    ];
        
    var menu = '<div class="header"><img src="images/style_logo.png" align="middle" />';
    menu += '<a href="index.html" title="Index">Index</a>';
    var links = "";
    for(var i = 1; i < items.length; i++) {
        links += "<a href=\"" + items[i][1] + "\" title=\"" + items[i][0] + "\">";
        if(items[i][2]) {
            links += "<img src=\"" + items[i][2] + "\" alt=\"" + items[i][0] + "\" class=\"thickbox\">";
        }
        links += items[i][0] + "</a>";
    }
    menu += links + "</div>";
    $("body").prepend(menu);
    $(".index").prepend(links);
    $("h2").append("&nbsp;&laquo;").prepend("&raquo;&nbsp;");
    
});
