#include "prototypes.h"


DataCell **GLOBALDATA_INIT(unsigned rows,unsigned cols){
/*Reserves memory for matrix of size [rows]x[cols]
  With the typedef of DataCell.
*/
	int i;
	DataCell **m;
	
	/*Allocate row pointers*/
	if((m=(DataCell**)malloc((unsigned)(rows + 1)*sizeof(DataCell*)))==NULL){
		printf("[GLOBALDATA_INIT]\n");
		printf("   NO MORE MEMORY: Tried to allocate memory for %u Rows!! Exiting\n",
		       rows);
		exit(0);
	}
	
	/*allocate cols & set previously allocated row pointers to point to these*/
	
	for(i=0;i<=rows;i++){
		if((m[i]=(DataCell*)malloc((cols + 1)*sizeof(DataCell)))==NULL){
		printf("[GLOBALDATA_INIT]\n");
		printf("   NO MORE MEMORY: Tried to allocate memory for %u cols in %u rows!!",
		       cols,rows);
		printf(" Exiting\n");
			exit(0);
		}
	}
	
	/*return pointer to the array of row pointers*/
	return m;
}
