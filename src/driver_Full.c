#include "prototypes.h"

int main(int argc, char *argv[]) {
/*
DRIVER_00 is a VENT FLUX LIMITED flow scheme! The flow will end when all vents
          have no more lava to effuse.
*/
/*DRIVER for a lava flow code ALGORITHM:
         Read in a Configuration File with INITIALIZE
         Load a DEM Raster with DEM_LOADER and assign parameters to a data grid
         Create Cellular Automata lists and assign source vents with INIT_FLOW
         
         Main Flow Loop:
           If there is more volume to erupt at source vents, call PULSE
           Move lava from cells to neighbors with DISTRIBUTE
         
         After flow is completely erupted:
           Check for Conservation of Mass
           Write out requested Model Output to user-defined file paths
*/
	
	/*VARIABLES******************************************************************/
	/*Main Arrays*/
	DataCell **dataGrid = NULL;         /*Global Data Grid                      */
	Automata *CAList = NULL;                  /*Cellular Automata Lists (Active Cells)*/
	VentArr  *Vents;                    /*Source Vent List                      */
	FlowStats FlowParam;                   //Flow parameter information
	DataCell **SpatialDensity = NULL;
	
	/*Model Parameters*/
	Inputs In;           /* Structure to hold model inputs named in Config file  */
	Outputs Out;         /* Structure to hold model outputs named in config file */
	Outputs Out_eachrun;
	int size;            /* variable used for creating seed phrase               */
	char *phrase;        /* seed phrase for random number generator              */
	int seed1;           /* random seed number */
 	int seed2;           /* random seed number */
	int      i=0,j=0, ret=0;            /*loop variables, function return value */
	//int      pulseCount  = 0;           /*Current number of Main PULSE loops    */
	
	/*Physical Parameters*/
	double   volumeErupted = 0;         /*Total Volume in All Active Cells      */
	
	
	/*TIME AND RANDOM NUMBER GEN*************************************************/
	GC_INIT();
	/* Define Start Time */
	startTime = time(NULL); 
	/* Seed random number generator */
	srand(time(NULL));             
	size = (size_t)(int)(startTime + 1);
	
	phrase = (char *)GC_MALLOC_ATOMIC(((size_t)size * sizeof(char)));	
  	if (phrase == NULL) {
    	fprintf(stderr, "Error: Cannot malloc memory for seed phrase:[%s]\n",
    	        strerror(errno));
    	return 1;
    }
    snprintf(phrase, size, "%d", (int)startTime);
    
	/* Initialize the generators. */
  	initialize ( );
	/* Set the seeds based on the phrase. */
  	phrtsd ( phrase, &seed1, &seed2 );
	/* Initialize all generators. */
  	set_initial_seed ( seed1, seed2 );
	/*WELCOME USER TO SIMULATION AND CHECK FOR CORRECT USAGE*********************/
	fprintf(stdout, "\n\n               MOLASSES is a lava flow simulator.\n\n");
	
	/*User must supply the name of the executable and a configuration file*/
	if(argc<2) {
		fprintf(stderr, "Usage: %s config-filename\n",argv[0]);
		return 1;
	}
	In.config_file = argv[1];
	if (argc == 3) {
		In.start = atoi(argv[2]);
		if (In.start < 0) In.start = 0;
	}
	else In.start = 0;
	
	fprintf(stdout, "Beginning flow simulation...\n");
	
	/*MODULE: INITIALIZE*********************************************************/
	/*        Assigns several empty variables based on a user-defined 
	            configuration file.                                             */
	
	ret = INITIALIZE(&In,        /* Input parameters structure  */
	                 &Out,       /* Output parameters structure */
	                 &Vents);    /* VentArr Structure           */
	                	
	/*Check for Error flag (INITIALIZE returns 1 if error, 0 if no errors)*/
	if(ret){
		fprintf(stderr, "\n[MAIN]: Error flag returned from [INITIALIZE].\n");
		fprintf(stderr, "Exiting.\n");
		return 1;
	}
	
	
	/*MODULE: DEM_LOADER*********************************************************/
	/*        Loads Raster into Global Data Grid based on code:
	            TOPOG - Loads a DEM raster into the data grid's dem_elev value
	            RESID - Loads a raster into the data grid's residual value
	            T_UNC - Loads a raster into the data grid's elev_uncert value
	          Returns a metadata list of geographic coordinates of the raster   */
	/*        DEMmetadata format:
		          [0] lower left x
		          [1] w-e pixel resolution
		          [2] number of cols, assigned manually
		          [3] lower left y
		          [4] number of lines, assigned manually
		          [5] n-s pixel resolution (negative value)                       */
	
	/*Assign Topography to Data Grid Locations*/
	In.dem_grid_data = DEM_LOADER(In.dem_file, /*char            DEM file name   */
	                         &dataGrid,    /*DataCell        Global Data Grid*/
	                         "TOPOG"       /*DEM_LOADER Code Topography      */
	                        );
	/*Check for Error flag (DEM_LOADER returns a null metadata list)*/
	if(In.dem_grid_data==NULL){
		fprintf(stderr, "\nError [MAIN]: Error flag returned from DEM_LOADER[TOPOG].\n");
		fprintf(stderr, "Exiting.\n");
		return(-1);
	}
	
	//Initialize Flow Parameter Set
	FlowParam.ca_list_size     = 0;
	FlowParam.active_count     = 0;
	FlowParam.vent_count       = 0;
	FlowParam.run              = 0; //this is actually important
	FlowParam.residual         = 0;
	FlowParam.remaining_volume = 0;
	FlowParam.total_volume     = 0;
	
	//Initialize the Outputs performed for each run (stats suggested)
	Out_eachrun.ascii_flow_file  = NULL;
	Out_eachrun.ascii_hits_file  = NULL;
	Out_eachrun.raster_hits_file = NULL;
	Out_eachrun.raster_flow_file = NULL;
	Out_eachrun.raster_post_topo = NULL;
	Out_eachrun.raster_pre_topo  = NULL;
	Out_eachrun.stats_file       = Out.stats_file;
	
	for (FlowParam.run = In.start; FlowParam.run < (In.runs + In.start); 
	     FlowParam.run++) {
		
		fprintf(stdout, "\n______________________________________________________");
		fprintf(stdout, "____________\n                         SIMULATION #%d\n\n",
		                (FlowParam.run+1));
		/*MODULE: INIT_FLOW**********************************************************/
		/*        Creates Active Cellular Automata lists and activates vents in them.
				      Also creates bookkeeper variables: 
				        total size of CA lists
				        total number of CA lists
				        total number of active automata in the CA list
				        total volume to erupt (combined volumes to erupt at vents)      */

		ret = INIT_FLOW(&dataGrid,      /*DataCell  Global Data Grid                 */
				            &CAList,        /*Automaton Active Cells List                */
				            Vents,          /*VentArr   Vent Data Array                  */
				            &In,
				            &FlowParam,
				            &SpatialDensity
				           );
		/*Check for Error flag (INIT_FLOW returns <0 value)*/
		if(ret) {
			fprintf(stderr, "\nError [MAIN]: Error flag returned from [INIT_FLOW].\n");
			fprintf(stderr, "Exiting.\n");
			return 1;
		}
		
		

	/****************************************************************************/
	/*MAIN FLOW LOOP: PULSE LAVA AND DISTRIBUTE TO CELLS*************************/
	/****************************************************************************/
		ret = SIMULATION(dataGrid, 
		                 CAList, 
		                 &Vents, 
		                 In, 
		                 &FlowParam
		                );
		fprintf(stdout,"\n");
		if (ret==2) {//OFF THE MAP ERROR
			if (FlowParam.run == 0) {
				//IF THE FLOW IS OFF THE MAP AND ITS THE FIRST FLOW, STILL OUTPUT THE 
				//STATS FILE HEADER, IF A STATS FILE WAS REQUESTED
				Vents[0].remainingvolume = DBL_MAX;
				ret = OUTPUT(Out_eachrun,
							       In,
							       dataGrid,         /*DataCell  Global Data Grid           */
							       CAList,        /*Automaton Active Cells List          */
							       FlowParam, /*unsigned  Number of active cells     */
							       Vents
							      );
				if(ret){ /*Check for Error flag (OUTPUT returns !0 value)*/
					fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
					fprintf(stderr, "Exiting.\n");
					return 1;
				}
			}
			fprintf(stdout, "  Continuing to next flow.\n");
			continue;
		}
		else if (ret) {
			fprintf (stderr, "Error [MAIN]: Error flag returned from [SIMULATION]\n");
			return 1;
		}
		
		
		/*POST FLOW WRAP UP**********************************************************/
	
		fprintf(stdout, "\n                        Flow Complete!\n\n");
		/*Print out final number of inundated cells*/
		fprintf(stdout, "Final Count: %d cells inundated.\n", 
		        FlowParam.active_count);
	
	
		/*POST FLOW WRAP UP: CONSERVATION OF MASS CHECK******************************/
		/*ALL DRIVER MODULES MUST HAVE THIS. In order to do unit testing during
			code compilation, makefile searches for the string "SUCCESS: MASS CONSERVED"
			to conclude that the code is Verified (though not validated).*/
	
		volumeErupted = 0;
		/*For each Active Flow Cell, add cell lava volume to volumeErupted*/
		for(i=1;i<=FlowParam.active_count;i++)
			volumeErupted += (CAList[i].thickness + 
				             dataGrid[CAList[i].row][CAList[i].col].residual) *
				             In.dem_grid_data[1] * In.dem_grid_data[5];
	
		/*print out volume delivered to vents and total volume now in cells*/
		
		//printf(" Total (IN) volume pulsed from vents:   %0.3f\n",FlowParam.total_volume);
		//printf(" Total (OUT) volume found in cells:     %0.3f\n",volumeErupted);
		/*Double data types are precise to 1e-8, so make sure that volume IN and
			volume OUT are within this precision.*/
		if(abs(volumeErupted-FlowParam.total_volume)<=1e-8) {
			if (FlowParam.run == (In.runs + In.start-1)) {
				fprintf(stdout, "Conservation of mass check (printed out for last flow)\n");
				fprintf(stdout, " SUCCESS: MASS CONSERVED\n\n");
			}
		}
		/*If volumes are significantly different (are more than Double Precision diff.
			then mass is NOT conserved!!*/
		else {
			/*Print the mass excess*/
			fprintf(stderr, " ERROR: MASS NOT CONSERVED! Excess: %0.2e m^3\n",
				     volumeErupted-FlowParam.total_volume);
			fprintf(stderr, "Program stopped\n");
			return 1;
		}
	
		//Append active list to hit count
		for(i=0; i < In.dem_grid_data[4]; i++) {
			for(j=0; j < In.dem_grid_data[2]; j++) {
				if (dataGrid[i][j].active) dataGrid[i][j].hit_count++;
			}
		}
	
		/*Output some model results for each Run                                    */
		/*MODULE: OUTPUT*************************************************************/
		/*        Writes out model output to a file path.
		*/
		ret = OUTPUT(Out_eachrun,
			           In,
			           dataGrid,         /*DataCell  Global Data Grid           */
			           CAList,        /*Automaton Active Cells List          */
			           FlowParam, /*unsigned  Number of active cells     */
			           Vents
			          );
		if(ret){ /*Check for Error flag (OUTPUT returns !0 value)*/
			fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
			fprintf(stderr, "Exiting.\n");
			return 1;
		}
	
	} /*END OF FOR (RUNS) LOOP                     */
	
	
	fprintf(stdout, "\nModel Output:\n");
	/*MODULE: OUTPUT*************************************************************/
	/*        Writes out model output to a file path.
	          File Output types available, and their codes:
	            ascii_flow_file:  X,Y,Thickness ascii flow list
	            ascii_hits_file:  Hit XY (Coordinate If Hit)
	            raster_hits_file: Hit Raster (1 = Hit, 0 = Not Hit)
	            raster_flow_file: Thickness Raster
	            raster_pre_topo:  Elevation Raster
	            raster_post_topo: Elevation + Lava Raster (pre_topo + flow)
	            stats_file:       Gives statistics on the flow(s)
	          
	          These are elements of the Outputs data structure (Out) 
	          
	          Codes are in File_output_type structure:
	            ascii_flow,
	            ascii_hits,
	            raster_hits,
	            raster_flow,
	            raster_post,
	            raster_pre,
	            stats_file
	          */
	Out.stats_file = NULL; //Stats were printed for each run, so don't reprint
	
	ret = OUTPUT(Out,
	             In,
	             dataGrid,         /*DataCell  Global Data Grid           */
	             CAList,        /*Automaton Active Cells List          */
	             FlowParam, /*unsigned  Number of active cells     */
	             Vents
	            );
	if(ret){ /*Check for Error flag (OUTPUT returns !0 value)*/
		fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
		fprintf(stderr, "Exiting.\n");
		return 1;
	}
	
	/*Calculate simulation time elapsed, and print it.*/
	endTime = time(NULL);
	fprintf(stdout, "\nElapsed Time of simulation approximately %u seconds.\n\n",
	       (unsigned)(endTime - startTime));
	
	return(0);
}
