#include "common.h"
#include "argparse.H"

namespace argparse
{

params_t::params_t()
 :  files_(new ptrarray_t<const char>())
{
}

params_t::~params_t()
{
    delete files_;
}

bool
params_t::set(const char *key, const char *value)
{
    return false;
}

void
params_t::add_file(const char *file)
{
    files_->append(file);
}

unsigned int
params_t::num_files() const
{
    return files_->length();
}

const char *
params_t::nth_file(unsigned i) const
{
    return files_->nth(i);
}

params_t::file_iterator_t
params_t::file_iter() const
{
    return files_->first();
}

void
params_t::setup_parser(parser_t &)
{
}

void
params_t::post_args()
{
}

simple_params_t::simple_params_t()
 :  values_(new hashtable_t<const char, const char>)
{
}

simple_params_t::~simple_params_t()
{
    delete values_;
}

bool
simple_params_t::set(const char *key, const char *value)
{
    if (!value)
	value = "1";
    values_->insert(key, value);
    return true;
}

bool
simple_params_t::has(const char *key) const
{
    return values_->lookup_extended(key, NULL, NULL);
}

const char *
simple_params_t::value(const char *key) const
{
    return values_->lookup(key);
}

void
simple_params_t::post_args()
{
}

parser_t::parser_t(params_t &params)
 :  params_(params),
    options_(new ptrarray_t<option_t>),
    popt_options_(0)
{
    params_.setup_parser(*this);
}

parser_t::~parser_t()
{
    for (ptrarray_iterator_t<option_t> itr = options_->first() ; *itr ; ++itr)
	delete *itr;
    delete options_;
    delete popt_options_;
}

option_t::option_t(char short_option, const char *long_option)
 :  name_(long_option),
    short_option_(short_option),
    long_option_(long_option),
    flags_(0),		// generic setter, no arguments
    setter_(0)
{
    if (!name_.data())
    {
	char buf[2];
	buf[0] = short_option;
	buf[1] = '\0';
	name_ = (const char *)buf;
    }
}

option_t::~option_t()
{
}

bool
option_t::set(params_t &params, const char *arg) const
{
    if ((flags_ & F_SETTER))
    {
	if ((flags_ & F_ARG))
	{
	    (params.*setter_)(arg);
	}
	else
	{
	    void (params_t::*setter)() = (void (params_t::*)())setter_;
	    (params.*setter)();
	}
	return true;
    }
    else
    {
	if (!(flags_ & F_ARG))
	    arg = 0;	// make sure this has an actual non-random value
	return params.set(name_.data(), arg);
    }
}

void
option_t::build_popt_option(struct poptOption *po)
{
    memset(po, 0, sizeof(*po));
    po->longName = long_option_.data();
    po->shortName = short_option_;
    po->argInfo = ((flags_ & F_ARG) ? POPT_ARG_STRING : POPT_ARG_NONE);
    po->descrip = description_.data();
}

option_t &
parser_t::add_option(char short_name, const char *long_name)
{
    option_t *o = new option_t(short_name, long_name);
    options_->append(o);
    return *o;
}

void
parser_t::popt_callback(poptContext con,
			enum poptCallbackReason reason,
			const struct poptOption *opt,
			const char *arg,
			const void *data)
{
    if (reason == POPT_CALLBACK_REASON_OPTION)
    {
	parser_t *self = (parser_t *)data;
	self->options_->nth(opt->val)->set(self->params_, arg);
    }
}

struct poptOption *
parser_t::get_popt_table()
{
    if (popt_options_ == 0)
    {
	static const struct poptOption headers[] =
	{
	    /* This entry hooks up the popt_callback to be called
	     * whenever the popt library detects an option.  A fact
	     * not documented in the manpage is that this entry
	     * must be the first in the table.
	     */
	    {
		0,				/* longname */
		0,                              /* shortname */
		POPT_ARG_CALLBACK,              /* argInfo */
		/*filled in later*/0,		/* arg */
		0,                              /* val 0=don't return */
		/*filled in later*/0,		/* descrip */
		0                               /* argDescrip */
	    }
	};
	static const unsigned int nheaders = sizeof(headers)/sizeof(headers[0]);
	static const struct poptOption trailers[] =
	{
	    /* This entry adds --help implemented internally in libpopt */
	    POPT_AUTOHELP
	    /* This entry marks the end of the options table */
	    POPT_TABLEEND
	};
	static const unsigned int ntrailers = sizeof(trailers)/sizeof(trailers[0]);

	struct poptOption *popts = new struct poptOption[nheaders + options_->length() + ntrailers];

	memcpy(&popts[0], headers, sizeof(headers));
	// this is for POPT_ARG_CALLBACK
	popts[0].arg = function_to_object((void (*)())popt_callback);
	popts[0].descrip = (const char *)this;

	int n = nheaders;
	for (ptrarray_iterator_t<option_t> itr = options_->first() ; *itr ; ++itr)
	{
	    (*itr)->build_popt_option(&popts[n]);
	    popts[n].val = (n-nheaders);
	    n++;
	}

	memcpy(&popts[n], trailers, sizeof(trailers));

	popt_options_ = popts;
    }

    return popt_options_;
}

int
parser_t::parse(int argc, char **argv)
{
    poptContext popt_context = poptGetContext(PACKAGE, argc, (const char**)argv, get_popt_table(), 0);
    if (other_option_help_.data())
	poptSetOtherOptionHelp(popt_context, other_option_help_.data());

    int rc;
    while ((rc = poptGetNextOpt(popt_context)) > 0)
	;
    if (rc < -1)
    {
	fprintf(stderr, "%s:%s at or near %s\n",
	    argv[0],
	    poptStrerror(rc),
	    poptBadOption(popt_context, POPT_BADOPTION_NOALIAS));
	poptFreeContext(popt_context);
	return -1;
    }

    handle_popt_tail(popt_context);

    poptFreeContext(popt_context);

    return 0;
}

void
parser_t::handle_popt_tail(poptContext popt_context)
{
    const char *file;
    while ((file = poptGetArg(popt_context)) != 0)
	params_.add_file(file);
    params_.post_args();
}

// close the namespace
}
