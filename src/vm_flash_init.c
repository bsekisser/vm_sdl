#include <stdint.h>
#include <stdio.h> // printf

#include "vm_run_thread.h"

#define DEBUG(x) x

#define c(data) ({ \
		DEBUG(printf("%02x ", data & 0xff)); \
		thread->flash[here++] = (uint8_t)(data); \
	})

#define c16(data) c(data); c((data) >> 8);
#define c32(data) c16(data); c16((data) >> 16);

#define add_r_i(r, i) ({ \
		c(add_m_i); c(r << 2); c(i); \
	})

#if 0
#define call(addr) ({ \
		c(call_x_m); resolve_label(addr);
	})
#else
	#define call(x)
#endif

#define mov_r_r(d, s) ({ \
		c(mov_m_mi); c(d << 2); c(s << 2); \
	})

#define mov_r_i32(d, i) ({ \
		c(mov_m_i32); c(d << 2); c32(i); \
	})

#define mov_r_ri(d, s) ({ \
		c(mov_m_mmi); c(d << 2); c(s << 2); \
	})

#define mov_ri_r(d, s) ({ \
		c(mov_mm_mi); c(d << 2); c(s << 2); \
	})

#define push(r) ({ \
		sub_r_i(rESP, 4); \
		SET_TOS(r); \
	})

#define pop(r) ({ \
		TOS(r); \
		add_r_i(rESP, 4); \
	})

#define sub_r_i(r, i) ({ \
		c(sub_m_i); c(r << 2); c(i); \
	})


#define begin() rstk[rsp++] = (uint32_t)here;
#define again() ({ \
	c(branch_x_i); \
	uint32_t branch_pc = rstk[--rsp] - ((uint32_t)here + 1); \
	c(branch_pc); \
	})

#define if_eq(r) ({ \
		c(branch_ne_x_m_i); c(r); \
		begin(); \
		c(0); \
	})

#define if_ne(r) ({ \
		c(branch_eq_x_m_i); c(r); \
		begin(); \
		c(0); \
	})

#define then() ({ \
		uint8_t *saved_here = here; \
		here = (uint8_t *)rstk[--rsp]; \
		uint32_t branch_pc = (uint32_t)saved_here - ((uint32_t)here + 1); \
		c(branch_pc); \
		here = saved_here; \
	})

enum {
	rEAX = 0,
	rEBX,
	rECX,
	rEDX,
	
	rESI,
	rEDI,
	
	rEBP,
	rESP
};

#define IJMP(r) ({ \
		c(branch_x_mi); c(rEAX); \
	})

#define LODSL ({ \
		mov_r_ri(rEAX, rESI); \
		add_r_i(rESI, sizeof(uint32_t)); \
	})

#define NEXT ({ \
		LODSL; \
		IJMP(EAX); \
	})

#define NOS(r) ({ \
		mov_r_r(r, rESP); \
		add_r_i(r, 4); \
		mov_r_ri(r, r); \
	})

#define PUSHRSP(r) ({ \
		sub_r_i(rESP, 4); \
		SET_TOS(r); \
	})

#define POPRSP(r) ({ \
		TOS(r); \
		add_r_i(rESP, 4); \
	})

#define SET_TOS(r) ({ \
	mov_ri_r(rESP, r); \
	})
	
#define TOS(r) ({ \
		mov_r_ri(r, rESP); \
	})

#define label(x) uint32_t x = here;

#define defcode(name, name_len, flags, code_label)

void vm_thread_flash_init(vm_thread_p thread)
{
	uint32_t	rstk[32];
	int		rsp = 0;
	
	DEBUG(printf("\n\n\n\nbegin: %s\n\n\n\n", __FUNCTION__));
	
	uint32_t here = 0;

	mov_r_i32(rESP, M_ADDR_SIZE - 1);

label(DOCOL);
	PUSHRSP(rESI);
	add_r_i(rEAX, 4);
	mov_r_r(rESI, rEAX);
	NEXT;

	uint32_t var_S0 = 0xff;
	uint32_t return_stack_top = 0xfe;

label(cold_start);
//	c32(0);

label(start);
	mov_r_r(var_S0, rESP);
	mov_r_r(rEBP, return_stack_top);
	call(setup_data_segment);
	mov_r_i32(rESI, cold_start);
	NEXT;
	
defcode("DROP", 4, 0, DROP);
	pop(rEAX);
	NEXT;

defcode("SWAP", 4, 0, SWAP);
	pop(rEAX);
	pop(rEBX);
	push(rEAX);
	push(rEBX);
	NEXT;

defcode("DUP", 3, 0, DUP);
	TOS(rEAX);
	push(rEAX);
	NEXT;

defcode("OVER", 4, 0, OVER);
#if 0
	pop(rEAX);
	pop(rEBX);
	push(rEAX);
	push(rEBX);
	push(rEAX);
#else
	NOS(rEAX);
	push(rEAX);
#endif
	NEXT;

defcode("ROT", 3, 0, ROT)
	pop(rEAX);
	pop(rEBX);
	pop(rECX);
	push(rEBX);
	push(rEAX);
	push(rECX);
	NEXT;

defcode("-ROT", 3, 0, NROT)
	pop(rEAX);
	pop(rEBX);
	pop(rECX);
	push(rEAX);
	push(rECX);
	push(rEBX);
	NEXT;

defcode("2DROP", 5, 0, TWODROP)
	pop(rEAX);
	pop(rEAX);
	NEXT;

defcode("2DUP", 4, 0, TWODUP)
	TOS(rEAX);
	NOS(rEBX);
	push(rEBX);
	push(rEAX);
	NEXT;

defcode("2SWAP", 5, 0, TWOSWAP)
	pop(rEAX);
	pop(rEBX);
	pop(rECX);
	pop(rEDX);
	push(rEBX);
	push(rEAX);
	push(rEDX);
	push(rECX);
	NEXT;

defcode("?DUP", 4, 0, QDUP)
	TOS(rEAX);
	if_ne(rEAX << 2);
		push(rEAX);
	then();
	NEXT;

defcode("1+", 2, 0, INCR)
	TOS(rEAX);
	add_r_i(rEAX, 1);
	SET_TOS(rEAX);

defcode("1-", 2, 0, DECR)
	TOS(rEAX);
	sub_r_i(rEAX, 1);
	SET_TOS(rEAX);

defcode("4+", 2, 0, INCR4)
	TOS(rEAX);
	add_r_i(rEAX, 4);
	SET_TOS(rEAX);

defcode("4-", 2, 0, DECR4)
	TOS(rEAX);
	sub_r_i(rEAX, 4);
	SET_TOS(rEAX);

defcode("+", 1, 0, ADD)
	pop(rEAX);
	TOS(rEBX);
	add_r_i(rEBX, rEAX);
	SET_TOS(rEBX);

defcode("-", 1, 0, SUB)
	pop(rEAX);
	TOS(rEBX);
	sub_r_i(rEBX, rEAX);
	SET_TOS(rEBX);

	DEBUG(printf("\n\n\n\nend: %s\n\n\n\n", __FUNCTION__));
}
