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
#include <unistd.h>
#include "h5_check.h"
#include "h5_error.h"

static 	int  	debug_verbose(void);
static	int	object_api(void);


/* for handling hard links */
static int	table_search(ck_addr_t);
static void	table_insert(ck_addr_t, int);

/* logical address for error reporting */
static ck_addr_t get_logical_addr(const uint8_t *, const uint8_t *, ck_addr_t);

/* Validation routines */
static ck_err_t check_btree(driver_t *, ck_addr_t, unsigned, int);
static ck_err_t check_sym(driver_t *, ck_addr_t, int);
static ck_err_t check_lheap(driver_t *, ck_addr_t, uint8_t **);
static ck_err_t check_gheap(driver_t *, ck_addr_t, uint8_t **);

static 	ck_addr_t locate_super_signature(driver_t *);
static  void      addr_decode(global_shared_t *, const uint8_t **, ck_addr_t *);
static  ck_err_t  gp_ent_decode(global_shared_t *, const uint8_t **, GP_entry_t *);
static  ck_err_t  gp_ent_decode_vec(global_shared_t *, const uint8_t **, GP_entry_t *, unsigned);
static	type_t    *type_alloc(ck_addr_t);

static  unsigned H5O_find_in_ohdr(driver_t *, H5O_t *, const obj_class_t **, int);
static	void 	 *H5O_shared_read(driver_t *, OBJ_shared_t *, const obj_class_t *, int);
static	ck_err_t  decode_validate_messages(driver_t *, H5O_t *, int);



/*
 *  Virtual file drivers
 */
static 	void 		set_driver(int *, char *);
static 	driver_class_t 	*get_driver(int);
static 	void 		*get_driver_info(int, global_shared_t *);
static 	ck_err_t 	decode_driver(global_shared_t *, const uint8_t *);

static 	ck_err_t 	FD_read(driver_t *, ck_addr_t, size_t, void */*out*/);

static 	driver_t 	*sec2_open(const char *, global_shared_t *, int);
static 	ck_err_t 	sec2_read(driver_t *, ck_addr_t, size_t, void */*out*/);
static 	ck_err_t 	sec2_close(driver_t *);
static 	ck_addr_t 	sec2_get_eof(driver_t *);
static 	char 		*sec2_get_fname(driver_t *, ck_addr_t);

static const driver_class_t sec2_g = {
    "sec2",                                /* name                  */
    NULL,                                  /* decode_driver         */
    sec2_open,                             /* open                  */
    sec2_close,                            /* close                 */
    sec2_read,                             /* read                  */
    sec2_get_eof,                          /* get_eof               */
    sec2_get_fname,			   /* get file name	    */
};


static 	ck_err_t 	multi_decode_driver(global_shared_t *, const unsigned char *);
static 	driver_t 	*multi_open(const char *, global_shared_t *, int);
static 	ck_err_t 	multi_close(driver_t *);
static 	ck_err_t 	multi_read(driver_t *, ck_addr_t, size_t, void */*out*/);
static 	ck_addr_t 	multi_get_eof(driver_t *file);
static 	char 		*multi_get_fname(driver_t *file, ck_addr_t logi_addr);


static void set_multi_driver_properties(driver_multi_fapl_t **, driver_mem_t *, const char **, ck_addr_t *);
static int compute_next(driver_multi_t *file);
static int open_members(driver_multi_t *file, global_shared_t *);

static const driver_class_t multi_g = {
    "multi",                               /* name                 */
    multi_decode_driver,                   /* decode_driver        */
    multi_open,                            /* open                  */
    multi_close,                           /* close                 */
    multi_read,                            /* read                  */
    multi_get_eof,                         /* get_eof               */
    multi_get_fname,			   /* get file name 	    */
};


/*
 * Header messages
 */

/* NIL: 0x0000 */
static const obj_class_t OBJ_NIL[1] = {{
    OBJ_NIL_ID,             /* message id number             */
    NULL,                   /* no decode method              */
    NULL		    /* no copy method yet 	     */
}};

