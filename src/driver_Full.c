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
	/*Files*/
	char     **Filenames;               /*A list of file paths for model output */
	char     tempFilename[15];          /*A temporary file path for whatever use*/
	
	/*Main Arrays*/
	DataCell **dataGrid = NULL;         /*Global Data Grid                      */
	Automata *CAList = NULL;                  /*Cellular Automata Lists (Active Cells)*/
	VentArr  *Vents;                    /*Source Vent List                      */
	unsigned ActiveCounter = 0;            /*Number of Active Cells in CA List     */
	FlowStats FlowParam;                   //Flow parameter information
	DataCell **SpatialDensity = NULL;
	
	/*Model Parameters*/
	Inputs In;           /* Structure to hold model inputs named in Config file  */
	Outputs Out;         /* Structure to hold model outputs named in config file */
	int size;            /* variable used for creating seed phrase               */
	char *phrase;        /* seed phrase for random number generator              */
	int seed1;           /* random seed number */
 	int seed2;           /* random seed number */
	int      i=0,j=0, ret=0;            /*loop variables, function return value */
	int      pulseCount  = 0;           /*Current number of Main PULSE loops    */
	
	/*Physical Parameters*/
	double   *DEMmetadata;              /*Geographic Coordinates of DEM Raster  */
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
		return(-1);
	}
	
	In.config_file = argv[1];
	
	printf("Beginning flow simulation...\n");
	
	/*MODULE: INITIALIZE*********************************************************/
	/*        Assigns several empty variables based on a user-defined 
	            configuration file.                                             */
	/*        File Name List output in this order:
	            [0] - DEM
	            [1] - Residual Flow Thickness
	            [2] - Elevation Uncertainty
	            [3] - Output file: ASCII X,Y,Thickness
	            [4] - Output file: Hit Map
	            [5] - Output file: Raster Thickness
	            [6] - Output file: Raster Elevation
	            [7] - Output file: Raster Elevation + Flow Thickness            */
	
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
	                In,
	                &FlowParam,
	                &SpatialDensity
	//               &CAListSize,    /*unsigned  Size of each empty CA List       */
	//               ventCount,      /*unsigned  Number of Vents                  */
	//               &ActiveCounter, /*unsigned  Number of active cells in CA List*/
	//               &volumeToErupt  /*double    Volume that the model will expel */
	              );
	
	/*Check for Error flag (INIT_FLOW returns <0 value)*/
	if(ret) {
		printf("\nError [MAIN]: Error flag returned from [INIT_FLOW].\n");
		printf("Exiting.\n");
		return 1;
	}
	
	
	
	/****************************************************************************/
	/*MAIN FLOW LOOP: PULSE LAVA AND DISTRIBUTE TO CELLS*************************/
	/****************************************************************************/
	
	printf("\n                         Running Flow\n");
	
	
	/*Loop to call PULSE and DISTRIBUTE only if volume remains to be erupted*/
	while(FlowParam.remaining_volume > 0) {
		/*MODULE: PULSE************************************************************/
		/*        Delivers lava to vents based on information in Vent Data Array.
		          Returns total volume remaining to be erupted.                   */
		
		ret = PULSE(CAList,        /*Automaton Active Cells List               */
		            &Vents,           /*VentArr   Vent Data Array                 */
		            In,
		            &FlowParam
		           );
		
		/*Check for Error flags (PULSE returns <0 or 0 value)*/
		if(ret > 1) {
			printf("\nERROR [MAIN]: Error flag returned from [PULSE].\n");
			printf("Exiting.\n");
			return -1;
		}
		else if (ret) {
			if (FlowParam.remaining_volume) {
				/*This return should not be possible, 
				  Pulse should return 0 if no volume remains*/
				printf("\nERROR [MAIN]: Error between [PULSE] return and lava vol.\n");
				printf("Exiting.\n");
				return(-1);
			}
			/*If ret=1, PULSE was called even though there was no lava to distribute.
			  Do not call Pulse or Distribute anymore! Break out of While loop.     */
			break;
		}
		/*if Pulse module successfully updated vents, ret will > 0.
		  Continue, call Distribute module.*/
		
		/*Update status message on screen*/
		fprintf(stderr,"\rInundated Cells: %-7d; Volume Remaining: %10.2f",
		       FlowParam.active_count, FlowParam.remaining_volume);
		
		
		/*MODULE: DISTRIBUTE*******************************************************/
		/*        Distributes lava from cells to neighboring cells depending on
		          module specific algorithm (e.g. slope-proportional sharing).
		          Updates a Cellular Automata List and the active cell counter.*/
		
		ret = DISTRIBUTE(dataGrid,          /*DataCell  Global Data Grid       */
		                 CAList,         /*Automaton Active Cells List      */
		                 &FlowParam.active_count, /*unsigned  Number of active cells */
		                 In.dem_grid_data        /*double    Geographic Metadata    */
		                );
		
		/*Check for Error flag (DISTRIBUTE returns <0 value)*/
		if(ret<0) {
			printf("\nERROR [MAIN]: Error flag returned from [DISTRIBUTE].\n");
			printf("Exiting.\n");
			return(-1);
		}
		
		/*If you want to output the flow at EVERY Pulse, here is a good place to do
		  it. A temporary file name variable is declared using the number of times 
		  this Pulse loop has been completed, then the OUTPUT module is called.
		  Uncomment the lines below if you want this. Commenting out the file name
		  declaration creates warnings since the variables won't later be used, so
		  I've left it uncommented out. (It only runs once per pulse, so it doesn't
		  slow the code down).*/
		
		/*increment pulse count, then rename the temporary file path.*/
		snprintf(tempFilename,15,"pulse_%04d.xyz",(++pulseCount));
		/*MODULE: OUTPUT**************************************/
		/*        writes out a file. Arguments:
		            DataCell  Global Data Grid
		            Automaton Active Cells List
		            unsigned  Number of active cells
		            string    Output Filename
		            OUTPUT Code: 0 = ASCII X,Y,Thickness File
		            double    Geographic Metadata
		            string    Original Raster Projection     */
		/*
		ret = OUTPUT(dataGrid,
		             CAList,
		             ActiveCounter,
		             tempFilename,
		             0,
		             DEMmetadata,
		             ""
		            );
		*/
		/*Check for Error flag (OUTPUT returns <0 value)*/
		/*
		if(ret<0){
			printf("\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
			printf("Exiting.\n");
			return(-1);
		}
		*/
		
		
	} /*End while main flow loop: (while(volumeRemaining>0)and Flow Motion)*/
	
	
	/*POST FLOW WRAP UP**********************************************************/
	
	printf("\n\n                     Single Flow Complete!\n");
	
	/*Print out final number of inundated cells*/
	printf("Final Count: %d cells inundated.\n\n", FlowParam.active_count);
	
	
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
	printf(" Total (IN) volume pulsed from vents:   %0.3f\n",FlowParam.total_volume);
	printf(" Total (OUT) volume found in cells:     %0.3f\n",volumeErupted);
	/*Double data types are precise to 1e-8, so make sure that volume IN and
	  volume OUT are within this precision.*/
	if(abs(volumeErupted-FlowParam.total_volume)<=1e-8)
		printf(" SUCCESS: MASS CONSERVED\n");
	/*If volumes are significantly different (are more than Double Precision diff.
	  then mass is NOT conserved!!*/
	else 
		/*Print the mass excess*/
		printf(" ERROR: MASS NOT CONSERVED! Excess: %0.2e m^3",
		       volumeErupted-FlowParam.total_volume);
	
	
	/*MODULE: OUTPUT*************************************************************/
	/*        Writes out model output to a file path.
	          File Output types available, and their codes:
	            0: X,Y,Thickness ascii flow list
	            1: Hit Raster (1 = Hit, 0 = Not Hit)
	            2: Thickness Raster
	            3: Elevation Raster
	            4: Elevation + Lava Raster (code 2 + code 3)
	          
	          Filename Index output from INITIALIZE:
	            Filenames[3] - Output file: ASCII X,Y,Thickness
	            Filenames[4] - Output file: Hit Map
	            Filenames[5] - Output file: Raster Thickness
	            Filenames[6] - Output file: Raster Elevation
	            Filenames[7] - Output file: Raster Elevation + Flow Thickness   */
	
	/*Check Filenames Array to see if a filename was given (so model output is
	  requested).*/
	for(i=0;i<4;i++){
		/*Check to see if the File Path is not empty (the following test will !=0)*/
		if(strlen(Filenames[i+3]) > 1) {
		/*If there's a file path given, write model output to it.*/
			ret = OUTPUT(dataGrid,         /*DataCell  Global Data Grid           */
			             CAList,        /*Automaton Active Cells List          */
			             ActiveCounter, /*unsigned  Number of active cells     */
			             Filenames[i+3],   /*string    Output File Path           */
			             i,                /*OUTPUT Code, see above               */
			             In.dem_grid_data,""    /*string    Original Raster Projection */
			            );
			
			/*Check for Error flag (OUTPUT returns <0 value)*/
			if(ret<0){
				printf("\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
				printf("Exiting.\n");
				return(-1);
			}
		}
	}
	
	/*Calculate simulation time elapsed, and print it.*/
	endTime = time(NULL);
	printf("\nElapsed Time of simulation approximately %u seconds.\n\n",
	       (unsigned)(endTime - startTime));
	
	return(0);
}
