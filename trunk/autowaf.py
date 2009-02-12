#!/usr/bin/env python
# Waf utilities for easily building standard unixey packages/libraries
# Licensed under the GNU GPL v2 or later, see COPYING file for details.
# Copyright (C) 2008 Dave Robillard
# Copyright (C) 2008 Nedko Arnaudov

import os
import misc
import Configure
import Options
import Utils
import sys
from TaskGen import feature, before, after

global g_is_child
g_is_child = False

# Only run autowaf hooks once (even if sub projects call several times)
global g_step
g_step = 0

# Compute dependencies globally
#import preproc
#preproc.go_absolute = True

@feature('cc', 'cxx')
@after('apply_lib_vars')
@before('apply_obj_vars_cc', 'apply_obj_vars_cxx')
def include_config_h(self):
	self.env.append_value('INC_PATHS', self.bld.srcnode)

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
	opt.add_option('--build-docs', action='store_true', default=False, dest='build_docs',
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
		if define != '':
			conf.check(header_name=name, define_name=define, mandatory=mandatory)
		else:
			conf.check(header_name=name, mandatory=mandatory)

def check_tool(conf, name):
	"Check for a tool iff it hasn't been checked for yet"
	if type(conf.env['AUTOWAF_TOOLS']) != dict:
		conf.env['AUTOWAF_TOOLS'] = {}

	checked = conf.env['AUTOWAF_TOOLS']
	if not name in checked:
		conf.check_tool(name)
		checked[name] = True

def check_pkg(conf, name, **args):
	"Check for a package iff it hasn't been checked for yet"
	var_name = 'HAVE_' + args['uselib_store']
	check = not var_name in conf.env
	if not check and 'atleast_version' in args:
		# Re-check if version is newer than previous check
		checked_version = conf.env['VERSION_' + name]
		if checked_version and checked_version < args['atleast_version']:
			check = True;
	if check:
		conf.check_cfg(package=name, args="--cflags --libs", **args)
		found = bool(conf.env['HAVE_' + args['uselib_store']])
		if found:
			conf.define('HAVE_' + args['uselib_store'], int(found))
			if 'atleast_version' in args:
				conf.env['VERSION_' + name] = args['atleast_version']
		else:
			conf.undefine('HAVE_' + args['uselib_store'])
			if args['mandatory'] == True:
				conf.fatal("Required package " + name + " not found")

def chop_prefix(conf, var):
	name = conf.env[var][len(conf.env['PREFIX']):]
	if len(name) > 0 and name[0] == '/':
		name = name[1:]
	if name == "":
		name = "/"
	return name;

def configure(conf):
	global g_step
	if g_step > 1:
		return
	def append_cxx_flags(val):
		conf.env.append_value('CCFLAGS', val)
		conf.env.append_value('CXXFLAGS', val)
	check_tool(conf, 'misc')
	check_tool(conf, 'compiler_cc')
	check_tool(conf, 'compiler_cxx')
	conf.env['BUILD_DOCS'] = Options.options.build_docs
	conf.env['DEBUG'] = Options.options.debug
	conf.env['PREFIX'] = os.path.abspath(os.path.expanduser(os.path.normpath(conf.env['PREFIX'])))
	if Options.options.bundle:
		conf.env['BUNDLE'] = True
		conf.define('BUNDLE', 1)
		conf.env['BINDIR'] = conf.env['PREFIX']
		conf.env['INCLUDEDIR'] = conf.env['PREFIX'] + '/Headers/'
		conf.env['LIBDIR'] = conf.env['PREFIX'] + '/Libraries/'
		conf.env['DATADIR'] = conf.env['PREFIX'] + '/Resources/'
		conf.env['HTMLDIR'] = conf.env['PREFIX'] + '/Resources/Documentation/'
		conf.env['MANDIR'] = conf.env['PREFIX'] + '/Resources/Man/'
		conf.env['LV2DIR'] = conf.env['PREFIX'] + '/PlugIns/'
	else:
		conf.env['BUNDLE'] = False
		if Options.options.bindir:
			conf.env['BINDIR'] = Options.options.bindir
		else:
			conf.env['BINDIR'] = conf.env['PREFIX'] + '/bin/'
		if Options.options.includedir:
			conf.env['INCLUDEDIR'] = Options.options.includedir
		else:
			conf.env['INCLUDEDIR'] = conf.env['PREFIX'] + '/include/'
		if Options.options.libdir:
			conf.env['LIBDIR'] = Options.options.libdir
		else:
			conf.env['LIBDIR'] = conf.env['PREFIX'] + '/lib/'
		if Options.options.datadir:
			conf.env['DATADIR'] = Options.options.datadir
		else:
			conf.env['DATADIR'] = conf.env['PREFIX'] + '/share/'
		if Options.options.htmldir:
			conf.env['HTMLDIR'] = Options.options.htmldir
		else:
			conf.env['HTMLDIR'] = conf.env['DATADIR'] + 'doc/' + Utils.g_module.APPNAME + '/'
		if Options.options.mandir:
			conf.env['MANDIR'] = Options.options.mandir
		else:
			conf.env['MANDIR'] = conf.env['DATADIR'] + 'man/'
		if Options.options.lv2dir:
			conf.env['LV2DIR'] = Options.options.lv2dir
		else:
			if Options.options.lv2_user:
				if sys.platform == "darwin":
					conf.env['LV2DIR'] = os.getenv('HOME') + '/Library/Audio/Plug-Ins/LV2'
				else:
					conf.env['LV2DIR'] = os.getenv('HOME') + '/.lv2'
			else:
				if sys.platform == "darwin":
					conf.env['LV2DIR'] = '/Library/Audio/Plug-Ins/LV2'
				else:
					conf.env['LV2DIR'] = conf.env['LIBDIR'] + 'lv2/'
		
	conf.env['BINDIRNAME'] = chop_prefix(conf, 'BINDIR')
	conf.env['LIBDIRNAME'] = chop_prefix(conf, 'LIBDIR')
	conf.env['DATADIRNAME'] = chop_prefix(conf, 'DATADIR')
	conf.env['LV2DIRNAME'] = chop_prefix(conf, 'LV2DIR')
	
	if Options.options.debug:
		conf.env['CCFLAGS'] = '-O0 -g -std=c99'
		conf.env['CXXFLAGS'] = '-O0 -g -ansi'
	if Options.options.strict:
		conf.env['CCFLAGS'] = '-O0 -g -std=c99 -pedantic'
		append_cxx_flags('-Wall -Wextra -Wno-unused-parameter')
		conf.env.append_value('CXXFLAGS', '-Woverloaded-virtual')
	append_cxx_flags('-fPIC -DPIC')
	g_step = 2
	
def set_local_lib(conf, name, has_objects):
	conf.define('HAVE_' + name.upper(), 1)
	if has_objects:
		if type(conf.env['AUTOWAF_LOCAL_LIBS']) != dict:
			conf.env['AUTOWAF_LOCAL_LIBS'] = {}
		conf.env['AUTOWAF_LOCAL_LIBS'][name.lower()] = True
	else:
		if type(conf.env['AUTOWAF_LOCAL_HEADERS']) != dict:
			conf.env['AUTOWAF_LOCAL_HEADERS'] = {}
		conf.env['AUTOWAF_LOCAL_HEADERS'][name.lower()] = True

def use_lib(bld, obj, libs):
	abssrcdir = os.path.abspath('.')
	libs_list = libs.split()
	for l in libs_list:
		in_headers = l.lower() in bld.env['AUTOWAF_LOCAL_HEADERS']
		in_libs    = l.lower() in bld.env['AUTOWAF_LOCAL_LIBS']
		if in_libs:
			if hasattr(obj, 'uselib_local'):
				obj.uselib_local += ' lib' + l.lower() + ' '
			else:
				obj.uselib_local = 'lib' + l.lower() + ' '
		
		if in_headers or in_libs:
			inc_flag = '-iquote ' + abssrcdir + '/' + l.lower()
			for f in ['CCFLAGS', 'CXXFLAGS']:
				if not inc_flag in bld.env[f]:
					bld.env.prepend_value(f, inc_flag)
		else:
			if hasattr(obj, 'uselib'):
				obj.uselib += ' ' + l
			else:
				obj.uselib = l


def display_header(title):
	Utils.pprint('BOLD', title)

def display_msg(conf, msg, status = None, color = None):
	color = 'CYAN'
	if type(status) == bool and status or status == "True":
		color = 'GREEN'
	elif type(status) == bool and not status or status == "False":
		color = 'YELLOW'
	print "%s : " % msg.ljust(conf.line_just),
	Utils.pprint(color, status)

def print_summary(conf):
	global g_step
	if g_step > 2:
		print
		return
	e = conf.env
	print
	display_header('Global configuration')
	display_msg(conf, "Install prefix", conf.env['PREFIX'])
	display_msg(conf, "Debuggable build", str(conf.env['DEBUG']))
	display_msg(conf, "Build documentation", str(conf.env['BUILD_DOCS']))
	print
	g_step = 3

def link_flags(env, lib):
	return ' '.join(map(lambda x: env['LIB_ST'] % x, env['LIB_' + lib]))

def compile_flags(env, lib):
	return ' '.join(map(lambda x: env['CPPPATH_ST'] % x, env['CPPPATH_' + lib]))

def set_recursive():
	global g_is_child
	g_is_child = True

def is_child():
	global g_is_child
	return g_is_child

# Pkg-config file
def build_pc(bld, name, version, libs):
	'''Build a pkg-config file for a library.
	name    -- uppercase variable name     (e.g. 'SOMENAME')
	version -- version string              (e.g. '1.2.3')
	libs    -- string/list of dependencies (e.g. 'LIBFOO GLIB')
	'''

	obj              = bld.new_task_gen('subst')
	obj.source       = name.lower() + '.pc.in'
	obj.target       = name.lower() + '.pc'
	obj.install_path = '${PREFIX}/${LIBDIRNAME}/pkgconfig'
	pkg_prefix       = bld.env['PREFIX'] 
	if pkg_prefix[-1] == '/':
		pkg_prefix = pkg_prefix[:-1]
	obj.dict = {
		'prefix'           : pkg_prefix,
		'exec_prefix'      : '${prefix}',
		'libdir'           : '${exec_prefix}/lib',
		'includedir'       : '${prefix}/include',
		name + '_VERSION'  : version,
	}
	if type(libs) != list:
		libs = libs.split()
	for i in libs:
		obj.dict[i + '_LIBS']   = link_flags(bld.env, i)
		obj.dict[i + '_CFLAGS'] = compile_flags(bld.env, i)

# Doxygen API documentation
def build_dox(bld, name, version, srcdir, blddir):
	if not bld.env['BUILD_DOCS']:
		return
	obj = bld.new_task_gen('subst')
	obj.source = 'doc/reference.doxygen.in'
	obj.target = 'doc/reference.doxygen'
	if is_child():
		src_dir = srcdir + '/' + name.lower()
		doc_dir = blddir + '/default/' + name.lower() + '/doc'
	else:
		src_dir = srcdir
		doc_dir = blddir + '/default/doc'
	obj.dict = {
		name + '_VERSION' : version,
		name + '_SRCDIR'  : os.path.abspath(src_dir),
		name + '_DOC_DIR' : os.path.abspath(doc_dir)
	}
	obj.install_path = ''
	out1 = bld.new_task_gen('command-output')
	out1.stdout = '/doc/doxygen.out'
	out1.stdin = '/doc/reference.doxygen' # whatever..
	out1.command = 'doxygen'
	out1.argv = [os.path.abspath(doc_dir) + '/reference.doxygen']
	out1.command_is_external = True

def shutdown():
	# This isn't really correct (for packaging), but people asking is annoying
	if Options.commands['install']:
		try: os.popen("/sbin/ldconfig")
		except: pass

