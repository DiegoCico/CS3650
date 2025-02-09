.section .text
.global array_max

array_max:
    # rdi = n (number of items), rsi = array pointer

    # If n == 0, return 0
    testq %rdi, %rdi        # Check if n == 0
    jz .return_zero         # Jump if n == 0

    # Set max to first element
    movq (%rsi), %rax       # rax = first element
    decq %rdi               # n -= 1
    addq $8, %rsi           # Move to next element

.loop_start:
    testq %rdi, %rdi        # Check if n > 0
    jz .done                # Exit if done

    movq (%rsi), %rcx       # rcx = current element
    cmpq %rax, %rcx         # Compare max (rax) with current (rcx)
    cmova %rcx, %rax        # Update max if rcx > rax

    decq %rdi               # n -= 1
    addq $8, %rsi           # Move to next element
    jmp .loop_start         # Repeat loop

.done:
    ret                     # Return max in rax

.return_zero:
    xorq %rax, %rax         # rax = 0
    ret                     # Return 0

