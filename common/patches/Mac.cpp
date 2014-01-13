#include "../debug.h"
#include "Mac.h"
#include "../opcodemgr.h"
#include "../logsys.h"
#include "../EQStreamIdent.h"
#include "../crc32.h"

#include "../eq_packet_structs.h"
#include "../packet_dump_file.h"
#include "../MiscFunctions.h"
#include "../packet_functions.h"
#include "../StringUtil.h"
#include "../Item.h"
#include "Mac_structs.h"
#include "../rulesys.h"

namespace Mac {

static const char *name = "Mac";
static OpcodeManager *opcodes = NULL;
static Strategy struct_strategy;

//char *WeaselTheJuice(const ItemInst *inst, int16 slot_id, int type = 0);

void Register(EQStreamIdentifier &into) {
	//create our opcode manager if we havent already
	if(opcodes == NULL) {
		std::string opfile = "patch_";
		opfile += name;
		opfile += ".conf";
		//load up the opcode manager.
		//TODO: figure out how to support shared memory with multiple patches...
		opcodes = new RegularOpcodeManager();
		if(!opcodes->LoadOpcodes(opfile.c_str())) {
			_log(NET__OPCODES, "Error loading opcodes file %s. Not registering patch %s.", opfile.c_str(), name);
			return;
		}
	}

	//ok, now we have what we need to register.

	EQStream::Signature signature;
	std::string pname;

	pname = std::string(name) + "_world";
	//register our world signature.
	signature.first_length = sizeof(structs::LoginInfo_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_SendLoginInfo);
	into.RegisterOldPatch(signature, pname.c_str(), &opcodes, &struct_strategy);

	pname = std::string(name) + "_zone";
	//register our zone signature.
	signature.first_length = sizeof(structs::SetDataRate_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_DataRate);
	into.RegisterOldPatch(signature, pname.c_str(), &opcodes, &struct_strategy);
		
	_log(NET__IDENTIFY, "Registered patch %s", name);
}

void Reload() {

	//we have a big problem to solve here when we switch back to shared memory
	//opcode managers because we need to change the manager pointer, which means
	//we need to go to every stream and replace it's manager.

	if(opcodes != NULL) {
		//TODO: get this file name from the config file
		std::string opfile = "patch_";
		opfile += name;
		opfile += ".conf";
		if(!opcodes->ReloadOpcodes(opfile.c_str())) {
			_log(NET__OPCODES, "Error reloading opcodes file %s for patch %s.", opfile.c_str(), name);
			return;
		}
		_log(NET__OPCODES, "Reloaded opcodes for patch %s", name);
	}
}



Strategy::Strategy()
: StructStrategy()
{
	//all opcodes default to passthrough.
	#include "SSRegister.h"
	#include "Mac_ops.h"
}

std::string Strategy::Describe() const {
	std::string r;
	r += "Patch ";
	r += name;
	return(r);
}


#include "SSDefine.h"



const EQClientVersion Strategy::ClientVersion() const
{
	return EQClientMac;
}


DECODE(OP_SendLoginInfo) {
	DECODE_LENGTH_EXACT(structs::LoginInfo_Struct);
	SETUP_DIRECT_DECODE(LoginInfo_Struct, structs::LoginInfo_Struct);
	memcpy(emu->login_info, eq->AccountName, 64);
	IN(zoning);
	FINISH_DIRECT_DECODE();
}

DECODE(OP_EnterWorld) {
	SETUP_DIRECT_DECODE(EnterWorld_Struct, structs::EnterWorld_Struct);
	strn0cpy(emu->name, eq->charname, 64);
	emu->return_home = 0;
	emu->tutorial = 0;
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ZoneServerInfo) {
	SETUP_DIRECT_ENCODE(ZoneServerInfo_Struct, structs::ZoneServerInfo_Struct);
	strcpy(eq->ip, emu->ip);
	eq->port = ntohs(emu->port);

	FINISH_ENCODE();
}

ENCODE(OP_ZoneEntry) { ENCODE_FORWARD(OP_NewSpawn); }

ENCODE(OP_PlayerProfile) {
	SETUP_DIRECT_ENCODE(PlayerProfile_Struct, structs::PlayerProfile_Struct);

	eq->available_slots=0xffff;

	int r = 0;
	//	OUT(checksum);
	OUT(gender);
	OUT(race);
	OUT(class_);
	OUT(level);
	eq->bind_point_zone = emu->binds[0].zoneId;
	eq->bind_location[0].x = emu->binds[0].x;
	eq->bind_location[0].y = emu->binds[0].y;
	eq->bind_location[0].z = emu->binds[0].z;
	//eq->bind_heading[0] = emu->binds[0].heading;


	OUT(deity);
	OUT(intoxication);
	//OUT_array(spellSlotRefresh, 8);
	//OUT(abilitySlotRefresh);
//	OUT(unknown0166[4]);
	OUT(haircolor);
	OUT(beardcolor);
	OUT(eyecolor1);
	OUT(eyecolor2);
	OUT(hairstyle);
	eq->beard_t = emu->beard;
	eq->trainingpoints = emu->points;
	OUT(mana);
	OUT(cur_hp);
	OUT(STR);
	OUT(STA);
	OUT(CHA);
	OUT(DEX);
	OUT(INT);
	OUT(AGI);
	OUT(WIS);
	OUT(face);
	OUT_array(spell_book, 256);
	OUT_array(mem_spells, 8);
	OUT(platinum);
	OUT(gold);
	OUT(silver);
	OUT(copper);
	OUT(platinum_cursor);
	OUT(gold_cursor);
	OUT(silver_cursor);
	OUT(copper_cursor);
//	OUT_array(skills, 75);
	OUT_array(skills, structs::MAX_PP_SKILL);  // 1:1 direct copy (100 dword)
	//OUT(toxicity);
	OUT(thirst_level);
	OUT(hunger_level);
	for(r = 0; r < 15; r++) {
		eq->buffs[r].visable = (emu->buffs[r].spellid == 0xFFFFFFFF || emu->buffs[r].spellid == 0) ? 0 : 2;
		OUT(buffs[r].level);
		OUT(buffs[r].bard_modifier);
		OUT(buffs[r].spellid);
		OUT(buffs[r].duration);
	}
//	OUT_array(recastTimers, structs::MAX_RECAST_TYPES);
//	OUT(endurance);
//	OUT(aapoints_spent);
	OUT(aapoints);
	OUT_str(name);
	strcpy(eq->Surname, emu->last_name);
	OUT(guild_id);
	OUT(birthday);
	OUT(lastlogin);
	OUT(timePlayedMin);
	OUT(pvp);
	OUT(anon);
	OUT(gm);
	OUT(guildrank);
	OUT(exp);
	OUT_array(languages, 26);
	OUT(x);
	OUT(y);
	eq->z=emu->z*10;
	OUT(heading);
	OUT(platinum_bank);
	OUT(gold_bank);
	OUT(silver_bank);
	OUT(copper_bank);
	OUT(level2);
//	OUT(platinum_shared);
//OUT(expansions);
	OUT(autosplit);
	eq->current_zone = emu->zone_id;
	for(r = 0; r < structs::MAX_PP_AA_ARRAY; r++) {
	//	int8 macaaid = 0;
	//	if(emu->aa_array[r].AA > 0)
	//		macaaid = zone->EmuToEQMacAA(emu->aa_array[r].AA);
	//	eq->aa_array[r].AA = macaaid;
		OUT(aa_array[r].AA);
		OUT(aa_array[r].value);
	}
	//OUT(zoneInstance);
	for(r = 0; r < 6; r++) {
		OUT_str(groupMembers[r]);
	}
//	OUT_str(groupLeader);	//this is prolly right after groupMembers, but I dont feel like checking.
//	OUT(leadAAActive);
//	OUT(showhelm);
	_log(NET__STRUCTS, "Player Profile Packet is %i bytes uncompressed", sizeof(structs::PlayerProfile_Struct));

	char* packet_dump = "pp_dump.txt";
	FileDumpPacketHex(packet_dump, __packet);

	CRC32::SetEQChecksum(__packet->pBuffer, sizeof(structs::PlayerProfile_Struct)-4);
	EQApplicationPacket* outapp = new EQApplicationPacket();
	outapp->SetOpcode(OP_PlayerProfile);
	outapp->pBuffer = new uchar[10000];
	outapp->size = DeflatePacket((unsigned char*)__packet->pBuffer, sizeof(structs::PlayerProfile_Struct), outapp->pBuffer, 10000);
	EncryptProfilePacket(outapp->pBuffer, outapp->size);
	_log(NET__STRUCTS, "Player Profile Packet is %i bytes compressed", outapp->size);
	dest->FastQueuePacket(&outapp);
	delete[] __emu_buffer;
	delete __packet;
}

DECODE(OP_CharacterCreate) {
	DECODE_LENGTH_EXACT(structs::CharCreate_Struct);
	SETUP_DIRECT_DECODE(CharCreate_Struct, structs::CharCreate_Struct);
	IN(class_);
	IN(beardcolor);
	IN(beard);
	IN(haircolor);
	IN(gender);
	IN(race);
	IN(start_zone);
	IN(hairstyle);
	IN(deity);
	IN(STR);
	IN(STA);
	IN(AGI);
	IN(DEX);
	IN(WIS);
	IN(INT);
	IN(CHA);
	IN(face);
	IN(eyecolor1);
	IN(eyecolor2);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ApproveWorld) {
	SETUP_DIRECT_ENCODE(ApproveWorld_Struct, structs::ApproveWorld_Struct);
	eq->response = 0;
	FINISH_ENCODE();	
}

ENCODE(OP_EnterWorld) {
	SETUP_DIRECT_ENCODE(ApproveWorld_Struct, structs::ApproveWorld_Struct);
	eq->response = 0;
	FINISH_ENCODE();	
}


ENCODE(OP_ExpansionInfo) {
	SETUP_DIRECT_ENCODE(ExpansionInfo_Struct, structs::ExpansionInfo_Struct);
	eq->flag = 15;
	FINISH_ENCODE();	
}


ENCODE(OP_SendCharInfo) {
	int r;
	ENCODE_LENGTH_EXACT(CharacterSelect_Struct);
	SETUP_DIRECT_ENCODE(CharacterSelect_Struct, structs::CharacterSelect_Struct);
	for(r = 0; r < 10; r++) {
		OUT(zone[r]);
		OUT(eyecolor1[r]);
		OUT(eyecolor2[r]);
		OUT(hairstyle[r]);
		OUT(primary[r]);
		if(emu->race[r] > 300)
			eq->race[r] = 1;
		else
			eq->race[r] = emu->race[r];
		OUT(class_[r]);
		OUT_str(name[r]);
		OUT(gender[r]);
		OUT(level[r]);
		OUT(secondary[r]);
		OUT(face[r]);
		OUT(beard[r]);
		int k;
		for(k = 0; k < 9; k++) {
			OUT(equip[r][k]);
			OUT(cs_colors[r][k].color);
		}
		OUT(haircolor[r]);
		OUT(deity[r]);
		OUT(beardcolor[r]);
	}
	FINISH_ENCODE();	
}

ENCODE(OP_GuildsList) {
	SETUP_DIRECT_ENCODE(GuildsList_Struct, structs::GuildsList_Struct);
	OUT_array(head, 4);

	int totalcount = (__packet->size - 64) / sizeof(GuildsListEntry_Struct);
	int r = 0;
	for(r = 0; r < totalcount; r++)
	{
		if(emu->Guilds[r].name[0] != '\0')
		{
			strn0cpy(eq->Guilds[r].name, emu->Guilds[r].name, 32);
			eq->Guilds[r].exists = 1;
			eq->Guilds[r].guildID = r;
		}
		else
			eq->Guilds[r].guildID = 0xFFFFFFFF;
	}

	FINISH_ENCODE();	
}



ENCODE(OP_NewZone) {
	SETUP_DIRECT_ENCODE(NewZone_Struct, structs::NewZone_Struct);
	OUT_str(char_name);
	OUT_str(zone_short_name);
	OUT_str(zone_long_name);
	OUT(ztype);
	OUT_array(fog_red, 4);
	OUT_array(fog_green, 4);
	OUT_array(fog_blue, 4);
	OUT_array(fog_minclip, 4);
	OUT_array(fog_maxclip, 4);
	OUT(gravity);
	OUT(time_type);
	OUT(sky);
	OUT(zone_exp_multiplier);
	OUT(safe_y);
	OUT(safe_x);
	OUT(safe_z);
	OUT(max_z);
	eq->underworld=emu->underworld;
	OUT(minclip);
	OUT(maxclip);
	FINISH_ENCODE();	
}

ENCODE(OP_ChannelMessage) {
	EQApplicationPacket *__packet = *p; 
	*p = nullptr; 
	unsigned char *__emu_buffer = __packet->pBuffer; 
	ChannelMessage_Struct *emu = (ChannelMessage_Struct *) __emu_buffer; 
	uint32 __i = 0; 
	__i++; /* to shut up compiler */
	
	int msglen = __packet->size - sizeof(ChannelMessage_Struct);
	int len = sizeof(structs::ChannelMessage_Struct) + msglen + 4;
	__packet->pBuffer = new unsigned char[len]; 
	__packet->size = len; 
	memset(__packet->pBuffer, 0, len); 
	structs::ChannelMessage_Struct *eq = (structs::ChannelMessage_Struct *) __packet->pBuffer; 
	strncpy(eq->targetname, emu->targetname, 64);
	strncpy(eq->sender, emu->sender, 64);
	eq->language = emu->language;
	eq->chan_num = emu->chan_num;
	eq->skill_in_language = emu->skill_in_language;
	strcpy(eq->message, emu->message);
	FINISH_ENCODE();
}


ENCODE(OP_SpecialMesg) {
	EQApplicationPacket *__packet = *p; 
	*p = nullptr; 
	unsigned char *__emu_buffer = __packet->pBuffer; 
	SpecialMesg_Struct *emu = (SpecialMesg_Struct *) __emu_buffer; 
	uint32 __i = 0; 
	__i++; /* to shut up compiler */
	
	int msglen = __packet->size - sizeof(structs::SpecialMesg_Struct);
	int len = sizeof(structs::SpecialMesg_Struct) + msglen + 1;
	__packet->pBuffer = new unsigned char[len]; 
	__packet->size = len; 
	memset(__packet->pBuffer, 0, len); 
	structs::SpecialMesg_Struct *eq = (structs::SpecialMesg_Struct *) __packet->pBuffer; 
	eq->msg_type = emu->msg_type;
	strcpy(eq->message, emu->message);
	FINISH_ENCODE();
}



DECODE(OP_ChannelMessage)
{
	unsigned char *__eq_buffer = __packet->pBuffer;
	structs::ChannelMessage_Struct *eq = (structs::ChannelMessage_Struct *) __eq_buffer;
	int msglen = __packet->size - sizeof(structs::ChannelMessage_Struct) - 4;
	int len = msglen + sizeof(ChannelMessage_Struct);
	__packet->size = len; 
	__packet->pBuffer = new unsigned char[len];
	MEMSET_IN(ChannelMessage_Struct);
	ChannelMessage_Struct *emu = (ChannelMessage_Struct *) __packet->pBuffer;
	strncpy(emu->targetname, eq->targetname, 64);
	strncpy(emu->sender, eq->targetname, 64);
	emu->language = eq->language;
	emu->chan_num = eq->chan_num;
	emu->skill_in_language = eq->skill_in_language;
	strcpy(emu->message, eq->message);
}

ENCODE(OP_MobUpdate)
{

	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;
	PlayerPositionUpdateServer_Struct *emu = (PlayerPositionUpdateServer_Struct *) __emu_buffer;

	EQApplicationPacket* app = new EQApplicationPacket(OP_MobUpdate,sizeof(structs::SpawnPositionUpdates_Struct) + sizeof(structs::SpawnPositionUpdate_Struct));
	structs::SpawnPositionUpdates_Struct* spu = (structs::SpawnPositionUpdates_Struct*)app->pBuffer;
	
	spu->num_updates = 1;
	float anim_type = (float)emu->animation / 37.0f;
	spu->spawn_update[0].anim_type = (uint8)emu->animation;
	spu->spawn_update[0].delta_heading = (uint8)emu->delta_heading;
	spu->spawn_update[0].delta_x = (uint32)emu->delta_x;
	spu->spawn_update[0].delta_y = (uint32)emu->delta_y;
	spu->spawn_update[0].delta_z = (uint32)emu->delta_z;
	spu->spawn_update[0].spawn_id = emu->spawn_id;
	spu->spawn_update[0].x_pos = (int16)emu->y_pos;
	spu->spawn_update[0].y_pos = (int16)emu->x_pos;
	spu->spawn_update[0].z_pos = (int16)emu->z_pos*10;
	spu->spawn_update[0].heading = (int8)emu->heading;
	spu->spawn_update[0].anim_type = anim_type * 7;
	dest->FastQueuePacket(&app);

	delete[] __emu_buffer;
}

ENCODE(OP_ClientUpdate)
{
	SETUP_DIRECT_ENCODE(PlayerPositionUpdateServer_Struct, structs::SpawnPositionUpdate_Struct);
	OUT(spawn_id);
	eq->x_pos = (int16)emu->y_pos;
	OUT(delta_x);
	OUT(delta_y);
	eq->z_pos=emu->z_pos*10;
	eq->delta_heading = (uint8)emu->delta_heading;
	eq->y_pos = (int16)emu->x_pos;
	OUT(delta_z);
	eq->anim_type = (uint8)emu->animation;
	eq->heading = (uint8)emu->heading;
	FINISH_ENCODE();
}

DECODE(OP_ClientUpdate)
{
	SETUP_DIRECT_DECODE(PlayerPositionUpdateClient_Struct, structs::SpawnPositionUpdate_Struct);
	IN(spawn_id);
//	IN(sequence);
	emu->x_pos = (int16)eq->y_pos;
	emu->y_pos = (int16)eq->x_pos;
	emu->z_pos = (int16)eq->z_pos/10;
	emu->heading = (uint8)eq->heading;
	IN(delta_x);
	IN(delta_y);
	IN(delta_z);
	emu->delta_heading = (uint8)eq->delta_heading;
	emu->animation = eq->anim_type;
	FINISH_DIRECT_DECODE();
}

DECODE(OP_TargetMouse)
{
	SETUP_DIRECT_DECODE(ClientTarget_Struct, structs::ClientTarget_Struct);
	IN(new_target);
	FINISH_DIRECT_DECODE();
}

DECODE(OP_TargetCommand)
{
	SETUP_DIRECT_DECODE(ClientTarget_Struct, structs::ClientTarget_Struct);
	IN(new_target);
	FINISH_DIRECT_DECODE();
}

DECODE(OP_SetServerFilter)
{
	DECODE_LENGTH_EXACT(structs::SetServerFilter_Struct);
	SETUP_DIRECT_DECODE(SetServerFilter_Struct, structs::SetServerFilter_Struct);
	emu->filters[0]=eq->filters[5]; //GuildChat
	emu->filters[1]=eq->filters[6]; //Socials
	emu->filters[2]=eq->filters[7]; //GroupChat
	emu->filters[3]=eq->filters[8]; //Shouts
	emu->filters[4]=eq->filters[9]; //Auctions
	emu->filters[5]=eq->filters[10];//OOC
	emu->filters[6]=1;				//BadWords
	emu->filters[7]=eq->filters[2]; //PC Spells 0 is on
	emu->filters[8]=0;				//NPC Spells Client has it but it doesn't work. 0 is on.
	emu->filters[9]=eq->filters[3]; //Bard Songs 0 is on
	emu->filters[10]=eq->filters[15]; //Spell Crits 0 is on
	int critm = eq->filters[16];
	if(critm > 0){critm = critm-1;}
	emu->filters[11]=critm;			//Melee Crits 0 is on EQMac has 3 options, Emu only 2.
	emu->filters[12]=eq->filters[0]; //Spell Damage 0 is on
	emu->filters[13]=eq->filters[11]; //My Misses
	emu->filters[14]=eq->filters[12]; //Others Misses
	emu->filters[15]=eq->filters[13]; //Others Hit
	emu->filters[16]=eq->filters[14]; //Missed Me
	emu->filters[17] = 0;			  //Damage Shields
	emu->filters[18] = 0;			  //DOT
	emu->filters[19] = 0;			  //Pet Hits
	emu->filters[20] = 0;			  //Pet Misses
	emu->filters[21] = 0;			  //Focus Effects
	emu->filters[22] = 0;			  //Pet Spells
	emu->filters[23] = 0;			  //HoT	
	emu->filters[24] = 0;			  //Unknowns
	emu->filters[25] = 0;			
	emu->filters[26] = 0;
	emu->filters[27] = 0;
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ZoneSpawns){

	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;
	Spawn_Struct *emu = (Spawn_Struct *) __emu_buffer;

	//determine and verify length
	int entrycount = in->size / sizeof(Spawn_Struct);
	if(entrycount == 0 || (in->size % sizeof(Spawn_Struct)) != 0) {
		_log(NET__STRUCTS, "Wrong size on outbound %s: Got %d, expected multiple of %d", opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(Spawn_Struct));
		delete in;
		return;
	}
	EQApplicationPacket* out = new EQApplicationPacket();
	out->SetOpcode(OP_ZoneSpawns);
	//make the EQ struct.
	out->size = sizeof(structs::Spawn_Struct)*entrycount;
	out->pBuffer = new unsigned char[out->size];
	structs::Spawn_Struct *eq = (structs::Spawn_Struct *) out->pBuffer;

	//zero out the packet. We could avoid this memset by setting all fields (including unknowns)
	//in the loop.
	memset(out->pBuffer, 0, out->size);
	_log(NET__STRUCTS, "Total size of bulkspawns packet STRUCT: %d", sizeof(structs::Spawn_Struct));

	//do the transform...
	int r;
	int k;
	for(r = 0; r < entrycount; r++, eq++, emu++) {
//		eq->unknown0000 = emu->unknown0000;
		eq->GM = emu->gm;
//		eq->unknown0003 = emu->unknown0003;
		eq->title = emu->aaitle;
//		eq->unknown0004 = emu->unknown0004;
		eq->anon = emu->anon;
		//eq->face = emu->face;
		memcpy(eq->name, emu->name, 47);
		eq->deity = emu->deity;
//		eq->unknown0073 = emu->unknown0073;
		eq->size = emu->size;
//		eq->unknown0079 = emu->unknown0079;
		eq->NPC = emu->NPC;
		eq->invis = emu->invis;
//		eq->haircolor = emu->haircolor;
		eq->cur_hp = emu->curHp;
		//eq->max_hp = emu->max_hp;
		//eq->findable = emu->findable;
//		eq->unknown0089[5] = emu->unknown0089[5];
		eq->deltaHeading = emu->deltaHeading;
		eq->x_pos = (int16)emu->x;
//		eq->padding0054 = emu->padding0054;
		eq->y_pos = (int16)emu->y;
		eq->animation = emu->animation;
//		eq->padding0058 = emu->padding0058;
		eq->z_pos = (int16)emu->z*10;
		eq->deltaY = 0;
		eq->deltaX = 0;
		eq->heading = (uint8)emu->heading;
//		eq->padding0066 = emu->padding0066;
		eq->deltaZ = 0;
//		eq->padding0070 = emu->padding0070;
//		eq->eyecolor1 = emu->eyecolor1;
//		eq->unknown0115[24] = emu->unknown0115[24];
		//eq->showhelm = emu->showhelm;
//		eq->unknown0140[4] = emu->unknown0140[4];
		//eq->is_npc = emu->is_npc;
//		eq->hairstyle = emu->hairstyle;

		//if(emu->gender == 1){
		//	eq->hairstyle = eq->hairstyle == 0xFF ? 0 : eq->hairstyle;
		//}
		eq->anim_type = 0x64;
//		eq->beardcolor = emu->beardcolor;
//		eq->unknown0147[4] = emu->unknown0147[4];
		eq->level = emu->level;
//		eq->unknown0259[4] = emu->unknown0259[4];
	//	eq->beard = emu->beard;
		eq->petOwnerId = emu->petOwnerId;
		eq->guildrank = emu->guildrank;
		if(emu->NPC == 1)
			eq->guildrank = 0;

		eq->texture = emu->equip_chest2;
//		eq->unknown0194[3] = emu->unknown0194[3];
		for(k = 0; k < 9; k++) {
			eq->equipment[k] = emu->equipment[k];
			eq->equipcolors[k].color = emu->colors[k].color;
		}
		eq->runspeed = emu->runspeed;
		eq->AFK = emu->afk;
		eq->GuildID = emu->guildID;
		if(eq->GuildID == 0)
			eq->GuildID = 0xFFFF;
		//eq->title = emu->face;
//		eq->unknown0274 = emu->unknown0274;
		eq->helm = emu->helm;
		eq->race = emu->race;
//		eq->unknown0288 = emu->unknown0288;
		strncpy(eq->Surname, emu->lastName, 20);
		eq->walkspeed = emu->walkspeed;
//		eq->unknown0328 = emu->unknown0328;
//		eq->is_pet = emu->is_pet;
		eq->light = emu->light;
		if(emu->class_ > 19 && emu->class_ < 35)
			eq->class_ = emu->class_-3;
		else if(emu->class_ == 40)
			eq->class_ = 16;
		else if(emu->class_ == 41)
			eq->class_ = 32;
		else 
			eq->class_ = emu->class_;
		eq->haircolor = emu->haircolor;
		eq->beardcolor = emu->beardcolor;
		eq->eyecolor1 = emu->eyecolor1;
		eq->eyecolor2 = emu->eyecolor2;
		eq->hairstyle = emu->hairstyle;
		eq->beard = emu->beard;
		eq->face = emu->face;
//		eq->unknown0333 = emu->unknown0333;
	//	eq->flymode = emu->flymode;
		eq->gender = emu->gender;
		eq->bodytype = emu->bodytype;
//		eq->unknown0336[3] = emu->unknown0336[3];
	//	eq->equip_chest2 = emu->equip_chest2;
		eq->spawn_id = emu->spawnId;
//		eq->unknown0344[4] = emu->unknown0344[4];
		//eq->lfg = emu->lfg;

		/*
		if (emu->face == 99)	      {eq->face = 0;}
		if (emu->eyecolor1 == 99)  {eq->eyecolor1 = 0;}
		if (emu->eyecolor2 == 99)  {eq->eyecolor2 = 0;}
		if (emu->hairstyle == 99)  {eq->hairstyle = 0;}
		if (emu->haircolor == 99)  {eq->haircolor = 0;}
		if (emu->beard == 99)      {eq->beard = 0;}
		if (emu->beardcolor == 99) {eq->beardcolor = 0;}
		*/

	}
	_log(NET__STRUCTS, "Total size of bulkspawns packet uncompressed: %d", out->size);
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_ZoneSpawns, sizeof(structs::Spawn_Struct)*entrycount);
	outapp->pBuffer = new uchar[sizeof(structs::Spawn_Struct)*entrycount];
	outapp->size = DeflatePacket((unsigned char*)out->pBuffer, out->size, outapp->pBuffer, sizeof(structs::Spawn_Struct)*entrycount);
	EncryptZoneSpawnPacket(outapp->pBuffer, outapp->size);
	_log(NET__STRUCTS, "Total size of bulkspawns packet compressed: %d", outapp->size);

