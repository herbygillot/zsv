# Makefile for use with GNU make

THIS_MAKEFILE_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

CONFIGFILE ?= ../config.mk
ifneq ($(CONFIGFILE),)
  ifeq ($(shell realpath `dirname ${CONFIGFILE}`),`realpath .`)
    $(error howdydy)
  endif
endif

$(info Using config file ${CONFIGFILE})
include ${CONFIGFILE}

CONFIGFILEPATH=$(shell ls ${CONFIGFILE} >/dev/null 2>/dev/null && realpath ${CONFIGFILE})
ifeq ($(CONFIGFILEPATH),)
  $(error Config file ${CONFIGFILE} not found)
endif

CC ?= cc

CFLAGS_EXE ?=

LIBZSV_A=libzsv.a
LIBZSV=${BUILD_DIR}/lib/${LIBZSV_A}
LIBZSV_INSTALL=${LIBDIR}/${LIBZSV_A}

STRING_LIB_INCLUDE=
STRINGLIB_INCLUDE_FLAG=
ifneq ($(STRING_LIB_INCLUDE),)
  CFLAGS+= -DSTRING_LIB_INCLUDE='${STRING_LIB_INCLUDE}'
endif

STRING_LIB_OBJ=

DEBUG=0
WIN=
ifeq ($(WIN),)
  WIN=0
  ifneq ($(findstring w64,$(CC)),) # e.g. mingw64
    WIN=1
  endif
endif

MORE_OBJECTS=
NO_LTO=0

ifeq ($(DEBUG),1)
  DBG_SUBDIR+=dbg
else
  DBG_SUBDIR+=rel
endif

NO_STDIN=
ifeq ($(NO_STDIN),1)
  CFLAGS+= -DNO_STDIN
endif

CFLAGS+= ${CFLAGS_PIC}
ifeq ($(WIN),0)
  BUILD_SUBDIR=$(shell uname)/${DBG_SUBDIR}
  EXE=

  ifneq ($(findstring emcc,$(CC)),) # emcc
    NO_THREADING ?= 1
    EXE=.em.js
    EXPORTED_FUNCTIONS='_free','_malloc'
    CFLAGS+= -s EXPORTED_FUNCTIONS="[${EXPORTED_FUNCTIONS}]" -s EXPORTED_RUNTIME_METHODS="['setValue','allocateUTF8','stringToUTF8Array','lengthBytesUTF8','writeArrayToMemory']" -s RESERVED_FUNCTION_POINTERS=4 -s ALLOW_MEMORY_GROWTH=1
    CFLAGS_EXE+= -s FORCE_FILESYSTEM=1 -lidbfs.js -s MAIN_MODULE
    ifeq ($(NO_THREADING),0)
      CFLAGS_EXE+= -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=2 -s PTHREAD_POOL_SIZE=8
    endif
  endif
else
  BUILD_SUBDIR=win/${DBG_SUBDIR}
  EXE=.exe
  CFLAGS+= -D__USE_MINGW_ANSI_STDIO -D_ISOC99_SOURCE
  CFLAGS+= -Wl,--strip-all
endif

CFLAGS+= -std=gnu11
ifeq ($(DEBUG),0)
  CFLAGS+= -O3 -DNDEBUG -Wno-gnu-statement-expression -Wshadow -Wall -Wextra -Wno-missing-braces -pedantic -DSTDC_HEADERS -D_GNU_SOURCE -ftree-vectorize ${CFLAGS_OPT}

  ifeq ($(PGO),1)
    CFLAGS+= -fprofile-generate -fprofile-dir=/tmp/p4
  else
    ifeq ($(PGO),2)
      CFLAGS+= -fprofile-use=/tmp/p4
    else
      $(echo "No profiling set. To use PGO, compile with PGO=1, then run with data, then compile again with PGO=2")
    endif
  endif

else
  CFLAGS += -g
endif

CCBN=$(shell basename ${CC})
CFLAGS+= -I${PREFIX}/include
THIS_LIB_BASE=$(shell cd .. && pwd)
INCLUDE_DIR=${THIS_LIB_BASE}/include
BUILD_DIR=${THIS_LIB_BASE}/build/${BUILD_SUBDIR}/${CCBN}
UTILS1=writer file err signal mem clock arg dl string dirs
# LDFLAGS=

ZSV_EXTRAS ?=
ifneq ($(WIN),0)
 UTILS1+= os
endif

ifneq ($(findstring emcc,$(CC)),) # emcc
  ZSV_EXTRAS=1
  UTILS1+=emcc/fs_api
  ifeq ($(NO_THREADING),0)
    LDFLAGS+=-pthread
  endif
else # not emcc
  CFLAGS+= ${CFLAGS_AVX2}
  LDFLAGS+=-lpthread # Linux explicitly requires
