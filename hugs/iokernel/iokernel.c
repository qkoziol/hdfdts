/**
 *
 * Sandia HUGS I/O Kernel Program
 *
 * October 5, 2006
 *
 * Maikael A. Thomas \n
 * Sandia National Laboratories \n
 * Albuquerque, NM 87185-0708
 *
*/

#pragma warning(disable : 810)
#pragma warning(disable : 981)

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <hdf5.h>
#include "iokernel.h"

void usage();
#define _XOPEN_SOURCE 600

/********************************************************************************************
 * Create a dataset in the HDF5 file
 ********************************************************************************************/
hid_t iokernelCreateH5Dataset(hid_t group_id, const char *name, hid_t type,
							  int rank, hsize_t size0, hsize_t chunk0,  ...) {


    va_list args;
    hid_t   space;
    hid_t   parms;
    hsize_t dims[3];
    hsize_t maxDims[3];
    hsize_t chunkDims[3];
    hid_t   new_id;
    int     i;

    /*-- Capture the slowest-changing dimension size --*/
    dims[0]      = size0;
    chunkDims[0] = chunk0;
    

    /*-- Collect the other 'rank' sizes --*/
    va_start(args, chunk0);

    for (i = 1; i < rank; i++) {

		hsize_t size =   (hsize_t)va_arg(args, int);
		hsize_t chunk =  (hsize_t)va_arg(args, int);
	
		dims[i]      = size;
		chunkDims[i] = chunk; 
		maxDims[i]   = (size > chunk) ? size : chunk;
    }

    va_end(args);

    /* 
     *  Since dataset is 'extensible', make the slowest changing dimension
     *  unlimited.
     */
	maxDims[0] = H5S_UNLIMITED;

	/*-- Create a simple space defining the size of the dataset --*/
	if ( (space = H5Screate_simple(rank, dims, maxDims)) < 0)
	{
	    printf("Failed to create a simple dataspace.\n");
	    return -1;
	}

	/*-- Create a parameter list so we can specify the chunksize --*/
	if ( (parms = H5Pcreate(H5P_DATASET_CREATE)) < 0)
	{
	    printf("Failed to create a parameter list.\n");
	    return -1;
	}

	/*-- Extensible datasets need a chunksize. --*/
	if(H5Pset_layout(parms, H5D_CHUNKED)<0) {
           printf("Failed to set chunking storage layout.\n");
           return -1;
        }

	if(H5Pset_chunk(parms, rank, chunkDims)<0){
            printf("Failed to create chunking property list.\n");
            return -1;
        }

	/*-- Create the new dataset --*/
	if((new_id = H5Dcreate(group_id, name, type, space, parms))<0){
           H5Sclose(space);
           H5Pclose(parms);
           printf("Failed to create an HDF5 dataset.\n");
           return -1;
        }
	
	/*-- Cleanup --*/
	if(H5Sclose(space)<0) {
          printf("Successfully close the dataspace.\n");
          return -1;
        }

	if(H5Pclose(parms)<0) {
          printf("Successfully close the creating property list.\n");
          return -1;
        }

    return new_id;
}

/********************************************************************************************
 * Extend a dataset in the HDF5 file
 ********************************************************************************************/
hid_t iokernelExtendH5Dataset(hid_t dataset_id, hsize_t newSize) {

    herr_t  status;
    hid_t   new_space;
    hsize_t dims[3];
    hsize_t maxDims[3];
    
    if ((new_space = H5Dget_space(dataset_id)) < 0) {
	printf("Can't get the space from this dataset.\n");
	return -1;
    }

    /*-- Get the old size --*/
    if(H5Sget_simple_extent_dims(new_space, dims, maxDims)<0) {
        printf("Can't obtain dimension size from the dataspace.\n");
        return -1;
    }

    /*-- Extend the dataspace in the slowest-varying dimension --*/
    dims[0] = newSize;
    status = H5Dextend(dataset_id, dims);
    if (status < 0) {
        H5Sclose(new_space);
	printf("Failed to extend dataset.\n");
	return -1;
    }
    if(H5Sclose(new_space)<0) {
       printf("Can't close the dataspace.\n");
       return -1;
    }

    return 0;
}


