#define         EXIT_SUCCESS		0
#define         EXIT_COMMAND_FAILURE    1
#define         EXIT_FORMAT_FAILURE     2
#define		VERSION			1

#define DEFAULT_VERBOSE 1
#define TERSE_VERBOSE   0
#define DEBUG_VERBOSE   2

extern int	g_verbose_num;

/* need to take care of haddr_t and HADDR_UNDEF */
/* see H5public.h for definition of haddr_t, H5pubconf.h */
typedef	unsigned long long		haddr_t;
#define	HADDR_UNDEF		((haddr_t)(-1))
#define HADDR_MAX            (HADDR_UNDEF-1)


/* need to take care of haddr_t and HADDR_UNDEF */
/* see H5public.h for definition of hsize_t, H5pubconf.h */
typedef size_t                  hsize_t;



#define H5F_SIGNATURE     "\211HDF\r\n\032\n"
#define H5F_SIGNATURE_LEN 8
#define H5F_SUPERBLOCK_SIZE  256
#define H5F_DRVINFOBLOCK_SIZE  1024


#define SEC2_DRIVER	1
#define MULTI_DRIVER	2



#define UINT16DECODE(p, i) {                                                  \
   (i)  = (uint16_t) (*(p) & 0xff);       (p)++;                              \
   (i) |= (uint16_t)((*(p) & 0xff) << 8); (p)++;                              \
}

#define UINT32DECODE(p, i) {                                                  \
   (i)  =  (uint32_t)(*(p) & 0xff);        (p)++;                             \
   (i) |= ((uint32_t)(*(p) & 0xff) <<  8); (p)++;                             \
   (i) |= ((uint32_t)(*(p) & 0xff) << 16); (p)++;                             \
   (i) |= ((uint32_t)(*(p) & 0xff) << 24); (p)++;                             \
}


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


#define H5F_DECODE_LENGTH(F,p,l) switch(H5F_SIZEOF_SIZE(F)) {     			      \
   case 4: UINT32DECODE(p,l); break;                                          \
   case 8: UINT64DECODE(p,l); break;                                          \
   case 2: UINT16DECODE(p,l); break;                                          \
}

#define INT32DECODE(p, i) {                                                 \
   (i)  = (          *(p) & 0xff);        (p)++;                            \
   (i) |= ((int32_t)(*(p) & 0xff) <<  8); (p)++;                            \
   (i) |= ((int32_t)(*(p) & 0xff) << 16); (p)++;                            \
   (i) |= ((int32_t)(((*(p) & 0xff) << 24) |                                \
                   ((*(p) & 0x80) ? ~0xffffffff : 0x0))); (p)++;             \
}

#define H5_CHECK_OVERFLOW(var,vartype,casttype) \
{                                               \
    casttype _tmp_overflow=(casttype)(var);     \
    assert((var)==(vartype)_tmp_overflow);      \
}


#define H5F_addr_defined(X)     (X!=HADDR_UNDEF)


#define H5F_SIZEOF_ADDR(F)      ((F).size_offsets)
#define H5F_SIZEOF_SIZE(F)      ((F).size_lengths)
#define H5F_SYM_LEAF_K(F)       ((F).gr_leaf_node_k)


typedef unsigned int hbool_t;
typedef int herr_t;


#define HDF5_SUPERBLOCK_VERSION_DEF     0       /* The default super block format         */
#define HDF5_SUPERBLOCK_VERSION_MAX     1       /* The maximum super block format         */
#define HDF5_FREESPACE_VERSION  0       /* of the Free-Space Info         */
#define HDF5_OBJECTDIR_VERSION  0       /* of the Object Directory format */
#define HDF5_SHAREDHEADER_VERSION 0     /* of the Shared-Header Info      */
#define HDF5_DRIVERINFO_VERSION 0       /* of the Driver Information Block*/

#define SUCCEED         0
#define FAIL            (-1)
#define UFAIL           (unsigned)(-1)


#define MAX(a,b)            (((a)>(b)) ? (a) : (b))
#ifndef FALSE
#   define FALSE 0
#endif
#ifndef TRUE
#   define TRUE 1
#endif

#define NELMTS(X)           (sizeof(X)/sizeof(X[0]))

#define H5G_SIZEOF_SCRATCH      16
#define H5G_SIZEOF_ENTRY(F) 			\
   (H5F_SIZEOF_SIZE(F) +   /* name offset */    		\
    H5F_SIZEOF_ADDR(F) +   /* object header address */    	\
    4 +        /* cache type */    				\
    4 +        /* reserved   */    				\
    H5G_SIZEOF_SCRATCH) /* scratch pad space */


