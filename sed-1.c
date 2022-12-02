#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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
  growBuffer(&buf, buf.bytesRead + 1);
  buf.base[buf.bytesRead] = '\0';
  printf("%s\n", buf.base);
  fclose(file);
  freeBuffer(&buf);
}