	//kill off the emu structure and send the eq packet.
	delete[] __emu_buffer;
	delete out;
	dest->FastQueuePacket(&outapp, ack_req);

}

ENCODE(OP_NewSpawn) {
	SETUP_DIRECT_ENCODE(Spawn_Struct, structs::Spawn_Struct);
	//zero out the packet. We could avoid this memset by setting all fields (including unknowns)
	//in the loop.
	int k = 0;
	//do the transform...
//		eq->unknown0000 = emu->unknown0000;
		eq->GM = emu->gm;
//		eq->unknown0003 = emu->unknown0003;
		//eq->title = emu->aaitle;
//		eq->unknown0004 = emu->unknown0004;
		eq->anon = emu->anon;
		//eq->face = emu->face;
		strncpy(eq->name, emu->name, 47);
		eq->deity = emu->deity;
//		eq->unknown0073 = emu->unknown0073
		eq->size = emu->size;
//		eq->unknown0079 = emu->unknown0079;
		eq->NPC = emu->NPC;
		eq->invis = emu->invis;
//		eq->haircolor = emu->haircolor;
		eq->cur_hp = emu->curHp;
		//eq->max_hp = emu->max_hp;
		//eq->findable = emu->findable;
//		eq->unknown0089[5] = emu->unknown0089[5];
		eq->deltaHeading = 0;
		eq->x_pos = (int16)emu->x;
//		eq->padding0054 = emu->padding0054;
		eq->y_pos = (int16)emu->y;
		eq->animation = emu->animation;
//		eq->padding0058 = emu->padding0058;
		eq->z_pos = (int16)emu->z*10;
		eq->deltaY = 0;
		eq->deltaX = 0;
		eq->heading = (uint8)emu->heading;
//		eq->padding0066 = emu->padding0066;
		eq->deltaZ = 0;
//		eq->padding0070 = emu->padding0070;
//		eq->eyecolor1 = emu->eyecolor1;
//		eq->unknown0115[24] = emu->unknown0115[24];
		//eq->showhelm = emu->showhelm;
//		eq->unknown0140[4] = emu->unknown0140[4];
		//eq->is_npc = emu->is_npc;
//		eq->hairstyle = emu->hairstyle;

		//if(emu->gender == 1){
		//	eq->hairstyle = eq->hairstyle == 0xFF ? 0 : eq->hairstyle;
		//}

		eq->haircolor = emu->haircolor;
		eq->beardcolor = emu->beardcolor;
		eq->eyecolor1 = emu->eyecolor1;
		eq->eyecolor2 = emu->eyecolor2;
		eq->hairstyle = emu->hairstyle;
		eq->beard = emu->beard;
		eq->face = emu->face;
//		eq->unknown0147[4] = emu->unknown0147[4];
		eq->level = emu->level;
//		eq->unknown0259[4] = emu->unknown0259[4];
	//	eq->beard = emu->beard;
		eq->petOwnerId = emu->petOwnerId;
		eq->guildrank = emu->guildrank;
//		eq->unknown0194[3] = emu->unknown0194[3];
		for(k = 0; k < 9; k++) {
			eq->equipment[k] = emu->equipment[k];
			eq->equipcolors[k].color = emu->colors[k].color;
		}
		eq->runspeed = emu->runspeed;
		eq->AFK = emu->afk;
		eq->GuildID = emu->guildID;
		eq->title = emu->face;
//		eq->unknown0274 = emu->unknown0274;
		eq->anim_type = 0x64;
		eq->texture = emu->equip_chest2;
        eq->helm = emu->helm;
		eq->race = emu->race;
		eq->GuildID = emu->guildID;
		if(eq->GuildID == 0)
			eq->GuildID = 0xFFFF;

		if(eq->guildrank == 0)
			eq->guildrank = 0xFF;
//		eq->unknown0288 = emu->unknown0288;
		strncpy(eq->Surname, emu->lastName, 20);
		eq->walkspeed = emu->walkspeed;
//		eq->unknown0328 = emu->unknown0328;
		//eq->is_pet = emu->is_pet;
		eq->light = emu->light;
		if(emu->class_ > 19 && emu->class_ < 35)
			eq->class_ = emu->class_-3;
		else if(emu->class_ == 40)
			eq->class_ = 16;
		else if(emu->class_ == 41)
			eq->class_ = 32;
		else 
			eq->class_ = emu->class_;
//		eq->eyecolor2 = emu->eyecolor2;
//		eq->unknown0333 = emu->unknown0333;
	//	eq->flymode = emu->flymode;
		eq->gender = emu->gender;
	//	eq->bodytype = emu->bodytype;
//		eq->unknown0336[3] = emu->unknown0336[3];
	//	eq->equip_chest2 = emu->equip_chest2;
		eq->spawn_id = emu->spawnId;
//		eq->unknown0344[4] = emu->unknown0344[4];
		//eq->lfg = emu->lfg;

		/*
		if (emu->face == 99)	      {eq->face = 0;}
		if (emu->eyecolor1 == 99)  {eq->eyecolor1 = 0;}
		if (emu->eyecolor2 == 99)  {eq->eyecolor2 = 0;}
		if (emu->hairstyle == 99)  {eq->hairstyle = 0;}
		if (emu->haircolor == 99)  {eq->haircolor = 0;}
		if (emu->beard == 99)      {eq->beard = 0;}
		if (emu->beardcolor == 99) {eq->beardcolor = 0;}
		*/
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_NewSpawn, sizeof(structs::Spawn_Struct));
	outapp->pBuffer = new uchar[sizeof(structs::Spawn_Struct)];
	outapp->size = DeflatePacket((unsigned char*)__packet->pBuffer, __packet->size, outapp->pBuffer, sizeof(structs::Spawn_Struct));
	EncryptZoneSpawnPacket(outapp->pBuffer, outapp->size);
	dest->FastQueuePacket(&outapp, ack_req);
	delete[] __emu_buffer;
}

