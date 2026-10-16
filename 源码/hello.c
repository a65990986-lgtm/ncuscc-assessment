#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) return 64;
    FILE *log = fopen("/workspace/run.log", "a");
    if (log == NULL) return 1;
    fprintf(log, "processed:%s\n", argv[1]);
    fclose(log);
    printf("hello:%s\n", argv[1]);
    return 0;
}
