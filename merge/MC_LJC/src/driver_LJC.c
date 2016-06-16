#include <prototypes_LJC.h>
#define BUFSIZE 15

/******************************************************************************/

void test_genunf ( char *phrase, double l, double h )

/******************************************************************************/
/*
  Purpose:
    TEST_GENUNF tests GENUNF, which generates uniform deviates.
  Licensing:
    This code is distributed under the GNU LGPL license.
  Modified:
    02 April 2013
  Author:
    John Burkardt
*/
{
  float a;
  float *array;
  float av;
  float avtr;
  float b;
  int i;
  int n = 1000;
  float param[2];
  char pdf[] = "unf";
  int seed1;
  int seed2;
  float var;
  float vartr;
  float xmax;
  float xmin;

  printf ( "\n" );
  printf ( "TEST_GENUNF\n" );
  printf ( "  Test GENUNF,\n" );
  printf ( "  which generates uniform deviates.\n" );
/*  Initialize the generators.*/
  initialize ( );
/*  Set the seeds based on the phrase.*/
  phrtsd ( phrase, &seed1, &seed2 );
/* Initialize all generators.*/
  set_initial_seed ( seed1, seed2 );
/* Select the parameters at random within a given range.*/
  a = (float) l;
  b = (float) h;
  /*
  a = genunf ( low, high );
  low = a + 1.0;
  high = a + 10.0;
  b = genunf ( low, high );
  */
  printf ( "\n" );
  printf ( "  N = %d\n", n );
  printf ( "\n" );
  printf ( "  Parameters:\n" );
  printf ( "\n" );
  printf ( "  A = %g\n", a );
  printf ( "  B = %g\n", b );
/*Generate N samples.*/
  array = ( float * ) malloc ( n * sizeof ( float ) );
  for ( i = 0; i < n; i++ )
  {
    array[i] = genunf ( a, b );
    /*fprintf (stderr, "%g ", array[i]);*/
  }
/*  Compute statistics on the samples.*/
  stats ( array, n, &av, &var, &xmin, &xmax );
/*  Request expected value of statistics for this distribution.*/
  param[0] = a;
  param[1] = b;
  trstat ( pdf, param, &avtr, &vartr );
  printf ( "\n" );
  printf ( "  Sample data range:          %14g  %14g\n", xmin, xmax );
  printf ( "  Sample mean, variance:      %14g  %14g\n", av,   var );
  printf ( "  Distribution mean, variance %14g  %14g\n", avtr, vartr );
  free ( array );
  return;
}

