/**********************************************************************\
*                                                                      *
*  Title:  Gopt                                                        *
*  Version: 3.0                                                        *
*  Date:  2004-11-29                                                   *
*  Author: Tom Vajzovic                                                *
*  Copyright:  Tom Vajzovic                                            *
*  URL:  http://gopt.sourceforge.net/                                  *
*  Contact:  tom_v (at) users.sourceforge.net                          *
*  Description: A GNU compliant command line argument parsing library  *
*                                                                      *
*  You may redistribute and modify this free software under the terms  *
*  of version 2 of the GNU General Public License as published by the  *
*  Free Software Foundation.                                           *
*                                                                      *
*  (NOT the Library/Lesser GPL, but the full GPL)                      *
*                                                                      *
*  You should have received a copy of the GNU GPL along with this      *
*  program, if not, write to the Free Software Foundation Inc,         *
*  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA, or see      *
*  http://www.gnu.org/licenses/                                        *
*                                                                      *
*  You may only redistribute a modified version or derivative of this  *
*  software under a different name.                                    *
*                                                                      *
*  Additionally, the author requests (but does not require) to be      *
*  notified, and that a comment containing the above URL be included.  *
*                                                                      *
*  This program is distributed in the hope that it will be useful, but *
*  without any warranty, without even the implied warranty of          *
*  merchantability or fitness for a particular purpose.  See the GNU   *
*  GPL for more details.                                               *
*                                                                      *
\**********************************************************************/

typedef enum {         /*    gopt() return values:               */
  GOPT_NOTPRESENT=0,   /* option was not present                 */
  GOPT_PRESENT_NOARG,  /* option was present without an argument */
  GOPT_PRESENT_WITHARG /* option was present with an argument    */
} gopt_t;

/* use gopts("ab",&argc,&argv)                                              */
/* where -a and -b are the short options which can take option arguments,   */
/* or gopts(NULL,&argc,&argv) if there are none                             */
/* argc and argv are as in main(int argc, char *argv[])                     */
/* returns a void pointer to an area of memory which must be free()d when   */
/* you have finished reading options                                        */
/* the void* must be passed as the first argument each time you call gopt   */
/* does nothing and returns NULL if out of memory                           */
/* if GOPT_PASSMALLOC is defined, use gopts("ab",&argc,&argv,&malloc)       */

#ifdef GOPT_PASSMALLOC
void *gopts(const char *shortoptswithargs, int* const mainargc, char** const*const mainargv, void *whichmalloc(size_t));
#else
void *gopts(const char *shortoptswithargs, int* const mainargc, char** const*const mainargv);
#endif

/* use gopt(opts,'o',"option",&ptr) where opts was returned by gopts                                           */
/* tests for the presence on the command line of short option -o or long option --option                       */
/* loads ptr with a pointer to the option argument if there was one                                            */
/* does not alter argval and returns GOPT_NOTPRESENT the option was not specified in either form               */
/* does not alter argval and returns GOPT_PRESENT_NOARG if it was specified without an argument                */
/* sets argval and returns GOPT_PRESENT_WITHARG if it was specified with an argument                           */
/* use gopt(opts,0,"option",&ptr) if there is no short option or gopt(opts,'o',NULL,&ptr) for no long          */
/* use gopt(opts,'o',"option",NULL) if you do not want the option argument, or you know that there was not one */

gopt_t gopt(void* const vptr2opts, const char shortname, const char *longname, const char** const argval);

/* If the input options are valid                                                                               */
int isValid(void* const vptr2opts,const char** validnames, int validNameSize);
