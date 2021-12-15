#include <SDL2/SDL.h>
#include <cassert>
#include <vector>
#include <cmath>

#define PI 3.14159

SDL_Renderer *renderer = NULL;
SDL_Window *win = NULL;

void initSDL() {
	assert(SDL_Init(SDL_INIT_EVERYTHING) >= 0);
	assert(win = SDL_CreateWindow("Stickman",
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED,
				640, 480, SDL_WINDOW_RESIZABLE));
	assert(renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE));
}

void endSDL() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

class joint {
	float length=0;
	std::vector<joint*> children;
public:
	joint *parent = 0;
	bool adopted = true;
	float x=0, y=0, a=0;

	joint(float x, float y, float length) {
		joint::x = x;
		joint::y = y;
		joint::length = length;
	}

	joint() {
		x = 0;
		y = 0;
		length = 0;
		parent = 0;
		adopted = true;
		a = 0;
	}

	~joint() {
		for(int i = 0; i < children.size(); i++)
			if(!children.at(i)->adopted)
				delete children.at(i);
	}

	void attach(joint *child) {
		children.push_back(child);
		children.back()->parent = this;
	}

	void attachTo(joint *parent) {
		parent->attach(this);
	}

	void newChild(float x, float y, float length) {
		children.push_back(new joint(x, y, length));
		children.back()->adopted = false;
		children.back()->parent = this;
	}

	joint *child(int i) {
		return children.at(i);
	}

	joint *root() {
		if(!parent)
			return this;
		joint *p;
		for(p = parent; p->parent; p = p->parent);
		return p;
	}

	void restrict() {
		int w, h;
		SDL_GetWindowSize(win, &w, &h);
		if(x < 0)
			x = 0;
		if(y < 0)
			y = 0;
		if(x >= w)
			x = w-1;
		if(y >= h)
			y = h-1;
	}

	void move(float xm, float ym) {
		x += xm;
		y += ym;
		restrict();
	}

	void moveAll(float xm, float ym) {
		for(int i = 0; i < children.size(); i++)
			children.at(i)->moveAll(xm, ym);
		move(xm, ym);
	}
	
	void drawAll(float xo, float yo) {
		for(int i = 0; i < children.size(); i++)
			children.at(i)->drawAll(xo, yo);
		if(!parent)
			return;
		SDL_RenderDrawLine(renderer, x+xo, y+yo,
				parent->x+xo, parent->y+yo);
	}

	void correct(float mul, float amul) {
		if(!parent)
			return;

		float a = atan2(parent->y-y, parent->x-x);
		float ad = joint::a-a;
		if(ad > PI)
			ad -= PI*2;
		if(ad < -PI)
			ad += PI*2;
		a += (ad)*amul;

		float dx = parent->x-cosf(a)*length;
		float dy = parent->y-sinf(a)*length;

		x += (dx-x)*mul;
		y += (dy-y)*mul;
	}

	void correctAll(float mul, float amul) {
		for(int i = 0; i < children.size(); i++)
			children.at(i)->correctAll(mul, amul);
		correct(mul, amul);
		restrict();
	}

	void correctParent(float mul, float amul) {
		if(!parent)
			return;

		float a = atan2(y-parent->y, x-parent->x);
		a -= (joint::a-a)*amul;

		float dx = x-cosf(a)*length;
		float dy = y-sinf(a)*length;

		parent->x += (dx-parent->x)*mul;
		parent->y += (dy-parent->y)*mul;
		parent->restrict();
	}

	void correctAllParents(float mul, float amul) {
		for(int i = 0; i < children.size(); i++)
			children.at(i)->correctAllParents(mul, amul);
		correctParent(mul, amul);
	}
};

joint stickman;

void initStickman() {
	/* head */
	stickman.newChild(-20, 0, 20);
	stickman.child(0)->a = PI/3;
	/* arms */
	stickman.newChild(50, 0, 50);
	stickman.child(1)->newChild(100, 0, 50);
	stickman.child(1)->a = PI;
	stickman.child(1)->child(0)->a = PI/4;
	stickman.newChild(-50, 0, 50);
	stickman.child(2)->newChild(-100, 0, 50);
	stickman.child(2)->a = 0;
	/* body */
	stickman.newChild(0, 100, 80);
	stickman.child(3)->a = PI*1.35;
	/* legs */
	stickman.child(3)->newChild(50, 100, 50);
	stickman.child(3)->newChild(-50, 100, 50);
	stickman.child(3)->child(0)->a = PI*1.75;
	stickman.child(3)->child(0)->newChild(100, 100, 50);
	stickman.child(3)->child(0)->child(0)->a = PI;
	stickman.child(3)->child(1)->newChild(-100, 100, 50);
}

void drawCircle(float x, float y, float r) {
	for(float a = 0; a < PI*2; a += 0.01)
		SDL_RenderDrawLine(renderer, x, y,
				x+cosf(a)*r, y+sinf(a)*r);
}

void draw() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
	stickman.drawAll(0, 0);
	drawCircle(stickman.child(0)->x, stickman.child(0)->y, 20);
	SDL_RenderPresent(renderer);
}

int main() {
	initSDL();
	initStickman();

	bool quit = false;
	int lastUpdate = SDL_GetTicks();
	bool alive = true;
	while(!quit) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev))
			switch(ev.type) {
				case SDL_QUIT: quit = true; break;
				case SDL_MOUSEBUTTONDOWN:
					alive = !alive;
					break;
			}
		int currentTime = SDL_GetTicks();

		while(currentTime-lastUpdate > 10) {
			//stickman.child(1)->correctAllParents(0.1, 0.2);
			stickman.correctAll(0.7, 0.1*alive);

			lastUpdate += 10;
		}
		int mx, my;
		SDL_GetMouseState(&mx, &my);
		stickman.x = mx;
		stickman.y = my;


		draw();
	}

	endSDL();
	return 0;
}
