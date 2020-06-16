// Changes can be made in this file if the program is to support 
// other memory specification 

// Definitions for values needed to extract page number and offset
#define FRAME_SIZE 256        
#define NUMBER_OF_FRAMES 256  
#define ADDRESS_MASK 65535  
#define OFFSET_MASK 255
#define TLB_SIZE 16      
#define PAGE_TABLE_SIZE 256  
// Definitions for reading from a file
#define BUFFER_SIZE 10
#define CHUNK 256
// Arrays for page tables and TLB
int pageTableNumbers[PAGE_TABLE_SIZE];  
int pageTableFrames[PAGE_TABLE_SIZE];   
int TLBPageNumber[TLB_SIZE];  
int TLBFrameNumber[TLB_SIZE];
// 2D array for physical memory 
int physicalMemory[NUMBER_OF_FRAMES][FRAME_SIZE];
