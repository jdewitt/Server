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

structs::Item_Struct* WeaselTheJuice(const ItemInst *inst, int16 slot_id, int type = 0);

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
	OUT(deity);
	OUT(intoxication);
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
	OUT_array(skills, structs::MAX_PP_SKILL);  // 1:1 direct copy (100 dword)
	OUT(thirst_level);
	OUT(hunger_level);
	for(r = 0; r < 15; r++) {
		eq->buffs[r].visable = (emu->buffs[r].spellid == 0xFFFFFFFF || emu->buffs[r].spellid == 0) ? 0 : 2;
		OUT(buffs[r].level);
		OUT(buffs[r].bard_modifier);
		OUT(buffs[r].spellid);
		OUT(buffs[r].duration);
	}
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
	OUT(autosplit);
	eq->current_zone = emu->zone_id;
	for(r = 0; r < structs::MAX_PP_AA_ARRAY; r++) {
		OUT(aa_array[r].AA);
		OUT(aa_array[r].value);
	}
	for(r = 0; r < 6; r++) {
		OUT_str(groupMembers[r]);
	}
	//_log(NET__STRUCTS, "Player Profile Packet is %i bytes uncompressed", sizeof(structs::PlayerProfile_Struct));

	CRC32::SetEQChecksum(__packet->pBuffer, sizeof(structs::PlayerProfile_Struct)-4);
	EQApplicationPacket* outapp = new EQApplicationPacket();
	outapp->SetOpcode(OP_PlayerProfile);
	outapp->pBuffer = new uchar[10000];
	outapp->size = DeflatePacket((unsigned char*)__packet->pBuffer, sizeof(structs::PlayerProfile_Struct), outapp->pBuffer, 10000);
	EncryptProfilePacket(outapp->pBuffer, outapp->size);
	//_log(NET__STRUCTS, "Player Profile Packet is %i bytes compressed", outapp->size);
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

DECODE(OP_SetGuildMOTD) {
	SETUP_DIRECT_DECODE(GuildMOTD_Struct, structs::GuildMOTD_Struct);
	strcpy(emu->name,eq->name);
	strcpy(emu->motd,eq->motd);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_GuildMOTD) {
	SETUP_DIRECT_ENCODE(GuildMOTD_Struct, structs::GuildMOTD_Struct);
	strcpy(eq->name,emu->name);
	strcpy(eq->motd,emu->motd);
	FINISH_ENCODE();
}