typedef enum H5G_type_t {
    H5G_CACHED_ERROR    = -1,   /*force enum to be signed       */
    H5G_NOTHING_CACHED  = 0,    /*nothing is cached, must be 0  */
    H5G_CACHED_STAB     = 1,    /*symbol table, `stab'          */
    H5G_CACHED_SLINK    = 2,    /*symbolic link                 */
    H5G_NCACHED         = 3     /*THIS MUST BE LAST             */
} H5G_type_t;


typedef union H5G_cache_t {
    struct {
        haddr_t btree_addr;             /*file address of symbol table B-tree*/
        haddr_t heap_addr;              /*file address of stab name heap     */
    } stab;

    struct {
        size_t  lval_offset;            /*link value offset                  */
    } slink;
} H5G_cache_t;


typedef struct H5G_entry_t {
    H5G_type_t  type;                   /*type of information cached         */
    H5G_cache_t cache;                  /*cached data from object header     */
    size_t      name_off;               /*offset of name within name heap    */
    haddr_t     header;                 /*file address of object header      */
} H5G_entry_t;


typedef struct H5G_node_t {
    unsigned nsyms;                     /*number of symbols                  */
    H5G_entry_t *entry;                 /*array of symbol table entries      */
} H5G_node_t;

typedef struct H5FD_t H5FD_t;


typedef struct H5FD_class_t H5FD_class_t;

typedef struct H5FD_multi_fapl_t	H5FD_multi_fapl_t;


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
	int		driverid;	    /* the driver id to be used */
	void		*fa;	    	    /* driver specific info */
} H5F_shared_t;


#define H5O_ALIGN(X)            (8*(((X)+8-1)/8))


#define H5O_SIZEOF_HDR(F)                                                     \
    H5O_ALIGN(1 +               /*version number        */                    \
              1 +               /*alignment             */                    \
              2 +               /*number of messages    */                    \
              4 +               /*reference count       */                    \
	      4)                /*header data size      */



#define H5O_NULL_ID     0x0000          /* Null Message.  */
#define H5O_SDSPACE_ID  0x0001          /* Simple Dataspace Message.  */
#define H5O_DTYPE_ID    0x0003          /* Datatype Message.  */
#define H5O_FILL_ID     0x0004          /* Fill Value Message. (Old)  */
#define H5O_FILL_NEW_ID 0x0005          /* Fill Value Message. (New)  */
#define H5O_EFL_ID      0x0007          /* External File List Message  */
#define H5O_LAYOUT_ID   0x0008          /* Data Storage Layout Message.  */
#define H5O_BOGUS_ID    0x0009          /* "Bogus" Message.  */
#define H5O_PLINE_ID    0x000b          /* Filter pipeline message.  */
#define H5O_ATTR_ID     0x000c          /* Attribute Message.  */
#define H5O_NAME_ID     0x000d          /* Object name message.  */
#define H5O_MTIME_ID    0x000e          /* Modification time message. (Old)  */
#define H5O_SHARED_ID   0x000f          /* Shared object message.  */
#define H5O_CONT_ID     0x0010          /* Object header continuation message.  */
#define H5O_STAB_ID     0x0011          /* Symbol table message.  */
#define H5O_MTIME_NEW_ID 0x0012         /* Modification time message. (New)  */


#define H5S_MAX_RANK    32

typedef enum H5S_class_t {
    H5S_NO_CLASS         = -1,  /*error                                      */
    H5S_SCALAR           = 0,   /*scalar variable                            */
    H5S_SIMPLE           = 1,   /*simple data space                          */
    H5S_COMPLEX          = 2    /*complex data space                         */
} H5S_class_t;


#define H5O_SDSPACE_VERSION        1

#define H5S_VALID_MAX   0x01


typedef struct {
    H5S_class_t type;   /* Type of extent */
    hsize_t nelem;      /* Number of elements in extent */

    unsigned rank;      /* Number of dimensions */
    hsize_t *size;      /* Current size of the dimensions */
    hsize_t *max;       /* Maximum size of the dimensions */
} H5S_extent_t;


#define H5O_DTYPE_VERSION_COMPAT        1

#define H5O_DTYPE_VERSION_UPDATED       2

#define H5T_OPAQUE_TAG_MAX      256     /* Maximum length of an opaque tag */
                                        /* This could be raised without too much difficulty */

