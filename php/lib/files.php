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

class cov_files_page extends cov_page
{
    var $percent_flag_ = true;

    function cov_files_page($e)
    {
	$this->env_ = $e;
    }

    function parse_args($get)
    {
	// Nothing to see here, move along
    }

    function title()
    {
	return 'Files';
    }

    function format_label($values)
    {
	$numerator = $values[COV_COVERED] + $values[COV_PARTCOVERED];
	$denominator = $numerator + $values[COV_UNCOVERED];

	if ($denominator == 0)
	{
	    return '';
	}
	else if ($values[COV_PARTCOVERED] == 0)
	{
	     if ($this->percent_flag_)
		return sprintf('%.1f%%', $numerator * 100.0 / $denominator);
	    else
		return sprintf('%lu/%lu', $numerator, $denominator);
	}
	else
	{
	    if ($this->percent_flag_)
		return sprintf('%.1f+%.1f%%',
			    $values[COV_COVERED] * 100.0 / $denominator,
			    $values[COV_PARTCOVERED] * 100.0 / $denominator);
	    else
		return sprintf('%lu+%lu/%lu',
			    $values[COV_COVERED],
			    $values[COV_PARTCOVERED],
			    $denominator);
	}
    }

    function render()
    {
	$cb = $this->env_->cb_;
	$file_index = $this->env_->file_index();

?>

<table border="0" cellpadding="5" cellspacing="0">
  <tr>
    <th>Filename</th>
    <th>Blocks</th>
    <th>Lines</th>
    <th>Functions</th>
    <th>Calls</th>
    <th>Branches</th>
  </tr>

<?php
	foreach ($file_index as $file_name => $file_id)
	{
	    $stats = $this->env_->fetch("FS$file_id");
	    $blocks_label = $this->format_label($stats[0]);
	    $blocks_color = cov::color_by_status(cov::status_by_values($stats[0]));
	    $lines_label = $this->format_label($stats[1]);
	    $lines_color = cov::color_by_status(cov::status_by_values($stats[1]));
	    $functions_label = $this->format_label($stats[2]);
	    $functions_color = cov::color_by_status(cov::status_by_values($stats[2]));
	    $calls_label = $this->format_label($stats[3]);
	    $calls_color = cov::color_by_status(cov::status_by_values($stats[3]));
	    $branches_label = $this->format_label($stats[4]);
	    $branches_color = cov::color_by_status(cov::status_by_values($stats[4]));
	    $url = $this->env_->curl('source.php', 'file', $file_name);

	    echo <<<HTML
  <tr>
    <td><a href="$url">$file_name</a></td>
    <td style="color:$blocks_color;">$blocks_label</td>
    <td style="color:$lines_color;">$lines_label</td>
    <td style="color:$functions_color;">$functions_label</td>
    <td style="color:$calls_color;">$calls_label</td>
    <td style="color:$branches_color;">$branches_label</td>
  </tr>

HTML;
	}
	echo <<<HTML
</table>

HTML;
    }
}

?>
