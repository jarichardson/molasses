CFLAGS = -Wall -O2
INCLUDES = -I$(GDAL_INCLUDE_PATH) -I../include -I./include

#If you set a specific path for your GDAL libraries, this will look for it!
ifndef GDAL_LIB_PATH
LIBS = -lgdal -L../lib -lgc -lran -lm
else
LIBS = -L$(GDAL_LIB_PATH) -lgdal -L../lib -lgc -lran -lm
endif

SRCS = driver_$(driver).c initialize_$(initialize).c \
	demloader_$(DEM_loader).c arrayinit_$(array_initializer).c \
	initflow_$(flow_initializer).c pulse_$(pulse).c \
	distribute_$(distribute).c neighbor_$(neighbor_ID).c \
	activate_$(activate).c output_$(output).c

OBJ = $(SRCS:.c=.o)

MAIN = molasses

.PHONY: depend clean check install uninstall

all:	$(MAIN)
		@echo "*** $(MAIN) has been compiled. ***"


$(MAIN): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $+ $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

$(OBJ): structs.h prototypes.h

check: all
	@echo "Testing for conservation of mass..."
	@./$(MAIN) ./pcheck/pancake.cfg
	#@./$(MAIN) ./pcheck/pancake.cfg | grep 'SUCCESS: MASS CONSERVED'
	@echo "*** All TESTS PASSED ***"

clean:
	$(RM) *.o *~ $(MAIN) pcheck/out pcheck/*~

install:
	install -d $(INSTALLPATH)
	install -m 0755 $(MAIN) $(INSTALLPATH)

uninstall:
	-rm $(INSTALLPATH)/$(MAIN) &>/dev/null

#depend: $(SRCS)
#	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
