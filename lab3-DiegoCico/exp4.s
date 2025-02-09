
.global main

.text
start_of_text:


# BEGIN pasted code
dummy2:
    # State of %rbp and %rsp before setting up the stack frame
    # [p $rsp]
    # [p $rbp]
    nop

    enter $32, $0  # Set up the stack frame

    # GUESS: What is the state of %rbp and %rsp after setting up the stack frame?
    # [p $rsp]
    # [p $rbp]
    nop

    leave  # Destroy the stack frame

    # GUESS: What is the state of %rbp and %rsp after destroying the stack frame?
    # [p $rsp]
    # [p $rbp]
    nop

    ret

main:
    enter $0, $0  # Set up the stack frame for `main`

    call dummy2  # Call dummy2 to observe stack behavior

    leave  # Destroy the stack frame for `main`
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

