#include <stdint.h>
#include <stdio.h> // printf

#include "vm_run_thread.h"

static uint_fast8_t trace_i8(vm_thread_p thread, uint32_t *tip)
{
	uint8_t data = thread->flash[*tip];
	*tip += 1;
	
	return(data);
}

static uint_fast16_t trace_i16(vm_thread_p thread, uint32_t *tip)
{
	uint_fast8_t lo = trace_i8(thread, tip);
	uint_fast8_t hi = trace_i8(thread, tip);

	return((hi << 8) | lo);
}

static uint_fast32_t trace_i32(vm_thread_p thread, uint32_t *tip)
{
	uint_fast16_t lo = trace_i16(thread, tip);
	uint_fast16_t hi = trace_i16(thread, tip);
	
	return((hi << 16) | lo);
}


static uint_fast32_t trace_m(vm_thread_p thread, uint_fast32_t maddr)
{
	uint32_t *ptr = (uint32_t *)&thread->zero[maddr & M_ADDR_MASK];
	return(*ptr);
}

static uint_fast32_t trace_m_i8(vm_thread_p thread, uint32_t *tip, uint_fast8_t *r)
{
	*r = trace_i8(thread, tip);
	return(trace_m(thread, *r));
}

static void trace_pr_hex_dump(vm_thread_p thread, uint32_t *tip, int count)
{
	uint32_t saved_tip = *tip;
	
	for(int i = 0; i < count; i++) {
		uint8_t ti8 = trace_i8(thread, &saved_tip);
		printf("%02x ", ti8);
	}
	
	for(int i = 8 - count; i > 0; i--) {
		printf("   ");
	}
}

static void trace_inst_esac(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 0);
	printf("%s", op);
}

static void trace_inst_m_x_i_esac(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint8_t		arg2_v = trace_i8(thread, tip);
	
	printf("%s\t[%02x](0x%08x), #0x%02x", op, arg1_r, arg1_v, arg2_v);
}

static void trace_inst_m_x_mi_esac(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint8_t		arg2_r;
	uint32_t	arg2_v = trace_m_i8(thread, tip, &arg2_r);
	
	printf("%s\t[%02x](0x%08x), [%02x](0x%08x)", op, arg1_r, arg1_v, arg2_r, arg2_v);
}

static void trace_branch_x_i(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 1);
	
	uint8_t		arg1_r = trace_i8(thread, tip);
	
	uint32_t	branch_pc = *tip + (int8_t)arg1_r;
	printf("branch\t(%08x)", branch_pc);
}

static void trace_branch_x_mi(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 1);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint32_t	branch_pc = *tip + (int8_t)arg1_v;
	printf("branch\t[%02x](%08x)", arg1_r, branch_pc);
}

static void trace_branch_eq_x_m_i(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint8_t		arg2_r = trace_i8(thread, tip);

	uint32_t	branch_pc = *tip + (int8_t)arg2_r;
	printf("branch\t(%08x), [%02x](0x%08x)\t%s", branch_pc, arg1_r, arg1_v, arg1_v ? "" : "will branch");
}

static void trace_branch_eq_x_m_mi(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint8_t		arg2_r;
	uint32_t	arg2_v = trace_m_i8(thread, tip, &arg2_r);

	uint32_t	branch_pc = *tip + (int8_t)arg1_v;
	printf("branch\t[%02x](%08x), [%02x](0x%08x)\t%s", arg2_r, branch_pc, arg1_r, arg1_v, arg1_v ? "" : "will branch");
}

static void trace_branch_ne_x_m_i(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint8_t		arg2_r = trace_i8(thread, tip);

	uint32_t	branch_pc = *tip + (int8_t)arg2_r;
	printf("branch\t[%02x](%08x), [%02x](0x%08x)\t%s", arg2_r, branch_pc, arg1_r, arg1_v, arg1_v ? "will branch": "");
}

static void trace_branch_ne_x_m_mi(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint8_t		arg2_r;
	uint32_t	arg2_v = trace_m_i8(thread, tip, &arg2_r);

	uint32_t	branch_pc = *tip + (int8_t)arg2_v;
	printf("branch\t[%02x](%08x), [%02x](0x%08x)\t%s", arg2_r, branch_pc, arg1_r, arg1_v, arg1_v ? "will branch": "");
}

