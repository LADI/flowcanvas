"use strict";

var app;
var Group = { GRID: 0, TOP: 1, SIDE: 2 };

var GRID_W = 8;
var GRID_H = 8;

function log(msg) {
	if (typeof(console) === "object") {
		console.log(msg);
	}
}

function click_handler(log, group, x, y) {
	return function () { 
		log("Clicked group " + group + " @ " + x + "," + y);
		var req = new XMLHttpRequest();
		req.open("POST", "/update", true);
		var obj = {
			selector: "click",
			group: group,
			x: x,
			y: y
		};
		var body = JSON.stringify(obj);
		req.setRequestHeader("Content-type", "application/javascript");
		req.send(body);
	};
}

function refresh(immediate) {
	path = "/event";
	if (immediate) {
		path = "/update";
	}
	var req = new XMLHttpRequest();
	req.open("GET", path, true);
	req.onreadystatechange = function () {
		if (req.readyState === 4) {
			if (req.status === 200) {
				var vals = JSON.parse(req.responseText);
				for (var y = 0; y < GRID_H; ++y) {
					for (var x = 0; x < GRID_W; ++x) {
						app.grid.rows[y].cells[x].style.backgroundColor = vals[y][x];
					}
				}
			}

			if (req.status >= 100 && req.status < 400)
				refresh(false);
		}
	};
	req.send();
}

function init() {
	app = {
		ui:   document.getElementById("ui"),
		grid: document.createElement("table"),
		top:  document.createElement("table"),
		side: document.createElement("table")
	};

	var row1 = app.ui.insertRow(0);
	row1.setAttribute("class", "expand-width");

	var top_parent = row1.insertCell(0);
	top_parent.appendChild(app.top);

	row1.insertCell(1); // Empty top right cell
	
	var row2 = app.ui.insertRow(1);
	row2.setAttribute("class", "expand-height");

	var grid_parent = row2.insertCell(0);
	grid_parent.setAttribute("class", "expand");
	grid_parent.appendChild(app.grid);

	var side_parent = row2.insertCell(1);
	side_parent.appendChild(app.side);

	var x;
	var y;

	// Build grid (8x8)
	for (y = 0; y < GRID_H; ++y) {
		var row = app.grid.insertRow(y);
		
		// Grid buttons
		for (x = 0; x < GRID_W; ++x) {
			var grid_cell = row.insertCell(x);
			grid_cell.setAttribute("class", "gridButton");
			grid_cell.onclick = click_handler(log, Group.GRID, x, y);
		}
	}

	// Build top row (8x1)
	var top_row = app.top.insertRow(0);
	for (x = 0; x < GRID_W; ++x) {
		var top_cell = top_row.insertCell(x);
		top_cell.setAttribute("class", "topButton");
		top_cell.onclick = click_handler(log, Group.TOP, x, 0);
		top_cell.appendChild(document.createTextNode(x + 1));
	}

	// Build side (1x8)
	for (y = 0; y < GRID_H; ++y) {
		var side_row  = app.side.insertRow(y);
		var side_cell = side_row.insertCell(0);
		side_cell.setAttribute("class", "sideButton");
		side_cell.onclick = click_handler(log, Group.SIDE, 0, y);
		side_cell.appendChild(document.createTextNode(y + 1));
	}

	app.grid.setAttribute("class", "grid");
	app.top.setAttribute("class",  "row");
	app.side.setAttribute("class", "column");

	refresh(true);
}
