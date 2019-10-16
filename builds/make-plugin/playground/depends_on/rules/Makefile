.PHONY: all

PAT=s2*

all: s5 s4 s1
	@echo Making $@

s1: s3 $(PAT)
	touch t1

$(PAT):
	cp t3 t2

s3:
	sleep 1
	touch t3

s4:
	touch t4

s5:
	touch t5

clean:
	rm -rf t*
