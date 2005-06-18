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
// $Id: calls.php,v 1.4 2005-06-17 17:13:13 gnb Exp $
//

require_once 'ggcov/lib/cov.php';

define('COV_ANY', '@ny');

class cov_calls_page extends cov_page
{
    var $env_;
    var $from_func_ = null;
    var $to_func_ = null;

    function cov_calls_page($e)
    {
	$this->env_ = $e;
    }

    function parse_args($get)
    {
	$cb = $this->env_->cb_;
	$node_index = $this->env_->callnode_index();

	// get scope from args
	if (array_key_exists('from', $get))
	{
	    $this->from_func_ = $get['from'];

	    if (!cov_valid::funcname($this->from_func_))
		$cb->fatal("Invalid from function");

	    if (array_key_exists($this->from_func_, $node_index))
		$this->from_id_ = $node_index[$this->from_func_];
	    else if ($this->from_func_ != COV_ANY)
		$cb->fatal("Unknown node");
	}
	if (array_key_exists('to', $get))
	{
	    $this->to_func_ = $get['to'];

	    if (!cov_valid::funcname($this->to_func_))
		$cb->fatal("Invalid to function");

	    if (array_key_exists($this->to_func_, $node_index))
		$this->to_id_ = $node_index[$this->to_func_];
	    else if ($this->to_func_ != COV_ANY)
		$cb->fatal("Unknown node");
	}
    }

    function title()
    {
	$f = ($this->from_func_ == COV_ANY || $this->from_func_ === null ?
		'' : " from $this->from_func_");
	$t = ($this->to_func_ == COV_ANY || $this->to_func_ === null
		? '' : " to $this->to_func_");
	return "Calls$f$t";
    }

    function render_calls($from_name, $from_id, $to_name)
    {
	$data = $this->env_->fetch("N$from_id");
	foreach ($data[3] as $ca)
	{
	    $name = $ca[0];
	    $count = $ca[2];
	    if ($to_name == COV_ANY || $to_name == $name)
	    {
		echo <<<HTML
	  <tr>
	    <td>$from_name</td>
	    <td>$name</td>
    <td><!-- TODO --></td>
	    <td>$count</td>
	  </tr>

HTML;
	    }
	}
    }

    function render()
    {
	$cb = $this->env_->cb_;
	$node_index = $this->env_->callnode_index();

	$self = basename($_SERVER['PHP_SELF']);
?>
	<form action="<?php echo $self; ?>" method="GET">
	<table border="0" cellpadding="5" cellspacing="0">
	  <tr>
	    <td colspan="4">
	      <table border="0" cellpadding="5" cellspacing="0">
	        <tr>
		  <td align="right">From Function:</td>
		  <td>
<?php $this->render_state_to_form('              '); ?>
		    <select name="from">
<?php
			  echo "		      <option value=\"" . COV_ANY . "\">Any Function</option>\n";
			  foreach ($node_index as $cn => $id)
			  {
			      $sel = ($cn == $this->from_func_ ? ' selected' : '');
			      echo "		      <option$sel>$cn</option>\n";
			  }
?>
		    </select>
		  </td>
		</tr>
	        <tr>
		  <td align="right">To Function:</td>
		  <td>
		    <select name="to">
<?php
			  echo "		      <option value=\"" . COV_ANY . "\">Any Function</option>\n";
			  foreach ($node_index as $cn => $id)
			  {
			      $sel = ($cn == $this->to_func_ ? ' selected' : '');
			      echo "		      <option$sel>$cn</option>\n";
			  }
?>
		    </select>
		  </td>
		</tr>
	      </table>
	    </td>
	  </tr>
	  <tr>
	    <td>From</td>
	    <td>To</td>
	    <td><!-- Line --></td>
	    <td>Count</td>
	  </tr>
<?php
	if ($this->from_func_ == COV_ANY)
	{
	    foreach ($node_index as $cn => $id)
	    {
		$this->render_calls($cn, $id, $this->to_func_);
	    }
	}
	else if ($this->from_func_ !== null)
	{
	    $this->render_calls($this->from_func_, $this->from_id_,
				$this->to_func_);
	}
?>
	  <tr>
	    <td colspan="4">
	      <input type="submit" name="update" value="Update">
	    </td>
	  </tr>
	</table>
	</form>
<?php
    }
}

?>
