function htmlEntities(str)
{
    return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
var ggcov = {
    maindiv: null,
    settitle: null,
    project: null,
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
    _switch_page: function(pp)
    {
	$('#ggcov div').css('display', 'none');
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
	    $('#ggcov #source #project_a').click(function(ev)
		{
		    ggcov.show_project_page();
		    return false;
		});
	    ggcov._switch_page('source');
	});
    },
    show_report_page: function(report)
    {
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
		    ggcov.show_report_page(report);
		    return false;
		});
	});
    },
    show_project_list_page: function()
    {
	ggcov.settitle("Project Browser");
	ggcov._switch_page('loading');
	$.getJSON(ggcov.cgi_url('listprojects', { }), function(data)
	{
	    var tbody = $('#ggcov #project_list #list tbody');
	    tbody.empty();
	    for (var i = 0; i < data.length; i++)
	    {
		var url = ggcov.cgi_url(null, { p: data[i].n });
		var mtime = new Date(1000*data[i].m);

		var tr = "<tr>";
		tr += "<td><a href=\"" + url + "\">" + htmlEntities(data[i].n) + "</a></td>";
		tr += "<td>" + mtime.toLocaleString() + "</td>\n";
		tr += "<td>" + htmlEntities(data[i].d) + "</td>\n";
		tr += "</tr>";
		tbody.append(tr);
	    }
	    $('a', tbody).click(function(ev)
		{
		    ggcov.project = ggcov.url_var(ev.target.href, "p");
		    ggcov.show_project_page();
		    return false;
		});
	    ggcov._switch_page('project_list');
	});
    },
    init: function(maindiv, settitle)
    {
	ggcov.maindiv = maindiv;
	ggcov.settitle = settitle;

	if (ggcov.project == null)
	    ggcov.show_project_list_page();
	else
	    ggcov.show_project_page();
    }
};
