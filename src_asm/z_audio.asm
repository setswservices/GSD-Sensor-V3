
	.cdecls C,NOLIST, "msp430.h", "gsd_asm.h", "gsd_config.h"      ; Processor specific definitions

	.global ANALYZE_AUDIO_EVENT_FINAL

AX0 .equ R4 
BX0 .equ R5 
CX0 .equ R6 
DX0 .equ R7 
BP0 .equ R8 

AX .equ R9
BX .equ R10
CX .equ R11
DX .equ R12
BP .equ R13
SI .equ R14
DT .equ R15

YES           .equ    0FFH  
NO            .equ    00H

LETTER_V      .equ 56H

RAM_EXTENDED     .equ  10000H              ;10000H-23FFFF=82K FOR FR5989    
RAM_FRAM           .equ  0BFC0H            ;BFC0 + 4000=FFC0 (EXTRA 16K)
No_AUDIO_PTS_LIVE     .equ 16384          ;16384 PTS FOR ALL CNTR WINDOWS (4000H)      
AUDIO_BIAS_MIDPOINT   .equ 80H            ;USED
ZERO_CLAMP_LEVEL      .equ  04
No_UPPER_WINDOWS .equ 05                  ;560ms + 1 LOWER=672ms
CODE_CALB_ALRM_TH   .equ  40H


xENABLE_TIMING_BIT    .equ  NO            ;ONLY FOR CHKOUT OR DEBUGGING
;xCHK_MULTIPLE_SHOTS   .equ  NO          ;ADDED 11/13/15---NOT CHKD OUT
xZERO_CLAMP_WF        .equ  YES           ;ENABLE OR DISABLE  (USED IN "FIND_VARIANCE")
xUSE_ADJVAR         .equ  YES   

	.if xCHK_MULTIPLE_SHOTS                  ;ADDED NOV 2015
No_CNT_WINDOWS   .equ 04                  ;# 224ms WINDOWS AFTER 672ms A/D WINDOW
	.else                                    
No_CNT_WINDOWS   .equ 02                  ;# 224ms WINDOWS AFTER 672ms A/D WINDOW
	.endif

	.sect ".data"
	.align 2
;;;;;;;;;;;;;;;;;;;;;;;; AUDIO ;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; AUDIO ;;;;;;;;;;;;;;;;;;;;;
RAM_AUDIO_VAR0     .space 04H
RAM_AUDIO_VAR1     .space 04H
RAM_AUDIO_VAR01    .space 04H   ;KEEP ORDER
RAM_AUDIO_ADJ_VAR  .space 04H   ;    
RAM_AUDIO_CALB_VAR .space 04H

RAM_EVENTNo       .space 02H
RAM_NoALRMS       .space 02H
RAM_ALRM_VAR      .space 04H
RAM_SCRATCH  .space 20H   ;GENERAL PURPOSE(SUBS & DEBUG)

	.global RAM_AUDIO_VAR0,RAM_AUDIO_VAR1,RAM_AUDIO_VAR01,RAM_AUDIO_ADJ_VAR,RAM_NoALRMS,RAM_EVENTNo

;**************************** STB & ST *************************
; STORE BYTE/WORD @ PTR                                        *
; EXAMPLE: STB   AX,BP        BYTE                             *
;        : ST    AX,BP        WORD                             *
;***************************************************************
STB:          .macro  SRC,PTR
              MOV.B  SRC,0(PTR)
              INC    PTR
              .endm

ST:           .macro  SRC,PTR
              MOV    SRC,0(PTR)
              INC    PTR
              INC    PTR
              .endm
;***************************************************************

;**************************** DJNZB ****************************
; NOTE: DJNZ.B WILL NOT WORK                                   *
; EXAMPLE: DJNZ AX,LABEL                                       *
;***************************************************************
DJNZB:        .macro  SRC,LABEL
              DEC.B  SRC
              JNZ    LABEL
              .endm
;***************************************************************
;**************************** DJNZB ****************************
; NOTE: DJNZ.B WILL NOT WORK                                   *
; EXAMPLE: DJNZ AX,LABEL                                       *
;***************************************************************
DJNZB:        .macro  SRC,LABEL
              DEC.B  SRC
              JNZ    LABEL
              .endm
;***************************************************************
;*************************** DJNZ ******************************
; EXAMPLE: DJNZ AX,LABEL                                       *
;***************************************************************
DJNZ:         .macro  SRC,LABEL
              DEC    SRC
              JNZ    LABEL
              .endm
;***************************************************************


	.if xCHK_MULTIPLE_SHOTS                  ;ADDED NOV 2015
No_CNT_WINDOWS   .equ 04                  ;# 224ms WINDOWS AFTER 672ms A/D WINDOW
	.else                                    
No_CNT_WINDOWS   .equ 02                  ;# 224ms WINDOWS AFTER 672ms A/D WINDOW
	.endif

	.ref	RAM_NoAUDIO_CNTS,RAM_ALRM_FLG,RAM_BKGD_VAR
	.ref DSPLY_CNTRS,DSPLY_CHAR,DSPLY_VARIANCE,DSPLY_ADJ_VAR
	.ref DSPLY_EVENTNo,DSPLY_VAR_LMT,DSPLY_MSG_ABOVE,DSPLY_MSG_BELOW
	.ref getRAMn_MODE,getRAMn_DEBUG,getRAMn_VAR_LMT0,getRAMn_VAR_LMT2


	.sect ".text"                     ; Code is relocatable

;OK
;************************ START_TIMERA1_LMT ********************
; START MODE=COUNT UP TO PRESET & RESTART @ 0                  *
;***************************************************************
              .def START_TIMERA1_LMT 

START_TIMERA1_LMT:
              BIS    #TACLR,&TA1CTL       ;CLR CNTR                 
              BIS    #MC0,&TA1CTL         ;ENABLE COUNT UP TO CCR0         
              RET 
;***************************************************************

;OK
;************************** STOP_TIMERA1 ***********************
; STOP TIMER, MC1=0,MC0=0                                      *
;***************************************************************
              .def STOP_TIMERA1

STOP_TIMERA1: BIC    #MC1+MC0+TAIFG,&TA1CTL     ;STOP CNTR                
              RET 
;***************************************************************

;OK
;*************************** RD_CNTRA0 *************************
; RAM_CNTRA=UPPER 16 BITS, USE "INT_CNTRA_TAIV"                *
; EXIT  with: AX=RESULT                                        *
;***************************************************************
              .def RD_CNTRA0

RD_CNTRA0:    MOV    &TA0R,AX                                              
              RET 
;***************************************************************

;********************** HW_MPY_1616 ********************
; WORKS                                                *
; HARDWARE MULITPLY UNSIGNED                           *
; ENTER with:AX=OPERAND 1                              *
;           :BX=OPERAND 2                              *
; EXIT  with:BX=RESULT-MSW                             *
;           :AX=RESULT=LSW                             *
;*******************************************************
		.def        HW_MPY_1616 

HW_MPY_1616:  MOV    AX,&MPY                ;LOAD OPERAND 1            
              MOV    BX,&OP2               ;LOAD OPERAND 2 & START MULTIPLICATION           
              MOV    &RESHI,BX              ;RESULT-MSW    
              MOV    &RESLO,AX              ;RESULT-LSW    
              RET 
