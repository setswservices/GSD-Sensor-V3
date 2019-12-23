
        .cdecls C,NOLIST, "msp430.h", "gsd_config.h"      ; Processor specific definitions

        .global  adcCopy2FRAM2            ; Declare symbol to be exported

RAM_EXTENDED0     .equ  10000H              ;10000H-23FFFF=82K FOR FR5989    
RAM_EXTENDED1     .equ  14000H
RAM_EXTENDED2     .equ  18000H
RAM_EXTENDED3     .equ  1C000H
RAM_EXTENDED4     .equ  20000H
RAM_FRAM          .equ  0BFC0H            ;BFC0 + 4000=FFC0 (EXTRA 16K)

	.ref	RAM_NoAUDIO_CNTS
	

        .sect ".text"                     ; Code is relocatable
adcCopy2FRAM2:   .asmfunc
;* r12   assigned to ADC12
    MOV.W     #__MSP430_BASEADDRESS_ADC12_B__,r12
;* r13   assigned to idx
    MOV.W     #0,r13			; idx=0
;* r14   assigned to data
;* r15	 assigned to address for FRAM2

; ====  Load chank #0 =====
; ==== Skip a first sample =====
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L12:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L12                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

; ==== Load to RAM_EXTENDED0 =====
	MOVA   #RAM_EXTENDED0,r15       ;EXTENDED FRAM PTR=0x10000
        BIS    #MC1,&TA0CTL               ;(RE)START CNTRA0             

$C$L13:   				; while(1) loop
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L14:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L14                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

        MOV.B  &ADC12MEM0,r14             ;r14=RESULT  (8 BITS)
        MOVX.B r14,0(r15)                  ;WRT BYTE TO EXTENDED MEMORY
        INCX.A r15                        ;INC PTR
        ADD.W  #1,r13	

        CMP.W     #16384,r13            ; [] |234| 
        JLO       $C$L13                ; [] |234| 
                                        ; [] |234| 
        BIC    #MC1+MC0,&TA0CTL         ; STOP CMP CNTRA0
        MOV    #RAM_NoAUDIO_CNTS+00H, r13
	MOV    &TA0R, r14		; RD TA0 counter
	MOV.W  r14, 0(r13)		; Save it
	ADD.W  r14, RAM_NoAUDIO_CNTS+12H 

	
   	xor.b  #16,&P9OUT		; Toggle P9.4, TP11
    
; ====  Load chank #1 =====
; ==== Skip a first sample =====
        MOV.W  #0,r13                	; [] |235| 
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L15:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L15                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

; ==== Load to RAM_EXTENDED1 =====
	MOVA   #RAM_EXTENDED1,r15       ;EXTENDED FRAM PTR=0x14000
        BIS    #MC1,&TA0CTL               ;(RE)START CNTRA0             

$C$L16:   				; while(1) loop
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L17:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L17                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

        MOV.B  &ADC12MEM0,r14             ;r14=RESULT  (8 BITS)
        MOVX.B r14,0(r15)                  ;WRT BYTE TO EXTENDED MEMORY
        INCX.A r15                        ;INC PTR
        ADD.W  #1,r13	

        CMP.W     #16384,r13            ; [] |234| 
        JLO       $C$L16                ; [] |234| 
                                        ; [] |234| 
        BIC    #MC1+MC0,&TA0CTL         ; STOP CMP CNTRA0
        MOV    #RAM_NoAUDIO_CNTS+02H, r13
	MOV    &TA0R, r14		; RD TA0 counter
	MOV.W  r14, 0(r13)		; Save it
	ADD.W  r14, RAM_NoAUDIO_CNTS+12H 

   	xor.b  #16,&P9OUT		; Toggle P9.4, TP11

; ====  Load chank #2 =====
; ==== Skip a first sample =====
        MOV.W  #0,r13                	; [] |235| 
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L18:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L18                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

; ==== Load to RAM_EXTENDED2 =====
	MOVA   #RAM_EXTENDED2,r15       ;EXTENDED FRAM PTR=0x18000
        BIS    #MC1,&TA0CTL               ;(RE)START CNTRA0             

$C$L19:   				; while(1) loop
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L20:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L20                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

        MOV.B  &ADC12MEM0,r14             ;r14=RESULT  (8 BITS)
        MOVX.B r14,0(r15)                  ;WRT BYTE TO EXTENDED MEMORY
        INCX.A r15                        ;INC PTR
        ADD.W  #1,r13	

        CMP.W     #16384,r13            ; [] |234| 
        JLO       $C$L19                ; [] |234| 
                                        ; [] |234| 
        BIC    #MC1+MC0,&TA0CTL         ; STOP CMP CNTRA0
        MOV    #RAM_NoAUDIO_CNTS+04H, r13
	MOV    &TA0R, r14		; RD TA0 counter
	MOV.W  r14, 0(r13)		; Save it
	ADD.W  r14, RAM_NoAUDIO_CNTS+12H 

   	xor.b  #16,&P9OUT		; Toggle P9.4, TP11

