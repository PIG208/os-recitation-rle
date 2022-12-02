#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NTHREADS 8
#define INPUT_THRESHOLD NTHREADS * 3

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

typedef struct {
  Buffer *buf;
  char *pattern;
  char *replace;
  //
  // For RLE: use a data structure that stores the characters and the length of
  // each run storing them as an array has an advantage of helping you to merge
  // them more easily
  //   4a3b  3b5c  5c9a
  //   ----  ----
  // threads carry intermediate information, not the final output.
  // have an buffer `struct RunEntry *entries`
  Buffer *result;
  unsigned int offset;
  unsigned int chunk_size;
} SedArg; // RunArg

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

// We skip this edge case, and assume that when we merge, we check only on the
// output stream. Note that RLE isn't subject to this issue. But you still have
// merge the outputs in some way

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

void sed(Buffer *buf, char *pattern, char *replace, Buffer *result, int start,
         int end) {
  *result = allocBuffer(end - start);
  for (int i = start; i < end;) {
    if (i > end - 2 || memcmp(buf->base + i, pattern, 2) != 0) {
      *(result->base + result->bytesRead) = buf->base[i];
      result->bytesRead++;
      i++;
    } else {
      memcpy(result->base + result->bytesRead, replace, 2);
      result->bytesRead += 2;
      i += 2;
    }
  }
}

// The entrypoint of the threads
void *run_sed(void *arg) {
  SedArg *data = (SedArg *)arg;
  sed(data->buf, data->pattern, data->replace, data->result, data->offset,
      data->offset + data->chunk_size);
  pthread_exit(NULL);
}

void go(Buffer *buf, char *pattern, char *replace) {
  int n_threads = 1;
  if (buf->bytesRead > INPUT_THRESHOLD) {
    n_threads = NTHREADS;
  }
  // Everything is allocated on the heap
  pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
  SedArg *args = malloc(n_threads * sizeof(SedArg));
  Buffer *results = malloc(n_threads * sizeof(Buffer));
  int chunk_size = buf->bytesRead / n_threads;
  for (int i = 0; i < n_threads; i++) {
    int size = chunk_size;
    if (i == n_threads - 1) {
      size = buf->bytesRead - i * chunk_size;
    }
    args[i] = (SedArg){
        .buf = buf,
        .pattern = pattern,
        .replace = replace,
        .offset = i * chunk_size,
        .chunk_size = size,
        .result = &results[i],
    };
    pthread_create(&threads[i], NULL, run_sed, &args[i]);
  }

  char prev_char;
  for (int i = 0; i < n_threads; i++) {
    pthread_join(threads[i], NULL);
    if (i > 0) {
      if (prev_char == pattern[0] && results[i].base[0] == pattern[1]) {
        fputc(replace[0], stdout);
        results[i].base[0] = replace[1];
      } else {
        fputc(prev_char, stdout);
      }
    }
    for (int j = 0; j < results[i].bytesRead - 1; j++) {
      fputc(results[i].base[j], stdout);
    }
    prev_char = results[i].base[results[i].bytesRead - 1];
    freeBuffer(&results[i]);
  }
  fputc('\n', stdout);

  free(threads);
  free(args);
  free(results);
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

  go(&buf, argv[1], argv[2]);
  freeBuffer(&buf);
}