DECODE(OP_GuildInviteAccept) {
	SETUP_DIRECT_DECODE(GuildInviteAccept_Struct, structs::GuildInviteAccept_Struct);
	strcpy(emu->inviter,eq->inviter);
	strcpy(emu->newmember,eq->newmember);
	IN(response);
	emu->guildeqid = (int16)eq->guildeqid;
	FINISH_DIRECT_DECODE();
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

DECODE(OP_Taunt)
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
	//_log(NET__STRUCTS, "Total size of bulkspawns packet STRUCT: %d", sizeof(structs::Spawn_Struct));

	//do the transform...
	int r;
	int k;
	for(r = 0; r < entrycount; r++, eq++, emu++) {
		eq->GM = emu->gm;
		eq->title = emu->aaitle;
		eq->anon = emu->anon;
		memcpy(eq->name, emu->name, 47);
		eq->deity = emu->deity;
		eq->size = emu->size;
		eq->NPC = emu->NPC;
		eq->invis = emu->invis;
		eq->cur_hp = emu->curHp;
		eq->deltaHeading = emu->deltaHeading;
		eq->x_pos = (int16)emu->x;
		eq->y_pos = (int16)emu->y;
		eq->animation = emu->animation;
		eq->z_pos = (int16)emu->z*10;
		eq->deltaY = 0;
		eq->deltaX = 0;
		eq->heading = (uint8)emu->heading;
		eq->deltaZ = 0;
		eq->anim_type = 0x64;
		eq->level = emu->level;
		eq->petOwnerId = emu->petOwnerId;
		eq->guildrank = emu->guildrank;
		if(emu->NPC == 1)
			eq->guildrank = 0;
		eq->texture = emu->equip_chest2;
		for(k = 0; k < 9; k++) {
			eq->equipment[k] = emu->equipment[k];
			eq->equipcolors[k].color = emu->colors[k].color;
		}
		eq->runspeed = emu->runspeed;
		eq->AFK = emu->afk;
		eq->GuildID = emu->guildID;
		if(eq->GuildID == 0)
			eq->GuildID = 0xFFFF;
		eq->helm = emu->helm;
		eq->race = emu->race;
		strncpy(eq->Surname, emu->lastName, 20);
		eq->walkspeed = emu->walkspeed;
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
		eq->gender = emu->gender;
		eq->bodytype = emu->bodytype;
		eq->spawn_id = emu->spawnId;

	}
	//_log(NET__STRUCTS, "Total size of bulkspawns packet uncompressed: %d", out->size);
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_ZoneSpawns, sizeof(structs::Spawn_Struct)*entrycount);
	outapp->pBuffer = new uchar[sizeof(structs::Spawn_Struct)*entrycount];
	outapp->size = DeflatePacket((unsigned char*)out->pBuffer, out->size, outapp->pBuffer, sizeof(structs::Spawn_Struct)*entrycount);
	EncryptZoneSpawnPacket(outapp->pBuffer, outapp->size);
	//_log(NET__STRUCTS, "Total size of bulkspawns packet compressed: %d", outapp->size);

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
		eq->GM = emu->gm;
		eq->anon = emu->anon;
		strncpy(eq->name, emu->name, 47);
		eq->deity = emu->deity;
		eq->size = emu->size;
		eq->NPC = emu->NPC;
		eq->invis = emu->invis;
		eq->cur_hp = emu->curHp;
		eq->deltaHeading = 0;
		eq->x_pos = (int16)emu->x;
		eq->y_pos = (int16)emu->y;
		eq->animation = emu->animation;
		eq->z_pos = (int16)emu->z*10;
		eq->deltaY = 0;
		eq->deltaX = 0;
		eq->heading = (uint8)emu->heading;
		eq->deltaZ = 0;
		eq->haircolor = emu->haircolor;
		eq->beardcolor = emu->beardcolor;
		eq->eyecolor1 = emu->eyecolor1;
		eq->eyecolor2 = emu->eyecolor2;
		eq->hairstyle = emu->hairstyle;
		eq->beard = emu->beard;
		eq->face = emu->face;
		eq->level = emu->level;
		eq->petOwnerId = emu->petOwnerId;
		eq->guildrank = emu->guildrank;
		for(k = 0; k < 9; k++) {
			eq->equipment[k] = emu->equipment[k];
			eq->equipcolors[k].color = emu->colors[k].color;
		}
		eq->runspeed = emu->runspeed;
		eq->AFK = emu->afk;
		eq->GuildID = emu->guildID;
		eq->title = emu->face;
		eq->anim_type = 0x64;
		eq->texture = emu->equip_chest2;
        eq->helm = emu->helm;
		eq->race = emu->race;
		eq->GuildID = emu->guildID;
		if(eq->GuildID == 0)
			eq->GuildID = 0xFFFF;
		if(eq->guildrank == 0)
			eq->guildrank = 0xFF;
		strncpy(eq->Surname, emu->lastName, 20);
		eq->walkspeed = emu->walkspeed;
		eq->light = emu->light;
		if(emu->class_ > 19 && emu->class_ < 35)
			eq->class_ = emu->class_-3;
		else if(emu->class_ == 40)
			eq->class_ = 16;
		else if(emu->class_ == 41)
			eq->class_ = 32;
		else 
			eq->class_ = emu->class_;
		eq->gender = emu->gender;
		eq->spawn_id = emu->spawnId;

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
	IN(zone_reason);
	IN(zoneID);
	IN(success);
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
	OUT(sequence);
	//OUT(unknown19);
	FINISH_ENCODE();
}

ENCODE(OP_Action2) { ENCODE_FORWARD(OP_Action); }
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

