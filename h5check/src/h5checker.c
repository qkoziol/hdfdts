#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "h5_check.h"


H5F_shared_t	shared_info;
H5G_entry_t 	root_ent;
table_t		*obj_table;

size_t strlen(const char *);
char *strcpy(char *, const char *);
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
char *strerror(int);


void H5E_push(const char *, const char *, haddr_t );
herr_t H5E_clear(void);
void H5E_print(FILE *);


herr_t  check_superblock(FILE *, H5F_shared_t *);
herr_t 	check_obj_header(H5FD_t *, H5F_shared_t, haddr_t, int, const H5O_class_t *);
herr_t 	check_btree(H5FD_t *, H5F_shared_t, haddr_t, unsigned);
herr_t 	check_sym(H5FD_t *, H5F_shared_t, haddr_t);
herr_t 	check_lheap(H5FD_t *, H5F_shared_t, haddr_t, uint8_t **);
herr_t 	check_gheap(H5FD_t *, H5F_shared_t, haddr_t, uint8_t **);


haddr_t locate_super_signature(FILE *);
void H5F_addr_decode(H5F_shared_t, const uint8_t **, haddr_t *);
herr_t H5G_ent_decode(H5F_shared_t, const uint8_t **, H5G_entry_t *);
herr_t H5G_ent_decode_vec(H5F_shared_t, const uint8_t **, H5G_entry_t *, unsigned);
H5T_t *H5T_alloc(void);

static unsigned H5O_find_in_ohdr(H5FD_t *, H5O_t *, const H5O_class_t **, int);
herr_t H5O_shared_read(H5FD_t *, H5O_shared_t *, const H5O_class_t *);
herr_t  decode_validate_messages(H5FD_t *, H5O_t *);

/* for handling hard links */
int	table_init(table_t **);
int	table_search(haddr_t);
void	table_insert(haddr_t, int);


/* copied and modified from H5Onull.c */
const H5O_class_t H5O_NULL[1] = {{
    H5O_NULL_ID,            /*message id number             */
    "null",                 /*message name for debugging    */
    0,                      /*native message size           */
    NULL                    /*no decode method              */
}};


/* copied and modified from H5Osdspace.c */
static void *H5O_sdspace_decode(const uint8_t *);

/* copied and modified from H5Osdspace.c */
/* This message derives from H5O */
const H5O_class_t H5O_SDSPACE[1] = {{
    H5O_SDSPACE_ID,             /* message id number                    */
    "simple_dspace",            /* message name for debugging           */
    sizeof(H5S_extent_t),       /* native message size                  */
    H5O_sdspace_decode         	/* decode message                       */
}};



/* copied and modified from H5Odtype.c */
static void *H5O_dtype_decode (const uint8_t *);

/* copied and modified from H5Odtype.c */
/* This message derives from H5O */
const H5O_class_t H5O_DTYPE[1] = {{
    H5O_DTYPE_ID,               /* message id number            */
    "data_type",                /* message name for debugging   */
    sizeof(H5T_t),              /* native message size          */
    H5O_dtype_decode            /* decode message               */
}};


/* copied and modified from H5Ofill.c */
static void  *H5O_fill_new_decode(const uint8_t *);

/* copied and modified from H5Ofill.c */
/* This message derives from H5O, for new fill value after version 1.4  */
const H5O_class_t H5O_FILL_NEW[1] = {{
    H5O_FILL_NEW_ID,            /*message id number                     */
    "fill_new",                 /*message name for debugging            */
    sizeof(H5O_fill_new_t),     /*native message size                   */
    H5O_fill_new_decode         /*decode message                        */
}};

static void *H5O_efl_decode(const uint8_t *p);

/* copied and modified from H5Oefl.c */
/* This message derives from H5O */
const H5O_class_t H5O_EFL[1] = {{
    H5O_EFL_ID,                 /*message id number             */
    "external file list",       /*message name for debugging    */
    sizeof(H5O_efl_t),          /*native message size           */
    H5O_efl_decode,             /*decode message                */
}};


/* copied and modified from H5Olayout.c */
static void *H5O_layout_decode(const uint8_t *);


/* copied and modified from H5Olayout.c */
/* This message derives from H5O */
const H5O_class_t H5O_LAYOUT[1] = {{
    H5O_LAYOUT_ID,              /*message id number             */
    "layout",                   /*message name for debugging    */
    sizeof(H5O_layout_t),       /*native message size           */
    H5O_layout_decode	        /*decode message                */
}};

/* copied and modified from H5Opline.c */
static void *H5O_pline_decode (const uint8_t *);

/* copied and modified from H5Opline.c */
/* This message derives from H5O */
const H5O_class_t H5O_PLINE[1] = {{
    H5O_PLINE_ID,               /* message id number            */
    "filter pipeline",          /* message name for debugging   */
    sizeof(H5O_pline_t),        /* native message size          */
    H5O_pline_decode,           /* decode message               */
}};

/* copied and modified from H5Oattr.c */
static void *H5O_attr_decode (const uint8_t *);

/* copied and modified from H5Oattr.c */
/* This message derives from H5O */
const H5O_class_t H5O_ATTR[1] = {{
    H5O_ATTR_ID,                /* message id number            */
    "attribute",                /* message name for debugging   */
    sizeof(H5A_t),              /* native message size          */
    H5O_attr_decode             /* decode message               */
}};


/* copied and modified from H5Oname.c */
static void *H5O_name_decode(const uint8_t *p);

/* copied and modified from H5Oname.c */
/* This message derives from H5O */
const H5O_class_t H5O_NAME[1] = {{
    H5O_NAME_ID,                /*message id number             */
    "name",                     /*message name for debugging    */
    sizeof(H5O_name_t),         /*native message size           */
    H5O_name_decode,            /*decode message                */
}};



/* copied from H5Oshared.c */
/* Old version, with full symbol table entry as link for object header sharing */
#define H5O_SHARED_VERSION_1    1

/* New version, with just address of object as link for object header sharing */
#define H5O_SHARED_VERSION      2

static void *H5O_shared_decode (const uint8_t*);

/* copied and modified from H5Oshared.c */
/* This message derives from H5O */
const H5O_class_t H5O_SHARED[1] = {{
    H5O_SHARED_ID,              /*message id number                     */
    "shared",                   /*message name for debugging            */
    sizeof(H5O_shared_t),       /*native message size                   */
    H5O_shared_decode,          /*decode method                         */
}};


/* copied and modified from H5Ocont.c */
static void *H5O_cont_decode(const uint8_t *);

/* copied and modified from H5Ocont.c */
/* This message derives from H5O */
const H5O_class_t H5O_CONT[1] = {{
    H5O_CONT_ID,                /*message id number             */
    "hdr continuation",         /*message name for debugging    */
    sizeof(H5O_cont_t),         /*native message size           */
    H5O_cont_decode             /*decode message                */
}};


/* copied and modified from H5Ostab.c */
static void *H5O_stab_decode(const uint8_t *);

/* copied and modified from H5Ostab.c */
/* This message derives from H5O */
const H5O_class_t H5O_STAB[1] = {{
    H5O_STAB_ID,               /*message id number             */
    "stab",                    /*message name for debugging    */
    sizeof(H5O_stab_t),        /*native message size           */
    H5O_stab_decode            /*decode message                */
}};




/* copied and modified from H5Omtime.c */
static void *H5O_mtime_new_decode(const uint8_t *);

/* copied and modified from H5Omtime.c */
/* This message derives from H5O */
/* (Only encode, decode & size routines are different from old mtime routines) */
const H5O_class_t H5O_MTIME_NEW[1] = {{
    H5O_MTIME_NEW_ID,           /*message id number             */
    "mtime_new",                /*message name for debugging    */
    sizeof(time_t),             /*native message size           */
    H5O_mtime_new_decode,       /*decode message                */
}};


/* copied and modified from H5O.c */
/* ID to type mapping */
/* Only defined for H5O_NULL and H5O_STAB, NULL the rest for NOW */
static const H5O_class_t *const message_type_g[] = {
    H5O_NULL,           /*0x0000 Null                                   */
    H5O_SDSPACE,        /*0x0001 Simple Dimensionality                  */
    NULL,          	/*0x0002 Data space (fiber bundle?)             */
    H5O_DTYPE,          /*0x0003 Data Type                              */
    NULL,           	/*0x0004 Old data storage -- fill value         */
    H5O_FILL_NEW,      	/*0x0005 New Data storage -- fill value         */
    NULL,               /*0x0006 Data storage -- compact object         */
    H5O_EFL,           	/*0x0007 Data storage -- external data files    */
    H5O_LAYOUT,        	/*0x0008 Data Layout                            */
    NULL,               /*0x0009 "Bogus"                                */
    NULL,               /*0x000A Not assigned                           */
    H5O_PLINE,       	/*0x000B Data storage -- filter pipeline        */
    H5O_ATTR,          	/*0x000C Attribute list                         */
    H5O_NAME,          	/*0x000D Object name                            */
    NULL,         	/*0x000E Object modification date and time      */
    H5O_SHARED,        	/*0x000F Shared header message                  */
    H5O_CONT,          	/*0x0010 Object header continuation             */
    H5O_STAB,           /*0x0011 Symbol table                           */
    H5O_MTIME_NEW	/*0x0012 New Object modification date and time  */
};


/* copied and modified from H5Gnode.c */
static size_t H5G_node_sizeof_rkey(H5F_shared_t, unsigned);
static void *H5G_node_decode_key(H5F_shared_t, unsigned, const uint8_t **);

/* copied and modified from H5Gnode.c */
/* H5G inherits B-tree like properties from H5B */
H5B_class_t H5B_SNODE[1] = {
    H5B_SNODE_ID,               /*id                    */
    sizeof(H5G_node_key_t),     /*sizeof_nkey           */
    H5G_node_sizeof_rkey,       /*get_sizeof_rkey       */
    H5G_node_decode_key,        /*decode                */
};

/* copied and modified from H5Distore.c */
static size_t H5D_istore_sizeof_rkey(H5F_shared_t, unsigned);
static void *H5D_istore_decode_key(H5F_shared_t, unsigned, const uint8_t **);

/* copied and modified from H5Distore.c */
/* inherits B-tree like properties from H5B */
H5B_class_t H5B_ISTORE[1] = {
    H5B_ISTORE_ID,              /*id                    */
    sizeof(H5D_istore_key_t),   /*sizeof_nkey           */
    H5D_istore_sizeof_rkey,     /*get_sizeof_rkey       */
    H5D_istore_decode_key,      /*decode                */
};

static const H5B_class_t *const node_key_g[] = {
    	H5B_SNODE,  	/* group node: symbol table */
	H5B_ISTORE	/* raw data chunk node */
};


/* copied and modified from H5FDsec2.c */
static H5FD_t *H5FD_sec2_open(const char *name, int driver_id);
static herr_t H5FD_sec2_read(H5FD_t *_file, haddr_t addr, size_t size, void *buf/*out*/);
static herr_t H5FD_sec2_close(H5FD_t *_file);
static haddr_t H5FD_sec2_get_eof(H5FD_t *_file);

/* copied and modified from H5FDsec2.c */
static const H5FD_class_t H5FD_sec2_g = {
    "sec2",                                     /*name                  */
#if 0
    MAXADDR,                                    /*maxaddr               */
    H5F_CLOSE_WEAK,                             /* fc_degree            */
    NULL,                                       /*sb_size               */
    NULL,                                       /*sb_decode             */
    0,                                          /*fapl_size             */
    NULL,                                       /*fapl_get              */
    NULL,                                       /*fapl_copy             */
    NULL,                                       /*fapl_free             */
    0,                                          /*dxpl_size             */
    NULL,                                       /*dxpl_copy             */
    NULL,                                       /*dxpl_free             */
    H5FD_sec2_cmp,                              /*cmp                   */
    H5FD_sec2_query,                            /*query                 */
    NULL,                                       /*alloc                 */
    NULL,                                       /*free                  */
#endif
    NULL,                                       /*sb_decode             */
    H5FD_sec2_open,                             /*open                  */
    H5FD_sec2_close,                            /*close                 */
    H5FD_sec2_read,                             /*read                  */
    H5FD_sec2_get_eof,                          /*get_eof               */
#if 0
    H5FD_sec2_get_eoa,                          /*get_eoa               */
    H5FD_sec2_set_eoa,                          /*set_eoa               */
    H5FD_sec2_get_handle,                       /*get_handle            */
    H5FD_sec2_read,                             /*read                  */
    H5FD_sec2_write,                            /*write                 */
    H5FD_sec2_flush,                            /*flush                 */
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE                           /*fl_map                */
#endif
};

/* copied and modified from H5FDfamily.c */
static herr_t H5FD_family_read(H5FD_t *_file, haddr_t addr, size_t size, void *buf/*out*/);
static H5FD_t *H5FD_family_open(const char *name, int driver_id);
static herr_t H5FD_family_close(H5FD_t *_file);
static haddr_t H5FD_family_get_eof(H5FD_t *_file);


/* copied and modified from H5FDfamily.c */
/* The class struct */
static const H5FD_class_t H5FD_family_g = {
    "family",                                   /*name                  */
#if 0
    HADDR_MAX,                                  /*maxaddr               */
    H5F_CLOSE_WEAK,                             /* fc_degree            */
    NULL,                                       /*sb_size               */
    NULL,                                       /*sb_encode             */
    sizeof(H5FD_family_fapl_t),                 /*fapl_size             */
    H5FD_family_fapl_get,                       /*fapl_get              */
    H5FD_family_fapl_copy,                      /*fapl_copy             */
    H5FD_family_fapl_free,                      /*fapl_free             */
    sizeof(H5FD_family_dxpl_t),                 /*dxpl_size             */
    H5FD_family_dxpl_copy,                      /*dxpl_copy             */
    H5FD_family_dxpl_free,                      /*dxpl_free             */
    H5FD_family_open,                           /*open                  */
    H5FD_family_close,                          /*close                 */
    H5FD_family_cmp,                            /*cmp                   */
    H5FD_family_query,                          /*query                 */
    NULL,                                       /*alloc                 */
    NULL,                                       /*free                  */
#endif
    NULL,                                       /*sb_decode             */
    H5FD_family_open,                           /*open                  */
    H5FD_family_close,                          /*close                 */
    H5FD_family_read,                           /*read                  */
    H5FD_family_get_eof,                        /*get_eof               */
#if 0
    H5FD_family_get_eoa,                        /*get_eoa               */
    H5FD_family_set_eoa,                        /*set_eoa               */
    H5FD_family_get_eof,                        /*get_eof               */
    H5FD_family_get_handle,                     /*get_handle            */
    H5FD_family_read,                           /*read                  */
    H5FD_family_write,                          /*write                 */
    H5FD_family_flush,                          /*flush                 */
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE                           /*fl_map                */
#endif
};
 

/* copied and modified from H5FDmulti.c */
static herr_t H5FD_multi_sb_decode(H5F_shared_t *_shared_info, const unsigned char *buf);
static H5FD_t *H5FD_multi_open(const char *name, int driver_id);
static herr_t H5FD_multi_close(H5FD_t *_file);
static herr_t H5FD_multi_read(H5FD_t *_file, haddr_t addr, size_t size, void *_buf/*out*/);
static haddr_t H5FD_multi_get_eof(H5FD_t *_file);


/* copied from H5FDmulti.c */
static int compute_next(H5FD_multi_t *file);
static int open_members(H5FD_multi_t *file);

/* copied and modified from H5FDmulti.c */
/* The class struct */
static const H5FD_class_t H5FD_multi_g = {
    "multi",                                    /*name                  */
#if 0
    HADDR_MAX,                                  /*maxaddr               */
    H5F_CLOSE_WEAK,                             /* fc_degree            */
    H5FD_multi_sb_size,                         /*sb_size               */
    H5FD_multi_sb_encode,                       /*sb_encode             */
    sizeof(H5FD_multi_fapl_t),                  /*fapl_size             */
    H5FD_multi_fapl_get,                        /*fapl_get              */
    H5FD_multi_fapl_copy,                       /*fapl_copy             */
    H5FD_multi_fapl_free,                       /*fapl_free             */
    sizeof(H5FD_multi_dxpl_t),                  /*dxpl_size             */
    H5FD_multi_dxpl_copy,                       /*dxpl_copy             */
    H5FD_multi_dxpl_free,                       /*dxpl_free             */
    H5FD_multi_cmp,                             /*cmp                   */
    H5FD_multi_query,                           /*query                 */
    H5FD_multi_alloc,                           /*alloc                 */
    H5FD_multi_free,                            /*free                  */
#endif
    H5FD_multi_sb_decode,                       /*sb_decode             */
    H5FD_multi_open,                            /*open                  */
    H5FD_multi_close,                           /*close                 */
    H5FD_multi_read,                            /*read                  */
    H5FD_multi_get_eof,                         /*get_eof               */
#if 0
    H5FD_multi_get_eoa,                         /*get_eoa               */
    H5FD_multi_set_eoa,                         /*set_eoa               */
    H5FD_multi_get_handle,                      /*get_handle            */
    H5FD_multi_write,                           /*write                 */
    H5FD_multi_flush,                           /*flush                 */
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_DEFAULT                          /*fl_map                */
#endif
};



int
table_init(table_t **obj_table)
{
	int 	i;
	table_t	*tb;

	tb = malloc(sizeof(table_t));
	if (tb == NULL) {
		H5E_push("table_init", "Couldn't malloc() table_t.", -1);
		return(FAIL);
	}
	tb->size = 20;
	tb->nobjs = 0;
	tb->objs = malloc(tb->size*sizeof(obj_t));
	for (i = 0; i < tb->size; i++) {
		tb->objs[i].objno = 0;
		tb->objs[i].nlink = 0;
	}
	*obj_table = tb;
	return(SUCCEED);
}

/* copied and modified from tool/lib/h5trav_table.c */
int
table_search(haddr_t obj_id)
{
	int	i;

	if (obj_table == NULL)
		return(FAIL);
	for (i = 0; i < obj_table->nobjs; i++) {
		if (obj_table->objs[i].objno == obj_id) /* FOUND */
			return(i);
	}
	return(FAIL);
}

/* copied and modified from tool/lib/h5trav_table.c */
void
table_insert(haddr_t objno, int nlink)
{
	int	i;

	if (obj_table == NULL)
		return;
	if (obj_table->nobjs == obj_table->size) {
		obj_table->size *= 2;
		obj_table->objs =  realloc(obj_table->objs, obj_table->size * sizeof(obj_t));
	}
	for (i = obj_table->nobjs; i < obj_table->size; i++) {
		obj_table->objs[i].objno = 0;
		obj_table->objs[i].nlink = 0;
	}
	i = obj_table->nobjs++;
	obj_table->objs[i].objno = objno;
	obj_table->objs[i].nlink = nlink;
}

/* copied and modified from H5Osdspace.c */
/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_decode
 PURPOSE
    Decode a simple dimensionality message and return a pointer to a memory
        struct with the decoded information
 USAGE
    void *H5O_sdspace_decode(f, raw_size, p)
        H5F_t *f;         IN: pointer to the HDF5 file struct
        size_t raw_size;        IN: size of the raw information buffer
        const uint8 *p;         IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
        This function decodes the "raw" disk form of a simple dimensionality
    message into a struct in memory native format.  The struct is allocated
    within this function using malloc() and is returned to the caller.

 MODIFICATIONS
        Robb Matzke, 1998-04-09
        The current and maximum dimensions are now H5F_SIZEOF_SIZET bytes
        instead of just four bytes.

        Robb Matzke, 1998-07-20
        Added a version number and reformatted the message for aligment.
