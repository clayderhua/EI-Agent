#!/bin/bash
libname=""

COLOR_REST='\e[0m'
COLOR_GREEN='\e[0;32m';
COLOR_RED='\e[0;31m';
COLOR_YELLOW='\e[0;33m';

function print_green()
{
	echo -e "${COLOR_GREEN}$1${COLOR_REST}"
}

function print_red()
{
	echo -e "${COLOR_RED}$1${COLOR_REST}"
}

function print_yellow()
{
	echo -e "${COLOR_YELLOW}$1${COLOR_REST}"
}

function prepare_source()
{
    ret=$(ls *.c 2>/dev/null; ls *.cpp 2>/dev/null)
	if [[ "${ret}" == "" ]]; then # no source in root
		ret=$(ls src/*.c 2>/dev/null; ls *.cpp 2>/dev/null)
		if [[ ${ret} == "" ]]; then # no source in src
			print_red "Please copy source files to current folder"
			exit 1
		fi
	fi
	
	mkdir -p inc
	mv *.c src 2>/dev/null
	mv *.cpp src 2>/dev/null
	mv *.h inc 2>/dev/null
}

function prepare_configure()
{
	if [[ -f configure.ac ]]; then
		return
	fi

	autoscan
	mv configure.scan configure.ac
	sed -i -E "s/AC_CONFIG_HEADERS(.+)/AC_CONFIG_HEADERS([config.h])\nAC_CONFIG_MACRO_DIRS([m4])\nAM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])\nAM_PROG_AR\nLT_INIT/" configure.ac
}

function prepare_makefile()
{
	# update la_SOURCES
	src_list=$(cd src; ls *.c 2>/dev/null | tr '\r\n' ' '; ls *.cpp 2>/dev/null | tr '\r\n' ' ')
	if [[ "$src_list" != "" ]]; then
		sed -i -E "s|_la_SOURCES.+|_la_SOURCES = ${src_list}|" src/Makefile.am
	fi

	# replace lib_LTLIBRARIES
	ret=$(grep LIBNAME src/Makefile.am)
	if [[ "$ret" == "" ]]; then # have the library name, extract it
		ret=$(grep lib_LTLIBRARIES src/Makefile.am | awk '{print $3}')
		libname=${ret%%"."*}
		return
	fi

	print_green "Please enter the library name, ex: libfoo"
	read libname
	if [[ "$libname" == "" ]]; then
		print_red "library name is not valid"
		exit 1
	fi

	sed -i "s/LIBNAME/${libname}/" src/Makefile.am
}

function do_commit()
{
	git add src/Makefile.am
	git add configure.ac
	git add src/*.c 2>/dev/null
	git add src/*.cpp 2>/dev/null
	git add inc/*.h 2>/dev/null
	git commit -m "Add ${libname} source"
	git clean -fdx
	git reset --hard
	
	print_green "Please review the changes with: git log"
}

function try_to_build()
{
	print_green "Try to build..."
	
	ret=$(grep LIBSRC src/Makefile.am)
	if [[ "$ret" != "" ]]; then
		print_red "la_SOURCES is invalid in src/Makefile.am"
	fi
	
	autoreconf -fiv || exit 1
	./configure || exit 1
	make
	if [[ "$?" != 0 ]]; then
		print_red "build fail, please fix build break and run init.sh again."
		exit 1
	fi
	
	print_green "Build passed ! Commit changes..."
	do_commit
}


prepare_source
prepare_configure
prepare_makefile
try_to_build

