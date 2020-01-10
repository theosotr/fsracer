# vim:set ft=dockerfile:
FROM schaliasos/sbuild:latest

ENV dev="git make vim wget python3 python3-pip"

USER root

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $dev

RUN mkdir -p /var/log/sbuild/straces/temp

RUN wget https://www.dropbox.com/s/h9n56nxw8nuaay5/fsmove?dl=0 -O /usr/local/bin/fsmove
RUN chmod +x /usr/local/bin/fsmove

COPY ./config/fsmove/sbuildrc /root/.sbuildrc
COPY ./config/fsmove/sbuildrc /home/builder/.sbuildrc
COPY ./config/fsmove/fstab /etc/schroot/sbuild/fstab

COPY ./tools/fsmake-shell /usr/local/bin/fsmake-shell
COPY ./tools/fsmake-make /usr/local/bin/fsmake-make
COPY ./syscalls.txt /var/lib/sbuild/build
COPY ./scripts/fsmove/analyzer /usr/local/bin/analyzer
COPY ./scripts/fsmove/entrypoint.sh /usr/local/bin/entrypoint.sh


run mkdir -p /results
RUN chown -R builder /results
RUN chmod o+w /results/

USER builder
WORKDIR /home/builder

ENTRYPOINT ["entrypoint.sh"]
