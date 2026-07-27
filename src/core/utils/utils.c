#include "error/error.h"
#include "utils/utils.h"
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <float.h>

static const double DOUBLE_COMPARISON_PRECISION = DBL_EPSILON;

Error freeArray(void* array, size_t count, size_t itemSize, free_f freeFunc) {
  if (!array ||
      !freeFunc ||
      !count ||
      !itemSize)
    return BadArgs;

  // NOTE: this method of iterating may be dangerous 
  // if (somehow) the array reallocates in the middle of this iteration
  // Since for now this hasn't caused any issues, I do not wish to change this
  char* endOfArray = (char*)array + count * itemSize;
  for (char* i = array; i < endOfArray; i += itemSize)
    freeFunc(i);

  return OK;
}

bool doubleEqual(double a, double b) {
  return fabs(a - b) < DOUBLE_COMPARISON_PRECISION;
}

Error snTimestampedFilename(char* dest, size_t n, 
                            const char* prefix, 
                            const char* suffix, 
                            uint count) {
  if (!prefix || !suffix)
    return BadArgs;

  char tsBuf[TIMESTAMP_BUF_SZ] = {};
  Error err = OK;
  if ((err = snTimestamp(tsBuf, TIMESTAMP_BUF_SZ)))
    return err;

  if (count) {
    if (snprintf(dest, n, 
                 "%s%s-%u%s", 
                 prefix, tsBuf, 
                 count, suffix) <= 0)
      return LongFormat;
  } else {
    if (snprintf(dest, n, 
                 "%s%s%s", 
                 prefix, tsBuf,
                 suffix) <= 0)
      return LongFormat;
  }

  return OK;
}

Error snTimestamp(char* dest, size_t n) {
  time_t timeAbs = time(NULL);
  struct tm* localTime = localtime(&timeAbs);
  return strftime(dest, n, "%d-%m-%Y-%H:%M:%S", localTime)
         ? OK
         : LongFormat;
}

Error mappedFileInit(MappedFile* mappedFile, const char* filename) {
  if (!filename || 
      !mappedFile)
    return BadArgs;
  if (mappedFile->data)
    return DenyReinit;

  int fd = open(filename, O_RDONLY);
  if (fd < 0)
    return FailFileOpen;

  struct stat fileStats = {};
  fstat(fd, &fileStats);
  size_t fileSize = (size_t)fileStats.st_size;
  char *buffer = mmap(NULL, fileSize,
                      PROT_READ, MAP_PRIVATE,
                      fd, 0);
  close(fd);
  if (buffer == MAP_FAILED)
    return FailMemoryMapping;

  mappedFile->size = fileSize;
  mappedFile->data = buffer;
  return OK;
}

Error mappedFileDestroy(MappedFile* mappedFile) {
  if (!mappedFile)
    return BadArgs;
  if (!mappedFile->data)
    return NullPointerField;

  if (munmap(mappedFile->data, mappedFile->size) < 0)
    return FailMemoryUnmapping;

  mappedFile->data = NULL;
  mappedFile->size = 0;
  return OK;
}
