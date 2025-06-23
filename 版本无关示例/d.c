#include "api.h"
void func()
{
    print_info_p p = Create();
    Print(p);
    Destroy(p);
}
