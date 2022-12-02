#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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
}
