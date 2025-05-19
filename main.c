#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "alloc.h"
#include "tests/tests.h"

int main() {
    int input;
    char buffer[10];
    printf("Enter test case:\n");
    scanf("%s", buffer);
    if (strcmp(buffer, "exit")) {
        input = atoi(buffer);
        switch (input) {
        case 1:
            test1();
            break;
        case 2:
            test2();
            break;
        case 3:
            test3();
            break;
        case 4:
            test4();
            break;
        case 5:
            test5();
            break;
        case 6:
            test6();
            break;
        default:
            printf("Wrong input\n");
            break;
        }
    }
}