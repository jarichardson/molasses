#ifndef _LAVA_STRUCTS_
#define _LAVA_STRUCTS_

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <float.h>
#include <string.h>

/*Active Cells*/
typedef struct Automata {
	unsigned row;
	unsigned col;
	double elev;
	double thickness;
	double lava_in;
	double lava_out;
	char parents;
	char vent;
} Automata;

/*Global Data Locations*/
typedef struct DataCell {
	double elev;
	unsigned active;
	double elev_uncert;
	double residual;
	double random_code;
	double dem_elev;
	unsigned hit_count;
} DataCell;

/* Spatial density grid */
typedef struct SpatialDensity {
	double easting;
	double northing;
	long double prob;
} SpatialDensity;

/*Vent Information*/
typedef struct VentArr {
	double northing;
	double easting;
	double volumeToErupt;
	double currentvolume;
	double pulsevolume;
	double residual;
	SpatialDensity *spd_grd;
} VentArr;

/*Input parameters*/
typedef struct Inputs {
	char *config_file;
	char *dem_file;
	char *slope_map;
	double residual;
	char *uncert_map;
	unsigned elev_uncert;
	char *spd_file;
	int num_grids;
	unsigned spd_grid_spacing;
	double min_residual;
	double max_residual;
	double log_mean_residual;
	double log_std_residual;
	double min_pulse_volume;
	double max_pulse_volume;
	double min_total_volume;
	double max_total_volume;
	double log_mean_volume;
	double log_std_volume;
	unsigned runs;
	unsigned flows;
} Inputs;

/*Program Outputs*/
typedef struct Outputs {
	char *flow_file;
	char *hits_file;
} Outputs;

typedef enum  {
	flow_map,
	hits_map,
	stats_file
} File_output_type;

enum new_vent {
	True,
	False
};

/*Global Variables*/
time_t startTime;
time_t endTime;

/*      ,``                 .`,   
      `,.+,               ``+`    
   ,.`#++``               ,;++`,  
 , +++'`.                  ,++++ `
` +++++++;.``  ,   ,     `#+++++ `
``+++++++++++`,   ,'++++++++++'`` 
  .,`  `++++ .   .++++++;.  ,,    
     ,+++++.   ` +++++ ,          
     ,`+++;, ``'++++`,            
    ,;+++`, . +++++,              
  . ++++;;:::++++',               
   ,'++++++++++#`,                
    ,.....````,`                  
                                  
        GO BULLS                */

#endif /*#ifndef _LAVA_STRUCTS_*/
