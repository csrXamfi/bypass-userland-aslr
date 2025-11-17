#include <stdio.h>
#include <stdlib.h>

void my_function() {
    printf("Functions called! ASLR bypass!\n");
    exit(0);
}

int main() {
    char input[32];
    unsigned long addr;
    printf("Enter address for my_function: %p\n", (void*)my_function);
    
    printf("Enter address for jump: ");
    scanf("%lx", &addr);
    void (*func)() = (void(*)())addr;
    func();
    
    return 0;
}
