####

target = vm_sdl

IPATH = .
IPATH += ./include

VPATH = ./src

CFLAGS += -O2
CFLAGS += -g -Wall

CPPFLAGS	+= --std=gnu99 -Wall

#LDFLAGS += -lpthread
LDFLAGS += -lSDL

OBJ = obj-${shell $(CC) -dumpmachine}

####

all: obj ${target} ${OBJ}/${target}.elf

project = ${OBJ}/${target}.elf

${project}: ${OBJ}/vm_sdl_main.o
${project}: ${OBJ}/vm_run_thread.o
${project}: ${OBJ}/vm_run_thread_trace.o
${project}: ${OBJ}/vm_flash_init.o

#${project}: ${OBJ}/${target}.o

${target}: ${project}
	@echo $@ done

####

${OBJ}/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD $<  -c -o $@

${OBJ}/%.elf:
	$(CC) -MMD $(CFLAGS)  $(LFLAGS) -o $@ $^ $(LDFLAGS)

obj: ${OBJ}

${OBJ}: 
	@mkdir -p ${OBJ}

clean-${OBJ}:
	rm -rf ${OBJ}

clean: clean-${OBJ}
	rm -rf ${target}

run: ${OBJ}/${target}.elf
	${OBJ}/${target}.elf
