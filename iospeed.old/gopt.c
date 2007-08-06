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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum {         /*    gopt() return values:               */
  GOPT_NOTPRESENT=0,   /* option was not present                 */
  GOPT_PRESENT_NOARG,  /* option was present without an argument */
  GOPT_PRESENT_WITHARG /* option was present with an argument    */
} gopt_t;

typedef struct {           /*     a command line option          */

  enum {                   /*  kind of option we have:           */
    OPTKIND_SHORT_NOARG,   /* short option without argument      */
    OPTKIND_SHORT_WITHARG, /* short option with argument         */
    OPTKIND_LONG_NOARG,    /* long option without argument       */
    OPTKIND_LONG_WITHARG,  /* long option with argument          */
    OPTKIND_NOTHING        /* marker for end of options          */
  } kind;

  char *name;             /*  name of the option:                */
                          /* pointer to a char for short options */
                          /* or a string for long ones           */

  char *val;            /* pointer to the argument of the option */
                        /* if it has one (else undefined)        */
} opt;

enum sorts_e {                   /*   sorts[] values (internal to gopts)                  */

  SORT_COMMAND,                  /* first argument is the command                         */
  SORT_SINGLE_SHORT,             /* single short option without argument                  */
  SORT_SINGLE_SHORT_WITHARG,     /* single short option without space before its argument */
  SORT_SINGLE_SHORT_ARGTOFOLLOW, /* single short option with space before its argument    */
  SORT_SHORTS_ARG,               /* option argument for prevoius option                   */
  SORT_LONG,                     /* long option without argument                          */
  SORT_LONG_ARG,                 /* long option with argument                             */
  SORT_MULTI_SHORT,              /* multiple short options                                */
  SORT_DOUBLEDASH,               /* double dash                                           */
  SORT_ARG                       /* non-otion argument                                    */
};

