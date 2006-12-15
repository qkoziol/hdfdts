#define H5E_NSLOTS      32      /* number of slots in an error stack */

/* Information about an error */
typedef struct H5E_error_t {
    const char  *func_name;             /* function in which error occurred   */
    const char  *desc;                  /* supplied description      */
    haddr_t	address;		/* address where error occurs */
} H5E_error_t;


/* An error stack */
typedef struct H5E_t {
    int nused;                  	/* num slots currently used in stack  */
    H5E_error_t slot[H5E_NSLOTS];       /* array of error records */
} H5E_t;

/* the current error stack */
H5E_t	H5E_stack_g[1];
#define	H5E_get_my_stack()	(H5E_stack_g+0)
void H5E_push(const char *, const char *, haddr_t );
herr_t H5E_clear(void);
void H5E_print(FILE *);
int H5E_FoundError(void);


