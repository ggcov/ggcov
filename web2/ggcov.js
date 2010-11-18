
// function form_get_url(form)
// {
//     var url = form.action;
//     var sep = "?";
//     if (url == "")
// 	url = form.baseURI.replace(/\?.*/, "");
//     for (var i = 0; i < form.elements.length; i++)
//     {
// 	if (form.elements[i].name != "")
// 	{
// 	    url += sep + encodeURIComponent(form.elements[i].name) + "=" + encodeURIComponent(form.elements[i].value);
// 	    sep = "&";
// 	}
//     }
//     return url;
// }
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
		    return kv.substr(j+1);
	    }
	}
	return null;
    },
    on_query_error: function(req, url)
    {
	ggcov.maindiv.innerHTML = "<p>Error loading results: url = " + url + " status = " + req.statusText + " response = " + req.responseText + "</p>";
    },
    query: function(query, vars, onreply, onerror)
    {
	if (onerror == null)
	    onerror = ggcov.on_query_error;
	var req = new XMLHttpRequest();
	var url = ggcov.cgi_url(query, vars);
	req.open('GET', url, true);
	req.onreadystatechange = function(ev)
	{
	    if (req.readyState == 4)
	    {
		if (req.status == 200)
		{
		    var results = req.responseText;
		    if (req.getResponseHeader("Content-Type") == "application/json")
			results = eval("(" + results + ")");
		    onreply(results);
		}
		else
		    onerror(req, url);
	    }
	};
	req.send(null);
    },
    show_source: function(filename)
    {
	ggcov.settitle("Source - " + filename);
	ggcov.maindiv.innerHTML = "Loading...";
	ggcov.query("annotate", { f: filename }, function(results)
	{
	    var table = "<table>\n";
	    var countstr;
	    var lineno = 1;
	    for (var i = 0; i < results.length; i++)
	    {
		table += "<tr class=\"status" + results[i].s + "f\">";
		table += "<td align=\"right\">" + lineno + "</td>";
		if (results[i].s == "CO" || results[i].s == "PA")
		    countstr = "" + results[i].c;
		else if (results[i].s == "UC")
		    countstr = "######";
		else
		    countstr = "";
		table += "<td align=\"right\">" + countstr + "</td>";
		table += "<td class=\"ggcovtext\">" + htmlEntities(results[i].t) + "</td>";
		table += "</tr>";
		lineno++;
	    }
	    table += "</table>";
	    ggcov.maindiv.innerHTML = table;
	});
    },
    on_choose_file: function(ev)
    {
	var file = ggcov.url_var(ev.target.href, "f");
	ggcov.show_source(file);
    },
    show_files: function()
    {
	ggcov.settitle("Files - " + htmlEntities(ggcov.project));
	ggcov.maindiv.innerHTML = "Loading...";
	ggcov.query("listfiles", { }, function(results)
	{
	    var html = "";
	    html += "<p>Please select a file...</p>\n";
	    html += "<table>\n";
	    for (var i = 0; i < results.length; i++)
	    {
		var url = ggcov.cgi_url(null, { f: results[i].n });
		var label = htmlEntities(results[i].n);
		var onclick = "ggcov.on_choose_file(event); return false;";

		html += "<tr>";
		html += "<td><a href=\"" + url + "\" onclick=\"" + onclick + "\">" + label + "</a></td>";
		html += "</tr>";
	    }
	    html += "</table>";
	    ggcov.maindiv.innerHTML = html;
	});
    },
    on_choose_project: function(ev)
    {
	ggcov.project = ggcov.url_var(ev.target.href, "p");
	ggcov.show_files();
    },
    show_projects: function()
    {
	ggcov.settitle("Project Browser");
	ggcov.maindiv.innerHTML = "Loading...";
	ggcov.query("listprojects", { }, function(results)
	{
	    var html = "";
	    html += "<p>Please select a project...</p>\n";
	    html += "<table>\n";
	    for (var i = 0; i < results.length; i++)
	    {
		var url = ggcov.cgi_url(null, { p: results[i] });
		var label = htmlEntities(results[i]);
		var onclick = "ggcov.on_choose_project(event); return false;";

		html += "<tr>";
		html += "<td><a href=\"" + url + "\" onclick=\"" + onclick + "\">" + label + "</a></td>";
		html += "</tr>";
	    }
	    html += "</table>";
	    ggcov.maindiv.innerHTML = html;
	});
    },
    init: function(maindiv, settitle)
    {
	ggcov.maindiv = maindiv;
	ggcov.settitle = settitle;

	if (ggcov.project == null)
	    ggcov.show_projects();
    }
};
