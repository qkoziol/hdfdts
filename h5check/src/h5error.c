#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "h5_check.h"
#include "h5_error.h"

static	int	nerrors=0;	/* number of errors found */
static  const char *get_prim_err(primary_err_t);
static  const char *get_sec_err(secondary_err_t);

/* error handling */
static  const   primary_err_mesg_t primary_err_mesg_g[] = {
        {ERR_NONE_PRIM, "none"},
        {ERR_LEV_0,     "Disk Format Level 0-File Metadata"},
        {ERR_LEV_1,     "Disk Format Level 1-File Infrastructure"},
        {ERR_LEV_2,     "Disk Format Level 2-Data Objects"},
        {ERR_FILE,  	"File Handling"},
        {ERR_INTERNAL, 	"Internal Error"},
};

static  const   secondary_err_mesg_t secondary_err_mesg_g[] = {
        {ERR_NONE_SEC, "none"},
        {ERR_LEV_0A,   "0A-File Signature and Super Block"},
        {ERR_LEV_0B,   "0B-File Driver Info"},
        {ERR_LEV_1A,    "1A-B-link Trees and B-tree Nodes"},
        {ERR_LEV_1B,    "1B-Group"},
        {ERR_LEV_1C,    "1C-Group Entry"},
        {ERR_LEV_1D,    "1D-Local Heaps"},
        {ERR_LEV_1E,    "1E-Global Heap"},
        {ERR_LEV_1F,    "1F-Free-space Index"},
        {ERR_LEV_2A,   "2A-Data Object Headers"},
        {ERR_LEV_2A1,  "2A1-Header Message: NIL"},
        {ERR_LEV_2A2,  "2A2-Header Message: Simple Dataspace"},
        {ERR_LEV_2A3,  "2A3-Header Message: Reserved-not assigned yet"},
        {ERR_LEV_2A4,  "2A4-Header Message: Datatype"},
        {ERR_LEV_2A5,  "2A5-Header Message: Data Storage-Fill Value(Old)"},
        {ERR_LEV_2A6,  "2A6-Header Message: Data Storage-Fill Value"},
        {ERR_LEV_2A7,  "2A7-Header Message: Reserved-not assigned yet"},
        {ERR_LEV_2A8,  "2A8-Header Message: Data Storage-External Data Files"},
        {ERR_LEV_2A9,  "2A9-Header Message: Layout"},
        {ERR_LEV_2A10, "2A10-Header Message: Reserved-not assigned yet"},
        {ERR_LEV_2A11, "2A11-Header Message: Reserved-not assigned yet"},
        {ERR_LEV_2A12, "2A12-Header Message: Data Storage-Filter Pipeline"},
        {ERR_LEV_2A13, "2A13-Header Message: Attribute"},
        {ERR_LEV_2A14, "2A14-Header Message: Object Comment"},
        {ERR_LEV_2A15, "2A15-Header Message: Object Modification Data & Time(Old)"},
        {ERR_LEV_2A16, "2A16-Header Message: Shared Object Message"},
        {ERR_LEV_2A17, "2A17-Header Message: Object Header Continuation"},
        {ERR_LEV_2A18, "2A18-Header Message: Group Message"},
        {ERR_LEV_2A19, "2A19-Header Message: Object Modification Date & Time"},
        {ERR_LEV_2B,    "2B-Shared Data Object Headers"},
        {ERR_LEV_2C,    "2C-Data Object Data Storage"},
};