static void trace_mov_m_i(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 5);
	
	uint8_t		arg1_r = trace_i8(thread, tip);
	
	uint32_t	arg2_v = trace_i8(thread, tip);
	
	printf("%s\t[%02x], #0x%02x", op, arg1_r, arg2_v);
}

static void trace_mov_m_i32(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 5);
	
	uint8_t		arg1_r = trace_i8(thread, tip);
	
	uint32_t	arg2_v = trace_i32(thread, tip);
	
	printf("%s\t[%02x], #0x%08x", op, arg1_r, arg2_v);
}

static void trace_mov_m_mi(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);

	uint8_t		arg2_r;
	uint32_t	arg2_v = trace_m_i8(thread, tip, &arg2_r);
	
	printf("%s\t[%02x]:0x%08x, [%02x]:0x%08x", op, arg1_r, arg1_v, arg2_r, arg2_v);
}

static void trace_mov_m_mmi(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r = trace_i8(thread, tip);
	
	uint8_t		arg2_r;
	uint32_t	arg2_v = trace_m_i8(thread, tip, &arg2_r);
	
	printf("%s\t[%02x], (([%02x]0x%08x):0x%08x)", op, arg1_r, arg2_r, arg2_v, trace_m(thread, arg2_v));
}

static void trace_mov_mm_mi(vm_thread_p thread, uint32_t *tip, const char *op)
{
	trace_pr_hex_dump(thread, tip, 2);
	
	uint8_t		arg1_r;
	uint32_t	arg1_v = trace_m_i8(thread, tip, &arg1_r);
	
	uint8_t		arg2_r;
	uint32_t	arg2_v = trace_m_i8(thread, tip, &arg2_r);
	
	printf("%s\t[%02x]:0x%08x, [%02x]:0x%08x", op, arg1_r, arg1_v, arg2_r, arg2_v);
}

#define trace_asr_m_i		trace_inst_m_x_i_esac
#define trace_asr_m_mi		trace_inst_m_x_mi_esac

#define trace_ior_m_mi		trace_inst_m_x_mi_esac
#define trace_iow_m_mi		trace_inst_m_x_mi_esac

#define trace_ror_m_i		trace_inst_m_x_i_esac
#define trace_ror_m_mi		trace_inst_m_x_mi_esac
	
#define trace_rol_m_i		trace_inst_m_x_i_esac
#define trace_rol_m_mi		trace_inst_m_x_mi_esac


#undef INST_ESAC
#define INST_ESAC(esac, oper) \
	case esac: { \
		trace_inst_esac(thread, &tip, oper); \
	} break;

#undef INST_x_I_ESAC
#define INST_x_I_ESAC(esac, oper) \
	case esac: { \
		trace_##esac(thread, &tip, #oper); \
	} break;

#undef INST_x_MI_ESAC
#define INST_x_MI_ESAC(esac, oper) \
	case esac: { \
		trace_##esac(thread, &tip, #oper); \
	} break;

#undef INST_x_M_I_ESAC
#define INST_x_M_I_ESAC(esac, oper) \
	case esac: { \
		trace_##esac(thread, &tip, #oper); \
	} break;

#undef INST_x_M_MI_ESAC
#define INST_x_M_MI_ESAC(esac, oper) \
	case esac: { \
		trace_##esac(thread, &tip, #oper); \
	} break;

#undef INST_M_x_I_ESAC
#define INST_M_x_I_ESAC(esac, oper) \
	case esac: { \
		trace_inst_m_x_i_esac(thread, &tip, #oper); \
	} break;

#undef INST_M_x_MI_ESAC
#define INST_M_x_MI_ESAC(esac, oper) \
	case esac: { \
		trace_inst_m_x_mi_esac(thread, &tip, #oper); \
	} break;

void vm_run_thread_trace(vm_thread_p thread)
{
	uint32_t	tip = thread->ip;
	uint8_t		i8 = trace_i8(thread, &tip);
	
	printf("%06x: %02x ", tip, i8);
	
	switch(i8) {
		INST_ESAC_TABLE
		default:
			printf("undefined operation");
	}
	
	printf("\n");
}

