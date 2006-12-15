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

/*-------------------------------------------------------------------------
 * Function:    H5E_push
 *
 * Purpose:     Pushes a new error record onto error stack.
 *              The name of a function where the error was detected
 *              and an error description string. 
 *              The function name and error description strings must
 *              be statically allocated (the FUNC_ENTER() macro takes care of
 *              the function name but the
 *              programmer is responsible for the description string.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, December 12, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5E_push(const char *function_name, const char *desc, haddr_t address)
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



/*-------------------------------------------------------------------------
 * Function:    H5E_clear
 *
 * Purpose:     Clears the error stack for the current thread.
 *		Free the memory for function and description holders
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, February 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_clear(void)
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
H5E_print(FILE *stream)
{
	int	i;
	H5E_t   *estack = H5E_get_my_stack ();

	if (!stream) 
		stream = stderr; 
	fprintf(stream, "***Error***\n");
	nerrors++;
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

int
H5E_FoundError(void)
{
    return(nerrors!=0);
}
