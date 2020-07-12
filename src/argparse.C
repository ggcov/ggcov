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
 :  values_(new hashtable_t<const char, char>)
{
}

simple_params_t::~simple_params_t()
{
    values_->foreach_remove(delete_one, 0);
    delete values_;
}

gboolean
simple_params_t::delete_one(const char *key, char *value, gpointer userdata)
{
    g_free(value);
    return TRUE;    /* please remove me */
}

bool
simple_params_t::set(const char *key, const char *value)
{
    if (!value)
	value = "1";
    values_->insert(key, g_strdup(value));
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
    options_by_name_(new hashtable_t<char, option_t>)
{
    params_.setup_parser(*this);
}

static gboolean
remove_one_option_key(char *key, option_t *val, void *closure)
{
    free(key);
    return TRUE;
}

parser_t::~parser_t()
{
    for (ptrarray_iterator_t<option_t> itr = options_->first() ; *itr ; ++itr)
	delete *itr;
    delete options_;

    options_by_name_->foreach_remove(remove_one_option_key, NULL);
    delete options_by_name_;
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


char *
option_t::describe() const
{
    if (long_option_.length())
        return g_strdup_printf("-%c/--%s", short_option_, long_option_.data());
    else
        return g_strdup_printf("-%c", short_option_);
}

option_t &
parser_t::add_option(char short_name, const char *long_name)
{
    option_t *o = new option_t(short_name, long_name);

    char *short_key = (short_name ? g_strdup_printf("-%c", short_name) : NULL);
    char *long_key = (long_name ? g_strdup_printf("--%s", long_name) : NULL);

    if (short_key)
    {
        option_t *old = options_by_name_->lookup(short_key);
        if (old)
        {
            string_var new_desc = o->describe();
            string_var old_desc = old->describe();
            fprintf(stderr, "ERROR duplicate short commandline option %s vs %s\n",
                    (const char *)old_desc, (const char *)new_desc);
        }
        /* TODO: if there's a dup the old key will be leaked */
        options_by_name_->insert(short_key, o);
    }
    if (long_key)
        options_by_name_->insert(long_key, o);

    options_->append(o);
    return *o;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

popt_parser_t::popt_parser_t(params_t &params)
  : parser_t(params),
    popt_options_(NULL)
{
}

popt_parser_t::~popt_parser_t()
{
    delete[] popt_options_;
}

void
popt_parser_t::popt_callback(poptContext con,
                             enum poptCallbackReason reason,
                             const struct poptOption *opt,
                             const char *arg,
                             const void *data)
{
    if (reason == POPT_CALLBACK_REASON_OPTION)
    {
	parser_t *self = (parser_t *)data;
        option_t *o = self->get_nth_option(opt->val);
        assert(o);
        self->set(o, arg);
    }
}

struct poptOption *
popt_parser_t::get_popt_table()
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
            option_t *o = *itr;
            struct poptOption *po = &popts[n];
            memset(po, 0, sizeof(*po));
            po->longName = o->long_option();
            po->shortName = o->short_option();
            po->argInfo = (o->has_argument() ? POPT_ARG_STRING : POPT_ARG_NONE);
            po->descrip = o->description();
            po->val = (n-nheaders);
	    n++;
	}

	memcpy(&popts[n], trailers, sizeof(trailers));

	popt_options_ = popts;
    }

    return popt_options_;
}

int
popt_parser_t::parse(int argc, char **argv)
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
popt_parser_t::handle_popt_tail(poptContext popt_context)
{
    const char *file;
    while ((file = poptGetArg(popt_context)) != 0)
	params_.add_file(file);
    params_.post_args();
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if HAVE_GTK_INIT_WITH_ARGS

goption_parser_t *goption_parser_t::instance_ = NULL;

goption_parser_t::goption_parser_t(params_t &params)
  : parser_t(params),
    goptions_(NULL)
{
    assert(instance_ == NULL);
    instance_ = this;
}

goption_parser_t::~goption_parser_t()
{
    delete[] goptions_;
    assert(instance_ == this);
    instance_ = NULL;
}

gboolean
goption_parser_t::goption_callback(const char *option_name,
                                   const char *value,
                                   gpointer data,   /* if only we could control this */
                                   GError **error)
{
    option_t *o = instance_->get_option_by_name(option_name);
    assert(o);
    instance_->set(o, value);
    return TRUE;
}

GOptionEntry *
goption_parser_t::get_goption_table()
{
    if (goptions_ == 0)
    {
	GOptionEntry *gopts = new GOptionEntry[options_->length()];

	int n = 0;
	for (ptrarray_iterator_t<option_t> itr = options_->first() ; *itr ; ++itr)
	{
            option_t *o = *itr;
            GOptionEntry *go = &gopts[n];
            memset(go, 0, sizeof(*go));
            go->long_name = o->long_option();
            go->short_name = o->short_option();
            go->flags = (o->has_argument() ? G_OPTION_FLAG_NONE : G_OPTION_FLAG_NO_ARG);
            go->arg = G_OPTION_ARG_CALLBACK;
            go->arg_data = (gpointer)goption_callback;
            go->description = o->description();
            go->arg_description = o->metavar();
            n++;
	}

        goptions_ = gopts;
    }
    return goptions_;
}

int
goption_parser_t::parse(int argc, char **argv)
{
    /*
     * This function is not needed, if we have GOption then
     * we're using gtk_init_with_args() to do the parsing.
     */
    fprintf(stderr, "Internal error: goption_parser_t::parse() not implemented\n");
    exit(1);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

// close the namespace
}