typedef enum {
  FALSE=0,
  TRUE=1
} bool;


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
void *gopts(const char *shortoptswithargs, int* const mainargc, char** const*const mainargv, void *whichmalloc(size_t))
#else
void *gopts(const char *shortoptswithargs, int* const mainargc, char** const*const mainargv)
#endif
{

  bool expectingoptarg=FALSE;
  enum sorts_e *sorts;
  unsigned int i,numopts=0;
  char *eq;
  opt *opts;

#ifdef DEBUG
  fprintf(stderr,"Testing function arguments.\n");
#endif
  if(NULL==mainargc||NULL==mainargv){
#ifdef DEBUG
    fprintf(stderr,"Passed bad function arguments.\n");
#endif
    return NULL;
  } /* if(NULL==shortoptswithargs||NULL==mainargc||NULL==mainargv) */

  if(NULL==shortoptswithargs){
#ifdef DEBUG
    fprintf(stderr,"shortoptswithargs is NULL, setting to the empty string.\n");
#endif
    shortoptswithargs=(char*)&"";
  }

#ifdef DEBUG
  fprintf(stderr,"Allocating memory for sorting.\n");
#endif

#ifdef GOPT_PASSMALLOC
  sorts=(enum sorts_e*)(*whichmalloc)(*mainargc*sizeof(enum sorts_e));
#else
  sorts=(enum sorts_e*)malloc(*mainargc*sizeof(enum sorts_e));
#endif

  if(NULL==sorts){
#ifdef DEBUG
    fprintf(stderr,"Not enough memory for sorting.\n");
#endif
    return NULL;
  } /* if(NULL==sorts) */

#ifdef DEBUG
  fprintf(stderr,"The first argument (%s) is the command.\n",(*mainargv)[0]);
#endif
  sorts[0]=SORT_COMMAND;

  for(i=1;i<*mainargc;++i){
#ifdef DEBUG
    fprintf(stderr,"Looking at argument %u (%s).\n",i,(*mainargv)[i]);
#endif

    if('-'==(*mainargv)[i][0]){
#ifdef DEBUG
      fprintf(stderr,"Argument starts with a dash.\n");
#endif
      expectingoptarg=FALSE;

      if('-'==(*mainargv)[i][1]){
#ifdef DEBUG
        fprintf(stderr,"Argument starts with two dashes.\n");
#endif

        if(0==(*mainargv)[i][2]){
#ifdef DEBUG
          fprintf(stderr,"Argument is a double dash.\n");
#endif
          sorts[i]=SORT_DOUBLEDASH;
#ifdef DEBUG
          fprintf(stderr,"All remaining arguments are not options:\n");
#endif
          for(++i;i<*mainargc;++i){
#ifdef DEBUG
            fprintf(stderr,"Argument %u (%s) is a non-option argument.\n",i,(*mainargv)[i]);
#endif
            sorts[i]=SORT_ARG;
          }      /* for(++i;i<*mainargc;++i) */
          break; /* for(i=1;i<*mainargc;++i) */

        }else{   /* if(0==(*mainargv)[i][2]) */
#ifdef DEBUG
          fprintf(stderr,"Argument is a long option");
#endif

          if(NULL!=strchr((*mainargv)[i],'=')){
#ifdef DEBUG
            fprintf(stderr," with an argument.\n");
#endif
            sorts[i]=SORT_LONG_ARG;
            ++numopts;
          }else{ /* if(NULL!=strchr((*mainargv)[i],'=')) */
#ifdef DEBUG
            fprintf(stderr," without an argument.\n");
#endif
            sorts[i]=SORT_LONG;
            ++numopts;
          } /* if(NULL!=strchr((*mainargv)[i],'=')) else */
        }   /* if(0==(*mainargv)[i][2]) else             */

      }else{ /* if('-'==(*mainargv)[i][1]) */
#ifdef DEBUG
        fprintf(stderr,"Argument starts with only one dash.\n");
#endif

        if(0==(*mainargv)[i][1]){
#ifdef DEBUG
          fprintf(stderr,"Argument is a single dash non-option argument.\n");
#endif
          sorts[i]=SORT_ARG;

        }else{ /* if(0==(*mainargv)[i][1]) */
          if(0==(*mainargv)[i][2]){
#ifdef DEBUG
            fprintf(stderr,"Argument is a single short option.\n");
#endif
            sorts[i]=SORT_SINGLE_SHORT;
            ++numopts;

            if(NULL!=strchr(shortoptswithargs,(*mainargv)[i][1])){
#ifdef DEBUG
              fprintf(stderr,"...which may be followed by its argument.\n");
#endif
              expectingoptarg=TRUE;
            }    /* if(NULL!=strchr(shortoptswithargs,(*mainargv)[i][1])) */

          }else{ /* if(0==(*mainargv)[i][2]) */
            if(NULL!=strchr(shortoptswithargs,(*mainargv)[i][1])){
#ifdef DEBUG
              fprintf(stderr,"Argument is a single short option, together with its argument.\n");
#endif
              sorts[i]=SORT_SINGLE_SHORT_WITHARG;
              ++numopts;

            }else{ /* if(NULL!=strchr(shortoptswithargs,(*mainargv)[i][1])) */
#ifdef DEBUG
              fprintf(stderr,"Argument is multiple short options:\n");
#endif
              sorts[i]=SORT_MULTI_SHORT;
              for(eq=(*mainargv)[i]+1;0!=*eq;++eq){
#ifdef DEBUG
                fprintf(stderr,"another.\n");
#endif
                ++numopts;
              }
            }  /* if(NULL!=strchr(shortoptswithargs,(*mainargv)[i][1])) else */
          }    /* if(0==(*mainargv)[i][2]) else                              */
        }      /* if(0==(*mainargv)[i][1]) else                              */
      }        /* if('-'==(*mainargv)[i][1]) else                            */

    }else{     /* if('-'==(*mainargv)[i][0])                                 */

#ifdef DEBUG
      fprintf(stderr,"Argument doesn't start with a dash.\n");
#endif

      if(expectingoptarg){
#ifdef DEBUG
        fprintf(stderr,"Argument is the argument to the previous option.\n");
#endif
        expectingoptarg=FALSE;
        sorts[i]=SORT_SHORTS_ARG;
        sorts[i-1]=SORT_SINGLE_SHORT_ARGTOFOLLOW;

      }else{ /* if(expectingoptarg) */
#ifdef DEBUG
        fprintf(stderr,"Argument is a non-otion argument.\n");
#endif
        sorts[i]=SORT_ARG;
      }  /* if(expectingoptarg) else            */
    }    /*  /* if('-'==(*mainargv)[i][0]) else */
  }      /* for(i=1;i<*mainargc;++i)            */




#ifdef DEBUG
  fprintf(stderr,"\nTo sumarise I have %d arguments:\n",*mainargc);
  for(i=0;i<*mainargc;++i)
    switch(sorts[i]){
      case SORT_COMMAND:
        fprintf(stderr,"Argument %u (%s) is the command.\n",i,(*mainargv)[i]);
        continue;
      case SORT_SINGLE_SHORT:
        fprintf(stderr,"Argument %u (%s) is a single short option with no argument.\n",i,(*mainargv)[i]);
        continue;
      case SORT_SINGLE_SHORT_WITHARG:
        fprintf(stderr,"Argument %u (%s) is a single short option with it's argument.\n",i,(*mainargv)[i]);
        continue;
      case SORT_SINGLE_SHORT_ARGTOFOLLOW:
        fprintf(stderr,"Argument %u (%s) is a single short option with it's argument to follow.\n",i,(*mainargv)[i]);
        continue;
      case SORT_SHORTS_ARG:
        fprintf(stderr,"Argument %u (%s) is the argument to the previous option.\n",i,(*mainargv)[i]);
        continue;
      case SORT_LONG:
        fprintf(stderr,"Argument %u (%s) is a long option with no argument.\n",i,(*mainargv)[i]);
        continue;
      case SORT_LONG_ARG:
        fprintf(stderr,"Argument %u (%s) is a long option with an argument.\n",i,(*mainargv)[i]);
        continue;
      case SORT_MULTI_SHORT:
        fprintf(stderr,"Argument %u (%s) is several short options.\n",i,(*mainargv)[i]);
        continue;
      case SORT_DOUBLEDASH:
        fprintf(stderr,"Argument %u (%s) is a double dash.\n",i,(*mainargv)[i]);
        continue;
      case SORT_ARG:
        fprintf(stderr,"Argument %u (%s) is a non-option argument.\n",i,(*mainargv)[i]);
        continue;
      default:
        fprintf(stderr,"Something has gone horribly wrong.\n");
    }
  fprintf(stderr,"There are %u options in total.\n\n",numopts);
#endif




#ifdef DEBUG
  fprintf(stderr,"Allocating memory for option details.\n");
#endif

#ifdef GOPT_PASSMALLOC
  opts=(opt*)(*whichmalloc)((1+numopts)*sizeof(opt));
#else
  opts=(opt*)malloc((1+numopts)*sizeof(opt));
#endif

  if(NULL==opts){
#ifdef DEBUG
    fprintf(stderr,"Not enough memory for option details.\nFreeing memory for sorting.\n");
#endif
    free(sorts);
    return NULL;
  }

  numopts=0;
  for(i=1;i<*mainargc;++i){
#ifdef DEBUG
    fprintf(stderr,"Adding argument %u (%s)\n",i,(*mainargv)[i],numopts,*((*mainargv)[i]+1));
#endif

    switch(sorts[i]){
      case SORT_SINGLE_SHORT:
#ifdef DEBUG
        fprintf(stderr,"...as short option %u (%c) with no argument.\n",i,(*mainargv)[i],numopts,*((*mainargv)[i]+1));
#endif
        opts[numopts].kind=OPTKIND_SHORT_NOARG;
        opts[numopts].name=(*mainargv)[i]+1;
        ++numopts;
        continue;

      case SORT_SINGLE_SHORT_WITHARG:
#ifdef DEBUG
        fprintf(stderr,"...as short option %u (%c) with argument '%s'.\n",i,(*mainargv)[i],numopts,*((*mainargv)[i]+1),(*mainargv)[i]+2);
#endif
        opts[numopts].kind=OPTKIND_SHORT_WITHARG;
        opts[numopts].name=(*mainargv)[i]+1;
        opts[numopts].val=(*mainargv)[i]+2;
        ++numopts;
        continue;

      case SORT_SINGLE_SHORT_ARGTOFOLLOW:
#ifdef DEBUG
        fprintf(stderr,"...as short option %u (%c) with argument '%s'.\n",i,(*mainargv)[i],numopts,*((*mainargv)[i]+1),(*mainargv)[i+1]);
#endif
        opts[numopts].kind=OPTKIND_SHORT_WITHARG;
        opts[numopts].name=(*mainargv)[i]+1;
        opts[numopts].val=(*mainargv)[i+1];
        ++numopts;
        continue;

      case SORT_LONG:
#ifdef DEBUG
        fprintf(stderr,"...as long option %u (%s) with no argument.\n",i,(*mainargv)[i],numopts,(*mainargv)[i]+2);
#endif
        opts[numopts].kind=OPTKIND_LONG_NOARG;
        opts[numopts].name=(*mainargv)[i]+2;
        ++numopts;
        continue;

      case SORT_LONG_ARG:
        opts[numopts].kind=OPTKIND_LONG_WITHARG;
        opts[numopts].name=(*mainargv)[i]+2;
#ifdef DEBUG
        fprintf(stderr,"...as long option %u with an argument, looking for '='...\n",i);
#endif
        eq=strchr(opts[numopts].name,'=');  /* it must find it, because we already looked for it once */
        *eq=0;
        opts[numopts].val=eq+1;
#ifdef DEBUG
        fprintf(stderr,"... option is %s argument '%s'.\n",opts[numopts].name,opts[numopts].val);
#endif
        ++numopts;
        continue;

      case SORT_MULTI_SHORT:
#ifdef DEBUG
        fprintf(stderr,"...as multiple short options without arguments:\n",i,(*mainargv)[i]);
#endif
        for(eq=(*mainargv)[i]+1;0!=*eq;++eq){
#ifdef DEBUG
          fprintf(stderr,"...option %u is %c.\n",numopts,*eq);
#endif
          opts[numopts].kind=OPTKIND_SHORT_NOARG;
          opts[numopts].name=eq;
          ++numopts;
        } /* for(eq=(*mainargv)[i]+1;*eq!=0;++eq) */
    }     /* switch(sorts[i])                     */
  } /* for(i=1;i<*mainargc;++i) */

#ifdef DEBUG
  fprintf(stderr,"There is no option %u.\n\n",numopts);
#endif
  opts[numopts].kind=OPTKIND_NOTHING;

  numopts=1;
  for(i=1;i<*mainargc;++i){
    if(SORT_ARG==sorts[i]){
#ifdef DEBUG
      fprintf(stderr,"Argument %u (%s) is a non-option argument - moving it to %u in argv.\n",i,(*mainargv)[i],numopts);
#endif
      (*mainargv)[numopts]=(*mainargv)[i];
      ++numopts;
    } /* if(SORT_ARG==sorts[i]) */
#ifdef DEBUG
    else{
      fprintf(stderr,"Argument %u (%s) is part of an option or a double dash.\nIt will be removed from argv.\n",i,(*mainargv)[i]);
    }  /* if(SORT_ARG==sorts[i]) else */
#endif
  } /* for(i=1;i<*mainargc;++i) */

#ifdef DEBUG
  fprintf(stderr,"\nFreeing sorting memory.\n");
#endif
  free(sorts);

#ifdef DEBUG
  fprintf(stderr,"Adding NULL pointer as argv[%u].\n",numopts);
#endif
  (*mainargv)[numopts]=NULL;

#ifdef DEBUG
  fprintf(stderr,"Correcting argc to %u.\n\n",numopts);
#endif
  *mainargc=numopts;
#ifdef DEBUG
    fprintf(stderr,"gopts() returning.\n");
#endif
  return (void*)opts;

}






