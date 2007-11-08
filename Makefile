# Ensure $JOT_ROOT matches the location of this Makefile

ifeq ($(ARCH),)
default %: 
	@echo Set ARCH to be one of WIN32, linux, or ...
else

ifeq ($(JOT_ROOT),)
default %:
	@echo Set JOT_ROOT to root of jot hierarchy
endif

# General targets
.PHONY :  FORCE default $(PROGS)

default: jot

all@% : FORCE
	@echo $(JOT_PREPEND) Making all in $*
	$(JOT_IGN)$(MAKE) -C $* all $(JOT_EXTRA_ALL_CMND) $(JOT_EXIT)

clean@% : FORCE
	@echo $(JOT_PREPEND) Making clean in $*
	$(JOT_IGN)$(MAKE) -C $* clean $(JOT_EXIT)

depend@% : FORCE
	@echo $(JOT_PREPEND) Making dependencies in $*
	$(JOT_IGN)$(MAKE) -C $* depend $(JOT_EXIT)

# Setup program directories
DIRS_jot = dev disp dlhandler ffs geom gest glew glui glut_jot glut_winsys \
        gtex gui base_jotapp manip map3d mesh mlib net npr pattern std stroke \
        tess widgets wnpr libpng zlib triangle sps proxy_pattern 

DIRS_smview = dev disp dlhandler geom gest glew glui glut_jot glut_winsys \
        gtex base_jotapp manip mesh mlib net std widgets libpng zlib

# Setup program targets
PROGS         = $(NORMPROGS) $(SPECPROGS)
# Programs compiled in jot/src
NORMPROGS     = jot smview
# Programs not compiled in jot/src
SPECPROGS     =
CLEAN_PREFIX  = clean-
DEPEND_PREFIX = depend-
CLEAN_PROGS   = $(foreach PROG,$(PROGS),$(CLEAN_PREFIX)$(PROG))
DEPEND_PROGS  = $(foreach PROG,$(PROGS),$(DEPEND_PREFIX)$(PROG))

depend: depend-jot

$(NORMPROGS) :
	$(JOT_IGN)$(MAKE) $(addprefix all@, $(DIRS_$@)) $@@src
$(SPECPROGS): FORCE
	$(JOT_IGN)$(MAKE) $(addprefix all@, $(DIRS_$@))
$(CLEAN_PROGS) :
	$(JOT_IGN)$(MAKE) $(addprefix clean@, $(DIRS_$(patsubst clean-%,%, $@)) src)
$(DEPEND_PROGS) :
	$(JOT_IGN)$(MAKE) $(addprefix depend@,$(DIRS_$(patsubst depend-%,%, $@)) src)
%@src:
	@echo $(JOT_PREPEND) Making $* in src
	$(JOT_IGN)$(MAKE) -C src $* $(JOT_EXIT)

# Variables
ifeq ($(JOT_EXIT),)
JOT_EXIT = || exit
endif

ifeq ($(JOT_PREPEND),)
JOT_PREPEND = ------------
endif

ifeq ($(JOT_IGN),)
JOT_IGN = @
endif

endif
# DO NOT DELETE
