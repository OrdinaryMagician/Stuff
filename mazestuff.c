/*
	A C implementation of some dungeon generator, or at least an attempt.
	Based mainly on http://journal.stuffwithstuff.com/2014/12/21/rooms-and-mazes
	(C)2019 Marisa Kirisame, UnSX Team.
	Released under the MIT license.
*/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>

// map dimensions
// should be odd, otherwise you'll get some "padding" on the bottom and right
#define MAP_WIDTH 32
#define MAP_HEIGHT 32
// dimensions of the display window
#define WIN_WIDTH (MAP_WIDTH*16)
#define WIN_HEIGHT (MAP_HEIGHT*16)
// size of each cell in pixels when rendered
#define CW (WIN_WIDTH/MAP_WIDTH)
#define CH (WIN_HEIGHT/MAP_HEIGHT)

// delay after each "draw map" operation
// helps visualize the generation better
#define GENDELAY 10

// room generator parameters
// sizes must be odd
// more tries means more dense room placement
// I wonder what the probability of there being only one room could be
#define ROOM_MIN 3
#define ROOM_MAX 7
#define ROOM_TRIES 100

// tile types
#define TILE_WALL 0
#define TILE_AIR  1
#define TILE_DOOR 2

// probability of mazes being more squiggly
#define WINDING_PCT 0.35
// probability of additional doors into a region appearing
#define EXTDOOR_PCT 0.15
// probability of doors connecting back to the same region
#define SELFDOOR_PCT 0.25

// the map itself
uint8_t map[MAP_WIDTH][MAP_HEIGHT] = {{TILE_WALL}};
// what region each cell is part of
// 0 = no region
int region[MAP_WIDTH][MAP_HEIGHT] = {{0}};
// secondary region, for grouping rooms
int sregion[MAP_WIDTH][MAP_HEIGHT] = {{0}};

SDL_Surface *ws;
SDL_Window *w;

int active = 1;

// a 2d vector
typedef struct
{
	int x, y;
} vec2_t;

// a room
typedef struct
{
	int x1, y1, x2, y2;
} room_t;

// a connection between two regions
typedef struct
{
	int x, y, r[2];
} conn_t;

// do these rooms touch each other?
int room_intersect( room_t* a, room_t* b )
{
	return ((a->x1 <= b->x2) && (a->x2 >= b->x1)
		&& (a->y1 <= b->y2) && (a->y2 >= b->y1));
}

vec2_t dirs[8] =
{
	{ 0,-1},	// north
	{ 0, 1},	// south
	{-1, 0},	// west
	{ 1, 0},	// east
	{-1,-1},	// northwest
	{ 1,-1},	// northeast
	{-1, 1},	// southwest
	{ 1, 1},	// southeast
};

int current_region = 0;
int current_sregion = 0;

// RNG stuff
double FRandom( double a, double b )
{
	return a + (rand()/(RAND_MAX/(b-a)));
}
int Random( int a, int b )
{
	return FRandom(a,b);
}

// draw the map surface onto the window surface
// black for walls, white for air
void drawmap( void )
{
	uint32_t *px = ws->pixels;
	uint32_t pal[3] =
	{
		SDL_MapRGB(ws->format,16,16,16),
		SDL_MapRGB(ws->format,192,192,192),
		SDL_MapRGB(ws->format,16,255,16),
	};
	for ( int y=0; y<MAP_HEIGHT; y++ )
	for ( int x=0; x<MAP_WIDTH; x++ )
	{
		for ( int yy=0; yy<CH; yy++ )
		for ( int xx=0; xx<CW; xx++ )
			px[(y*CH+yy)*MAP_WIDTH*CH+(x*CW+xx)] = pal[map[x][y]];
	}
	SDL_Event e;
	while ( SDL_PollEvent(&e) )
	{
		if ( e.type == SDL_QUIT )
			active = 0;
	}
	if ( !active )
	{
		SDL_Quit();
		exit(0);
	}
	SDL_UpdateWindowSurface(w);
#ifdef GENDELAY
	SDL_Delay(GENDELAY);
#endif
}

