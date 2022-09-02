#include <sys/statvfs.h>
#include <stdio.h>
int main() {
    struct statvfs fs_stat;
    statvfs(".", &fs_stat);
    printf("%lu\n", fs_stat.f_bsize);
}