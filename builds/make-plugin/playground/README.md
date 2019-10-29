Install
=======

```
sudo install fsmake-shell /usr/local/bin
cp syscalls.txt ~/syscalls.txt
```

Makefile with missing dependency
================================

* Missing **s2.c** or use `%.o: %.c`

```make
$(OBJECTS): main.c s1.c s1.h s2.h
        $(CC) -c main.c s1.c s2.c $(CFLAGS)
```

Examples
========

* `make`

* `make -f mOBJ`

* `make SHELL='fsmake-shell '\''$(MAKEFILE_LIST)'\'' '\''$@'\'' '\''$^'\'''`

* `strace -s 1000 -f -e "$(tr -s '\r\n' ',' < ~/syscalls.txt | sed -e 's/,$/\n/')" -o make.traces SHELL='fsmake-shell '\''$(MAKEFILE_LIST)'\'' '\''$@'\'' '\''$^'\'''`