DECODE(OP_Damage) { DECODE_FORWARD(OP_EnvDamage); }
DECODE(OP_EnvDamage) {
	DECODE_LENGTH_EXACT(structs::EnvDamage2_Struct);
	SETUP_DIRECT_DECODE(EnvDamage2_Struct, structs::EnvDamage2_Struct);
	IN(id);
	IN(dmgtype);
	IN(damage);
	IN(constant);
	FINISH_DIRECT_DECODE();
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
		structs::Item_Struct* weasel = WeaselTheJuice((ItemInst*)int_struct->inst,int_struct->slot_id);

		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ItemPacket,sizeof(structs::Item_Struct));
		memcpy(outapp->pBuffer,weasel,sizeof(structs::Item_Struct));

		outapp->SetOpcode(OP_Unknown);
		
		if(old_item_pkt->PacketType == ItemPacketSummonItem || int_struct->slot_id == 30)
			outapp->SetOpcode(OP_SummonedItem);
		else if(old_item_pkt->PacketType == ItemPacketViewLink)
			outapp->SetOpcode(OP_ItemLinkResponse);
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

		if(outapp->size != sizeof(structs::Item_Struct))
			_log(ZONE__INIT,"Invalid size on OP_ItemPacket packet. Expected: %i, Got: %i", sizeof(structs::Item_Struct), outapp->size);

		dest->FastQueuePacket(&outapp);
		delete[] __emu_buffer;
	}
}

ENCODE(OP_TradeItemPacket){
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
		structs::Item_Struct* weasel = WeaselTheJuice((ItemInst*)int_struct->inst,int_struct->slot_id);

		EQApplicationPacket* outapp = new EQApplicationPacket(OP_TradeItemPacket,sizeof(structs::TradeItemsPacket_Struct));
		structs::TradeItemsPacket_Struct* myitem = (structs::TradeItemsPacket_Struct*) outapp->pBuffer;
		myitem->fromid = old_item_pkt->fromid;
		myitem->slotid = int_struct->slot_id;
		memcpy(&myitem->item,weasel,sizeof(structs::Item_Struct));
		
		if(outapp->size != sizeof(structs::TradeItemsPacket_Struct))
			_log(ZONE__INIT,"Invalid size on OP_TradeItemPacket packet. Expected: %i, Got: %i", sizeof(structs::TradeItemsPacket_Struct), outapp->size);

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

	int16 itemcount = in->size / sizeof(InternalSerializedItem_Struct);
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
	std::string weasel_string;
	int r;
	//std::string weasel_string;
	for(r = 0; r < itemcount; r++, eq++) 
	{
		structs::Item_Struct* weasel = WeaselTheJuice((ItemInst*)eq->inst,eq->slot_id);

		char *weasel_char = reinterpret_cast<char*>(weasel);
		weasel_string.append(weasel_char,sizeof(structs::Item_Struct));
		safe_delete_array(weasel_char);	
	}
	int32 length = 5000;
	int buffer = 2;

	memcpy(pi->packets,weasel_string.c_str(),weasel_string.length());
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_CharInventory, length);
	outapp->size = buffer + DeflatePacket((uchar*) pi->packets, itemcount * sizeof(structs::Item_Struct), &outapp->pBuffer[buffer], length-buffer);
	outapp->pBuffer[0] = itemcount;

	dest->FastQueuePacket(&outapp);
	delete[] __emu_buffer;
}

ENCODE(OP_ShopInventoryPacket)
{
	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;

	int16 itemcount = in->size / sizeof(InternalSerializedItem_Struct);
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
	std::string weasel_string;
	int r = 0;
	for(r = 0; r < itemcount; r++, eq++) 
	{
		structs::Item_Struct* weasel = WeaselTheJuice((ItemInst*)eq->inst,eq->slot_id,1);

		structs::MerchantItemsPacket_Struct* merchant = new struct structs::MerchantItemsPacket_Struct;
		memset(merchant,0,sizeof(structs::MerchantItemsPacket_Struct));
		memcpy(&merchant->item,weasel,sizeof(structs::Item_Struct));
		merchant->itemtype = weasel->ItemClass;

		char *weasel_char = reinterpret_cast<char*>(merchant);
		weasel_string.append(weasel_char,sizeof(structs::MerchantItemsPacket_Struct));
		safe_delete_array(weasel_char);	
	}
	int32 length = 5000;
	int buffer = 2;

	memcpy(pi->packets,weasel_string.c_str(),weasel_string.length());
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_ShopInventoryPacket, length);
	outapp->size = buffer + DeflatePacket((uchar*) pi->packets, itemcount * sizeof(structs::MerchantItemsPacket_Struct), &outapp->pBuffer[buffer], length-buffer);
	outapp->pBuffer[0] = itemcount;

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
	eq->unknown[0] = 0x71;
	eq->unknown[1] = 0x54;
	eq->unknown[2] = 0x00;
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
	if(emu->type == 0)
		eq->type=64;
	else
		OUT(type);
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

