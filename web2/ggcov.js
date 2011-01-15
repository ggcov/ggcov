function htmlEntities(str)
{
    return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
var ggcov = {
    maindiv: null,
    settitle: null,
    project: {
	name: null,
	files: null,
	reports: null,
	summary: null
    },
    projects: {
	curr_page: 0,
	page_size: 10,
	data: null
    },
    status_short_names: [
	"CO",   /* COVERED */
	"PA",   /* PARTCOVERED */
	"UC",   /* UNCOVERED */
	"UI",   /* UNINSTRUMENTED */
	"SU"    /* SUPPRESSED */
    ],
    status_long_names: [
	"COVERED",
	"PARTCOVERED",
	"UNCOVERED",
	"UNINSTRUMENTED",
	"SUPPRESSED"
    ],
    //
    // Build and return a backend query URL
    //
    cgi_url: function(query, vars)
    {
	var url = "ggcov.cgi";
	if (ggcov.project.name != null)
	    vars.p = ggcov.project.name;
	if (query != null)
	    vars.q = query;
	var sep = "?";
	for (name in vars)
	{
	    url += sep + name + "=" + encodeURIComponent(vars[name]);
	    sep = "&";
	}
	return url;
    },
    //
    // Extract, and return, from the given URL's query string,
    // the value of the given variable.
    //
    url_var: function(url, name)
    {
	url = url.replace(/^[^?]*\?/, '');
	var a = url.split('&');
	for (var i = 0; i < a.length ; i++)
	{
	    var kv = a[i];
	    var j = kv.indexOf('=');
	    if (j > 0)
	    {
		var k = kv.substr(0, j);
		if (k == name)
		    return decodeURIComponent(kv.substr(j+1));
	    }
	}
	return null;
    },
    _statusbar: function(vals)
    {
	var d = vals.CO + vals.PA + vals.UC;

	if (d == 0)
	    return null;

	var W = 80;
	var H = 20;
	var names = [ "CO", "PA", "UC" ];
	var x = 0;

	var html = "<div style=\"position:relative;top:0px;width:" +
		    W + "px;height:" + H + "px;\">";
	for (var i = 0 ; i < names.length ; i++) {
	    var n = names[i];
	    var w = parseInt(W * vals[n] / d);
	    html += "<div style=\"position:absolute;left:" + x + "px;top:0px;height:" + H +
		    "px;width:" + w + "px;\" class=\"status" + n + "b\">&nbsp;</div>";
	    x += w;
	}
	html += "</div>";

	return html;
    },
    _update_statusbar: function(q, vals)
    {
	var html = ggcov._statusbar(vals);

	if (html == null)
	{
	    q.css('visibility', 'hidden');
	}
	else
	{
	    q.empty();
	    q.html(html);
	    q.css('visibility', 'visible');
	}
    },
    _update_summary_aux: function(frac_q, pc_q, vals)
    {
	var frac_str, pc_str;
	var n = vals.CO + vals.PA;
	var d = n + vals.UC;

	if (d == 0)
	{
	    frac_str = '0/0';
	    pc_str = '';
	}
	else if (vals.PA == 0)
	{
	    frac_str = '' + n + '/' + d;
	    pc_str = '' + (100.0 * n / d).toFixed(1) + '%';
	}
	else
	{
	    frac_str = '' + vals.CO + '+' + vals.PA + '/' + d;
	    pc_str = '' + (100.0 * vals.CO / d).toFixed(1) + '+' + (100.0 * vals.PA / d).toFixed(1) + '%';
	}
	frac_q.html(frac_str);
	pc_q.html(pc_str);
    },
    _project_show_summary: function()
    {
	var table = $('#ggcov #project #summary');
	var stats = ggcov.project.summary;
	ggcov._update_summary_aux($('#lines_frac', table),
				  $('#lines_pc', table),
				  stats.li);
	ggcov._update_summary_aux($('#functions_frac', table),
				  $('#functions_pc', table),
				  stats.fn);
	ggcov._update_summary_aux($('#calls_frac', table),
				  $('#calls_pc', table),
				  stats.ca);
	ggcov._update_summary_aux($('#branches_frac', table),
				  $('#branches_pc', table),
				  stats.br);
	ggcov._update_summary_aux($('#blocks_frac', table),
				  $('#blocks_pc', table),
				  stats.bl);
	ggcov._update_statusbar($('#lines_bar', table), stats.li);
	ggcov._update_statusbar($('#functions_bar', table), stats.fn);
	ggcov._update_statusbar($('#calls_bar', table), stats.ca);
	ggcov._update_statusbar($('#branches_bar', table), stats.br);
	ggcov._update_statusbar($('#blocks_bar', table), stats.bl);
    },
    _switch_page: function(pp)
    {
	$('#ggcov .page').css('display', 'none');
	$('#ggcov #' + pp).css('display', 'block');
    },
    show_source_page: function(filename)
    {
	ggcov.settitle("Source - " + filename);
	ggcov._switch_page('loading');
	var pending = 2;
	$.getJSON(ggcov.cgi_url('annotate', { f: filename }), function(data)
	{
	    var tbody = $('#ggcov #source #list tbody');
	    tbody.empty();
	    var lineno = 1;
	    for (var i = 0; i < data.length; i++)
	    {
		var tr = "<tr class=\"status" + data[i].s + "f\" id=\"line" + lineno + "\">";
		tr += "<td align=\"right\"><a name=\"" + lineno + "\">" + lineno + "</td>";

		var countstr;
		if (data[i].s == "CO" || data[i].s == "PA")
		    countstr = "" + data[i].c;
		else if (data[i].s == "UC")
		    countstr = "######";
		else
		    countstr = "";
		tr += "<td align=\"right\">" + countstr + "</td>";

		tr += "<td class=\"ggcovtext\">" + htmlEntities(data[i].t) + "</td>";
		tr += "</tr>";
		tbody.append(tr);
		lineno++;
	    }
	    if (--pending == 0)
		ggcov._switch_page('source');
	});
	$.getJSON(ggcov.cgi_url('listfunctions', { f: filename }), function(data)
	{
	    var select = $('#ggcov #source #functions select');
	    select.empty();
	    for (var i = 0; i < data.length; i++)
	    {
		var fl = data[i].fl.replace(/.*:/, '');
		var label = htmlEntities(data[i].n);
		var opt = "<option value=\"" + fl + "\">" + label + "</option>";
		select.append(opt);
	    }
	    select.change(function(ev)
	    {
		$('html,body').animate(
		    { scrollTop: $('#ggcov #source #list tbody #line' + ev.target.value).offset().top },
		    { duration: 'slow' });
	    });
	    if (--pending == 0)
		ggcov._switch_page('source');
	});
    },
    show_report_page: function(report, title)
    {
	ggcov.settitle("Report - " + title);
	ggcov._switch_page('loading');
	$.get(ggcov.cgi_url('report', { r: report }), function(data)
	{
	    $('#ggcov #report pre').html(data);
	    ggcov._switch_page('report');
	},'text');
    },
    show_diagram_page: function(diagram, title)
    {
	ggcov.settitle("Diagram - " + title);
// 	ggcov._switch_page('loading');
	$('#ggcov #diagram img').attr('src', ggcov.cgi_url('diagram', { d: diagram }));
	ggcov._switch_page('diagram');
    },
    _project_show_files: function()
    {
	var table = $('#ggcov #project #file_list');
	var files = ggcov.project.files;
	var map = [];
	var row = 1;
	var root = { file: null, children: [], row: row };
	row++;
	table.empty();

	var tr = "<tr id=\"node-1\">";
	tr += "<td align=\"left\">";
	tr += "<img valign=\"center\" src=\"fopen.gif\">&nbsp;" + htmlEntities(ggcov.project.name)
	tr += "</td>";
	tr += "<td align=\"right\"></td>";
	tr += "</tr>";
	table.append(tr);

	for (var i = 0; i < files.length; i++)
	{
	    var path = files[i].n.split('/');
	    var node = root;
	    for (var j = 0 ; j < path.length ; j++)
	    {
		var k = node.children.length-1;
		while (k >= 0 && node.children[k].name != path[j])
		    k--;
		if (k < 0) {
		    k = node.children.push({
			    name: path[j],
			    file: files[i],
			    children: [],
			    row: row }) - 1;

		    var statusbar = "";
		    var label = htmlEntities(path[j]);
		    var icon = "fopen.gif";
		    if (j == path.length-1)
		    {
			var url = ggcov.cgi_url(null, { f: files[i].n });
			label = "<a href=\"" + url + "\">" + label + "</a>";
			statusbar = ggcov._statusbar(files[i].s.li);
			icon = "new.gif";
		    }
		    var tr = "<tr id=\"node-" + row + "\" class=\"child-of-node-" + node.row + "\">";
		    tr += "<td align=\"left\">";
		    tr += "<img valign=\"center\" src=\"" + icon + "\">&nbsp;" + label;
		    tr += "</td>";
		    tr += "<td align=\"right\">" + statusbar + "</td>";
		    tr += "</tr>";
		    table.append(tr);
		    row++;
		}
		node = node.children[k];
	    }
	}
	$('a', table).click(function(ev)
	{
	    var file = ggcov.url_var(ev.target.href, "f");
	    ggcov.show_source_page(file);
	    return false;
	});
	table.treeTable({
	    initialState: "expanded"
	});
    },
    _project_show_reports: function()
    {
	var tbody = $('#ggcov #project #report_list tbody');
	var data = ggcov.project.reports;
	tbody.empty();
	for (var i = 0; i < data.length; i++)
	{
	    var url = ggcov.cgi_url(null, { n: data[i].n, t: data[i].t });
	    var label = htmlEntities(data[i].l).replace(/ /g, '&nbsp;');

	    var tr = "<tr>";
	    tr += "<td><a class=\"" + data[i].c + "\" href=\"" + url + "\">" + label + "</a></td>";
	    tr += "</tr>";
	    tbody.append(tr);
	}
	$('a.report', tbody).click(function(ev)
	{
	    var name = ggcov.url_var(ev.target.href, "n");
	    ggcov.show_report_page(name, $(ev.target).html());
	    return false;
	});
	$('a.diagram', tbody).click(function(ev)
	{
	    var name = ggcov.url_var(ev.target.href, "n");
	    ggcov.show_diagram_page(name, $(ev.target).html());
	    return false;
	});
    },
    show_project_page: function()
    {
	ggcov.settitle("Project - " + htmlEntities(ggcov.project.name));
	ggcov._switch_page('loading');

	var pending = 3;

	var got_files = function(data)
	{
	    if (data != null)
		ggcov.project.files = data;
	    ggcov._project_show_files();
	    if (--pending == 0)
		ggcov._switch_page('project');
	};
	var got_reports = function(data)
	{
	    if (data != null)
		ggcov.project.reports = data;
	    ggcov._project_show_reports();
	    if (--pending == 0)
		ggcov._switch_page('project');
	};
	var got_summary = function(data)
	{
	    if (data != null)
		ggcov.project.summary = data;
	    ggcov._project_show_summary();
	    if (--pending == 0)
		ggcov._switch_page('project');
	};

	if (ggcov.project.files != null)
	    got_files(null);
	else
	    $.getJSON(ggcov.cgi_url('listfiles', { }), got_files);

	if (ggcov.project.reports != null)
	    got_reports(null);
	else
	    $.getJSON(ggcov.cgi_url('listreports', { }), got_reports);

	if (ggcov.project.summary != null)
	    got_summary(null);
	else
	    $.getJSON(ggcov.cgi_url('summary', { s: 'overall' }), got_summary);
    },
    _update_project_list: function(page)
    {
	var npages = parseInt((ggcov.projects.data.length + ggcov.projects.page_size -1 )
			    / ggcov.projects.page_size);
	if (page < 0)
	    page = 0;
	else if (page >= npages)
	    page = npages-1;
	ggcov.projects.curr_page = page;

	var tbody = $('#ggcov #project_list #list tbody');
	tbody.empty();
	for (var pi = 0 ; pi < ggcov.projects.page_size ; pi++)
	{
	    var i = page * ggcov.projects.page_size + pi;
	    if (i >= ggcov.projects.data.length)
		break;
	    var proj = ggcov.projects.data[i];
	    var url = ggcov.cgi_url(null, { p: proj.n });
	    var mtime = new Date(1000*proj.m);
	    var rclass = (pi % 2 ? 'odd' : 'even');

	    var row = "<tr><td colspan=\"3\"><table width=\"100%\" cellspacing=\"0\"><tr class=\"" + rclass + "\">";
	    row += "<td align=\"left\" width=\"1\" rowspan=\"2\">";
	    if (0)  // client authorised to delete or edit
		row += "DEL<br>PROP";
	    row += "</td>";
	    row += "<td id=\"name\" align=\"left\">";
	    row += "<a href=\"" + url + "\">" + htmlEntities(proj.n) + "</a>";
	    row += "</td>";
	    row += "<td id=\"date\" width=\"1\" align=\"right\">";
	    row += mtime.toLocaleDateString().replace(/ /g, '&nbsp;');
	    row += "</td>";
	    row += "</tr><tr class=\"" + rclass + "\">";
	    row += "<td id=\"description\" colspan=\"2\">";
	    row += "<p>" + htmlEntities(proj.d) + "</p>";
	    row += "</td>";
	    row += "</tr></table></td></tr>";

	    tbody.append(row);
	}
	$('a', tbody).click(function(ev)
	{
	    ggcov.project.name = ggcov.url_var(ev.target.href, "p");
	    ggcov.project.summary = null;
	    ggcov.project.files = null;
	    ggcov.show_project_page();
	    return false;
	});

	var first = page * ggcov.projects.page_size + 1;
	var last = first + ggcov.projects.page_size - 1;
	if (last > ggcov.projects.data.length)
	    last = ggcov.projects.data.length;
	var nav = $('#ggcov #project_list .nav');
	$('#loc', nav).html(first + ".." + last + " of " + ggcov.projects.data.length);
	$('#first, #prev', nav).css('visibility', (page == 0 ? 'hidden' : 'visible'));
	$('#next, #last', nav).css('visibility', (page == npages-1 ? 'hidden' : 'visible'));
    },
    show_project_list_page: function()
    {
	ggcov.settitle("Please Select A Project");
	ggcov._switch_page('loading');
	$.getJSON(ggcov.cgi_url('listprojects', { }), function(data)
	{
	    ggcov.projects.data = data;
	    ggcov._update_project_list(0);
	    ggcov._switch_page('project_list');
	});
	$('#ggcov #project_list .nav a').click(function(ev)
	{
	    var a = ev.target.id;
	    var page = ggcov.projects.curr_page;
	    if (a == "first") {
		page = 0;
	    } else if (a == "prev") {
		page--;
	    } else if (a == "next") {
		page++;
	    } else if (a == "last") {
		page = 10000;
	    }
	    ggcov._update_project_list(page);
	    return false;
	});
    },
    init: function(maindiv, settitle)
    {
	ggcov.maindiv = maindiv;
	ggcov.settitle = settitle;

	$('#error').ajaxError(function(ev, xhr, settings)
	{
	    $(this).html('<p>AJAX call failed to ' + settings.url +
			 ' failed with: ' + xhr.responseText + '</p>');
	});
	$('#ggcov a.project').click(function(ev)
	{
	    ggcov.show_project_page();
	    return false;
	});

	if (ggcov.project.name == null)
	    ggcov.show_project_list_page();
	else
	    ggcov.show_project_page();
    }
};
