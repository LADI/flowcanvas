#!/bin/sh

cd ext

echo
echo "**** Generating code documentation with doxygen"
doxygen Doxyfile

rm index.html
echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>" >> index.html 
echo "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML+RDFa 1.0//EN\" \"http://www.w3.org/MarkUp/DTD/xhtml-rdfa-1.dtd\">" >> index.html
echo "<html xmlns=\"http://www.w3.org/1999/xhtml\"><head>" >> index.html
echo "<meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml;charset=utf-8\"/>" >> index.html
echo "<title>LV2 Extensions</title>" >> index.html
echo "<style type=\"text/css\">" >> index.html
cat ../specgen/style.css >> index.html
echo "</style></head>" >> index.html
echo "<body><h1>LV2 Extensions</h1><ul>" >> index.html

for bundle in `find -name '*.lv2'`; do
	b=`echo $bundle | sed 's/\.lv2$//' | sed 's/^.*\///'`
	if [ -e $b.lv2/$b.ttl ]; then
		echo
		echo "**** Generating RDF documentation $b.html"
		../specgen/lv2specgen.py $b.lv2/$b.ttl dman ../specgen/template.html ../specgen/style.css $b.lv2/$b.html -i;
		echo "<li><a rel=\"rdfs:seeAlso\" href=\"$b.lv2/$b.html\">$b</a></li>" >> index.html;
	fi
	if [ -e $b.lv2/$b.h ]; then
		echo
		echo "**** Copying code documentation $b.h.html"
	fi
done

echo "</ul><hr/>" >> index.html
cat ../specgen/footer.html >> index.html
echo "</body></html>" >> index.html
