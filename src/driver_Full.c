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
    	fprintf(stderr, "Cannot malloc memory for seed phrase:[%s]\n", strerror(errno));
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
	printf("\n\n               MOLASSES is a lava flow simulator.\n\n");
	
	/*User must supply the name of the executable and a configuration file*/
	if(argc<2) {
		printf("Usage: %s config-filename\n",argv[0]);
		return 1;
	}
	In.config_file = argv[1];
	if (argc == 3) {
		In.start = atoi(argv[2]);
		if (In.start < 0) In.start = 0;
	}
	else In.start = 0;
	
	printf("Beginning flow simulation...\n");
	
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
		printf("\nError [MAIN]: Error flag returned from DEM_LOADER[TOPOG].\n");
		printf("Exiting.\n");
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
		
		fprintf(stderr, "\n______________________________________________________");
		fprintf(stderr, "____________\n                         SIMULATION #%d\n\n",
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
			printf("\nError [MAIN]: Error flag returned from [INIT_FLOW].\n");
			printf("Exiting.\n");
			return 1;
		}
		
		
		ret = SIMULATION(dataGrid, 
		                 CAList, 
		                 &Vents, 
		                 In, 
		                 &FlowParam);
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
					printf("\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
					printf("Exiting.\n");
					return 1;
				}
			}
			fprintf(stderr, "  Continuing to next flow.\n");
			continue;
		}
		else if (ret) {
			fprintf (stderr, "Error [MAIN]: Error flag returned from [SIMULATION]\n");
			return 1;
		}
		
		
		/*POST FLOW WRAP UP**********************************************************/
	
		printf("\n                        Flow Complete!\n\n");
		/*Print out final number of inundated cells*/
		printf("Final Count: %d cells inundated.\n", FlowParam.active_count);
	
	
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
		printf("Conservation of mass check\n");
		//printf(" Total (IN) volume pulsed from vents:   %0.3f\n",FlowParam.total_volume);
		//printf(" Total (OUT) volume found in cells:     %0.3f\n",volumeErupted);
		/*Double data types are precise to 1e-8, so make sure that volume IN and
			volume OUT are within this precision.*/
		if(abs(volumeErupted-FlowParam.total_volume)<=1e-8)
			fprintf(stderr, " SUCCESS: MASS CONSERVED\n\n");
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
			printf("\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
			printf("Exiting.\n");
			return 1;
		}
	
	} /*END OF FOR (RUNS) LOOP                     */
	
	
	fprintf(stderr, "\nModel Output:\n");
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
		printf("\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
		printf("Exiting.\n");
		return 1;
	}
	
	/*Calculate simulation time elapsed, and print it.*/
	endTime = time(NULL);
	printf("\nElapsed Time of simulation approximately %u seconds.\n\n",
	       (unsigned)(endTime - startTime));
	
	return(0);
}













	/****************************************************************************/
	/*MAIN FLOW LOOP: PULSE LAVA AND DISTRIBUTE TO CELLS*************************/
	/****************************************************************************/

int SIMULATION(DataCell **dataGrid, Automata *CAList, VentArr **Vents, 
               Inputs In, FlowStats *FlowParam) {
	int ret;

	printf("\n                         Running Flow\n");


	/*Loop to call PULSE and DISTRIBUTE only if volume remains to be erupted*/
	while(FlowParam->remaining_volume > 0) {
		/*MODULE: PULSE************************************************************/
		/*        Delivers lava to vents based on information in Vent Data Array.
			        Returns total volume remaining to be erupted.                   */
	
		ret = PULSE(CAList,        /*Automaton Active Cells List               */
			          Vents,           /*VentArr   Vent Data Array                 */
			          In,
			          FlowParam
			         );
	
		/*Check for Error flags (PULSE returns <0 or 0 value)*/
		if(ret > 1) {
			printf("\nERROR [MAIN]: Error flag returned from [PULSE].\n");
			printf("Exiting.\n");
			return 1;
		}
		else if (ret) {
			if (FlowParam->remaining_volume) {
				/*This return should not be possible, 
					Pulse should return 0 if no volume remains*/
				printf("\nERROR [MAIN]: Error between [PULSE] return and lava vol.\n");
				printf("Exiting.\n");
				return 1;
			}
			/*If ret=1, PULSE was called even though there was no lava to distribute.
				Do not call Pulse or Distribute anymore! Break out of While loop.     */
			break;
		}
		/*if Pulse module successfully updated vents, ret will > 0.
			Continue, call Distribute module.*/
	
		/*Update status message on screen*/
		fprintf(stderr,"\rInundated Cells: %-7d; Volume Remaining: %10.2f",
			     FlowParam->active_count, FlowParam->remaining_volume);
	
	
		/*MODULE: DISTRIBUTE*******************************************************/
		/*        Distributes lava from cells to neighboring cells depending on
			        module specific algorithm (e.g. slope-proportional sharing).
			        Updates a Cellular Automata List and the active cell counter.*/
	
		ret = DISTRIBUTE(dataGrid,          /*DataCell  Global Data Grid       */
			               CAList,         /*Automaton Active Cells List      */
			               &FlowParam->active_count, /*unsigned  Number of active cells */
			               In.dem_grid_data        /*double    Geographic Metadata    */
			              );
	
		/*Check for Error flag (DISTRIBUTE returns <0 value)*/
		if(ret == 2) { //OFF THE MAP ERROR, continue to next run
			return 2;
		}
		else if (ret) { // GENERAL ERROR, Stop program
			printf("\nERROR [MAIN]: Error flag returned from [DISTRIBUTE].\n");
			printf("Exiting.\n");
			return 1;
		}
	
		/*If you want to output the flow at EVERY Pulse, here is a good place to do
			it.
			Increment pulse count, then rename the temporary file path.*/
		//snprintf(tempFilename,15,"pulse_%04d.xyz",(++pulseCount));
	
	
	} /*End while main flow loop: (while(volumeRemaining>0)and Flow Motion)*/
	
	/*SUCCESSFUL SIMULATION*/
	return 0;
}
