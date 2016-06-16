#include "include/prototypes_LJC.h"

int PULSE(Automata *actList, VentArr *vent, unsigned actCt,
          double *volumeRemaining, double *gridinfo){
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
	
	double   pulseThickness = 0; /*Pulse Volume divided by data grid resolution*/
	double pulsevolume = 0;
	
	/*If there is still volume to pulse to at least one vent...*/
	pulsevolume = vent->pulsevolume;
	
	if (*volumeRemaining > (double) 0.0) {
	
		/* First active cell is vent, add pulse 	
		Decrease Pulse volume to Total Remaining lava volume at vent if 
		Remaining lava is less than the Pulse Volume
		*/
		if (pulsevolume > vent->currentvolume) pulsevolume = vent->currentvolume;
		
		/*
		Calculate thickness of lava to deliver 
		based on pulse volume and grid resolution
		*/
				
		pulseThickness = pulsevolume / (gridinfo[1] * gridinfo[5]);
		(actList+0)->thickness += pulseThickness;
		(actList+0)->elev      += pulseThickness;
		/*fprintf (stderr,  "\npulse thickness = %f /(%f * %f) = %f\n",
		         pulsevolume, 
		         gridinfo[1],
		         gridinfo[5],
		         pulseThickness);*/
				
		/* Subtract vent's pulse from vent's total magma budget */
		vent->currentvolume -= pulsevolume;
				
		/* Tally up remaining volume, from scratch 
		*volumeRemaining = (double)0.0;*/
		*volumeRemaining = vent->currentvolume;
		/*Return 0, successful PULSE*/
					
		return 0;
	}

	/*If PULSE was called with no more volume to erupt, exit without doing anything.*/
	else if (*volumeRemaining == (double) 0.0) return 0; 
	
	else { /*(*vol Remaining < 0) Volume remaining should NEVER fall below 0*/
		fprintf(stderr, "\nError [PULSE]: Remaining volume input is negative.\n");
		return 1;
	}
	
}
