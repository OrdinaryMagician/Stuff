/*
	glfuzz.c : Fancy hardware filter.
	I was bored.
	(C)2016 Marisa Kirisame, UnSX Team.
	Released under the GNU GPLv3 (or later).
*/
#include <epoxy/gl.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <string.h>

GLuint prog, frag, vert;
GLuint flat;

const char *vcode =
"varying vec2 coord;\n"
"void main()\n"
"{\n"
"coord=(gl_Vertex.xy+1.0)*0.5;\n"
"gl_Position=gl_Vertex;\n"
"}\n";
const char *fcode =
"uniform float timer;\n"
"uniform vec2 resolution;\n"
"varying vec2 coord;\n"
"float rnd(in vec2 sd)\n"
"{\n"
"return cos(sd.y*3874.8674+sd.x*6783.5325)*2737.8474;\n"
"}\n"
"void main()\n"
"{\n"
"vec2 uv=floor(coord*resolution*0.5);\n"
"vec3 col=vec3(0.71,0.67,0.95)*2.0*abs(fract(rnd(uv)+timer*1.3526)-0.5);\n"
"uv=floor(coord*resolution*0.25);\n"
"col*=vec3(0.66,0.84,0.73)*2.0*abs(fract(rnd(uv)+timer*0.7843)-0.5);\n"
"uv=floor(coord*resolution*0.125);\n"
"col*=vec3(0.95,0.73,0.81)*2.0*abs(fract(rnd(uv)+timer*0.3725)-0.5);\n"
"gl_FragColor=vec4(col,1.0);\n"
"}\n";

float t = 0.f;
const int fw = 640, fh = 480;

SDL_Window *w;
SDL_GLContext *ctx;

#define NANOS_SEC 1000000000L

long ticker( void )
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	return ts.tv_nsec+ts.tv_sec*NANOS_SEC;
}

void DrawFrame( void )
{
	GLint uf;
	glUseProgram(prog);
	uf = glGetUniformLocation(prog,"timer");
	glUniform1f(uf,t);
	uf = glGetUniformLocation(prog,"resolution");
	glUniform2f(uf,fw,fh);
	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER,flat);
	glVertexPointer(3,GL_FLOAT,0,0);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	glFinish();
}

int main( int argc, char **argv )
{
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
	w = SDL_CreateWindow("Fuzz (OpenGL)",SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,fw,fh,
		SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
	ctx = SDL_GL_CreateContext(w);
	SDL_GL_SetSwapInterval(!((argc>1)&&!strcmp(argv[1],"--novsync")));
	GLint len, suc;
	char *log;
	/* fragment */
	frag = glCreateShader(GL_FRAGMENT_SHADER);
	len = strlen(fcode);
	glShaderSource(frag,1,&fcode,&len);
	glCompileShader(frag);
	glGetShaderiv(frag,GL_COMPILE_STATUS,&suc);
	if ( !suc )
	{
		glGetShaderiv(frag,GL_INFO_LOG_LENGTH,&len);
		log = malloc(len);
		glGetShaderInfoLog(frag,len,&suc,log);
		printf("fragment:\n%s\n",log);
		free(log);
	}
	/* vertex */
	vert = glCreateShader(GL_VERTEX_SHADER);
	len = strlen(vcode);
	glShaderSource(vert,1,&vcode,&len);
	glCompileShader(vert);
	glGetShaderiv(vert,GL_COMPILE_STATUS,&suc);
	if ( !suc )
	{
		glGetShaderiv(vert,GL_INFO_LOG_LENGTH,&len);
		log = malloc(len);
		glGetShaderInfoLog(vert,len,&suc,log);
		printf("vertex:\n%s\n",log);
		free(log);
	}
	/* program */
	prog = glCreateProgram();
	glAttachShader(prog,frag);
	glAttachShader(prog,vert);
	glLinkProgram(prog);
	glGetProgramiv(prog,GL_LINK_STATUS,&suc);
	if ( !suc )
	{
		glGetProgramiv(prog,GL_INFO_LOG_LENGTH,&len);
		log = malloc(len);
		glGetProgramInfoLog(prog,len,&suc,log);
		printf("program:\n%s\n",log);
		free(log);
	}
	glDeleteShader(frag);
	glDeleteShader(vert);
	/* flat */
	GLfloat v[12] =
	{
		-1.0,-1.0, 0.0,  1.0,-1.0, 0.0,
		-1.0, 1.0, 0.0,  1.0, 1.0, 0.0,
	};
	glGenBuffers(1,&flat);
	glBindBuffer(GL_ARRAY_BUFFER,flat);
	glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*12,v,GL_STATIC_DRAW_ARB);
	SDL_Event e;
	int active = 1;
	long tick, tock;
	while ( active )
	{
		while ( SDL_PollEvent(&e) ) if ( (e.type == SDL_QUIT)
			|| ((e.type == SDL_KEYDOWN)
			&& (e.key.keysym.sym == SDLK_ESCAPE)) ) active = 0;
		tick = ticker();
		DrawFrame();
		SDL_GL_SwapWindow(w);
		tock = ticker();
		float frame = (float)(tock-tick)/NANOS_SEC;
		printf("FPS: %.2f\n",1.f/frame);
		t += frame;
	}
	glDeleteBuffers(1,&flat);
	glDeleteProgram(prog);
	SDL_GL_DeleteContext(ctx);
	SDL_DestroyWindow(w);
	SDL_Quit();
	return 0;
}
