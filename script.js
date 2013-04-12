var NUM_PARS = 30;
var PT_PER_PX = 0.5;
var PS = 12;
var VS = 14;
var PS_PX = PS / PT_PER_PX;
var VS_PX = VS / PT_PER_PX;
var TOP_MARGIN = 1.5 * VS_PX;
var BOT_MARGIN = 2 * TOP_MARGIN;
var PAR_SEP = VS_PX;
var MIN_GUTTER = 12 / PT_PER_PX;
var GALLEY_WIDTHS = [];
var PARAGRAPHS = [];
var PARAGRAPH_TREE;
var BODYHEIGHT = (window.innerHeight - BOT_MARGIN - TOP_MARGIN);

var FLOAT_TYPES = Object.freeze({
    NONE: 0,
    DUMB: 1,
    MSWORD: 2,
    SIMPLEQUEUE: 3
});

var FLOAT_TYPE;

FLOAT_TYPE = FLOAT_TYPES.DUMB;

$(document).ready(function() {

    /*
    // Create some paragraphs of text of varying lengths
    for (var i = 0; i < NUM_PARS; i++) {
        PARAGRAPHS.push(nrand());
    }

    console.log(PARAGRAPHS);

    */

    // 
    function Floatable(width, height) {
        'use strict';
        this.width = width;
        this.height = height;
    }

    // Here's one I made earlier
    PARAGRAPHS = [6408, new Floatable(50, 50), 10944, 8880, 8592, new Floatable(50, 25), new Floatable(25, 50), 3936, 7752, 10440, 7200, 11112, 10776, 12216, 7320, new Floatable(40, 30), 12504, 5904, 11496, 6144, 10944, 6216, 8640, 6336, 6312, 9888, 13176, 7176, 11712, 13104, 10752, 5880, 7320, 6792];

    // Now populate the widths array
    for (var i = 5; i <= 30; i += 5) {
        GALLEY_WIDTHS.push(PS_PX * i);
    }

    // Create the paragraph tree: one element per paragraph-level item
    PARAGRAPH_TREE = Array(PARAGRAPHS.length);

    // Now fill the PARAGRAPH_TREE array

    //For each paragraph-level item...
    for (var i = 0; i < PARAGRAPHS.length; i++) {

        if (typeof PARAGRAPHS[i] === 'number') {
            // ...if it's a normal paragraph, add an array with one element per galley width
            PARAGRAPH_TREE[i] = Array(GALLEY_WIDTHS.length);

            //Now populate the array
            for (var j = 0; j < GALLEY_WIDTHS.length; j++) {

                var size_left = PARAGRAPHS[i];
                var gw = GALLEY_WIDTHS[j];
                PARAGRAPH_TREE[i][j] = [];

                while (size_left > 0) {
                    if (size_left < gw) {
                        gw = size_left;
                    }
                    PARAGRAPH_TREE[i][j].push(gw);
                    size_left -= gw;
                }
            }
        } else {
            // It must be a Floatable object. Just copy it across.
            PARAGRAPH_TREE[i] = PARAGRAPHS[i];
        }
    }

    doLayout();

});

