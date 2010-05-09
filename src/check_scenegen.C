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

#include "check_scenegen.H"

CVSID("$Id: check_scenegen.C,v 1.5 2010-05-09 05:37:14 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

check_scenegen_t::check_scenegen_t()
{
    points_ = (double *)0;
    npoints_ = 0;
    points_size_ = 0;
}

check_scenegen_t::~check_scenegen_t()
{
    if (points_ != 0)
	g_free(points_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
check_scenegen_t::noborder()
{
}

void
check_scenegen_t::border(unsigned int rgb)
{
}

void
check_scenegen_t::nofill()
{
}

void
check_scenegen_t::fill(unsigned int rgb)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
check_scenegen_t::box(double x, double y, double w, double h)
{
    box_t *b = new box_t;

    b->name_ = name_.data();
    b->bbox_.x1 = x;
    b->bbox_.y1 = y;
    b->bbox_.x2 = x+w;
    b->bbox_.y2 = y+h;

    boxes_.append(b);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
check_scenegen_t::textbox(
    double x,
    double y,
    double w,
    double h,
    const char *text)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
check_scenegen_t::polyline_begin(gboolean arrow)
{
    npoints_ = 0;
    bbox_.initialise();
}

void
check_scenegen_t::polyline_point(double x, double y)
{
    unsigned int newsize = 2 * sizeof(double) * (npoints_+1);
    if (newsize > points_size_)
    {
	/* round newsize up to a 256-byte boundary to reduce allocations */
	newsize = (newsize + 0xff) & ~0xff;
	/* TODO: use a new gnb_xrealloc */
	points_ = (double *)g_realloc(points_, newsize);
	points_size_ = newsize;
    }

    double *p = points_ + 2*npoints_;
    p[0] = x;
    p[1] = y;
    npoints_++;
    bbox_.adjust(x, y);
}

void
check_scenegen_t::polyline_end(gboolean arrow)
{
    if (npoints_ < 2)
	return;

    polyline_t *pl = new polyline_t;

    pl->bbox_ = bbox_;
    pl->npoints_ = npoints_;
    pl->points_ = (double *)g_memdup(points_, sizeof(double)*2*npoints_);

    polylines_.append(pl);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
check_scenegen_t::object(cov_function_t *f)
{
    name_ = f->name();
    scenegen_t::object(f);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
check_scenegen_t::check()
{
    gboolean ret = TRUE;

    ret &= check_box_intersections();

    return ret;
}

gboolean
check_scenegen_t::check_box_intersections()
{
    list_iterator_t<box_t> aiter;
    list_iterator_t<box_t> biter;
    unsigned int n_box_intersect = 0;

    for (aiter = boxes_.first() ; aiter != (box_t *)0 ;  ++aiter)
    {
	box_t *a = (*aiter);

	for (biter = boxes_.first() ; biter != (box_t *)0 ; ++biter)
	{
	    box_t *b = (*biter);

	    if (a == b)
		break;
	    if (a->bbox_.intersects(b->bbox_))
	    {
	    	fprintf(stderr, "boxes %s and %s intersect\n",
		    	a->name_.data(), b->name_.data());
		n_box_intersect++;
	    }
	}
    }
    fprintf(stderr, "%u boxes, %u box intersections\n",
	    boxes_.length(), n_box_intersect);
    return (n_box_intersect == 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
