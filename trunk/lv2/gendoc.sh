#!/bin/sh

cd ext

echo
echo "**** Generating code documentation with doxygen"
doxygen Doxyfile

for bundle in `find -name '*.lv2'`; do
	b=`echo $bundle | sed 's/\.lv2$//' | sed 's/^.*\///'`
	if [ -e $b.lv2/$b.ttl ]; then
		echo
		echo "**** Generating RDF documentation $b.html"
		../specgen/lv2specgen.py $b.lv2/$b.ttl dman ../specgen/template.html $b.lv2/$b.html -i;
	fi
	if [ -e $b.lv2/$b.h ]; then
		echo
		echo "**** Copying code documentation $b.h.html"
		cp doc/html/${b}_8h.html $b.lv2/$b.h.html;
	fi
done

