#!/bin/sh

cd ext

doxygen Doxyfile

for b in dyn-manifest port-groups uri-map; do
	if [ -x $b.lv2/$b.ttl ]; then
		../specgen/lv2specgen.py $b.lv2/$b.ttl dman ../specgen/template.html $b.lv2/$b.html -i;
	fi
	if [ -x $b.lv2/$b.h ]; then
		cp doc/html/${b}_8h.html $b.lv2/$b.h.html;
	fi
done