typedef enum H5T_order_t {
    H5T_ORDER_ERROR      = -1,  /*error                                      */
    H5T_ORDER_LE         = 0,   /*little endian                              */
    H5T_ORDER_BE         = 1,   /*bit endian                                 */
    H5T_ORDER_VAX        = 2,   /*VAX mixed endian                           */
    H5T_ORDER_NONE       = 3    /*no particular order (strings, bits,..)     */
    /*H5T_ORDER_NONE must be last */
} H5T_order_t;

typedef enum H5T_sign_t {
    H5T_SGN_ERROR        = -1,  /*error                                      */
    H5T_SGN_NONE         = 0,   /*this is an unsigned type                   */
    H5T_SGN_2            = 1,   /*two's complement                           */

    H5T_NSGN             = 2    /*this must be last!                         */
} H5T_sign_t;


typedef enum H5T_norm_t {
    H5T_NORM_ERROR       = -1,  /*error                                      */
    H5T_NORM_IMPLIED     = 0,   /*msb of mantissa isn't stored, always 1     */
    H5T_NORM_MSBSET      = 1,   /*msb of mantissa is always 1                */
    H5T_NORM_NONE        = 2    /*not normalized                             */
    /*H5T_NORM_NONE must be last */
} H5T_norm_t;


typedef enum H5T_pad_t {
    H5T_PAD_ERROR        = -1,  /*error                                      */
    H5T_PAD_ZERO         = 0,   /*always set to zero                         */
    H5T_PAD_ONE          = 1,   /*always set to one                          */
    H5T_PAD_BACKGROUND   = 2,   /*set to background value                    */

    H5T_NPAD             = 3    /*THIS MUST BE LAST                          */
} H5T_pad_t;

typedef enum H5T_cset_t {
    H5T_CSET_ERROR       = -1,  /*error                                      */
    H5T_CSET_ASCII       = 0,   /*US ASCII                                   */
    H5T_CSET_RESERVED_1  = 1,   /*reserved for later use                     */
    H5T_CSET_RESERVED_2  = 2,   /*reserved for later use                     */
    H5T_CSET_RESERVED_3  = 3,   /*reserved for later use                     */
    H5T_CSET_RESERVED_4  = 4,   /*reserved for later use                     */
    H5T_CSET_RESERVED_5  = 5,   /*reserved for later use                     */
    H5T_CSET_RESERVED_6  = 6,   /*reserved for later use                     */
    H5T_CSET_RESERVED_7  = 7,   /*reserved for later use                     */
    H5T_CSET_RESERVED_8  = 8,   /*reserved for later use                     */
    H5T_CSET_RESERVED_9  = 9,   /*reserved for later use                     */
    H5T_CSET_RESERVED_10 = 10,  /*reserved for later use                     */
    H5T_CSET_RESERVED_11 = 11,  /*reserved for later use                     */
    H5T_CSET_RESERVED_12 = 12,  /*reserved for later use                     */
    H5T_CSET_RESERVED_13 = 13,  /*reserved for later use                     */
    H5T_CSET_RESERVED_14 = 14,  /*reserved for later use                     */
    H5T_CSET_RESERVED_15 = 15   /*reserved for later use                     */
} H5T_cset_t;


typedef enum H5T_str_t {
    H5T_STR_ERROR        = -1,  /*error                                      */
    H5T_STR_NULLTERM     = 0,   /*null terminate like in C                   */
    H5T_STR_NULLPAD      = 1,   /*pad with nulls                             */
    H5T_STR_SPACEPAD     = 2,   /*pad with spaces like in Fortran            */
    H5T_STR_RESERVED_3   = 3,   /*reserved for later use                     */
    H5T_STR_RESERVED_4   = 4,   /*reserved for later use                     */
    H5T_STR_RESERVED_5   = 5,   /*reserved for later use                     */
    H5T_STR_RESERVED_6   = 6,   /*reserved for later use                     */
    H5T_STR_RESERVED_7   = 7,   /*reserved for later use                     */
    H5T_STR_RESERVED_8   = 8,   /*reserved for later use                     */
    H5T_STR_RESERVED_9   = 9,   /*reserved for later use                     */
    H5T_STR_RESERVED_10  = 10,  /*reserved for later use                     */
    H5T_STR_RESERVED_11  = 11,  /*reserved for later use                     */
    H5T_STR_RESERVED_12  = 12,  /*reserved for later use                     */
    H5T_STR_RESERVED_13  = 13,  /*reserved for later use                     */
    H5T_STR_RESERVED_14  = 14,  /*reserved for later use                     */
    H5T_STR_RESERVED_15  = 15   /*reserved for later use                     */
} H5T_str_t;


