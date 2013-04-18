var PT_PER_IN = 72;
var PX_PER_PT = 2;
var PX_PER_IN = PX_PER_PT * PT_PER_IN;
var PS = 12;
var VS = 14;
var PS_PX = PS * PX_PER_PT;
var VS_PX = VS * PX_PER_PT;
var TOP_MARGIN = 1.5 * VS_PX;
var BOT_MARGIN = TOP_MARGIN;
var PAR_SEP = VS_PX;
var MIN_GUTTER = 12 * PX_PER_PT;
var GALLEY_WIDTHS = [];
var PARAGRAPHS = [];
var PARAGRAPH_TREE;
var BODYHEIGHT;
var FLOAT_QUEUE;
var LINE_QUEUE;
var CURR_P;
var CURR_ROW;
var CURR_COL;

var COLUMNS;
var NUM_ROWS;

var FLOAT_TYPES = Object.freeze({
    NONE: 0,
    DUMB: 1,
    MSWORD: 2,
    SIMPLEQUEUE: 3
});

var MULTI_COLUMN = false;

var FLOAT_TYPE;

FLOAT_TYPE = FLOAT_TYPES.DUMB;

function Floatable(width, height) {
    this.width = width;
    this.height = height;
}

function getDiv(cssclass, width, height, text) {
    return '<div class="' + cssclass + '" style="width: ' + width + 'px; height: ' + height + 'px; font-size: ' + PS_PX + 'px">' + text + '</div>\n';
}

function getLine(width, height, text) {
    return getDiv("line", width, height, text);
}

function getFloat(width, height, text) {
    return getDiv("floatable", width, height, text);
}

function outputDiv(cssclass, x, y, text) {
    $('#main').append('<div class="' + cssclass + '" style="position:absolute; top: ' + y + 'px; left: ' + x + 'px">' + text + '</div>');
}

function nrand() { // thanks to http://www.protonfish.com/random.shtml
    var r, mean, sd;
    r = 2 * Math.random() - 1;
    r += 2 * Math.random() - 1;
    r += 2 * Math.random() - 1;
    mean = 350;
    sd = 100;
    return PS_PX * Math.floor(sd * r + mean);
}

function generatePar(width) {
    var size, j;
    size = nrand();

    for (j = 0; size > 0; j++) {
        if (size < width) {
            width = size;
        }
        $('#main').append('<div class="bgblue" style="width: ' + width + 'em; height: 12pt"></div>');
        size -= width;
    }
}

function nextPos() {
    // Increments CURR_ROW and CURR_COL to next available empty space (possibly the same empty space)
    var c, r;
    for (c = CURR_COL; true; c++) {
        for (r = CURR_ROW; r < NUM_ROWS; r++) {
            if (Array.isArray(COLUMNS[c])) {
                if (COLUMNS[c][r] === undefined) {
                    CURR_COL = c;
                    CURR_ROW = r;
                    return;
                }
            } else {
                COLUMNS[c] = [];
                CURR_COL = c;
                CURR_ROW = 0;
                return;
            }
        }
        CURR_ROW = 0;
    }
}

function enqueueNext(galleywidth, min_badness_index) {
    var l, w, h, scale;
    if (CURR_P >= PARAGRAPH_TREE.length) {
        return false;
    }

    if (Array.isArray(PARAGRAPH_TREE[CURR_P])) {
        for (l = 0; l < PARAGRAPH_TREE[CURR_P][min_badness_index].length; l++) {
            LINE_QUEUE.push(PARAGRAPH_TREE[CURR_P][min_badness_index][l]);
        }
    } else {

        // Scale width to fit column (and height proportionately)
        w = PARAGRAPH_TREE[CURR_P].width;
        h = PARAGRAPH_TREE[CURR_P].height;

        scale = galleywidth / w;

        w *= scale;
        h *= scale;

        if (h > BODYHEIGHT) {
            scale = BODYHEIGHT / h;
            w *= scale;
            h = BODYHEIGHT;
        }

        FLOAT_QUEUE.push(new Floatable(w, h));
    }
    CURR_P++;
    return true;
}

