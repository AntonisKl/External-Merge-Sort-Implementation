#include "bf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


int compareRecords(Record record1, Record record2, int fieldNo)
{
  if (fieldNo == 0 && record1.id > record2.id)
    return 1;
  else if (fieldNo == 0 && record1.id == record2.id)
    return 0;
  else if (fieldNo == 0 && record1.id < record2.id)
    return -1;
  else if (fieldNo == 1)
    return strcmp(record1.name, record2.name);
  else if (fieldNo == 2)
    return strcmp(record1.surname, record2.surname);
  else
    return strcmp(record1.city, record2.city);
}

void sort(Record **records, int n, int fieldNo)
{
    Record temp;
    for (int i = 0; i < n; i++)
    {
        for (int j = n - 1; j > i; j--)
        {
            if (compareRecords(*records[i], *records[j], fieldNo) > 0)
             {
                 // swap
                 temp = (*records[i]);
                 (*records[i]) = (*records[j]);
                 (*records[j]) = temp;
             }
        }
    }
}

Record entryToRecord(char* entry)
{
  char* entryCopy = malloc(MAX_ENTRY_SIZE);

  strcpy(entryCopy, entry);

  Record record;
  char* blockDataTokens[4];
  // get fields of entry
  blockDataTokens[0] = strtok(entryCopy, "$");
  if (blockDataTokens[0] != NULL)
  {
    record.id = atoi(blockDataTokens[0]);
    for (int k = 1; k < 4;k++)
      blockDataTokens[k] = strtok(NULL,"$");
    if (blockDataTokens[1] != NULL)
    {
      strcpy(record.name, blockDataTokens[1]);
      strcpy(record.surname, blockDataTokens[2]);
      strcpy(record.city, blockDataTokens[3]);
    }
  }

  free(entryCopy);
  return record;
}



int allEntriesNull(char** currEntries, int start, int end)
{
  int count = 0;
  for (int i = start; i < end; i++)
  {
    if (currEntries[i] == NULL)
      count++;
    else
      break;
  }
  if (count == end - start)
    return 1;
  else
    return 0;
}

int copyFile(const char* inputFileName, const char* outputFileName, int flag)
{
  BF_Block* block, *block1;
  int inputFileDesc, outputFileDesc, blocksNum;
  char* blockData, *blockData1;

  if (BF_OpenFile(inputFileName, &inputFileDesc) != BF_OK)
    return -1;

  if (BF_GetBlockCounter(inputFileDesc, &blocksNum) != BF_OK)
    return -1;

  if (BF_CreateFile(outputFileName) != BF_OK)
    return -1;

  if (BF_OpenFile(outputFileName, &outputFileDesc) != BF_OK)
    return -1;

  BF_Block_Init(&block);
  BF_Block_Init(&block1);
  if (flag == 1)
  {
    if (BF_AllocateBlock(outputFileDesc, block1) != BF_OK)
      return -1;

    blockData1 = BF_Block_GetData(block1);

    strcpy(blockData1, "This is a sort file");

    BF_Block_SetDirty(block1);
    if (BF_UnpinBlock(block1) != BF_OK)
      return -1;
  }


  for (int i = 0; i < blocksNum; i++)
  {
    if (BF_GetBlock(inputFileDesc, i, block) != BF_OK)
      return -1;

    blockData = BF_Block_GetData(block);

    if (BF_AllocateBlock(outputFileDesc, block1) != BF_OK)
      return -1;

    blockData1 = BF_Block_GetData(block1);

    strcpy(blockData1, blockData);

    BF_Block_SetDirty(block1);
    if (BF_UnpinBlock(block1) != BF_OK)
      return -1;

    if (BF_UnpinBlock(block) != BF_OK)
      return -1;
  }

  if (BF_CloseFile(inputFileDesc) != BF_OK)
    return -1;

  if (BF_CloseFile(outputFileDesc) != BF_OK)
    return -1;

  BF_Block_Destroy(&block);
  BF_Block_Destroy(&block1);

  return outputFileDesc;
}

