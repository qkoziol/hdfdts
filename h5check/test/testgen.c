/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
   FILE
   testgen.c - HDF5 test generator main file for h5check

 */

#include "testgen.h"

/* macro definitions for test generator functions */

#define NUM_GROUPS 512 /* number of groups for linear structure */

#define HEIGHT 2 /* height of group trees (recursion levels) */

/* type of group structure */
#define HIERARCHICAL 0
#define MULTIPATH 1
#define CYCLICAL 2

/* name prefixes */
#define GROUP_PREFIX "group"
#define DATASET_PREFIX "dataset"

/* dataset fill status */
#define EMPTY 0
#define PARTIAL 1
#define FULL 2

/* dataspace properties */
#define RANK 2      /* default rank */
#define SIZE 10     /* default dimension size */

/* child nodes of a binary tree */
#define LEFT 0
#define RIGHT 1

/* numerical basic types */
#define NTYPES 9
#define NATIVE_TYPE(i)  \
    (i==0)?H5T_NATIVE_CHAR: \
    (i==1)?H5T_NATIVE_SHORT:    \
    (i==2)?H5T_NATIVE_INT:  \
    (i==3)?H5T_NATIVE_UINT: \
    (i==4)?H5T_NATIVE_LONG: \
    (i==5)?H5T_NATIVE_LLONG:    \
    (i==6)?H5T_NATIVE_FLOAT:    \
    (i==7)?H5T_NATIVE_DOUBLE:   \
        H5T_NATIVE_LDOUBLE

char *ntype_dset[]={"char","short","int","uint","long","llong",
    "float","double","ldouble"};

int nerrors=0;



/*
* This function creates a file using default properties.
*/

static hid_t create_file(char *fname, char *options)
{
    hid_t fid;

    VRFY(isalnum(*fname),"filename");

    /* create a file */  
    fid = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    VRFY((fid>=0), "H5Fcreate");

    return fid;

}



/*
* This function closes a file.
*/

static void close_file(hid_t fid, char *options)
{
    herr_t ret;

    /* close file */
    ret = H5Fclose(fid);
    VRFY((ret>=0), "H5Fclose");

}



/*
* This fuction generates several (empty) group structures according to option:
* HIERARCHICAL creates a binary tree,
* MULTIPATH creates a graph in which some groups at the same level can be
* reached through multiple paths,
* CYCLICAL  creates a graph in which some groups point to their parents or
* grandparents. For one group, a pointer to the root causes the group to point
* to itself (this may or may not be the intended behavior of HDF5).
* All pointers are implemented as HARDLINKS.
*/

static void gen_group_struct(hid_t parent_id, char* prefix, int height, int option)
{
    hid_t child_gid0, child_gid1; /* groups IDs */
    herr_t ret;

    char gname0[64]; /* groups names */
    char gname1[64];
    char child_name[64];

    /* create child group 0 */
    sprintf(gname0, "%s_%d", prefix, 0);
    child_gid0 = H5Gcreate(parent_id, gname0, 0);
    VRFY((child_gid0 >= 0), "H5Gcreate");

    /* create child group 1 */
    sprintf(gname1, "%s_%d", prefix, 1);
    child_gid1 = H5Gcreate(parent_id, gname1, 0);
    VRFY((child_gid1 >= 0), "H5Gcreate");

    /* recursive calls for remaining levels */        
    if( height > 1 ){


        /* create subtree at child 0 */
        gen_group_struct(child_gid0, gname0, height-1, option);

        /* create subtree at child 1 */
        gen_group_struct(child_gid1, gname1, height-1, option);

        switch(option){

        case HIERARCHICAL: /* binary tree already constructed */
            break;

        case MULTIPATH:
            /* a third entry in child_gid1 points to a child of child_gid_0*/
            ret = H5Glink2(child_gid0, strcat(gname0,"_0"), H5G_LINK_HARD, child_gid1, strcat(gname1, "_2"));
            VRFY((ret>=0), "H5Glink2");
            break;

        case CYCLICAL:
               
            /* a third entry in child_gid1 points to its grandparent. However, if
            grandparent is the root, the entry will point to child_gid1. */
            sprintf(child_name, "%s_%d", gname1, 2);
            ret = H5Glink2(parent_id, ".", H5G_LINK_HARD, child_gid1, child_name);
            VRFY((ret>=0), "H5Glink2");

            /* a fourth entry in child_gid1 points its parent */
            sprintf(child_name, "%s_%d", gname1, 3);
            ret = H5Glink2(child_gid1, ".", H5G_LINK_HARD, child_gid1, child_name);
            VRFY((ret>=0), "H5Glink2");
            break;
               
        default:
            
            VRFY(FALSE, "Invalid option");
        }
    }

    /* close groups */
    ret = H5Gclose(child_gid0);
    VRFY((ret>=0), "H5Gclose");

    ret = H5Gclose(child_gid1);
    VRFY((ret>=0), "H5Gclose");
}