ENCODE(OP_Trader) {

	if((*p)->size == sizeof(Trader_Struct)) {
		ENCODE_LENGTH_EXACT(Trader_Struct);
		SETUP_DIRECT_ENCODE(Trader_Struct, structs::Trader_Struct);
		OUT(Code);
		int k;
		for(k = 0; k < 80; k++) {
			eq->Items[k] = emu->Items[k];
		}
		for(k = 0; k < 80; k++) {
			eq->ItemCost[k] = emu->ItemCost[k];
		}
		FINISH_ENCODE();
	}
	else if((*p)->size == sizeof(Trader_ShowItems_Struct)) {
		ENCODE_LENGTH_EXACT(Trader_ShowItems_Struct);
		SETUP_DIRECT_ENCODE(Trader_ShowItems_Struct, structs::Trader_ShowItems_Struct);
		OUT(Code);
		OUT(TraderID);
		if(emu->SubAction == 0)
			eq->SubAction = emu->Code;
		else
			OUT(SubAction);
		eq->Items[0] = emu->Unknown08[0];
		eq->Items[1] = emu->Unknown08[1];
		FINISH_ENCODE();
	}
	else if((*p)->size == sizeof(TraderPriceUpdate_Struct)) {
		ENCODE_LENGTH_EXACT(TraderPriceUpdate_Struct);
		SETUP_DIRECT_ENCODE(TraderPriceUpdate_Struct, structs::TraderPriceUpdate_Struct);
		OUT(Action);
		OUT(SubAction);
		OUT(SerialNumber);
		OUT(NewPrice);

		FINISH_ENCODE();
	}
	else if((*p)->size == sizeof(TraderBuy_Struct)) {
		ENCODE_LENGTH_EXACT(TraderBuy_Struct);
		SETUP_DIRECT_ENCODE(TraderBuy_Struct, structs::TraderBuy_Struct);
		OUT(Action);
		OUT(TraderID);
		OUT(ItemID);
		OUT(Price);
		OUT(Quantity);
		//OUT(Slot);
		strcpy(eq->ItemName,emu->ItemName);
		FINISH_ENCODE();
	}
}

DECODE(OP_Trader) {

	if(__packet->size == sizeof(structs::Trader_Struct)) {
		DECODE_LENGTH_EXACT(structs::Trader_Struct);
		SETUP_DIRECT_DECODE(Trader_Struct, structs::Trader_Struct);
		IN(Code);
		int k;
		for(k = 0; k < 80; k++) {
			emu->Items[k] = eq->Items[k];
		}
		for(k = 0; k < 80; k++) {
			emu->ItemCost[k] = eq->ItemCost[k];
		}
		FINISH_DIRECT_DECODE();
	}
	else if(__packet->size == sizeof(structs::TraderStatus_Struct)) {
		DECODE_LENGTH_EXACT(structs::TraderStatus_Struct);
		SETUP_DIRECT_DECODE(TraderStatus_Struct, structs::TraderStatus_Struct);
		IN(Code);
		IN(TraderID);
		FINISH_DIRECT_DECODE();
	}
	else if(__packet->size == sizeof(structs::TraderPriceUpdate_Struct)) {
		DECODE_LENGTH_EXACT(structs::TraderPriceUpdate_Struct);
		SETUP_DIRECT_DECODE(TraderPriceUpdate_Struct, structs::TraderPriceUpdate_Struct);
		IN(Action);
		IN(SubAction);
		IN(SerialNumber);
		IN(NewPrice);
		FINISH_DIRECT_DECODE();
	}
}