DECODE(OP_ZoneChange)
{
	DECODE_LENGTH_EXACT(structs::ZoneChange_Struct);
	SETUP_DIRECT_DECODE(ZoneChange_Struct, structs::ZoneChange_Struct);
	memcpy(emu->char_name, eq->char_name, sizeof(emu->char_name));
	IN(zoneID);
	IN(success);
	eq->instanceID = 0;
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ZoneChange)
{
	ENCODE_LENGTH_EXACT(ZoneChange_Struct);
	SETUP_DIRECT_ENCODE(ZoneChange_Struct, structs::ZoneChange_Struct);

	memcpy(emu->char_name, eq->char_name, sizeof(emu->char_name));
	OUT(zoneID);
	OUT(success);
	if(emu->success != 1){
		memset(eq->error,0xff,sizeof(eq->error));
	}
	FINISH_ENCODE();
}

ENCODE(OP_CancelTrade)
{
	ENCODE_LENGTH_EXACT(CancelTrade_Struct);
	SETUP_DIRECT_ENCODE(CancelTrade_Struct, structs::CancelTrade_Struct);
	OUT(fromid);
	eq->action=1665;
	FINISH_ENCODE();
}

DECODE(OP_CancelTrade) {
	DECODE_LENGTH_EXACT(structs::TradeRequest_Struct);
	SETUP_DIRECT_DECODE(TradeRequest_Struct, structs::TradeRequest_Struct);
	IN(from_mob_id);
	IN(to_mob_id);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_MemorizeSpell) {
	SETUP_DIRECT_ENCODE(MemorizeSpell_Struct, structs::MemorizeSpell_Struct);
	OUT(slot);
	OUT(spell_id);
	OUT(scribing);
	FINISH_ENCODE();	
}

DECODE(OP_MemorizeSpell) {
	DECODE_LENGTH_EXACT(structs::MemorizeSpell_Struct);
	SETUP_DIRECT_DECODE(MemorizeSpell_Struct, structs::MemorizeSpell_Struct);
	IN(slot);
	IN(spell_id);
	IN(scribing);
	FINISH_DIRECT_DECODE();	
}

ENCODE(OP_Buff) {
	SETUP_DIRECT_ENCODE(SpellBuffFade_Struct, structs::SpellBuffFade_Struct);
	OUT(entityid);
	OUT(spellid);
	OUT(slotid);
	FINISH_ENCODE();	
}

DECODE(OP_Buff) {
	DECODE_LENGTH_EXACT(structs::Buff_Struct);
	SETUP_DIRECT_DECODE(SpellBuffFade_Struct, structs::SpellBuffFade_Struct);
	IN(entityid);
	IN(spellid);
	IN(slotid);
	FINISH_DIRECT_DECODE();	
}

