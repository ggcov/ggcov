function htmlEntities(str)
{
    return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
var ggcov = {
    maindiv: null,
    settitle: null,
    project: null,
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
	if (ggcov.project != null)
	    vars.p = ggcov.project;
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
	    pc_str = '' + (100.0 * n / d) + '%';
	}
	else
	{
	    frac_str = '' + vals.CO + '+' + vals.PA + '/' + d;
	    pc_str = '' + (100.0 * vals.CO / d) + '+' + (100.0 * vals.PA / d) + '%';
	}
	frac_q.html(frac_str);
	pc_q.html(pc_str);
    },
    _update_summary: function(table, stats)
    {
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
	$.getJSON(ggcov.cgi_url('annotate', { f: filename }), function(data)
	{
	    var tbody = $('#ggcov #source #list tbody');
	    tbody.empty();
	    var lineno = 1;
	    for (var i = 0; i < data.length; i++)
	    {
		var tr = "<tr class=\"status" + data[i].s + "f\">";
		tr += "<td align=\"right\">" + lineno + "</td>";

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
    show_project_page: function()
    {
	ggcov.settitle("Project - " + htmlEntities(ggcov.project));
	ggcov._switch_page('loading');
	$.getJSON(ggcov.cgi_url('listfiles', { }), function(data)
	{
	    var tbody = $('#ggcov #project #file_list tbody');
	    tbody.empty();
	    for (var i = 0; i < data.length; i++)
	    {
		var url = ggcov.cgi_url(null, { f: data[i].n });
		var label = htmlEntities(data[i].n);

		var tr = "<tr>";
		tr += "<td><a href=\"" + url + "\">" + label + "</a></td>";
		tr += "</tr>";
		tbody.append(tr);
	    }
	    $('a', tbody).click(function(ev)
		{
		    var file = ggcov.url_var(ev.target.href, "f");
		    ggcov.show_source_page(file);
		    return false;
		});
	    ggcov._switch_page('project');
	});
	$.getJSON(ggcov.cgi_url('listreports', { }), function(data)
	{
	    var tbody = $('#ggcov #project #report_list tbody');
	    tbody.empty();
	    for (var i = 0; i < data.length; i++)
	    {
		var url = ggcov.cgi_url(null, { r: data[i].n });
		var label = htmlEntities(data[i].l);

		var tr = "<tr>";
		tr += "<td><a href=\"" + url + "\">" + label + "</a></td>";
		tr += "</tr>";
		tbody.append(tr);
	    }
	    $('a', tbody).click(function(ev)
		{
		    var report = ggcov.url_var(ev.target.href, "r");
		    ggcov.show_report_page(report, $(ev.target).html());
		    return false;
		});
	});
	$.getJSON(ggcov.cgi_url('summary', { s: 'overall' }), function(data)
	{
	    ggcov._update_summary($('#ggcov #project #summary'), data);
	});
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
		ggcov.project = ggcov.url_var(ev.target.href, "p");
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
	ggcov.settitle("Project Browser");
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
	$('#ggcov #project_a').click(function(ev)
	{
	    ggcov.show_project_page();
	    return false;
	});

	if (ggcov.project == null)
	    ggcov.show_project_list_page();
	else
	    ggcov.show_project_page();
    }
};
