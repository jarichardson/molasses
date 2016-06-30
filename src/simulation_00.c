#include "prototypes.h"

int SIMULATION(DataCell **dataGrid, Automata *CAList, VentArr **Vents, 
               Inputs In, FlowStats *FlowParam) {
	/****************************************************************************/
	/*MAIN FLOW LOOP: PULSE LAVA AND DISTRIBUTE TO CELLS*************************/
	/****************************************************************************/

	int i, ret;
	unsigned PulseCount = 0;
	unsigned PrintCount = 1; //minimum # of pulses to print
	
	//only print up to 1000 statuses after the pulse loop.
	for (i=0; i < FlowParam->vent_count; i++) {
		if (PrintCount < (unsigned) (*Vents+i)->totalvolume/(*Vents+i)->pulsevolume/1000)
			PrintCount = (unsigned) (*Vents+i)->totalvolume/(*Vents+i)->pulsevolume/1000;
	}
	
	fprintf(stdout, "\n                         Running Flow\n");
	
	/*Loop to call PULSE and DISTRIBUTE only if volume remains to be erupted*/
	while(FlowParam->remaining_volume > 0) {
		/*MODULE: PULSE************************************************************/
		/*        Delivers lava to vents based on information in Vent Data Array.
			        Returns total volume remaining to be erupted.                   */
	
		ret = PULSE(CAList,        /*Automaton Active Cells List               */
			          Vents,           /*VentArr   Vent Data Array                 */
			          In,
			          FlowParam
			         );
		/*Check for Error flags (PULSE returns <0 or 0 value)*/
		if(ret > 1) { // General Error
			fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [PULSE].\n");
			fprintf(stderr, "Exiting.\n");
			return 1;
		}
		else if (ret) {
			if (FlowParam->remaining_volume) {
				/*This return should not be possible, 
					Pulse should return 0 if no volume remains*/
				fprintf(stderr, "\nERROR [MAIN]: Error between [PULSE] return and lava vol.\n");
				fprintf(stderr, "Exiting.\n");
				return 1;
			}
			/*If ret=1, PULSE was called even though there was no lava to distribute.
				Do not call Pulse or Distribute anymore! End flow (but not program).  */
			return 0;
		}
	
		/*Update status message on screen*/
		if ((++PulseCount)%PrintCount == 0)
			fprintf(stdout,"\rInundated Cells: %-7d; Volume Remaining: %10.2f",
			        FlowParam->active_count, FlowParam->remaining_volume);
	
	
		/*MODULE: DISTRIBUTE*******************************************************/
		/*        Distributes lava from cells to neighboring cells depending on
			        module specific algorithm (e.g. slope-proportional sharing).
			        Updates a Cellular Automata List and the active cell counter.*/
	
		ret = DISTRIBUTE(dataGrid,          /*DataCell  Global Data Grid       */
			               CAList,         /*Automaton Active Cells List      */
			               &FlowParam->active_count, /*unsigned  Number of active cells */
			               In.dem_grid_data        /*double    Geographic Metadata    */
			              );
	
		/*Check for Error flag (DISTRIBUTE returns <0 value)*/
		if(ret == 2) { //OFF THE MAP ERROR, continue to next run
			return 2;
		}
		else if (ret) { // GENERAL ERROR, Stop program
			fprintf(stderr, "\nERROR [MAIN]: Error flag returned from [DISTRIBUTE].\n");
			fprintf(stderr, "Exiting.\n");
			return 1;
		}
	
	} /*End while remainingvolume loop (PULSE/DISTRIBUTE Loop)*/
	
	/*SUCCESSFUL SIMULATION*/
	return 0;
}
