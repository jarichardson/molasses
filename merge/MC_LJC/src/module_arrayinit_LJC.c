#include "include/prototypes_LJC.h"

Automata *ACTIVELIST_INIT(unsigned CAListSize){
	
/*Reserves memory for CAListCount number of lists with size [CAListSize]
  With the typedef of Automata.
*/
	Automata *m;
	
	/*Allocate active list*/
	m = (Automata*) GC_MALLOC( (size_t)(CAListSize) * sizeof(Automata) );
	if (m == NULL) {
		fprintf(stderr, "[ACTIVELIST_INIT]\n");
		fprintf(stderr, "   NO MORE MEMORY: Tried to allocate memory Active Lists!! Program stopped!\n");
		exit(1);
	}
	
	/*allocate cols & set previously allocated row pointers to point to these*/
	/*row 0 isnt enumerated, and will not be used. start at 1.
	for(i=0; i < CAListCount; i++){
		if((m[i] = (Automata*) GC_MALLOC( (size_t)(CAListSize) * sizeof(Automata) )) == NULL){
		printf("[ACTIVELIST_INIT]\n");
		printf("   NO MORE MEMORY: Tried to allocate memory for %u cells in %u Lists!! Program stopped!\n",
		       CAListSize,(CAListCount));
		exit(1);
		}
	} */

	/*return pointer to the array*/	
	
	return (m);
}

DataCell **GLOBALDATA_INIT(unsigned rows, unsigned cols){
/*Reserves memory for matrix of size [rows]x[cols]
  With the typedef of DataCell.
*/
	int i;
	DataCell **m;
	
	/*Allocate row pointers*/
	if((m = (DataCell**) GC_MALLOC((size_t)(rows) * sizeof(DataCell*) )) == NULL){
		printf("[GLOBALDATA_INIT]\n");
		printf("   NO MORE MEMORY: Tried to allocate memory for %u Rows!! Program stopped!\n", rows);
		exit(1);
	}
	
	/*allocate cols & set previously allocated row pointers to point to these*/
	
	for (i = 0; i < rows; i++) {
		if((m[i] = (DataCell*) GC_MALLOC_ATOMIC((size_t)(cols) * sizeof(DataCell) )) == NULL){
			printf("[GLOBALDATA_INIT]\n");
			printf("   NO MORE MEMORY: Tried to allocate memory for %u cols in %u rows!! Program stopped!", cols,rows);
			exit(1);
		}
	}
	
	/*return pointer to the array of row pointers*/
	return m;
}