static void *OBJ_sds_decode(driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Simple Database: 0x0001 */
static const obj_class_t OBJ_SDS[1] = {{
    OBJ_SDS_ID,             	/* message id number         */
    OBJ_sds_decode,         	/* decode message            */
    NULL		        /* no copy method yet 	     */
}};



static void *OBJ_dt_decode (driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);
static ck_err_t OBJ_dt_decode_helper(driver_t *, const uint8_t **, type_t *, const uint8_t *, ck_addr_t);
static void *OBJ_dt_copy(const void *, void *);

/* Datatype: 0x0003 */
static const obj_class_t OBJ_DT[1] = {{
    OBJ_DT_ID,              /* message id number          */
    OBJ_dt_decode,           /* decode message            */
    OBJ_dt_copy		    /* no copy method yet 	  */
}};


static void  *OBJ_fill_decode(driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Data Storage - Fill Value: 0x0005 */
static const obj_class_t OBJ_FILL[1] = {{
    OBJ_FILL_ID,            /* message id number               */
    OBJ_fill_decode,        /* decode message                  */
    NULL		    /* no copy method yet 	       */
}};

static void *OBJ_edf_decode(driver_t *, const uint8_t *p, const uint8_t *, ck_addr_t);

/* Data Storage - External Data Files: 0x0007 */
static const obj_class_t OBJ_EDF[1] = {{
    OBJ_EDF_ID,                 /* message id number             */
    OBJ_edf_decode,             /* decode message                */
    NULL		        /* no copy method yet 	         */
}};


static void *OBJ_layout_decode(driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Data Storage - layout: 0x0008 */
static const obj_class_t OBJ_LAYOUT[1] = {{
    OBJ_LAYOUT_ID,              /* message id number             */
    OBJ_layout_decode,	        /* decode message                */
    NULL		        /* no copy method yet 	         */
}};

static void *OBJ_filter_decode (driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Data Storage - Filter pipeline: 0x000B */
static const obj_class_t OBJ_FILTER[1] = {{
    OBJ_FILTER_ID,              /* message id number            */
    OBJ_filter_decode,          /* decode message               */
    NULL		        /* no copy method yet 	        */
}};

static void *OBJ_attr_decode (driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);
static void *OBJ_attr_copy (const void *src_mesg, void *dest_mesg);

/* Attribute: 0x000C */
static const obj_class_t OBJ_ATTR[1] = {{
    OBJ_ATTR_ID,                /* message id number            */
    OBJ_attr_decode,             /* decode message              */
    OBJ_attr_copy		/* copy the message 		*/
}};


static void *OBJ_comm_decode(driver_t *, const uint8_t *p, const uint8_t *, ck_addr_t);

/* Object Comment: 0x000D */
static const obj_class_t OBJ_COMM[1] = {{
    OBJ_COMM_ID,                /* message id number             */
    OBJ_comm_decode,            /* decode message                */
    NULL		        /* no copy method yet 	         */
}};



/* Old version, with full symbol table entry as link for object header sharing */
#define H5O_SHARED_VERSION_1    1

/* New version, with just address of object as link for object header sharing */
#define H5O_SHARED_VERSION      2

static void *OBJ_shared_decode (driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Shared Object Message: 0x000F */
static const obj_class_t OBJ_SHARED[1] = {{
    OBJ_SHARED_ID,              /* message id number             */
    OBJ_shared_decode,          /* decode method                 */
    NULL		        /* no copy method yet 	         */
}};


static void *OBJ_cont_decode(driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Object Header Continuation: 0x0010 */
static const obj_class_t OBJ_CONT[1] = {{
    OBJ_CONT_ID,                /* message id number             */
    OBJ_cont_decode,             /* decode message               */
    NULL		        /* no copy method yet 	         */
}};


static void *OBJ_group_decode(driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Group Message: 0x0011 */
static const obj_class_t OBJ_GROUP[1] = {{
    OBJ_GROUP_ID,              /* message id number             */
    OBJ_group_decode,          /* decode message                */
    NULL		       /* no copy method yet 	        */
}};


static void *OBJ_mdt_decode(driver_t *, const uint8_t *, const uint8_t *, ck_addr_t);

/* Object Modification Date & Time: 0x0012 */
static const obj_class_t OBJ_MDT[1] = {{
    OBJ_MDT_ID,           	/* message id number             */
    OBJ_mdt_decode,       	/* decode message                */
    NULL		        /* no copy method yet 	         */
}};


static const obj_class_t *const message_type_g[] = {
    OBJ_NIL,           	/* 0x0000 NIL                                   */
    OBJ_SDS,        	/* 0x0001 Simple Dataspace			*/
    NULL,          	/* 0x0002 Reserved - Not Assigned Yet		*/
    OBJ_DT,          	/* 0x0003 Datatype                              */
    NULL,           	/* 0x0004 Data Storage - Fill Value (Old) 	*/
    OBJ_FILL,      	/* 0x0005 Data storage - Fill Value         	*/
    NULL,               /* 0x0006 Reserved - Not Assigned Yet		*/
    OBJ_EDF,           	/* 0x0007 Data Storage - External Data Files    */
    OBJ_LAYOUT,        	/* 0x0008 Data Storage - Layout                 */
    NULL,               /* 0x0009 Reserved - Not Assigned Yet		*/
    NULL,               /* 0x000A Reserved - Not assigned Yet           */
    OBJ_FILTER,       	/* 0x000B Data storage - Filter Pipeline        */
    OBJ_ATTR,          	/* 0x000C Attribute 				*/
    OBJ_COMM,       	/* 0x000D Object Comment                        */
    NULL,         	/* 0x000E Object Modification Date & Time (Old) */
    OBJ_SHARED,        	/* 0x000F Shared Object Message			*/
    OBJ_CONT,          	/* 0x0010 Object Header Continuation            */
    OBJ_GROUP,          /* 0x0011 Group Message				*/
    OBJ_MDT		/* 0x0012 Object modification Date & Time  	*/
};

/* end header messages */

static size_t gp_node_sizeof_rkey(global_shared_t *, unsigned);
static void *gp_node_decode_key(global_shared_t *, unsigned, const uint8_t **);

BT_class_t BT_SNODE[1] = {
    BT_SNODE_ID,              /* id                    */
    sizeof(GP_node_key_t),    /* sizeof_nkey           */
    gp_node_sizeof_rkey,      /* get_sizeof_rkey       */
    gp_node_decode_key,       /* decode                */
};

static size_t raw_node_sizeof_rkey(global_shared_t *, unsigned);
static void *raw_node_decode_key(global_shared_t *, unsigned, const uint8_t **);

static BT_class_t BT_ISTORE[1] = {
    BT_ISTORE_ID,             /* id                    */
    sizeof(RAW_node_key_t),   /* sizeof_nkey           */
    raw_node_sizeof_rkey,     /* get_sizeof_rkey       */
    raw_node_decode_key,      /* decode                */
};

static const BT_class_t *const node_key_g[] = {
    	BT_SNODE,  	/* group node: symbol table */
	BT_ISTORE	/* raw data chunk node */
};


static ck_addr_t
get_logical_addr(const uint8_t *p, const uint8_t *start, ck_addr_t base)
{
	ck_addr_t	diff;

	diff = p - start;
	return(base + diff);
}

/*
 *  For handling hard links
 */

int
table_init(table_t **obj_table)
{
	int 	i;
	table_t	*tb;

	tb = malloc(sizeof(table_t));
	if (tb == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, "Could not malloc() table_t", -1, NULL);
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

static int
table_search(ck_addr_t obj_id)
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

static void
table_insert(ck_addr_t objno, int nlink)
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

/*
 *  Virtual drivers
 */
static void
set_driver(int *driverid, char *driver_name) 
{
	if (strcmp(driver_name, "NCSAmult") == 0)  {
		*driverid = MULTI_DRIVER;
	} else {
		/* default driver */
		*driverid = SEC2_DRIVER;
	}
}

static driver_class_t *
get_driver(int driver_id)
{
	driver_class_t	*driver = NULL;
	size_t		size;

	size = sizeof(driver_class_t);
	driver = malloc(size);
	
	switch (driver_id) {
	case SEC2_DRIVER:
		memcpy(driver, &sec2_g, size);
		break;
	case MULTI_DRIVER:
		memcpy(driver, &multi_g, size);
		break;
	default:
		printf("Something is wrong\n");
		break;
	}  /* end switch */

	return(driver);
}


static void *
get_driver_info(int driver_id, global_shared_t *shared)
{
	driver_multi_fapl_t 	*driverinfo;
	size_t			size;

	switch (driver_id) {
	case SEC2_DRIVER:
		driverinfo = NULL;
		break;
	case MULTI_DRIVER:
		size = sizeof(driver_multi_fapl_t);
		driverinfo = malloc(size);
		memcpy(driverinfo, shared->fa, size);
		break;
	default:
		printf("Something is wrong\n");
		break;
	}  /* end switch */
	return(driverinfo);
}


static ck_err_t
decode_driver(global_shared_t *shared, const uint8_t *buf)
{
	if (shared->driverid == MULTI_DRIVER)
		multi_decode_driver(shared, buf);
}

driver_t *
FD_open(const char *name, global_shared_t *shared, int driver_id)
{
	driver_t	*file= NULL;
	driver_class_t 	*driver;

	file = calloc(1, sizeof(driver_t));
	driver = get_driver(driver_id);
	file = driver->open(name, shared, driver_id);
	if (file != NULL) {
		file->cls = driver;
		file->driver_id = driver_id;
		file->shared = shared;
	}
	return(file);
}


ck_err_t
FD_close(driver_t *file)
{
	int	ret = SUCCEED;

	if(file->cls->close(file) == FAIL)
		ret = FAIL;
	return(ret);
}

static ck_err_t
FD_read(driver_t *file, ck_addr_t addr, size_t size, void *buf/*out*/)
{
	int		ret = SUCCEED;
	ck_addr_t	new_addr;


	/* adjust logical "addr" to be the physical address */
	new_addr = addr + file->shared->super_addr;
	if (file->cls->read(file, new_addr, size, buf) == FAIL)
		ret = FAIL;

	return(ret);
}

ck_addr_t
FD_get_eof(driver_t *file)
{
    	ck_addr_t     ret_value;

    	assert(file && file->cls);

    	if (file->cls->get_eof)
		ret_value = file->cls->get_eof(file);
	return(ret_value);
}

char *
FD_get_fname(driver_t *file, ck_addr_t logi_addr)
{
	ck_addr_t	new_logi_addr;

	/* adjust logical "addr" to be the physical address */
	new_logi_addr = logi_addr + file->shared->super_addr;
	return(file->cls->get_fname(file, new_logi_addr));
}


/*
 *  sec2 file driver
 */
static driver_t *
sec2_open(const char *name, global_shared_t *UNUSED, int driver_id)
{
	driver_sec2_t	*file = NULL;
	driver_t	*ret_value;
	int		fd = -1;
	struct stat	sb;

	/* do unix open here */
	fd = open(name, O_RDONLY);
	if (fd < 0) {
		error_push(ERR_FILE, ERR_NONE_SEC, "Unable to open the file", -1, NULL);
		goto done;
	}
	if (fstat(fd, &sb)<0) {
		error_push(ERR_FILE, ERR_NONE_SEC, "Unable to fstat file", -1, NULL);
		goto done;
	}
	file = calloc(1, sizeof(driver_sec2_t));
	file->fd = fd;
	file->eof = sb.st_size;
	file->name = strdup(name);
done:
	ret_value = (driver_t *)file;
	return(ret_value);
}

static ck_addr_t
sec2_get_eof(driver_t *file)
{
    driver_sec2_t *sec2_file = (driver_sec2_t*)file;
 
    return(sec2_file->eof);

}

static char *
sec2_get_fname(driver_t *file, ck_addr_t UNUSED)
{
	
	return(((driver_sec2_t *)file)->name);
}

static ck_err_t
sec2_close(driver_t *file)
{
	int	ret;

	ret = close(((driver_sec2_t *)file)->fd);
	free(file);
	return(ret);
}


static ck_err_t
sec2_read(driver_t *file, ck_addr_t addr, size_t size, void *buf/*out*/)
{
	int	status;
	int	ret = SUCCEED;
	int	fd;

#ifdef TEMP
	/* adjust logical "addr" to be the physical address */
	addr = addr + shared_info.super_addr;
#endif

	/* NEED To find out about lseek64??? */
	fd = ((driver_sec2_t *)file)->fd;
	if (lseek(fd, addr, SEEK_SET) < 0)
		ret = FAIL;
	/* end of file: return 0; errors: return -1 */
       	else if (read(fd, buf, size) <= 0)
		ret = FAIL;
	return(ret);
}


/*
 *  multi file driver
 */

#define UNIQUE_MEMBERS(MAP,LOOPVAR) {                                         \
    driver_mem_t _unmapped, LOOPVAR;                                            \
    ck_bool_t _seen[FD_MEM_NTYPES];                                           \
                                                                              \
    memset(_seen, 0, sizeof _seen);                                           \
    for (_unmapped=FD_MEM_SUPER; _unmapped<FD_MEM_NTYPES; _unmapped=(driver_mem_t)(_unmapped+1)) {  \
        LOOPVAR = MAP[_unmapped];                                             \
        if (FD_MEM_DEFAULT==LOOPVAR) LOOPVAR=_unmapped;                     \
        assert(LOOPVAR>0 && LOOPVAR<FD_MEM_NTYPES);                         \
        if (_seen[LOOPVAR]++) continue;


#define ALL_MEMBERS(LOOPVAR) {                                                \
    driver_mem_t LOOPVAR;                                                       \
    for (LOOPVAR=FD_MEM_DEFAULT; LOOPVAR<FD_MEM_NTYPES; LOOPVAR=(driver_mem_t)(LOOPVAR+1)) {


#define END_MEMBERS     }}


static void
set_multi_driver_properties(driver_multi_fapl_t **fa, driver_mem_t map[], const char *memb_name[], ck_addr_t memb_addr[])
{
	*fa = malloc(sizeof(driver_multi_fapl_t));
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

static ck_err_t
multi_decode_driver(global_shared_t *shared, const unsigned char *buf)
{
    	char                x[2*FD_MEM_NTYPES*8];
    	driver_mem_t        map[FD_MEM_NTYPES];
    	int                 i;
    	size_t              nseen=0;
    	const char          *memb_name[FD_MEM_NTYPES];
    	ck_addr_t           memb_addr[FD_MEM_NTYPES];
    	ck_addr_t           memb_eoa[FD_MEM_NTYPES];
	ck_addr_t	    xx;
/* handle memb_eoa[]?? */
/* handle mt_in use??? */


    	/* Set default values */
    	ALL_MEMBERS(mt) {
        	memb_addr[mt] = CK_ADDR_UNDEF;
        	memb_eoa[mt] = CK_ADDR_UNDEF;
        	memb_name[mt] = NULL;
    	} END_MEMBERS;

    	/*
     	 * Read the map and count the unique members.
      	 */
    	memset(map, 0, sizeof map);
    	for (i=0; i<6; i++) {
        	map[i+1] = (driver_mem_t)buf[i];
    	}
    	UNIQUE_MEMBERS(map, mt) {
        	nseen++;
    	} END_MEMBERS;
    	buf += 8;

    	/* Decode Address and EOA values */
    	assert(sizeof(ck_addr_t)<=8);
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

	set_multi_driver_properties((driver_multi_fapl_t **)&(shared->fa), map, memb_name, memb_addr);
    	return 0;
}

static driver_t *
multi_open(const char *name, global_shared_t *shared, int driver_id)
{
    	driver_multi_t        	*file=NULL;
    	driver_multi_fapl_t   	*fa;
    	driver_mem_t          	m;

	driver_t		*ret_value;
	int			len, ret;


	ret = SUCCEED;

    	/* Check arguments */
    	if (!name || !*name) {
		error_push(ERR_FILE, ERR_NONE_SEC, "Invalid file name", -1, NULL);
		goto error;
	}

    	/*
     	 * Initialize the file from the file access properties, using default
       	 * values if necessary.
      	 */
    	if (NULL==(file=calloc(1, sizeof(driver_multi_t)))) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, "Unable to calloc() driver_multi_t", -1, NULL);
		goto error;
	}

	fa = get_driver_info(driver_id, shared);
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
    	if (open_members(file, shared) != SUCCEED) {
		error_push(ERR_FILE, ERR_NONE_SEC, "Unable to open member files", -1, NULL);
		goto error;
	}

/* NEED To checkon this also, make shared_info to be _shared_info for pointer */
/* need to check on return values for driver routines */
/* error check at the end of each driver routine, also, free malloc space */
#if 0
    if (FD_MEM_DEFAULT==(m=file->fa.memb_map[FD_MEM_SUPER]))
        m = FD_MEM_SUPER;
    if (NULL==file->memb[m])
        goto error;
#endif

	ret_value = (driver_t *)file;
	return(ret_value);

error:
    	if (file) {
       	 	ALL_MEMBERS(mt) {
            		if (file->memb[mt]) (void)FD_close(file->memb[mt]);
            		if (file->fa.memb_name[mt]) free(file->fa.memb_name[mt]);
        	} END_MEMBERS;
        	if (file->name) free(file->name);
        	free(file);
    	}
    	return NULL;
}

static ck_err_t 
multi_close(driver_t *file)
{
 	driver_multi_t   	*multi_file = (driver_multi_t*)file;
	int		errs = 0;


    	/* Close as many members as possible */
    	ALL_MEMBERS(mt) {
        	if (multi_file->memb[mt]) {
            		if (FD_close(multi_file->memb[mt])<0)
                		errs++;
            	else
                	multi_file->memb[mt] = NULL;
		}
	} END_MEMBERS;

    	if (errs) {
		error_push(ERR_FILE, ERR_NONE_SEC, "Error closing member file(s)", -1, NULL);
		return(FAIL);
	}

    	/* Clean up other stuff */
    	ALL_MEMBERS(mt) {
        if (multi_file->fa.memb_name[mt]) 
		free(multi_file->fa.memb_name[mt]);
    	} END_MEMBERS;
    	free(multi_file->name);
    	free(multi_file);
    	return 0;
}

static int
compute_next(driver_multi_t *file)
{

    ALL_MEMBERS(mt) {
        file->memb_next[mt] = CK_ADDR_UNDEF;
    } END_MEMBERS;

    UNIQUE_MEMBERS(file->fa.memb_map, mt1) {
        UNIQUE_MEMBERS(file->fa.memb_map, mt2) {
            if (file->fa.memb_addr[mt1]<file->fa.memb_addr[mt2] &&
                (CK_ADDR_UNDEF==file->memb_next[mt1] ||
                 file->memb_next[mt1]>file->fa.memb_addr[mt2])) {
                file->memb_next[mt1] = file->fa.memb_addr[mt2];
            }
        } END_MEMBERS;
        if (CK_ADDR_UNDEF==file->memb_next[mt1]) {
            file->memb_next[mt1] = CK_ADDR_MAX; /*last member*/
        }
    } END_MEMBERS;

    return 0;
}


static int
open_members(driver_multi_t *file, global_shared_t *shared)
{
    	char    tmp[1024], newname[1024];
	char	*ptr;
	int	ret;


	/* fix the name */
	strcpy(newname, file->name);
	ptr = strrchr(newname, '-');
	*ptr = '\0';

	ret = SUCCEED;

    	UNIQUE_MEMBERS(file->fa.memb_map, mt) {
        	assert(file->fa.memb_name[mt]);
        	sprintf(tmp, file->fa.memb_name[mt], newname);
            	file->memb[mt] = FD_open(tmp, shared, SEC2_DRIVER);
		if (file->memb[mt] == NULL)
			ret = FAIL;
    	} END_MEMBERS;

    	return(ret);
}


static ck_err_t
multi_read(driver_t *file, ck_addr_t addr, size_t size, void *buf/*out*/)
{
    	driver_multi_t        *ff = (driver_multi_t*)file;
    	driver_mem_t          mt, mmt, hi=FD_MEM_DEFAULT;
    	ck_addr_t             start_addr=0;


#ifdef TEMP
	/* adjust logical "addr" to be the physical address */
	addr = addr + shared_info.super_addr;
#endif

    	/* Find the file to which this address belongs */
    	for (mt=FD_MEM_SUPER; mt<FD_MEM_NTYPES; mt=(driver_mem_t)(mt+1)) {
        	mmt = ff->fa.memb_map[mt];
        	if (FD_MEM_DEFAULT==mmt) 
			mmt = mt;
        	assert(mmt>0 && mmt<FD_MEM_NTYPES);

        	if (ff->fa.memb_addr[mmt]>addr) 
			continue;
        	if (ff->fa.memb_addr[mmt]>=start_addr) {
            		start_addr = ff->fa.memb_addr[mmt];
            		hi = mmt;
        	}
    	}
    	assert(hi>0);

    	/* Read from that member */
    	return FD_read(ff->memb[hi], addr-start_addr, size, buf);
}


static ck_addr_t
multi_get_eof(driver_t *file)
{
    	driver_multi_t    *multi_file = (driver_multi_t *)file;
    	ck_addr_t         eof=0, tmp;

    	UNIQUE_MEMBERS(multi_file->fa.memb_map, mt) {
        	if (multi_file->memb[mt]) {
                	tmp = FD_get_eof(multi_file->memb[mt]);
            		if (tmp == CK_ADDR_UNDEF) {
				error_push(ERR_FILE, ERR_NONE_SEC, 
				  "Member file has unknown eof", -1, NULL);
				return(CK_ADDR_UNDEF);
			}
            		if (tmp > 0) 
				tmp += multi_file->fa.memb_addr[mt];
        	} else {
			error_push(ERR_FILE, ERR_NONE_SEC, "Bad eof", -1, NULL);
			return(CK_ADDR_UNDEF);
        	}

        	if (tmp > eof) 
			eof = tmp;
    	} END_MEMBERS;

    return(eof);
}

static char *
multi_get_fname(driver_t *file, ck_addr_t logi_addr)
{
	char 	*tmp, *newname;
	char	*ptr;
    	driver_multi_t        *ff = (driver_multi_t*)file;
    	driver_mem_t          mt, mmt, hi=FD_MEM_DEFAULT;
    	ck_addr_t             start_addr=0;
	ck_addr_t	addr;

#ifdef TEMP
	/* adjust logical "addr" to be the physical address */
	addr = logi_addr + shared_info.super_addr;
#endif

    	/* Find the file to which this address belongs */
    	for (mt=FD_MEM_SUPER; mt<FD_MEM_NTYPES; mt=(driver_mem_t)(mt+1)) {
        	mmt = ff->fa.memb_map[mt];
        	if (FD_MEM_DEFAULT==mmt) 
			mmt = mt;
        	assert(mmt>0 && mmt<FD_MEM_NTYPES);

        	if (ff->fa.memb_addr[mmt]>addr) 
			continue;
        	if (ff->fa.memb_addr[mmt]>=start_addr)
            		hi = mmt;
    	}
    	assert(hi>0);

	tmp = strdup(ff->name);
	ptr = strrchr(tmp, '-');
	*ptr = '\0';
	newname = malloc(strlen(tmp)+10);
       	sprintf(newname, ff->fa.memb_name[hi], tmp);
    	return(newname);
}
/*
 * End virtual file drivers
 */


/* 
 * Decode messages
 */

/* Simple Dataspace */
static void *
OBJ_sds_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	SDS_extent_t  	*sdim = NULL;	/* New extent dimensionality structure */
    	void          	*ret_value;
    	unsigned        i;              /* local counting variable */
    	unsigned        flags, version;
	int		ret;
	ck_addr_t	logical;
	int		badinfo;


    	/* check args */
    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	/* decode */
    	sdim = calloc(1, sizeof(SDS_extent_t));
	if (sdim == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() SDS_extent_t for Simple Dataspace Message", 
			logical, NULL);
		ret++;
		goto done;
	}
        /* Check version */
	logical = get_logical_addr(p, start_buf, logi_base);
        version = *p++;

        if (version != SDS_VERSION) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A2, 
		  "Version number should be 1 for Simple Dataspace Message", 
		  logical, &badinfo);
		ret++;
	}

	/* SHOULD I CHECK FOR IT: not specified in the specification, but in coding only?? */
        /* Get rank */
	logical = get_logical_addr(p, start_buf, logi_base);
        sdim->rank = *p++;
        if (sdim->rank > SDS_MAX_RANK) {
		badinfo = sdim->rank;
		error_push(ERR_LEV_2, ERR_LEV_2A2, 
		  "Dimensionality is too large for Simple Dataspace Message", 
		  logical, &badinfo);
		ret++;
	}

        /* Get dataspace flags for later */
	logical = get_logical_addr(p, start_buf, logi_base);
        flags = *p++;
	if (flags > 0x3) {  /* Only bit 0 and bit 1 can be set */
		error_push(ERR_LEV_2, ERR_LEV_2A2, 
		  "Corrupt flags for Simple Dataspace Message", logical, NULL);
		ret++;
	}

	 /* Set the dataspace type to be simple or scalar as appropriate */
        if(sdim->rank>0)
            	sdim->type = SDS_SIMPLE;
        else
            	sdim->type = SDS_SCALAR;


        p += 5; /*reserved*/

	logical = get_logical_addr(p, start_buf, logi_base);
	if (sdim->rank > 0) {
            	if ((sdim->size=malloc(sizeof(ck_size_t)*sdim->rank))==NULL) {
			error_push(ERR_INTERNAL, ERR_NONE_SEC, 
			  "Could not malloc() ck_size_t for Simple Dataspace Message", 
			  logical, NULL);
			ret++;
			goto done;
		}
            	for (i = 0; i < sdim->rank; i++)
                	DECODE_LENGTH(file->shared, p, sdim->size[i]);
		logical = get_logical_addr(p, start_buf, logi_base);
            	if (flags & SDS_VALID_MAX) {
                	if ((sdim->max=malloc(sizeof(ck_size_t)*sdim->rank))==NULL) {
				error_push(ERR_INTERNAL, ERR_NONE_SEC, 
				  "Could not malloc() ck_size_t for Simple Dataspace Message", 
				  logical, NULL);
				ret++;
				goto done;
			}
                	for (i = 0; i < sdim->rank; i++)
                    		DECODE_LENGTH(file->shared, p, sdim->max[i]);
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


/* Datatype */
static ck_err_t
OBJ_dt_decode_helper(driver_t *file, const uint8_t **pp, type_t *dt, const uint8_t *start_buf, ck_addr_t logi_base)
{
 	unsigned        flags, version, tmp;
    	unsigned        i, j;
    	size_t          z;
    	ck_err_t      	ret=SUCCEED;       /* Return value */
	ck_addr_t	logical;
	int		badinfo;


    	/* check args */
    	assert(pp && *pp);
    	assert(dt && dt->shared);

    	/* decode */
	logical = get_logical_addr(*pp, start_buf, logi_base);
    	UINT32DECODE(*pp, flags);
    	version = (flags>>4) & 0x0f;
    	if (version != DT_VERSION_COMPAT && version != DT_VERSION_UPDATED) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Version number should be 1 or 2 for Datatype Message", logical, &badinfo);
		ret++;
	}

    	dt->shared->type = (DT_class_t)(flags & 0x0f);
	if ((dt->shared->type < DT_INTEGER) ||(dt->shared->type >DT_ARRAY)) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Invalid class value for Datatype Mesage.", logical, NULL);
		ret++;
		return(ret);
	}
    	flags >>= 8;
    	UINT32DECODE(*pp, dt->shared->size);
/* Do i need to check for size < 0??? */


    	switch (dt->shared->type) {
          case DT_INTEGER:  /* Fixed-Point datatypes */
            dt->shared->u.atomic.order = (flags & 0x1) ? DT_ORDER_BE : DT_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? DT_PAD_ONE : DT_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? DT_PAD_ONE : DT_PAD_ZERO;
            dt->shared->u.atomic.u.i.sign = (flags & 0x8) ? DT_SGN_2 : DT_SGN_NONE;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
	    if ((flags>>4) != 0) {  /* should be reserved(zero) */
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Fixed-Point:Bits 4-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
            break;

	 case DT_FLOAT:  /* Floating-Point datatypes */
            dt->shared->u.atomic.order = (flags & 0x1) ? DT_ORDER_BE : DT_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? DT_PAD_ONE : DT_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? DT_PAD_ONE : DT_PAD_ZERO;
            dt->shared->u.atomic.u.f.pad = (flags & 0x8) ? DT_PAD_ONE : DT_PAD_ZERO;
            switch ((flags >> 4) & 0x03) {
                case 0:
                    dt->shared->u.atomic.u.f.norm = DT_NORM_NONE;
                    break;
                case 1:
                    dt->shared->u.atomic.u.f.norm = DT_NORM_MSBSET;
                    break;
                case 2:
                    dt->shared->u.atomic.u.f.norm = DT_NORM_IMPLIED;
                    break;
                default:
		    error_push(ERR_LEV_2, ERR_LEV_2A4, 
		      "Unknown Floating-Point normalization for Datatype Message", logical, NULL);
		    ret++;
		    break;
            }
            dt->shared->u.atomic.u.f.sign = (flags >> 8) & 0xff;

	    if (((flags >> 6) & 0x03) != 0) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Floating-Point:Bits 6-7 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
	    if ((flags >> 16) != 0) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Floating-Point:Bits 16-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
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

	case DT_TIME:  /* Time datatypes */
            dt->shared->u.atomic.order = (flags & 0x1) ? DT_ORDER_BE : DT_ORDER_LE;

	    if ((flags>>1) != 0) {  /* should be reserved(zero) */
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Time:Bits 1-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }

            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

	  case DT_STRING:  /* character string datatypes */
            dt->shared->u.atomic.order = DT_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;
            dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = DT_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = DT_PAD_ZERO;

            dt->shared->u.atomic.u.s.pad = (DT_str_t)(flags & 0x0f);
            dt->shared->u.atomic.u.s.cset = (DT_cset_t)((flags>>4) & 0x0f);            

	    /* Padding Type supported: Null Terminate, Null Pad and Space Pad */
	    if ((dt->shared->u.atomic.u.s.pad != DT_STR_NULLTERM) &&
	        (dt->shared->u.atomic.u.s.pad != DT_STR_NULLPAD) &&
	        (dt->shared->u.atomic.u.s.pad != DT_STR_SPACEPAD)) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "String:Unsupported padding type for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
	    /* The only character set supported is the 8-bit ASCII */
	    if (dt->shared->u.atomic.u.s.cset != DT_CSET_ASCII) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "String:Unsupported character set for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
	    if ((flags>>8) != 0) {  /* should be reserved(zero) */
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "String:Bits 8-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
	    break;
	
        case DT_BITFIELD:  /* Bit fields datatypes */
            dt->shared->u.atomic.order = (flags & 0x1) ? DT_ORDER_BE : DT_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? DT_PAD_ONE : DT_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? DT_PAD_ONE : DT_PAD_ZERO;
	    if ((flags>>3) != 0) {  /* should be reserved(zero) */
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Bitfield:Bits 3-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case DT_OPAQUE:  /* Opaque datatypes */
            z = flags & (DT_OPAQUE_TAG_MAX - 1);
            assert(0==(z&0x7)); /*must be aligned*/
	    if ((flags>>8) != 0) {  /* should be reserved(zero) */
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Opaque:Bits 8-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }

            if ((dt->shared->u.opaque.tag=malloc(z+1))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not malloc() Opaque ASCII Tag for Datatype Message", logical, NULL);
		ret++;
		return(ret);
	    }

	    /*??? should it be nul-terminated???? see spec. */
            memcpy(dt->shared->u.opaque.tag, *pp, z);
            dt->shared->u.opaque.tag[z] = '\0';
            *pp += z;
            break;

	 case DT_COMPOUND:  /* Compound datatypes */
            dt->shared->u.compnd.nmembs = flags & 0xffff;
            assert(dt->shared->u.compnd.nmembs > 0);
	    if ((flags>>16) != 0) {  /* should be reserved(zero) */
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Compound:Bits 16-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
            dt->shared->u.compnd.packed = TRUE; /* Start off packed */
            dt->shared->u.compnd.nalloc = dt->shared->u.compnd.nmembs;
            dt->shared->u.compnd.memb = calloc(dt->shared->u.compnd.nalloc,
                            sizeof(DT_cmemb_t));
            if ((dt->shared->u.compnd.memb==NULL)) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() DT_cmemb_t for Compound type for Datatype Message", 
		  logical, NULL);
		ret++;
		return(ret);
	    }
            for (i = 0; i < dt->shared->u.compnd.nmembs; i++) {
                unsigned ndims=0;     /* Number of dimensions of the array field */
		ck_size_t dim[OBJ_LAYOUT_NDIMS];  /* Dimensions of the array */
                unsigned perm_word=0;    /* Dimension permutation information */
                type_t *temp_type;   /* Temporary pointer to the field's datatype */
		size_t	tmp_len;

                /* Decode the field name */
		if (!((const char *)*pp)) {
			error_push(ERR_LEV_2, ERR_LEV_2A4, 
			  "Invalid string pointer for Compound type for Datatype Message", 
			  logical, NULL);
			ret++;
			return(ret);
		}
		tmp_len = strlen((const char *)*pp) + 1;
                if ((dt->shared->u.compnd.memb[i].name=malloc(tmp_len))==NULL) {
			error_push(ERR_INTERNAL, ERR_NONE_SEC, 
			  "Could not malloc() string for Compound type for Datatype Message", 
			  logical, NULL);
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
	        if(version==DT_VERSION_COMPAT) {
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

		if ((temp_type = type_alloc(logical)) == NULL) {
              		error_push(ERR_LEV_2, ERR_LEV_2A4, 
			  "Compound:Failure in allocating structures via type_alloc() for Datatype Message", 
			  logical, NULL);
			ret++;
               		return(ret);
		}

                /* Decode the field's datatype information */
                if (OBJ_dt_decode_helper(file, pp, temp_type, start_buf, logi_base) != SUCCEED) {
                    for (j = 0; j <= i; j++)
                        free(temp_type->shared->u.compnd.memb[j].name);
                    free(temp_type->shared->u.compnd.memb);
                    error_push(ERR_LEV_2, ERR_LEV_2A4, 
		      "Unable to decode Compound member type for Datatype Message", logical, NULL);
		    ret++;
                    return(ret);
                }

                /* Member size */
                dt->shared->u.compnd.memb[i].size = temp_type->shared->size;

                dt->shared->u.compnd.memb[i].type = temp_type;

            }
            break;

        case DT_REFERENCE: /* Reference datatypes...  */
            dt->shared->u.atomic.order = DT_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;            
	    dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = DT_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = DT_PAD_ZERO;

            /* Set reference type */
            dt->shared->u.atomic.u.r.rtype = (DTR_type_t)(flags & 0x0f);
	    /* Value of 2 is not implemented yet (see spec.) */
	    /* value of 3-15 is supposedly to be reserved */
	    if ((flags&0x0f) >= 2) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Reference:Invalid class bit field for Datatype Message", logical, NULL);
		ret++;
	    }

	    if ((flags>>4) != 0) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Reference:Bits 4-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }
            break;

	case DT_ENUM:  /* Enumeration datatypes */
            dt->shared->u.enumer.nmembs = dt->shared->u.enumer.nalloc = flags & 0xffff;
	    if ((flags>>16) != 0) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Enumeration:Bits 16-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }

	    if ((dt->shared->parent=type_alloc(logical)) == NULL) {
              	error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Enumeration:Failure in allocating structures via type_alloc() for Datatype Message", 
		  logical, NULL);
		ret++;
               	return(ret);
	    }

            if (OBJ_dt_decode_helper(file, pp, dt->shared->parent, start_buf, logi_base) != SUCCEED) {
                    error_push(ERR_LEV_2, ERR_LEV_2A4, 
		      "Unable to decode Enumeration parent type for Datatype Message", logical, NULL);
		    ret++;
                    return(ret);
	    }

	    if ((dt->shared->u.enumer.name=calloc(dt->shared->u.enumer.nalloc, sizeof(char*)))==NULL) {
              	error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() Enumeration name pointer for Datatype Message", logical, NULL);
		ret++;
               	return(ret);
	    }
            if ((dt->shared->u.enumer.value=calloc(dt->shared->u.enumer.nalloc,
                    dt->shared->parent->shared->size))==NULL) {
              		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  	  "Could not calloc() space for Enumeration values for Datatype Message", 
			  logical, NULL);
		ret++;
               	return(ret);
	    }


            /* Names, each a multiple of 8 with null termination */
            for (i=0; i<dt->shared->u.enumer.nmembs; i++) {
		size_t	tmp_len;

                /* Decode the field name */
		if (!((const char *)*pp)) {
			error_push(ERR_LEV_2, ERR_LEV_2A4, 
			  "Invalid Enumeration string pointer for Datatype Message", logical, NULL);
			ret++;
			return(ret);
		}
		tmp_len = strlen((const char *)*pp) + 1;
                if ((dt->shared->u.enumer.name[i]=malloc(tmp_len))==NULL) {
			error_push(ERR_INTERNAL, ERR_NONE_SEC, 
			  "Could not malloc() Enumeration string for Datatype Message", logical, NULL);
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

        case DT_VLEN:  /* Variable length datatypes */
            /* Set the type of VL information, either sequence or string */
            dt->shared->u.vlen.type = (DT_vlen_type_t)(flags & 0x0f);
            if(dt->shared->u.vlen.type == DT_VLEN_STRING) {
                dt->shared->u.vlen.pad  = (DT_str_t)((flags>>4) & 0x0f);
                dt->shared->u.vlen.cset = (DT_cset_t)((flags>>8) & 0x0f);
            } /* end if */

	    /* Only sequence and string are defined */
	    if (dt->shared->u.vlen.type > DT_VLEN_STRING) {
		
	    }
	    /* Padding type and character set should be zero for vlen sequence */
	    if (dt->shared->u.vlen.type == DT_VLEN_SEQUENCE) {
		if (((flags>>4)&0x0f) != 0) {
		}
		if (((flags>>8)&0x0f)!= 0) {
		}
	    }
	    if ((flags>>12) != 0) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Variable-Length:Bits 12-23 should be 0 for class bit field for Datatype Message", 
		  logical, NULL);
		ret++;
	    }

            /* Decode base type of VL information */
            if((dt->shared->parent = type_alloc(logical))==NULL) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Variable-Length:Failure in allocating structures via type_alloc() for Datatype Message", 
		  logical, NULL);
		ret++;
		return(ret);
	    }

            if (OBJ_dt_decode_helper(file, pp, dt->shared->parent, start_buf, logi_base) != SUCCEED) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Unable to decode Variable-Length parent type for Datatype Message", logical, NULL);
		ret++;
		return(ret);
	    }

            break;


        case DT_ARRAY:  /* Array datatypes */
            /* Decode the number of dimensions */
            dt->shared->u.array.ndims = *(*pp)++;

            /* Double-check the number of dimensions */
            assert(dt->shared->u.array.ndims <= SDS_MAX_RANK);

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
            if((dt->shared->parent = type_alloc(logical))==NULL) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Array:Failure in allocating structures via type_alloc() for Datatype Message", 
		  logical, NULL);
		ret++;
		return(ret);
	    }
            if (OBJ_dt_decode_helper(file, pp, dt->shared->parent, start_buf, logi_base) != SUCCEED) {
		error_push(ERR_LEV_2, ERR_LEV_2A4, 
		  "Unable to decode Array parent type for Datatype Message", logical, NULL);
		ret++;
		return(ret);
	    }

            break;

	   default:
	     /* shouldn't come to here */
	     printf("OBJ_dt_decode_helper: datatype class not handled yet...type=%d\n", dt->shared->type);
    	}

	return(ret);
}


