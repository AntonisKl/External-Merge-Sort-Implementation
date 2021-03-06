#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>

#include "bf.h"
#include "sort_file.h"

#define CALL_OR_DIE(call)     \
  {                           \
    SR_ErrorCode code = call; \
    if (code != SR_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

#define RUN_AND_TIME(call)                                              \
  {                                                                     \
    clock_t start = clock();                                            \
    call;                                                               \
    clock_t end = clock();                                              \
    printf("CPU Time: %lf\n", (end - start) / (double)CLOCKS_PER_SEC);  \
  }

int main() {
  BF_Init(LRU);
  CALL_OR_DIE(SR_Init());

  printf("Sorting 'unsorted_data.db' file in field 'name' ...\n");
  CALL_OR_DIE(SR_SortedFile("unsorted_data.db", "sorted_name.db", 1, 3, 1))
  printf("Sorting 'unsorted_data.db' file in field 'surname' ...\n");
  CALL_OR_DIE(SR_SortedFile("unsorted_data.db", "sorted_surname.db", 2, 33, 2))
  printf("Sorting sorted_surname.db file in 'field' ...\n");
  CALL_OR_DIE(SR_SortedFile("sorted_name.db", "sorted_id.db", 0, 9, 3))
  BF_Close();
}
