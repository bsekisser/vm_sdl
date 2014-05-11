#include <stdint.h>
#include <stdio.h> // printf
#include <unistd.h>

#include "vm_run_thread.h"

#define DEBUG(x)

/* helpers */

static uint_fast8_t _i8(vm_thread_p thread)
{
	uint8_t data = thread->flash[thread->ip++];
	DEBUG(printf("%02x ", data));
	return(data);
}

static uint_fast16_t _i16(vm_thread_p thread)
{
	uint_fast8_t lo = _i8(thread);
	uint_fast8_t hi = _i8(thread);

	return((hi << 8) | lo);
}

static uint_fast32_t _i32(vm_thread_p thread)
{
	uint_fast16_t lo = _i16(thread);
	uint_fast16_t hi = _i16(thread);
	
	return((hi << 16) | lo);
}

static uint_fast32_t _mi8(vm_thread_p thread)
{
	uint32_t *ptr = (uint32_t *)&thread->zero[_i8(thread)];
	DEBUG(printf("[0x%08x] ", *ptr));
	return(*ptr);
}

static uint_fast32_t _m_i8(vm_thread_p thread, uint_fast8_t *r)
{
	*r = _i8(thread);
	uint32_t *ptr = (uint32_t *)&thread->zero[*r];
	DEBUG(printf("[0x%08x] ", *ptr));
	return(*ptr);
}

static void store32(vm_thread_p thread, uint_fast32_t maddr, uint_fast32_t data)
{
	uint32_t *ptr = (uint32_t *)&thread->zero[maddr];
	DEBUG(printf("((0x%06x) = 0x%08x) ", maddr, *ptr));
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
	thread->ip += (int8_t)arg1;
	DEBUG(printf("thread->ip = %06x", thread->ip));
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

static void _mov_m_i(vm_thread_p thread, uint_fast32_t arg1)
{
	store32(thread, arg1, _i8(thread));
}

static void _mov_m_i32(vm_thread_p thread, uint_fast32_t arg1)
{
	store32(thread, arg1, _i32(thread));
}

static void _mov_m_mi(vm_thread_p thread, uint_fast32_t arg1)
{
	store32(thread, arg1, _mi8(thread));
}

static uint_fast32_t _mul_m_mi(vm_thread_p thread, uint_fast32_t arg1, uint_fast32_t arg2)
{
	return(arg1 * arg2);
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

#if 1
//	#define STATE(_f, args...) printf("%06x: " _f, thread->ip, ## args);
	#define STATE(_f, args...) DEBUG(printf(_f " ", ## args));
#else
	#define STATE(_f, args...)
#endif

#undef INST_ESAC
#define INST_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]", # esac); \
		oper(thread); \
	} break;

#undef INST_x_I_ESAC
#define INST_x_I_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]", # esac); \
		oper(thread, _i8(thread)); \
	} break;

#undef INST_x_MI_ESAC
#define INST_x_MI_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]", # esac); \
		oper(thread, _mi8(thread)); \
	} break;

#undef INST_x_M_I_ESAC
#define INST_x_M_I_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]", # esac); \
		uint_fast32_t arg1v = _mi8(thread); \
		oper(thread, arg1v, _i8(thread)); \
	} break;

#undef INST_x_M_MI_ESAC
#define INST_x_M_MI_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]", # esac); \
		uint_fast32_t arg1v = _mi8(thread); \
		oper(thread, arg1v, _mi8(thread)); \
	} break;

#undef INST_M_x_I_ESAC
#define INST_M_x_I_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]", # esac); \
		uint_fast8_t arg1r; \
		uint_fast32_t arg1v = _m_i8(thread, &arg1r); \
		store32(thread, arg1r, oper(thread, arg1v, _i8(thread))); \
	} break;

#undef INST_M_x_MI_ESAC
#define INST_M_x_MI_ESAC(esac, oper) \
	case esac: { \
		STATE("esac[%s]", # esac); \
		uint_fast8_t arg1r; \
		uint_fast32_t arg1v = _m_i8(thread, &arg1r); \
		store32(thread, arg1r, oper(thread, arg1v, _mi8(thread))); \
	} break;

static int vm_run_thread(vm_thread_p thread)
{
	thread->ip &= ~PAGE_MASK;

	DEBUG(printf("%06x: ", thread->ip));
	
	switch((uint8_t)_i8(thread)) {
		INST_ESAC_TABLE
		case 0xff:
			abort();
			break;
		default:
			/* null or otherwise undefined operation...  sleep it off. */
			usleep(1);
			break;
	}
	
	DEBUG(printf("\n"));
	
	return(0);
}

void vm_run_no_thread(vm_thread_p thread)
{
	uint32_t count = thread->runCycles;

	while((count--) && !vm_run_thread(thread)) ;
	
	thread->cycle += thread->runCycles - count;
}

