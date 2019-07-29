FSRacer
=======

A client that detects data races in file system resources
on top of the [DynamoRIO](https://www.dynamorio.org) framework.


# Setup

## Install Dependencies

```shell
# Install GNU gengetopt to autogenerate command-line interface.
sudo apt install gengetopt
```

## Download DynamoRIO

First, download the binary package of the DynamoRIO core:

```shell
wget -O dynamo.tar.gz https://github.com/DynamoRIO/dynamorio/releases/download/cronbuild-7.90.17998/DynamoRIO-x86_64-Linux-7.90.17998-0.tar.gz
tar -xvf dynamo.tar.gz
```

## Build the patched Node binary

Build the patched `Node` binary from [this](https://github.com/theosotr/node) repo
by following the official [instructions](https://github.com/theosotr/node/blob/v10.15.3-patch/BUILDING.md#unixmacos).


## Build FSRacer 

To build `FSRacer` run the following commands:

```shell
mkdir build
cd build
cmake -DTEST_BINARY_PATH=<path to the patched node binary> -DDynamoRIO_BUILD_DIR=<path to the build directory of DynamoRIO> ..
make
```

Note that the option `-DTEST_BINARY_PATH` corresponds to the path
where the `Node` binary is located as produced by the previous step.
On the other hand, the `-DDynamoRIO_BUILD_DIR` is the path to the
directory where the installation of the `DynamoRIO` is placed.


# Run FSRacer

Run the `FSRacer` client as follows:

```shell
<dynamorio binary> -c build/libfsracer.so -- <node binary> <node program>
```

Run tests through:

```shell
make test
```
