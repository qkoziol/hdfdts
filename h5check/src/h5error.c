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

void
error_push(const char *function_name, const char *desc, haddr_t address)
{
	
    	H5E_t   *estack = H5E_get_my_stack ();
	char	*new_func;
	char 	*new_desc;
	int	desc_len, func_len;

    	/*
     	 * Don't fail if arguments are bad.  Instead, substitute some default
       	 * value.
     	 */
    	if (!function_name) 
		function_name = "Unknown_Function";
	if (!desc) 
		desc = "No description given";

	new_desc = (unsigned char *)malloc((size_t) (strlen(desc)+1) * sizeof(char));
	new_func = (unsigned char *)malloc((size_t) (strlen(function_name)+1) * sizeof(char));
	strcpy(new_desc, desc);
	strcpy(new_func, function_name);
#ifdef PRINT
	printf("new_desc=%s\n", new_desc);
	printf("new_func=%s\n", new_func);
#endif
   	/*
     	 * Push the error if there's room.  Otherwise just forget it.
     	 */
    	assert (estack);
    	if (estack->nused < H5E_NSLOTS) {
        	estack->slot[estack->nused].func_name = new_func;
        	estack->slot[estack->nused].desc = new_desc;
        	estack->slot[estack->nused].address = address;
        	estack->nused++;
    	}
}



herr_t
error_clear(void)
{
	int	i;
    	H5E_t   *estack = H5E_get_my_stack ();

	for (i = estack->nused-1; i >=0; --i) {
		free((void *)estack->slot[i].func_name);
		free((void *)estack->slot[i].desc);
	}
    	if (estack) 
		estack->nused = 0;
	return(0);
}

void
error_print(FILE *stream)
{
	int	i;
	H5E_t   *estack = H5E_get_my_stack ();

	if (!stream) 
		stream = stderr; 
	nerrors++;
	if (g_verbose_num) {
		fprintf(stream, "***Error***\n");
		for (i = estack->nused-1; i >=0; --i) {
			if ((int)estack->slot[i].address == -1)
    		     	fprintf(stream, "%s(): %s\n", 
			     estack->slot[i].func_name, 
			     estack->slot[i].desc);
    			else fprintf(stream, "%s(at %llu): %s\n", 
			     estack->slot[i].func_name, 
			     estack->slot[i].address,
			     estack->slot[i].desc);
		}
		fprintf(stream, "***End of Error messages***\n");

	}

}

int
found_error(void)
{
    return(nerrors!=0);
}

