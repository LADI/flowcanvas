#!/usr/bin/env python
# Waf utilities for easily building standard unixey packages/libraries
# Licensed under the GNU GPL v2 or later, see COPYING file for details.
# Copyright (C) 2008-2010 David Robillard
# Copyright (C) 2008 Nedko Arnaudov

import os
import subprocess
import sys

import Configure
import Logs
import Options

from waflib import Context, Task, Utils, Node
from TaskGen import feature, before, after

global g_is_child
g_is_child = False

# Only run autowaf hooks once (even if sub projects call several times)
global g_step
g_step = 0

# Compute dependencies globally
#import preproc
#preproc.go_absolute = True

@feature('c', 'cxx')
@after('apply_incpaths')
def include_config_h(self):
	self.env.append_value('INCPATHS', self.bld.bldnode.abspath())

def set_options(opt):
	"Add standard autowaf options if they havn't been added yet"
	global g_step
	if g_step > 0:
		return
	opt.tool_options('compiler_cc')
	opt.tool_options('compiler_cxx')
	opt.add_option('--debug', action='store_true', default=False, dest='debug',
			help="Build debuggable binaries [Default: False]")
	opt.add_option('--strict', action='store_true', default=False, dest='strict',
			help="Use strict compiler flags and show all warnings [Default: False]")
	opt.add_option('--docs', action='store_true', default=False, dest='docs',
			help="Build documentation - requires doxygen [Default: False]")
	opt.add_option('--bundle', action='store_true', default=False,
			help="Build a self-contained bundle [Default: False]")
	opt.add_option('--bindir', type='string',
			help="Executable programs [Default: PREFIX/bin]")
	opt.add_option('--libdir', type='string',
			help="Libraries [Default: PREFIX/lib]")
	opt.add_option('--includedir', type='string',
			help="Header files [Default: PREFIX/include]")
	opt.add_option('--datadir', type='string',
			help="Shared data [Default: PREFIX/share]")
	opt.add_option('--configdir', type='string',
			help="Configuration data [Default: PREFIX/etc]")
	opt.add_option('--mandir', type='string',
			help="Manual pages [Default: DATADIR/man]")
	opt.add_option('--htmldir', type='string',
			help="HTML documentation [Default: DATADIR/doc/PACKAGE]")
	opt.add_option('--lv2-user', action='store_true', default=False, dest='lv2_user',
			help="Install LV2 bundles to user-local location [Default: False]")
	if sys.platform == "darwin":
		opt.add_option('--lv2dir', type='string',
				help="LV2 bundles [Default: /Library/Audio/Plug-Ins/LV2]")
	else:
		opt.add_option('--lv2dir', type='string',
				help="LV2 bundles [Default: LIBDIR/lv2]")
	g_step = 1

def check_header(conf, name, define='', mandatory=False):
	"Check for a header iff it hasn't been checked for yet"
	if type(conf.env['AUTOWAF_HEADERS']) != dict:
		conf.env['AUTOWAF_HEADERS'] = {}

	checked = conf.env['AUTOWAF_HEADERS']
	if not name in checked:
		checked[name] = True
		includes = '' # search default system include paths
		if sys.platform == "darwin":
			includes = '/opt/local/include'
		if define != '':
			conf.check_cxx(header_name=name, includes=includes, define_name=define, mandatory=mandatory)
		else:
			conf.check_cxx(header_name=name, includes=includes, mandatory=mandatory)

def nameify(name):
	return name.replace('/', '_').replace('++', 'PP').replace('-', '_').replace('.', '_')

def define(conf, var_name, value):
	conf.define(var_name, value)
	conf.env[var_name] = value

def check_pkg(conf, name, **args):
	if not 'mandatory' in args:
		args['mandatory'] = True
	"Check for a package iff it hasn't been checked for yet"
	var_name = 'HAVE_' + nameify(args['uselib_store'])
	check = not var_name in conf.env
	if not check and 'atleast_version' in args:
		# Re-check if version is newer than previous check
		checked_version = conf.env['VERSION_' + name]
		if checked_version and checked_version < args['atleast_version']:
			check = True;
	if check:
		conf.check_cfg(package=name, args="--cflags --libs", **args)
		found = bool(conf.env[var_name])
		if found:
			define(conf, var_name, int(found))
			if 'atleast_version' in args:
				conf.env['VERSION_' + name] = args['atleast_version']
		else:
			conf.undefine(var_name)
			if args['mandatory'] == True:
				conf.fatal("Required package " + name + " not found")

