#include <signal.h>

#include <SDL/SDL.h>

#define kScreenWidth	320
#define kScreenHeight	200

#define kVideoBuffWidth		25
#define kVideoBuffHeight	24

#define	kVideoPixelWidth	(kVideoBuffWidth<<3)
#define kVideoScanlines		(kVideoBuffHeight<<3)
#define kVideoBufferSize	((kVideoBuffWidth*kVideoScanlines)<<2)

#define kVideoPixelTop		((kScreenHeight>>1)-(kVideoScanlines>>1))
#define kVideoPixelLeft		((kScreenWidth>>1)-(kVideoPixelWidth>>1))

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
	SDL_Event event;
	video_t video;
	vm_thread_t thread;

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
}

int main(int argc, char *argv[])
{
	vm_p vm = &default_vm_rec;

	signal(SIGINT, catch_sig);
	signal(SIGTERM, catch_sig);

	video_init(argc, argv, &vm->video);

	vm->state = vm_state_running;
	
	while(default_vm_rec.state != vm_state_done) {
		if(vm->video.needRefresh)
			video_update(&vm->video);

		vm_run_no_thread(&vm->thread);

		SDL_PollEvent(&vm->event);
		switch (vm->event.type) {
			case	SDL_QUIT:
				vm->state = vm_state_done;
				break;
			case	SDL_KEYDOWN: {
				int scancode = vm->event.key.keysym.scancode;
				if(0x1b == scancode)
					vm->state = vm_state_done;
			} break;
		}
	}

	return(0);
}

