#include <epoxy/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

const char *vsrc =
"#version 430\n"
"layout(location=0) in vec3 vPosition;\n"
"layout(location=1) in vec2 vCoord;\n"
"out vec2 fCoord;\n"
"void main()\n"
"{\n"
"\tgl_Position.xyz = vPosition;\n"
"\tgl_Position.w = 1.0;\n"
"\tfCoord = vCoord;\n"
"}\n";
const char *fsrc =
"#version 430\n"
"in vec2 fCoord;\n"
"layout(location=0) out vec4 FragColor;\n"
"layout(binding=0) uniform samplerCube Texture;\n"
"void main()\n"
"{\n"
"\tvec2 p = fCoord-0.5;\n"
"\tfloat pitch = length(p)*3.14159265;\n"
"\tfloat roll = atan(p.y,p.x);\n"
"\tvec3 ccoord = vec3(sin(pitch)*cos(roll),-cos(pitch),sin(pitch)*sin(roll));\n"
"\tvec4 res = texture(Texture,-ccoord);\n"
"\tFragColor = res;\n"
"}\n";

GLint prog;
GLuint vao, vbuf, tex;

int tex_load( const char *basename )
{
	const char f[6] = {'b','f','l','r','d','t'};
	const int glf[6] =
	{
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	};
	char filename[256];
	glGenTextures(1,&tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP,tex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
	for ( int i=0; i<6; i++ )
	{
		snprintf(filename,256,"%s_%c.png",basename,f[i]);
		SDL_Surface *tx = IMG_Load(filename);
		if ( !tx )
		{
			fprintf(stderr,"could not load %s: %s\n",filename,SDL_GetError());
			return -1;
		}
		SDL_Surface *txconv = SDL_ConvertSurfaceFormat(tx,SDL_PIXELFORMAT_RGBA32,0);
		glTexImage2D(glf[i],0,GL_RGBA,txconv->w,txconv->h,0,GL_RGBA,GL_UNSIGNED_BYTE,txconv->pixels);
		SDL_FreeSurface(txconv);
		SDL_FreeSurface(tx);
	}
	return 0;
}

void init_render()
{
	glClearColor(0.5f,0.5f,0.5f,1.f);
	glClearDepth(1.f);
	glUseProgram(prog);
	glBindBuffer(GL_ARRAY_BUFFER,vbuf);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindTexture(GL_TEXTURE_CUBE_MAP,tex);
	glActiveTexture(GL_TEXTURE0);
}

void draw_frame()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*5,
		(char*)(0));
	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(float)*5,
		(char*)(sizeof(float)*3));
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void scr_load()
{
	glGenVertexArrays(1,&vao);
	glBindVertexArray(vao);
	glGenBuffers(1,&vbuf);
	glBindBuffer(GL_ARRAY_BUFFER,vbuf);
	float vboe[20] =
	{
		-1, -1, 0, 0., 0.,
		-1,  1, 0, 0., 1.,
		 1, -1, 0, 1., 0.,
		 1,  1, 0, 1., 1.,
	};
	glBufferData(GL_ARRAY_BUFFER,sizeof(float)*20,&vboe[0],
		GL_STATIC_DRAW);
}

GLint compile_shader( GLenum type, const char *src )
{
	GLint hnd, len, suc;
	char *log;
	hnd = glCreateShader(type);
	len = strlen(src);
	glShaderSource(hnd,1,&src,&len);
	glCompileShader(hnd);
	glGetShaderiv(hnd,GL_COMPILE_STATUS,&suc);
	if ( !suc )
	{
		glGetShaderiv(hnd,GL_INFO_LOG_LENGTH,&len);
		log = malloc(len);
		glGetShaderInfoLog(hnd,len,&suc,log);
		fprintf(stderr,"Shader compile error:\n%s\n",log);
		free(log);
		return -1;
	}
	return hnd;
}

GLint link_shader( GLint geom, GLint vert, GLint frag )
{
	GLint hnd, suc, len;
	char *log;
	hnd = glCreateProgram();
	if ( geom != -1 ) glAttachShader(hnd,geom);
	if ( vert != -1 ) glAttachShader(hnd,vert);
	if ( frag != -1 ) glAttachShader(hnd,frag);
	glLinkProgram(hnd);
	glGetProgramiv(hnd,GL_LINK_STATUS,&suc);
	if ( !suc )
	{
		glGetShaderiv(hnd,GL_INFO_LOG_LENGTH,&len);
		log = malloc(len);
		glGetShaderInfoLog(hnd,len,&suc,log);
		fprintf(stderr,"Shader link error:\n%s\n",log);
		free(log);
		return -1;
	}
	return hnd;
}

int prog_load()
{
	GLint vert, frag;
	if ( (frag=compile_shader(GL_FRAGMENT_SHADER,fsrc)) == -1 ) return -1;
	if ( (vert=compile_shader(GL_VERTEX_SHADER,vsrc)) == -1 ) return -1;
	if ( (prog=link_shader(-1,vert,frag)) == -1 ) return -1;
	glDeleteShader(frag);
	glDeleteShader(vert);
	return 0;
}

int main( int argc, char **argv )
{
	int retcode = 0;
	if ( argc < 2 )
	{
		fprintf(stderr,"usage: cube2enviro <basename> [size]\n");
		return 0;
	}
	int sz = 256;
	if ( argc > 2 ) sscanf(argv[2],"%u",&sz);
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS);
	IMG_Init(IMG_INIT_PNG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,4);
	SDL_Window *win = SDL_CreateWindow("cube2enviro",
		SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,sz,sz,
		SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
	SDL_GLContext *ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(1);
	if ( tex_load(argv[1]) == -1 )
	{
		retcode = 1;
		goto bail;
	}
	if ( prog_load() == -1 )
	{
		retcode = 2;
		goto bail;
	}
	scr_load();
	init_render();
	SDL_Event e;
	int active = 1;
	float frame = 0.f;
	long tick, tock;
	while ( active )
	{
		while ( SDL_PollEvent(&e) )
		{
			if ( (e.type == SDL_QUIT) || (((e.type == SDL_KEYDOWN)
				 || (e.type == SDL_KEYUP))
				 && (e.key.keysym.sym == SDLK_ESCAPE))  )
				active = 0;
		}
		draw_frame();
		SDL_GL_SwapWindow(win);
	}
bail:
	SDL_GL_DeleteContext(ctx);
	SDL_DestroyWindow(win);
	IMG_Quit();
	SDL_Quit();
	return retcode;
}

