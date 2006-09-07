
#include "iospeed.h"

#define READWRITE_TEST 0
#define WRITE_TEST  1
#define READ_TEST   2
#define iospeed_min (1 * MB)
#define DEFAULT_FILE_NAME "./test_temp_file.dat"

/* memory page size needed for the Direct IO option. */
size_t mem_page_size;

char* initBuffer(size_t bsize)
{
    char *buffer, *p;
    int  ret;
    size_t i;

    /* mem_page_size is set in main. */
    if (posix_memalign(&buffer, mem_page_size, bsize) != 0) {
        printf("\n Error unable to reserve memory\n\n");
        return NULL;
    }

    /*Initialize the buffer*/
    p = buffer;
    for(i=0; i<bsize; i++, p++)
	*p = (char)i;

    return buffer;
}

double reportTime(struct timeval start)
{
    struct timeval timeval_stop,timeval_diff;

    /*end timing*/
    gettimeofday(&timeval_stop,NULL);
    /* Calculate the elapsed gettimeofday time */
    timeval_diff.tv_usec=timeval_stop.tv_usec-start.tv_usec;
    timeval_diff.tv_sec=timeval_stop.tv_sec-start.tv_sec;
    if(timeval_diff.tv_usec<0) {
        timeval_diff.tv_usec+=1000000;
        timeval_diff.tv_sec--;
    } /* end if */
    return (double)timeval_diff.tv_sec+((double)timeval_diff.tv_usec/(double)1000000.0);        
}

/* print sizes in human readable format */
void showiosize(char* prefix, size_t value, char* suffix)
{
    printf("%s%lu", prefix, value);
    if (value > KB){
	if (value < MB)
	    /* Use KB range */
	    printf("(%gKB)",value/(double)KB);
	else if (value < GB)
	    /* Use MB range */
	    printf("(%gMB)",value/(double)MB);
	else
	    /* Use GB range */
	    printf("(%gGB)",value/(double)GB);
    }

    printf("%s", suffix);
    return;
}

double testPosixIO(int operation_type, size_t fsize, size_t bsize, char *fname)
{
    FILE* file;
    size_t written = 0,read_data = 0;    
    size_t op_size;    
    struct timeval timeval_start;    
    char* buffer;

    if ((buffer = initBuffer(bsize)) == NULL)
        return IOERR;
    if (operation_type != READ_TEST)
    {
        /*start timing*/
        gettimeofday(&timeval_start,NULL);
    }
    
    /* create file */
    if ((file=fopen(fname,"w"))== 0)
    {
        printf(" unable to create the file\n");
        return IOERR;
    }    
            
    while(written < fsize)
    {
        if ((op_size = fwrite(buffer,sizeof(char), bsize,file)) == 0)
        {
            printf(" unable to write sufficent data to file \n");
            return IOERR;
        }            
        if (operation_type != READ_TEST)
        {
            if (( fsize/ reportTime(timeval_start)) < iospeed_min)
            {
		/* IO speed too slow. Abort. */
                return IOSLOW;
            }
        }
        written += op_size;
#if 0
	/* check is not needed any more? */
        if (written < 0)
        {
	    /* integer overflow detected. */
            printf( "Error: Integer overflow during write\n");
            return IOERR;
        }
#endif
    }
#ifndef NDEBUG
	showiosize("Data written=", written, "\n");
#endif

    if (fclose(file) < 0)
    {
        printf(" unable to close the file\n");
        return IOERR;
    }    
    
    if (operation_type == WRITE_TEST)
    {
        return reportTime(timeval_start);
    }
    else if (operation_type == READ_TEST)
    {
        /*start timing*/
        gettimeofday(&timeval_start,NULL);
    } 
    /* create file no matter what mode */
    if ((file=fopen(fname,"r"))== 0)
    {
        printf(" unable to open the file\n");
        return IOERR;
    }

    while(read_data < fsize)
    {
        if ((op_size = fread(buffer,sizeof(char), bsize,file)) == 0)
        {
            printf(" unable to read sufficent data from file \n");
            return IOERR;
        }            
        if (( fsize/ reportTime(timeval_start)) < iospeed_min)
        {
	    /* IO speed too slow. Abort. */
            return IOSLOW;
        }
        read_data += op_size;        
#if 0
	/* check is not needed any more? */
        if (read_data < 0)
        {
	    /* integer overflow detected. */
            printf( "Error: Integer overflow during read\n");
            return IOERR;
        }
#endif
    }
#ifndef NDEBUG
	showiosize("Data read=", read_data, "\n");
#endif

    fclose(file);
    free(buffer);

    return reportTime(timeval_start);
}


