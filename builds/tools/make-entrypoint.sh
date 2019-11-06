#!/usr/bin/env bash

package=$@
apt source $package
# cd to source directory
re='[a-z0-9+.]*-'
for dir in *; do
    [[ $dir =~ $re ]] && { cd "$dir" && break; }
done
# Install deps, clean-configure, build-generate traces
deps=$(dpkg-checkbuilddeps 2>&1 | sed 's/.*: //' | sed -e 's/([^()]*)//g') && \
    apt install -y $deps && \
    debian/rules clean && dh_testdir && dh_auto_configure && \
    strace -s 1000 -f -e "$(tr -s '\r\n' ',' < /root/syscalls.txt | \
        sed -e 's/,$/\n/')" -o make.traces make -- \
        SHELL='fsmake-shell '\''$(MAKEFILE_LIST)'\'' '\''$@'\'' '\''$^'\''' && \
        cp make.traces /root/traces/$package.traces &&
        path=$(pwd)
cd /root/traces
adapter.py $path < $package.traces > $package.fstraces
