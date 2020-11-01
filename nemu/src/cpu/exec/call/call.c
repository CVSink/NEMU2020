#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"

make_helper(call) {
    uint32_t displacement = instr_fetch(eip + 1, 4);
    
    print_asm("call %u", displacement);
    
	return 5 + (int)displacement;
}