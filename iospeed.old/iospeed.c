
#include "iospeed.h"


#define READWRITE_TEST 0
#define WRITE_TEST  1
#define READ_TEST   2
#define threshhold 10 * MB
#define DEFAULT_FILE_NAME "./test_temp_file.dat"


char* initBuffer(size_t bsize)
{
    char *buffer;

    if (NULL==(buffer=malloc(bsize* sizeof(char))))
    {
        printf("\n Error unable to reserve memory\n\n");
        return NULL;
    }
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

double testPosixIO(int operation_type, size_t fsize, size_t bsize, char *fname)
{
    FILE* file;
    size_t written = 0,read_data = 0;    
    size_t op_size;    
    struct timeval timeval_start;    
    char* buffer;

    if ((buffer = initBuffer(bsize)) == NULL)
        return -1;
    if (operation_type != READ_TEST)
    {
        /*start timing*/
        gettimeofday(&timeval_start,NULL);
    }
    
    /* create file */
    if ((file=fopen(fname,"w"))== 0)
    {
        printf(" unable to create the file\n");
        return -1;
    }    
            
    while(written <= fsize)
    {
        if ((op_size = fwrite(buffer,sizeof(char), bsize,file)) == 0)
        {
            printf(" unable to write sufficent data to file \n");
            return -1;
        }            
        if (operation_type != READ_TEST)
        {
            if (( fsize/ reportTime(timeval_start)) < threshhold)
            {
                return -2;
            }
        }
        written += op_size;
        if (written < 0)
        {
            printf( "Error Overflow\n");
            return -1;
        }
    }
    if (fclose(file) < 0)
    {
        printf(" unable to close the file\n");
        return -1;
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
        return -1;
    }

    while(read_data <= fsize)
    {
        if ((op_size = fread(buffer,sizeof(char), bsize,file)) == 0)
        {
            printf(" unable to read sufficent data from file \n");
            return -1;
        }            
        if (( fsize/ reportTime(timeval_start)) < threshhold)
        {
            return -2;
        }
        read_data += op_size;        
        if (read_data < 0)
        {
            printf( "Error Overflow\n");
            return -1;
        }
    }
    fclose(file);
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
        return -1;

    /*start timing*/
    if (operation_type != READ_TEST)
    {
        gettimeofday(&timeval_start,NULL);
    }    
    /* create file*/
    if(type == 1)    
        flag = O_WRONLY | O_CREAT | O_DIRECT;
    else if (type == 2)
        flag = O_WRONLY | O_CREAT | O_NONBLOCK;
    else
        flag = O_WRONLY | O_CREAT;

    if ((file=open(fname,flag,S_IRWXU))== -1)
    {
        printf(" unable to create the file\n");
        return -1;
    }    
            
    while(written <= fsize)
    {
        if ((op_size = write(file, buffer,bsize)) < 0)
        {
            printf(" Error in writing data to file because %s \n", strerror(errno));
            return -1;
        }
        else if ((op_size = write(file, buffer,bsize)) == 0)
        {
            printf(" unable to write sufficent data to file because %s \n", strerror(errno));
            return -1;
        }        
        if(operation_type != READ_TEST)
        {
            if ((fsize/ reportTime(timeval_start)) < threshhold)
            {
                return -2;
            }
        }
        written += op_size;
        if (written < 0)
        {
            printf( "Error Overflow\n");
            return -1;
        }

    }
    if (close(file) < 0)
    {
        printf(" unable to close the file\n");
        return -1;
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
    if(type == 1)    
        flag = O_RDWR | O_DIRECT;
    else if (type == 2)
        flag = O_RDWR | O_NONBLOCK;
    else
        flag = O_RDWR;

    if ((file=open(fname,flag))== -1)
    {
        printf(" unable to open the file because %s \n", strerror(errno));
        return -1;
    }    

    while(read_data <= fsize)
    {
        if ((op_size = read(file, buffer, bsize)) <= 0)
        {
            printf(" unable to read sufficent data from file \n");
            return -1;
        }    
        if (( fsize/ reportTime(timeval_start)) < threshhold)
        {
            return -2;
        }        

        read_data += op_size;        
        if (read_data < 0)
        {
            printf( "Error Overflow\n");
            return -1;
        }
    }
    close(file);
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
        return -1;
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
        return -1;
    }    
            
    while(written <= fsize)
    {
        if ((op_size = ffwrite(file, buffer, bsize)) <= 0)
        {          
            printf("\n unable to write sufficent data to file \n\n");
            return -1;
        }
        if (operation_type != READ_TEST)
        {
            if ((fsize/ reportTime(timeval_start)) < threshhold)
            {
                return -2;
            }
        }
        written += op_size;        
        if (written < 0)
        {
            printf( "Error Overflow\n");
            return -1;
        }
    }
    if (ffclose(file) < 0 )
    {
        printf(" unable to close the file\n");
        return -1;
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
        return -1;
    }    

    while(read_data <= fsize)
    {
        if ((op_size = ffread(file,buffer, bsize)) <= 0)
        {
            printf(" unable to read sufficent data from file\n");
            return -1;
        }            
        if ((fsize/ reportTime(timeval_start)) < threshhold)
        {
            return -2;
        }
        read_data += op_size;        
        if (read_data < 0)
        {
            printf( "Error Overflow\n");
            return -1;
        }
    }
    ffclose(file);
    return reportTime(timeval_start);

}
#endif

void printInfo()
{
    printf("Usage: \n");
    printf("[-m <mode>] [-w] [-r] [-b <bsize>] [-s <fsize>] [-f <fname>]\n");
    printf("-m          I/O API <mode> to test.  1 for Unix I/O, 2 for Posix I/O, 3 for Unix with O_DIRECT, 4 for FFIO\n");
    printf("            , and 5 for Unix I/O with O_NONBlOCK\n");
    printf("-r          read only\n");
    printf("-w          write only (default to do both read and write)\n");
    printf("-b <bsize>  data transfer block size (default 1KB=1024B)\n");
    printf("-s <fsize>  total file size in MB=1024*1024B (default 128)\n");
    printf("-f <fname>  temporary data file name (default current directory)\n");
    printf("help        prints this message\n");
}

/*-------------------------------------------------------------------------
 * Function:    wmain
 * 
 * Purpose:     Tests the basic different modes of I/O
 * 
 * Return:      Success:        exit(0)
 *              
 *              Failure:        exit(1)
 * 
 * Programmer:  Arash Termehchy 
 *              3/11/06
 * 
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
wmain (int argc, char **argv)
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
    block_size = 1 * KB;
    file_size = 128 * MB;       
    if ((validOptions=malloc(6*sizeof(char*))) == NULL)
        return -1;
    for(i = 0; i < 6; i++)
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
   if ((!isValid(options,validOptions,1)) && (oldargc > 1))
    {
        printInfo();
        return -1;
    }

    /*mode*/
    goptreturn=gopt(options,'m',0,&ptr);    
    switch(goptreturn){
        case GOPT_NOTPRESENT:
            mode = 1;
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
            mode = 1;
            break;
        case GOPT_PRESENT_WITHARG:
            if (*ptr =='1')
            {
                mode = 1;
                if (header)
                {
                    printf(" Mode=Unix I/O ");      
                }
            }
            else if (*ptr =='2')
            {
                mode = 2;
                if (header)
                {
                    printf(" Mode=Posix I/O ");      
                }
            }
            else if (*ptr == '3')
            {
                mode = 3;
                if (header)
                {
                    printf(" Mode=Direct Unix I/O  ");      
                }
            }
#ifdef FFIO_MACHINE
            else if (*ptr == '4')
            {                
                mode = 4;
                if (header)
                {
                    printf(" Mode=FFIO ");      
                }

            }
#endif
            else if (*ptr == '5')
            {
                mode = 5;
                if (header)
                {
                    printf(" Mode=Non-Blocking Unix I/O ");      
                }
            }
            else
            {
                printf("error\n");
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
    printf("%lu, ",file_size);

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
    printf("%lu, ",block_size);
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

    /*run*/    
 
  switch(mode){
    case 1:
      elapsed_time = testUnixIO(input_operation,0,file_size, block_size, filename);
      break;
    case 2:      
      elapsed_time = testPosixIO(input_operation,file_size, block_size, filename);
      break;
    case 3:
      elapsed_time = testUnixIO(input_operation,1,file_size, block_size, filename);
      break;
#ifdef FFIO_MACHINE
    case 4:
      elapsed_time = testFFIO(input_operation,file_size, block_size, filename);
      break;
#endif
    case 5:
      elapsed_time = testUnixIO(input_operation,2,file_size, block_size, filename);
      break;
    default:
        printInfo();
        return 1;
        break;
  }
   
  if (elapsed_time  == -1)
      printf("Error\n");
  else if (elapsed_time  == -2)
      printf("Too Long\n");
  else
  {
      printf(" %f,", elapsed_time);      
      printf(" %f \n", file_size / ( elapsed_time * MB));
  }

  free(options);
  remove(filename);
  if (elapsed_time < 0)
      return -1;
  return 0;
}

/*-------------------------------------------------------------------------
 * Function:    main
 * 
 * Purpose:     a wrapper to the wmain function
 * 
 * Return:      Success:        exit(0)
 *              
 *              Failure:        exit(1)
 * 
 * Programmer:  Arash Termehchy 
 *              3/11/06
 * 
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
main (int argc, char **argv)
{  
        
    return wmain(argc,argv);
    
/*    
    int fSizeCount = 0;
    int bSizeCount = 0;
    int mode=0, count = 0, i = 0, unfinished = 0;
    int wr = WRITE_TEST;
    
    char command[11][255];

    if (argc > 2)
    {
        return wmain(argc,argv);
    }
    else if(argc <= 1)
    {
        printf ("Error, enter parameter\n");
        exit(1);
    }
        
    sprintf(command[count],"iospeed");        
    count++;
    for(mode = 1; mode <= 4 ; mode++)
    {                
        if (mode == 3)
            continue;
        sprintf(command[count],"-m");        
        count++;
        sprintf(command[count],"%d",mode);
        count++;        
        for(wr = WRITE_TEST; wr <= READ_TEST; wr++)
        {                    
            header = 1;
            if (wr == WRITE_TEST)
            {
                sprintf(command[count],"-w");
                count++;

            }
            else if (wr == READ_TEST)
            {
                sprintf(command[count],"-r");
                count++;
            }
            for(bSizeCount = 1; bSizeCount <= 5; bSizeCount)
            {
                sprintf(command[count],"-b");
                count++;
                if (bSizeCount <= 2)
                {
                    sprintf(command[count],"%d",bSizeCount);                        
                    count++;
                }
                else if (bSizeCount < 5)
                {
                    sprintf(command[count],"%d",(bSizeCount - 2) * KB);                        
                    count++;
                }
                else
                {
                    sprintf(command[count],"%d",(bSizeCount - 1) * KB);
                    count++;
                }
                for(fSizeCount = 128; fSizeCount <= 8192; fSizeCount*= 2)
                {
                    sprintf(command[count],"-s");            
                    count++;
                    sprintf(command[count],"%d",fSizeCount);            
                    count++;
                    sprintf(command[count],"-f");   
                    count++;
                    sprintf(command[count],"%s",argv[1]);   
                    count++;
                    for (i = 0; i < count; i++)
                        printf("%s\n",command[i]);

                    if (wmain(count,command) < 0)
                    {
                        unfinished = 1;
                        header = 0;
                        break;
                    }
                    header = 0;
                }
            }
        }
    }*/

}


