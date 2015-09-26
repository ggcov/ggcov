# Setup C development environment and prereqs
<% if @ppa %>
add-apt-repository -y ppa:<%= @ppa %>
<% end %>
apt-get update
apt-get install -y \
    <% if @gcc_version %>gcc-<%= @gcc_version %><% else %>gcc<% end %> \
    <% if @gcc_version %>g++-<%= @gcc_version %><% else %>g++<% end %> \
    git autoconf automake libtool pkg-config \
    libglade2-dev libgnomeui-dev libgconf2-dev libglib2.0-dev \
    libxml2-dev binutils-dev

# Newer Ubuntu needs the libiberty-dev package installed to
# build with BFD; it was split out from binutils some time ago
# So attempt to install it if the name exists
ee=`apt-get install --print-uris libiberty-dev | grep '^E: Unable to locate package' > /dev/null`
[ -z "$ee" ] && apt-get install -y libiberty-dev

<% if @gcc_version %>
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-<%= @gcc_version %> \
    60 --slave /usr/bin/g++ g++ /usr/bin/g++-<%= @gcc_version %>
<% end %>
tasksel install gnome-desktop --new-install
