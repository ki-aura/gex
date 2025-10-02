#include "animal_main.h"

void test_ext(void) {
    struct Animal *p = malloc(sizeof(*p));
    if (!p) { perror("malloc"); return; }

    p->key = 42;
    p->name = "Tiger";

    RB_INSERT(AnimalTree_key, &keyHead, p);
    RB_INSERT(AnimalTree_name, &nameHead, p);
    printf("[test_ext] After insert, tree size: %d\n", RB_SIZE(&keyHead));
    printf("[test_ext] After insert, tree size: %d\n", RB_SIZE(&nameHead));

    RB_DELETE(AnimalTree_key, &keyHead, p);
    RB_DELETE(AnimalTree_name, &nameHead, p);
    printf("[test_ext] After delete, tree size: %d\n", RB_SIZE(&keyHead));
    printf("[test_ext] After delete, tree size: %d\n", RB_SIZE(&nameHead));

    free(p);
}