function doLayout() {
    var i, nc, ex_ws, rq_ws, galleywidth, colsep, curr_x, curr_y, dropdown, pageWidth, numCols, badness, min_badness_index, name, p, l, w, h, scale, span;

    pageWidth = window.innerWidth;
    badness = [];
    min_badness_index = 0;

    BODYHEIGHT = (window.innerHeight - BOT_MARGIN - TOP_MARGIN);

    for (i = 0; i < GALLEY_WIDTHS.length; i++) {
        nc = Math.floor(pageWidth / (GALLEY_WIDTHS[i] + MIN_GUTTER));
        ex_ws = pageWidth % ((GALLEY_WIDTHS[i] + MIN_GUTTER));
        rq_ws = nc * MIN_GUTTER;

        badness.push((ex_ws /*+ rq_ws*/ + 100) * Math.sqrt(nc));
        if (nc === 0) {
            badness[i] = 1000 * i;
        }

        if (badness[i] <= badness[min_badness_index]) {
            min_badness_index = i;
            numCols = nc;
        }
    }

    galleywidth = GALLEY_WIDTHS[min_badness_index];

    colsep = Math.floor(pageWidth / numCols);

    /*  
        Previously, we (and I quote) "just shove[d] lines onto screen" -- Now we need to populate the COLUMNS array and then output the contents of that to the screen.

        So, what goes into the COLUMNS array?
            - There must be one element for every column in the document
            - These can be .push()ed on the fly
            - These are themselves arrays with one element per multiple of the main leading (VS_PX in this case) that will fit in the column
                - If an element is undefined -- we assume it's available
                - Otherwise the element will be a string containing the html to output (possibly the empty string)
                    - the html should specify its own dimensions, but not its position (this will be calculated by its position in the COLUMNS array)

        Yep, that sounds plausible.

    */

    COLUMNS = [];

    // How many lines? We have (window.innerHeight - TOP_MARGIN - BOT_MARGIN) total space, and each line is VS_PX high. So:
    NUM_ROWS = Math.floor((window.innerHeight - TOP_MARGIN - BOT_MARGIN) / VS_PX);
    if (NUM_ROWS < 1) {
        NUM_ROWS = 1;
    }

    CURR_ROW = 0;
    CURR_COL = 0;

    if (FLOAT_TYPE === FLOAT_TYPES.SIMPLEQUEUE) {/*
        FLOAT_QUEUE = [];
        LINE_QUEUE = [];
        CURR_P = 0;

        while (enqueueNext(galleywidth, min_badness_index)) {
            while ((FLOAT_QUEUE.length > 0 && ((FLOAT_QUEUE[0].height + curr_y) <= (window.innerHeight - BOT_MARGIN))) || LINE_QUEUE.length > 0) {
                if (curr_y > (window.innerHeight - BOT_MARGIN)) { //start new column
                    curr_y = TOP_MARGIN;
                    curr_x += colsep;
                }

                if (FLOAT_QUEUE.length > 0 && (FLOAT_QUEUE[0].height + curr_y) <= (window.innerHeight - BOT_MARGIN)) {
                    outputFloat(FLOAT_QUEUE[0].width, FLOAT_QUEUE[0].height, curr_x, curr_y);
                    curr_y += FLOAT_QUEUE[0].height;
                    curr_y += PAR_SEP;
                    FLOAT_QUEUE.shift();
                } else if (LINE_QUEUE.length > 0) {
                    outputLine(LINE_QUEUE[0], PS_PX, curr_x, curr_y, 'W' + min_badness_index + ' Qwertyuiop Lorem ipsum dolor sit amet, consectetur adipiscing elit.');
                    curr_y += VS_PX;
                    LINE_QUEUE.shift();
                    if (LINE_QUEUE.length === 0) { // end of a paragraph
                        curr_y += PAR_SEP;
                    }
                }

            }
        }
*/
    } else {

        for (p = 0; p < PARAGRAPH_TREE.length; p++) {

            if (Array.isArray(PARAGRAPH_TREE[p])) {
                // We assume it's a normal paragraph
                for (l = 0; l < PARAGRAPH_TREE[p][min_badness_index].length; l++) {
                    nextPos();
                    COLUMNS[CURR_COL][CURR_ROW] = getLine(PARAGRAPH_TREE[p][min_badness_index][l], PS_PX, 'W' + min_badness_index + ' P' + p + ' L' + l + ' Qwertyuiop Lorem ipsum dolor sit amet, consectetur adipiscing elit.');
                }
                nextPos();
                if (CURR_ROW !== 0) {
                    COLUMNS[CURR_COL][CURR_ROW] = "";
                }
            } else {
                // It must be a Floatable object
                if (FLOAT_TYPE === FLOAT_TYPES.NONE) {
                    continue;
                }

                // Scale width to fit column (and height proportionately)
                w = PARAGRAPH_TREE[p].width;
                h = PARAGRAPH_TREE[p].height;

                // Firstly, scale down the height if it's bigger than the body height
                if (h > BODYHEIGHT) {
                    scale = BODYHEIGHT / h;
                    w *= scale;
                    h *= scale;
                }

                // Now have a look at the width and see if it could span any columns
                span = Math.round(w / galleywidth);

                if (span > numCols) {
                    span = numCols;
                }

                if (span < 1 || !MULTI_COLUMN) {
                    span = 1;
                }

                scale = (span * galleywidth + (span - 1) * (colsep - galleywidth)) / w;

                w *= scale;
                h *= scale;

                if (FLOAT_TYPE === FLOAT_TYPES.DUMB) {

                    // Just stick the figure out wherever
                    nextPos();
                    // Now reserve some space for the figure
                    
                    // How many lines do we need to reserve?
                    // Let's say if it's less than n.5,  no need for an extra space
                    var nlines = Math.round(h / VS_PX) + 1;
                    for (i = CURR_ROW; i < CURR_ROW + nlines; i++) {
                        for (j = CURR_COL; j < CURR_COL + span; j++) {
                            if (!Array.isArray(COLUMNS[j])) {
                                COLUMNS[j] = [];
                            }
                            COLUMNS[j][i] = "";
                        }
                    }
                    
                    COLUMNS[CURR_COL][CURR_ROW] = getFloat(w, h, "float scaled to " + (Math.round(scale * 10000)) / 100 + "%");
                    
                    if (CURR_ROW > 0) { // blank out any lines above
                        for (i = CURR_COL + 1; i < CURR_COL + span; i++) {
                            COLUMNS[i][CURR_ROW - 1] = "<!-- foo -->";
                            console.log('foo');
                        }
                    }

                } else if (FLOAT_TYPE === FLOAT_TYPES.MSWORD) {

                    // check if it'll actually fit in the column, and if not, move to a new one
                    // if (curr_y + h > (window.innerHeight - BOT_MARGIN)) {
                        // curr_y = TOP_MARGIN;
                        // curr_x += colsep;
                    // }

                    // getFloat(w, h, "FLOAT");
                    // curr_y += h;

                } else {
                        // Don't output anything.
                        // console.log(FLOAT_TYPE);
                }
            }
            //curr_y += PAR_SEP;

        }
    }

    $('#main').empty(); //clear old stuff

    dropdown = '<select id="dropdown" onchange="updateFloatType(this)">\n';
    for (name in FLOAT_TYPES) {
        dropdown += '<option value="' + FLOAT_TYPES[name] + '"' + (FLOAT_TYPE === FLOAT_TYPES[name] ? " selected" : "") + '>' + name + '</option>';
    }
    dropdown += "</select>\n";

    $('#main').append('<div class="title" style="position:absolute; width: ' + 1000 + 'px; height: ' + (PS_PX + 5) + 'px; left: ' + ((window.innerWidth - 1000) / 2) + 'px; top: ' + 3 + 'px">' + dropdown + (FLOAT_TYPE === FLOAT_TYPES.NONE ? "none" : (FLOAT_TYPE === FLOAT_TYPES.DUMB ? "dumb" : (FLOAT_TYPE === FLOAT_TYPES.MSWORD ? "MS Word" : (FLOAT_TYPE === FLOAT_TYPES.SIMPLEQUEUE ? "simple queue" : "unknown (" + FLOAT_TYPE + ")")))) + ", " + numCols + " cols, badness: " + Math.round(badness[min_badness_index] * 100) / 100 + ' <label><input id="multi" onclick="toggleMulti();" type="checkbox" ' + (MULTI_COLUMN ? 'checked' : '') + ' > multicolumn?</label></div>');

    // Now we need to output the contents of the COLUMNS array
    for (p = 0; p < COLUMNS.length; p++) {
        //console.log("p is " + p);
        if (!Array.isArray(COLUMNS[p])) {
            continue;
        }

        for (l = 0; l < COLUMNS[p].length; l++) {
            //console.log("\tl is " + l);
            if (COLUMNS[p][l] === undefined || COLUMNS[p][l] === "") {
                continue;
            }

            outputDiv("container", (colsep - galleywidth) / 2 + p * colsep, TOP_MARGIN + l * VS_PX, COLUMNS[p][l]);
        }
    }
    //console.log(COLUMNS);

}

