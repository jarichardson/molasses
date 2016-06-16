CFLAGS = -Wall -O2
INCLUDES = -I$(GDAL_INCLUDE_PATH) -I../include -I./include

#If you set a specific path for your GDAL libraries, this will look for it!
ifndef GDAL_LIB_PATH
LIBS = -lgdal -lm
else
LIBS = -L$(GDAL_LIB_PATH) -lgdal -lm
endif

SRCS = driver_$(driver).c module_initialize_$(initialize).c \
	module_demloader_$(DEM_loader).c module_arrayinit_$(array_initializer).c \
	module_initflow_$(flow_initializer).c module_pulse_$(pulse).c \
	module_distribute_$(distribute).c module_neighbor_$(neighbor_ID).c \
	module_activate_$(activate).c module_output_$(output).c

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
	@./$(MAIN) ./pcheck/pancake.cfg | grep 'SUCCESS: MASS CONSERVED'
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