ENCODE(OP_BeginCast)
{
	SETUP_DIRECT_ENCODE(BeginCast_Struct, structs::BeginCast_Struct);
	OUT(spell_id);
	OUT(caster_id);
	OUT(cast_time);
	FINISH_ENCODE();
}

ENCODE(OP_CastSpell)
{
	SETUP_DIRECT_ENCODE(CastSpell_Struct, structs::CastSpell_Struct);
	OUT(slot);
	OUT(spell_id);
	OUT(inventoryslot);
	OUT(target_id);
	FINISH_ENCODE();
}

DECODE(OP_CastSpell) {
	DECODE_LENGTH_EXACT(structs::CastSpell_Struct);
	SETUP_DIRECT_DECODE(CastSpell_Struct, structs::CastSpell_Struct);
	IN(slot);
	IN(spell_id);
	IN(inventoryslot);
	IN(target_id);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_Damage) {
	ENCODE_LENGTH_EXACT(CombatDamage_Struct);
	SETUP_DIRECT_ENCODE(CombatDamage_Struct, structs::CombatDamage_Struct);
	OUT(target);
	OUT(source);
	OUT(type);
	OUT(spellid);
	OUT(damage);
	//OUT(unknown11);
	//OUT(sequence);
	//OUT(unknown19);
	FINISH_ENCODE();
}

ENCODE(OP_Action) {
	ENCODE_LENGTH_EXACT(Action_Struct);
	SETUP_DIRECT_ENCODE(Action_Struct, structs::Action_Struct);
	OUT(target);
	OUT(source);
	OUT(level);
	eq->unknown6 = 0x41; //Think this is target level.
	OUT(instrument_mod);
	OUT(bard_focus_id);
	OUT(sequence);
	OUT(type);
	OUT(spell);
	OUT(buff_unknown);
	FINISH_ENCODE();
}

DECODE(OP_ConsiderCorpse) { DECODE_FORWARD(OP_Consider); }
DECODE(OP_Consider) {
	DECODE_LENGTH_EXACT(structs::Consider_Struct);
	SETUP_DIRECT_DECODE(Consider_Struct, structs::Consider_Struct);
	IN(playerid);
	IN(targetid);
	IN(faction);
	IN(level);
	IN(cur_hp);
	IN(max_hp);
	IN(pvpcon);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_Consider) {
	ENCODE_LENGTH_EXACT(Consider_Struct);
	SETUP_DIRECT_ENCODE(Consider_Struct, structs::Consider_Struct);
	OUT(playerid);
	OUT(targetid);
	OUT(faction);
	OUT(level);
	OUT(cur_hp);
	OUT(max_hp);
	OUT(pvpcon);
	FINISH_ENCODE();
}


DECODE(OP_ClickDoor) {
	DECODE_LENGTH_EXACT(structs::ClickDoor_Struct);
	SETUP_DIRECT_DECODE(ClickDoor_Struct, structs::ClickDoor_Struct);
	IN(doorid);
	IN(item_id);
	IN(player_id);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_InterruptCast) {
	ENCODE_LENGTH_EXACT(InterruptCast_Struct);
	SETUP_DIRECT_ENCODE(InterruptCast_Struct, structs::InterruptCast_Struct);
	OUT(spawnid);
	OUT(messageid);
	eq->message[0] = emu->message[0];
	FINISH_ENCODE();
}



DECODE(OP_GMEndTraining) {
	DECODE_LENGTH_EXACT(structs::GMTrainEnd_Struct);
	SETUP_DIRECT_DECODE(GMTrainEnd_Struct, structs::GMTrainEnd_Struct);
	IN(npcid);
	IN(playerid);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ItemLinkResponse) {  ENCODE_FORWARD(OP_ItemPacket); }
ENCODE(OP_ItemPacket) {
	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;
	ItemPacket_Struct *old_item_pkt=(ItemPacket_Struct *)__emu_buffer;
	InternalSerializedItem_Struct *int_struct=(InternalSerializedItem_Struct *)(old_item_pkt->SerializedItem);

	const ItemInst * item = (const ItemInst *)int_struct->inst;
	
	if(item)
	{
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ItemPacket,sizeof(structs::Item_Struct));

		outapp->SetOpcode(OP_Unknown);
		if(old_item_pkt->PacketType == ItemPacketSummonItem || int_struct->slot_id == 30)
			outapp->SetOpcode(OP_SummonedItem);
		else if(old_item_pkt->PacketType == ItemPacketViewLink)
			outapp->SetOpcode(OP_ItemLinkResponse);
		else if(old_item_pkt->PacketType == ItemPacketTradeView)
			outapp->SetOpcode(OP_TradeItemPacket);
		else if(old_item_pkt->PacketType == ItemPacketTrade || old_item_pkt->PacketType == ItemPacketMerchant)
			outapp->SetOpcode(OP_MerchantItemPacket);
		else if(old_item_pkt->PacketType == ItemPacketLoot)
			outapp->SetOpcode(OP_LootItemPacket);
		else if(item->GetItem()->ItemClass == 1)
			outapp->SetOpcode(OP_ContainerPacket);
		else if(item->GetItem()->ItemClass == 2)
			outapp->SetOpcode(OP_BookPacket);
		else
			outapp->SetOpcode(OP_ItemPacket);

		if(outapp->GetOpcode() == OP_BookPacket)
			outapp->size=261;
		else
			outapp->size=sizeof(structs::Item_Struct);

        structs::Item_Struct* myitem = (structs::Item_Struct*) outapp->pBuffer;
		
		//structs::Item_Struct* weasel = (structs::Item_Struct*) outapp->pBuffer;
		//_log(ZONE__INIT,"About to weasel in itempacket. Item: %s in slot: %i", item->GetItem()->Name, int_struct->slot_id);
		//WeaselTheJuice((ItemInst*)int_struct->inst,int_struct->slot_id,&outapp);

		if((int_struct->slot_id >= 251 && int_struct->slot_id <= 330) || (int_struct->slot_id >= 2031 && int_struct->slot_id <= 2110))
			myitem->equipSlot = int_struct->slot_id-1;
        else
			myitem->equipSlot = int_struct->slot_id;

		myitem->ItemClass = item->GetItem()->ItemClass;
		strcpy(myitem->Name,item->GetItem()->Name);
		strcpy(myitem->Lore,item->GetItem()->Lore);       
		strcpy(myitem->IDFile,item->GetItem()->IDFile);  
		myitem->Weight = item->GetItem()->Weight;      
		myitem->NoRent = item->GetItem()->NoRent;         
		myitem->NoDrop = item->GetItem()->NoDrop;         
		myitem->Size = item->GetItem()->Size;           
		myitem->ID = item->GetItem()->ID;        
		myitem->Icon = item->GetItem()->Icon;       
		myitem->Slots = item->GetItem()->Slots;  
		myitem->Price = item->GetPrice(); 
		myitem->Charges = item->GetCharges();     
		myitem->SellRate = item->GetItem()->SellRate;
		myitem->CastTime = item->GetItem()->CastTime;  
		myitem->SkillModType = item->GetItem()->SkillModType;
		myitem->SkillModValue = item->GetItem()->SkillModValue;
		myitem->BaneDmgRace = item->GetItem()->BaneDmgRace;
		myitem->BaneDmgBody = item->GetItem()->BaneDmgBody;
		myitem->BaneDmgAmt = item->GetItem()->BaneDmgAmt;
		myitem->RecLevel = item->GetItem()->RecLevel;       
		myitem->RecSkill = item->GetItem()->RecSkill;   
		myitem->ElemDmgType = item->GetItem()->ElemDmgType; 
		myitem->ElemDmgAmt = item->GetItem()->ElemDmgAmt;
		myitem->ReqLevel = item->GetItem()->ReqLevel; 
		myitem->FocusEffect = item->GetItem()->Focus.Effect;
	/*	myitem->unknown0212=0x8a;
		myitem->unknown0213=0x26;
		myitem->unknown0216=0x01;
		myitem->unknown0282=0xFF;
		myitem->unknown0283=0xFF;*/

		if(outapp->GetOpcode() == OP_ContainerPacket || item->GetItem()->ItemClass == 1){
			myitem->container.BagType = item->GetItem()->BagType; 
			myitem->container.BagSlots = item->GetItem()->BagSlots;         
			myitem->container.BagSize = item->GetItem()->BagSize;    
			myitem->container.BagWR = item->GetItem()->BagWR; 
		}
		else if(outapp->GetOpcode() == OP_BookPacket || item->GetItem()->ItemClass == 2){
			strcpy(myitem->book.Filename,item->GetItem()->Filename);
			myitem->book.Book = item->GetItem()->Book;         
			myitem->book.BookType = item->GetItem()->BookType; 
		}
		else
		{
		myitem->common.AStr = item->GetItem()->AStr;           
		myitem->common.ASta = item->GetItem()->ASta;           
		myitem->common.ACha = item->GetItem()->ACha;           
		myitem->common.ADex = item->GetItem()->ADex;           
		myitem->common.AInt = item->GetItem()->AInt;           
		myitem->common.AAgi = item->GetItem()->AAgi;           
		myitem->common.AWis = item->GetItem()->AWis;           
		myitem->common.MR = item->GetItem()->MR;             
		myitem->common.FR = item->GetItem()->FR;             
		myitem->common.CR = item->GetItem()->CR;             
		myitem->common.DR = item->GetItem()->DR;             
		myitem->common.PR = item->GetItem()->PR;             
		myitem->common.HP = item->GetItem()->HP;             
		myitem->common.Mana = item->GetItem()->Mana;           
		myitem->common.AC = item->GetItem()->AC;		
		myitem->common.MaxCharges = item->GetItem()->MaxCharges;    
		myitem->common.Light = item->GetItem()->Light;          
		myitem->common.Delay = item->GetItem()->Delay;          
		myitem->common.Damage = item->GetItem()->Damage;               
		myitem->common.Range = item->GetItem()->Range;          
		myitem->common.ItemType = item->GetItem()->ItemType;          
		myitem->common.Magic = item->GetItem()->Magic;          
		myitem->common.Material = item->GetItem()->Material;   
		myitem->common.Color = item->GetItem()->Color;    
		myitem->common.Deity = item->GetItem()->Deity;   
		myitem->common.Classes = item->GetItem()->Classes;  
		myitem->common.Races = item->GetItem()->Races;  
		myitem->common.Stackable = item->GetItem()->Stackable_; 
		}
		if(item->GetItem()->Click.Effect > 0){
			myitem->common.Effect1 = item->GetItem()->Click.Effect;
			myitem->Effect2 = item->GetItem()->Click.Effect; 
			myitem->EffectType2 = item->GetItem()->Click.Type;  
			myitem->common.EffectType1 = item->GetItem()->Click.Type;
			if(item->GetItem()->Click.Level > 0)
			{
				myitem->common.EffectLevel1 = item->GetItem()->Click.Level; 
				myitem->EffectLevel2 = item->GetItem()->Click.Level;
			}
			else
			{
				myitem->common.EffectLevel1 = item->GetItem()->Click.Level2; 
				myitem->EffectLevel2 = item->GetItem()->Click.Level2;  
			}
		}
		else if(item->GetItem()->Scroll.Effect > 0){
			myitem->common.Effect1 = item->GetItem()->Scroll.Effect;
			myitem->Effect2 = item->GetItem()->Scroll.Effect; 
			myitem->EffectType2 = item->GetItem()->Scroll.Type;  
			myitem->common.EffectType1 = item->GetItem()->Scroll.Type;
			if(item->GetItem()->Scroll.Level > 0)
			{
				myitem->common.EffectLevel1 = item->GetItem()->Scroll.Level; 
				myitem->EffectLevel2 = item->GetItem()->Scroll.Level;
			}
			else
			{
				myitem->common.EffectLevel1 = item->GetItem()->Scroll.Level2; 
				myitem->EffectLevel2 = item->GetItem()->Scroll.Level2;  
			}
		}
		else if(item->GetItem()->Proc.Effect > 0){
			myitem->common.Effect1 = item->GetItem()->Proc.Effect;
			myitem->Effect2 = item->GetItem()->Proc.Effect; 
			myitem->EffectType2 = item->GetItem()->Proc.Type;  
			myitem->common.EffectType1 = item->GetItem()->Proc.Type;
			if(item->GetItem()->Proc.Level > 0)
			{
				myitem->common.EffectLevel1 = item->GetItem()->Proc.Level; 
				myitem->EffectLevel2 = item->GetItem()->Proc.Level;
			}
			else
			{
				myitem->common.EffectLevel1 = item->GetItem()->Proc.Level2; 
				myitem->EffectLevel2 = item->GetItem()->Proc.Level2;  
			}
		}

		if(outapp->size != 360)
			_log(ZONE__INIT,"Invalid size on OP_ItemPacket packet. Expected: 360, Got: %i", outapp->size);

		//_log(ZONE__INIT,"I sent you a %s it's in slot: %i with charges: %i", myitem->Name, myitem->equipSlot, myitem->Charges);

		DumpPacket(outapp);
		dest->FastQueuePacket(&outapp);
		delete[] __emu_buffer;

	}
}

