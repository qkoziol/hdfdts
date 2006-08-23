/* need to take care of haddr_t and HADDR_UNDEF */
/* see H5public.h for definition of haddr_t, H5pubconf.h */
typedef	unsigned long		haddr_t;
#define	HADDR_UNDEF		((haddr_t)(-1))



/* 3 defines copied from H5Fpkg.h */
#define H5F_SIGNATURE     "\211HDF\r\n\032\n"
#define H5F_SIGNATURE_LEN 8
#define H5F_SUPERBLOCK_SIZE  256
#define H5F_DRVINFOBLOCK_SIZE  1024




/* copied from H5Fprivate.h */
#define UINT16DECODE(p, i) {                                                  \
   (i)  = (uint16_t) (*(p) & 0xff);       (p)++;                              \
   (i) |= (uint16_t)((*(p) & 0xff) << 8); (p)++;                              \
}

/* copied from H5Fprivate.h */
#define UINT32DECODE(p, i) {                                                  \
   (i)  =  (uint32_t)(*(p) & 0xff);        (p)++;                             \
   (i) |= ((uint32_t)(*(p) & 0xff) <<  8); (p)++;                             \
   (i) |= ((uint32_t)(*(p) & 0xff) << 16); (p)++;                             \
   (i) |= ((uint32_t)(*(p) & 0xff) << 24); (p)++;                             \
}


/* copied from H5Fprivate.h */
#define UINT64DECODE(p, n) {                                                  \
   /* WE DON'T CHECK FOR OVERFLOW! */                                         \
   size_t _i;                                                                 \
   n = 0;                                                                     \
   (p) += 8;                                                                  \
   for (_i=0; _i<sizeof(uint64_t); _i++) {                                    \
      n = (n<<8) | *(--p);                                                    \
   }                                                                          \
   (p) += 8;                                                                  \
}


/* copied from H5Fprivate.h */
#define H5F_DECODE_LENGTH(F,p,l) switch(H5F_SIZEOF_SIZE(F)) {     			      \
   case 4: UINT32DECODE(p,l); break;                                          \
   case 8: UINT64DECODE(p,l); break;                                          \
   case 2: UINT16DECODE(p,l); break;                                          \
}

/* copied from H5Fprivate.h */
#define H5F_addr_defined(X)     (X!=HADDR_UNDEF)


/* copied and modified from H5Fprivate.h */
#define H5F_SIZEOF_ADDR(F)      ((F).size_offsets)
#define H5F_SIZEOF_SIZE(F)      ((F).size_lengths)
#define H5F_SYM_LEAF_K(F)       ((F).gr_leaf_node_k)


/* copied from H5public.h: */
typedef unsigned int hbool_t;
typedef int herr_t;


/* copied from H5private.h */
/* Version #'s of the major components of the file format */
#define HDF5_SUPERBLOCK_VERSION_DEF     0       /* The default super block format         */
#define HDF5_SUPERBLOCK_VERSION_MAX     1       /* The maximum super block format         */
#define HDF5_FREESPACE_VERSION  0       /* of the Free-Space Info         */
#define HDF5_OBJECTDIR_VERSION  0       /* of the Object Directory format */
#define HDF5_SHAREDHEADER_VERSION 0     /* of the Shared-Header Info      */
#define HDF5_DRIVERINFO_VERSION 0       /* of the Driver Information Block*/

/* copied from H5private.h */
/*
 * Status return values for the `herr_t' type.
 * Since some unix/c routines use 0 and -1 (or more precisely, non-negative
 * vs. negative) as their return code, and some assumption had been made in
 * the code about that, it is important to keep these constants the same
 * values.  When checking the success or failure of an integer-valued
 * function, remember to compare against zero and not one of these two
 * values.
 */
#define SUCCEED         0
#define FAIL            (-1)
#define UFAIL           (unsigned)(-1)


/* copied from H5private.h */
/*
 * HDF Boolean type.
 */
#define MAX(a,b)            (((a)>(b)) ? (a) : (b))
#ifndef FALSE
#   define FALSE 0
#endif
#ifndef TRUE
#   define TRUE 1
#endif

/* copied from H5private.h */
#define NELMTS(X)           (sizeof(X)/sizeof(X[0]))

/* copied from H5Gprivate.h */
/* The disk size for a group entry */
#define H5G_SIZEOF_SCRATCH      16
#define H5G_SIZEOF_ENTRY(F) 			\
   (H5F_SIZEOF_SIZE(F) +   /* name offset */    		\
    H5F_SIZEOF_ADDR(F) +   /* object header address */    	\
    4 +        /* cache type */    				\
    4 +        /* reserved   */    				\
    H5G_SIZEOF_SCRATCH) /* scratch pad space */


/* copied from H5Gprivate.h */
/*
 * Various types of object header information can be cached in a symbol
 * table entry (it's normal home is the object header to which the entry
 * points).  This datatype determines what (if anything) is cached in the
 * symbol table entry.
 */
