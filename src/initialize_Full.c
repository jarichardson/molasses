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
	double min_pulsevolume;
	double max_pulsevolume;
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
	In->vent_count = 0;
	In->runs = 1;
	In->flows = 1;
	In->dem_grid_data = NULL;
	In->spd_grid_data = NULL;
	In->dem_proj = NULL;
	
	/* Initialize output parmaeters */
	Out->ascii_flow_file = NULL;
	Out->ascii_hits_file = NULL;
	Out->raster_hits_file = NULL;
	Out->raster_flow_file = NULL;
	Out->raster_post_topo = NULL;
	Out->raster_pre_topo = NULL;
	Out->stats_file = NULL;
	
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
	(*Vents+In->vent_count)->min_pulsevolume     = 0;
	(*Vents+In->vent_count)->max_pulsevolume     = 0;
	
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
		else fprintf(stderr, "%20s (#%u)\n","NEW VENT",(In->vent_count+1+firstvent)); /*print new vent flag*/
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
		
		else if (!strncmp(var, "VENT_SPATIAL_DENSITY_FILE", strlen("VENT_SPATIAL_DENSITY_FILE"))) {
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
			(void)fclose(Opener);
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
		
		/*XY-HIT COUNT LIST, OUTPUT FILE*/
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
		
		/*HIT COUNT RASTER, OUTPUT FILE*/
		else if (!strncmp(var, "TIFF_HIT_MAP", strlen("TIFF_HIT_MAP"))) {
			Out->raster_hits_file = (char *)GC_MALLOC(((strlen(value)+1) * sizeof(char)));	
  			if (Out->raster_hits_file == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for TIFF_HIT_MAP:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(Out->raster_hits_file, value, strlen(value)+1);
		}
		
		/*TIFF THICKNESS RASTER, OUTPUT FILE*/
		else if (!strncmp(var, "TIFF_THICKNESS_MAP", strlen("TIFF_THICKNESS_MAP"))) {
			Out->raster_flow_file = (char *)GC_MALLOC(((strlen(value)+1) * sizeof(char)));	
  			if (Out->raster_flow_file == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for TIFF_HIT_MAP:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(Out->raster_flow_file, value, strlen(value)+1);
		}
		
		/*TIFF PREFLOW ELEVATION RASTER, OUTPUT FILE*/
		else if (!strncmp(var, "TIFF_ELEVATION_MAP", strlen("TIFF_ELEVATION_MAP"))) {
			Out->raster_pre_topo = (char *)GC_MALLOC(((strlen(value)+1) * sizeof(char)));	
  			if (Out->raster_pre_topo == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for TIFF_HIT_MAP:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(Out->raster_pre_topo, value, strlen(value)+1);
		}
		
		/*TIFF POSTFLOW ELEVATION RASTER, OUTPUT FILE*/
		else if (!strncmp(var, "TIFF_NEW_ELEV_MAP", strlen("TIFF_NEW_ELEV_MAP"))) {
			Out->raster_post_topo = (char *)GC_MALLOC(((strlen(value)+1) * sizeof(char)));	
  			if (Out->raster_post_topo == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for TIFF_HIT_MAP:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(Out->raster_post_topo, value, strlen(value)+1);
		}
		
		/*STATISTICS METADATA FILE*/
		else if (!strncmp(var, "STATS_FILE", strlen("STATS_FILE"))) {
			Out->stats_file = (char *)GC_MALLOC(((strlen(value)+10) * sizeof(char)));	
  			if (Out->stats_file == NULL) {
    			fprintf(stderr, 
                        "Cannot malloc memory for ascii_flow_map:[%s]\n", 
                        strerror(errno));
    			return 1;
    		}
			strncpy(Out->stats_file, value, strlen(value)+1);
   		Opener = fopen(Out->stats_file, "w");
			if (Opener == NULL) {
				fprintf(stderr, 
				        "\nERROR [INITIALIZE]: Failed to open stats file: [%s]:[%s]!\n",
				        Out->stats_file, strerror(errno));
				return 1;
			}
			(void)fclose(Opener);
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
			if(In->vent_count+firstvent) {
				//Reallocate memory for a larger Vent array
				*Vents = (VentArr*) realloc(*Vents, (sizeof (VentArr) * ((++In->vent_count+1))));
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
				(*Vents+In->vent_count)->min_pulsevolume     = 0;
				(*Vents+In->vent_count)->max_pulsevolume     = 0;
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
			if (dval > 0) (*Vents+In->vent_count)->min_pulsevolume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for MIN_PULSE_VOLUME\n");
				return 1;
			
			}
		}
		
		/*Maximum Pulse Volume*/
		else if (!strncmp(var, "MAX_PULSE_VOLUME", strlen("MAX_PULSE_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->max_pulsevolume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for MAX_PULSE_VOLUME\n");
				return 1;
			}
		}
		
		/*VENT TOTAL VOLUME*/
		else if (!strncmp(var, "VENT_TOTAL_VOLUME", strlen("VENT_TOTAL_VOLUME"))) {
			dval = strtod(value, &ptr);
			if (dval > 0) (*Vents+In->vent_count)->totalvolume = dval;
			else {
				fprintf(stderr, 
				        "\n[INITIALIZE]: Unable to read value for VENT_TOTAL_VOLUME\n");
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
		if strncmp(line, "NEW_VENT", strlen("NEW_VENT"))
			fprintf (stderr,"[assigned]\n");
	}

	/****************************************************************************
	                      CHECK FOR VALID PARAMETER SET
	****************************************************************************/
	
	//If SIMULATIONS > 1, and probabilistic parameters are given, determined parameters
	//   WILL BE OVERWRITTEN
	
	//EITHER NEW_VENT or SPATIAL_DENSITY_FILE MUST BE SET
	//IF NEW_VENT is not set, min/max volume parameters MUST BE SET
	//IF NEW_VENT is set, all VENT Parameter Types MUST BE SET
	//IF SPATIAL_DENSITY_FILE is set, all PROBABILISTIC parameters must be set
	
	//At least one output type must be set
	//A DEM must be set
	//Elevation uncertainty must be >=0 or -1 (file)
	//THE RESIDUAL either must be positive or -1 (file)

	/*Check for missing parameters*/
	fprintf(stderr, "Checking validity of parameter set ....\n");
	
	In->vent_count += firstvent; //Change vent_count from an index into a # of vents
	
	if(In->runs > 1) { // Multiple Simulations!! Prefer Probabilistic Parameters!
		if (!In->vent_count) { // No vents have been declared, run fully probabilistic
			if (In->spd_file==NULL || strlen (In->spd_file) < 2) { 
				//no spatial density file given
				fprintf(stderr, "\nERROR [INITIALIZE]: ");
				fprintf(stderr, "No VENT_SPATIAL_DENSITY_FILE or NEW_VENT assigned!!\n");
				return 1;
			}
			
			if ( ((*Vents+0)->northing != DBL_MAX) || ((*Vents+0)->easting != DBL_MAX) ) {
				//Vent location was declared without a NEW_VENT flag...
				fprintf(stderr, 
				        "\nERROR [INITIALIZE]: VENT Location given without NEW_VENT!!\n");
				fprintf(stderr, 
				        "\n  When Spatial Density File is given, vent locations should be omitted!!\n");
				return 1;
			}
			
			if ( ((*Vents+0)->min_pulsevolume <= 0) || 
			     ((*Vents+0)->max_pulsevolume < (*Vents+0)->min_pulsevolume) ) {
				// No Pulse Volume Range
				fprintf(stderr, 
				        "\nERROR [INITIALIZE]: MIN/MAX_PULSE_VOLUME not correctly given!!\n");
				return 1;
			}
			else (*Vents+0)->pulsevolume = 0; //reset totalvolume if range correctly set
			
			if ( ((*Vents+0)->min_totalvolume <= 0) || 
			     ((*Vents+0)->max_totalvolume < (*Vents+0)->min_totalvolume) ) {
				// No Total Volume Range
				fprintf(stderr, 
				        "\nERROR [INITIALIZE]: MIN/MAX_TOTAL_VOLUME not correctly given!!\n");
				return 1;
			}
			else (*Vents+0)->totalvolume = 0; //reset totalvolume if range correctly set
			
			if ( (In->min_residual <= 0) || (In->max_residual < In->min_residual) ) {
				// No Residual Thickness Range
				fprintf(stderr, 
				        "\nERROR [INITIALIZE]: MIN/MAX_RESIDUAL not correctly given!!\n");
				return 1;
			}
			else In->residual = 0; // reset residual if range correctly set
			
			
		}
		else { // There is at least one NEW_VENT declared!
			//Vent locations are required, everything else can be done in a few ways
			for(i=0; i < In->vent_count; i++) {
				if ((*Vents+i)->northing == DBL_MAX) { //Vent Northing
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: VENT_NORTHING not set for VENT #%d!!\n", i+1);
					return 1;
				}
				if ((*Vents+i)->easting == DBL_MAX) {  //Vent Easting
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: VENT_EASTING not set for VENT #%d!!\n", i+1);
					return 1;
				}
				//Vent Pulse Volume
				if ( ((*Vents+i)->min_pulsevolume > 0) && 
				   ((*Vents+i)->max_pulsevolume >= (*Vents+i)->min_pulsevolume) ) {
					//if probabilistic pulse volumes are given, reset determined pulse vol
					(*Vents+i)->pulsevolume = 0;
				}
				else if ((*Vents+i)->pulsevolume <= 0) { //No valid pulse volume is given
					fprintf(stderr, "\nERROR [INITIALIZE]: ");
					fprintf(stderr, "VENT_PULSE_VOLUME, or MIN/MAX_PULSE_VOLUME ");
					fprintf(stderr, "not correctly set for VENT #%d!!\n", i+1);
					return 1;
				}
				
				//Vent Total Volume
				if ( ((*Vents+i)->min_totalvolume > 0) && 
				   ((*Vents+i)->max_totalvolume >= (*Vents+i)->min_totalvolume) ) {
					//if probabilistic total volumes are given, reset determined total vol
					(*Vents+i)->totalvolume = 0;
				}
				else if ((*Vents+i)->totalvolume <= 0) { //No valid total volume is given
					fprintf(stderr, "\nERROR [INITIALIZE]: ");
					fprintf(stderr, "VENT_PULSE_VOLUME, or MIN/MAX_PULSE_VOLUME ");
					fprintf(stderr, "not correctly set for VENT #%d!!\n", i+1);
					return 1;
				}
			}
			
			//Residual Thickness
			if ((In->min_residual > 0) && (In->max_residual >= In->min_residual)) {
				//reset residual thickness
				In->residual = 0;
			}
			else if (In->residual <= 0) { // no residual or range is given!
				fprintf(stderr, "\nERROR [INITIALIZE]: ");
				fprintf(stderr, "RESIDUAL_THICKNESS, or MIN/MAX_RESIDUAL");
				fprintf(stderr, "not correctly set!!\n");
				return 1;
			}
			
			
			In->spd_file = NULL;
		}
	}
	else if (In->runs == 1) { //1 Simulation!! Prefer Determined Parameters!
		if (!In->vent_count) { // No Vents declared but only one simulation - this is an error
			fprintf(stderr, "\nERROR [INITIALIZE]: No vent declared for single simulation!!\n");
			fprintf(stderr, "  Include NEW_VENT and VENT parameters in configuration file\n");
			return 1;
		}
		else { // Vents have been declared. Cycle through vents to check for parameters
			for(i=0; i < In->vent_count; i++) {
				if ((*Vents+i)->northing == DBL_MAX) { //Vent Northing
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: VENT_NORTHING not set for VENT #%d!!\n", i+1);
					return 1;
				}
				if ((*Vents+i)->easting == DBL_MAX) {  //Vent Easting
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: VENT_EASTING not set for VENT #%d!!\n", i+1);
					return 1;
				}
				if ((*Vents+i)->pulsevolume <= 0) { //Vent Pulse Volume
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: VENT_PULSE_VOLUME not correctly set for VENT #%d!!\n", i+1);
					return 1;
				}
				else { 
					//if pulse volume is given correctly, reset probabilistic params to 0
					(*Vents+i)->min_pulsevolume = (*Vents+i)->max_pulsevolume = 0;
				}
				if ((*Vents+i)->totalvolume <= 0) { //Vent Total Volume
					fprintf(stderr, 
					        "\nERROR [INITIALIZE]: VENT_TOTAL_VOLUME not correctly set for VENT #%d!!\n", i+1);
					return 1;
				}
				else { 
					//if total volume is given correctly, reset probabilistic params to 0
					(*Vents+i)->min_totalvolume = (*Vents+i)->max_totalvolume = 0;
				}
			}
			
			//reset spatial density file
			In->spd_file = NULL;
		}
	}
	else { // <1 Simulation! Bad user input!
		fprintf(stderr, "\nERROR [INITIALIZE]: Number of simulations to run <= 0!!\n");
		return 1;
	}
	
	
	if(strlen (In->dem_file) < 2) { /*DEM Filename is missing.*/
		fprintf(stderr, "\nERROR [INITIALIZE]: No DEM Filename Given!!\n");
		return 1;
	}
	
	if(!In->elev_uncert) { /*Elevation uncertainty is either missing or is 0.*/
		fprintf(stderr, "  ELEVATION UNCERTAINTY = 0: DEM values are assumed to be true.\n");
	}
	else if ((In->elev_uncert < 0)  && (In->elev_uncert != -1)) {
		fprintf(stderr, "\nERROR [INITIALIZE]: DEM Error Uncertainty <= 0!!\n");
		return 1;
	}
	
	fprintf(stderr, "Parameter set is valid!\n");
	
	(void) fclose(ConfigFile);
	return 0;
}
