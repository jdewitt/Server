#ifndef EVOLUTION_H_
#define EVOLUTION_H_

#include "../StructStrategy.h"

class EQStreamIdentifier;

namespace Evolution {

	//these are the only public member of this namespace.
	extern void Register(EQStreamIdentifier &into);
	extern void Reload();



	//you should not directly access anything below..
	//I just dont feel like making a seperate header for it.

	class Strategy : public StructStrategy {
	public:
		Strategy();

	protected:

		virtual std::string Describe() const;
		virtual const EQClientVersion ClientVersion() const;
		//magic macro to declare our opcodes
		#include "SSDeclare.h"
		#include "Evolution_ops.h"


	};

};



#endif /*TEMPLATE_H_*/