typedef enum H5G_type_t {
    H5G_CACHED_ERROR    = -1,   /*force enum to be signed       */
    H5G_NOTHING_CACHED  = 0,    /*nothing is cached, must be 0  */
    H5G_CACHED_STAB     = 1,    /*symbol table, `stab'          */
    H5G_CACHED_SLINK    = 2,    /*symbolic link                 */
    H5G_NCACHED         = 3     /*THIS MUST BE LAST             */
} H5G_type_t;


/* copied from H5Gprivate.h */
/*
 * A symbol table entry caches these parameters from object header
 * messages...  The values are entered into the symbol table when an object
 * header is created (by hand) and are extracted from the symbol table with a
 *  callback function registered in H5O_init_interface().  Be sure to update
 * H5G_ent_decode(), H5G_ent_encode(), and H5G_ent_debug() as well.
 */
typedef union H5G_cache_t {
    struct {
        haddr_t btree_addr;             /*file address of symbol table B-tree*/
        haddr_t heap_addr;              /*file address of stab name heap     */
    } stab;

    struct {
        size_t  lval_offset;            /*link value offset                  */
    } slink;
} H5G_cache_t;


/* copied and modified from H5Gprivate.h */
/*
 * A symbol table entry.  The two important fields are `name_off' and
 * `header'.  The remaining fields are used for caching information that
 * also appears in the object header to which this symbol table entry
 * points.
 */
typedef struct H5G_entry_t {
    H5G_type_t  type;                   /*type of information cached         */
    H5G_cache_t cache;                  /*cached data from object header     */
    size_t      name_off;               /*offset of name within name heap    */
    haddr_t     header;                 /*file address of object header      */
} H5G_entry_t;


/* copied from H5Gpkg.h */
/*
 * A symbol table node is a collection of symbol table entries.  It can
 * be thought of as the lowest level of the B-link tree that points to
 * a collection of symbol table entries that belong to a specific symbol
 * table or group.
 */
typedef struct H5G_node_t {
    unsigned nsyms;                     /*number of symbols                  */
    H5G_entry_t *entry;                 /*array of symbol table entries      */
} H5G_node_t;


/* copied and modified from "typedef struct H5F_file_t" of H5Fpkg.h */
/* NEED TO DETERMINE what info to be included on there */
/*
 *  A global structure for storing the information obtained
 *  from the superblock to be shared by all routines.
 */
typedef struct 	H5F_shared_t {
	haddr_t		super_addr;	    /* absolute address of the super block */
        size_t          size_offsets;       /* size of offsets: sizeof_addr */
        size_t          size_lengths;       /* size of lengths: sizeof_size */
	unsigned	gr_leaf_node_k;	    /* group leaf node k */
	unsigned	gr_int_node_k;	    /* group internal node k */
        uint32_t        file_consist_flg;   /* file consistency flags */
        unsigned        btree_k;            /* indexed storage internal node k */
        haddr_t         base_addr;          /* absolute base address for rel.addrs. */
        haddr_t         freespace_addr;     /* relative address of free-space info  */
        haddr_t         stored_eoa;         /* end of file address */
        haddr_t         driver_addr;        /* file driver information block address*/
	H5G_entry_t 	*root_grp;	    /* ?? */
} H5F_shared_t;


/* copied from H5Opkg.h */
/*
 * Align messages on 8-byte boundaries because we would like to copy the
 * object header chunks directly into memory and operate on them there, even
 * on 64-bit architectures.  This allows us to reduce the number of disk I/O
 * requests with a minimum amount of mem-to-mem copies.
 */
#define H5O_ALIGN(X)            (8*(((X)+8-1)/8))


/* copied from H5Opkg.h */
/*
/*
 * Size of object header header.
 */
#define H5O_SIZEOF_HDR(F)                                                     \
    H5O_ALIGN(1 +               /*version number        */                    \
              1 +               /*alignment             */                    \
              2 +               /*number of messages    */                    \
              4 +               /*reference count       */                    \
	      4)                /*header data size      */



/* copied from H5Oprivate.h */
/* Header message IDs */
#define H5O_NULL_ID     0x0000          /* Null Message.  */
#define H5O_SDSPACE_ID  0x0001          /* Simple Dataspace Message.  */
/* Complex dataspace is/was planned for message 0x0002 */
#define H5O_DTYPE_ID    0x0003          /* Datatype Message.  */
#define H5O_FILL_ID     0x0004          /* Fill Value Message. (Old)  */
#define H5O_FILL_NEW_ID 0x0005          /* Fill Value Message. (New)  */
/* Compact data storage is/was planned for message 0x0006 */
#define H5O_EFL_ID      0x0007          /* External File List Message  */
#define H5O_LAYOUT_ID   0x0008          /* Data Storage Layout Message.  */
#define H5O_BOGUS_ID    0x0009          /* "Bogus" Message.  */
/* message 0x000a appears unused... */
#define H5O_PLINE_ID    0x000b          /* Filter pipeline message.  */
#define H5O_ATTR_ID     0x000c          /* Attribute Message.  */
#define H5O_NAME_ID     0x000d          /* Object name message.  */
#define H5O_MTIME_ID    0x000e          /* Modification time message. (Old)  */
#define H5O_SHARED_ID   0x000f          /* Shared object message.  */
#define H5O_CONT_ID     0x0010          /* Object header continuation message.  */
#define H5O_STAB_ID     0x0011          /* Symbol table message.  */
#define H5O_MTIME_NEW_ID 0x0012         /* Modification time message. (New)  */


