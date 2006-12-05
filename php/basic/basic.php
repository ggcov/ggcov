<?php
//
// ggcov - A GTK frontend for exploring gcov coverage data
// Copyright (c) 2005 Greg Banks <gnb@alphalink.com.au>
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// $Id: basic.php,v 1.11 2006-12-04 13:02:24 gnb Exp $
//

require_once 'ggcov/lib/cov.php';

function basic_test_dir()
{
    static $test_dir = null;

    if ($test_dir === null)
    {
	$test_dir = getenv('GGCOV_TEST_DIR');
	if (!$test_dir)
	    $test_dir = '/var/ggcov/tests';
    }
    return $test_dir;
}

function basic_valid_test($t)
{
    return (preg_match('/^[a-zA-Z0-9][a-zA-Z0-9_.-]*$/', $t) &&
	    cov::database_exists(basic_test_dir() . '/' . $t));
}

function basic_test()
{
    static $test = null;

    if ($test === null)
    {
	if (array_key_exists('test', $_GET))
	{
	    $test = $_GET['test'];
	    if (!basic_valid_test($test))
		cov_callbacks::fatal("Invalid test");
	}
	if ($test === null)
	{
	    $t = getenv('GGCOV_DEFAULT_TEST');
	    if ($t && basic_valid_test($t))
		$test = $t;
	}
	if ($test === null && file_exists(basic_test_dir()))
	{
	    $d = opendir(basic_test_dir());
	    if ($d)
	    {
		while (($test = readdir($d)) && !basic_valid_test($test))
		    ;
		closedir($d);
	    }
	}
    }

    return $test;
}

function basic_list_tests()
{
    $list = array();
    if (file_exists(basic_test_dir()) && $d = opendir(basic_test_dir()))
    {
	while ($test = readdir($d))
	{
	    if (basic_valid_test($test))
		$list[] = $test;
	}
	closedir($d);
	sort($list);
    }
    return $list;
}

class basic_cov_callbacks extends cov_callbacks
{
    function add_state($query)
    {
	$test = basic_test();
	if ($test)
	    $query['test'] = $test;
	return $query;
    }
}

function basic_cov()
{
    $cov = new cov(new basic_cov_callbacks);
    $test = basic_test();
    if (!$test)
    {
	// No default test, so redirect to tests.php which will at
	// least tell the user what needs to be done to add a test.
	$cov->credirect('tests.php');
    }
    $cov->attach(basic_test_dir() . '/' . $test);
    return $cov;
}

function basic_url($base)
{
    $test = basic_test();
    if ($test)
    {
	$base .= (strchr($base,'?') == null ? '?' : '&');
	$base .= 'test=' . urlencode($test);
    }
    return htmlentities($base);
}


function basic_header($title)
{
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
  <title><?php echo $title; ?></title>
  <link rel="stylesheet" type="text/css" href="basic.css">
  <link rel="SHORTCUT ICON" href="favicon.ico">
</head>
<body>
  <table border="0" cellpadding="5" cellspacing="0" width="100%">
    <tr>
      <td colspan="3" class="nav_header"><?php echo $title; ?></td>
    </tr>
    <tr>
      <td class="nav_menu" valign="top" width="10" nowrap>
	<img width="64" height="64" src="ggcov64.gif" alt="ggcov"><br><br>
	<a href="<?php echo 'tests.php'; ?>">Tests</a><br>
	<a href="<?php echo basic_url('summary.php'); ?>">Summary</a><br>
	<a href="<?php echo basic_url('reports.php'); ?>">Reports</a><br>
	<a href="<?php echo basic_url('files.php'); ?>">Files</a><br>
	<a href="<?php echo basic_url('functions.php'); ?>">Functions</a><br>
	<a href="<?php echo basic_url('calls.php'); ?>">Calls</a><br>
	<a href="<?php echo basic_url('callbutterfly.php'); ?>">Call&nbsp;Butterfly</a><br>
	<a href="<?php echo basic_url('diagram.php?d=callgraph'); ?>">Call&nbsp;Graph</a><br>
	<a href="<?php echo basic_url('diagram.php?d=lego'); ?>">Lego&nbsp;Diagram</a><br>
	<a href="<?php echo basic_url('source.php'); ?>">Source</a><br>
<!--
	<a href="http://validator.w3.org/check/referer"><img width=88 height=31 border=0 src="valid-html40.gif" alt="Valid HTML 4.0"></a>
-->
      </td>
      <td valign="top">
<?php
}

function basic_footer()
{
?>
      </td>
      <td class="nav_menu">&nbsp;</td>
    </tr>
    <tr>
      <td colspan="3" class="nav_footer">Generated by <a href="http://ggcov.sourceforge.net/">ggcov</a> which is Copyright (c) 2001-2005 Greg Banks</td>
    </tr>
  </table>
</body>
</html>
<?php
}

function basic_page()
{
    $cov = basic_cov();

    $page = $cov->create_page($_SERVER['PHP_SELF']);

    $page->parse_args($_GET);

    if ($page->content_type() == 'text/html')
	basic_header($page->title());

    $page->render();

    $cov->detach();

    if ($page->content_type() == 'text/html')
	basic_footer();
}

?>
