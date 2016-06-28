#include "prototypes.h"

/*int PULSE(Automata *actList, VentArr **Vents, unsigned actCt,*/
/*          double *volumeRemaining, unsigned ventCount, double *gridinfo){*/
int PULSE(Automata *actList, VentArr **Vents, Inputs In, FlowStats *param){
/*Module: PULSE
	If volume remains to be erupted, delivers a pre-defined pulse of magma to each
	vent cell and subtracts this pulse from the vent cell's volume. Returns the
	total remaining volume through a pointer to the volumeRemaining variable.
	
	args: Active Cell List, Vent Data Array, Active Count
	      Remaining Total Volume, Vent Count, grid metadata
	
	Algorithm:
	  If more lava remains to be erupted:
	    Search Cellular Automata List for first Vent
	    Calculate thickness to deliver to first vent
	    Add thickness to vent cell, subtract volume from remaining volume in cell
	    Repeat search for vents, until number of found vents==no. of known vents
	      Calculate total volume remaining, exit
	  If no lava remains to be erupted:
	    Exit without doing anything
	  If negative lava remains:
	    Exit with an Error
	  If not all vents were found:
	    Exit with an Error
	
	gridinfo elements used in this module:
		[1] w-e pixel resolution
		[5] n-s pixel resolution (negative value)                                 */
	
	unsigned i, currentVent;
	double   pulseThickness; /*Pulse Volume divided by data grid resolution*/
	
	/*If there is still volume to pulse to at least one vent...*/
	if (param->remaining_volume > (double) 0.0) {
		currentVent = 0;
		
		/*Search for Vents among the active cells*/
		for( i=0; i <= param->active_count; i++) {
			/*if this cell is a vent cell (vent!=0), add pulse*/
			if ((actList+i)->vent) {
				/*Decrease Pulse volume to Total Remaining lava volume at vent if 
				    Remaining lava is less than the Pulse Volume*/
				if ((*Vents+currentVent)->pulsevolume >
				    (*Vents+currentVent)->remainingvolume)
					(*Vents+currentVent)->pulsevolume = (*Vents+currentVent)->remainingvolume;
				
				/*Calculate thickness of lava to deliver based on pulse volume and
				    grid resolution*/
				pulseThickness = (*Vents+currentVent)->pulsevolume / 
				                  (In.dem_grid_data[1] * In.dem_grid_data[5]);
				(actList+i)->thickness += pulseThickness;
				(actList+i)->elev      += pulseThickness;
				
				/*Subtract vent's pulse from vent's total magma budget*/
				(*Vents+currentVent)->remainingvolume -= (*Vents+currentVent)->pulsevolume;
				
				/*Move vent counter to next vent*/
				currentVent++;
				
				/*If all vents have been found and pulses delivered:*/
				if(currentVent >= param->vent_count) {
					/*Tally up remaining volume, from scratch*/
					param->remaining_volume = 0;
					for(currentVent=0; currentVent < param->vent_count; currentVent++)
						param->remaining_volume += (*Vents+currentVent)->remainingvolume;
					
					/*Return 0, successful PULSE*/
					return 0;
				}
			}
		}
	}
	/*If PULSE was called with no more volume to erupt, exit without doing anything.*/
	else if (param->remaining_volume == (double) 0.0) return 1; 
	
	else { /*(*vol Remaining < 0) Volume remaining should NEVER come in below 0*/
		fprintf(stderr, "\nError [PULSE]: Remaining volume input is negative.\n");
		return 2;
	}
	
	/*Return 1: if this line is reached, all cells were checked to be vents but
	  all the vents found did not reach the ventCount. This is an Error!*/
	fprintf(stderr, "\nError [PULSE]: All magma not delivered to vents/vents missing!\n");
	return 3;
}