// the basic cell editor function
// can also "uncarve" back walls
void carve( int x, int y, int col )
{
	map[x][y] = col;
	region[x][y] = col?current_region:0;	// un-carving resets region
	// debug
	// comment out if you don't want to see the generation frame by frame
	drawmap();
}

// build a whole room
void carve_room( room_t* r )
{
	current_region++;
	for ( int j=r->y1; j<r->y2; j++ ) for ( int i=r->x1; i<r->x2; i++ )
		carve(i,j,TILE_AIR);
	// add other stuff here if you want
}

// place rooms
void add_rooms()
{
	int rw, rh, rx, ry;
	room_t *rooms = malloc(0);
	int nrooms = 0;
	for ( int i=0; i<ROOM_TRIES; i++ )
	{
		rw = (Random(ROOM_MIN,ROOM_MAX)/2)*2+1;
		rh = (Random(ROOM_MIN,ROOM_MAX)/2)*2+1;
		rx = (Random(0,MAP_WIDTH-rw-1)/2)*2+1;
		ry = (Random(0,MAP_HEIGHT-rh-1)/2)*2+1;
		room_t room = {rx,ry,rx+rw,ry+rh};
		// overlap check
		int failed = 0;
		for ( int j=0; j<nrooms; j++ )
		{
			if ( !room_intersect(&room,&rooms[j]) ) continue;
			failed = 1;
			break;
		}
		if ( failed ) continue;
		// add it
		nrooms++;
		rooms = realloc(rooms,nrooms*sizeof(room_t));
		memcpy(rooms+nrooms-1,&room,sizeof(room_t));
		carve_room(&room);
	}
	// feel free to do something here like picking which room has the start
	// and which the exit
	free(rooms);
}

// check if the maze can advance in this direction
int can_carve( int sx, int sy, int dir )
{
	int x = sx+dirs[dir].x*3;
	int y = sy+dirs[dir].y*3;
	if ( (x < 0) || (x >= MAP_WIDTH) || (y < 0) || (y >= MAP_HEIGHT) )
		return 0;
	x = sx+dirs[dir].x*2;
	y = sy+dirs[dir].y*2;
	return (map[x][y] == TILE_WALL);
}

// make a maze
void grow_maze( int sx, int sy )
{
	int lastdir = -1;
	current_region++;
	carve(sx,sy,TILE_AIR);
	vec2_t *cells = malloc(sizeof(vec2_t));
	int ncells = 1;
	cells[0].x = sx;
	cells[0].y = sy;
	while ( ncells > 0 )
	{
		vec2_t *ccell = &cells[ncells-1];
		int carvedirs[4] = {0};
		int cancarve = 0;
		for ( int i=0; i<4; i++ )
		{
			if ( can_carve(ccell->x,ccell->y,i) )
			{
				carvedirs[i] = 1;
				cancarve = 1;
			}
		}
		if ( cancarve )
		{
			int dir;
			if ( (lastdir != -1) && carvedirs[lastdir]
				&& (FRandom(0,1) > WINDING_PCT) )
				dir = lastdir;
			else
			{
				// cheap-arse random picking
				int wdir[4];
				int ndirs = 0;
				int j = 0;
				for ( int i=0; i<4; i++ )
				{
					if ( !carvedirs[i] ) continue;
					wdir[j++] = i;
					ndirs++;
				}
				// shuffle
				for ( int i=0; i<ndirs; i++ )
				{
					int j = Random(i,ndirs-1);
					int tmp = wdir[i];
					wdir[i] = wdir[j];
					wdir[j] = tmp;
				}
				// pick
				dir = wdir[0];
			}
			int x = ccell->x+dirs[dir].x;
			int y = ccell->y+dirs[dir].y;
			carve(x,y,TILE_AIR);
			x += dirs[dir].x;
			y += dirs[dir].y;
			carve(x,y,TILE_AIR);
			ncells++;
			cells = realloc(cells,sizeof(vec2_t)*ncells);
			cells[ncells-1].x = x;
			cells[ncells-1].y = y;
			lastdir = dir;
		}
		else
		{
			ncells--;
			cells = realloc(cells,sizeof(vec2_t)*ncells);
			lastdir = -1;
		}
	}
	free(cells);
}

