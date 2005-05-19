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
// $Id: summary.php,v 1.3 2005-05-18 14:15:52 gnb Exp $
//

require_once 'ggcov/lib/cov.php';

class cov_summary_page extends cov_page
{
    var $env_;
    var $scope_ = null;
    var $file_name_ = null;
    var $function_ = null;
    var $func_file_name_ = null;
    var $stats_ = null;
    var $description_ = null;

    function cov_summary_page($e)
    {
	$this->env_ = $e;
    }

    function parse_args($get)
    {
	$cb = $this->env_->cb_;
	$file_index = $this->env_->file_index();
	$func_list = $this->env_->global_function_list();

	// setup some defaults
	$this->scope_ = 'overall';

	foreach ($file_index as $f => $v) break;
	$this->file_name_ = $f;

	foreach ($func_list as $fn => $f) break;
	$this->function_ = preg_replace('/\[.*$/', '', $fn);
	$this->func_file_name_ = $f;

	// get scope from args
	if (array_key_exists('scope', $get))
	{
	    $this->scope_ = $get['scope'];

	    if (!cov_valid::scope($this->scope_))
		$cb->fatal("Invalid scope");
	}

	// verify scope and get further variables from args
	switch ($this->scope_)
	{

	case 'overall':
	    $this->stats_ = $this->env_->fetch('OS');
	    $this->description_ = 'Overall';
	    break;

	case 'file':
	    if (!array_key_exists('file', $get))
		$cb->fatal("No file name given");

	    $this->file_name_ = $get['file'];

	    if (!cov_valid::filename($this->file_name_))
		$cb->fatal("Invalid file name");

	    if (!array_key_exists($this->file_name_, $file_index))
		$cb->fatal("Unknown file");

	    $file_id = $file_index[$this->file_name_];
	    $this->stats_ = $this->env_->fetch("FS$file_id");
	    $this->description_ = "File $this->file_name_";
	    break;

	case 'function':
	    if (!array_key_exists('function', $get))
		$cb->fatal("No function name given");

	    $this->function_ = $get['function'];

	    if (!cov_valid::funcname($this->function_))
		$cb->fatal("Invalid function");

	    if (array_key_exists('file', $get))
	    {
		// optional filename, to disambiguate
		$this->func_file_name_ = $get['file'];

		if (!cov_valid::filename($this->func_file_name_))
		    $cb->fatal("Invalid file name");

		if (!array_key_exists($this->func_file_name_, $file_index))
		    $cb->fatal("Unknown file");

		$label = $this->function_ . ' [' . $this->func_file_name_ . ']';
		if (!array_key_exists($this->function_, $func_list) &&
		    !array_key_exists($label, $func_list))
		    $cb->fatal("Function unknown");
	    }
	    else
	    {
		if (!array_key_exists($this->function_, $func_list))
		    $cb->fatal("Function unknown or ambiguous (try specifying filename too)");
		$this->func_file_name_ = $func_list[$this->function_];
	    }

	    $file_id = $file_index[$this->func_file_name_];
	    $file_function_index = $this->env_->fetch("FUI$file_id");
	    
	    $func_id = $file_function_index[$this->function_];
	    $this->stats_ = $this->env_->fetch("US$func_id");
	    $this->description_ = "Function $this->function_";
	    break;

	case 'range':	// TODO
	default:
	    $cb->fatal("Unknown scope");
	}
    }

    function title()
    {
	return "Summary: $this->description_";
    }