/*
* This function generates a binary tree group structure. One of the child groups
* will contain a dataset while the other is empty based on the argument 'data_child'.
* The dataset placement criteria reverses at every level of the tree.
*/

static void gen_group_datasets(hid_t parent_id, char* prefix, int height, int data_child)
{

    hid_t child_gid0, child_gid1, gid; /* groups IDs */
    hid_t dset_id, dspace_id;
    hsize_t dims[RANK];
    herr_t ret;

    char gname0[64]; /* groups names */
    char gname1[64];
    char child_name[64];

    int *buffer;
    long int i;

    VRFY((RANK>0), "RANK");
    VRFY((SIZE>0), "SIZE");

    /* create left child group */
    sprintf(gname0, "%s_%d", prefix, 0);
    child_gid0 = H5Gcreate(parent_id, gname0, 0);
    VRFY((child_gid0 >= 0), "H5Gcreate");

    /* create right child group */
    sprintf(gname1, "%s_%d", prefix, 1);
    child_gid1 = H5Gcreate(parent_id, gname1, 0);
    VRFY((child_gid1 >= 0), "H5Gcreate");

    /* selection of the group for placement of dataset */
    gid = data_child ? child_gid1 : child_gid0;

    for(i=0; i < RANK; i++)
        dims[i] = SIZE;

    /* create dataspace */
    dspace_id = H5Screate_simple(RANK, dims, NULL);
    VRFY((dspace_id>=0), "H5Screate_simple");

    /* create dataset */
    dset_id = H5Dcreate(gid, DATASET_PREFIX, H5T_NATIVE_INT, dspace_id, H5P_DEFAULT);
    VRFY((dset_id>=0), "H5Dcreate");

    /* allocate buffer and assign data */
    buffer = (int *)malloc(powl(SIZE,RANK)*sizeof(int));
    VRFY((buffer!=NULL), "malloc");

    for (i=0; i < powl(SIZE,RANK); i++)
        buffer[i] = i%SIZE;

    /* write buffer into dataset */
    ret = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
    VRFY((ret>=0), "H5Dwrite");

    free(buffer);

    /* close dataset */
    ret = H5Dclose(dset_id);
    VRFY((ret>=0), "H5Dclose");

    /* close dataspace */
    ret = H5Sclose(dspace_id);
    VRFY((ret>=0), "H5Sclose");

    /* recursive calls for remaining levels */        
    if( height > 1 ){

        /* dataset placement criteria reversed */
        /* create subtree at child 0 */
        gen_group_datasets(child_gid0, gname0, height-1, !data_child);

        /* create subtree at child 1 */
        gen_group_datasets(child_gid1, gname1, height-1, data_child);
    
    }

    /* close groups */
   ret = H5Gclose(child_gid0);
   VRFY((ret>=0), "H5Gclose");

   ret = H5Gclose(child_gid1);
   VRFY((ret>=0), "H5Gclose");
}



/*
* This function generates a linear nesting of NUM_GROUPS within one group.
* The generated structure tests the format validation of a large number of groups.
*/