ENCODE(OP_BecomeTrader)
{
	ENCODE_LENGTH_EXACT(BecomeTrader_Struct);
	SETUP_DIRECT_ENCODE(BecomeTrader_Struct, structs::BecomeTrader_Struct);
	OUT(Code);
	OUT(ID);
	FINISH_ENCODE();
}

ENCODE(OP_TraderBuy)
{
	ENCODE_LENGTH_EXACT(TraderBuy_Struct);
	SETUP_DIRECT_ENCODE(TraderBuy_Struct, structs::TraderBuy_Struct);
	OUT(Action);
	OUT(TraderID);
	OUT(ItemID);
	OUT(Price);
	OUT(Quantity);
	eq->Slot = emu->AlreadySold;
	strcpy(eq->ItemName,emu->ItemName);
	FINISH_ENCODE();
}

DECODE(OP_TraderBuy){
	DECODE_LENGTH_EXACT(structs::TraderBuy_Struct);
	SETUP_DIRECT_DECODE(TraderBuy_Struct, structs::TraderBuy_Struct);
	IN(Action);
	IN(TraderID);
	IN(ItemID);
	IN(Price);
	IN(Quantity);
	emu->AlreadySold = eq->Slot;
	strcpy(emu->ItemName,eq->ItemName);
	FINISH_DIRECT_DECODE();
}

DECODE(OP_BazaarSearch){

	if(__packet->size == sizeof(structs::BazaarSearch_Struct)) {
		DECODE_LENGTH_EXACT(structs::BazaarSearch_Struct);
		SETUP_DIRECT_DECODE(BazaarSearch_Struct, structs::BazaarSearch_Struct);
		IN(Beginning.Action);
		IN(TraderID);
		IN(Class_);
		IN(Race);
		IN(ItemStat);
		IN(Slot);
		IN(Type);
		IN(MinPrice);
		IN(MaxPrice);
		strcpy(emu->Name,eq->Name);
		FINISH_DIRECT_DECODE();
	}
	else if(__packet->size == sizeof(structs::BazaarWelcome_Struct)) {
		DECODE_LENGTH_EXACT(structs::BazaarWelcome_Struct);
		SETUP_DIRECT_DECODE(BazaarWelcome_Struct, structs::BazaarWelcome_Struct);
		emu->Beginning.Action = eq->Action;
		IN(Traders);
		IN(Items);
		FINISH_DIRECT_DECODE();
	}
	else if(__packet->size == sizeof(BazaarInspect_Struct))
	{
		DECODE_LENGTH_EXACT(BazaarInspect_Struct);
	}
}

ENCODE(OP_WearChange) {
	ENCODE_LENGTH_EXACT(WearChange_Struct);
	SETUP_DIRECT_ENCODE(WearChange_Struct, structs::WearChange_Struct);
	OUT(spawn_id);
	OUT(material);
	eq->color = emu->color.color;
	eq->blue = emu->color.rgb.blue;
	eq->green = emu->color.rgb.green;
	eq->red = emu->color.rgb.red;
	eq->use_tint = emu->color.rgb.use_tint;
	OUT(wear_slot_id);
	FINISH_ENCODE();
}

DECODE(OP_WearChange) {
	DECODE_LENGTH_EXACT(structs::WearChange_Struct);
	SETUP_DIRECT_DECODE(WearChange_Struct, structs::WearChange_Struct);
	IN(spawn_id);
	IN(material);
	emu->color.color = eq->color;
	emu->color.rgb.blue = eq->blue;
	emu->color.rgb.green = eq->green;
	emu->color.rgb.red = eq->red;
	emu->color.rgb.use_tint = eq->use_tint;
	OUT(wear_slot_id);
	FINISH_DIRECT_DECODE();
}

ENCODE(OP_ExpUpdate) {
	ENCODE_LENGTH_EXACT(ExpUpdate_Struct);
	SETUP_DIRECT_ENCODE(ExpUpdate_Struct, structs::ExpUpdate_Struct);
	OUT(exp);
	FINISH_ENCODE();
}

