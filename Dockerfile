FROM ubuntu:18.04

ENV deps="sudo git vim wget gcc-8 g++-8 python2.7 make cmake flex bison gengetopt curl jq"
ENV NODE_REPO="https://github.com/theosotr/node"

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade
RUN apt install -yqq $deps
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8

USER root
WORKDIR /root

# Install nlohmann/json library.
ENV json_version=3.7.2
RUN wget https://github.com/nlohmann/json/archive/v${json_version}.tar.gz && \
  tar -xf v${json_version}.tar.gz
WORKDIR /root/json-${json_version}
RUN mkdir build && cd build && cmake .. && make && make install
WORKDIR /root
RUN rm -rf v${json_version}.tar.gz json-${json_version}

# Install the Node patch
RUN git clone $NODE_REPO
RUN cd node && ./configure && make -j4 && make install

# Add the fsracer user whose password is 'fsracer'
RUN useradd -ms /bin/bash fsracer && \
    echo fsracer:fsracer | chpasswd && \
    cp /etc/sudoers /etc/sudoers.bak && \
    echo 'fsracer ALL=(root) NOPASSWD:ALL' >> /etc/sudoers
USER fsracer
ENV HOME /home/fsracer

WORKDIR $HOME

# Install dynamo
RUN wget -O dynamo.tar.gz https://github.com/DynamoRIO/dynamorio/releases/download/cronbuild-7.90.17998/DynamoRIO-x86_64-Linux-7.90.17998-0.tar.gz
RUN tar -xvf dynamo.tar.gz && rm -rf dynamo.tar.gz && \
    mv DynamoRIO-x86_64-Linux-7.90.17998-0 dynamo

# Copy fsracer
# XXX: Clone the private repo instead of copying files.
RUN mkdir fsracer
COPY ./lib fsracer/lib
COPY ./tools fsracer/tools
COPY ./scripts fsracer/scripts
COPY ./tests fsracer/tests
COPY ./CMakeLists.txt fsracer/CMakeLists.txt

USER root
WORKDIR $HOME
RUN chown -R fsracer:fsracer fsracer

USER fsracer
WORKDIR $HOME/fsracer

# Build fsracer
RUN mkdir build && cd build && \
    cmake -DTEST_BINARY_PATH=/usr/local/bin/node -DDynamoRIO_BUILD_DIR=/home/fsracer/dynamo ..
WORKDIR $HOME/fsracer/build
RUN make fsracer

ENV DYNAMORIO_BIN=$HOME/dynamo/bin64

WORKDIR $HOME

COPY ./scripts/run_fsracer.sh /usr/local/bin/
