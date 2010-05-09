<?php
//
// ggcov - A GTK frontend for exploring gcov coverage data
// Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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
// $Id: reports.php,v 1.4 2010-05-09 05:37:14 gnb Exp $
//

require_once 'ggcov/lib/cov.php';

class cov_reports_page extends cov_page
{
    var $name_ = null;
    var $id_ = null;
    var $label_ = null;

    function cov_reports_page($e)
    {
	$this->env_ = $e;
    }

    function parse_args($get)
    {
	$cb = $this->env_->cb_;

	if (array_key_exists('report', $get))
	{
	    $this->name_ = $get['report'];

	    if (!cov_valid::report($this->name_))
		$cb->fatal("Invalid report name");

	    $report_index = $this->env_->report_index();

	    if (!array_key_exists($this->name_, $report_index))
		$cb->fatal("Unknown report");

	    $report_data = $report_index[$this->name_];
	    $this->id_ = $report_data[0];
	    $this->label_ = $report_data[1];
	}
    }

    function title()
    {
	if ($this->id_ === null)
	    return 'Reports: choose a report';
	else
	    return "Reports: $this->label_";
    }

    function render()
    {
	$cb = $this->env_->cb_;
	$report_index = $this->env_->report_index();

	if ($this->id_ === null)
	{
	    // No report, list all the reports and let the user choose one
	    echo "<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n";
	    foreach ($report_index as $report_name => $report_data)
	    {
		$label = $report_data[1];
		$url = $this->env_->url('reports.php', 'report', $report_name);

		echo "  <tr><td><a href=\"$url\">$label</a></td></tr>\n";
	    }
	    echo "</table>\n";
	}
	else
	{
	    // A report was specified, fetch and display its data
	    echo "<pre>\n" . $this->env_->fetch("R$this->id_") . "\n</pre>\n";
	}
    }
}

?>
