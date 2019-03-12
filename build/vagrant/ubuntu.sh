# Setup C development environment and prereqs

#
# Use ppa: ubuntu-toolchain-r/test in the VM yaml file to get toolchain test builds
#
<% if @ppa %>
add-apt-repository -y ppa:<%= @ppa %>
<% end %>
apt-get update
apt-get install -y \
    <% if @gcc_version %>gcc-<%= @gcc_version %><% else %>gcc<% end %> \
    <% if @gcc_version %>g++-<%= @gcc_version %><% else %>g++<% end %> \
    git autoconf automake libtool pkg-config \
    libglade2-dev libgnomeui-dev libgconf2-dev libglib2.0-dev \
    libxml2-dev binutils-dev libgd-dev libdb-dev ruby-mustache \
    tasksel

# Newer Ubuntu needs the libiberty-dev package installed to
# build with BFD; it was split out from binutils some time ago
# So attempt to install it if the name exists
ee=`apt-get install --print-uris libiberty-dev | grep '^E: Unable to locate package' > /dev/null`
[ -z "$ee" ] && apt-get install -y libiberty-dev

<% if @gcc_version %>
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-<%= @gcc_version %> \
    60 --slave /usr/bin/g++ g++ /usr/bin/g++-<%= @gcc_version %>
<% end %>

# this trick from
# http://people.skolelinux.org/pere/blog/Calling_tasksel_like_the_installer__while_still_getting_useful_output.html
export DEBIAN_FRONTEND=noninteractive
cmd="$(tasksel --test --new-install install gnome-desktop | sed 's/debconf-apt-progress -- //')"
$cmd
