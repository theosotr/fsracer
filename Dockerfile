FROM ubuntu:18.04

ENV deps="git vim wget gcc g++ clang python2.7 make cmake"

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade
RUN apt install -yqq $deps

# Add fsracer user
RUN useradd -ms /bin/bash fsracer
USER fsracer
ENV HOME /home/fsracer

WORKDIR $HOME

# Install dynamo
RUN wget -O dynamo.tar.gz https://github.com/DynamoRIO/dynamorio/releases/download/cronbuild-7.90.17998/DynamoRIO-x86_64-Linux-7.90.17998-0.tar.gz
RUN tar -xvf dynamo.tar.gz && rm -rf dynamo.tar.gz && \
    mv DynamoRIO-x86_64-Linux-7.90.17998-0 dynamo

WORKDIR $HOME

# Install node
RUN git clone https://github.com/theosotr/node engines/node
WORKDIR engines/node
RUN ./configure && make -j4

WORKDIR $HOME

# Copy fsracer
# XXX: Clone the private repo instead of copying files.
RUN mkdir fsracer
COPY ./src fsracer/src
COPY ./tests fsracer/tests
COPY ./CMakeLists.txt fsracer/CMakeLists.txt

WORKDIR $HOME/fsracer

# Build fsracer
RUN export NODE=$HOME/engines/node/node; \
    export DYNAMO=$HOME/dynamo; \
    mkdir build && cd build && \
    cmake -DTEST_BINARY_PATH=$NODE -DDynamoRIO_BUILD_DIR=$DYNAMO ..
WORKDIR $HOME/fsracer/build
RUN make