    function render_value_row($label, $values)
    {
	$numerator = $values[COV_COVERED] + $values[COV_PARTCOVERED];
	$denominator = $numerator + $values[COV_UNCOVERED];

	if ($denominator == 0)
	{
	    $frac = '0/0';
	    $pc = '';
	}
	else if ($values[COV_PARTCOVERED] == 0)
	{
	     $frac = sprintf('%lu/%lu', $numerator, $denominator);
	     $pc = sprintf('%.1f%%', $numerator * 100.0 / $denominator);
	}
	else
	{
	     $frac = sprintf('%lu+%lu/%lu',
			    $values[COV_COVERED],
			    $values[COV_PARTCOVERED],
			    $denominator);
	     $pc = sprintf('%.1f+%.1f%%',
			    $values[COV_COVERED] * 100.0 / $denominator,
			    $values[COV_PARTCOVERED] * 100.0 / $denominator);
	}

	$w = 150;
	$h = 20;
	$url = $this->env_->url('covbar.php',
		    'w', $w, 
		    'h', $h, 
		    'c', $values[COV_COVERED], 
		    'pc', $values[COV_PARTCOVERED], 
		    'uc', $values[COV_UNCOVERED]);

	echo <<<HTML
	    <tr>
	      <td align="right">$label</td>
	      <td align="right">$frac</td>
	      <td align="right">$pc</td>
	      <td><img width="$w" height="$h" src="$url"></TD>
	    </tr>

HTML;
    }

    function render()
    {
	$cb = $this->env_->cb_;
	$file_index = $this->env_->file_index();
	$func_list = $this->env_->global_function_list();

	$self = basename($_SERVER['PHP_SELF']);
?>
	<form name="summary" action="<?php echo $self; ?>" method="GET">
	<table border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td colspan="3">
<?php $this->render_state_to_form('            '); ?>
	      <input name="scope" value="overall" type="radio"<?php echo ($this->scope_ == 'overall' ? ' checked' : ''); ?>>Overall
	    </td>
	  </tr>
	  <tr>
	    <td>
	      <input name="scope" value="file" type="radio"<?php echo ($this->scope_ == 'file' ? ' checked' : ''); ?>>Filename
	    </td>
	    <td>
	      <select name="file">
		<?php
		    foreach ($file_index as $fn => $id)
		    {
			$sel = ($fn == $this->file_name_ ? ' selected' : '');
			echo "<option$sel>$fn</option>\n";
		    }
		?>
	      </select>
	    </td>
	    <td>
		<a href="<?php echo $this->env_->url('source.php', 'file', $this->file_name_); ?>">View</a>
	    </td>
	  </tr>
	  <tr>
	    <td>
	      <input name="scope" value="function" type="radio"<?php echo ($this->scope_ == 'function' ? ' checked' : ''); ?>>Function
	    </td>
	    <td>
	      <select name="function">
		<?php
		    $sellab = $this->function_ . ' [' . $this->func_file_name_ . ']';
		    foreach ($func_list as $label => $f)
		    {
			$sel = ($label == $this->function_ ||
			        $label == $sellab ? ' selected' : '');
			echo "<option$sel>$label</option>\n";
		    }
		?>
	      </select>
	    </td>
	    <td>
		<a href="<?php echo $this->env_->url('source.php', 'function', $this->function_, 'file', $this->func_file_name_); ?>">View</a>
	    </td>
	  </tr>
	  <tr>
	    <td>
	      <input name="scope" value="range" type="radio"<?php echo ($this->scope_ == 'range' ? ' checked' : ''); ?>>Range
	    </td>
	    <td colspan="2">
		TODO
	    </td>
	  </tr>
	  <tr>
	    <td colspan="3">
	      <table border="0" cellspacing="0" cellpadding="2">
		<?php
		    $this->render_value_row('Lines', $this->stats_[1]);
		    $this->render_value_row('Functions', $this->stats_[2]);
		    $this->render_value_row('Calls', $this->stats_[3]);
		    $this->render_value_row('Branches', $this->stats_[4]);
		    $this->render_value_row('Blocks', $this->stats_[0]);
		?>
	      </table>
	    </td>
	  </tr>
	  <tr>
	    <td colspan="3">
	      <input type="submit" name="update" value="Update">
	    </td>
	  </tr>
	</table>
	</form>
<?php
    }
}

?>
