/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2004-2005 Greg Banks <gnb@users.sourceforge.net>
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

#include "cpp_parser.H"
#include "tok.H"
#include "logging.H"

#define ISBLANK(c)      ((c) == ' ' || (c) == '\t')

static logging::logger_t &_log = logging::find_logger("cpp");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cpp_parser_t::cpp_parser_t(const char *filename)
 :  filename_(filename)
{
}

cpp_parser_t::~cpp_parser_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
cpp_parser_t::xgetc()
{
    return line_.data()[index_++];
}

void
cpp_parser_t::xungetc(int c)
{
    assert(index_ > 0);
    assert(line_.data()[index_-1] == c);
    index_--;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
cpp_parser_t::getc_commentless()
{
    while (index_ < line_.length())
    {
	int c = xgetc();

	if (!in_comment_)
	{
	    /* scanning for start of comment */
	    if (c == '/')
	    {
		c = xgetc();
		if (c == '*')
		{
		    in_comment_ = TRUE;
		    comment_.truncate();
		}
		else if (c == '/')  /* // C++ style comment */
		{
		    got_comment(line_, index_);
		    return EOF;
		}
		else
		{
		    xungetc(c);
		    return '/';
		}
	    }
	    else
		return c;
	}
	else
	{
	    /* scanning for '*' at end of C comment */
	    if (c == '*')
	    {
		c = xgetc();
		if (c == '/')
		{
		    in_comment_ = FALSE;
		    got_comment(comment_, 0);
		}
	    }
	    else
	    {
		comment_.append_char(c);
	    }
	}
    }
    return EOF;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Called when a single-line comment has become available
 * via lexical analysis, with a buffer and the offset
 * into the buffer at which the comment starts (it's
 * assumed to continue until the end of the buffer).
 * Strips surrounding whitespace, inplace and destructively,
 * Then calls the virtual handle_comment().
 */
void
cpp_parser_t::got_comment(estring &e, unsigned int offset)
{
    if (comment_.length())
    {
	_log.debug2("got_comment: e=\"%s\", offset=%u\n", e.data(), offset);

	char *buf = (char *)e.data() + offset;
	char *p = (char *)e.data() + e.length() - 1;

	while (*buf && isspace(*buf))
	    buf++;
	while (p >= buf && isspace(*p))
	    *p-- = '\0';

	handle_comment(buf);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: handle token pasting ??? */
int
cpp_parser_t::get_token()
{
    int c;
    unsigned int len = 0;

    while ((c = getc_commentless()) != EOF && ISBLANK(c))
	;
    if (c == EOF)
	return EOF;

    tokenbuf_[0] = '\0';

    if (isdigit(c))
    {
	do
	{
	    tokenbuf_[len++] = c;
	}
	while ((c = getc_commentless()) != EOF &&
	       isdigit(c) &&
	       len < sizeof(tokenbuf_));
	tokenbuf_[len] = '\0';
	if (c != EOF)
	    xungetc(c);
	return T_NUMBER;
    }
    else if (isalpha(c) || (c) == '_')
    {
	do
	{
	    tokenbuf_[len++] = c;
	}
	while ((c = getc_commentless()) != EOF &&
	       (isalnum(c) || (c) == '_') &&
	       len < sizeof(tokenbuf_));
	tokenbuf_[len] = '\0';
	if (c != EOF)
	    xungetc(c);

	if (!strcmp(tokenbuf_, "include"))
	    return T_INCLUDE;
	if (!strcmp(tokenbuf_, "if"))
	    return T_IF;
	if (!strcmp(tokenbuf_, "ifdef"))
	    return T_IFDEF;
	if (!strcmp(tokenbuf_, "ifndef"))
	    return T_IFNDEF;
	if (!strcmp(tokenbuf_, "else"))
	    return T_ELSE;
	if (!strcmp(tokenbuf_, "elif"))
	    return T_ELIF;
	if (!strcmp(tokenbuf_, "endif"))
	    return T_ENDIF;
	if (!strcmp(tokenbuf_, "define"))
	    return T_DEFINE;
	if (!strcmp(tokenbuf_, "undef"))
	    return T_UNDEF;
	if (!strcmp(tokenbuf_, "defined"))
	    return T_DEFINED;
	return T_IDENTIFIER;
    }
    else if (c == '&')
    {
	if ((c = xgetc()) == '&')
	    return T_LOG_AND;
	xungetc(c);
	return '&';
    }
    else if (c == '|')
    {
	if ((c = xgetc()) == '|')
	    return T_LOG_OR;
	xungetc(c);
	return '|';
    }
    return c;
}

const char *
cpp_parser_t::token_as_string(int tok) const
{
    static const char *token_strs[] =
    {
	"INCLUDE",
	"IF",
	"IFDEF",
	"IFNDEF",
	"ELSE",
	"ELIF",
	"ENDIF",
	"DEFINE",
	"UNDEF",
	"DEFINED",
	"&&",
	"||",
	"NUMBER",
	"IDENTIFIER",
	"BOOL_EXPR",
	"LOG_OP"
    };

    if (tok < 256)
    {
	static char cbuf[8];
	if (isprint(tok))
	{
	    cbuf[0] = tok;
	    cbuf[1] = '\0';
	}
	else
	{
	    snprintf(cbuf, sizeof(cbuf), "\\%03o", tok);
	}
	return cbuf;
    }
    return (tok >= T_MAX_TOKEN ? "unknown" : token_strs[tok-256]);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cpp_parser_t::depend_t::depend_t()
{
    deltas_ = new hashtable_t<const char, int>;
}

static gboolean
delete_one_delta(const char *var, int *dd, void *closure)
{
    delete dd;
    return TRUE;
}

cpp_parser_t::depend_t::~depend_t()
{
    deltas_->foreach_remove(delete_one_delta, 0);
    delete deltas_;
}


void
cpp_parser_t::set_delta(depend_t *dep, const char *var, int delta)
{
    int *dd;

    if ((dd = dep->deltas_->lookup(var)) == 0)
    {
	dd = new int;
	dep->deltas_->insert(g_strdup(var), dd);
    }
    else if (*dd != delta)
    {
	_log.warning(
		"%s:%ld: complex expression involves %s"
		" more than once, code suppression may not work.\n",
		filename_.data(), lineno_, var);
    }
    *dd = delta;
}

gboolean
cpp_parser_t::depends(const char *var) const
{
    unsigned int count[3];
    gboolean ret;

    memset(count, 0, sizeof(count));
    for (list_iterator_t<depend_t> iter = depend_stack_.first() ; *iter ; ++iter)
    {
	depend_t *dep = *iter;
	int *dd = dep->deltas_->lookup(var);
	if (dd != 0)
	    count[(*dd)+1]++;
    }
    ret = (count[2] && !count[0]); // some positive and no negative
    _log.debug2("depends: %s=%d\n", var, ret);
    return ret;
}

void
cpp_parser_t::dump() const
{
    _log.debug("Depend stack dump:\n");
    estring buf;
    for (list_iterator_t<depend_t> iter = depend_stack_.last() ; *iter ; --iter)
    {
	buf.truncate();
	for (hashtable_iter_t<const char, int> ditr = (*iter)->deltas_->first() ; *ditr ; ++ditr)
	    buf.append_printf(" %s=%d", ditr.key(), *ditr.value());
	_log.debug("    [%ld]%s\n", (*iter)->lineno_, buf.data());
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cpp_parser_t::stack_dump()
{
    int i;
    estring buf;

    for (i = 0 ; i < depth_ ; i++)
	buf.append_printf(" %s", token_as_string(stack_[i]));
    _log.debug2("Parse stack:%s\n", buf.data());
}

void
cpp_parser_t::stack_push(int token)
{
    assert(depth_ < MAX_STACK);
    stack_[depth_++] = token;

    if (_log.is_enabled(logging::DEBUG2))
	stack_dump();
}

int
cpp_parser_t::stack_pop()
{
    assert(depth_ > 0);
    return stack_[--depth_];

    if (_log.is_enabled(logging::DEBUG2))
	stack_dump();
}

int
cpp_parser_t::stack_peek(int off)
{
    assert(depth_ > off);
    return stack_[depth_-1-off];
}

void
cpp_parser_t::stack_replace(int ntoks, int newtoken)
{
    assert(depth_ >= ntoks);
    depth_ -= ntoks;
    stack_[depth_++] = newtoken;

    if (_log.is_enabled(logging::DEBUG2))
	stack_dump();
}

gboolean
cpp_parser_t::parse_boolean_expr(gboolean inverted)
{
    int tok;
    string_var tmp;

    while ((tok = get_token()) != EOF)
    {
	_log.debug2("Read token %s %s\n", token_as_string(tok), tokenbuf_);

	/* shift */
	switch (tok)
	{
	case T_LEFT_PAREN:
	case T_DEFINED:
	    stack_push(tok);
	    break;
	case T_LOG_AND:
	case T_LOG_OR:
	    stack_push(T_LOG_OP);
	    break;
	case T_RIGHT_PAREN:
	    if (depth_ >= 3 &&
		stack_peek(0) == T_IDENTIFIER &&
		stack_peek(1) == T_LEFT_PAREN &&
		stack_peek(2) == T_DEFINED)
		stack_replace(3, T_BOOL_EXPR);
	    else if (depth_ >= 2 &&
		stack_peek(0) == T_BOOL_EXPR &&
		stack_peek(1) == T_LEFT_PAREN)
		stack_replace(2, stack_peek(0));
	    break;
	case T_IDENTIFIER:
	    stack_push(T_IDENTIFIER);
	    set_delta(depend_stack_.head(), tokenbuf_, (inverted ? -1 : 1));
	    break;
	case T_BANG:
	    stack_push(T_BANG);
	    inverted = !inverted;
	    break;
	default:
	    return FALSE;
	}

	/* reduce */
	for (;;)
	{
	    if (depth_ >= 2 &&
		stack_peek(0) == T_BOOL_EXPR &&
		stack_peek(1) == T_BANG)
	    {
		stack_replace(2, stack_peek(0));
		inverted = !inverted;
	    }
	    else if (depth_ >= 3 &&
		stack_peek(0) == T_BOOL_EXPR &&
		stack_peek(1) == T_LOG_OP &&
		stack_peek(2) == T_BOOL_EXPR)
		stack_replace(3, T_BOOL_EXPR);
	    else
		break;
	}
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cpp_parser_t::parse_if()
{
    _log.debug("[%ld] if\n", lineno_);

    depend_t *dep = new depend_t;
    dep->lineno_ = lineno_;
    depend_stack_.prepend(dep);

    depth_ = 0;
    parse_boolean_expr(FALSE);

    // notify subclasses
    depends_changed();
}

void
cpp_parser_t::parse_ifdef()
{
    _log.debug("[%ld] ifdef\n", lineno_);

    if (get_token() != T_IDENTIFIER)
	return; /* syntax error */
    if (get_token() != EOF)
	return; /* syntax error */

    depend_t *dep = new depend_t;
    dep->lineno_ = lineno_;
    depend_stack_.prepend(dep);
    set_delta(dep, tokenbuf_, 1);

    // notify subclasses
    depends_changed();
}

void
cpp_parser_t::parse_ifndef()
{
    _log.debug("[%ld] ifndef\n", lineno_);

    if (get_token() != T_IDENTIFIER)
	return; /* syntax error */
    if (get_token() != EOF)
	return; /* syntax error */

    depend_t *dep = new depend_t;
    dep->lineno_ = lineno_;
    depend_stack_.prepend(dep);
    set_delta(dep, tokenbuf_, -1);

    // notify subclasses
    depends_changed();
}

void
cpp_parser_t::parse_else()
{
    _log.debug("[%ld] else\n", lineno_);

    depend_t *dep;
    if ((dep = depend_stack_.head()) == 0)
    {
	_log.warning("%s:%ld: unmatched else\n",
		     filename_.data(), lineno_);
	return;
    }
    dep->lineno_ = lineno_;
    for (hashtable_iter_t<const char, int> ditr = dep->deltas_->first() ; *ditr ; ++ditr)
    {
	int *dd = ditr.value();
	*dd = -*dd;
    }

    // notify subclasses
    depends_changed();
}

void
cpp_parser_t::parse_endif()
{
    _log.debug("[%ld] endif\n", lineno_);

    depend_t *dep;

    if ((dep = depend_stack_.remove_head()) == 0)
    {
	_log.warning("%s:%ld: unmatched endif\n",
		     filename_.data(), lineno_);
	return;
    }

    delete dep;

    // notify subclasses
    depends_changed();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cpp_parser_t::parse_cpp_line(unsigned long lineno)
{
    lineno_ = lineno;
    index_ = 0;
    in_comment_ = FALSE;

    _log.debug2("parse_cpp_line: line[%lu]=\"%s\"\n",
		lineno, line_.data());

    int tok = get_token();
    switch (tok)
    {
    case T_IF: parse_if(); break;
    case T_IFDEF: parse_ifdef(); break;
    case T_IFNDEF: parse_ifndef(); break;
    case T_ELSE: parse_else(); break;
    case T_ENDIF: parse_endif(); break;
    }

    post_line();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cpp_parser_t::parse_c_line(unsigned long lineno)
{
    lineno_ = lineno;
    index_ = 0;
    in_comment_ = FALSE;

    _log.debug2("parse_c_line: line[%lu]=\"%s\"\n", lineno, line_.data());

    while (getc_commentless() != EOF)
	;

    post_line();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cpp_parser_t::parse()
{
    FILE *fp;
    char *start, *end;
    unsigned long lineno = 0, extended = 0, lineno_adj = 0;
    char buf[1024];

    if ((fp = fopen(filename_, "r")) == 0)
    {
	perror(filename_);
	return FALSE;
    }

    while (fgets(buf, sizeof(buf), fp) != 0)
    {
	++lineno;
	/* drop trailing newline */
	for (end = buf+strlen(buf)-1 ;
	     end >= buf && (*end == '\n' || *end == '\r') ;
	     )
	    *end-- = '\0';

	/* skip initial ws */
	for (start = buf ; ISBLANK(*start) ; start++)
	    ;

	if (extended) /* previous line was extended */
	{
	    line_.append_char(' ');
	}
	else
	{
	    line_.truncate();
	    if (*start != '#')
	    {
		line_.append_string(start);
		parse_c_line(lineno - lineno_adj);
		continue;           /* not a pre-processor line */
	    }
	    start++;
	}

	if (*end == '\\')
	{
	    *end-- = '\0';
	    extended++;
	}
	else
	{
	    lineno_adj = extended;
	    extended = 0;
	}

	line_.append_string(start);

	if (extended)           /* this line is extended */
	    continue;

	/* Have a complete pre-processor line in `line_' */
	parse_cpp_line(lineno - lineno_adj);
    }

    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