endif
UTILS=$(addprefix ${BUILD_DIR}/objs/utils/,$(addsuffix .o,${UTILS1}))

ifeq ($(ZSV_EXTRAS),1)
  CFLAGS+= -DZSV_EXTRAS
endif

OBJECTS=${UTILS}

ifeq ($(NO_MEMMEM),1)
  OBJECTS+= ${BUILD_DIR}/objs/utils/memmem.o
  CFLAGS+=-DNO_MEMMEM
endif

ZSV=$(BINDIR)/zsv${EXE}

SOURCES= echo count select 2json serialize flatten pretty stack desc 2tsv sql 2db
CLI_SOURCES=select desc count pretty sql flatten 2json 2tsv serialize stack 2db

ifneq ($(LDFLAGS_JQ),)
  SOURCES+= jq
  CLI_SOURCES+= jq
  CFLAGS+= -DUSE_JQ
endif

STANDALONE_PFX=${BUILD_DIR}/bin/zsv_

TARGETS=$(addprefix ${STANDALONE_PFX},$(addsuffix ${EXE},${SOURCES})) ${CLI}

BUILDS=$(addprefix build-,${SOURCES})
CLEANS=$(addprefix clean-,${SOURCES})

## cli
VERSION= $(shell (git describe --always --dirty --tags 2>/dev/null || echo "v0.0-zsv") | sed 's/^v//')
CLI_OBJ_PFX=${BUILD_DIR}/objs/cli_
CLI_APP_OBJECT=${CLI_OBJ_PFX}cli.o
CLI=${BUILD_DIR}/bin/cli${EXE}
ifneq ($(findstring emcc,$(CC)),) # emcc
    CLI_ADDITIONAL=${BUILD_DIR}/bin/cli.em.wasm
endif
CLI_OBJECTS=$(addprefix ${CLI_OBJ_PFX},$(addsuffix .o,${CLI_SOURCES}))

CFLAGS+=${CFLAGS_AUTO}
ifeq ($(NO_LTO),1)
  CFLAGS+= -fno-lto
else
  CFLAGS+=${CFLAGS_LTO}
  LDFLAGS_OPT+= ${LDFLAGS_OPT_LTO}
endif

ifeq ($(VERBOSE),1)
  CFLAGS+= ${CFLAGS_VECTORIZE_OPTIMIZED} ${CFLAGS_VECTORIZE_MISSED} ${CFLAGS_VECTORIZE_ALL}
endif

INIH_SRC=external/inih
INIH_INCLUDE=external/inih
INIH_OBJECT=${BUILD_DIR}/external/inih/inih.o
CLI_INCLUDE+= -I${INIH_INCLUDE}

YAJL_SRC=external/yajl/yajl.c
YAJL_OBJ1=yajl yajl_alloc yajl_buf yajl_encode yajl_gen yajl_lex yajl_parser yajl_tree yajl_version
YAJL_OBJ=$(addprefix ${BUILD_DIR}/external/yajl/,$(addsuffix .o,${YAJL_OBJ1}))
YAJL_INCLUDE=-Iexternal/yajl/build/yajl-2.1.1/include

YAJL_HELPER_OBJ=${BUILD_DIR}/external/yajl_helper/yajl_helper.o
YAJL_HELPER_INCLUDE=-Iexternal/yajl_helper

## json writer
JSONWRITER_SRC=external/json_writer-1.0
JSONWRITER_INCLUDE=-Iexternal/json_writer-1.0
JSONWRITER_OBJECT=${BUILD_DIR}/external/json_writer-1.0/jsonwriter.o

## utf8proc
UTF8PROC_SRC=${THIS_MAKEFILE_DIR}/external/utf8proc-2.6.1
UTF8PROC_INCLUDE=external/utf8proc-2.6.1
UTF8PROC_OBJECT=${BUILD_DIR}/external/utf8proc-2.6.1/utf8proc.o
CFLAGS+= -I${UTF8PROC_INCLUDE} -DUTF8PROC -DUTF8PROC_STATIC

# sqlite3
SQLITE_EXT=${BUILD_DIR}/external/sqlite3/sqlite3_and_csv_vtab.o
SQLITE_SRC=${THIS_MAKEFILE_DIR}/external/sqlite3/sqlite3*.c
SQLITE_EXT_INCLUDE=-Iexternal/sqlite3

help:
	@echo "To build: ${MAKE} [DEBUG=1] [clean] [BINDIR=${BINDIR}] <install|all|test>"
	@echo
	@echo "To build and test individual apps, run:"
	@echo "  ${MAKE} test"
	@echo "which will build and test all apps, or to build/test a single app:"
	@echo "  ${MAKE} test-xx"
	@echo "where xx is any of:"
	@echo "  echo count select 2json serialize flatten pretty stack desc 2tsv sql 2db"
	@echo ""

