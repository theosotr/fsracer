FSRacer
=======

A client that detects data races in file system resources
on top of the [DynamoRIO](https://www.dynamorio.org) framework.


## Setup

First, download the binary package of the DynamoRIO core:

```
wget -O dynamo.tar.gz https://github.com/DynamoRIO/dynamorio/releases/download/release_7.1.0/DynamoRIO-Linux-7.1.0-1.tar.gz
tar -xvf dynamo.tar.gz
```

Set the environment variable `$DynamoRIO_BUILD_DIR`
that points to the directory of the extracted archive
(See the previous step).

Then, build the FSracer client by executing the
following instructions inside the root directory
of this repository:

```
mkdir build
cd build
cmake -DDynamoRIO_DIR=${Dynamo_BUILD_DIR}/cmake ..
make
```

After the build, a file named `libfsracer.so` is created
inside the `build` directory.


## Example

Run the FSracer client in an example JavaScript program:

```
${Dynamo_BUILD_DIR}/bin64/drrun -c build/libfsracer.so -- node examples/timers.js
```

This will produce a trace file named `fsracer.trace`.
