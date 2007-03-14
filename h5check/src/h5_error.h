typedef enum primary_err_t {
	ERR_NONE_PRIM	= 0,
        ERR_LEV_0,
        ERR_LEV_1,
        ERR_LEV_2,
        ERR_FILE,
	ERR_INTERNAL
} primary_err_t;

typedef enum secondary_err_t {
	ERR_NONE_SEC 	= 0,
        ERR_LEV_0A,
        ERR_LEV_0B,
        ERR_LEV_1A,
        ERR_LEV_1B,
        ERR_LEV_1C,
        ERR_LEV_1D,
        ERR_LEV_1E,
        ERR_LEV_1F,
        ERR_LEV_2A,
        ERR_LEV_2A1,	/* NIL */
        ERR_LEV_2A2,	/* Simple Dataspace */
        ERR_LEV_2A3,	/* Reserved - not assigned yet */
        ERR_LEV_2A4,	/* Datatype */
        ERR_LEV_2A5,	/* Data Storage - Fill Value (Old) */
        ERR_LEV_2A6,	/* Data Storage - Fill Value */
        ERR_LEV_2A7,	/* Reserved - not assigned yet */
        ERR_LEV_2A8,	/* Data Storage - External Data Files */
        ERR_LEV_2A9,	/* Data Storage - Layout */
        ERR_LEV_2A10,	/* Reserved - not assigned yet */
        ERR_LEV_2A11,	/* Reserved - not assigned yet */
        ERR_LEV_2A12,	/* Data Storage - Filter Pipeline */
        ERR_LEV_2A13,	/* Attribute */
        ERR_LEV_2A14,	/* Object Comment */
        ERR_LEV_2A15,	/* Object Modification Date & Time (Old) */
        ERR_LEV_2A16,	/* Shared Object Message */
        ERR_LEV_2A17,	/* Object Header Continuation */
        ERR_LEV_2A18,	/* Group Message */
        ERR_LEV_2A19,	/* Object Modification Date & Time */
        ERR_LEV_2B,	/* Shared Data Object Headers */
        ERR_LEV_2C	/* Data Object Data Storage */
} secondary_err_t;	

typedef struct   primary_err_mesg_t {
        primary_err_t   err_code;
        const   char    *str;
} primary_err_mesg_t;

typedef struct   secondary_err_mesg_t {
        secondary_err_t err_code;
        const   char    *str;
} secondary_err_mesg_t;

/* mainly used to report the version number decoded */
typedef struct	error_info_t {
	int	reported;	/* whether to report the "bad_info" */
	int	bad_info;
} error_info_t;

/* Information about an error */
typedef struct  error_t {
        primary_err_t   prim_err;	/* Primary Format Level where error is found */
        secondary_err_t sec_err;	/* Secondary Format Level where error is found */
        const char      *desc;		/* Detail description of error */
    	ck_addr_t		logical_addr;  	/* logical address where error occurs */
	error_info_t	err_info;	/* for reporting wrong/correct version info */
} error_t;

#define	REPORTED	1
#define	NOT_REP		0
#define H5E_NSLOTS      32      /* number of slots in an error stack */

/* An error stack */
typedef struct ERR_t {
    int nused;                  	/* num slots currently used in stack  */
    error_t slot[H5E_NSLOTS];       /* array of error records */
} ERR_t;

/* the current error stack */
ERR_t	ERR_stack_g[1];

#define	ERR_get_my_stack()	(ERR_stack_g+0)

void 	error_push(primary_err_t, secondary_err_t, const char *, ck_addr_t, int, int);
ck_err_t 	error_clear(void);
void 	error_print(FILE *, driver_t *);
int 	found_error(void);
