#!/usr/bin/env python
import Params
import autowaf

# Version of this package (even if built as a child)
FLOWCANVAS_VERSION = '0.5.2'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Version history:
#   0.4.0 = 0,0,0
#   0.5.0 = 1,0,0
#   0.5.1 = 2,0,0
FLOWCANVAS_LIB_VERSION = '2.0.0'

# Variables for 'waf dist'
APPNAME = 'flowcanvas'
VERSION = FLOWCANVAS_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.add_option('--anti-alias', action='store_false', default=True, dest='anti_alias',
			help="Anti-alias canvas (much prettier butslower) [Default: True]")

def configure(conf):
	autowaf.configure(conf)
	autowaf.check_tool(conf, 'compiler_cxx')
	autowaf.check_pkg(conf, 'libgvc', destvar='AGRAPH', vnum='2.8', mandatory=False)
	autowaf.check_pkg(conf, 'gtkmm-2.4', destvar='GLIBMM', vnum='2.10.0', mandatory=True)
	autowaf.check_pkg(conf, 'libgnomecanvasmm-2.6', destvar='GNOMECANVASMM', vnum='2.6.0', mandatory=True)

	autowaf.check_header(conf, 'boost/shared_ptr.hpp')
	autowaf.check_header(conf, 'boost/weak_ptr.hpp')
	conf.write_config_header('config.h')
	conf.env['ANTI_ALIAS'] = bool(Params.g_options.anti_alias)
	
	autowaf.print_summary(conf)
	autowaf.display_header('FlowCanvas Configuration')
	autowaf.display_msg("Auto-arrange", str(conf.env['HAVE_AGRAPH'] == 1), 'YELLOW')
	autowaf.display_msg("Anti-Aliasing", str(bool(conf.env['ANTI_ALIAS'])), 'YELLOW')
	print

def build(bld):
	# Headers
	install_files('INCLUDEDIR', 'flowcanvas', 'flowcanvas/*.hpp')
	
	# Pkgconfig file
	autowaf.build_pc(bld, 'FLOWCANVAS', FLOWCANVAS_VERSION, 'AGRAPH GLIBMM GNOMECANVASMM')
	
	# Library
	obj = bld.create_obj('cpp', 'shlib')
	obj.source = '''
		src/Canvas.cpp 
		src/Connectable.cpp 
		src/Connection.cpp 
		src/Ellipse.cpp 
		src/Item.cpp 
		src/Module.cpp 
		src/Port.cpp
	'''
	obj.includes = ['.']
	obj.name     = 'libflowcanvas'
	obj.target   = 'flowcanvas'
	obj.uselib   = 'GTKMM GNOMECANVASMM AGRAPH'
	obj.vnum     = FLOWCANVAS_LIB_VERSION
	obj.inst_dir = bld.env()['LIBDIRNAME']
	
	# Documentation
	autowaf.build_dox(bld, 'FLOWCANVAS', FLOWCANVAS_VERSION, srcdir, blddir)
	install_files('HTMLDIR', '', blddir + '/default/doc/html/*')

def shutdown():
	autowaf.shutdown()