ENCODE(OP_CharInventory){

	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;

	int itemcount = in->size / sizeof(InternalSerializedItem_Struct);
	if(itemcount == 0 || (in->size % sizeof(InternalSerializedItem_Struct)) != 0) {
		_log(NET__STRUCTS, "Wrong size on outbound %s: Got %d, expected multiple of %d", opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(InternalSerializedItem_Struct));
		delete in;
		return;
	}

	int pisize = sizeof(structs::PlayerItems_Struct) + (250 * sizeof(structs::PlayerItemsPacket_Struct));
	structs::PlayerItems_Struct* pi = (structs::PlayerItems_Struct*) new uchar[pisize];
	memset(pi, 0, pisize);

	InternalSerializedItem_Struct *eq = (InternalSerializedItem_Struct *) in->pBuffer;
	//do the transform...
	int r;
	//std::string weasel_string;
	for(r = 0; r < itemcount; r++, eq++) {

		const ItemInst * sm_item = (const ItemInst *)eq->inst;

	//	_log(ZONE__INIT,"About to weasel in inventory.");
	//	char* weasel=WeaselTheJuice(sm_item,eq->slot_id);
	//	weasel_string.append(weasel);

		if((eq->slot_id >= 251 && eq->slot_id <= 330) || (eq->slot_id >= 2031 && eq->slot_id <= 2110))
			pi->packets[r].item.equipSlot = eq->slot_id-1;
        else
			pi->packets[r].item.equipSlot = eq->slot_id;

		pi->packets[r].item.ItemClass = sm_item->GetItem()->ItemClass;
		strcpy(pi->packets[r].item.Name,sm_item->GetItem()->Name);
		strcpy(pi->packets[r].item.Lore,sm_item->GetItem()->Lore);       
		strcpy(pi->packets[r].item.IDFile,sm_item->GetItem()->IDFile);  
		pi->packets[r].item.Weight = sm_item->GetItem()->Weight;      
		pi->packets[r].item.NoRent = sm_item->GetItem()->NoRent;         
		pi->packets[r].item.NoDrop = sm_item->GetItem()->NoDrop;         
		pi->packets[r].item.Size = sm_item->GetItem()->Size;           
		pi->packets[r].item.ID = sm_item->GetItem()->ID;        
		pi->packets[r].item.Icon = sm_item->GetItem()->Icon;       
		pi->packets[r].item.Slots = sm_item->GetItem()->Slots;  
		pi->packets[r].item.Price = sm_item->GetPrice(); 
		pi->packets[r].item.Charges = sm_item->GetCharges();     
		pi->packets[r].item.SellRate = sm_item->GetItem()->SellRate;
		pi->packets[r].item.CastTime = sm_item->GetItem()->CastTime;  
		pi->packets[r].item.SkillModType = sm_item->GetItem()->SkillModType;
		pi->packets[r].item.SkillModValue = sm_item->GetItem()->SkillModValue;
		pi->packets[r].item.BaneDmgRace = sm_item->GetItem()->BaneDmgRace;
		pi->packets[r].item.BaneDmgBody = sm_item->GetItem()->BaneDmgBody;
		pi->packets[r].item.BaneDmgAmt = sm_item->GetItem()->BaneDmgAmt;
		pi->packets[r].item.RecLevel = sm_item->GetItem()->RecLevel;       
		pi->packets[r].item.RecSkill = sm_item->GetItem()->RecSkill;   
		pi->packets[r].item.ElemDmgType = sm_item->GetItem()->ElemDmgType; 
		pi->packets[r].item.ElemDmgAmt = sm_item->GetItem()->ElemDmgAmt;
		pi->packets[r].item.ReqLevel = sm_item->GetItem()->ReqLevel; 
		pi->packets[r].item.FocusEffect = sm_item->GetItem()->Focus.Effect;
	/*	pi->packets[r].item.unknown0212=0x8a;
		pi->packets[r].item.unknown0213=0x26;
		pi->packets[r].item.unknown0216=0x01;
		pi->packets[r].item.unknown0282=0xFF;
		pi->packets[r].item.unknown0283=0xFF;*/

		if(sm_item->GetItem()->ItemClass == 1){
			pi->packets[r].item.container.BagType = sm_item->GetItem()->BagType; 
			pi->packets[r].item.container.BagSlots = sm_item->GetItem()->BagSlots;         
			pi->packets[r].item.container.BagSize = sm_item->GetItem()->BagSize;    
			pi->packets[r].item.container.BagWR = sm_item->GetItem()->BagWR; 
		}
		else if(sm_item->GetItem()->ItemClass == 2){
			strcpy(pi->packets[r].item.book.Filename,sm_item->GetItem()->Filename);
			pi->packets[r].item.book.Book = sm_item->GetItem()->Book;         
			pi->packets[r].item.book.BookType = sm_item->GetItem()->BookType; 
		}
		else
		{
		pi->packets[r].item.common.AStr = sm_item->GetItem()->AStr;           
		pi->packets[r].item.common.ASta = sm_item->GetItem()->ASta;           
		pi->packets[r].item.common.ACha = sm_item->GetItem()->ACha;           
		pi->packets[r].item.common.ADex = sm_item->GetItem()->ADex;           
		pi->packets[r].item.common.AInt = sm_item->GetItem()->AInt;           
		pi->packets[r].item.common.AAgi = sm_item->GetItem()->AAgi;           
		pi->packets[r].item.common.AWis = sm_item->GetItem()->AWis;           
		pi->packets[r].item.common.MR = sm_item->GetItem()->MR;             
		pi->packets[r].item.common.FR = sm_item->GetItem()->FR;             
		pi->packets[r].item.common.CR = sm_item->GetItem()->CR;             
		pi->packets[r].item.common.DR = sm_item->GetItem()->DR;             
		pi->packets[r].item.common.PR = sm_item->GetItem()->PR;             
		pi->packets[r].item.common.HP = sm_item->GetItem()->HP;             
		pi->packets[r].item.common.Mana = sm_item->GetItem()->Mana;           
		pi->packets[r].item.common.AC = sm_item->GetItem()->AC;		
		pi->packets[r].item.common.MaxCharges = sm_item->GetItem()->MaxCharges;    
		pi->packets[r].item.common.Light = sm_item->GetItem()->Light;          
		pi->packets[r].item.common.Delay = sm_item->GetItem()->Delay;          
		pi->packets[r].item.common.Damage = sm_item->GetItem()->Damage;               
		pi->packets[r].item.common.Range = sm_item->GetItem()->Range;          
		pi->packets[r].item.common.ItemType = sm_item->GetItem()->ItemType;          
		pi->packets[r].item.common.Magic = sm_item->GetItem()->Magic;          
		pi->packets[r].item.common.Material = sm_item->GetItem()->Material;   
		pi->packets[r].item.common.Color = sm_item->GetItem()->Color;    
		pi->packets[r].item.common.Deity = sm_item->GetItem()->Deity;   
		pi->packets[r].item.common.Classes = sm_item->GetItem()->Classes;  
		pi->packets[r].item.common.Races = sm_item->GetItem()->Races;  
		pi->packets[r].item.common.Stackable = sm_item->GetItem()->Stackable_; 
		}
		if(sm_item->GetItem()->Click.Effect > 0){
			pi->packets[r].item.common.Effect1 = sm_item->GetItem()->Click.Effect;
			pi->packets[r].item.Effect2 = sm_item->GetItem()->Click.Effect; 
			pi->packets[r].item.EffectType2 = sm_item->GetItem()->Click.Type;  
			pi->packets[r].item.common.EffectType1 = sm_item->GetItem()->Click.Type;
			if(sm_item->GetItem()->Click.Level > 0)
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Click.Level; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Click.Level;
			}
			else
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Click.Level2; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Click.Level2;  
			}
		}
		else if(sm_item->GetItem()->Scroll.Effect > 0){
			pi->packets[r].item.common.Effect1 = sm_item->GetItem()->Scroll.Effect;
			pi->packets[r].item.Effect2 = sm_item->GetItem()->Scroll.Effect; 
			pi->packets[r].item.EffectType2 = sm_item->GetItem()->Scroll.Type;  
			pi->packets[r].item.common.EffectType1 = sm_item->GetItem()->Scroll.Type;
			if(sm_item->GetItem()->Scroll.Level > 0)
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Scroll.Level; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Scroll.Level;
			}
			else
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Scroll.Level2; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Scroll.Level2;  
			}
		}
		else if(sm_item->GetItem()->Proc.Effect > 0){
			pi->packets[r].item.common.Effect1 = sm_item->GetItem()->Proc.Effect;
			pi->packets[r].item.Effect2 = sm_item->GetItem()->Proc.Effect; 
			pi->packets[r].item.EffectType2 = sm_item->GetItem()->Proc.Type;  
			pi->packets[r].item.common.EffectType1 = sm_item->GetItem()->Proc.Type;
			if(sm_item->GetItem()->Proc.Level > 0)
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Proc.Level; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Proc.Level;
			}
			else
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Proc.Level2; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Proc.Level2;  
			}
		}

	}
	int32 length = 5000;
	int buffer = 2;

//	memcpy(pi->packets,weasel_string.c_str(),weasel_string.length());
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_CharInventory, length);
	outapp->size = buffer + DeflatePacket((uchar*) pi->packets, itemcount * sizeof(structs::PlayerItemsPacket_Struct), &outapp->pBuffer[buffer], length-buffer);
	pi->count = itemcount;

	dest->FastQueuePacket(&outapp);
	delete[] __emu_buffer;
	
}

