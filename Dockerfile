FROM ubuntu:18.04

ENV deps="git vim wget gcc g++ clang python2.7 make cmake"

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade
RUN apt install -yqq $deps

WORKDIR /root

# Copy fsracer
COPY ./ fsracer/
WORKDIR fsracer
RUN git clone https://github.com/theosotr/node engines/node

# Install dynamo
RUN wget -O dynamo.tar.gz https://github.com/DynamoRIO/dynamorio/releases/download/cronbuild-7.90.17998/DynamoRIO-x86_64-Linux-7.90.17998-0.tar.gz
RUN tar -xvf dynamo.tar.gz && rm -rf dynamo.tar.gz && \
    mv DynamoRIO-x86_64-Linux-7.90.17998-0 dynamo

# Install node
WORKDIR engines/node
RUN ./configure && make -j4

WORKDIR /root/fsracer

# Build fsracer
RUN export NODE=$(pwd)/engines/node/node; export DYNAMO=$(pwd)/dynamo; \
    mkdir build && cd build && \
    cmake -DTEST_BINARY_PATH=$NODE -DDynamoRIO_BUILD_DIR=$DYNAMO ..
WORKDIR /root/fsracer/build
RUN make