static void gen_linear_rec(hid_t parent_id, char *prefix, unsigned int level)
{
    hid_t child_gid; /* group ID */
    herr_t ret;

    char gname[64]; /* group name */

    /* appending level to the name prefix */
    sprintf(gname, "%s_%d", prefix, level);

    /* create new group within parent_id group */
    child_gid = H5Gcreate(parent_id, gname, 0);
    VRFY((child_gid>=0), "H5Gcreate");

    /* recursive call for the remaining levels */        
    if( level > 1 )
        gen_linear_rec(child_gid, prefix, level-1);

    /* close group */
    ret = H5Gclose(child_gid);
    VRFY((ret>=0), "H5Gclose");
}



/*
* This function generates NUM_GROUPS empty groups at the root and a linear
* nesting of NUM_GROUPS within one group. The generated structure tests
* the format validation of a large number of groups.
*/

static void gen_linear(hid_t fid)
{

    hid_t gid; /* group ID */
    herr_t ret;
    char gname[64]; /* group name */

    VRFY((NUM_GROUPS > 0), "");

    /* create NUM_GROUPS at the root */
    for (int i=0; i<NUM_GROUPS; i++){
        
        /* append index to a name prefix */
        sprintf(gname, "group%d", i);

        /* create a group */
        gid = H5Gcreate(fid, gname, 0);
        VRFY((gid>=0), "H5Gcreate");

        /* close group*/
        ret = H5Gclose(gid);
        VRFY((ret>=0), "H5Gclose");
    }

    /* create a linear nesting of NUM_GROUPS within
    one group */
    gen_linear_rec(fid, "rec_group", NUM_GROUPS);

}



/*
*
* This function generates a dataset per allowed dimensionality, i.e. one
* dataset is 1D, another one is 2D, and so on, up to dimension H5S_MAX_RANK.
* The option 'status' determines if data is to be written in the file.
*/

static void gen_rank_datasets(hid_t oid, int fill_dataset)
{

    hid_t dset_id, dspace_id; /* HDF5 IDs */
    hsize_t dims[H5S_MAX_RANK]; /* H5S_MAX_RANK is the maximum rank allowed
                                 in the HDF5 library */
    herr_t ret;                 

    int *buffer;    /* buffer for writing the dataset */
    int size;       /* size for all the dimensions of the current rank */

    long int rank, i;

    char dname[64]; /* dataset name */

    /* iterates over all possible ranks (zero rank is not considered) */
    for (rank=1; rank<=H5S_MAX_RANK; rank++){

        /* all the dimensions for a particular rank have the same size */
        size=(rank<=10)?4:((rank<=20)?2:1);

        for(i=0; i < rank; i++)
            dims[i] = size;

        /* generate name for current dataset */
        sprintf(dname, "%s_%d", DATASET_PREFIX, rank);

        /* create dataspace */
        dspace_id = H5Screate_simple(rank, dims, NULL);
        VRFY((dspace_id>=0), "H5Screate_simple");

        /* create dataset */
        dset_id = H5Dcreate(oid, dname, H5T_NATIVE_INT, dspace_id, H5P_DEFAULT);
        VRFY((dset_id>=0), "H5Dcreate");
        
        /* dataset is to be full */
        if (fill_dataset){

            /* allocate buffer and assigns data */
            buffer = (int *)malloc(powl(size,rank)*sizeof(int));
            VRFY((buffer!=NULL), "malloc");

            for (i=0; i < powl(size,rank); i++)
                buffer[i] = i%size;

            /* writes buffer into dataset */
            ret = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
            VRFY((ret>=0), "H5Dwrite");

            free(buffer);
        }

        /* close dataset */
        ret = H5Dclose(dset_id);
        VRFY((ret>=0), "H5Dclose");

        /* close dataspace */
        ret = H5Sclose(dspace_id);
        VRFY((ret>=0), "H5Sclose");

    }
}



/*
*
* This function generates a dataset per numerical datatype , i.e. one
* dataset is char, another one is int, and so on. The option 'fill_dataset' determines
* if data is to be written in the file.
*/

