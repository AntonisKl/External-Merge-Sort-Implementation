#ifndef UTILS_H
#define UTILS_H
#include "bf.h"
#include "sort_file.h"
#define MAX_ENTRY_SIZE 55*sizeof(char) + sizeof(int) + 4 + 1


int compareRecords(Record record1, Record record2, int fieldNo); // compares 2 records according to fieldNo and returns a number <0, =0, or >0 according to the comparison result

void sort(Record **records, int n, int fieldNo); // sorts an array of records according to fieldNo

Record entryToRecord(char* entry); // converts an entry(char*) to Record and returns it

int allEntriesNull(char** currEntries, int start, int end); // checks if all entries (in positions between start and end) in the array currEntries are NULL and returns 1 if they are, otherwise 0

int copyFile(const char* inputFileName, const char* outputFileName, int flag); // copies the file with name "inputFileName" into a new file with name "outputFileName",
                                                                               // if flag is "1" the new file has some metadata in its first block

/*
Function char* findMinCurrEntry:
Finds and returns the minimum entry of array currEntries starting from position "start", updates entriesToSkip array, advances entries in minimum entry's block and sets a new current entry
if needed, it also advances block in the case of merging different files
singleFileCallFlag is "1" when currEntries contain the current entries of input blocks
singleFileCallFlag is "0" when currEntries contain the current entries of input files (each one contains many blocks)
BF_Block** blocks: current input blocks
int* currBlockIds: block ids of current input blocks
int* inputFileDescs: file descriptors of current input files
int* entriesToSkip: number of entries to skip in each of the current blocks in order to find the next entry
char** currEntries: current entry of each input file/block
int start: starting position of valid current entries in array currEntries (start is "0" if called in the first step of external merge-sort and is ">0" in the rest of the steps)
*/
char* findMinCurrEntry(int fieldNo, char** currEntries, int start, int currEntriesNum, BF_Block** blocks, int* currBlockIds /*for multiple files only*/, int* inputFileDescs /*for multiple files only*/, int* entriesToSkip, int singleFileCallFlag);


/*
Function int mergeAndSortFiles:
Implements all steps of external merge-sort except for step 0 and step 1 (they are pointed and implemented in "sort_file.c")
returns an error code or "0" if no error was occured
*/
int mergeAndSortFiles(int fieldNo, int* inputFileDescs, int inputFilesNum, int outputFilesNum, int bufferSize, const char* output_filename, int callNumber, int step);

#endif
