#!/bin/sh

rm -rf upload
mkdir upload

URIPREFIX=http://lv2plug.in/ns/
	
echo "**** Generating core documentation"
SPECGENDIR=./specgen
mkdir upload/lv2core
cp core.lv2/lv2.h upload/lv2core
cp core.lv2/lv2.ttl upload/lv2core
cp core.lv2/manifest.ttl upload/lv2core
$SPECGENDIR/lv2specgen.py core.lv2/lv2.ttl $SPECGENDIR/template.html $SPECGENDIR/style.css upload/lv2core/lv2core.html -i;
cd core.lv2
doxygen
cd ..

for dir in ext dev; do
	echo
	echo "**** Generating $dir documentation"

	mkdir upload/$dir
	cp -r $dir upload
	rm -rf `find upload/$dir -name '.svn'`
	cd upload/$dir
	SPECGENDIR=../../specgen

	doxygen Doxyfile
	
	echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>" > index.html 
	echo "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML+RDFa 1.0//EN\" \"http://www.w3.org/MarkUp/DTD/xhtml-rdfa-1.dtd\">" >> index.html
	echo "<html xmlns=\"http://www.w3.org/1999/xhtml\"><head>" >> index.html
	echo "<meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml;charset=utf-8\"/>" >> index.html
	echo "<title>LV2 Extensions</title>" >> index.html
	echo "<style type=\"text/css\">" >> index.html
	cat $SPECGENDIR/style.css >> index.html
	echo "</style></head>" >> index.html
	echo "<body><h1>LV2 Extensions</h1>" >> index.html
	echo "<h2>$URIPREFIX$dir/</h2><ul>" >> index.html
	
	for bundle in `find -name '*.lv2'`; do
		b=`echo $bundle | sed 's/\.lv2$//' | sed 's/^.*\///'`
		if [ -e $b.lv2/$b.ttl ]; then
			echo
			echo "**** Generating RDF documentation $b.html"
			$SPECGENDIR/lv2specgen.py $b.lv2/$b.ttl $SPECGENDIR/template.html $SPECGENDIR/style.css $b.lv2/$b.html -i;
			echo "<li><a rel=\"rdfs:seeAlso\" href=\"$b.lv2/$b.html\">$b</a></li>" >> index.html;
		fi
		if [ -e $b.lv2/$b.h ]; then
			echo
			echo "**** Copying code documentation $b.h.html"
		fi
	done
	
	echo "</ul><hr/>" >> index.html
	cat $SPECGENDIR/footer.html >> index.html
	echo "</body></html>" >> index.html

	cd ../..
done

