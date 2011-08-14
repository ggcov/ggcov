/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@users.sourceforge.net>
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

#include "utils.H"
#include "estring.H"

CVSID("$Id: utils.C,v 1.2 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
populate_function_combo(
    ui_combo_t *cbox,
    const list_t<cov_function_t> *list,
    gboolean add_all_item,
    const cov_function_t **currentp)
{
    estring label;
    static const char all_functions[] = N_("All Functions");

    clear(cbox);
    if (add_all_item)
	add(cbox, _(all_functions), 0);

    for (list_iterator_t<cov_function_t> iter = list->first() ; *iter ; ++iter)
    {
	cov_function_t *fn = *iter;

	if (currentp != 0 && *currentp == 0)
	    *currentp = fn;

	label.truncate();
	label.append_string(fn->name());

	/* see if we need to present some more scope to uniquify the name */
	list_iterator_t<cov_function_t> next = iter.peek_next();
	list_iterator_t<cov_function_t> prev = iter.peek_prev();
	if ((*next && !strcmp((*next)->name(), fn->name())) ||
	    (*prev && !strcmp((*prev)->name(), fn->name())))
	{
	    label.append_string(" (");
	    label.append_string(fn->file()->minimal_name());
	    label.append_string(")");
	}

	add(cbox, label.data(), fn);
    }

    if (currentp != 0)
	set_active(cbox, (gpointer)(*currentp));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
