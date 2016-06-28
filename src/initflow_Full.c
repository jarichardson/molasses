#include "prototypes.h"

//int INIT_FLOW (DataCell **dataGrid, Automata **CAList, VentArr *ventList,
//               unsigned *CAListSize, unsigned ventCount,
//               unsigned param->active_count, double *gridInfo, double *totalVolume) {
int INIT_FLOW (DataCell ***dataGrid, Automata **CAList, VentArr *ventList, 
               Inputs *In, FlowStats *param, DataCell ***spdGrid) {
/* Module INIT_FLOW
	creates Cellular Automata Lists (active cell list)
	checks that all vents are within DEM area
	adds vent locations to CA List
	add all vent volumes to calculate total incoming volume
	
	args: data array, *CA lists, vent array, *CA List Count, *CA List Size,
	      vent count, *Cell counts, DEM metadata, *total volume in
	
	DEM transform data (gridInfo) format:
		[0] lower left x
		[1] w-e pixel resolution, column size
		[2] number of columns
		[3] lower left y
		[4] number of lines/rows
		[5] n-s pixel resolution, row size
*/
	
	unsigned ventRow, ventCol;
	double log_min, log_max;
	int i,j,ret=0;
	
	param->total_volume = 0;
	
	/*DATA GRID PREPARATION******************************************************/
	// 1. remove "active values" if not the first run
	// 2. assign new residual values if needed
	// 3. assign new elevations if needed
	
	//Remove previous active indices
	if(param->run > In->start) { //If positive value, this is not first run!
		for(i=0; i < In->dem_grid_data[4]; i++) {
			for(j=0; j < In->dem_grid_data[2]; j++) {
				(*(*dataGrid+i)+j)->active = 0;
			}
		}
	}
	
	// assign/reassign residual values
	if ((In->residual > 0) && (param->run == In->start)) { //if 1st run and resid set by user
		for(i=0; i < In->dem_grid_data[4]; i++) {
			for(j=0; j < In->dem_grid_data[2]; j++) {
				(*(*dataGrid+i)+j)->residual = In->residual;
			}
		}
	}
	else if (In->residual == 0) { //if residual is probabilistic
		//calculate a residual for this flow
		if ((In->log_mean_residual > 0) && (In->log_std_residual > 0)) {
			//log normal distribution
			log_min = log10(In->min_residual);
			log_max = log10(In->max_residual);
			//This produces a pseudorandom number w/ RANLIB's normal distribution gen
			while ( param->residual = (double) gennor ( (float) In->log_mean_residual,
			                                            (float) In->log_std_residual  ),
			        param->residual > log_max || param->residual < log_min
			      );
			param->residual = pow(10, param->residual);
		}
		else {
			//uniform distribution
			//This produces a pseudorandom number w/ RANLIB's uniform distribution gen
			param->residual = (double) genunf ( (float) In->min_residual, 
			                                    (float) In->max_residual );
		}
		
		//Assign to param->residual NOT In->residual.
		for(i=0; i < In->dem_grid_data[4]; i++) {
			for(j=0; j < In->dem_grid_data[2]; j++) {
				(*(*dataGrid+i)+j)->residual = param->residual;
			}
		}
	}
	else if ((In->residual == -1) &&
	        (param->run == In->start)) { //if residual is a map (only do this first run)
		In->dem_grid_data = DEM_LOADER(In->residual_map, /*char            Residual filename*/
			                     dataGrid,    /*DataCell        Global Data Grid */
			                     "RESID"       /*DEM_LOADER Code Resid Thickness  */
			                    );
		/*Check for Error flag (DEM_LOADER returns a null metadata list)*/
		if(In->dem_grid_data == NULL){
			fprintf(stderr, 
			        "\nError [INIT_FLOW]: Error flag returned from DEM_LOADER[RESID].\n");
			return 1;
		}
	}
	
	
	// assign/reassign elevations
	if ((In->elev_uncert == 0) && (param->run == In->start)) { //if 1st run and elevations are "true"
		for(i=0; i < In->dem_grid_data[4]; i++) {
			for(j=0; j < In->dem_grid_data[2]; j++) {
				(*(*dataGrid+i)+j)->elev = (*(*dataGrid+i)+j)->dem_elev;
			}
		}
	}
	else if ( In->elev_uncert > 0 ) { //if elevation has uncertainty, reassign elevs
		for(i=0; i < In->dem_grid_data[4]; i++) {
			for(j=0; j < In->dem_grid_data[2]; j++) {
				//This produces a pseudorandom number w/ RANLIB's normal distribution gen
				(*(*dataGrid+i)+j)->elev = (double) gennor ( 
				                           (float) (*(*dataGrid+i)+j)->dem_elev,
				                           (float) In->elev_uncert );
			}
		}
	}
	else if ( In->elev_uncert == -1 ) { //if elev_uncert is a file
		//load in elevation uncertainty values for each cell
		In->dem_grid_data = DEM_LOADER(In->uncert_map, /*char            uncertny filename*/
			                       dataGrid,    /*DataCell        Global Data Grid */
			                       "T_UNC"       /*DEM_LOADER Code elev uncertainty */
			                      );
		/*Check for Error flag (DEM_LOADER returns a null metadata list)*/
		if ( In->dem_grid_data == NULL ) {
			fprintf(stderr, 
			       "\nError [INIT_FLOW]: Error flag returned from DEM_LOADER[T_UNC].\n");
			return 1;
		}
		
		for(i=0; i < In->dem_grid_data[4]; i++) {
			for(j=0; j < In->dem_grid_data[2]; j++) {
			/*RANDOMIZE ELEVATIONS IF NEEDED*/
				/*if elev_uncert is essentially 0 (within Double Precision of 0), copy
					DEM elevation to the Flow Elevation.*/
				if((*(*dataGrid+i)+j)->elev_uncert < 1e-8)
					(*(*dataGrid+i)+j)->elev = (*(*dataGrid+i)+j)->dem_elev; /*copy DEM elev*/
				/*If elevation uncertainty is not 0, produce a random elevation*/
				else {
				//This produces a pseudorandom number w/ RANLIB's normal distribution gen
					(*(*dataGrid+i)+j)->elev = (double) gennor ( 
					                           (float) (*(*dataGrid+i)+j)->dem_elev,
					                           (float) (*(*dataGrid+i)+j)->elev_uncert );
				}
			}
		}
	}
	
	
	
	/*CELLULAR AUTOMATA LIST PREPARATION*****************************************/
	/*Create CA List if it has never been created!*/
	if (*CAList==NULL) {
	
		//The maximum number of possible cells is the size of the map grid
		param->ca_list_size = In->dem_grid_data[4] * In->dem_grid_data[2];
	
		//Alternatively, The maximum number of possible cells can be decided by 3 considerations
		//1: The theoretical maximum-possible-cell flow geometry would be a long 1-cell wide line of lava, at residual thickness, with all neighboring cells inundated with >~0 m of lava.
		/*for(i=0; i < param->vent_count; i++) maxCellsPossible += ventList[i].totalvolume;*/
		/*maxCellsPossible *= (3/(gridInfo[1] * gridInfo[5] * minResidual));*/
		/*maxCellsPossible += 6; //neighbors at the end of the theoretical lava line*/
		//2: Even if the total erupted volume is very minimal, the maximum flow size should be at least the number of vents in the simulation plus their grid neighbors.
		/*if(param->vent_count*9 > maxCellsPossible) maxCellsPossible = param->vent_count*9;*/
		//3: The practical maximum is the size of the map grid. There's no use to have a lava flow active list that cannot be filled with real grid spaces
		/*if(maxCellsPossible > gridInfo[4] * gridInfo[2]){*/
			//Here would be a good place to check if the flow is *WAY* bigger than the grid, and print a warning.
		/*	maxCellsPossible = gridInfo[4] * gridInfo[2];*/
		/*}*/
		/*param->ca_list_size = (unsigned) maxCellsPossible;*/
		
		
		/****************************
		    CA ARRAY DECLARATION
		****************************/
		*CAList=(Automata*) GC_MALLOC((size_t)(param->ca_list_size) *
		        sizeof(Automata));
		if (*CAList==NULL) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr,
			        "   NO MORE MEMORY: Tried to allocate memory for %u flow cells!!\n",
				      (param->ca_list_size));
			fprintf(stderr, "program stopped!\n");
			exit(1);
		}
	}
	
	
	
	/*Initialize or reinitialize the CA List*/
	for(i=0; i < param->ca_list_size; i++) {
		(*CAList+i)->row = UINT_MAX;
		(*CAList+i)->col = UINT_MAX;
		(*CAList+i)->elev = DBL_MAX;
		(*CAList+i)->thickness = DBL_MAX;
		(*CAList+i)->lava_in = DBL_MAX;
		(*CAList+i)->lava_out = DBL_MAX;
		(*CAList+i)->parents = (char) 0;
		(*CAList+i)->vent = (char) 0;
	}
	
	//Reset number of active cells in the flow to 0
	param->active_count=0;
	
	
	/*CHOOSE NEW VENT LOCATION IF NEEDED*****************************************/
	/*Find new vent location if In->vent_count = 0*/
	if (In->vent_count == 0) {
		/*MODULE: CHOOSE NEW VENT*/
		ret = CHOOSE_NEW_VENT(In, spdGrid, &ventList[0]);
		if (ret) {
			fprintf(stderr, "\n[MAIN]: Error flag returned from [CHOOSE_NEW_VENT].\n");
			fprintf(stderr, "Exiting.\n");
			return 1;
		}
		
		param->vent_count = 1;
	}
	else param->vent_count = In->vent_count;
	
	
	/*LOAD VENTS INTO CA LIST****************************************************/
	for(i=0; i < param->vent_count; i++) {
		/*check that each vent is inside global grid*/
		if((ventRow = (unsigned)
		  (((ventList+i)->northing-In->dem_grid_data[3])/In->dem_grid_data[5])) < 0) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (SOUTH of region)\n");
			fprintf(stderr, " Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       (unsigned) (((ventList+i)->easting-In->dem_grid_data[0])/In->dem_grid_data[1]));
			return 1;
		}
		else if(ventRow >= In->dem_grid_data[4]) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (NORTH of region)\n");
			fprintf(stderr, " Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       (unsigned) (((ventList+i)->easting-In->dem_grid_data[0])/In->dem_grid_data[1]));
			return 1;
		}
		else if((ventCol = (unsigned) 
		       (((ventList+i)->easting-In->dem_grid_data[0])/In->dem_grid_data[1])) < 0) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (WEST of region)\n");
			fprintf(stderr, " Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       ventCol);
			return 1;
		}
		else if(ventCol >= In->dem_grid_data[2]) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (EAST of region)\n");
			fprintf(stderr, " Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       ventCol);
			return 1;
		}
		
		
		/*Select new total volume, pulse volume if necessary*/
		if (((ventList+i)->min_pulsevolume > 0) && 
		     ((ventList+i)->max_pulsevolume >= (ventList+i)->min_pulsevolume )) {
			//If these aren't 0, then choose new pulse volume probabilistically
			
			(ventList+i)->pulsevolume = (double) genunf ( 
			                              (float) (ventList+i)->min_pulsevolume, 
			                              (float) (ventList+i)->max_pulsevolume );
		}
		
		if (((ventList+i)->min_totalvolume > 0) && 
		     ((ventList+i)->max_totalvolume >= (ventList+i)->min_totalvolume )) {
			//choose new total volume
			if (((ventList+i)->log_mean_totalvolume > 0) &&
			    ((ventList+i)->log_std_totalvolume  > 0)) {
				//log normal distribution
				log_min = log10( (ventList+i)->min_totalvolume );
				log_max = log10( (ventList+i)->max_totalvolume );
				
				while ( (ventList+i)->totalvolume = (double) gennor ( 
				                           (float) (ventList+i)->log_mean_totalvolume,
				                           (float) (ventList+i)->log_std_totalvolume  ),
				        ((ventList+i)->totalvolume > log_max ||
				         (ventList+i)->totalvolume < log_min)
				      );
				(ventList+i)->totalvolume = pow(10, (ventList+i)->totalvolume);
			}
			else {
				//uniform distribution
			(ventList+i)->totalvolume = (double) genunf ( 
			                              (float) (ventList+i)->min_totalvolume, 
			                              (float) (ventList+i)->max_totalvolume );
			}
		}
		
		/*Activate vents in CA list*/
		/*MODULE: ACTIVATE*********************************************************/
		/*        Appends a cell to the CA List using data from a Global Data Grid*/
		
		param->active_count = ACTIVATE(*dataGrid,       /*DataCell Global Data Grid     */
		                     *CAList,    /*Automata CA List              */
		                     ventRow,        /*unsigned Cell Row Location    */
		                     ventCol,        /*unsigned Cell Column Location */
		                     param->active_count, /*unsigned No. Cells in CA List */
		                     0,              /*char     Parent Code: 0=none  */
		                     1               /*char     Vent Code: 1=vent    */
		                    );
		
		/*Check for Error Flags*/
		/*Check that activect of first worker has been updated (ct should be i+1)*/
		if(param->active_count<=i){
			fprintf(stderr, 
			        "\nError [INIT_FLOW]: Error flag returned from [ACTIVATE]\n");
			return 1;
		}
		
		/*Print vent location to standard output*/
		if (i==0) { /*Print first time a vent is loaded*/
			fprintf(stderr, "\nVents Loaded into Lava Flow Active List:\n");
		}
		fprintf(stderr, " #%u [%u][%u] %15.3f cu. m.\n",
		        (i+1),ventRow,ventCol,(ventList+i)->totalvolume);
		
		/*Add current vent's volume to total volume in.*/
		param->total_volume += (ventList+i)->totalvolume;
		
		/*Make vent's remaining volume the total volume*/
		(ventList+i)->remainingvolume = (ventList+i)->totalvolume;
	}
	
	/*Print total volume.*/
	fprintf(stderr, "----------------------------------------\n");
	fprintf(stderr, "Total Volume: %16.3f cu. m.\n", param->total_volume);
	
	//copy total volume to remaining volume
	param->remaining_volume = param->total_volume;
	
	return 0;
}
