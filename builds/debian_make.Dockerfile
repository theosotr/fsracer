# vim:set ft=dockerfile:
FROM debian:stable

ENV dev="git make vim wget python3 python3-pip"
ENV sbuild="sbuild schroot debootstrap"

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $dev $sbuild

# INSTALL sbuild
RUN sbuild-adduser root && \
    sbuild-createchroot --include=eatmydata,ccache,gnupg stable /srv/chroot/stable-amd64-sbuild http://deb.debian.org/debian

# DIRECTORY TO SAVE STATS
RUN mkdir -p /var/log/sbuild/stats
RUN mkdir -p /var/log/sbuild/straces/temp

WORKDIR /root

COPY ./config/sbuildrc /root/.sbuildrc
COPY ./config/fstab /etc/schroot/sbuild/fstab

COPY ./tools/fsmake-shell /usr/local/bin/fsmake-shell
COPY ./syscalls.txt /root/syscalls.txt
COPY ./tools/fsmake-analyzer /usr/local/bin/analyzer
COPY ./tools/debian-entrypoint.sh /usr/local/bin/entrypoint.sh

ENTRYPOINT ["entrypoint.sh"]