function doLayout() {
    //console.info("Window size is ("+window.innerWidth+", "+window.innerHeight+")");

    // Choose display width
    var pageWidth = window.innerWidth;
    var numCols;
    var badness = Array(GALLEY_WIDTHS.length);
    var min_badness_index = 0;

    for (var i = 0; i < GALLEY_WIDTHS.length; i++) {
        var nc = Math.floor(pageWidth / (GALLEY_WIDTHS[i] + MIN_GUTTER));
        var ex_ws = pageWidth % ((GALLEY_WIDTHS[i] + MIN_GUTTER));
        var rq_ws = nc * MIN_GUTTER;

        badness[i] = ((ex_ws /*+ rq_ws*/) + 100) * Math.sqrt(nc);
        if (nc == 0) {
            badness[i] = 1000 * i;
        }

        if (badness[i] <= badness[min_badness_index]) {
            min_badness_index = i;
            numCols = nc;
        }
    }

    var galleywidth = GALLEY_WIDTHS[min_badness_index];

    var colsep = Math.floor(pageWidth / numCols);
    var curr_x = (colsep - galleywidth) / 2;
    var curr_y = TOP_MARGIN;

    // Shove lines onto screen

    $('#main').empty(); //clear old stuff

    var dropdown = '<select id="dropdown" onchange="updateFloatType(this)">\n';
    for (var name in FLOAT_TYPES) {
        dropdown += '<option value="' + FLOAT_TYPES[name] + '"' + (FLOAT_TYPE == FLOAT_TYPES[name] ? " selected" : "") + '>' + name + '</option>'; 
    }
    dropdown += "</select>\n";

    outputDiv("title", 1000, PS_PX + 5, (window.innerWidth - 1000) / 2, 3, dropdown + (FLOAT_TYPE == FLOAT_TYPES.NONE ? "none" : (FLOAT_TYPE == FLOAT_TYPES.DUMB ? "dumb" : (FLOAT_TYPE == FLOAT_TYPES.MSWORD ? "MS Word" : ( FLOAT_TYPE == FLOAT_TYPES.SIMPLEQUEUE ? "simple queue" : "unknown (" + FLOAT_TYPE + ")")))) + ", " + numCols + " cols, badness: "+ Math.round(badness[min_badness_index] * 100) / 100 );

    var floatQueue = [];
    var lineQueue = [];

    for (var p = 0; p < PARAGRAPH_TREE.length; p++) {
        if (curr_y > (window.innerHeight - BOT_MARGIN)) { //start new column
            curr_y = TOP_MARGIN;
            curr_x += colsep;
        }

        if (Array.isArray(PARAGRAPH_TREE[p])) {
            // We assume it's a normal paragraph
            for (var l = 0; l < PARAGRAPH_TREE[p][min_badness_index].length; l++) {
                if (curr_y > (window.innerHeight - BOT_MARGIN)) { //start new column
                    curr_y = TOP_MARGIN;
                    curr_x += colsep;
                }
                width = PARAGRAPH_TREE[p][min_badness_index][l];
                outputLine(width, PS_PX, curr_x, curr_y, 'W' + min_badness_index + ' P' + p + ' L' + l + ' Qwertyuiop Lorem ipsum dolor sit amet, consectetur adipiscing elit.');

                curr_y += VS_PX;
            }
        } else {
            // It must be a Floatable object

            // Scale width to fit column (and height proportionately)
            var w = PARAGRAPH_TREE[p].width;
            var h = PARAGRAPH_TREE[p].height;

            var scale = galleywidth / w;

            w *= scale;
            h *= scale;

            if (h > BODYHEIGHT) {
                scale = BODYHEIGHT / h;
                w *= scale;
                h *= scale;
            }

            if (FLOAT_TYPE == FLOAT_TYPES.DUMB) {

                // Just stick the figure out wherever
                outputFloat(w, h, curr_x, curr_y);
                curr_y += h;

            } else if (FLOAT_TYPE == FLOAT_TYPES.MSWORD) {

                // check if it'll actually fit in the column, and if not, move to a new one
                if (curr_y + h > (window.innerHeight - BOT_MARGIN)) {
                    curr_y = TOP_MARGIN;
                    curr_x += colsep;
                }

                outputFloat(w, h, curr_x, curr_y);
                curr_y += h;

            } else if (FLOAT_TYPE == FLOAT_TYPES.SIMPLEQUEUE) {
                if (curr_y + h > (window.innerHeight - BOT_MARGIN)) {
                    // Won't fit in place, so enqueue it
                    floatQueue.push(PARAGRAPH_TREE[p]);

                    // Now output lines as normal until the bottom of the column, and enqueue the rest
                    // (also enqueue any further figures encountered whilst doing this)
                    for ( ; p < PARAGRAPH_TREE.length; p++) {

                        if (Array.isArray(PARAGRAPH_TREE[p])) {
                            // It's a normal paragraph
                            for (var l = 0; l < PARAGRAPH_TREE[p][min_badness_index].length; l++) {
                                if (curr_y > (window.innerHeight - BOT_MARGIN)) { //start new column
                                    while (l < PARAGRAPH_TREE[p][min_badness_index].length) {
                                        lineQueue.push(PARAGRAPH_TREE[p][min_badness_index][l++]);
                                    }
                                    curr_y = TOP_MARGIN;
                                    curr_x += colsep;
                                    break;
                                }
                                width = PARAGRAPH_TREE[p][min_badness_index][l];
                                outputLine(width, PS_PX, curr_x, curr_y, 'W' + min_badness_index + ' P' + p + ' L' + l + ' Qwertyuiop Lorem ipsum dolor sit amet, consectetur adipiscing elit.');

                                curr_y += VS_PX;
                            }
                            if (curr_y != TOP_MARGIN) {
                                curr_y += PAR_SEP;
                            }
                        } else {
                            // It must be a Floatable object
                            floatQueue.push(PARAGRAPH_TREE[p]);
                        }
                    }

                    // Now output as many floats as we can fit from the float queue
                    console.log('a' + floatQueue);
                    while (floatQueue.length > 0) {
                        console.log('b' + floatQueue);
                        // Scale width to fit column (and height proportionately)
                        var floatable = floatQueue[0];

                        var w = floatable.width;
                        var h = floatable.height;

                        var scale = galleywidth / w;

                        w *= scale;
                        h *= scale;

                        if (h > BODYHEIGHT) {
                            scale = BODYHEIGHT / h;
                            w *= scale;
                            h *= scale;
                        }

                        // If it's too big to fit, break out of the loop
                        if (curr_y + h > (window.innerHeight - BOT_MARGIN)) {
                            break;
                        }
                        console.log('c' + floatQueue);
                        // Otherwise dequeue and output it
                        floatQueue.shift();
                         console.log('d' + floatQueue);
                        outputFloat(w, h, curr_x, curr_y);
                        curr_y += h;
                    }

                    // Now output the rest of the lines
                    

                } else {
                    // output it in place
                    outputFloat(w, h, curr_x, curr_y);
                    curr_y += h;
                }

            } else {
                    // Don't output anything.
                    // console.log(FLOAT_TYPE);
            }
        }
        curr_y += PAR_SEP;

    }

}

