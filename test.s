	.file	"cpu_intr.c"
	.option nopic
	.attribute arch, "rv32i2p1_m2p0_a2p1_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.align	1
	.globl	mtime_get
	.type	mtime_get, @function
mtime_get:
	addi	sp,sp,-32
	sw	ra,28(sp)
	sw	s0,24(sp)
	addi	s0,sp,32
.L8:
	lui	a6,%hi(earth)
	lw	a6,%lo(earth)(a6)
	lw	a6,44(a6)
	bne	a6,zero,.L2
	li	a6,-268320768
	addi	a6,a6,-4
	j	.L3
.L2:
	li	a6,33603584
	addi	a6,a6,-4
.L3:
	lw	a6,0(a6)
	sw	a6,-20(s0)
	lui	a6,%hi(earth)
	lw	a6,%lo(earth)(a6)
	lw	a6,44(a6)
	bne	a6,zero,.L4
	li	a6,-268320768
	addi	a6,a6,-8
	j	.L5
.L4:
	li	a6,33603584
	addi	a6,a6,-8
.L5:
	lw	a6,0(a6)
	sw	a6,-24(s0)
	lui	a6,%hi(earth)
	lw	a6,%lo(earth)(a6)
	lw	a6,44(a6)
	bne	a6,zero,.L6
	li	a6,-268320768
	addi	a6,a6,-4
	j	.L7
.L6:
	li	a6,33603584
	addi	a6,a6,-4
.L7:
	lw	a6,0(a6)
	lw	a7,-20(s0)
	bne	a7,a6,.L8
	lw	a6,-20(s0)
	mv	t1,a6
	li	t2,0
	slli	a5,t1,0
	li	a4,0
	lw	a6,-24(s0)
	mv	a2,a6
	li	a3,0
	or	a0,a4,a2
	or	a1,a5,a3
	mv	a4,a0
	mv	a5,a1
	mv	a0,a4
	mv	a1,a5
	lw	ra,28(sp)
	lw	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	mtime_get, .-mtime_get
	.align	1
	.type	mtimecmp_set, @function
mtimecmp_set:
	addi	sp,sp,-32
	sw	ra,28(sp)
	sw	s0,24(sp)
	addi	s0,sp,32
	sw	a0,-24(s0)
	sw	a1,-20(s0)
	sw	a2,-28(s0)
	lui	a5,%hi(earth)
	lw	a5,%lo(earth)(a5)
	lw	a5,44(a5)
	bne	a5,zero,.L11
	li	a5,-268353536
	j	.L12
.L11:
	li	a5,33570816
.L12:
	lw	a4,-28(s0)
	slli	a4,a4,3
	add	a5,a5,a4
	addi	a5,a5,4
	mv	a4,a5
	li	a5,-1
	sw	a5,0(a4)
	lui	a5,%hi(earth)
	lw	a5,%lo(earth)(a5)
	lw	a5,44(a5)
	bne	a5,zero,.L13
	li	a5,-268353536
	j	.L14
.L13:
	li	a5,33570816
.L14:
	lw	a4,-28(s0)
	slli	a4,a4,3
	add	a5,a5,a4
	mv	a4,a5
	lw	a5,-24(s0)
	sw	a5,0(a4)
	lw	a5,-20(s0)
	srli	a6,a5,0
	li	a7,0
	lui	a5,%hi(earth)
	lw	a5,%lo(earth)(a5)
	lw	a5,44(a5)
	bne	a5,zero,.L15
	li	a5,-268353536
	j	.L16
.L15:
	li	a5,33570816
.L16:
	lw	a4,-28(s0)
	slli	a4,a4,3
	add	a5,a5,a4
	addi	a5,a5,4
	mv	a4,a6
	sw	a4,0(a5)
	nop
	lw	ra,28(sp)
	lw	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	mtimecmp_set, .-mtimecmp_set
	.align	1
	.type	timer_reset, @function
