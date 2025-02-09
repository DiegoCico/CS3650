.global main

.text

main:
  push %r12
  push %r13
  push %r14
  enter $8, $0

  mov %rdi, %r12
  mov %rsi, %r13

  movq %r12, %r14
  decq %r14

loop:
  cmpq $0, %r14
  jl loop_end

  movq (%r13, %r14, 8), %rdi
  call puts

  decq %r14
  jmp loop

loop_end:
  leave
  pop %r14
  pop %r13
  pop %r12
  ret

