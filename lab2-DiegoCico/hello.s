# Your code here


.global main

.text
main:
    enter $0, $0

    # Part 1 - Task 1: Hello, World!
    mov $message, %rdi
    mov $0, %al
    call printf

    # Part 1 - Task 2: Calculate (2 * a + b * c) >> 4
    mov a(%rip), %rax
    add %rax, %rax
    mov b(%rip), %rbx
    mov c(%rip), %rcx
    imul %rbx, %rcx
    add %rcx, %rax
    shr $4, %rax

    mov $long_format, %rdi
    mov %rax, %rsi
    mov $0, %al
    call printf

    # Part 2 - Task 1: Call labs(d)
    mov d(%rip), %rdi
    call labs

    mov $long_format, %rdi
    mov %rax, %rsi
    mov $0, %al
    call printf

    # Part 2 - Task 2: Calculate labs((d * e) << 3)
    mov d(%rip), %rax
    imul e(%rip), %rax
    shl $3, %rax
    mov %rax, %rdi
    call labs

    mov $long_format, %rdi
    mov %rax, %rsi
    mov $0, %al
    call printf

    leave
    ret

.data
message:
    .asciz "Hello, World!\n"

# Part 1 - Task 2 Data
a: .quad 0x3908
b: .quad 0x721
c: .quad 16
long_format:
    .asciz "%ld\n"

# Part 2 - Task Data
d: .quad -32
e: .quad 64

