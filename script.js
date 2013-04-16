var PT_PER_IN = 72;
var PX_PER_PT = 2;
var PX_PER_IN = PX_PER_PT * PT_PER_IN;
var PS = 12;
var VS = 14;
var PS_PX = PS * PX_PER_PT;
var VS_PX = VS * PX_PER_PT;
var TOP_MARGIN = 1.5 * VS_PX;
var BOT_MARGIN = 2 * TOP_MARGIN;
var PAR_SEP = VS_PX;
var MIN_GUTTER = 12 * PX_PER_PT;
var GALLEY_WIDTHS = [];
var PARAGRAPHS = [];
var PARAGRAPH_TREE;
var BODYHEIGHT;
var FLOAT_QUEUE;
var LINE_QUEUE;
var CURR_P;

var FLOAT_TYPES = Object.freeze({
    NONE: 0,
    DUMB: 1,
    MSWORD: 2,
    SIMPLEQUEUE: 3
});

var FLOAT_TYPE;

FLOAT_TYPE = FLOAT_TYPES.SIMPLEQUEUE;

function Floatable(width, height) {
    this.width = width;
    this.height = height;
}

function outputDiv(cssclass, width, height, x, y, text) {
    $('#main').append('<div class="' + cssclass + '" style="width: ' + width + 'px; height: ' + height + 'px; position: absolute; top: ' + y + 'px; left: ' + x + 'px; font-size: ' + PS_PX + 'px">' + text + '</div>\n');
}

function outputLine(width, height, x, y, text) {
    outputDiv("line", width, height, x, y, text);
}

function outputFloat(width, height, x, y) {
    outputDiv("floatable", width, height, x, y, "figure");
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
    var i, nc, ex_ws, rq_ws, galleywidth, colsep, curr_x, curr_y, dropdown, pageWidth = window.innerWidth, numCols, badness = [], min_badness_index = 0, name, p, l, w, h, scale;

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
    curr_x = (colsep - galleywidth) / 2;
    curr_y = TOP_MARGIN;

    // Shove lines onto screen

    $('#main').empty(); //clear old stuff

    dropdown = '<select id="dropdown" onchange="updateFloatType(this)">\n';
    for (name in FLOAT_TYPES) {
        dropdown += '<option value="' + FLOAT_TYPES[name] + '"' + (FLOAT_TYPE === FLOAT_TYPES[name] ? " selected" : "") + '>' + name + '</option>';
    }
    dropdown += "</select>\n";

    outputDiv("title", 1000, PS_PX + 5, (window.innerWidth - 1000) / 2, 3, dropdown + (FLOAT_TYPE === FLOAT_TYPES.NONE ? "none" : (FLOAT_TYPE === FLOAT_TYPES.DUMB ? "dumb" : (FLOAT_TYPE === FLOAT_TYPES.MSWORD ? "MS Word" : (FLOAT_TYPE === FLOAT_TYPES.SIMPLEQUEUE ? "simple queue" : "unknown (" + FLOAT_TYPE + ")")))) + ", " + numCols + " cols, badness: " + Math.round(badness[min_badness_index] * 100) / 100);

    if (FLOAT_TYPE === FLOAT_TYPES.SIMPLEQUEUE) {
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
//            curr_y += PAR_SEP;
        }

    } else {

        for (p = 0; p < PARAGRAPH_TREE.length; p++) {
            if (curr_y > (window.innerHeight - BOT_MARGIN)) { //start new column
                curr_y = TOP_MARGIN;
                curr_x += colsep;
            }

            if (Array.isArray(PARAGRAPH_TREE[p])) {
                // We assume it's a normal paragraph
                for (l = 0; l < PARAGRAPH_TREE[p][min_badness_index].length; l++) {
                    if (curr_y > (window.innerHeight - BOT_MARGIN)) { //start new column
                        curr_y = TOP_MARGIN;
                        curr_x += colsep;
                    }

                    outputLine(PARAGRAPH_TREE[p][min_badness_index][l], PS_PX, curr_x, curr_y, 'W' + min_badness_index + ' P' + p + ' L' + l + ' Qwertyuiop Lorem ipsum dolor sit amet, consectetur adipiscing elit.');

                    curr_y += VS_PX;
                }
            } else {
                // It must be a Floatable object

                // Scale width to fit column (and height proportionately)
                w = PARAGRAPH_TREE[p].width;
                h = PARAGRAPH_TREE[p].height;

                scale = galleywidth / w;

                w *= scale;
                h *= scale;

                if (h > BODYHEIGHT) {
                    scale = BODYHEIGHT / h;
                    w *= scale;
                    h *= scale;
                }

                if (FLOAT_TYPE === FLOAT_TYPES.DUMB) {

                    // Just stick the figure out wherever
                    outputFloat(w, h, curr_x, curr_y);
                    curr_y += h;

                } else if (FLOAT_TYPE === FLOAT_TYPES.MSWORD) {

                    // check if it'll actually fit in the column, and if not, move to a new one
                    if (curr_y + h > (window.innerHeight - BOT_MARGIN)) {
                        curr_y = TOP_MARGIN;
                        curr_x += colsep;
                    }

                    outputFloat(w, h, curr_x, curr_y);
                    curr_y += h;

                } else {
                        // Don't output anything.
                        // console.log(FLOAT_TYPE);
                }
            }
            curr_y += PAR_SEP;

        }
    }
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
