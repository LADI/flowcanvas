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
cp index.php upload/lv2core
$SPECGENDIR/lv2specgen.py core.lv2/lv2.ttl $SPECGENDIR/template.html $SPECGENDIR/style.css upload/lv2core/lv2core.html -i;
cd core.lv2
doxygen
cd ..

for dir in ext dev extensions; do
	echo
	echo "**** Generating $dir documentation"

	mkdir upload/$dir
    mkdir upload/$dir/releases
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
		ext=`roqet -q -e "
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
SELECT ?ext FROM <$b.lv2/$b.ttl> WHERE { ?ext a lv2:Specification }"`
		if [ "$ext" != "" ]; then
			ext=`echo $ext | sed 's/.*ext=uri<\(.*\)>.*/\1/'`
			rev=`roqet -q -e "
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
PREFIX doap: <http://usefulinc.com/ns/doap#>
SELECT ?rev FROM <$b.lv2/$b.ttl> WHERE { <$ext> doap:release [ doap:revision ?rev ] }"`
			if [ "$rev" != "" ]; then
				rev=`echo $rev | sed 's/.*rev=string("\(.*\)").*/\1/'`
			else
                rev="1"
			fi
			tar -czf releases/$b.lv2-$rev.tgz $b.lv2
		fi
		if [ -e $b.lv2/$b.ttl ]; then
			echo
			echo "**** Generating XHTML schema documentation for $b"
			$SPECGENDIR/lv2specgen.py $b.lv2/$b.ttl $SPECGENDIR/template.html $SPECGENDIR/style.css $b.lv2/$b.html -i;
			echo "<li><a rel=\"rdfs:seeAlso\" href=\"$b\">$b</a></li>" >> index.html;
		fi
		if [ -e $b.lv2/$b.h ]; then
			echo
			echo "**** Copying code documentation $b.h.html"
		fi
        mv $b.lv2 $b
		cp ../../index.php $b
	done
	
	echo "</ul>" >> index.html

	echo "<div><a href=\"./releases\">Releases</a></div>" >> index.html

	echo "<hr/>" >> index.html
	cat $SPECGENDIR/footer.html >> index.html
	echo "</body></html>" >> index.html

	rm Doxyfile

	cd ../..
done

