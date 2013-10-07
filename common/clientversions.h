#ifndef CLIENTVERSIONS_H
#define CLIENTVERSIONS_H

static const uint32 BIT_Mac = 1;
static const uint32 BIT_Client62 = 2;
static const uint32 BIT_Titanium = 4;
static const uint32 BIT_SoF = 8;
static const uint32 BIT_SoD = 16;
static const uint32 BIT_Underfoot = 32;
static const uint32 BIT_RoF = 64;
static const uint32 BIT_TitaniumAndEarlier = 7;
static const uint32 BIT_SoFAndLater = 0xFFFFFFF8;
static const uint32 BIT_SoDAndLater = 0xFFFFFFF0;
static const uint32 BIT_UnderfootAndLater = 0xFFFFFFE0;
static const uint32 BIT_RoFAndLater = 0xFFFFFFC0;
static const uint32 BIT_AllClients = 0xFFFFFFFF;

typedef enum {
	EQClientUnknown = 0,
	EQClientMac,
	EQClient62,
	EQClientTitanium,
	EQClientSoF,
	EQClientSoD,
	EQClientUnderfoot,
	EQClientRoF
} EQClientVersion;

#endif /* CLIENTVERSIONS_H */
