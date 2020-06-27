/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2020 Greg Banks <gnb@fastmail.fm>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _ggcov_cov_line_H_
#define _ggcov_cov_line_H_ 1

#include "string_var.H"
#include "list.H"

class cov_file_t;
class cov_function_t;
class cov_suppression_t;

class cov_line_t
{
public:
    cov_line_t();
    ~cov_line_t();

    count_t count()
    {
	if (!count_valid_)
	    calculate_count();
	return count_;
    }

    cov::status_t status()
    {
	if (!count_valid_)
	    calculate_count();
	return status_;
    }
    /* Check for suppression without the side effect of calculate_count() */
    gboolean is_suppressed() const
    {
	return (suppression_ != 0);
    }

    const list_t<cov_block_t> &blocks() const
    {
	return blocks_;
    }
    cov_function_t *function() const;

    /*
     * Format a string compactly describing the list of blocks
     * on a particular line.  Used in the Source window and
     * for tggcov.  Width should be at least 8.
     */
    unsigned int format_blocks(char *buf, unsigned int maxlen);

    gboolean has_blocks() const
    {
	return blocks_.head() != 0;
    }
    unsigned int num_blocks() const
    {
	return blocks_.length();
    }
    void add_block(cov_block_t *b)
    {
	blocks_.append(b);
    }
    void remove_block(cov_block_t *b)
    {
	blocks_.remove(b);
    }

    void suppress(const cov_suppression_t *);
    const cov_suppression_t *suppression() const { return suppression_; }

    void finalise();

private:
    void calculate_count();

    list_t<cov_block_t> blocks_;
    boolean count_valid_;  /* covers status_, count_ */
    cov::status_t status_;
    count_t count_;
    const cov_suppression_t *suppression_;
};


#endif /* _ggcov_cov_line_H_ */
