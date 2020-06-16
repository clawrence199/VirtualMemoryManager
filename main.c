/* Resources
 * built on code from https://github.com/malcium/vm
 * https://www.geeksforgeeks.org/translation-lookaside-buffer-tlb-in-paging/
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include"virtualMemoryConfig.h"

/* This program is a virtual memory manager
 * It will read a file with 32 bit logical addresses and use a bit mask
 * to extract the page number and offset.  If the page is a hit in the TLB
 * it will use the frame number from the TLB or else it will consult 
 * the page table.  If there is a page fault, the page will be 
 * read in from BACKING_STORE.bin
 */
 
// Counters to print 
int pageFaults = 0;   
int TLBHits = 0;     
// Counters for available frames, table entries, and TLB entries
int currentFrame = 0;  
int currentPageTableNumber = 0;  
int numberOfTLBEntries = 0;             
// File structs
FILE    *address_file;
FILE    *backing_store;

char    address[BUFFER_SIZE];
int     logical_address;
signed char     buffer[CHUNK];// the buffer reading from the backing store
signed char     value;// the value in memory

// headers for the functions used below
void getPage(int address);
void readFromStore(int pageNumber);
void insertIntoTLB(int pageNumber, int frameNumber);

// function to take the logical address and obtain the physical address and value stored at that address
/* This function takes the logical address 
 */
void getPage(int logical_address){
    
    int pageNumber = ((logical_address & ADDRESS_MASK)>>8);
    int offset = (logical_address & OFFSET_MASK);
    // Frame number stays at -1 if there is a TLB miss
    int frameNumber = -1; 

    // Search the TLB
    for(int i = 0; i < TLB_SIZE; i++){
        if(TLBPageNumber[i] == pageNumber){   
            frameNumber = TLBFrameNumber[i];  
            TLBHits++;                
        }
    }
    
    /* The following code executes upon a TLB miss. 
     * First the contents of the page table array are searched.
     * The frame number is set if the page number was found in the page table.
     * If the for loop makes it through the occupied cells of the array without
     * finding the page number, then a page fault occurs
     */
    if(frameNumber == -1){  
        for(int i = 0; i < currentPageTableNumber; i++){
            if(pageTableNumbers[i] == pageNumber){         
                frameNumber = pageTableFrames[i];         
            }
        }
        // At this point if frame number is still -1, a page fault has occured
        if(frameNumber == -1){    
            // The page number will be retrieved from the backing store                
            readFromStore(pageNumber);             
            pageFaults++;                          
            frameNumber = currentFrame - 1;  
        }
    }
    // The page number and frame number are stored in the TLB
    insertIntoTLB(pageNumber, frameNumber);  
    // Finally retrieve the physical address value
    value = physicalMemory[frameNumber][offset];  
    // Output the details to the console
    printf("Logical address: %d\nPhysical address: %d\nValue: %d\n\n", logical_address, (frameNumber << 8) | offset, value);
}

// Implemented FIFO replacement to add to the TLB
void insertIntoTLB(int pageNumber, int frameNumber){
    
    // Skip the insertion in the values already exist in the TLB
    int i;
    for(i = 0; i < numberOfTLBEntries; i++){
        if(TLBPageNumber[i] == pageNumber){
            break;
        }
    }
    
    // If the index is equal to the number of entries 
    // This will be the case when the values are not already in the TLB
    if(i == numberOfTLBEntries){
        // If the TLB has room, the page and frame are inserted on the end
        if(numberOfTLBEntries < TLB_SIZE){  
            TLBPageNumber[numberOfTLBEntries] = pageNumber;    
            TLBFrameNumber[numberOfTLBEntries] = frameNumber;
        }
        // If the TLB is full, everything is moved over 
        else{                                           
            for(i = 0; i < TLB_SIZE - 1; i++){
                TLBPageNumber[i] = TLBPageNumber[i + 1];
                TLBFrameNumber[i] = TLBFrameNumber[i + 1];
            }
            // Page and frame are inserted on the end
            TLBPageNumber[numberOfTLBEntries-1] = pageNumber;  
            TLBFrameNumber[numberOfTLBEntries-1] = frameNumber;
        }        
    }
    
    /* If the index is not equal to the number of entries.
     * This is the case when the new values were already found the TLB
     * The other values in the TLB are moved over a spot and the new values are
     * added on the end to maintain FIFO replacement.
     */
    else{
        for(i = i; i < numberOfTLBEntries - 1; i++){      
            TLBPageNumber[i] = TLBPageNumber[i + 1];      
            TLBFrameNumber[i] = TLBFrameNumber[i + 1];
        }
        if(numberOfTLBEntries < TLB_SIZE){                
            TLBPageNumber[numberOfTLBEntries] = pageNumber;
            TLBFrameNumber[numberOfTLBEntries] = frameNumber;
        }
        else{                                             
            TLBPageNumber[numberOfTLBEntries-1] = pageNumber;
            TLBFrameNumber[numberOfTLBEntries-1] = frameNumber;
        }
    }

    // Increment the number of entries
    // if the TLB is full, the number of entries will not increase 
    if(numberOfTLBEntries < TLB_SIZE){                    
        numberOfTLBEntries++;
    }    
}

/* This function reads from the backing store: "BACKING_STORE.bin" and stores
 * the frame in the physical memory and the page table
 */
void readFromStore(int pageNumber){
    // Start from the begining of the file 
    // Seek a byte chunk based on the page number
    if (fseek(backing_store, pageNumber * CHUNK, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking in BACKING_STORE.bin\n");
    }
    
    // Read the byte chunk into the buffer
    if (fread(buffer, sizeof(signed char), CHUNK, backing_store) == 0) {
        fprintf(stderr, "Error reading from BACKING_STORE.bin\n");        
    }
    
    // Load the data into physical memory in the first available frame
    for(int i = 0; i < CHUNK; i++){
        physicalMemory[currentFrame][i] = buffer[i];
    }
    
    // At the first available frame in the page table, load the frame number
    pageTableNumbers[currentPageTableNumber] = pageNumber;
    pageTableFrames[currentPageTableNumber] = currentFrame;
    
    // Increment the first available frame and page table number 
    currentFrame++;
    currentPageTableNumber++;
}

/* Program Entry Point
 * The text file and backing store are opened and each line from 
 * 'addresses.txt' is iterated over while calling getPage()
 */
int main(int argc, char *argv[])
{
    // Open the backing store
    backing_store = fopen("BACKING_STORE.bin", "rb");
    // Open the file to iterate over
    address_file = fopen(argv[1], "r");
    // Initialize counter for statistics
    int numberOfTranslatedAddresses = 0;
    // Contintue reading logical addresses until we reach null 
    while ( fgets(address, BUFFER_SIZE, address_file) != NULL) {
        logical_address = atoi(address);
        
        // Function call for each line in the file 
        getPage(logical_address);
        numberOfTranslatedAddresses++;       
    }
    
    // Print out relevant inforation after the algorithm finishes 
    printf("Addresses translated = %d\n", numberOfTranslatedAddresses);
    double pageFaultRate = pageFaults / (double)numberOfTranslatedAddresses;
    double TLBHitRate = TLBHits / (double)numberOfTranslatedAddresses;
    
    printf("Page Faults = %d\n", pageFaults);
    printf("Percentage of page faults = %%%.2f\n",pageFaultRate * 100);
    printf("TLB Hits = %d\n", TLBHits);
    printf("Percentage of TLB Hits = %%%.2f\n", TLBHitRate * 100);
    
    // Close the files
    fclose(address_file);
    fclose(backing_store);
    
    return 0;//End
}
