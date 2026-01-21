/* Start.s 
 *	Assembly language assist for user programs running on top of Nachos.
 *
 *	Since we don't want to pull in the entire C library, we define
 *	what we need for a user program here, namely Start and the system
 *	calls.
 */

#include "syscall.h"

        .text   
        .align  2

/* -------------------------------------------------------------
 * __start
 *	Initialize running a C program, by calling "main". 
 *
 * 	NOTE: This has to be first, so that it gets loaded at location 0.
 *	The Nachos kernel always starts a program by jumping to location 0.
 * -------------------------------------------------------------
 */

	.globl __start
	.ent	__start
__start:
	jal	main
	move	$4,$2		
	jal	Exit	 /* if we return from main, exit(0) */
	.end __start

/* -------------------------------------------------------------
 * System call stubs:
 *	Assembly language assist to make system calls to the Nachos kernel.
 *	There is one stub per system call, that places the code for the
 *	system call into register r2, and leaves the arguments to the
 *	system call alone (in other words, arg1 is in r4, arg2 is 
 *	in r5, arg3 is in r6, arg4 is in r7)
 *
 * 	The return value is in r2. This follows the standard C calling
 * 	convention on the MIPS.
 * -------------------------------------------------------------
 */

	.globl Halt
	.ent	Halt
Halt:
	addiu $2,$0,SC_Halt
	syscall
	j	$31
	.end Halt
	
	.globl Exit
	.ent	Exit
Exit:
	addiu $2,$0,SC_Exit
	syscall
	j	$31
	.end Exit

	.globl Exec
	.ent	Exec
Exec:
	addiu $2,$0,SC_Exec
	syscall
	j	$31
	.end Exec

	.globl Join
	.ent	Join
Join:
	addiu $2,$0,SC_Join
	syscall
	j	$31
	.end Join

	.globl Create
	.ent	Create
Create:
	addiu $2,$0,SC_Create
	syscall
	j	$31
	.end Create

	.globl Open
	.ent	Open
Open:
	addiu $2,$0,SC_Open
	syscall
	j	$31
	.end Open

	.globl Read
	.ent	Read
Read:
	addiu $2,$0,SC_Read
	syscall
	j	$31
	.end Read

	.globl Write
	.ent	Write
Write:
	addiu $2,$0,SC_Write
	syscall
	j	$31
	.end Write

	.globl Close
	.ent	Close
Close:
	addiu $2,$0,SC_Close
	syscall
	j	$31
	.end Close

	.globl Mkdir
	.ent	Mkdir
Mkdir:
	addiu $2,$0,SC_Mkdir
	syscall
	j	$31
	.end Mkdir
	
	.globl Chdir
	.ent	Chdir
Chdir:
	addiu $2,$0,SC_Chdir
	syscall
	j	$31
	.end Chdir

	.globl Fork
	.ent	Fork
Fork:
	addiu $2,$0,SC_Fork
	syscall
	j	$31
	.end Fork

	.globl Yield
	.ent	Yield
Yield:
	addiu $2,$0,SC_Yield
	syscall
	j	$31
	.end Yield

    .globl PutChar
    .ent   PutChar
PutChar:
    addiu $2,$0,SC_PutChar
    syscall
    j $31
    .end PutChar

	.globl PutString
	.ent   PutString
PutString:
    addiu $2,$0,SC_PutString
    syscall
    j $31
	.end PutString

	.global GetChar
	.ent GetChar
GetChar:
	addiu $2,$0,SC_GetChar
    syscall
    j $31
	.end GetChar

	.global GetString
	.ent GetString
GetString:
	addiu $2,$0,SC_GetString
    syscall
    j $31
	.end GetString

	.global PutInt
	.ent PutInt
PutInt:
	addiu $2,$0,SC_PutInt
    syscall
    j $31
	.end PutInt

	.global GetInt
	.ent GetInt
GetInt:
	addiu $2,$0,SC_GetInt
    syscall
    j $31
	.end GetInt
	
	.global UserThreadCreate
	.ent UserThreadCreate
