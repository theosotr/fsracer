fsracer builds
==============================

Docker
------

Build

```
docker build -t fs .
```

Run

```
docker run -it -v $(pwd)/traces:/root/traces --cap-add SYS_ADMIN fs PACKAGE
```
