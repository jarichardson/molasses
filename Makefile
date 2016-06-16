#Makefile for compiling MOLASSES Modular Lava Simulator
#The Floor is Lava.

#MODULES: Alter as needed. 
#Check docs/ for module codes and destriptions.
export driver            = 00
export initialize        = 00
export DEM_loader        = 00
export array_initializer = 00
export flow_initializer  = 00
export pulse             = 00
export distribute        = 00
export neighbor_ID       = 01
export activate          = Par
export output            = 00


# Linking and compiling variables: Alter as needed for your system.
export CC               ?= gcc
export INSTALLPATH       = $(PWD)/bin
#GDAL
#     -> Where is gdal.h ?
export GDAL_INCLUDE_PATH = /usr/include/gdal
#     -> Where is gdal.so ? (If it's in a common lib folder, you can likely leave this blank)
export GDAL_LIB_PATH     = 



all clean check install uninstall molasses:
	$(MAKE) -C src $@

.PHONY: FORCE all clean check install uninstall
