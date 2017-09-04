/*
	schange.c : run command on screen change events from RandR.
	(C)2015 Marisa Kirisame, UnSX Team. For personal use mostly.
	Released under the GNU GPLv3 (or any later version).
	Please refer to <https://gnu.org/licenses/gpl.txt> for license terms.
*/
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/randr.h>

xcb_connection_t *c;
xcb_screen_t *s;

char **cmdline;

void handle_event( xcb_generic_event_t *e )
{
	const xcb_query_extension_reply_t *r;
	r = xcb_get_extension_data(c,&xcb_randr_id);
	if ( (e->response_type&0x7f) != (r->first_event+XCB_RANDR_NOTIFY) )
		return;
	/* fork and exec command */
	int pid = fork();
	if ( !pid ) execvp(cmdline[0],cmdline);
	else waitpid(pid,0,0);
}

int main( int argc, char **argv )
{
	if ( argc < 2 ) return 1;
	cmdline = argv+1;
	c = xcb_connect(0,0);
	s = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	xcb_randr_select_input(c,s->root,XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE);
	xcb_flush(c);
	xcb_generic_event_t *e;
	while ( (e=xcb_wait_for_event(c)) )
	{
		handle_event(e);
		free(e);
	}
	xcb_disconnect(c);
	return 0;
}