; ====  Load chank #3 =====
; ==== Skip a first sample =====
        MOV.W  #0,r13                	; [] |235| 
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L21:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L21                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

; ==== Load to RAM_EXTENDED3 =====
	MOVA   #RAM_EXTENDED3,r15       ;EXTENDED FRAM PTR=0x1C000
        BIS    #MC1,&TA0CTL               ;(RE)START CNTRA0             

$C$L22:   				; while(1) loop
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L23:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L23                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

        MOV.B  &ADC12MEM0,r14             ;r14=RESULT  (8 BITS)
        MOVX.B r14,0(r15)                  ;WRT BYTE TO EXTENDED MEMORY
        INCX.A r15                        ;INC PTR
        ADD.W  #1,r13	

        CMP.W     #16384,r13            ; [] |234| 
        JLO       $C$L22                ; [] |234| 
                                        ; [] |234| 
        BIC    #MC1+MC0,&TA0CTL         ; STOP CMP CNTRA0
        MOV    #RAM_NoAUDIO_CNTS+06H, r13
	MOV    &TA0R, r14		; RD TA0 counter
	MOV.W  r14, 0(r13)		; Save it
	ADD.W  r14, RAM_NoAUDIO_CNTS+12H 

   	xor.b  #16,&P9OUT		; Toggle P9.4, TP11

; ====  Load chank #4 =====
; ==== Skip a first sample =====
        MOV.W  #0,r13                	; [] |235| 
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L24:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L24                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

; ==== Load to RAM_EXTENDED4 =====
	MOVA   #RAM_EXTENDED4,r15       ;EXTENDED FRAM PTR=0x20000
        BIS    #MC1,&TA0CTL               ;(RE)START CNTRA0             

$C$L25:   				; while(1) loop
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L26:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L26                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

        MOV.B  &ADC12MEM0,r14             ;r14=RESULT  (8 BITS)
        MOVX.B r14,0(r15)                  ;WRT BYTE TO EXTENDED MEMORY
        INCX.A r15                        ;INC PTR
        ADD.W  #1,r13	

        CMP.W     #16384,r13            ; [] |234| 
        JLO       $C$L25                ; [] |234| 
                                        ; [] |234| 
        BIC    #MC1+MC0,&TA0CTL         ; STOP CMP CNTRA0
        MOV    #RAM_NoAUDIO_CNTS+08H, r13
	MOV    &TA0R, r14		; RD TA0 counter
	MOV.W  r14, 0(r13)		; Save it
	ADD.W  r14, RAM_NoAUDIO_CNTS+12H 

   	xor.b  #16,&P9OUT		; Toggle P9.4, TP11
;	.if DEBUGGING_MENU_ENABLED
;	.else
; ====  Load chank #5 =====
        MOV.W  #0,r13                	; [] |235| 

; ==== Load to RAM_FRAM =====
	MOV    #RAM_FRAM,r15       ;EXTENDED FRAM PTR=0x20000
        BIS    #MC1,&TA0CTL               ;(RE)START CNTRA0             

$C$L28:   				; while(1) loop
    	OR.B      #3,0(r12)             ; (OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC
$C$L29:    				; while() loop
        BIT    	  #ADC12BUSY,&ADC12CTL1 ; 
        JNE       $C$L29                ; 
        BIC    	  #ADC12ENC,&ADC12CTL0  ;RESET ENC FOR NXT CONVERSION

        MOV.B  &ADC12MEM0,r14             ;r14=RESULT  (8 BITS)
        MOV.B  r14,0(r15)                  ;WRT BYTE TO RAM_FRAM
        INC    r15                        ;INC PTR
        ADD.W  #1,r13	

        CMP.W     #16384,r13            ; [] |234| 
        JLO       $C$L28                ; [] |234| 
                                        ; [] |234| 
        BIC    #MC1+MC0,&TA0CTL         ; STOP CMP CNTRA0
        MOV    #RAM_NoAUDIO_CNTS+0AH, r13
	MOV    &TA0R, r14		; RD TA0 counter
	MOV.W  r14, 0(r13)		; Save it
	ADD.W  r14, RAM_NoAUDIO_CNTS+12H 

   	xor.b  #16,&P9OUT		; Toggle P9.4, TP11
;	.endif
	
   .if ($defined(__MSP430_HAS_MSP430XV2_CPU__) | $defined(__MSP430_HAS_MSP430X_CPU__))
        reta
   .else
        ret
   .endif
      .endasmfunc

        .end

