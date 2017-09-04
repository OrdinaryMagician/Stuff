#include <fmodex/fmod.h>
#include <stdio.h>

int main( int argc, char **argv )
{
	FMOD_SYSTEM *sys;
	FMOD_SOUND *bank;
	FMOD_SOUND *snd;
	FMOD_CHANNEL *ch = 0;
	FMOD_System_Create(&sys);
	FMOD_System_Init(sys,32,FMOD_INIT_NORMAL,0);
	FMOD_System_CreateSound(sys,argv[1],FMOD_SOFTWARE,0,&bank);
	int iss = 0, ss = 0;
	sscanf(argv[2],"%d",&iss);
	FMOD_Sound_GetNumSubSounds(bank,&ss);
	printf("%s : %d\n",argv[1],ss);
	int i;
	char name[256];
	for ( i=iss; i<ss; i++ )
	{
		FMOD_Sound_GetSubSound(bank,i,&snd);
		FMOD_Sound_GetName(snd,name,255);
		printf("Playing: %s\n",name);
		FMOD_System_PlaySound(sys,FMOD_CHANNEL_FREE,snd,0,&ch);
		int active = 1;
		while ( active )
		{
			FMOD_System_Update(sys);
			FMOD_Channel_IsPlaying(ch,&active);
		}
		FMOD_Sound_Release(snd);
	}
	FMOD_Sound_Release(bank);
	FMOD_System_Close(sys);
	FMOD_System_Release(sys);
	return 0;
}
