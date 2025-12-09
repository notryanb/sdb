.global main

.section .data
        hex_format:   .asciz "%#x" # encode the string into ascii with a null terminator
        float_format: .asciz "%.2f"

.section .text
.macro trap
        movq $62, %rax # kill is signal id 62
        movq %r12, %rdi # syscalls expect the first arg to be placed in rdi, the PID from the previous syscall is stored in r12, so copy from r12 to rdi
        movq $5, %rsi # the signal ID is the second arg to syscall and sigtrap is syscall id 5
        syscall
.endm

main:
        # Function Prologue: setup
        push %rbp # put the current callee frame base pointer at the top of the stack
        movq %rsp, %rbp # moves ("copies") a quad word stack pointer from rsp into rbp

        # Get PID
        movq $39, %rax # getpid is syscall id 39
        syscall # the syscall will store the result into rax
        movq %rax, %r12 # move the result from the getpid syscall into another register.
        trap

        # Print contents of rsi
        leaq hex_format(%rip), %rdi # Load effective address of 'hex_format' into rdi. (%rip) with a label uses instruction pointer relative addressing, which is a requirement for -pie flags
        movq $0, %rax # printf is a vararg function, and sysv abi expects rax to contain the number of vector registers involved in passing the args.
        call printf@plt #call printf with the procedural linkage table, which calls functions in shared libraries
        movq $0, %rdi 
        call fflush@plt # call fflush with 0 as the argument in rdi so it flushes all streams
        trap

        # Print contents of mm0
        movq %mm0, %rsi
        leaq hex_format(%rip), %rdi
        movq $0, %rax
        call printf@plt
        movq $0, %rdi
        call fflush@plt
        trap

        # Print contents of xmm0
        leaq float_format(%rip), %rdi
        movq $1, %rax
        call printf@plt
        movq $0, %rdi
        call fflush@plt
        trap

        # Function Epilogue: cleanup
        popq %rbp  # restore the callee's frame base pointer into rbp
        movq $0, %rax # put a 0 in the return register
        ret # return from the function
