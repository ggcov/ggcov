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
// $Id: source.php,v 1.1 2005-05-18 13:14:18 gnb Exp $
//

require_once 'cov.php';

class cov_source_page extends cov_page
{
    var $env_;
    var $file_name_ = null;
    var $file_id_ = null;
    var $function_ = null;
    var $start_line_ = 0;
    var $end_line_ = 0;

    function cov_source_page($e)
    {
	$this->env_ = $e;
    }

    function parse_args($get)
    {
	$cb = $this->env_->cb_;
	$file_index = $this->env_->file_index();

	if (array_key_exists('function', $get))
	{
	    // TODO: input filtering
	    $this->function_ = $get['function'];

	    if (array_key_exists('file', $get))
	    {
		// optional filename to disambiguate
		$this->file_name_ = $get['file'];

		if (!array_key_exists($this->file_name_, $file_index))
		    $cb->fatal("Unknown file");

		$this->file_id_ = $file_index[$this->file_name_];

		$function_index = $this->env_->fetch("FUI$this->file_id_");

		if (!array_key_exists($this->function_, $function_index))
		    $cb->fatal("Unknown function");

		$func_id = $function_index[$this->function_];

		$func_data = $this->env_->fetch("U$func_id");
	    }
	    else
	    {
		// only the function name, it better be unambiguous
		$function_index = $this->env_->fetch("UI");

		if (!array_key_exists($this->function_, $function_index))
		    $cb->fatal("Unknown function");

		$func_id = reset($function_index[$this->function_]);

		$func_data = $this->env_->fetch("U$func_id");
		
		$this->file_id_ = $file_index[$func_data[2]];
	    }

	    $this->start_line_ = $func_data[3] - 2;
	    $this->end_line_ = $func_data[4] + 2;
	}
	else if (array_key_exists('file', $get))
	{
	    // TODO: input filtering
	    $this->file_name_ = $get['file'];

	    if (!array_key_exists($this->file_name_, $file_index))
		$cb->fatal("Unknown file");
	    $this->file_id_ = $file_index[$this->file_name_];
	}
    }

    function title()
    {
	if ($this->file_name_ == null)
	{
	    return 'Source: Choose A File';
	}
	else
	{
	    if ($this->function_ != null)
		return "Source: function $this->function_";
	    else
		return "Source: file $this->file_name_";
	}
    }

    function render()
    {
	$cb = $this->env_->cb_;
	$file_index = $this->env_->file_index();

	if ($this->file_name_ == null)
	{
	    //
	    // No filename given in the URL; display all
	    // the files and let the user choose one.
	    //
	    echo "<table border=\"0\" cellspacing=\"5\" cellpadding=\"0\">\n";
	    foreach ($file_index as $file_name => $file_id)
	    {
		$url = $this->env_->url('source.php', 'file', $file_name);
		echo "  <tr><td><a href=\"$url\">$file_name</a></td></tr>\n";
	    }
	    echo "</table>\n";
	}
	else
	{
	    $lines = $this->env_->fetch("FL$this->file_id_");

	    $fp = fopen($this->env_->base_directory_ . '/' . $this->file_name_, 'r');
	    if ($fp == false)
		$cb->fatal("Can't open source file");

	    echo <<<HTML
	  <table border="0" cellspacing="0" cellpadding="0">
	    <tr>
	      <th>Line</th>
	      <th>Count</th>
	      <th>Blocks</th>
	      <th>Text</th>
	    </tr>

HTML;

	    $lineno = 0;
	    while ($s = fgets($fp, 1024))
	    {
		$text = htmlentities(rtrim($s));
		$lineno++;
		if ($lineno < $this->start_line_ ||
		    ($this->end_line_ != 0 && $lineno > $this->end_line_))
		    continue;
		if (array_key_exists($lineno, $lines))
		{
		    $ln = $lines[$lineno];
		    $status = $ln[0];
		    $count = $ln[1];
		    $blocks = $ln[2];
		}
		else
		{
		    $status = COV_UNINSTRUMENTED;
		    $count = '';
		    $blocks = '';
		}

		$color = cov::color_by_status($status);
		switch ($status)
		{
		case COV_UNCOVERED:
		    $count = '####';
		    break;
		case COV_UNINSTRUMENTED:
		case COV_SUPPRESSED:
		    $count = '';
		    $blocks = '';
		    break;
		}

		echo <<<HTML
	    <tr style="color:$color;">
	      <td class="basic">$lineno</td>
	      <td class="basic">$count</td>
	      <td class="basic">$blocks</td>
	      <td class="source">$text</td>
	    </tr>

HTML;
	    }

	    echo "  </table>\n";
	}
    }
}

?>
