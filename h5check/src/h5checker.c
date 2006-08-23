#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "h5_check.h"

#define	INPUTFILE	"./STUDY/eegroup.h5"
#if 0
#define	INPUTFILE	"./STUDY/tryout.h5"
#define	INPUTFILE	"./STUDY/eegroup.h5"
#define	INPUTFILE	"/mnt/sdt/vchoi/work/H5CHECKER/TEST/h5check/test/hierarchical.h5"
#define	INPUTFILE	"./example1.h5"
#define	INPUTFILE	"/mnt/sdt/vchoi/work/STEP/READ_STEP/NARA/indexing/STEP/allspline.h5"
#define	INPUTFILE	"./STUDY/ee.h5"
/* couldn't use this one for the time being....need to put in user block */
#define	INPUTFILE	"./DOQ.h5"
#endif

H5F_shared_t	shared_info;
H5G_entry_t 	root_ent;

herr_t  check_superblock(FILE *);
herr_t 	check_obj_header(FILE *, haddr_t);
herr_t 	check_btree(FILE *, haddr_t);
herr_t 	check_sym(FILE *, haddr_t);
herr_t 	check_lheap(FILE *, haddr_t);
herr_t 	check_gheap(FILE *, haddr_t);


haddr_t locate_super_signature(FILE *);
void H5F_addr_decode(H5F_shared_t, const uint8_t **, haddr_t *);
herr_t H5G_ent_decode(H5F_shared_t, const uint8_t **, H5G_entry_t *);
herr_t H5G_ent_decode_vec(H5F_shared_t, const uint8_t **, H5G_entry_t *, unsigned);

/* copied and modified from H5Onull.c */
const H5O_class_t H5O_NULL[1] = {{
    H5O_NULL_ID,            /*message id number             */
    "null",                 /*message name for debugging    */
    0,                      /*native message size           */
    NULL                    /*no decode method              */
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

/* copied and modified from H5Ocont.c */
static void *H5O_cont_decode(const uint8_t *);

/* copied and modified from H5Ocont.c */
/* This message derives from H5O */
const H5O_class_t H5O_CONT[1] = {{
    H5O_CONT_ID,                /*message id number             */
    "hdr continuation",         /*message name for debugging    */
    sizeof(H5O_cont_t),         /*native message size           */
    H5O_cont_decode,            /*decode message                */
}};


/* copied and modified from H5O.c */
/* ID to type mapping */
/* Only defined for H5O_NULL and H5O_STAB, NULL the rest for NOW */
static const H5O_class_t *const message_type_g[] = {
    H5O_NULL,           /*0x0000 Null                                   */
    NULL, 	        /*0x0001 Simple Dimensionality                  */
    NULL,               /*0x0002 Data space (fiber bundle?)             */
    NULL,               /*0x0003 Data Type                              */
    NULL,           	/*0x0004 Old data storage -- fill value         */
    NULL,       	/*0x0005 New Data storage -- fill value         */
    NULL,               /*0x0006 Data storage -- compact object         */
    NULL,            	/*0x0007 Data storage -- external data files    */
    NULL,         	/*0x0008 Data Layout                            */
    NULL,               /*0x0008 "Bogus"                                */
    NULL,               /*0x000A Not assigned                           */
    NULL,          	/*0x000B Data storage -- filter pipeline        */
    NULL,           	/*0x000C Attribute list                         */
    NULL,           	/*0x000D Object name                            */
    NULL,          	/*0x000E Object modification date and time      */
    NULL,        	/*0x000F Shared header message                  */
    H5O_CONT,          	/*0x0010 Object header continuation             */
    H5O_STAB,           /*0x0011 Symbol table                           */
    NULL       		/*0x0012 New Object modification date and time  */
};


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
    H5O_cont_t             *cont = NULL;
    void                   *ret_value;

    	assert(p);

    	/* decode */
	cont = malloc(sizeof(H5O_cont_t));
	if (cont == NULL) {
		H5E_push("H5O_cont_decode", "Couldn't malloc() H5O_cont_t.", -1);
		return(ret_value=NULL);
	}
	
    	H5F_addr_decode(shared_info, &p, &(cont->addr));
    	H5F_DECODE_LENGTH(shared_info, p, cont->size);
    	cont->chunkno=0;

    	/* Set return value */
    	return(ret_value=cont);
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

    	assert(p);

    	/* decode */
	stab = malloc(sizeof(H5O_stab_t));
	if (stab == NULL) {
		H5E_push("H5O_stab_decode", "Couldn't malloc() H5O_stab_t.", -1);
		return(ret_value=NULL);
	}

	H5F_addr_decode(shared_info, &p, &(stab->btree_addr));
	H5F_addr_decode(shared_info, &p, &(stab->heap_addr));

	/* Set return value */
    	return(ret_value=stab);
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
#ifdef DEBUG
			printf("FOUND super block signature\n");
#endif
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
/* NEEd TO CHECK INTO THIS causing ABORT */
#if 0
            		assert(0 == **pp);  /*overflow */
#endif
        	}
    	}
    	if (all_zero)
        	*addr_p = HADDR_UNDEF;
}



