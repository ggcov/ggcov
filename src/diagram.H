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

#ifndef _ggcov_diagram_H_
#define _ggcov_diagram_H_ 1

#include "common.h"
#include "cov.H"
#include "scenegen.H"
#include "geometry.H"

class diagram_t
{
public:
    diagram_t();
    virtual ~diagram_t();

    virtual const char *name() = 0;
    virtual const char *title() = 0;
    virtual void prepare() = 0;
    virtual void render(scenegen_t *) = 0;
    virtual void get_bounds(dbounds_t *) = 0;

    void set_fg(cov::status_t, color_t rgb);
    void set_bg(cov::status_t, color_t rgb);

protected:
    color_t fg_by_status(scenegen_t *, cov::status_t);
    color_t bg_by_status(scenegen_t *, cov::status_t);

private:
    unsigned int    fg_rgb_by_status_[cov::NUM_STATUS];
    unsigned int    bg_rgb_by_status_[cov::NUM_STATUS];
};


class diagram_factory_t
{
public:
    virtual const char *name() const = 0;
    virtual const char *label() const = 0;
    static diagram_t *create(const char *name);

protected:
    diagram_factory_t()
    {
	next_ = all_;
	all_ = this;
    }

    virtual diagram_t *creator(void) const = 0;
private:
    diagram_factory_t *next_;
    static diagram_factory_t *all_;
    friend class diagram_factory_iterator_t;
};

#define DIAGRAM_FACTORY(nm, lab) \
struct nm##_diagram_factory_t : public diagram_factory_t \
{ \
    const char *name() const { return #nm; } \
    const char *label() const { return lab; } \
    diagram_t *creator() const { return new nm##_diagram_t(); } \
}; \
nm##_diagram_factory_t nm##_factory

class diagram_factory_iterator_t
{
public:
    diagram_factory_iterator_t()
    {
	df_ = diagram_factory_t::all_;
    }

    const diagram_factory_t *next()
    {
	diagram_factory_t *r = df_;
	if (df_)
	    df_ = df_->next_;
	return r;
    }

private:
    diagram_factory_t *df_;
};

#endif /* _ggcov_diagram_H_ */