def configure(conf):
	global g_step
	if g_step > 1:
		return
	def append_cxx_flags(vals):
		conf.env.append_value('CFLAGS', vals.split())
		conf.env.append_value('CXXFLAGS', vals.split())
	print
	display_header('Global Configuration')
	conf.check_tool('compiler_cc')
	conf.check_tool('compiler_cxx')
	if Options.options.docs:
		conf.load('doxygen')
	conf.env['DOCS'] = Options.options.docs
	conf.env['DEBUG'] = Options.options.debug
	conf.env['STRICT'] = Options.options.strict
	conf.env['PREFIX'] = os.path.abspath(os.path.expanduser(os.path.normpath(conf.env['PREFIX'])))
	if Options.options.bundle:
		conf.env['BUNDLE'] = True
		define(conf, 'BUNDLE', 1)
		conf.env['BINDIR'] = conf.env['PREFIX']
		conf.env['INCLUDEDIR'] = os.path.join(conf.env['PREFIX'], 'Headers')
		conf.env['LIBDIR'] = os.path.join(conf.env['PREFIX'], 'Libraries')
		conf.env['DATADIR'] = os.path.join(conf.env['PREFIX'], 'Resources')
		conf.env['HTMLDIR'] = os.path.join(conf.env['PREFIX'], 'Resources/Documentation')
		conf.env['MANDIR'] = os.path.join(conf.env['PREFIX'], 'Resources/Man')
		conf.env['LV2DIR'] = os.path.join(conf.env['PREFIX'], 'PlugIns')
	else:
		conf.env['BUNDLE'] = False
		if Options.options.bindir:
			conf.env['BINDIR'] = Options.options.bindir
		else:
			conf.env['BINDIR'] = os.path.join(conf.env['PREFIX'], 'bin')
		if Options.options.includedir:
			conf.env['INCLUDEDIR'] = Options.options.includedir
		else:
			conf.env['INCLUDEDIR'] = os.path.join(conf.env['PREFIX'], 'include')
		if Options.options.libdir:
			conf.env['LIBDIR'] = Options.options.libdir
		else:
			conf.env['LIBDIR'] = os.path.join(conf.env['PREFIX'], 'lib')
		if Options.options.datadir:
			conf.env['DATADIR'] = Options.options.datadir
		else:
			conf.env['DATADIR'] = os.path.join(conf.env['PREFIX'], 'share')
		if Options.options.configdir:
			conf.env['CONFIGDIR'] = Options.options.configdir
		else:
			conf.env['CONFIGDIR'] = os.path.join(conf.env['PREFIX'], 'etc')
		if Options.options.htmldir:
			conf.env['HTMLDIR'] = Options.options.htmldir
		else:
			conf.env['HTMLDIR'] = os.path.join(conf.env['DATADIR'], 'doc', Context.g_module.APPNAME)
		if Options.options.mandir:
			conf.env['MANDIR'] = Options.options.mandir
		else:
			conf.env['MANDIR'] = os.path.join(conf.env['DATADIR'], 'man')
		if Options.options.lv2dir:
			conf.env['LV2DIR'] = Options.options.lv2dir
		else:
			if Options.options.lv2_user:
				if sys.platform == "darwin":
					conf.env['LV2DIR'] = os.path.join(os.getenv('HOME'), 'Library/Audio/Plug-Ins/LV2')
				else:
					conf.env['LV2DIR'] = os.path.join(os.getenv('HOME'), '.lv2')
			else:
				if sys.platform == "darwin":
					conf.env['LV2DIR'] = '/Library/Audio/Plug-Ins/LV2'
				else:
					conf.env['LV2DIR'] = os.path.join(conf.env['LIBDIR'], 'lv2')

	conf.env['BINDIRNAME'] = os.path.basename(conf.env['BINDIR'])
	conf.env['LIBDIRNAME'] = os.path.basename(conf.env['LIBDIR'])
	conf.env['DATADIRNAME'] = os.path.basename(conf.env['DATADIR'])
	conf.env['CONFIGDIRNAME'] = os.path.basename(conf.env['CONFIGDIR'])
	conf.env['LV2DIRNAME'] = os.path.basename(conf.env['LV2DIR'])

	if Options.options.docs:
		doxygen = conf.find_program('doxygen')
		if not doxygen:
			conf.fatal("Doxygen is required to build documentation, configure without --docs")

		dot = conf.find_program('dot')
		if not dot:
			conf.fatal("Graphviz (dot) is required to build documentation, configure without --docs")

	if Options.options.debug:
		conf.env['CFLAGS'] = [ '-O0', '-g' ]
		conf.env['CXXFLAGS'] = [ '-O0',  '-g' ]
	else:
		append_cxx_flags('-DNDEBUG')

	if Options.options.strict:
		conf.env.append_value('CFLAGS', [ '-std=c99', '-pedantic' ])
		conf.env.append_value('CXXFLAGS', [ '-ansi', '-Woverloaded-virtual', '-Wnon-virtual-dtor'])
		append_cxx_flags('-Wall -Wextra -Wno-unused-parameter')

	append_cxx_flags('-fPIC -DPIC -fshow-column')

	append_cxx_flags('-I' + os.path.abspath('build'))
	
	display_msg(conf, "Install prefix", conf.env['PREFIX'])
	display_msg(conf, "Debuggable build", str(conf.env['DEBUG']))
	display_msg(conf, "Strict compiler flags", str(conf.env['STRICT']))
	display_msg(conf, "Build documentation", str(conf.env['DOCS']))
	print

	g_step = 2

