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
// $Id: covbar.php,v 1.2 2005-05-18 13:52:55 gnb Exp $
//

require_once 'ggcov/lib/cov.php';

class cov_covbar_page extends cov_page
{
    var $env_;
    var $width_;
    var $height_;
    var $values_;
    var $format_;

    function cov_covbar_page($e)
    {
	$this->env_ = $e;
    }

    function get_number($get, $name, $def)
    {
	if (array_key_exists($name, $get))
	    $x = (int)$get[$name];
	else
	    $x = $def;
	return $x;
    }

    function parse_args($get)
    {
	$cb = $this->env_->cb_;

	$this->width_ = $this->get_number($get, 'w', 150);
	$this->height_ = $this->get_number($get, 'h', 25);

	$this->values_ = array();
	$this->values_[COV_COVERED] = $this->get_number($get, 'c', 0);
	$this->values_[COV_PARTCOVERED] = $this->get_number($get, 'pc', 0);
	$this->values_[COV_UNCOVERED] = $this->get_number($get, 'uc', 0);

	if (function_exists('imagegif'))
	    $this->format_ = 'gif';
	else if (function_exists('imagepng'))
	    $this->format_ = 'png';
	else
	    $cb->fatal("Can't find any image format output function");
    }

    function content_type()
    {
	return 'image/gif';
    }

    function title()
    {
	return null;
    }

    function render()
    {
	global $cov_rgb;
	$cb = $this->env_->cb_;

	$total = array_sum($this->values_);
	if ($total == 0)
	    exit;   // die and emit no output
	else if ($total < 0)
	    $cb->fatal("illegal count total");

	$img = imagecreate($this->width_, $this->height_);

	$x = 0;
	foreach (array(COV_COVERED, COV_PARTCOVERED, COV_UNCOVERED) as $status)
	{
	    $rgb = $cov_rgb[$status];
	    $col = imagecolorallocate($img, $rgb[0], $rgb[1], $rgb[2]);
	    $w = $this->width_ * $this->values_[$status] / $total;
	    imagefilledrectangle($img, $x, 0, $x+$w, $this->height_, $col);
	    $x += $w;
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
