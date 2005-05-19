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
// $Id: tests.php,v 1.2 2005-05-18 14:03:10 gnb Exp $
//
require_once 'ggcov/basic/basic.php';
basic_header('Choose a Test');

echo "<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n";
$list = basic_list_tests();
foreach($list as $test)
{
    $url = 'summary.php?test=' . urlencode($test);
    echo "  <tr><td><a href=\"$url\">$test</a></td></tr>\n";
}
echo "</table>\n";

basic_footer();
?>
