#include <prototypes_LJC.h>

int OUTPUT(
            File_output_type type,
			DataCell **dataGrid, 
			Automata *flowList, 
			unsigned flowcount,
			char *file, 
            VentArr *vent,
			double *geotransform) {
			
/*  Module OUTPUT
	Print out flow data to a file.
	
	File types to output:
	0 = flow_map, format = X Y Z, easting northing thickness(m)
	1 = hits_map, format = X Y Z, easting northing hits(count)
	not implemented:
	2: Thickness Raster
	3: Elevation Raster
	4: Elevation + Lava Raster (2+3)
	5: Update internal DEMgrid to build volcano from lavaflows
	
*/
/*    GeoTransform Double List:	
	  Transform Index    This Code                        GDAL
	  geotransform[0]    lower left x                     top left x (SAME)
	  geotransform[1]    w-e pixel res                    SAME
	  geotransform[2]    number of cols                   0
	  geotransform[3]    lower left y                     top left y
	  geotransform[4]    number of lines                  0
	  geotransform[5]    n-s pixel res (positive value)   (negative value)
	
*/
	
	FILE     *out;
	unsigned j, row, col;
	double easting, northing, value, elev;
	
/*  GDALDatasetH hDstDS;*/
/*  double GDALGeoTransform[6];*/
/*  const char *pszFormat = "GTiff";*/
/*  GDALDriverH hDriver;*/
/*  GDALRasterBandH hBand;*/
/*  GByte *RasterDataB;*/
/*  double *RasterDataD; */
	
	fprintf(stderr,"\nWriting output file: \n\n");
	switch(type) {
	   
	case flow_map :
	  out  = fopen(file, "w");
	  if (out == NULL) {
	    fprintf(stderr, "Cannot open FLOW file=[%s]:[%s]! Exiting.\n",
	    file, strerror(errno));
	    return 1;
	  }
	  for(j=0; j < flowcount; j++) { 
	
		/*
		for each cell in the current worker list
		Retrieve Active Cell values and print to flowfile:
		x y z: easting  northing  thickness
		*/
		easting =geotransform[0] + (geotransform[1] * flowList[j].col);
		northing = geotransform[3] + (geotransform[5] * flowList[j].row);
		value = flowList[j].thickness + dataGrid[flowList[j].row][flowList[j].col].residual;
		elev = dataGrid[flowList[j].row][flowList[j].col].elev;
		if (!j)
			fprintf(out, "# %0.3f\t%.03f\t%0.4f\t%0.4f\t%0.1f",
			        vent->easting, vent->northing, vent->volumeToErupt, vent->pulsevolume, vent->residual );	
		fprintf(out, "\n%0.3f\t%0.3f\t%0.6f\t%0.1f", easting, northing, value, elev);		 
	  }
	  fflush(out);
	  fclose(out);
		
	  fprintf(stderr, " ASCII Output file: %s successfully written.\n (x,y,thickness)\n",
		      file);
	break;
	
	case hits_map :
	  out = fopen(file, "w");
	  if (out == NULL) {
		fprintf(stderr, "Cannot open hit file: file=[%s]:[%s]! Exiting.\n",
	    file, strerror(errno));
	    return 1;
	  }
	
	  for(row=0; row < geotransform[4]; row++) { 
		for(col=0; col < geotransform[2]; col++) {
			/*
			For each cell in the dem,
			retrieve hit count values and print to hit map:
			x y z: easting  northing  count
			
			if (!row) fprintf (out, "#");
			*/
			easting = geotransform[0] + (geotransform[1] * col);
			northing = geotransform[3] + (geotransform[5] * row);
			value = (double) dataGrid[row][col].hit_count;
			if (value) fprintf(out, "\n%0.3f\t%0.3f\t%0.0f", easting, northing, value);
		}			
	  }
	  fflush(out);
	  fclose(out);
	  fprintf(stderr, " ASCII Output file: %s successfully written.\n (x,y,hit count)\n",
		      file);
	break;
	}
	
	/*for each cell in the dem*/
			/*Retrieve new elevation and print to file*/
			/*x,y,z: easting, northing, elevation
	out = fopen(Out->total_thickness_map, "w");
	if (out == NULL) {
		fprintf(stderr, "Cannot open total thickness map: file=[%s]:[%s]! Exiting.\n",
	    Out->total_thickness_map, strerror(errno));
	    return(-1);
	}
	
	for(row=0; row < geotransform[4]; row++) { 
		for(col=0; col < geotransform[2]; col++) {
			
			if (dataGrid[row][col].pile) 
				fprintf(out, "%0.3f\t%0.3f\t%0.6f\n",
			        (float) (geotransform[0] + (geotransform[1] * col)),
			        (float) (geotransform[3] + (geotransform[5] * row)),                           
			        (dataGrid[row][col].pile));
		}
	}
	fclose(out);
	fprintf(stderr, " ASCII Output file: %s successfully written.\n (x,y,thickness)\n",
		      Out->total_thickness_map);
*/
	/*END ASCII DATATYPES********************************************************/
	/*END RASTER DATATYPES*******************************************************/
	
	return(0);
}