--------------------------------------------------------------------------*/
static void *
H5O_sdspace_decode(const uint8_t *p)
{
    	H5S_extent_t  	*sdim = NULL;	/* New extent dimensionality structure */
    	void          	*ret_value;
    	unsigned        i;              /* local counting variable */
    	unsigned        flags, version;
	int		ret;


    	/* check args */
    	assert(p);
	ret = SUCCEED;

    	/* decode */
    	sdim = calloc(1, sizeof(H5S_extent_t));
	if (sdim == NULL) {
		H5E_push("H5O_sdspace_decode", "Couldn't malloc() H5S_extent_t.", -1);
		ret++;
		goto done;
	}
        /* Check version */
        version = *p++;

        if (version != H5O_SDSPACE_VERSION) {
		H5E_push("H5O_sdspace_decode", "Bad version number in simple dataspace message.", -1);
		ret++;
	}

	/* SHOULD I CHECK FOR IT: not specified in the specification, but in coding only?? */
        /* Get rank */
        sdim->rank = *p++;
        if (sdim->rank > H5S_MAX_RANK) {
		H5E_push("H5O_sdspace_decode", "Simple dataspace dimensionality is too large.", -1);
		ret++;
	}

        /* Get dataspace flags for later */
        flags = *p++;
	if (flags > 0x3) {  /* Only bit 0 and bit 1 can be set */
		H5E_push("H5O_sdspace_decode", "Corrupt flags for simple dataspace.", -1);
		ret++;
	}

	 /* Set the dataspace type to be simple or scalar as appropriate */
        if(sdim->rank>0)
            	sdim->type = H5S_SIMPLE;
        else
            	sdim->type = H5S_SCALAR;


        p += 5; /*reserved*/

	if (sdim->rank > 0) {
            	if ((sdim->size=malloc(sizeof(hsize_t)*sdim->rank))==NULL) {
			H5E_push("H5O_sdspace_decode", "Couldn't malloc() hsize_t.", -1);
			ret++;
			goto done;
		}
            	for (i = 0; i < sdim->rank; i++)
                	H5F_DECODE_LENGTH (shared_info, p, sdim->size[i]);
            	if (flags & H5S_VALID_MAX) {
                	if ((sdim->max=malloc(sizeof(hsize_t)*sdim->rank))==NULL) {
				H5E_push("H5O_sdspace_decode", "Couldn't malloc() hsize_t.", -1);
				ret++;
				goto done;
			}
                	for (i = 0; i < sdim->rank; i++)
                    		H5F_DECODE_LENGTH (shared_info, p, sdim->max[i]);
            	}
        }

        /* Compute the number of elements in the extent */
        for(i=0, sdim->nelem=1; i<sdim->rank; i++)
           	sdim->nelem *= sdim->size[i];

    	ret_value = (void*)sdim;    /*success*/
done:
	if (ret) {
		if (sdim) {
			if (sdim->size) free(sdim->size);
			if (sdim->max) free(sdim->max);
			free(sdim);
		}
		ret_value = NULL;
	}
    	/* Set return value */
    	return(ret_value);
}


/* copied and modified from H5Odtype.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_dtype_decode_helper
 *
 * Purpose:     Decodes a datatype
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *              Robb Matzke, Thursday, May 20, 1999
 *              Added support for bitfields and opaque datatypes.
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_decode_helper(const uint8_t **pp, H5T_t *dt)
{
 	unsigned        flags, version, tmp;
    	unsigned        i, j;
    	size_t          z;
    	herr_t      	ret=SUCCEED;       /* Return value */


    	/* check args */
    	assert(pp && *pp);
    	assert(dt && dt->shared);

    	/* decode */
    	UINT32DECODE(*pp, flags);
    	version = (flags>>4) & 0x0f;
    	if (version != H5O_DTYPE_VERSION_COMPAT && version != H5O_DTYPE_VERSION_UPDATED) {
		H5E_push("H5O_dtype_decode_helper", "Bad version number for datatype message.\n", -1);
		ret++;
	}

    	dt->shared->type = (H5T_class_t)(flags & 0x0f);
	if ((dt->shared->type < H5T_INTEGER) ||(dt->shared->type >H5T_ARRAY)) {
		H5E_push("H5O_dtype_decode_helper", "Invalid class value for datatype mesage.", -1);
		ret++;
		return(ret);
	}
    	flags >>= 8;
    	UINT32DECODE(*pp, dt->shared->size);