/* Based on H5F_read_superblock() in H5Fsuper.c */
/*
 * 1. Read in the information from the superblock
 * 2. Validate the information obtained.
 */
herr_t
check_superblock(FILE *inputfd)
{
	uint		n;
	uint8_t		buf[H5F_SUPERBLOCK_SIZE];
	uint8_t		*p;
	herr_t		ret;

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
	shared_info.super_addr = locate_super_signature(inputfd);
	if (shared_info.super_addr == HADDR_UNDEF) {
		H5E_push("check_superblock", "Couldn't find super block.", -1);
		printf("Assuming super block at address 0.\n");
		shared_info.super_addr = 0;
		ret++;
	}
	
	printf("Validating the super block at %d...\n\n", shared_info.super_addr);

	fseek(inputfd, shared_info.super_addr, SEEK_SET);
	fread(buf, 1, fixed_size, inputfd);
	/* super block signature already checked */
	p = buf + H5F_SIGNATURE_LEN;

	super_vers = *p++;
	freespace_vers = *p++;
	root_sym_vers = *p++;
	/* skip over reserved byte */
	p++;

	shared_head_vers = *p++;
	shared_info.size_offsets = *p++;
	shared_info.size_lengths = *p++;
	/* skip over reserved byte */
	p++;

	UINT16DECODE(p, shared_info.gr_leaf_node_k);
	UINT16DECODE(p, shared_info.gr_int_node_k);

	/* File consistency flags. Not really used yet */
    	UINT32DECODE(p, shared_info.file_consist_flg);


	/* decode the variable length part of the super block */
	/* Potential indexed storage internal node K */
    	variable_size = (super_vers>0 ? 4 : 0) + 
                    shared_info.size_offsets +  /* base addr*/
                    shared_info.size_offsets +  /* address of global free-space heap */
                    shared_info.size_offsets +  /* end of file address*/
                    shared_info.size_offsets +  /* driver-information block address */
                    H5G_SIZEOF_ENTRY(shared_info); /* root group symbol table entry */

	if ((fixed_size + variable_size) > sizeof(buf)) {
		H5E_push("check_superblock", "Total size of super block is incorrect.", shared_info.super_addr);
		ret++;
		return(ret);
	}

	fseek(inputfd, shared_info.super_addr+fixed_size, SEEK_SET);
	fread(p, 1, variable_size, inputfd);

    	/*
     	 * If the superblock version # is greater than 0, read in the indexed
     	 * storage B-tree internal 'K' value
     	 */
    	if (super_vers > 0) {
        	UINT16DECODE(p, shared_info.btree_k);
        	p += 2;   /* reserved */
    	}


	H5F_addr_decode(shared_info, (const uint8_t **)&p, &shared_info.base_addr/*out*/);
    	H5F_addr_decode(shared_info, (const uint8_t **)&p, &shared_info.freespace_addr/*out*/);
    	H5F_addr_decode(shared_info, (const uint8_t **)&p, &shared_info.stored_eoa/*out*/);
    	H5F_addr_decode(shared_info, (const uint8_t **)&p, &shared_info.driver_addr/*out*/);

	if (H5G_ent_decode(shared_info, (const uint8_t **)&p, &root_ent/*out*/) < 0) {
		H5E_push("check_superblock", "Unable to read root symbol entry.", shared_info.super_addr);
		ret++;
		return(ret);
	}
	shared_info.root_grp = &root_ent;


	}  /* DONE WITH SCANNING and STORING OF SUPERBLOCK INFO */

