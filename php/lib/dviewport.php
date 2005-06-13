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
// $Id: dviewport.php,v 1.1 2005-06-13 07:43:34 gnb Exp $
//

require_once 'ggcov/lib/cov.php';

class cov_dviewport_page extends cov_page
{
    var $env_;
    var $width_;
    var $height_;
    var $zoom_;
    var $panx_;
    var $pany_;
    var $format_;

    function cov_dviewport_page($e)
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

	$this->width_ = $this->get_integer($get, 'w', 64);
	$this->height_ = $this->get_integer($get, 'h', 64);
	$this->zoom_ = $this->get_floating($get, 'z', 1.0);
	$this->panx_ = $this->get_floating($get, 'px', 0.0);
	$this->pany_ = $this->get_floating($get, 'py', 0.0);

	// We get passed the diagram name in 'd' but for now
	// we ignore it.  It will be useful later to generate
	// a thumbnail to use as the viewport background.

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
	return "Diagram Viewport";    // never seen
    }

    function render()
    {
	$cb = $this->env_->cb_;

	$img = imagecreate($this->width_, $this->height_);

	// allocate colours
	$cwhite = imagecolorallocate($img, 0xff, 0xff, 0xff);
	$cblack = imagecolorallocate($img, 0x00, 0x00, 0x00);
	$cpurple = imagecolorallocate($img, 0x7d, 0x7d, 0x94);

	// fill with white
	imagefilledrectangle($img, 0, 0,
			     $this->width_-1, $this->height_-1,
			     $cwhite);

	// draw a purple rectangle to represent the viewport
	$x = ($this->width_-2) * $this->panx_ + 1;
	$y = ($this->height_-2) * $this->pany_ + 1;
	$w = ($this->width_-2) / $this->zoom_;
	$h = ($this->height_-2) / $this->zoom_;
	imagefilledrectangle($img, $x, $y, $x+$w, $y+$h, $cpurple);

	// draw a black border; do this last to avoid having to clip
	imagerectangle($img, 0, 0,
		       $this->width_-1, $this->height_-1,
		       $cblack);

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