/********************************************************************************************
 * Write data to the HDF5 file
 ********************************************************************************************/
int iokernelWriteH5Data(hid_t dataset_id, hid_t type, ...) {

    int       rank;
    herr_t    status;
    hid_t     memspace;
    hid_t     filespace;
    hsize_t   overallDims[3];
    hsize_t   dims[3];
    hsize_t   maxDims[3];
    hsize_t   regionCounts[3];
    hsize_t   regionOffsets[3];
    va_list   args;
    void     *data;
    int       i;
    

    /*-- First, retrieve the overall dimensions --*/
    if ( (filespace = H5Dget_space(dataset_id)) < 0) {
	printf("Can't get the space from this dataset.\n");
	return -1;
    }

    if((rank = H5Sget_simple_extent_ndims(filespace))<0) {
       printf("Can't obtain the rank of the dataspace.\n");
       return -1;
    }

    if(H5Sget_simple_extent_dims(filespace, overallDims, maxDims)<0){
      printf("Can't obtain the dimensional size of the dataspace.\n");
      return -1;
    }

    /*-- Retrieve the offsets and counts --*/
    va_start(args, type);
    
    for (i = 0; i < rank; i++) {

	hsize_t offset = (hsize_t) va_arg(args, int);
	hsize_t count  = (hsize_t) va_arg(args, int);

	/*-- Do we want to span ALL the data in this dimension? --*/
	if (count == 0) {
	   count  = overallDims[i];
	   offset = 0;
	}

	/*-- Do a range-check for this dimension --*/
	if (offset + count > overallDims[i]) {

            printf("The specified offset and element count exceed the size of the dataset in dimension %d.  You wanted to write %d elements starting at %d, but the dataset stops after %d elements\n", i, (int) count, (int) offset, (int) overallDims[i]);
		}
	
	    /*-- Specify the memory space that describes the data --*/
	    dims[i] = count;

           /*-- Specify the region to store the memspace in our filespace --*/
            regionCounts[i]  = count;
            regionOffsets[i] = offset;
    }

    /*-- Finally, get our data pointer from the variable argument list --*/
    data = (void *) va_arg(args, void *);
    va_end(args);

    /*-- Build a memory space that describes our data pointer --*/
    memspace = H5Screate_simple(rank, dims, maxDims);

    if (memspace < 0) {
	printf("Failed to create a memory space.\n");
	return -1;
    }

    /*-- Build the region to store the memspace in our filespace --*/
    status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, 
				 regionOffsets, NULL, regionCounts, NULL);
    if (status < 0) {
	printf("Failed to select a hyperslab.\n");
	return -1;
    }

    /*-- Write the data --*/
    status = H5Dwrite(dataset_id, type, memspace, filespace, H5P_DEFAULT, data);
    if(status < 0) {
       H5Sclose(filespace);
       H5Sclose(memspace);
       printf("Failed to write the data.\n");
       return -1;
    }

    /*-- Cleanup --*/
    if(H5Sclose(filespace)<0){
       H5Sclose(memspace);
       printf("Can't close file dataspace.\n");
       return -1;
    }

    if(H5Sclose(memspace)<0) {
       printf("Can't close memory dataspace.\n");
       return -1;
    }

#if 0
    /*-- Garbage collection to free internal resources --*/
    if(H5garbage_collect()<0) {
      printf("Can't free unused memory.\n");
      return -1;
    }
#endif
	
    return 0;
}


/********************************************************************************************
 * HUGS I/O Kernel Main Routine
 ********************************************************************************************/