#ifdef DEBUG 
	printf("super_addr = %ld\n", shared_info.super_addr);
	printf("super_vers=%d, freespace_vers=%d, root_sym_vers=%d\n",	
		super_vers, freespace_vers, root_sym_vers);
	printf("size_offsets=%d, size_lengths=%d\n",	
		shared_info.size_offsets, shared_info.size_lengths);
	printf("gr_leaf_node_k=%d, gr_int_node_k=%d, file_consist_flg=%d\n",	
		shared_info.gr_leaf_node_k, shared_info.gr_int_node_k, shared_info.file_consist_flg);
	printf("base_addr=%d, freespace_addr=%d, stored_eoa=%d, driver_addr=%d\n",
		shared_info.base_addr, shared_info.freespace_addr, shared_info.stored_eoa, shared_info.driver_addr);

	/* print root group table entry */
	printf("name0ffset=%d, header_address=%d\n", 
		shared_info.root_grp->name_off, shared_info.root_grp->header);
	printf("btree_addr=%d, heap_addr=%d\n", 
		shared_info.root_grp->cache.stab.btree_addr, 
		shared_info.root_grp->cache.stab.heap_addr);
#endif

	/* 
	 * superblock signature is already checked
	 * DO the validation of the superblock info
	 */

	/* BEGIN VALIDATION */

	/* fixed size part validation */
	if (super_vers != HDF5_SUPERBLOCK_VERSION_DEF && 
	    super_vers != HDF5_SUPERBLOCK_VERSION_MAX) {
		H5E_push("check_superblock", "Version number of the superblock is incorrect.", shared_info.super_addr);
		ret++;
	}
	if (freespace_vers != HDF5_FREESPACE_VERSION) {
		H5E_push("check_superblock", "Version number of the file free-space information is incorrect.", shared_info.super_addr);
		ret++;
	}
	if (root_sym_vers != HDF5_OBJECTDIR_VERSION) {
		H5E_push("check_superblock", "Version number of the root group symbol table entry is incorrect.", shared_info.super_addr);
		ret++;
	}
	if (shared_head_vers != HDF5_SHAREDHEADER_VERSION) {
		H5E_push("check_superblock", "Version number of the shared header message format is incorrect.", shared_info.super_addr);
		ret++;
	}
	if (shared_info.size_offsets != 2 && shared_info.size_offsets != 4 &&
            shared_info.size_offsets != 8 && shared_info.size_offsets != 16 && 
	    shared_info.size_offsets != 32) {
		H5E_push("check_superblock", "Bad byte number in an address.", shared_info.super_addr);
		ret++;
	}
	if (shared_info.size_lengths != 2 && shared_info.size_lengths != 4 &&
            shared_info.size_lengths != 8 && shared_info.size_lengths != 16 && 
	    shared_info.size_lengths != 32) {
		H5E_push("check_superblock", "Bad byte number for object size.", shared_info.super_addr);
		ret++;
	}
	if (shared_info.gr_leaf_node_k <= 0) {
		H5E_push("check_superblock", "Invalid leaf node of a group B-tree.", shared_info.super_addr);
		ret++;
	}
	if (shared_info.gr_int_node_k <= 0) {
		H5E_push("check_superblock", "Invalid internal node of a group B-tree.", shared_info.super_addr);
		ret++;
	}
	if (shared_info.file_consist_flg > 0x03) {
		H5E_push("check_superblock", "Invalid file consistency flags.", shared_info.super_addr, shared_info.super_addr);
		ret++;
	}


	/* variable size part validation */
    	if (super_vers > 0) { /* indexed storage internal node k */
    		if (shared_info.btree_k <= 0) {
			H5E_push("check_superblock", "Invalid internal node of an indexed storage b-tree.", shared_info.super_addr);
			ret++;
		}
    	}

	/* SHOULD THERE BE MORE VALIDATION of base_addr?? */
	if ((shared_info.base_addr != shared_info.super_addr) ||
	    (shared_info.base_addr >= shared_info.stored_eoa))
	{
		H5E_push("check_superblock", "Invalid base address.", shared_info.super_addr);
		ret++;
	} 

	if (shared_info.freespace_addr != HADDR_UNDEF) {
		H5E_push("check_superblock", "Invalid address of global free-space index.", shared_info.super_addr);
		ret++;
	}

	if (shared_info.stored_eoa == HADDR_UNDEF) {
		H5E_push("check_superblock", "Invalid end of file address.", shared_info.super_addr);
		ret++;
	}

	/* Read in driver information block and validate */
	/* Defined driver information block address or not */
    	if (shared_info.driver_addr != HADDR_UNDEF) {
		haddr_t drv_addr = shared_info.base_addr + shared_info.driver_addr;
        	uint8_t dbuf[H5F_DRVINFOBLOCK_SIZE];     /* Local buffer */
		size_t  driver_size;   /* Size of driver info block, in bytes */


		if (((drv_addr+16) == HADDR_UNDEF) || ((drv_addr+16) >= shared_info.stored_eoa)) {
			H5E_push("check_superblock", "Invalid driver information block.", shared_info.super_addr);
			ret++;
			return(ret);
		}
		/* read in the first 16 bytes of the driver information block */
		fseek(inputfd, drv_addr, SEEK_SET);
		fread(dbuf, 1, 16, inputfd);
		p = dbuf;
		if (*p++ != HDF5_DRIVERINFO_VERSION) {
			H5E_push("check_superblock", "Bad driver information block version number.", shared_info.super_addr);
			ret++;
		}
		p += 3; /* reserved */
		/* Driver info size */
        	UINT32DECODE(p, driver_size);
		p += 8; /* advance past driver identification */
		 /* Read driver information and decode */
        	assert((driver_size + 16) <= sizeof(dbuf));
		if (((drv_addr+16+driver_size) == HADDR_UNDEF) || 
		    ((drv_addr+16+driver_size) >= shared_info.stored_eoa)) {
			H5E_push("check_superblock", "Invalid driver information size.", shared_info.super_addr);
			ret++;
		}
	}  /* DONE with driver information block */


	/* NEED to validate shared_info.root_grp->name_off??? to be within size of local heap */

	if ((shared_info.root_grp->header==HADDR_UNDEF) || 
	    (shared_info.root_grp->header >= shared_info.stored_eoa)) {
		H5E_push("check_superblock", "Invalid object header address.", shared_info.super_addr);
		ret++;
	}
	if (shared_info.root_grp->type < 0) {
		H5E_push("check_superblock", "Invalid cache type.", shared_info.super_addr);
		ret++;
	}
	return(ret);
}


