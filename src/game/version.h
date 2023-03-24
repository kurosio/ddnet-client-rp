/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#ifndef GAME_RELEASE_VERSION
#define GAME_RELEASE_VERSION "16.8"
#endif
#define GAME_VERSION "0.6.4, " GAME_RELEASE_VERSION
#define GAME_NETVERSION "0.6 626fce9a778df4d4"
#define CLIENT_VERSIONNR 16080
extern const char *GIT_SHORTREV_HASH;
#define GAME_NAME "DDNet"

// ~~ RELEASE PROTOCOL(CLIENT/SERVER SIDE) VERSION
// in case of a change it will force to update the client when entering the server to the value that is specified here
#define PROTOCOL_VERSION_MRPG 2000

#endif
