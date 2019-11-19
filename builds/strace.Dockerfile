# vim:set ft=dockerfile:
FROM schaliasos/sbuild:latest

ENV dev="git make vim wget python3 python3-pip"

USER root

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $dev

RUN mkdir -p /var/log/sbuild/straces/temp

COPY ./config/strace/sbuildrc /root/.sbuildrc
COPY ./config/strace/sbuildrc /home/builder/.sbuildrc
COPY ./config/strace/fstab /etc/schroot/sbuild/fstab

COPY ./tools/fsmake-shell /usr/local/bin/fsmake-shell
COPY ./tools/fsmake-make /usr/local/bin/fsmake-make
COPY ./syscalls.txt /var/lib/sbuild/build
COPY ./scripts/strace/analyzer /usr/local/bin/analyzer
COPY ./scripts/strace/entrypoint.sh /usr/local/bin/entrypoint.sh

run mkdir -p /results
RUN chown -R builder /results
RUN chmod o+w /results/

USER builder
WORKDIR /home/builder

ENTRYPOINT ["entrypoint.sh"]