ENCODE(OP_Death) {
	ENCODE_LENGTH_EXACT(Death_Struct);
	SETUP_DIRECT_ENCODE(Death_Struct, structs::Death_Struct);
	OUT(spawn_id);
	OUT(killer_id);
	OUT(corpseid);
	OUT(spell_id);
	OUT(attack_skill);
	OUT(damage);
	FINISH_ENCODE();
}

DECODE(OP_Bug) {
	DECODE_LENGTH_EXACT(structs::BugStruct);
	SETUP_DIRECT_DECODE(BugStruct, structs::BugStruct);
	strcpy(emu->chartype,eq->chartype);
	strn0cpy(emu->bug,eq->bug,sizeof(eq->bug));
	strcpy(emu->name,eq->name);
	strcpy(emu->target_name,eq->target_name);
	strcpy(emu->ui,"EQMac Client");
	IN(x);
	IN(y);
	IN(z);
	IN(heading);
	IN(type);
	FINISH_DIRECT_DECODE();
}

DECODE(OP_CombatAbility) {
	DECODE_LENGTH_EXACT(structs::CombatAbility_Struct);
	SETUP_DIRECT_DECODE(CombatAbility_Struct, structs::CombatAbility_Struct);
	IN(m_target);
	IN(m_atk);
	IN(m_skill);
	FINISH_DIRECT_DECODE();
}

