#Makefile for compiling MOLASSES Modular Lava Simulator
#The Floor is Lava.

#MODULES: Alter as needed. 
#Check docs/ for module codes and destriptions.
export driver            = Full
export initialize        = Full
export DEM_loader        = 00
export flow_initializer  = Full
export simulation        = 00
export pulse             = Full
export distribute        = SlopeBlind
export neighbor_ID       = 8
export activate          = Par
export output            = Full
export choose_new_vent   = raster


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
