#include <stdio.h>

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