typedef enum {
    H5R_BADTYPE     =   (-1),   /*invalid Reference Type                     */
    H5R_OBJECT,                 /*Object reference                           */
    H5R_DATASET_REGION,         /*Dataset Region Reference                   */
    H5R_INTERNAL,               /*Internal Reference                         */
    H5R_MAXTYPE                 /*highest type (Invalid as true type)        */
} H5R_type_t;


typedef enum H5T_class_t {
    H5T_NO_CLASS         = -1,  /*error                                      */
    H5T_INTEGER          = 0,   /*integer types                              */
    H5T_FLOAT            = 1,   /*floating-point types                       */
    H5T_TIME             = 2,   /*date and time types                        */
    H5T_STRING           = 3,   /*character string types                     */
    H5T_BITFIELD         = 4,   /*bit field types                            */
    H5T_OPAQUE           = 5,   /*opaque types                               */
    H5T_COMPOUND         = 6,   /*compound types                             */
    H5T_REFERENCE        = 7,   /*reference types                            */
    H5T_ENUM             = 8,   /*enumeration types                          */
    H5T_VLEN             = 9,   /*Variable-Length types                      */
    H5T_ARRAY            = 10,  /*Array types                                */

    H5T_NCLASSES                /*this must be last                          */
} H5T_class_t;



typedef struct H5T_atomic_t {
    H5T_order_t         order;  /*byte order                                 */
    size_t              prec;   /*precision in bits                          */
    size_t              offset; /*bit position of lsb of value               */
    H5T_pad_t           lsb_pad;/*type of lsb padding                        */
    H5T_pad_t           msb_pad;/*type of msb padding                        */
    union {
        struct {
            H5T_sign_t  sign;   /*type of integer sign                       */
        } i;                    /*integer; integer types                     */

        struct {
            size_t      sign;   /*bit position of sign bit                   */
            size_t      epos;   /*position of lsb of exponent                */
            size_t      esize;  /*size of exponent in bits                   */
            uint64_t    ebias;  /*exponent bias                              */
            size_t      mpos;   /*position of lsb of mantissa                */
            size_t      msize;  /*size of mantissa                           */
            H5T_norm_t  norm;   /*normalization                              */
            H5T_pad_t   pad;    /*type of padding for internal bits          */
        } f;                    /*floating-point types                       */

        struct {
            H5T_cset_t  cset;   /*character set                              */
            H5T_str_t   pad;    /*space or null padding of extra bytes       */
        } s;                    /*string types                               */

        struct {
            H5R_type_t  rtype;  /*type of reference stored                   */
        } r;                    /*reference types                            */
    } u;
} H5T_atomic_t;

typedef enum H5T_sort_t {
    H5T_SORT_NONE       = 0,            /*not sorted                         */
    H5T_SORT_NAME       = 1,            /*sorted by member name              */
    H5T_SORT_VALUE      = 2             /*sorted by memb offset or enum value*/
} H5T_sort_t;


typedef struct H5T_enum_t {
    unsigned    nalloc;         /*num entries allocated              */
    unsigned    nmembs;         /*number of members defined in enum  */
    H5T_sort_t  sorted;         /*how are members sorted?            */
    uint8_t     *value;         /*array of values                    */
    char        **name;         /*array of symbol names              */
} H5T_enum_t;


typedef struct H5T_compnd_t {
    unsigned    nalloc;         /*num entries allocated in MEMB array*/
    unsigned    nmembs;         /*number of members defined in struct*/
    H5T_sort_t  sorted;         /*how are members sorted?            */
    hbool_t     packed;         /*are members packed together?       */
    struct H5T_cmemb_t  *memb;  /*array of struct members            */
} H5T_compnd_t;

typedef enum {
    H5T_VLEN_BADTYPE =  -1, /* invalid VL Type */
    H5T_VLEN_SEQUENCE=0,    /* VL sequence */
    H5T_VLEN_STRING,        /* VL string */
    H5T_VLEN_MAXTYPE        /* highest type (Invalid as true type) */
} H5T_vlen_type_t;

typedef struct H5T_vlen_t {
    H5T_vlen_type_t     type;   /* Type of VL data in buffer */
    H5T_cset_t          cset;   /* For VL string. character set */
    H5T_str_t           pad;    /* For VL string.  space or null padding of
                                 * extra bytes */
} H5T_vlen_t;

typedef struct H5T_opaque_t {
    char                *tag;           /*short type description string      */
} H5T_opaque_t;


typedef struct H5T_array_t {
    size_t      nelem;          /* total number of elements in array */
    int         ndims;          /* member dimensionality        */
    size_t      dim[H5S_MAX_RANK];  /* size in each dimension       */
    int         perm[H5S_MAX_RANK]; /* index permutation            */
} H5T_array_t;

