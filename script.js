var num_pars = 30;

var pt_per_px = 0.5;

var ps = 12;
var vs = 14;

var ps_px = ps / pt_per_px;
var vs_px = vs / pt_per_px;

var topmargin = 1.5 * vs_px;
var botmargin = 2 * topmargin;
var parsep = vs_px;

var mingutter = 12 / pt_per_px;
var galleywidths = [];
var paragraphs = [];
var partree;
//debugger;

$(document).ready(function(){
	// Create some paragraphs of text of varying lengths
	for (i = 0; i < num_pars; i++) {
		paragraphs.push(nrand());
	}
	
	// Now populate the widths array
	for (i = 5; i <= 30; i += 5) {
		galleywidths.push(ps_px * i);
	}
	
	partree = Array(paragraphs.length);
	
	// Now fill the partree array
	
	//For each paragraph
	for (i = 0; i < paragraphs.length; i++) {
		//for each galley width
		// Add an array of the right length
		partree[i] = Array(galleywidths.length);
		for (j = 0; j < galleywidths.length; j++) {
			// split the paragraph into bits of the right length, and add them to the right place in the partree
			size_left = paragraphs[i];
			gw = galleywidths[j];
			partree[i][j] = [];
			while (size_left > 0) {
				if (size_left < gw)
					gw = size_left;
				partree[i][j].push(gw);
				//$('#main').append('<div class="bgblue" style="width: '+gw+'em; height: 12pt"></div>');
				size_left -= gw;
			}
		}
	}
	
	doLayout();
	
});


function doLayout() {
	//console.info("Window size is ("+window.innerWidth+", "+window.innerHeight+")");
	
	// Choose display width
	var pageWidth = window.innerWidth;
	var numCols;
	var badness = Array(galleywidths.length);
	var min_badness_index = 0;
	
	for (i = 0; i < galleywidths.length; i++) {
		var nc = Math.floor(pageWidth / (galleywidths[i] + mingutter));
		ex_ws = pageWidth % ((galleywidths[i] + mingutter));
		rq_ws = nc * mingutter;
		//console.info("num cols:", nc);
		//console.info("ex ws:", ex_ws);
		//console.info("rq ws:", rq_ws);
		badness[i] = ((ex_ws /*+ rq_ws*/) + 100) * Math.sqrt(nc);
		if (nc == 0) badness[i] = 1000 * i;
		//console.info("badness["+i+"] =",badness[i]);
		if (badness[i] <= badness[min_badness_index]) {
			min_badness_index = i;
			numCols = nc;
		}
	}
	
	var colsep = Math.floor(pageWidth / numCols);
	var curr_x = (colsep - galleywidths[min_badness_index]) / 2;
	var curr_y = topmargin;
	
	
	
	// Shove lines onto screen
	
	$('#main').empty(); //clear old stuff
	
	for (p = 0; p < partree.length; p++) {
		for (l = 0; l < partree[p][min_badness_index].length; l++) {
			if (curr_y > (window.innerHeight - botmargin)) { //start new column
				curr_y = topmargin;
				curr_x += colsep;
			}
			width = partree[p][min_badness_index][l];
			$('#main').append('<div class="bgblue" style="width: ' + width + 'px; height: ' + ps_px + 'px; position: absolute; top: ' + curr_y + 'px; left: ' + curr_x + 'px; font-size: ' + ps_px + 'px">W' + min_badness_index + ' P' + p + ' L' + l + ' Qwertyuiop Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel enim vitae mauris vestibulum egestas.Suspendisse potenti. Pellentesque leo nunc, lobortis vitae gravida vel, congue at nulla.</div>');
			curr_y += vs_px;
		}
		curr_y += parsep;
	}
	
}


var resizeTimer;
$(window).resize(function(){
	clearTimeout(resizeTimer);
	resizeTimer = setTimeout(doLayout, 200);
	doLayout();
});

function nrand() { // thanks to http://www.protonfish.com/random.shtml
	r = 2 * Math.random() - 1;
	r += 2 * Math.random() - 1;
	r += 2 * Math.random() - 1;
	mean = 350;
	sd = 100;
	return ps_px * Math.floor(sd * r + mean);
}

function generatePar(width) {
	size = nrand();
	
	for (j=0; size > 0; j++) {
		if (size < width)
			width = size;
		$('#main').append('<div class="bgblue" style="width: '+width+'em; height: 12pt"></div>');
		size -= width;
	}
}
