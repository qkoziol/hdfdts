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

/* Definitions for test generator functions */

#define NUM_GROUPS 512 /* number of groups and recursion levels for linear
                        structure */

#define HEIGHT 5 /* height of group trees (recursion levels) */

/* types of group structures */
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
#define RANK 2     /* default rank */
#define SIZE 10    /* default size for all dimensions */

/* child nodes of a binary tree */
#define LEFT 0
#define RIGHT 1

#define STR_SIZE 12 /* default size for fixed-length strings */

/* numerical basic datatypes */
#define NTYPES 12
#define NATIVE_TYPE(i)  \
    (i==0)?H5T_NATIVE_CHAR: \
    (i==1)?H5T_NATIVE_SHORT:    \
    (i==2)?H5T_NATIVE_INT:  \
    (i==3)?H5T_NATIVE_UINT: \
    (i==4)?H5T_NATIVE_LONG: \
    (i==5)?H5T_NATIVE_LLONG:    \
    (i==6)?H5T_NATIVE_FLOAT:    \
    (i==7)?H5T_NATIVE_DOUBLE:   \
    (i==8)?H5T_NATIVE_LDOUBLE:    \
    (i==9)?H5T_NATIVE_B8:    \
    (i==10)?H5T_NATIVE_OPAQUE:    \
        string_type1    \

char *ntype_dset[]={"char","short","int","uint","long","llong",
    "float","double","ldouble", "bitfield","opaque","string"};


/* Definition for generation of files with compound and
   variable length datatypes */
typedef struct s1_t {
    unsigned int a;
    unsigned int b;
    unsigned int c[4];
    unsigned int d;
    unsigned int e;
} s1_t;


/* Definition for generation of file with enumerated
   datatype */
#define CPTR(VAR,CONST)	((VAR)=(CONST),&(VAR))

typedef enum {
    E1_RED,
    E1_GREEN,
    E1_BLUE,
    E1_WHITE,
    E1_BLACK
} c_e1;

#define NUM_VALUES 5 /* number of possible values in enumerated datatype */


/* Definition for generation of a compound datatype which will
   be pointed by a reference datatype */
typedef struct s2_t {
    unsigned int a;
    unsigned int b;
    float c;
} s2_t;


/* Macros for testing filters */
#define DSET_CONV_BUF_NAME	"conv_buf"
#define DSET_TCONV_NAME		"tconv"
#define DSET_DEFLATE_NAME	"deflate"
#define DSET_SZIP_NAME          "szip"
#define DSET_SHUFFLE_NAME	"shuffle"
#define DSET_FLETCHER32_NAME	"fletcher32"
#define DSET_FLETCHER32_NAME_2	"fletcher32_2"
#define DSET_FLETCHER32_NAME_3	"fletcher32_3"
#define DSET_SHUF_DEF_FLET_NAME	"shuffle+deflate+fletcher32"
#define DSET_SHUF_DEF_FLET_NAME_2	"shuffle+deflate+fletcher32_2"
#define DSET_SHUF_SZIP_FLET_NAME	"shuffle+szip+fletcher32"
#define DSET_SHUF_SZIP_FLET_NAME_2	"shuffle+szip+fletcher32_2"
#define DSET_BOGUS_NAME		"bogus"

/* Parameters for internal filter test */
#define CHUNKING_FACTOR 10

/* Temporary filter IDs used for testing */
#define H5Z_FILTER_BOGUS	305
#define H5Z_FILTER_CORRUPT	306
#define H5Z_FILTER_BOGUS2	307

#define H5_SZIP_NN_OPTION_MASK          32

unsigned szip_options_mask=H5_SZIP_NN_OPTION_MASK;
unsigned szip_pixels_per_block=4;

int nerrors=0;

/* Local prototypes for filter functions */
static size_t filter_bogus(unsigned int flags, size_t cd_nelmts,
const unsigned int *cd_values, size_t nbytes, size_t *buf_size, void **buf);


/* This message derives from H5Z */
const H5Z_class_t H5Z_BOGUS[1] = {{
    H5Z_FILTER_BOGUS,		/* Filter id number		*/
    "bogus",			/* Filter name for debugging	*/
    NULL,                       /* The "can apply" callback     */
    NULL,                       /* The "set local" callback     */
    filter_bogus,		/* The actual filter function	*/
}};



/*
* This function implements a bogus filter that just returns the value
* of an argument.
*/

