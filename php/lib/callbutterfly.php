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
// $Id: callbutterfly.php,v 1.1 2005-05-18 13:14:18 gnb Exp $
//

require_once 'cov.php';

class cov_callbutterfly_page extends cov_page
{
    var $env_;
    var $node_name_ = null;
    var $node_data_ = null;

    function cov_callbutterfly_page($e)
    {
	$this->env_ = $e;
    }

    function parse_args($get)
    {
	$cb = $this->env_->cb_;
	$node_index = $this->env_->callnode_index();

	// get scope from args
	$this->node_name_ = null;
	if (array_key_exists('node', $get))
	{
	    // TODO: input filtering
	    $this->node_name_ = $get['node'];

	    if (!array_key_exists($this->node_name_, $node_index))
		$cb->fatal("Unknown node");

	    $node_id = $node_index[$this->node_name_];
	    $this->node_data_ = $this->env_->fetch("N$node_id");
	}
    }

    function title()
    {
	if ($this->node_name_ === null)
	    return 'Call Butterfly: choose a node';
	else
	    return "Call Butterfly: $this->node_name_";
    }

    function render_arcs($arcs)
    {
	$self = basename($_SERVER['PHP_SELF']);

	echo <<<HTML
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td>Count</td>
    <td>Function</td>
  </tr>

HTML;
	$totcount = 0;
	foreach ($arcs as $ca)
	    $totcount += $ca[2];
	foreach ($arcs as $ca)
	{
	    $name = $ca[0];
	    // TODO: don't need $ca[1] the peer id
	    $count = $ca[2];
	    $url = $this->env_->url($self, 'node', $name);

	    echo "  <tr><td>$count/$totcount</td><td><a href=\"$url\">$name</a></td></tr>\n";
	}
	echo "</table>\n";
    }

    function render()
    {
	$cb = $this->env_->cb_;
	$node_index = $this->env_->callnode_index();

	$self = basename($_SERVER['PHP_SELF']);
?>
	<form name="callbutterfly" action="<?php echo $self; ?>" method="GET">
	<table border="0" cellpadding="5" cellspacing="0">
	  <tr>
	    <td colspan="2">
	      <table border="0" cellpadding="0" cellspacing="0">
	        <tr>
		  <td>Function:</td>
		  <td>
<?php $this->render_state_to_form('              '); ?>
		    <select name="node">
<?php
			  foreach ($node_index as $cn => $id)
			  {
			      $sel = ($cn == $this->node_name_ ? ' selected' : '');
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
	    <td valign="top">
	      Called from
<?php if ($this->node_data_) $this->render_arcs($this->node_data_[2]); ?>
	    </td>
	    <td valign="top">
	      Calls to
<?php if ($this->node_data_) $this->render_arcs($this->node_data_[3]); ?>
	    </td>
	  </tr>
	  <tr>
	    <td colspan="2">
	      <input type="submit" name="update" value="Update">
	    </td>
	  </tr>
	</table>
	</form>
<?php
    }
}

?>
