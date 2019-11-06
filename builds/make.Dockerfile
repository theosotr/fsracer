# vim:set ft=dockerfile:
FROM debian:stable

ENV deps="git make vim wget python3 python3-pip strace"

# Add sources
RUN printf "deb-src http://deb.debian.org/debian stable main" >> /etc/apt/sources.list

RUN apt update -y

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $deps

COPY ./tools/fsmake-shell /usr/local/bin/fsmake-shell
COPY syscalls.txt /root/syscalls.txt
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
COPY ./tools/adapter.py /usr/local/bin/adapter.py
RUN mkdir -p /root/traces

WORKDIR /root
# ENTRYPOINT ["entrypoint.sh"]
