#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libguess.h"

int main(int argc, char **argv)
{
    FILE *fp;
    char buf[1024 * 1024];

    if (argc != 2)
        exit(1);

    fp = fopen(*(argv + 1), "r");
    if (!fp) {
        printf("cannot open file \"%s\"\n", *(argv + 1));
        exit(1);
    }

    while (fgets(buf, 1024 * 1024 - 1, fp)) {
//        printf("%s\n", buf);
        printf("length = %d\n", strlen(buf));
        printf("jp = %s\n", guess_jp(buf, strlen(buf)));
        printf("tw = %s\n", guess_tw(buf, strlen(buf)));
        printf("cn = %s\n", guess_cn(buf, strlen(buf)));
        printf("kr = %s\n", guess_kr(buf, strlen(buf)));
        printf("ru = %s\n", guess_ru(buf, strlen(buf)));
        printf("ar = %s\n", guess_ar(buf, strlen(buf)));
        printf("tr = %s\n", guess_tr(buf, strlen(buf)));
        printf("gr = %s\n", guess_gr(buf, strlen(buf)));
        printf("hw = %s\n", guess_hw(buf, strlen(buf)));
    }
    fclose(fp);
}