/* Based on H5G_node_load() in H5Gnode.c */
herr_t
check_sym(FILE *inputfd, haddr_t sym_addr) 
{
	size_t 		size = 0;
	uint8_t		*buf = NULL;
	uint8_t		*p;
	herr_t 		ret;
	unsigned	nsyms, u;
	H5G_node_t	*sym=NULL;
	H5G_entry_t	*ent;

	assert(H5F_addr_defined(sym_addr));

	printf("\n\nValidating the SNOD at %d...\n\n", sym_addr);
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
		return(ret);
	}

	sym = malloc(sizeof(H5G_node_t));
	if (sym == NULL) {
		H5E_push("check_sym", "Unable to malloc() H5G_node_t.", sym_addr);
		ret++;
		return(ret);
	}
	sym->entry = malloc(2*H5F_SYM_LEAF_K(shared_info)*sizeof(H5G_entry_t));
	if (sym->entry == NULL) {
		H5E_push("check_sym", "Unable to malloc() H5G_entry_t.", sym_addr);
		ret++;
		return(ret);
	}

	fseek(inputfd, sym_addr, SEEK_SET);
       	if (fread(buf, 1, size, inputfd)<0) {
		H5E_push("check_sym", "Unable to read in the symbol table node.", sym_addr);
		ret++;
		return(ret);
	}

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
#ifdef DEBUG
	printf("sym->nsyms=%d\n",sym->nsyms);
