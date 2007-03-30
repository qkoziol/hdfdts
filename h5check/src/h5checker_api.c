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
#include "h5_error.h"


/*
 * for handling the API via file descriptors 
 */
int	g_fd_inuse = 0;
#define NUM_FD  2       /* can open 5 files simultaneously for checking */
typedef struct stored_fd {
        driver_t        *file;
        global_shared_t *shared;
} stored_fd;

stored_fd fd_table[NUM_FD];

static 	ck_err_t        h5checker_init(char *); 
static	void            h5checker_close(int); 



ck_err_t
h5checker_init(char *fname)
{
	driver_t 	*thefile;
	int		ret, i;
	ck_addr_t	ss;
	global_shared_t	*shared;

	thefile = NULL;
	shared = calloc(1, sizeof(global_shared_t));	
	thefile = (driver_t *)FD_open(fname, shared, SEC2_DRIVER);

	if (thefile == NULL) {
                error_push(ERR_FILE, ERR_NONE_SEC,
                  "Failure in opening input file using the default driver. Validation discontinued.", -1, NULL);
                goto done;
        }

        ret = check_superblock(thefile, shared);
        /* superblock validation has to be all passed before proceeding further */
        if (ret != SUCCEED) {
                error_push(ERR_LEV_0, ERR_NONE_SEC,
                  "Errors found when checking superblock. Validation stopped.", -1, NULL);
		thefile = NULL;
                goto done;
        }

        /* not using the default driver */
        if (shared->driverid != SEC2_DRIVER) {
                ret = FD_close(thefile);
                if (ret != SUCCEED) {
                        error_push(ERR_FILE, ERR_NONE_SEC,
                          "Errors in closing input file using the default driver", -1, NULL);
			thefile = NULL;
                	goto done;
                }
                printf("Switching to new file driver...\n");
                thefile = (driver_t *)FD_open(fname, shared, shared->driverid);
                if (thefile == NULL) {
                        error_push(ERR_FILE, ERR_NONE_SEC,
                          "Errors in opening input file. Validation stopped.", -1, NULL);
                        goto done;
                }
        }

	ss = FD_get_eof(thefile);
        if ((ss==CK_ADDR_UNDEF) || (ss<shared->stored_eoa)) {
                error_push(ERR_FILE, ERR_NONE_SEC,
                  "Invalid file size or file size less than superblock eoa. Validation stopped.", 
                  -1, NULL);
		thefile = NULL;
                goto done;
        }

	if (g_fd_inuse == NUM_FD) {
                error_push(ERR_FILE, ERR_NONE_SEC,
                  "Exceed limit of allowed file descriptors. Validation stopped.", 
                  -1, NULL);
		thefile = NULL;
                goto done;
	}
	for (i = 0; i < NUM_FD; i++) {
		if (fd_table[i].file == NULL) {
			g_fd_inuse++;
			break;
		}
	}
	
	fd_table[i].file = thefile;
	fd_table[i].shared = shared;
	return(i);
done:
	return(-1);
}

ck_err_t
h5checker_obj(char *fname, ck_addr_t obj_addr, ck_errmsg_t *errbuf)
{
	ck_err_t	ret, ret_fd, status;
	int             prev_entries = -1;

	g_obj_api = TRUE;
	g_obj_api_err = 0;
	g_obj_addr = obj_addr;
	printf("\nVALIDATING %s:", fname);
        if (g_obj_addr == CK_ADDR_UNDEF)
		printf("The whole file is validated\n");
	else
                printf("Only validated the specified object header %llu\n", g_obj_addr);

	ret_fd = h5checker_init(fname);
	if (ret_fd < 0) {
		g_obj_api_err++;
		goto done;
	}

	if ((g_obj_addr != CK_ADDR_UNDEF) && (g_obj_addr >= fd_table[ret_fd].shared->stored_eoa)) {
                error_push(ERR_FILE, ERR_NONE_SEC,
                  "Invalid Object header address provided. Validation stopped.",
                  -1, NULL);
		g_obj_api_err++;
                goto done;
        }

	ret = table_init(&obj_table);
        if (ret != SUCCEED) {
                error_push(ERR_INTERNAL, ERR_NONE_SEC,
                  "Errors in initializing hard link table", -1, NULL);
		g_obj_api_err++;
        }

	/* check the whole file if g_obj_addr is undefined */
	if (g_obj_addr == CK_ADDR_UNDEF) {
		g_obj_addr = fd_table[ret_fd].shared->root_grp->header;
	}

	status = check_obj_header(fd_table[ret_fd].file, fd_table[ret_fd].shared, g_obj_addr, NULL, NULL, prev_entries);
	if (status != SUCCEED) {
                error_push(ERR_LEV_0, ERR_NONE_SEC,
                  "Errors found when checking the object header", g_obj_addr, NULL);
		g_obj_api_err++;
        }
done:
	if (ret_fd >= 0)
		h5checker_close(ret_fd);
	if ((errbuf != NULL) && (g_obj_api_err))
		process_errors(errbuf);
	return(g_obj_api_err?-1:0);
}

void
h5checker_close(int fd)
{
	ck_err_t	ret;

	ret = FD_close(fd_table[fd].file);
	if (ret != SUCCEED) {
                error_push(ERR_FILE, ERR_NONE_SEC,
                	"Errors in closing the input file", -1, NULL);
		g_obj_api_err++;
	}
	if (fd_table[fd].shared) {
		free(fd_table[fd].shared);
		fd_table[fd].shared = NULL;
	}
	g_fd_inuse--;
}