ENCODE(OP_ShopInventoryPacket)
{
	//TODO: Add opcode field to InternalSerializedItem_Struct to combine this with OP_CharInventory as it's the same function :I
	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;

	int itemcount = in->size / sizeof(InternalSerializedItem_Struct);
	if(itemcount == 0 || (in->size % sizeof(InternalSerializedItem_Struct)) != 0) {
		_log(ZONE__INIT, "Wrong size on outbound %s: Got %d, expected multiple of %d", opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(InternalSerializedItem_Struct));
		delete in;
		return;
	}

	int pisize = sizeof(structs::MerchantItems_Struct) + (80 * sizeof(structs::MerchantItemsPacket_Struct));
	structs::MerchantItems_Struct* pi = (structs::MerchantItems_Struct*) new uchar[pisize];
	memset(pi, 0, pisize);

	InternalSerializedItem_Struct *eq = (InternalSerializedItem_Struct *) in->pBuffer;
	//do the transform...
	//std::string weasel_string;
	int r = 0;
	for(r = 0; r < itemcount; r++, eq++) {

		const ItemInst * sm_item = (const ItemInst *)eq->inst;

	//	_log(ZONE__INIT,"About to weasel in merchant.");
	//	char* weasel=WeaselTheJuice(sm_item,r,1);
	//	weasel_string.append(weasel);

//_log(ZONE__INIT, "About to start copying items to struct %s in merchant slot: %i with price: %i with charges: %i", sm_item->GetItem()->Name, r, sm_item->GetPrice(), sm_item->GetCharges());
		pi->packets[r].itemtype = sm_item->GetItem()->ItemClass;
		pi->packets[r].item.equipSlot = sm_item->GetMerchantSlot();
		pi->packets[r].item.ItemClass = sm_item->GetItem()->ItemClass;
		strcpy(pi->packets[r].item.Name,sm_item->GetItem()->Name);
		strcpy(pi->packets[r].item.Lore,sm_item->GetItem()->Lore);       
		strcpy(pi->packets[r].item.IDFile,sm_item->GetItem()->IDFile);  
		pi->packets[r].item.Weight = sm_item->GetItem()->Weight;      
		pi->packets[r].item.NoRent = sm_item->GetItem()->NoRent;         
		pi->packets[r].item.NoDrop = sm_item->GetItem()->NoDrop;         
		pi->packets[r].item.Size = sm_item->GetItem()->Size;           
		pi->packets[r].item.ID = sm_item->GetItem()->ID;        
		pi->packets[r].item.Icon = sm_item->GetItem()->Icon;       
		pi->packets[r].item.Slots = sm_item->GetItem()->Slots;  
		pi->packets[r].item.Price = sm_item->GetPrice(); 
		if(sm_item->GetMerchantCount() > 1)
			pi->packets[r].item.Charges = sm_item->GetMerchantCount();    
		else
			pi->packets[r].item.Charges = 1;    
		pi->packets[r].item.SellRate = sm_item->GetItem()->SellRate;
		pi->packets[r].item.CastTime = sm_item->GetItem()->CastTime;  
		pi->packets[r].item.SkillModType = sm_item->GetItem()->SkillModType;
		pi->packets[r].item.SkillModValue = sm_item->GetItem()->SkillModValue;
		pi->packets[r].item.BaneDmgRace = sm_item->GetItem()->BaneDmgRace;
		pi->packets[r].item.BaneDmgBody = sm_item->GetItem()->BaneDmgBody;
		pi->packets[r].item.BaneDmgAmt = sm_item->GetItem()->BaneDmgAmt;
		pi->packets[r].item.RecLevel = sm_item->GetItem()->RecLevel;       
		pi->packets[r].item.RecSkill = sm_item->GetItem()->RecSkill;   
		pi->packets[r].item.ElemDmgType = sm_item->GetItem()->ElemDmgType; 
		pi->packets[r].item.ElemDmgAmt = sm_item->GetItem()->ElemDmgAmt;
		pi->packets[r].item.ReqLevel = sm_item->GetItem()->ReqLevel; 
		pi->packets[r].item.FocusEffect = sm_item->GetItem()->Focus.Effect;
	/*	pi->packets[r].item.unknown0212=0x8a;
		pi->packets[r].item.unknown0213=0x26;
		pi->packets[r].item.unknown0216=0x01;
		pi->packets[r].item.unknown0282=0xFF;
		pi->packets[r].item.unknown0283=0xFF;*/

		if(sm_item->GetItem()->ItemClass == 1){
			pi->packets[r].item.container.BagType = sm_item->GetItem()->BagType; 
			pi->packets[r].item.container.BagSlots = sm_item->GetItem()->BagSlots;         
			pi->packets[r].item.container.BagSize = sm_item->GetItem()->BagSize;    
			pi->packets[r].item.container.BagWR = sm_item->GetItem()->BagWR; 
		}
		else if(sm_item->GetItem()->ItemClass == 2){
			strcpy(pi->packets[r].item.book.Filename,sm_item->GetItem()->Filename);
			pi->packets[r].item.book.Book = sm_item->GetItem()->Book;         
			pi->packets[r].item.book.BookType = sm_item->GetItem()->BookType; 
		}
		else
		{
		pi->packets[r].item.common.AStr = sm_item->GetItem()->AStr;           
		pi->packets[r].item.common.ASta = sm_item->GetItem()->ASta;           
		pi->packets[r].item.common.ACha = sm_item->GetItem()->ACha;           
		pi->packets[r].item.common.ADex = sm_item->GetItem()->ADex;           
		pi->packets[r].item.common.AInt = sm_item->GetItem()->AInt;           
		pi->packets[r].item.common.AAgi = sm_item->GetItem()->AAgi;           
		pi->packets[r].item.common.AWis = sm_item->GetItem()->AWis;           
		pi->packets[r].item.common.MR = sm_item->GetItem()->MR;             
		pi->packets[r].item.common.FR = sm_item->GetItem()->FR;             
		pi->packets[r].item.common.CR = sm_item->GetItem()->CR;             
		pi->packets[r].item.common.DR = sm_item->GetItem()->DR;             
		pi->packets[r].item.common.PR = sm_item->GetItem()->PR;             
		pi->packets[r].item.common.HP = sm_item->GetItem()->HP;             
		pi->packets[r].item.common.Mana = sm_item->GetItem()->Mana;           
		pi->packets[r].item.common.AC = sm_item->GetItem()->AC;		
		pi->packets[r].item.common.MaxCharges = sm_item->GetItem()->MaxCharges;    
		pi->packets[r].item.common.Light = sm_item->GetItem()->Light;          
		pi->packets[r].item.common.Delay = sm_item->GetItem()->Delay;          
		pi->packets[r].item.common.Damage = sm_item->GetItem()->Damage;               
		pi->packets[r].item.common.Range = sm_item->GetItem()->Range;          
		pi->packets[r].item.common.ItemType = sm_item->GetItem()->ItemType;          
		pi->packets[r].item.common.Magic = sm_item->GetItem()->Magic;          
		pi->packets[r].item.common.Material = sm_item->GetItem()->Material;   
		pi->packets[r].item.common.Color = sm_item->GetItem()->Color;    
		pi->packets[r].item.common.Deity = sm_item->GetItem()->Deity;   
		pi->packets[r].item.common.Classes = sm_item->GetItem()->Classes;  
		pi->packets[r].item.common.Races = sm_item->GetItem()->Races;  
		pi->packets[r].item.common.Stackable = sm_item->GetItem()->Stackable_; 
		}
		if(sm_item->GetItem()->Click.Effect > 0){
			pi->packets[r].item.common.Effect1 = sm_item->GetItem()->Click.Effect;
			pi->packets[r].item.Effect2 = sm_item->GetItem()->Click.Effect; 
			pi->packets[r].item.EffectType2 = sm_item->GetItem()->Click.Type;  
			pi->packets[r].item.common.EffectType1 = sm_item->GetItem()->Click.Type;
			if(sm_item->GetItem()->Click.Level > 0)
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Click.Level; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Click.Level;
			}
			else
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Click.Level2; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Click.Level2;  
			}
		}
		else if(sm_item->GetItem()->Scroll.Effect > 0){
			pi->packets[r].item.common.Effect1 = sm_item->GetItem()->Scroll.Effect;
			pi->packets[r].item.Effect2 = sm_item->GetItem()->Scroll.Effect; 
			pi->packets[r].item.EffectType2 = sm_item->GetItem()->Scroll.Type;  
			pi->packets[r].item.common.EffectType1 = sm_item->GetItem()->Scroll.Type;
			if(sm_item->GetItem()->Scroll.Level > 0)
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Scroll.Level; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Scroll.Level;
			}
			else
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Scroll.Level2; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Scroll.Level2;  
			}
		}
		else if(sm_item->GetItem()->Proc.Effect > 0){
			pi->packets[r].item.common.Effect1 = sm_item->GetItem()->Proc.Effect;
			pi->packets[r].item.Effect2 = sm_item->GetItem()->Proc.Effect; 
			pi->packets[r].item.EffectType2 = sm_item->GetItem()->Proc.Type;  
			pi->packets[r].item.common.EffectType1 = sm_item->GetItem()->Proc.Type;
			if(sm_item->GetItem()->Proc.Level > 0)
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Proc.Level; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Proc.Level;
			}
			else
			{
				pi->packets[r].item.common.EffectLevel1 = sm_item->GetItem()->Proc.Level2; 
				pi->packets[r].item.EffectLevel2 = sm_item->GetItem()->Proc.Level2;  
			}
		}
		

   //_log(ZONE__INIT,"MERCHANT PRICE IS SET TO: %i for item: %s in slot: %i quantity: %i", pi->packets[r].item.Price, pi->packets[r].item.Name, pi->packets[r].item.equipSlot, pi->packets[r].item.Charges);
		
	}
	int32 length = 5000;
	int buffer = 2;

	//_log(ZONE__INIT,"Senind %i merchant items", itemcount);
	//memcpy(pi->packets,weasel_string.c_str(),weasel_string.length());
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_ShopInventoryPacket, length);
	outapp->size = buffer + DeflatePacket((uchar*) pi->packets, itemcount * sizeof(structs::MerchantItemsPacket_Struct), &outapp->pBuffer[buffer], length-buffer);
	pi->count = itemcount;

	dest->FastQueuePacket(&outapp);
	delete[] __emu_buffer;
}

DECODE(OP_MoveItem)
{
	SETUP_DIRECT_DECODE(MoveItem_Struct, structs::MoveItem_Struct);

	if(eq->to_slot == 0)
		emu->to_slot=30;
	else if((eq->to_slot >= 250 && eq->to_slot <= 329) || (eq->to_slot >= 2030 && eq->to_slot <= 2109)) 
		emu->to_slot = eq->to_slot+1;
	else
		emu->to_slot=eq->to_slot;

	if(eq->from_slot == 0)
		emu->from_slot=30;
	else if((eq->from_slot >= 250 && eq->from_slot <= 329) || (eq->from_slot >= 2030 && eq->from_slot <= 2109)) 
		emu->from_slot = eq->from_slot+1;
	else
		emu->from_slot=eq->from_slot;

	IN(number_in_stack);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_DeleteItem) {  ENCODE_FORWARD(OP_MoveItem); }
ENCODE(OP_DeleteCharge) {  ENCODE_FORWARD(OP_MoveItem); }
ENCODE(OP_MoveItem)
{
	ENCODE_LENGTH_EXACT(MoveItem_Struct);
	SETUP_DIRECT_ENCODE(MoveItem_Struct, structs::MoveItem_Struct);

	if(emu->to_slot == 30)
		eq->to_slot=0;
	else if((emu->to_slot >= 251 && emu->to_slot <= 330) || (emu->to_slot >= 2031 && emu->to_slot <= 2110)) 
		eq->to_slot = emu->to_slot-1;
	else
		eq->to_slot=emu->to_slot;

	if(emu->from_slot == 30)
		eq->from_slot=0;
	else if((emu->from_slot >= 251 && emu->from_slot <= 330) || (emu->from_slot >= 2031 && emu->from_slot <= 2110)) 
		eq->from_slot = emu->from_slot-1;
	else
		eq->from_slot=emu->from_slot;

	OUT(number_in_stack);
	FINISH_ENCODE();
}

ENCODE(OP_Stamina)
{
	ENCODE_LENGTH_EXACT(Stamina_Struct);
	SETUP_DIRECT_ENCODE(Stamina_Struct, structs::Stamina_Struct);
	OUT(food);
	OUT(water);
	FINISH_ENCODE();
}

ENCODE(OP_HPUpdate)
{
	ENCODE_LENGTH_EXACT(SpawnHPUpdate_Struct);
	SETUP_DIRECT_ENCODE(SpawnHPUpdate_Struct, structs::SpawnHPUpdate_Struct);
	OUT(spawn_id);
	OUT(cur_hp);
	OUT(max_hp);
	FINISH_ENCODE();
}

ENCODE(OP_MobHealth)
{
	ENCODE_LENGTH_EXACT(SpawnHPUpdate_Struct2);
	SETUP_DIRECT_ENCODE(SpawnHPUpdate_Struct2, structs::SpawnHPUpdate_Struct);
	OUT(spawn_id);
	eq->cur_hp=emu->hp;
	eq->max_hp=100;
	FINISH_ENCODE();
}