def set_local_lib(conf, name, has_objects):
	var_name = 'HAVE_' + nameify(name.upper())
	define(conf, var_name, 1)
	if has_objects:
		if type(conf.env['AUTOWAF_LOCAL_LIBS']) != dict:
			conf.env['AUTOWAF_LOCAL_LIBS'] = {}
		conf.env['AUTOWAF_LOCAL_LIBS'][name.lower()] = True
	else:
		if type(conf.env['AUTOWAF_LOCAL_HEADERS']) != dict:
			conf.env['AUTOWAF_LOCAL_HEADERS'] = {}
		conf.env['AUTOWAF_LOCAL_HEADERS'][name.lower()] = True

def append_property(obj, key, val):
	if hasattr(obj, key):
		setattr(obj, key, getattr(obj, key) + val)
	else:
		setattr(obj, key, val)

def use_lib(bld, obj, libs):
	abssrcdir = os.path.abspath('.')
	libs_list = libs.split()
	for l in libs_list:
		in_headers = l.lower() in bld.env['AUTOWAF_LOCAL_HEADERS']
		in_libs    = l.lower() in bld.env['AUTOWAF_LOCAL_LIBS']
		if in_libs:
			append_property(obj, 'use', ' lib%s ' % l.lower())
			append_property(obj, 'framework', bld.env['FRAMEWORK_' + l])
		if in_headers or in_libs:
			inc_flag = '-iquote ' + os.path.join(abssrcdir, l.lower())
			for f in ['CFLAGS', 'CXXFLAGS']:
				if not inc_flag in bld.env[f]:
					bld.env.append_value(f, inc_flag)
		else:
			append_property(obj, 'uselib', ' ' + l)

def display_header(title):
	Logs.pprint('BOLD', title)

def display_msg(conf, msg, status = None, color = None):
	color = 'CYAN'
	if type(status) == bool and status or status == "True":
		color = 'GREEN'
	elif type(status) == bool and not status or status == "False":
		color = 'YELLOW'
	Logs.pprint('BOLD', " *", sep='')
	Logs.pprint('NORMAL', "%s" % msg.ljust(conf.line_just - 3), sep='')
	Logs.pprint('BOLD', ":", sep='')
	Logs.pprint(color, status)

def link_flags(env, lib):
	return ' '.join(map(lambda x: env['LIB_ST'] % x, env['LIB_' + lib]))

def compile_flags(env, lib):
	return ' '.join(map(lambda x: env['CPPPATH_ST'] % x, env['INCLUDES_' + lib]))

def set_recursive():
	global g_is_child
	g_is_child = True

def is_child():
	global g_is_child
	return g_is_child

# Pkg-config file
def build_pc(bld, name, version, libs, subst_dict={}):
	'''Build a pkg-config file for a library.
	name    -- uppercase variable name     (e.g. 'SOMENAME')
	version -- version string              (e.g. '1.2.3')
	libs    -- string/list of dependencies (e.g. 'LIBFOO GLIB')
	'''
	pkg_prefix       = bld.env['PREFIX']
	if pkg_prefix[-1] == '/':
		pkg_prefix = pkg_prefix[:-1]

	obj = bld(features     = 'subst',
	          source       = name.lower() + '.pc.in',
	          target       = name.lower() + '.pc',
	          install_path = '${PREFIX}/${LIBDIRNAME}/pkgconfig',
	          exec_prefix  = '${prefix}',
	          PREFIX       = pkg_prefix,
	          EXEC_PREFIX  = '${prefix}',
	          LIBDIR       = '${prefix}/' + bld.env['LIBDIRNAME'],
	          INCLUDEDIR   = '${prefix}/include')

	if type(libs) != list:
		libs = libs.split()

	subst_dict[name + '_VERSION'] = version
	for i in libs:
		subst_dict[i + '_LIBS']   = link_flags(bld.env, i)
		lib_cflags = compile_flags(bld.env, i)
		if lib_cflags == '':
			lib_cflags = ' '
		subst_dict[i + '_CFLAGS'] = lib_cflags

	obj.__dict__.update(subst_dict)

