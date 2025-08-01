
.global main

.text
start_of_text:


# BEGIN pasted code
main:
    # Stack manipulation
    # What is the initial value of the stack pointer, i.e., %rsp [print $rsp]
    pushq $1
    pushq $2
    pushq $3

    # GUESS: how many bytes were added to the stack, 
    # i.e., compare the current value of %rsp to its value before the pushes [print $rsp]

    popq %rax
    # GUESS: What is in %rax? [print $rax]

    # GUESS: What is on the top of the stack now, i.e., (%rsp)? [x $rsp]

    # GUESS: What is the item right below the top of the stack, i.e., 8(%rsp) [x $rsp+8]

    # Drop the top of the stack
    addq $8, %rsp

    popq %rbx
    # GUESS: What is in %rbx? [print $rbx]

    ret

# END pasted code

end_of_text:

.data
start_of_data:

stuff:
  .quad 1, 2, 3, 4, 5, 6, 7, 8

end_of_data:

size_of_stuff = end_of_data - stuff 
size_of_text = end_of_text - start_of_text
size_of_data = end_of_data - start_of_data