DECODE(OP_Consume) 
{
	DECODE_LENGTH_EXACT(structs::Consume_Struct);
	SETUP_DIRECT_DECODE(Consume_Struct, structs::Consume_Struct);

	if(eq->slot >= 250 && eq->slot <= 329)
		emu->slot = eq->slot+1;
	else
		emu->slot = eq->slot;
	IN(type);
	IN(auto_consumed);

	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ReadBook) 
{

	EQApplicationPacket *in = *p;
	*p = nullptr;

	unsigned char *__emu_buffer = in->pBuffer;
	BookText_Struct *emu_BookText_Struct = (BookText_Struct *)__emu_buffer;
	in->size = sizeof(structs::BookText_Struct) + strlen(emu_BookText_Struct->booktext);
	in->pBuffer = new unsigned char[in->size];
	structs::BookText_Struct *eq_BookText_Struct = (structs::BookText_Struct*)in->pBuffer;

	eq_BookText_Struct->type = emu_BookText_Struct->type;
	strcpy(eq_BookText_Struct->booktext, emu_BookText_Struct->booktext);

	delete[] __emu_buffer;
	dest->FastQueuePacket(&in, ack_req);

}

DECODE(OP_ReadBook) 
{
	DECODE_LENGTH_ATLEAST(structs::BookRequest_Struct);
	SETUP_DIRECT_DECODE(BookRequest_Struct, structs::BookRequest_Struct);

	IN(type);
	strn0cpy(emu->txtfile, eq->txtfile, sizeof(emu->txtfile));

	FINISH_DIRECT_DECODE();
}

ENCODE(OP_Illusion) 
{
	ENCODE_LENGTH_EXACT(Illusion_Struct);
	SETUP_DIRECT_ENCODE(Illusion_Struct, structs::Illusion_Struct);
	OUT(spawnid);
	//OUT_str(charname);
	OUT(race);
	OUT(gender);
	OUT(texture);
	OUT(helmtexture);
	OUT(face);
	OUT(hairstyle);
	OUT(haircolor);
	OUT(beard);
	OUT(beardcolor);
	OUT(size);
	eq->unknown_void=0xFFFFFFFF;

	FINISH_ENCODE();
}

ENCODE(OP_ShopRequest)
{
	ENCODE_LENGTH_EXACT(Merchant_Click_Struct);
	SETUP_DIRECT_ENCODE(Merchant_Click_Struct, structs::Merchant_Click_Struct);
	eq->npcid=emu->npcid;
	OUT(playerid);
	OUT(command);
	//eq->unknown[0] = 0xD4;
	//eq->unknown[1] = 0x53;
	//eq->unknown[2] = 0x00;
	OUT(rate);
	FINISH_ENCODE();
}

DECODE(OP_ShopRequest) 
{
	DECODE_LENGTH_EXACT(structs::Merchant_Click_Struct);
	SETUP_DIRECT_DECODE(Merchant_Click_Struct, structs::Merchant_Click_Struct);
	emu->npcid=eq->npcid;
	IN(playerid);
	IN(command);
	IN(rate);
	FINISH_DIRECT_DECODE();
}

DECODE(OP_ShopPlayerBuy)
{
	DECODE_LENGTH_EXACT(structs::Merchant_Sell_Struct);
	SETUP_DIRECT_DECODE(Merchant_Sell_Struct, structs::Merchant_Sell_Struct);
	emu->npcid=eq->npcid;
	IN(playerid);
	IN(itemslot);
	IN(quantity);
	IN(price);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ShopPlayerBuy)
{
	ENCODE_LENGTH_EXACT(Merchant_Sell_Struct);
	SETUP_DIRECT_ENCODE(Merchant_Sell_Struct, structs::Merchant_Sell_Struct);
	eq->npcid=emu->npcid;
	eq->playerid=emu->playerid;
	OUT(itemslot);
	OUT(quantity);
	OUT(price);
	FINISH_ENCODE();
}

DECODE(OP_ShopPlayerSell)
{
	DECODE_LENGTH_EXACT(structs::Merchant_Purchase_Struct);
	SETUP_DIRECT_DECODE(Merchant_Purchase_Struct, structs::Merchant_Purchase_Struct);
	emu->npcid=eq->npcid;
	//IN(playerid);
	IN(itemslot);
	IN(quantity);
	IN(price);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ShopPlayerSell)
{
	ENCODE_LENGTH_EXACT(Merchant_Purchase_Struct);
	SETUP_DIRECT_ENCODE(Merchant_Purchase_Struct, structs::Merchant_Purchase_Struct);
	eq->npcid=emu->npcid;
	//eq->playerid=emu->playerid;
	OUT(itemslot);
	OUT(quantity);
	OUT(price);
	FINISH_ENCODE();
}

ENCODE(OP_ShopDelItem)
{
	ENCODE_LENGTH_EXACT(Merchant_DelItem_Struct);
	SETUP_DIRECT_ENCODE(Merchant_DelItem_Struct, structs::Merchant_DelItem_Struct);
	eq->npcid=emu->npcid;
	OUT(playerid);
	OUT(itemslot);
	FINISH_ENCODE();
}

ENCODE(OP_Animation)
{
	ENCODE_LENGTH_EXACT(Animation_Struct);
	SETUP_DIRECT_ENCODE(Animation_Struct, structs::Animation_Struct);
	OUT(spawnid);
	OUT(action);
	eq->a_unknown[5]=0x80;
	eq->a_unknown[6]=0x3F;
	FINISH_ENCODE();
}

DECODE(OP_Animation)
{
	DECODE_LENGTH_EXACT(structs::Animation_Struct);
	SETUP_DIRECT_DECODE(Animation_Struct, structs::Animation_Struct);
	IN(spawnid);
	IN(action);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_LootItem) {

	ENCODE_LENGTH_EXACT(LootingItem_Struct);
	SETUP_DIRECT_ENCODE(LootingItem_Struct, structs::LootingItem_Struct);
	OUT(lootee);
	OUT(looter);
	OUT(slot_id);
	OUT(auto_loot);

	FINISH_ENCODE();
}

DECODE(OP_LootItem) {
	DECODE_LENGTH_EXACT(structs::LootingItem_Struct);
	SETUP_DIRECT_DECODE(LootingItem_Struct, structs::LootingItem_Struct);
	IN(lootee);
	IN(looter);
	IN(slot_id);
	IN(auto_loot);

	FINISH_DIRECT_DECODE();
}

ENCODE(OP_AAExpUpdate){
	
	ENCODE_LENGTH_EXACT(AltAdvStats_Struct);
	SETUP_DIRECT_ENCODE(AltAdvStats_Struct, structs::AltAdvStats_Struct);
	OUT(experience);
	OUT(unspent);
	OUT(percentage);
	FINISH_ENCODE();
}

ENCODE(OP_AAAction){
	ENCODE_LENGTH_EXACT(UseAA_Struct);
	SETUP_DIRECT_ENCODE(UseAA_Struct, structs::UseAA_Struct);
	OUT(end);
	OUT(ability);
	OUT(begin);
	eq->unknown_void=2154;

	char* packet_dump = "AAAction_OUT.txt";
	FileDumpPacketHex(packet_dump, __packet);

	FINISH_ENCODE();
}

ENCODE(OP_GroundSpawn) {

	ENCODE_LENGTH_EXACT(Object_Struct);
	SETUP_DIRECT_ENCODE(Object_Struct, structs::Object_Struct);
    OUT(drop_id);
	OUT(zone_id);
	OUT(heading);
	OUT(z);
	OUT(y);
	OUT(x);
	strncpy(eq->object_name,emu->object_name,16);
	OUT(object_type);
	eq->charges=1;
	eq->maxcharges=1;
	FINISH_ENCODE();
}

DECODE(OP_GroundSpawn) {
	DECODE_LENGTH_EXACT(structs::Object_Struct);
	SETUP_DIRECT_DECODE(Object_Struct, structs::Object_Struct);
    IN(drop_id);
	IN(zone_id);
	IN(heading);
	IN(z);
	IN(y);
	IN(x);
	strncpy(emu->object_name,eq->object_name,16);
	IN(object_type);

	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ClickObjectAction) {

	ENCODE_LENGTH_EXACT(ClickObjectAction_Struct);
	SETUP_DIRECT_ENCODE(ClickObjectAction_Struct, structs::ClickObjectAction_Struct);
	OUT(player_id);
    OUT(drop_id);
	OUT(open);
	OUT(type);
	OUT(icon);
	eq->slot=emu->unknown16;
	FINISH_ENCODE();
}

DECODE(OP_ClickObjectAction) {
	DECODE_LENGTH_EXACT(structs::ClickObjectAction_Struct);
	SETUP_DIRECT_DECODE(ClickObjectAction_Struct, structs::ClickObjectAction_Struct);
	IN(player_id);
    IN(drop_id);
	IN(open);
	IN(type);
	IN(icon);
	emu->unknown16=eq->slot;
	FINISH_DIRECT_DECODE();
}

DECODE(OP_TradeSkillCombine) {
	DECODE_LENGTH_EXACT(structs::Combine_Struct);
	SETUP_DIRECT_DECODE(NewCombine_Struct, structs::Combine_Struct);
	IN(container_slot);
	FINISH_DIRECT_DECODE();
}

DECODE(OP_TradeRequest) {
	DECODE_LENGTH_EXACT(structs::TradeRequest_Struct);
	SETUP_DIRECT_DECODE(TradeRequest_Struct, structs::TradeRequest_Struct);
	IN(from_mob_id);
	IN(to_mob_id);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_TradeRequest) {

	ENCODE_LENGTH_EXACT(TradeRequest_Struct);
	SETUP_DIRECT_ENCODE(TradeRequest_Struct, structs::TradeRequest_Struct);
	OUT(from_mob_id);
    OUT(to_mob_id);
	FINISH_ENCODE();
}

DECODE(OP_TradeRequestAck) {
	DECODE_LENGTH_EXACT(structs::TradeRequest_Struct);
	SETUP_DIRECT_DECODE(TradeRequest_Struct, structs::TradeRequest_Struct);
	IN(from_mob_id);
	IN(to_mob_id);
	FINISH_DIRECT_DECODE();
}


ENCODE(OP_TradeRequestAck) {

	ENCODE_LENGTH_EXACT(TradeRequest_Struct);
	SETUP_DIRECT_ENCODE(TradeRequest_Struct, structs::TradeRequest_Struct);
	OUT(from_mob_id);
	OUT(to_mob_id);
	FINISH_ENCODE();
}

ENCODE(OP_ManaChange){
	ENCODE_LENGTH_EXACT(ManaChange_Struct);
	SETUP_DIRECT_ENCODE(ManaChange_Struct, structs::ManaChange_Struct);
	OUT(new_mana);
	OUT(spell_id);
	FINISH_ENCODE();
}

ENCODE(OP_DeleteSpawn) {
	SETUP_DIRECT_ENCODE(DeleteSpawn_Struct, structs::DeleteSpawn_Struct);
	OUT(spawn_id);
	FINISH_ENCODE();
}

ENCODE(OP_TimeOfDay){
	SETUP_DIRECT_ENCODE(TimeOfDay_Struct, structs::TimeOfDay_Struct);
	OUT(hour);
	OUT(minute);
	OUT(day);
	OUT(month);
	OUT(year);
	FINISH_ENCODE();
}

DECODE(OP_WhoAllRequest) {
	DECODE_LENGTH_EXACT(structs::Who_All_Struct);
	SETUP_DIRECT_DECODE(Who_All_Struct, structs::Who_All_Struct);
	strcpy(emu->whom,eq->whom);
	IN(wrace);
	IN(wclass);
	IN(lvllow);
	IN(lvlhigh);
	IN(gmlookup);
	IN(guildid);
	emu->type = 3;
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_GroupInvite2) { ENCODE_FORWARD(OP_GroupInvite); }
ENCODE(OP_GroupInvite) {

	ENCODE_LENGTH_EXACT(GroupInvite_Struct);
	SETUP_DIRECT_ENCODE(GroupInvite_Struct, structs::GroupInvite_Struct);
	strcpy(eq->invitee_name,emu->invitee_name);
	strcpy(eq->inviter_name,emu->inviter_name);
	FINISH_ENCODE();
}