/* Datatype */
static void *
OBJ_dt_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	type_t		*dt = NULL;
    	void    	*ret_value;     /* Return value */
	int		ret;
	ck_addr_t		logical;


    	/* check args */
    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
	if ((dt = calloc(1, sizeof(type_t))) == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() type_t for Datatype Message", logical, NULL);
		return(ret_value=NULL);
	}
	/* NOT SURE what to use with the group entry yet */
	memset(&(dt->ent), 0, sizeof(GP_entry_t));
	dt->ent.header = CK_ADDR_UNDEF;
	if ((dt->shared = calloc(1, sizeof(DT_shared_t))) == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() H5T_shared_t for Datatype Message", logical, NULL);
		return(ret_value=NULL);
	}

    	if (OBJ_dt_decode_helper(file, &p, dt, start_buf, logi_base) != SUCCEED) {
		ret++;
		/* should have errors already pushed onto the error stack */
#if 0
		error_push("OBJ_dt_decode", ERR_LEV_2, ERR_LEV_2A4, 
		  "Cannot decode datatype message", -1, -1);
#endif
	}

	if (ret)	
		dt = NULL;
    	/* Set return value */
    	ret_value = dt;
	return(ret_value);
}

static void *
OBJ_dt_copy(const void *_src_attr, void *_dest_attr)
{
	printf("I am in dt_copy\n");
}