double testUnixIO(int operation_type, int type, size_t fsize, size_t bsize, char *fname)
{
    int file, flag;
    size_t written = 0,read_data = 0;    
    ssize_t op_size;    
    struct timeval timeval_start,timeval_stop;
    struct timeval timeval_diff;
    char* buffer;

    if ((buffer = initBuffer(bsize)) == NULL)
        return IOERR;

    /*start timing*/
    if (operation_type != READ_TEST)
    {
        gettimeofday(&timeval_start,NULL);
    }    
    /* create file*/
    if(type == UNIX_DIRECTIO)    
        flag = O_CREAT|O_TRUNC|O_WRONLY|O_DIRECT;
    else if (type == UNIX_NONBLOCK)
        flag = O_CREAT|O_TRUNC|O_WRONLY|O_NONBLOCK;
    else
        flag = O_CREAT|O_TRUNC|O_WRONLY;

    if ((file=open(fname,flag,S_IRWXU))== -1)
    {
        printf(" unable to create the file\n");
        return IOERR;
    }    
            
    while(written < fsize)
    {
        op_size = write(file, buffer,bsize);
        if (op_size < 0)
        {
            printf(" Error in writing data to file because %s \n", strerror(errno));
            return IOERR;
        }
        else if (op_size == 0)
        {
            printf(" unable to write sufficent data to file because %s \n", strerror(errno));
            return IOERR;
        }
        if(operation_type != READ_TEST)
        {
            if ((fsize/ reportTime(timeval_start)) < iospeed_min)
            {
		/* IO speed too slow. Abort. */
                return IOSLOW;
            }
        }
        written += op_size;
#if 0
	/* check is not needed any more? */
        if (written < 0)
        {
	    /* integer overflow detected. */
            printf( "Error: Integer overflow during write\n");
            return IOERR;
        }
#endif
    }
#ifndef NDEBUG
	showiosize("Data written=", written, "\n");
#endif

    if (close(file) < 0)
    {
        printf(" unable to close the file\n");
        return IOERR;
    }
    
    if (operation_type == WRITE_TEST)
    {
        return reportTime(timeval_start);    
    }
    else if (operation_type == READ_TEST)
    {
        /*start timing*/
        gettimeofday(&timeval_start,NULL);
    }
 
    if(type == UNIX_DIRECTIO)    
        flag = O_RDONLY | O_DIRECT;
    else if (type == UNIX_NONBLOCK)
        flag = O_RDONLY | O_NONBLOCK;
    else
        flag = O_RDONLY;

    if ((file=open(fname,flag))== -1)
    {
        printf(" unable to open the file because %s \n", strerror(errno));
        return IOERR;
    }    

    while(read_data < fsize)
    {
        if ((op_size = read(file, buffer, bsize)) <= 0)
        {
            printf(" unable to read sufficent data from file \n");
            return IOERR;
        }    
        if (( fsize/ reportTime(timeval_start)) < iospeed_min)
        {
	    /* IO speed too slow. Abort. */
            return IOSLOW;
        }        
        read_data += op_size;        
#if 0
	/* check is not needed any more? */
        if (read_data < 0)
        {
	    /* integer overflow detected. */
            printf( "Error: Integer overflow during read\n");
            return IOERR;
        }
#endif
    }
#ifndef NDEBUG
	showiosize("Data read=", read_data, "\n");
#endif

    close(file);
    free(buffer);

    return reportTime(timeval_start);
}

#ifdef FFIO_MACHINE
double testFFIO(int operation_type, size_t fsize, size_t bsize, char *fname)
{    
    int file, flag;
    size_t written = 0,read_data = 0;    
    ssize_t op_size;    
    struct timeval timeval_start,timeval_stop;
    struct timeval timeval_diff;
    char* buffer;

    if ((buffer = initBuffer(bsize)) == NULL)
        return IOERR;
    if (operation_type != READ_TEST)
    {
        /*start timing*/
        gettimeofday(&timeval_start,NULL);
    }
    
    /* create file*/    
    flag = O_WRONLY | O_CREAT;    
    if ((file=ffopen(fname,flag))== -1)
    {
        printf(" unable to create the file\n");
        return IOERR;
    }    
            
    while(written < fsize)
    {
        if ((op_size = ffwrite(file, buffer, bsize)) <= 0)
        {          
            printf("\n unable to write sufficent data to file \n\n");
            return IOERR;
        }
        if (operation_type != READ_TEST)
        {
            iospeed_min ((fsize/ reportTime(timeval_start)) < iospeed_min)
            {
		/* IO speed too slow. Abort. */
                return IOSLOW;
            }
        }
        written += op_size;        
#if 0
	/* check is not needed any more? */
        if (written < 0)
        {
	    /* integer overflow detected. */
            printf( "Error: Integer overflow during write\n");
            return IOERR;
        }
#endif
    }
#ifndef NDEBUG
	showiosize("Data written=", written, "\n");
#endif

    if (ffclose(file) < 0 )
    {
        printf(" unable to close the file\n");
        return IOERR;
    }
    
    if (operation_type == WRITE_TEST)
    {                               
        return reportTime(timeval_start);    
    }
    else if (operation_type == READ_TEST)
    {       
        /*start timing*/
        gettimeofday(&timeval_start,NULL);
    } 
    flag = O_RDONLY ;

    if ((file=ffopen(fname,flag))== -1)
    {
        printf(" unable to open the file\n");
        return IOERR;
    }    

    while(read_data < fsize)
    {
        if ((op_size = ffread(file,buffer, bsize)) <= 0)
        {
            printf(" unable to read sufficent data from file\n");
            return IOERR;
        }            
        if ((fsize/ reportTime(timeval_start)) < iospeed_min)
        {
	    /* IO speed too slow. Abort. */
            return IOSLOW;
        }
        read_data += op_size;        
#if 0
	/* check is not needed any more? */
        if (read_data < 0)
        {
	    /* integer overflow detected. */
            printf( "Error: Integer overflow during read\n");
            return IOERR;
        }
#endif
    }
#ifndef NDEBUG
	showiosize("Data read=", read_data, "\n");
#endif

    ffclose(file);
    free(buffer);

    return reportTime(timeval_start);
}
#endif

