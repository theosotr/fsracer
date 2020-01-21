# vim:set ft=dockerfile:
FROM ubuntu:18.04

ENV dev="git make vim wget cmake clang libboost-all-dev bc python-yaml"

USER root
WORKDIR root

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $dev

# INSTALL mkcheck
RUN git clone https://github.com/nandor/mkcheck
WORKDIR mkcheck
COPY ./patches/mkcheck/syscall.cpp mkcheck/syscall.cpp
RUN mkdir Release && cd Release && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ && \
    make
RUN install ./Release/mkcheck /usr/local/bin/
COPY ./scripts/mkcheck/fuzz_test /usr/local/bin


COPY ./tools/mkcheck-script.sh /usr/local/bin/build-project

WORKDIR /root
