;
;  AngelCode Scripting Library
;  Copyright (c) 2003-2011 Andreas Jonsson
;
;  This software is provided 'as-is', without any express or implied 
;  warranty. In no event will the authors be held liable for any 
;  damages arising from the use of this software.
;
;  Permission is granted to anyone to use this software for any 
;  purpose, including commercial applications, and to alter it and 
;  redistribute it freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you 
;     must not claim that you wrote the original software. If you use
;     this software in a product, an acknowledgment in the product 
;     documentation would be appreciated but is not required.
;
;  2. Altered source versions must be plainly marked as such, and 
;     must not be misrepresented as being the original software.
;
;  3. This notice may not be removed or altered from any source 
;     distribution.
;
;  The original version of this library can be located at:
;  http://www.angelcode.com/angelscript/
;
;  Andreas Jonsson
;  andreas@angelcode.com
;

.code
PUBLIC CallX64

; asQWORD CallX64(const asQWORD *args, const asQWORD *floatArgs, int paramSize, asQWORD func)

CallX64 PROC FRAME

	; PROLOG

	; We must save preserved registers that are used
	; TODO: No need to save unused registers
	push rbp
.pushreg rbp
	sub rsp, 050h
.allocstack 050h
	mov rbp, rsp
.setframe rbp, 0
.endprolog

	; Move function param to non-scratch register
	mov r10, r9		; r10 = function

	; Allocate space on the stack for the arguments
	; Make room for at least 4 arguments even if there are less. When
    ; the compiler does optimizations for speed it may use these for 
	; temporary storage.
	; Make sure the stack pointer is 16 byte aligned so the
	; whole program optimizations will work properly
	sub rsp, r8  ; r8 = paramSize
	sub rsp, 32
	and rsp, -16 ; -16 is ~15
		
	; Negate the 4 params from the size to be copied
	sub r8d, 32
	jle  callfunc ; Jump if less than or equal result

	; Now copy all remaining params onto stack allowing space for first four
	; params to be flushed back to the stack if required by the callee.
	; Put the stack pointer into r9 while leaving space for first 4 args on stack 
	; Move params to non-scratch registers
	lea rax, [rcx+32]	; rax = pArgs+4 
	lea r9,  [rsp+32]	

copyoverflow:
	mov r11, qword ptr [rax]	; Read param from source stack into r11
	mov qword ptr [r9], r11	    ; Copy param to real stack
	add r9, 8					; Move virtual stack pointer
	add rax, 8					; Move source stack pointer
	sub r8d, 8					; Decrement remaining count
	jnz copyoverflow			; Continue if more params
	
callfunc:
	; Copy float arguments

	movlpd xmm0, qword ptr [rdx]     ; rdx = floatArgs
	movlpd xmm1, qword ptr [rdx + 8]
	movlpd xmm2, qword ptr [rdx + 16]
	movlpd xmm3, qword ptr [rdx + 24]

	; Copy arguments from script stack to application stack
	; Order is (first to last):
	; rcx, rdx, r8, r9 & everything else goes on stack
	mov r9,  qword ptr [rcx + 24] ; rcx = args
	mov r8,  qword ptr [rcx + 16]
	mov rdx, qword ptr [rcx + 8]
	mov rcx, qword ptr [rcx]
	
	; Call function
	call r10
	
	; EPILOG: Restore stack & preserved registers
	lea rsp, [rbp + 050h]
	pop rbp

	; return value in RAX
	ret

CallX64 ENDP


PUBLIC GetReturnedFloat

; asDWORD GetReturnedFloat()

GetReturnedFloat PROC FRAME

	; PROLOG: Store registers and allocate stack space
	
	sub rsp, 8   ; We'll need 4 bytes for temporary storage (8 bytes with alignment)
.allocstack 8
.endprolog

	; Move the float value from the XMM0 register to RAX register
	movss dword ptr [rsp], xmm0
	mov   eax, dword ptr [rsp]
	
	; EPILOG: Clean up
	
	add rsp, 8

	ret

GetReturnedFloat ENDP


PUBLIC GetReturnedDouble

; asDWORD GetReturnedDouble()

GetReturnedDouble PROC FRAME

	; PROLOG: Store registers and allocate stack space
	
	sub rsp, 8	; We'll need 8 bytes for temporary storage
.allocstack 8
.endprolog

	; Move the double value from the XMM0 register to the RAX register
	movlpd qword ptr [rsp], xmm0
	mov    rax, qword ptr [rsp]
	
	; EPILOG: Clean up
	
	add rsp, 8
	
	ret
	
GetReturnedDouble ENDP

END