static size_t filter_bogus(unsigned int UNUSED flags, size_t UNUSED cd_nelmts,
const unsigned int UNUSED *cd_values, size_t nbytes,size_t UNUSED *buf_size,
void UNUSED **buf)
{
    return nbytes;
}


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
    buffer = (int *)malloc(pow(SIZE,RANK)*sizeof(int));
    VRFY((buffer!=NULL), "malloc");

    for (i=0; i < pow(SIZE,RANK); i++)
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
    int i;

    VRFY((NUM_GROUPS > 0), "");

    /* create NUM_GROUPS at the root */
    for (i=0; i<NUM_GROUPS; i++){
        
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
* The option 'status' determines if data is to be written in the datasets.
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
            buffer = (int *)malloc(pow(size,rank)*sizeof(int));
            VRFY((buffer!=NULL), "malloc");

            for (i=0; i < pow(size,rank); i++)
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
* This function generates a dataset per basic datatype , i.e. one dataset is char,
* another one is int, and so on. The option 'fill_dataset' determines whether data is
* to be written into the dataset.
*/

static void gen_basic_types(hid_t oid, int fill_dataset)
{

    hid_t dset_id, dspace_id; /* HDF5 IDs */
    hid_t datatype, string_type1;
    hsize_t dims[RANK]; 
    herr_t ret;
    unsigned char *uchar_buffer, *string_buffer;
    float *float_buffer;

    long int i;


    VRFY((RANK>0), "RANK");
    VRFY((SIZE>0), "SIZE");

    /* define size for dataspace dimensions */
    for(i=0; i < RANK; i++)
        dims[i] = SIZE;

    /* create dataspace */
    dspace_id = H5Screate_simple(RANK, dims, NULL);
    VRFY((dspace_id>=0), "H5Screate_simple");

    /* allocate integer buffer and assign data */
    uchar_buffer = (unsigned char *)malloc(pow(SIZE,RANK)*sizeof(unsigned char));
    VRFY((uchar_buffer!=NULL), "malloc");
    
    /* allocate string buffer and assign data */
    string_buffer = (unsigned char *)malloc(STR_SIZE*pow(SIZE,RANK)*sizeof(unsigned char));
    VRFY((uchar_buffer!=NULL), "malloc");
    
    /* allocate float buffer and assign data */
    float_buffer = (float *)malloc(pow(SIZE,RANK)*sizeof(float));
    VRFY((float_buffer!=NULL), "malloc");

    /* Create a datatype to refer to */
    string_type1 = H5Tcopy (H5T_C_S1);
    VRFY((string_type1>=0), "H5Tcopy");

    ret = H5Tset_size (string_type1, STR_SIZE);
    VRFY((ret>=0), "H5Tset_size");

    /* fill buffer with data */
    for (i=0; i < pow(SIZE,RANK); i++){
        float_buffer[i] = uchar_buffer[i] = i%SIZE;
        strcpy(&string_buffer[i*STR_SIZE], "sample text");
    }

    /* iterate over basic datatypes */            
    for(i=0;i<NTYPES;i++){
 
        /* create dataset */
        dset_id = H5Dcreate(oid, ntype_dset[i], NATIVE_TYPE(i), dspace_id, H5P_DEFAULT);
        VRFY((dset_id>=0), "H5Dcreate");

        /* fill dataset with data */
        if (fill_dataset){
            /* since some datatype conversions are not allowed, select buffer with
            compatible datatype class */
            switch (H5Tget_class(NATIVE_TYPE(i))) {
            case H5T_INTEGER:
                ret = H5Dwrite(dset_id, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, uchar_buffer);
                break;
            case H5T_FLOAT:
                ret = H5Dwrite(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, float_buffer);
                break;
            case H5T_BITFIELD:
                ret = H5Dwrite(dset_id, H5T_NATIVE_B8, H5S_ALL, H5S_ALL, H5P_DEFAULT, uchar_buffer);
                break;
            case H5T_OPAQUE:
                ret = H5Dwrite(dset_id, H5T_NATIVE_OPAQUE, H5S_ALL, H5S_ALL, H5P_DEFAULT, uchar_buffer);
                break;
            case H5T_STRING:
                ret = H5Dwrite(dset_id, string_type1, H5S_ALL, H5S_ALL, H5P_DEFAULT, string_buffer);
                break;
            default:
                VRFY(FALSE, "Invalid datatype conversion");
            }
            VRFY((ret>=0), "H5Dwrite");
        }

        /* close dataset */
        ret = H5Dclose(dset_id);
        VRFY((ret>=0), "H5Dclose");
    }

    /* close dataspace */
    ret = H5Sclose(dspace_id);
    VRFY((ret>=0), "H5Sclose");

    free(uchar_buffer);
    free(float_buffer);
    free(string_buffer);
}



/*
*
* This function generates a dataset with a compound datatype. The option
* 'fill_dataset' determines whether data is to be written into the dataset.
*/

static void gen_compound(hid_t oid, int fill_dataset)
{
    s1_t *s1;                   /* compound struct */
    hid_t dset_id, dspace_id;   /* dataset and dataspace IDs */
    hid_t s1_tid, array_dt;     /* datatypes IDs */

    hsize_t dims[RANK];         /* dataspace dimension size */
    hsize_t memb_size[1] = {4}; /* array datatype dimension */
    herr_t ret;

    long int i;

    VRFY((RANK>0), "RANK");
    VRFY((SIZE>0), "SIZE");

    /* define size for dataspace dimensions */
    for(i=0; i < RANK; i++)
        dims[i] = SIZE;

    /* Create the data space */
    dspace_id = H5Screate_simple (RANK, dims, NULL);
    VRFY((dspace_id>=0),"H5Screate_simple");
    
    /* create compound datatype */
    s1_tid = H5Tcreate (H5T_COMPOUND, sizeof(s1_t));
    VRFY((s1_tid>=0), "H5Tcreate");

    /* compound datatype includes an array */
    array_dt=H5Tarray_create(H5T_NATIVE_INT, 1, memb_size, NULL);
    VRFY((array_dt>=0), "H5Tarray_create");
   
    /* define the fields of the compound datatype */
    if (H5Tinsert (s1_tid, "a", HOFFSET(s1_t,a), H5T_NATIVE_INT)<0 ||
        H5Tinsert (s1_tid, "b", HOFFSET(s1_t,b), H5T_NATIVE_INT)<0 ||
        H5Tinsert (s1_tid, "c", HOFFSET(s1_t,c), array_dt)<0 ||
        H5Tinsert (s1_tid, "d", HOFFSET(s1_t,d), H5T_NATIVE_INT)<0 ||
        H5Tinsert (s1_tid, "e", HOFFSET(s1_t,e), H5T_NATIVE_INT)<0)
        VRFY(FALSE, "H5Tinsert");

    /* close array datatype */
    ret = H5Tclose(array_dt);
    VRFY((ret>=0), "H5Tclose");

    /* Create the dataset */
    dset_id = H5Dcreate (oid, "compound1", s1_tid, dspace_id, H5P_DEFAULT);
    VRFY((dset_id>=0), "H5Dcreate");

    if (fill_dataset) {
        /* allocate buffer and assign data */
        s1 = (s1_t *)malloc(pow(SIZE,RANK)*sizeof(s1_t));
        VRFY((s1!=NULL), "malloc");
    
        /* Initialize the dataset */
        for (i=0; i<pow(SIZE,RANK); i++) {
	        s1[i].a = 8*i+0;
	        s1[i].b = 2000+2*i;
	        s1[i].c[0] = 8*i+2;
	        s1[i].c[1] = 8*i+3;
	        s1[i].c[2] = 8*i+4;
	        s1[i].c[3] = 8*i+5;
	        s1[i].d = 2001+2*i;
	        s1[i].e = 8*i+7;
        }

        /* Write the data */
        ret = H5Dwrite (dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, s1);
        VRFY((ret>=0), "H5Dwrite");

        free(s1);
    }

    /* close dataset */
    ret = H5Dclose(dset_id);
    VRFY((ret>=0), "H5Dclose");

    /* close dataspace */
    ret = H5Sclose(dspace_id);
    VRFY((ret>=0), "H5Sclose");
}



/*
*
* This function generates a dataset with a VL datatype. The option
* 'fill_dataset' determines whether data is to be written into the dataset.
*/

static void gen_vl(hid_t oid, int fill_dataset)
{
    hid_t dset_id, dspace_id;   /* dataset and dataspace IDs */
    hid_t s1_tid, array_dt;     /* datatypes IDs */

    hsize_t dims[RANK];         /* dataspace dimension size */
    hsize_t memb_size[1] = {4}; /* array datatype dimension */
    herr_t ret;

    hvl_t *wdata;   /* Information to write */
    hid_t tid1;         /* Datatype ID			*/
    hsize_t size;       /* Number of bytes which will be used */
    unsigned i,j;       /* counting variables */
    size_t mem_used=0;  /* Memory used during allocation */


    VRFY((RANK>0), "RANK");
    VRFY((SIZE>0), "SIZE");

    /* define size for dataspace dimensions */
    for(i=0; i < RANK; i++)
        dims[i] = SIZE;

    /* Create dataspace for datasets */
    dspace_id = H5Screate_simple(RANK, dims, NULL);
    VRFY((dspace_id>=0), "H5Screate_simple");

    /* Create a datatype to refer to */
    tid1 = H5Tvlen_create(H5T_NATIVE_UINT);
    VRFY((tid1>=0), "H5Tvlen_create");

    /* Create a dataset */
    dset_id = H5Dcreate(oid,"Dataset1", tid1, dspace_id, H5P_DEFAULT);
    VRFY((dset_id>=0), "H5Dcreate");

    if (fill_dataset) {
        /* allocate buffer */
        wdata = (hvl_t *)malloc(pow(SIZE,RANK)*sizeof(hvl_t));
        VRFY((wdata!=NULL), "malloc");

        /* initialize VL data to write */
        for (i=0; i<pow(SIZE,RANK); i++) {
            wdata[i].p=malloc((i+1)*sizeof(unsigned int));
            wdata[i].len=i+1;
            for(j=0; j<(i+1); j++)
                ((unsigned int *)wdata[i].p)[j]=i*10+j;
        } /* end for */

        /* write dataset */
        ret=H5Dwrite(dset_id,tid1,H5S_ALL,H5S_ALL,H5P_DEFAULT,wdata);
        VRFY((ret>=0), "H5Dwrite");

        free(wdata);
    }

    /* Close Dataset */
    ret = H5Dclose(dset_id);
    VRFY((ret>=0), "H5Dclose");

    /* Close datatype */
    ret = H5Tclose(tid1);
    VRFY((ret>=0), "H5Tclose");

    /* Close disk dataspace */
    ret = H5Sclose(dspace_id);
    VRFY((ret>=0), "H5Sclose");

}


/*
*
* This function generates a dataset with an enumerated datatype. The option
* 'fill_dataset' determines whether data is to be written into the dataset.
*/

static int gen_enum(hid_t oid, int fill_dataset)
{
    hid_t	type, dspace_id, dset_id, ret;  /* HDF5 IDs */
    c_e1 val;
    hsize_t dims[RANK]; /* dataspace dimension size */
    c_e1 *data1;        /* data buffer */
    hsize_t	i, j;

    VRFY((RANK>0), "RANK");
    VRFY((SIZE>0), "SIZE");

    /* define size for dataspace dimensions */
    for(i=0; i < RANK; i++)
        dims[i] = SIZE;

    /* create enumerated datatype */
    type = H5Tcreate(H5T_ENUM, sizeof(c_e1));
    VRFY((type>=0), "H5Tcreate");

    /* insertion of enumerated values */
    if ((H5Tenum_insert(type, "RED",   CPTR(val, E1_RED  ))<0) ||
        (H5Tenum_insert(type, "GREEN", CPTR(val, E1_GREEN))<0) ||
        (H5Tenum_insert(type, "BLUE",  CPTR(val, E1_BLUE ))<0) ||
        (H5Tenum_insert(type, "WHITE", CPTR(val, E1_WHITE))<0) ||
        (H5Tenum_insert(type, "BLACK", CPTR(val, E1_BLACK))<0))
        VRFY(FALSE, "H5Tenum_insert");

    /* create dataspace */
    dspace_id=H5Screate_simple(RANK, dims, NULL);
    VRFY((dspace_id>=0), "H5Screate_simple");

    /* create dataset */
    dset_id=H5Dcreate(oid, "color_table", type, dspace_id, H5P_DEFAULT);
    VRFY((dset_id>=0), "H5Dcreate");
   
    if (fill_dataset) {
        /* allocate buffer */
        data1 = (c_e1 *)malloc(pow(SIZE,RANK)*sizeof(c_e1));
        VRFY((data1!=NULL), "malloc");

        /* allocate and initialize enumerated data to write */
        for (i=0; i<pow(SIZE,RANK); i++) {
            j=i%NUM_VALUES;
            data1[i]=(j<1)?E1_RED:((j<2)?E1_GREEN:((j<3)?E1_BLUE:((j<4)?E1_WHITE:
            E1_BLACK)));
        } /* end for */

        /* write dataset */
        ret=H5Dwrite(dset_id, type, dspace_id, dspace_id, H5P_DEFAULT, data1);
        VRFY((ret>=0), "H5Dwrite");

        free(data1);
    }

    /* close dataset */
    ret=H5Dclose(dset_id);
    VRFY((ret>=0), "H5Dclose");

    /* close dataspace */
    ret=H5Sclose(dspace_id);
    VRFY((ret>=0), "H5Sclose");

    /* close datatype */
    ret=H5Tclose(type);
    VRFY((ret>=0), "H5Tclose");

}


/*
*
* This function generates 1 group with 2 datasets and a compound datatype.
* Then it creates a dataset containing references to these 4 objects.
*/

static void gen_reference(hid_t oid)
{
    hid_t		dset_id;	/* Dataset ID			*/
    hid_t		group;      /* Group ID             */
    hid_t		dspace_id;       /* Dataspace ID			*/
    hid_t		tid1;       /* Datatype ID			*/
    hsize_t		dims[RANK];
    hobj_ref_t      *wbuf;      /* buffer to write to disk */
    unsigned      *tu32;      /* Temporary pointer to uint32 data */
    int        i;          /* counting variables */
    const char *write_comment="Foo!"; /* Comments for group */
    herr_t		ret;		/* Generic return value		*/

    VRFY((RANK>0), "RANK");
    VRFY((SIZE>0), "SIZE");

    /* define size for dataspace dimensions */
    for(i=0; i < RANK; i++)
        dims[i] = SIZE;

    /* Allocate write & read buffers */
    wbuf=malloc(MAX(sizeof(unsigned),sizeof(hobj_ref_t))*pow(SIZE,RANK));

    /* Create dataspace for datasets */
    dspace_id = H5Screate_simple(RANK, dims, NULL);
    VRFY((dspace_id>=0), "H5Screate_simple");

    /* Create a group */
    group=H5Gcreate(oid,"Group1",(size_t)-1);
    VRFY((group>=0), "H5Gcreate");

    /* Set group's comment */
    ret=H5Gset_comment(group,".",write_comment);
    VRFY((ret>=0), "H5Gset_comment");

    /* Create a dataset (inside Group1) */
    dset_id=H5Dcreate(group,"Dataset1",H5T_NATIVE_UINT,dspace_id,H5P_DEFAULT);
    VRFY((dset_id>=0), "H5Dcreate");

    for(tu32=(unsigned *)wbuf,i=0; i<pow(SIZE,RANK); i++)
        *tu32++=i*3;

    /* Write selection to disk */
    ret=H5Dwrite(dset_id,H5T_NATIVE_UINT,H5S_ALL,H5S_ALL,H5P_DEFAULT,wbuf);
    VRFY((ret>=0), "H5Dwrite");

    /* Close Dataset */
    ret = H5Dclose(dset_id);
    VRFY((ret>=0), "H5Dclose");

    /* Create another dataset (inside Group1) */
    dset_id=H5Dcreate(group,"Dataset2",H5T_NATIVE_UCHAR,dspace_id,H5P_DEFAULT);
    VRFY((dset_id>=0), "H5Dcreate");

    /* Close Dataset */
    ret = H5Dclose(dset_id);
    VRFY((ret>=0), "H5Dclose");

    /* Create a datatype to refer to */
    tid1 = H5Tcreate (H5T_COMPOUND, sizeof(s2_t));
    VRFY((tid1>=0), "H5Tcreate");

    /* Insert fields */
    ret=H5Tinsert (tid1, "a", HOFFSET(s2_t,a), H5T_NATIVE_INT);
    VRFY((ret>=0), "H5Tinsert");

    ret=H5Tinsert (tid1, "b", HOFFSET(s2_t,b), H5T_NATIVE_INT);
    VRFY((ret>=0), "H5Tinsert");

    ret=H5Tinsert (tid1, "c", HOFFSET(s2_t,c), H5T_NATIVE_FLOAT);
    VRFY((ret>=0), "H5Tinsert");

    /* Save datatype for later */
    ret=H5Tcommit (group, "Datatype1", tid1);
    VRFY((ret>=0), "H5Tcommit");

    /* Close datatype */
    ret = H5Tclose(tid1);
    VRFY((ret>=0), "H5Tclose");

    /* Close group */
    ret = H5Gclose(group);
    VRFY((ret>=0), "H5Gclose");

    /* Create a dataset */
    dset_id=H5Dcreate(oid,"Dataset3",H5T_STD_REF_OBJ,dspace_id,H5P_DEFAULT);
    VRFY((dset_id>=0), "H5Dcreate");
    
    for (i=0;i<((int)pow(SIZE,RANK))/4;i++){
        /* Create reference to dataset */
        ret = H5Rcreate(&wbuf[i*4+0],oid,"/Group1/Dataset1",H5R_OBJECT,-1);
        VRFY((ret>=0), "H5Rcreate");

        /* Create reference to dataset */
        ret = H5Rcreate(&wbuf[i*4+1],oid,"/Group1/Dataset2",H5R_OBJECT,-1);
        VRFY((ret>=0),"H5Rcreate");

        /* Create reference to group */
        ret = H5Rcreate(&wbuf[i*4+2],oid,"/Group1",H5R_OBJECT,-1);
        VRFY((ret>=0), "H5Rcreate");

        /* Create reference to named datatype */
        ret = H5Rcreate(&wbuf[i*4+3],oid,"/Group1/Datatype1",H5R_OBJECT,-1);
        VRFY((ret>=0), "H5Rcreate");
    }

    /* Write selection to disk */
    ret=H5Dwrite(dset_id,H5T_STD_REF_OBJ,H5S_ALL,H5S_ALL,H5P_DEFAULT,wbuf);
    VRFY((ret>=0), "H5Dwrite");

    /* Close disk dataspace */
    ret = H5Sclose(dspace_id);
    VRFY((ret>=0), "H5Sclose");

    /* Close Dataset */
    ret = H5Dclose(dset_id);
    VRFY((ret>=0), "H5Dclose");

    /* Free memory buffers */
    free(wbuf);

}   /* test_reference_obj() */



/*
* This function is called by test_filters in order to create a dataset with
* the respective filters.
*/

static void test_filter_internal(hid_t fid, const char *name, hid_t dcpl, hsize_t *dset_size)
{
    hid_t		dataset;        /* Dataset ID */
    hid_t		dxpl;           /* Dataset xfer property list ID */
    hid_t		write_dxpl;     /* Dataset xfer property list ID for writing */
    hid_t		sid;            /* Dataspace ID */
    hsize_t	size[RANK];           /* Dataspace dimensions */
    int *points;
    void		*tconv_buf = NULL;      /* Temporary conversion buffer */
    hsize_t		i, j, n;        /* Local index variables */
    herr_t              ret;         /* Error status */


    /* define size for dataspace dimensions */
    for(i=0; i < RANK; i++)
        size[i] = SIZE+szip_pixels_per_block;

    /* Create the data space */
    sid = H5Screate_simple(RANK, size, NULL);
    VRFY((sid>=0), "H5Screate_simple");

    /*
     * Create a small conversion buffer to test strip mining. We
     * might as well test all we can!
     */
    dxpl = H5Pcreate (H5P_DATASET_XFER);
    VRFY((dxpl>=0), "H5Pcreate");

    tconv_buf = malloc (1000);
#ifdef H5_WANT_H5_V1_4_COMPAT
    ret=H5Pset_buffer (dxpl, (hsize_t)1000, tconv_buf, NULL);
#else /* H5_WANT_H5_V1_4_COMPAT */
    ret=H5Pset_buffer (dxpl, (size_t)1000, tconv_buf, NULL);
#endif /* H5_WANT_H5_V1_4_COMPAT */
    VRFY((ret>=0), "H5Pset_buffer");

    write_dxpl = H5Pcopy(dxpl);
    VRFY((write_dxpl>=0), "H5Screate_simple");

    /* Check if all the filters are available */
    VRFY((H5Pall_filters_avail(dcpl)==TRUE), "Incorrect filter availability");

    /* Create the dataset */
    dataset = H5Dcreate(fid, name, H5T_NATIVE_INT, sid, dcpl);
    VRFY((dataset>=0), "H5Dcreate");

    /*----------------------------------------------------------------------
     * Test filters by setting up a chunked dataset and writing
     * to it.
     *----------------------------------------------------------------------
     */

    points = (int *)malloc(pow(SIZE,RANK)*sizeof(int));
    VRFY((points!=NULL), "malloc");

    for (i=0; i < pow(SIZE,RANK); i++)
        points[i]= (int)rand();

    ret=H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, write_dxpl, points);
    VRFY((ret>=0), "H5Dwrite");
	
    *dset_size=H5Dget_storage_size(dataset);
    VRFY((*dset_size!=0), "H5Dget_storage_size");

    ret=H5Dclose(dataset);
    VRFY((ret>=0), "H5Dclose");

    ret=H5Sclose(sid);
    VRFY((ret>=0), "H5Sclose");

    ret=H5Pclose (dxpl);
    VRFY((ret>=0), "H5Pclose");

    free(points);
    free (tconv_buf);

}