// this generates doors
// it's kind of convoluted
// but not as much as the original implementation lol
void connect_regions()
{
	int creg[MAP_WIDTH][MAP_HEIGHT][5] = {{{0}}};
	for ( int j=1; j<MAP_HEIGHT-1; j++ )
	for ( int i=1; i<MAP_WIDTH-1; i++ )
	{
		if ( map[i][j] ) continue;
		// check regions this wall is touching
		for ( int d=0; d<4; d++ )
		{
			int r = region[i+dirs[d].x][j+dirs[d].y];
			creg[i][j][d+1] = r;
			if ( r <= 0 ) continue;
			creg[i][j][0]++;
		}
	}
	// list all those walls that touch two regions in a straight line
	conn_t *conn = malloc(0);
	int nconn = 0;
	current_region++;
	for ( int j=0; j<MAP_HEIGHT; j++ )
	for ( int i=0; i<MAP_WIDTH; i++ )
	{
		if ( creg[i][j][0] != 2 ) continue;
		if ( !((creg[i][j][1] && creg[i][j][2])
			|| (creg[i][j][3] && creg[i][j][4])) ) continue;
		conn_t cconn;
		cconn.x = i;
		cconn.y = j;
		int k = 0, l = 1;
		while ( k < 2 )
		{
			if ( creg[i][j][l] )
			{
				cconn.r[k] = creg[i][j][l];
				k++;
			}
			l++;
		}
		// discard doors connecting between the same region
		// by random chance
		if ( (cconn.r[0] == cconn.r[1]) && (FRandom(0,1) > SELFDOOR_PCT) )
			continue;
		nconn++;
		conn = realloc(conn,nconn*sizeof(conn_t));
		memcpy(conn+nconn-1,&cconn,sizeof(conn_t));
	}
	// iterate through all regions and place doors
	// make sure there is at least ONE door between two regions
	// but allow a random chance for more
	for ( int i=1; i<current_region; i++ )
	{
		// locate all connections to this region
		int *connl = malloc(0);
		int nconnl = 0;
		for ( int j=0; j<nconn; j++ )
		{
			if ( (conn[j].r[0] != i) && (conn[j].r[1] != i) )
				continue;
			nconnl++;
			connl = realloc(connl,nconnl*sizeof(int));
			connl[nconnl-1] = j;
		}
		// pick a random index
		int rconn = Random(0,nconnl-1);
		// open additional doors, making sure they're not too close
		// to any other
		for ( int j=0; j<nconnl; j++ )
		{
			if ( j == rconn )
			{
				// this one's always guaranteed
				carve(conn[connl[j]].x,conn[connl[j]].y,TILE_DOOR);
				continue;
			}
			if ( FRandom(0,1) > EXTDOOR_PCT ) continue;
			// open an extra door
			carve(conn[connl[j]].x,conn[connl[j]].y,TILE_DOOR);
		}
		free(connl);
	}
	// clean up any overlapping doors
	// must be done on a second pass
	for ( int i=0; i<nconn; i++ )
	{
		int cleanme = 0;
		for ( int j=0; j<4; j++ )
		{
			if ( map[conn[i].x+dirs[j].x][conn[i].y+dirs[j].y] == TILE_DOOR )
				cleanme = 1;
		}
		if ( cleanme ) carve(conn[i].x,conn[i].y,TILE_WALL);
	}
	free(conn);
}

