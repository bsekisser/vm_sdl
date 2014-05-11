#define CHAR_BITS 8

/* PAGE_SHIFT, PAGE_SIZE, PAGE_MASK part of linux kernel */
#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12
#endif

/* PAGE_SHIFT, PAGE_SIZE, PAGE_MASK part of linux kernel */
#ifndef PAGE_SIZE
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif

/* PAGE_SHIFT, PAGE_SIZE, PAGE_MASK part of linux kernel */
#ifndef PAGE_MASK
#define PAGE_MASK (~(PAGE_SIZE - 1))
#endif

#define INST_ESAC_TABLE \
		INST_M_x_I_ESAC(add_m_i, _add_m_mi) \
		INST_M_x_MI_ESAC(add_m_mi, _add_m_mi) \
		INST_M_x_I_ESAC(and_m_i, _and_m_mi) \
		INST_M_x_MI_ESAC(and_m_mi, _and_m_mi) \
		INST_M_x_I_ESAC(asr_m_i, _asr_m_mi) \
		INST_M_x_MI_ESAC(asr_m_mi, _asr_m_mi) \
		INST_x_I_ESAC(branch_x_i, _branch_mi) \
		INST_x_MI_ESAC(branch_x_mi, _branch_mi) \
		INST_x_M_I_ESAC(branch_eq_x_m_i, _branch_eq_m_mi) \
		INST_x_M_MI_ESAC(branch_eq_x_m_mi, _branch_eq_m_mi) \
		INST_x_M_I_ESAC(branch_ne_x_m_i, _branch_ne_m_mi) \
		INST_x_M_MI_ESAC(branch_ne_x_m_mi, _branch_ne_m_mi) \
		INST_M_x_I_ESAC(eor_m_i, _eor_m_mi) \
		INST_M_x_MI_ESAC(eor_m_mi, _eor_m_mi) \
		INST_M_x_I_ESAC(lsl_m_i, _lsl_m_mi) \
		INST_M_x_MI_ESAC(lsl_m_mi, _lsl_m_mi) \
		INST_M_x_I_ESAC(lsr_m_i, _lsr_m_mi) \
		INST_M_x_MI_ESAC(lsr_m_mi, _lsr_m_mi) \
		INST_x_I_ESAC(mov_m_i, _mov_m_i) \
		INST_x_I_ESAC(mov_m_i32, _mov_m_i32) \
		INST_x_MI_ESAC(mov_m_mi, _mov_m_mi) \
		INST_M_x_I_ESAC(mul_m_i, _mul_m_mi) \
		INST_M_x_MI_ESAC(mul_m_mi, _mul_m_mi) \
		INST_M_x_I_ESAC(or_m_i, _or_m_mi) \
		INST_M_x_MI_ESAC(or_m_mi, _or_m_mi) \
		INST_M_x_I_ESAC(rol_m_i, _rol_m_mi) \
		INST_M_x_MI_ESAC(rol_m_mi, _rol_m_mi) \
		INST_M_x_I_ESAC(ror_m_i, _ror_m_mi) \
		INST_M_x_MI_ESAC(ror_m_mi, _ror_m_mi) \
		INST_M_x_I_ESAC(sub_m_i, _sub_m_mi) \
		INST_M_x_MI_ESAC(sub_m_mi, _sub_m_mi)

#define INST_ESAC(esac, oper) \
		esac,
		
#define INST_x_I_ESAC(esac, oper) \
		esac,

#define INST_x_MI_ESAC(esac, oper) \
		esac,

#define INST_x_M_I_ESAC(esac, oper) \
		esac,

#define INST_x_M_MI_ESAC(esac, oper) \
		esac,

#define INST_M_x_I_ESAC(esac, oper) \
		esac,

#define INST_M_x_MI_ESAC(esac, oper) \
		esac,

enum {
	undefined = 0,
	INST_ESAC_TABLE
};

typedef struct vm_thread_t {
//	pthread_t		thread;
	uint32_t		runCycles;
	uint64_t		cycle;

	uint8_t			flash[PAGE_SIZE + 3];
	uint8_t			zero[16 * PAGE_SIZE];
	uint32_t		ip;
	uint32_t		sp;
}vm_thread_t, *vm_thread_p;

void vm_run_no_thread(vm_thread_p thread);

