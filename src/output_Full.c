#include "prototypes.h"

int OUTPUT(Outputs Out, Inputs In, DataCell **dataGrid, Automata *flowList,
           FlowStats FlowParam, VentArr *Vents) {
/*Module OUTPUT
	Prints grid cells locations inundated by lava and thicknesses to a tab-del
	file. For flexibility, more parameters are needed as arguments for this module
	in case of future needs to output additional information
	
	File types to output:
	0: X,Y,Thickness ascii flow list
	1: Hit Raster (1 = Hit, 0 = Not Hit)
	2: Thickness Raster
	3: Elevation Raster
	4: Elevation + Lava Raster (2+3)
	
*/
	/*GeoTransform Double List:
	
	  Transform Index    This Code                        GDAL
		In.dem_grid_data[0]    lower left x                     top left x (SAME)
	  In.dem_grid_data[1]    w-e pixel res                    SAME
	  In.dem_grid_data[2]    number of cols                   0
	  In.dem_grid_data[3]    lower left y                     top left y
	  In.dem_grid_data[4]    number of lines                  0
	  In.dem_grid_data[5]    n-s pixel res (positive value)   (negative value)
	
	*/
	
	FILE     *flowfile;
	unsigned f,i,j,k;
	File_output_type type;
	char *outputfilename;
	char projIfNull[] = "";
	
	GDALDatasetH hDstDS;
	double GDALGeoTransform[6];
	/*const char *pszFormat = "GTiff";*/
  GDALDriverH hDriver = GDALGetDriverByName("GTiff"); /*Try this*/
	GDALRasterBandH hBand;
	GUInt16 *RasterDataU;
	float   *RasterDataF;
	int raster_double_file = 0;
	CPLErr IOErr;
	
	/*Modify Metadata back to GDAL format*/
	GDALGeoTransform[0] = In.dem_grid_data[0];
	GDALGeoTransform[1] = In.dem_grid_data[1];
	GDALGeoTransform[3] = In.dem_grid_data[3] + (In.dem_grid_data[5] * In.dem_grid_data[4]);
	GDALGeoTransform[5] = -1 * In.dem_grid_data[5];
	GDALGeoTransform[2] = GDALGeoTransform[4] = 0;
	
	if (In.dem_proj==NULL) In.dem_proj = projIfNull;
	
	//CYCLE THROUGH FILES IN OUTPUT LIST. THIS WILL NEED UPDATING WHEN THE OUTPUT
	//STRUCTURE CHANGES
	//But I'd rather do this here than call OUTPUT 7 times in a row elsewhere -JR
	for (f=0; f<=6; f++) {
		//Determine if output file exists for each file type
		if (f==0) {
			if (Out.ascii_flow_file != NULL) {
				type = ascii_flow;
				outputfilename = Out.ascii_flow_file;
			}
			else continue;
		}
		else if (f==1) {
			if (Out.ascii_hits_file != NULL) {
				type = ascii_hits;
				outputfilename = Out.ascii_hits_file;
			}
			else continue;
		}
		else if (f==2) {
			if (Out.raster_hits_file != NULL) {
				type = raster_hits;
				outputfilename = Out.raster_hits_file;
			}
			else continue;
		}
		else if (f==3) {
			if (Out.raster_flow_file != NULL) {
				type = raster_flow;
				outputfilename = Out.raster_flow_file;
			}
			else continue;
		}
		else if (f==4) {
			if (Out.raster_pre_topo != NULL) {
				type = raster_pre;
				outputfilename = Out.raster_pre_topo;
			}
			else continue;
		}
		else if (f==5) {
			if (Out.raster_post_topo != NULL) {
				type = raster_post;
				outputfilename = Out.raster_post_topo;
			}
			else continue;
		}
		else if (f==6) {
			if (Out.stats_file != NULL) {
				type = stats_file;
				outputfilename = Out.stats_file;
			}
			else continue;
		}
		
		raster_double_file = 0;
		
		/************************************************/
		//      PRINT OUT FILES IF PROVIDED
		/************************************************/
		
		switch(type) {
	
		/*X,Y,Thickness ASCII Flow List********************************************/
		case ascii_flow :
			/*Open flow file (x,y,thickness)*/
			flowfile  = fopen(outputfilename, "w");
			if (flowfile == NULL) {
				fprintf(stderr, "Cannot open FLOW file=[%s]:[%s]! Exiting.\n",
				outputfilename, strerror(errno));
				return 1;
			}
	
				for(i=1;i<=FlowParam.active_count;i++) { /*for each cell in flow list*/
					/*Retrieve Active Cell values and print to flowfile*/
					/*x,y,z: easting, northing, thickness*/
					fprintf(flowfile, "%0.3f\t%0.3f\t%0.6f\n", 
							   (float) In.dem_grid_data[0] + (In.dem_grid_data[1] * flowList[i].col),
							   (float) In.dem_grid_data[3] + (In.dem_grid_data[5] * flowList[i].row),
							   (float) (flowList[i].thickness + 
							            dataGrid[flowList[i].row][flowList[i].col].residual));
				}
			fclose(flowfile);
			fprintf(stdout,
				     " ASCII   Output file: %s successfully written.\n  (x,y,thickness)\n",
				     outputfilename);
	
		break;
	
		/*ASCII HIT COUNT LIST*****************************************************/
		case ascii_hits :
			/*Open hit file (x,y,hit count)*/
			flowfile  = fopen(outputfilename, "w");
			if (flowfile == NULL) {
				fprintf(stderr, "Cannot open HITS file=[%s]:[%s]! Exiting.\n",
				outputfilename, strerror(errno));
				return 1;
			}
	
				for(i=1;i<=FlowParam.active_count;i++) { /*for each cell in flow list*/
					/*Retrieve Hit Count Cell values and print to file*/
					/*x,y,z: easting, northing, hit count*/
					fprintf(flowfile, "%0.3f\t%0.3f\t%d\n", 
							   (float) In.dem_grid_data[0] + (In.dem_grid_data[1] * flowList[i].col),
							   (float) In.dem_grid_data[3] + (In.dem_grid_data[5] * flowList[i].row),
							   (unsigned) (dataGrid[flowList[i].row][flowList[i].col].hit_count));
				}
			fclose(flowfile);
			fprintf(stdout,
				     " ASCII   Output file: %s successfully written.\n  (x,y,hit count)\n",
				     outputfilename);
		break;
	
		/*END ASCII DATATYPES******************************************************/
	
		/*HIT COUNT RASTER*********************************************************/
		case raster_hits :
		
			/*Create Data Block*/
			RasterDataU = malloc (sizeof (GUInt16) * 
			                      In.dem_grid_data[2] * In.dem_grid_data[4]);
			if (RasterDataU == NULL) {
				fprintf(stderr, "[OUTPUT] Out of Memory creating Outgoing Raster!!\n\n");
				return 1;
			}
			
			/*Assign Model Data to Data Block*/
			k=0; /*Data Counter*/
			for(i=In.dem_grid_data[4];i>0;i--) { /*For each row, TOP DOWN*/
				for(j=0;j<In.dem_grid_data[2];j++) {         /*For each col, Left->Right*/
					//if(dataGrid[i-1][j].active) RasterDataB[k++] = 1;
					//else RasterDataB[k++] = 0;
					RasterDataU[k++] = dataGrid[i-1][j].hit_count;
				}
			}
		
			/*Setup Raster Dataset*/
			hDstDS = GDALCreate(hDriver, outputfilename, In.dem_grid_data[2], 
				                  In.dem_grid_data[4], 1, GDT_UInt16, NULL);
		
			GDALSetGeoTransform( hDstDS, GDALGeoTransform ); /*Set Transform*/
			GDALSetProjection( hDstDS, In.dem_proj );     /*Set Projection*/
			hBand = GDALGetRasterBand( hDstDS, 1 );      /*Set Band to 1*/
		
			/*Write the formatted raster data to a file*/
			IOErr = GDALRasterIO(hBand, GF_Write, 0, 0, In.dem_grid_data[2], In.dem_grid_data[4],
						       RasterDataU, In.dem_grid_data[2], In.dem_grid_data[4], GDT_UInt16, 
						       0, 0 );
			if(IOErr) {
				fprintf(stderr, "ERROR [OUTPUT]: Error from GDALRasterIO!!\n");
				return 1;
			}
		
			/*Properly close the raster dataset*/
			GDALClose( hDstDS );
			free(RasterDataU);
		
			fprintf(stdout, " %s Output file: %s successfully written.\n",
				  	   GDALGetDriverLongName( hDriver ),outputfilename);
		
		break;
	
		/*Thickness Raster*********************************************************/
		case raster_flow :
		
			/*Create Data Block*/
			if((RasterDataF = malloc (sizeof (float) * In.dem_grid_data[2] *
						                    In.dem_grid_data[4]))==NULL) {
				fprintf(stderr, 
				        "[OUTPUT] Out of Memory creating Outgoing Raster Data Array\n");
				return(-1);
			}
		
			k=0; /*Data Counter*/
			for(i=In.dem_grid_data[4];i>0;i--) { /*For each row, TOP DOWN*/
				for(j=0;j<In.dem_grid_data[2];j++) {       /*For each col, Left->Right*/
					if(dataGrid[i-1][j].active) {
						RasterDataF[k++] = (float) 
						                   flowList[dataGrid[i-1][j].active].thickness + 
						                   dataGrid[i-1][j].residual;
					}
					else RasterDataF[k++] = (float) 0.0;
				}
			}
			
			raster_double_file = 1;
			
		break;
	
		/*PRE FLOW TOPOGRAPHY RASTER***********************************************/
		case raster_pre :
		
			/*Create Data Block*/
			if((RasterDataF = malloc (sizeof (float) * In.dem_grid_data[2] *
						                    In.dem_grid_data[4]))==NULL) {
				fprintf(stderr,
				        "[OUTPUT] Out of Memory creating Outgoing Raster Data Array\n");
				return(-1);
			}
		
			k=0; /*Data Counter*/
			for(i=In.dem_grid_data[4];i>0;i--) { /*For each row, TOP DOWN*/
				for(j=0;j<In.dem_grid_data[2];j++) {       /*For each col, Left->Right*/
					RasterDataF[k++] = (float) dataGrid[i-1][j].elev;
				}
			}
			
			raster_double_file = 1;
		break;
	
		/*POST FLOW TOPOGRAPHY RASTER**********************************************/
		case raster_post :
		
			/*Create Data Block*/
			if((RasterDataF = malloc (sizeof (float) * In.dem_grid_data[2] *
						                    In.dem_grid_data[4]))==NULL) {
				fprintf(stderr,
				        "[OUTPUT] Out of Memory creating Outgoing Raster Data Array\n");
				return(-1);
			}
		
				k=0; /*Data Counter*/
				for(i=In.dem_grid_data[4];i>0;i--) { /*For each row, TOP DOWN*/
					for(j=0;j<In.dem_grid_data[2];j++) {         /*For each col, Left->Right*/
						if(dataGrid[i-1][j].active) {
							RasterDataF[k++] = (float) flowList[dataGrid[i-1][j].active].elev;
						}
						else RasterDataF[k++] = (float) dataGrid[i-1][j].elev;
					}
				}
				
			raster_double_file = 1;
		break;
	
		/*STATISTICS FILE**********************************************************/
		case stats_file :
			if (!FlowParam.run) flowfile  = fopen(outputfilename, "w");
			else flowfile  = fopen(outputfilename, "a");
			if (flowfile == NULL) {
				fprintf(stderr, "Cannot open stats file=[%s]:[%s]! Exiting.\n",
				        outputfilename, strerror(errno));
				return 1;
			}
			
			//Print out header if first run
			if (!FlowParam.run) {
				//Run, Residual, Vent Easting, Vent Northing, Vent Volume, Vent Pulse, (xVents)
				fprintf(flowfile, "#Sim\tResidual");
				for (i=1; i<=FlowParam.vent_count; ++i ) {
					fprintf(flowfile,"\tVent%d_Easting\tVent%d_Northing\tVent%d_TotVol\tVent%d_PulseVol",
						      i,i,i,i);
				}
				fprintf(flowfile,"\n");
			}
			
			if (Vents[0].remainingvolume != DBL_MAX) { //This is the Error handle
			//Only write stats if 1st Vent's remaining volume is real.
				fprintf(flowfile, "%d\t%0.3f", FlowParam.run, FlowParam.residual);
				for (i=0; i<FlowParam.vent_count; i++ ) {
					fprintf(flowfile, "\t%0.3f\t%0.3f\t%0.3f\t%0.3f",
							    Vents[i].easting,
							    Vents[i].northing,
							    Vents[i].totalvolume,
							    Vents[i].pulsevolume
							   );
				}
				fprintf(flowfile,"\n");
			}
			
			fclose(flowfile);
			fprintf(stdout,
				     " STATS   Output file: %s successfully written.\n",
				     outputfilename);
		break;
	
		}
	
		//Write the raster file here if cells have double floating point values.
		if (raster_double_file) {
			/*Setup Raster Dataset*/
			hDstDS = GDALCreate(hDriver, outputfilename, In.dem_grid_data[2], 
				                  In.dem_grid_data[4], 1, GDT_Float32, NULL);
		
			GDALSetGeoTransform( hDstDS, GDALGeoTransform ); /*Set Transform*/
			GDALSetProjection( hDstDS, In.dem_proj );     /*Set Projection*/
			hBand = GDALGetRasterBand( hDstDS, 1 );      /*Set Band to 1*/
		
			/*Write the formatted raster data to a file*/
			IOErr = GDALRasterIO(hBand, GF_Write, 0, 0, In.dem_grid_data[2], In.dem_grid_data[4],
						       RasterDataF, In.dem_grid_data[2], In.dem_grid_data[4], GDT_Float32, 
						       0, 0 );
			if(IOErr) {
				fprintf(stderr, "ERROR [OUTPUT]: Error from GDALRasterIO!!\n");
				return 1;
			}
		
			/*Properly close the raster dataset*/
			GDALClose( hDstDS );
			free(RasterDataF);
		
			fprintf(stdout, " %s Output file: %s successfully written.\n",
				     GDALGetDriverLongName( hDriver ),outputfilename);
	
	
		}
	}
	
	return 0;
	
}
