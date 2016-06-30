#### NAME
**MOLASSES**
 
#### DESCRIPTION

MOLASSES is an in-progress fluid flow simulator, written in C. It is not yet complete and is in need of testing and additional programming before it can be used effectively.

MOLASSES stands for *MO*dular *LA*va *S*imulation *S*oftware for *E*arth *S*cience.

#### REQUIREMENTS

MOLASSES requires the GDAL C libraries and a C compiler. To get the GDAL libraries, install gdal-devel on your UNIX machine. We have tested this program on computers that use the C compiler gcc.

#### INSTALLATION

Before compilation of MOLASSES it is prudent to modify the Makefile in this directory for your use. Check to make sure that the installation path and the path to GDAL are properly set. Modify the module numbers at the top of the Makefile to change the model algorithm to what you require.

To compile and install MOLASSES execute the following commands:

		make
		make install

You may need to be in super-user mode to perform make install.

#### EXECUTING

To run the code after installation, use the command

		molasses <config-file> <starting simulation count>

where `<config-file>` is a text configuration file. Sample configuration files (sample_*.cfg) are given in the main directory. `<starting simulation count>` is optional, but might be useful if MOLASSES output needs to avoid overwriting previous output. An elevation model is required to run molasses, and at least one output file path and one vent location must be given in the configuration file. More information is documented in the docs directory.

##### Sample Configuration File

	DEM_FILE = raster_samples/flat_1300x1300.asc
	ELEVATION_UNCERT = 0
	
	#Output
	ASCII_THICKNESS_LIST = flow.xyz
	#ASCII_HIT_LIST = hits.xyz
	#TIFF_HIT_MAP = hits.tif
	TIFF_THICKNESS_MAP = thickness.tif
	#TIFF_ELEVATION_MAP = paleosurface.tif
	#TIFF_NEW_ELEV_MAP = new_elev.tif
	#STATS_FILE = stats.dat
	
	#Model Parameters
	RESIDUAL_THICKNESS = 1
	
	NEW_VENT
	VENT_EASTING = 650
	VENT_NORTHING = 650
	VENT_PULSE_VOLUME = 1
	VENT_TOTAL_VOLUME = 7000

#### UNINSTALLATION

To uninstall MOLASSES run
		make uninstall

#### AUTHORS

Jacob Richardson (jarichardson@mail.usf.edu)

Laura Connor     (lconnor@usf.edu)

Chuck Connor     (cbconnor@usf.edu)

#### COPYRIGHT

Copyright 2014, Creative Commons Attribution-NonCommercial 3.0 Unported (CC BY-NC 3.0). http://creativecommons.org/licenses/by-nc/3.0/
