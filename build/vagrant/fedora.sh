# Setup C development environment and prereqs
domainname localdomain

yum -y groupinstall 'Development Tools'
yum -y install \
    git valgrind-devel binutils-devel libxml2-devel \
    doxygen python-pygments python-markdown \
    perl-XML-LibXML strace python-pip \
    glib2-devel gd-devel
pip install breathe Sphinx