/* copied and modified from H5Oprivate.h */
/*
 * Symbol table message.
 * (Data structure in memory)
 */
typedef struct H5O_stab_t {
    haddr_t     btree_addr;             /*address of B-tree                  */
    haddr_t     heap_addr;              /*address of name heap               */
} H5O_stab_t;


/* copied and modified from H5Oprivate.h */
/*
 * Object header continuation message.
 * (Data structure in memory)
 */

typedef struct H5O_cont_t {
    haddr_t     addr;                   /*address of continuation block      */
    size_t      size;                   /*size of continuation block         */

    /* the following field(s) do not appear on disk */
    unsigned    chunkno;                /*chunk this mesg refers to          */
} H5O_cont_t;


/* copied and modified from H5Opkg.h */
typedef struct H5O_class_t {
    int id;                             /*message type ID on disk   */
    const char  *name;                  /*for debugging             */
    size_t      native_size;            /*size of native message    */
    void        *(*decode)(const uint8_t*);
} H5O_class_t;


/* copied H5Bprivate.h */
#define H5B_SIZEOF_MAGIC 4         /* size of magic number               */
/* copied from H5B.c */
#define H5B_SIZEOF_HDR(F)                                                     \
   (H5B_SIZEOF_MAGIC +          /* magic number                            */  \
    4 +                         /* type, level, num entries                */  \
    2*H5F_SIZEOF_ADDR(F)) /* left and right sibling addresses    */

/* copied from H5Bprivate.h */
#define H5B_MAGIC  "TREE"          /* tree node magic number */


/* copied from H5Gprivate.h */
#define H5G_NODE_MAGIC  "SNOD"          /*symbol table node magic number     */
#define H5G_NODE_SIZEOF_MAGIC 4         /*sizeof symbol node magic number    */


/* copied from H5Gnode.c */
#define H5G_NODE_VERS   1               /*symbol table node version number   */
#define H5G_NODE_SIZEOF_HDR(F) (H5G_NODE_SIZEOF_MAGIC + 4)

/* copied from H5HLprivate.h */
#define H5HL_MAGIC      "HEAP"          /* heap magic number */
#define H5HL_SIZEOF_MAGIC 4
#define H5HL_ALIGN(X)   (((X)+7)&(unsigned)(~0x07)) /*align on 8-byte boundary  */

/* copied from H5HL.c */
/*
 * Local heap collection version.
 */
#define H5HL_VERSION    0
#define H5HL_FREE_NULL  1         	/*end of free list on disk      */



/* copied from H5HLpkg.h */
#define H5HL_SIZEOF_HDR(F)                                                    \
    H5HL_ALIGN(H5HL_SIZEOF_MAGIC +      /*heap signature                */    \
               4 +                      /*reserved                      */    \
               H5F_SIZEOF_SIZE (F) +    /*data size                     */    \
               H5F_SIZEOF_SIZE (F) +    /*free list head                */    \
               H5F_SIZEOF_ADDR (F))     /*data address                  */


/* copied from H5HG.c */
/*
 * The size of the collection header, always a multiple of the alignment so
 * that the stuff that follows the header is aligned.
 */
#define H5HG_SIZEOF_HDR(f)                                                    \
    H5HG_ALIGN(4 +                      /*magic number          */            \
               1 +                      /*version number        */            \
               3 +                      /*reserved              */            \
               H5F_SIZEOF_SIZE(f))      /*collection size       */


/* copied from H5HG.c */
#define H5HG_MINSIZE     4096

/* copied from H5HGprivate.h */
#define H5HG_SIZEOF_MAGIC 4

/* copied and modified from H5HGpkg.h */
typedef struct H5HG_obj_t {
    int         nrefs;          /*reference count               */
    size_t              size;           /*total size of object          */
    uint8_t             *begin;         /*ptr to object into heap->chunk*/
} H5HG_obj_t;

/* copied and modified from H5HGpkg.h */
struct H5HG_heap_t {
    haddr_t             addr;           /*collection address            */
    size_t              size;           /*total size of collection      */
    uint8_t             *chunk;         /*the collection, incl. header  */
    size_t              nalloc;         /*numb object slots allocated   */
    size_t              nused;          /*number of slots used          */
                                        /* If this value is >65535 then all indices */
                                        /* have been used at some time and the */
                                        /* correct new index should be searched for */
    H5HG_obj_t  *obj;           /*array of object descriptions  */
};

