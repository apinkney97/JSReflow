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
var CURR_PAGE;
var PAGES;
var NUM_COLS;
var GALLEY_WIDTH;
var COL_SEP;

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

FLOAT_TYPE = FLOAT_TYPES.SIMPLEQUEUE;

function Floatable(width, height, content) {
    this.width = width;
    this.height = height;
	this.content = content;
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

function checkSpace(nlines, span) {
    var c, r;
    // console.log(nlines, span);
    if (nlines > NUM_ROWS && CURR_ROW === 0) { //if it's too big, put it at the top of a column
        return true;
    }

    if (CURR_ROW + nlines > NUM_ROWS) {
        // too far down
        // console.log("too big: need ", nlines)
        // console.log("rows: ", NUM_ROWS);
        // console.log("curr: ", CURR_ROW);
        return false;
    }

    // Now check that there is enough contiguous space in both directions
    for (r = CURR_ROW; r < CURR_ROW + nlines; r++) {
        for (c = CURR_COL; c < CURR_COL + span; c++) {
            if (!Array.isArray(COLUMNS[c])) {
                COLUMNS[c] = [];
            }
            if (COLUMNS[c][r] !== undefined) {
                //console.log("not undefined:", c, r);
                return false;
            }
        }
    }
    return true;
}

function placeFloat(w, h, nlines, span, text) {
    // Reserve space for the figure, and place it

    // How many lines do we need to reserve?
    // Let's say if it's less than n.5,  no need for an extra space

    var i, j;

    for (i = CURR_ROW; i < CURR_ROW + nlines; i++) {
        for (j = CURR_COL; j < CURR_COL + span; j++) {
            if (!Array.isArray(COLUMNS[j])) {
                COLUMNS[j] = [];
            }
            COLUMNS[j][i] = "";
        }
    }

    COLUMNS[CURR_COL][CURR_ROW] = getFloat(w, h, text);

    if (CURR_ROW > 0) { // blank out any lines above
        for (i = CURR_COL + 1; i < CURR_COL + span; i++) {
            COLUMNS[i][CURR_ROW - 1] = "";
        }
    }
}

function paginate(offset) {
    if (offset + CURR_PAGE > PAGES) {
        $('#next').attr("disabled", true);
    } else if (offset + CURR_PAGE < 1) {
        $('#prev').attr("disabled", true);
    } else {
        CURR_PAGE += offset;
        
        if (CURR_PAGE > 1) {
            $('#prev').attr("disabled", false);
        }
        if (CURR_PAGE < PAGES) {
            $('#next').attr("disabled", false);
        }
        
        if (CURR_PAGE >= PAGES) {
            $('#next').attr("disabled", true);
        } else if (CURR_PAGE <= 1) {
            $('#prev').attr("disabled", true);
        }
        
        
        $('#pageno').html(CURR_PAGE);
        
        $('#main').html("");
        
        // Now we need to output the contents of the COLUMNS array
		var start = (CURR_PAGE - 1) * NUM_COLS;
		var end = CURR_PAGE * NUM_COLS;
		
        for (p = start; p < COLUMNS.length && p < end; p++) {
            if (!Array.isArray(COLUMNS[p])) {
                continue;
            }

            for (l = 0; l < COLUMNS[p].length; l++) {
                if (COLUMNS[p][l] === undefined || COLUMNS[p][l] === "") {
                    continue;
                }

                outputDiv("container", ((COL_SEP - GALLEY_WIDTH) / 2 + (p - start) * COL_SEP), TOP_MARGIN + l * VS_PX, COLUMNS[p][l]);
            }
        }
        //console.log(COLUMNS);
    }
    
    
}

function doLayout() {
    var i, nc, ex_ws, rq_ws, dropdown, pageWidth,badness, min_badness_index, name, p, l, w, h, scale, span, nlines, currcol, currrow;

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
            NUM_COLS = nc;
        }
    }

    GALLEY_WIDTH = GALLEY_WIDTHS[min_badness_index];

    COL_SEP = Math.floor(pageWidth / NUM_COLS);

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

            // Have a look at the width and see if it could span any columns
            span = Math.round(w / GALLEY_WIDTH);

            if (span > NUM_COLS) {
                span = NUM_COLS;
            }

            if (span < 1 || !MULTI_COLUMN) {
                span = 1;
            }

            scale = (span * GALLEY_WIDTH + (span - 1) * (COL_SEP - GALLEY_WIDTH)) / w;

            w *= scale;
            h *= scale;

            // Is it too big to fit? If so scale down the height
            if (h > BODYHEIGHT) {
                scale = BODYHEIGHT / h;
                w *= scale;
                h *= scale;
            }

            nlines = Math.round(h / VS_PX) + 1;

            if (FLOAT_TYPE === FLOAT_TYPES.SIMPLEQUEUE) {
                // Don't actually need a queue in gridlayout
                nextPos();
                currcol = CURR_COL;
                currrow = CURR_ROW;

                // check if it'll actually fit in the column, and if not, move to a new one
                while (!checkSpace(nlines, span)) {
                    CURR_COL++;
                    CURR_ROW = 0;
                    nextPos();
                }

                placeFloat(w, h, nlines, span, "float scaled to " + (Math.round(scale * 10000)) / 100 + "%");

                CURR_COL = currcol;
                CURR_ROW = currrow;

            } else if (FLOAT_TYPE === FLOAT_TYPES.DUMB) {

                // Just stick the figure out wherever
                nextPos();

                placeFloat(w, h, nlines, span, "float scaled to " + (Math.round(scale * 10000)) / 100 + "%");

            } else if (FLOAT_TYPE === FLOAT_TYPES.MSWORD) {
                nextPos();
                // check if it'll actually fit in the column, and if not, move to a new one
                while (!checkSpace(nlines, span)) {
                    CURR_COL++;
                    CURR_ROW = 0;
                    nextPos();
                }

                placeFloat(w, h, nlines, span, "float scaled to " + (Math.round(scale * 10000)) / 100 + "%");

            } /*else {
                // Don't output anything.
                // console.log(FLOAT_TYPE);
            }*/
        }

    }
    console.log(PARAGRAPH_TREE);
    PAGES = Math.ceil(COLUMNS.length / NUM_COLS);
    CURR_PAGE = 1;

    $('#main').empty(); //clear old stuff

    dropdown = '<select id="dropdown" onchange="updateFloatType(this)">\n';
    for (name in FLOAT_TYPES) {
        if (FLOAT_TYPES.hasOwnProperty(name)) {
            dropdown += '<option value="' + FLOAT_TYPES[name] + '"' + (FLOAT_TYPE === FLOAT_TYPES[name] ? " selected" : "") + '>' + name + '</option>';
        }
    }
    dropdown += "</select>\n";

    $('#top').html('<div class="title" style="position:absolute; width: ' + 100 + '%; height: ' + (PS_PX + 5) + 'px; left: ' + 0 + 'px; top: ' + 3 + 'px">' + dropdown + (FLOAT_TYPE === FLOAT_TYPES.NONE ? "none" : (FLOAT_TYPE === FLOAT_TYPES.DUMB ? "dumb" : (FLOAT_TYPE === FLOAT_TYPES.MSWORD ? "MS Word" : (FLOAT_TYPE === FLOAT_TYPES.SIMPLEQUEUE ? "simple queue" : "unknown (" + FLOAT_TYPE + ")")))) + ", " + NUM_COLS + " cols, " + PAGES + " pages, badness: " + Math.round(badness[min_badness_index] * 100) / 100 + ' <label><input id="multi" onclick="toggleMulti();" type="checkbox" ' + (MULTI_COLUMN ? 'checked' : '') + ' > multicolumn?</label> <input type="button" id="prev" value="prev" onclick="paginate(-1)" disabled> <span id="pageno">' + CURR_PAGE + '</span> <input type="button" id="next" value="next" onclick="paginate(1)" ' + (PAGES < 2 ? 'disabled' : '') + '></div>');
    
    
    paginate(0);
    
    

}

function updateFloatType(dd) {
    FLOAT_TYPE = parseInt(dd.options[dd.selectedIndex].value, 10);
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
