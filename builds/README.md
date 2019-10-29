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
