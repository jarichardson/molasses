#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "prototypes.h"

/*int CHOOSE_NEW_VENT(SpatialDensity *grid,
	unsigned num_grids,
	unsigned num_vents, 
	double grid_spacing,
	unsigned *new_east,
	unsigned *new_north) {

	 Parameters
	In->spdfilespatial_density = $_[0];
	num_vents = $_[1];
 	spd_spacing = $_[2];
  my @lambda;
  my $sum;
  my $random;
  my $sum_lambda = 0;
*/
int CHOOSE_NEW_VENT(Inputs *In, DataCell ***SpDens ,VentArr *Vent) {
	double sum_lambda = 0, sum = 0, random;
	unsigned i,j;
	
	//If the Spatial Density Grid is NULL, declare it by loading in the SPD FILE
	if (*SpDens==NULL) {
		fprintf(stdout, "\nLoading Vent Spatial Density file...\n");
		In->spd_grid_data = DEM_LOADER(In->spd_file,
		                        SpDens,
		                        "DENSITY"
		                       );
		if ((In->spd_grid_data == NULL) || (*SpDens == NULL)) {
			fprintf(stderr, "\nError [CHOOSE_NEW_VENT]: ");
			fprintf(stderr, "Error flag returned from DEM_LOADER[DENSITY].\n");
			return 1;
		}
	}
	
	/*Metadata:
	  In->spd_grid_data[0] lower left x
	  In->spd_grid_data[1] w-e pixel resolution
	  In->spd_grid_data[2] number of cols
	  In->spd_grid_data[3] lower left y
	  In->spd_grid_data[4] number of lines
	  In->spd_grid_data[5] n-s pixel resolution */
	
	//Integrate the spatial density within the grid
	for(i=0; i < In->spd_grid_data[4]; i++) {
		for(j=0; j < In->spd_grid_data[2]; j++) {
			sum_lambda += (*(*SpDens+i)+j)->prob;
		}
	}
	if (sum_lambda <=0 ) {
		fprintf(stderr, "\nError [CHOOSE_NEW_VENT]: Spatial Density sums to <= 0!!\n");
		return 1;
	}
	
	
	//fprintf(stderr,"New Vent Location:");
	
	//Choose a random number (0 to integrated density) to match to a grid cell
	random = (double) genunf ( (float) 0, (float) sum_lambda );
	
	j=i=0;
	while (sum < random) {
		sum += (*(*SpDens+i)+(j++))->prob;
		if (j == (In->spd_grid_data[2])){
			i++;
			j=0;
		}
	}
	//j will be one step too far after this loop, so let's take 1 step back.
	if(j==0) {
		j=In->spd_grid_data[2];
		i--;
	}
	j--;
	
	Vent->northing = In->spd_grid_data[3] + (i * In->spd_grid_data[5]);
	Vent->easting = In->spd_grid_data[0] + (j * In->spd_grid_data[1]);
	
	//Choose a random location within the cell
	Vent->northing += ((double) genunf ( (float) 0, (float) 1)) * 
	                   In->spd_grid_data[5];
	Vent->easting  += ((double) genunf ( (float) 0, (float) 1)) * 
	                   In->spd_grid_data[1];
	
	
	//fprintf(stderr,"  %0.3f e, %0.3f n\n",Vent->easting,Vent->northing);
	
	
  	return 0;
	
}
