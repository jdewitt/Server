
#include "../debug.h"
#include "patches.h"

#include "Mac.h"
#include "Evolution.h"

void RegisterAllPatches(EQStreamIdentifier &into) {
	Mac::Register(into);
	Evolution::Register(into);

}

void ReloadAllPatches() {
	Mac::Reload();
	Evolution::Reload();
}

