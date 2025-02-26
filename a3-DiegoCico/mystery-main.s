.section .data
hat: .asciz "hat\n"
tea: .asciz "tea\n"
beer: .asciz "beer\n"
two_args_required: .asciz "Two arguments required.\n"

.section .text
.global main
.extern crunch
.extern strtol

main:
    push %rbp
    mov %rsp, %rbp
    push %r12
    push %r13
    push %r14
    push %r15  # Save important registers

    # Need exactly two args
    cmp $3, %rdi
    je valid_args
    
    # Complain and exit if wrong number of args
    mov $1, %rax
    mov $1, %rdi
    lea two_args_required(%rip), %rsi
    mov $23, %rdx
    syscall

    mov $60, %rax
    mov $1, %rdi
    syscall

valid_args:
    mov %rsi, %r15  # Save argv pointer

    # Convert first argument
    mov 8(%r15), %rdi
    xor %rsi, %rsi
    xor %rdx, %rdx
    call strtol@plt
    mov %rax, %r12

    # Convert second argument
    mov 16(%r15), %rdi
    xor %rsi, %rsi
    xor %rdx, %rdx
    call strtol@plt
    mov %rax, %r13

    # Call crunch function
    mov %r12, %rdi
    mov %r13, %rsi
    call crunch@plt
    mov %rax, %r14

    # Decide what to print
    cmp $0, %r14
    jl print_hat
    je print_tea
    jmp print_beer

print_hat:
    mov $1, %rax
    mov $1, %rdi
    lea hat(%rip), %rsi
    mov $4, %rdx
    syscall
    jmp exit

print_tea:
    mov $1, %rax
    mov $1, %rdi
    lea tea(%rip), %rsi
    mov $4, %rdx
    syscall
    jmp exit

print_beer:
    mov $1, %rax
    mov $1, %rdi
    lea beer(%rip), %rsi
    mov $5, %rdx
    syscall
    jmp exit

exit:
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    mov %rbp, %rsp
    pop %rbp

    mov $60, %rax
    xor %rdi, %rdi
    syscall