#endif
	if (sym->nsyms > (2 * H5F_SYM_LEAF_K(shared_info))) {
		H5E_push("check_sym", "Number of symbols exceed (2*Group Leaf Node K).", sym_addr);
		ret++;
	}


	/* reading the vector of symbol table group entries */
	ret = H5G_ent_decode_vec(shared_info, (const uint8_t **)&p, sym->entry, sym->nsyms);
	if (ret != SUCCEED) {
		H5E_push("check_sym", "Unable to read in symbol table group entries.", sym_addr);
		ret++;
		return(ret);
	}

	/* validate symbol table group entries here  */
    	for (u=0, ent = sym->entry; u < sym->nsyms; u++, ent++) {
#ifdef DEBUG
		printf("ent->type=%d, ent->name_off=%d, ent->header=%d\n",
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

		ret = check_obj_header(inputfd, ent->header);
		if (ret != SUCCEED) {
			H5E_print(stderr);
			H5E_clear();
			ret = SUCCEED;
		}
	}

	if (sym->entry)
		free(sym->entry);
	if (sym)
		free(sym);
    	if (buf)
        	free(buf);

	return(ret);
}

/* Based on H5B_load() in H5B.c */
herr_t
check_btree(FILE *inputfd, haddr_t btree_addr)
{
	uint8_t		*buf=NULL, *buffer=NULL;
	uint8_t		*p, nodetype;
	unsigned	nodelev, entries, u;
	haddr_t         left_sib;   /*address of left sibling */
	haddr_t         right_sib;  /*address of left sibling */
	size_t		hdr_size, name_offset, key_ptr_size;
	int		ret, status;
	haddr_t		child;



	assert(H5F_addr_defined(btree_addr));
    	hdr_size = H5B_SIZEOF_HDR(shared_info);
	ret = SUCCEED;

	printf("\n\nValidating the btree at %d...\n\n", btree_addr);

#ifdef DEBUG
	printf("btree_addr=%d, hdr_size=%d\n", btree_addr, hdr_size);
#endif

	buf = malloc(hdr_size);
	if (buf == NULL) {
		H5E_push("check_btree", "Unable to malloc() btree header.", btree_addr);
		ret++;
		return(ret);
	}

	 /* read fixed-length part of object header */
	fseek(inputfd, btree_addr, SEEK_SET);
       	if (fread(buf, 1, (size_t)hdr_size, inputfd)<0) {
		H5E_push("check_btree", "Unable to read btree header.", btree_addr);
		ret++;
		return(ret);
	}
	p = buf;

	/* magic number */
	ret = memcmp(p, H5B_MAGIC, (size_t)H5B_SIZEOF_MAGIC);
	if (ret != 0) {
		H5E_push("check_btree", "Couldn't find btree signature.", btree_addr);
		ret++;
		return(ret);
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
	

	fseek(inputfd, btree_addr+hdr_size, SEEK_SET);

	if (nodetype == 0) {
		/* the remaining node size: key + child pointer */
		/* probably should put that in a macro */
		key_ptr_size = (2*shared_info.gr_int_node_k)*H5F_SIZEOF_ADDR(shared_info) +
			(2*(shared_info.gr_int_node_k+1))*H5F_SIZEOF_SIZE(shared_info);

		buffer = malloc(key_ptr_size);
		if (buffer == NULL) {
			H5E_push("check_btree", "Unable to malloc() key+child.", btree_addr);
			ret++;
			return(ret);
		}
       		if (fread(buffer, 1, key_ptr_size, inputfd)<0) {
			H5E_push("check_btree", "Unable to read key+child.", btree_addr);
			ret++;
			return(ret);
		}
		p = buffer;
		
		for (u = 0; u < entries; u++) {
			H5F_DECODE_LENGTH(shared_info, p, name_offset);
#ifdef DEBUG
			printf("name_offset=%d\n", name_offset);
#endif

			/* NEED TO VALIDATE name_offset to be within the local heap's size??HOW */

        		/* Decode address value */
  			H5F_addr_decode(shared_info, (const uint8_t **)&p, &child/*out*/);
#ifdef DEBUG
			printf("child=%d\n", child);
#endif

			if ((child != HADDR_UNDEF) && (child >= shared_info.stored_eoa)) {
				H5E_push("check_btree", "Invalid child address.", btree_addr);
				ret++;
				continue;
			}

			if (nodelev > 0) {
				printf("Internal node pointing to sub-trees\n");	
				printf("name_offset=%d\n", name_offset);
				printf("child=%d\n", child);
				check_btree(inputfd, child);
			} else {
				status = check_sym(inputfd, child);
				if (status != SUCCEED) {
					H5E_print(stderr);
					H5E_clear();
					ret = SUCCEED;
					continue;
				}
			}
		}
		/* decode final key */
		if (entries > 0) {
			H5F_DECODE_LENGTH(shared_info, p, name_offset);
			printf("final name_offset=%d\n", name_offset);
		}

	} else if ((nodelev == 0) && (nodetype == 1)) {  /* this tree points to raw data chunks */
		printf("This is a leaf node pointing raw data chunks\n");
	} else { 
		H5E_push("check_btree", "Inconsistent node level and type.", btree_addr);
		ret++;
	}

	if (buf)
		free(buf);
	if (buffer)
		free(buffer);

	return(ret);
}


/* copied and modified from H5HL_load() of H5HL.c */
herr_t
check_lheap(FILE *inputfd, haddr_t lheap_addr)
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

	printf("\n\nValidating the local heap at %d...\n\n", lheap_addr);
	ret = SUCCEED;

	fseek(inputfd, lheap_addr, SEEK_SET);
       	if (fread(hdr, 1, (size_t)hdr_size, inputfd)<0) {
		H5E_push("check_lheap", "Unable to read local heap header.", lheap_addr);
		ret++;
		return(ret);
	}
    	p = hdr;

	/* magic number */
	ret = memcmp(p, H5HL_MAGIC, H5HL_SIZEOF_MAGIC);
	if (ret != 0) {
		H5E_push("check_lheap", "Couldn't find local heap signature.", lheap_addr);
		ret++;
		return(ret);
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
		return(ret);
	}

	/* offset to head of free-list */
    	H5F_DECODE_LENGTH(shared_info, p, next_free_off);
	if ((haddr_t)next_free_off != HADDR_UNDEF && next_free_off != H5HL_FREE_NULL && next_free_off >= data_seg_size) {
		H5E_push("check_lheap", "Offset to head of free list is invalid.", lheap_addr);
		ret++;
		return(ret);
	}

	/* address of data segment */
    	H5F_addr_decode(shared_info, &p, &addr_data_seg);
	if ((addr_data_seg == HADDR_UNDEF) || (addr_data_seg >= shared_info.stored_eoa)) {
		H5E_push("check_lheap", "Address of data segment is invalid.", lheap_addr);
		ret++;
		return(ret);
	}

	heap_chunk = malloc(hdr_size+data_seg_size);
	if (heap_chunk == NULL) {
		H5E_push("check_lheap", "Memory allocation failed for local heap data segment.", lheap_addr);
		ret++;
		return(ret);
	}
	
#ifdef DEBUG
	printf("data_seg_size=%d, next_free_off =%d, addr_data_seg=%d\n",
		data_seg_size, next_free_off, addr_data_seg);
#endif

	if (data_seg_size) {
		fseek(inputfd, addr_data_seg, SEEK_SET);
       		if (fread(heap_chunk+hdr_size, 1, data_seg_size, inputfd)<0) {
			H5E_push("check_lheap", "Unable to read local heap data segment.", lheap_addr);
			ret++;
			return(ret);
		}
	} 


	/* traverse the free list */
	while (next_free_off != H5HL_FREE_NULL) {
		if (next_free_off >= data_seg_size) {
			H5E_push("check_lheap", "Offset of the next free block is invalid.", lheap_addr);
			ret++;
			return(ret);
		}
		saved_offset = next_free_off;
		p = heap_chunk + hdr_size + next_free_off;
		H5F_DECODE_LENGTH(shared_info, p, next_free_off);
		H5F_DECODE_LENGTH(shared_info, p, size_free_block);
#ifdef DEBUG
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
			return(ret);
		}
	}

	if (heap_chunk)
		free(heap_chunk);
	return(ret);
}