install: ${ZSV}

uninstall:
	rm -rf ${ZSV}

build: all

all: ${TARGETS}

${LIBZSV_INSTALL}:
	${MAKE} -C ../src CONFIGFILE=${CONFIGFILEPATH} install DEBUG=${DEBUG}

${ZSV}: ${CLI}
	@mkdir -p `dirname "$@"`
ifneq ($(findstring emcc,$(CC)),) # emcc
	cp -p $< `dirname "$@"`/
	cp -p ${CLI_ADDITIONAL} `dirname "$@"`/
else
	cp -p $< $@
endif

cli: build-cli

build-cli: ${CLI}

clean-cli:
	@rm -f  ${CLI_APP_OBJECT} ${CLI_OBJECTS}

${BUILDS}: build-%: ${STANDALONE_PFX}%${EXE}
	@echo Built ${STANDALONE_PFX}$*${EXE}

${CLEANS}: clean-%:
	rm -f ${STANDALONE_PFX}$*${EXE}
	rm -f ${BUILD_DIR}/*/*$*.o

.PHONY: all install cli build build-% clean clean-% test test-%

.SECONDARY: ${OBJECTS}

.POSIX:
.SUFFIXES:
.SUFFIXES: .o .c .a

${BUILD_DIR}/objs/utils/%.o : utils/%.c ${INCLUDE_DIR}/zsv/utils/%.h
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDE_DIR} -I${UTF8PROC_INCLUDE} -DINCLUDE_SRC -o $@ -c utils/$*.c ${MORE_SOURCE}

${BUILD_DIR}/objs/zsv_%.o: %.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDE_DIR} -I${UTF8PROC_INCLUDE} -c $< -o $@

${UTF8PROC_OBJECT}: ${UTF8PROC_SRC}/utf8proc.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${UTF8PROC_INCLUDE} -c ${UTF8PROC_SRC}/utf8proc.c -o $@

${INIH_OBJECT}: ${INIH_SRC}/ini.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INIH_INCLUDE} -DINI_HANDLER_LINENO=1 -DINI_CALL_HANDLER_ON_NEW_SECTION=1 -c $< -o $@

${CLI_APP_OBJECT} : cli_ini.c builtin/*.c
${CLI_APP_OBJECT} ${CLI_OBJECTS}: ${CLI_OBJ_PFX}%.o: %.c ${UTF8PROC_SRC}/utf8proc.c # ${MORE_OBJECTS}
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -DVERSION=\"${VERSION}\" -DZSV_CLI -DMAIN=main_$* ${CLI_INCLUDE} -Iexternal/sglib -I${INCLUDE_DIR} -c $< -o $@ ${MORE_SOURCE}

${CLI}: cli_internal.c.in cli_internal.h cli_internal.h.in ${CLI_APP_OBJECT} ${CLI_OBJECTS} ${OBJECTS} ${UTF8PROC_OBJECT} cli_ini.c ${INIH_OBJECT} ${LIBZSV_INSTALL} ${MORE_OBJECTS}
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} ${CFLAGS_EXE} -I${INCLUDE_DIR} -o $@ ${CLI_APP_OBJECT} ${CLI_OBJECTS} ${OBJECTS} ${UTF8PROC_OBJECT} ${INIH_OBJECT} -L${LIBDIR} -lzsv ${LDFLAGS} ${LDFLAGS_OPT} ${MORE_OBJECTS} ${MORE_SOURCE} ${MORE_LIBS}
	@echo Built $@

# cli_internal: cli_internal.c.in cli_internal.h cli_internal.h.in

cli_internal.h.in: ${THIS_MAKEFILE_DIR}/../include/zsv/ext/implementation_private.h
	cat $< | perl -ne 'print if /ZSV_EXT_EXPORT/ .. /;/' | sed 's/ZSV_EXT_EXPORT *//g' | sed $$'s/ *;.*/;\\\n/' | sed 's/(/)(/' | sed 's/zsv_ext_\([a-z]*\))/(*\1)/' | grep -v '^$$' > $@

cli_internal.c.in: cli_internal.h.in
	cat cli_internal.h.in | sed 's/\(.*(\*\)\([^)]*\)\(.*\);/zsv_ext_func_assign((\1\3), \2);/' > $@

${SQLITE_EXT}: ${SQLITE_SRC}

${JSONWRITER_OBJECT}: ${JSONWRITER_SRC}/jsonwriter.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} ${JSONWRITER_INCLUDE} -DINCLUDE_UTILS $< -c -o $@

# flatten stack desc use sglib
${STANDALONE_PFX}flatten${EXE} ${STANDALONE_PFX}stack${EXE} ${STANDALONE_PFX}desc${EXE}: MORE_SOURCE+=-Iexternal/sglib

