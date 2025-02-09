
.global main

.text
start_of_text:


# BEGIN pasted code
dummy1:
    # GUESS: What is on top of the stack? Compare with the value printed by `p $after_dummy1`
    nop
    ret

main:
    enter $0, $0

    # OBSERVE: Print the current stack pointer before calling `dummy1`
    # [p $rsp]
    call dummy1

after_dummy1:
    # GUESS: What is the current stack pointer? [p $rsp]
    # OBSERVE: What is the return address? Compare it to `$after_dummy1` or `$rip`
    nop

    leave
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

