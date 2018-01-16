#include "sort_file.h"
#include "bf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"


SR_ErrorCode SR_Init() {

  return SR_OK;
}

SR_ErrorCode SR_CreateFile(const char *fileName) {

  if (BF_CreateFile(fileName) != BF_OK)
    return SR_ERROR;

  int fileDesc;
  BF_Block* block;
  BF_Block_Init(&block);
  char* blockData;

  if (BF_OpenFile(fileName, &fileDesc) != BF_OK)
    return SR_ERROR;

  // create the first identifying block of the sort file
  if (BF_AllocateBlock(fileDesc, block) != BF_OK)
    return SR_ERROR;

  blockData = BF_Block_GetData(block);

  strcpy(blockData, "This is a sort file");

  BF_Block_SetDirty(block);
  if (BF_UnpinBlock(block) != BF_OK)
    return SR_ERROR;

  BF_CloseFile(fileDesc);

  // free allocated memory
  BF_Block_Destroy(&block);

  return SR_OK;
}

SR_ErrorCode SR_OpenFile(const char *fileName, int *fileDesc) {

  if (BF_OpenFile(fileName, fileDesc) != BF_OK)
    return SR_ERROR;

  BF_Block* block;
  BF_Block_Init(&block);
  char* blockData;

  // get the first block and check if the file is an sort file
  if (BF_GetBlock(*fileDesc, 0, block) != BF_OK)
    return SR_ERROR;

  blockData = BF_Block_GetData(block);

  if (strcmp(blockData, "This is a sort file") != 0)
  {
    printf("Not a sort file\n");
    return SR_ERROR;
  }

  if (BF_UnpinBlock(block) != BF_OK)
    return SR_ERROR;

  // free allocated memory
  BF_Block_Destroy(&block);

  return SR_OK;
}

SR_ErrorCode SR_CloseFile(int fileDesc) {

  if (BF_CloseFile(fileDesc) != BF_OK)
    return SR_ERROR;

  return SR_OK;
}

