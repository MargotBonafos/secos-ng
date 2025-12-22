.intel_syntax noprefix
.global int32_stub
.extern int32_handler

int32_stub:
    cli
    pushad
    push ds
    push es
    push fs
    push gs

    mov eax, [esp + 48] 
    and eax, 3
    cmp eax, 0
    je noyau

    mov eax, esp
    push eax
    call int32_handler
    add esp, 4
    mov esp, eax

    pop gs
    pop fs
    pop es
    pop ds
    popad
    sti
    iret

noyau:
    pop gs
    pop fs
    pop es
    pop ds
    popad
    sti
    iret