#include <signal.h>
#include <unistd.h> // sleep

#include <SDL/SDL.h>

#define kScreenWidth	320
#define kScreenHeight	200

#define kVideoBuffWidth		(kScreenWidth >> 3)
#define kVideoBuffHeight	(kScreenHeight >> 3)

#define	kVideoPixelWidth	(kVideoBuffWidth << 3)
#define kVideoScanlines		(kVideoBuffHeight << 3)
#define kVideoBufferSize	((kVideoBuffWidth * kVideoScanlines) << 2)

#define kVideoPixelTop		((kScreenHeight >> 1) - (kVideoScanlines >> 1))
#define kVideoPixelLeft		((kScreenWidth >> 1) - (kVideoPixelWidth >> 1))

#define kVideoRefresh		30

typedef struct video_buffer_t  {
	uint8_t data[kVideoBufferSize];
	int x, y;
}video_buffer_t;

typedef struct video_t {
	SDL_Surface* surface;
	video_buffer_t buffer;
	int needRefresh;
}video_t, *video_p;

#include "vm_run_thread.h"

enum {
	vm_state_done,
	vm_state_crashed,
	vm_state_stopped,
	vm_state_step_one,
	vm_state_running,
};

typedef struct vm_t *vm_p;
typedef struct vm_t {
	SDL_Event	event;
	video_t		video;
	vm_thread_t	thread;
	
	int state;
	
	void (*run)(vm_p vm);
	void (*terminate)(vm_p vm);
	
}vm_t;

vm_t default_vm_rec;

void SDLInit(int argc, char* argv[], SDL_Surface** surface) {
	SDL_Init(SDL_INIT_VIDEO);

//	*surface=SDL_SetVideoMode(kScreenWidth, kScreenHeight, 16, SDL_FULLSCREEN|SDL_DOUBLEBUF|SDL_HWSURFACE);
	*surface=SDL_SetVideoMode(kScreenWidth, kScreenHeight, 16, SDL_SWSURFACE);
	if(*surface==NULL)
		exit(0);

	SDL_EnableKeyRepeat(125, 50);
}

void video_init(int argc, char *argv[], video_t* video) {
	SDLInit(argc, argv, &video->surface);

	video->buffer.x = 0;
	video->buffer.y = 0;
	
	video->needRefresh = 1;
}

void catch_sig(int sign)
{
	printf("\n\n\n\nsignal caught, terminating.\n\n");

	default_vm_rec.state = vm_state_done;
//	default_vm_rec.terminate(&default_vm_rec);

	exit(0);
}

void video_update(video_p video) {
	SDL_LockSurface(video->surface);
	
//	int i;
	for(int i=0; i<((kScreenWidth * kScreenHeight) >> 1); i++)
		((uint32_t *)video->surface->pixels)[i] = default_vm_rec.thread.zero[i];
	
	SDL_UnlockSurface(video->surface);
	SDL_Flip(video->surface);
}

static inline uint64_t get_dtime(void) {
   	uint32_t hi, lo;
   	
	__asm__ __volatile__ ("xorl %%eax,%%edx\n" : : : "%eax", "%edx");
	__asm__ __volatile__ ("xorl %%eax,%%edx\n" : : : "%eax", "%edx");
	__asm__ __volatile__ ("xorl %%eax,%%edx\n" : : : "%eax", "%edx");
	__asm__ __volatile__ ("xorl %%eax,%%edx\n" : : : "%eax", "%edx");
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	
	return(((uint64_t)hi << 32) | (uint64_t)lo);
}

#define KHz(hz) ((hz)*1000ULL)
#define MHz(hz) KHz(KHz(hz))

static uint64_t calibrate_get_dtime_loop(void)
{
   	uint64_t start, elapsedTime;

	start = get_dtime();
	elapsedTime = get_dtime() - start;

	int i;
	for(i=2; i<=1024; i++) {
		start = get_dtime();
		elapsedTime += get_dtime() - start;
	}
		
	return(elapsedTime / i);
	
}

static uint64_t calibrate_get_dtime_sleep(void)
{
   	uint64_t start = get_dtime();
	
	sleep(1);
		
	return(get_dtime() - start);
}

static uint64_t dtime_calibrate(void)
{
	uint64_t cycleTime = calibrate_get_dtime_loop();
	uint64_t elapsedTime, ecdt;
	double emhz;

	printf("%s: calibrate_get_dtime_cycles(%016llu)\n", __FUNCTION__, cycleTime);

	elapsedTime = 0;

	for(int i = 1; i <= 3; i++) {
		elapsedTime += calibrate_get_dtime_sleep() - cycleTime;

		ecdt = elapsedTime / i;
		emhz = ecdt / MHz(1);
		printf("%s: elapsed time: %016llu  ecdt: %016llu  estMHz: %010.4f\n", __FUNCTION__, elapsedTime, ecdt, emhz);
	}
	return(ecdt);
}

void vm_thread_flash_init(vm_thread_p thread);

int main(int argc, char *argv[])
{
	int count = 0;
	
	vm_p vm = &default_vm_rec;

	signal(SIGINT, catch_sig);
	signal(SIGTERM, catch_sig);

	video_init(argc, argv, &vm->video);

	vm->state = vm_state_running;

	uint64_t cyclesPerSecond = dtime_calibrate();
	
	vm->thread.runCycles = ((cyclesPerSecond / 100) / kVideoRefresh);

	vm_thread_flash_init(&vm->thread);

	uint64_t run_start_time = get_dtime();
	while(default_vm_rec.state != vm_state_done) {
		vm_run_no_thread(&vm->thread);

		if(vm->video.needRefresh) {
			video_update(&vm->video);
		} count++; if(30 > count) {
			uint64_t elapsed_dtime = get_dtime() - run_start_time;
//			double run_seconds = elapsed_dtime / (double)cyclesPerSecond;
			double eacdt = (double)elapsed_dtime / (double)vm->thread.cycle;
			vm->thread.runCycles = (cyclesPerSecond / kVideoRefresh) / eacdt;
			double emips = cyclesPerSecond / (eacdt * (double)1000000.0);
			printf("[vm_run_thread] - cycle: %016llu elapsed: %016llu eacdt: %08.4f run_cycles: %016u mips: %08.4f\r", 
				vm->thread.cycle, elapsed_dtime, eacdt, vm->thread.runCycles, emips);
			count = 0;
		}
		
		SDL_PollEvent(&vm->event);
		switch (vm->event.type) {
			case	SDL_QUIT:
				vm->state = vm_state_done;
				break;
			case	SDL_KEYDOWN: {
				int scancode = vm->event.key.keysym.scancode;
				if(/*0x1b*/ 0x09 == scancode)
					vm->state = vm_state_done;
			} break;
		}
	}

	printf("\n\n");

	return(0);
}

