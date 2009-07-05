#!/bin/sh

cd ext

doxygen Doxyfile

../specgen/lv2specgen.py dyn-manifest/dyn-manifest.ttl dman ../specgen/template.html dyn-manifest/dyn-manifest.html -i

cp doc/html/dyn-manifest_8h.html dyn-manifest/dyn-manifest.h.html

cd dyn-manifest && ln -fs ../doc/html/doxygen.css doxygen.css

cd -