/* Data Storage - Fill Value */
static void *
OBJ_fill_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	obj_fill_t      *mesg=NULL;
    	int          	version, ret;
    	void         	*ret_value;
	ck_addr_t	logical;
	int		badinfo;


    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	mesg = calloc(1, sizeof(obj_fill_t));
	if (mesg == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() obj_fill_t for Data Storage-Fill Value Message(new)", 
		  logical, NULL);
		ret++;
		goto done;
	}

    	/* Version */
	logical = get_logical_addr(p, start_buf, logi_base);
    	version = *p++;
    	if ((version != OBJ_FILL_VERSION) && (version != OBJ_FILL_VERSION_2)) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A6, 
		  "Version number should be 1 or 2 for Data Storage-Fill Value Message(new)", 
		  logical, &badinfo);
		ret++;
	}


    	/* Space allocation time */
	logical = get_logical_addr(p, start_buf, logi_base);
    	mesg->alloc_time = (fill_alloc_time_t)*p++;
	if ((mesg->alloc_time != FILL_ALLOC_TIME_EARLY) &&
	    (mesg->alloc_time != FILL_ALLOC_TIME_LATE) &&
	    (mesg->alloc_time != FILL_ALLOC_TIME_INCR)) {
		error_push(ERR_LEV_2, ERR_LEV_2A6, 
		  "Bad Space Allocation Time for Data Storage-Fill Value Message(new)", 
		  logical, NULL);
		ret++;
	}

    	/* Fill value write time */
	logical = get_logical_addr(p, start_buf, logi_base);
    	mesg->fill_time = (fill_time_t)*p++;
	if ((mesg->fill_time != FILL_TIME_ALLOC) &&
	    (mesg->fill_time != FILL_TIME_NEVER) &&
	    (mesg->fill_time != FILL_TIME_IFSET)) {
		error_push(ERR_LEV_2, ERR_LEV_2A6, 
		  "Bad Fill Value Write Time for Data Storage-Fill Value Message(new)", 
		  logical, NULL);
		ret++;
	}
	

    	/* Whether fill value is defined */
	logical = get_logical_addr(p, start_buf, logi_base);
    	mesg->fill_defined = *p++;
	if ((mesg->fill_defined != 0) && (mesg->fill_defined != 1)) {
		error_push(ERR_LEV_2, ERR_LEV_2A6, 
		  "Bad Fill Value Defined for Data Storage-Fill Value Message(new)", 
		  logical, NULL);
		ret++;
	}


    	/* Only decode fill value information if one is defined */
    	if(mesg->fill_defined) {
		logical = get_logical_addr(p, start_buf, logi_base);
        	INT32DECODE(p, mesg->size);
        	if (mesg->size > 0) {
            		CHECK_OVERFLOW(mesg->size,ssize_t,size_t);
            		if (NULL==(mesg->buf=malloc((size_t)mesg->size))) {
				error_push(ERR_INTERNAL, ERR_NONE_SEC, 
				  "Could not malloc() Fill Value buffer for Data Storage-Fill Value Message(new)", 
				  logical, NULL);
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

/* Data Storage - External Data Files */
static void *
OBJ_edf_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	obj_edf_t       *mesg = NULL;
    	int             version, ret;
    	const char      *s = NULL;
    	size_t          u;      	/* Local index variable */
    	void 		*ret_value;     /* Return value */
	ck_addr_t	logical;
	int		badinfo;

    	/* Check args */
    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	if ((mesg = calloc(1, sizeof(obj_edf_t)))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() obj_edf_t for Data Storage-External Data Files Message", 
		  logical, NULL);
		ret++;
		goto done;
	}

    	/* Version */
	logical = get_logical_addr(p, start_buf, logi_base);
    	version = *p++;
    	if (version != OBJ_EDF_VERSION) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A8, 
		  "Version number should be 1 for Data Storage-External Data Files Message", 
		  logical, &badinfo);
		ret++;
	}
    	/* Reserved */
    	p += 3;

    	/* Number of slots */
	logical = get_logical_addr(p, start_buf, logi_base);
    	UINT16DECODE(p, mesg->nalloc);
    	assert(mesg->nalloc>0);
    	UINT16DECODE(p, mesg->nused);
    	assert(mesg->nused <= mesg->nalloc);

	if (!(mesg->nalloc >= mesg->nused)) {
		error_push(ERR_LEV_2, ERR_LEV_2A8, 
		  "Inconsistent number of Allocated Slots for Data Storage-External Data Files Message", 
		  logical, NULL);
		ret++;
		goto done;
	}

    	/* Heap address:will be validated later in decode_validate_messages() */
    	addr_decode(file->shared, &p, &(mesg->heap_addr));
    	assert(addr_defined(mesg->heap_addr));

    	/* Decode the file list */
	logical = get_logical_addr(p, start_buf, logi_base);
    	mesg->slot = calloc(mesg->nalloc, sizeof(obj_edf_entry_t));
    	if (NULL == mesg->slot) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() H5O_efl_entry_t for Data Storage-External Data Files Message", 
		  logical, NULL);
		ret++;
		goto done;
	}

    	for (u = 0; u < mesg->nused; u++) {
        	/* Name offset: will be validated later in decode_validate_messages() */
        	DECODE_LENGTH(file->shared, p, mesg->slot[u].name_offset);
        	/* File offset */
        	DECODE_LENGTH(file->shared, p, mesg->slot[u].offset);
		if (!(mesg->slot[u].offset >=0)) {
			error_push(ERR_LEV_2, ERR_LEV_2A8, 
		  		"Invalid file offset for Data Storage-External Data Files Message", 
		  		logical, NULL);
			ret++;
		}
        	/* size */
        	DECODE_LENGTH(file->shared, p, mesg->slot[u].size);
		if (mesg->slot[u].size <= 0) {
			error_push(ERR_LEV_2, ERR_LEV_2A8, 
		  		"Invalid size for Data Storage-External Data Files Message", 
		  		logical, NULL);
			ret++;
		}
		if (!(mesg->slot[u].offset <= mesg->slot[u].size)) {
			error_push(ERR_LEV_2, ERR_LEV_2A8, 
		  		"Inconsistent file offset/size for Data Storage-External Data Files Message", 
		  		logical, NULL);
			ret++;
		}
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


/* Data Storage - Layout */
static void *
OBJ_layout_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	OBJ_layout_t    *mesg = NULL;
    	unsigned        u;
    	void            *ret_value;          /* Return value */
	int		ret;
	ck_addr_t	logical;
	int		badinfo;


    	/* check args */
    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	/* decode */
	if ((mesg=calloc(1, sizeof(OBJ_layout_t)))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() OBJ_layout_t for Data Storage-Layout Message", 
		  logical, NULL);
		return(ret_value=NULL);
	}

    	/* Version. 1 when space allocated; 2 when space allocation is delayed */
	logical = get_logical_addr(p, start_buf, logi_base);
    	mesg->version = *p++;
    	if (mesg->version<OBJ_LAYOUT_VERSION_1 || mesg->version>OBJ_LAYOUT_VERSION_3) {
		error_push(ERR_LEV_2, ERR_LEV_2A9, 
		  "Version number should be either 1, 2 or 3 for Data Storage-Layout Message.", 
		  logical, NULL);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

    	if(mesg->version < OBJ_LAYOUT_VERSION_3) {  /* version 1 & 2 */
        	unsigned        ndims;    /* Num dimensions in chunk           */

        	/* Dimensionality */
		logical = get_logical_addr(p, start_buf, logi_base);
        	ndims = *p++;
		/* this is not specified in the format specification ??? */
        	if (ndims > OBJ_LAYOUT_NDIMS) {
			badinfo = ndims;
			error_push(ERR_LEV_2, ERR_LEV_2A9, 
			  "Dimensionality is too large for Data Storage-Layout Message", logical, &badinfo);
			ret++;
		}

        	/* Layout class */
        	mesg->type = (DATA_layout_t)*p++;
        	assert(DATA_CONTIGUOUS == mesg->type || DATA_CHUNKED == mesg->type || DATA_COMPACT == mesg->type);

        	/* Reserved bytes */
        	p += 5;

        	/* Address */
        	if(mesg->type==DATA_CONTIGUOUS) {
            		addr_decode(file->shared, &p, &(mesg->u.contig.addr));
        	} else if(mesg->type==DATA_CHUNKED) {
            		addr_decode(file->shared, &p, &(mesg->u.chunk.addr));
		}

        	/* Read the size */
        	if(mesg->type!=DATA_CHUNKED) {
            		mesg->unused.ndims=ndims;

            		for (u = 0; u < ndims; u++)
                		UINT32DECODE(p, mesg->unused.dim[u]);
        	} else {
            		mesg->u.chunk.ndims=ndims;
            		for (u = 0; u < ndims; u++) {
                		UINT32DECODE(p, mesg->u.chunk.dim[u]);
			}

            		/* Compute chunk size */
            		for (u=1, mesg->u.chunk.size=mesg->u.chunk.dim[0]; u<ndims; u++)
                		mesg->u.chunk.size *= mesg->u.chunk.dim[u];
        	}

        	if(mesg->type == DATA_COMPACT) {
			logical = get_logical_addr(p, start_buf, logi_base);
            		UINT32DECODE(p, mesg->u.compact.size);
            		if(mesg->u.compact.size > 0) {
                		if(NULL==(mesg->u.compact.buf=malloc(mesg->u.compact.size))) {
					error_push(ERR_INTERNAL, ERR_NONE_SEC, 
					  "Could not malloc() Compact Data buffer for Data Storage-Layout Message", logical, NULL);
					return(ret_value=NULL);
				}

                		memcpy(mesg->u.compact.buf, p, mesg->u.compact.size);
                		p += mesg->u.compact.size;
            		}
        	}
    	} else {  /* version 3 */
        	/* Layout class */
		logical = get_logical_addr(p, start_buf, logi_base);
        	mesg->type = (DATA_layout_t)*p++;

        	/* Interpret the rest of the message according to the layout class */
        	switch(mesg->type) {
            	  case DATA_CONTIGUOUS:
                  	addr_decode(file->shared, &p, &(mesg->u.contig.addr));
			DECODE_LENGTH(file->shared, p, mesg->u.contig.size);
                	break;

               	  case DATA_CHUNKED:
                   	/* Dimensionality */
			logical = get_logical_addr(p, start_buf, logi_base);
                    	mesg->u.chunk.ndims = *p++;
                	if (mesg->u.chunk.ndims>OBJ_LAYOUT_NDIMS) {
				badinfo = mesg->u.chunk.ndims;
				error_push(ERR_LEV_2, ERR_LEV_2A9, 
				  "Chunked layout:Dimensionality is too large for Data Storage-Layout Message", 
				  logical, &badinfo);
				ret++;
			}

                	/* B-tree address */
                	addr_decode(file->shared, &p, &(mesg->u.chunk.addr));

                	/* Chunk dimensions */
                	for (u = 0; u < mesg->u.chunk.ndims; u++)
                    		UINT32DECODE(p, mesg->u.chunk.dim[u]);

                	/* Compute chunk size */
                	for (u=1, mesg->u.chunk.size=mesg->u.chunk.dim[0]; u<mesg->u.chunk.ndims; u++)
                    		mesg->u.chunk.size *= mesg->u.chunk.dim[u];
                	break;

            	  case DATA_COMPACT:
			logical = get_logical_addr(p, start_buf, logi_base);
                	UINT16DECODE(p, mesg->u.compact.size);
                	if(mesg->u.compact.size > 0) {
                    		if(NULL==(mesg->u.compact.buf=malloc(mesg->u.compact.size))) {
					error_push(ERR_LEV_2, ERR_NONE_SEC, 
					  "Compact layout:Could not malloc() buffer for Data Storage-Layout Message", 
					  logical, NULL);
					return(ret_value=NULL);
				}
                    		memcpy(mesg->u.compact.buf, p, mesg->u.compact.size);
                    		p += mesg->u.compact.size;
                	} /* end if */
                	break;

            	  default:
			error_push(ERR_LEV_2, ERR_LEV_2A9, 
			  "Invalid Layout Class for Data Storage-Layout Message", logical, NULL);
			ret++;
        	} /* end switch */
    	} /* version 3 */

	if (ret)
		mesg = NULL;
    	/* Set return value */
    	ret_value=mesg;
    	return(ret_value);
}