;*******************************************************
;********************** HW_MPY_3232 ********************
; F5X FAMILY                                           *
; WORKS                                                *
;     BX,AX * BX0,AX0=DX,CX,BX,AX                      *
; HARDWARE MULITPLY UNSIGNED                           *
; ENTER with:AX=OPERAND 1-LSW                          *
;           :BX=OPERAND 1-MSW                          *
;           :AX0=OPERAND 2-LSW                         *
;           :BX0=OPERAND 2-MSW                         *
; EXIT  with:BX=RESULT-MSW                             *
;           :AX=RESULT=LSW                             *
;*******************************************************
		.def        HW_MPY_3232                ;BX,AX * BX0,AX0=DX,CX,BX,AX    

HW_MPY_3232:  MOV    AX,&MPY32L          ;LOAD OPERAND 1-LSW        
              MOV    BX,&MPY32H          ;LOAD OPERAND 1-MSW                              
              MOV    AX0,&OP2L           ;LOAD OPERAND 2-LSW        
              MOV    BX0,&OP2H           ;LOAD OPERAND 2 & START MULTIPLICATION           
              MOV    &RES3,DX            ;RESULT-MSW    
              MOV    &RES2,CX            ;              
              MOV    &RES1,BX            ;              
              MOV    &RES0,AX            ;RESULT-MSW    
              RET 
;*******************************************************

;WORKS
;********************** MUL_1616 ***********************
; UNSIGNED 16 x 16 MULTIPLY (METERING BOOK, PG 83)     *
;                           (SOFTWARE BOOK, PG 4-3)    *
; # EXECUTION CYCLES: 132-164                          *
;                                                      *
; ENTER with:BP=MULTIPLICAND                           *
;           :DX=MULTIPLIER=00                          *
;           :CX=MULTIPLIER                             *
; EXIT  with:BX=RESULT-MSB                             *
;           :AX=      -LSB                             *
; USES: AX,BX,CX,DX,BP,SI                              *
;*******************************************************
		.def        MUL_1616,MUL_1616x


;EUIVALENT TO "HW_MPY_1616"  AX*BX=BX,AX
MUL_1616x:    MOV    BX,BP
              MOV    AX,CX
              CALL   #MUL_1616
              RET

;BP*CX=BX,AX
MUL_1616:     CLR    AX                  ;RESULT   
              CLR    BX                  ;   
              CLR    SI                  ;SCRATCH-BIT TEST
              CLR    DX                  ;MSBs MULITPLIER
              MOV    #1,SI                                    
L$162:        BIT    SI,BP               ;TEST BIT
              JZ     L$161               ;IF 0 DO NOTHING
              ADD    CX,AX               ;IF 1 ADD MULTIPLIER TO RESULT
              ADDC   DX,BX
L$161:        RLA    CX                  ;MULTIPLIER x 2
              RLC    DX
              RLA    SI                  ;NXT BIT TO TEST
              JNC    L$162
              RET 
;*******************************************************
;WORKS
;*************************** MUL_3232 **************************
; REPEATEDLY HALVE THE MULITPLICAND (DISREGARD REMAINDERS)     *
;   AND DOUBLE THE MULITPLIER UNTIL THE FORMER IS 1. FOR EVERY *
;   ODD MULITPLICAND, ADD THE RESPECTIVE MULTIPLIER.           *
;                                                              *
; ENTER with:[SI]=MULTIPLICAND  (DWORD)                        *
;           :[DT]=MULTIPLIER    (DWORD)                        *
;           :[BP]=RESULT        (4 WORDS)                      *
;***************************************************************
              .def MUL_3232
;             EXTERN RAM_SCRATCH,SND_WORD,WAIT_KYBD,SND_SP,SND_CR_LF

MUL_3232: 
;             MOV    #RAM_SCRATCH+00,SI     ;MULTIPLICAND (DWORD)
;             MOV    #RAM_SCRATCH+04H,DT    ;MULTIPLIER   (DWORD)
;             MOV    #RAM_SCRATCH+10H,BP    ;RESULT

;             MOV    #0FFFFH,RAM_SCRATCH+06
;             MOV    #6000H,RAM_SCRATCH+02

              CLR    0(BP)               ;CLR RESULT
              CLR    2(BP)
              CLR    4(BP)
              CLR    6(BP)
              CLR    4(DT)               ;CLR UPPER MULTIPLIER
              CLR    6(DT)
              JMP    START_3232
L$1:          RLA    0(DT)               ;MULTIPLIER x 2
              RLC    2(DT)
              RLC    4(DT)
              RLC    6(DT)
              CLRC
              RRC    2(SI)               ;DIVIDE MULTIPLICAND BY 2
              RRC    0(SI)
START_3232:   BIT    #BIT0,0(SI)         ;CHK IF ODD
              JZ     NO_ADD
              ADD    0(DT),0(BP)         ;IF ODD ADD MULTIPLIER TO RESULT
              ADDC   2(DT),2(BP)
              ADDC   4(DT),4(BP)
              ADDC   6(DT),6(BP)
NO_ADD:       TST    2(SI)               ;CHK IF DONE
              JNZ    L$1    
              TST    0(SI) 
              JNZ    L$1    
              RET         

;             CALL   #SND_CR_LF
;             CALL   #SND_CR_LF
;             MOV    RAM_SCRATCH+16H,AX
;             CALL   #SND_WORD
;             CALL   #SND_SP
;             MOV    RAM_SCRATCH+14H,AX
;             CALL   #SND_WORD
;             CALL   #SND_SP
;             MOV    RAM_SCRATCH+12H,AX
;             CALL   #SND_WORD
;             CALL   #SND_SP
;             MOV    RAM_SCRATCH+10H,AX
;             CALL   #SND_WORD
;             CALL   #WAIT_KYBD
;             RET
;*******************************************************
;WORKS
;*************************** MUL_3232x *************************
; REPEATEDLY HALVE THE MULITPLICAND (DISREGARD REMAINDERS)     *
;   AND DOUBLE THE MULITPLIER UNTIL THE FORMER IS 1. FOR EVERY *
;   ODD MULITPLICAND, ADD THE RESPECTIVE MULTIPLIER.           *
;                                                              *
; ENTER with:DX0=MULTIPLICAND   (2 WORDS=DWORD)                *
;           :CX0=MULTIPLICAND   (2 WORDS=DWORD)                *
;           :BX0=MULTIPLIER     (2 WORDS=DWORD)                *
;           :AX0=MULTIPLIER     (2 WORDS=DWORD)                *
; EXIT  with:DX=RESULT                                         *
;           :CX=RESULT                                         *
;           :BX=RESULT                                         *
;           :AX=RESULT                                         *
;***************************************************************
              .def MUL_3232x
;             EXTERN RAM_SCRATCH         ;RAM_SCRATCH MUST BE DS=20H

MUL_3232x:    MOV    CX0,RAM_SCRATCH
              MOV    DX0,RAM_SCRATCH+2
              MOV    AX0,RAM_SCRATCH+4
              MOV    BX0,RAM_SCRATCH+6
              MOV    #RAM_SCRATCH+00,SI     ;MULTIPLICAND (DWORD)
              MOV    #RAM_SCRATCH+04H,DT    ;MULTIPLIER   (DWORD)
              MOV    #RAM_SCRATCH+10H,BP    ;RESULT

              CALL   #MUL_3232

              MOV    RAM_SCRATCH+10H,AX
              MOV    RAM_SCRATCH+12H,BX
              MOV    RAM_SCRATCH+14H,CX
              MOV    RAM_SCRATCH+16H,DX
              RET
