.global main

.text

main:
  push %r12
  push %r13
  push %r14
  enter $8, $0

  mov %rdi, %r12
  mov %rsi, %r13

  movq $0, -8(%rbp)
  movq $1, %r14

loop:
  cmpq %r12, %r14
  jge loop_end

  movq (%r13, %r14, 8), %rdi
  call atol

  addq %rax, -8(%rbp)
  incq %r14
  jmp loop

loop_end:
  movq $sum_format, %rdi
  movq -8(%rbp), %rsi
  movq $0, %rax
  call printf

  leave
  pop %r14
  pop %r13
  pop %r12
  ret

.data

sum_format:
  .asciz "Sum: %ld\n"