SR_ErrorCode SR_InsertEntry(int fileDesc,	Record record) {

  BF_Block* block;
  BF_Block_Init(&block);
  char* blockData;
  int blocksNum;
  char* entry = malloc(MAX_ENTRY_SIZE); // temp: the string that contains the entry to be inserted in the sort file

  char* s = malloc(sizeof(int) + 1);
  sprintf(s, "%d", record.id);

  // create the entry to be inserted in the sort file
  sprintf(entry, "%d$%s$%s$%s&", record.id, record.name, record.surname, record.city);

  if (BF_GetBlockCounter(fileDesc, &blocksNum) != BF_OK)
    return SR_ERROR;

  if (blocksNum > 1) // entries have been added to the file previously
  {
    if (BF_GetBlock(fileDesc, blocksNum - 1 , block) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(block);

    if (strlen(blockData) + strlen(entry) + 2 <= BF_BLOCK_SIZE) // entry can fit in the block
      strcat(blockData, entry); // add entry to the block
    else
    {
      BF_Block_SetDirty(block);
      // unpin the full block
      if (BF_UnpinBlock(block) != BF_OK)
        return SR_ERROR;

      // create a new empty block
      if (BF_AllocateBlock(fileDesc, block) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(block);
      strcpy(blockData, entry);
    }
  }
  else // the file does not contain any entries
  {
    if (BF_AllocateBlock(fileDesc, block) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(block);

    strcpy(blockData, entry);
  }

  BF_Block_SetDirty(block);
  if (BF_UnpinBlock(block) != BF_OK)
    return SR_ERROR;

  // free allocated memory
  BF_Block_Destroy(&block);
  free(entry);
  free(s);

  return SR_OK;
}


SR_ErrorCode SR_SortedFile(
  const char* input_filename,
  const char* output_filename,
  int fieldNo,
  int bufferSize,
  int callNumber
) {

  int fileDesc, blocksNum, error;
  BF_Block* block;

  char tempName[30];
  sprintf(tempName, "input_temp_%d", callNumber);

  // copy input file to a temporary input file for manipulation
  error = copyFile(input_filename, tempName, 0);
  if (error == -1)
    return error;

  // open temporary file
  if (BF_OpenFile(tempName, &fileDesc) != BF_OK)
    return SR_ERROR;

  if (BF_GetBlockCounter(fileDesc, &blocksNum) != BF_OK)
    return SR_ERROR;


  char* blockDataEntries[40], *blockDataTokens[4], *blockData;
  char* blockDataCopy;
  blockDataCopy = malloc(BF_BLOCK_SIZE);

  BF_Block_Init(&block);

  char* entry = malloc(MAX_ENTRY_SIZE);

  Record* records[40]; // 40: maximum entries count in a block
  for (int i = 0; i < 40; i++)
    records[i] = NULL;

  int recordsNum;

  /*************************************************************** step 0: sort every block individually *********************************************************/
  for (int i = 1; i < blocksNum; i++)
  {
    if (BF_GetBlock(fileDesc, i, block) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(block);

    strcpy(blockDataCopy, blockData);

    // get entries of block
    blockDataEntries[0] = strtok(blockDataCopy, "&");
    int j = 1;
    while (blockDataEntries[j-1] != NULL && j < 40)
    {
      blockDataEntries[j] = strtok(NULL, "&");
      j++;
    }

    j = 0;
    recordsNum = 0;
    // transform each entry to Record and store it in a dynamic array
    while (blockDataEntries[j] != NULL && j < 40)
    {
      records[j] = malloc(MAX_ENTRY_SIZE);
      (*records[j]) = entryToRecord(blockDataEntries[j]);
      recordsNum++;
      j++;
    }

    //sort records according to fieldNo
    sort(records, recordsNum, fieldNo);
    strcpy(blockData, ""); // empty block

    // insert sorted entries in current block
    for (j = 0; j < recordsNum; j++)
    {
      // create the entry to be inserted in current block
      sprintf(entry, "%d$%s$%s$%s&", records[j]->id, records[j]->name, records[j]->surname, records[j]->city);
      strcat(blockData, entry);
    }

    BF_Block_SetDirty(block);
    if (BF_UnpinBlock(block) != BF_OK)
      return SR_ERROR;

    // free allocated memory
    for (j = 0; j < recordsNum; j++)
    {
      if (records[j] != NULL)
      {
        records[j] = NULL;
        free(records[j]);
      }
    }

    for (j = 0; j < 40; j++)
      blockDataEntries[j] = NULL;
  }

  free(entry);
  BF_Block_Destroy(&block);


  /*********************************************************** step 1: sort groups of bufferSize - 1 blocks and store each group in a different file *********************************************/
  // calculate number of output files
  int size = blocksNum/(bufferSize - 1); // size: number of output files
  if (blocksNum%(bufferSize - 1) > 0)
    size = (blocksNum/(bufferSize - 1)) + 1;

  int fileDescs[size];

  int currEntriesNum;
  BF_Block* blocks[bufferSize]; // one block is for output and bufferSize - 1 blocks are for input and merge-sort
  for (int i = 0; i < bufferSize; i++)
    BF_Block_Init(&blocks[i]);

  char* currEntries[bufferSize - 1]; // : the current minimum entries (one for each block of the buffer)
  int entriesToSkip[bufferSize - 1]; // : how many entries to skip in each block to find its next entry


  int fileIndex = 0; // : output file index in array fileDescs
  int i = 1; // first data block (skip the first metadata block)
  int j;
  while (i < blocksNum)
  {
    // allocate memory and initialize array
    for (int i = 0; i < bufferSize - 1; i++)
      currEntries[i] = malloc(MAX_ENTRY_SIZE); // max size of entry
    for (j = 0; j < bufferSize - 1; j++)
      entriesToSkip[j] = 0;

    // get the first entry of each of (bufferSize - 1) blocks
    j = 0;
    currEntriesNum = 0;
    while (j < bufferSize - 1 && i < blocksNum)
    {
      if (BF_GetBlock(fileDesc, i, blocks[j]) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(blocks[j]);
      strcpy(blockDataCopy, blockData);

      // get first entry of current block
      strcpy(currEntries[j], strtok(blockDataCopy, "&"));

      // update variables
      entriesToSkip[j]++;
      currEntriesNum++;
      j++;
      i++;
    }

    // put the entries of (bufferSize - 1) blocks in an output file sorted
    char fileName[30];
    // create output file with distinct name (because having the same name with an old or a newer file will cause problems)
    sprintf(fileName, "output_%d_0_%d", callNumber, fileIndex);

    if (BF_CreateFile(fileName) != BF_OK)
      return SR_ERROR;
    if (BF_OpenFile(fileName, &fileDescs[fileIndex]) != BF_OK)
      return SR_ERROR;

    // allocate the first output block
    if (BF_AllocateBlock(fileDescs[fileIndex], blocks[bufferSize - 1]) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(blocks[bufferSize - 1]);

    char* minEntry;
    // while there are more entries in at least one block
    while (!allEntriesNull(currEntries, 0, currEntriesNum))
    {
      // get the minimum current entry
      minEntry = findMinCurrEntry(fieldNo, currEntries, 0, currEntriesNum, blocks, NULL/*int* currBlockIds /*for multiple files only*/, NULL, entriesToSkip, 1);
      // insert minEntry into the output file
      if (strlen(blockData) + strlen(minEntry) + 3 <= BF_BLOCK_SIZE) // entry fits in block
      {
        strcat(blockData, minEntry);
        strcat(blockData, "&");
      }
      else // entry does not fit in block
      {
        BF_Block_SetDirty(blocks[bufferSize - 1]);
        if (BF_UnpinBlock(blocks[bufferSize - 1]) != BF_OK)
          return SR_ERROR;

        // allocate the first output block
        if (BF_AllocateBlock(fileDescs[fileIndex], blocks[bufferSize - 1]) != BF_OK)
          return SR_ERROR;

        blockData = BF_Block_GetData(blocks[bufferSize - 1]);

        strcat(blockData, minEntry);
        strcat(blockData, "&");
      }
    }
    free(minEntry);

    BF_Block_SetDirty(blocks[bufferSize - 1]);
    if (BF_UnpinBlock(blocks[bufferSize - 1]) != BF_OK)
      return SR_ERROR;

    for (int i = 0; i < bufferSize - 1; i++)
      free(currEntries[i]);

    // unpin all blocks to make room for the next ones
    for (j = 0; j < bufferSize - 1; j++)
    {
      if (BF_UnpinBlock(blocks[j]) != BF_OK)
        return SR_ERROR;
    }
    // update output file index
    fileIndex++;
  }

  // close all output files
  for (int i = 0; i < fileIndex; i++)
  {
    if (BF_CloseFile(fileDescs[i]) != BF_OK)
      return SR_ERROR;
  }

  // close temporary input file
  if (BF_CloseFile(fileDesc) != BF_OK)
    return SR_ERROR;

  char command[40];
  // delete temporary input file
  sprintf(command, "rm -rf %s", tempName);
  int ret = system(command);

  // free allocated memory for blocks
  for (int i = 0; i < bufferSize; i++)
    BF_Block_Destroy(&blocks[i]);


  /********************************************************** steps 2- : sort groups of (bufferSize - 1)*(bufferSize - 1) blocks and store each group in a different file **************************************************/
  // calculate number of next step's output files
  int size1 = fileIndex/(bufferSize - 1);
  if (bufferSize - 1 > fileIndex)
    size1 = 1;

  // call mergeAndSortFiles to complete the rest of the steps of merge-sort
  error = mergeAndSortFiles(fieldNo, fileDescs, fileIndex, size1, bufferSize, output_filename, callNumber, 1);

  if (error == SR_ERROR)
    return SR_ERROR;

  free(blockDataCopy);
  return SR_OK;
}

SR_ErrorCode SR_PrintAllEntries(int fileDesc) {

  BF_Block* block = malloc(BF_BLOCK_SIZE);
  char* blockData;
  int blocksNum;
  char* blockDataEntries[40]; // 40 is the average number of entries that fit in a block
  char* blockDataTokens[4];
  char* temp = malloc(BF_BLOCK_SIZE);

  if (BF_GetBlockCounter(fileDesc, &blocksNum) != BF_OK)
    return SR_ERROR;

  if (blocksNum > 1) // file contains entries
  {
    // access all data blocks of the sort file and print all entries in a user-friendly format
    for (int i = 1; i < blocksNum; i++)
    {
      if (BF_GetBlock(fileDesc, i, block) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(block);
      strcpy(temp, blockData);

      blockDataEntries[0] = strtok(temp, "&");

      int j = 1;
      while (blockDataEntries[j - 1] != NULL && j < 40) // while the previous entry is valid
      {
        blockDataEntries[j] = strtok(NULL, "&");
      	j++;
      }

      j = 0;
      while (blockDataEntries[j] != NULL && j < 40)
      {
        blockDataTokens[0] = strtok(blockDataEntries[j], "$");
        if (blockDataTokens[0] != NULL)
        {
          for (int k = 1; k < 4;k++)
             blockDataTokens[k] = strtok(NULL,"$");

          if (blockDataTokens[1] != NULL)
            printf("%d, \"%s\", \"%s\", \"%s\"\n", atoi(blockDataTokens[0]), blockDataTokens[1], blockDataTokens[2], blockDataTokens[3]);
        }
        else
          break;

      	j++;
      }
      if (BF_UnpinBlock(block) != BF_OK)
        return SR_ERROR;
    }
  }
  else // file empty
  {
    printf("File is empty");
    return SR_ERROR;
  }

  // free allocated memory
  BF_Block_Destroy(&block);
  free(temp);

  return SR_OK;
}