structs::Item_Struct* WeaselTheJuice(const ItemInst *inst, int16 slot_id, int type) {
	const Item_Struct *item=inst->GetItem();

	structs::Item_Struct *thejuice = new struct structs::Item_Struct;
	memset(thejuice,0,sizeof(structs::Item_Struct));

  	if(type == 0)
  	{
  		if((slot_id >= 251 && slot_id <= 330) || (slot_id >= 2031 && slot_id <= 2110))
  			thejuice->equipSlot = slot_id-1;
  		else
  			thejuice->equipSlot = slot_id;

		thejuice->Charges = inst->GetCharges(); 
		thejuice->Price = item->Price;
  	}
  	else
  	{ 
  		if(inst->GetMerchantCount() > 1)
  			thejuice->Charges = inst->GetMerchantCount();
  		else
  			thejuice->Charges = 1;
  
  		thejuice->equipSlot = inst->GetMerchantSlot();
		thejuice->Price = inst->GetPrice();  
    }
  
		thejuice->ItemClass = item->ItemClass;
		strcpy(thejuice->Name,item->Name);
		strcpy(thejuice->Lore,item->Lore);       
		strcpy(thejuice->IDFile,item->IDFile);  
		thejuice->Weight = item->Weight;      
		thejuice->NoRent = item->NoRent;         
		thejuice->NoDrop = item->NoDrop;         
		thejuice->Size = item->Size;           
		thejuice->ID = item->ID;        
		thejuice->Icon = item->Icon;       
		thejuice->Slots = item->Slots;  
		thejuice->SellRate = item->SellRate;
		thejuice->CastTime = item->CastTime;  
		thejuice->SkillModType = item->SkillModType;
		thejuice->SkillModValue = item->SkillModValue;
		thejuice->BaneDmgRace = item->BaneDmgRace;
		thejuice->BaneDmgBody = item->BaneDmgBody;
		thejuice->BaneDmgAmt = item->BaneDmgAmt;
		thejuice->RecLevel = item->RecLevel;       
		thejuice->RecSkill = item->RecSkill;   
		thejuice->ProcRate = item->ProcRate; 
		thejuice->ElemDmgType = item->ElemDmgType; 
		thejuice->ElemDmgAmt = item->ElemDmgAmt;
		thejuice->ReqLevel = item->ReqLevel; 
		thejuice->BardType = item->BardType;
		thejuice->BardValue = item->BardValue;
		thejuice->FocusEffect = item->Focus.Effect;
	/*	thejuice->unknown0212=0x8a;
		thejuice->unknown0213=0x26;
		thejuice->unknown0216=0x01;
		thejuice->unknown0282=0xFF;
		thejuice->unknown0283=0xFF;*/

		if(item->ItemClass == 1){
			thejuice->container.BagType = item->BagType; 
			thejuice->container.BagSlots = item->BagSlots;         
			thejuice->container.BagSize = item->BagSize;    
			thejuice->container.BagWR = item->BagWR; 
		}
		else if(item->ItemClass == 2){
			strcpy(thejuice->book.Filename,item->Filename);
			thejuice->book.Book = item->Book;         
			thejuice->book.BookType = item->BookType; 
		}
		else
		{
		thejuice->common.AStr = item->AStr;           
		thejuice->common.ASta = item->ASta;           
		thejuice->common.ACha = item->ACha;           
		thejuice->common.ADex = item->ADex;           
		thejuice->common.AInt = item->AInt;           
		thejuice->common.AAgi = item->AAgi;           
		thejuice->common.AWis = item->AWis;           
		thejuice->common.MR = item->MR;             
		thejuice->common.FR = item->FR;             
		thejuice->common.CR = item->CR;             
		thejuice->common.DR = item->DR;             
		thejuice->common.PR = item->PR;             
		thejuice->common.HP = item->HP;             
		thejuice->common.Mana = item->Mana;           
		thejuice->common.AC = item->AC;		
		thejuice->common.MaxCharges = item->MaxCharges;    
		thejuice->common.Light = item->Light;          
		thejuice->common.Delay = item->Delay;          
		thejuice->common.Damage = item->Damage;               
		thejuice->common.Range = item->Range;          
		thejuice->common.ItemType = item->ItemType;          
		thejuice->common.Magic = item->Magic;          
		thejuice->common.Material = item->Material;   
		thejuice->common.Color = item->Color;    
		//thejuice->common.Faction = item->Faction;   
		thejuice->common.Classes = item->Classes;  
		thejuice->common.Races = item->Races;  
		thejuice->common.Stackable = item->Stackable_; 
		}
		if(item->Click.Effect > 0){
			thejuice->common.Effect1 = item->Click.Effect;
			thejuice->Effect2 = item->Click.Effect; 
			thejuice->EffectType2 = item->Click.Type;  
			thejuice->common.EffectType1 = item->Click.Type;
			if(item->Click.Level > 0)
			{
				thejuice->common.EffectLevel1 = item->Click.Level; 
				thejuice->EffectLevel2 = item->Click.Level;
			}
			else
			{
				thejuice->common.EffectLevel1 = item->Click.Level2; 
				thejuice->EffectLevel2 = item->Click.Level2;  
			}
		}
		else if(item->Scroll.Effect > 0){
			thejuice->common.Effect1 = item->Scroll.Effect;
			thejuice->Effect2 = item->Scroll.Effect; 
			thejuice->EffectType2 = item->Scroll.Type;  
			thejuice->common.EffectType1 = item->Scroll.Type;
			if(item->Scroll.Level > 0)
			{
				thejuice->common.EffectLevel1 = item->Scroll.Level; 
				thejuice->EffectLevel2 = item->Scroll.Level;
			}
			else
			{
				thejuice->common.EffectLevel1 = item->Scroll.Level2; 
				thejuice->EffectLevel2 = item->Scroll.Level2;  
			}
		}
		else if(item->Proc.Effect > 0){
			thejuice->common.Effect1 = item->Proc.Effect;
			thejuice->Effect2 = item->Proc.Effect; 
			if(item->Worn.Type > 0)
			{
				thejuice->EffectType2 = item->Worn.Type;  
				thejuice->common.EffectType1 = item->Worn.Type;
			}
			else
			{
				thejuice->EffectType2 = item->Proc.Type;  
				thejuice->common.EffectType1 = item->Proc.Type;
			}
			if(item->Proc.Level > 0)
			{
				thejuice->common.EffectLevel1 = item->Proc.Level; 
				thejuice->EffectLevel2 = item->Proc.Level;
			}
			else
			{
				thejuice->common.EffectLevel1 = item->Proc.Level2; 
				thejuice->EffectLevel2 = item->Proc.Level2;  
			}
		}

	return thejuice;
}

} //end namespace Mac