/* Data Storage - Filter Pipeline */
static void *
OBJ_filter_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	OBJ_filter_t     *pline = NULL;
    	void            *ret_value;
    	unsigned        version;
    	size_t          i, j, n, name_length;
	int		ret;
	ck_addr_t	logical;
	int		badinfo;

    	/* check args */
    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);

    	/* Decode */
    	if ((pline = calloc(1, sizeof(OBJ_filter_t)))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() OBJ_filter_t for Data Storage-Filter Pipeline Message", 
		  logical, NULL);
		ret++;
		goto done;
	}

	logical = get_logical_addr(p, start_buf, logi_base);
    	version = *p++;
    	if (version != OBJ_FILTER_VERSION) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A12, 
		  "Version number should be 1 for Data Storage-Filter Pipeline Message", logical, &badinfo);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

	logical = get_logical_addr(p, start_buf, logi_base);
    	pline->nused = *p++;
    	if (pline->nused > OBJ_MAX_NFILTERS) {
		badinfo = pline->nused;
		error_push(ERR_LEV_2, ERR_LEV_2A12, 
		  "Too many filters for Data Storage-Filter Pipeline Message", logical, &badinfo);
		ret++;  /* ?????SHOULD I LET IT CONTINUE */
	}

    	p += 6;     /*reserved*/
    	pline->nalloc = pline->nused;
    	pline->filter = calloc(pline->nalloc, sizeof(pline->filter[0]));
    	if (pline->filter==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() H5O_pline_t->filter for Data Storage-Filter Pipeline Message", 
		  logical, NULL);
		ret++;
		goto done;
	}

    	for (i = 0; i < pline->nused; i++) {
        	UINT16DECODE(p, pline->filter[i].id);
		logical = get_logical_addr(p, start_buf, logi_base);
        	UINT16DECODE(p, name_length);
        	if (name_length % 8) {
			error_push(ERR_LEV_2, ERR_LEV_2A12, 
			  "Filter name length is not a multiple of eight for Data Storage-Filter Pipeline Message", 
			  logical, NULL);
			ret++;
			goto done;
		}

        	UINT16DECODE(p, pline->filter[i].flags);
        	UINT16DECODE(p, pline->filter[i].cd_nelmts);
        	if (name_length) {
            		pline->filter[i].name = malloc(name_length+1);
            		memcpy(pline->filter[i].name, p, name_length);
            		pline->filter[i].name[name_length] = '\0';
            		p += name_length;
        	}
		logical = get_logical_addr(p, start_buf, logi_base);
        	if ((n=pline->filter[i].cd_nelmts)) {
            		pline->filter[i].cd_values = malloc(n*sizeof(unsigned));
            		if (pline->filter[i].cd_values==NULL) {
				error_push(ERR_INTERNAL, ERR_NONE_SEC, 
				  "Could not malloc() Client Data buffer for Data Storage-Filter Pipeline Message", 
				  logical, NULL);
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



/* Attribute */

static void *
OBJ_attr_copy(const void *_src_attr, void *_dest_attr)
{
	OBJ_attr_t	*src_attr, *dest_attr, *ret_value;
	
	src_attr = (OBJ_attr_t *)_src_attr;
	dest_attr = (OBJ_attr_t *)_dest_attr;

	printf("In copy method \n");
	if (dest_attr == NULL)
		dest_attr = malloc(sizeof(OBJ_attr_t));
	*dest_attr = *src_attr;
	dest_attr->name = strdup(dest_attr->name);
/*
        dest_attr->dt = H5T_copy(dest_attr->dt);
        dest_attr->ds = H5S_copy(dest_attr->ds);
*/

	if(dest_attr->data) {
            	dest_attr->data = malloc(src_attr->data_size);
        	memcpy(dest_attr->data, src_attr->data, src_attr->data_size);
	}

	ret_value = dest_attr;
	return(ret_value);
}

static void *
OBJ_attr_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	OBJ_attr_t      *attr = NULL;
    	SDS_extent_t    *extent;        /* extent dimensionality information  */
    	size_t          name_len;       /* attribute name length */
    	int             version;        /* message version number*/
    	unsigned        flags=0;	/* Attribute flags */
    	OBJ_attr_t     	*ret_value;     /* Return value */
	int		ret;
	ck_addr_t	logical;
	int		badinfo;


    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	if ((attr=calloc(1, sizeof(OBJ_attr_t))) == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() OBJ_attr_t for Attribute Message", logical, NULL);
		ret++;
		goto done;
	}

	logical = get_logical_addr(p, start_buf, logi_base);
    	/* Version number */
    	version = *p++;
	/* The format specification only describes version 1 of attribute messages */
	if (version != OBJ_ATTR_VERSION && version != OBJ_ATTR_VERSION_NEW) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A13, 
		  "Version number should be 1 or 2 for Attribute Message", logical, &badinfo);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, logi_base);
    	/* version 1 does not support flags; it is reserved and set to zero */
	/* version 2 used this flag to indicate data type is shared or not shared */
        flags = *p++;
	if ((version==OBJ_ATTR_VERSION) && (flags!=0)) {
		error_push(ERR_LEV_2, ERR_LEV_2A13, 
		  "Flag is unused for version 1 of Attribute Message", logical, NULL);
		ret++; 
	} else if ((version==OBJ_ATTR_VERSION_NEW) && (flags>OBJ_ATTR_FLAG_TYPE_SHARED)) {
		error_push(ERR_LEV_2, ERR_LEV_2A13, 
		  "Flag for version 1 of Attribute Message should be 0 or 1", logical, NULL);
		ret++; 
	}

    	/*
     	 * Decode the sizes of the parts of the attribute.  The sizes stored in
         * the file are exact but the parts are aligned on 8-byte boundaries.
     	 */
	logical = get_logical_addr(p, start_buf, logi_base);
    	UINT16DECODE(p, name_len); /*including null*/
    	UINT16DECODE(p, attr->dt_size);
    	UINT16DECODE(p, attr->ds_size);
/* SHOULD I CHCK FOR THOSE 2 fields above to be greater than 0 or >=??? */

 	/* Decode and store the name */
	attr->name = NULL;
	if (p) {
		if ((attr->name = strdup((const char *)p)) == NULL) {
			error_push(ERR_INTERNAL, ERR_NONE_SEC, 
			  "Could not allocate space for name for Attribute Message", 
			  logical, NULL);
			ret++;
			goto done;
		}
	}
#if 0
	if (name_len != 0) {
		if ((attr->name = malloc(name_len)) == NULL) {
			error_push(ERR_INTERNAL, ERR_NONE_SEC, 
			  "Could not malloc() space for name for Attribute Message", 
			  logical, NULL);
			ret++;
			goto done;
		}
		logical = get_logical_addr(p, start_buf, logi_base);
		strcpy(attr->name, (const char *)p);
		/* should be null-terminated  but should it name_len or name_len -1???*/
		if (attr->name[name_len-1] != '\0') {
			error_push(ERR_LEV_2, ERR_LEV_2A13, 
			  "Name for Attribute message should be null-terminated for Attribute Message", 
			  logical, NULL);
			ret++;
		}
	}
#endif
	if(version < OBJ_ATTR_VERSION_NEW)
        	p += CK_ALIGN(name_len);    /* advance the memory pointer */
    	else
        	p += name_len;    /* advance the memory pointer */


	logical = get_logical_addr(p, start_buf, logi_base);
	
	/* decode the attribute datatype */
    	if (flags & OBJ_ATTR_FLAG_TYPE_SHARED) {
        	OBJ_shared_t *sh_shared; 
	
printf("I am in OBJ_ATTR_FLAG_TYPE_SHARED\n");
        	/* Get the shared information */
        	if (NULL == (sh_shared = (OBJ_SHARED->decode) (file, p, start_buf, logi_base))) {
            		printf("unable to decode shared message");
		}
        		/* Get the actual datatype information */
printf("Going to H5O_shared_read\n");
/* NEED TO TAKE OF prev_entries */
		if ((attr->dt = H5O_shared_read(file, sh_shared, OBJ_DT, -1))==NULL)
			printf("uanble to decode attribute datatype\n");

    	} /* end if */
    	else {
        	if((attr->dt=(OBJ_DT->decode)(file, p, start_buf, logi_base))==NULL) {
		error_push(ERR_LEV_2, ERR_LEV_2A13, 
		  "Errors found when decoding datatype description for Attribute Message", 
		  logical, NULL);
		ret++;
		goto done;
		}
	}
	
	if (version < OBJ_ATTR_VERSION_NEW)
        	p += CK_ALIGN(attr->dt_size);
	else
		p += attr->dt_size;


    	/* decode the attribute dataspace */
	/* ??? there is select info in the structure */
	logical = get_logical_addr(p, start_buf, logi_base);
    	if ((attr->ds = calloc(1, sizeof(OBJ_space_t)))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() OBJ_space_t for Attribute Message", logical, NULL);
		ret++;
		goto done;
	}

    	if((extent=(OBJ_SDS->decode)(file, p, start_buf, logi_base))==NULL) {
		error_push(ERR_LEV_2, ERR_LEV_2A13, 
		  "Errors found when decoding dataspace description for Attribute Message", 
		  logical, NULL);
		ret++;
		goto done;
	}

    	/* Copy the extent information */
    	memcpy(&(attr->ds->extent),extent, sizeof(SDS_extent_t));

 	/* Release temporary extent information */ 
	if (extent)
    		free(extent);

    	/* Compute the size of the data */
	attr->data_size = attr->ds->extent.nelem * attr->dt->shared->size;
    	H5_ASSIGN_OVERFLOW(attr->data_size,attr->ds->extent.nelem*attr->dt->shared->size,ck_size_t,size_t);

	if (version < OBJ_ATTR_VERSION_NEW)
        	p += CK_ALIGN(attr->ds_size);
	else
		p += attr->ds_size;

    	/* Go get the data */
	logical = get_logical_addr(p, start_buf, logi_base);
    	if(attr->data_size) {
		if ((attr->data = malloc(attr->data_size))==NULL) {
			error_push(ERR_INTERNAL, ERR_NONE_SEC, 
			  "Could not malloc() space for Data for Attribute Message", logical, NULL);
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


/* Object Comment */
static void *
OBJ_comm_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
	OBJ_comm_t       *mesg;
    	void             *ret_value;     /* Return value */
	int		 len;
	ck_addr_t		 logical;


    	/* check args */
    	assert(p);

	logical = get_logical_addr(p, start_buf, logi_base);
	len = strlen((const char *)p);
    	/* decode */
    	if ((mesg=calloc(1, sizeof(OBJ_comm_t))) == NULL ||
            (mesg->s = malloc(len+1))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() OBJ_comm_t for Object Comment Message", logical, NULL);
		ret_value = NULL;
		goto done;
	}
    	strcpy(mesg->s, (const char*)p);
	if (mesg->s[len] != '\0') {
		error_push(ERR_LEV_2, ERR_LEV_2A14, 
		  "The comment string should be null-terminated for Object Comment Message", 
		  logical, NULL);
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



/* Shared Object Message */
static void *
OBJ_shared_decode(driver_t *file, const uint8_t *buf, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	OBJ_shared_t    *mesg=NULL;
    	unsigned        flags, version;
    	void            *ret_value;     /* Return value */
	int		ret=SUCCEED;
	ck_addr_t	logical;
	int		badinfo;


    	/* Check args */
    	assert (buf);

	logical = get_logical_addr(buf, start_buf, logi_base);
    	/* Decode */
    	if ((mesg = calloc(1, sizeof(OBJ_shared_t)))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() OBJ_shared_t for Shared Object Message", -1, NULL);
		ret++;
		goto done;
 	}

    	/* Version */
    	version = *buf++;
	logical = get_logical_addr(buf, start_buf, logi_base);

	/* ????NOT IN SPECF FOR DIFFERENT VERSIONS */
	/* SHOULD be only version 1 is described in the spec */
    	if (version != OBJ_SHARED_VERSION_1 && version != OBJ_SHARED_VERSION) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A16, 
		  "Version number should be 1 or?2 for Shared Object Message", logical, &badinfo);
		ret++;
		goto done;
	}

    	/* Get the shared information flags */
	logical = get_logical_addr(buf, start_buf, logi_base);
    	flags = *buf++;
    	mesg->in_gh = (flags & 0x01);

if (mesg->in_gh)
	printf("IN GLOBAL HEAP\n");
else
	printf("NOT IN GLOBAL HEAP\n");

	if ((flags>>1) != 0) {  /* should be reserved(zero) */
		error_push(ERR_LEV_2, ERR_LEV_2A16, 
		  "Bits 1-7 in Flags should be 0 for Shared Object Message", logical, NULL);
		ret++;
	}

    	/* Skip reserved bytes (for version 1) */
    	if(version==OBJ_SHARED_VERSION_1)
        	buf += 6;

	logical = get_logical_addr(buf, start_buf, logi_base);
    	/* Body */
    	if (mesg->in_gh) {
		/* NEED TO HANDLE GLOBAL HEAP HERE  and also validation */
        	addr_decode (file->shared, &buf, &(mesg->u.gh.addr));
		/* need to valiate idx not exceeding size here?? */
        	INT32DECODE(buf, mesg->u.gh.idx);
    	}
    	else {
        	if(version==OBJ_SHARED_VERSION_1)
			/* ??supposedly validate in that routine but not, need to check */
            		gp_ent_decode(file->shared, &buf, &(mesg->u.ent));
        	else {
            		assert(version==OBJ_SHARED_VERSION);
            		addr_decode(file->shared, &buf, &(mesg->u.ent.header));
			if ((mesg->u.ent.header == CK_ADDR_UNDEF) || (mesg->u.ent.header >= file->shared->stored_eoa)) {
				error_push(ERR_LEV_2, ERR_LEV_2A16, 
				  "Invalid object header address for Shared Object Message", 
				  logical, NULL);
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



/* Object Header Continutaion */
static void *
OBJ_cont_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	OBJ_cont_t 	*cont = NULL;
    	void       	*ret_value;
	int		ret;
	ck_addr_t	logical;

    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	/* decode */
	cont = malloc(sizeof(OBJ_cont_t));
	if (cont == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not malloc() OBJ_cont_t for Object Header Continuation Message", 
		  logical, NULL);
		ret++;
		goto done;
	}
	
	logical = get_logical_addr(p, start_buf, logi_base);
    	addr_decode(file->shared, &p, &(cont->addr));

	if ((cont->addr == CK_ADDR_UNDEF) || (cont->addr >= file->shared->stored_eoa)) {
		error_push(ERR_LEV_2, ERR_LEV_2A17, 
		  "Invalid offset for Object Header Continuation Message", logical, NULL);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, logi_base);
    	DECODE_LENGTH(file->shared, p, cont->size);

	if (cont->size < 0) {
		error_push( ERR_LEV_2, ERR_LEV_2A17, 
		  "Invalid length for Object Header Continuation Message", logical, NULL);
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


/* Group Message */
static void *
OBJ_group_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	H5O_stab_t  	*stab=NULL;
    	void    	*ret_value;     /* Return value */
	int		ret;
	ck_addr_t		logical;

    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	/* decode */
	stab = malloc(sizeof(H5O_stab_t));
	if (stab == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not malloc() H5O_stab_t for Group Message", logical, NULL);
		ret++;
		goto done;
	}

	addr_decode(file->shared, &p, &(stab->btree_addr));
	if ((stab->btree_addr == CK_ADDR_UNDEF) || (stab->btree_addr >= file->shared->stored_eoa)) {
		error_push(ERR_LEV_2, ERR_LEV_2A18, 
		  "Invalid btree address for Group Message", logical, NULL);
		ret++;
	}
	addr_decode(file->shared, &p, &(stab->heap_addr));
	if ((stab->heap_addr == CK_ADDR_UNDEF) || (stab->heap_addr >= file->shared->stored_eoa)) {
		error_push(ERR_LEV_2, ERR_LEV_2A18, 
		  "Invalid heap address for Group Message", logical, NULL);
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


/* Object Modification Date & Time */
static void *
OBJ_mdt_decode(driver_t *file, const uint8_t *p, const uint8_t *start_buf, ck_addr_t logi_base)
{
    	time_t      	*mesg;
    	uint32_t    	tmp_time;       /* Temporary copy of the time */
    	void        	*ret_value;     /* Return value */
	int		ret;
	ck_addr_t	logical;
	int		version, badinfo;


    	assert(p);
	ret = SUCCEED;

	logical = get_logical_addr(p, start_buf, logi_base);
    	/* decode */
	version = *p++;
    	if(version != H5O_MTIME_VERSION) {
		badinfo = version;
		error_push(ERR_LEV_2, ERR_LEV_2A19, 
		  "Version number should be 1 for Object Modification Date & Time Message", 
		  logical, &badinfo);
		ret++;
	}

    	/* Skip reserved bytes */
    	p+=3;

    	/* Get the time_t from the file */
	logical = get_logical_addr(p, start_buf, logi_base);
    	UINT32DECODE(p, tmp_time);

    	/* The return value */
    	if ((mesg=malloc(sizeof(time_t))) == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not malloc() time_t for Object Modification Date & Time Message", 
		  logical, NULL);
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

/*
 *  end decoding header messages
 */





static type_t *
type_alloc(ck_addr_t logical)
{
    type_t *dt;                  /* Pointer to datatype allocated */
    type_t *ret_value;           /* Return value */


    	/* Allocate & initialize new datatype info */
    	if((dt=calloc(1, sizeof(type_t)))==NULL) {
              	error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() type_t in type_alloc()", logical, NULL);
		return(ret_value=NULL);
	}

	assert(&(dt->ent));
    	memset(&(dt->ent), 0, sizeof(GP_entry_t));
	dt->ent.header = CK_ADDR_UNDEF;

    	if((dt->shared=calloc(1, sizeof(DT_shared_t)))==NULL) {
		if (dt != NULL) free(dt);
              	error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() H5T_shared_t in type_alloc()", logical, NULL);
		return(ret_value=NULL);
	}
    
    	/* Assign return value */
    	ret_value = dt;
    	return(ret_value);
}



static size_t
H5G_node_size(global_shared_t *shared)
{

    return (H5G_NODE_SIZEOF_HDR(shared) +
            (2 * SYM_LEAF_K(shared)) * GP_SIZEOF_ENTRY(shared));
}

static ck_err_t
gp_ent_decode(global_shared_t *shared, const uint8_t **pp, GP_entry_t *ent)
{
    	const uint8_t   *p_ret = *pp;
    	uint32_t        tmp;
	ck_err_t		ret = SUCCEED;

    	assert(pp);
    	assert(ent);

    	/* decode header */
    	DECODE_LENGTH(shared, *pp, ent->name_off);
    	addr_decode(shared, pp, &(ent->header));
    	UINT32DECODE(*pp, tmp);
    	*pp += 4; /*reserved*/
    	ent->type=(GP_type_t)tmp;

    	/* decode scratch-pad */
    	switch (ent->type) {
        case GP_NOTHING_CACHED:
            	break;

        case GP_CACHED_STAB:
            	assert(2*SIZEOF_ADDR(shared) <= GP_SIZEOF_SCRATCH);
            	addr_decode(shared, pp, &(ent->cache.stab.btree_addr));
            	addr_decode(shared, pp, &(ent->cache.stab.heap_addr));
            	break;

        case GP_CACHED_SLINK:
            	UINT32DECODE (*pp, ent->cache.slink.lval_offset);
            	break;

        default:  /* allows other cache values; will be validated later */
            	break;
    	}

    	*pp = p_ret + GP_SIZEOF_ENTRY(shared);
	return(SUCCEED);
}


static ck_err_t
gp_ent_decode_vec(global_shared_t *shared, const uint8_t **pp, GP_entry_t *ent, unsigned n)
{
    	unsigned    u;
    	ck_err_t      ret_value=SUCCEED;       /* Return value */

    	/* check arguments */
    	assert(pp);
    	assert(ent);

    	for (u = 0; u < n; u++)
        	if (ret_value=gp_ent_decode(shared, pp, ent + u) < 0) {
#if 0
/* it will never fail unless for assert() */
			error_push("gp_ent_decode_vec", ERR_LEV_1, ERR_LEV_1C, 
			  "Unable to read symbol table entries.", -1, -1);
            		return(ret_value);
#endif
		}

	return(ret_value);
}


static ck_addr_t
locate_super_signature(driver_t *file)
{
    	ck_addr_t       addr;
    	uint8_t         buf[HDF_SIGNATURE_LEN];
    	unsigned        n, maxpow;
	int		ret;

	addr = 0;

	/* find file size */
	addr = FD_get_eof(file);
	
    	/* Find the smallest N such that 2^N is larger than the file size */
    	for (maxpow=0; addr; maxpow++)
        	addr>>=1;
    	maxpow = MAX(maxpow, 9);

    	/*
     	 * Search for the file signature at format address zero followed by
     	 * powers of two larger than 9.
     	 */
    	for (n=8; n<maxpow; n++) {
        	addr = (8==n) ? 0 : (ck_addr_t)1 << n;
		if (FD_read(file, addr, (size_t)HDF_SIGNATURE_LEN, buf) == FAIL) {
			error_push(ERR_FILE, ERR_NONE_SEC, 
				"Errors when reading superblock signature", LOGI_SUPER_BASE, NULL);
			addr = CK_ADDR_UNDEF;
			break;
		}
		ret = memcmp(buf, HDF_SIGNATURE, (size_t)HDF_SIGNATURE_LEN);
		if (ret == 0) {
			if (debug_verbose())
				printf("FOUND super block signature\n");
            		break;
		}
    	}
	if (n >= maxpow) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
			"Unable to find super block signature", LOGI_SUPER_BASE, NULL);
		addr = CK_ADDR_UNDEF;
	}

    	/* Set return value */
    	return(addr);
}


static void
addr_decode(global_shared_t *shared, const uint8_t **pp/*in,out*/, ck_addr_t *addr_p/*out*/)
{
    	unsigned                i;
    	ck_addr_t                 tmp;
    	uint8_t                 c;
    	ck_bool_t                 all_zero = TRUE;
	ck_addr_t			ret;

    	assert(pp && *pp);
    	assert(addr_p);

    	*addr_p = 0;

    	for (i=0; i<SIZEOF_ADDR(shared); i++) {
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
        	*addr_p = CK_ADDR_UNDEF;
}


static size_t
gp_node_sizeof_rkey(global_shared_t *shared, unsigned UNUSED)
{

    return (SIZEOF_SIZE(shared));       /*the name offset */
}

/*????shared UNUSED ??? */
static void *
gp_node_decode_key(global_shared_t *shared, unsigned UNUSED, const uint8_t **p/*in,out*/)
{
	GP_node_key_t	*key;
	void		*ret_value;
	int		ret;

    	assert(p);
	assert(*p);
	ret = 0;

    	/* decode */
    	key = calloc(1, sizeof(GP_node_key_t));
	if (key == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() GP_node_key_t for key in B-tree", -1, NULL);
		return(ret_value=NULL);
	}

    	DECODE_LENGTH(shared, *p, key->offset);

    	/* Set return value */
    	ret_value = (void*)key;    /*success*/
    	return(ret_value);
}

static void *
raw_node_decode_key(global_shared_t *shared, size_t ndims, const uint8_t **p)
{

    	RAW_node_key_t    	*key = NULL;
    	unsigned            	u;
	void			*ret_value;
	int			ret;

    	/* check args */
    	assert(p);
	assert(*p);
	assert(ndims>0);
    	assert(ndims<=OBJ_LAYOUT_NDIMS);
	ret = 0;

    	/* decode */
    	key = calloc(1, sizeof(RAW_node_key_t));
	if (key == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Could not calloc() H5G_istore_key_t for key in B-tree", -1, NULL);
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





static size_t
raw_node_sizeof_rkey(global_shared_t *shared, unsigned ndims)
{
	
    	size_t 	nbytes;

	assert(ndims>0 && ndims<=OBJ_LAYOUT_NDIMS);

    	nbytes = 4 +           /*storage size          */
             	 4 +           /*filter mask           */
             	 ndims*8;      /*dimension indices     */

	return(nbytes);
}




/*
 * 1. Read in the information from the superblock
 * 2. Validate the information obtained.
 */
ck_err_t
check_superblock(driver_t *file)
{
	GP_entry_t 	*root_ent;
	uint		n;
	uint8_t		buf[SUPERBLOCK_SIZE];
	uint8_t		*p, *start_buf;
	ck_addr_t	logical;	/* logical address */
	ck_err_t	ret;
	char 		driver_name[9];
	int		badinfo;

	/* fixed size portion of the super block layout */
	const	size_t	fixed_size = 24;    /* fixed size portion of the superblock */
	unsigned	super_vers;         /* version # of super block */
    	unsigned	freespace_vers;     /* version # of global free-space storage */
    	unsigned	root_sym_vers;      /* version # of root group symbol table entry */
    	unsigned	shared_head_vers;   /* version # of shared header message format */

	/* variable length part of the super block */
	/* the rest is in shared global structure */
	size_t		variable_size;	    /* variable size portion of the superblock */

	global_shared_t	*lshared;

	/* 
	 * The super block may begin at 0, 512, 1024, 2048 etc. 
	 * or may not be there at all.
	 */
	ret = SUCCEED;

	lshared = file->shared;
	lshared->super_addr = 0;
	lshared->super_addr = locate_super_signature(file);
	if (lshared->super_addr == CK_ADDR_UNDEF) {
		if (!object_api()) {
			error_print(stderr, file);
			error_clear();
		}
		printf("ASSUMING super block at physical address 0.\n");
		lshared->super_addr = 0;
	}
	
	if (debug_verbose())
		printf("VALIDATING the super block at physical address %llu...\n", 
			lshared->super_addr);

	if (FD_read(file, LOGI_SUPER_BASE, fixed_size, buf) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, 
		  "Unable to read in the fixed size portion of the superblock", 
		  LOGI_SUPER_BASE, NULL);
		ret++;
		goto done;
	} 
	/* superblock signature already checked */
	p = buf + HDF_SIGNATURE_LEN;
	start_buf = buf;

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	super_vers = *p++;
	if (super_vers != SUPERBLOCK_VERSION_DEF && 
	    super_vers != SUPERBLOCK_VERSION_MAX) {
		badinfo = super_vers;
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Version number of Super Block should be 0 or 1", 
		  logical, &badinfo);
		ret++;
	}


	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	freespace_vers = *p++;

	if (freespace_vers != FREESPACE_VERSION) {
		badinfo = freespace_vers;
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Version number of Global Free-space Storage should be 0", 
		  logical, &badinfo);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	root_sym_vers = *p++;
	if (root_sym_vers != OBJECTDIR_VERSION) {
		badinfo = root_sym_vers;
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Version number of the Root Group Symbol Table Entry should be 0", 
		  logical, &badinfo);
		ret++;
	}

	/* skip over reserved byte */
	p++;

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	shared_head_vers = *p++;
	if (shared_head_vers != SHAREDHEADER_VERSION) {
		badinfo = shared_head_vers;
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Version number of Shared Header Message Format should be 0", 
		  logical, &badinfo);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	lshared->size_offsets = *p++;
	if (lshared->size_offsets != 2 && lshared->size_offsets != 4 &&
            lshared->size_offsets != 8 && lshared->size_offsets != 16 && 
	    lshared->size_offsets != 32) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid Size of Offsets", logical, NULL);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	lshared->size_lengths = *p++;
	if (lshared->size_lengths != 2 && lshared->size_lengths != 4 &&
            lshared->size_lengths != 8 && lshared->size_lengths != 16 && 
	    lshared->size_lengths != 32) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid Size of Lengths", logical, NULL);
		ret++;
	}
	/* skip over reserved byte */
	p++;

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	UINT16DECODE(p, lshared->gr_leaf_node_k);
	if (lshared->gr_leaf_node_k <= 0) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid value for Group Leaf Node K", logical, NULL);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	UINT16DECODE(p, lshared->btree_k[BT_SNODE_ID]);
	if (lshared->btree_k[BT_SNODE_ID] <= 0) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid value for Group Internal Node K", logical, NULL);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	/* File consistency flags. Not really used yet */
    	UINT32DECODE(p, lshared->file_consist_flg);
	if (lshared->file_consist_flg > 0x03) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid file consistency flags.", logical, NULL);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	/* decode the variable length part of the super block */
	/* Potential indexed storage internal node K */
    	variable_size = (super_vers>0 ? 4 : 0) + 
                    lshared->size_offsets +  /* base addr */
                    lshared->size_offsets +  /* address of global free-space heap */
                    lshared->size_offsets +  /* end of file address*/
                    lshared->size_offsets +  /* driver-information block address */
                    GP_SIZEOF_ENTRY(lshared); /* root group symbol table entry */

	if ((fixed_size + variable_size) > sizeof(buf)) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
			"Total size of super block is incorrect", LOGI_SUPER_BASE, NULL);
		ret++;
		return(ret);
	}

	if (FD_read(file, LOGI_SUPER_BASE+fixed_size, variable_size, p) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, 
		  "Unable to read in the variable length part of the superblock", 
		  logical, NULL);
		ret++;
		goto done;
	}

    	/*
     	 * If the superblock version # is greater than 0, read in the indexed
     	 * storage B-tree internal 'K' value
     	 */
    	if (super_vers > 0) {
		logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
        	UINT16DECODE(p, lshared->btree_k[BT_ISTORE_ID]);
        	p += 2;   /* reserved */
    		if (lshared->btree_k <= 0) {
			error_push(ERR_LEV_0, ERR_LEV_0A, 
			  "Invalid value for Indexed Storage Internal Node K", 
			  logical, NULL);
			ret++;
		}
	} else
		lshared->btree_k[BT_ISTORE_ID] = BT_ISTORE_K;


	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	addr_decode(lshared, (const uint8_t **)&p, &lshared->base_addr);
	/* SHOULD THERE BE MORE VALIDATION of base_addr?? */
	if (lshared->base_addr != lshared->super_addr) {
		error_push(ERR_LEV_0, ERR_LEV_0A, "Invalid base address", 
		  logical, NULL);
		ret++;
	} 

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
    	addr_decode(lshared, (const uint8_t **)&p, &lshared->freespace_addr);
	if (lshared->freespace_addr != CK_ADDR_UNDEF) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid address of Global Free-space Heap", logical, NULL);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
    	addr_decode(lshared, (const uint8_t **)&p, &lshared->stored_eoa);
	if ((lshared->stored_eoa == CK_ADDR_UNDEF) ||
	    (lshared->base_addr >= lshared->stored_eoa)) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid End of File Address", logical, NULL);
		ret++;
	}

    	addr_decode(lshared, (const uint8_t **)&p, &lshared->driver_addr);


	root_ent = calloc(1, sizeof(GP_entry_t));
	logical = get_logical_addr(p, start_buf, LOGI_SUPER_BASE);
	if (gp_ent_decode(lshared, (const uint8_t **)&p, root_ent/*out*/) < 0) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
			"Unable to read root symbol entry", logical, NULL);
		ret++;
		return(ret);
	}
	lshared->root_grp = root_ent;