DECODE(OP_GroupInvite2) { DECODE_FORWARD(OP_GroupInvite); }
DECODE(OP_GroupInvite) {
	DECODE_LENGTH_EXACT(structs::GroupInvite_Struct);
	SETUP_DIRECT_DECODE(GroupInvite_Struct, structs::GroupInvite_Struct);
	strcpy(emu->invitee_name,eq->invitee_name);
	strcpy(emu->inviter_name,eq->inviter_name);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_TradeCoins) {
	SETUP_DIRECT_ENCODE(TradeCoin_Struct, structs::TradeCoin_Struct);
	OUT(trader);
	OUT(slot);
	OUT(amount);
	FINISH_ENCODE();
}

DECODE(OP_ItemLinkResponse){
	DECODE_LENGTH_EXACT(structs::ItemViewRequest_Struct);
	SETUP_DIRECT_DECODE(LDONItemViewRequest_Struct, structs::ItemViewRequest_Struct);
	IN(item_id);
	strcpy(emu->item_name,eq->item_name);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_LogServer) {

	ENCODE_LENGTH_EXACT(LogServer_Struct);
	SETUP_DIRECT_ENCODE(LogServer_Struct, structs::LogServer_Struct);
	eq->pk_active = emu->enable_pvp;
	eq->rp_active = emu->enable_FV;
	strcpy(eq->worldshortname, emu->worldshortname);
	FINISH_ENCODE();
}

ENCODE(OP_RequestClientZoneChange){
	SETUP_DIRECT_ENCODE(RequestClientZoneChange_Struct, structs::RequestClientZoneChange_Struct);
	OUT(zone_id);
	OUT(x);
	OUT(y);
	OUT(z);
	OUT(heading);
	OUT(type);
	FINISH_ENCODE();
}

//Just using OP_RequestClientZoneChange to keep the zoning code as stock as possible.
ENCODE(OP_ZonePlayerToBind){
	SETUP_DIRECT_ENCODE(ZonePlayerToBind_Struct, structs::RequestClientZoneChange_Struct);
	eq->zone_id = emu->bind_zone_id;
	OUT(x);
	OUT(y);
	OUT(z);
	OUT(heading);
	FINISH_ENCODE();
}

/*ENCODE(OP_FormattedMessage)
{
	EQApplicationPacket *__packet = *p; 
	*p = nullptr; 
	unsigned char *__emu_buffer = __packet->pBuffer; 
	FormattedMessage_Struct *emu = (FormattedMessage_Struct *) __emu_buffer; 
	uint32 __i = 0; 
	__i++; 
	
	int msglen = __packet->size - sizeof(FormattedMessage_Struct);
	int len = sizeof(structs::FormattedMessage_Struct) + msglen - 6;
	__packet->pBuffer = new unsigned char[len]; 
	__packet->size = len; 
	memset(__packet->pBuffer, 0, len); 
	structs::FormattedMessage_Struct *eq = (structs::FormattedMessage_Struct *) __packet->pBuffer; 
	eq->string_id = emu->string_id;
	eq->type = emu->type;
	eq->unknown0 = 0x0000;
	strcpy(eq->message, emu->message);
	FINISH_ENCODE();
}*/

/*char *WeaselTheJuice(const ItemInst *inst, int16 slot_id, int type) {
	char *returned_juice = nullptr;
	//structs::Item_Struct* thejuice = (structs::Item_Struct*) returned_juice;
	//structs::Item_Struct* thejuice = (structs::Item_Struct*)returned_juice;

	 EQApplicationPacket* outapp = new EQApplicationPacket(OP_ItemPacket,sizeof(structs::Item_Struct));


      outapp->size=sizeof(structs::Item_Struct);
      structs::Item_Struct* thejuice = (structs::Item_Struct*) outapp->pBuffer;

  		if(type == 0)
  		{
  			if(inst->GetCharges() > 0)
  				thejuice->Charges = inst->GetCharges();
			else
  				thejuice->Charges = 1;
  
  			if((slot_id >= 251 && slot_id <= 330) || (slot_id >= 2031 && slot_id <= 2110))
  				thejuice->equipSlot = slot_id-1;
  			else
  				thejuice->equipSlot = slot_id;
  
  			thejuice->Price = inst->GetItem()->Price;
  		}
  		else
  		{
  			if(inst->GetMerchantCount() > 0)
  				thejuice->Charges = inst->GetMerchantCount();
  			else
  				thejuice->Charges = -1;
  
  			thejuice->equipSlot = inst->GetMerchantSlot();
  			thejuice->Price = inst->GetPrice();
  
  			_log(ZONE__INIT, "Shopkeeper item: %s in slot: %i set price to: %i and quantity to: %i", inst->GetItem()->Name, slot_id, thejuice->Price, thejuice->Charges); 
  		}
  		_log(ZONE__INIT, "Shopkeeper item: %s in slot: %i set price to: %i and quantity to: %i", inst->GetItem()->Name, slot_id, thejuice->Price, thejuice->Charges); 
  
  		thejuice->ItemClass = inst->GetItem()->ItemClass;
  		strcpy(thejuice->Name,inst->GetItem()->Name);
  		strcpy(thejuice->Lore,inst->GetItem()->Lore);       
  		strcpy(thejuice->IDFile,inst->GetItem()->IDFile);  	
  		thejuice->Weight = inst->GetItem()->Weight;      
  		thejuice->NoRent = inst->GetItem()->NoRent;         
  		thejuice->NoDrop = inst->GetItem()->NoDrop;         
  		thejuice->Size = inst->GetItem()->Size;           
  		thejuice->ID = inst->GetItem()->ID;        
  		thejuice->Icon = inst->GetItem()->Icon;       
  		thejuice->Slots = inst->GetItem()->Slots;          
  		thejuice->SellRate = inst->GetItem()->SellRate;
  		thejuice->CastTime = inst->GetItem()->CastTime;  
  		thejuice->SkillModType = inst->GetItem()->SkillModType;
  		thejuice->SkillModValue = inst->GetItem()->SkillModValue;
  		thejuice->BaneDmgRace = inst->GetItem()->BaneDmgRace;
  		thejuice->BaneDmgBody = inst->GetItem()->BaneDmgBody;
  		thejuice->BaneDmgAmt = inst->GetItem()->BaneDmgAmt;
  		thejuice->RecLevel = inst->GetItem()->RecLevel;       
  		thejuice->RecSkill = inst->GetItem()->RecSkill;   
  		thejuice->ElemDmgType = inst->GetItem()->ElemDmgType; 
  		thejuice->ElemDmgAmt = inst->GetItem()->ElemDmgAmt;
  		thejuice->ReqLevel = inst->GetItem()->ReqLevel; 
  		thejuice->FocusEffect = inst->GetItem()->Focus.Effect;
  
  		if(inst->GetItem()->ItemClass == 1)
  		{
  			thejuice->container.BagType = inst->GetItem()->BagType; 
  			thejuice->container.BagSlots = inst->GetItem()->BagSlots;         
  			thejuice->container.BagSize = inst->GetItem()->BagSize;    
  			thejuice->container.BagWR = inst->GetItem()->BagWR; 
  		}
  		else if(inst->GetItem()->ItemClass == 2)
  		{
  			strcpy(thejuice->book.Filename,inst->GetItem()->Filename);
  			thejuice->book.Book = inst->GetItem()->Book;         
  			thejuice->book.BookType = inst->GetItem()->BookType; 
  		}
  		else
  		{
  			thejuice->common.AStr = inst->GetItem()->AStr;           
  			thejuice->common.ASta = inst->GetItem()->ASta;           
  			thejuice->common.ACha = inst->GetItem()->ACha;           
  			thejuice->common.ADex = inst->GetItem()->ADex;           
  			thejuice->common.AInt = inst->GetItem()->AInt;           
  			thejuice->common.AAgi = inst->GetItem()->AAgi;           
  			thejuice->common.AWis = inst->GetItem()->AWis;           
  			thejuice->common.MR = inst->GetItem()->MR;             
  			thejuice->common.FR = inst->GetItem()->FR;             
  			thejuice->common.CR = inst->GetItem()->CR;             
  			thejuice->common.DR = inst->GetItem()->DR;             
  			thejuice->common.PR = inst->GetItem()->PR;             
  			thejuice->common.HP = inst->GetItem()->HP;             
  			thejuice->common.Mana = inst->GetItem()->Mana;           
  			thejuice->common.AC = inst->GetItem()->AC;		
  			thejuice->common.MaxCharges = inst->GetItem()->MaxCharges;    
  			//thejuice->common.GMFlag = inst->GetItem()->GMFlag;         
  			thejuice->common.Light = inst->GetItem()->Light;          
  			thejuice->common.Delay = inst->GetItem()->Delay;          
  			thejuice->common.Damage = inst->GetItem()->Damage;            
  			thejuice->common.Range = inst->GetItem()->Range;          
  			thejuice->common.ItemType = inst->GetItem()->ItemType;          
  			thejuice->common.Magic = inst->GetItem()->Magic;              
  			thejuice->common.Material = inst->GetItem()->Material;   
  			thejuice->common.Deity = inst->GetItem()->Deity; 
  			thejuice->common.Color = inst->GetItem()->Color;    
  			thejuice->common.Classes = inst->GetItem()->Classes;  
  			thejuice->common.Races = inst->GetItem()->Races;  
  			thejuice->common.Stackable = inst->GetItem()->Stackable;
  		}
  		
  		if(inst->GetItem()->Click.Effect > 0){
  			thejuice->common.Effect1 = inst->GetItem()->Click.Effect;
  			thejuice->Effect2 = inst->GetItem()->Click.Effect; 
  			thejuice->EffectType2 = inst->GetItem()->Click.Type;  
  			thejuice->common.EffectType1 = inst->GetItem()->Click.Type;
  			if(inst->GetItem()->Click.Level > 0)
  			{
  				thejuice->common.EffectLevel1 = inst->GetItem()->Click.Level; 
  				thejuice->EffectLevel2 = inst->GetItem()->Click.Level;
  			}
  			else
  			{
  				thejuice->common.EffectLevel1 = inst->GetItem()->Click.Level2; 
  				thejuice->EffectLevel2 = inst->GetItem()->Click.Level2;  
  			}
  		}
  		else if(inst->GetItem()->Scroll.Effect > 0){
  			thejuice->common.Effect1 = inst->GetItem()->Scroll.Effect;
  			thejuice->Effect2 = inst->GetItem()->Scroll.Effect; 
  			thejuice->EffectType2 = inst->GetItem()->Scroll.Type;  
  			thejuice->common.EffectType1 = inst->GetItem()->Scroll.Type;
  			if(inst->GetItem()->Scroll.Level > 0)
  			{
  				thejuice->common.EffectLevel1 = inst->GetItem()->Scroll.Level; 
  				thejuice->EffectLevel2 = inst->GetItem()->Scroll.Level;
  			}
  			else
  			{
  				thejuice->common.EffectLevel1 = inst->GetItem()->Scroll.Level2; 
  				thejuice->EffectLevel2 = inst->GetItem()->Scroll.Level2;  
  			}
  		}
  		else if(inst->GetItem()->Proc.Effect > 0){
  			thejuice->common.Effect1 = inst->GetItem()->Proc.Effect;
  			thejuice->Effect2 = inst->GetItem()->Proc.Effect; 
  			thejuice->EffectType2 = inst->GetItem()->Proc.Type;  
  			thejuice->common.EffectType1 = inst->GetItem()->Proc.Type;
  			if(inst->GetItem()->Proc.Level > 0)
  			{
  				thejuice->common.EffectLevel1 = inst->GetItem()->Proc.Level; 
  				thejuice->EffectLevel2 = inst->GetItem()->Proc.Level;
  			}
  			else
  			{
  				thejuice->common.EffectLevel1 = inst->GetItem()->Proc.Level2; 
  				thejuice->EffectLevel2 = inst->GetItem()->Proc.Level2;  
  			}
  		}

	//	_log(ZONE__INIT,"I weaseled you a %s it's in slot: %i with charges: %i", myitem->Name, myitem->equipSlot, myitem->Charges);
	return ;
}*/

} //end namespace Mac