int main(int argc, char *argv[]) {
/*
DRIVER_LJC is a VENT FLUX LIMITED flow scheme! The flow will end when all vents
          have no more lava to effuse.

DRIVER for a lava flow code ALGORITHM:
         Read in a Configuration File with INITIALIZE
         Load a DEM Raster with DEM_LOADER and assign parameters to a data grid
         Create Cellular Automata list and assign source vent with INIT_FLOW
         
         Main Flow Loop:
           If there is more volume to erupt at source vent, call PULSE
           Move lava from cells to neighbors with DISTRIBUTE
         
         After flow is completely erupted:
           Check for Conservation of Mass
           Write out requested Model Output to user-specified files
*/
	
	/*Main Arrays*/
	DataCell **dataGrid = NULL;      /* Global Data Grid                    */
	Automata *CAList = NULL;         /* Active Cell list                    */
	VentArr  Vent;                   /* Source Vent structure               */
	unsigned ActiveCounter = 0;      /* Maintains current # of Active Cells */
	
	/*Model Parameters*/
	Inputs In;           /* Structure to hold model inputs named in Config file  */
	Outputs Out;         /* Structure to hold model outputs named in config file */
	int size;            /* variable used for creating seed phrase               */
	char *phrase;        /* seed phrase for random number generator              */
	int seed1;           /* random seed number */
 	int seed2;           /* random seed number */
	int      i,j, ret;                  /* loop variables, function return value            */
	unsigned CAListSize  = 0;           /* Size of CA List, def in INIT_FLOW                */
	int      pulseCount  = 0;           /* Current number of Main PULSE loops               */
	
	/*Physical Parameters*/
	double   *DEMmetadata = NULL;       /* Geographic Metadata  GDAL             */
	double   volumeToErupt = 0;         /* Total Lava Volume to be erupted       */
	double   volumeErupted = 0;         /* Total Lava Volume in All Active Cells */
	double   volumeRemaining = 0;	     /* Volume Remaining to be Erupted        */
	double	total = 0;
	float	log_min;
	float	log_max;
	char thisfile[10];	
	int run = 0;       /* Maintains current lava flow run */ 
	int start = 0;     /* run number to start, from command line or 0 */
	
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
    fprintf(stderr, "Seeding random number generator: %s\n", phrase);
    
	/* Initialize the generators. */
  	initialize ( );
	/* Set the seeds based on the phrase. */
  	phrtsd ( phrase, &seed1, &seed2 );
	/* Initialize all generators. */
  	set_initial_seed ( seed1, seed2 );
    
	fprintf(stderr, "\n\n               MOLASSES is a lava flow simulator.\n\n");
	
	/*User must supply the name of the executable and a configuration file*/
	if(argc < 2) {
		fprintf(stderr, "Usage: %s config-filename\n",argv[0]);
		return 1;
	}
	if (argc == 3) {
		start = atoi(argv[2]);
		if (start < 0) start = 0;
	}
	
	fprintf(stderr, "Beginning flow simulation...\n");	
    
	In.config_file = argv[1]; 
	fprintf(stderr, "%s\n", In.config_file);
	
	ret = INITIALIZE(&In,        /* Input parameters structure  */
	                  &Out,       /* Output parameters structure */
	                  &Vent);     /* VentArr Structure           */
	                	
	/*Check for Error flag (INITIALIZE returns 1 if error, 0 if no errors)*/
	if(ret){
		fprintf(stderr, "\n[MAIN]: Error flag returned from [INITIALIZE].\n");
		fprintf(stderr, "Exiting.\n");
		return 1;
	}
	
	/*MODULE: DEM_LOADER*********************************************************
	          Loads Raster into Global Data Grid based on code:
	            TOPOG - Loads a DEM raster into the data grid's dem_elev value
	            RESID - Loads a raster into the data grid's residual value
	            T_UNC - Loads a raster into the data grid's elev_uncert value
	          Returns a metadata list of geographic coordinates of the raster   
	          DEMmetadata format:
		          [0] lower left x
		          [1] w-e pixel resolution
		          [2] number of cols, assigned manually
		          [3] lower left y
		          [4] number of lines, assigned manually
		          [5] n-s pixel resolution (negative value)                      */
	
	/*Assign Topography to Data Grid Locations*/
	DEMmetadata = DEM_LOADER(In.dem_file,  /* char            DEM file name    */
	                         &dataGrid,    /* DataCell        Global Data Grid */
	                         "TOPOG");     /* DEM_LOADER Code Topography       */
	                        
	/*Check for Error flag (DEM_LOADER returns a null metadata list)*/
	if(DEMmetadata==NULL){
		fprintf(stderr, "\n[MAIN]: Error flag returned from DEM_LOADER[TOPOG].\n");
		fprintf(stderr, "Exiting.\n");
		return 1;
	}
	
	for (run = start; run < (In.runs+start); run++) {
		pulseCount = 0;
		ActiveCounter = 0;
		fprintf (stderr, "RUN #%d\n\n", run);
		fprintf (stdout, "\nRUN #%d\n", run);
		/* Assign lava volumes */
			
		if (In.min_residual > 0 && In.max_residual > 0 && In.max_residual >= In.min_residual) {	
			if (In.log_mean_residual > 0 && In.log_std_residual > 0) {	
			
				/* Select a residual value from a log-normal distribution between
	   				In.min_residual and In.max_residual. */
			
				log_min = log10(In.min_residual);
				log_max = log10(In.max_residual);
				while (Vent.residual = (double) gennor ( (float) In.log_mean_residual, (float) In.log_std_residual ),
		       		Vent.residual > log_max || Vent.residual < log_min);
				Vent.residual = pow(10, Vent.residual); 
			}
			else {
				/* Select a flow residual from a uniform random distribution between
	   			In.min_residual and In.max_residual */
	        
				Vent.residual = (double) genunf ( (float) In.min_residual, (float) In.max_residual );		
			}			
			fprintf(stderr, "Selected flow residual: %0.2f (meters)\n", Vent.residual);
		}
		
		/* Select a flow pulse voloume from a uniform random distribution between
	   		In.min_pulse_volume and In.max_pulse_volume */
	        
		if (In.min_pulse_volume > 0 && In.max_pulse_volume > 0 && In.max_pulse_volume >= In.min_pulse_volume) {	
			Vent.pulsevolume = (double) genunf ( (float) In.min_pulse_volume, (float) In.max_pulse_volume );		
			fprintf(stderr, "Selected flow pulse volume: %0.2g (cubic meters)\n", Vent.pulsevolume);
		}
			
		if (In.min_total_volume > 0 && In.max_total_volume > 0 && In.max_total_volume >= In.min_total_volume) {
			if (In.log_mean_volume > 0 && In.log_std_volume > 0) {
				
				/* Select a flow volume from a log-normal distribution between
	   			In.min_total_volume and In.max_total_volume. */
	            
				log_min = log10(In.min_total_volume);
				log_max = log10(In.max_total_volume);
				while (Vent.volumeToErupt = (double) gennor ( (float) In.log_mean_volume, (float) In.log_std_volume ),
		       		Vent.volumeToErupt > log_max || Vent.volumeToErupt < log_min);
				Vent.volumeToErupt = pow(10, Vent.volumeToErupt); 
			}
			else {
				
				/* Select a volume to erupt from a uniform random distribution between
	   			In.min_total_volume and In.max_total_volume */
	        
				Vent.volumeToErupt = (double) genunf ( (float) In.min_total_volume, (float) In.max_total_volume );
			}
			fprintf(stderr, "Selected total lava volume: %0.2g (cubic meters)\n", Vent.volumeToErupt);
			Vent.currentvolume = Vent.volumeToErupt;
		}
		
		/*Assign Residual Thickness to Data Grid Locations*/

		/*Write residual flow thickness into 2D Global Data Array*/
		for(i=0; i < DEMmetadata[4]; i++) {
			for(j=0; j < DEMmetadata[2]; j++) {
				dataGrid[i][j].residual = Vent.residual;
				dataGrid[i][j].active = (unsigned) 0;
				
			}
		}
		
		/*Assign Elevation Uncertainty to Data Grid Locations*/
		/*If elevationUncertainty is -1, user input an elevation uncertainty map*/
		if(In.elev_uncert == -1) {
			DEMmetadata = DEM_LOADER(In.uncert_map,  /*char uncertny filename*/
			                       	&dataGrid,    /* DataCell  Global Data Grid */
			                       	"T_UNC");     /* DEM_LOADER Code elev uncertainty */
			                      
			/*Check for Error flag (DEM_LOADER returns a null metadata list)*/
			if(DEMmetadata == NULL){
				fprintf(stderr, "\n[MAIN]: Error flag returned from DEM_LOADER[T_UNC].\n");
				fprintf(stderr, "Exiting.\n");
				return 1;
			}
		}
	
		/*If elevationUncertainty is not -1, it is constant globally.*/
		else {
			/*Write elevation uncertainty values into 2D Global Data Array*/
			for(i=0;i<DEMmetadata[4];i++) {
				for(j=0;j<DEMmetadata[2];j++) {
					dataGrid[i][j].elev_uncert = In.elev_uncert;
				}
			}
		}
		if (!Vent.easting || !Vent.northing) {
		/* Select a new vent from the spatial density grid */
			ret = CHOOSE_NEW_VENT(&In, &Vent);
		
			/*Check for Error, CHOOSE_NEW_VENT returns 1 if error, 0 if no errors) */
			if (ret) {
				fprintf (stderr, "\n[MAIN] Error returned from [CHOOSE_NEW_VENT].\nExiting!\n");
				return 1;
			}
		}

		/*MODULE: INIT_FLOW**********************************************************/
		/*      Creates Active Cellular Automata list and activates vent.
	          	Also creates bookkeeper variables: 
	            	size of CA list
	            	total number of CA lists
	            	total number of active automata in the CA list
	            	total volume to erupt (      */
	
		ret = INIT_FLOW(dataGrid,         /* DataCell  Global Data Grid               */
	               		&CAList,          /* Automaton Active Cells List              */
	               		&Vent,            /* VentArr   Vent Data Array                */
	               		&CAListSize,      /* unsigned  Size of each empty CA List     */
	               		&ActiveCounter,   /* unsigned  Current Number of active cells */
	               		DEMmetadata,      /* double    Geographic Metadata            */
	               		&volumeToErupt);  /* double    Volume to erupt                */
	                    
	    /*Check for Vent outside of map region*/
		if (ret == -1) {
			run--;
			continue;
		}
	              	
		/*Check for Error, (INIT_FLOW returns 1 if error, 0 if no error)*/
		if (ret) {
			fprintf(stderr, "\n[MAIN]: Error flag returned from [INIT_FLOW].\n");
			fprintf(stderr, "Exiting.\n");
			return 1;
		}
	
		/****************************************************************************
		  MAIN FLOW LOOP: PULSE LAVA AND DISTRIBUTE TO CELLS*************************
		****************************************************************************/

		volumeRemaining = volumeToErupt; /*set countdown bookkeeper volumeRemaining*/
	
		fprintf(stderr, "\n                         Running Flow #%d\n", run);
	
		/*Loop to call PULSE and DISTRIBUTE only if volume remains to be erupted*/
		while(volumeRemaining > (double)0.0) {
			
			/*MODULE: PULSE************************************************************
			          Delivers lava to vent based on information in Vent Data Array.
		          	  Returns total volume remaining to be erupted.                   

			fprintf(stderr, 
			        "\rInundated Cells: %-7d; Volume Remaining: %13.3f Pulse count : %5d",
		        	ActiveCounter,
		        	volumeRemaining,
		        	pulseCount++);
			*/
			ret = PULSE(CAList,        /* Automaton Active Cells List               */
		            &Vent,             /* VentArr   Vent Data Array                 */
		            ActiveCounter,     /* unsigned  Current # of active cells       */
		            &volumeRemaining,  /* double    Lava volume not yet erupted     */
		            DEMmetadata);      /* double    Geographic Metadata             */
		           
		
			/*Check for Error flags (PULSE returns 0 if no error, i if error)*/
			if(ret) {
				fprintf(stderr, 
				        "\nERROR [MAIN]: Error flag returned from [PULSE]. Program stopping!\n");
				if (volumeRemaining) {
					/*This return should not be possible, 
				  		Pulse should return 0 if no volume remains*/
					fprintf(stderr, 
					        "\nERROR [MAIN]: Error between [PULSE] return and lava vol. Program stopping!\n");
				}
				return 1;
			}
		
			/*Update status message on screen
			fprintf(stderr, 
			        "\rInundated Cells: %-7d; Volume Remaining: %13.3f Pulse count : %5d",
		        	ActiveCounter,
		        	volumeRemaining,
		        	pulseCount++);
			fprintf(stdout, 
			        "\rInundated Cells: %-7d; Volume Remaining: %13.3f Pulse count : %5d",
		        	ActiveCounter,
		        	volumeRemaining,
		        	pulseCount++);
		*/
		
			/*MODULE: DISTRIBUTE*******************************************************/
			/*        Distributes lava from cells to neighboring cells depending on
		 	         module specific algorithm (e.g. slope-proportional sharing).
		 	         Updates a Cellular Automata List and the active cell counter.*/
		
			ret = DISTRIBUTE(dataGrid,         /*DataCell  Global Data Grid       */
		     	            CAList,         /*Automaton Active Cells List      */
		     	            &ActiveCounter, /*unsigned  Number of active cells */
		     	            DEMmetadata        /*double    Geographic Metadata    */
		     	           );
		
			/*Check for Error flag (DISTRIBUTE returns 0 if no error, 1 if error, negative value if flow off map)*/
			if(ret) {
				fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [DISTRIBUTE]. \n");
				
				/* If flow is off map area, get new values and redo run */
				if (ret < 0) {
					run--;
					break;
				}
				fprintf (stderr, "Program stopping\n");
				return 1;
			}
		
			/* Here is a good place to output the flow at EVERY Pulse. */ 		
		
		} /* End while main flow loop: (while(volumeRemaining > 0) */
	
		/*POST FLOW WRAP UP**********************************************************
		Update status message on screen
		
		fprintf(stderr, "\rInundated Cells: %-7d; Volume Remaining: %10.3f",
	    	   	ActiveCounter,
	        	volumeRemaining);
		fprintf(stderr, "\n\n                     Single Flow Complete!\n");
*/	
		/*Print out final number of inundated cells*/
		fprintf(stderr, "Final Count: %d cells inundated.\n\n", 
	        	ActiveCounter);
		
		/*POST FLOW WRAP UP: CONSERVATION OF MASS CHECK******************************/
		/*ALL DRIVER MODULES MUST HAVE THIS. In order to do unit testing during
		  code compilation, makefile searches for the string "SUCCESS: MASS CONSERVED"
		  to conclude that the code is Verified (though not validated).*/
	
		volumeErupted = 0;
		
		for(i=0; i < ActiveCounter; i++) {
			/* Sum lava volume in each active flow cell
			Volume erupted = lava thickness + lava residual * cell length * cell width
			*/
			volumeErupted += ((CAList+i)->thickness + 
				dataGrid[(CAList+i)->row][(CAList+i)->col].residual) *
			    DEMmetadata[1] * DEMmetadata[5];
			
			/* Sum lava inundations per cell for Hit Map */
			dataGrid[(CAList+i)->row][(CAList+i)->col].hit_count++;
		}
		/* print out volume delivered to vents and total volume now in cells */
		fprintf(stderr, "Conservation of mass check\n");
		fprintf(stderr, " Total (IN) volume pulsed from vents:   %12.3f\n", volumeToErupt);
		fprintf(stderr, " Total (OUT) volume found in cells:     %12.3f\n\n", volumeErupted);
		
		/* Double data types are precise to 1e-8, so make sure that volume IN and
		   volume OUT are within this precision.
		*/	
		total = volumeErupted-volumeToErupt;
		if(abs(total) > 1e-8)
			fprintf(stderr, " ERROR: MASS NOT CONSERVED! Excess: %12.3f\n", total);
		fprintf(stderr, "----------------------------------------\n");
		
	    /* MODULE: OUTPUT*************************************************************
	       Writes out flow locations and thickness to a file.
	       File Output type (int):
	       0 = flow_map, format = X Y Z, easting northing thickness(m)
	   
	    */   
		sprintf (thisfile, "%s%d", Out.flow_file, run);		       
		ret = OUTPUT(
		flow_map,         /* file output type */
		dataGrid,         /* DataCell  Global Data Grid */
		CAList,           /* Automaton Active Cells List */
		ActiveCounter,    /* unsigned  Number of active cells */
		thisfile,             /* string    Output File */
		&Vent,            /* VentArr  vent structure */
		DEMmetadata);     /* string    DEM metadata from GDAL */             			            
			
		/* Check for Error flags (OUTPUT returns 0, if no error, 1 on error) */
		if (ret){
			fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
			fprintf(stderr, "Exiting.\n");
			return 1;
		}	
	} /* END:  for (run = start; run < (In.runs+start); run++) { */	
	
	/* MODULE: OUTPUT*************************************************************
	   Write out hit map to a file.
	   File Output type (int):
	   1 = hits_map, format = X Y Z, easting northing hits(count)
	 */
	 sprintf (thisfile, "%s%d", Out.hits_file, start);		       
	 ret = OUTPUT(
		hits_map,         /* file output type */
		dataGrid,         /* DataCell  Global Data Grid */
		CAList,           /* Automaton Active Cells List */
		ActiveCounter,    /* unsigned  Number of active cells */
		thisfile,             /* string    Output Data structure */
		&Vent,            /* VentArr  vent structure */
		DEMmetadata);     /* string    DEM metadata from GDAL */
		
	/* Check for Error flags (OUTPUT returns 0, if no error, 1 on error) */
	if (ret){
		fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");
		fprintf(stderr, "Exiting.\n");
		return 1;
	}	
		
	/* Calculate simulation time elapsed, and print it.*/
	endTime = time(NULL);
	if ((endTime - startTime) > 60)  
		fprintf(stdout, "\n\nElapsed Time of simulation approximately %0.1f minutes.\n\n",
		       (double)(endTime - startTime)/60.0);
	else fprintf(stdout, "\n\nElapsed Time of simulation approximately %u seconds.\n\n",
		       (unsigned)(endTime - startTime));
	return 0;
}
