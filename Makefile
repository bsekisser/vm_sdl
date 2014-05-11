####

ARCH = ${shell $(CC) -dumpmachine | sed -e 's/^x/i/' -e 's/\(.\).*/\1/'}

VPATH = ./src

CFLAGS += -O2
CFLAGS += -g -Wall

CPPFLAGS	+= --std=gnu99 -Wall

LDFLAGS += -lpthread
LDFLAGS += -lSDL

OBJ = obj-${shell $(CC) -dumpmachine}

${OBJ}: 
	@mkdir -p ${OBJ}

${OBJ}/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD $<  -c -o $@

${OBJ}/%.elf:
	$(CC) -MMD $(CFLAGS)  $(LFLAGS) -o $@ $^ $(LDFLAGS)

clean-${OBJ}:
	rm -rf ${OBJ}

####

target = vm_sdl

all: ${OBJ} ${target}

project = ${OBJ}/${target}.elf

${project}: ${OBJ}/vm_sdl_main.o
${project}: ${OBJ}/vm_run_thread.o
${project}: ${OBJ}/vm_flash_init.o

#${project}: ${OBJ}/${target}.o

${target}: ${project}


run: ${OBJ}/${target}.elf
	${OBJ}/${target}.elf

clean: clean-${OBJ}
	rm -rf ${target}
