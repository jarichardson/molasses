#include "structs.h"  /* Global Structures and Variables*/
#include "gdal.h"     /* GDAL */
#include "cpl_conv.h" /* GDAL for CPLMalloc() */
#include <gc.h>
#include <ranlib.h>
#include <rnglib.h>


int PULSE(Automata*,VentArr**,Inputs,FlowStats*);
	/*args: Active CA List,
	        Active Count,
	        Total Pulse Volume, 
	        Remaining Total Volume,
	        Vent Cell Count
	*/

int OUTPUT(DataCell**, Automata*, unsigned, char*, char, double*, const char*);
	/*args:
		Global Grid,
		Flow List,
		Flow List Cell Count,
		Output File Name,
		Output File Type,
		DEM transform metadata,
		DEM projection metadata
	*/

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

int INIT_FLOW (DataCell***,Automata**,VentArr*,Inputs,FlowStats*,DataCell***);
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

Automata *NEIGHBOR_ID(Automata,DataCell**,double*,Automata*,int*);
	/*args:   Active Cell,
	          Global Grid,
	          Global Grid Metadata,
	          Active List,
	          neighbor count
	  return: neighbor_list
	*/

int DISTRIBUTE(DataCell**,Automata*,unsigned*,double*);
	/*args: Global Grid, Active List, Active Count, DEM metadata*/

int INITIALIZE(Inputs *, Outputs *, VentArr **);
	/*args:
		Input file list,
		Output file list,
		Vent Array
	*/


double *DEM_LOADER(char*, DataCell***,char*);
	/*args: DEM file name, Null Global Data Grid pointer, Model Code*/
	/*Model Codes:
	  ELEV
	  RESID
	  TOPOG
	  T_UNC
	*/
	
	
int CHOOSE_NEW_VENT(Inputs*, DataCell***, VentArr*);
