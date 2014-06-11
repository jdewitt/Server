#ifndef CLIENTVERSIONS_H
#define CLIENTVERSIONS_H

static const uint32 BIT_Mac = 1;
static const uint32 BIT_AllClients = 0xFFFFFFFF;

typedef enum {
	EQClientUnknown = 0,
	EQClientMac,
	EQClientEvolution,
} EQClientVersion;

#endif /* CLIENTVERSIONS_H */
