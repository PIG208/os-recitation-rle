#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void sed(Buffer *buf, char *pattern, char *replace, Buffer *result) {
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

  Buffer result;
  sed(&buf, argv[1], argv[2], &result);
  growBuffer(&result, result.bytesRead + 1);
  result.base[result.bytesRead] = '\0';
  printf("%s\n", result.base);

  freeBuffer(&buf);
  freeBuffer(&result);
}