typedef struct H5T_shared_t {
    hsize_t             fo_count; /* number of references to this file object */
    H5T_class_t         type;   /*which class of type is this?               */
    size_t              size;   /*total size of an instance of this type     */
    struct H5T_t        *parent;/*parent type for derived datatypes          */
    union {
        H5T_atomic_t    atomic; /* an atomic datatype              */
        H5T_compnd_t    compnd;
        H5T_enum_t      enumer;
        H5T_vlen_t      vlen; 
        H5T_array_t     array;  /* an array datatype                */
        H5T_opaque_t    opaque;
    } u;
} H5T_shared_t;

typedef struct H5T_t H5T_t;

struct H5T_t {
    H5G_entry_t     ent;    /* entry information if the type is a named type */
    H5T_shared_t   *shared; /* all other information */
};

typedef struct H5T_cmemb_t {
    char                *name;          /*name of this member                */
    size_t              offset;         /*offset from beginning of struct    */
    size_t              size;           /*total size: dims * type_size       */
    struct H5T_t        *type;          /*type of this member                */
} H5T_cmemb_t;



#define H5O_FILL_VERSION      1
#define H5O_FILL_VERSION_2    2


typedef enum H5D_alloc_time_t {
    H5D_ALLOC_TIME_ERROR        =-1,
    H5D_ALLOC_TIME_DEFAULT      =0,
    H5D_ALLOC_TIME_EARLY        =1,
    H5D_ALLOC_TIME_LATE =2,
    H5D_ALLOC_TIME_INCR =3
} H5D_alloc_time_t;

typedef enum H5D_fill_time_t {
    H5D_FILL_TIME_ERROR =-1,
    H5D_FILL_TIME_ALLOC =0,
    H5D_FILL_TIME_NEVER =1,
    H5D_FILL_TIME_IFSET =2
} H5D_fill_time_t;


typedef struct H5O_fill_new_t {
    ssize_t             size;           /*number of bytes in the fill value  */
    void                *buf;           /*the fill value                     */
    H5D_alloc_time_t    alloc_time;     /* time to allocate space            */
    H5D_fill_time_t     fill_time;      /* time to write fill value          */
    hbool_t             fill_defined;   /* whether fill value is defined     */
} H5O_fill_new_t;


#define H5O_EFL_VERSION         1

#define H5O_EFL_ALLOC           16      /*number of slots to alloc at once   */
#define H5O_EFL_UNLIMITED       H5F_UNLIMITED /*max possible file size       */

typedef struct H5O_efl_entry_t {
    size_t      name_offset;            /*offset of name within heap         */
    char        *name;                  /*malloc'd name                      */
    off_t       offset;                 /*offset of data within file         */
    hsize_t     size;                   /*size allocated within file         */
} H5O_efl_entry_t;

typedef struct H5O_efl_t {
    haddr_t     heap_addr;              /*address of name heap               */
    size_t      nalloc;                 /*number of slots allocated          */
    size_t      nused;                  /*number of slots used               */
    H5O_efl_entry_t *slot;              /*array of external file entries     */
} H5O_efl_t;



#define H5O_LAYOUT_VERSION_1    1
#define H5O_LAYOUT_VERSION_2    2
#define H5O_LAYOUT_VERSION_3    3

#define H5O_LAYOUT_NDIMS        (H5S_MAX_RANK+1)


typedef enum H5D_layout_t {
    H5D_LAYOUT_ERROR    = -1,
    H5D_COMPACT         = 0,    /*raw data is very small                     */
    H5D_CONTIGUOUS      = 1,    /*the default                                */
    H5D_CHUNKED         = 2,    /*slow and fancy                             */
    H5D_NLAYOUTS        = 3     /*this one must be last!                     */
} H5D_layout_t;


typedef struct H5O_layout_contig_t {
    haddr_t     addr;                   /* File address of data              */
    hsize_t     size;                   /* Size of data in bytes             */
} H5O_layout_contig_t;


typedef struct H5O_layout_chunk_t {
    haddr_t     addr;                   /* File address of B-tree            */
    unsigned    ndims;                  /* Num dimensions in chunk           */
    size_t      dim[H5O_LAYOUT_NDIMS];  /* Size of chunk in elements         */
    size_t      size;                   /* Size of chunk in bytes            */
#if 0
    H5RC_t     *btree_shared;           /* Ref-counted info for B-tree nodes */
#endif
} H5O_layout_chunk_t;


