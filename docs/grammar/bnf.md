fsracer ABNF
============

```
T           =   Operations* Blocks*
Operation   =   'Operation' id 'do' Opexp* 'Done'
Opexp       =   'setcwd' PATH
            |   'newfd' dirfd PATH DIGIT
            |   'hpath' dirfd PATH access
dirfd       =   AT_FDCWD / NUMBER
access      =   'consumed' / 'produced' / 'expunged'
id          =   NUMBER
```

Translation Rules
=================

A number means the X argument of a system call

```
access -> hpath AT_FDCWD 0 consumed;
```
