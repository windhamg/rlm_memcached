#######################################################################
#
# TARGET should be set by autoconf only.  Don't touch it.
#
# The SRCS definition should list ALL source files.
#
# The HEADERS definition should list ALL header files
#
# RLM_CFLAGS defines addition C compiler flags.  You usually don't
# want to modify this, though.  Get it from autoconf.
#
# The RLM_LIBS definition should list ALL required libraries.
# These libraries really should be pulled from the 'config.mak'
# definitions, if at all possible.  These definitions are also
# echoed into another file in ../lib, where they're picked up by
# ../main/Makefile for building the version of the server with
# statically linked modules.  Get it from autoconf.
#
# RLM_INSTALL is the names of additional rules you need to install
# some particular portion of the module.  Usually, leave it blank.
#
#######################################################################
TARGET      = rlm_memcached
SRCS        = rlm_memcached.c
HEADERS     = rlm_memcached.h
CPPFLAGS = -I/opt/freeradius/include
LIBTOOL=./libtool


rlm_memcached.la: rlm_memcached.lo
	$(LIBTOOL) --mode=link $(CC) -release 1.0 -module  -export-dynamic $(LDFLAGS) -o $@ -rpath /opt/freeradius/lib

rlm_memcached.lo: rlm_memcached.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) -c $<

