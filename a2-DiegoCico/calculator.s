# A terminal calculator
#
# Reads a line of input, interprets it as a simple arithmetic expression,
# and prints only the result. The input format is:
# <long_integer> <operation> <long_integer>

# Make `main` accessible outside of this module
.global main

# Start of the code section
.text

main:
    pushq %rbp
    movq %rsp, %rbp

    # Read input
    leaq scanf_fmt(%rip), %rdi     
    leaq a(%rip), %rsi              # first integer
    leaq op(%rip), %rdx             # operator
    leaq b(%rip), %rcx              # second integer
    xorq %rax, %rax                
    call scanf                      # Call scanf 

    # Analyze operation and execute
    movb op(%rip), %al             

    cmpb $'+', %al                 
    je add_op

    cmpb $'-', %al                
    je sub_op

    cmpb $'*', %al                 
    je mul_op

    cmpb $'/', %al                  
    je div_op

    leaq error_msg(%rip), %rdi      
    xorq %rax, %rax                
    call printf
    jmp exit_main

add_op:
    movq a(%rip), %rax           
    addq b(%rip), %rax            
    jmp print_result

sub_op:
    movq a(%rip), %rax        
    subq b(%rip), %rax         
    jmp print_result

mul_op:
    movq a(%rip), %rax
    imulq b(%rip)
    jmp print_result

div_op:
    movq a(%rip), %rax             
    cmpq $0, b(%rip)
    je div_zero_error

    cqto                        
    idivq b(%rip)                  
    jmp print_result

div_zero_error:
    leaq div_by_zero_msg(%rip), %rdi
    xorq %rax, %rax                
    call printf
    jmp exit_main

print_result:
    leaq raw_output_fmt(%rip), %rdi
    movq %rax, %rsi                
    xorq %rax, %rax                
    call printf                    
    jmp exit_main

exit_main:
    # Function epilogue
    movq %rbp, %rsp
    popq %rbp
    ret

# Start of the data section
.data

raw_output_fmt:
    .asciz "%ld\n"
scanf_fmt:
    .asciz "%ld %c %ld"

a: .quad 0
b: .quad 0
op: .byte 0

error_msg:
    .asciz "Unknown operation\n"
div_by_zero_msg:
    .asciz "Division by zero\n"

