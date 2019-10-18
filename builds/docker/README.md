Dockerfile for fsmake analysis
==============================

Use sbuild

```
sbuild --apt-update --no-apt-upgrade --no-apt-distupgrade --batch --stats-dir=/var/log/sbuild/stats --dist=stable --arch=amd64 PACKAGE
```