// get rid of every single cell that is surrounded by at least 3 walls
void clean_deadends()
{
	int found;
	do
	{
		found = 0;
		for ( int j=1; j<MAP_HEIGHT-1; j++ )
		for ( int i=1; i<MAP_WIDTH-1; i++ )
		{
			// needs to be air
			if ( map[i][j] != TILE_AIR ) continue;
			int nwalls = 0;
			for ( int k=0; k<4; k++ )
			{
				if ( map[i+dirs[k].x][j+dirs[k].y] == TILE_WALL )
					nwalls++;
			}
			if ( nwalls < 3 ) continue;
			// un-carve nearby doors
			// so as to not end up with doors to nowhere
			for ( int k=0; k<4; k++ )
			{
				if ( map[i+dirs[k].x][j+dirs[k].y] == TILE_DOOR )
					carve(i+dirs[k].x,j+dirs[k].y,TILE_WALL);
			}
			carve(i,j,TILE_WALL);
			found++;
		}
	} while ( found > 0 );
}

// get rid of groups of rooms that are "isolated" from the larger part of the
// dungeon
int spread_sregion( int x, int y )
{
	// non-recursive flood fill using the same queue system as the maze
	// generator
	int ssz = 1;
	int ncells = 1;
	vec2_t *cells = malloc(sizeof(vec2_t));
	sregion[x][y] = current_sregion;
	cells[0].x = x;
	cells[0].y = y;
	while ( ncells > 0 )
	{
		x = cells[ncells-1].x;
		y = cells[ncells-1].y;
		int dir = -1;
		for ( int i=0; i<4; i++ )
		{
			if ( (map[x+dirs[i].x][y+dirs[i].y] == TILE_WALL)
				|| sregion[x+dirs[i].x][y+dirs[i].y] )
				continue;
			dir = i;
			break;
		}
		if ( dir != -1 )
		{
			ncells++;
			cells = realloc(cells,sizeof(vec2_t)*ncells);
			sregion[x+dirs[dir].x][y+dirs[dir].y] = current_sregion;
			cells[ncells-1].x = x+dirs[dir].x;
			cells[ncells-1].y = y+dirs[dir].y;
			ssz++;
		}
		else
		{
			ncells--;
			cells = realloc(cells,sizeof(vec2_t)*ncells);
		}
	}
	free(cells);
	return ssz;
}

void clean_isolated()
{
	int *ssizes = malloc(0);
	for ( int j=1; j<MAP_HEIGHT-1; j++ )
	for ( int i=1; i<MAP_WIDTH-1; i++ )
	{
		if ( (map[i][j] == TILE_WALL) || sregion[i][j] ) continue;
		current_sregion++;
		ssizes = realloc(ssizes,sizeof(int)*current_sregion);
		int ssz = spread_sregion(i,j);
		ssizes[current_sregion-1] = ssz;
	}
	// find largest superregion
	int largest = 0;
	for ( int i=0; i<current_sregion; i++ )
		if ( ssizes[i] > ssizes[largest] ) largest = i;
	for ( int j=1; j<MAP_HEIGHT-1; j++ )
	for ( int i=1; i<MAP_WIDTH-1; i++ )
	{
		if ( (map[i][j] == TILE_WALL)
			|| (sregion[i][j] == largest+1) ) continue;
		// uncarve it
		carve(i,j,TILE_WALL);
		sregion[i][j] = 0;
		// note: if you kept track of rooms, you might also want to
		// make sure to erase any rooms caught by this uncarving
		// (just check if the position is inside the room's bounds)
	}
	free(ssizes);
}

// this is where the magic happens
void dungeon_make()
{
	add_rooms();
	for ( int j=1; j<MAP_HEIGHT-1; j+=2 )
	for ( int i=1; i<MAP_WIDTH-1; i+=2 )
	{
		if ( map[i][j] != TILE_WALL ) continue;
		grow_maze(i,j);
	}
	connect_regions();
	clean_deadends();
	clean_isolated();
}

// this is just here for displaying the stuff(tm)
int main( int argc, char **argv )
{
	unsigned seed = 0;
	if ( argc > 1 ) sscanf(argv[1],"%u",&seed);
	else seed = time(0);
	srand(seed);
	printf("seed: %u\n",seed);
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
	w = SDL_CreateWindow("Dungeon",SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,WIN_WIDTH,WIN_HEIGHT,
		SDL_WINDOW_SHOWN);
	ws = SDL_GetWindowSurface(w);
	dungeon_make();
	while ( active ) drawmap();
	return 0;
}
