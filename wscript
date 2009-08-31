#!/usr/bin/env python
import autowaf
import Options

# Version of this package (even if built as a child)
FLOWCANVAS_VERSION = '0.5.3'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Version history:
#   0.4.0 = 0,0,0
#   0.5.0 = 1,0,0
#   0.5.2 = 2,0,0
#   0.5.3 = 2,1,0
FLOWCANVAS_LIB_VERSION = '2.1.0'

# Variables for 'waf dist'
APPNAME = 'flowcanvas'
VERSION = FLOWCANVAS_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.add_option('--anti-alias', action='store_false', default=True, dest='anti_alias',
			help="Anti-alias canvas (much prettier but slower) [Default: True]")

def configure(conf):
	autowaf.configure(conf)
	conf.check_tool('compiler_cxx')
	autowaf.check_pkg(conf, 'libgvc', uselib_store='AGRAPH', atleast_version='2.8', mandatory=False)
	autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='GLIBMM', atleast_version='2.10.0', mandatory=True)
	autowaf.check_pkg(conf, 'libgnomecanvasmm-2.6', uselib_store='GNOMECANVASMM', atleast_version='2.6.0', mandatory=True)

	# Boost headers
	autowaf.check_header(conf, 'boost/shared_ptr.hpp', mandatory=True)
	autowaf.check_header(conf, 'boost/weak_ptr.hpp', mandatory=True)
	
	conf.write_config_header('flowcanvas-config.h')
	conf.env['ANTI_ALIAS'] = bool(Options.options.anti_alias)

	autowaf.print_summary(conf)
	autowaf.display_header('FlowCanvas Configuration')
	autowaf.display_msg(conf, "Auto-arrange", str(conf.env['HAVE_AGRAPH'] == 1))
	autowaf.display_msg(conf, "Anti-Aliasing", str(bool(conf.env['ANTI_ALIAS'])))
	print

def build(bld):
	# Headers
	bld.install_files('${INCLUDEDIR}/flowcanvas', 'flowcanvas/*.hpp')

	# Pkgconfig file
	autowaf.build_pc(bld, 'FLOWCANVAS', FLOWCANVAS_VERSION, 'AGRAPH GLIBMM GNOMECANVASMM')

	# Library
	obj = bld.new_task_gen('cxx', 'shlib')
	obj.export_incdirs = ['.']
	obj.source = '''
		src/Canvas.cpp
		src/Connectable.cpp
		src/Connection.cpp
		src/Ellipse.cpp
		src/Item.cpp
		src/Module.cpp
		src/Port.cpp
	'''
	obj.includes     = ['.', './src']
	obj.name         = 'libflowcanvas'
	obj.target       = 'flowcanvas'
	obj.uselib       = 'GTKMM GNOMECANVASMM AGRAPH'
	obj.vnum         = FLOWCANVAS_LIB_VERSION
	obj.install_path = '${LIBDIR}'

	# Documentation
	autowaf.build_dox(bld, 'FLOWCANVAS', FLOWCANVAS_VERSION, srcdir, blddir)
	bld.install_files('${HTMLDIR}', blddir + '/default/doc/html/*')

def shutdown():
	autowaf.shutdown()

