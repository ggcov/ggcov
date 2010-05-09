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
    GtkCombo *combo,
    const list_t<cov_function_t> *list,
    gboolean add_all_item,
    const cov_function_t **currentp)
{
    list_iterator_t<cov_function_t> iter;
    estring label;
    static const char all_functions[] = N_("All Functions");

    ui_combo_clear(combo);    /* stupid glade2 */
    if (add_all_item)
	ui_combo_add_data(combo, _(all_functions), 0);

    for (iter = list->first() ; iter != (cov_function_t *)0 ; ++iter)
    {
    	cov_function_t *fn = *iter;

	if (currentp != 0 && *currentp == 0)
	    *currentp = fn;

    	label.truncate();
	label.append_string(fn->name());

    	/* see if we need to present some more scope to uniquify the name */
	list_iterator_t<cov_function_t> niter = iter.peek_next();
	list_iterator_t<cov_function_t> piter = iter.peek_prev();
	if ((niter != (cov_function_t *)0 &&
	     !strcmp((*niter)->name(), fn->name())) ||
	    (piter != (cov_function_t *)0 &&
	     !strcmp((*piter)->name(), fn->name())))
	{
	    label.append_string(" (");
	    label.append_string(fn->file()->minimal_name());
	    label.append_string(")");
	}
	
    	ui_combo_add_data(combo, label.data(), fn);
    }
    
    if (currentp != 0)
	ui_combo_set_current_data(combo, (gpointer)(*currentp));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