typedef struct H5O_layout_compact_t {
    hbool_t     dirty;                  /* Dirty flag for compact dataset    */
    size_t      size;                   /* Size of buffer in bytes           */
    void        *buf;                   /* Buffer for compact dataset        */
} H5O_layout_compact_t;


typedef struct H5O_layout_t {
    H5D_layout_t type;                  /* Type of layout                    */
    unsigned version;                   /* Version of message                */
    /* Structure for "unused" dimension information */
    struct {
        unsigned ndims;                 /*num dimensions in stored data      */
        hsize_t dim[H5O_LAYOUT_NDIMS];  /*size of data or chunk in bytes     */
    } unused;
    union {
        H5O_layout_contig_t contig;     /* Information for contiguous layout */
        H5O_layout_chunk_t chunk;       /* Information for chunked layout    */
        H5O_layout_compact_t compact;   /* Information for compact layout    */
    } u;
} H5O_layout_t;



#define H5O_PLINE_VERSION       1

#define H5Z_MAX_NFILTERS        32      /* Maximum number of filters allowed in a pipeline (should probably be allowed to be an unlimited amount) */


typedef int H5Z_filter_t;

typedef struct {
    H5Z_filter_t        id;             /*filter identification number       */
    unsigned            flags;          /*defn and invocation flags          */
    char                *name;          /*optional filter name               */
    size_t              cd_nelmts;      /*number of elements in cd_values[]  */
    unsigned            *cd_values;     /*client data values                 */
} H5Z_filter_info_t;


typedef struct H5O_pline_t {
    size_t      nalloc;                 /*num elements in `filter' array     */
    size_t      nused;                  /*num filters defined                */
    H5Z_filter_info_t *filter;          /*array of filters                   */
} H5O_pline_t;



#define H5O_ATTR_VERSION        1
#define H5O_ATTR_VERSION_NEW    2
#define H5O_ATTR_FLAG_TYPE_SHARED       0x01

struct H5S_t {
    H5S_extent_t extent;                /* Dataspace extent */
    void 	 *select;               /* ??? DELETED for now: Dataspace selection */
};



typedef struct H5S_t H5S_t;

typedef struct H5A_t H5A_t;

struct H5A_t {
    char        *name;      /* Attribute's name */
    H5T_t       *dt;        /* Attribute's datatype */
    size_t      dt_size;    /* Size of datatype on disk */
    H5S_t       *ds;        /* Attribute's dataspace */
    size_t      ds_size;    /* Size of dataspace on disk */
    void        *data;      /* Attribute data (on a temporary basis) */
    size_t      data_size;  /* Size of data on disk */
};



typedef struct H5O_name_t {
    char        *s;                     /*ptr to malloc'd memory             */
} H5O_name_t;


typedef struct H5HG_t {
    haddr_t             addr;           /*address of collection         */
    size_t              idx;            /*object ID within collection   */
} H5HG_t;



#define H5O_FLAG_SHARED         0x02u


#define H5O_SHARED_VERSION_1    1
#define H5O_SHARED_VERSION      2

typedef struct H5O_shared_t {
    hbool_t             in_gh;          /*shared by global heap?             */
    union {
        H5HG_t          gh;             /*global heap info                   */
        H5G_entry_t     ent;            /*symbol table entry info            */
    } u;
} H5O_shared_t;



typedef struct H5O_cont_t {
    haddr_t     addr;                   /*address of continuation block      */
    size_t      size;                   /*size of continuation block         */

    /* the following field(s) do not appear on disk */
    unsigned    chunkno;                /*chunk this mesg refers to          */
} H5O_cont_t;


typedef struct H5O_stab_t {
    haddr_t     btree_addr;             /*address of B-tree                  */
    haddr_t     heap_addr;              /*address of name heap               */
} H5O_stab_t;

#define H5O_MTIME_VERSION       1


typedef struct H5O_class_t {
    int id;                             /*message ID */
    void        *(*decode)(const uint8_t*);
} H5O_class_t;


typedef struct H5O_mesg_t {
    const H5O_class_t   *type;          /*type of message                    */
    hbool_t             dirty;          /*raw out of date wrt native         */
    uint8_t             flags;          /*message flags                      */
    unsigned            chunkno;        /*chunk number for this mesg         */
    void                *native;        /*native format message              */
    uint8_t             *raw;           /*ptr to raw data                    */
    size_t              raw_size;       /*size with alignment                */
} H5O_mesg_t;


typedef struct H5O_chunk_t {
    hbool_t     dirty;                  /*dirty flag                         */
    haddr_t     addr;                   /*chunk file address                 */
    size_t      size;                   /*chunk size                         */
    uint8_t     *image;                 /*image of file                      */
} H5O_chunk_t;


