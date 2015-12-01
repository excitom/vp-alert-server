include $(UROOT)/inc/make.h


## compilation/link flags
OBJ        = ${UOBJ}/alert
GINC       = -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include
DBINC      = -I/usr/sybase/include
FLAGS      = -I../inc -I../../../lib/vp/inc ${GINC} ${DBINC} ${CCFLAGS} -DTOM
DBLIB      = ${SYBASE}/lib/libsybdb.a
GLIB       = -lglib-2.0
LIBS       = ${LVP} ${LSCT} ${LUBQ} ${GLIB}
OBJECTS = \
  alert.o \
  db.o \
  main.o \
  event.o \
  place.o \
  sock.o \
  subscriber.o \
  users.o 
OBJS       = $(addprefix $(OBJ)/,$(OBJECTS))
JR_OBJECTS = \
  alert_jr.o \
  main_jr.o \
  event.o \
  place.o \
  sock.o \
  subscriber.o \
  users.o 
JR_OBJS       = $(addprefix $(OBJ)/,$(JR_OBJECTS))

## for plain, purify based, and quantify based
MODULES = ${UBIN}/vpalert ${UBIN}/vpalertjr
MOD_EXE = $(addsuffix .exe,$(MODULES))
MOD_PUR = $(addsuffix .pur,$(MODULES))
MOD_QNT = $(addsuffix .qnt,$(MODULES))

#=======================================================================    

install: mk_obj_dir ${MOD_EXE}

all: install purify quantify

purify:   mk_obj_dir ${MOD_PUR}

quantify: mk_obj_dir ${MOD_QNT}

mk_obj_dir:
	if test ! -d ${OBJ}; then mkdir -p ${OBJ}; fi;

${UBIN}/vpalert.exe: ${OBJS} ${LIBS} ${DBLIB}
	/bin/rm -f $@
	${CC} ${LDFLAGS} ${OBJS} ${LIBS} ${SYSLIB} ${DBLIB} -o $@
	chmod 777 $@

${UBIN}/vpalertjr.exe: ${JR_OBJS} ${LIBS}
	/bin/rm -f $@
	${CC} ${LDFLAGS} ${JR_OBJS} ${LIBS} ${SYSLIB} -o $@
	chmod 777 $@

clean cleanup:
	/bin/rm -f ${OBJS} ${JR_OBJS} ${MOD_EXE} ${MOD_PUR} ${MOD_QNT}

## Compilation rules
.SUFFIXES: .cc

${OBJ}/%.o: %.cc
	/bin/rm -f $@
	${CC} ${FLAGS} -c $< -o $@
	chmod 666 $@

## dependencies stuff
Makefile: depend-$(UCONFIG).$(UMODE);

depend-$(UCONFIG).$(UMODE): ${OBJECTS:.o=.cc}
	#mkdp -f "$(FLAGS)" -p "$(OBJ)" -i -- depend-$(UCONFIG).$(UMODE) $?
	touch depend-$(UCONFIG).$(UMODE) $?
	chmod 666 depend-$(UCONFIG).$(UMODE)

include depend-$(UCONFIG).$(UMODE)
