#include "prototypes.h"

int main(int argc, char *argv[]) {
/*
This driver is used to test the speed of many iterations of different modules
*/
	
	/*VARIABLES******************************************************************/
	
	int size;            /* variable used for creating seed phrase               */
	char *phrase;        /* seed phrase for random number generator              */
	int seed1;           /* random seed number */
 	int seed2;           /* random seed number */
	int ret=0;            /*loop variables, function return value */
	//int      pulseCount  = 0;           /*Current number of Main PULSE loops    */
	
	/*Physical Parameters*/
	//double   volumeErupted = 0;         /*Total Volume in All Active Cells      */
	
	//Benchmarking Parameters
	Inputs In;           /* Structure to hold model inputs named in Config file  */
	Automata *ActiveList;
	FlowStats ParameterList;
	VentArr *OneVent;
	int PulseCount = 0;
	//VentArr TwoVent[2];
	
	
	//Set up dummy flow
	ParameterList.active_count = 100;
	ParameterList.vent_count = 1;
	
	In.dem_grid_data = malloc (sizeof (double) * 6);
	if (In.dem_grid_data == NULL) {
		fprintf(stderr, "ERROR [DEM_LOADER]: Out of Memory creating Metadata Array!\n");
		return 1;
	}
	In.dem_grid_data[1] = In.dem_grid_data[5] = (double) 1.0;
	
	if((OneVent = (VentArr*) malloc (sizeof (VentArr) * (2))) ==NULL) {
		fprintf(stderr, "\nERROR [INITIALIZE] Out of Memory creating vent array!\n");
		return 1;
	}
	OneVent[0].pulsevolume = (double) 1.0;
	OneVent[0].totalvolume = OneVent[0].remainingvolume = (double) 100000000.0;
/*	OneVent[1].pulsevolume = (double) 1.0;*/
/*	OneVent[1].totalvolume = OneVent[1].remainingvolume = (double) 100000000.0;*/
	ParameterList.remaining_volume = OneVent[0].remainingvolume;
	
	//20 seconds, 1G   Pulses
	//10 seconds, 500M Pulses
	//2 seconds,  100M Pulses
	
	if((ActiveList = (Automata*) malloc (sizeof (Automata) * (ParameterList.active_count))) ==NULL) {
		fprintf(stderr, "\nERROR [INITIALIZE] Out of Memory creating vent array!\n");
		return 1;
	}
	ActiveList[1].vent = 1;
	ActiveList[1].thickness = (double) 0;
	ActiveList[1].elev = (double) 0;
/*	ActiveList[2].vent = 1;*/
/*	ActiveList[2].thickness = (double) 0;*/
/*	ActiveList[2].elev = (double) 0;*/
	
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
	
	fprintf(stdout, "Beginning time benchmarking...\n");
	
	/*MODULE: INITIALIZE*********************************************************/
/*	ret = INITIALIZE(&In, &Out, &Vents);*/
/*	if(ret){*/
/*		fprintf(stderr, "\n[MAIN]: Error flag returned from [INITIALIZE].\n");*/
/*		fprintf(stderr, "Exiting.\n");*/
/*		return 1;*/
/*	}*/
	
	
	/*MODULE: DEM_LOADER*********************************************************/
/*	In.dem_grid_data = DEM_LOADER(In.dem_file, &dataGrid, "TOPOG");*/
/*	if(In.dem_grid_data==NULL){*/
/*		fprintf(stderr, "\nError [MAIN]: Error flag returned from DEM_LOADER[TOPOG].\n");*/
/*		fprintf(stderr, "Exiting.\n");*/
/*		return(-1);*/
/*	}*/
	
	//Initialize Flow Parameter Set
/*	FlowParam.ca_list_size     = 0;*/
/*	FlowParam.active_count     = 0;*/
/*	FlowParam.vent_count       = 0;*/
/*	FlowParam.run              = 0; //this is actually important*/
/*	FlowParam.residual         = 0;*/
/*	FlowParam.remaining_volume = 0;*/
/*	FlowParam.total_volume     = 0;*/
	
	//Initialize the Outputs performed for each run (stats suggested)
/*	Out_eachrun.ascii_flow_file  = NULL;*/
/*	Out_eachrun.ascii_hits_file  = NULL;*/
/*	Out_eachrun.raster_hits_file = NULL;*/
/*	Out_eachrun.raster_flow_file = NULL;*/
/*	Out_eachrun.raster_post_topo = NULL;*/
/*	Out_eachrun.raster_pre_topo  = NULL;*/
/*	Out_eachrun.stats_file       = Out.stats_file;*/
	
	/*MODULE: INIT_FLOW**********************************************************/
/*	ret = INIT_FLOW(&dataGrid, &CAList, Vents, &In, &FlowParam, &SpatialDensity);*/
/*	if(ret) {*/
/*		fprintf(stderr, "\nError [MAIN]: Error flag returned from [INIT_FLOW].\n");*/
/*		fprintf(stderr, "Exiting.\n");*/
/*		return 1;*/
/*	}*/

	/****************************************************************************/
	/*MAIN FLOW LOOP: PULSE LAVA AND DISTRIBUTE TO CELLS*************************/
	/****************************************************************************/
/*	ret = SIMULATION(dataGrid, CAList, &Vents, In, &FlowParam);*/
/*	fprintf(stdout,"\n");*/
/*	else if (ret) {*/
/*		fprintf (stderr, "Error [MAIN]: Error flag returned from [SIMULATION]\n");*/
/*		return 1;*/
/*	}*/
	
	fprintf(stdout, "Total volume = %0.3f\n", ParameterList.remaining_volume);
	/*Loop to call PULSE and DISTRIBUTE only if volume remains to be erupted*/
	while(ParameterList.remaining_volume > 0) {
		/*MODULE: PULSE************************************************************/
		/*        Delivers lava to vents based on information in Vent Data Array.
			        Returns total volume remaining to be erupted.                   */
	
		ret = PULSE(ActiveList, &OneVent, In, &ParameterList);
		/*Check for Error flags (PULSE returns <0 or 0 value)*/
		if (ret) {
			fprintf(stderr, "\nERROR [MAIN]: Error between [PULSE] return and lava vol.\n");
			return 1;
		}
		
		if ((++PulseCount)%10000==0) {
			fprintf(stdout,"\rInundated Cells: %-7d; Volume Remaining: %10.2f",
					   ParameterList.active_count, ParameterList.remaining_volume);
		}
	}
	
	
	/*MODULE: OUTPUT*************************************************************/
/*	ret = OUTPUT(Out_eachrun, In, dataGrid, CAList, FlowParam, Vents);*/
/*	if(ret){ */
/*		fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");*/
/*		fprintf(stderr, "Exiting.\n");*/
/*		return 1;*/
/*	}*/
	
	/*MODULE: OUTPUT*************************************************************/
/*	Out.stats_file = NULL; //Stats were printed for each run, so don't reprint*/
/*	*/
/*	ret = OUTPUT(Out, In, dataGrid, CAList, FlowParam, Vents);*/
/*	if(ret){*/
/*		fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [OUTPUT].\n");*/
/*		return 1;*/
/*	}*/
	
	/*Calculate simulation time elapsed, and print it.*/
	endTime = time(NULL);
	fprintf(stdout, "\nElapsed Time of simulation approximately %u seconds.\n\n",
	       (unsigned)(endTime - startTime));
	       
	
	
	fprintf(stdout, "Vent 1 Thickness: %0.3f\n", ActiveList[1].thickness);
	fprintf(stdout, "Vent 2 Thickness: %0.3f\n", ActiveList[2].thickness);
	return 0;
}