# Doxygen API documentation
def build_dox(bld, name, version, srcdir, blddir):
	if not bld.env['DOCS']:
		return

	if is_child():
		src_dir = os.path.join(srcdir, name.lower())
		doc_dir = os.path.join(blddir, name.lower(), 'doc')
	else:
		src_dir = srcdir
		doc_dir = os.path.join(blddir, 'doc')

	subst_tg = bld(features     = 'subst',
	               source       = 'doc/reference.doxygen.in',
	               target       = 'doc/reference.doxygen',
	               install_path = '',
	               name         = 'doxyfile')

	subst_dict = {
		name + '_VERSION' : version,
		name + '_SRCDIR'  : os.path.abspath(src_dir),
		name + '_DOC_DIR' : os.path.abspath(doc_dir)
	}

	subst_tg.__dict__.update(subst_dict)

	subst_tg.post()

	docs = bld(features = 'doxygen',
	           doxyfile = 'doc/reference.doxygen')

	docs.post()

	bld.install_files('${HTMLDIR}',     bld.path.get_bld().ant_glob('doc/html/*'))
	bld.install_files('${MANDIR}/man1', bld.path.get_bld().ant_glob('doc/man/man1/*'))
	bld.install_files('${MANDIR}/man3', bld.path.get_bld().ant_glob('doc/man/man3/*'))


# Version code file generation
def build_version_files(header_path, source_path, domain, major, minor, micro):
	header_path = os.path.abspath(header_path)
	source_path = os.path.abspath(source_path)
	text  = "int " + domain + "_major_version = " + str(major) + ";\n"
	text += "int " + domain + "_minor_version = " + str(minor) + ";\n"
	text += "int " + domain + "_micro_version = " + str(micro) + ";\n"
	try:
		o = file(source_path, 'w')
		o.write(text)
		o.close()
	except IOError:
		print "Could not open", source_path, " for writing\n"
		sys.exit(-1)

	text  = "#ifndef __" + domain + "_version_h__\n"
	text += "#define __" + domain + "_version_h__\n"
	text += "extern const char* " + domain + "_revision;\n"
	text += "extern int " + domain + "_major_version;\n"
	text += "extern int " + domain + "_minor_version;\n"
	text += "extern int " + domain + "_micro_version;\n"
	text += "#endif /* __" + domain + "_version_h__ */\n"
	try:
		o = file(header_path, 'w')
		o.write(text)
		o.close()
	except IOError:
		print "Could not open", header_path, " for writing\n"
		sys.exit(-1)

	return None

def run_tests(ctx, appname, tests):
	orig_dir = os.path.abspath(os.curdir)
	failures = 0
	base = '.'

	top_level = (len(ctx.stack_path) > 1)
	if top_level:
		os.chdir('./build/' + appname)
		base = '..'
	else:
		os.chdir('./build')

	lcov = True
	lcov_log = open('lcov.log', 'w')
	try:
		# Clear coverage data
		subprocess.call('lcov -d ./src -z'.split(),
				stdout=lcov_log, stderr=lcov_log)
	except:
		lcov = False
		print "Failed to run lcov, no coverage report will be generated"


	# Run all tests
	for i in tests:
		print
		Logs.pprint('BOLD', 'Running %s test %s' % (appname, i))
		if subprocess.call(i) == 0:
			Logs.pprint('GREEN', 'Passed %s %s' % (appname, i))
		else:
			failures += 1
			Logs.pprint('RED', 'Failed %s %s' % (appname, i))

	if lcov:
		# Generate coverage data
		coverage_lcov = open('coverage.lcov', 'w')
		subprocess.call(('lcov -c -d ./src -d ./test -b ' + base).split(),
				stdout=coverage_lcov, stderr=lcov_log)
		coverage_lcov.close()

		# Strip out unwanted stuff
		coverage_stripped_lcov = open('coverage-stripped.lcov', 'w')
		subprocess.call('lcov --remove coverage.lcov *boost* c++*'.split(),
				stdout=coverage_stripped_lcov, stderr=lcov_log)
		coverage_stripped_lcov.close()

		# Generate HTML coverage output
		if not os.path.isdir('./coverage'):
			os.makedirs('./coverage')
		subprocess.call('genhtml -o coverage coverage-stripped.lcov'.split(),
				stdout=lcov_log, stderr=lcov_log)

	lcov_log.close()

	print
	Logs.pprint('BOLD', 'Summary:', sep=''),
	if failures == 0:
		Logs.pprint('GREEN', 'All ' + appname + ' tests passed')
	else:
		Logs.pprint('RED', str(failures) + ' ' + appname + ' test(s) failed')

	Logs.pprint('BOLD', 'Coverage:', sep='')
	print os.path.abspath('coverage/index.html')

	os.chdir(orig_dir)

def run_ldconfig(ctx):
	if ctx.cmd == 'install':
		print 'Running /sbin/ldconfig'
		try:
			os.popen("/sbin/ldconfig")
		except:
			print >> sys.stderr, 'Error running ldconfig, libraries may not be linkable'

