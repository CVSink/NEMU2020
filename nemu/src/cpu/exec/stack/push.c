#include "cpu/exec/helper.h"
#include "cpu/reg.h"

make_helper(push_r_v) {
    uint32_t offset = 80;
    uint32_t reg = instr_fetch(eip, 1) - offset;
    op_src->val = R_ESP - 4;
    op_src->type = OP_TYPE_REG;
    op_src->reg = R_ESP;
    op_dest->type = OP_TYPE_MEM;
    op_dest->val = reg_l(reg);
    op_dest->addr = R_ESP - 4;

    swaddr_write(op_dest->addr, 4, op_dest->val);
//  MEM_W(op_dest->addr, op_dest->val);
    cpu.esp = R_ESP - 4;
//  OPERAND_W(op_src, op_src->val);
	print_asm("push %0x", op_dest->val);

	return 1;
}