void
error_push(primary_err_t prim_err, secondary_err_t sec_err, const char *desc, ck_addr_t logical_addr, int *badinfo)
{
	
    	ERR_t   *estack = ERR_get_my_stack();
	char	*new_func;
	char 	*new_desc;
	int	desc_len, func_len;

    	/*
     	 * Don't fail if arguments are bad.  Instead, substitute some default
       	 * value.
     	 */
	if (!desc) 
		desc = "No description given";

   	/*
     	 * Push the error if there's room.  Otherwise just forget it.
     	 */
    	assert (estack);
    	if (estack->nused < H5E_NSLOTS) {
        	estack->slot[estack->nused].prim_err = prim_err;
        	estack->slot[estack->nused].sec_err = sec_err;
        	estack->slot[estack->nused].desc = desc;
        	estack->slot[estack->nused].logical_addr = logical_addr;
		if (badinfo != NULL) {
        		estack->slot[estack->nused].err_info.reported = REPORTED;
        		estack->slot[estack->nused].err_info.badinfo = *badinfo;
		} else {
        		estack->slot[estack->nused].err_info.reported = NOT_REP;
        		estack->slot[estack->nused].err_info.badinfo = -1;
		}
        	estack->nused++;
    	}
}



ck_err_t
error_clear(void)
{
	int	i;
    	ERR_t   *estack = ERR_get_my_stack();

	for (i = estack->nused-1; i >=0; --i) {
/* NEED TO CHECK INTO THIS LATER */
#if 0
		free((void *)estack->slot[i].func_name);
		free((void *)estack->slot[i].desc);
#endif
	}
    	if (estack) 
		estack->nused = 0;
	return(0);
}

void
error_print(FILE *stream, driver_t *file, global_shared_t *shared)
{
	int	i;
	const char	*prim_str = NULL;
	const char	*sec_str = NULL;
	ERR_t   *estack = ERR_get_my_stack();
	char		*fname;

	if (!stream) 
		stream = stderr; 
	nerrors++;
	if (g_verbose_num) {
		fprintf(stream, "***Error***\n");
		for (i = estack->nused-1; i >=0; --i) {
			int sec_null = 0;
			fprintf(stream, "%s", estack->slot[i].desc);
			if ((int)estack->slot[i].logical_addr != -1) {
				fname = FD_get_fname(file, shared, estack->slot[i].logical_addr);
				fprintf(stream, "\n  file=%s;", fname);
				fprintf(stream, " at logical addr %llu",
			     		(unsigned long long)estack->slot[i].logical_addr);
				if (estack->slot[i].err_info.reported)
					fprintf(stream, "; Value decoded: %d",
			     			estack->slot[i].err_info.badinfo);
			}
			fprintf(stream, "\n");

			prim_str = get_prim_err(estack->slot[i].prim_err);
			sec_str = get_sec_err(estack->slot[i].sec_err);
			if (estack->slot[i].sec_err == ERR_NONE_SEC)
				sec_null = 1;
			if (sec_null)
				fprintf(stream, "  %s\n", prim_str);
			else
				fprintf(stream, "  %s-->%s\n", prim_str, sec_str);
		}
		fprintf(stream, "***End of Error messages***\n");
	}

}

const char *
get_prim_err(primary_err_t n)
{
	unsigned    i;
    	const char *ret_value="Invalid primary error number";


    	for (i=0; i < NELMTS(primary_err_mesg_g); i++)
        	if (primary_err_mesg_g[i].err_code == n)
            		return(primary_err_mesg_g[i].str);

    	return(ret_value);
}

const char *
get_sec_err(secondary_err_t n)
{
	unsigned    i;
    	const char *ret_value="Invalid secondary error number";


    	for (i=0; i < NELMTS(secondary_err_mesg_g); i++)
        	if (secondary_err_mesg_g[i].err_code == n)
            		return(secondary_err_mesg_g[i].str);

    	return(ret_value);
}

int
found_error(void)
{
    return(nerrors!=0);
}

void
process_errors(ck_errmsg_t *errbuf)
{
	int 	i;
	ERR_t   *estack = ERR_get_my_stack();

	errbuf->nused = estack->nused;
	for (i = estack->nused-1; i >=0; --i) {
		errbuf->slot[i].desc = strdup(estack->slot[i].desc);
		errbuf->slot[i].addr = estack->slot[i].logical_addr;
	}
}
