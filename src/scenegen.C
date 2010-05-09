/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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

#include "scenegen.H"

CVSID("$Id: scenegen.C,v 1.5 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

scenegen_t::scenegen_t()
{
    arrow_size_ = 0.5;
    arrow_shape_[0] = 1.0;
    arrow_shape_[1] = 1.0;
    arrow_shape_[2] = 0.25;
}

scenegen_t::~scenegen_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
scenegen_t::arrow_size(double s)
{
    arrow_size_ = s;
}

void
scenegen_t::arrow_shape(double a, double b, double c)
{
    arrow_shape_[0] = a;
    arrow_shape_[1] = b;
    arrow_shape_[2] = c;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
scenegen_t::set_object(object_type_t type, void *obj)
{
    object_ = obj;
    object_type_ = type;
}

void
scenegen_t::object(cov_function_t *fn)
{
    set_object(OT_FUNCTION, fn);
}

void
scenegen_t::object(cov_file_t *f)
{
    set_object(OT_FILE, f);
}

void *
scenegen_t::get_object(object_type_t type)
{
    void *obj = 0;

    if (type == object_type_)
    {
	obj = object_;
	object_ = 0;
	object_type_ = OT_NONE;
    }

    return obj;
}

cov_function_t *
scenegen_t::get_function()
{
    return (cov_function_t *)get_object(OT_FUNCTION);
}

cov_file_t *
scenegen_t::get_file()
{
    return (cov_file_t *)get_object(OT_FILE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