typedef struct H5O_t {
    int         version;                /*version number                     */
    int         nlink;                  /*link count                         */
    unsigned 	nmesgs;                 /*number of messages                 */
    unsigned    alloc_nmesgs;           /*number of message slots            */
    H5O_mesg_t  *mesg;                  /*array of messages                  */
    unsigned    nchunks;                /*number of chunks                   */
    unsigned    alloc_nchunks;          /*chunks allocated                   */
    H5O_chunk_t *chunk;                 /*array of chunks                    */
} H5O_t;


#define H5O_VERSION     1
#define H5O_NMESGS     	32      /*initial number of messages         */
#define H5O_NCHUNKS    	8       /*initial number of chunks           */


#define H5B_SIZEOF_MAGIC 4         /* size of magic number               */
#define H5B_SIZEOF_HDR(F)                                                     \
   (H5B_SIZEOF_MAGIC +          /* magic number                            */  \
    4 +                         /* type, level, num entries                */  \
    2*H5F_SIZEOF_ADDR(F)) /* left and right sibling addresses    */

#define H5B_MAGIC  "TREE"          /* tree node magic number */


#define H5G_NODE_MAGIC  "SNOD"          /*symbol table node magic number     */
#define H5G_NODE_SIZEOF_MAGIC 4         /*sizeof symbol node magic number    */


#define H5G_NODE_VERS   1               /*symbol table node version number   */
#define H5G_NODE_SIZEOF_HDR(F) (H5G_NODE_SIZEOF_MAGIC + 4)

#define H5HL_MAGIC      "HEAP"          /* heap magic number */
#define H5HL_SIZEOF_MAGIC 4
#define H5HL_ALIGN(X)   (((X)+7)&(unsigned)(~0x07)) /*align on 8-byte boundary  */

#define H5HL_VERSION    0
#define H5HL_FREE_NULL  1         	/*end of free list on disk      */



#define H5HL_SIZEOF_HDR(F)                                                    \
    H5HL_ALIGN(H5HL_SIZEOF_MAGIC +      /*heap signature                */    \
               4 +                      /*reserved                      */    \
               H5F_SIZEOF_SIZE (F) +    /*data size                     */    \
               H5F_SIZEOF_SIZE (F) +    /*free list head                */    \
               H5F_SIZEOF_ADDR (F))     /*data address                  */

#define H5HG_MINSIZE     4096
#define H5HG_VERSION     1


#define H5HG_MAGIC      	"GCOL"
#define H5HG_SIZEOF_MAGIC 	4


#define H5HG_SIZEOF_HDR(F)                                                    \
    H5HG_ALIGN(4 +                      /*magic number          */            \
               1 +                      /*version number        */            \
               3 +                      /*reserved              */            \
               H5F_SIZEOF_SIZE(F))      /*collection size       */

#define H5HG_ALIGNMENT  8
#define H5HG_ALIGN(X)   (H5HG_ALIGNMENT*(((X)+H5HG_ALIGNMENT-1)/              \
                                         H5HG_ALIGNMENT))
#define H5HG_ISALIGNED(X) ((X)==H5HG_ALIGN(X))
#define H5HG_SIZEOF_OBJHDR(F)                                                 \
    H5HG_ALIGN(2 +                      /*object id number      */            \
               2 +                      /*reference count       */            \
               4 +                      /*reserved              */            \
               H5F_SIZEOF_SIZE(F))      /*object data size      */

#define H5HG_NOBJS(F,z) (int)((((z)-H5HG_SIZEOF_HDR(F))/                      \
                               H5HG_SIZEOF_OBJHDR(F)+2))


typedef struct H5HG_obj_t {
    int         	nrefs;          /*reference count               */
    size_t              size;           /*total size of object          */
    uint8_t             *begin;         /*ptr to object into heap->chunk*/
} H5HG_obj_t;

struct H5HG_heap_t {
    haddr_t             addr;           /*collection address            */
    size_t              size;           /*total size of collection      */
    uint8_t             *chunk;         /*the collection, incl. header  */
    size_t              nalloc;         /*numb object slots allocated   */
    size_t              nused;          /*number of slots used          */
                                        /* If this value is >65535 then all indices */
                                        /* have been used at some time and the */
                                        /* correct new index should be searched for */
    H5HG_obj_t  *obj;           	/*array of object descriptions  */
};

typedef struct H5HG_heap_t H5HG_heap_t;

