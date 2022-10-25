#ifndef DOOMVER_H
#define DOOMVER_H

#define DOOM_MAJOR_VERSION	1
#define DOOM_SUBVERSION		16
#define DOOM_VERSION_TEXT "1.16"

#define DOOM_VERSION ((DOOM_MAJOR_VERSION*100)+DOOM_SUBVERSION)


#define C_API_VERSION DOOM_VERSION
#define D_API_VERSION DOOM_VERSION

#define API_INFO_VERSION (2<<16)

#define DOOM_NET_VERSION DOOM_VERSION

#define DOOM_DEMO_VERSION DOOM_VERSION//will attempt to read _any_ game version

#define DOOM_SAVE_VERSION 109 //will attempt to load any version

#endif
