#include "structs_LJC.h"  /* Global Structures and Variables*/
#include <gdal.h>     /* GDAL */
#include <cpl_conv.h> /* GDAL for CPLMalloc() */
#include <gc.h>
#include <ranlib.h>
#include <rnglib.h>

/*#######################
# MODULE ACTIVATE
########################*/
unsigned ACTIVATE(DataCell**,Automata*,unsigned,unsigned,unsigned,char,char);
	/*args:
		Global Grid,
		Active List,
		row,
		col,
		active count,
		parent code,
		vent code,
		residual
	*/
	
/*#######################
# MODULE DEMLOADER
########################*/
double *DEM_LOADER(char*, DataCell***,char*);
	/*args: DEM file name, 
		Null Global Data Grid pointer, 
		Model Code (one of ELEV RESID TOPOG T_UNC)
	*/

/*########################
# MODULE DISTRIBUTE
########################*/
int DISTRIBUTE(DataCell**,Automata*,unsigned*,double*);
	/*args: Global Grid, 
		Active List, 
		Active Count, 
		DEM metadata
	*/

/*########################
# MODULE INITFLOW
########################*/
int INIT_FLOW (DataCell**,Automata**,VentArr*,unsigned*,
               unsigned*,double*,double*);
	/*args:
		data array,
		*active list,
		vent array,
		*worker count,
		worker capacity,
		vent count,
		*active counts,
		DEM transform data,
		*total volume in,
		residual
	*/

/*########################
# MODULE INITIALIZE
########################*/
int INITIALIZE(Inputs *, Outputs *, VentArr *);
	/*args:
		Input file list,
		Output file list,
		Vent Array
	*/

/*########################
# MODULE NEIGHBOR
########################*/
Automata *NEIGHBOR_ID(Automata *, DataCell**,double*,Automata*,int*);
	/*args:   Active Cell,
	          Global Grid,
	          Global Grid Metadata,
	          Active List,
	          neighbor count
	  return: neighbor_list
	*/
	
/*########################
# MODULE OUTPUT
########################*/
int OUTPUT(File_output_type, DataCell**, Automata*, unsigned, char *, VentArr *, double*);
	/*args:
        Flow map type (int),
		Global Grid (2D),
		Flow List ,
		Flow List Cell Count,
		Output File Name,
                Vent data structure,
		DEM transform metadata
	*/

/*########################
# MODULE PULSE
########################*/
int PULSE(Automata*, VentArr*, unsigned, double*, double*);
	/*args: Active Cells List,
	        Current # of active cells,
	        Total Pulse Volume, 
	        Remaining Total Volume,
	        
	*/

Automata *ACTIVELIST_INIT(unsigned);
DataCell **GLOBALDATA_INIT(unsigned,unsigned);
	/*args: 
		rows, 
		colums
	*/
/*#############################
# MODULE CHOOSE_NEW_VENT
##############################*/

int CHOOSE_NEW_VENT(Inputs *, VentArr *);
/* args:
	Input structure
	VentArr structure
*/
int load_spd_data(FILE *, VentArr *, int *);
unsigned int count_rows(char file[], long len);
