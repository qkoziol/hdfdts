/**
 *
 * iokernel.h   
 *
 * Private headers for the HUGS I/O kernel program.
 *
 * Maikael A. Thomas \n
 * Sandia National Laboratories \n
 * Albuquerque, NM 87185-0708
 *
*/

/* Defines */
#define IOKERNEL_OKAY 0

#define MAXLINE 180
#define MAXSTREAMS 33

/* Convert bytes to megabytes for benchmarking */
#define MBpB (1.0 / (1024.0 * 1024.0))

#define MB 1024*1024


/* Metadata structures.  The first three should be 256 bytes, while the last 2048 bytes.
   The first metadata structure will store information */
typedef struct {

	int datastream;
	int rows;
	int cols;
	int num_contiguous;
	char pad[240];

} Metadata_Info;

typedef struct {

	char pad[256];

} Metadata_256;

typedef struct {

	char pad[2048];

} Metadata_2048;

typedef struct {
        char pad1[256];
        char pad2[256];
        char pad3[2048];
}Metadata_all;