timer_reset:
	addi	sp,sp,-32
	sw	ra,28(sp)
	sw	s0,24(sp)
	addi	s0,sp,32
	sw	a0,-20(s0)
	call	mtime_get
	lui	a5,%hi(earth)
	lw	a5,%lo(earth)(a5)
	lw	a4,44(a5)
	li	a5,1
	bne	a4,a5,.L18
	li	a4,499712
	addi	a4,a4,288
	li	a5,0
	j	.L19
.L18:
	li	a4,49999872
	addi	a4,a4,128
	li	a5,0
.L19:
	add	a2,a4,a0
	mv	a6,a2
	sltu	a6,a6,a4
	add	a3,a5,a1
	add	a5,a6,a3
	mv	a3,a5
	mv	a4,a2
	mv	a5,a3
	lw	a2,-20(s0)
	mv	a0,a4
	mv	a1,a5
	call	mtimecmp_set
	nop
	lw	ra,28(sp)
	lw	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	timer_reset, .-timer_reset
	.section	.rodata
	.align	2
.LC0:
	.string	"Use direct mode and put the address of the trap_entry into mtvec"
	.text
	.align	1
	.globl	intr_init
	.type	intr_init, @function
intr_init:
	addi	sp,sp,-32
	sw	ra,28(sp)
	sw	s0,24(sp)
	addi	s0,sp,32
	sw	a0,-20(s0)
	lui	a5,%hi(earth)
	lw	a5,%lo(earth)(a5)
	lui	a4,%hi(timer_reset)
	addi	a4,a4,%lo(timer_reset)
	sw	a4,12(a5)
	lw	a2,-20(s0)
	li	a0,-1
	li	a1,268435456
	addi	a1,a1,-1
	call	mtimecmp_set
	lui	a5,%hi(trap_entry)
	addi	a5,a5,%lo(trap_entry)
 #APP
# 42 "earth/cpu_intr.c" 1
	csrw mtvec, a5
# 0 "" 2
 #NO_APP
	lui	a5,%hi(.LC0)
	addi	a0,a5,%lo(.LC0)
	call	INFO
	li	a5,0
 #APP
# 46 "earth/cpu_intr.c" 1
	csrw mip, a5
# 0 "" 2
 #NO_APP
	li	a5,128
 #APP
# 47 "earth/cpu_intr.c" 1
	csrs mie, a5
# 0 "" 2
 #NO_APP
	li	a5,136
 #APP
# 48 "earth/cpu_intr.c" 1
	csrs mstatus, a5
# 0 "" 2
 #NO_APP
	nop
	lw	ra,28(sp)
	lw	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	intr_init, .-intr_init
	.align	1
	.globl	_test_
	.type	_test_, @function
_test_:
	addi	sp,sp,-48
	sw	ra,44(sp)
	sw	s0,40(sp)
	addi	s0,sp,48
	call	mtime_get
	sw	a0,-24(s0)
	sw	a1,-20(s0)
	li	a4,123
	li	a5,0
	sw	a4,-32(s0)
	sw	a5,-28(s0)
	lw	a2,-24(s0)
	lw	a3,-20(s0)
	lw	a0,-32(s0)
	lw	a1,-28(s0)
	sub	a4,a2,a0
	mv	a6,a4
	sgtu	a6,a6,a2
	sub	a5,a3,a1
	sub	a3,a5,a6
	mv	a5,a3
	sw	a4,-40(s0)
	sw	a5,-36(s0)
	lw	a4,-40(s0)
	lw	a5,-36(s0)
	mv	a0,a4
	mv	a1,a5
	lw	ra,44(sp)
	lw	s0,40(sp)
	addi	sp,sp,48
	jr	ra
	.size	_test_, .-_test_
	.ident	"GCC: (xPack GNU RISC-V Embedded GCC x86_64) 14.2.0"
	.section	.note.GNU-stack,"",@progbits
