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
// $Id: diagram.php,v 1.6 2005-06-17 17:13:13 gnb Exp $
//

require_once 'ggcov/lib/cov.php';

define('MIN_IMAGE_SIZE', 64);
define('MAX_IMAGE_SIZE', 1024);
define('ZOOM_FACTOR', 1.5);
define('MIN_ZOOM', 1.0);
define('MAX_ZOOM', pow(ZOOM_FACTOR,6));
define('PAN_STEP', 0.25);

function image_limit($x)
{
    if ($x > MAX_IMAGE_SIZE)
	$x = MAX_IMAGE_SIZE;
    if ($x < MIN_IMAGE_SIZE)
	$x = MIN_IMAGE_SIZE;
    return $x;
}

function zoom_limit($z)
{
    if ($z > MAX_ZOOM)
	$z = MAX_ZOOM;
    if ($z < MIN_ZOOM)
	$z = MIN_ZOOM;
    return $z;
}

function pan_limit($x, $w)
{
    if ($x + $w > 1.0)
	$x = 1.0 - $w;
    if ($x < 0.0)
	$x = 0.0;
    return $x;
}

class cov_diagram_page extends cov_page
{
    var $env_;
    var $diagram_ = null;
    var $title_;
    var $width_;
    var $height_;
    var $zoom_;
    var $panx_;
    var $pany_;


    function cov_diagram_page($e)
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

	$this->width_ = image_limit($this->get_integer($get, 'w', 450));
	$this->height_ = image_limit($this->get_integer($get, 'h', 450));
	$this->zoom_ = zoom_limit($this->get_floating($get, 'z', 1.0));
	$this->panx_ = pan_limit($this->get_floating($get, 'px', 0.0), 0.0);
	$this->pany_ = pan_limit($this->get_floating($get, 'py', 0.0), 0.0);

