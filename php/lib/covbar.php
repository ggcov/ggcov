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
// $Id: covbar.php,v 1.1 2005-05-18 13:14:18 gnb Exp $
//

require_once 'cov.php';

function get_number($name, $def)
{
    if (array_key_exists($name, $_GET))
	$x = (int)$_GET[$name];
    else
	$x = $def;
    return $x;
}

$width = get_number('w', 150);
$height = get_number('h', 25);

$values = array();
$values[COV_COVERED] = get_number('c', 0);
$values[COV_PARTCOVERED] = get_number('pc', 0);
$values[COV_UNCOVERED] = get_number('uc', 0);
$total = array_sum($values);
if ($total <= 0)
    return;

$img = imagecreate($width, $height);

$x = 0;
foreach (array(COV_COVERED, COV_PARTCOVERED, COV_UNCOVERED) as $status)
{
    $rgb = $cov_rgb[$status];
    $col = imagecolorallocate($img, $rgb[0], $rgb[1], $rgb[2]);
    $w = $width * $values[$status] / $total;
    imagefilledrectangle($img, $x, 0, $x+$w, $height, $col);
    $x += $w;
}

if (function_exists('imagegif'))
{
    header('Content-type: image/gif');
    imagegif($img);
}
else if (function_exists('imagepng'))
{
    header('Content-type: image/png');
    imagepng($img);
}

imagedestroy($img);

?>