static void gen_basic_ntypes(hid_t oid, int fill_dataset)
{

    hid_t dset_id, dspace_id; /* HDF5 IDs */
    hid_t datatype;
    hsize_t dims[RANK]; 
    herr_t ret;
    int *int_buffer;
    float *float_buffer;

    long int i;

    for(i=0; i < RANK; i++)
        dims[i] = SIZE;

    /* create dataspace */
    dspace_id = H5Screate_simple(RANK, dims, NULL);
    VRFY((dspace_id>=0), "H5Screate_simple");

    /* allocate integer buffer and assigns data */
    int_buffer = (int *)malloc(powl(SIZE,RANK)*sizeof(int));
    VRFY((int_buffer!=NULL), "malloc");
    
    /* allocate float buffer and assigns data */
    float_buffer = (float *)malloc(powl(SIZE,RANK)*sizeof(float));
    VRFY((float_buffer!=NULL), "malloc");

    /* fill buffer with data */
    for (i=0; i < powl(SIZE,RANK); i++)
        float_buffer[i] = int_buffer[i] = i%SIZE;

    /* iterate over numerical basic datatypes */            
    for(i=0;i<NTYPES;i++){
 
        /* create dataset */
        dset_id = H5Dcreate(oid, ntype_dset[i], NATIVE_TYPE(i), dspace_id, H5P_DEFAULT);
        VRFY((dset_id>=0), "H5Dcreate");

        /* fill dataset with data */
        if (fill_dataset){
            /* since some datatype conversions are not allowed, select buffer with
            compatible datatype class */
            if(H5Tget_class(NATIVE_TYPE(i)) == H5T_INTEGER)
                /* writes buffer into dataset */
                ret = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, int_buffer);
            else
                /* writes buffer into dataset */
                ret = H5Dwrite(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, float_buffer);

            VRFY((ret>=0), "H5Dwrite");
        }

        /* close dataset */
        ret = H5Dclose(dset_id);
        VRFY((ret>=0), "H5Dclose");
    }

    /* close dataspace */
    ret = H5Sclose(dspace_id);
    VRFY((ret>=0), "H5Sclose");
}



/* main function of test generator */

int main(int argc, char *argv[])
{

    hid_t fid;

    char *fname[]={"root.h5","linear.h5","hierarchical.h5","multipath.h5","cyclical.h5",
        "rank_dsets_empty.h5","rank_dsets_full.h5","group_dsets.h5","basic_ntypes.h5"}; /* file names */

    unsigned i=0;

    /* initial message */
    printf("Generating test files for H5check...");
    fflush(stdout);
    
    /* root group is created along with the file */
    fid = create_file(fname[i++], "");
    close_file(fid, "");

    /* create a linear group structure */
    fid = create_file(fname[i++], "");
    gen_linear(fid);
    close_file(fid, "");

    /* create a treelike structure */
    fid = create_file(fname[i++], "");
    gen_group_struct(fid, GROUP_PREFIX, HEIGHT, HIERARCHICAL);
    close_file(fid, "");

    /* create a multipath structure */
    fid = create_file(fname[i++], "");
    gen_group_struct(fid, GROUP_PREFIX, HEIGHT, MULTIPATH);
    close_file(fid, "");
    
    /* create a cyclical structure */
    fid = create_file(fname[i++], "");
    gen_group_struct(fid, GROUP_PREFIX, HEIGHT, CYCLICAL);
    close_file(fid, "");

    /* create an empty dataset for each possible rank */
    fid = create_file(fname[i++], "");
    gen_rank_datasets(fid, EMPTY);
    close_file(fid, "");

    /* create a full dataset for each possible rank */
    fid = create_file(fname[i++], "");
    gen_rank_datasets(fid, FULL);
    close_file(fid, "");

    /* create a tree like structure where some groups are empty
    while others contain a dataset */
    fid = create_file(fname[i++], "");
    gen_group_datasets(fid, GROUP_PREFIX, HEIGHT, RIGHT);
    close_file(fid, "");

    fid = create_file(fname[i++], "");
    gen_basic_ntypes(fid, FULL);
    close_file(fid, "");

    
    /* successful completion message */
    printf("\rTest files generation for H5check successful!\n");

    return 0;

}                               /* end main() */
