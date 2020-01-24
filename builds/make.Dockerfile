ARG IMAGE_NAME=ubuntu:18.04
FROM ${IMAGE_NAME}

USER root
ENV deps="git make vim wget python3 python3-pip strace bc"

RUN apt update -y

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $deps

COPY ./tools/fsmake-make /usr/local/bin/fsmake-make
COPY ./tools/build-project.sh /usr/local/bin/build-project
COPY ./tools/fsmake-shell /usr/local/bin/fsmake-shell
COPY syscalls.txt /root/syscalls.txt
COPY ./tools/make-entrypoint.sh /usr/local/bin/entrypoint.sh
RUN mkdir -p /root/traces

USER fsmove
WORKDIR $HOME
