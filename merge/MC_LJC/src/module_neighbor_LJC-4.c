#include "include/prototypes_LJC.h"

Automata *NEIGHBOR_ID(Automata *cCell, DataCell **grid, double *gridMetadata,
                  Automata *CAList, int *neighborCount) {
/*Module: NEIGHBOR_ID (4 Directions)
	identifies 4-neighbors, List of Cells (4 long) updated, # of eligible-for-lava
	neighbors returned
	
	*Accepts a Cell structure (active cell location)
	*Checks length of neighbor list for validity (must be at least 4 Cells long)
	
	*If location is at global grid boundary:
		return "off the grid" error, -1 to -4
	Else:
	*finds neighbors in cardinal directions:
		that aren't parent cells (using bitswitching: 0th bit = right;1=d;2=l;3=u
		with lower elevations
	*Flags eligible neighbors in neighbor list
	*Updates neighbor count
	
	*Neighbor list is returned as an Automaton List with rows, cols, elevs of 
	 eligible neighbors
	*Passes back Neighbor Count as pointer
*/
	
	char parents;                      /*Parent bitcode*/
	int uRow,dRow,lCol,rCol,cRow,cCol; /*Row, Col addresses (center,up,down,L,R)*/
	Automata *neighborList;
	unsigned active;
	
	/*Create the neighbor list array*/
	if((neighborList = (Automata*)GC_MALLOC(4 * sizeof(Automata)))==NULL) {
		printf("\nERROR [NEIGHBOR_ID]: No more memory while creating Neighbor List!!");
		printf("  Exiting.\n");
		exit(0);
	}
	
	*neighborCount = 0; /*Reset neighbor counter*/
	
	/*define column and row numbers for current cell and neighbors*/
	cRow = cCell->row;
	cCol = cCell->col;
	uRow = cRow + 1;
	dRow = cRow - 1;
	rCol = cCol + 1;
	lCol = cCol - 1;

	/*Off grid handling*/
	/*gridMetadata format:
		[0] lower left x
		[1] w-e pixel resolution
		[2] number of cols, assigned manually      **********
		[3] lower left y
		[4] number of lines, assigned manually     **********
		[5] n-s pixel resolution (negative value)
	*/
	if (dRow < 0) {
		printf("\nFLOW IS OFF THE MAP! (South) [NEIGHBOR_ID]\n");
		*neighborCount = -1;
		return neighborList;
	} else if (uRow >= gridMetadata[4]) {
		printf("\nFLOW IS OFF THE MAP! (North) [NEIGHBOR_ID]\n");
		*neighborCount = -2;
		return neighborList;
	} else if (lCol < 0) {
		printf("\nFLOW IS OFF THE MAP! (West) [NEIGHBOR_ID]\n");
		*neighborCount = -3;
		return neighborList;
	} else if (rCol >= gridMetadata[2]) {
		printf("\nFLOW IS OFF THE MAP! (East) [NEIGHBOR_ID]\n");
		*neighborCount = -4;
		return neighborList;
	}

	/*Parent finding requires bit switching*/
	parents = cCell->parents;
	
	/****
	**FOR EACH DIRECTION, TEST FOR PARENTAGE, ACTIVE STATUS, ELEVATION
	****/
	
	
	/*NORTH*/
	active = grid[uRow][cCol].active;
	if(!(parents & (1 << 3))) { 
	/*if 3rd parent bit False, UPWARD cell is not parent: Continue*/
		if(!active) {
		/*if UPWARD cell is inactive in Global Grid, use grid elevation value*/
			if(cCell->elev > grid[uRow][cCol].elev) {
			/*if active cell's elevation is higher than UPWARD cell, flag up cell*/
				neighborList[*neighborCount].row  = uRow;
				neighborList[*neighborCount].col  = cCol;
				neighborList[*neighborCount].elev = grid[uRow][cCol].elev;
				*neighborCount+=1;
			}
		} else {
		/*if UPWARD cell is active in Global Grid, use active list elev value*/
			if(cCell->elev > (CAList+active)->elev) {
			/*if active cell's elevation is higher than UPWARD cell, flag up cell*/
				neighborList[*neighborCount].row  = uRow;
				neighborList[*neighborCount].col  = cCol;
				neighborList[*neighborCount].elev = (CAList+active)->elev;
				*neighborCount+=1;
			}
		}
	}
	
	/*SOUTH*/
	active = grid[dRow][cCol].active;
	if(!(parents & (1 << 1))) {
	/*if 1st parent bit False, DOWNWARD cell is not parent: Continue*/
		if(!active) {
		/*if DOWNWARD cell is inactive in Global Grid, use grid elevation value*/
			if(cCell->elev > grid[dRow][cCol].elev) {
			/*if active cell's elevation is higher than DOWNWARD cell, flag down cell*/
				neighborList[*neighborCount].row  = dRow;
				neighborList[*neighborCount].col  = cCol;
				neighborList[*neighborCount].elev = grid[dRow][cCol].elev;
				*neighborCount+=1;
			}
		} else {
		/*if DOWNWARD cell is active in Global Grid, use active list elev value*/
			if(cCell->elev > (CAList+active)->elev) {
			/*if active cell's elevation is higher than DOWNWARD cell, flag down cell*/
				neighborList[*neighborCount].row  = dRow;
				neighborList[*neighborCount].col  = cCol;
				neighborList[*neighborCount].elev = (CAList+active)->elev;
				*neighborCount+=1;
			}
		}
	}
	
		/*WEST*/
	active = grid[cRow][lCol].active;
	if(!(parents & (1 << 2))) { 
	/*if 2nd parent bit False, LEFTWARD cell is not parent: Continue*/
		if(!active) {
		/*if LEFTWARD cell is inactive in Global Grid, use grid elevation value*/
			if(cCell->elev > grid[cRow][lCol].elev) {
			/*if active cell's elevation is higher than LEFTWARD cell, flag left cell*/
				neighborList[*neighborCount].row  = cRow;
				neighborList[*neighborCount].col  = lCol;
				neighborList[*neighborCount].elev = grid[cRow][lCol].elev;
				*neighborCount+=1;
			}
		} else {
		/*if LEFTWARD cell is active in Global Grid, use active list elev value*/
			if(cCell->elev > (CAList+active)->elev) {
			/*if active cell's elevation is higher than LEFTWARD cell, flag left cell*/
				neighborList[*neighborCount].row  = cRow;
				neighborList[*neighborCount].col  = lCol;
				neighborList[*neighborCount].elev = (CAList+active)->elev;
				*neighborCount+=1;
			}
		}
	}
	
	/*EAST*/
	active = grid[cRow][rCol].active;
	if(!(parents & (1 << 0))) { 
	/*if 0th parent bit False, RIGHTWARD cell is not parent: Continue*/
		if(!active) {
		/*if RIGHTWARD cell is inactive in Global Grid, use grid elevation value*/
			if(cCell->elev > grid[cRow][rCol].elev) {
			/*if active cell's elevation is higher than RIGHTWARD cell, flag right cell*/
				neighborList[*neighborCount].row  = cRow;
				neighborList[*neighborCount].col  = rCol;
				neighborList[*neighborCount].elev = grid[cRow][rCol].elev;
				*neighborCount+=1;
			}
		} else {
		/*if RIGHTWARD cell is active in Global Grid, use active list elev value*/
			if(cCell->elev > (CAList+active)->elev) {
			/*if active cell's elevation is higher than RIGHTWARD cell, flag right cell*/
				neighborList[*neighborCount].row  = cRow;
				neighborList[*neighborCount].col  = rCol;
				neighborList[*neighborCount].elev = (CAList+active)->elev;
				*neighborCount+=1;
			}
		}
	}
	
	
	/*neighbor counter is updated as a pointer and is passed back to [DISTRIBUTE]*/
	/*return list of neighbors*/
	return neighborList;
}
