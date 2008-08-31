all: calfpytools.cpp ttl.cpp ttl.h Makefile setup.py ttldata.h
	python setup.py build

ttl.h ttl.cpp: ttl.l Makefile ttldata.h
	flex --batch --nodefault --c++ -o ttl.cpp --header-file=ttl.h ttl.l

install:
	python setup.py install