#ifdef NEEDSTOBEDONE
WHAT IF THE Root grp pointer is undefined???
what if the rootgrp ->header is undefined for symbolic link??? will that happen
OR the whole thing shouldn't be checked here should be in check_obj_header()???
Should all the asserts be validation???
#endif
	/* NEED to validate lshared->root_grp->name_off??? to be within size of local heap */
	if ((lshared->root_grp->header==CK_ADDR_UNDEF) || 
	    (lshared->root_grp->header >= lshared->stored_eoa)) {
		error_push(ERR_LEV_0, ERR_LEV_0A, 
		  "Invalid object header address in root group symbol table entry", logical, NULL);
		ret++;
	}
	if (lshared->root_grp->type < 0) {
		error_push(ERR_LEV_0, ERR_LEV_0A, "Invalid cache type in root group symbol table entry", logical, NULL);
		ret++;
	}

	driver_name[0] = '\0';

	/* Read in driver information block and validate */
	/* Defined driver information block address or not */
    	if (lshared->driver_addr != CK_ADDR_UNDEF) {
		/* since base_addr may not be valid, use super_addr to be sure */
		ck_addr_t drv_addr = lshared->super_addr + lshared->driver_addr;
        	uint8_t dbuf[DRVINFOBLOCK_SIZE];     /* Local buffer */
		uint8_t	*drv_start;
		size_t  driver_size;   /* Size of driver info block, in bytes */
		int	drv_version;

		if (((drv_addr+16) == CK_ADDR_UNDEF) || ((drv_addr+16) >= lshared->stored_eoa)) {
			error_push(ERR_LEV_0, ERR_LEV_0B, 
			  "Invalid driver information block", LOGI_SUPER_BASE+lshared->driver_addr, NULL);
			ret++;
			return(ret);
		}

		/* read in the first 16 bytes of the driver information block */
		if (FD_read(file, lshared->driver_addr, 16, dbuf) == FAIL) {
			error_push(ERR_FILE, ERR_NONE_SEC, 
			  "Unable to read in the 16 bytes of the driver information block.", 
			  LOGI_SUPER_BASE+lshared->driver_addr, NULL);
			ret++;
			goto done;
		}

		drv_start = dbuf;
		p = dbuf;
		logical = get_logical_addr(p, drv_start, lshared->driver_addr);
		drv_version = *p++;
		if (drv_version != DRIVERINFO_VERSION) {
			badinfo = drv_version;
			error_push(ERR_LEV_0, ERR_LEV_0B, 
			  "Driver information block version number should be 0", 
			  logical, &badinfo);
			ret++;
		}
		p += 3; /* reserved */

		/* Driver info size */
        	UINT32DECODE(p, driver_size);

		
		 /* Driver name and/or version */
        	strncpy(driver_name, (const char *)p, (size_t)8);
        	driver_name[8] = '\0';
		set_driver(&(lshared->driverid), driver_name);
		p += 8; /* advance past driver identification */

		logical = get_logical_addr(p, drv_start, lshared->driver_addr);

		 /* Read driver information and decode */
        	assert((driver_size + 16) <= sizeof(dbuf));
		if (((drv_addr+16+driver_size) == CK_ADDR_UNDEF) || 
		    ((drv_addr+16+driver_size) >= lshared->stored_eoa)) {
			error_push(ERR_LEV_0, ERR_LEV_0B, 
			  "Invalid driver information size", logical, NULL);
			ret++;
		}
		if (FD_read(file, lshared->driver_addr+16, driver_size, p) == FAIL) {
			error_push(ERR_FILE, ERR_NONE_SEC, 
			  "Unable to read in the driver information part", logical, NULL);
			ret++;
			goto done;
		}
		decode_driver(lshared, p);
	}  /* DONE with driver information block */
	else /* sec2 driver is used when no driver information */
		set_driver(&(lshared->driverid), driver_name);

#ifdef DEBUG
	printf("base_addr=%llu, super_addr=%llu, stored_eoa=%llu, driver_addr=%llu\n",
		lshared->base_addr, lshared->super_addr, lshared->stored_eoa, 
		lshared->driver_addr==CK_ADDR_UNDEF?9999:lshared->driver_addr);
	printf("name0ffset=%d, header_address=%llu\n", 
		lshared->root_grp->name_off, lshared->root_grp->header);
	printf("btree_addr=%llu, heap_addr=%llu\n", 
		lshared->root_grp->cache.stab.btree_addr, 
		lshared->root_grp->cache.stab.heap_addr);
	printf("super_addr = %llu\n", lshared->super_addr);
	printf("super_vers=%d, freespace_vers=%d, root_sym_vers=%d\n",	
		super_vers, freespace_vers, root_sym_vers);
	printf("size_offsets=%d, size_lengths=%d\n",	
		lshared->size_offsets, lshared->size_lengths);
	printf("gr_leaf_node_k=%d, gr_int_node_k=%d, file_consist_flg=%d\n",	
		lshared->gr_leaf_node_k, lshared->gr_int_node_k, lshared->file_consist_flg);
	printf("base_addr=%llu, freespace_addr=%llu, stored_eoa=%llu, driver_addr=%llu\n",
		lshared->base_addr, lshared->freespace_addr, lshared->stored_eoa, lshared->driver_addr);

	/* print root group table entry */
	printf("name0ffset=%d, header_address=%llu\n", 
		lshared->root_grp->name_off, lshared->root_grp->header);
	printf("btree_addr=%llu, heap_addr=%llu\n", 
		lshared->root_grp->cache.stab.btree_addr, 
		lshared->root_grp->cache.stab.heap_addr);
#endif

done:
	return(ret);
}


static ck_err_t
check_sym(driver_t *file, ck_addr_t sym_addr, int prev_entries) 
{
	size_t 		size = 0;
	uint8_t		*buf = NULL;
	uint8_t		*p;
	ck_err_t 		ret;
	unsigned	nsyms, u;
	GP_node_t	*sym=NULL;
	GP_entry_t	*ent;
	int		sym_version, badinfo;

	assert(addr_defined(sym_addr));

	if (debug_verbose())
		printf("VALIDATING the SNOD at logical address %llu...\n", sym_addr);
	size = H5G_node_size(file->shared);
	ret = SUCCEED;

#ifdef DEBUG
	printf("sym_addr=%d, symbol table node size =%d\n", 
		sym_addr, size);
#endif

	buf = malloc(size);
	if (buf == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Unable to malloc() a symbol table node.", sym_addr, NULL);
		ret++;
		goto done;
	}

	sym = malloc(sizeof(GP_node_t));
	if (sym == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, "Unable to malloc() GP_node_t.", 
		  sym_addr, NULL);
		ret++;
		goto done;
	}
	sym->entry = malloc(2*SYM_LEAF_K(file->shared)*sizeof(GP_entry_t));
	if (sym->entry == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, "Unable to malloc() GP_entry_t.", 
		  sym_addr, NULL);
		ret++;
		goto done;
	}

	if (FD_read(file, sym_addr, size, buf) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, "Unable to read in the symbol table node.", 
		  sym_addr, NULL);
		ret++;
		goto done;
	}

	p = buf;
    	ret = memcmp(p, H5G_NODE_MAGIC, H5G_NODE_SIZEOF_MAGIC);
	if (ret != 0) {
		error_push(ERR_LEV_1, ERR_LEV_1B, "Could not find SNOD signature.", sym_addr, NULL);
		ret++;
	} else if (debug_verbose())
		printf("FOUND SNOD signature.\n");

    	p += 4;
	
	 /* version */
	sym_version = *p++;
    	if (H5G_NODE_VERS != sym_version) {
		badinfo = sym_version;
		error_push(ERR_LEV_1, ERR_LEV_1B, "Symbol table node version should be 1.", 
		  sym_addr, &badinfo);
		ret++;
	}

    	/* reserved */
    	p++;

    	/* number of symbols */
    	UINT16DECODE(p, sym->nsyms);
	if (sym->nsyms > (2 * SYM_LEAF_K(file->shared))) {
		error_push(ERR_LEV_1, ERR_LEV_1B, "Number of symbols exceed (2*Group Leaf Node K)", 
		  sym_addr, NULL);
		ret++;
	}


	/* reading the vector of symbol table group entries */
	ret = gp_ent_decode_vec(file->shared, (const uint8_t **)&p, sym->entry, sym->nsyms);
	if (ret != SUCCEED) {
		error_push(ERR_LEV_1, ERR_LEV_1B, 
		  "Unable to read in symbol table group entries", sym_addr, NULL);
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
		/* for symbolic link: validate link value to be within size of local heap */

		/* for symbolic link: the object header address is undefined */
		if (ent->type != GP_CACHED_SLINK) {
		if ((ent->header==CK_ADDR_UNDEF) || (ent->header >= file->shared->stored_eoa)) {
			error_push(ERR_LEV_1, ERR_LEV_1C, "Invalid object header address.", 
			  sym_addr, NULL);
			ret++;
			continue;
		}
	
		if (ent->type < 0) {
			error_push(ERR_LEV_1, ERR_LEV_1C, "Invalid cache type", sym_addr, NULL);
			ret++;
		}

		ret = check_obj_header(file, ent->header, NULL, NULL, prev_entries);
		if (ret != SUCCEED) {
			error_push(ERR_LEV_1, ERR_LEV_1C, 
			  "Errors found when checking the object header in the group entry...", 
			  ent->header, NULL);
			if (!object_api()) {
				error_print(stderr, file);
				error_clear();
			}
			ret = SUCCEED;
		}
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

static ck_err_t
check_btree(driver_t *file, ck_addr_t btree_addr, unsigned ndims, int prev_entries)
{
	uint8_t		*buf=NULL, *buffer=NULL;
	uint8_t		*p, nodetype;
	unsigned	nodelev, entries, u;
	ck_addr_t         left_sib;   /*address of left sibling */
	ck_addr_t         right_sib;  /*address of left sibling */
	size_t		hdr_size, name_offset, key_ptr_size;
	size_t		key_size, new_key_ptr_size;
	int		ret, status;
	ck_addr_t		child;
	void 		*key;

	uint8_t		*start_buf;
	ck_addr_t	logical;
	int		badinfo;


	assert(addr_defined(btree_addr));
    	hdr_size = H5B_SIZEOF_HDR(file->shared);
	ret = SUCCEED;

	if (debug_verbose())
		printf("VALIDATING the btree at logical address %llu...\n", btree_addr);

	buf = malloc(hdr_size);
	if (buf == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "B-tree:Unable to malloc() B-tree header", btree_addr, NULL);
		ret++;
		goto done;
	}

	if (FD_read(file, btree_addr, hdr_size, buf) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, 
		  "B-tree:Unable to read B-tree header", logical, NULL);
		ret++;
		goto done;
	}

	start_buf = buf;
	logical = get_logical_addr(p, start_buf, btree_addr);
	p = buf;

	/* magic number */
	ret = memcmp(p, H5B_MAGIC, (size_t)H5B_SIZEOF_MAGIC);
	if (ret != 0) {
		error_push(ERR_LEV_1, ERR_LEV_1A, 
		  "B-tree:Could not find B-tree signature", btree_addr, NULL);
		ret++;
		goto done;
	} else if (debug_verbose())
		printf("FOUND btree signature.\n");

	p+=4;

	logical = get_logical_addr(p, start_buf, btree_addr);
	nodetype = *p++;
	if ((nodetype != 0) && (nodetype !=1)) {
		badinfo = nodetype;
		error_push(ERR_LEV_1, ERR_LEV_1A, 
		  "B-tree:Node Type should be 0 or 1", logical, &badinfo);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, btree_addr);
	nodelev = *p++;
	if (nodelev < 0) {
		badinfo = nodelev;
		error_push(ERR_LEV_1, ERR_LEV_1A, 
		  "B-tree:Node Level should not be negative", logical, &badinfo);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, btree_addr);
	UINT16DECODE(p, entries);

	/* probably need to get previous level */
	/* for nodelev==0 & nodetype==0, check_sym() will check on the limit */
	if (prev_entries > 1){ /* this is not a single leaf node */
#if 0
		printf("THIS IS  NOT A SINGLE LEAF NODE\n");
#endif
	if (!((nodelev==0) && (nodetype==0))) {
		if (!((file->shared->btree_k[nodetype] <= entries) && 
		     (entries < 2*file->shared->btree_k[nodetype])))
			printf("nodelev=%d, nodetype=%d, K=%d,entries should be >=K && < 2K=%d\n", 
			nodelev, nodetype, file->shared->btree_k[nodetype], entries);
	} else  {
#if 0
		printf("A SNOD leaf node, validate in check_sym()\n");
#endif
	}

	} else {
#if 0
		printf("THIS IS  A SINGLE NODE\n");
#endif
		if ((nodelev > 0) || (nodetype==1)) {
			if (entries >= 2*file->shared->btree_k[nodetype])
				printf("Should have fewer than 2k nodes for single node\n");
		} else { /* printf("And an SNOD\n"); */ }
	}

	prev_entries = entries;

#if 0
		  "B-tree:Entries Used is less than K or exceed 2K", logical, -1);