function updateFloatType(dd) {
    FLOAT_TYPE = parseInt(dd.options[dd.selectedIndex].value);
    doLayout();
}

function toggleMulti() {
    MULTI_COLUMN = !MULTI_COLUMN;
    $('#multi').prop('checked', MULTI_COLUMN);
    doLayout();
}

var RESIZETIMER;
$(window).resize(function() {
    clearTimeout(RESIZETIMER);
    RESIZETIMER = setTimeout(doLayout, 200);
    doLayout();
});

$(document).ready(function() {
    var i, j, size_left, gw;
    // Here's one I made earlier
    PARAGRAPHS = [6408, new Floatable(2 * PX_PER_IN, 2 * PX_PER_IN), 10944, 8880, 8592, new Floatable(4 * PX_PER_IN, 2 * PX_PER_IN), new Floatable(2 * PX_PER_IN, 4 * PX_PER_IN), 3936, 7752, 10440, 7200, 11112, 10776, 12216, 7320, new Floatable(2 * PX_PER_IN, 1.5 * PX_PER_IN), 12504, 5904, 11496, 6144, 10944, 6216, 8640, 6336, 6312, 9888, 13176, 7176, 11712, 13104, 10752, 5880, 7320, 6792];

    // Now populate the widths array
    for (i = 5; i <= 30; i += 5) {
        GALLEY_WIDTHS.push(PS_PX * i);
    }

    // Create the paragraph tree: one element per paragraph-level item
    PARAGRAPH_TREE = [];

    // Now fill the PARAGRAPH_TREE array

    //For each paragraph-level item...
    for (i = 0; i < PARAGRAPHS.length; i++) {

        if (typeof PARAGRAPHS[i] === 'number') {
            // ...if it's a normal paragraph, add an array with one element per galley width
            PARAGRAPH_TREE.push([]);

            //Now populate the array
            for (j = 0; j < GALLEY_WIDTHS.length; j++) {

                size_left = PARAGRAPHS[i];
                gw = GALLEY_WIDTHS[j];
                PARAGRAPH_TREE[i].push([]);

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