char* findMinCurrEntry(int fieldNo, char** currEntries, int start, int currEntriesNum, BF_Block** blocks, int* currBlockIds /*for multiple files only*/, int* inputFileDescs /*for multiple files only*/, int* entriesToSkip, int singleFileCallFlag)
{
  Record* records[currEntriesNum];

  // store entries as Records temporarily
  for (int i = 0; i < currEntriesNum; i++)
  {
    records[i] = malloc(MAX_ENTRY_SIZE);
    if (currEntries[i + start] != NULL)
      (*records[i]) = entryToRecord(currEntries[i + start]);
    else
      records[i] = NULL;
  }

  Record* minRecord;
  int minRecordIndex;
  // set a Record as minimum record
  for (int i = 0; i < currEntriesNum; i++)
  {
    if (records[i] != NULL)
    {
      minRecord = records[i];
      minRecordIndex = i;
      break;
    }
  }

  // find minimum record and store its index
  for (int i = 0; i < currEntriesNum; i++)
  {
    if (records[i] != NULL)
    {
      if (compareRecords(*minRecord, *records[i], fieldNo) > 0)
      {
        minRecord = records[i];
        minRecordIndex = i;
      }
    }
  }

  // create the minimum entry
  char* minEntry = malloc(MAX_ENTRY_SIZE);
  strcpy(minEntry, currEntries[start + minRecordIndex]);

  // free allocated memory
  for (int i = 0; i < currEntriesNum; i++)
  {
    if (records[i] != NULL)
    {
      records[i] = NULL;
      free(records[i]);
    }
  }

  // for step 1 of external merge-sort
  if (singleFileCallFlag == 1)
  {
    char* blockData = BF_Block_GetData(blocks[minRecordIndex]);
    char* blockDataCopy = malloc(BF_BLOCK_SIZE);
    strcpy(blockDataCopy, blockData);
    char* temp;
    temp = strtok(blockDataCopy, "&");

    // get the next entry
    for (int i = 0; i < entriesToSkip[minRecordIndex]; i++)
      temp = strtok(NULL, "&");

    // update currEntries
    if (temp != NULL)
      strcpy(currEntries[minRecordIndex], temp);
    else
      currEntries[minRecordIndex] = NULL;

    // update entriesToSkip
    entriesToSkip[minRecordIndex]++;

    free(blockDataCopy);
  }
  // for steps 2- of external merge-sort
  else /*singleFileCallFlag == 0*/
  {
    // start may be >0 because in the current step there are more than one input files
    int currIndex = minRecordIndex + start;
    int currFileBlocksNum;

    char* blockData = BF_Block_GetData(blocks[currIndex]);
    char* blockDataCopy = malloc(BF_BLOCK_SIZE);
    strcpy(blockDataCopy, blockData);

    char* temp;
    temp = strtok(blockDataCopy, "&");

    // get the next entry
    for (int i = 0; i < entriesToSkip[currIndex]; i++)
      temp = strtok(NULL, "&");

    if (temp != NULL)
    {
      if (strlen(temp) + 1 > MAX_ENTRY_SIZE)  // must advance to next block because temp contains all data of next block sometimes
      {
        if (BF_GetBlockCounter(inputFileDescs[currIndex], &currFileBlocksNum) != BF_OK)
        {
          printf("Block counter error\n");
          return NULL;
        }

        if (currBlockIds[currIndex] < currFileBlocksNum - 1) // not the last block of file
        {
          if (BF_UnpinBlock(blocks[currIndex]) != BF_OK)
            return NULL;

          currBlockIds[currIndex]++;
          if (BF_GetBlock(inputFileDescs[currIndex], currBlockIds[currIndex], blocks[currIndex]) != BF_OK)
            return NULL;

          blockData = BF_Block_GetData(blocks[currIndex]);
          strcpy(blockDataCopy, blockData);

          // get the first entry of next block
          strcpy(currEntries[currIndex], strtok(blockDataCopy, "&"));
          entriesToSkip[currIndex] = 1;

        }
        else // current block is the last block of file so there is no block to advance to
        {
          currEntries[currIndex] = NULL;
          if (BF_UnpinBlock(blocks[currIndex]) != BF_OK)
            return NULL;
        }
      }
      else
      {
        strcpy(currEntries[currIndex], temp);
        entriesToSkip[currIndex]++;
      }

    }
    else
    {
      if (BF_GetBlockCounter(inputFileDescs[currIndex], &currFileBlocksNum) != BF_OK)
      {
        printf("Block counter error\n");
        return NULL;
      }

      if (currBlockIds[currIndex] < currFileBlocksNum - 1) // not the last block of file
      {
        if (BF_UnpinBlock(blocks[currIndex]) != BF_OK)
          return NULL;


        currBlockIds[currIndex]++;
        if (BF_GetBlock(inputFileDescs[currIndex], currBlockIds[currIndex], blocks[currIndex]) != BF_OK)
          return NULL;

        blockData = BF_Block_GetData(blocks[currIndex]);
        strcpy(blockDataCopy, blockData);

        // get the first entry of next block
        strcpy(currEntries[currIndex], strtok(blockDataCopy, "&"));
        entriesToSkip[currIndex] = 1;

      }
      else // current block is the last block of file so there is no block to advance to
      {
        currEntries[currIndex] = NULL;
        if (BF_UnpinBlock(blocks[currIndex]) != BF_OK)
          return NULL;
      }
    }
    free(blockDataCopy);

  }
  return minEntry;
}



