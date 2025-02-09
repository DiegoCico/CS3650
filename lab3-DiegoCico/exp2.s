
.global main

.text
start_of_text:


# BEGIN pasted code
main:
    enter $0, $0

    # Observe where the .text section starts
    # OBSERVE: where does .text start? [p start_of_text]

    # Observe where the .data section starts
    # OBSERVE: where does .data start? [info var start_of_data]

    # Observe addresses of labels in .text
    # GUESS: What are the addresses of labels in the .data section? [i var stuff]

    # Contents of address pointed to by `stuff`
    movq $stuff, %rax
    # GUESS: What's the 5th element of `stuff`? [x $rax + (4*8)]

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