void printInfo()
{
    printf(
	"Usage: iospeed [-m <mode>] [-w] [-r] [-b <bsize>] [-s <fsize>] [-f <fname>]\n"
	"    -m          I/O API <mode> to test.\n"
        "                1 for Unix I/O\n"
        "                2 for Posix I/O\n"
        "                3 for Unix I/O with O_DIRECT\n"
#ifdef FFIO_MACHINE
        "                4 for FFIO\n"
#endif
        "                5 for Unix I/O with O_NONBlOCK\n"
        "    -r          read only\n"
        "    -w          write only (default to do both read and write)\n"
        "    -b <bsize>  data transfer block size (default 1KB=1024B)\n"
        "    -s <fsize>  total file size in MB=1024*1024B (default 128)\n"
        "    -f <fname>  temporary data file name (default current directory)\n"
        "    help        prints this message\n"
    );
}

/*-------------------------------------------------------------------------
 * Function:    main
 * 
 * Purpose:     Tests the basic different modes of I/O
 * 
 * Return:      Success:        exit(0)
 *              
 *              Failure:        exit(1)
 * 
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
main (int argc, char **argv)
{  
    char shortopt;
    unsigned int i, mode = 1;
    int input_operation=READWRITE_TEST;
    gopt_t goptreturn;
    void *options;
    const char *ptr;
    double elapsed_time = 0;
    char** validOptions;
    int oldargc = argc;
    int header = 1;
    size_t block_size;
    size_t file_size;
    char filename[150];

    /*init*/    
    /* memory page size needed for the Direct IO option. */
    mem_page_size = getpagesize();
    block_size = mem_page_size;		/* default to memory page size */
    file_size = 128 * MB;       
    if ((validOptions=malloc(NUMOPTIONS*sizeof(char*))) == NULL)
        return -1;
    for(i = 0; i < NUMOPTIONS; i++)
    {
        if ((validOptions[i]=malloc(1*sizeof(char))) == NULL)
            return -1;
    }

    validOptions[0][0]='m';
    validOptions[1][0]='w';
    validOptions[2][0]='r';
    validOptions[3][0]='b';
    validOptions[4][0]='s';
    validOptions[5][0]='f';

    /* information mode*/
    if ((argc >=2) &&  (strcmp(argv[1],"help") == 0))
    {
        printInfo();
        return 0;     
    }
    options=gopts("mbsf",&argc,&argv);    
    if(options==NULL)
    {
        printInfo();
        return -1;
    }
    /* Checking*/
   if ((!isValid(options,(const char**)validOptions,NUMOPTIONS)) && (oldargc > 1))
    {
        printInfo();
        return -1;
    }

    /*mode*/
    goptreturn=gopt(options,'m',0,&ptr);    
    switch(goptreturn){
        case GOPT_NOTPRESENT:
            mode = UNIX_BASIC;
            if (header)
            {
                printf(" Mode=Unix I/O ");      
            }
            break;
        case GOPT_PRESENT_NOARG:
            if (header)
            {
                printf(" Mode=Unix I/O ");      
            }
            mode = UNIX_BASIC;
            break;
        case GOPT_PRESENT_WITHARG:
            if (*ptr =='1')
            {
                mode = UNIX_BASIC;
                if (header)
                {
                    printf(" Mode=Unix I/O ");      
                }
            }
            else if (*ptr =='2')
            {
                mode = Posix_IO;
                if (header)
                {
                    printf(" Mode=Posix I/O ");      
                }
            }
            else if (*ptr == '3')
            {
                mode = UNIX_DIRECTIO;
                if (header)
                {
                    printf(" Mode=Direct Unix I/O  ");      
                }
            }
#ifdef FFIO_MACHINE
            else if (*ptr == '4')
            {                
                mode = FFIO;
                if (header)
                {
                    printf(" Mode=FFIO ");      
                }

            }
#endif
            else if (*ptr == '5')
            {
                mode = UNIX_NONBLOCK;
                if (header)
                {
                    printf(" Mode=Non-Blocking Unix I/O ");      
                }
            }
            else
            {
                printf("Error: Unknown mode(%c)\n", *ptr);
                printInfo();
                return 1;
            }
                
            break;
    }
    
    /*read, write */
    goptreturn=gopt(options,'r',0,&ptr);       
    printf(", Operation:");
    switch(goptreturn){
    case GOPT_NOTPRESENT:      
      break;
    case GOPT_PRESENT_NOARG:
        input_operation = READ_TEST;        
      break;
    case GOPT_PRESENT_WITHARG:
      printInfo();
      return 1;
      break;
    }

    goptreturn=gopt(options,'w',0,&ptr);    
    switch(goptreturn){
    case GOPT_NOTPRESENT:      
      break;
    case GOPT_PRESENT_NOARG:            
            if (input_operation == READ_TEST)
            {
                input_operation = READWRITE_TEST;                
            }
            else
            {
                input_operation = WRITE_TEST;
            }
            break;                
    case GOPT_PRESENT_WITHARG:
      printInfo();
      return 1;
      break;
    }
    if (header)
    {
        switch(input_operation)
        {
            case READ_TEST:
                printf("Read \n");      
                break;
            case WRITE_TEST:
                printf ("Write \n");
                break;
            case READWRITE_TEST:
                printf("Read Write \n");
                break;
            default:
                printInfo();
                return 1;
                break;
        }
    }
            
    /*file size*/
    goptreturn=gopt(options,'s',0,&ptr);    
    switch(goptreturn){
    case GOPT_NOTPRESENT:      
      break;
    case GOPT_PRESENT_NOARG:
        printInfo();
        return 1;
        break;
    case GOPT_PRESENT_WITHARG:
        {            
            sscanf(ptr, "%lu",&file_size);                                   
            file_size = file_size * MB;
            break;
        }
    }
    showiosize("file-size=", file_size, ", ");

    /* block size*/
    goptreturn=gopt(options,'b',0,&ptr);    
    switch(goptreturn){
    case GOPT_NOTPRESENT:      
      break;
    case GOPT_PRESENT_NOARG:
        printInfo();
        break;
    case GOPT_PRESENT_WITHARG:
        {
            sscanf(ptr, "%li",&block_size);                        
            break;
        }
    }
    showiosize("block-size=", block_size, ", ");

    /*file name*/
    goptreturn=gopt(options,'f',0,&ptr);    
    switch(goptreturn){
    case GOPT_NOTPRESENT:      
        strcpy(filename,DEFAULT_FILE_NAME);
        break;
    case GOPT_PRESENT_NOARG:
        strcpy(filename,DEFAULT_FILE_NAME);
        break;
    case GOPT_PRESENT_WITHARG:
      strcpy(filename,ptr);
      break;
    }    
    printf("filename=%s\n",filename);

    /*run*/    
 
  switch(mode){
    case UNIX_BASIC:
      elapsed_time = testUnixIO(input_operation,UNIX_BASIC,file_size, block_size, filename);
      break;
    case Posix_IO:      
      elapsed_time = testPosixIO(input_operation,file_size, block_size, filename);
      break;
    case UNIX_DIRECTIO:
      elapsed_time = testUnixIO(input_operation,UNIX_DIRECTIO,file_size, block_size, filename);
      break;
#ifdef FFIO_MACHINE
    case FFIO:
      elapsed_time = testFFIO(input_operation,file_size, block_size, filename);
      break;
#endif
    case UNIX_NONBLOCK:
      elapsed_time = testUnixIO(input_operation,UNIX_NONBLOCK,file_size, block_size, filename);
      break;
    default:
        printInfo();
        return 1;
        break;
  }
   
  if (elapsed_time  == IOERR)
      printf("IO Error encountered.  Test aborted.\n");
  else if (elapsed_time  == IOSLOW)
      printf("IO Speed too slow (<%dMB/s). Test aborted.\n", iospeed_min/MB);
  else
  {
      printf("elapsed time=%fs,", elapsed_time);      
      printf(" IO speed=%f(MB/s) \n", file_size / ( elapsed_time * MB));
  }

  free(options);
  remove(filename);
  if (elapsed_time < 0)
      return -1;
  return 0;
}

