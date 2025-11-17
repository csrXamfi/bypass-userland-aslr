#include <stdio.h>
#include <stdlib.h>

void my_function() {
    printf("Функция выполнена! ASLR нагнут!\n");
    exit(0);
}

int main() {
    char input[32];
    unsigned long addr;
    printf("Адрес my_function: %p\n", (void*)my_function);
    
    printf("Введите адрес для перехода: ");
    scanf("%lx", &addr);
    void (*func)() = (void(*)())addr;
    func();
    
    return 0;
}