#define H5_ASSIGN_OVERFLOW(var,expr,exprtype,vartype)   \
{                                                       \
    exprtype _tmp_overflow=(exprtype)(expr);              \
    vartype _tmp_overflow2=(vartype)(_tmp_overflow);  \
    assert((vartype)_tmp_overflow==_tmp_overflow2);    \
    (var)=_tmp_overflow2;                               \
}

typedef enum H5B_subid_t {
    H5B_SNODE_ID         = 0,   /*B-tree is for symbol table nodes           */
    H5B_ISTORE_ID        = 1,   /*B-tree is for indexed object storage       */
    H5B_NUM_BTREE_ID            /* Number of B-tree key IDs (must be last)   */
} H5B_subid_t;


typedef struct H5B_class_t {
    H5B_subid_t id;                                     /*id as found in file*/
    size_t      sizeof_nkey;                    /*size of native (memory) key*/
    size_t      (*get_sizeof_rkey)(H5F_shared_t, unsigned);    /*raw key size   */
    /* encode, decode, debug key values */
    void *	(*decode)(H5F_shared_t, unsigned, const uint8_t **);
} H5B_class_t;

typedef struct H5G_node_key_t {
    size_t      offset;                 /*offset into heap for name          */
} H5G_node_key_t;

typedef struct H5D_istore_key_t {
    size_t      nbytes;                         /*size of stored data   */
    hsize_t     offset[H5O_LAYOUT_NDIMS];       /*logical offset to start*/
    unsigned    filter_mask;                    /*excluded filters      */
} H5D_istore_key_t;

typedef struct obj_t {
    haddr_t 	objno;
    int	  	nlink;
} obj_t;


typedef struct table_t {
    size_t 	size;
    size_t 	nobjs; 
    obj_t	*objs;
} table_t;


/*
 *  Virtual file layer
 */

                    
struct H5FD_class_t { 
    const char *name;
    herr_t  (*sb_decode)(H5F_shared_t *_shared_info, const unsigned char *p);
    H5FD_t *(*open)(const char *name, int driver_id);
    herr_t  (*close)(H5FD_t *file);
    herr_t  (*read)(H5FD_t *file, haddr_t addr, size_t size, void *buffer); 
    haddr_t (*get_eof)(H5FD_t *file);

};



struct H5FD_t {
    int			driver_id;      /*driver ID for this file   */
    const H5FD_class_t *cls;            /*constant class info       */
};

typedef struct H5FD_sec2_t {
    H5FD_t      pub;                    /*public stuff, must be first   */
    int         fd;                     /*the unix file                 */
    haddr_t     eof;                    /*end of file; current file size*/
} H5FD_sec2_t;


typedef struct H5FD_family_t {
    H5FD_t      pub;            /*public stuff, must be first           */
    hsize_t     memb_size;      /*maximum size of each member file      */
    unsigned    nmembs;         /*number of family members              */
    unsigned    amembs;         /*number of member slots allocated      */
    H5FD_t      **memb;         /*dynamic array of member pointers      */
    haddr_t     eoa;            /*end of allocated addresses            */
    char        *name;          /*name generator printf format          */
} H5FD_family_t;


typedef enum H5FD_mem_t {
    H5FD_MEM_NOLIST     = -1,                   /*must be negative*/
    H5FD_MEM_DEFAULT    = 0,                    /*must be zero*/
    H5FD_MEM_SUPER      = 1,
    H5FD_MEM_BTREE      = 2,
    H5FD_MEM_DRAW       = 3,
    H5FD_MEM_GHEAP      = 4,
    H5FD_MEM_LHEAP      = 5,
    H5FD_MEM_OHDR       = 6,

    H5FD_MEM_NTYPES                             /*must be last*/
} H5FD_mem_t;


struct H5FD_multi_fapl_t {
    H5FD_mem_t  memb_map[H5FD_MEM_NTYPES]; /*memory usage map           */
    char        *memb_name[H5FD_MEM_NTYPES];/*name generators           */
    haddr_t     memb_addr[H5FD_MEM_NTYPES];/*starting addr per member   */
};


typedef struct H5FD_multi_t {
    H5FD_t      pub;            /*public stuff, must be first           */
    H5FD_multi_fapl_t fa;       /*driver-specific file access properties*/
    haddr_t     memb_next[H5FD_MEM_NTYPES];/*addr of next member        */
    H5FD_t      *memb[H5FD_MEM_NTYPES]; /*member pointers               */
    haddr_t     eoa;            /*end of allocated addresses            */
    char        *name;          /*name passed to H5Fopen or H5Fcreate   */
}H5FD_multi_t;