/* copied and modified from H5O_load() of H5O.c */
/* JUST DO A SIMPLE VERSION OF reading in the header messages */
/* should handle chunks not just one chunk and more messages */
/* SHOULD put decode messages routine in another file, I think */
herr_t
check_obj_header(FILE *inputfd, haddr_t obj_head_addr)
{
	size_t		hdr_size, chunk_size;
	haddr_t		chunk_addr;
	uint8_t		buf[16], *p, flags;
	uint8_t		*chunk_image;
	unsigned	nmesgs, id;
	size_t		mesg_size;
	int		version, nlink, ret;

	H5O_stab_t 	*stab;

	H5O_cont_t	*cont;
	uint8_t		*cont_chunk_image;
	uint8_t		*pp, pp_flags;
	unsigned	pp_id;
	size_t		pp_mesg_size;
	H5O_stab_t 	*pp_stab;

	assert(H5F_addr_defined(obj_head_addr));
    	hdr_size = H5O_SIZEOF_HDR(shared_info);
    	assert(hdr_size<=sizeof(buf));

	printf("\n\nValidating the object header at %d...\n\n", obj_head_addr);
	ret = SUCCEED;

#ifdef DEBUG
	printf("obj_head_addr=%d, hdr_size=%d\n", 
		obj_head_addr, hdr_size);
#endif

	 /* read fixed-length part of object header */
	fseek(inputfd, obj_head_addr, SEEK_SET);
       	if (fread(buf, 1, (size_t)hdr_size, inputfd)<0) {
		H5E_push("check_obj_header", "Unable to read object header.", obj_head_addr);
		ret++;
		return(ret);
	}
    	p = buf;
	version = *p++;
	if (version != 1) {
		H5E_push("check_obj_header", "Invalid version number.", obj_head_addr);
		ret++;
	}
	p++;
	UINT16DECODE(p, nmesgs);
	if ((int)nmesgs < 0) {  /* ??? */
		H5E_push("check_obj_header", "Incorrect number of header messages.", obj_head_addr);
		ret++;
	}

	UINT32DECODE(p, nlink);
	/* DON'T KNOW HOW TO VADIATE object reference count yet */
	

	chunk_addr = obj_head_addr + (size_t)hdr_size;
	UINT32DECODE(p, chunk_size);

#ifdef DEBUG
	printf("version =%d, nmesgs=%d, nlink=%d\n", version, nmesgs, nlink);
	printf("chunk_addr=%d, chunk_size=%d\n", chunk_addr, chunk_size);
#endif

	/* read in the chunk of header messages */
	chunk_image = malloc(chunk_size);
	if (chunk_image == NULL) {
		H5E_push("check_obj_header", "Unable to malloc() header messages.", obj_head_addr);
		ret++;
		return(ret);
	}

	assert(H5F_addr_defined(chunk_addr));
	fseek(inputfd, chunk_addr, SEEK_SET);
       	if (fread(chunk_image, 1, (size_t)chunk_size, inputfd)<0) {
		H5E_push("check_obj_header", "Unable to read chunk of header messages.", obj_head_addr);
		ret++;
		return(ret);
	}
	
	/* load messages from this chunk */
	/* should handle multiple chunks and more than NMESGS */
	p = chunk_image;
	mesg_size = 0;
	for (p = chunk_image; p < chunk_image+chunk_size; p+=mesg_size) {
		/* NEED TO HANDLE CONTINUATION message for more btree */
		UINT16DECODE(p, id);
            	UINT16DECODE(p, mesg_size);

#ifdef DEBUG
		printf("\nid=%d, mesg_size=%d\n", id, mesg_size);
#endif

		assert(mesg_size==H5O_ALIGN(mesg_size));
		flags = *p++;
		p += 3;  /* reserved */

		/* Try to detect invalidly formatted object header messages */
            	if (p + mesg_size > chunk_image + chunk_size) {
			H5E_push("check_obj_header", "Corrupt object header message.", obj_head_addr);
			ret++;
			return(ret);
		}

            	/* Skip header messages we don't know about */
            	if (id >= NELMTS(message_type_g) || NULL == message_type_g[id]) {
			printf("Unknown header message id=%d\n", id);
                	continue;
		}
		if (id == H5O_NULL_ID) {
			printf("Encountered a NULL message id\n");
		} else if (id == H5O_STAB_ID) {
			printf("\nEncountered a symbol table/group message id\n");
			stab = (H5O_STAB->decode) (p);
			if (stab == NULL) {
				H5E_push("check_obj_header", "Failure in H5O_STAB->decode().", obj_head_addr);
				ret++;
				return(ret);
			}
#ifdef DEBUG
			printf("stab->btree_addr=%d,stab->heap_addr=%d\n",
				stab->btree_addr, stab->heap_addr);
#endif
			ret = check_btree(inputfd, stab->btree_addr);
			if (ret != SUCCEED) {
				H5E_print(stderr);
				H5E_clear();
				ret = SUCCEED;
			}
			ret = check_lheap(inputfd, stab->heap_addr);
			if (ret != SUCCEED) {
				H5E_print(stderr);
				H5E_clear();
				ret = SUCCEED;
			}
		} else
			printf("message id=%d not handled yet!\n", id);
	}
	
	if (chunk_image)
		free(chunk_image);
	return(ret);
}


int main(int argc, char **argv)
{
	haddr_t	ret;
	FILE 	*inputfd;

	haddr_t	gheap_addr;

	printf("\nVALIDATING %s\n\n", INPUTFILE);

	inputfd = fopen(INPUTFILE, "r");

        if (inputfd == NULL) {
                fprintf(stderr, "fopen(\"%s\") failed: %s\n", INPUTFILE,
                                 strerror(errno));
                exit(errno);
        }


	ret = check_superblock(inputfd);
	if (ret != SUCCEED) {
		H5E_print(stderr);
		H5E_clear();
		ret = SUCCEED;
	}

	ret = check_obj_header(inputfd, shared_info.root_grp->header);
	if (ret != SUCCEED) {
		H5E_print(stderr);
		H5E_clear();
		ret = SUCCEED;
	}

	fclose(inputfd);
}

