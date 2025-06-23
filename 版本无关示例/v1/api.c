#include <stdio.h>
#include <stdlib.h>

struct print_info_t {
    const char *Msg;
};

typedef struct print_info_t * print_info_p;

char defaultMsg[] = "HelloV1";

print_info_p Create()
{
    print_info_p ptr = (print_info_p) malloc(sizeof(struct print_info_t));
    ptr->Msg = defaultMsg;
    return ptr;
}

void Destroy(print_info_p ptr)
{
    free(ptr);
}

void Print(print_info_p p)
{
    printf("%s\n", p->Msg);
}