	if (array_key_exists('d', $get))
	{
	    $d = $get['d'];

	    if (!array_key_exists($d, $diagram_index))
		$cb->fatal("Invalid diagram");
	    $diag = $diagram_index[$d];
	}
	else
	{
	    foreach ($diagram_index as $d => $diag)
	    {
		break;
	    }
	}
	$this->diagram_ = $d;
	$this->title_ = $diag[1];
    }

    function title()
    {
	return "$this->title_";
    }

    function render()
    {
	$cb = $this->env_->cb_;
//	$file_index = $this->env_->file_index();
//	$func_list = $this->env_->global_function_list();

	$pan_step = PAN_STEP / $this->zoom_;

	$self = basename($_SERVER['PHP_SELF']);
	$durl = $this->env_->url('drender.php',
		    'd', $this->diagram_,
		    'w', $this->width_,
		    'h', $this->height_,
		    'z', $this->zoom_,
		    'px', $this->panx_,
		    'py', $this->pany_);
	$saurl = $this->env_->url($self,
		    'd', $this->diagram_,
		    'w', $this->width_,
		    'h', $this->height_,
		    'z', 1,
		    'px', 0,
		    'py', 0);
	$ziurl = $this->env_->url($self,
		    'd', $this->diagram_,
		    'w', $this->width_,
		    'h', $this->height_,
		    'z', zoom_limit($this->zoom_ * ZOOM_FACTOR),
		    'px', $this->panx_,
		    'py', $this->pany_);
	$zourl = $this->env_->url($self,
		    'd', $this->diagram_,
		    'w', $this->width_,
		    'h', $this->height_,
		    'z', zoom_limit($this->zoom_ / ZOOM_FACTOR),
		    'px', $this->panx_,
		    'py', $this->pany_);
	$purls = array();
	$zw = 1.0 / $this->zoom_;
	for ($i = 0 ; $i < 9 ; $i++)
	{
	    $dx = (($i % 3) - 1) * $pan_step;
	    $dy = ((int)($i / 3) - 1) * $pan_step;
	    
	    $purls[$i] = $this->env_->url($self,
		    'd', $this->diagram_,
		    'w', $this->width_,
		    'h', $this->height_,
		    'z', $this->zoom_,
		    'px', sprintf('%f', pan_limit($this->panx_+$dx, $zw)),
		    'py', sprintf('%f', pan_limit($this->pany_+$dy, $zw)));
	}

	$vpw = 64;
	$vph = 64;
	$vpurl = $this->env_->url('dviewport.php',
		'd', $this->diagram_,
		'w', $vpw,
		'h', $vph,
		'z', $this->zoom_,
		'px', $this->panx_,
		'py', $this->pany_);

?>
	<table border="0" cellpadding="5" cellspacing="0">
	  <tr>
	    <td valign="top">
	      <img src="<?php echo $durl; ?>" width="<?php echo $this->width_; ?>" height="<?php echo $this->height_; ?>" alt="diagram">
	    </td>
	    <td valign="top">
	      <table border="0" cellpadding="0" cellspacing="0">
	        <tr>
		  <td align="center">
		    <a href="<?php echo $ziurl; ?>"><img src="zoom-in.gif" alt="Zoom In" border="0" width="28" height="28"></a><br>
		  </td>
		  <td align="center">
		    <a href="<?php echo $zourl; ?>"><img src="zoom-out.gif" alt="Zoom Out" border="0" width="28" height="28"></a><br>
		  </td>
	        </tr>
	        <tr>
		  <td colspan="2" align="center">
		    <a href="<?php echo $saurl; ?>"><img src="show-all.gif" alt="Show All" border="0" width="28" height="28"></a><br>
		  </td>
	        </tr>
	        <tr><td>&nbsp;</td></tr>
	        <tr>
	          <td colspan="2" align="center">
		    <table border="0" cellpadding="0" cellspacing="0">
		      <tr>
			<td>
			  <a href="<?php echo $purls[0]; ?>"><img src="pan-ul.gif" alt="Pan Up&amp;Left" border="0" width="23" height="23"></a>
			</td>
			<td>
			  <a href="<?php echo $purls[1]; ?>"><img src="pan-u.gif" alt="Pan Up" border="0" width="19" height="23"></a>
			</td>
			<td>
			  <a href="<?php echo $purls[2]; ?>"><img src="pan-ur.gif" alt="Pan Up&amp;Right" border="0" width="23" height="23"></a>
			</td>
		      </tr>
		      <tr>
			<td>
			  <a href="<?php echo $purls[3]; ?>"><img src="pan-l.gif" alt="Pan Left" border="0" width="23" height="19"></a>
			</td>
			<td>
			  <img src="pan-c.gif" width="19" height="19" alt="">
			</td>
			<td>
			  <a href="<?php echo $purls[5]; ?>"><img src="pan-r.gif" alt="Pan Right" border="0" width="23" height="19"></a>
			</td>
		      </tr>
		      <tr>
			<td>
			  <a href="<?php echo $purls[6]; ?>"><img src="pan-dl.gif" alt="Pan Down&amp;Left" border="0" width="23" height="23"></a>
			</td>
			<td>
			  <a href="<?php echo $purls[7]; ?>"><img src="pan-d.gif" alt="Pan Down" border="0" width="19" height="23"></a>
			</td>
			<td>
			  <a href="<?php echo $purls[8]; ?>"><img src="pan-dr.gif" alt="Pan Down&amp;Right" border="0" width="23" height="23"></a>
			</td>
		      </tr>
		    </table>
	          </td>
	        </tr>
	        <tr><td>&nbsp;</td></tr>
	        <tr>
	          <td colspan="2" align="center">
		    <img src="<?php echo $vpurl; ?>" width="<?php echo $vpw; ?>" height="<?php echo $vph; ?>" alt="viewport">
	          </td>
	        </tr>
	      </table>
	    </td>
	  </tr>
	</table>
<?php
    }
}

?>