function outputLine(width, height, x, y, text) {
    outputDiv("line", width, height, x, y, text);
}

function outputFloat(width, height, x, y) {
    outputDiv("floatable", width, height, x, y, "figure");
}

function outputDiv(cssclass, width, height, x, y, text) {
    $('#main').append('<div class="' + cssclass + '" style="width: ' + width + 'px; height: ' + height + 'px; position: absolute; top: ' + y + 'px; left: ' + x + 'px; font-size: ' + PS_PX + 'px">' + text + '</div>\n');
}

function updateFloatType(dd) {
    FLOAT_TYPE = dd.options[dd.selectedIndex].value;
    doLayout();
}

var RESIZETIMER;
$(window).resize(function() {
    clearTimeout(RESIZETIMER);
    RESIZETIMER = setTimeout(doLayout, 200);
    doLayout();
});

function nrand() { // thanks to http://www.protonfish.com/random.shtml
    var r = 2 * Math.random() - 1;
    r += 2 * Math.random() - 1;
    r += 2 * Math.random() - 1;
    var mean = 350;
    var sd = 100;
    return PS_PX * Math.floor(sd * r + mean);
}

function generatePar(width) {
    var size = nrand();

    for (j=0; size > 0; j++) {
        if (size < width)
            width = size;
        $('#main').append('<div class="bgblue" style="width: '+width+'em; height: 12pt"></div>');
        size -= width;
    }
}
