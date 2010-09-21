#!/usr/bin/env python

import os
import shutil
import subprocess
import glob
import re

shutil.rmtree('upload')
os.mkdir('upload')

URIPREFIX  = 'http://lv2plug.in/ns/'
SPECGENDIR = './specgen'

print '**** Generating core documentation'

os.mkdir('upload/lv2core')
shutil.copy('core.lv2/lv2.h',        'upload/lv2core')
shutil.copy('core.lv2/lv2.ttl',      'upload/lv2core')
shutil.copy('core.lv2/manifest.ttl', 'upload/lv2core')
shutil.copy('index.php',             'upload/lv2core')

devnull = open(os.devnull, 'w')

def gendoc(specgen_dir, bundle_dir, ttl_filename, html_filename):
    subprocess.call([os.path.join(specgen_dir, 'lv2specgen.py'),
              os.path.join(bundle_dir, ttl_filename),
              os.path.join(specgen_dir, 'template.html'),
              os.path.join(specgen_dir, 'style.css'),
              os.path.join('upload', html_filename),
              '-i'])
    subprocess.call('doxygen', cwd=bundle_dir, stdout=devnull)

gendoc('./specgen', 'core.lv2', 'lv2.ttl', 'lv2core/lv2core.html')

style = open('./specgen/style.css', 'r')
footer = open('./specgen/footer.html', 'r')

for dir in ['ext', 'dev', 'extensions']:
    print "**** Generating %s documentation" % dir

    outdir = os.path.join('upload', dir)

    shutil.copytree(dir, outdir, ignore = lambda src, names: '.svn')
    os.mkdir(os.path.join(outdir, 'releases'))

    subprocess.call(['doxygen', 'Doxyfile'], cwd=outdir, stdout=devnull)
    
    index_html = """
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML+RDFa 1.0//EN" "http://www.w3.org/MarkUp/DTD/xhtml-rdfa-1.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="application/xhtml+xml;charset=utf-8"/>
<title>LV2 Extensions</title>
<style type="text/css">
"""

    index_html += style.read()

    index_html += """
</style></head>
<body><h1>LV2 Extensions</h1>
<h2>""" + URIPREFIX + dir + "/</h2><ul>\n"

    for bundle in glob.glob(os.path.join(dir, '*.lv2')):
        b = bundle.replace('.lv2', '')
        b = b[b.find('/') + 1:]

        # Get extension URI
        ext = subprocess.Popen(['roqet', '-q', '-e', """
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
SELECT ?ext FROM <%s.lv2/%s.ttl> WHERE { ?ext a lv2:Specification }
""" % (os.path.join(dir, b), b)], stdout=subprocess.PIPE).communicate()[0]

        if ext == "":
            continue

        ext = re.sub('^result: \[ext=uri<', '', ext)
        ext = re.sub('>\]$', '', ext).strip()

        # Get revision
        query = """
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
PREFIX doap: <http://usefulinc.com/ns/doap#>
SELECT ?rev FROM <%s.lv2/%s.ttl> WHERE { <%s> doap:release [ doap:revision ?rev ] }
""" % (os.path.join(dir, b), b, ext)

        rev = subprocess.Popen(['roqet', '-q', '-e', query],
                               stdout=subprocess.PIPE).communicate()[0]

        if rev != '':
            rev = re.sub('^result: \[rev=string\("', '', rev)
            rev = re.sub('"\)\]$', '', rev).strip()
        else:
            rev = '0'

        subprocess.call(['tar', '-czf', outdir + '/releases/%s.lv2-%s.tgz' % (b, rev),
                         outdir + '/%s.lv2' % b])
            

        print '****', b
        specgendir = '../../specgen/'
        if (os.access(outdir + '/%s.lv2/%s.ttl' % (b, b), os.R_OK)):
            print '  ** Generating ontology documentation %s/%s.html' % (b, b)
            subprocess.call([specgendir + 'lv2specgen.py',
                             '%s.lv2/%s.ttl' % (b, b),
                             specgendir + 'template.html',
                             specgendir + 'style.css',
                             '%s.lv2/%s.html' % (b, b),
                             '-i'], cwd=outdir);

            index_html += '<li><a rel="rdfs:seeAlso" href="%s">%s</a>' % (b, b)
            if rev == '0':
                index_html += '<span style="color: red;"> (proposal)</span>'
            index_html += '</li>\n'

        shutil.copy('index.php', os.path.join(outdir, b + '.lv2', 'index.php'))
    
        # Remove .lv2 suffix from bundle name (to make URI resolvable)
        os.rename(outdir + '/%s.lv2' % b, outdir + '/%s' % b)
    
    index_html += '</ul>\n'
    index_html += '<div><a href="./releases">Releases</a></div>\n'
    index_html += '<hr/>\n'

    index_html += footer.read()

    index_html += '</body></html>\n'

    index_file = open(os.path.join(outdir, 'index.html'), 'w')
    print >>index_file, index_html
    index_file.close()

    os.remove(os.path.join(outdir, 'Doxyfile'))

devnull.close()
style.close()
footer.close()
