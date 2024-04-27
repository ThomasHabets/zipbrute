// Hello world

.data
fmthex:		.string "0x%x\n"
fmtstr:		.string "%s\n"
fmtspstr:	.string "  %s\n"
fmtint:		.string "%d\n"
fmtchr:		.string "%c\n"
fmtkeys:	.string "%.10s  %.8x  %.8x  %.8x  %.2x\n"
badcache:	.byte 1
strenter:	.string "Entering function"
was_at:		.long 0
		
.bss
cachekey0:	.long 0
cachekey1:	.long 0
cachekey2:	.long 0
cachekey3:	.byte 0
cachekey_pad:	.string "\0\0\0"
key0:		.long 0
key1:		.long 0
key2:		.long 0
key3:		.byte 0
key_pad:	.string "\0\0\0"
skey0:		.long 0
skey1:		.long 0
skey2:		.long 0
skey3:		.byte 0
skey_pad:	.string "\0\0\0"
cachestop:	.long 0
		.comm charindex,255,32
		.comm current_password,255,32
				
.text

.extern asmcrctab, asmkey, asmkey3, asmcharset, asmcharindex, asmcurrent_password,asmfilecount
.extern asmfiles,asmfileheaders

//#define current_password asmcurrent_password
//#define charindex asmcharindex
/*
 * Input:	eax = plain byte
 * speedups:	interleave things and use more than 4 registers
 */
asmupdateKeys:
//	key0 = ( key0 >> 8) ^ asmcrctab[ (key0 ^ c) & 0xff]
	xorl	key0, %eax
	movl	$asmcrctab, %ebx
	andl	$0xff, %eax
	movl	(%ebx, %eax, 4), %eax
	// right side done = eax
	movl	key0, %edx
	shrl	$8, %edx
	// left side done = edx
	xorl	%edx, %eax
	movl	%eax, key0

//	key[1] = (key[1] + (key[0] & 0xff)) * 134775813 + 1;
//	eax = key0
	andl	$0xff, %eax
	addl	key1, %eax
	movl	$134775813, %edx
	mull	%edx
	incl	%eax
	movl	%eax, key1

#define CRC32(x,c)      (((x)>>8)^asmcrctab[((x)^(c))&0xff])
//	key2 = (key2 >> 8) ^ asmcrctab[(key2^((key[1] >> 24) & 0xff)) & 0xff]
//	eax = key1
//	ebx = $asmcrctab
	movl	key2, %edx
	movl	%edx, %esi
	shrl	$8, %edx
	// left side done = edx
	// esi = key2
	shrl	$24, %eax
	xorl	%esi, %eax
	andl	$0xff, %eax
	movl	(%ebx, %eax, 4), %eax
	xorl	%edx, %eax
	movl	%eax, key2

//	temp = (key[2] & 0xffff) | 3;
//	key3 = ((temp * (temp ^ 1)) >> 8) & 0xff;
	andl	$0xffff, %eax
	orl	$3, %eax
	movl	%eax, %edx
	xorl	$1, %edx
	mull	%edx
	shrl	$8, %eax
	movb	%al, key3
	ret
	
.globl asmTestLength
asmTestLength:
	/*
	 * init
	 */
	pushl	%ebp
	movl	%esp, %ebp
	pusha // FIXME:	find out exacly which regs need pushing

	movl	8(%ebp), %ecx	// length
	movb	asmcharset, %al
	movl	$current_password, %edi
	rep
	stosb

	movl	8(%ebp), %ecx	// length
	xorb	%al,%al
	movl	$charindex, %edi
	rep
	stosb

	movb	$1, badcache

	/*
	 * Generate key[] from password
	 */
	movl	8(%ebp), %ecx	// length
	movl	$0, current_password(%ecx)
.mainloop:
	testb	$1, badcache
	jnz	.updatecache

	// old cache is good, copy it
	movl	$cachekey0, %esi
	movl	$key0, %edi
	movsl
	movsl
	movsl
	movsl
	jmp	.pre_forgenpw

.updatecache:
	movl	$0, badcache
	movl	$0x12345678, key0
	movl	$0x23456789, key1
	movl	$0x34567890, key2

	movl	8(%ebp), %ecx     // length
	decl	%ecx
	movl	$current_password, %edi
.for_createcache:
	movb	(%edi), %al
	call	asmupdateKeys
	incl	%edi
	decl	%ecx
	jnz	.for_createcache
	movl	%edi, cachestop
	movb	$0, badcache

	movl	$key0, %esi
	movl	$cachekey0, %edi
	movsl
	movsl
	movsl
	movsl
	
.pre_forgenpw:
	movl	cachestop, %edi
	movb	(%edi), %al
	call	asmupdateKeys

/*
 * key{0-3} is now set up
 */
//	movl	asmfilecount, %ecx
	movl	$0, %ecx
	movl	$asmfiles, was_at

//	call	printkeys
	movl	$key0, %esi
	movl	$skey0, %edi
	movsl
	movsl
	movsl
	movsl
.for_fileloop:
	movl	$skey0, %esi
	movl	$key0, %edi
	movsl
	movsl
	movsl
	movsl

	movl	was_at, %edi

//	call	printkeys
	
#define round	movb (%edi), %al; xorb key3, %al;call asmupdateKeys ;incl %edi
#define rounds round;round;round;round;round
	rounds
	rounds
	round
	
	movb	(%edi), %al
	xorb	key3, %al
	incl	%edi

	movl	%edi, was_at
	
//	call printkeys

	// al is now the calculated value to be compared to fileheaders[file]
	movb	asmfileheaders(%ecx), %dl
	cmpb	%al, %dl
	jne	.next

	pusha
	pushl	$current_password
	pushl	$fmtspstr
	call	printf
	addl	$8, %esp
	popa

	incl	%ecx
	cmpl	asmfilecount, %ecx
	jne	.for_fileloop
	
	pusha
	pushl	$current_password
	pushl	$fmtstr
	call	printf
	addl	$8, %esp
	popa	

.next:	
	movl	$skey0, %esi
	movl	$key0, %edi
	movsl
	movsl
	movsl
	movsl

/*
 * Generate new password
 */
#if 0
	cmpb	$'a', current_password+2
	jne	.exitnow
	movl	$'b', current_password+2
#else
	movl	8(%ebp), %edx
	decl	%edx

	xorl	%eax, %eax
.while_chars:	
	movb	charindex(%edx), %al
	incb	%al
	movb	asmcharset(%eax), %bl
	testb	%bl, %bl
	jnz	.nowrap
	xorb	%al, %al
	movb	%al, charindex(%edx)
	movb	asmcharset, %al
	movb	%al, current_password(%edx)
	cmpb	$0, %dl
	je	.exitnow
	decl	%edx
	jmp	.while_chars
.nowrap:
	movb	%al, charindex(%edx)
	movb	asmcharset(%eax), %al
	movb	%al, current_password(%edx)
#endif
	jmp	.mainloop

.exitnow:
	popa
	movl	%ebp, %esp
	popl	%ebp
	ret

printkeys:
	pusha
	pushl	key3
	pushl	key2
	pushl	key1
	pushl	key0
	pushl	$current_password
	pushl	$fmtkeys
	call	printf
	addl	$24, %esp
	popa
	ret
	