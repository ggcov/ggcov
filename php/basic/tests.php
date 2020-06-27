<?php
//
// ggcov - A GTK frontend for exploring gcov coverage data
// Copyright (c) 2005-2020 Greg Banks <gnb@fastmail.fm>
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
require_once 'ggcov/basic/basic.php';

$list = basic_list_tests();
if (count($list) == 0)
{
    basic_header('No Tests Available');
    echo <<<HTML
<p>
No test data is available.  Please consult the ggcov documentation
for instructions on how to add test data to ggcov web.
</p>
HTML;
}
else
{
    basic_header('Choose a Test');
    echo "<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n";
    foreach($list as $test)
    {
	$url = 'summary.php?test=' . urlencode($test);
	echo "  <tr><td><a href=\"$url\">$test</a></td></tr>\n";
    }
    echo "</table>\n";
}

basic_footer();
?>
