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

Debian Docker Make
------------------

* Build

```bash
docker build -f debian_make.Dockerfile -t sbuild_fsmake .
```

* Run

```bash
docker run -it -v $(pwd)/straces:/var/log/sbuild/straces --cap-add SYS_ADMIN sbuild_fsmake PACKAGE
```

* Possible Results
    - `straces/PACKAGE/PACKAGE.[anerr,conferr,strerr,straces]`

mkcheck
-------

```bash
docker run -it --privileged -v $(pwd)/results:/results mkcheck PACKAGE
```