/* use gopt(opts,'o',"option",&ptr) where opts was returned by gopts                                           */
/* tests for the presence on the command line of short option -o or long option --option                       */
/* loads ptr with a pointer to the option argument if there was one                                            */
/* does not alter argval and returns GOPT_NOTPRESENT the option was not specified in either form               */
/* does not alter argval and returns GOPT_PRESENT_NOARG if it was specified without an argument                */
/* sets argval and returns GOPT_PRESENT_WITHARG if it was specified with an argument                           */
/* use gopt(opts,0,"option",&ptr) if there is no short option or gopt(opts,'o',NULL,&ptr) for no long          */
/* use gopt(opts,'o',"option",NULL) if you do not want the option argument, or you know that there was not one */

gopt_t gopt(void* const vptr2opts, const char shortname, const char *longname, const char** const argval){
  unsigned int i;
  opt *opts;

  if(NULL==longname){
#ifdef DEBUG
    fprintf(stderr,"Long option is NULL, setting to the empty string.\n");
#endif
    longname=(char*)&"";
  }

  if(NULL==vptr2opts||(!*longname&&!shortname)){
#ifdef DEBUG
    fprintf(stderr,"This function was called by an idiot.\n");
#endif
    return GOPT_NOTPRESENT;
  }

#ifdef DEBUG
  if(shortname){
    if(*longname)
      fprintf(stderr,"Looking for long option %s or short option %c.\n",longname,shortname);
    else
      fprintf(stderr,"Looking for short option %c.\n",shortname);
  }else
    fprintf(stderr,"Looking for long option %s.\n",longname);
#endif

  opts=(opt*)vptr2opts;

  for(i=0;;++i){
#ifdef DEBUG
    fprintf(stderr,"Testing option %u.\n",i);
#endif


    switch(opts[i].kind){


      case OPTKIND_SHORT_NOARG:
#ifdef DEBUG
        fprintf(stderr,"...it's a short option with no argument.\n");
#endif
        if(*opts[i].name==shortname){
#ifdef DEBUG
          fprintf(stderr,"Match found, returning with no argument.\n");
#endif
          return GOPT_PRESENT_NOARG;
        }
#ifdef DEBUG
        fprintf(stderr,"...it is %c, no match.\n",*opts[i].name);
#endif
        continue;


      case OPTKIND_SHORT_WITHARG:
#ifdef DEBUG
        fprintf(stderr,"...it's a short option with an argument.\n");
#endif
        if(*opts[i].name==shortname){
#ifdef DEBUG
          fprintf(stderr,"Match found...\n");
#endif
          if(NULL!=argval){
#ifdef DEBUG
            fprintf(stderr,"...setting argument pointer...\n");
#endif
            *argval=opts[i].val;
          }
#ifdef DEBUG
          else{
            fprintf(stderr,"...argument pointer is NULL...\n");
          }
          fprintf(stderr,"...returning.\n");
#endif
          return GOPT_PRESENT_WITHARG;
        }
#ifdef DEBUG
        fprintf(stderr,"...it is %c, no match.\n",*opts[i].name);
#endif
        continue;


      case OPTKIND_LONG_NOARG:
#ifdef DEBUG
        fprintf(stderr,"...it's a long option with no argument.\n");
#endif
        if(!strcmp(opts[i].name,longname)){
#ifdef DEBUG
          fprintf(stderr,"Match found, returning with no argument.\n");
#endif
          return GOPT_PRESENT_NOARG;
        }
#ifdef DEBUG
        fprintf(stderr,"...it is %s, no match.\n",opts[i].name);
#endif
        continue;


      case OPTKIND_LONG_WITHARG:
#ifdef DEBUG
        fprintf(stderr,"...it's a long option with an argument.\n");
#endif
        if(!strcmp(opts[i].name,longname)){
#ifdef DEBUG
          fprintf(stderr,"Match found...\n");
#endif
          if(NULL!=argval){
#ifdef DEBUG
            fprintf(stderr,"...setting argument pointer...\n");
#endif
            *argval=opts[i].val;
          }
#ifdef DEBUG
          else{
            fprintf(stderr,"...argument pointer is NULL...\n");
          }
          fprintf(stderr,"...returning.\n");
#endif
          return GOPT_PRESENT_WITHARG;
        }
#ifdef DEBUG
        fprintf(stderr,"...it is %s, no match.\n",opts[i].name);
#endif
        continue;


      default:
#ifdef DEBUG
        fprintf(stderr,"End of options reached, returning not found.\n");
#endif
        return GOPT_NOTPRESENT;
    } /* switch(opts[i].kind) */
  }   /* for(i=0;;++i)        */
}     /* gopt_t gopt(...)     */

/****************************************************************************************************************/
/* If the input options are valid                                                                               */
int isValid(void* const vptr2opts,const char** validnames, int validNameSize)
{
    unsigned int i;
    unsigned int j;
    int ret_Value = 1;
    opt *opts;
 
    if ((NULL == validnames) && (vptr2opts == NULL))
    {
        return 1;
    }
    if (vptr2opts == NULL)
    {
        return 0;
    }
    opts=(opt*)vptr2opts;
    if (validnames == NULL)    
       if (opts[0].kind == OPTKIND_NOTHING)
       {
           return 1;
       }
       else
       {
           return 0;   
       }
    j = 0;   
    while((opts[j].kind != NULL) && (opts[j].name != NULL) && (opts[j].kind != OPTKIND_NOTHING))
    {
        i=0;
        while((strcmp(opts[j].name,validnames[i])!=0) && (i < validNameSize)) 
        {
            i++;       
        }
        if (i >= validNameSize)
        {
            return 0;
        }
        j++;
    }/* while()        */     
    if (j==0)
    {
        return 0; 
    }
    return 1;
}