# 2db uses yajl
${CLI} ${STANDALONE_PFX}2db${EXE}: ${YAJL_OBJ} ${YAJL_HELPER_OBJ}
${CLI} ${STANDALONE_PFX}2db${EXE}: MORE_OBJECTS+= ${YAJL_OBJ} ${YAJL_HELPER_OBJ}
${STANDALONE_PFX}2db${EXE} ${CLI_OBJ_PFX}2db.o: MORE_SOURCE+= ${YAJL_INCLUDE} ${YAJL_HELPER_INCLUDE}

# sql, 2db, 2json use sqlite3
${CLI} ${STANDALONE_PFX}sql${EXE} ${STANDALONE_PFX}2db${EXE} ${STANDALONE_PFX}2json${EXE}: ${SQLITE_EXT}
${CLI} ${STANDALONE_PFX}sql${EXE} ${STANDALONE_PFX}2db${EXE} ${STANDALONE_PFX}2json${EXE}: MORE_OBJECTS+=${SQLITE_EXT}
${STANDALONE_PFX}sql${EXE} ${CLI_OBJ_PFX}sql.o ${STANDALONE_PFX}2db${EXE} ${CLI_OBJ_PFX}2db.o ${STANDALONE_PFX}2json${EXE} ${CLI_OBJ_PFX}2json.o: MORE_SOURCE+=${SQLITE_EXT_INCLUDE}

# 2json and desc use jsonwriter
${CLI} ${STANDALONE_PFX}2json${EXE} ${STANDALONE_PFX}desc${EXE}: ${JSONWRITER_OBJECT}
${CLI} ${STANDALONE_PFX}2json${EXE} ${STANDALONE_PFX}desc${EXE}: MORE_OBJECTS+= ${JSONWRITER_OBJECT}
${STANDALONE_PFX}2json${EXE} ${CLI_OBJ_PFX}2json.o ${STANDALONE_PFX}desc${EXE} ${CLI_OBJ_PFX}desc.o: MORE_SOURCE+= ${JSONWRITER_INCLUDE}

# utils/db uses jsonwriter
${BUILD_DIR}/objs/utils/db.o : MORE_SOURCE+= ${JSONWRITER_INCLUDE} ${SQLITE_EXT_INCLUDE}

# 2json uses utils/db
${CLI} ${STANDALONE_PFX}2json${EXE}: ${BUILD_DIR}/objs/utils/db.o
${CLI} ${STANDALONE_PFX}2json${EXE}: MORE_OBJECTS+= ${BUILD_DIR}/objs/utils/db.o

# pretty uses termcap
${CLI} ${STANDALONE_PFX}pretty${EXE}: MORE_LIBS+=${LDFLAGS_TERMCAP}

${CLI} ${STANDALONE_PFX}jq${EXE}: MORE_LIBS+=${LDFLAGS_JQ}

${STANDALONE_PFX}%${EXE}: %.c ${OBJECTS} ${MORE_OBJECTS} ${LIBZSV_INSTALL} ${UTF8PROC_OBJECT}
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDE_DIR} -o $@ $< ${OBJECTS} ${MORE_OBJECTS} ${MORE_SOURCE} -L${LIBDIR} -lzsv ${UTF8PROC_OBJECT} ${LDFLAGS} ${LDFLAGS_OPT} ${MORE_LIBS}

${BUILD_DIR}/external/sqlite3/sqlite3_and_csv_vtab.o: ${BUILD_DIR}/external/%.o : external/%.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDE_DIR} -o $@ -c $<

${YAJL_OBJ}: ${BUILD_DIR}/external/yajl/%.o : external/yajl/src/%.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} ${YAJL_INCLUDE} -c external/yajl/src/$*.c -o $@

${YAJL_HELPER_OBJ}: external/yajl_helper/yajl_helper.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${BASEDIR}/yajl_helper ${YAJL_INCLUDE} ${YAJL_HELPER_INCLUDE} -c $< -o $@

test:
	@${MAKE} -C test $@ QUIET=1 LEAKS=${LEAKS} CONFIGFILE=${CONFIGFILEPATH} DEBUG=${DEBUG}

clean: clean-obj
	rm -rf ${BUILD_DIR}
	make -C test clean CONFIGFILE=${CONFIGFILEPATH} DEBUG=${DEBUG}

clean-obj:
	rm -rf ${BUILD_DIR}/bin ${INIH_OBJECT} ${JSONWRITER_OBJECT} ${UTF8PROC_OBJECT}
	rm -rf external/utf8proc-2.6.1

${UTF8PROC_SRC}/utf8proc.c: external/utf8proc-2.6.1.tar.gz
	@cd external && tar xf ../$<
	@touch $@