#endif

	logical = get_logical_addr(p, start_buf, btree_addr);
	addr_decode(file->shared, (const uint8_t **)&p, &left_sib/*out*/);
	if ((left_sib != CK_ADDR_UNDEF) && (left_sib >= file->shared->stored_eoa)) {
		error_push(ERR_LEV_1, ERR_LEV_1A, 
		  "B-tree:Invalid left sibling address", logical, NULL);
		ret++;
	}

	logical = get_logical_addr(p, start_buf, btree_addr);
	addr_decode(file->shared, (const uint8_t **)&p, &right_sib/*out*/);
	if ((right_sib != CK_ADDR_UNDEF) && (right_sib >= file->shared->stored_eoa)) {
		error_push(ERR_LEV_1, ERR_LEV_1A, 
		  "B-tree:Invalid left sibling address", logical, NULL);
		ret++;
	}

#if 0
	printf("nodeytpe=%d, nodelev=%d, entries=%u, btree_k0=%u, btree_k1=%u, leaf_k=%u\n",
		nodetype, nodelev, entries, file->shared->btree_k[0], file->shared->btree_k[1], 
		file->shared->gr_leaf_node_k);
	printf("left_sib=%d, right_sib=%d\n", left_sib, right_sib);
#endif

	/* the remaining node size: key + child pointer */
	key_size = node_key_g[nodetype]->get_sizeof_rkey(file->shared, ndims);

	key_ptr_size = (2*file->shared->btree_k[nodetype])*SIZEOF_ADDR(file->shared) +
		       (2*(file->shared->btree_k[nodetype]+1))*key_size;
#if 0
	printf("btree_k[SNODE]=%u, btree_k[ISTORE]=%u\n", file->shared->btree_k[BT_SNODE_ID], 
		file->shared->btree_k[BT_ISTORE_ID]);
	printf("key_size=%d, key_ptr_size=%d\n", key_size, key_ptr_size);
