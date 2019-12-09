fsracer builds
==============================

Docker Make
-----------

Build

```
docker build -t fsmake .
```

Run

```
docker run -it -v $(pwd)/traces:/root/traces --cap-add SYS_ADMIN fsmake PACKAGE
```

Debian Docker sbuild fsmake
---------------------------

* Build

```bash
docker build -f strace.Dockerfile -t sbuild_fsmake .
```

* Run

```bash
docker run -it -v $(pwd)/out:/results --privileged sbuild_fsmake PACKAGE
```

* Possible Results
    - `straces/PACKAGE/PACKAGE.[anerr,conferr,strerr,straces]`
    
Adapter
-------

```bash
adapter.py -c make -m out/PACKAGE/PACKAGE.makedb /build/PACKAGE-XMBMdq/PACKAGE-1.71 < out/PACKAGE/PACKAGE.straces > out/PACKAGE/PACKAGE.fstraces
```

fsracer
-------

```bash
docker run --rm -v $(pwd)/out/PACKAGE:/home/fsracer/PACKAGE fsracer run_fsracer.sh PACKAGE
```

mkcheck
-------

```bash
docker run -it --privileged -v $(pwd)/results:/results mkcheck PACKAGE
```
