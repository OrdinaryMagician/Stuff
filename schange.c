/*
	schange.c : run command on crtc and output change events from RandR.
	(C)2015-2017 Marisa Kirisame, UnSX Team. For personal use mostly.
	Released under the MIT license.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/randr.h>

const xcb_query_extension_reply_t *r;
xcb_connection_t *c;
xcb_screen_t *s;

char *handler = 0;

void handle_event( xcb_generic_event_t *e )
{
	if ( (e->response_type&0x7f) != (r->first_event+XCB_RANDR_NOTIFY) )
		return;
	xcb_randr_notify_data_t d = ((xcb_randr_notify_event_t*)e)->u;
	char* hargs[8];
	hargs[0] = handler;
	char geom[64];
	char oname[32];
	int rot, len;
	char *nm;
	switch ( ((xcb_randr_notify_event_t*)e)->subCode )
	{
	case 0:
		rot = d.cc.rotation&0xf;
		hargs[1] = "cc";
		hargs[2] = (rot==1)?"0":(rot==2)?"90":(rot==4)?"180":"270";
		hargs[3] = (d.cc.rotation&0x10)?(d.cc.rotation&0x20)?"xy":"x"
			:(d.cc.rotation&0x20)?"y":"none";
		snprintf(geom,64,"%ux%u%+d%+d",d.cc.width,d.cc.height,d.cc.x,
			d.cc.y);
		hargs[4] = geom;
		hargs[5] = 0;
		printf("schange: crtc change\n rotation: %s\n"
			" reflection: %s\n geometry: %s\n",hargs[2],hargs[3],
			hargs[4]);
		break;
	case 1:
		hargs[1] = "oc";
		xcb_randr_get_output_info_cookie_t ck;
		ck = xcb_randr_get_output_info_unchecked(c,d.oc.output,
			d.oc.config_timestamp);
		xcb_randr_get_output_info_reply_t *inf;
		inf = xcb_randr_get_output_info_reply(c,ck,0);
		if ( !inf ) hargs[2] = "UNKNOWN";
		else
		{
			len = xcb_randr_get_output_info_name_length(inf);
			nm = (char*)xcb_randr_get_output_info_name(inf);
			strncpy(oname,nm,len);
			oname[len] = 0;
			hargs[2] = oname;
		}
		hargs[3] = (d.oc.connection==0)?"connected"
			:(d.oc.connection==1)?"disconnected":"unknown";
		hargs[4] = 0;
		printf("schange: output change\n name: %s\n"
			" connection: %s\n",hargs[2],hargs[3]);
		break;
	default:
		/* ??? */
		printf("schange: unhandled subcode %u\n",
			((xcb_randr_notify_event_t*)e)->subCode);
		hargs[1] = "unknown";
		hargs[2] = 0;
		break;
	}
	/* skip everything if there's no handler registered */
	if ( !handler ) return;
	int pid = fork();
	if ( !pid ) execvp(hargs[0],hargs);
	else waitpid(pid,0,0);
}

int main( int argc, char **argv )
{
	if ( argc > 1 ) handler = argv[1];
	else printf("schange: warning, no handler assigned.\n");
	c = xcb_connect(0,0);
	r = xcb_get_extension_data(c,&xcb_randr_id);
	if ( !r->present )
	{
		printf("schange: no RandR extension present\n");
		xcb_disconnect(c);
		return 1;
	}
	s = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	xcb_randr_select_input(c,s->root,XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE
		|XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);
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