#endif
	buffer = malloc(key_ptr_size);
	if (buffer == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "B-tree:Unable to malloc() key+child", btree_addr, NULL);
		ret++;
		goto done;
	}

	if (FD_read(file, btree_addr+hdr_size, key_ptr_size, buffer) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, 
		  "B-tree:Unable to read key+child", btree_addr, NULL);
		ret++;
		goto done;
	}

	start_buf = buffer;
	p = buffer;
		
	for (u = 0; u < entries; u++) {
		key = node_key_g[nodetype]->decode(file->shared, ndims, (const uint8_t **)&p);
		if (key == NULL) {
			error_push(ERR_LEV_1, ERR_LEV_1A, 
			  "B-tree:Errors when decoding key", logical, NULL);
			if (!object_api()) {
				error_print(stderr, file);
				error_clear();
			}
			continue;
		}

#if 0
		if (nodetype == 0)
		printf("key's offset=%d\n", ((GP_node_key_t *)key)->offset);
		else if (nodetype == 1) {
		printf("size of chunk =%d;offset:%u,%u,%u\n", ((RAW_node_key_t *)key)->nbytes,
			((RAW_node_key_t *)key)->offset[0],
			((RAW_node_key_t *)key)->offset[1],
			((RAW_node_key_t *)key)->offset[2]);
		}
#endif
		
		/* NEED TO VALIDATE name_offset to be within the local heap's size??HOW */

		logical = get_logical_addr(p, start_buf, btree_addr+hdr_size);
        	/* Decode address value */
  		addr_decode(file->shared, (const uint8_t **)&p, &child/*out*/);

		if ((child != CK_ADDR_UNDEF) && (child >= file->shared->stored_eoa)) {
			error_push(ERR_LEV_1, ERR_LEV_1A, 
			  "B-tree:Invalid child address found", logical, NULL);
			ret++;
			continue;
		}

		if (nodelev > 0) {
			status = check_btree(file, child, ndims, prev_entries);
			if (status != SUCCEED) {
				error_push(ERR_LEV_1, ERR_LEV_1A, 
				  "Errors from when checking B-tree...", logical, NULL);
				if (!object_api()) {
					error_print(stderr, file);
					error_clear();
				}
				ret = SUCCEED;
				continue;
			}
		} else {
			if (nodetype == 0) {
				status = check_sym(file, child, prev_entries);
				if (status != SUCCEED) {
					error_push(ERR_LEV_1, ERR_LEV_1A, 
					  "Errors when checking Group Node...", logical, NULL);
					if (!object_api()) {
						error_print(stderr, file);
						error_clear();
					}
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
		key = node_key_g[nodetype]->decode(file->shared, ndims, (const uint8_t **)&p);
		/* After decoding, then what ???  NEED TO CHECK INTO THIS */
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


static ck_err_t
check_lheap(driver_t *file, ck_addr_t lheap_addr, uint8_t **ret_heap_chunk)
{
	uint8_t		hdr[52];
	size_t		hdr_size, data_seg_size;
 	size_t  	next_free_off = H5HL_FREE_NULL;
	size_t		size_free_block, saved_offset;
	uint8_t		*heap_chunk=NULL;

	ck_addr_t		addr_data_seg;
	int		ret;
	const uint8_t	*p = NULL;

	uint8_t	*start_buf = NULL;
	ck_addr_t	logical;
	int		lheap_version, badinfo;



	assert(addr_defined(lheap_addr));
    	hdr_size = H5HL_SIZEOF_HDR(file->shared);
    	assert(hdr_size<=sizeof(hdr));

	if (debug_verbose())
		printf("VALIDATING the local heap at logical address %llu...\n", lheap_addr);
	ret = SUCCEED;

	if (FD_read(file, lheap_addr, hdr_size, hdr) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, 
		  "Local Heap:Unable to read local heap header", lheap_addr, NULL);
		ret++;
		goto done;
	}

	start_buf = hdr;
    	p = hdr;
	logical = get_logical_addr(p, start_buf, lheap_addr);

	/* magic number */
	ret = memcmp(p, H5HL_MAGIC, H5HL_SIZEOF_MAGIC);
	if (ret != 0) {
		error_push(ERR_LEV_1, ERR_LEV_1D, 
		  "Local Heap:Could not find local heap signature", logical, NULL);
		ret++;
		goto done;
	} else if (debug_verbose())
		printf("FOUND local heap signature.\n");

	p += H5HL_SIZEOF_MAGIC;

	logical = get_logical_addr(p, start_buf, lheap_addr);
	 /* Version */
	lheap_version = *p++;
    	if (lheap_version != H5HL_VERSION) {
		badinfo = lheap_version;
		error_push(ERR_LEV_1, ERR_LEV_1D, 
		  "Local Heap:version number should be 0", logical, &badinfo);
		ret++; /* continue on */
	}

    	/* Reserved */
    	p += 3;


	logical = get_logical_addr(p, start_buf, lheap_addr);
	/* data segment size */
    	DECODE_LENGTH(file->shared, p, data_seg_size);
	if (data_seg_size <= 0) {
		error_push(ERR_LEV_1, ERR_LEV_1D, 
		  "Local Heap:Invalid data segment size", logical, NULL);
		ret++;
		goto done;
	}

	/* offset to head of free-list */
    	DECODE_LENGTH(file->shared, p, next_free_off);
#if 0
	if ((ck_addr_t)next_free_off != CK_ADDR_UNDEF && (ck_addr_t)next_free_off != H5HL_FREE_NULL && (ck_addr_t)next_free_off >= data_seg_size) {
		error_push(ERR_LEV_1, ERR_LEV_1D, 
		  "Offset to head of free list is invalid.", lheap_addr, NOT_REP, -1);
		ret++;
		goto done;
	}
#endif

	/* address of data segment */
	logical = get_logical_addr(p, start_buf, lheap_addr);
    	addr_decode(file->shared, &p, &addr_data_seg);
#if 0
	addr_data_seg = addr_data_seg + file->shared->base_addr;
#endif
	if ((addr_data_seg == CK_ADDR_UNDEF) || ((addr_data_seg+file->shared->base_addr) >= file->shared->stored_eoa)) {
		error_push(ERR_LEV_1, ERR_LEV_1D, 
		  "Local Heap:Address of data segment is invalid", logical, NULL);
		ret++;
		goto done;
	}

	heap_chunk = malloc(hdr_size+data_seg_size);
	if (heap_chunk == NULL) {
		error_push(ERR_LEV_1, ERR_LEV_1D, 
		  "Local Heap:Memory allocation failed for data segment", logical, NULL);
		ret++;
		goto done;
	}
	
#ifdef DEBUG
	printf("data_seg_size=%d, next_free_off =%d, addr_data_seg=%d\n",
		data_seg_size, next_free_off, addr_data_seg);
#endif

	if (data_seg_size) {
		if (FD_read(file, addr_data_seg, data_seg_size, heap_chunk+hdr_size) == FAIL) {
			error_push(ERR_FILE, ERR_NONE_SEC, 
			  "Local Heap:Unable to read data segment", logical, NULL);
			ret++;
			goto done;
		}
	} 


	start_buf = heap_chunk+hdr_size;

	/* traverse the free list */
	while (next_free_off != H5HL_FREE_NULL) {
		if (next_free_off >= data_seg_size) {
			error_push(ERR_LEV_1, ERR_LEV_1D, 
			  "Local Heap:Offset of the next free block is invalid", logical, NULL);
			ret++;
			goto done;
		}
		saved_offset = next_free_off;
		p = heap_chunk + hdr_size + next_free_off;
		DECODE_LENGTH(file->shared, p, next_free_off);
		get_logical_addr(p, start_buf, addr_data_seg);
		DECODE_LENGTH(file->shared, p, size_free_block);
#if DEBUG
		printf("next_free_off=%d, size_free_block=%d\n",
			next_free_off, size_free_block);
#endif

		if (!(size_free_block >= (2*SIZEOF_SIZE(file->shared)))) {
			error_push(ERR_LEV_1, ERR_LEV_1D, 
			  "Local Heap:Offset of the next free block is invalid", logical, NULL);
			ret++;  /* continue on */
		}
        	if (saved_offset + size_free_block > data_seg_size) {
			error_push(ERR_LEV_1, ERR_LEV_1D, 
			  "Local Heap:Bad heap free list", logical, NULL);
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


static ck_err_t
check_gheap(driver_t *file, ck_addr_t gheap_addr, uint8_t **ret_heap_chunk)
{
    	H5HG_heap_t 	*heap = NULL;
    	uint8_t     	*p = NULL;
    	int 		ret, i;
    	size_t      	nalloc, need;
    	size_t      	max_idx=0;              /* The maximum index seen */
    	H5HG_heap_t 	*ret_value = NULL;      /* Return value */

	uint8_t		*start_buf;
	ck_addr_t	logical;
	int		gheap_version, badinfo;


    	/* check arguments */
    	assert(addr_defined(gheap_addr));
	ret = SUCCEED;

    	/* Read the initial 4k page */
    	if ((heap = calloc(1, sizeof(H5HG_heap_t)))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Global Heap:Couldn't calloc() H5HG_heap_t", gheap_addr, NULL);
		ret++;
		goto done;
	}
    	heap->addr = gheap_addr;
    	if ((heap->chunk = malloc(H5HG_MINSIZE))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Gloabl Heap:Couldn't malloc() H5HG_heap_t->chunk", gheap_addr, NULL);
		ret++;
		goto done;
	}

 	if (debug_verbose())
		printf("VALIDATING the global heap at logical address %llu...\n", gheap_addr);
        
	if (FD_read(file, gheap_addr, H5HG_MINSIZE, heap->chunk) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, 
		  "Global Heap:Unable to read global heap collection", gheap_addr, NULL);
		ret++;
		goto done;
	}

    	/* Magic number */
    	if (memcmp(heap->chunk, H5HG_MAGIC, H5HG_SIZEOF_MAGIC)) {
		error_push(ERR_LEV_1, ERR_LEV_1E, 
		  "Global Heap:Could not find GCOL signature", gheap_addr, NULL);
		ret++;
	} else if (debug_verbose())
		printf("FOUND GLOBAL HEAP SIGNATURE\n");

	start_buf = heap->chunk;
    	p = heap->chunk + H5HG_SIZEOF_MAGIC;
	logical = get_logical_addr(p, start_buf, gheap_addr);

    	/* Version */
	gheap_version = *p++;
    	if (H5HG_VERSION != gheap_version) {
		badinfo = gheap_version;
		error_push(ERR_LEV_1, ERR_LEV_1E, 
		  "Global Heap:version number should be 1", logical, &badinfo);
		ret++;
	} else
		printf("Version 1 of global heap is detected\n");

    	/* Reserved */
    	p += 3;

	logical = get_logical_addr(p, start_buf, gheap_addr);
    	/* Size */
    	DECODE_LENGTH(file->shared, p, heap->size);
    	assert (heap->size>=H5HG_MINSIZE);

    	if (heap->size > H5HG_MINSIZE) {
        	ck_addr_t next_addr = gheap_addr + (ck_size_t)H5HG_MINSIZE;

        	if ((heap->chunk = realloc(heap->chunk, heap->size))==NULL) {
			error_push(ERR_LEV_1, ERR_LEV_1E, 
			  "Global Heap:Couldn't realloc() H5HG_heap_t->chunk", logical, NULL);
			ret++;
			goto done;
		}
		if (FD_read(file, next_addr, heap->size-H5HG_MINSIZE, heap->chunk+H5HG_MINSIZE) == FAIL) {
			error_push(ERR_FILE, ERR_NONE_SEC, 
			  "Global Heap:Unable to read global heap collection", logical, NULL);
			ret++;
			goto done;
		}
	}  /* end if */

    	/* Decode each object */
    	p = heap->chunk + H5HG_SIZEOF_HDR(file->shared);
	logical = get_logical_addr(p, start_buf, gheap_addr);
    	nalloc = H5HG_NOBJS(file->shared, heap->size);
    	if ((heap->obj = malloc(nalloc*sizeof(H5HG_obj_t)))==NULL) {
                error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Global Heap:Couldn't malloc() H5HG_obj_t", logical, NULL);
                ret++;
		goto done;
	}
    	heap->obj[0].size = heap->obj[0].nrefs=0;
    	heap->obj[0].begin = NULL;

    	heap->nalloc = nalloc;
    	while (p < heap->chunk+heap->size) {
        	if (p+H5HG_SIZEOF_OBJHDR(file->shared) > heap->chunk+heap->size) {
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

				logical = get_logical_addr(p, start_buf, gheap_addr);
                		/* Reallocate array of objects */
                		if ((new_obj=realloc(heap->obj, new_alloc*sizeof(H5HG_obj_t)))==NULL) {
                			error_push(ERR_LEV_1, ERR_LEV_1E, 
					  "Global Heap:Couldn't realloc() H5HG_obj_t", logical, NULL);
                			ret++;
					goto done;
				}

                		/* Update heap information */
                		heap->nalloc = new_alloc;
                		heap->obj = new_obj;
            		} /* end if */

            		UINT16DECODE (p, heap->obj[idx].nrefs);
            		p += 4; /*reserved*/
            		DECODE_LENGTH (file->shared, p, heap->obj[idx].size);
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
                		need = H5HG_SIZEOF_OBJHDR(file->shared) + H5HG_ALIGN(heap->obj[idx].size);

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

static ck_err_t
decode_validate_messages(driver_t *file, H5O_t *oh, int prev_entries)
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
	ck_addr_t		global;
	unsigned	ndims = 0;
	unsigned	local_ndims = 0;

uint8_t	*start_buf, *p;
ck_addr_t	logical, logi_base;

	ret = SUCCEED;
	for (i = 0; i < oh->nmesgs; i++) {	
	  status = SUCCEED;
	  start_buf = oh->chunk[oh->mesg[i].chunkno].image;
	  p = oh->mesg[i].raw;
	  logi_base = oh->chunk[oh->mesg[i].chunkno].addr;
	  logical = get_logical_addr(p, start_buf, logi_base);
#if 0
printf("addr=%llu, type=%d, logical=%llu, raw_size=%llu\n", 
	logi_base, oh->mesg[i].type->id, logical, oh->mesg[i].raw_size);
#endif
	  id = oh->mesg[i].type->id;
	  if (id == OBJ_CONT_ID) {
		continue;
	  } else if (id == OBJ_NIL_ID) {
		continue;
	  } else if (oh->mesg[i].flags & OBJ_FLAG_SHARED) {
	  	mesg = OBJ_SHARED->decode(file, oh->mesg[i].raw, start_buf, logi_base);
		if (mesg != NULL) {
/* NEED TO TAKE CARE OF THIS */
			mesg = H5O_shared_read(file, mesg, oh->mesg[i].type, prev_entries);
printf("I am returnon from H5O_shared_read in decode validatemessage\n");
			if (mesg == NULL) {
				error_push(ERR_LEV_2, ERR_LEV_2A, 
				  "Errors from H5O_shared_read() when decoding header message", 
				  logical, NULL);
				if (!object_api()) {
					error_print(stderr, file);
					error_clear();
				}
				ret = SUCCEED;
				continue;
			}
		}
	  } else {
	  	mesg = message_type_g[id]->decode(file, oh->mesg[i].raw, start_buf, logi_base);
	  }

	  if (mesg == NULL) {
#if 0
printf("failure logi_base=%lld, type=%d, logical=%lld, raw_size=%lld\n", 
	logi_base, oh->mesg[i].type->id, logical, oh->mesg[i].raw_size);
#endif
		error_push(ERR_LEV_2, ERR_LEV_2A, 
		  "Errors found when decoding header message", logical, NULL);
		ret++;
		continue;
	  }
	  switch (id) {
		case OBJ_SDS_ID:
		case OBJ_DT_ID:
		case OBJ_FILL_ID:
    		case OBJ_FILTER_ID:
		case OBJ_MDT_ID:
		case OBJ_COMM_ID:
    		case OBJ_ATTR_ID:
			break;

    		case OBJ_EDF_ID:
			status = check_lheap(file,((obj_edf_t *)mesg)->heap_addr, &heap_chunk);
			if (status != SUCCEED) {
				error_push(ERR_LEV_2, ERR_LEV_2A8, 
				  "Errors found when checking Local Heap from External Data Files header message",
				  logical, NULL);
				if (!object_api()) {
					error_print(stderr, file);
					error_clear();
				}
				ret = SUCCEED;
			} else {  /* SUCCEED */
				int 	has_error = FALSE;
    				for (k = 0; k < ((obj_edf_t *)mesg)->nused; k++) {
					s = heap_chunk+H5HL_SIZEOF_HDR(file->shared)
						+((obj_edf_t *)mesg)->slot[k].name_offset;
					if (s) {
						if (!(*s)) {
							error_push(ERR_LEV_2, ERR_LEV_2A8, 
				  				"Invalid external file name found in local heap",
				  				logical, NULL);
							has_error = TRUE;
						}
					} else {
						error_push(ERR_LEV_2, ERR_LEV_2A8, 
				  			"Invalid name offset into local heap", logical, NULL);
						has_error = TRUE;
					}
				}  /* end for */
				if (has_error) {
					if (!object_api()) {
						error_print(stderr, file);
						error_clear();
					}
					ret = SUCCEED;
				}
			}  /* end else */
			if (heap_chunk) 
				free(heap_chunk);
			break;
		case OBJ_LAYOUT_ID:
			if (((OBJ_layout_t *)mesg)->type == DATA_CHUNKED) {
				ck_addr_t		btree_addr;
				
				local_ndims = ((OBJ_layout_t *)mesg)->u.chunk.ndims;
				btree_addr = ((OBJ_layout_t *)mesg)->u.chunk.addr;
				/* NEED TO CHECK ON THIS */
				status = check_btree(file, btree_addr, local_ndims, prev_entries);
			}

			break;

		case OBJ_GROUP_ID:
			status = check_btree(file, ((H5O_stab_t *)mesg)->btree_addr, ndims, prev_entries);
			if (status != SUCCEED) {
				error_push(ERR_LEV_2, ERR_LEV_2A18, 
				  "Errors found when checking B-tree from header message...", 
				  logical, NULL);
				if (!object_api()) {
					error_print(stderr, file);
					error_clear();
				}
				ret = SUCCEED;
			}
			status = check_lheap(file, ((H5O_stab_t *)mesg)->heap_addr, NULL);
			if (status != SUCCEED) {
				error_push(ERR_LEV_2, ERR_LEV_2A18, 
				  "Errors found when checking Local Heap from header message...", 
				  logical, NULL);
				if (!object_api()) {
			 		error_print(stderr, file);
					error_clear();
				}
				ret = SUCCEED;
			}
			break;
		
		default:
			printf("Done with decode/validate for a TO-BE-HANDLED message id=%d.\n", id);
			break;
			
	  }  /* end switch */
	}  /* end for */

	return(ret);
}



static unsigned
H5O_find_in_ohdr(driver_t *file, H5O_t *oh, const obj_class_t **type_p, int sequence)
{
	unsigned            u, FOUND;
    	const obj_class_t   *type = NULL;
    	unsigned            ret_value;
	const uint8_t	*start_buf;
	ck_addr_t	logi_base;

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
    	if (oh->mesg[u].flags & OBJ_FLAG_SHARED) {
        	type = OBJ_SHARED;
    	} else {
        	type = oh->mesg[u].type;
	}

	start_buf = oh->chunk[oh->mesg[u].chunkno].image;
	logi_base = oh->chunk[oh->mesg[u].chunkno].addr;

    	if (oh->mesg[u].native == NULL) {
        	assert(type->decode);
        	oh->mesg[u].native = (type->decode) (file, oh->mesg[u].raw, start_buf, logi_base);
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


static void *
H5O_shared_read(driver_t *file, OBJ_shared_t *obj_shared, const obj_class_t *type, int prev_entries)
{
    	int	ret = SUCCEED;
	H5O_t	*oh;
	int	idx, status;
	void	*ret_value = NULL;
	void	*mesg;


    	/* check args */
    	assert(obj_shared);
    	assert(type);

    	/* Get the shared message */
    	if (obj_shared->in_gh) {
		printf("The message pointed to by the shared message is in global heap..NOT HANDLED YET\n");
#if 0
        	void *tmp_buf, *tmp_mesg;
        if (NULL==(tmp_buf = H5HG_read (f, dxpl_id, &(obj_shared->u.gh), NULL)))
            HGOTO_ERROR (H5E_OHDR, H5E_CANTLOAD, NULL, "unable to read shared message from global heap");
        tmp_mesg = (type->decode)(file, dxpl_id, tmp_buf, shared_obj);
        tmp_buf = H5MM_xfree (tmp_buf);
        if (!tmp_mesg)
            HGOTO_ERROR (H5E_OHDR, H5E_CANTLOAD, NULL, "unable to decode object header shared message");

#endif
    	} else {
	  	ret = check_obj_header(file, obj_shared->u.ent.header, &oh, type, prev_entries);
/* What if ret from check_obj_header is error ?? */

		idx = H5O_find_in_ohdr(file, oh, &type, 0);
		if (oh->mesg[idx].flags & OBJ_FLAG_SHARED) {
			OBJ_shared_t *sh_shared;

        		sh_shared = (OBJ_shared_t *)(oh->mesg[idx].native);
/* NEED to check ret_val */
        		ret_value = H5O_shared_read(file, sh_shared, type, prev_entries);
		} else {
printf("I am returning the native\n");
			ret_value = oh->mesg[idx].native;
/*
			ret_value = (type->copy)(oh->mesg[idx].native, mesg);
*/
		}
    	}
	
	return(ret_value);
}




ck_err_t
check_obj_header(driver_t *file, ck_addr_t obj_head_addr, 
H5O_t **ret_oh, const obj_class_t *type, int prev_entries)
{
	size_t		hdr_size, chunk_size;
	ck_addr_t	chunk_addr;
	uint8_t		buf[16], *p, flags;
	uint8_t		*start_buf;
	unsigned	nmesgs, id;
	ck_addr_t	logical, logi_base;
	size_t		mesg_size;
	int		version, nlink, ret, status;

	H5O_t		*oh=NULL;
	unsigned    	chunkno;
	unsigned    	curmesg=0;
	int		mesgno, i, j;
	unsigned	saved;


       	OBJ_cont_t 	*cont;
	H5O_stab_t 	*stab;
	int		idx, badinfo;
	table_t		tb;

	global_shared_t	*lshared;


	assert(addr_defined(obj_head_addr));
	ret = SUCCEED;
	idx = table_search(obj_head_addr);
	if (idx >= 0) { /* FOUND the object */
		if (obj_table->objs[idx].nlink > 0) {
			obj_table->objs[idx].nlink--;
if (!(ret_oh) )
			return(ret);
		} else {
			error_push(ERR_LEV_2, ERR_LEV_2A, 
			  "Object Header:Inconsistent reference count", obj_head_addr, NULL);
			ret++;  /* SHOULD I return??? */
			return(ret);
		}
	}

	lshared = file->shared;
    	hdr_size = CK_SIZEOF_HDR(lshared);
    	assert(hdr_size<=sizeof(buf));

	if (debug_verbose())
		printf("VALIDATING the object header at logical address %llu...\n", obj_head_addr);

	printf("obj_head_addr=%llu, hdr_size=%d\n", 
		obj_head_addr, hdr_size);

	oh = calloc(1, sizeof(H5O_t));
	if (oh == NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Object Header:Unable to calloc() H5O_t", obj_head_addr, NULL);
		ret++;
		return(ret);
	}

	if (FD_read(file, obj_head_addr, (size_t)hdr_size, buf) == FAIL) {
		error_push(ERR_FILE, ERR_NONE_SEC, 
		  "Object Header:Unable to read object header", obj_head_addr, NULL);
		ret++;
		return(ret);
	}

	start_buf = buf;
    	p = buf;

	logical = get_logical_addr(p, start_buf, obj_head_addr);
	oh->version = *p++;
	if (oh->version != H5O_VERSION) {
		badinfo = oh->version;
		error_push(ERR_LEV_2, ERR_LEV_2A, 
		  "Object Header:version number should be 1", logical, &badinfo);
		ret++;
	}
	p++;

	logical = get_logical_addr(p, start_buf, obj_head_addr);
	UINT16DECODE(p, nmesgs);
	if ((int)nmesgs < 0) {  /* ??? */
		badinfo = nmesgs;
		error_push(ERR_LEV_2, ERR_LEV_2A, 
		  "Object Header:number of header messages should not be negative", logical, &badinfo);
		ret++;
	}

	UINT32DECODE(p, oh->nlink);

	table_insert(obj_head_addr, oh->nlink-1);
	

	chunk_addr = obj_head_addr + (size_t)hdr_size;
	UINT32DECODE(p, chunk_size);
	logical = get_logical_addr(p, start_buf, chunk_addr);

	oh->alloc_nmesgs = MAX(H5O_NMESGS, nmesgs);
	if ((oh->mesg = calloc(oh->alloc_nmesgs, sizeof(H5O_mesg_t)))==NULL) {
		error_push(ERR_INTERNAL, ERR_NONE_SEC, 
		  "Object Header:calloc() failed for oh->alloc_nmesgs", logical, NULL);
		ret++;
		return(ret);
	}

#if 0
	printf("oh->version =%d, nmesgs=%d, oh->nlink=%d\n", oh->version, nmesgs, oh->nlink);
	printf("chunk_addr=%llu, chunk_size=%ld\n", chunk_addr, chunk_size);
#endif

	/* read each chunk from disk */
    	while (addr_defined(chunk_addr)) {
		  /* increase chunk array size */
        	if (oh->nchunks >= oh->alloc_nchunks) {
            		unsigned na = oh->alloc_nchunks + H5O_NCHUNKS;
            		H5O_chunk_t *x = realloc(oh->chunk, (size_t)(na*sizeof(H5O_chunk_t)));
            		if (!x) {
				error_push(ERR_INTERNAL, ERR_NONE_SEC, 
				  "Object Header:Realloc() failed for H5O_chunk_t", logical, NULL);
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
			error_push(ERR_INTERNAL, ERR_NONE_SEC, 
			  "Object Header:calloc() failed for oh->chunk[].image", logical, NULL);
			ret++;
			return(ret);
		}

		if (FD_read(file, chunk_addr, chunk_size, oh->chunk[chunkno].image) == FAIL) {
			error_push(ERR_FILE, ERR_NONE_SEC, 
			  "Object Header:Unable to read object header data", logical, NULL);
			ret++;
			return(ret);
		}


		start_buf = oh->chunk[chunkno].image;
		for (p = oh->chunk[chunkno].image; p < oh->chunk[chunkno].image+chunk_size; p += mesg_size) {
			UINT16DECODE(p, id);
            		UINT16DECODE(p, mesg_size);
			assert(mesg_size==CK_ALIGN(mesg_size));
			flags = *p++;
			p += 3;  /* reserved */
			/* Try to detect invalidly formatted object header messages */
			logical = get_logical_addr(p, start_buf, chunk_addr);
            		if (p + mesg_size > oh->chunk[chunkno].image + chunk_size) {
				error_push(ERR_LEV_2, ERR_LEV_2A, 
				  "Object Header:Corrupt object header message", logical, NULL);
				ret++;
				return(ret);
			}

            		/* Skip header messages we don't know about */
            		if (id >= NELMTS(message_type_g) || NULL == message_type_g[id]) {
                		continue;
			}
			 /* new message */
                	if (oh->nmesgs >= nmesgs) {
				error_push(ERR_LEV_2, ERR_LEV_2A, 
				  "Object Header:Corrupt object header", logical, NULL);
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
        	for (chunk_addr = CK_ADDR_UNDEF; !addr_defined(chunk_addr) && curmesg < oh->nmesgs; ++curmesg) {
            		if (oh->mesg[curmesg].type->id == OBJ_CONT_ID) {
				start_buf = oh->chunk[oh->mesg[curmesg].chunkno].image;
				logi_base = oh->chunk[oh->mesg[curmesg].chunkno].addr;
                		cont = (OBJ_CONT->decode) (file, oh->mesg[curmesg].raw, start_buf, logi_base);
				logical = get_logical_addr(oh->mesg[curmesg].raw, start_buf, logi_base);
				if (cont == NULL) {
					error_push(ERR_LEV_2, ERR_LEV_2A, 
					  "Object Header:Corrupt continuation message...skipped", logical, NULL);
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

	status = SUCCEED;
	if (ret_oh)
		*ret_oh = oh;
	else
		status = decode_validate_messages(file, oh, prev_entries);
#if 0
	if (search) {
		idx = H5O_find_in_ohdr(file, oh, &type, 0);
		if (oh->mesg[idx].flags & OBJ_FLAG_SHARED) {
			OBJ_shared_t *sh_shared;
			void	*ret_value;

        		sh_shared = (OBJ_shared_t *)(oh->mesg[idx].native);
        		status = H5O_shared_read(file, sh_shared, type, prev_entries);
		}
	} else {
		status = decode_validate_messages(file, oh, prev_entries);
	}
#endif
	if (status != SUCCEED)
		ret++;

	return(ret);
}

void
print_version(const char *prog_name)
{
	fprintf(stdout, "%s: Version %s\n", prog_name, H5Check_VERSION);
}

void
usage(char *prog_name)
{
	fflush(stdout);
    	fprintf(stdout, "usage: %s [OPTIONS] file\n", prog_name);
    	fprintf(stdout, "  OPTIONS\n");
    	fprintf(stdout, "     -h,  --help   	Print a usage message and exit\n");
    	fprintf(stdout, "     -V,  --version	Print version number and exit\n");
    	fprintf(stdout, "     -vn, --verbose=n	Verboseness mode\n");
    	fprintf(stdout, "     		n=0	Terse--only indicates if the file is compliant or not\n");
    	fprintf(stdout, "     		n=1	Default--prints its progress and all errors found\n");
    	fprintf(stdout, "     		n=2	Verbose--prints everything it knows, usually for debugging\n");
    	fprintf(stdout, "     -oa, --object=a	Check object header\n");
    	fprintf(stdout, "     		a	Address of the object header to be validated\n");
	fprintf(stdout, "\n");

}

void
leave(int ret)
{
	exit(ret);
}

int
debug_verbose(void)
{
        return(g_verbose_num==DEBUG_VERBOSE);
}

int 
object_api(void)
{
	if (g_obj_api)
		g_obj_api_err++;
	return(g_obj_api);
}
