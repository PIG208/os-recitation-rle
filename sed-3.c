#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// pthread_mutex_init(lock);

typedef struct {
  char *base;
  unsigned int capacity;
  unsigned int bytesRead;
} Buffer;

Buffer allocBuffer(int initCapacity) {
  return (Buffer){
      .base = malloc(initCapacity),
      .capacity = initCapacity,
      .bytesRead = 0,
  };
}

void growBuffer(Buffer *buf, int newSize) {
  if (buf->capacity >= newSize) {
    return;
  }
  buf->base = realloc(buf->base, newSize);
  buf->capacity = newSize;
}

Buffer readIntoBuffer(FILE *file) {
  int n;
  Buffer buf = allocBuffer(10);
  while ((n = fread(buf.base + buf.bytesRead, sizeof(char),
                    buf.capacity - buf.bytesRead, file)) > 0) {
    buf.bytesRead += n;
    if (buf.bytesRead == buf.capacity) {
      growBuffer(&buf, buf.capacity * 2);
    }
  }
  return buf;
}

void freeBuffer(Buffer *buf) { free(buf->base); }

// Notes

// Goal: parallelize the tasks as much as possible.
// If we have 8 threads, how do we split the input?
//   1. Input is small: no need to split, create 1 thread only       
//   2. Otherwise, split the input into even parts

// Input:
// Lorem ipsum dolor sit amet

// What are shared, and what are not shared?
// buf, pattern, replace are shared
// result, start, end are unique for each thread

// How to split the input (DIVIDE)

// asdqwezxcasd (bytesRead == 12)

// 4 threads
// asd
// qwe..
// 12 / 4 = 3 (chunksize)
// [start, end) 
// 0, 3; 3, 6; 6, 9; 9, 12

// asdqwezxcasdas (bytesRead == 14)
// 14 // 4 = 3 ... 2
// 0, 3; 3, 6; 6, 9; 9, 14

// Running the algorithm (CONQUE)
// sed asd, sed qwe, sed zxc

// holdon, what if pattern is dqw?
// asdqwezxcasd (bytesRead == 12)
//   dqw
// asd qwe

// aabc bcbc bcbc bcbc
// aabc xxxc xxxc xxxc
//   bc b

// We skip this edge case, and assume that when we merge, we check only on the output stream.
// Note that RLE isn't subject to this issue. But you still have merge the outputs in some way

// Joining the threads and their outputs (MERGE)
// struct Something result;

// pthread_create(thread, attr, func, arg)
// pthread_create(thread, NULL, func, arg)
// pthread_join(thread, &result)
// pthread_exit(retval)

// pthread_mutex (not used)
// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// phtread_mutex_lock(&lock);
// phtread_mutex_unlock(&lock);

void sed(Buffer *buf, char *pattern, char *replace, Buffer *result, int start, int end) {
  *result = allocBuffer(buf->bytesRead);
  for (int i = 0; i < buf->bytesRead;) {
    if (i > buf->bytesRead - 3 || memcmp(buf->base + i, pattern, 3) != 0) {
      *(result->base + result->bytesRead) = buf->base[i];
      result->bytesRead++;
      i++;
    } else {
      memcpy(result->base + result->bytesRead, replace, 3);
      result->bytesRead += 3;
      i += 3;
    }
  }
}

// sed foo bar filename
// cat filename | sed foo bar
int main(int argc, char *argv[]) {
  // int fd = 0;
  FILE *file = stdin;
  if (argc == 4) {
    // open
    // open(argv[3], O_RDONLY);
    file = fopen(argv[3], "r");
  }

  Buffer buf = readIntoBuffer(file);
  fclose(file);

  // Buffer result;
  // sed(&buf, argv[1], argv[2], &result);
  // growBuffer(&result, result.bytesRead + 1);
  // result.base[result.bytesRead] = '\0';
  // printf("%s\n", result.base);

  freeBuffer(&buf);
  // freeBuffer(&result);
}