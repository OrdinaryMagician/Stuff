Random single-file programs I've written in my spare time for small tasks.

* **bleep:** I got bored and decided to write a pc speaker music program.
* **ckmextract:** Extracts ESP and BSA from Skyrim steam workshop mod archives.
* **cropng:** Crops PNGs and applies grAb offsets (while respecting existing ones).
* **cube2enviro:** A simple GL 4.4 program. Loads a cubemap and draws a flattened hemisphere environment map that can be used in Unreal.
* **ddsinfo:** Shows contents of a DDS header.
* **dood:** Reads an ENDOOM lump and mirrors every word "down the middle".
* **dtexdupes:** Small tool I've used once or twice to clean up my doom projects of duplicate textures.
* **fmod\_playbank (formerly fuck\_fmod):** Tool for playback of .fsb files.
* **fuzz:** A fancy blocky noise filter using proto-AliceGL designs.
* **glfuzz:** OpenGL version of the filter.
* **iwad64ex:** A small, failed experiment for ripping the Hexen 64 IWAD.
* **lutconv:** A program for converting various "3D" LUT textures to actual 3D LUTs in DDS volume maps. Successor to mkvolume. Used for MariENB. Plus two additional tools for "deconverting" volume maps, and one for smoothing them out to reduce potential banding.
* **mazestuff:** A dungeon generator for roguelikes. This was made as part of a commission for a friend, hence the very detailed comments.
* **memrd/memsk/memwr:** Quick 'n dirty tools for memory manipulation on running programs.
* **mkfont:** A tool I use to convert UE fonts exported with UTPT into fonts for GZDoom. Requires ImageMagick to be installed.
* **mkgauss:** Make an array of gaussian blur kernel values with passed radius and sigma. Used for shader development.
* **mksoundwad:** Program used during the early days of Tim Allen Doom. Deprecated as notsanae now also replaces sounds through an OpenAL hook.
* **mkssao:** Make an array of SSAO samples. Also for shader development.
* **mkvolume:** Old program for making LUT volume maps.
* **mkwall:** A program I use on a daily basis to set my wallpaper on every Linux machine.
* **osnorm:** Experiment for generating object-space normals from an .obj model.
* **pcxpalex:** Extracts the palette from PCX images.
* **pframes:** Short utility for automating long FrameIndex lists for MODELDEF.
* **schange:** Program used along with mkwall to update the wallpaper on screen geometry changes.
* **skse_cosave:** Experiment for dumping information in SKSE co-saves.
* **soapstone:** Random soapstone messages from all 3 dark souls games. Messages can be generated in bulk.
* **totty:** Sends text from stdin to tty1. Used to send certain commands when remoting into a Raspberry Pi.
* **u95/u083/u086extract:** Programs for extracting data from Unreal alpha packages. This and other Unreal tools might be shifted to another repo.
* **udmfvis:** dmvis clone in C for UDMF maps. No external dependencies.
* **ufontext:** companion to mkfont, for extracting UE fonts. Currently does not yet extract the textures themselves.
* **umxunpack:** Extractor for music in UE archives, with support for Unreal 227's UMX files containing vorbis audio.
* **unrundeleter:** WIP program to unset the bDeleteMe flag on stuff in UE1 maps. Yes, some mappers are so hellbent on preventing modification that they delete all brushes after baking the geometry.
* **usndextract:** Extracts sounds from UE archives.
* **utxextract:** Extracts textures from UE archives.
* **vc2sdl:** Passes the contents of the VC4 framebuffer to a SDL window. Was used for video playback experiments on a Raspberry Pi with a SPI LCD.
* **wavrip:** Cheap WAV file extractor that naively searchs for RIFF headers.
* **wavsmpl:** Extracts loop information from WAV files that have it, because only certain proprietary audio editors support this directly.
* **withhands:** Talk like W.D. Gaster.
* **zimagekver:** Quick program to extract version info from an ARM Linux kernel image.

All programs and code here are under the MIT license.
