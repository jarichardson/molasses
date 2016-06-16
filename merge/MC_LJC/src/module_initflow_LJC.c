#include "include/prototypes_LJC.h"

int INIT_FLOW (DataCell **dataGrid, Automata **CAList, VentArr *ventList,
               unsigned *CAListSize,
               unsigned *activects, double *gridInfo, double *totalVolume) {
	               
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
	/*Automata *IFCAList;  working array for CAList in Init_Flow module */
/*	unsigned *IFactivects;  working array for active counts*/
	int i = 0,j,k;
	double minResidual = 0;
	double randElev;
	double maxCellsPossible = 0;
	
	*totalVolume = 0;
	if (*CAList == NULL) {
	/*DATA GRID PREPARATION*/
	/* 1. Find minimum residual in entire grid (to safely calculate maximum number
	      of cells)
	   2. Calculate DEM elevations based on elevation and uncertainty*/
	k=0;
	for(i=0; i < gridInfo[4]; i++) {
		for(j=0; j < gridInfo[2]; j++) {
			
			/*if first cell, assign modal thickness as min and iterate k*/
			if ((k++) == 0)	minResidual = dataGrid[i][j].residual;
			/*if cell modal thickness is less than min, rewrite min.*/
			else if (minResidual > dataGrid[i][j].residual)
				minResidual = dataGrid[i][j].residual;
			
			/*RANDOMIZE ELEVATIONS IF NEEDED*/
			/*if elev_uncert is essentially 0 (within Double Precision of 0), copy
			  DEM elevation to the Flow Elevation.*/
			if(dataGrid[i][j].elev_uncert < 1e-8)
				dataGrid[i][j].elev = dataGrid[i][j].dem_elev; /*copy DEM elev to flow elev*/
			/*If elevation uncertainty is not 0, produce a random elevation*/
			else {
				/*This produces a pseudorandom number w/ box-muller method
				  for a pdf with mean 0, STD 1.0 (M_PI comes from the math library)*/
				randElev = sqrt(-2 * log((rand()+1.0)/(RAND_MAX+1.0)))*
				              cos(2*M_PI*((rand()+1.0)/(RAND_MAX+1.0)));
				/*scale random number by uncertainty (scale the STD)*/
				randElev *= dataGrid[i][j].elev_uncert;
				/*apply to the DEM value (translate the mean)*/
				randElev += dataGrid[i][j].dem_elev;
				dataGrid[i][j].elev = randElev;
			}
		}
	}
	/*Error check that all flow thicknesses are positive!*/
	if (minResidual <= 1e-8) {
		fprintf(stderr, "\nERROR [INIT_FLOW]:");
		fprintf(stderr, " Minimum Modal thickness (Residual Thickness) is <= 0!\n");
		return 1;
	}
	
	/*CELLULAR AUTOMATA LIST PREPARATION*****************************************/
	/*While no parellelization is in effect, scale CA List Size to the model*/
	/*The MOST cells a flow can inundate is 3*(total volume/residual volume)+2
	maxCellsPossible = 0.0;
	for(i=0; i < ventCount; i++) maxCellsPossible += ventList->totalvolume; 
	maxCellsPossible *= (3/(gridInfo[1] * gridInfo[5] * minResidual));
	maxCellsPossible += 2; 
	
	if (maxCellsPossible > gridInfo[4] * gridInfo[2]) */
		
	maxCellsPossible = gridInfo[4] * gridInfo[2];
		
	*CAListSize = (unsigned) maxCellsPossible;
	
	/*ARRAY DECLARATION*/
	/*Declare 1 Cellular Automata list of size CAListSize 
	*CAListCount = 1; */
	fprintf(stderr, "Allocating Memory for Active Cell List, size = %u ", *CAListSize);
	
	*CAList = ACTIVELIST_INIT(*CAListSize);
	
	/*IFCAList = *CAList; */
	
	
	/*Declare an array of Active Counters for each CA List, initialize at 0
	printf("and counters...");
	if((*activects =
       (unsigned *) GC_MALLOC( (size_t)(*CAListCount) * sizeof (unsigned) )) == NULL){
		printf("\nERROR [INIT_FLOW]:");
		printf(" No more memory! Tried to create Active Count List\n");
		return 1;
	}
	printf(" Done.\n");
	 IFactivects = *activects; */
	}
	
	/*Declare Active List Counts to 0 (No Active Cells yet)
	for(i=0; i < *CAListCount; i++) *activects = 0; */
	*activects = 0;
	
	/*LOAD VENTS INTO CA LIST****************************************************/
	/* for(i=0; i < ventCount; i++) {
		check that each vent is inside global grid*/
		
		if((ventRow = (unsigned) (( ventList->northing - gridInfo[3]) / gridInfo[5]) ) <= 0) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (SOUTH of region)\n");
			fprintf(stderr, " Vent #%d at cell: [%u][%u].\n",
			       (i),
			       ventRow,
			       (unsigned) (( ventList->easting - gridInfo[0]) / gridInfo[1]) );
			return -1;
		}
		else if(ventRow >= gridInfo[4]) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (NORTH of region)\n");
			fprintf(stderr, " Vent #%u at cell: [%u][%u].\n",
			       (i),
			       ventRow,
			       (unsigned) (( ventList->easting - gridInfo[0]) / gridInfo[1]) );
			return -1;
		}
		else if((ventCol = (unsigned) 
		       ( (ventList->easting - gridInfo[0]) / gridInfo[1]) ) <= 0) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (WEST of region)\n");
			fprintf(stderr, " Vent #%d at cell: [%u][%u].\n",
			       (i),
			       ventRow,
			       ventCol);
			return -1;
		}
		else if(ventCol >= gridInfo[2]) {
			fprintf(stderr, "\nERROR [INIT_FLOW]:");
			fprintf(stderr, " Vent not within region covered by DEM! (EAST of region)\n");
			fprintf(stderr, " Vent #%d at cell: [%u][%u].\n",
			       (i),
			       ventRow,
			       ventCol);
			return -1;
		}
		
		/*Activate vents in CA list*/
		/*MODULE: ACTIVATE*********************************************************/
		/*        Appends a cell to the CA List using data from a Global Data Grid*/
		
		*activects = ACTIVATE(dataGrid,         /* (DataCell)  Global Data Grid     */
		                      *CAList,          /* (Automata)  CA List              */
		                      ventRow,          /* (unsigned)  Cell Row Location    */
		                      ventCol,          /* (unsigned)  Cell Column Location */
		                      0,       			/* (unsigned)  No. Cells in CA List */
		                      0,                /* (char)      Parent Code: 0=none  */
		                      1);               /* (char)      Vent Code: 1=vent    */
		                         
		
		/*Check for Error Flags*/
		/*Check that activect of first worker has been updated */
		if(!(*activects)){
			fprintf(stderr, "\nError [INIT_FLOW]:[ACTIVATE] Active count is 0! \n");
			return 1;
		}
		
		/*Print vent location to standard output*/
		/*if (i==0) { Print first time a vent is loaded*/
		fprintf(stderr, "\n\nVents Loaded into Lava Flow Active List:\n");
		fprintf(stderr, " #%u [%u][%u] %15.3f cu. m.\n",
		       (i),ventRow,ventCol, ventList->currentvolume);
		
		/*Add current vent's volume to total volume in.*/
		*totalVolume += ventList->currentvolume;
/*	} */
	
	/*Print total volume.*/
	fprintf(stderr, "----------------------------------------\n");
	fprintf(stderr, "Total Volume: %15.3f cu. m.\n", *totalVolume);
	fflush(stderr);
	return 0;
}
