#include "prototypes.h"

int INIT_FLOW (DataCell **dataGrid, Automata **CAList, VentArr *ventList,
               unsigned *CAListSize, unsigned ventCount,
               unsigned *activeCt, double *gridInfo, double *totalVolume) {
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
	int i,j,k;
	double minResidual = 0;
	double randElev;
	double maxCellsPossible = 0;
	
	*totalVolume = 0;
	
	/*DATA GRID PREPARATION*/
	/* 1. Find minimum residual in entire grid (to safely calculate maximum number
	      of cells)
	   2. Calculate new grid elevations based on DEM elevation and uncertainty*/
	k=0;
	for(i=0; i < gridInfo[4]; i++) {
		for(j=0; j < gridInfo[2]; j++) {
			/*if first cell, assign modal thickness as min and iterate k*/
			if ((k++)==0)	minResidual = dataGrid[i][j].residual;
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
	if (minResidual<=1e-8) {
		printf("\nERROR [INIT_FLOW]:");
		printf(" Minimum Modal thickness (Residual Thickness) is <= 0!\n");
		return(-1);
	}
	
	/*CELLULAR AUTOMATA LIST PREPARATION*****************************************/
	//The maximum number of possible cells is decided by 3 considerations
	//1: The theoretical maximum-possible-cell flow geometry would be a long 1-cell wide line of lava, at residual thickness, with all neighboring cells inundated with >~0 m of lava.
	for(i=0;i<ventCount;i++) maxCellsPossible += ventList[i].totalvolume;
	maxCellsPossible *= (3/(gridInfo[1] * gridInfo[5] * minResidual));
	maxCellsPossible += 6; //neighbors at the end of the theoretical lava line
	
	//2: Even if the total erupted volume is very minimal, the maximum flow size should be at least the number of vents in the simulation plus their grid neighbors.
	if(ventCount*9 > maxCellsPossible) maxCellsPossible = ventCount*9;
	
	//3: The practical maximum is the size of the map grid. There's no use to have a lava flow active list that cannot be filled with real grid spaces
	if(maxCellsPossible > gridInfo[4] * gridInfo[2]){
		//Here would be a good place to check if the flow is *WAY* bigger than the grid, and print a warning.
		maxCellsPossible = gridInfo[4] * gridInfo[2];
	}
	
	*CAListSize = (unsigned) maxCellsPossible;
	
	/*ARRAY DECLARATION*/
	printf("Allocating Memory for Active Cell Lists... ");
	if(( *CAList=(Automata*)malloc( (unsigned)(*CAListSize) * sizeof(Automata) ) ) == NULL){
		printf("\nERROR [INIT_FLOW]:");
		printf("   NO MORE MEMORY: Tried to allocate memory for %u flow cells!! Exiting\n",
		       (*CAListSize));
		exit(1);
	}
	
	//Reset number of active cells in the flow to 0
	*activeCt=0;
	
	/*LOAD VENTS INTO CA LIST****************************************************/
	for(i=0;i<ventCount;i++) {
		/*check that each vent is inside global grid*/
		if((ventRow = (unsigned)
		  ((ventList[i].northing-gridInfo[3])/gridInfo[5])) <= 0) {
			printf("\nERROR [INIT_FLOW]:");
			printf(" Vent not within region covered by DEM! (SOUTH of region)\n");
			printf(" Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       (unsigned) ((ventList[i].easting-gridInfo[0])/gridInfo[1]));
			return (-1);
		}
		else if(ventRow >= gridInfo[4]) {
			printf("\nERROR [INIT_FLOW]:");
			printf(" Vent not within region covered by DEM! (NORTH of region)\n");
			printf(" Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       (unsigned) ((ventList[i].easting-gridInfo[0])/gridInfo[1]));
			return (-1);
		}
		else if((ventCol = (unsigned) 
		       ((ventList[i].easting-gridInfo[0])/gridInfo[1])) <= 0) {
			printf("\nERROR [INIT_FLOW]:");
			printf(" Vent not within region covered by DEM! (WEST of region)\n");
			printf(" Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       ventCol);
			return (-1);
		}
		else if(ventCol >= gridInfo[2]) {
			printf("\nERROR [INIT_FLOW]:");
			printf(" Vent not within region covered by DEM! (EAST of region)\n");
			printf(" Vent #%u at cell: [%u][%u].\n",
			       (i+1),
			       ventRow,
			       ventCol);
			return (-1);
		}
		
		/*Activate vents in CA list*/
		/*MODULE: ACTIVATE*********************************************************/
		/*        Appends a cell to the CA List using data from a Global Data Grid*/
		
		*activeCt = ACTIVATE(dataGrid,       /*DataCell Global Data Grid     */
		                     *CAList,    /*Automata CA List              */
		                     ventRow,        /*unsigned Cell Row Location    */
		                     ventCol,        /*unsigned Cell Column Location */
		                     *activeCt, /*unsigned No. Cells in CA List */
		                     0,              /*char     Parent Code: 0=none  */
		                     1               /*char     Vent Code: 1=vent    */
		                    );
		
		/*Check for Error Flags*/
		/*Check that activect of first worker has been updated (ct should be i+1)*/
		if(*activeCt<=i){
			printf("\nError [INIT_FLOW]: Error flag returned from [ACTIVATE]\n");
			return(-1);
		}
		
		/*Print vent location to standard output*/
		if (i==0) { /*Print first time a vent is loaded*/
			printf("\nVents Loaded into Lava Flow Active List:\n");
		}
		printf(" #%u [%u][%u] %15.3f cu. m.\n",
		       (i+1),ventRow,ventCol,ventList[i].totalvolume);
		
		/*Add current vent's volume to total volume in.*/
		*totalVolume += ventList[i].totalvolume;
	}
	
	/*Print total volume.*/
	printf("----------------------------------------\n");
	printf("Total Volume: %15.3f cu. m.\n", *totalVolume);
	
	return(0);
}