int mergeAndSortFiles(int fieldNo, int* inputFileDescs, int inputFilesNum, int outputFilesNum, int bufferSize, const char* output_filename, int callNumber, int step)
{
  char outputFileName[30];
  int outputFileDescs[outputFilesNum];
  int currBlockIds[inputFilesNum*(bufferSize - 1)]; // array for block ids of blocks of all files which will be created
  BF_Block* blocks[inputFilesNum*(bufferSize - 1)]; // array for block pointers of blocks of all files which will be created
  BF_Block* outputBlock;
  BF_Block_Init(&outputBlock);
  char* currEntries[inputFilesNum*(bufferSize - 1)]; // array for current entries of all files which will be created
  int currEntriesNum; // number of current entries in each step of external merge-sort

  int entriesToSkip[inputFilesNum];

  char* blockData, *blockDataCopy;
  blockDataCopy = malloc(BF_BLOCK_SIZE);

  char fileName[30], *temp;
  // open current input files
  for (int i = 0; i < inputFilesNum; i++)
  {
    sprintf(fileName, "output_%d_%d_%d", callNumber, step - 1, i);
    if (BF_OpenFile(fileName, &inputFileDescs[i]) != BF_OK)
      return SR_ERROR;
  }


  int outputFilesIndex = 0;
  int i = 0;
  int j;
  int runNum = 0;
  int n;
  while (i < inputFilesNum)
  {
    n = i + bufferSize - 1;
    currEntriesNum = 0;
    while (i < n && i < inputFilesNum) // while (bufferSize - 1) is not exceeded
    {
      currEntries[i] = malloc(MAX_ENTRY_SIZE); // max size of entry
      currBlockIds[i] = 0;
      entriesToSkip[i] = 0;
      BF_Block_Init(&blocks[i]);

      if (BF_GetBlock(inputFileDescs[i], 0, blocks[i]) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(blocks[i]);
      strcpy(blockDataCopy, blockData);
      // get the current entry of each block
      strcpy(currEntries[i], strtok(blockDataCopy, "&")); // first entry of current block

      entriesToSkip[i]++;
      currEntriesNum++;
      i++;
    }

    // put the entries of the bufferSize - 1 files in a single output file sorted
    // create output file with distinct name
    sprintf(fileName, "output_%d_%d_%d", callNumber, step, outputFilesIndex);
    if (BF_CreateFile(fileName) != BF_OK)
      return SR_ERROR;
    if (BF_OpenFile(fileName, &outputFileDescs[outputFilesIndex]) != BF_OK)
      return SR_ERROR;

    // store output file's name for later
    strcpy(outputFileName, fileName);

    // allocate the first output block
    if (BF_AllocateBlock(outputFileDescs[outputFilesIndex], outputBlock) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(outputBlock);

    char* minEntry;
    int start = i - currEntriesNum;
    int end = i;
    while (!allEntriesNull(currEntries, start, end))
    {
      // get the minimum current entry
      minEntry = findMinCurrEntry(fieldNo, currEntries, start, currEntriesNum, blocks, currBlockIds, inputFileDescs, entriesToSkip, 0);
      // store the minimum current entry into the output file
      if (strlen(minEntry) > 2) // entry is valid
      {
        if (strlen(blockData) + strlen(minEntry) + 3 <= BF_BLOCK_SIZE) // if entry fits in block
        {
          strcat(blockData, minEntry);
          strcat(blockData, "&");
        }
        else
        {
          BF_Block_SetDirty(outputBlock);
          if (BF_UnpinBlock(outputBlock) != BF_OK)
            return SR_ERROR;

          // allocate the first output block
          if (BF_AllocateBlock(outputFileDescs[outputFilesIndex], outputBlock) != BF_OK)
            return SR_ERROR;

          blockData = BF_Block_GetData(outputBlock);

          strcpy(blockData, minEntry);
          strcat(blockData, "&");
        }
      }
    }

    BF_Block_SetDirty(outputBlock);
    if (BF_UnpinBlock(outputBlock) != BF_OK)
      return SR_ERROR;

    // unpin all blocks to make room for the next ones
    for (j = start; j < end; j++)
    {
      if (BF_UnpinBlock(blocks[j]) != BF_OK)
        return SR_ERROR;
      BF_Block_Destroy(&blocks[j]);
      // free previous currEntries
      free(currEntries[j]);
    }

    minEntry = NULL;
    free(minEntry);

    outputFilesIndex++;
  }

  blockDataCopy = NULL;
  free(blockDataCopy);
  BF_Block_Destroy(&outputBlock);

  // close and delete all current input files
  for (int i = 0; i < inputFilesNum; i++)
  {
    if (BF_CloseFile(inputFileDescs[i]) != BF_OK)
      return SR_ERROR;

    sprintf(fileName, "rm -rf output_%d_%d_%d", callNumber, step - 1, i);

    int ret = system(fileName);
  }

  if (outputFilesIndex == 1) // there is only one output file so current step is the last step of external merge-sort
  {
    if (BF_CloseFile(outputFileDescs[0]) != BF_OK)
      return SR_ERROR;

    // copy sorted output file to a new sorted file with the given name
    int error = copyFile(outputFileName, output_filename, 1);
    if (error == -1)
      return SR_ERROR;

    char command[40];
    // delete the old output file
    sprintf(command, "rm -rf %s", outputFileName);
    int ret = system(command);

    return 0;
  }
  // calculate number of output files of the next step of external merge-sort
  int num = outputFilesIndex/(bufferSize - 1);
  if (bufferSize - 1 > outputFilesIndex)
    num = 1;

  // call mergeAndSortFiles recursively to complete the next step of external merge-sort
  mergeAndSortFiles(fieldNo, outputFileDescs, outputFilesIndex, num, bufferSize, output_filename, callNumber, ++step);

  return 0;
}
