#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

int main() {
  printf("%d\n", getpid());
  FILE *heapBuffer = fopen("fileWithHeap.bin", "wb");
  scanf("\n");
  int *zeroes = (int *)malloc(sizeof(int) * 100);
  for (int i = 0; i < 100; i++) {
    zeroes[i] = 0;
  }
  for (int i = 0; i < 10000; i++) {
    fwrite(zeroes, sizeof(int), 100, heapBuffer);
  }
  fclose(heapBuffer);
  return 0;
}