;*******************************************************
;********************** DIV_3232y **********************
; UNSIGNED 32 x 32 DIVIDE (AP BOOK PG 5-24)            *
; DX0,CX0/BX0,AX0=BX,AX    (DX,CX)                     *
;                                                      *
; NORMAL DIVIDE                                        *
; EXAMPLE: 9/4  (AX=2, CX=1)                           *
;        : 9/7  (AX=1, CX=2)                           *
;        :11/3  (AX=3, CX=2)                           *
;                                                      *
; FRACTIONAL                                           *
; EXAMPLE: 9/4  (AX=2, CX=1F4 (500)      2.5           *
;        : 9/7  (AX=1, CX=11D (285)      1.285         *
;        :11/3  (AX=3, CX=29A (666)      3.666         *
;                                                      *
; ENTER with:DX0=DIVIDEND       (32 BITS)              *
;           :CX0=DIVIDEND                              *
;           :BX0=DIVISOR        (32 BITS)              *
;           :AX0=DIVISOR                               *
;           :BX=RESULT          (32 BITS)              *
;           :AX=RESULT                                 *
;           :DX=REMAINDER       (32 BITS)              *
;           :CX=REMAINDER                              *
;*******************************************************
;	.def        DIV_3232y,DIV_3216y_ROUNDED

DIV_3232y:    CLR    BX                  ;CLR RESULT
              CLR    AX     
              CLR    DX                  ;CLR REMAINDER
              CLR    CX      

              PUSH   BP
              MOV.B  #32,BP              ;CNTR                                                             
L$461:        RLA    CX0                 ;SHIFT 1 BIT OF DIVIDEND INTO REMAINDER
              RLC    DX0                 ;                 
              RLC    CX                  ;                 
              RLC    DX                  ;                 
              CMP    BX0,DX              ;IS SUBTRACTION NECESSARY
              JLO    L$464               ;NO   (SAME AS JNC <)
              JNE    L$463               ;YES
              CMP    AX0,CX              ;EQUAL?
              JLO    L$464               ;JNC <
L$463:        SUB    AX0,CX              ;YES, SUBTRACT DIVISOR FROM REMAINDER
              SUBC   BX0,DX       
L$464:        RLC    AX                  ;RESULT
              RLC    BX     
              DJNZB  BP,L$461
              POP    BP
              RET 

DIV_3232z:    CALL   #DIV_3232y          ;DX0,CX0/BX0,AX0=BX,AX  (DX,CX)
;FIND FRACTIONAL REMAINDER=SUBTRACTION REMAINDER * 1000/DIVISOR
              PUSH   AX                  ;SAVE INTEGER
              PUSH   BX                  ;SAVE INTEGER
              PUSH   AX0                 ;SAVE DIVISOR
              PUSH   BX0                 ;SAVE DIVISOR
              MOV    BX,DX0
              MOV    AX,CX0
              CLR    BX0
              MOV    #1000,AX0
              CALL   #MUL_3232x          ;DX0,CX0*BX0,AX0=DX,CX,BX,AX
              MOV    BX,DX0
              MOV    AX,CX0
              POP    BX0                 ;RETREIVE DIVISOR
              POP    AX0
              CALL   #DIV_3232y          ;DX0,CX0/BX0,AX0=BX,AX, (DX,CX=REMAINDER)
              MOV    AX,CX               ;CX=FRACTIONAL REMAINDER
              POP    BX
              POP    AX
              RET

DIV_3216y_ROUNDED:    
              CALL   #DIV_3232z          ;DX0,CX0/BX0,AX0=BX,AX  (DX,CX)
              CMP    #500,CX
              JNC    NO_ROUNDUPy
              INC    AX
              ADDC   #0000H,BX 
NO_ROUNDUPy:  RET                        ;RESULT: BX,AX
;*******************************************************

;WORKS
;********************** DIV_3216 ***********************
; UNSIGNED 32 x 16 DIVIDE (SOFTWARE BOOK, PG 4-8)      *
; # EXECUTION CYCLES: 237-242                          *
; CX,BX/DX=AX                                          *
; EXAMPLE: 9/4  (AX=2, CX=1)                           *
;        : 9/7  (AX=1, CX=2)                           *
;        :11/3  (AX=3, CX=2)                           *
;                                                      *
; ENTER with:CX=DIVIDEND-MSB                           *
;           :BX=   :    -LSB                           *
;           :DX=DIVISOR                                *
; EXIT  with:AX=RESULT                                 *
;           :CX=REMAINDER  (TRUE REMAINDER IN HEX)     *
;           :CY=0, OK                                  *
;           :CY=1, QUOTIENT > 16 (DIVIDEND > DIVISOR)  *
;                                                      *
;NOTE: NEED TO CHK FOR CY FOR VALID RESULT             *
;*******************************************************
;	.def        DIV_3216

DIV_3216:     PUSH   BP
              CLR    AX                  ;RESULT   
              TST    DX                  ;CHK DIVISOR=0
              JZ     EE_DIVx  
              MOV    #17,BP              ;CNTR                                                             
DIV1:         CMP    DX,CX               ;DIVIDEND > DIVISOR
              JLO    DIV2                ;NO                   
              SUB    DX,CX               ;NO, DIVIDEND=DIVIDEND-DIVISOR
DIV2:         RLC    AX                  ;CY=0, IF DIVIDEND < DIVISOR
              JC     DIV4                ;IF CY, END                     
              DEC    BP                                                       
              JZ     DIV3                ;DONE
              RLA    BX                  ;DOUBLE DIVIDEND
              RLC    CX
              JNC    DIV1                                       
              SUB    DX,CX
              SETC   
              JMP    DIV2
DIV3:         CLRC
DIV4:         POP    BP
              RET 

EE_DIVx:      CLR    CX                  ;AX & CX=0
              SETC                       ;ERR FLG
              POP    BP
              RET
;*******************************************************

;WORKS
;********************** DIV_3216z **********************
; UNSIGNED 32 x 16 DIVIDE (SOFTWARE BOOK, PG 4-8)      *
; # EXECUTION CYCLES: 237-242                          *
; CX,BX/DX=AX                                          *
; EXAMPLE: 9/4  (AX=2, CX=1F4 (500)      2.5           *
;        : 9/7  (AX=1, CX=11D (285)      1.285         *
;        :11/3  (AX=3, CX=29A (666)      3.666         *
;                                                      *
; ENTER with:CX=DIVIDEND-MSB                           *
;           :BX=   :    -LSB                           *
;           :DX=DIVISOR                                *
; EXIT  with:AX=RESULT                                 *
;           :CX=REMAINDER (FRACTIONAL IN HEX)          *
;           :CY=0, OK                                  *
;           :CY=1, QUOTIENT > 16 (DIVIDEND > DIVISOR)  *
;           ;TO DISPLAY   (AX.CX)                      *
;                                                      *
;NOTE: NEED TO CHK FOR CY FOR VALID RESULT             *
;*******************************************************
;	.def        DIV_3216z,DIV_3216_ROUNDED

DIV_3216z:    CALL   #DIV_3216           ;AX=RESULT, CX=SUBTRACTION REMAINDER
;FIND FRACTIONAL REMAINDER=SUBTRACTION REMAINDER * 1000/DIVISOR
              PUSH   AX                  ;SAVE INTEGER
              PUSH   DX                  ;SAVE DIVISOR
              MOV    #1000,BP
              CALL   #MUL_1616           ;BP*CX=BX,AX
              MOV    BX,CX
              MOV    AX,BX
              POP    DX
              CALL   #DIV_3216           ;CX,BX/DX=AX, (CX=REMAINDER)
              MOV    AX,CX               ;CX=FRACTIONAL REMAINDER
              POP    AX
              RET


DIV_3216_ROUNDED:    
              CALL   #DIV_3216z
              CMP    #500,CX
              JNC    NO_ROUNDUP
              INC    AX
NO_ROUNDUP:   RET
;*******************************************************
	

;CALLED FROM "ANALYZE_AUDIO_VARIANCE"
;CALLED FROM "ANALYZE_AUDIO_EVENT_FINAL"
;************************** FIND_VARIANCE **********************
;MAX VARIANCE=(A/D-BIAS)^2 * # A/D PTS                         *
;            = 128*128*16384                                   *
;            = 268,435,456  (1000 0000 HEX)                    *
;                                                              *
;ENTER with: AX=DATA                                           *
;EXIT  with: VAR=CX0,BX0      (DWORD)                          *
;            (USES AX0,BX0,CX0,BX)                             *
;***************************************************************
;              .ref HW_MPY_1616   

FIND_VARIANCE:
              CLR    CX0                          ;RESULT-MSW
              CLR    BX0                          ;RESULT-LSW

              MOV.B  #AUDIO_BIAS_MIDPOINT,AX0   ;BIAS LEVEL
;             MOV.B  RAMn_AUDIO_BIAS,AX0        ;BIAS LEVEL

              CMP.B  AX0,AX              ;AX0=BIAS, AX=A/D DATA VALUE
              JNC    BELOW_AVG           ;<
ABOVE_AVG:    SUB.B  AX0,AX              ;VALUE-AVG/BIAS)  (AX=AX-AX0)
	.if xZERO_CLAMP_WF
;             BIT.B  #BIT1,RAMn_MODE
;             JZ     NO_CLAMP_CHKx
              CMP.B  #ZERO_CLAMP_LEVEL,AX
;             CMP.B  RAMn_ZERO_CLAMP_LEVEL,AX
              JNC    EXIT_ZERO           ;< 
NO_CLAMP_CHKx:
	.endif
              MOV.B  AX,BX               ;BX=AX
              JMP    FINISH_VAR             

;BELOW BIAS
BELOW_AVG:    MOV.B  AX0,BX              ;AX0=BIAS=BX
              SUB.B  AX,BX               ;BIAS-VALUE  (BX-AX=BX)
              MOV.B  BX,AX               ;AX=BX
	.if xZERO_CLAMP_WF
;             BIT.B  #BIT1,RAMn_MODE
;             JZ     NO_CLAMP_CHKy
              CMP.B  #ZERO_CLAMP_LEVEL,AX
;             CMP.B  RAMn_ZERO_CLAMP_LEVEL,AX
              JNC    EXIT_ZERO           ;<   
NO_CLAMP_CHKy:
	.endif
FINISH_VAR:   
              CALL   #HW_MPY_1616        ;SQUARE VALUE (AX*BX=BX,AX)
              ADD    AX,BX0              ;RESULT-LSW
              ADDC   BX,CX0              ;      -MSW
              JC     OUT_OF_RANGE
              RET

EXIT_ZERO:
              RET


OUT_OF_RANGE: MOV    #0FFFFH,BX0         ;MAX FLG
              MOV    #0FFFFH,CX0         ;MAX FLG
              RET
;***************************************************************


;CALLED FROM "ACTIVE_SHOOTER_LIVE" (NOT USED)
;CALLED FROM "ANALYZE_AUDIO_EVENT_FINAL"
;********************** ANALYZE_AUDIO_VARIANCE *****************
;CALLED FROM MENU & SHOOTER LIVE                               *
;                                                              *
;MAX VARIANCE=(A/D-BIAS)^2 * # A/D PTS                         *
;            = 128*128*16384                                   *
;            = 268,435,456  (1000 0000 HEX)                    *
;                                                              *
;ENTER with: BP=DATA PTR                                       *
;EXIT  with: CX0  (MSW)                                        *
;          : DX   (LSW)                                        *
;***************************************************************
ANALYZE_AUDIO_VARIANCE:
;             MOV    #MSG_AUDIOx,SI
;             CALL   #SND_NULL
              MOV    #No_AUDIO_PTS_LIVE,BP0   ;# PTS=16,384  
;             MOV    #No_AUDIO_PTS,BP0   ;# PTS=1024 

;             CLR    RAM_AUDIO_VAR0+00   ;CLR RESULT
;             CLR    RAM_AUDIO_VAR0+02   ;CLR RESULT

ANALYZE_AUDIO_VARIANCEx:
              CLR    DX0                 ;CLR LSW
              CLR    DX                  ;CLR MSW

;             MOVA   #RAM_EXTENDED,SI    
;DEBUG
;             MOV    #RAM_AUDIOarray,BP
;             MOV    #04H,BP0            ;# PTS=04
NXT_VAR_PT:
              MOVX.B @SI+,AX   
;             STB    AX,BP               ;DEBUG
              CALL   #FIND_VARIANCE      ;FROM AX, VAR=CX0,BX0   "AUDIO.ASM"                 
;             ADD    BX0,RAM_AUDIO_VAR   ;LSW
;             ADDC   CX0,RAM_AUDIO_VAR+2 ;   
              ADD    BX0,DX              ;LSW
              ADDC   CX0,DX0             ;MSW
              DJNZ   BP0,NXT_VAR_PT      ;# TOTAL VAR PTS
              RET


;MSG_AUDIOx:   DB     "-CALC VAR:"
	.align 2
;***************************************************************

;CALLED FROM "ACTIVE_SHOOTER_LIVE"  (NOT USED)
;CALLED FROM "ANALYZE_AUDIO_EVENT_FINAL"
;********************** ANALYZE_ZERO_CROSSINGS *****************
;CONCEPT                                                       *
;   -FIRST CNT WINDOW OF 224ms SAME AS VARIANCE CALCULATION    *
;   -TAKE PERCENT DIFFERENCE OF WINDOW CNTS TO VARIANCE CNTS   *
;       & TAKE RESULT TIMES VARIANCE AND ADD RESULT TO VARIANCE*
;                                                              *
;ENTER with: RAM_AUDIO_VAR0+00        (LSW)--4 16KB=64KB, 448ms*
;          : RAM_AUDIO_VAR0+02        (MSW)                    *
;          : RAM_AUDIO_VAR1+00        (LSW)--2 16KB=32KB, 224ms*
;          : RAM_AUDIO_VAR1+02        (MSW)                    *
;          : RAM_AUDIO_VAR01+00       (LSW)--6 16KB=96KB, 672ms*
;          : RAM_AUDIO_VAR01+02       (MSW)                    *
;          : RAM_NoAUDIO_CNTS+00H     (VAR A/D WINDOW 1, 224ms *
;          : RAM_NoAUDIO_CNTS+02H     (VAR A/D WINDOW 2, 224ms *
;          : RAM_NoAUDIO_CNTS+04H     (VAR A/D WINDOW 3, 224ms *
;          : RAM_NoAUDIO_CNTS+06H     (TIMER WINDOW 4,   224ms *
;          : RAM_NoAUDIO_CNTS+08H     (TIMER WINDOW 5,   224ms *
;          : RAM_NoAUDIO_CNTS+0AH     (TOTAL CNTS)             *
;          : RAM_NoAUDIO_CNTS+0EH     (TOTAL CNTS)             *
;EXIT  with: RAM_AUDIO_ADJ_VAR        (FULL VARIANCE DWORD)    *
;          : RAM_AUDIO_ADJ_VAR        (ADJ  VARIANCE DWORD)    *
;***************************************************************
;              .ref DIV_3216_ROUNDED,DIV_3232y   

ANALYZE_ZERO_CROSSINGS:
;CHKOUT 
;             MOV    #1000H,RAM_AUDIO_VAR+00H
;             MOV    #0100H,RAM_AUDIO_VAR+02H
;             MOV    #1234H,RAM_NoAUDIO_CNTS+00H
;             MOV    #1234H,RAM_NoAUDIO_CNTS+02H
;             MOV    #0000H,RAM_NoAUDIO_CNTS+04H
;             MOV    #0000H,RAM_NoAUDIO_CNTS+06H
;             CALL   #DSPLY_VARIANCE

;MOVE VARIANCE0+1 TO ADJUSTED VARIANCE AS DEFAULT  
              MOV    RAM_AUDIO_VAR01,RAM_AUDIO_ADJ_VAR      
              MOV    RAM_AUDIO_VAR01+02H,RAM_AUDIO_ADJ_VAR+02H

;FIND RATIO FOR A/D CROSSING VS TIMER WINDOW CROSSING
;USE RATIO THEN TO MULTIPLY x A/D VARIANCE
;COMPARE #CNTS FROM VARIANCE WINDOW3 TO CNT#4
              TST    RAM_NoAUDIO_CNTS+04H   ;LAST A/D WINDOW---WINDOW #3
              JZ     EXIT_CROSSINGS
              TST    RAM_NoAUDIO_CNTS+06H   ;FIRST TIMER WINDOW--224ms, WINDOW#4
              JZ     EXIT_CROSSINGS
              CMP    RAM_NoAUDIO_CNTS+04H,RAM_NoAUDIO_CNTS+06H
              JNC    ADJ_VAR3            ;WINDOW CNTS4 < A/D CNTS 3
              JEQ    EQUAL_VAR3          ;=           =
;DIVIDE WINDOW CNTS4 BY A/D CNTS3        ;WINDOW CNTS4 > A/D CNTS 3
              CLR    CX
              MOV    RAM_NoAUDIO_CNTS+06H,BX ;CNTS #4--FIRST TIMER WINDOW
              MOV    RAM_NoAUDIO_CNTS+04H,DX ;VARIANCE A/D CNTS FOR LAST 224ms
              CALL   #DIV_3216_ROUNDED       ;CX,BX/DX=AX   
;TAKE CNT RATIO MULTIPLIER * A/D VARIANCE FOR 2ND 16K PTS (11
              CLR.B  BX
              MOV    RAM_AUDIO_VAR1+00H,AX0    ;A/D VAR LAST 224ms
              MOV    RAM_AUDIO_VAR1+02H,BX0    ;        
              CALL   #HW_MPY_3232              ;BX,AX*BX0,AX0=DX,CX,BX,AX
              ADD    AX,RAM_AUDIO_ADJ_VAR+00H  ;ADD MULTIPLIED VAR TO ADJ VAR
              ADDC   BX,RAM_AUDIO_ADJ_VAR+02H  ;ADD MULTIPLIED VAR TO ADJ VAR
              JMP    CHK_CNTS4
EQUAL_VAR3:
              ADD    RAM_AUDIO_VAR1+00H,RAM_AUDIO_ADJ_VAR+00H  ;ADD VAR1 TO ADJ
              ADDC   RAM_AUDIO_VAR1+02H,RAM_AUDIO_ADJ_VAR+02H
              JMP    CHK_CNTS4
ADJ_VAR3:                                       ;WINDOW CNTS4 < A/D CNTS 3
;DIVIDE VARIANCE CNTS3/CNTS4 FOR SCALE FACTOR             
              CLR    CX
              MOV    RAM_NoAUDIO_CNTS+04H,BX    ;VARIANCE CNTS FOR LAST A/D 32KB
              MOV    RAM_NoAUDIO_CNTS+06H,DX    ;CNTS #4--TIMER 224ms
              CALL   #DIV_3216_ROUNDED          ;CX,BX/DX=AX   
;DIVIDE SCALE FACTOR INTO FULL VARIANCE             
              MOV    AX,AX0
              CLR    BX0
              MOV    RAM_AUDIO_VAR1+00H,CX0     ;A/D VAR LAST 224ms
              MOV    RAM_AUDIO_VAR1+02H,DX0     ;        
              CALL   #DIV_3232y                 ;(DX0,CX0/BX0,AX0=BX,AX)   
;ADD DOWN SCALED VARIANCE TO ADJUSTED VARIANCE             
              ADD    AX,RAM_AUDIO_ADJ_VAR+00H   ;ADD VAR TO ADJ
              ADDC   BX,RAM_AUDIO_ADJ_VAR+02H
;;;;;;;;;;;;;;;;;




;;;;;;;;;;;;;;;;;
CHK_CNTS4:    TST    RAM_NoAUDIO_CNTS+08H       ;WINDOW #5, 2ND TIMER WINDOW--224ms
              JZ     EXIT_CROSSINGS
              CMP    RAM_NoAUDIO_CNTS+04H,RAM_NoAUDIO_CNTS+08H
              JNC    ADJ_VAR4            ;WINDOW CNTS5 < A/D CNTS3
              JEQ    EQUAL_VAR4          ;            =
;DIVIDE WINDOW CNTS5 BY A/D CNTS3        ;WINDOW CNTS5 > A/D CNTS3
              CLR    CX
              MOV    RAM_NoAUDIO_CNTS+08H,BX ;CNTS #5
              MOV    RAM_NoAUDIO_CNTS+04H,DX ;VARIANCE CNTS FOR LAST A/D 32KB
              CALL   #DIV_3216_ROUNDED       ;CX,BX/DX=AX   
;TAKE CNT RATIO MULTIPLIER * A/D VARIANCE
              CLR.B  BX
              MOV    RAM_AUDIO_VAR1+00H,AX0     ;A/D VAR LAST 224ms
              MOV    RAM_AUDIO_VAR1+02H,BX0     ;        
              CALL   #HW_MPY_3232               ;BX,AX*BX0,AX0=DX,CX,BX,AX
              ADD    AX,RAM_AUDIO_ADJ_VAR+00H   ;ADD MULTIPLIED VAR TO ADJ VAR
              ADDC   BX,RAM_AUDIO_ADJ_VAR+02H   ;ADD MULTIPLIED VAR TO ADJ VAR
              JMP    EXIT_CROSSINGS
EQUAL_VAR4:
              ADD    RAM_AUDIO_VAR1+00H,RAM_AUDIO_ADJ_VAR+00H  ;ADD VAR1 TO ADJ
              ADDC   RAM_AUDIO_VAR1+02H,RAM_AUDIO_ADJ_VAR+02H
              JMP    EXIT_CROSSINGS
ADJ_VAR4:
;DIVIDE VARIANCE CNTS3/CNTS5 FOR SCALE FACTOR             
              CLR    CX
              MOV    RAM_NoAUDIO_CNTS+04H,BX ;VARIANCE CNTS FOR LAST A/D 32KB
              MOV    RAM_NoAUDIO_CNTS+08H,DX ;CNTS #5
              CALL   #DIV_3216_ROUNDED       ;CX,BX/DX=AX   
;DIVIDE SCALE FACTOR INTO FULL VARIANCE             
              MOV    AX,AX0
              CLR    BX0
              MOV    RAM_AUDIO_VAR1+00H,CX0      ;A/D VAR LAST 224ms
              MOV    RAM_AUDIO_VAR1+02H,DX0      ;        
              CALL   #DIV_3232y                  ;(DX0,CX0/BX0,AX0=BX,AX)   
;ADD DOWN SCALED VARIANCE TO ADJUSTED VARIANCE             
              ADD    AX,RAM_AUDIO_ADJ_VAR+00H    ;ADD FULL VAR TO ADJ
              ADDC   BX,RAM_AUDIO_ADJ_VAR+02H

EXIT_CROSSINGS:
;             CALL   #DSPLY_ADJ_VAR
              RET
;;;;;;;;;;;;;;;;;
;***************************************************************
;CALLED FROM "ACTIVE_SHOOTER_LIVE"  (NOT USED)
;CALLED FROM "ANALYZE_AUDIO_EVENT_FINAL"
;************************* CMP_VAR_THRESHOLD *******************
;                                                              *
;ENTER with: RAM_AUDIO_VAR+02 (MSW)                            *
;          : RAM_AUDIO_VAR+00 (LSW)                            *
;          : RAM_AUDIO_ADJ_VAR+00 (LSW)                        *
;          : RAM_AUDIO_ADJ_VAR+00 (LSW)                        *
;          : RAMn_VAR_LMT2    (MSW)                            *
;          : RAMn_VAR_LMT0    (LSW)                            *
;EXIT  with: RAM_ALRM_FLG     (1=ALRM, 0=REJECT)               *
;          ; RAM_ALRM_VAR+02                                   *
;          ; RAM_ALRM_VAR+00                                   *
;***************************************************************
;              .ref SHOT_ALRMFLG,RAM_HEALTH
;              .ref RAM_ALRM_VAR,RAM_ALRM_RTC
;              .ref RAM_BKGD_VAR,RAM_BKGD_RTC

CMP_VAR_THRESHOLD:
		 CALLA #getRAMn_DEBUG
              TST.B  r12
;              TST.B  RAMn_DEBUG
              JZ     NO_LMT_DSPLY
;              CALL   #SND_CR_LF
              CALLA   #DSPLY_VAR_LMT      ;DSPLY VAR LMT
;             CALL   #SND_SP
NO_LMT_DSPLY:

;COMPARE VARIANCE TO MSW AUDIO ALRM/REJECT LMT
		 CALLA  #getRAMn_VAR_LMT2
              CMP    r12, RAM_AUDIO_ADJ_VAR+02  ;ADJUSTED VAR---MSW
;              CMP    RAMn_VAR_LMT2,RAM_AUDIO_ADJ_VAR+02  ;ADJUSTED VAR---MSW
              JNC    VAR_BELOW_LMTz
              JEQ    CHK_VAR_LMT0
VAR_ABOVE_LMTz:
		CALLA #DSPLY_MSG_ABOVE
;              MOV    #MSG_ABOVE,SI
;              CALL   #SND_NULL
;             CALL   #DSPLY_VARIANCE 
              MOV.B  #01,RAM_ALRM_FLG    ;SET RAM_ALRM_FLG=01
;[ADK]              BIS.B  #SHOT_ALRMFLG,RAM_HEALTH
              INC    RAM_NoALRMS         ;INC # ALARMS CNTR
;SAVE ALRM VAR
              MOV    RAM_AUDIO_ADJ_VAR,RAM_ALRM_VAR              
              MOV    RAM_AUDIO_ADJ_VAR+02,RAM_ALRM_VAR+02              
;SAVE ALRM TIME
;              MOV    #RAM_ALRM_RTC,DT
;              CALL   #RD_FORMAT_RTCxyz   ;4 BYTES INTO [DT]
              SETC                       ;CY=1, AUDIO ABOVE LMT
              RET

;COMPARE VARIANCE TO LSW AUDIO ALRM/REJECT LMT
CHK_VAR_LMT0:
		 CALLA  #getRAMn_VAR_LMT0
              CMP    r12,RAM_AUDIO_ADJ_VAR+00   ;ADJUSTED VAR---LSW
;              CMP    RAMn_VAR_LMT0,RAM_AUDIO_ADJ_VAR+00   ;ADJUSTED VAR---LSW
              JC     VAR_ABOVE_LMTz
VAR_BELOW_LMTz: 
		CALLA #DSPLY_MSG_BELOW
;              MOV    #MSG_BELOW,SI
;              CALL   #SND_NULL
;             CALL   #DSPLY_VARIANCE 

;CMP PREVIOUS BKGD MAX VAR TO NEW BKGD VAR 
              CMP    RAM_BKGD_VAR+02,RAM_AUDIO_ADJ_VAR+02  ;ADJUSTED VAR---MSW
              JNC    BKGD_VAR_BELOW_MAX                    ;CY=0
              CMP    RAM_BKGD_VAR,RAM_AUDIO_ADJ_VAR+00  ;ADJUSTED VAR---LSW
              JNC    BKGD_VAR_BELOW_MAX                 ;CY=0

;SAVE BKGD MAX VAR & ASSOCIATED RTC
              MOV    RAM_AUDIO_ADJ_VAR,RAM_BKGD_VAR              
              MOV    RAM_AUDIO_ADJ_VAR+02,RAM_BKGD_VAR+02              
; [ADK]              MOV    #RAM_BKGD_RTC,DT
; [ADK]              CALL   #RD_FORMAT_RTCxyz   ;4 BYTES INTO [DT]

BKGD_VAR_BELOW_MAX:
              MOV.B  #00,RAM_ALRM_FLG    ;SET RAM_ALRM_FLG=00
              CLRC                       ;CY=0, REJECT
              RET  
;;;;;;;;;;;;;;;;;;;

;***************************************************************
;***************************************************************
;***************************************************************
;***************************************************************
;***************************************************************

ANALYZE_AUDIO_EVENT_FINAL:	.asmfunc
;CALLED FROM "RUN.ASM"
			PUSHM.A		#7, r10

; [ADK] Do that before call ..             CALL   #SND_STAR                  ;DEBUG CHAR

;COMBINE 112ms OF 16KB VARIANCE CNT WINDOWS INTO 224ms OF 32KB WINDOWS
;RAM_NoAUDIO_CNTS+00H ---- A/D CNT WINDOW 1--112ms (16KB)
;RAM_NoAUDIO_CNTS+02H ----           "    2    "
;RAM_NoAUDIO_CNTS+04H ----           "    3    "
;RAM_NoAUDIO_CNTS+06H ----           "    4    "   (TIMER)
;RAM_NoAUDIO_CNTS+08H ----           "    5    "      
;RAM_NoAUDIO_CNTS+12H ---- SUM OF ALL WINDOWS

              ADD    RAM_NoAUDIO_CNTS+02H,RAM_NoAUDIO_CNTS+00    ;1ST 32KB CNTS             
              ADD    RAM_NoAUDIO_CNTS+06H,RAM_NoAUDIO_CNTS+04    ;2ND 32KB CNTS           
              ADD    RAM_NoAUDIO_CNTS+0AH,RAM_NoAUDIO_CNTS+08H    ;3RD 32KB CNTS           
              MOV    RAM_NoAUDIO_CNTS+04H,RAM_NoAUDIO_CNTS+02    ;SHIFT 2ND CNTS          
              MOV    RAM_NoAUDIO_CNTS+08H,RAM_NoAUDIO_CNTS+04    ;SHIFT 3RD CNTS          

;RAM_NoAUDIO_CNTS+00H ---- A/D CNT WINDOW 1--224ms (32KB)
;RAM_NoAUDIO_CNTS+02H ----           "    2    "
;RAM_NoAUDIO_CNTS+04H ----           "    3    "

;RAM_NoAUDIO_CNTS+06H ---- 224ms CNT  "   4    "   (TIMER)
;RAM_NoAUDIO_CNTS+08H ---- 224ms CNT  "   5    "      
;RAM_NoAUDIO_CNTS+12H ---- SUM OF ALL WINDOWS

              CLR    RAM_NoAUDIO_CNTS+06H       ;224ms CNT WINDOWS     
              CLR    RAM_NoAUDIO_CNTS+08H       ;           
              CLR    RAM_NoAUDIO_CNTS+0AH       ;           

              MOV    #RAM_NoAUDIO_CNTS+06,DT    ;SET CNTR WRT PTR
;START CNTING/SAVING 4 WINDOWS 6-7 OR 6-9
              BIS    #TACLR,&TA0CTL             ;CLR CNTRA0               
              BIS    #MC1,&TA0CTL               ;RESTART CNTRA0             
              CALL   #START_TIMERA1_LMT         ;START 224ms CNT DOWN WINDOW
              MOV.B  #No_CNT_WINDOWS,CX0
WAIT_CNT_WINDOW_ENDx:
              BIT    #BIT0,&TA1CTL              ;WAIT FOR 224ms INT PENDING FLG
              JZ     WAIT_CNT_WINDOW_ENDx
              BIC    #MC1+MC0,&TA0CTL           ;STOP CMP CNTR0                    
              CALL   #RD_CNTRA0                 ;AX=CNTS FOR 224ms WINDOW
              ST     AX,DT                      ;SAVE 224ms CNT WINDOW
              ADD    AX,RAM_NoAUDIO_CNTS+12H    ;SUM OF ALL WINDOWS
              BIS    #TACLR,&TA0CTL             ;CLR CNTRA0               
              BIC    #BIT0,&TA1CTL              ;CLR TIMERA1 INT FLG
              BIS    #MC1,&TA0CTL               ;RESTART CNTRA0             
	.if xENABLE_TIMING_BIT 
	XOR.B  #TIMING_BIT,&TIMING_PORT   ;END OF 224ms  (TIMING BIT=1) 
	.endif
              DJNZB  CX0,WAIT_CNT_WINDOW_ENDx   ;CX0=No_CNT_WINDOWS

              CALL   #STOP_TIMERA1              ;STOP CNT WINDOW TIMER
              INC    RAM_EVENTNo                ;INC EVENT #

;SHIFT TOTAL CNTS
;             MOV    RAM_NoAUDIO_CNTS+12H,RAM_NoAUDIO_CNTS+0AH   ;TOTAL CNTS

;RAM_NoAUDIO_CNTS+00H ---- A/D CNT WINDOW 1--224ms (32KB)
;RAM_NoAUDIO_CNTS+02H ----           "    2    "
;RAM_NoAUDIO_CNTS+04H ----           "    3    "
;RAM_NoAUDIO_CNTS+06H ---- 224ms CNT  "   4    "   (TIMER)
;RAM_NoAUDIO_CNTS+08H ---- 224ms CNT  "   5    "      
;RAM_NoAUDIO_CNTS+0AH ---- 224ms CNT  "   6    "           (DOUBLE SHOT)
;RAM_NoAUDIO_CNTS+0CH ---- 224ms CNT  "   7    "                   
;RAM_NoAUDIO_CNTS+12H ---- SUM OF ALL WINDOWS



;DSPLY COMPARATOR CNTRS VALUE--??ms
              CALLA   #DSPLY_CNTRS               ;CNTRA0 (# COMPARATOR CROSSES IN 1120ms)
                                                ;CHKS DEBUG FLG

;ANALYZE TIME MEASURED 150ms (12/29/14)   ??ms (3/11/15)
              MOV.B  #LETTER_V,DX               ;SND LETTER V
              CALLA   #DSPLY_CHAR
              
;FIND VAR OF 4 EACH 16K PTS  (RAM_AUDIO_VAR0)--USES ZERO CLAMPING OPTION
;64K OF VAR
	.if xENABLE_TIMING_BIT 
	BIC.B  #TIMING_BIT,&TIMING_PORT   ;START OF VAR  (TIMING BIT=0) 
	.endif
              CLR    RAM_AUDIO_VAR0+00          ;CLR RESULT
              CLR    RAM_AUDIO_VAR0+02          ;CLR RESULT
              MOVA   #RAM_EXTENDED,SI           ;FRAM 10000

              MOV    #No_AUDIO_PTS_LIVE,AX      ;# PTS=16,384  
              MOV    #No_UPPER_WINDOWS-1,BX     ;05-01=04
              CALL   #HW_MPY_1616               ;AX*BX=BX,AX 
              MOV    AX,BP0                     ;# PTS=65,536  
              CALL   #ANALYZE_AUDIO_VARIANCEx   ;RESULT @ DX0,DX
              MOV    DX0,RAM_AUDIO_VAR0+02      ;MSW RESULT
              MOV    DX,RAM_AUDIO_VAR0          ;LSW RESULT

;FIND VAR OF NXT 16K PTS  (RAM_AUDIO_VAR1)--CHKS FOR ZERO CLAMPING OPTION
;STILL IN UPPER FRAM
;+16K OF VAR
              CLR    RAM_AUDIO_VAR1+00          ;CLR RESULT
              CLR    RAM_AUDIO_VAR1+02          ;CLR RESULT
;             MOV    #RAM_FRAM,SI               ;FRAM AF80
              CALL   #ANALYZE_AUDIO_VARIANCE    ;RESULT @ DX0,DX
              MOV    DX0,RAM_AUDIO_VAR1+02      ;MSW RESULT       
              MOV    DX,RAM_AUDIO_VAR1          ;LSW RESULT       

;FIND VAR OF LAST 16K PTS  (RAM_AUDIO_VAR1)--CHKS FOR ZERO CLAMPING OPTION
;IN LOWER FRAM
;+16K OF VAR
              MOV    #RAM_FRAM,SI               ;FRAM AF80
              CALL   #ANALYZE_AUDIO_VARIANCE    ;RESULT @ DX0,DX
              ADD    DX,RAM_AUDIO_VAR1          ;ADD LAST TO PREVIOUS VAR1
              ADDC   DX0,RAM_AUDIO_VAR1+02      ;          
	.if xENABLE_TIMING_BIT 
	BIS.B  #TIMING_BIT,&TIMING_PORT   ;END OF VAR  (TIMING BIT=1) 
	.endif
;SAVE SUM OF VARIANCE 0+1 TO RAM_AUDIO_VAR01 (688ms OF A/D)              
;64K+32K=96K OF VAR
              MOV    RAM_AUDIO_VAR0,RAM_AUDIO_VAR01    
              MOV    RAM_AUDIO_VAR0+02H,RAM_AUDIO_VAR01+02H
              ADD    RAM_AUDIO_VAR1,RAM_AUDIO_VAR01          ;RAM_AUDIO_VAR01 
              ADDC   RAM_AUDIO_VAR1+02H,RAM_AUDIO_VAR01+02H  ;     =VAR OF 96K RAM
              MOV.B  #LETTER_V,DX               ;SND LETTER V=VARIANCE START
              CALLA   #DSPLY_CHAR

;              TST.B  RAMn_DEBUG
		CALLA #getRAMn_DEBUG
		TST.B	r12
              JZ     NO_VAR_DSPLY
              CALLA   #DSPLY_VARIANCE            ;VAR0, VAR1 & VAR 0+1
NO_VAR_DSPLY:


;RAM_AUDIO_VAR01=VAR OF 96K RAM 
;FACTOR IN # ZERO CROSSINGS TO ACCOUNT FOR WF DURATION PAST A/D WINDOWS
;   RATIO CNTS FROM WINDOW #1 TO WINDOW X
;   MAX ADJ VARIANCE=FFFF FFFF=4,294,967,295 (10 DIGITS)
              CALL   #ANALYZE_ZERO_CROSSINGS    ;ADJUSTED VAR @ RAM_AUDIO_ADJ_VAR (DWORD)
                                                ;INCLUDES "DSPLY_ADJ_VARIANCE"

	.if xUSE_ADJVAR 
              CALLA   #DSPLY_ADJ_VAR
	.endif
              CALLA   #DSPLY_EVENTNo

	.if !xUSE_ADJVAR 
              MOV    RAM_AUDIO_VAR01+02H,RAM_AUDIO_ADJ_VAR+02H ;SKIP ADJ VAR
              MOV    RAM_AUDIO_VAR01+00H,RAM_AUDIO_ADJ_VAR+00H ;SKIP ADJ VAR
	.endif

;CHK IF IN "CALB ALRM THRESHOLD" MODE
;              CMP.B  #CODE_CALB_ALRM_TH,RAMn_MODE ;CHK MODE FLG FOR ALRM CALIBRATION TH
		 	   CALLA #getRAMn_MODE
              CMP.B  #CODE_CALB_ALRM_TH,r12 ;CHK MODE FLG FOR ALRM CALIBRATION TH
              JNE    NO_ALRM_CALB
; [ADK] 12/13/2019              CALL   #CALB_ALRM_THRESHOLD         ;USES LARGEST ADJ VAR FOR NEW ALRM TH
NO_ALRM_CALB:


;CHK VARIANCE FOR ALRM/REJECT
              CALL   #CMP_VAR_THRESHOLD         ;CY=1 ALARM, RAM_ALRM_FLG=01

	.if xCHK_MULTIPLE_SHOTS
              BIT.B  #BIT0,RAM_ALRM_FLG         ;CHK IF AN ALRM EVENT
              JZ     NO_ALRM
              CALL   #CHK_MULTIPLE_SHOTS        ;ALRM FLG: B7=DOUBLE SHOT
NO_ALRM:
	.endif                                          
	.if 0
[ADK]  12/13/2019 -- move that to the C-part	

;CHK FLG TO SND EVENT OUT VIA RF POD
              CMP.B  #CODE_SND_WF_HW,RAMn_MODE    ;CHK MODE FLG FOR HARDWIRE OUT         
              JEQ    NO_RF_OUT                    ; -NO RF OUT FOR HARDWIRE 
              CMP.B  #CODE_CALB_ALRM_TH,RAMn_MODE ;CHK MODE FLG FOR ALRM CALIBRATION TH
              JEQ    NO_RF_IN_CALB                ; -NO RF OUT IN CALB MODE
              CMP.B  #CODE_SND_TEST_DATA,RAMn_MODE  ;04=SND TEST DATA
              JNE    MODE_EVENT_OUT
MODE_TESTDATA_OUT:
              MOV    #MSG_RFOUT,SI              ;MSG FOR RF OUT
              CALL   #SND_NULL
              CALL   #SND_TESTDATA_RF           ;AUDIO.ASM
              JMP    RF_CONTINUE
MODE_EVENT_OUT:
              CMP.B  #CODE_SND_EVENT,RAMn_MODE  ;02=SND EVENT-THREAT OR NON-THREAT
              JEQ    SND_EVENT_RFx  
              TST.B  RAM_ALRM_FLG               ;00=NON-THREAT
              JZ     NO_RF_OUT                  ;01=ALARM
SND_EVENT_RFx:
	.if xRUN_SENSOR_RF_MODE                         ;SEE "DEFINES.INC"
              MOV    #MSG_RFOUT,SI              ;MSG FOR RF OUT
              CALL   #SND_NULL
              CALL   #SND_EVENT_RF              ;RUN.ASM
	.endif
NO_RF_OUT:
NO_RF_IN_CALB: 
RF_CONTINUE:

;CHK FLG TO SND WAVEFORM OUT OVER HARD WIRE TO HOST
;16 SECS AT 9600,  <1 SEC @ 115KBAUD
              CMP.B  #CODE_SND_WF_HW,RAMn_MODE  ;CHK MODE FLG FOR SNDING HARD WIRE TO HOST
              JNE    NO_HOST_OUT
              CALL   #SND_SP
              CALL   #SND_SP
              MOV.B  #LETTER_H,AX               ;SND LETTER H
              CALL   #VIDEO_OUT
	.if xENABLE_TIMING_BIT 
	BIC.B  #TIMING_BIT,&TIMING_PORT   ;START OF SND  (TIMING BIT=0) 
	.endif
              CALL   #SND_WF_DATA_HARDWIRE      ;AUDIO.ASM
	.if xENABLE_TIMING_BIT 
	BIS.B  #TIMING_BIT,&TIMING_PORT   ;END   OF SND  (TIMING BIT=1) 
	.endif
              MOV.B  #LETTER_H,AX               ;SND LETTER H
              CALL   #VIDEO_OUT

NO_HOST_OUT:
              CALL   #SND_SP
              CALL   #SND_SP
              BIS.B  #LED_LED1_GRN,&LED_LED1_PORT ;TURN OFF GRN REJECT/DETECT LED  "LEDS1.ASM"
              CALL   #FLASH_ALRM_REJECT_LED     ;FLASH ALRM/REJECT LED (RED/GRN)
                                                ;  --NO MONOS
	.endif
		POPM.A  #7, r10
   .if ($defined(__MSP430_HAS_MSP430XV2_CPU__) | $defined(__MSP430_HAS_MSP430X_CPU__))
        reta
   .else
        ret
   .endif
      .endasmfunc

        .end