/* Do i need to check for size < 0??? */


    	switch (dt->shared->type) {
          case H5T_INTEGER:
            /*
             * Integer types...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.u.i.sign = (flags & 0x8) ? H5T_SGN_2 : H5T_SGN_NONE;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
	    if ((flags>>4) != 0) {  /* should be reserved(zero) */
		H5E_push("H5O_dtype_decode_helper", "Bits 4-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }
            break;

	 case H5T_FLOAT:
            /*
             * Floating-point types...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.u.f.pad = (flags & 0x8) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            switch ((flags >> 4) & 0x03) {
                case 0:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_NONE;
                    break;
                case 1:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_MSBSET;
                    break;
                case 2:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_IMPLIED;
                    break;
                default:
		    H5E_push("H5O_dtype_decode_helper", "Unknown floating-point normalization.", -1);
		    ret++;
		    break;
            }
            dt->shared->u.atomic.u.f.sign = (flags >> 8) & 0xff;

	    if (((flags >> 6) & 0x03) != 0) {
		H5E_push("H5O_dtype_decode_helper", "Bits 6-7 should be 0 for datatype class bit field.", -1);
		ret++;
	    }
	    if ((flags >> 16) != 0) {
		H5E_push("H5O_dtype_decode_helper", "Bits 16-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }
	
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            dt->shared->u.atomic.u.f.epos = *(*pp)++;
            dt->shared->u.atomic.u.f.esize = *(*pp)++;
            assert(dt->shared->u.atomic.u.f.esize > 0);
            dt->shared->u.atomic.u.f.mpos = *(*pp)++;
            dt->shared->u.atomic.u.f.msize = *(*pp)++;
            assert(dt->shared->u.atomic.u.f.msize > 0);
            UINT32DECODE(*pp, dt->shared->u.atomic.u.f.ebias);
            break;

	case H5T_TIME:  /* Time datatypes */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;

	    if ((flags>>1) != 0) {  /* should be reserved(zero) */
		H5E_push("H5O_dtype_decode_helper", "Bits 1-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }

            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

	  case H5T_STRING:
            /*
             * Character string types...
             */
            dt->shared->u.atomic.order = H5T_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;
            dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;

            dt->shared->u.atomic.u.s.pad = (H5T_str_t)(flags & 0x0f);
            dt->shared->u.atomic.u.s.cset = (H5T_cset_t)((flags>>4) & 0x0f);            

	    /* Padding Type supported: Null Terminate, Null Pad and Space Pad */
	    if ((dt->shared->u.atomic.u.s.pad != H5T_STR_NULLTERM) &&
	        (dt->shared->u.atomic.u.s.pad != H5T_STR_NULLPAD) &&
	        (dt->shared->u.atomic.u.s.pad != H5T_STR_SPACEPAD)) {
		H5E_push("H5O_dtype_decode_helper", "Unsupported padding type for datatype class bit field.", -1);
		ret++;
	    }
	    /* The only character set supported is the 8-bit ASCII */
	    if (dt->shared->u.atomic.u.s.cset != H5T_CSET_ASCII) {
		H5E_push("H5O_dtype_decode_helper", "Unsupported character set for datatype class bit field.", -1);
		ret++;
	    }
	    if ((flags>>8) != 0) {  /* should be reserved(zero) */
		H5E_push("H5O_dtype_decode_helper", "Bits 8-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }
	    break;
	
        case H5T_BITFIELD:
            /*
             * Bit fields...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
	    if ((flags>>3) != 0) {  /* should be reserved(zero) */
		H5E_push("H5O_dtype_decode_helper", "Bits 3-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_OPAQUE:
            /*
             * Opaque types...
             */
            z = flags & (H5T_OPAQUE_TAG_MAX - 1);
            assert(0==(z&0x7)); /*must be aligned*/
	    if ((flags>>8) != 0) {  /* should be reserved(zero) */
		H5E_push("H5O_dtype_decode_helper", "Bits 8-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }

            if ((dt->shared->u.opaque.tag=malloc(z+1))==NULL) {
		H5E_push("H5O_dtype_decode_helper", "Couldn't malloc tag for opaque type.", -1);
		ret++;
		return(ret);
	    }

	    /*??? should it be nul-terminated???? see spec. */
            memcpy(dt->shared->u.opaque.tag, *pp, z);
            dt->shared->u.opaque.tag[z] = '\0';
            *pp += z;
            break;

	 case H5T_COMPOUND:
            /*
             * Compound datatypes...
             */
            dt->shared->u.compnd.nmembs = flags & 0xffff;
            assert(dt->shared->u.compnd.nmembs > 0);
	    if ((flags>>16) != 0) {  /* should be reserved(zero) */
		H5E_push("H5O_dtype_decode_helper", "Bits 16-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }
            dt->shared->u.compnd.packed = TRUE; /* Start off packed */
            dt->shared->u.compnd.nalloc = dt->shared->u.compnd.nmembs;
            dt->shared->u.compnd.memb = calloc(dt->shared->u.compnd.nalloc,
                            sizeof(H5T_cmemb_t));
            if ((dt->shared->u.compnd.memb==NULL)) {
		H5E_push("H5O_dtype_decode_helper", "Couldn't calloc() H5T_cmemb_t.", -1);
		ret++;
		return(ret);
	    }
            for (i = 0; i < dt->shared->u.compnd.nmembs; i++) {
                unsigned ndims=0;     /* Number of dimensions of the array field */
		hsize_t dim[H5O_LAYOUT_NDIMS];  /* Dimensions of the array */
                unsigned perm_word=0;    /* Dimension permutation information */
                H5T_t *temp_type;   /* Temporary pointer to the field's datatype */
		size_t	tmp_len;

                /* Decode the field name */
		if (!((const char *)*pp)) {
			H5E_push("H5O_dtype_decode_helper", "Invalid string pointer.", -1);
			ret++;
			return(ret);
		}
		tmp_len = strlen((const char *)*pp) + 1;
                if ((dt->shared->u.compnd.memb[i].name=malloc(tmp_len))==NULL) {
			H5E_push("H5O_dtype_decode_helper", "Couldn't malloc() string.", -1);
			ret++;
			return(ret);
		}
		strcpy(dt->shared->u.compnd.memb[i].name, (const char *)*pp);

                /*multiple of 8 w/ null terminator */
                *pp += ((strlen((const char *)*pp) + 8) / 8) * 8;

                /* Decode the field offset */
                UINT32DECODE(*pp, dt->shared->u.compnd.memb[i].offset);

                /* Older versions of the library allowed a field to have
                 * intrinsic 'arrayness'.  Newer versions of the library
                 * use the separate array datatypes. */
	        if(version==H5O_DTYPE_VERSION_COMPAT) {
                    /* Decode the number of dimensions */
                    ndims = *(*pp)++;
                    assert(ndims <= 4);
                    *pp += 3;           /*reserved bytes */

                    /* Decode dimension permutation (unused currently) */
                    UINT32DECODE(*pp, perm_word);

                    /* Skip reserved bytes */
                    *pp += 4;

                    /* Decode array dimension sizes */
                    for (j=0; j<4; j++)
                        UINT32DECODE(*pp, dim[j]);
                } /* end if */

		if ((temp_type = H5T_alloc()) == NULL) {
              		H5E_push("H5O_dtype_decode_helper", "Failure in H5T_alloc().", -1);
			ret++;
               		return(ret);
		}

                /* Decode the field's datatype information */
                if (H5O_dtype_decode_helper(pp, temp_type) != SUCCEED) {
                    for (j = 0; j <= i; j++)
                        free(temp_type->shared->u.compnd.memb[j].name);
                    free(temp_type->shared->u.compnd.memb);
                    H5E_push("H5O_dtype_decode_helper", "Unable to decode compound member type.", -1);
		    ret++;
                    return(ret);
                }
		/* delete for NOW */
		/* Go create the array datatype now, for older versions of the datatype message */

		/* delete for NOW */
                /*
                 * Set the "force conversion" flag if VL datatype fields exist in this
                 * type or any component types
                 */

                /* Member size */
                dt->shared->u.compnd.memb[i].size = temp_type->shared->size;

                /* Set the field datatype (finally :-) */
                dt->shared->u.compnd.memb[i].type = temp_type;

		/* delete for now */
		/* Check if the datatype stayed packed */
            }
            break;

        case H5T_REFERENCE: /* Reference datatypes...  */
            dt->shared->u.atomic.order = H5T_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;            
	    dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;

            /* Set reference type */
            dt->shared->u.atomic.u.r.rtype = (H5R_type_t)(flags & 0x0f);
	    /* Value of 2 is not implemented yet (see spec.) */
	    /* value of 3-15 is supposedly to be reserved */
	    if ((flags&0x0f) >= 2) {
		H5E_push("H5O_dtype_decode_helper", "Invalid datatype class bit field.", -1);
		ret++;
	    }

	    if ((flags>>4) != 0) {
		H5E_push("H5O_dtype_decode_helper", "Bits 4-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }
            break;

	case H5T_ENUM:
            /*
             * Enumeration datatypes...
             */
            dt->shared->u.enumer.nmembs = dt->shared->u.enumer.nalloc = flags & 0xffff;
	    if ((flags>>16) != 0) {
		H5E_push("H5O_dtype_decode_helper", "Bits 16-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }

	    if ((dt->shared->parent=H5T_alloc()) == NULL) {
              	H5E_push("H5O_dtype_decode_helper", "Failure in H5T_alloc().", -1);
		ret++;
               	return(ret);
	    }

            if (H5O_dtype_decode_helper(pp, dt->shared->parent) != SUCCEED) {
                    H5E_push("H5O_dtype_decode_helper", "Unable to decode parent datatype.", -1);
		    ret++;
                    return(ret);
	    }

	    if ((dt->shared->u.enumer.name=calloc(dt->shared->u.enumer.nalloc, sizeof(char*)))==NULL) {
              	H5E_push("H5O_dtype_decode_helper", "Couldn't calloc() enum. name pointer", -1);
		ret++;
               	return(ret);
	    }
            if ((dt->shared->u.enumer.value=calloc(dt->shared->u.enumer.nalloc,
                    dt->shared->parent->shared->size))==NULL) {
              	H5E_push("H5O_dtype_decode_helper", "Couldn't calloc() space for enum. values", -1);
		ret++;
               	return(ret);
	    }


            /* Names, each a multiple of 8 with null termination */
            for (i=0; i<dt->shared->u.enumer.nmembs; i++) {
		size_t	tmp_len;

                /* Decode the field name */
		if (!((const char *)*pp)) {
			H5E_push("H5O_dtype_decode_helper", "Invalid string pointer.", -1);
			ret++;
			return(ret);
		}
		tmp_len = strlen((const char *)*pp) + 1;
                if ((dt->shared->u.enumer.name[i]=malloc(tmp_len))==NULL) {
			H5E_push("H5O_dtype_decode_helper", "Couldn't malloc() string.", -1);
			ret++;
			return(ret);
		}
		strcpy(dt->shared->u.enumer.name[i], (const char *)*pp);
                /*multiple of 8 w/ null terminator */
                *pp += ((strlen((const char *)*pp) + 8) / 8) * 8;
            }

            /* Values */
            memcpy(dt->shared->u.enumer.value, *pp,
                 dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size);
            *pp += dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size;
            break;

        case H5T_VLEN:  /* Variable length datatypes...  */
            /* Set the type of VL information, either sequence or string */
            dt->shared->u.vlen.type = (H5T_vlen_type_t)(flags & 0x0f);
            if(dt->shared->u.vlen.type == H5T_VLEN_STRING) {
                dt->shared->u.vlen.pad  = (H5T_str_t)((flags>>4) & 0x0f);
                dt->shared->u.vlen.cset = (H5T_cset_t)((flags>>8) & 0x0f);
            } /* end if */

	    /* Only sequence and string are defined */
	    if (dt->shared->u.vlen.type > H5T_VLEN_STRING) {
		
	    }
	    /* Padding type and character set should be zero for vlen sequence */
	    if (dt->shared->u.vlen.type == H5T_VLEN_SEQUENCE) {
		if (((flags>>4)&0x0f) != 0) {
		}
		if (((flags>>8)&0x0f)!= 0) {
		}
	    }
	    if ((flags>>12) != 0) {
		H5E_push("H5O_dtype_decode_helper", "Bits 12-23 should be 0 for datatype class bit field.", -1);
		ret++;
	    }

            /* Decode base type of VL information */
            if((dt->shared->parent = H5T_alloc())==NULL) {
		H5E_push("H5O_dtype_decode_helper", "Failure in H5T_alloc().", -1);
		ret++;
		return(ret);
	    }

            if (H5O_dtype_decode_helper(pp, dt->shared->parent) != SUCCEED) {
		H5E_push("H5O_dtype_decode_helper", "Unable to decode VL parent type.", -1);
		ret++;
		return(ret);
	    }

	    /* delete for NOW */
            /* dt->shared->force_conv=TRUE; */

	    /* delete for NOW */
            /* Mark this type as on disk (via H5T_vlen_mark()) */
            break;


        case H5T_ARRAY:  /* Array datatypes...  */
            /* Decode the number of dimensions */
            dt->shared->u.array.ndims = *(*pp)++;

            /* Double-check the number of dimensions */
            assert(dt->shared->u.array.ndims <= H5S_MAX_RANK);

            /* Skip reserved bytes */
            *pp += 3;

            /* Decode array dimension sizes & compute number of elements */
            for (j=0, dt->shared->u.array.nelem=1; j<(unsigned)dt->shared->u.array.ndims; j++) {
                UINT32DECODE(*pp, dt->shared->u.array.dim[j]);
                dt->shared->u.array.nelem *= dt->shared->u.array.dim[j];
            } /* end for */

            /* Decode array dimension permutations (even though they are unused currently) */
            for (j=0; j<(unsigned)dt->shared->u.array.ndims; j++)
                UINT32DECODE(*pp, dt->shared->u.array.perm[j]);

            /* Decode base type of array */
            if((dt->shared->parent = H5T_alloc())==NULL) {
		H5E_push("H5O_dtype_decode_helper", "Failure in H5T_alloc().", -1);
		ret++;
		return(ret);
	    }
            if (H5O_dtype_decode_helper(pp, dt->shared->parent) != SUCCEED) {
		H5E_push("H5O_dtype_decode_helper", "Unable to decode array parent type.", -1);
		ret++;
		return(ret);
	    }


	    /* delete for NOW */
            /*
             * Set the "force conversion" flag if a VL base datatype is used or
             * or if any components of the base datatype are VL types.
             */
            break;

	   default:
	     /* shouldn't come to here */
	     printf("H5O_dtype_decode_helper: datatype class not handled yet...type=%d\n", dt->shared->type);
    	}

	return(ret);
}


/* copied and modified from H5Odtype.c */
/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_decode
 PURPOSE
    Decode a message and return a pointer to a memory struct
        with the decoded information
 USAGE
    void *H5O_dtype_decode(f, raw_size, p)
        H5F_t *f;               IN: pointer to the HDF5 file struct
        size_t raw_size;        IN: size of the raw information buffer
        const uint8 *p;         IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
        This function decodes the "raw" disk form of a simple datatype message
    into a struct in memory native format.  The struct is allocated within this
    function using malloc() and is returned to the caller.
--------------------------------------------------------------------------*/
static void *
H5O_dtype_decode(const uint8_t *p)
{
    	H5T_t		*dt = NULL;
    	void    	*ret_value;     /* Return value */
	int		ret;


    	/* check args */
    	assert(p);
	ret = SUCCEED;

	if ((dt = calloc(1, sizeof(H5T_t))) == NULL) {
		H5E_push("H5O_dtype_decode", "Couldn't malloc() H5T_t.", -1);
		return(ret_value=NULL);
	}
	/* NOT SURE what to use with the group entry yet */
	memset(&(dt->ent), 0, sizeof(H5G_entry_t));
	dt->ent.header = HADDR_UNDEF;
	if ((dt->shared = calloc(1, sizeof(H5T_shared_t))) == NULL) {
		H5E_push("H5O_dtype_decode", "Couldn't malloc() H5T_shared_t.", -1);
		return(ret_value=NULL);
	}

    	if (H5O_dtype_decode_helper(&p, dt) != SUCCEED) {
		ret++;
		H5E_push("H5O_dtype_decode", "Can't decode datatype message.", -1);
	}

	if (ret)	
		dt = NULL;
    	/* Set return value */
    	ret_value = dt;
	return(ret_value);
}


/* copied and modified from H5Ofill.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_fill_new_decode
 *
 * Purpose:     Decode a new fill value message.  The new fill value
 *              message is fill value plus space allocation time and
 *              fill value writing time and whether fill value is defined.
 *
 * Return:      Success:        Ptr to new message in native struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Raymond Lu
 *              Feb 26, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_fill_new_decode(const uint8_t *p)
{
    	H5O_fill_new_t      *mesg=NULL;
    	int                 version, ret;
    	void                *ret_value;


    	assert(p);
	ret = SUCCEED;

    	mesg = calloc(1, sizeof(H5O_fill_new_t));
	if (mesg == NULL) {
		H5E_push("H5O_fill_new_decode", "Couldn't malloc() H5O_fill_new_t.", -1);
		ret++;
		goto done;
	}

    	/* Version */
    	version = *p++;
    	if ((version != H5O_FILL_VERSION) && (version !=H5O_FILL_VERSION_2)) {
		H5E_push("H5O_fill_new_decode", "Bad version number for fill value message(new).", -1);
		ret++;
	}


    	/* Space allocation time */
    	mesg->alloc_time = (H5D_alloc_time_t)*p++;
	if ((mesg->alloc_time != H5D_ALLOC_TIME_EARLY) &&
	    (mesg->alloc_time != H5D_ALLOC_TIME_LATE) &&
	    (mesg->alloc_time != H5D_ALLOC_TIME_INCR)) {
		H5E_push("H5O_fill_new_decode", "Bad space allocation time for fill value message(new).", -1);
		ret++;
	}

    	/* Fill value write time */
    	mesg->fill_time = (H5D_fill_time_t)*p++;
	if ((mesg->fill_time != H5D_FILL_TIME_ALLOC) &&
	    (mesg->fill_time != H5D_FILL_TIME_NEVER) &&
	    (mesg->fill_time != H5D_FILL_TIME_IFSET)) {
		H5E_push("H5O_fill_new_decode", "Bad fill value write time for fill value message(new).", -1);
		ret++;
	}
	

    	/* Whether fill value is defined */
    	mesg->fill_defined = *p++;
	if ((mesg->fill_defined != 0) && (mesg->fill_defined != 1)) {
		H5E_push("H5O_fill_new_decode", "Bad fill value defined for fill value message(new).", -1);
		ret++;
	}


    	/* Only decode fill value information if one is defined */
    	if(mesg->fill_defined) {
        	INT32DECODE(p, mesg->size);
        	if (mesg->size > 0) {
            		H5_CHECK_OVERFLOW(mesg->size,ssize_t,size_t);
            		if (NULL==(mesg->buf=malloc((size_t)mesg->size))) {
				H5E_push("H5O_fill_new_decode", "Couldn't malloc() buffer for fill value.", -1);
				ret++;
				goto done;
			}
            		memcpy(mesg->buf, p, (size_t)mesg->size);
        	}
    	} else 
		mesg->size = (-1);


    	ret_value = (void*)mesg;
done:
	if (ret) {
		if (mesg) {
			if (mesg->buf) free(mesg->buf);
			free(mesg);
		}
		ret_value = NULL;
	}
    	return(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5O_efl_decode
 *
 * Purpose:     Decode an external file list message and return a pointer to
 *              the message (and some other data).
 *
 * Return:      Success:        Ptr to a new message struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, November 25, 1997
 *
 * Modifications:
 *      Robb Matzke, 1998-07-20
 *      Rearranged the message to add a version number near the beginning.
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_efl_decode(const uint8_t *p)
{
    	H5O_efl_t       *mesg = NULL;
    	int             version, ret;
    	const char      *s = NULL;
    	size_t          u;      	/* Local index variable */
    	void 		*ret_value;     /* Return value */

    	/* Check args */
    	assert(p);
	ret = SUCCEED;

    	if ((mesg = calloc(1, sizeof(H5O_efl_t)))==NULL) {
		H5E_push("H5O_efl_decode", "Couldn't malloc() H5O_efl_t.", -1);
		ret++;
		goto done;
	}

    	/* Version */
    	version = *p++;
    	if (version != H5O_EFL_VERSION) {
		H5E_push("H5O_efl_decode", "Bad version number for External Data Files message.", -1);
		ret++;
	}

    	/* Reserved */
    	p += 3;

    	/* Number of slots */
    	UINT16DECODE(p, mesg->nalloc);
    	assert(mesg->nalloc>0);
    	UINT16DECODE(p, mesg->nused);
    	assert(mesg->nused <= mesg->nalloc);

	if (!(mesg->nalloc >= mesg->nused)) {
		H5E_push("H5O_efl_decode", "Inconsistent number of allocated slots.", -1);
		ret++;
		goto done;
	}

    	/* Heap address */
    	H5F_addr_decode(shared_info, &p, &(mesg->heap_addr));
    	assert(H5F_addr_defined(mesg->heap_addr));

    	/* Decode the file list */
    	mesg->slot = calloc(mesg->nalloc, sizeof(H5O_efl_entry_t));
    	if (NULL == mesg->slot) {
		H5E_push("H5O_efl_decode", "Couldn't malloc() H5O_efl_entry_t.", -1);
		ret++;
		goto done;
	}

    	for (u = 0; u < mesg->nused; u++) {
        	/* Name offset */
        	H5F_DECODE_LENGTH (shared_info, p, mesg->slot[u].name_offset);
        	/* File offset */
        	H5F_DECODE_LENGTH (shared_info, p, mesg->slot[u].offset);
        	/* Size */
        	H5F_DECODE_LENGTH (shared_info, p, mesg->slot[u].size);
        	assert (mesg->slot[u].size>0);
    	}

    	ret_value = mesg;
done:
	if (ret) {
		if (mesg) {
			if (mesg->slot)	 free(mesg->slot);
		}
		ret_value = NULL;
	}
    	return(ret_value);
}






/* copied and modified from H5Olayout.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_layout_decode
 *
 * Purpose:     Decode an data layout message and return a pointer to a
 *              new one created with malloc().
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *      Robb Matzke, 1998-07-20
 *      Rearranged the message to add a version number at the beginning.
 *
 *      Raymond Lu, 2002-2-26
 *      Added version number 2 case depends on if space has been allocated
 *      at the moment when layout header message is updated.
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_layout_decode(const uint8_t *p)
{
    	H5O_layout_t    *mesg = NULL;
    	unsigned        u;
    	void            *ret_value;          /* Return value */
	int		ret;


    	/* check args */
    	assert(p);
	ret = SUCCEED;


    	/* decode */
	if ((mesg=calloc(1, sizeof(H5O_layout_t)))==NULL) {
		H5E_push("H5O_layout_decode", "Couldn't malloc() H5O_layout_t.", -1);
		return(ret_value=NULL);
	}

    	/* Version. 1 when space allocated; 2 when space allocation is delayed */
    	mesg->version = *p++;
    	if (mesg->version<H5O_LAYOUT_VERSION_1 || mesg->version>H5O_LAYOUT_VERSION_3) {
		H5E_push("H5O_layout_decode", "Bad version number for layout message.", -1);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

    	if(mesg->version < H5O_LAYOUT_VERSION_3) {  /* version 1 & 2 */
        	unsigned        ndims;    /* Num dimensions in chunk           */

        	/* Dimensionality */
        	ndims = *p++;
		/* this is not specified in the format specification ??? */
        	if (ndims > H5O_LAYOUT_NDIMS) {
			H5E_push("H5O_layout_decode", "Dimensionality is too large.", -1);
			ret++;
		}

        	/* Layout class */
        	mesg->type = (H5D_layout_t)*p++;
        	assert(H5D_CONTIGUOUS == mesg->type || H5D_CHUNKED == mesg->type || H5D_COMPACT == mesg->type);

        	/* Reserved bytes */
        	p += 5;

        	/* Address */
        	if(mesg->type==H5D_CONTIGUOUS) {
            		H5F_addr_decode(shared_info, &p, &(mesg->u.contig.addr));
        	} else if(mesg->type==H5D_CHUNKED) {
            		H5F_addr_decode(shared_info, &p, &(mesg->u.chunk.addr));
		}

        	/* Read the size */
        	if(mesg->type!=H5D_CHUNKED) {
            		mesg->unused.ndims=ndims;

            		for (u = 0; u < ndims; u++)
                		UINT32DECODE(p, mesg->unused.dim[u]);

            	/* Don't compute size of contiguous storage here, due to possible
             	 * truncation of the dimension sizes when they were stored in this
             	 * version of the layout message.  Compute the contiguous storage
             	 * size in the dataset code, where we've got the dataspace
             	 * information available also.  - QAK 5/26/04
             	 */
        	} else {
            		mesg->u.chunk.ndims=ndims;
            		for (u = 0; u < ndims; u++) {
                		UINT32DECODE(p, mesg->u.chunk.dim[u]);
			}

            		/* Compute chunk size */
            		for (u=1, mesg->u.chunk.size=mesg->u.chunk.dim[0]; u<ndims; u++)
                		mesg->u.chunk.size *= mesg->u.chunk.dim[u];

#ifdef DEBUG
printf("CHUNKED_STORAGE:btree address=%lld, chunk size=%d, ndims=%d\n", 
	mesg->u.chunk.addr, mesg->u.chunk.size, mesg->u.chunk.ndims);
#endif
        	}

        	if(mesg->type == H5D_COMPACT) {
            		UINT32DECODE(p, mesg->u.compact.size);
            		if(mesg->u.compact.size > 0) {
                		if(NULL==(mesg->u.compact.buf=malloc(mesg->u.compact.size))) {
					H5E_push("H5O_layout_decode", "Couldn't malloc() compact storage buffer.", -1);
					return(ret_value=NULL);
				}

                		memcpy(mesg->u.compact.buf, p, mesg->u.compact.size);
                		p += mesg->u.compact.size;
            		}
        	}
    	} else {  /* version 3 */
        	/* Layout class */
        	mesg->type = (H5D_layout_t)*p++;

        	/* Interpret the rest of the message according to the layout class */
        	switch(mesg->type) {
            	  case H5D_CONTIGUOUS:
                  	H5F_addr_decode(shared_info, &p, &(mesg->u.contig.addr));
                	H5F_DECODE_LENGTH(shared_info, p, mesg->u.contig.size);
                	break;

               	  case H5D_CHUNKED:
                   	/* Dimensionality */
                    	mesg->u.chunk.ndims = *p++;
                	if (mesg->u.chunk.ndims>H5O_LAYOUT_NDIMS) {
				H5E_push("H5O_layout_decode", "Dimensionality is too large.", -1);
				ret++;
			}

                	/* B-tree address */
                	H5F_addr_decode(shared_info, &p, &(mesg->u.chunk.addr));

                	/* Chunk dimensions */
                	for (u = 0; u < mesg->u.chunk.ndims; u++)
                    		UINT32DECODE(p, mesg->u.chunk.dim[u]);

                	/* Compute chunk size */
                	for (u=1, mesg->u.chunk.size=mesg->u.chunk.dim[0]; u<mesg->u.chunk.ndims; u++)
                    		mesg->u.chunk.size *= mesg->u.chunk.dim[u];
                	break;

            	  case H5D_COMPACT:
                	UINT16DECODE(p, mesg->u.compact.size);
                	if(mesg->u.compact.size > 0) {
                    		if(NULL==(mesg->u.compact.buf=malloc(mesg->u.compact.size))) {
					H5E_push("H5O_layout_decode", "Couldn't malloc() compact storage buffer.", -1);
					return(ret_value=NULL);
				}
                    		memcpy(mesg->u.compact.buf, p, mesg->u.compact.size);
                    		p += mesg->u.compact.size;
                	} /* end if */
                	break;

            	  default:
			H5E_push("H5O_layout_decode", "Invalid layout class.", -1);
			ret++;
        	} /* end switch */
    	} /* version 3 */

	if (ret)
		mesg = NULL;
    	/* Set return value */
    	ret_value=mesg;
    	return(ret_value);
}


/* copied and modified from H5Opline.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_pline_decode
 *
 * Purpose:     Decodes a filter pipeline message.
 *
 * Return:      Success:        Ptr to the native message.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April 15, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_pline_decode(const uint8_t *p)
{
    	H5O_pline_t     *pline = NULL;
    	void            *ret_value;
    	unsigned        version;
    	size_t          i, j, n, name_length;
	int		ret;

    	/* check args */
    	assert(p);
	ret = SUCCEED;

    	/* Decode */
    	if ((pline = calloc(1, sizeof(H5O_pline_t)))==NULL) {
		H5E_push("H5O_pline_decode", "Couldn't malloc() H5O_pline_t.", -1);
		ret++;
		goto done;
	}

    	version = *p++;
    	if (version!=H5O_PLINE_VERSION) {
		H5E_push("H5O_pline_decode", "Bad version number for filter pipeline message.", -1);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

    	pline->nused = *p++;
    	if (pline->nused > H5Z_MAX_NFILTERS) {
		H5E_push("H5O_pline_decode", "filter pipeline message has too many filters.", -1);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

    	p += 6;     /*reserved*/
    	pline->nalloc = pline->nused;
    	pline->filter = calloc(pline->nalloc, sizeof(pline->filter[0]));
    	if (pline->filter==NULL) {
		H5E_push("H5O_pline_decode", "Couldn't malloc() H5O_pline_t->filter.", -1);
		ret++;
		goto done;
	}

    	for (i = 0; i < pline->nused; i++) {
        	UINT16DECODE(p, pline->filter[i].id);
        	UINT16DECODE(p, name_length);
        	if (name_length % 8) {
			H5E_push("H5O_pline_decode", 
			  "filter name length is not a multiple of eight.", -1);
			ret++;
			goto done;
		}

        	UINT16DECODE(p, pline->filter[i].flags);
        	UINT16DECODE(p, pline->filter[i].cd_nelmts);
        	if (name_length) {
            		/*
             		 * Get the name, allocating an extra byte for an extra null
             		 * terminator just in case there isn't one in the file (there
             		 * should be, but to be safe...)
             		 */
            		pline->filter[i].name = malloc(name_length+1);
            		memcpy(pline->filter[i].name, p, name_length);
            		pline->filter[i].name[name_length] = '\0';
            		p += name_length;
        	}
        	if ((n=pline->filter[i].cd_nelmts)) {
            	/*
             	 * Read the client data values and the padding
             	 */
            	pline->filter[i].cd_values = malloc(n*sizeof(unsigned));
            	if (pline->filter[i].cd_values==NULL) {
			H5E_push("H5O_pline_decode", "Couldn't malloc() cd_values.", -1);
			ret++;
			goto done;
		}

            	for (j=0; j<pline->filter[i].cd_nelmts; j++)
                	UINT32DECODE(p, pline->filter[i].cd_values[j]);

            	if (pline->filter[i].cd_nelmts % 2)
               		p += 4; /*padding*/
        	}
    	}

    	/* Set return value */
    	ret_value = pline;

done:
	if (ret) {
		if (pline) {
			if (pline->filter) {
            			for (i=0; i<pline->nused; i++) {
                			free(pline->filter[i].name);
                			free(pline->filter[i].cd_values);
				}
				free(pline->filter);
            		}
			free(pline);
		}
		ret_value = NULL;
	}
	return(ret_value);
}




/* copied and modified from H5Oattr.c */
/*--------------------------------------------------------------------------
 NAME
    H5O_attr_decode
 PURPOSE
    Decode a attribute message and return a pointer to a memory struct
        with the decoded information
 USAGE
    void *H5O_attr_decode(f, raw_size, p)
        H5F_t *f;               IN: pointer to the HDF5 file struct
        size_t raw_size;        IN: size of the raw information buffer
        const uint8_t *p;         IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
        This function decodes the "raw" disk form of a attribute message
    into a struct in memory native format.  The struct is allocated within this
    function using malloc() and is returned to the caller.
 *
 * Modifications:
 *      Robb Matzke, 17 Jul 1998
 *      Added padding for alignment.
 *
 *      Robb Matzke, 20 Jul 1998
 *      Added a version number at the beginning.
 *
 *      Raymond Lu, 8 April 2004
 *      Changed Dataspace operation on H5S_simple_t to H5S_extent_t.
 *
--------------------------------------------------------------------------*/
static void *
H5O_attr_decode(const uint8_t *p)
{
    	H5A_t           *attr = NULL;
    	H5S_extent_t    *extent;        /*extent dimensionality information  */
    	size_t          name_len;       /*attribute name length */
    	int             version;        /*message version number*/
    	unsigned        unused_flags=0; /* Attribute flags */
    	H5A_t     	*ret_value;     /* Return value */
	int		ret;


    	assert(p);
	ret = SUCCEED;

    	if ((attr=calloc(1, sizeof(H5A_t))) == NULL) {
		H5E_push("H5O_attr_decode", "Couldn't calloc() H5A_t.", -1);
		ret++;
		goto done;
	}

    	/* Version number */
    	version = *p++;
	/* The format specification only describes version 1 of attribute messages */
    	if (version != H5O_ATTR_VERSION) {
		H5E_push("H5O_attr_decode", "Bad version number for Attribute message.", -1);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

    	/* version 1 does not support flags; it is reserved and set to zero */
        unused_flags = *p++;
	if (unused_flags != 0) {
		H5E_push("H5O_attr_decode", "Attribute flag is unused for version 1.", -1);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

    	/*
     	 * Decode the sizes of the parts of the attribute.  The sizes stored in
         * the file are exact but the parts are aligned on 8-byte boundaries.
     	 */
    	UINT16DECODE(p, name_len); /*including null*/
    	UINT16DECODE(p, attr->dt_size);
    	UINT16DECODE(p, attr->ds_size);
/* SHOULD I CHCK FOR THOSE 2 fields above to be greater than 0 or >=??? */

 	/* Decode and store the name */
	attr->name = NULL;
	if (name_len != 0) {
		if ((attr->name = malloc(name_len)) == NULL) {
			H5E_push("H5O_attr_decode", "Couldn't malloc() space for attribute name.", -1);
			ret++;
			goto done;
		}
		strcpy(attr->name, (const char *)p);
		/* should be null-terminated */
		if (attr->name[name_len-1] != '\0') {
			H5E_push("H5O_attr_decode", "Attribute name should be null-terminated.", -1);
			ret++;
		}
	}

        p += H5O_ALIGN(name_len);    /* advance the memory pointer */


        if((attr->dt=(H5O_DTYPE->decode)(p))==NULL) {
		H5E_push("H5O_attr_decode", "Can't decode attribute datatype.", -1);
		ret++;
		goto done;
	}

        p += H5O_ALIGN(attr->dt_size);

    	/* decode the attribute dataspace */
	/* ??? there is select info in the structure */
    	if ((attr->ds = calloc(1, sizeof(H5S_t)))==NULL) {
		H5E_push("H5O_attr_decode", "Couldn't malloc() H5S_t.", -1);
		ret++;
		goto done;
	}

    	if((extent=(H5O_SDSPACE->decode)(p))==NULL) {
		H5E_push("H5O_attr_decode", "Can't decode attribute dataspace.", -1);
		ret++;
		goto done;
	}

    	/* Copy the extent information */
    	memcpy(&(attr->ds->extent),extent, sizeof(H5S_extent_t));

 	/* Release temporary extent information */ 
	if (extent)
    		free(extent);

    	/* Compute the size of the data */
	attr->data_size = attr->ds->extent.nelem * attr->dt->shared->size;
    	H5_ASSIGN_OVERFLOW(attr->data_size,attr->ds->extent.nelem*attr->dt->shared->size,hsize_t,size_t);

        p += H5O_ALIGN(attr->ds_size);
    	/* Go get the data */
    	if(attr->data_size) {
		if ((attr->data = malloc(attr->data_size))==NULL) {
			H5E_push("H5O_attr_decode", "Couldn't malloc() space for attribute data", -1);
			ret++;
			goto done;
		}
		/* How the data is interpreted is not stated in the format specification. */
		memcpy(attr->data, p, attr->data_size);
	}

    	ret_value = attr;
done:
	if (ret) {
		if (attr) {
			if (attr->name) free(attr->name);
			if (attr->ds) free(attr->ds);
			if (attr->data) free(attr->data);
			free(attr);
		}
		ret_value = NULL;
	}
	return(ret_value);
}


/* copied and modified from H5Oname.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_name_decode
 *
 * Purpose:     Decode a name message and return a pointer to a new
 *              native message struct.
 *
 * Return:      Success:        Ptr to new message in native struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_name_decode(const uint8_t *p)
{
	H5O_name_t          *mesg;
    	void                *ret_value;     /* Return value */
	int		    len;


    	/* check args */
    	assert(p);

	len = strlen((const char *)p);
    	/* decode */
    	if ((mesg=calloc(1, sizeof(H5O_name_t))) == NULL ||
            (mesg->s = malloc(len+1))==NULL) {
		H5E_push("H5O_name_decode", "Couldn't malloc() H5O_name_t or comment string.", -1);
		ret_value = NULL;
		goto done;
	}
    	strcpy(mesg->s, (const char*)p);
	if (mesg->s[len] != '\0') {
		H5E_push("H5O_name_decode", "The comment string should be null-terminated.", -1);
		ret_value = NULL;
		goto done;
	}


    	/* Set return value */
    	ret_value = mesg;

done:
    	if(ret_value == NULL) {
		if (mesg) {
			if (mesg->s) free(mesg->s);
        		free(mesg);
		}
    	}
	return(ret_value);
}



/*-------------------------------------------------------------------------
 * Function:    H5O_shared_decode
 *
 * Purpose:     Decodes a shared object message and returns it.
 *
 * Return:      Success:        Ptr to a new shared object message.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  2, 1998
 *
 * Modifications:
 *      Robb Matzke, 1998-07-20
 *      Added a version number to the beginning of the message.
 *-------------------------------------------------------------------------
 */
static void *
H5O_shared_decode (const uint8_t *buf)
{
    	H5O_shared_t    *mesg=NULL;
    	unsigned        flags, version;
    	void            *ret_value;     /* Return value */
	int		ret=SUCCEED;


    	/* Check args */
    	assert (buf);

    	/* Decode */
    	if ((mesg = calloc(1, sizeof(H5O_shared_t)))==NULL) {
		H5E_push("H5O_shared_decode", "Couldn't malloc() H5O_shared_t.", -1);
		ret++;
		goto done;
 	}

    	/* Version */
    	version = *buf++;

	/* ????NOT IN SPECF FOR DIFFERENT VERSIONS */
	/* SHOULD be only version 1 is described in the spec */
    	if (version != H5O_SHARED_VERSION_1 && version != H5O_SHARED_VERSION) {
		H5E_push("H5O_shared_decode", "Bad version number for shared object message.", -1);
		ret++;
		goto done;
	}

    	/* Get the shared information flags */
    	flags = *buf++;
    	mesg->in_gh = (flags & 0x01);

#ifdef DEBUG
if (mesg->in_gh)
	printf("IN GLOBAL HEAP\n");
else
	printf("NOT IN GLOBAL HEAP\n");
#endif

	if ((flags>>1) != 0) {  /* should be reserved(zero) */
		H5E_push("H5O_shared_decode", "Bits 1-7 should be 0 for shared Object flags.", -1);
		ret++;
	}

    	/* Skip reserved bytes (for version 1) */
    	if(version==H5O_SHARED_VERSION_1)
        	buf += 6;

    	/* Body */
    	if (mesg->in_gh) {
		/* NEED TO HANDLE GLOBAL HEAP HERE  and also validation */
        	H5F_addr_decode (shared_info, &buf, &(mesg->u.gh.addr));
		/* need to valiate idx not exceeding size here?? */
        	INT32DECODE(buf, mesg->u.gh.idx);
    	}
    	else {
        	if(version==H5O_SHARED_VERSION_1)
			/* ??supposedly validate in that routine but not, need to check */
            		H5G_ent_decode (shared_info, &buf, &(mesg->u.ent));
        	else {
            		assert(version==H5O_SHARED_VERSION);
            		H5F_addr_decode(shared_info, &buf, &(mesg->u.ent.header));
			if ((mesg->u.ent.header == HADDR_UNDEF) || (mesg->u.ent.header >= shared_info.stored_eoa)) {
				H5E_push("H5O_shared_decode", "Invalid object header address.", -1);
				ret++;
				goto done;
			}
#ifdef DEBUG
			printf("header =%lld\n", mesg->u.ent.header);
#endif
        	} /* end else */
    	}

    	ret_value = mesg;
done:
	if (ret) {
		if (mesg != NULL) free(mesg);
		ret_value = NULL;
	}
	return(ret_value);
}



/* copied and modified from H5Ocont.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_cont_decode
 *
 * Purpose:     Decode the raw header continuation message.
 *
 * Return:      Success:        Ptr to the new native message
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_cont_decode(const uint8_t *p)
{
    	H5O_cont_t 	*cont = NULL;
    	void       	*ret_value;
	int		ret;

    	assert(p);
	ret = SUCCEED;

    	/* decode */
	cont = malloc(sizeof(H5O_cont_t));
	if (cont == NULL) {
		H5E_push("H5O_cont_decode", "Couldn't malloc() H5O_cont_t.", -1);
		ret++;
		goto done;
	}
	
    	H5F_addr_decode(shared_info, &p, &(cont->addr));

	if ((cont->addr == HADDR_UNDEF) || (cont->addr >= shared_info.stored_eoa)) {
		H5E_push("H5O_cont_decode", "Invalid header continuation offset.", -1);
		ret++;
	}

    	H5F_DECODE_LENGTH(shared_info, p, cont->size);

	if (cont->size < 0) {
		H5E_push("H5O_cont_decode", "Invalid header continuation length.", -1);
		ret++;
	}

    	cont->chunkno = 0;

	ret_value = cont;
done:
	if (ret) {
		if (cont) free(cont);
		ret_value = NULL;
	}
    	return(ret_value);
}


/* copied and modified from H5Ostab.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_stab_decode
 *
 * Purpose:     Decode a symbol table message and return a pointer to
 *              a newly allocated one.
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_stab_decode(const uint8_t *p)
{
    	H5O_stab_t  	*stab=NULL;
    	void    	*ret_value;     /* Return value */
	int		ret;

    	assert(p);
	ret = SUCCEED;

    	/* decode */
	stab = malloc(sizeof(H5O_stab_t));
	if (stab == NULL) {
		H5E_push("H5O_stab_decode", "Couldn't malloc() H5O_stab_t.", -1);
		ret++;
		goto done;
	}

	H5F_addr_decode(shared_info, &p, &(stab->btree_addr));
	if ((stab->btree_addr == HADDR_UNDEF) || (stab->btree_addr >= shared_info.stored_eoa)) {
		H5E_push("H5O_stab_decode", "Invalid btree address.", -1);
		ret++;
	}
	H5F_addr_decode(shared_info, &p, &(stab->heap_addr));
	if ((stab->heap_addr == HADDR_UNDEF) || (stab->heap_addr >= shared_info.stored_eoa)) {
		H5E_push("H5O_stab_decode", "Invalid heap address.", -1);
		ret++;
	}

	ret_value = stab;
done:
	if (ret) {
		if (stab) free(stab);
		ret_value = NULL;
	}
    	return(ret_value);
}







/*-------------------------------------------------------------------------
 * Function:    H5O_mtime_new_decode
 *
 * Purpose:     Decode a new modification time message and return a pointer to a
 *              new time_t value.
 *
 * Return:      Success:        Ptr to new message in native struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Jan  3 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_mtime_new_decode(const uint8_t *p)
{
    	time_t      	*mesg;
    	uint32_t    	tmp_time;       /* Temporary copy of the time */
    	void        	*ret_value;     /* Return value */
	int		ret;


    	assert(p);
	ret = SUCCEED;

    	/* decode */
    	if(*p++ != H5O_MTIME_VERSION) {
		H5E_push("H5O_mtime_new_decode", "Bad version number for mtime message.", -1);
		ret++;
	}

    	/* Skip reserved bytes */
    	p+=3;

    	/* Get the time_t from the file */
    	UINT32DECODE(p, tmp_time);

    	/* The return value */
    	if ((mesg=malloc(sizeof(time_t))) == NULL) {
		H5E_push("H5O_mtime_new_decode", "Couldn't malloc() time_t.", -1);
		ret++;
		goto done;
	}
    	*mesg = (time_t)tmp_time;

    	/* Set return value */
    	ret_value = mesg;
done:
	if (ret) {
		if (mesg) free(mesg);
		ret_value = NULL;
	}
	/* Set return value */
    	return(ret_value);
}

/* copied and modified from H5T.c */
/*-------------------------------------------------------------------------
 * Function:    H5T_alloc
 *
 * Purpose:     Allocates a new H5T_t structure, initializing it correctly.
 *
 * Return:      Pointer to new H5T_t on success/NULL on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, August 29, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_alloc(void)
{
    H5T_t *dt;                  /* Pointer to datatype allocated */
    H5T_t *ret_value;           /* Return value */


    	/* Allocate & initialize new datatype info */
    	if((dt=calloc(1, sizeof(H5T_t)))==NULL) {
              	H5E_push("H5T_alloc", "Couldn't calloc() H5T_t.", -1);
		return(ret_value=NULL);
	}

	assert(&(dt->ent));
    	memset(&(dt->ent), 0, sizeof(H5G_entry_t));
	dt->ent.header = HADDR_UNDEF;

    	if((dt->shared=calloc(1, sizeof(H5T_shared_t)))==NULL) {
		if (dt != NULL) free(dt);
              	H5E_push("H5T_alloc", "Couldn't calloc() H5T_shared_t.", -1);
		return(ret_value=NULL);
	}
    
    	/* Assign return value */
    	ret_value = dt;
    	return(ret_value);
}



/* copied and modified from H5Gnode.c */
/*-------------------------------------------------------------------------
 * Function:    H5G_node_size
 *
 * Purpose:     Returns the total size of a symbol table node.
 *
 * Return:      Success:        Total size of the node in bytes.
 *
 *              Failure:        Never fails.
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jun 23 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5G_node_size(H5F_shared_t shared_info)
{

    return (H5G_NODE_SIZEOF_HDR(shared_info) +
            (2 * H5F_SYM_LEAF_K(shared_info)) * H5G_SIZEOF_ENTRY(shared_info));
}

/*
 *  Copied and modified from H5G_ent_decode() of H5Gent.c
 */
/*-------------------------------------------------------------------------
 * Function:    H5G_ent_decode
 *
 * Purpose:     Decodes a symbol table entry pointed to by `*pp'.
 *
 * Errors:
 *
 * Return:      Success:        Non-negative with *pp pointing to the first byte
 *                              following the symbol table entry.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *      Robb Matzke, 17 Jul 1998
 *      Added a 4-byte padding field for alignment and future expansion.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_decode(H5F_shared_t shared_info, const uint8_t **pp, H5G_entry_t *ent)
{
    	const uint8_t   *p_ret = *pp;
    	uint32_t        tmp;
	herr_t		ret = SUCCEED;

    	assert(pp);
    	assert(ent);

    	/* decode header */
    	H5F_DECODE_LENGTH(shared_info, *pp, ent->name_off);
    	H5F_addr_decode(shared_info, pp, &(ent->header));
    	UINT32DECODE(*pp, tmp);
    	*pp += 4; /*reserved*/
    	ent->type=(H5G_type_t)tmp;

    	/* decode scratch-pad */
    	switch (ent->type) {
        case H5G_NOTHING_CACHED:
            	break;

        case H5G_CACHED_STAB:
            	assert(2*H5F_SIZEOF_ADDR(shared_info) <= H5G_SIZEOF_SCRATCH);
            	H5F_addr_decode(shared_info, pp, &(ent->cache.stab.btree_addr));
            	H5F_addr_decode(shared_info, pp, &(ent->cache.stab.heap_addr));
            	break;

        case H5G_CACHED_SLINK:
            	UINT32DECODE (*pp, ent->cache.slink.lval_offset);
            	break;

        default:  /* allows other cache values; will be validated later */
            	break;
    	}

    	*pp = p_ret + H5G_SIZEOF_ENTRY(shared_info);
	return(SUCCEED);
}


/* copied and modified from H5Gent.c */
/*-------------------------------------------------------------------------
 * Function:    H5G_ent_decode_vec
 *
 * Purpose:     Same as H5G_ent_decode() except it does it for an array of
 *              symbol table entries.
 *
 * Errors:
 *              SYM       CANTDECODE    Can't decode.
 *
 * Return:      Success:        Non-negative, with *pp pointing to the first byte
 *                              after the last symbol.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_decode_vec(H5F_shared_t shared_info, const uint8_t **pp, H5G_entry_t *ent, unsigned n)
{
    	unsigned    u;
    	herr_t      ret_value=SUCCEED;       /* Return value */

    	/* check arguments */
    	assert(pp);
    	assert(ent);

    	for (u = 0; u < n; u++)
        	if (ret_value=H5G_ent_decode(shared_info, pp, ent + u) < 0) {
			H5E_push("H5G_ent_decode_vec", "Unable to read symbol table entries.", -1);
            		return(ret_value);
		}

	return(ret_value);
}

/*
 *  Copied and modified from H5F_locate_signature() of H5F.c
 */
/*-------------------------------------------------------------------------
 * Function:    locate_super_signature
 *
 * Purpose:     Finds the HDF5 super block signature in a file. The signature
 *              can appear at address 0, or any power of two beginning with
 *              512.
 *
 * Return:      Success:        The absolute format address of the signature.
 *
 *              Failure:        HADDR_UNDEF
 *
 *-------------------------------------------------------------------------
 */

haddr_t
locate_super_signature(FILE *inputfd)
{
    	haddr_t         addr;
    	uint8_t         buf[H5F_SIGNATURE_LEN];
    	unsigned        n, maxpow;
	int		ret;

	addr = 0;
	/* find file size */
	if (fseek(inputfd, 0, SEEK_END) < 0) {
		H5E_push("locate_super_signature", "Failure in seeking to the end of file.", -1);
		return(HADDR_UNDEF);
	}
	ret = ftell(inputfd);
	if (ret < 0) {
		H5E_push("locate_super_signature", "Failure in finding the end of file.", -1);
		return(HADDR_UNDEF);
	}
	addr = (haddr_t)ret;
	rewind(inputfd);
	
    	/* Find the least N such that 2^N is larger than the file size */
    	for (maxpow=0; addr; maxpow++)
        	addr>>=1;
    	maxpow = MAX(maxpow, 9);

    	/*
     	 * Search for the file signature at format address zero followed by
     	 * powers of two larger than 9.
     	 */
    	for (n=8; n<maxpow; n++) {
        	addr = (8==n) ? 0 : (haddr_t)1 << n;
		fseek(inputfd, addr, SEEK_SET);
        	if (fread(buf, 1, (size_t)H5F_SIGNATURE_LEN, inputfd)<0) {
			H5E_push("locate_super_signature", "Unable to read super block signature.", -1);
			addr = HADDR_UNDEF;
			break;
		}
		ret = memcmp(buf, H5F_SIGNATURE, (size_t)H5F_SIGNATURE_LEN);
		if (ret == 0) {
			printf("FOUND super block signature\n");
            		break;
		}
    	}
	if (n >= maxpow) {
		H5E_push("locate_super_signature", "Unable to find super block signature.", -1);
		addr = HADDR_UNDEF;
	}
	rewind(inputfd);

    	/* Set return value */
    	return(addr);

}


/* copied and modified from H5F_addr_decode() of H5F.c */
/*-------------------------------------------------------------------------
 * Function:    H5F_addr_decode
 *
 * Purpose:     Decodes an address from the buffer pointed to by *PP and
 *              updates the pointer to point to the next byte after the
 *              address.
 *
 *              If the value read is all 1's then the address is returned
 *              with an undefined value.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Friday, November  7, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5F_addr_decode(H5F_shared_t shared_info, const uint8_t **pp/*in,out*/, haddr_t *addr_p/*out*/)
{
    	unsigned                i;
    	haddr_t                 tmp;
    	uint8_t                 c;
    	hbool_t                 all_zero = TRUE;
	haddr_t			ret;

    	assert(pp && *pp);
    	assert(addr_p);

    	*addr_p = 0;

    	for (i=0; i<H5F_SIZEOF_ADDR(shared_info); i++) {
        	c = *(*pp)++;
        	if (c != 0xff)
            		all_zero = FALSE;

        	if (i<sizeof(*addr_p)) {
            		tmp = c;
            		tmp <<= (i * 8);    /*use tmp to get casting right */
            		*addr_p |= tmp;
        	} else if (!all_zero) {
            		assert(0 == **pp);  /*overflow */
        	}
    	}
    	if (all_zero)
        	*addr_p = HADDR_UNDEF;
}


/* copied and modified from H5Gnode.c */
/*-------------------------------------------------------------------------
 * Function:    H5G_node_sizeof_rkey
 *
 * Purpose:     Returns the size of a raw B-link tree key for the specified
 *              file.
 *
 * Return:      Success:        Size of the key.
 *
 *              Failure:        never fails
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 14 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5G_node_sizeof_rkey(H5F_shared_t shared_info, unsigned UNUSED)
{

    return (H5F_SIZEOF_SIZE(shared_info));       /*the name offset */
}

/*-------------------------------------------------------------------------
 * Function:    H5G_node_decode_key
 *
 * Purpose:     Decodes a raw key into a native key.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  8 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5G_node_decode_key(H5F_shared_t shared_info, unsigned UNUSED, const uint8_t **p/*in,out*/)
{
	H5G_node_key_t	*key;
	void		*ret_value;
	int		ret;

    	assert(p);
	assert(*p);
	ret = 0;

    	/* decode */
    	key = calloc(1, sizeof(H5G_node_key_t));
	if (key == NULL) {
		H5E_push("H5G_node_decode_key", "Couldn't malloc() H5G_node_key_t.", -1);
		return(ret_value=NULL);
	}

    	H5F_DECODE_LENGTH(shared_info, *p, key->offset);

    	/* Set return value */
    	ret_value = (void*)key;    /*success*/
    	return(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5D_istore_decode_key
 *
 * Purpose:     Decodes a raw key into a native key for the B-tree
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, October 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5D_istore_decode_key(H5F_shared_t shared_info, size_t ndims, const uint8_t **p)
{

    	H5D_istore_key_t    	*key = NULL;
    	unsigned            	u;
	void			*ret_value;
	int			ret;

    	/* check args */
    	assert(p);
	assert(*p);
	assert(ndims>0);
    	assert(ndims<=H5O_LAYOUT_NDIMS);
	ret = 0;

    	/* decode */
    	key = calloc(1, sizeof(H5D_istore_key_t));
	if (key == NULL) {
		H5E_push("H5G_node_decode_key", "Couldn't malloc() H5G_istore_key_t.", -1);
		return(ret_value=NULL);
	}

    	UINT32DECODE(*p, key->nbytes);
    	UINT32DECODE(*p, key->filter_mask);
    	for (u=0; u<ndims; u++)
        	UINT64DECODE(*p, key->offset[u]);

    	/* Set return value */
    	ret_value = (void*)key;    /*success*/
    	return(ret_value);
}





/* copied and modified from H5Distore.c */
/*-------------------------------------------------------------------------
 * Function:    H5D_istore_sizeof_rkey
 *
 * Purpose:     Returns the size of a raw key for the specified UDATA.  The
 *              size of the key is dependent on the number of dimensions for
 *              the object to which this B-tree points.  The dimensionality
 *              of the UDATA is the only portion that's referenced here.
 *
 * Return:      Success:        Size of raw key in bytes.
 *
 *              Failure:        abort()
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static size_t
H5D_istore_sizeof_rkey(H5F_shared_t shared_data, unsigned ndims)
{
	
    	size_t 	nbytes;

	assert(ndims>0 && ndims<=H5O_LAYOUT_NDIMS);

    	nbytes = 4 +           /*storage size          */
             	 4 +           /*filter mask           */
             	 ndims*8;      /*dimension indices     */

	return(nbytes);
}

/*
 *  Virtual driver 
 */
void
set_driver(int *driverid, char *driver_name) 
{
	if (strcmp(driver_name, "NCSAmult") == 0)  {
		*driverid = MULTI_DRIVER;
		printf("The driver to be used is MULTI\n");
	} else {
		/* default driver */
		*driverid = SEC2_DRIVER;
#ifdef DEBUG
		printf("The driver to be used is SEC2\n");
#endif
	}
}

H5FD_class_t *
get_driver(int driver_id)
{
	H5FD_class_t	*driver = NULL;
	size_t		size;

	size = sizeof(H5FD_class_t);
	driver = malloc(size);
	
	switch (driver_id) {
	case SEC2_DRIVER:
		memcpy(driver, &H5FD_sec2_g, size);
		break;
	case MULTI_DRIVER:
		memcpy(driver, &H5FD_multi_g, size);
		break;
	default:
		printf("Something is wrong\n");
		break;
	}  /* end switch */

	return(driver);
}


void *
get_driver_info(int driver_id)
{
	H5FD_multi_fapl_t 	*driverinfo;
	size_t			size;

	switch (driver_id) {
	case SEC2_DRIVER:
		driverinfo = NULL;
		break;
	case MULTI_DRIVER:
		size = sizeof(H5FD_multi_fapl_t);
		driverinfo = malloc(size);
		memcpy(driverinfo, shared_info.fa, size);
		break;
	default:
		printf("Something is wrong\n");
		break;
	}  /* end switch */
	return(driverinfo);
}


/*
 * A modified version: the .h5 file is opened using fread etc.
 * There is no FD_open() done yet at this point.
 */
static herr_t
decode_driver(H5F_shared_t * _shared_info, const uint8_t *buf)
{
	if (_shared_info->driverid == MULTI_DRIVER)
		H5FD_multi_sb_decode(_shared_info, buf);
}

static H5FD_t *
H5FD_open(const char *name, int driver_id)
{
	H5FD_t	*file = NULL;
	H5FD_class_t 	*driver;

/* probably check for callbck functions exists here too or in set_driver */
	driver = get_driver(driver_id);
	file = driver->open(name, driver_id);
	if (file != NULL) {
		file->cls = driver;
		file->driver_id = driver_id;
	}
	return(file);
}

static herr_t
H5FD_close(H5FD_t *_file)
{
	int	ret = SUCCEED;

	if(_file->cls->close(_file) == FAIL)
		ret = FAIL;
	return(ret);
}

static herr_t
H5FD_read(H5FD_t *_file, haddr_t addr, size_t size, void *buf/*out*/)
{
	int	ret = SUCCEED;

	if (_file->cls->read(_file, addr, size, buf) == FAIL)
		ret = FAIL;

	return(ret);
}

haddr_t
H5FD_get_eof(H5FD_t *file)
{
    	haddr_t     ret_value;

    	assert(file && file->cls);

    	if (file->cls->get_eof)
		ret_value = file->cls->get_eof(file);
	return(ret_value);
#if 0
    /* Dispatch to driver */
    if (file->cls->get_eof) {
        if (HADDR_UNDEF==(ret_value=(file->cls->get_eof)(file)))
            printf("driver get_eof request failed");
    } else {
        ret_value = file->maxaddr;
    }
#endif
}



/*
 *  sec2 driver
 */
/* copied and modified from H5FDsec2.c */
static H5FD_t *
H5FD_sec2_open(const char *name, int driver_id)
{
	H5FD_sec2_t	*file = NULL;
	H5FD_t		*ret_value;
	int		fd = -1;
	struct stat	sb;

	/* do unix open here */
	fd = open(name, O_RDONLY);
	if (fd < 0) {
		H5E_push("H5FD_sec2_open", "Unable to open the file.", -1);
		goto done;
	}
	if (fstat(fd, &sb)<0) {
		H5E_push("H5FD_sec2_open", "Unable to fstat file.", -1);
		goto done;
	}
	file = calloc(1, sizeof(H5FD_sec2_t));
	file->fd = fd;
	file->eof = sb.st_size;
done:
	ret_value = (H5FD_t *)file;
	return(ret_value);
}

static haddr_t
H5FD_sec2_get_eof(H5FD_t *_file)
{
    H5FD_sec2_t *file = (H5FD_sec2_t*)_file;
 
    return(file->eof);

}

static herr_t
H5FD_sec2_close(H5FD_t *_file)
{
	int	ret;

	ret = close(((H5FD_sec2_t *)_file)->fd);
	return(ret);
}


static herr_t
H5FD_sec2_read(H5FD_t *_file, haddr_t addr, size_t size, void *buf/*out*/)
{
	int	status;
	int	ret = SUCCEED;
	int	fd;

	/* NEED To find out about lseek64??? */
	fd = ((H5FD_sec2_t *)_file)->fd;
	lseek(fd, addr, SEEK_SET);
       	if (read(fd, buf, size)<0)
		ret = FAIL;
}


/*
 *  MULTI driver
 */
/* copied and modified from H5FDmulti.c */

/* Loop through all mapped files */
#define UNIQUE_MEMBERS(MAP,LOOPVAR) {                                         \
    H5FD_mem_t _unmapped, LOOPVAR;                                            \
    hbool_t _seen[H5FD_MEM_NTYPES];                                           \
                                                                              \
    memset(_seen, 0, sizeof _seen);                                           \
    for (_unmapped=H5FD_MEM_SUPER; _unmapped<H5FD_MEM_NTYPES; _unmapped=(H5FD_mem_t)(_unmapped+1)) {  \
        LOOPVAR = MAP[_unmapped];                                             \
        if (H5FD_MEM_DEFAULT==LOOPVAR) LOOPVAR=_unmapped;                     \
        assert(LOOPVAR>0 && LOOPVAR<H5FD_MEM_NTYPES);                         \
        if (_seen[LOOPVAR]++) continue;


#define ALL_MEMBERS(LOOPVAR) {                                                \
    H5FD_mem_t LOOPVAR;                                                       \
    for (LOOPVAR=H5FD_MEM_DEFAULT; LOOPVAR<H5FD_MEM_NTYPES; LOOPVAR=(H5FD_mem_t)(LOOPVAR+1)) {


#define END_MEMBERS     }}


void
set_multi_driver_properties(H5FD_multi_fapl_t **fa, H5FD_mem_t map[], const char *memb_name[], haddr_t memb_addr[])
{
	*fa = malloc(sizeof(H5FD_multi_fapl_t));
        /* Commit map */
        ALL_MEMBERS(mt) {
            	(*fa)->memb_map[mt] = map[mt];
	    	(*fa)->memb_addr[mt] = memb_addr[mt];
		if (memb_name[mt]) {
	    		(*fa)->memb_name[mt] = malloc(strlen(memb_name[mt])+1);
	    		strcpy((*fa)->memb_name[mt], memb_name[mt]);
		} else 
			(*fa)->memb_name[mt] = NULL;
        } END_MEMBERS;
}

static herr_t
H5FD_multi_sb_decode(H5F_shared_t * _shared_info, const unsigned char *buf)
{
    	char                x[2*H5FD_MEM_NTYPES*8];
    	H5FD_mem_t          map[H5FD_MEM_NTYPES];
    	int                 i;
    	size_t              nseen=0;
    	const char          *memb_name[H5FD_MEM_NTYPES];
    	haddr_t             memb_addr[H5FD_MEM_NTYPES];
    	haddr_t             memb_eoa[H5FD_MEM_NTYPES];
	haddr_t	xx;
/* handle memb_eoa[]?? */
/* handle mt_in use??? */


    	/* Set default values */
    	ALL_MEMBERS(mt) {
        	memb_addr[mt] = HADDR_UNDEF;
        	memb_eoa[mt] = HADDR_UNDEF;
        	memb_name[mt] = NULL;
    	} END_MEMBERS;

    	/*
     	 * Read the map and count the unique members.
      	 */
    	memset(map, 0, sizeof map);
    	for (i=0; i<6; i++) {
        	map[i+1] = (H5FD_mem_t)buf[i];
    	}
    	UNIQUE_MEMBERS(map, mt) {
        	nseen++;
    	} END_MEMBERS;
    	buf += 8;

    	/* Decode Address and EOA values */
    	assert(sizeof(haddr_t)<=8);
    	UNIQUE_MEMBERS(map, mt) {
        	UINT64DECODE(buf, xx);
        	memb_addr[_unmapped] = xx;
        	UINT64DECODE(buf, xx);
        	memb_eoa[_unmapped] = xx;
#ifdef DEBUG
		printf("memb_addr[]=%llu;memb_eoa[]=%llu\n", 
			memb_addr[_unmapped], memb_eoa[_unmapped]);
#endif
    	} END_MEMBERS;

    	/* Decode name templates */
    	UNIQUE_MEMBERS(map, mt) {
        	size_t n = strlen((const char *)buf)+1;
        	memb_name[_unmapped] = (const char *)buf;
#ifdef DEBUG
		printf("memb_name=%s\n", memb_name[_unmapped]);
#endif
        	buf += (n+7) & ~((unsigned)0x0007);
    	} END_MEMBERS;

	set_multi_driver_properties((H5FD_multi_fapl_t **)&(_shared_info->fa), map, memb_name, memb_addr);
    	return 0;
}

static H5FD_t *
H5FD_multi_open(const char *name, int driver_id)
{
    	H5FD_multi_t        	*file=NULL;
    	H5FD_multi_fapl_t   	*fa;
    	H5FD_mem_t          	m;

	H5FD_t			*ret_value;
	int			len, ret;


	ret = SUCCEED;

    	/* Check arguments */
    	if (!name || !*name) {
		H5E_push("H5FD_multi_open", "Invalid file name.", -1);
		goto error;
	}

    	/*
     	 * Initialize the file from the file access properties, using default
       	 * values if necessary.
      	 */
    	if (NULL==(file=calloc(1, sizeof(H5FD_multi_t)))) {
		H5E_push("H5FD_multi_open", "Memory allocation failed.", -1);
		goto error;
	}

	fa = get_driver_info(driver_id);
    	assert(fa);
    	ALL_MEMBERS(mt) {
        	file->fa.memb_map[mt] = fa->memb_map[mt];
        	file->fa.memb_addr[mt] = fa->memb_addr[mt];
        	if (fa->memb_name[mt]) {
			file->fa.memb_name[mt] = malloc(strlen(fa->memb_name[mt]));
            		strcpy(file->fa.memb_name[mt], fa->memb_name[mt]);
        	} else
            		file->fa.memb_name[mt] = NULL;
    	} END_MEMBERS;

	file->name = malloc(strlen(name)+1);
    	strcpy(file->name, name);

    	if (compute_next(file) <0 )
        	printf("compute_next() failed\n");
    	if (open_members(file) != SUCCEED) {
		H5E_push("H5FD_multi_open", "Unable to open member files.", -1);
		goto error;
	}

/* NEED To checkon this also, make shared_info to be _shared_info for pointer */
/* need to check on return values for driver routines */
/* error check at the end of each driver routine, also, free malloc space */
#if 0
    if (H5FD_MEM_DEFAULT==(m=file->fa.memb_map[H5FD_MEM_SUPER]))
        m = H5FD_MEM_SUPER;
    if (NULL==file->memb[m])
        goto error;
#endif

	ret_value = (H5FD_t *)file;
	return(ret_value);

error:
    	/* Cleanup and fail */
    	if (file) {
       	 	ALL_MEMBERS(mt) {
            		if (file->memb[mt]) (void)H5FD_close(file->memb[mt]);
            		if (file->fa.memb_name[mt]) free(file->fa.memb_name[mt]);
        	} END_MEMBERS;
        	if (file->name) free(file->name);
        	free(file);
    	}
    	return NULL;
}

static herr_t 
H5FD_multi_close(H5FD_t *_file)
{
 	H5FD_multi_t   	*file = (H5FD_multi_t*)_file;
	int		nerrors = 0;


    /* Close as many members as possible */
    	ALL_MEMBERS(mt) {
        	if (file->memb[mt]) {
            		if (H5FD_close(file->memb[mt])<0)
                		nerrors++;
            	else
                	file->memb[mt] = NULL;
		}
	} END_MEMBERS;

    	if (nerrors) {
		H5E_push("H5FD_multi_close", "Error closing member file(s).", -1);
		return(FAIL);
	}

    	/* Clean up other stuff */
    	ALL_MEMBERS(mt) {
        if (file->fa.memb_name[mt]) 
		free(file->fa.memb_name[mt]);
    	} END_MEMBERS;
    	free(file->name);
    	free(file);
    	return 0;
}

static int
compute_next(H5FD_multi_t *file)
{

    ALL_MEMBERS(mt) {
        file->memb_next[mt] = HADDR_UNDEF;
    } END_MEMBERS;

    UNIQUE_MEMBERS(file->fa.memb_map, mt1) {
        UNIQUE_MEMBERS(file->fa.memb_map, mt2) {
            if (file->fa.memb_addr[mt1]<file->fa.memb_addr[mt2] &&
                (HADDR_UNDEF==file->memb_next[mt1] ||
                 file->memb_next[mt1]>file->fa.memb_addr[mt2])) {
                file->memb_next[mt1] = file->fa.memb_addr[mt2];
            }
        } END_MEMBERS;
        if (HADDR_UNDEF==file->memb_next[mt1]) {
            file->memb_next[mt1] = HADDR_MAX; /*last member*/
        }
    } END_MEMBERS;

    return 0;
}


static int
open_members(H5FD_multi_t *file)
{
    	char    tmp[1024], newname[1024];
	char	*ptr;
	int	ret;


	/* fix the name */
	strcpy(newname, file->name);
	ptr = strchr(newname, '-');
	*ptr = '\0';

	ret = SUCCEED;

    	UNIQUE_MEMBERS(file->fa.memb_map, mt) {
#if 0
        if (file->memb[mt]) continue; /*already open*/
#endif
        	assert(file->fa.memb_name[mt]);
        	sprintf(tmp, file->fa.memb_name[mt], newname);
            	file->memb[mt] = H5FD_open(tmp, SEC2_DRIVER);
		if (file->memb[mt] == NULL)
			ret = FAIL;
    	} END_MEMBERS;

    	return(ret);
}


static herr_t
H5FD_multi_read(H5FD_t *_file, haddr_t addr, size_t size, void *_buf/*out*/)
{
    	H5FD_multi_t        *file = (H5FD_multi_t*)_file;
    	H5FD_mem_t          mt, mmt, hi=H5FD_MEM_DEFAULT;
    	haddr_t             start_addr=0;

    	/* Find the file to which this address belongs */
    	for (mt=H5FD_MEM_SUPER; mt<H5FD_MEM_NTYPES; mt=(H5FD_mem_t)(mt+1)) {
        	mmt = file->fa.memb_map[mt];
        	if (H5FD_MEM_DEFAULT==mmt) 
			mmt = mt;
        	assert(mmt>0 && mmt<H5FD_MEM_NTYPES);

        	if (file->fa.memb_addr[mmt]>addr) 
			continue;
        	if (file->fa.memb_addr[mmt]>=start_addr) {
            		start_addr = file->fa.memb_addr[mmt];
            		hi = mmt;
        	}
    	}
    	assert(hi>0);

    	/* Read from that member */
    	return H5FD_read(file->memb[hi], addr-start_addr, size, _buf);
}


static haddr_t
H5FD_multi_get_eof(H5FD_t *_file)
{
    	H5FD_multi_t    *file = (H5FD_multi_t*)_file;
    	haddr_t         eof=0, tmp;

    	UNIQUE_MEMBERS(file->fa.memb_map, mt) {
        	if (file->memb[mt]) {
                	tmp = H5FD_get_eof(file->memb[mt]);
            		if (tmp == HADDR_UNDEF) {
				H5E_push("H5FD_multi_get_eof", "Member file has unknown eof.", -1);
				return(HADDR_UNDEF);
			}
            		if (tmp > 0) 
				tmp += file->fa.memb_addr[mt];
        	} else {
			H5E_push("H5FD_multi_get_eof", "Bad eof.", -1);
			return(HADDR_UNDEF);
        	}

        	if (tmp > eof) 
			eof = tmp;
    	} END_MEMBERS;

    return(eof);
}



/*
 *  FAMILY file driver
 */
static H5FD_t *
H5FD_family_open(const char *name, int driver_id)
{
	H5FD_family_t	*file = NULL;
	H5FD_t		*ret_value;

}

static herr_t
H5FD_family_close(H5FD_t *_file)
{
	/* NEEDTO CHANGE THAT */
	return(SUCCEED);
}


static herr_t
H5FD_family_read(H5FD_t *_file, haddr_t addr, size_t size, void *buf/*out*/)
{
	int	ret = SUCCEED;
	int	fd;

}

static haddr_t
H5FD_family_get_eof(H5FD_t *_file)
{

}



/* Based on H5F_read_superblock() in H5Fsuper.c */
/*
 * 1. Read in the information from the superblock
 * 2. Validate the information obtained.
 */
herr_t
check_superblock(FILE *inputfd, H5F_shared_t *_shared_info)
{
	uint		n;
	uint8_t		buf[H5F_SUPERBLOCK_SIZE];
	uint8_t		*p;
	herr_t		ret;
	char 		driver_name[9];

	/* fixed size portion of the super block layout */
	const	size_t	fixed_size = 24;    /* fixed size portion of the superblock */
	unsigned	super_vers;         /* version # of super block */
    	unsigned	freespace_vers;     /* version # of global free-space storage */
    	unsigned	root_sym_vers;      /* version # of root group symbol table entry */
    	unsigned	shared_head_vers;   /* version # of shared header message format */

	/* variable length part of the super block */
	/* the rest is in shared_info global structure */
	size_t		variable_size;	    /* variable size portion of the superblock */

	{ /* BEGIN SCANNING and STORING OF THE SUPER BLOCK INFO */
	/* 
	 * The super block may begin at 0, 512, 1024, 2048 etc. 
	 * or may not be there at all.
	 */
	ret = SUCCEED;
	_shared_info->super_addr = locate_super_signature(inputfd);
	if (_shared_info->super_addr == HADDR_UNDEF) {
		H5E_push("check_superblock", "Couldn't find super block.", -1);
		H5E_print(stderr);
		H5E_clear();
		printf("ASSUMING super block at address 0.\n");
		_shared_info->super_addr = 0;
	}
	
	printf("VALIDATING the super block at %llu...\n", _shared_info->super_addr);

	fseek(inputfd, _shared_info->super_addr, SEEK_SET);
	fread(buf, 1, fixed_size, inputfd);
	/* super block signature already checked */
	p = buf + H5F_SIGNATURE_LEN;

	super_vers = *p++;
	freespace_vers = *p++;
	root_sym_vers = *p++;
	/* skip over reserved byte */
	p++;

	shared_head_vers = *p++;
	_shared_info->size_offsets = *p++;
	_shared_info->size_lengths = *p++;
	/* skip over reserved byte */
	p++;

	UINT16DECODE(p, _shared_info->gr_leaf_node_k);
	UINT16DECODE(p, _shared_info->gr_int_node_k);

	/* File consistency flags. Not really used yet */
    	UINT32DECODE(p, _shared_info->file_consist_flg);


	/* decode the variable length part of the super block */
	/* Potential indexed storage internal node K */
    	variable_size = (super_vers>0 ? 4 : 0) + 
                    _shared_info->size_offsets +  /* base addr*/
                    _shared_info->size_offsets +  /* address of global free-space heap */
                    _shared_info->size_offsets +  /* end of file address*/
                    _shared_info->size_offsets +  /* driver-information block address */
                    H5G_SIZEOF_ENTRY(*_shared_info); /* root group symbol table entry */

	if ((fixed_size + variable_size) > sizeof(buf)) {
		H5E_push("check_superblock", "Total size of super block is incorrect.", _shared_info->super_addr);
		ret++;
		return(ret);
	}

	fseek(inputfd, _shared_info->super_addr+fixed_size, SEEK_SET);
	fread(p, 1, variable_size, inputfd);

    	/*
     	 * If the superblock version # is greater than 0, read in the indexed
     	 * storage B-tree internal 'K' value
     	 */
    	if (super_vers > 0) {
        	UINT16DECODE(p, _shared_info->btree_k);
        	p += 2;   /* reserved */
    	}


	H5F_addr_decode(*_shared_info, (const uint8_t **)&p, &_shared_info->base_addr/*out*/);
    	H5F_addr_decode(*_shared_info, (const uint8_t **)&p, &_shared_info->freespace_addr/*out*/);
    	H5F_addr_decode(*_shared_info, (const uint8_t **)&p, &_shared_info->stored_eoa/*out*/);
    	H5F_addr_decode(*_shared_info, (const uint8_t **)&p, &_shared_info->driver_addr/*out*/);

	if (H5G_ent_decode(*_shared_info, (const uint8_t **)&p, &root_ent/*out*/) < 0) {
		H5E_push("check_superblock", "Unable to read root symbol entry.", _shared_info->super_addr);
		ret++;
		return(ret);
	}
	_shared_info->root_grp = &root_ent;


	}  /* DONE WITH SCANNING and STORING OF SUPERBLOCK INFO */

#if DEBUG
	printf("super_addr = %llu\n", _shared_info->super_addr);
	printf("super_vers=%d, freespace_vers=%d, root_sym_vers=%d\n",	
		super_vers, freespace_vers, root_sym_vers);
	printf("size_offsets=%d, size_lengths=%d\n",	
		_shared_info->size_offsets, _shared_info->size_lengths);
	printf("gr_leaf_node_k=%d, gr_int_node_k=%d, file_consist_flg=%d\n",	
		_shared_info->gr_leaf_node_k, _shared_info->gr_int_node_k, _shared_info->file_consist_flg);
	printf("base_addr=%llu, freespace_addr=%llu, stored_eoa=%llu, driver_addr=%llu\n",
		_shared_info->base_addr, _shared_info->freespace_addr, _shared_info->stored_eoa, _shared_info->driver_addr);

	/* print root group table entry */
	printf("name0ffset=%d, header_address=%llu\n", 
		_shared_info->root_grp->name_off, _shared_info->root_grp->header);
	printf("btree_addr=%llu, heap_addr=%llu\n", 
		_shared_info->root_grp->cache.stab.btree_addr, 
		_shared_info->root_grp->cache.stab.heap_addr);
#endif

	/* 
	 * superblock signature is already checked
	 * DO the validation of the superblock info
	 */

	/* BEGIN VALIDATION */

	/* fixed size part validation */
	if (super_vers != HDF5_SUPERBLOCK_VERSION_DEF && 
	    super_vers != HDF5_SUPERBLOCK_VERSION_MAX) {
		H5E_push("check_superblock", "Version number of the superblock is incorrect.", _shared_info->super_addr);
		ret++;
	}
	if (freespace_vers != HDF5_FREESPACE_VERSION) {
		H5E_push("check_superblock", "Version number of the file free-space information is incorrect.", _shared_info->super_addr);
		ret++;
	}
	if (root_sym_vers != HDF5_OBJECTDIR_VERSION) {
		H5E_push("check_superblock", "Version number of the root group symbol table entry is incorrect.", _shared_info->super_addr);
		ret++;
	}
	if (shared_head_vers != HDF5_SHAREDHEADER_VERSION) {
		H5E_push("check_superblock", "Version number of the shared header message format is incorrect.", _shared_info->super_addr);
		ret++;
	}
	if (_shared_info->size_offsets != 2 && _shared_info->size_offsets != 4 &&
            _shared_info->size_offsets != 8 && _shared_info->size_offsets != 16 && 
	    _shared_info->size_offsets != 32) {
		H5E_push("check_superblock", "Bad byte number in an address.", _shared_info->super_addr);
		ret++;
	}
	if (_shared_info->size_lengths != 2 && _shared_info->size_lengths != 4 &&
            _shared_info->size_lengths != 8 && _shared_info->size_lengths != 16 && 
	    _shared_info->size_lengths != 32) {
		H5E_push("check_superblock", "Bad byte number for object size.", _shared_info->super_addr);
		ret++;
	}
	if (_shared_info->gr_leaf_node_k <= 0) {
		H5E_push("check_superblock", "Invalid leaf node of a group B-tree.", _shared_info->super_addr);
		ret++;
	}
	if (_shared_info->gr_int_node_k <= 0) {
		H5E_push("check_superblock", "Invalid internal node of a group B-tree.", _shared_info->super_addr);
		ret++;
	}
	if (_shared_info->file_consist_flg > 0x03) {
		H5E_push("check_superblock", "Invalid file consistency flags.", _shared_info->super_addr);
		ret++;
	}


	/* variable size part validation */
    	if (super_vers > 0) { /* indexed storage internal node k */
    		if (_shared_info->btree_k <= 0) {
			H5E_push("check_superblock", "Invalid internal node of an indexed storage b-tree.", _shared_info->super_addr);
			ret++;
		}
    	}

	/* SHOULD THERE BE MORE VALIDATION of base_addr?? */
	if ((_shared_info->base_addr != _shared_info->super_addr) ||
	    (_shared_info->base_addr >= _shared_info->stored_eoa))
	{
		H5E_push("check_superblock", "Invalid base address.", _shared_info->super_addr);
		ret++;
	} 

	if (_shared_info->freespace_addr != HADDR_UNDEF) {
		H5E_push("check_superblock", "Invalid address of global free-space index.", _shared_info->super_addr);
		ret++;
	}

	if (_shared_info->stored_eoa == HADDR_UNDEF) {
		H5E_push("check_superblock", "Invalid end of file address.", _shared_info->super_addr);
		ret++;
	}


	driver_name[0] = '\0';

	/* Read in driver information block and validate */
	/* Defined driver information block address or not */
    	if (_shared_info->driver_addr != HADDR_UNDEF) {
		haddr_t drv_addr = _shared_info->base_addr + _shared_info->driver_addr;
        	uint8_t dbuf[H5F_DRVINFOBLOCK_SIZE];     /* Local buffer */
		size_t  driver_size;   /* Size of driver info block, in bytes */


		if (((drv_addr+16) == HADDR_UNDEF) || ((drv_addr+16) >= _shared_info->stored_eoa)) {
			H5E_push("check_superblock", "Invalid driver information block.", _shared_info->super_addr);
			ret++;
			return(ret);
		}
		/* read in the first 16 bytes of the driver information block */
		fseek(inputfd, drv_addr, SEEK_SET);
		fread(dbuf, 1, 16, inputfd);
		p = dbuf;
		if (*p++ != HDF5_DRIVERINFO_VERSION) {
			H5E_push("check_superblock", "Bad driver information block version number.", _shared_info->super_addr);
			ret++;
		}
		p += 3; /* reserved */
		/* Driver info size */
        	UINT32DECODE(p, driver_size);

		
		 /* Driver name and/or version */
        	strncpy(driver_name, (const char *)p, (size_t)8);
        	driver_name[8] = '\0';
		set_driver(&(_shared_info->driverid), driver_name);
		p += 8; /* advance past driver identification */

		 /* Read driver information and decode */
        	assert((driver_size + 16) <= sizeof(dbuf));
		if (((drv_addr+16+driver_size) == HADDR_UNDEF) || 
		    ((drv_addr+16+driver_size) >= _shared_info->stored_eoa)) {
			H5E_push("check_superblock", "Invalid driver information size.", _shared_info->super_addr);
			ret++;
		}
		fseek(inputfd, drv_addr+16, SEEK_SET);
		fread(p, 1, driver_size, inputfd);
		decode_driver(_shared_info, p);
	}  /* DONE with driver information block */
	else /* sec2 driver is used when no driver information */
		set_driver(&(_shared_info->driverid), driver_name);
#if 0
	printf("driver_id = %d\n", _shared_info->driver->driver_id);
	H5FD_t file;
	_shared_info->driver->cls->get_eoa(&file);
	/*
     * Tell the file driver how much address space has already been
     * allocated so that it knows how to allocate additional memory.
     */
    if (H5FD_set_eoa(lf, stored_eoa) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to set end-of-address marker for file")
#endif



	/* NEED to validate shared_info.root_grp->name_off??? to be within size of local heap */

	if ((_shared_info->root_grp->header==HADDR_UNDEF) || 
	    (_shared_info->root_grp->header >= _shared_info->stored_eoa)) {
		H5E_push("check_superblock", "Invalid object header address.", _shared_info->super_addr);
		ret++;
	}
	if (_shared_info->root_grp->type < 0) {
		H5E_push("check_superblock", "Invalid cache type.", _shared_info->super_addr);
		ret++;
	}
	return(ret);
}


/* Based on H5G_node_load() in H5Gnode.c */
herr_t
check_sym(H5FD_t *_file, H5F_shared_t shared_info, haddr_t sym_addr) 
{
	size_t 		size = 0;
	uint8_t		*buf = NULL;
	uint8_t		*p;
	herr_t 		ret;
	unsigned	nsyms, u;
	H5G_node_t	*sym=NULL;
	H5G_entry_t	*ent;

	assert(H5F_addr_defined(sym_addr));

	printf("VALIDATING the SNOD at %llu...\n", sym_addr);
	size = H5G_node_size(shared_info);
	ret = SUCCEED;

#ifdef DEBUG
	printf("sym_addr=%d, symbol table node size =%d\n", 
		sym_addr, size);
#endif

	buf = malloc(size);
	if (buf == NULL) {
		H5E_push("check_sym", "Unable to malloc() a symbol table node.", sym_addr);
		ret++;
		goto done;
	}

	sym = malloc(sizeof(H5G_node_t));
	if (sym == NULL) {
		H5E_push("check_sym", "Unable to malloc() H5G_node_t.", sym_addr);
		ret++;
		goto done;
	}
	sym->entry = malloc(2*H5F_SYM_LEAF_K(shared_info)*sizeof(H5G_entry_t));
	if (sym->entry == NULL) {
		H5E_push("check_sym", "Unable to malloc() H5G_entry_t.", sym_addr);
		ret++;
		goto done;
	}

	if (H5FD_read(_file, sym_addr, size, buf) == FAIL) {
		H5E_push("check_sym", "Unable to read in the symbol table node.", sym_addr);
		ret++;
		goto done;
	}
#if 0
	fseek(inputfd, sym_addr, SEEK_SET);
       	if (fread(buf, 1, size, inputfd)<0) {
		H5E_push("check_sym", "Unable to read in the symbol table node.", sym_addr);
		ret++;
		goto done;
	}
#endif

	p = buf;
    	ret = memcmp(p, H5G_NODE_MAGIC, H5G_NODE_SIZEOF_MAGIC);
	if (ret != 0) {
		H5E_push("check_sym", "Couldn't find SNOD signature.", sym_addr);
		ret++;
	} else
		printf("FOUND SNOD signature.\n");

    	p += 4;
	
	 /* version */
    	if (H5G_NODE_VERS != *p++) {
		H5E_push("check_sym", "Bad symbol table node version.", sym_addr);
		ret++;
	}

    	/* reserved */
    	p++;

    	/* number of symbols */
    	UINT16DECODE(p, sym->nsyms);
	if (sym->nsyms > (2 * H5F_SYM_LEAF_K(shared_info))) {
		H5E_push("check_sym", "Number of symbols exceed (2*Group Leaf Node K).", sym_addr);
		ret++;
	}


	/* reading the vector of symbol table group entries */
	ret = H5G_ent_decode_vec(shared_info, (const uint8_t **)&p, sym->entry, sym->nsyms);
	if (ret != SUCCEED) {
		H5E_push("check_sym", "Unable to read in symbol table group entries.", sym_addr);
		ret++;
		goto done;
	}

	/* validate symbol table group entries here  */
    	for (u=0, ent = sym->entry; u < sym->nsyms; u++, ent++) {
#ifdef DEBUG
		printf("ent->type=%d, ent->name_off=%d, ent->header=%lld\n",
			ent->type, ent->name_off, ent->header);
#endif

		/* validate ent->name_off??? to be within size of local heap */

		if ((ent->header==HADDR_UNDEF) || (ent->header >= shared_info.stored_eoa)) {
			H5E_push("check_sym", "Invalid object header address.", sym_addr);
			ret++;
			continue;
		}
		if (ent->type < 0) {
			H5E_push("check_sym", "Invalid cache type.", sym_addr);
			ret++;
		}
/* NEED TO check for symbolic link, if so, the object header address is undefined */

		ret = check_obj_header(_file, shared_info, shared_info.base_addr+ent->header, 0, NULL);
		if (ret != SUCCEED) {
			H5E_push("check_sym", "Errors from check_obj_header()", sym_addr);
			H5E_print(stderr);
			H5E_clear();
			ret = SUCCEED;
		}
	}

done:
	if (buf) free(buf);
	if (sym) {
		if (sym->entry) free(sym->entry);
		free(sym);
	}

	return(ret);
}

herr_t
check_btree(H5FD_t *_file, H5F_shared_t shared_info, haddr_t btree_addr, unsigned ndims)
{
	uint8_t		*buf=NULL, *buffer=NULL;
	uint8_t		*p, nodetype;
	unsigned	nodelev, entries, u;
	haddr_t         left_sib;   /*address of left sibling */
	haddr_t         right_sib;  /*address of left sibling */
	size_t		hdr_size, name_offset, key_ptr_size;
	size_t		key_size, new_key_ptr_size;
	int		ret, status;
	haddr_t		child;
	void 		*key;



	assert(H5F_addr_defined(btree_addr));
    	hdr_size = H5B_SIZEOF_HDR(shared_info);
	ret = SUCCEED;

	printf("VALIDATING the btree at %llu...\n", btree_addr);

	buf = malloc(hdr_size);
	if (buf == NULL) {
		H5E_push("check_btree", "Unable to malloc() btree header.", btree_addr);
		ret++;
		goto done;
	}

	if (H5FD_read(_file, btree_addr, hdr_size, buf) == FAIL) {
		H5E_push("check_btree", "Unable to read btree header.", btree_addr);
		ret++;
		goto done;
	}
#if 0
	 /* read fixed-length part of object header */
	fseek(inputfd, btree_addr, SEEK_SET);
       	if (fread(buf, 1, (size_t)hdr_size, inputfd)<0) {
		H5E_push("check_btree", "Unable to read btree header.", btree_addr);
		ret++;
		goto done;
	}
#endif
	p = buf;

	/* magic number */
	ret = memcmp(p, H5B_MAGIC, (size_t)H5B_SIZEOF_MAGIC);
	if (ret != 0) {
		H5E_push("check_btree", "Couldn't find btree signature.", btree_addr);
		ret++;
		goto done;
	} else
		printf("FOUND btree signature.\n");

	p+=4;
	nodetype = *p++;
	nodelev = *p++;
	UINT16DECODE(p, entries);
	H5F_addr_decode(shared_info, (const uint8_t **)&p, &left_sib/*out*/);
	H5F_addr_decode(shared_info, (const uint8_t **)&p, &right_sib/*out*/);

#ifdef DEBUG
	printf("nodeytpe=%d, nodelev=%d, entries=%d\n",
		nodetype, nodelev, entries);
	printf("left_sib=%d, right_sib=%d\n", left_sib, right_sib);
#endif

	if ((nodetype != 0) && (nodetype !=1)) {
		H5E_push("check_btree", "Incorrect node type.", btree_addr);
		ret++;
	}

	if (nodelev < 0) {
		H5E_push("check_btree", "Incorrect node level.", btree_addr);
		ret++;
	}

	if (entries >= (2*shared_info.gr_int_node_k)) {
		H5E_push("check_btree", "Entries used exceed 2K.", btree_addr);
		ret++;
	}
	
	if ((left_sib != HADDR_UNDEF) && (left_sib >= shared_info.stored_eoa)) {
		H5E_push("check_btree", "Invalid left sibling address.", btree_addr);
		ret++;
	}
	if ((right_sib != HADDR_UNDEF) && (right_sib >= shared_info.stored_eoa)) {
		H5E_push("check_btree", "Invalid left sibling address.", btree_addr);
		ret++;
	}
	
	/* the remaining node size: key + child pointer */
	key_size = node_key_g[nodetype]->get_sizeof_rkey(shared_info, ndims);
	key_ptr_size = (2*shared_info.gr_int_node_k)*H5F_SIZEOF_ADDR(shared_info) +
		       (2*(shared_info.gr_int_node_k+1))*key_size;
#ifdef DEBUG
	printf("key_size=%d, key_ptr_size=%d\n", key_size, key_ptr_size);
#endif
	buffer = malloc(key_ptr_size);
	if (buffer == NULL) {
		H5E_push("check_btree", "Unable to malloc() key+child.", btree_addr);
		ret++;
		goto done;
	}

	if (H5FD_read(_file, btree_addr+hdr_size, key_ptr_size, buffer) == FAIL) {
		H5E_push("check_btree", "Unable to read key+child.", btree_addr);
		ret++;
		goto done;
	}

#if 0
	fseek(inputfd, btree_addr+hdr_size, SEEK_SET);
       	if (fread(buffer, 1, key_ptr_size, inputfd)<0) {
		H5E_push("check_btree", "Unable to read key+child.", btree_addr);
		ret++;
		goto done;
	}
#endif
	p = buffer;
		
	for (u = 0; u < entries; u++) {
		key = node_key_g[nodetype]->decode(shared_info, ndims, (const uint8_t **)&p);

#ifdef DEBUG
		if (nodetype == 0)
		printf("key's offset=%d\n", ((H5G_node_key_t *)key)->offset);
		else if (nodetype == 1)
		printf("size of chunk =%d\n", ((H5D_istore_key_t *)key)->nbytes);
#endif
		
		/* NEED TO VALIDATE name_offset to be within the local heap's size??HOW */

        	/* Decode address value */
  		H5F_addr_decode(shared_info, (const uint8_t **)&p, &child/*out*/);

		if ((child != HADDR_UNDEF) && (child >= shared_info.stored_eoa)) {
			H5E_push("check_btree", "Invalid child address.", btree_addr);
			ret++;
			continue;
		}

		if (nodelev > 0) {
/* NEED TO CHECK on something about ret value */
			check_btree(_file, shared_info, shared_info.base_addr+child, 0);
		} else {
			if (nodetype == 0) {
				status = check_sym(_file, shared_info, shared_info.base_addr+child);
				if (status != SUCCEED) {
					H5E_push("check_btree", "Errors from check_sym().", btree_addr);
					H5E_print(stderr);
					H5E_clear();
					ret = SUCCEED;
					continue;
				}
			} else if (nodetype == 1) {
				/* check_chunk_data() for btree */
			}
		}
	}  /* end for */
	/* decode final key */
	if (entries > 0) {
		key = node_key_g[nodetype]->decode(shared_info, ndims, (const uint8_t **)&p);
#ifdef DEBUG
		if (nodetype == 0)
		printf("Final key's offset=%d\n", ((H5G_node_key_t *)key)->offset);
		else if (nodetype == 1)
		printf("Final size of key data=%d\n", ((H5D_istore_key_t *)key)->nbytes);
#endif
	}

done:
	if (buf) free(buf);
	if (buffer) free(buffer);

	return(ret);
}


/* copied and modified from H5HL_load() of H5HL.c */
herr_t
check_lheap(H5FD_t *_file, H5F_shared_t shared_info, haddr_t lheap_addr, uint8_t **ret_heap_chunk)
{
	uint8_t		hdr[52];
	size_t		hdr_size, data_seg_size;
 	size_t  	next_free_off = H5HL_FREE_NULL;
	size_t		size_free_block, saved_offset;
	uint8_t		*heap_chunk=NULL;

	haddr_t		addr_data_seg;
	int		ret;
	const uint8_t	*p = NULL;


	assert(H5F_addr_defined(lheap_addr));
    	hdr_size = H5HL_SIZEOF_HDR(shared_info);
    	assert(hdr_size<=sizeof(hdr));

	printf("VALIDATING the local heap at %llu...\n", lheap_addr);
	ret = SUCCEED;

	if (H5FD_read(_file, lheap_addr, hdr_size, hdr) == FAIL) {
		H5E_push("check_lheap", "Unable to read local heap header.", lheap_addr);
		ret++;
		goto done;
	}
#if 0
	fseek(inputfd, lheap_addr, SEEK_SET);
       	if (fread(hdr, 1, (size_t)hdr_size, inputfd)<0) {
		H5E_push("check_lheap", "Unable to read local heap header.", lheap_addr);
		ret++;
		goto done;
	}
#endif
    	p = hdr;

	/* magic number */
	ret = memcmp(p, H5HL_MAGIC, H5HL_SIZEOF_MAGIC);
	if (ret != 0) {
		H5E_push("check_lheap", "Couldn't find local heap signature.", lheap_addr);
		ret++;
		goto done;
	} else
		printf("FOUND local heap signature.\n");

	p += H5HL_SIZEOF_MAGIC;

	 /* Version */
    	if (*p++ != H5HL_VERSION) {
		H5E_push("check_lheap", "Wrong local heap version number.", lheap_addr);
		ret++; /* continue on */
	}

    	/* Reserved */
    	p += 3;


	/* data segment size */
    	H5F_DECODE_LENGTH(shared_info, p, data_seg_size);
	if (data_seg_size <= 0) {
		H5E_push("check_lheap", "Invalid data segment size.", lheap_addr);
		ret++;
		goto done;
	}

	/* offset to head of free-list */
    	H5F_DECODE_LENGTH(shared_info, p, next_free_off);
#if 0
	if ((haddr_t)next_free_off != HADDR_UNDEF && (haddr_t)next_free_off != H5HL_FREE_NULL && (haddr_t)next_free_off >= data_seg_size) {
		H5E_push("check_lheap", "Offset to head of free list is invalid.", lheap_addr);
		ret++;
		goto done;
	}
#endif

	/* address of data segment */
    	H5F_addr_decode(shared_info, &p, &addr_data_seg);
	addr_data_seg = addr_data_seg + shared_info.base_addr;
	if ((addr_data_seg == HADDR_UNDEF) || (addr_data_seg >= shared_info.stored_eoa)) {
		H5E_push("check_lheap", "Address of data segment is invalid.", lheap_addr);
		ret++;
		goto done;
	}

	heap_chunk = malloc(hdr_size+data_seg_size);
	if (heap_chunk == NULL) {
		H5E_push("check_lheap", "Memory allocation failed for local heap data segment.", lheap_addr);
		ret++;
		goto done;
	}
	
#ifdef DEBUG
	printf("data_seg_size=%d, next_free_off =%d, addr_data_seg=%d\n",
		data_seg_size, next_free_off, addr_data_seg);
#endif

	if (data_seg_size) {
		if (H5FD_read(_file, addr_data_seg, data_seg_size, heap_chunk+hdr_size) == FAIL) {
			H5E_push("check_lheap", "Unable to read local heap data segment.", lheap_addr);
			ret++;
			goto done;
		}
#if 0
		fseek(inputfd, addr_data_seg, SEEK_SET);
       		if (fread(heap_chunk+hdr_size, 1, data_seg_size, inputfd)<0) {
			H5E_push("check_lheap", "Unable to read local heap data segment.", lheap_addr);
			ret++;
			goto done;
		}
#endif
	} 


	/* traverse the free list */
	while (next_free_off != H5HL_FREE_NULL) {
		if (next_free_off >= data_seg_size) {
			H5E_push("check_lheap", "Offset of the next free block is invalid.", lheap_addr);
			ret++;
			goto done;
		}
		saved_offset = next_free_off;
		p = heap_chunk + hdr_size + next_free_off;
		H5F_DECODE_LENGTH(shared_info, p, next_free_off);
		H5F_DECODE_LENGTH(shared_info, p, size_free_block);
#if DEBUG
		printf("next_free_off=%d, size_free_block=%d\n",
			next_free_off, size_free_block);
#endif

		if (!(size_free_block >= (2*H5F_SIZEOF_SIZE(shared_info)))) {
			H5E_push("check_lheap", "Offset of the next free block is invalid.", lheap_addr);
			ret++;  /* continue on */
		}
        	if (saved_offset + size_free_block > data_seg_size) {
			H5E_push("check_lheap", "Bad heap free list.", lheap_addr);
			ret++;
			goto done;
		}
	}


/* NEED TO CHECK ON this */
done:
	if ((ret==SUCCEED) && ret_heap_chunk) {
		*ret_heap_chunk = (uint8_t *)heap_chunk;
	} else {
		if (ret) /* fail */
			*ret_heap_chunk = NULL;
		if (heap_chunk)
			free(heap_chunk);
	}
	return(ret);
}


/* copied and modified from H5HG_load() of H5HG.c */
/*-------------------------------------------------------------------------
 * Function:    H5HG_load
 *
 * Purpose:     Loads a global heap collection from disk.
 *
 * Return:      Success:        Ptr to a global heap collection.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, March 27, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *
 *      Quincey Koziol, 2002-7-180
 *      Added dxpl parameter to allow more control over I/O from metadata
 *      cache.
 *-------------------------------------------------------------------------
 */
herr_t
check_gheap(H5FD_t *_file, H5F_shared_t shared_info, haddr_t gheap_addr, uint8_t **ret_heap_chunk)
{
    	H5HG_heap_t 	*heap = NULL;
    	uint8_t     	*p = NULL;
    	int 		ret, i;
    	size_t      	nalloc, need;
    	size_t      	max_idx=0;              /* The maximum index seen */
    	H5HG_heap_t 	*ret_value = NULL;      /* Return value */


    	/* check arguments */
    	assert(H5F_addr_defined(gheap_addr));
	ret = SUCCEED;

    	/* Read the initial 4k page */
    	if ((heap = calloc(1, sizeof(H5HG_heap_t)))==NULL) {
		H5E_push("check_gheap", "Couldn't calloc() H5HG_heap_t.", gheap_addr);
		ret++;
		goto done;
	}
    	heap->addr = gheap_addr;
    	if ((heap->chunk = malloc(H5HG_MINSIZE))==NULL) {
		H5E_push("check_gheap", "Couldn't malloc() H5HG_heap_t->chunk.", gheap_addr);
		ret++;
		goto done;
	}

 	printf("VALIDATING the global heap at %llu...\n", gheap_addr);
        
	if (H5FD_read(_file, gheap_addr, H5HG_MINSIZE, heap->chunk) == FAIL) {
		H5E_push("check_gheap", "Unable to read global heap collection.", gheap_addr);
		ret++;
		goto done;
	}
#if 0
        fseek(inputfd, gheap_addr, SEEK_SET);
        if (fread(heap->chunk, 1, H5HG_MINSIZE, inputfd)<0) {
                H5E_push("check_gheap", "Unable to read global heap collection.", gheap_addr);
                ret++;
		goto done;
	}
#endif

    	/* Magic number */
    	if (memcmp(heap->chunk, H5HG_MAGIC, H5HG_SIZEOF_MAGIC)) {
		H5E_push("check_gheap", "Couldn't find GCOL signature.", gheap_addr);
		ret++;
	} else
		printf("FOUND GLOBAL HEAP SIGNATURE\n");

    	p = heap->chunk + H5HG_SIZEOF_MAGIC;

    	/* Version */
    	if (H5HG_VERSION != *p++) {
		H5E_push("check_gheap", "Wrong version number in global heap.", gheap_addr);
		ret++;
	} else
		printf("Version 1 of global heap is detected\n");

    	/* Reserved */
    	p += 3;

    	/* Size */
    	H5F_DECODE_LENGTH(shared_info, p, heap->size);
    	assert (heap->size>=H5HG_MINSIZE);

    	/*
     	 * If we didn't read enough in the first try, then read the rest of the
         * collection now.
     	 */
    	if (heap->size > H5HG_MINSIZE) {
        	haddr_t next_addr = gheap_addr + (hsize_t)H5HG_MINSIZE;

        	if ((heap->chunk = realloc(heap->chunk, heap->size))==NULL) {
			H5E_push("check_gheap", "Couldn't realloc() H5HG_heap_t->chunk.", gheap_addr);
			ret++;
			goto done;
		}
		if (H5FD_read(_file, next_addr, heap->size-H5HG_MINSIZE, heap->chunk+H5HG_MINSIZE) == FAIL) {
			H5E_push("check_gheap", "Unable to read global heap collection.", gheap_addr);
			ret++;
			goto done;
		}
#if 0
        	fseek(inputfd, next_addr, SEEK_SET);
        	if (fread(heap->chunk+H5HG_MINSIZE, 1, heap->size-H5HG_MINSIZE, inputfd)<0) {
                	H5E_push("check_gheap", "Unable to read global heap collection.", gheap_addr);
                	ret++;
			goto done;
    		}
#endif
	}  /* end if */

    	/* Decode each object */
    	p = heap->chunk + H5HG_SIZEOF_HDR(shared_info);
    	nalloc = H5HG_NOBJS(shared_info, heap->size);
    	if ((heap->obj = malloc(nalloc*sizeof(H5HG_obj_t)))==NULL) {
                H5E_push("check_gheap", "Couldn't malloc() H5HG_obj_t.", gheap_addr);
                ret++;
		goto done;
	}
    	heap->obj[0].size = heap->obj[0].nrefs=0;
    	heap->obj[0].begin = NULL;

    	heap->nalloc = nalloc;
    	while (p < heap->chunk+heap->size) {
        	if (p+H5HG_SIZEOF_OBJHDR(shared_info) > heap->chunk+heap->size) {
            		/*
             		 * The last bit of space is too tiny for an object header, so we
              		 * assume that it's free space.
             		 */
            		assert(NULL==heap->obj[0].begin);
            		heap->obj[0].size = (heap->chunk+heap->size) - p;
            		heap->obj[0].begin = p;
            		p += heap->obj[0].size;
        	} else {
            		unsigned idx;
            		uint8_t *begin = p;

            		UINT16DECODE (p, idx);

            		/* Check if we need more room to store heap objects */
            		if(idx >= heap->nalloc) {
                		size_t new_alloc;       /* New allocation number */
                		H5HG_obj_t *new_obj;    /* New array of object descriptions */

                		/* Determine the new number of objects to index */
                		new_alloc = MAX(heap->nalloc*2,(idx+1));

                		/* Reallocate array of objects */
                		if ((new_obj=realloc(heap->obj, new_alloc*sizeof(H5HG_obj_t)))==NULL) {
                			H5E_push("check_gheap", "Couldn't realloc() H5HG_obj_t.", gheap_addr);
                			ret++;
					goto done;
				}

                		/* Update heap information */
                		heap->nalloc = new_alloc;
                		heap->obj = new_obj;
            		} /* end if */

            		UINT16DECODE (p, heap->obj[idx].nrefs);
            		p += 4; /*reserved*/
            		H5F_DECODE_LENGTH (shared_info, p, heap->obj[idx].size);
            		heap->obj[idx].begin = begin;
#ifdef DEBUG
			if (idx > 0)
				printf("global:%s\n", p);
#endif
            		/*
             		 * The total storage size includes the size of the object header
             		 * and is zero padded so the next object header is properly
             		 * aligned. The last bit of space is the free space object whose
             		 * size is never padded and already includes the object header.
             		 */
            		if (idx > 0) {
                		need = H5HG_SIZEOF_OBJHDR(shared_info) + H5HG_ALIGN(heap->obj[idx].size);

                		/* Check for "gap" in index numbers (caused by deletions) 
			   	   and fill in heap object values */
                		if(idx > (max_idx+1))
                    			memset(&heap->obj[max_idx+1],0,sizeof(H5HG_obj_t)*(idx-(max_idx+1)));
                		max_idx = idx;
            		} else {
                		need = heap->obj[idx].size;
            		}
            		p = begin + need;
        		}  /* end else */
	}  /* end while */

    	assert(p==heap->chunk+heap->size);
    	assert(H5HG_ISALIGNED(heap->obj[0].size));

    	/* Set the next index value to use */
    	if(max_idx > 0)
        	heap->nused = max_idx + 1;
    	else
        	heap->nused = 1;

done:
	if ((ret==SUCCEED) && ret_heap_chunk) {
		*ret_heap_chunk = (uint8_t *)heap;
	} else {
		if (ret) /* fail */
			*ret_heap_chunk = NULL;
		if (heap) {
			if (heap->chunk) free(heap->chunk);
			if (heap->obj) free(heap->obj);
			free(heap);
		}
	}
	return(ret);
}

herr_t
decode_validate_messages(H5FD_t *_file, H5O_t *oh)
{
	int 		ret, i, j, status;
	unsigned	id, u;
	struct tm *mtm;
	void		*mesg;
	uint8_t		*heap_chunk;
	size_t		k;
    	const char      *s = NULL;

	uint8_t		*pp;
	unsigned	global1, global2;
	haddr_t		global;

	ret = SUCCEED;

	for (i = 0; i < oh->nmesgs; i++) {	
	  id = oh->mesg[i].type->id;
	  if (id == H5O_CONT_ID) {
		continue;
	  } else if (id == H5O_NULL_ID) {
		continue;
	  } else if (oh->mesg[i].flags & H5O_FLAG_SHARED) {
	  	mesg = H5O_SHARED->decode(oh->mesg[i].raw);
		if (mesg != NULL) {
			status = H5O_shared_read(_file, mesg, oh->mesg[i].type);
			if (status != SUCCEED) {
				H5E_push("decode_validate_messages", "Errors from H5O_shared_read()", -1);
				H5E_print(stderr);
				H5E_clear();
				ret = SUCCEED;
			}
			continue;
		}
	   } else {
	  	mesg = message_type_g[id]->decode(oh->mesg[i].raw);
	    }

	    if (mesg == NULL) {
		H5E_push("decode_validate_messages", "Failure in type->decode().", -1);
		ret++;
		continue;
	    }
	    switch (id) {
		case H5O_SDSPACE_ID:
#ifdef DEBUG
			printf("sdim->type=%d, sdim->nelem=%d, sdim->rank=%d\n",
				((H5S_extent_t *)mesg)->type, ((H5S_extent_t *)mesg)->nelem, 
				((H5S_extent_t *)mesg)->rank);
                	for (j = 0; j < ((H5S_extent_t *)mesg)->rank; j++)
                    		printf("size=%d\n", ((H5S_extent_t *)mesg)->size[j]);
			if (((H5S_extent_t *)mesg)->max == NULL)
				printf("NULL pointer for sdim->max\n");
			else {
                		for (j = 0; j < ((H5S_extent_t *)mesg)->rank; j++)
                    			printf("max=%d\n", ((H5S_extent_t *)mesg)->max[j]);
			}
#endif
			break;
		case H5O_DTYPE_ID:
#ifdef DEBUG
			printf("type=%d, size=%d\n", ((H5T_t *)mesg)->shared->type, 
				((H5T_t *)mesg)->shared->size);
#endif
			break;

		case H5O_FILL_NEW_ID:
			break;
    		case H5O_EFL_ID:
			status = check_lheap(_file, shared_info, shared_info.base_addr+((H5O_efl_t *)mesg)->heap_addr, &heap_chunk);
			if (status != SUCCEED) {
				H5E_push("decode_validate_messages", "Failure from check_lheap()", -1);
				H5E_print(stderr);
				H5E_clear();
				ret = SUCCEED;
			}
#if 0
        assert(mesg->slot[u].name);
#endif
			/* should be linked together as else when status == SUCCEED */
    			for (k = 0; k < ((H5O_efl_t *)mesg)->nused; k++) {
			
				s = heap_chunk+H5HL_SIZEOF_HDR(shared_info)+((H5O_efl_t *)mesg)->slot[k].name_offset;
        			assert (s && *s);
#ifdef DEBUG
				printf("External name_offset:%d\n", 
					((H5O_efl_t *)mesg)->slot[k].name_offset);
				printf("Externalfile:%s\n", s);
#endif
			}
			if (heap_chunk) free(heap_chunk);
			break;
		case H5O_LAYOUT_ID:
#ifdef DEBUG
			printf("type=%d\n", ((H5O_layout_t *)mesg)->version, ((H5O_layout_t *)mesg)->type);
			printf("contig address=%d, ndims=%d\n", 
				((H5O_layout_t *)mesg)->u.contig.addr, ((H5O_layout_t *)mesg)->unused.ndims);
			for (u = 0; u < ((H5O_layout_t *)mesg)->unused.ndims; u++)
				printf("dim=%d\n", ((H5O_layout_t *)mesg)->unused.dim[u]);
#endif
			if (((H5O_layout_t *)mesg)->type == H5D_CHUNKED) {
				unsigned	ndims;
				haddr_t		btree_addr;
				
				ndims = ((H5O_layout_t *)mesg)->u.chunk.ndims;
				btree_addr = ((H5O_layout_t *)mesg)->u.chunk.addr;
/* NEED TO CHECK ON THIS */
				status = check_btree(_file, shared_info, shared_info.base_addr+btree_addr, ndims);
			}

			break;
    		case H5O_PLINE_ID:
#ifdef DEBUG
			for (j=0; j<((H5O_pline_t *)mesg)->nused; j++) {
				printf("plinename=%s;\n", ((H5O_pline_t *)mesg)->filter[j].name);
			}
#endif
			break;
    		case H5O_ATTR_ID:
#ifdef DEBUG
			printf("Attribute dt_size=%d,ds_size=%d\n", 
				((H5A_t *)mesg)->dt_size, ((H5A_t *)mesg)->ds_size);
			printf("Attribute name=%s\n", ((H5A_t *)mesg)->name);
			printf("attribute type=%d, attribute size=%d\n", 
				((H5A_t *)mesg)->dt->shared->type, ((H5A_t *)mesg)->dt->shared->size);
			printf("attr->ds->extent:type=%d, :rank=%d, :nelem=%d\n", 
				((H5A_t *)mesg)->ds->extent.type, ((H5A_t *)mesg)->ds->extent.rank, 
				((H5A_t *)mesg)->ds->extent.nelem);
			printf("attr->data_size=%d\n", ((H5A_t *)mesg)->data_size);
#endif

#ifdef TEMP
			/* IT MAY NOT BE a string, maybe global heap pointer or 
			   some others but for now can i say get type if string print string, 
			   otherwise print integer */
			pp = ((H5A_t*)mesg)->data;
            		UINT32DECODE(pp, global1);
		H5F_addr_decode(shared_info, (const uint8_t **)&pp, &global);
			printf("global1=%ld, global=%lld\n", global1, global);
/* SHULD CHECK for return status */
			check_gheap(_file, shared_info, shared_info.base_addr+global, NULL);
#endif

			break;
		case H5O_STAB_ID:
#ifdef DEBUG
			printf("stab->btree_addr=%lld,stab->heap_addr=%lld\n",
				((H5O_stab_t *)mesg)->btree_addr, ((H5O_stab_t *)mesg)->heap_addr);
#endif
			status = check_btree(_file, shared_info, shared_info.base_addr+((H5O_stab_t *)mesg)->btree_addr, 0);
			if (status != SUCCEED) {
				H5E_push("decode_validate_messages", "Failure from check_btree()", -1);
				H5E_print(stderr);
				H5E_clear();
				ret = SUCCEED;
			}
			status = check_lheap(_file, shared_info, shared_info.base_addr+((H5O_stab_t *)mesg)->heap_addr, NULL);
			if (status != SUCCEED) {
				H5E_push("decode_validate_messages", "Failure from check_lheap()", -1);
				H5E_print(stderr);
				H5E_clear();
				ret = SUCCEED;
			}
			break;
		case H5O_MTIME_NEW_ID:
#ifdef DEBUG
			mtm = localtime((time_t *)mesg);
			printf("month=%d, mday=%d, year=%d\n", 
				mtm->tm_mon, mtm->tm_mday, mtm->tm_year);
#endif
			break;
		case H5O_NAME_ID:
#ifdef DEBUG
			printf("The comment string is %s\n", ((H5O_name_t *)mesg)->s);
#endif
			break;
		
		default:
			printf("Done with decode/validate for a TO-BE-HANDLED message id=%d.\n", id);
			break;
			
	  }  /* end switch */
	}  /* end for */

	return(ret);
}



/* copied and modified from H5O.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_find_in_ohdr
 *
 * Purpose:     Find a message in the object header without consulting
 *              a symbol table entry.
 *
 * Return:      Success:    Index number of message.
 *              Failure:    Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *      Robb Matzke, 1999-07-28
 *      The ADDR argument is passed by value.
 *
 *      Bill Wendling, 2003-09-30
 *      Modified so that the object header needs to be AC_protected
 *      before calling this function.
 *-------------------------------------------------------------------------
 */
static unsigned
H5O_find_in_ohdr(H5FD_t *_file, H5O_t *oh, const H5O_class_t **type_p, int sequence)
{
	unsigned            u, FOUND;
    	const H5O_class_t   *type = NULL;
    	unsigned            ret_value;

/* DO I NEED TO PASS type_p, and modified it ??? */
    	/* Check args */
    	assert(oh);
    	assert(type_p);

    	/* Scan through the messages looking for the right one */
    	for (u = 0; u < oh->nmesgs; u++) {
        	if (*type_p && (*type_p)->id != oh->mesg[u].type->id)
            		continue;
		if (--sequence < 0)
			break;
    	}
#ifdef DEBBUG
	if (sequence >= 0)
	printf("Unable to find object header messges\n");
#endif
	
    	/*
     	 * Decode the message if necessary.  If the message is shared then decode
     	 * a shared message, ignoring the message type.
     	 */
    	if (oh->mesg[u].flags & H5O_FLAG_SHARED) {
        	type = H5O_SHARED;
    	} else {
        	type = oh->mesg[u].type;
	}

    	if (oh->mesg[u].native == NULL) {
        	assert(type->decode);
        	oh->mesg[u].native = (type->decode) (oh->mesg[u].raw);
        	if (NULL == oh->mesg[u].native)
/* NEED TO CHECK ON THIS */
            		printf("unable to decode message\n");
    	}

    /*
     * Return the message type. If this is a shared message then return the
     * pointed-to type.
     */
    *type_p = oh->mesg[u].type;

   	/* Set return value */
    	ret_value = u;
	return(ret_value);
}


/* copied and modified from H5Oshared.c */
/*-------------------------------------------------------------------------
 * Function:    H5O_shared_read
 *
 * Purpose:     Reads a message referred to by a shared message.
 *
 * Return:      Success:        Ptr to message in native format.  The message
 *                              should be freed by calling H5O_reset().  If
 *                              MESG is a null pointer then the caller should
 *                              also call H5MM_xfree() on the return value
 *                              after calling H5O_reset().
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Sep 24 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_shared_read(H5FD_t *_file, H5O_shared_t *shared, const H5O_class_t *type)
{
    	int	ret = SUCCEED;


    	/* check args */
    	assert(shared);
    	assert(type);

    	/* Get the shared message */
    	if (shared->in_gh) {
		printf("The message pointed to by the shared message is in global heap..NOT HANDLED YET\n");
#if 0
        	void *tmp_buf, *tmp_mesg;
        if (NULL==(tmp_buf = H5HG_read (f, dxpl_id, &(shared->u.gh), NULL)))
            HGOTO_ERROR (H5E_OHDR, H5E_CANTLOAD, NULL, "unable to read shared message from global heap");
        tmp_mesg = (type->decode)(f, dxpl_id, tmp_buf, shared);
        tmp_buf = H5MM_xfree (tmp_buf);
        if (!tmp_mesg)
            HGOTO_ERROR (H5E_OHDR, H5E_CANTLOAD, NULL, "unable to decode object header shared message");

#endif
    	} else {
	  	ret = check_obj_header(_file, shared_info, shared_info.base_addr+shared->u.ent.header, 1, type);
    	}
	
	return(ret);
}




/* copied and modified from H5O_load() of H5O.c */
herr_t
check_obj_header(H5FD_t *_file, H5F_shared_t shared_info, haddr_t obj_head_addr, int search, const H5O_class_t *type)
{
	size_t		hdr_size, chunk_size;
	haddr_t		chunk_addr;
	uint8_t		buf[16], *p, flags;
	unsigned	nmesgs, id;
	size_t		mesg_size;
	int		version, nlink, ret, status;

	H5O_t		*oh=NULL;
	unsigned    	chunkno;
	unsigned    	curmesg=0;
	int		mesgno, i, j;
	unsigned	saved;


       	H5O_cont_t 	*cont;
	H5O_stab_t 	*stab;
	int		idx;
	table_t		tb;


	assert(H5F_addr_defined(obj_head_addr));
	ret = SUCCEED;
	idx = table_search(obj_head_addr);
	if (idx >= 0) { /* FOUND the object */
		if (obj_table->objs[idx].nlink > 0) {
			obj_table->objs[idx].nlink--;
			return(ret);
		} else {
			H5E_push("check_obj_header", "Inconsistent reference count.", obj_head_addr);
			ret++;  /* SHOULD I return??? */
			return(ret);
		}
	}
    	hdr_size = H5O_SIZEOF_HDR(shared_info);
    	assert(hdr_size<=sizeof(buf));

	printf("VALIDATING the object header at %llu...\n", obj_head_addr);

#ifdef DEBUG
	printf("obj_head_addr=%d, hdr_size=%d\n", 
		obj_head_addr, hdr_size);
#endif
	oh = calloc(1, sizeof(H5O_t));
	if (oh == NULL) {
		H5E_push("check_obj_header", "Unable to calloc() H5O_t.", obj_head_addr);
		ret++;
		return(ret);
	}

	if (H5FD_read(_file, obj_head_addr, (size_t)hdr_size, buf) == FAIL) {
		H5E_push("check_obj_header", "Unable to read object header.", obj_head_addr);
		ret++;
		return(ret);
	}
#if 0
	 /* read fixed-length part of object header */
	fseek(inputfd, obj_head_addr, SEEK_SET);
       	if (fread(buf, 1, (size_t)hdr_size, inputfd)<0) {
		H5E_push("check_obj_header", "Unable to read object header.", obj_head_addr);
		ret++;
		return(ret);
	}
#endif 

    	p = buf;
	oh->version = *p++;
	if (oh->version != H5O_VERSION) {
		H5E_push("check_obj_header", "Invalid version number.", obj_head_addr);
		ret++;
	}
	p++;
	UINT16DECODE(p, nmesgs);
	if ((int)nmesgs < 0) {  /* ??? */
		H5E_push("check_obj_header", "Incorrect number of header messages.", obj_head_addr);
		ret++;
	}

	UINT32DECODE(p, oh->nlink);

	table_insert(obj_head_addr, oh->nlink-1);
	

	chunk_addr = obj_head_addr + (size_t)hdr_size;
	UINT32DECODE(p, chunk_size);

	oh->alloc_nmesgs = MAX(H5O_NMESGS, nmesgs);
	oh->mesg = calloc(oh->alloc_nmesgs, sizeof(H5O_mesg_t));

#ifdef DEBUG
	printf("oh->version =%d, nmesgs=%d, oh->nlink=%d\n", oh->version, nmesgs, oh->nlink);
	printf("chunk_addr=%d, chunk_size=%d\n", chunk_addr, chunk_size);
#endif

	/* read each chunk from disk */
    	while (H5F_addr_defined(chunk_addr)) {
		  /* increase chunk array size */
        	if (oh->nchunks >= oh->alloc_nchunks) {
            		unsigned na = oh->alloc_nchunks + H5O_NCHUNKS;
            		H5O_chunk_t *x = realloc(oh->chunk, (size_t)(na*sizeof(H5O_chunk_t)));
            		if (!x) {
				H5E_push("check_obj_header", "Realloc() failed for H5O_chunk_t.", obj_head_addr);
				ret++;
				return(ret);
			}
            		oh->alloc_nchunks = na;
            		oh->chunk = x;
        	}  /* end if */

        	/* read the chunk raw data */
        	chunkno = oh->nchunks++;
        	oh->chunk[chunkno].addr = chunk_addr;
        	oh->chunk[chunkno].size = chunk_size;

		if ((oh->chunk[chunkno].image = calloc(1, chunk_size)) == NULL) {
			H5E_push("check_obj_header", "calloc() failed for oh->chunk[].image", obj_head_addr);
			ret++;
			return(ret);
		}

		if (H5FD_read(_file, chunk_addr, chunk_size, oh->chunk[chunkno].image) == FAIL) {
			H5E_push("check_obj_header", "Unable to read object header data.", obj_head_addr);
			ret++;
			return(ret);
		}

#if 0
		fseek(inputfd, chunk_addr, SEEK_SET);
       		if (fread(oh->chunk[chunkno].image, 1, (size_t)chunk_size, inputfd)<0) {
			H5E_push("check_obj_header", "Unable to read object header data.", obj_head_addr);
			ret++;
			return(ret);
		}  /* end if */
#endif


		for (p = oh->chunk[chunkno].image; p < oh->chunk[chunkno].image+chunk_size; p += mesg_size) {
			UINT16DECODE(p, id);
            		UINT16DECODE(p, mesg_size);
			assert(mesg_size==H5O_ALIGN(mesg_size));
			flags = *p++;
			p += 3;  /* reserved */
			/* Try to detect invalidly formatted object header messages */
            		if (p + mesg_size > oh->chunk[chunkno].image + chunk_size) {
				H5E_push("check_obj_header", "Corrupt object header message.", obj_head_addr);
				ret++;
				return(ret);
			}

            		/* Skip header messages we don't know about */
            		if (id >= NELMTS(message_type_g) || NULL == message_type_g[id]) {
                		continue;
			}
			 /* new message */
                	if (oh->nmesgs >= nmesgs) {
				H5E_push("check_obj_header", "Corrupt object header.", obj_head_addr);
				ret++;
				return(ret);
			}
                	mesgno = oh->nmesgs++;
                	oh->mesg[mesgno].type = message_type_g[id];
                	oh->mesg[mesgno].flags = flags;
			oh->mesg[mesgno].native = NULL;
                	oh->mesg[mesgno].raw = p;
                	oh->mesg[mesgno].raw_size = mesg_size;
                	oh->mesg[mesgno].chunkno = chunkno;
		}  /* end for */

		assert(p == oh->chunk[chunkno].image + chunk_size);

        	/* decode next object header continuation message */
        	for (chunk_addr = HADDR_UNDEF; !H5F_addr_defined(chunk_addr) && curmesg < oh->nmesgs; ++curmesg) {
            		if (oh->mesg[curmesg].type->id == H5O_CONT_ID) {
                		cont = (H5O_CONT->decode) (shared_info.base_addr+oh->mesg[curmesg].raw);
				if (cont == NULL) {
					H5E_push("check_obj_header", "Corrupt continuation message...skipped.", 
						obj_head_addr);
					ret++;
					continue;
				}
                		oh->mesg[curmesg].native = cont;
                		chunk_addr = cont->addr;
                		chunk_size = cont->size;
                		cont->chunkno = oh->nchunks;    /*the next chunk to allocate */
            		}  /* end if */
        	}  /* end for */
	}  /* end while */

	if (search) {
		idx = H5O_find_in_ohdr(_file, oh, &type, 0);
		if (oh->mesg[idx].flags & H5O_FLAG_SHARED) {
			H5O_shared_t *shared;
			void	*ret_value;

        		shared = (H5O_shared_t *)(oh->mesg[idx].native);
        		status = H5O_shared_read(_file, shared, type);
		}
	} else {
		status = decode_validate_messages(_file, oh);
	}
	if (status != SUCCEED)
		ret++;

	return(ret);
}

print_version(const char *prog_name)
{
	fprintf(stdout, "%s: Version %d\n", prog_name, VERSION);
}

void
usage(prog_name)
{
	fflush(stdout);
    	fprintf(stdout, "usage: %s [OPTIONS] file\n", prog_name);
    	fprintf(stdout, "  OPTIONS\n");
    	fprintf(stdout, "     -h, --help   	Print a usage message and exit\n");
    	fprintf(stdout, "     -v, --version	Print version number and exit\n");
    	fprintf(stdout, "     -V, --verbose	Detail output of what is being done\n");
	fprintf(stdout, "\n");

}

leave(int ret)
{
	exit(ret);
}


int main(int argc, char **argv)
{
	int		ret;
	haddr_t		ss;
	haddr_t		gheap_addr;
	FILE 		*inputfd;
	H5FD_t 		*thefile;


	/* command line declarations */
	int	argno;
	const 	char *s = NULL;
	char	*prog_name;
	char	*fname;
	

	if ((prog_name=strrchr(argv[0], '/'))) 
		prog_name++;
	else 
		prog_name = argv[0];

	for (argno=1; argno<argc && argv[argno][0]=='-'; argno++) {
		if (!strcmp(argv[argno], "--help")) {
			usage(prog_name);
			leave(EXIT_SUCCESS);
		} else if (!strcmp(argv[argno], "--version")) {
			print_version(prog_name);
			leave(EXIT_SUCCESS);
		} else if (!strcmp(argv[argno], "--verbose"))
			printf("VERBOSE is true\n");
		else if (argv[argno][1] != '-') {
			for (s=argv[argno]+1; *s; s++) {
				switch (*s) {
					case 'h':  /* --help */
						usage(prog_name);
						leave(EXIT_SUCCESS);
					case 'v':  /* --version */
						print_version(prog_name);
						leave(EXIT_SUCCESS);
					case 'V':  /* --verbose */
						printf("single VERBOSE is true\n");
						break;
					default:
						usage(prog_name);	
						leave(EXIT_COMMAND_FAILURE);
				}  /* end switch */
			}  /* end for */
		} else
			printf("default is true, no option provided...\n");
	}

	if (argno >= argc) {
		usage(prog_name);
		leave(EXIT_COMMAND_FAILURE);
	}


	ret = SUCCEED;
	fname = strdup(argv[argno]);
	printf("\nVALIDATING %s\n\n", fname);

	inputfd = fopen(fname, "r");

        if (inputfd == NULL) {
                fprintf(stderr, "fopen(\"%s\") failed: %s\n", fname,
                                 strerror(errno));
                exit(errno);
        }


	ret = check_superblock(inputfd, &shared_info);
	if (ret != SUCCEED) {
		H5E_push("Main", "Errors from check_superblock(). Validation stopped.", -1);
		H5E_print(stderr);
		H5E_clear();
		fclose(inputfd);
		exit(1);
	}

	fclose(inputfd);

	/* superblock validation has to be all passed before proceeding further */
	/* from now on, use virtual file driver for opening/reading/closing */
	thefile = H5FD_open(fname, shared_info.driverid);
	if (thefile == NULL) {
		H5E_push("Main", "Errors from H5FD_open(). Validation stopped.", -1);
		H5E_print(stderr);
		H5E_clear();
                exit(1);
        }


	ss = thefile->cls->get_eof(thefile);
	if ((ss==HADDR_UNDEF) || (ss<shared_info.stored_eoa)) {
		H5E_push("Main", "Invalid file size or file size less than superblock eoa. Validation stopped.", -1);
		H5E_print(stderr);
		H5E_clear();
                exit(1);
	}

	ret = table_init(&obj_table);
	if (ret != SUCCEED) {
		H5E_push("Main", "Errors from table_init().", -1);
		H5E_print(stderr);
		H5E_clear();
		ret = SUCCEED;
	}

	ret = check_obj_header(thefile, shared_info, shared_info.base_addr+shared_info.root_grp->header, 0, NULL);
	if (ret != SUCCEED) {
		H5E_push("Main", "Errors from check_obj_header().", -1);
		H5E_print(stderr);
		H5E_clear();
		ret = SUCCEED;
	}

	ret = H5FD_close(thefile);
	if (ret != SUCCEED) {
		H5E_push("Main", "Errors from H5FD_close().", -1);
		H5E_print(stderr);
		H5E_clear();
		exit(1);
	}
}
