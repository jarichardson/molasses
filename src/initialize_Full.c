#include "prototypes.h"
#define CR 13            /* Decimal code of Carriage Return char */
#define LF 10            /* Decimal code of Line Feed char */

int INITIALIZE(Inputs *In, /* Structure of input parmaeters */
               Outputs *Out, /* Structure of model outputs */
               VentArr **Vents /* Array of active vent structures */
               ) {
/*Module INITIALIZE
	Accepts a configuration file and returns model variables. 
	(format: keyword = value):
	The following KEYWORDS are accepted:
	
	(INPUTS)
	string DEM_FILE
	string LOG_FILE
	(double or string) ELEVATION_UNCERT (could be filename or single value)
	(double or string) RESIDUAL (could be filename or single value)
	string SPATIAL_DENSITY_FILE
	unsigned SPD_GRID_SPACING
	
	(OUTPUTS)
	string ASCII_FLOW_MAP  (Output one file for each flow)
	string ASCII_HIT_MAP (Output one file after all flows complete)
	string ASCII_TOTAL_THICKNESS_MAP (Optional)
	
	(VENT PARAMETERS)
	NEW_VENT (Tag; vent info follows)
	double VENT_EASTING (units = km)
	double VENT_NORTHING (units = km)
	
	(FLOW PARAMETERS)
	double MIN_RESIDUAL
	double MAX_RESIDUAL
	
	double MIN_TOTAL_VOLUME
	double MAX_TOTAL_VOLUME
	
	double MEAN_TOTAL_VOLUME
	double STD_TOTAL_VOLUME
	
	double MIN_PULSE_VOLUME
	double MAX_PULSE_VOLUME
	
	(Simulation Parameters)
	unsigned FLOWS
	unsigned RUNS
	
	
	Checks at end for configuration file errors, where mandatory parameters were
	  not assigned.
*/

	unsigned maxLineLength = 256;
	char     line[256];             /*Line string from file       */
	char     var[64];               /*Parameter Name  in each line*/
	char     value[256];            /*Parameter Value in each line*/
	char     *ptr;
	
	FILE     *ConfigFile;
	FILE     *Opener;     /*Dummy File variable to test valid output file paths */
	/* enum ASCII_types ascii_file; */
	double dval;
	int ret = 0;
	int i = 0;
	int firstvent = 0;
	
	/*
	typedef struct VentArr {
	double northing;
	double easting;
	double totalvolume;
	double pulsevolume;
	double min_totalvolume;
	double max_totalvolume;
	double log_mean_totalvolume;
	double log_std_totalvolume;
	double min_pulse_volume;
	double max_pulse_volume;
} VentArr;
*/
	/* Initialize input parameters */
	In->dem_file = NULL;
	In->residual_map = NULL;
	In->residual = 0;
	In->uncert_map = NULL;
	In->elev_uncert = 0;
	In->spd_file = NULL;
	In->num_grids = 0;
	In->spd_grid_spacing = 0;
	In->min_residual = 0;
	In->max_residual = 0;
	In->log_mean_residual = 0;
	In->log_std_residual = 0;
	In->min_pulse_volume = 0;
	In->max_pulse_volume = 0;
	In->min_total_volume = 0;
	In->max_total_volume = 0;
	In->log_mean_volume = 0;
	In->log_std_volume = 0;
	In->vent_count = 0;
	In->runs = 1;
	In->flows = 1;
	
	/* Initialize output parmaeters */
	Out->ascii_flow_file = NULL;
	Out->ascii_hits_file = NULL;
	
	/***********************
	   Create vent array
	***********************/
	if((*Vents = (VentArr*) malloc (sizeof (VentArr) * (In->vent_count+1)))
	    ==NULL) {
		fprintf(stderr, "\nERROR [INITIALIZE] Out of Memory creating vent array!\n");
		return 1;
	}
	/*Set first vent values to DBL_MAX/0, to indicate they are not yet entered*/
	(*Vents+In->vent_count)->northing             = DBL_MAX;
	(*Vents+In->vent_count)->easting              = DBL_MAX;
	(*Vents+In->vent_count)->totalvolume          = 0;
	(*Vents+In->vent_count)->min_totalvolume      = 0;
	(*Vents+In->vent_count)->max_totalvolume      = 0;
	(*Vents+In->vent_count)->remainingvolume      = 0;
	(*Vents+In->vent_count)->log_mean_totalvolume = 0;
	(*Vents+In->vent_count)->log_std_totalvolume  = 0;
	(*Vents+In->vent_count)->pulsevolume          = 0;
	(*Vents+In->vent_count)->min_pulse_volume     = 0;
	(*Vents+In->vent_count)->max_pulse_volume     = 0;
	
	fprintf(stderr, "Reading in Parameters...\n");
	
	/*open configuration file*/
	ConfigFile = fopen(In->config_file, "r");
	if (ConfigFile == NULL) {
		fprintf(stderr, 
		        "\nERROR [INITIALIZE]: Cannot open configuration file=[%s]:[%s]!\n", 
			In->config_file, strerror(errno));
		return 1;
	}
	
	// use each line in configure file to compare to known model parameters
	while (fgets(line, maxLineLength, ConfigFile) != NULL) {
		//if first character is comment, new line, space, return to next line
		if (line[0] == '#' || line[0] == '\n' || line[0] == ' ') continue;
		
		//print incoming parameter
		sscanf (line,"%s = %s",var,value); //split line into before ' = ' and after
		if (strncmp(var, "NEW_VENT", strlen("NEW_VENT"))) /*dont print if line=newvent*/
			fprintf(stderr, "%25s = %-33s ",var, value); /*print incoming parameter value*/
		else fprintf(stderr, "%20s (#%u)\n","NEW VENT",(In->vent_count+1)); /*print new vent flag*/
		fflush(stderr);
		
		/*INPUT FILES AND GLOBAL MODEL PARAMETERS**********************************/
		/*INPUT DEM FILE*/
		//if no difference in variable name, malloc filename string, copy value
		if (!strncmp(var, "DEM_FILE", strlen("DEM_FILE"))) { 
			In->dem_file = (char*) GC_MALLOC(sizeof(char) * (strlen(value)+1));
			if (In->dem_file == NULL) {
				fprintf(stderr, 
				        "\n[INITIALIZE] Out of Memory assigning filenames!\n");
				return 1;
			}
			strncpy(In->dem_file, value, strlen(value)+1);
		}
		
		/*ELEVATION UNCERTAINTY value OR raster file*/
		else if (!strncmp(var, "ELEVATION_UNCERT", strlen("ELEVATION_UNCERT"))) {
			dval = strtod(value, &ptr);
			//File Treatment
			if (strlen(ptr) > 0) {
				In->uncert_map = (char *) GC_MALLOC(sizeof(char) * (strlen(ptr)+1));
				if (In->uncert_map == NULL) {
					fprintf(stderr, "\n[INITIALIZE] Out of Memory assigning filenames!\n");
					return 1;
				}
				strncpy(In->uncert_map, ptr, strlen(ptr)+1);
				In->elev_uncert = -1;
				Opener = fopen(In->uncert_map, "r");
				if (Opener == NULL) {
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: Failed to open input file: [%s]:[%s]!\n",
				       		In->uncert_map, strerror(errno));
					return 1;
				}
				(void) fclose(Opener);
			}
			//Single Variable Treatment
			else if (dval > 0) 	In->elev_uncert = dval;
		}		
		/*
		else if (!strncmp(var, "SPATIAL_DENSITY_FILE", strlen("SPATIAL_DENSITY_FILE"))) {
			In->spd_file = (char *)GC_MALLOC(((strlen(value)+1) * sizeof(char)));	
  			if (In->spd_file == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for spatial density file:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(In->spd_file, value, strlen(value)+1);			
			Opener = fopen(In->spd_file, "r");
			if (Opener == NULL) {
				fprintf(stderr, 
				        "\nERROR [INITIALIZE]: Failed to open input file: [%s]:[%s]!\n",
				        In->spd_file, strerror(errno));
				return 1;
			}
			ret = load_spd_data(Opener, (*Vents+i), &In->num_grids);
			(void)fclose(Opener);
			if (ret) {
				fprintf (stderr, 
				         "\nERROR []INITIALIZE: error returned from [load_spd_file].\n");
				return 1;
			}
			
		}		*/
		else if (!strncmp(var, "SPD_GRID_SPACING", strlen("SPD_GRID_SPACING"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) In->spd_grid_spacing = dval;
			else {
				fprintf(stderr, 
					"\nERROR [INITIALIZE]: unable to read in a value for the spatial density grid spacing!\n");
				fprintf(stderr, 
				        "Please set a value for SPD_GRID_SPACING in config file\n" );
				return 1;
			}
		}		
		
		/*XYZ THICKNESS LIST, OUTPUT FILE*/
		else if (!strncmp(var, "ASCII_THICKNESS_LIST", strlen("ASCII_THICKNESS_LIST"))) {
			Out->ascii_flow_file = (char *)GC_MALLOC(((strlen(value)+10) * sizeof(char)));	
  			if (Out->ascii_flow_file == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for ascii_flow_map:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(Out->ascii_flow_file, value, strlen(value)+1);
		}
		
		/*XY-HIT BINARY LIST, OUTPUT FILE*/
		else if (!strncmp(var, "ASCII_HIT_LIST", strlen("ASCII_HIT_LIST"))) {
			Out->ascii_hits_file = (char *)GC_MALLOC(((strlen(value)+1) * sizeof(char)));	
  			if (Out->ascii_hits_file == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for ASCII_HIT_MAP:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(Out->ascii_hits_file, value, strlen(value)+1);
		}		
		
		/***********************
		    FLOW PARAMETERS
		**********************/
		/*RESIDUAL THICKNESS value OR raster file*/
		//if no difference in variable name, make value into double. if value is string,
		//pass to file. if not a string, pass double to single variable.
		else if (!strncmp(var, "RESIDUAL_THICKNESS", strlen("RESIDUAL_THICKNESS"))) {
			dval = strtod(value, &ptr);
			//File Treatment
			if (strlen(ptr) > 0) {
				In->residual_map = (char *) GC_MALLOC(sizeof(char) * (strlen(ptr)+1));
				if (In->residual_map == NULL) {
					fprintf(stderr, 
					        "\n[INITIALIZE] Out of Memory assigning filenames!\n");
					return 1;
				}
				strncpy(In->residual_map, ptr, strlen(ptr)+1); //set residual map parameter
				In->residual = -1; //flag that says residual is a file
				Opener = fopen(In->residual_map, "r"); //check file to make sure its real
				if (Opener == NULL) {
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: Failed to open input file at [%s]:[%s]!\n", 
					        In->residual_map, strerror(errno));
					return 1;
				}
				else (void) fclose(Opener);
			}
			//Single Variable Treatment
			else if (dval > 0) In->residual = dval;
		}
		
		/*Minimum Residual Value*/
		else if (!strncmp(var, "MIN_RESIDUAL", strlen("MIN_RESIDUAL"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) In->min_residual = dval;
			else {
				fprintf(stderr, "\n[INITIALIZE]: Unable to read value for MIN_RESIDUAL\n");
				return 1;
			}
		}
		
		/*Maximum Residual Value*/
		else if (!strncmp(var, "MAX_RESIDUAL", strlen("MAX_RESIDUAL"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) In->max_residual = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for MAX_RESIDUAL\n");
				return 1;
			}
		}
		
		/*Log-mean Residual Value*/
		else if (!strncmp(var, "LOG_MEAN_RESIDUAL", strlen("LOG_MEAN_RESIDUAL"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) In->log_mean_residual = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for LOG_MEAN_RESIDUAL\n");
				return 1;
			}
		}
		
		/*Log std-dev Residual Value*/
		else if (!strncmp(var, "LOG_STD_DEV_RESIDUAL", strlen("LOG_STD_DEV_RESIDUAL"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) In->log_std_residual = dval;
			else {
				fprintf(stderr, "\n[INITIALIZE]: Unable to read value for LOG_STD_DEV_RESIDUAL\n");
				return 1;
			}
		}
		
		/***********************
		    VENT PARAMETERS
		**********************/
		/*NEW VENT*/
		else if (!strncmp(line, "NEW_VENT", strlen("NEW_VENT"))) {
			//If a new vent is declared, do nothing for first vent, check that first 
			//vent is totally described before adding a new vent.
			if(In->vent_count && firstvent) {
				//Reallocate memory for a larger Vent array
				*Vents = (VentArr*) realloc(*Vents, (sizeof (VentArr) *
					       ((++In->vent_count))));
				if(*Vents==NULL) {
					fprintf(stderr,
					        "\n[INITIALIZE] Out of Memory adding vent to vent array!\n");
					return 1;
				}
				
				//Set new vent values to DBL_MAX/0, to indicate they are empty
				(*Vents+In->vent_count)->northing             = DBL_MAX;
				(*Vents+In->vent_count)->easting              = DBL_MAX;
				(*Vents+In->vent_count)->totalvolume          = 0;
				(*Vents+In->vent_count)->min_totalvolume      = 0;
				(*Vents+In->vent_count)->max_totalvolume      = 0;
				(*Vents+In->vent_count)->remainingvolume      = 0;
				(*Vents+In->vent_count)->log_mean_totalvolume = 0;
				(*Vents+In->vent_count)->log_std_totalvolume  = 0;
				(*Vents+In->vent_count)->pulsevolume          = 0;
				(*Vents+In->vent_count)->min_pulse_volume     = 0;
				(*Vents+In->vent_count)->max_pulse_volume     = 0;
			}
			
			//if this is the first vent, add this marker so that next NEW_VENT makes ventarray larger
			else ++firstvent;
		}
		
		/*SINGLE VENT PULSE VOLUME*/
		else if (!strncmp(var, "VENT_PULSE_VOLUME", strlen("VENT_PULSE_VOLUME"))) {
			//Assign vent pulse volume value to current vent array element*/
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->pulsevolume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for VENT_PULSE_VOLUME\n");
				return 1;
			
			}
		}
		
		/*Minimum Pulse Volume*/
		else if (!strncmp(var, "MIN_PULSE_VOLUME", strlen("MIN_PULSE_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->min_pulse_volume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for MIN_PULSE_VOLUME\n");
				return 1;
			
			}
		}
		
		/*Maximum Pulse Volume*/
		else if (!strncmp(var, "MAX_PULSE_VOLUME", strlen("MAX_PULSE_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->max_pulse_volume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for MAX_PULSE_VOLUME\n");
				return 1;
			}
		}
		
		/*Minimum Total Volume*/
		else if (!strncmp(var, "MIN_TOTAL_VOLUME", strlen("MIN_TOTAL_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->min_totalvolume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for MIN_TOTAL_VOLUME\n");
				return 1;
			}
		}
		
		/*Maximum Total Volume*/
		else if (!strncmp(var, "MAX_TOTAL_VOLUME", strlen("MAX_TOTAL_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->max_totalvolume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for MAX_TOTAL_VOLUME\n");
				return 1;
			}
		}		
		
		/*Log-mean Total Volume*/
		else if (!strncmp(var, "LOG_MEAN_TOTAL_VOLUME", strlen("LOG_MEAN_TOTAL_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->log_mean_totalvolume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for LOG_MEAN_TOTAL_VOLUME\n");
				return 1;
			}
		}
		
		/*Log standard deviation Total Volume*/
		else if (!strncmp(var, "LOG_STD_DEV_TOTAL_VOLUME", strlen("LOG_STD_DEV_TOTAL_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->log_std_totalvolume = dval;
			else {
				fprintf(stderr, "\n[INITIALIZE]: Unable to read value for LOG_STD_DEV_TOTAL_VOLUME\n");
				return 1;
			}
		}
		
		/*Vent Location: Easting*/
		else if (!strncmp(var, "VENT_EASTING", strlen("VENT_EASTING"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->easting = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for VENT_EASTING\n");
				return 1;
			}
		}
		
		/*Vent Location: Northing*/
		else if (!strncmp(var, "VENT_NORTHING", strlen("VENT_NORTHING"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->northing = dval;
			else {
				fprintf(stderr, 
					    "\n[INITIALIZE]: Unable to read value for VENT_NORTHING\n");
				return 1;
			}
		}
		
		/*
		else if (!strncmp(var, "FLOWS", strlen("FLOWS"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) In->flows = (unsigned)dval;
			else {
				fprintf(stderr, 
					    "\n[INITIALIZE]: Unable to read value for number of lava flows\n");
				return 1;
			}
		}
		*/
		else if (!strncmp(var, "SIMULATIONS", strlen("SIMULATIONS"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) In->runs = (unsigned)dval;
			else {
				fprintf(stderr, 
					    "\n[INITIALIZE]: Unable to read value for SIMULATIONS\n");
				return 1;
			}
		}
		else {
			fprintf (stderr, "[not assigned]\n");
			continue;
		}
		fprintf (stderr,"[assigned]\n");
	}

	/*Check for missing parameters*/
	fprintf(stderr, "Checking for missing parameters ....\n");
	if(!In->elev_uncert) { /*Elevation uncertainty is either missing or is 0.*/
		In->elev_uncert = 0;
		fprintf(stderr, "ELEVATION UNCERTAINTY = 0: DEM values are assumed to be true.\n");
	}
	if(!In->min_residual && In->residual) In->min_residual = In->residual;
	if(!In->max_residual && In->residual) In->max_residual = In->residual;
	if(In->min_residual <= 0 || In->max_residual <= 0) { /*Residual <= 0.*/
		fprintf(stderr, "\nERROR [INITIALIZE]: Flow residual thickness <= 0!!\n");
		return 1;
	}
	if(strlen (In->dem_file) < 2) { /*DEM Filename is missing.*/
		fprintf(stderr, "\nERROR [INITIALIZE]: No DEM Filename Given!!\n");
		return 1;
	}
	if(In->min_pulse_volume <= 0 || In->max_pulse_volume <= 0) { /*Pulse_volume <= 0.*/
		fprintf(stderr, "\nERROR [INITIALIZE]: Lava pulse volume <= 0!!\n");
		return 1;
	}
	if(In->min_total_volume <= 0 || In->max_total_volume <= 0) { /*Total_volume <= 0.*/
		fprintf(stderr, "\nERROR [INITIALIZE]: Total lava volume <= 0!!\n");
		return 1;
	}
	if(In->spd_file == NULL || strlen (In->spd_file) < 2) {  /*Spatial density Filename is missing.*/
		fprintf(stderr, "\nERROR [INITIALIZE]: No spatial density Filename Given!!\n");
		if (!(*Vents+i)->easting || !(*Vents+i)->northing) 
			return 1;
	}
	if(In->spd_grid_spacing <= 0) { /*Spatial density grid spacing <= 0.*/
		fprintf(stderr, "\nERROR [INITIALIZE]: Spatial density grid spacing <= 0!!\n");
		if (!(*Vents+i)->easting || !(*Vents+i)->northing) 
			return 1;
	}
	fprintf(stderr, "Nothing missing.\n");
	
	(void) fclose(ConfigFile);
	return 0;
}
