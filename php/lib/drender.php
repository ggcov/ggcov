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

require_once 'ggcov/lib/cov.php';

define('CODE_COLOR', 1);
define('CODE_RECTANGLE', 2);
define('CODE_TEXT', 3);
define('CODE_POLYLINE', 4);
define('CODE_POLYGON', 5);

class cov_drender_page extends cov_page
{
    var $env_;
    var $width_;
    var $height_;
    var $zoom_;
    var $panx_;
    var $pany_;
    var $format_;
    var $scene_;

    function cov_drender_page($e)
    {
	$this->env_ = $e;
    }

    function get_integer($get, $name, $def)
    {
	if (array_key_exists($name, $get) && cov_valid::integer($get[$name]))
	    $x = (int)$get[$name];
	else
	    $x = $def;
	return $x;
    }
    function get_floating($get, $name, $def)
    {
	if (array_key_exists($name, $get) && cov_valid::floating($get[$name]))
	    $x = (double)$get[$name];
	else
	    $x = $def;
	return $x;
    }

    function parse_args($get)
    {
	$cb = $this->env_->cb_;
	$diagram_index = $this->env_->diagram_index();

	$this->width_ = $this->get_integer($get, 'w', 500);
	$this->height_ = $this->get_integer($get, 'h', 500);
	$this->zoom_ = $this->get_floating($get, 'z', 1.0);
	$this->panx_ = $this->get_floating($get, 'px', 0.0);
	$this->pany_ = $this->get_floating($get, 'py', 0.0);

	if (array_key_exists('d', $get))
	{
	    $d = $get['d'];

	    if (!array_key_exists($d, $diagram_index))
		$cb->fatal("Invalid diagram");
	    $diag = $diagram_index[$d];
	}
	else
	{
	    $diag = reset($diagram_index);
	}
	$this->scene_ = $this->env_->fetch("G$diag[0]");

	if (function_exists('imagegif'))
	    $this->format_ = 'gif';
	else if (function_exists('imagepng'))
	    $this->format_ = 'png';
	else
	    $cb->fatal("Can't find any image format output function");
    }

    function content_type()
    {
	return "image/$this->format_";
    }

    function title()
    {
	return "Diagram Render";    // never seen
    }

    //
    // Render an arrowhead whose point is at `to' and which
    // lies along the line from `from' to `to'.  Unfortunately
    // all this maths needs to be done in image space so it
    // can't be done in ggcov-webdb.
    //
    function arrowhead($img, $fromx, $fromy, $tox, $toy, $size, $color)
    {
	$c = array();

	// Calculate the arrowhead geometry
	$c[0] = $tox;
	$c[1] = $toy;

	// calculate and normalise the vector along the arrowhead to the point
	$nx = $tox - $fromx;
	$ny = $toy - $fromy;
	$d = sqrt($nx*$nx + $ny*$ny);
	$nx /= $d;
	$ny /= $d;

	// rotate the arrowhead vector to get the base normal
	$bnx = -$ny;
	$bny = $nx;

	// calculate the centre of the arrowhead base
	$bcx = $tox - $size * $nx;
	$bcy = $toy - $size * $ny;

	// calculate the arrowhead barb points
	$size /= 4.0;
	$c[2] = $bcx + $size * $bnx;
	$c[3] = $bcy + $size * $bny;
	$c[4] = $bcx - $size * $bnx;
	$c[5] = $bcy - $size * $bny;
	
	imagefilledpolygon($img, $c, 3, $color);
    }

    function render()
    {
	global $cov_rgb;
	$cb = $this->env_->cb_;


	$img = imagecreate($this->width_, $this->height_);

	// fill image with white
	$c = imagecolorallocate($img, 0xff, 0xff, 0xff);
	imagefilledrectangle($img, 0, 0, $this->width_-1, $this->height_-1, $c);

	$colormap = array();

	$bounds = end($this->scene_);
	$sx = $this->zoom_ * ($this->width_-1) / $bounds[3];
	$ox = (-$sx) * ($this->panx_ * $bounds[3] + $bounds[1]);
	$sy = $this->zoom_ * ($this->height_-1) / $bounds[4];
	$oy = (-$sy) * ($this->pany_ * $bounds[4] + $bounds[2]);

	foreach ($this->scene_ as $elem)
	{
	    switch ($elem[0])
	    {
	    case CODE_COLOR:
		$colormap[$elem[1]] = imagecolorallocate($img, 
					    $elem[2], $elem[3], $elem[4]);
		break;
	    case CODE_RECTANGLE:
		if ($elem[5] !== null)
		    imagefilledrectangle($img,
			    $sx * $elem[1] + $ox, $sy * $elem[2] + $oy,
			    $sx * $elem[3] + $ox, $sy * $elem[4] + $oy,
			    $colormap[$elem[5]]);
		if ($elem[6] !== null)
		    imagerectangle($img,
			    $sx * $elem[1] + $ox, $sy * $elem[2] + $oy,
			    $sx * $elem[3] + $ox, $sy * $elem[4] + $oy,
			    $colormap[$elem[6]]);
		break;
	    case CODE_TEXT:
		$font = 3;
		$lines = split("\n", $elem[3]);
		$y = $sy * $elem[2] + $oy;
		$dy = imagefontheight($font)+4;
		$c = $colormap[$elem[4]];
		foreach ($lines as $line)
		{
		    imagestring($img, $font, 
			$sx * $elem[1] + $ox, $y,
			$line, $c);
		    $y += $dy;
		}
		break;
	    case CODE_POLYLINE:
		$coords = array();
		foreach ($elem[1] as $k => $v)
		{
		    if ($k % 2 == 0)
			$coords[] = $sx * $v + $ox;
		    else
			$coords[] = $sy * $v + $oy;
		}
		$ncoords = count($coords);
		$c = $colormap[$elem[4]];
		for ($i = 0 ; $i < $ncoords-2 ; $i += 2)
		    imageline($img, $coords[$i], $coords[$i+1],
			      $coords[$i+2], $coords[$i+3], $c);
		if ($elem[2] !== null)
		    cov_drender_page::arrowhead($img,
			    $coords[2], $coords[3],
			    $coords[0], $coords[1],
			    $sx * $elem[2], $c);
		if ($elem[3] !== null)
		    cov_drender_page::arrowhead($img,
			    $coords[$ncoords-4], $coords[$ncoords-3],
			    $coords[$ncoords-2], $coords[$ncoords-1],
			    $sx * $elem[3], $c);
		break;
	    case CODE_POLYGON:
		$coords = array();
		foreach ($elem[1] as $k => $v)
		{
		    if ($k % 2 == 0)
			$coords[] = $sx * $v + $ox;
		    else
			$coords[] = $sy * $v + $oy;
		}
		if ($elem[2] !== null)
		    imagefilledpolygon($img, $coords, count($coords)/2,
			     $colormap[$elem[2]]);
		if ($elem[3] !== null)
		    imagepolygon($img, $coords, count($coords)/2,
			     $colormap[$elem[3]]);
		break;
	    }
	}

	header("Content-type: image/$this->format_");
	switch ($this->format_)
	{
	case 'gif':
	    imagegif($img);
	    break;
	case 'png':
	    imagepng($img);
	    break;
	}

	imagedestroy($img);
    }
}

?>
