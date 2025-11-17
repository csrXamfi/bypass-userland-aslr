# bypass-userland-aslr

Bypass aslr macos 
```bash
clang -o main.c -o bypass_aslr
clang jumper.c -o jumper

output process bypass:
csrXamfi 5680   0.0  0.0 435298976   1216 s006  S+    8:20PM   0:00.00 ./jumper
csrXamfi@MacBook-Air-csrXamfi Debug % nm -gU jumper
0000000100000000 T __mh_execute_header
000000010000047c T _main
0000000100000460 T _my_function
csrXamfi@MacBook-Air-mikhail Debug % sudo ./bypass_aslr
Enter offset to function (nm -gU your_function): 0x0000000100000460
Enter PID: 5680
[+] ASLR slide for ./jumper: 0x4b78000
Your address: 0x104b78460
--------------------------------------------
csrXamfi@MacBook-Air-csrXamfi Debug % ./jumper
Address for my_function: 0x104b78460
Enter address for jump: 0x104b78460
Functions called! ASLR bypass!
```