/*
* This function generates several datasets with different filters. Filters are used
* individually and in combinations. The combination of filters in different order is
* also tested.
*/

static void test_filters(hid_t file)
{
    hid_t	dc;                 /* Dataset creation property list ID */
    hsize_t chunk_size[RANK]; /* Chunk dimensions */
    hsize_t     null_size;          /* Size of dataset with null filter */

    hsize_t     fletcher32_size;       /* Size of dataset with Fletcher32 checksum */

    hsize_t     deflate_size;       /* Size of dataset with deflate filter */

    hsize_t     szip_size;       /* Size of dataset with szip filter */


    hsize_t     shuffle_size;       /* Size of dataset with shuffle filter */

    hsize_t     combo_size;     /* Size of dataset with shuffle+deflate filter */

    herr_t ret;

    int i,j;

    VRFY((RANK>0), "RANK");
    VRFY((SIZE>0), "SIZE");

    /* define size for dataspace dimensions */
    for(i=0; i < RANK; i++)
        chunk_size[i] = SIZE/CHUNKING_FACTOR+szip_pixels_per_block;

    /*----------------------------------------------------------
     * STEP 0: Test null I/O filter by itself.
     *----------------------------------------------------------
     */
    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk(dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

#ifdef H5_WANT_H5_V1_4_COMPAT
    ret=H5Zregister (H5Z_FILTER_BOGUS, "bogus", filter_bogus);
#else /* H5_WANT_H5_V1_4_COMPAT */
    ret=H5Zregister (H5Z_BOGUS);
#endif /* H5_WANT_H5_V1_4_COMPAT */
    VRFY((ret>=0), "H5Zregister");

    ret=H5Pset_filter (dc, H5Z_FILTER_BOGUS, 0, 0, NULL);
    VRFY((ret>=0), "H5Pset_filter");

    test_filter_internal(file,DSET_BOGUS_NAME,dc,&null_size);

    /* Clean up objects used for this test */
    ret=H5Pclose(dc);
    VRFY((ret>=0), "H5Pclose");


    /*----------------------------------------------------------
     * STEP 1: Test Fletcher32 Checksum by itself.
     *----------------------------------------------------------
     */
    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk(dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

    ret=H5Pset_filter(dc,H5Z_FILTER_FLETCHER32,0,0,NULL);
    VRFY((ret>=0), "H5Pset_filter");

    /* Enable checksum during read */
    test_filter_internal(file,DSET_FLETCHER32_NAME,dc,&fletcher32_size);
    
    VRFY((fletcher32_size>null_size), "size after checksumming is incorrect.");

    /* Clean up objects used for this test */
    ret=H5Pclose(dc);
    VRFY((ret>=0), "H5Pclose");

    /*----------------------------------------------------------
     * STEP 2: Test deflation by itself.
     *----------------------------------------------------------
     */
    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk(dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

    ret=H5Pset_deflate(dc, 6);
    VRFY((ret>=0), "H5Pset_deflate");

    test_filter_internal(file,DSET_DEFLATE_NAME,dc,&deflate_size);

    /* Clean up objects used for this test */
    ret=H5Pclose(dc);
    VRFY((ret>=0), "H5Pclose");
    
    /*----------------------------------------------------------
     * STEP 3: Test szip compression by itself.
     *----------------------------------------------------------
     */

	/* with encoder */
    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk(dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

	ret=H5Pset_szip(dc, szip_options_mask, szip_pixels_per_block);
    VRFY((ret>=0), "H5Pset_szip");

	test_filter_internal(file,DSET_SZIP_NAME,dc,&szip_size);

    ret=H5Pclose(dc);
    VRFY((ret>=0), "H5Pclose");
    	
    /*----------------------------------------------------------
     * STEP 4: Test shuffling by itself.
     *----------------------------------------------------------
     */
    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk(dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

    ret=H5Pset_shuffle(dc);
    VRFY((ret>=0), "H5Pset_shuffle");

    test_filter_internal(file,DSET_SHUFFLE_NAME,dc,&shuffle_size);

    VRFY((shuffle_size==null_size), "Shuffled size not the same as uncompressed size.");

    /* Clean up objects used for this test */
    ret=H5Pclose (dc);
    VRFY((dc>=0), "H5Pclose");

    /*----------------------------------------------------------
     * STEP 5: Test shuffle + deflate + checksum in any order.
     *----------------------------------------------------------
     */
    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk (dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

    ret=H5Pset_fletcher32(dc);
    VRFY((ret>=0), "H5Pset_fletcher32");

    ret=H5Pset_shuffle(dc);
    VRFY((ret>=0), "H5Pset_shuffle");

    ret=H5Pset_deflate (dc, 6);
    VRFY((ret>=0), "H5Pset_deflate");

    test_filter_internal(file,DSET_SHUF_DEF_FLET_NAME,dc,&combo_size);

    /* Clean up objects used for this test */
    ret=H5Pclose(dc);
    VRFY((ret>=0), "H5Pclose");

    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk (dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

    ret=H5Pset_shuffle(dc);
    VRFY((ret>=0), "H5Pset_shuffle");

    ret=H5Pset_deflate (dc, 6);
    VRFY((ret>=0), "H5Pcreate");

    ret=H5Pset_fletcher32(dc);
    VRFY((ret>=0), "H5Pcreate");

    test_filter_internal(file,DSET_SHUF_DEF_FLET_NAME_2,dc,&combo_size);

    /* Clean up objects used for this test */
    ret=H5Pclose (dc);
    VRFY((ret>=0), "H5Pclose");

    /*----------------------------------------------------------
     * STEP 6: Test shuffle + szip + checksum in any order.
     *----------------------------------------------------------
     */

    dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

    ret=H5Pset_chunk (dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

    ret=H5Pset_fletcher32(dc);
    VRFY((ret>=0), "H5Pset_fletcher");

    ret=H5Pset_shuffle(dc);
    VRFY((ret>=0), "H5Pset_shuffle");

	/* Make sure encoding is enabled */
	ret=H5Pset_szip(dc, szip_options_mask, szip_pixels_per_block);
    VRFY((ret>=0), "H5Pset_szip");

	test_filter_internal(file,DSET_SHUF_SZIP_FLET_NAME,dc,&combo_size);

    /* Clean up objects used for this test */
    ret=H5Pclose(dc);
    VRFY((ret>=0), "H5Pclose");

    /* Make sure encoding is enabled */
	dc = H5Pcreate(H5P_DATASET_CREATE);
    VRFY((dc>=0), "H5Pcreate");

	ret=H5Pset_chunk (dc, RANK, chunk_size);
    VRFY((ret>=0), "H5Pset_chunk");

	ret=H5Pset_shuffle(dc);
    VRFY((ret>=0), "H5Pset_shuffle");

	ret=H5Pset_szip(dc, szip_options_mask, szip_pixels_per_block);
    VRFY((ret>=0), "H5Pset_szip");

	ret=H5Pset_fletcher32(dc);
    VRFY((ret>=0), "H5Pset_fletcher32");

    test_filter_internal(file,DSET_SHUF_SZIP_FLET_NAME_2,dc,&combo_size);

	/* Clean up objects used for this test */
	ret=H5Pclose(dc);
    VRFY((ret>=0), "H5Pclose");
}




/* main function of test generator */

int main(int argc, char *argv[])
{

    hid_t fid;

    char *fname[]={"root.h5","linear.h5","hierarchical.h5","multipath.h5","cyclical.h5",
        "rank_dsets_empty.h5","rank_dsets_full.h5","group_dsets.h5","basic_types.h5",
        "compound.h5","vl.h5","enum.h5","refer.h5","filters.h5"}; /* file names */

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

    /* creates a file with datasets using different
    basic datatypes */
    fid = create_file(fname[i++], "");
    gen_basic_types(fid, FULL);
    close_file(fid, "");

    /* create a file with a dataset using a compound datatype */
    fid = create_file(fname[i++], "");
    gen_compound(fid, FULL);
    close_file(fid, "");

    /* create a file with a dataset using a VL datatype */
    fid = create_file(fname[i++], "");
    gen_vl(fid, FULL);
    close_file(fid, "");

    /* create a file with a dataset using an enumerated datatype */
    fid = create_file(fname[i++], "");
    gen_enum(fid, FULL);
    close_file(fid, "");
    
    /* create a file with a dataset using reference datatype */
    fid = create_file(fname[i++], "");
    gen_reference(fid);
    close_file(fid, "");

    /* create a file with several datasets using different filters */
    fid = create_file(fname[i++], "");
    test_filters(fid);
    close_file(fid, "");
    
    /* successful completion message */
    printf("\rTest files generation for H5check successful!\n");

    return 0;

}                               /* end main() */
