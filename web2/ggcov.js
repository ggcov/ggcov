
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
    project: "hacky",
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
    cgi_url: function(query, vars)
    {
	var url = "ggcov.cgi";
	if (ggcov.project != null)
	    vars.p = ggcov.project;
	vars.q = query;
	var sep = "?";
	for (name in vars)
	{
	    url += sep + name + "=" + encodeURIComponent(vars[name]);
	    sep = "&";
	}
	return url;
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
		    onreply(req.responseText);
		else
		    onerror(req, url);
	    }
	};
	req.send(null);
    },
    show_source: function(filename)
    {
	ggcov.settitle("Source " + filename);
	ggcov.maindiv.innerHTML = "Loading...";
	ggcov.query("annotate", { f: filename }, function(response)
	{
// 	    ggcov.maindiv.innerHTML = "<p>Response was:</p><pre>" + response + "</pre>";
	    var results = eval("(" + response + ")");
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
    init: function(maindiv, settitle)
    {
	ggcov.maindiv = maindiv;
	ggcov.settitle = settitle;


// 	var txt = "";
// 	for (i = 0 ; i < ggcov.status_short_names.length ; i++)
// 	{
// 	    var sn = ggcov.status_short_names[i];
// 	    var ln = ggcov.status_long_names[i];
// 	    txt += "<p>";
// 	    txt += "<span class=\"status" + sn + "f\">" + ln + "</span>";
// 	    txt += "<span class=\"status" + sn + "b\">" + ln + "</span>";
// 	    txt += "</p>";
// 	}
// 	maindiv.innerHTML = txt;

	ggcov.show_source("test001/foo.c");
    }
};