int main(int argc, char **argv) {

	int i, j, k,iclean;
	int num_iter;
	int page_size;

	/* Variables to read simulation file */
	FILE *simFP;
        char linebuf[MAXLINE];
	int streamNum, row, col;
	int numStreams;

	
	/* Memory buffers to simulate shared memory */
	Metadata_Info *meta_main;
	unsigned char *metadataShm,  *rawDataShm;
	unsigned char *metadataShmPtr,  *rawDataShmPtr;
	Metadata_256 *meta1_256, *meta2_256;
	Metadata_2048 *meta3_2048;
	unsigned short *rawData;
	
	/* Shared memory variables */
	size_t rawShmSize, metaShmSize;

	/* HDF5 variables */
	char hdf5_name[256], group_path[256];
	hid_t file_creation_properties, file_access_properties;
	hid_t file_id, root_id;
	hid_t group_id[MAXSTREAMS];
	unsigned int h5flags;
	hid_t frameDatastream[MAXSTREAMS], meta1Dataset[MAXSTREAMS], meta2Dataset[MAXSTREAMS], meta3Dataset[MAXSTREAMS];
	hid_t meta256_type_id, meta2048_type_id;
	unsigned int frame_count[MAXSTREAMS];

        /*timing variables */
        struct timeval start_time, end_time;
        int    total_wtime, raw_wtime, meta_wtime;
        int*   total_stream_wtime, *raw_stream_wtime,*meta_stream_wtime;
        int    total_bwidth,raw_bwidth,meta_bwidth; 
        int*   total_stream_bwidth,*raw_stream_bwidth,*meta_stream_bwidth;

	if(argc!=5){
	   usage();
           return 0;
        }

	sprintf(hdf5_name, "%s", argv[1]); 
	num_iter      = atol(argv[2]);
printf("number of iteration is %d\n",num_iter);
	rawShmSize    = MB*atol(argv[3]);
printf("raw memory size is %llu\n",rawShmSize);
	metaShmSize   = MB*atol(argv[4]);
printf("meta memory size is %llu\n",metaShmSize);

	
       
   /**********************************************************************************
    * Set up main metadata buffer
    ***********************************************************************************/
	meta_main = (Metadata_Info *) calloc (MAXSTREAMS, sizeof(Metadata_Info));
        if(meta_main == NULL) {
          printf("Cannot allocate memory.\n");
          return -1;
        }

   /**********************************************************************************
    * Parse simulation file
    ***********************************************************************************/
    if ((simFP = fopen("sim_input.txt", "r")) == NULL) {

	fprintf(stderr,"iokernel: Can't open input file: %s\n", "sim_input.txt");
	exit(-1);
    }

	i = 0;
	while (fgets(linebuf,MAXLINE,simFP)) {

		//fprintf(stderr, "sim line: %s", linebuf);

		/* assume blank or comment line */
		if (linebuf[0] == '#' || linebuf[0] == ' ' || linebuf[0] == '\n' || linebuf[0] == '\t')
			continue;	

		/* Get the important parameters from this line */
		sscanf(linebuf,"%d %d %d", &streamNum, &row, &col);

		fprintf(stderr, "Parsed LINE: %d %d %d\n", streamNum, row, col);

		/* Boundary checking for stream number */
		if ((streamNum < 0) || (streamNum >= MAXSTREAMS)) {

         		fprintf(stderr, "Invalid Stream on line %s Stream must be btwn 0 and %d\n", linebuf, MAXSTREAMS);
			exit (-1);
		}

		/* Boundary checking for row and column size */
		if (row < 64 || row >= 8192 || col < 64 || col >= 8192) {

			fprintf(stderr, "Invalid row/col on line %s Row/col must be btwn 64 and 8192\n", linebuf);
			exit (-1);
		}

		/* For this simple test, limit the sizes of data allowed */
		if (row != col || (row != 64 && row != 128 && row != 256 && row != 512 && row != 1024 && row != 2048 && row != 4196 && row != 8192)) {

			fprintf(stderr, "Rows must equal columns AND only power of 2 frame sizes supported\n");
			exit (-1);
		}

		/* Fill in the metadata info structure to keep track of the streams */
		meta_main[i].datastream = streamNum;
		meta_main[i].rows = row;
		meta_main[i].cols = col;

		/* Set the frame counter to zero */
		frame_count[i] = 0;

		/* Fill in the number of contiguous frames to write based on frame size */
		switch (row) {
			
		case 64:
			meta_main[i].num_contiguous = 128;
			break;
		case 128:
			meta_main[i].num_contiguous = 32;
			break;
		case 256:
			meta_main[i].num_contiguous = 8;
			break;
		case 512:
			meta_main[i].num_contiguous = 2;
			break;
		default:
			meta_main[i].num_contiguous = 1;
			break;
		}

		/* If the sim file contains more lines than streams, time to stop reading the file */
		i++;
		if (i >= MAXSTREAMS - 1) {

			fprintf(stderr, "Sim input file too long, read first %d lines\n", i);
			break;
		}
	}

	numStreams = i;

   /**********************************************************************************
    * Set up simulated raw and metadata shared memory buffers
    ***********************************************************************************/
	page_size = getpagesize();
        printf("page_size = %d\n",page_size);
/*	rawShmSize = 2*1024L*1024L*1024L;
	metaShmSize = 500*1024L*1024L;
*/
	/* Allocate raw memory buffer on a page boundary */
	if (posix_memalign ((void **) &rawDataShm, page_size, rawShmSize) != 0) {

		fprintf(stderr, "Cannot alloate raw memory\n");
		exit (-1);
	}

	/* Memset the raw memory to tell the difference between chunks of memory */
	rawData = (unsigned short *) rawDataShm;
	for (i=0, j=0; i<rawShmSize/sizeof(short); i+=1024*512, j++) {
		for (k=i; k<i+1024*512;k++)
			rawData[k] = j;
	}

	/* Stick a random variable in each page of memory to reduce caching effects */
	rawDataShmPtr = rawDataShm + page_size-1;
        for(i=page_size-1; i<rawShmSize; i+=page_size, rawDataShmPtr+=page_size) {
		*rawDataShmPtr = (char) (255.0*rand()/(RAND_MAX+1.0));
	}

	/* Allocate metadata buffer */
	if (posix_memalign ((void **) &metadataShm, page_size, metaShmSize) != 0) {
		fprintf(stderr, "Cannot alloate metadata memory\n");
		exit (-1);
	}

	/* Memset the metadata memory to tell the difference between chunks of memory */
	metadataShmPtr = metadataShm;
	for (i=0, j=0; i<metaShmSize; i+=256, j++) {
		memset(metadataShmPtr, j, 256);
		metadataShmPtr += 256;
	}

	/* Stick a random variable in each page of memory to reduce caching effects */
	metadataShmPtr = metadataShm + page_size-1;
    for(i=page_size-1; i<metaShmSize; i+=page_size, metadataShmPtr+=page_size) {

		*metadataShmPtr = (char) (255.0*rand()/(RAND_MAX+1.0));
	}

	/* Point raw and metadata pointers to simulated shared memory */
	rawDataShmPtr = rawDataShm;
	metadataShmPtr = metadataShm;

/**********************************************************************************
    * Open the HDF5 file
    ***********************************************************************************/
	h5flags = H5F_ACC_TRUNC;

/*	sprintf(hdf5_name, "%s", "iokernel.h5"); */ 

    /*-- Initialize our default file creation parameters --*/
    if ( (file_creation_properties = H5Pcreate(H5P_FILE_CREATE)) < 0 ) {

		printf("Failed to create default file creation properties.");
		exit (-1);
	}

    /*-- Initialize our default file access parameters --*/
    if ( (file_access_properties = H5Pcreate(H5P_FILE_ACCESS)) < 0 ) {

		printf("Failed to create default file access properties.");
		exit (-1);
	}

	/*-- Open the file --*/
	if ( (file_id = H5Fcreate(hdf5_name, h5flags, file_creation_properties, file_access_properties)) < 0 )
	{
            H5Pclose(file_creation_properties);
            H5Pclose(file_access_properties);
	    printf("Failed to open the data product file '%s'.\n", hdf5_name);
	    exit (-1);
	}

	/* Root */
	if ( (root_id = H5Gopen(file_id, "/")) < 0 )
	{
	    printf("Failed to open the '/' group in the data product file '%s'.", hdf5_name);
            H5Pclose(file_creation_properties);
            H5Pclose(file_access_properties);
	    H5Fclose(file_id);
	    exit (-1);
	}

	/**********************************************************************************
    * Create datasets in the HDF5 file
    ***********************************************************************************/

	/* Create a 256 byte metadata type */
	if((meta256_type_id = H5Tcreate(H5T_COMPOUND, sizeof(Metadata_256)))<0){
           printf("Cannot create HDF5 datatype at line %d\n",__LINE__);
           H5Pclose(file_creation_properties);
           H5Pclose(file_access_properties);
           H5Fclose(file_id);
           exit(-1);
        }

	if((H5Tinsert(meta256_type_id, "Meta 1", 0, H5T_NATIVE_UCHAR))<0){
            printf("Can't insert atomic datatype into compound datatype at line %d\n",__LINE__);
            H5Tclose(meta256_type_id);
            H5Pclose(file_creation_properties);
            H5Pclose(file_access_properties);
            H5Fclose(file_id);
            exit(-1);
        }

	/* Create a 2048 byte metadata type */
	if((meta2048_type_id = H5Tcreate(H5T_COMPOUND, sizeof(Metadata_2048)))<0) {
           printf("Cannot create HDF5 datatype at line %d\n",__LINE__);
           H5Tclose(meta256_type_id);
           H5Pclose(file_creation_properties);
           H5Pclose(file_access_properties);
           H5Fclose(file_id);
           exit(-1);
        }               
	if((H5Tinsert(meta2048_type_id, "Meta 1", 0, H5T_NATIVE_UCHAR))<0) {
           printf("Cannot insert HDF5 datatype at line %d\n",__LINE__);
           H5Tclose(meta256_type_id);
           H5Tclose(meta2048_type_id);
           H5Pclose(file_creation_properties);
           H5Pclose(file_access_properties);
           H5Fclose(file_id);
           exit(-1);
        }

	for (i=0; i<numStreams; i++) {

		/* Create the Datastream group */
		sprintf(group_path, "%s%d", "/Datastream", meta_main[i].datastream);

		if ( (group_id[meta_main[i].datastream] = H5Gcreate(file_id, group_path, 0)) < 0 ) {

	           printf("Failed to open the %s group in the data product file '%s'.\n", group_path, hdf5_name);
                   H5Tclose(meta256_type_id);
                   H5Tclose(meta2048_type_id);
                   H5Pclose(file_creation_properties);
                   H5Pclose(file_access_properties);
                   H5Fclose(file_id);
	           exit (-1);
		}

		/* Create the frame dataset, make it extensible */

printf("meta_main[%d].num_contiguous=%d\n",i,meta_main[i].num_contiguous);
printf("meta_main[%d].rows=%d\n",i,meta_main[i].rows);
printf("meta_main[%d].cols=%d\n",i,meta_main[i].cols);
		if ( (frameDatastream[meta_main[i].datastream] = iokernelCreateH5Dataset(group_id[meta_main[i].datastream], "Frame Data", H5T_NATIVE_USHORT,
		 3, 0, meta_main[i].num_contiguous, meta_main[i].rows, 
		 meta_main[i].rows, meta_main[i].cols, meta_main[i].cols)) <= 0) { 

		   printf("Failed to create the frame datastream.\n");
	           H5Tclose(meta256_type_id);
                   H5Tclose(meta2048_type_id);
                   H5Pclose(file_creation_properties);
                   H5Pclose(file_access_properties);
                   H5Fclose(file_id);
 		   exit (-1);
		}

		/* Create the 256 byte Metadata 1 dataset, make it extensible */
		if ( (meta1Dataset[meta_main[i].datastream] = iokernelCreateH5Dataset(group_id[meta_main[i].datastream], "Metadata1", meta256_type_id,
			 1, 0, meta_main[i].num_contiguous)) <= 0) { 

         	   printf("Failed to create the metadata 1 dataset.\n");
 	           H5Tclose(meta256_type_id);
                   H5Tclose(meta2048_type_id);
                   H5Pclose(file_creation_properties);
                   H5Pclose(file_access_properties);
                   H5Fclose(file_id);
                   exit (-1);
		}

		/* Create the 256 byte Metadata 2 dataset, make it extensible */
		if ( (meta2Dataset[meta_main[i].datastream] = iokernelCreateH5Dataset(group_id[meta_main[i].datastream], "Metadata2", meta256_type_id,
			 1, 0, meta_main[i].num_contiguous)) <= 0) { 

        	   printf("Failed to create the metadata 2 dataset.\n");
		   H5Tclose(meta256_type_id);
                   H5Tclose(meta2048_type_id);
                   H5Pclose(file_creation_properties);
                   H5Pclose(file_access_properties);
                   H5Fclose(file_id);
 		   exit (-1);
		}

		/* Create the 2048 byte Metadata 3 dataset, make it extensible */
		if ( (meta3Dataset[meta_main[i].datastream] = iokernelCreateH5Dataset(group_id[meta_main[i].datastream], "Metadata3", meta2048_type_id,
																				 1, 0, meta_main[i].num_contiguous)) <= 0) { 

		   printf("Failed to create the metadata 3 dataset.\n");
	           H5Tclose(meta256_type_id);
                   H5Tclose(meta2048_type_id);
                   H5Pclose(file_creation_properties);
                   H5Pclose(file_access_properties);
                   H5Fclose(file_id);
 		   exit (-1);
		}
	}

	/**********************************************************************************
    * Iterate through each of the active datastreams and write to the HDF5 file
    ***********************************************************************************/
/*	num_iter = 10;*/
       
        total_wtime = 0;
        raw_wtime   = 0;
        meta_wtime  = 0;
        total_stream_wtime = (int*)calloc((size_t)numStreams,sizeof(int));
        raw_stream_wtime   = (int*)calloc((size_t)numStreams,sizeof(int));
        meta_stream_wtime  = (int*)calloc((size_t)numStreams,sizeof(int));
         

	for (i=0; i<num_iter; i++) {

		for (j=0; j<numStreams; j++) {

              /*Start measuring the time */
              gettimeofday(&start_time,(struct timeval*)0);


			/* Extend the frame dataset */
			if( iokernelExtendH5Dataset(frameDatastream[meta_main[j].datastream], frame_count[j] + meta_main[j].num_contiguous) < 0) {

		   printf("Failed to extend frame dataset.\n");
	           H5Tclose(meta256_type_id);
                   H5Tclose(meta2048_type_id);
                   H5Pclose(file_creation_properties);
                   H5Pclose(file_access_properties);
                   H5Fclose(file_id);
 		   exit (-1);
		   }

			/* Write some frames to the frame dataset */
			if( iokernelWriteH5Data(frameDatastream[meta_main[j].datastream], H5T_NATIVE_USHORT, frame_count[j], meta_main[j].num_contiguous,
									0, 0, 0, 0, (void *)rawDataShmPtr) < 0) {

		   printf("Failed to write to frame datastream.\n");
	           H5Tclose(meta256_type_id);
                   H5Tclose(meta2048_type_id);
                   H5Pclose(file_creation_properties);
                   H5Pclose(file_access_properties);
                   H5Fclose(file_id);
 	           exit (-1);
		   }

                gettimeofday(&end_time,(struct timeval*)0);

                raw_stream_wtime[i] += (end_time.tv_sec-start_time.tv_sec)*1000000 +
                                       (end_time.tv_usec-start_time.tv_usec);

			/* Update the raw data shared memory pointer to the next shared memory location (and aligned to a page size boundary) */
			rawDataShmPtr += (meta_main[j].rows*meta_main[j].cols*meta_main[j].num_contiguous*sizeof(short));

			if (rawDataShmPtr >= rawDataShm + rawShmSize)
				rawDataShmPtr = rawDataShm;

                 gettimeofday(&start_time,(struct timeval*)0);
			/* Extend the 256 byte metadata 1 dataset */
			if( iokernelExtendH5Dataset(meta1Dataset[meta_main[j].datastream], frame_count[j] + meta_main[j].num_contiguous) < 0) {

				printf("Failed to extend meta 1 dataset.\n");
	        	        H5Tclose(meta256_type_id);
                                H5Tclose(meta2048_type_id);
                                H5Pclose(file_creation_properties);
                                H5Pclose(file_access_properties);
                                H5Fclose(file_id);
 	                        exit (-1);
			}

			/* Write some metadata 1 blocks to the metadata 1 dataset */
			if( iokernelWriteH5Data(meta1Dataset[meta_main[j].datastream], meta256_type_id, frame_count[j], meta_main[j].num_contiguous,
									(void *)metadataShmPtr) < 0) {

				printf("Failed to write to meta 1 dataset.\n");
			        H5Tclose(meta256_type_id);
                                H5Tclose(meta2048_type_id);
                                H5Pclose(file_creation_properties);
                                H5Pclose(file_access_properties);
                                H5Fclose(file_id);
 	                        exit (-1);
			}

			/* Update the metadata shared memory pointer to the next shared memory location (and aligned to a page size boundary) */
			metadataShmPtr += (meta_main[j].num_contiguous*sizeof(Metadata_256));

			if (metadataShmPtr >= metadataShm + metaShmSize)
				metadataShmPtr = metadataShm;


			/* Extend the 256 byte metadata 2 dataset */
			if( iokernelExtendH5Dataset(meta2Dataset[meta_main[j].datastream], frame_count[j] + meta_main[j].num_contiguous) < 0) {

				printf("Failed to extend meta 2 dataset.\n");
			        H5Tclose(meta256_type_id);
                                H5Tclose(meta2048_type_id);
                                H5Pclose(file_creation_properties);
                                H5Pclose(file_access_properties);
                                H5Fclose(file_id);
 	                        exit (-1);
			}

			/* Write some metadata 2 blocks to the metadata 1 dataset */
			if( iokernelWriteH5Data(meta2Dataset[meta_main[j].datastream], meta256_type_id, frame_count[j], meta_main[j].num_contiguous,
									(void *)metadataShmPtr) < 0) {

				printf("Failed to write to meta 2 dataset.\n");
				H5Tclose(meta256_type_id);
                                H5Tclose(meta2048_type_id);
                                H5Pclose(file_creation_properties);
                                H5Pclose(file_access_properties);
                                H5Fclose(file_id);
 	                        exit (-1);
			}

			/* Update the metadata shared memory pointer to the next shared memory location (and aligned to a page size boundary) */
			metadataShmPtr += (meta_main[j].num_contiguous*sizeof(Metadata_256));

			if (metadataShmPtr >= metadataShm + metaShmSize)
				metadataShmPtr = metadataShm;


			/* Extend the 2048 bytes metadata 3 dataset */
			if( iokernelExtendH5Dataset(meta3Dataset[meta_main[j].datastream], frame_count[j] + meta_main[j].num_contiguous) < 0) {

				printf("Failed to extend frame dataset.\n");
			        H5Tclose(meta256_type_id);
                                H5Tclose(meta2048_type_id);
                                H5Pclose(file_creation_properties);
                                H5Pclose(file_access_properties);
                                H5Fclose(file_id);
 	                        exit (-1);
			}

			/* Write some metadata 3 blocks to the metadata 1 dataset */
			if( iokernelWriteH5Data(meta3Dataset[meta_main[j].datastream], meta2048_type_id, frame_count[j], meta_main[j].num_contiguous,
									(void *)metadataShmPtr) < 0) {

				printf("Failed to write to meta 3 dataset.\n");
			        H5Tclose(meta256_type_id);
                                H5Tclose(meta2048_type_id);
                                H5Pclose(file_creation_properties);
                                H5Pclose(file_access_properties);
                                H5Fclose(file_id);
 	                        exit (-1);
			}

                       gettimeofday(&end_time,(struct timeval*)0);
                       meta_stream_wtime[i] += (end_time.tv_sec-start_time.tv_sec)*1000000 +
                                              (end_time.tv_usec-start_time.tv_usec);


			/* Update the metadata shared memory pointer to the next shared memory location (and aligned to a page size boundary) */
			metadataShmPtr += (meta_main[j].num_contiguous*sizeof(Metadata_2048));

			if (metadataShmPtr >= metadataShm + metaShmSize)
				metadataShmPtr = metadataShm;


			/* Update the frame count for this datastream */
			frame_count[j] += meta_main[j].num_contiguous;

		}		
	}

/* Obtain the time for writing the total data for each stream */

      for(i=0;i<numStreams;i++) {
         total_stream_wtime[i] = raw_stream_wtime[i] + meta_stream_wtime[i];
         printf("stream %d raw data writing time =%8.3f\n",i,((float)raw_stream_wtime[i])/1000000);
         printf("stream %d meta data writing time=%8.3f\n",i,((float)meta_stream_wtime[i])/1000000);
         printf("stream %d total data writing time=%8.3f\n",i,((float)total_stream_wtime[i])/1000000);
      }


	/**********************************************************************************
    * Close the HDF5 file and free allocated data
    ***********************************************************************************/
        if(H5Tclose(meta256_type_id)<0) {
           printf("Failed to close the datatype id.\n");
           exit(-1);
        }

        if(H5Tclose(meta2048_type_id)<0) {
           printf("Failed to close the datatype id.\n");
           exit(-1);
        }

        if(H5Pclose(file_creation_properties)<0) {
          printf("Failed to close the file creation property list.\n");
          exit(-1);
        }

        if(H5Pclose(file_access_properties)<0) {
          printf("Failed to close the file access property list.\n");
          exit(-1);
        }



	for (i=0; i<numStreams; i++) {

           printf("i= %d close rawdata\n",i);
	   if(H5Dclose(frameDatastream[meta_main[i].datastream])<0){
	     printf("Failed to close the raw data dataset id.\n");
	     exit(-1);
           }

           
           printf("i= %d close meta1data\n",i);
	   if(H5Dclose(meta1Dataset[meta_main[i].datastream])<0){
	     printf("Failed to close the raw data dataset id.\n");
	     exit(-1);
           }

           printf("i= %d close meta2data\n",i);
	   if(H5Dclose(meta2Dataset[meta_main[i].datastream])<0){
	     printf("Failed to close the raw data dataset id.\n");
	     exit(-1);
           }

           printf("i= %d close meta3data\n",i);
           printf("dataset id=%d\n",meta3Dataset[meta_main[i].datastream]);
	   if(H5Dclose(meta3Dataset[meta_main[i].datastream])<0){
	     printf("Failed to close the raw data dataset id.\n");
	     exit(-1);
           }


           printf("i= %d close groupdata\n",i);
	   if(H5Gclose(group_id[meta_main[i].datastream])<0){
	      printf("Failed to close the group id.\n");
              exit(-1);
           }
        
	}


	if ( H5Fclose(file_id) < 0 )
	{
	    printf("Failed to close the product file '%s'.", hdf5_name);
	    exit (-1);
	}


	/* Free memory */
	free(meta_main);
	free(metadataShm);
	free(rawDataShm);
        free(total_stream_wtime);
        free(meta_stream_wtime);
        free(raw_stream_wtime);

	return (0);

}

void usage() {

   printf("\n USAGE: \n\n");
   printf("The first argument is the HDF5 file name. \n");
   printf("The second argument is number of iteration.\n");
   printf("The third argument is the raw data memory size in MB.\n");
   printf("The fourth argument is the metadata memory size in MB. \n");
  
}
