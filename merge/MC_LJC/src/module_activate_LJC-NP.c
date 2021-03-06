#include "include/prototypes_LJC.h"

unsigned ACTIVATE(DataCell **dataGrid, Automata *CAList, unsigned row, 
                  unsigned col, unsigned aCt, char parent, char vent) {
/*ACTIVATE_01 is a PARENT-LESS Activation Sceme! No parents are recorded.*/

/*Module: ACTIVATE
	Accepts a location marker (row,column)
	Checks active value in a global data grid to make sure cell is not already
	  active
	Assigns data from global data grid to Cellular Automata List
	
	if active value == 0 (inactive), changes:
		active count (aCt) is incremented
		dataGrid[row][column] active value <- aCt
		CAList[aCt] data values <- dataGrid[row][column]
		CAList[aCt] vent value <- vent argument (1=vent, 0=not vent)
		CAList[aCt] thickness value <- -1*residual argument
		
	return the active count. 
	Active count should never be 0, so this can be used for error handling.
*/

	/*If inactive:*/
	if(!dataGrid[row][col].active) {
		/*fprintf (stderr, " Activating new cell. ");	*/	
		(CAList+aCt)->row       = row;
		(CAList+aCt)->col       = col;
		(CAList+aCt)->elev      = dataGrid[row][col].elev;
		(CAList+aCt)->vent      = vent;
		(CAList+aCt)->lava_in   = 0.0;
		(CAList+aCt)->lava_out  = 0.0;
		(CAList+aCt)->thickness = (double) -1*dataGrid[row][col].residual;
		(CAList+aCt)->parents   = (char) 0 ;
		
		/* (CAList+aCt)->parents   = (char) parent; */

		dataGrid[row][col].active = aCt;
		aCt = aCt + (unsigned) 1;
		return aCt; /*Return active count if just activated*/
	}
	fprintf (stderr, " Active Count = %u ", dataGrid[row][col].active);
	return aCt; /*Return active count if cell was active; nothing changes*/
}

