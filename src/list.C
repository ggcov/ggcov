#include "list.H"
#include "estring.H"

char *
join(const char *sep, const list_t<char> &list)
{
    estring buf;
    for (list_iterator_t<char> itr = list.first() ; *itr ; ++itr)
    {
        if (buf.length() && sep)
            buf.append_string(sep);
        buf.append_string(*itr);
    }
    char *res = buf.take();
    return (res ? res : g_strdup(""));
}
