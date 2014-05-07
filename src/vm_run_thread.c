#include <stdint.h>

#include "vm_run_thread.h"

/* helpers */

static int_fast8_t _i8(vm_thread_p thread)
{
	return(thread->flash[thread->ip++]);
}

static int_fast16_t _i16(vm_thread_p thread)
{
	return(_i8(thread) | (_i8(thread) << 8));
}

static uint_fast32_t _mi8(vm_thread_p thread)
{
	uint32_t *ptr = (uint32_t *)&thread->zero[_i8(thread)];
	return(*ptr);
}

static uint_fast32_t _m_i8(vm_thread_p thread, uint_fast8_t *r)
{
	*r = _i8(thread);
	uint32_t *ptr = (uint32_t *)&thread->zero[*r];
	return(*ptr);
}

static void store32(vm_thread_p thread, uint_fast32_t maddr, uint_fast32_t data)
{
	uint32_t *ptr = (uint32_t *)&thread->zero[maddr];
	*ptr = data;
}

/* instruction functions */

static uint_fast32_t _add_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	return(arg1 + arg2);
}

static uint_fast32_t _and_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	return(arg1 & arg2);
}

static uint_fast32_t _eor_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	return(arg1 ^ arg2);
}

static uint_fast32_t _asr_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t shift)
{
	return(((signed)arg1) >> shift);
}

static void _branch_mi(vm_thread_p thread, int_fast32_t arg1)
{
	thread->ip += arg1;
}

static void _branch_eq_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	if(!arg1)
		_branch_mi(thread, arg2);
}

static void _branch_ne_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	if(arg1)
		_branch_mi(thread, arg2);
}

static uint_fast32_t _lsl_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t shift)
{
	return(arg1 << shift);
}

static uint_fast32_t _lsr_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t shift)
{
	return(arg1 >> shift);
}

static void _mov_m_mi(vm_thread_p thread, uint_fast32_t arg1)
{
	store32(thread, arg1, _mi8(thread));
}

static uint_fast32_t _or_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	return(arg1 | arg2);
}

/* Implimentation of rol and ror code, 
 * http://en.wikipedia.org/wiki/Circular_shift
 */
static uint_fast32_t _rol_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t shift)
{
	return((arg1 << shift) | (arg1 >> ((sizeof(arg1)) * CHAR_BITS - shift)));
}

/* Implimentation of rol and ror code, 
 * http://en.wikipedia.org/wiki/Circular_shift
 */
static uint_fast32_t _ror_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t shift)
{
	return((arg1 >> shift) | (arg1 << ((sizeof(arg1)) * CHAR_BITS - shift)));
}

static uint_fast32_t _sub_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	return(arg1 + arg2);
}

#if 0
	#define STATE(_f, args...) printf("%06x: " _f, thread->ip, ## args);
#else
	#define STATE(_f, args...)
#endif

#undef INST_ESAC
#define INST_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]\n", # esac); \
		oper(thread); \
	} break;

#undef INST_x_I_ESAC
#define INST_x_I_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]\n", # esac); \
		oper(thread, _i8(thread)); \
	} break;

#undef INST_x_MI_ESAC
#define INST_x_MI_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]\n", # esac); \
		oper(thread, _mi8(thread)); \
	} break;

#undef INST_x_M_I_ESAC
#define INST_x_M_I_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]\n", # esac); \
		uint_fast32_t arg1v = _mi8(thread); \
		oper(thread, arg1v, _i8(thread)); \
	} break;

#undef INST_x_M_MI_ESAC
#define INST_x_M_MI_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]\n", # esac); \
		uint_fast32_t arg1v = _mi8(thread); \
		oper(thread, arg1v, _mi8(thread)); \
	} break;

#undef INST_M_x_I_ESAC
#define INST_M_x_I_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]\n", # esac); \
		uint_fast8_t arg1r; \
		uint_fast32_t arg1v = _m_i8(thread, &arg1r); \
		store32(thread, arg1r, oper(thread, arg1v, _i8(thread))); \
	} break;

#undef INST_M_x_MI_ESAC
#define INST_M_x_MI_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]\n", # esac); \
		uint_fast8_t arg1r; \
		uint_fast32_t arg1v = _m_i8(thread, &arg1r); \
		store32(thread, arg1r, oper(thread, arg1v, _mi8(thread))); \
	} break;

static int vm_run_thread(vm_thread_p thread)
{
	thread->ip &= ~PAGE_MASK;
	switch(_i8(thread)) {
		INST_ESAC_TABLE
		default:
			usleep(1);
			break;
	}
	
	return(0);
}

void vm_run_no_thread(vm_thread_p thread)
{
	uint32_t count = thread->runCycles;

	while((0 != count) && (0 == vm_run_thread(thread))) {
		count--;
		thread->cycle++;
	}
}

