# vim:set ft=dockerfile:
FROM schaliasos/sbuild:latest

ENV dev="git make vim wget python3 python3-pip"

USER root

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $dev

RUN mkdir -p /var/log/sbuild/straces/temp

COPY ./config/make/sbuildrc /root/.sbuildrc
COPY ./config/make/sbuildrc /home/builder/.sbuildrc
COPY ./config/make/fstab /etc/schroot/sbuild/fstab

COPY ./scripts/make/analyzer /usr/local/bin/analyzer
COPY ./scripts/make/entrypoint.sh /usr/local/bin/entrypoint.sh

run mkdir -p /results
RUN chown -R builder /results
RUN chmod o+w /results/

USER builder
WORKDIR /home/builder

ENTRYPOINT ["entrypoint.sh"]
