/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005-2020 Greg Banks <gnb@fastmail.fm>
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

#include "lego_diagram.H"
#include "tok.H"
#include "logging.H"

#define WIDTH   (1.0)
#define HEIGHT  (1.0)

static logging::logger_t &_log = logging::find_logger("lego");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

lego_diagram_t::lego_diagram_t()
{
}

lego_diagram_t::~lego_diagram_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
lego_diagram_t::name()
{
    return "lego";
}

const char *
lego_diagram_t::title()
{
    return "Lego Diagram";
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
lego_diagram_t::dump_node(node_t *node)
{
    unsigned int i;
    estring indentbuf;

    for (i = 0 ; i < node->depth_ ; i++)
	indentbuf.append_string("    ");
    _log.debug("%s%s depth=%u blocks=%lu/%lu geom=[%g %g %g %g]\n",
	indentbuf.data(),
	node->name_.data(),
	node->depth_,
	node->stats_.blocks_executed(),
	node->stats_.blocks_total(),
	node->x_,
	node->y_,
	node->w_,
	node->h_);

    for (list_iterator_t<node_t> niter = node->children_.first() ; *niter ; ++niter)
	dump_node(*niter);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
lego_diagram_t::assign_geometry(
    node_t *node,
    double x,
    double y,
    double w,
    double h)
{
    node->x_ = x;
    node->y_ = y;
    node->w_ = w;
    node->h_ = h;
    node->h_cov_ = h * node->stats_.blocks_fraction();
    node->h_uncov_ = h - node->h_cov_;

    x += w;
    h /= (double)node->stats_.blocks_total();

    for (list_iterator_t<node_t> niter = node->children_.first() ; *niter ; ++niter)
    {
	node_t *child = *niter;
	double h2 = h * (double)child->stats_.blocks_total();
	assign_geometry(child, x, y, w, h2);
	y += h2;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
lego_diagram_t::root_name() const
{
    /* common_path always includes a trailing / */
    const char *cpath = cov_file_t::common_path();
    const char *start, *end;

    for (end = cpath + strlen(cpath) - 1 ;
	 end > cpath && *end == '/' ;
	 --end)
	;
    if (end == cpath)
    {
	/* cpath is "/" */
	start = cpath;
    }
    else
    {
	for (start = end ;
	     start > cpath && *start != '/' ;
	     --start)
	    ;
	if (*start == '/')
	    start++;
    }
    return g_strndup(start, (end - start + 1));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
lego_diagram_t::prepare()
{
    _log.debug("lego_diagram_t::prepare\n");

    root_ = new node_t();
    root_->name_ = root_name();
    root_->depth_ = 0;
    maxdepth_ = 0;

    /* First pass: construct a tree of nodes */
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	cov_file_t *f = *iter;

	_log.debug("    adding file \"%s\"\n", f->minimal_name());

	cov_scope_t *sc = new cov_file_scope_t(f);
	const cov_stats_t *st = sc->get_stats();
	if (st->blocks_total() == 0)
	{
	    delete sc;
	    continue;
	}

	node_t *parent = root_;
	node_t *node = 0;
	tok_t tok(f->minimal_name(), "/");
	const char *comp;
	while ((comp = tok.next()) != 0)
	{
	    node = 0;

	    for (list_iterator_t<node_t> niter = parent->children_.first() ; *niter ; ++niter)
	    {
		if (!strcmp((*niter)->name_, comp))
		{
		    node = *niter;
		    break;
		}
	    }
	    if (node == 0)
	    {
		node = new node_t();
		node->name_ = comp;
		parent->children_.append(node);
		node->depth_ = parent->depth_ + 1;
		if (node->depth_ > maxdepth_)
		    maxdepth_ = node->depth_;
	    }
	    parent->stats_.accumulate(st);
	    parent = node;
	}

	assert(node != 0);
	assert(node->file_ == 0);
	node->file_ = f;
	node->stats_ = *st;

	delete sc;
    }

    /* Second pass: assign geometry to the nodes */
    assign_geometry(root_, 0.0, 0.0, (WIDTH / (double)(maxdepth_+1)), HEIGHT);

    /* Debug: dump tree */
    if (_log.is_enabled(logging::DEBUG2))
	dump_node(root_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
lego_diagram_t::show_node(node_t *node, scenegen_t *sg)
{
    string_var label;

    label = g_strdup_printf("%s %4.2f%%",
	node->name_.data(),
	100.0 * node->stats_.blocks_fraction());

    _log.debug2("legowin_t::show_node depth=%u name=\"%s\" label=\"%s\"\n",
		node->depth_, node->name_.data(), label.data());

    sg->fill(bg_rgb_by_status_[cov::COVERED]);
    sg->noborder();
    sg->box(node->x_, node->y_, node->w_, node->h_cov_);

    sg->fill(bg_rgb_by_status_[cov::UNCOVERED]);
    sg->noborder();
    sg->box(node->x_, node->y_+node->h_cov_, node->w_, node->h_uncov_);

    sg->nofill();
    sg->border(RGB(0,0,0));
    sg->box(node->x_, node->y_, node->w_, node->h_);
    sg->object(node->file_);
    sg->textbox(node->x_, node->y_, node->w_, node->h_, label.data());

    for (list_iterator_t<node_t> niter = node->children_.first() ; *niter ; ++niter)
	show_node(*niter, sg);
}

void
lego_diagram_t::render(scenegen_t *sg)
{
    show_node(root_, sg);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
lego_diagram_t::get_bounds(dbounds_t *db)
{
    db->x1 = 0.0;
    db->y1 = 0.0;
    db->x2 = WIDTH;
    db->y2 = HEIGHT;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
