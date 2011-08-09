#include "list.H"
#include "string_var.H"
#include "testfw.h"

struct item
{
    string_var name_;
    item(const char *name) :  name_(name) { }
};

static void count_cb(item *t, void *closure)
{
    unsigned int *countp = (unsigned int *)closure;
    (*countp)++;
}

TEST(ctor)
{
    list_t<item> list;

    check(list.head() == 0);
    check(list.tail() == 0);
    check(list.length() == 0);
}

TEST(append)
{
    list_t<item> list;
    item *foo, *bar, *baz;
    unsigned int count;

    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    foo = new item("foo");
    list.append(foo);
    check_num_equals(list.length(), 1);
    check_ptr_equals(list.head(), foo);
    check_ptr_equals(list.tail(), foo);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 1);

    bar = new item("bar");
    list.append(bar);
    check_num_equals(list.length(), 2);
    check_ptr_equals(list.head(), foo);
    check_ptr_equals(list.tail(), bar);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 2);

    baz = new item("baz");
    list.append(baz);
    check_num_equals(list.length(), 3);
    check_ptr_equals(list.head(), foo);
    check_ptr_equals(list.tail(), baz);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 3);

    item *p = list.remove_head();
    check_ptr_equals(p, foo);
    check_num_equals(list.length(), 2);
    check_ptr_equals(list.head(), bar);
    check_ptr_equals(list.tail(), baz);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 2);

    p = list.remove_head();
    check_ptr_equals(p, bar);
    check_num_equals(list.length(), 1);
    check_ptr_equals(list.head(), baz);
    check_ptr_equals(list.tail(), baz);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 1);

    p = list.remove_head();
    check_ptr_equals(p, baz);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    p = list.remove_head();
    check_null(p);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    item *quux = new item("quux");
    list.append(quux);
    check_num_equals(list.length(), 1);
    check_ptr_equals(list.head(), quux);
    check_ptr_equals(list.tail(), quux);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 1);

    p = list.remove_head();
    check_ptr_equals(p, quux);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    p = list.remove_head();
    check_null(p);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    delete foo;
    delete bar;
    delete baz;
    delete quux;
}

TEST(prepend)
{
    list_t<item> list;
    item *foo, *bar, *baz;
    unsigned int count;

    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    foo = new item("foo");
    list.prepend(foo);
    check_num_equals(list.length(), 1);
    check_ptr_equals(list.head(), foo);
    check_ptr_equals(list.tail(), foo);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 1);

    bar = new item("bar");
    list.prepend(bar);
    check_num_equals(list.length(), 2);
    check_ptr_equals(list.head(), bar);
    check_ptr_equals(list.tail(), foo);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 2);

    baz = new item("baz");
    list.prepend(baz);
    check_num_equals(list.length(), 3);
    check_ptr_equals(list.head(), baz);
    check_ptr_equals(list.tail(), foo);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 3);

    item *p = list.remove_head();
    check_ptr_equals(p, baz);
    check_num_equals(list.length(), 2);
    check_ptr_equals(list.head(), bar);
    check_ptr_equals(list.tail(), foo);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 2);

    p = list.remove_head();
    check_ptr_equals(p, bar);
    check_num_equals(list.length(), 1);
    check_ptr_equals(list.head(), foo);
    check_ptr_equals(list.tail(), foo);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 1);

    p = list.remove_head();
    check_ptr_equals(p, foo);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    p = list.remove_head();
    check_null(p);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    item *quux = new item("quux");
    list.prepend(quux);
    check_num_equals(list.length(), 1);
    check_ptr_equals(list.head(), quux);
    check_ptr_equals(list.tail(), quux);
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 1);

    p = list.remove_head();
    check_ptr_equals(p, quux);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    p = list.remove_head();
    check_null(p);
    check_num_equals(list.length(), 0);
    check_null(list.head());
    check_null(list.tail());
    count = 0;
    list.foreach(count_cb, (void *)&count);
    check_num_equals(count, 0);

    delete foo;
    delete bar;
    delete baz;
    delete quux;
}

#if 0
TEST(iterator)
{
    hashtable_t<const char, item> *ht =
	    new hashtable_t<const char, item>;
    item *foo, *bar, *baz;
    unsigned int count;

    foo = new item("foo", 1001);
    ht->insert(foo->name_, foo);
    bar = new item("bar", 1002);
    ht->insert(bar->name_, bar);
    baz = new item("baz", 1003);
    ht->insert(baz->name_, baz);

    check(ht->size() == 3);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == bar);
    check(ht->lookup("baz") == baz);

    hashtable_iter_t<const char, item> itr;
    check(*itr == 0);
    check(itr.key() == 0);
    check(itr.value() == 0);
    check(++itr == FALSE);

    unsigned int found = 0x0;
    int i;
    itr = ht->first();

    for (i = 0 ; i < 3 ; i++)
    {
	check(itr.key() != 0);
	check(itr.value() != 0);
	if (!strcmp(itr.key(), "foo"))
	{
	    check(itr.value() == foo);
	    found |= 0x1;
	}
	else if (!strcmp(itr.key(), "bar"))
	{
	    check(itr.value() == bar);
	    found |= 0x2;
	}
	else if (!strcmp(itr.key(), "baz"))
	{
	    check(itr.value() == baz);
	    found |= 0x4;
	}
	if (i < 2)
	    check(++itr != 0);
	else
	    check(++itr == 0);
    }
    check(itr.key() == 0);
    check(itr.value() == 0);
    check(++itr == 0);

    delete foo;
    delete bar;
    delete baz;
    delete ht;
}

#endif