UserThreadCreate:
    la    $6, __UserThreadFinish   # 3e argument = adresse wrapper
    addiu $2,$0,SC_UserThreadCreate
    syscall
    j $31
	.end UserThreadCreate

	.global UserThreadExit
	.ent UserThreadExit
UserThreadExit:
	addiu $2,$0,SC_UserThreadExit
    syscall
    j $31
	.end UserThreadExit

	.global UserThreadJoin
	.ent UserThreadJoin
UserThreadJoin:
	addiu $2,$0,SC_UserThreadJoin
    syscall
    j $31
	.end UserThreadJoin

.globl  __UserThreadFinish
.ent    __UserThreadFinish
__UserThreadFinish:
    jal     UserThreadExit     # appel du syscall
    nop
	.end __UserThreadFinish

    .globl ForkExec
    .ent   ForkExec
ForkExec:
    addiu $2,$0,SC_ForkExec
    syscall
    j $31
    .end ForkExec


	.globl Wait
    .ent    Wait
Wait:
    addiu $2,$0,SC_Wait
    syscall
    j $31
    .end Wait
    .globl SemCreate
    .ent   SemCreate
SemCreate:
    addiu $2,$0,SC_SemCreate
    syscall
    j $31
    .end SemCreate

    .globl SemDestroy
    .ent   SemDestroy
SemDestroy:
    addiu $2,$0,SC_SemDestroy
    syscall
    j     $31
    nop
    .end  SemDestroy

    .globl P
    .ent   P
P:
    addiu $2,$0,SC_P
    syscall
    j     $31
    nop
    .end  P

    .globl V
    .ent   V
V:
    addiu $2,$0,SC_V
    syscall
    j     $31
    nop
    .end  V

    .globl SBRK
    .ent   SBRK
SBRK:
    addiu $2,$0,SC_SBRK
    syscall
    j     $31
    nop
    .end  SBRK
	
/* dummy function to keep gcc happy */
		.globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main
/* --- RESEAU GENERIQUE --- */
    .globl ReseauSend
    .ent   ReseauSend
ReseauSend:
    addiu $2,$0,SC_ReseauSend
    syscall
    j     $31
    .end  ReseauSend

    .globl ReseauReceive
    .ent   ReseauReceive
ReseauReceive:
    addiu $2,$0,SC_ReseauReceive
    syscall
    j     $31
    .end  ReseauReceive

/* --- FTP --- */
    .globl FtpGet
    .ent   FtpGet
FtpGet:
    addiu $2,$0,SC_FtpGet
    syscall
    j     $31
    .end  FtpGet

    .globl FtpPut
    .ent   FtpPut
FtpPut:
    addiu $2,$0,SC_FtpPut
    syscall
    j     $31
    .end  FtpPut

    .globl FtpList
    .ent   FtpList
FtpList:
    addiu $2,$0,SC_FtpList
    syscall
    j     $31
    .end  FtpList

    .globl FtpMkdir
    .ent   FtpMkdir
FtpMkdir:
    addiu $2,$0,SC_FtpMkdir
    syscall
    j     $31
    .end  FtpMkdir

    .globl FtpRmdir
    .ent   FtpRmdir
FtpRmdir:
    addiu $2,$0,SC_FtpRmdir
    syscall
    j     $31
    .end  FtpRmdir

    .globl FtpDelete
    .ent   FtpDelete
FtpDelete:
    addiu $2,$0,SC_FtpDelete
    syscall
    j     $31
    .end  FtpDelete

    .globl FtpRename
    .ent   FtpRename
FtpRename:
    addiu $2,$0,SC_FtpRename
    syscall
    j     $31
    .end  FtpRename

    .globl FtpStartServer
    .ent   FtpStartServer
FtpStartServer:
    addiu $2,$0,SC_FtpStartServer
    syscall
    j     $31
    .end  FtpStartServer
.globl LocalList
    .ent   LocalList
LocalList:
    addiu $2, $0, SC_LocalList
    syscall
    j     $31
    .end   LocalList
.globl LocalCat
    .ent   LocalCat
LocalCat:
    addiu $2, $0, SC_LocalCat
    syscall
    j     $31
    .end   LocalCat

