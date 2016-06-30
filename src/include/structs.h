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
#include <ctype.h>

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
	unsigned hit_count;
	double elev_uncert;
	double residual;
	double random_code;
	double dem_elev;
	long double prob;
} DataCell;

/*Input parameters*/
typedef struct Inputs {
	char *config_file;
	char *dem_file;
	char *residual_map;
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
	unsigned vent_count;
	unsigned runs;
	int start;
	unsigned flows;
	double *dem_grid_data;
	char *dem_proj;
	double *spd_grid_data;
} Inputs;

typedef struct FlowStats {
	unsigned ca_list_size;
	unsigned active_count;
	unsigned vent_count;
	unsigned run;
	double   residual;
	double   remaining_volume;
	double   total_volume;
} FlowStats;

/*Program Outputs*/
typedef struct Outputs {
	char *ascii_flow_file;
	char *ascii_hits_file;
	char *raster_hits_file;
	char *raster_flow_file;
	char *raster_post_topo;
	char *raster_pre_topo;
	char *stats_file;
} Outputs;

typedef enum  {
	ascii_flow,
	ascii_hits,
	raster_hits,
	raster_flow,
	raster_post,
	raster_pre,
	stats_file
} File_output_type;

/*Vent Information*/
typedef struct VentArr {
	double northing;
	double easting;
	double totalvolume;
	double min_totalvolume;
	double max_totalvolume;
	double remainingvolume;
	double log_mean_totalvolume;
	double log_std_totalvolume;
	double pulsevolume;
	double min_pulsevolume;
	double max_pulsevolume;
} VentArr;


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
