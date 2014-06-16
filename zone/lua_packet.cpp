#ifdef LUA_EQEMU

#include "lua.hpp"
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>

#include "masterentity.h"
#include "lua_packet.h"

struct Opcodes { };

Lua_Packet::Lua_Packet(int opcode, int size) {
	SetLuaPtrData(new EQApplicationPacket(static_cast<EmuOpcode>(opcode), size));
	owned_ = true;
}

Lua_Packet& Lua_Packet::operator=(const Lua_Packet& o) {
	if(o.owned_) {
		owned_ = true;
		EQApplicationPacket *app = reinterpret_cast<EQApplicationPacket*>(o.d_);
		if(app)
			d_ = new EQApplicationPacket(app->GetOpcode(), app->pBuffer, app->size);
		else
			d_ = nullptr;
	} else {
		owned_ = false;
		d_ = o.d_;
	}
	return *this;
}

Lua_Packet::Lua_Packet(const Lua_Packet& o) {
	if(o.owned_) {
		owned_ = true;
		EQApplicationPacket *app = reinterpret_cast<EQApplicationPacket*>(o.d_);
		if(app)
			d_ = new EQApplicationPacket(app->GetOpcode(), app->pBuffer, app->size);
		else
			d_ = nullptr;
	} else {
		owned_ = false;
		d_ = o.d_;
	}
}

int Lua_Packet::GetSize() {
	Lua_Safe_Call_Int();
	return static_cast<int>(self->size);
}

int Lua_Packet::GetOpcode() {
	Lua_Safe_Call_Int();
	return static_cast<int>(self->GetOpcode());
}

void Lua_Packet::SetOpcode(int op) {
	Lua_Safe_Call_Void();
	self->SetOpcode(static_cast<EmuOpcode>(op));
}

void Lua_Packet::WriteInt8(int offset, int value) {
	Lua_Safe_Call_Void();

	if(offset + sizeof(int8) <= self->size) {
		*reinterpret_cast<int8*>(self->pBuffer + offset) = value;
	}
}

void Lua_Packet::WriteInt16(int offset, int value) {
	Lua_Safe_Call_Void();

	if(offset + sizeof(int16) <= self->size) {
		*reinterpret_cast<int16*>(self->pBuffer + offset) = value;
	}
}

void Lua_Packet::WriteInt32(int offset, int value) {
	Lua_Safe_Call_Void();

	if(offset + sizeof(int32) <= self->size) {
		*reinterpret_cast<int32*>(self->pBuffer + offset) = value;
	}
}

void Lua_Packet::WriteInt64(int offset, int64 value) {
	Lua_Safe_Call_Void();

	if(offset + sizeof(int64) <= self->size) {
		*reinterpret_cast<int64*>(self->pBuffer + offset) = value;
	}
}

void Lua_Packet::WriteFloat(int offset, float value) {
	Lua_Safe_Call_Void();

	if(offset + sizeof(float) <= self->size) {
		*reinterpret_cast<float*>(self->pBuffer + offset) = value;
	}
}

void Lua_Packet::WriteDouble(int offset, double value) {
	Lua_Safe_Call_Void();

	if(offset + sizeof(double) <= self->size) {
		*reinterpret_cast<double*>(self->pBuffer + offset) = value;
	}
}

void Lua_Packet::WriteString(int offset, std::string value) {
	Lua_Safe_Call_Void();

	if(offset + value.length() + 1 <= self->size) {
		memcpy(self->pBuffer + offset, value.c_str(), value.length());
		*reinterpret_cast<int8*>(self->pBuffer + offset + value.length()) = 0;
	}
}

void Lua_Packet::WriteFixedLengthString(int offset, std::string value, int string_length) {
	Lua_Safe_Call_Void();

	if(offset + string_length <= static_cast<int>(self->size)) {
		memset(self->pBuffer + offset, 0, string_length);
		memcpy(self->pBuffer + offset, value.c_str(), value.length());
	}
}

int Lua_Packet::ReadInt8(int offset) {
	Lua_Safe_Call_Int();
	if(offset + sizeof(int8) <= self->size) {
		int8 v = *reinterpret_cast<int8*>(self->pBuffer + offset);
		return v;
	}

	return 0;
}

int Lua_Packet::ReadInt16(int offset) {
	Lua_Safe_Call_Int();

	if(offset + sizeof(int16) <= self->size) {
		int16 v = *reinterpret_cast<int16*>(self->pBuffer + offset);
		return v;
	}

	return 0;
}

int Lua_Packet::ReadInt32(int offset) {
	Lua_Safe_Call_Int();

	if(offset + sizeof(int32) <= self->size) {
		int32 v = *reinterpret_cast<int32*>(self->pBuffer + offset);
		return v;
	}

	return 0;
}

int64 Lua_Packet::ReadInt64(int offset) {
	Lua_Safe_Call_Int();

	if(offset + sizeof(int64) <= self->size) {
		int64 v = *reinterpret_cast<int64*>(self->pBuffer + offset);
		return v;
	}

	return 0;
}

float Lua_Packet::ReadFloat(int offset) {
	Lua_Safe_Call_Real();

	if(offset + sizeof(float) <= self->size) {
		float v = *reinterpret_cast<float*>(self->pBuffer + offset);
		return v;
	}

	return 0;
}

double Lua_Packet::ReadDouble(int offset) {
	Lua_Safe_Call_Real();

	if(offset + sizeof(double) <= self->size) {
		double v = *reinterpret_cast<double*>(self->pBuffer + offset);
		return v;
	}

	return 0;
}

std::string Lua_Packet::ReadString(int offset) {
	Lua_Safe_Call_String();

	if(offset < static_cast<int>(self->size)) {
		std::string ret;

		int i = offset;
		for(;;) {
			if(i >= static_cast<int>(self->size)) {
				break;
			}

			char c = *reinterpret_cast<char*>(self->pBuffer + i);

			if(c == '\0') {
				break;
			}

			ret.push_back(c);
			++i;
		}

		return ret;
	}

	return "";
}

std::string Lua_Packet::ReadFixedLengthString(int offset, int string_length) {
	Lua_Safe_Call_String();

	if(offset + string_length <= static_cast<int>(self->size)) {
		std::string ret;

		int i = offset;
		for(;;) {
			if(i >= offset + string_length) {
				break;
			}

			char c = *reinterpret_cast<char*>(self->pBuffer + i);
			ret.push_back(c);
			++i;
		}

		return ret;
	}

	return "";
}

luabind::scope lua_register_packet() {
	return luabind::class_<Lua_Packet>("Packet")
		.def(luabind::constructor<>())
		.def(luabind::constructor<int,int>())
		.property("null", &Lua_Packet::Null)
		.property("valid", &Lua_Packet::Valid)
		.def("GetSize", &Lua_Packet::GetSize)
		.def("GetOpcode", &Lua_Packet::GetOpcode)
		.def("SetOpcode", &Lua_Packet::SetOpcode)
		.def("WriteInt8", &Lua_Packet::WriteInt8)
		.def("WriteInt16", &Lua_Packet::WriteInt16)
		.def("WriteInt32", &Lua_Packet::WriteInt32)
		.def("WriteInt64", &Lua_Packet::WriteInt64)
		.def("WriteFloat", &Lua_Packet::WriteFloat)
		.def("WriteDouble", &Lua_Packet::WriteDouble)
		.def("WriteString", &Lua_Packet::WriteString)
		.def("WriteFixedLengthString", &Lua_Packet::WriteFixedLengthString)
		.def("ReadInt8", &Lua_Packet::ReadInt8)
		.def("ReadInt16", &Lua_Packet::ReadInt16)
		.def("ReadInt32", &Lua_Packet::ReadInt32)
		.def("ReadInt64", &Lua_Packet::ReadInt64)
		.def("ReadFloat", &Lua_Packet::ReadFloat)
		.def("ReadDouble", &Lua_Packet::ReadDouble)
		.def("ReadString", &Lua_Packet::ReadString)
		.def("ReadFixedLengthString", &Lua_Packet::ReadFixedLengthString);
}

luabind::scope lua_register_packet_opcodes() {
	return luabind::class_<Opcodes>("Opcode")
		.enum_("constants")
		[
			luabind::value("ExploreUnknown", static_cast<int>(OP_ExploreUnknown)),
			luabind::value("Heartbeat", static_cast<int>(OP_Heartbeat)),
			luabind::value("ReloadUI", static_cast<int>(OP_ReloadUI)),
			luabind::value("IncreaseStats", static_cast<int>(OP_IncreaseStats)),
			luabind::value("Dye", static_cast<int>(OP_Dye)),
			luabind::value("Stamina", static_cast<int>(OP_Stamina)),
			luabind::value("ControlBoat", static_cast<int>(OP_ControlBoat)),
			luabind::value("MobUpdate", static_cast<int>(OP_MobUpdate)),
			luabind::value("ClientUpdate", static_cast<int>(OP_ClientUpdate)),
			luabind::value("ChannelMessage", static_cast<int>(OP_ChannelMessage)),
			luabind::value("SimpleMessage", static_cast<int>(OP_SimpleMessage)),
			luabind::value("FormattedMessage", static_cast<int>(OP_FormattedMessage)),
			luabind::value("TGB", static_cast<int>(OP_TGB)),
			luabind::value("Bind_Wound", static_cast<int>(OP_Bind_Wound)),
			luabind::value("Charm", static_cast<int>(OP_Charm)),
			luabind::value("Begging", static_cast<int>(OP_Begging)),
			luabind::value("MoveCoin", static_cast<int>(OP_MoveCoin)),
			luabind::value("SpawnDoor", static_cast<int>(OP_SpawnDoor)),
			luabind::value("Sneak", static_cast<int>(OP_Sneak)),
			luabind::value("ExpUpdate", static_cast<int>(OP_ExpUpdate)),
			luabind::value("DumpName", static_cast<int>(OP_DumpName)),
			luabind::value("RespondAA", static_cast<int>(OP_RespondAA)),
			luabind::value("UpdateAA", static_cast<int>(OP_UpdateAA)),
			luabind::value("SendAAStats", static_cast<int>(OP_SendAAStats)),
			luabind::value("SendAATable", static_cast<int>(OP_SendAATable)),
			luabind::value("AAAction", static_cast<int>(OP_AAAction)),
			luabind::value("BoardBoat", static_cast<int>(OP_BoardBoat)),
			luabind::value("LeaveBoat", static_cast<int>(OP_LeaveBoat)),
			luabind::value("SendExpZonein", static_cast<int>(OP_SendExpZonein)),
			luabind::value("RaidUpdate", static_cast<int>(OP_RaidUpdate)),
			luabind::value("GuildLeader", static_cast<int>(OP_GuildLeader)),
			luabind::value("GuildPeace", static_cast<int>(OP_GuildPeace)),
			luabind::value("GuildRemove", static_cast<int>(OP_GuildRemove)),
			luabind::value("GuildMemberList", static_cast<int>(OP_GuildMemberList)),
			luabind::value("GuildMemberUpdate", static_cast<int>(OP_GuildMemberUpdate)),
			luabind::value("GuildMemberLevelUpdate", static_cast<int>(OP_GuildMemberLevelUpdate)),
			luabind::value("GuildInvite", static_cast<int>(OP_GuildInvite)),
			luabind::value("GuildMOTD", static_cast<int>(OP_GuildMOTD)),
			luabind::value("SetGuildMOTD", static_cast<int>(OP_SetGuildMOTD)),
			luabind::value("GuildPublicNote", static_cast<int>(OP_GuildPublicNote)),
			luabind::value("GetGuildsList", static_cast<int>(OP_GetGuildsList)),
			luabind::value("GuildDemote", static_cast<int>(OP_GuildDemote)),
			luabind::value("GuildInviteAccept", static_cast<int>(OP_GuildInviteAccept)),
			luabind::value("GuildWar", static_cast<int>(OP_GuildWar)),
			luabind::value("GuildDelete", static_cast<int>(OP_GuildDelete)),
			luabind::value("GuildManageRemove", static_cast<int>(OP_GuildManageRemove)),
			luabind::value("GuildManageAdd", static_cast<int>(OP_GuildManageAdd)),
			luabind::value("GuildManageStatus", static_cast<int>(OP_GuildManageStatus)),
			luabind::value("GuildManageBanker", static_cast<int>(OP_GuildManageBanker)),
			luabind::value("GetGuildMOTD", static_cast<int>(OP_GetGuildMOTD)),
			luabind::value("Trader", static_cast<int>(OP_Trader)),
			luabind::value("Bazaar", static_cast<int>(OP_Bazaar)),
			luabind::value("BecomeTrader", static_cast<int>(OP_BecomeTrader)),
			luabind::value("TraderItemUpdate", static_cast<int>(OP_TraderItemUpdate)),
			luabind::value("TraderShop", static_cast<int>(OP_TraderShop)),
			luabind::value("TraderBuy", static_cast<int>(OP_TraderBuy)),
			luabind::value("PetCommands", static_cast<int>(OP_PetCommands)),
			luabind::value("TradeSkillCombine", static_cast<int>(OP_TradeSkillCombine)),
			luabind::value("ItemName", static_cast<int>(OP_ItemName)),
			luabind::value("ShopItem", static_cast<int>(OP_ShopItem)),
			luabind::value("ShopPlayerBuy", static_cast<int>(OP_ShopPlayerBuy)),
			luabind::value("ShopPlayerSell", static_cast<int>(OP_ShopPlayerSell)),
			luabind::value("ShopDelItem", static_cast<int>(OP_ShopDelItem)),
			luabind::value("ShopRequest", static_cast<int>(OP_ShopRequest)),
			luabind::value("ShopEnd", static_cast<int>(OP_ShopEnd)),
			luabind::value("LFGCommand", static_cast<int>(OP_LFGCommand)),
			luabind::value("LFGAppearance", static_cast<int>(OP_LFGAppearance)),
			luabind::value("GroupUpdate", static_cast<int>(OP_GroupUpdate)),
			luabind::value("GroupInvite", static_cast<int>(OP_GroupInvite)),
			luabind::value("GroupDisband", static_cast<int>(OP_GroupDisband)),
			luabind::value("GroupInvite2", static_cast<int>(OP_GroupInvite2)),
			luabind::value("GroupFollow", static_cast<int>(OP_GroupFollow)),
			luabind::value("GroupFollow2", static_cast<int>(OP_GroupFollow2)),
			luabind::value("GroupCancelInvite", static_cast<int>(OP_GroupCancelInvite)),
			luabind::value("CustomTitles", static_cast<int>(OP_CustomTitles)),
			luabind::value("Split", static_cast<int>(OP_Split)),
			luabind::value("Jump", static_cast<int>(OP_Jump)),
			luabind::value("ConsiderCorpse", static_cast<int>(OP_ConsiderCorpse)),
			luabind::value("SkillUpdate", static_cast<int>(OP_SkillUpdate)),
			luabind::value("GMEndTrainingResponse", static_cast<int>(OP_GMEndTrainingResponse)),
			luabind::value("GMEndTraining", static_cast<int>(OP_GMEndTraining)),
			luabind::value("GMTrainSkill", static_cast<int>(OP_GMTrainSkill)),
			luabind::value("GMTraining", static_cast<int>(OP_GMTraining)),
			luabind::value("DeleteItem", static_cast<int>(OP_DeleteItem)),
			luabind::value("CombatAbility", static_cast<int>(OP_CombatAbility)),
			luabind::value("TrackUnknown", static_cast<int>(OP_TrackUnknown)),
			luabind::value("TrackTarget", static_cast<int>(OP_TrackTarget)),
			luabind::value("Track", static_cast<int>(OP_Track)),
			luabind::value("ItemLinkClick", static_cast<int>(OP_ItemLinkClick)),
			luabind::value("ItemLinkResponse", static_cast<int>(OP_ItemLinkResponse)),
			luabind::value("ItemLinkText", static_cast<int>(OP_ItemLinkText)),
			luabind::value("RezzAnswer", static_cast<int>(OP_RezzAnswer)),
			luabind::value("RezzComplete", static_cast<int>(OP_RezzComplete)),
			luabind::value("SendZonepoints", static_cast<int>(OP_SendZonepoints)),
			luabind::value("SetRunMode", static_cast<int>(OP_SetRunMode)),
			luabind::value("InspectRequest", static_cast<int>(OP_InspectRequest)),
			luabind::value("InspectAnswer", static_cast<int>(OP_InspectAnswer)),
			luabind::value("SenseTraps", static_cast<int>(OP_SenseTraps)),
			luabind::value("DisarmTraps", static_cast<int>(OP_DisarmTraps)),
			luabind::value("Assist", static_cast<int>(OP_Assist)),
			luabind::value("PickPocket", static_cast<int>(OP_PickPocket)),
			luabind::value("LootRequest", static_cast<int>(OP_LootRequest)),
			luabind::value("EndLootRequest", static_cast<int>(OP_EndLootRequest)),
			luabind::value("MoneyOnCorpse", static_cast<int>(OP_MoneyOnCorpse)),
			luabind::value("LootComplete", static_cast<int>(OP_LootComplete)),
			luabind::value("LootItem", static_cast<int>(OP_LootItem)),
			luabind::value("MoveItem", static_cast<int>(OP_MoveItem)),
			luabind::value("WhoAllRequest", static_cast<int>(OP_WhoAllRequest)),
			luabind::value("WhoAllResponse", static_cast<int>(OP_WhoAllResponse)),
			luabind::value("Consume", static_cast<int>(OP_Consume)),
			luabind::value("AutoAttack", static_cast<int>(OP_AutoAttack)),
			luabind::value("AutoAttack2", static_cast<int>(OP_AutoAttack2)),
			luabind::value("TargetMouse", static_cast<int>(OP_TargetMouse)),
			luabind::value("TargetCommand", static_cast<int>(OP_TargetCommand)),
			luabind::value("TargetReject", static_cast<int>(OP_TargetReject)),
			luabind::value("TargetHoTT", static_cast<int>(OP_TargetHoTT)),
			luabind::value("Hide", static_cast<int>(OP_Hide)),
			luabind::value("Forage", static_cast<int>(OP_Forage)),
			luabind::value("Fishing", static_cast<int>(OP_Fishing)),
			luabind::value("Bug", static_cast<int>(OP_Bug)),
			luabind::value("Emote", static_cast<int>(OP_Emote)),
			luabind::value("Consider", static_cast<int>(OP_Consider)),
			luabind::value("FaceChange", static_cast<int>(OP_FaceChange)),
			luabind::value("RandomReq", static_cast<int>(OP_RandomReq)),
			luabind::value("RandomReply", static_cast<int>(OP_RandomReply)),
			luabind::value("Camp", static_cast<int>(OP_Camp)),
			luabind::value("YellForHelp", static_cast<int>(OP_YellForHelp)),
			luabind::value("SafePoint", static_cast<int>(OP_SafePoint)),
			luabind::value("Buff", static_cast<int>(OP_Buff)),
			luabind::value("BuffFadeMsg", static_cast<int>(OP_BuffFadeMsg)),
			luabind::value("SpecialMesg", static_cast<int>(OP_SpecialMesg)),
			luabind::value("Consent", static_cast<int>(OP_Consent)),
			luabind::value("ConsentResponse", static_cast<int>(OP_ConsentResponse)),
			luabind::value("Stun", static_cast<int>(OP_Stun)),
			luabind::value("BeginCast", static_cast<int>(OP_BeginCast)),
			luabind::value("CastSpell", static_cast<int>(OP_CastSpell)),
			luabind::value("InterruptCast", static_cast<int>(OP_InterruptCast)),
			luabind::value("Death", static_cast<int>(OP_Death)),
			luabind::value("FeignDeath", static_cast<int>(OP_FeignDeath)),
			luabind::value("Illusion", static_cast<int>(OP_Illusion)),
			luabind::value("LevelUpdate", static_cast<int>(OP_LevelUpdate)),
			luabind::value("LevelAppearance", static_cast<int>(OP_LevelAppearance)),
			luabind::value("MemorizeSpell", static_cast<int>(OP_MemorizeSpell)),
			luabind::value("HPUpdate", static_cast<int>(OP_HPUpdate)),
			luabind::value("Mend", static_cast<int>(OP_Mend)),
			luabind::value("Taunt", static_cast<int>(OP_Taunt)),
			luabind::value("GMDelCorpse", static_cast<int>(OP_GMDelCorpse)),
			luabind::value("GMFind", static_cast<int>(OP_GMFind)),
			luabind::value("GMServers", static_cast<int>(OP_GMServers)),
			luabind::value("GMGoto", static_cast<int>(OP_GMGoto)),
			luabind::value("GMSummon", static_cast<int>(OP_GMSummon)),
			luabind::value("GMKill", static_cast<int>(OP_GMKill)),
			luabind::value("GMLastName", static_cast<int>(OP_GMLastName)),
			luabind::value("GMToggle", static_cast<int>(OP_GMToggle)),
			luabind::value("GMEmoteZone", static_cast<int>(OP_GMEmoteZone)),
			luabind::value("GMBecomeNPC", static_cast<int>(OP_GMBecomeNPC)),
			luabind::value("GMHideMe", static_cast<int>(OP_GMHideMe)),
			luabind::value("GMZoneRequest", static_cast<int>(OP_GMZoneRequest)),
			luabind::value("GMZoneRequest2", static_cast<int>(OP_GMZoneRequest2)),
			luabind::value("Petition", static_cast<int>(OP_Petition)),
			luabind::value("PetitionRefresh", static_cast<int>(OP_PetitionRefresh)),
			luabind::value("PDeletePetition", static_cast<int>(OP_PDeletePetition)),
			luabind::value("PetitionBug", static_cast<int>(OP_PetitionBug)),
			luabind::value("PetitionUpdate", static_cast<int>(OP_PetitionUpdate)),
			luabind::value("PetitionCheckout", static_cast<int>(OP_PetitionCheckout)),
			luabind::value("PetitionCheckout2", static_cast<int>(OP_PetitionCheckout2)),
			luabind::value("PetitionDelete", static_cast<int>(OP_PetitionDelete)),
			luabind::value("PetitionResolve", static_cast<int>(OP_PetitionResolve)),
			luabind::value("PetitionCheckIn", static_cast<int>(OP_PetitionCheckIn)),
			luabind::value("PetitionUnCheckout", static_cast<int>(OP_PetitionUnCheckout)),
			luabind::value("PetitionQue", static_cast<int>(OP_PetitionQue)),
			luabind::value("SetServerFilter", static_cast<int>(OP_SetServerFilter)),
			luabind::value("NewSpawn", static_cast<int>(OP_NewSpawn)),
			luabind::value("Animation", static_cast<int>(OP_Animation)),
			luabind::value("ZoneChange", static_cast<int>(OP_ZoneChange)),
			luabind::value("DeleteSpawn", static_cast<int>(OP_DeleteSpawn)),
			luabind::value("EnvDamage", static_cast<int>(OP_EnvDamage)),
			luabind::value("Action", static_cast<int>(OP_Action)),
			luabind::value("Damage", static_cast<int>(OP_Damage)),
			luabind::value("ManaChange", static_cast<int>(OP_ManaChange)),
			luabind::value("ClientError", static_cast<int>(OP_ClientError)),
			luabind::value("Save", static_cast<int>(OP_Save)),
			luabind::value("LocInfo", static_cast<int>(OP_LocInfo)),
			luabind::value("Surname", static_cast<int>(OP_Surname)),
			luabind::value("ClearSurname", static_cast<int>(OP_ClearSurname)),
			luabind::value("SwapSpell", static_cast<int>(OP_SwapSpell)),
			luabind::value("DeleteSpell", static_cast<int>(OP_DeleteSpell)),
			luabind::value("CloseContainer", static_cast<int>(OP_CloseContainer)),
			luabind::value("ClickObjectAction", static_cast<int>(OP_ClickObjectAction)),
			luabind::value("GroundSpawn", static_cast<int>(OP_GroundSpawn)),
			luabind::value("ClearObject", static_cast<int>(OP_ClearObject)),
			luabind::value("ZoneUnavail", static_cast<int>(OP_ZoneUnavail)),
			luabind::value("ItemPacket", static_cast<int>(OP_ItemPacket)),
			luabind::value("TradeRequest", static_cast<int>(OP_TradeRequest)),
			luabind::value("TradeRequestAck", static_cast<int>(OP_TradeRequestAck)),
			luabind::value("TradeAcceptClick", static_cast<int>(OP_TradeAcceptClick)),
			luabind::value("TradeMoneyUpdate", static_cast<int>(OP_TradeMoneyUpdate)),
			luabind::value("TradeCoins", static_cast<int>(OP_TradeCoins)),
			luabind::value("CancelTrade", static_cast<int>(OP_CancelTrade)),
			luabind::value("FinishTrade", static_cast<int>(OP_FinishTrade)),
			luabind::value("SaveOnZoneReq", static_cast<int>(OP_SaveOnZoneReq)),
			luabind::value("Logout", static_cast<int>(OP_Logout)),
			luabind::value("LogoutReply", static_cast<int>(OP_LogoutReply)),
			luabind::value("PreLogoutReply", static_cast<int>(OP_PreLogoutReply)),
			luabind::value("DuelResponse2", static_cast<int>(OP_DuelResponse2)),
			luabind::value("InstillDoubt", static_cast<int>(OP_InstillDoubt)),
			luabind::value("SafeFallSuccess", static_cast<int>(OP_SafeFallSuccess)),
			luabind::value("DisciplineUpdate", static_cast<int>(OP_DisciplineUpdate)),
			luabind::value("RecipesFavorite", static_cast<int>(OP_RecipesFavorite)),
			luabind::value("RecipesSearch", static_cast<int>(OP_RecipesSearch)),
			luabind::value("RecipeReply", static_cast<int>(OP_RecipeReply)),
			luabind::value("RecipeDetails", static_cast<int>(OP_RecipeDetails)),
			luabind::value("RecipeAutoCombine", static_cast<int>(OP_RecipeAutoCombine)),
			luabind::value("Shielding", static_cast<int>(OP_Shielding)),
			luabind::value("FindPersonRequest", static_cast<int>(OP_FindPersonRequest)),
			luabind::value("FindPersonReply", static_cast<int>(OP_FindPersonReply)),
			luabind::value("ZoneEntry", static_cast<int>(OP_ZoneEntry)),
			luabind::value("PlayerProfile", static_cast<int>(OP_PlayerProfile)),
			luabind::value("CharInventory", static_cast<int>(OP_CharInventory)),
			luabind::value("ZoneSpawns", static_cast<int>(OP_ZoneSpawns)),
			luabind::value("Weather", static_cast<int>(OP_Weather)),
			luabind::value("ReqNewZone", static_cast<int>(OP_ReqNewZone)),
			luabind::value("NewZone", static_cast<int>(OP_NewZone)),
			luabind::value("ReqClientSpawn", static_cast<int>(OP_ReqClientSpawn)),
			luabind::value("SpawnAppearance", static_cast<int>(OP_SpawnAppearance)),
			luabind::value("ClientReady", static_cast<int>(OP_ClientReady)),
			luabind::value("ZoneComplete", static_cast<int>(OP_ZoneComplete)),
			luabind::value("ApproveWorld", static_cast<int>(OP_ApproveWorld)),
			luabind::value("LogServer", static_cast<int>(OP_LogServer)),
			luabind::value("MOTD", static_cast<int>(OP_MOTD)),
			luabind::value("SendLoginInfo", static_cast<int>(OP_SendLoginInfo)),
			luabind::value("DeleteCharacter", static_cast<int>(OP_DeleteCharacter)),
			luabind::value("SendCharInfo", static_cast<int>(OP_SendCharInfo)),
			luabind::value("ExpansionInfo", static_cast<int>(OP_ExpansionInfo)),
			luabind::value("CharacterCreate", static_cast<int>(OP_CharacterCreate)),
			luabind::value("RandomNameGenerator", static_cast<int>(OP_RandomNameGenerator)),
			luabind::value("GuildsList", static_cast<int>(OP_GuildsList)),
			luabind::value("ApproveName", static_cast<int>(OP_ApproveName)),
			luabind::value("EnterWorld", static_cast<int>(OP_EnterWorld)),
			luabind::value("PostEnterWorld	", static_cast<int>(OP_PostEnterWorld	)),
			luabind::value("SendSystemStats", static_cast<int>(OP_SendSystemStats)),
			luabind::value("World_Client_CRC1", static_cast<int>(OP_World_Client_CRC1)),
			luabind::value("World_Client_CRC2", static_cast<int>(OP_World_Client_CRC2)),
			luabind::value("SetChatServer", static_cast<int>(OP_SetChatServer)),
			luabind::value("SetChatServer2", static_cast<int>(OP_SetChatServer2)),
			luabind::value("ZoneServerInfo", static_cast<int>(OP_ZoneServerInfo)),
			luabind::value("WorldClientReady", static_cast<int>(OP_WorldClientReady)),
			luabind::value("WorldUnknown001", static_cast<int>(OP_WorldUnknown001)),
			luabind::value("WearChange", static_cast<int>(OP_WearChange)),
			luabind::value("CrashDump", static_cast<int>(OP_CrashDump)),
			luabind::value("LoginComplete", static_cast<int>(OP_LoginComplete)),
			luabind::value("GMNameChange", static_cast<int>(OP_GMNameChange)),
			luabind::value("ReadBook", static_cast<int>(OP_ReadBook)),
			luabind::value("GMKick", static_cast<int>(OP_GMKick)),
			luabind::value("RezzRequest", static_cast<int>(OP_RezzRequest)),
			luabind::value("MultiLineMsg", static_cast<int>(OP_MultiLineMsg)),
			luabind::value("TimeOfDay", static_cast<int>(OP_TimeOfDay)),
			luabind::value("MoneyUpdate", static_cast<int>(OP_MoneyUpdate)),
			luabind::value("ClickObject", static_cast<int>(OP_ClickObject)),
			luabind::value("MoveDoor", static_cast<int>(OP_MoveDoor)),
			luabind::value("TraderDelItem", static_cast<int>(OP_TraderDelItem)),
			luabind::value("TestBuff", static_cast<int>(OP_TestBuff)),
			luabind::value("DuelResponse", static_cast<int>(OP_DuelResponse)),
			luabind::value("RequestDuel", static_cast<int>(OP_RequestDuel)),
			luabind::value("BazaarInspect", static_cast<int>(OP_BazaarInspect)),
			luabind::value("ClickDoor", static_cast<int>(OP_ClickDoor)),
			luabind::value("GroupAcknowledge", static_cast<int>(OP_GroupAcknowledge)),
			luabind::value("GroupDelete", static_cast<int>(OP_GroupDelete)),
			luabind::value("ShopEndConfirm", static_cast<int>(OP_ShopEndConfirm)),
			luabind::value("Sound", static_cast<int>(OP_Sound)),
			luabind::value("WorldComplete", static_cast<int>(OP_WorldComplete)),
			luabind::value("MobRename", static_cast<int>(OP_MobRename)),
			luabind::value("Some3ByteHPUpdate", static_cast<int>(OP_Some3ByteHPUpdate)),
			luabind::value("FloatListThing", static_cast<int>(OP_FloatListThing)),
			luabind::value("AAExpUpdate", static_cast<int>(OP_AAExpUpdate)),
			luabind::value("ForceFindPerson", static_cast<int>(OP_ForceFindPerson)),
			luabind::value("PlayMP3", static_cast<int>(OP_PlayMP3)),
			luabind::value("RequestClientZoneChange", static_cast<int>(OP_RequestClientZoneChange)),
			luabind::value("SomeItemPacketMaybe", static_cast<int>(OP_Projectile)),
			luabind::value("QueryResponseThing", static_cast<int>(OP_QueryResponseThing)),
			luabind::value("Some6ByteHPUpdate", static_cast<int>(OP_Some6ByteHPUpdate)),
			luabind::value("BecomeCorpse", static_cast<int>(OP_BecomeCorpse)),
			luabind::value("Action2", static_cast<int>(OP_Action2)),
			luabind::value("BazaarSearch", static_cast<int>(OP_BazaarSearch)),
			luabind::value("SetTitle", static_cast<int>(OP_SetTitle)),
			luabind::value("SetTitleReply", static_cast<int>(OP_SetTitleReply)),
			luabind::value("ConfirmDelete", static_cast<int>(OP_ConfirmDelete)),
			luabind::value("ConsentDeny", static_cast<int>(OP_ConsentDeny)),
			luabind::value("DeletePetition", static_cast<int>(OP_DeletePetition)),
			luabind::value("DenyResponse", static_cast<int>(OP_DenyResponse)),
			luabind::value("Disarm", static_cast<int>(OP_Disarm)),
			luabind::value("Feedback", static_cast<int>(OP_Feedback)),
			luabind::value("FriendsWho", static_cast<int>(OP_FriendsWho)),
			luabind::value("GMApproval", static_cast<int>(OP_GMApproval)),
			luabind::value("GMSearchCorpse", static_cast<int>(OP_GMSearchCorpse)),
			luabind::value("GuildBank", static_cast<int>(OP_GuildBank)),
			luabind::value("InitialHPUpdate", static_cast<int>(OP_InitialHPUpdate)),
			luabind::value("InitialMobHealth", static_cast<int>(OP_InitialMobHealth)),
			luabind::value("LFGGetMatchesRequest", static_cast<int>(OP_LFGGetMatchesRequest)),
			luabind::value("LFGGetMatchesResponse", static_cast<int>(OP_LFGGetMatchesResponse)),
			luabind::value("LFGResponse", static_cast<int>(OP_LFGResponse)),
			luabind::value("LFPCommand", static_cast<int>(OP_LFPCommand)),
			luabind::value("LFPGetMatchesRequest", static_cast<int>(OP_LFPGetMatchesRequest)),
			luabind::value("LFPGetMatchesResponse", static_cast<int>(OP_LFPGetMatchesResponse)),
			luabind::value("LoadSpellSet", static_cast<int>(OP_LoadSpellSet)),
			luabind::value("LockoutTimerInfo", static_cast<int>(OP_LockoutTimerInfo)),
			luabind::value("MendHPUpdate", static_cast<int>(OP_MendHPUpdate)),
			luabind::value("MobHealth", static_cast<int>(OP_MobHealth)),
			luabind::value("MoveLogDisregard", static_cast<int>(OP_MoveLogDisregard)),
			luabind::value("MoveLogRequest", static_cast<int>(OP_MoveLogRequest)),
			luabind::value("PetitionSearch", static_cast<int>(OP_PetitionSearch)),
			luabind::value("PetitionSearchResults", static_cast<int>(OP_PetitionSearchResults)),
			luabind::value("PetitionSearchText", static_cast<int>(OP_PetitionSearchText)),
			luabind::value("RaidInvite", static_cast<int>(OP_RaidInvite)),
			luabind::value("Report", static_cast<int>(OP_Report)),
			luabind::value("SenseHeading", static_cast<int>(OP_SenseHeading)),
			luabind::value("DynamicWall", static_cast<int>(OP_DynamicWall)),
			luabind::value("RequestTitles", static_cast<int>(OP_RequestTitles)),
			luabind::value("ZoneInAvatarSet", static_cast<int>(OP_ZoneInAvatarSet)),
			luabind::value("ZoneServerReady	", static_cast<int>(OP_ZoneServerReady	)),
			luabind::value("SendTitleList", static_cast<int>(OP_SendTitleList)),
			luabind::value("NewTitlesAvailable", static_cast<int>(OP_NewTitlesAvailable)),
			luabind::value("OpenDiscordMerchant", static_cast<int>(OP_OpenDiscordMerchant)),
			luabind::value("DiscordMerchantInventory", static_cast<int>(OP_DiscordMerchantInventory)),
			luabind::value("GiveMoney", static_cast<int>(OP_GiveMoney)),
			luabind::value("OnLevelMessage", static_cast<int>(OP_OnLevelMessage)),
			luabind::value("WeaponEquip1", static_cast<int>(OP_WeaponEquip1)),
			luabind::value("WeaponEquip2", static_cast<int>(OP_WeaponEquip2)),
			luabind::value("WeaponUnequip2", static_cast<int>(OP_WeaponUnequip2)),
			luabind::value("WorldLogout", static_cast<int>(OP_WorldLogout)),
			luabind::value("SessionReady", static_cast<int>(OP_SessionReady)),
			luabind::value("Login", static_cast<int>(OP_Login)),
			luabind::value("ServerListRequest", static_cast<int>(OP_ServerListRequest)),
			luabind::value("PlayEverquestRequest", static_cast<int>(OP_PlayEverquestRequest)),
			luabind::value("ChatMessage", static_cast<int>(OP_ChatMessage)),
			luabind::value("LoginAccepted", static_cast<int>(OP_LoginAccepted)),
			luabind::value("ServerListResponse", static_cast<int>(OP_ServerListResponse)),
			luabind::value("Poll", static_cast<int>(OP_Poll)),
			luabind::value("PlayEverquestResponse", static_cast<int>(OP_PlayEverquestResponse)),
			luabind::value("EnterChat", static_cast<int>(OP_EnterChat)),
			luabind::value("PollResponse", static_cast<int>(OP_PollResponse)),
			luabind::value("Command", static_cast<int>(OP_Command)),
			luabind::value("ZonePlayerToBind", static_cast<int>(OP_ZonePlayerToBind)),
			luabind::value("Rewind", static_cast<int>(OP_Rewind)),
			luabind::value("PetBuffWindow", static_cast<int>(OP_PetBuffWindow)),
			luabind::value("RaidJoin", static_cast<int>(OP_RaidJoin)),
			luabind::value("Translocate", static_cast<int>(OP_Translocate)),
			luabind::value("Sacrifice", static_cast<int>(OP_Sacrifice)),
			luabind::value("KeyRing", static_cast<int>(OP_KeyRing)),
			luabind::value("PopupResponse", static_cast<int>(OP_PopupResponse)),
			luabind::value("DeleteCharge", static_cast<int>(OP_DeleteCharge)),
			luabind::value("PotionBelt", static_cast<int>(OP_PotionBelt)),
			luabind::value("Barter", static_cast<int>(OP_Barter)),
			luabind::value("WorldObjectsSent", static_cast<int>(OP_WorldObjectsSent)),
			luabind::value("MarkNPC", static_cast<int>(OP_MarkNPC)),
			luabind::value("ClearNPCMarks", static_cast<int>(OP_ClearNPCMarks)),
			luabind::value("DelegateAbility", static_cast<int>(OP_DelegateAbility)),
			luabind::value("SetGroupTarget", static_cast<int>(OP_SetGroupTarget)),
			luabind::value("ApplyPoison", static_cast<int>(OP_ApplyPoison)),
			luabind::value("FinishWindow", static_cast<int>(OP_FinishWindow)),
			luabind::value("FinishWindow2", static_cast<int>(OP_FinishWindow2)),
			luabind::value("ItemVerifyRequest", static_cast<int>(OP_ItemVerifyRequest)),
			luabind::value("ItemVerifyReply", static_cast<int>(OP_ItemVerifyReply)),
			luabind::value("GMTrainSkillConfirm", static_cast<int>(OP_GMTrainSkillConfirm)),
			luabind::value("RestState", static_cast<int>(OP_RestState)),
			luabind::value("PVPStats", static_cast<int>(OP_PVPStats)),
			luabind::value("PVPLeaderBoardRequest", static_cast<int>(OP_PVPLeaderBoardRequest)),
			luabind::value("PVPLeaderBoardReply", static_cast<int>(OP_PVPLeaderBoardReply)),
			luabind::value("PVPLeaderBoardDetailsRequest", static_cast<int>(OP_PVPLeaderBoardDetailsRequest)),
			luabind::value("PVPLeaderBoardDetailsReply", static_cast<int>(OP_PVPLeaderBoardDetailsReply)),
			luabind::value("DisciplineTimer", static_cast<int>(OP_DisciplineTimer)),
			luabind::value("RespawnWindow", static_cast<int>(OP_RespawnWindow)),
			luabind::value("SetStartCity", static_cast<int>(OP_SetStartCity)),
			luabind::value("LoginUnknown1", static_cast<int>(OP_LoginUnknown1)),
			luabind::value("LoginUnknown2", static_cast<int>(OP_LoginUnknown2)),
			luabind::value("ItemViewUnknown", static_cast<int>(OP_ItemViewUnknown)),
			luabind::value("GetGuildMOTDReply", static_cast<int>(OP_GetGuildMOTDReply)),
			luabind::value("SpawnPositionUpdate", static_cast<int>(OP_SpawnPositionUpdate)),
			luabind::value("ManaUpdate", static_cast<int>(OP_ManaUpdate)),
			luabind::value("EnduranceUpdate", static_cast<int>(OP_EnduranceUpdate)),
			luabind::value("MobManaUpdate", static_cast<int>(OP_MobManaUpdate)),
			luabind::value("MobEnduranceUpdate", static_cast<int>(OP_MobEnduranceUpdate)),
			luabind::value("GroupUpdateB", static_cast<int>(OP_GroupUpdateB)),
			luabind::value("GroupDisbandYou", static_cast<int>(OP_GroupDisbandYou)),
			luabind::value("GroupDisbandOther", static_cast<int>(OP_GroupDisbandOther)),
			luabind::value("GroupLeaderChange", static_cast<int>(OP_GroupLeaderChange)),
			luabind::value("GroupRoles", static_cast<int>(OP_GroupRoles)),
			luabind::value("SendFindableNPCs", static_cast<int>(OP_SendFindableNPCs)),
			luabind::value("HideCorpse", static_cast<int>(OP_HideCorpse)),
			luabind::value("TargetBuffs", static_cast<int>(OP_TargetBuffs)),
			luabind::value("TradeBusy", static_cast<int>(OP_TradeBusy)),
			luabind::value("GuildUpdateURLAndChannel", static_cast<int>(OP_GuildUpdateURLAndChannel)),
			luabind::value("CameraEffect", static_cast<int>(OP_CameraEffect)),
			luabind::value("SpellEffect", static_cast<int>(OP_SpellEffect)),
			luabind::value("BuffCreate", static_cast<int>(OP_BuffCreate)),
			luabind::value("GuildStatus", static_cast<int>(OP_GuildStatus)),
			luabind::value("CorpseDrag", static_cast<int>(OP_CorpseDrag)),
			luabind::value("CorpseDrop", static_cast<int>(OP_CorpseDrop)),
			luabind::value("ChangeSize", static_cast<int>(OP_ChangeSize)),
			luabind::value("GroupMakeLeader", static_cast<int>(OP_GroupMakeLeader)),
			luabind::value("RemoveAllDoors", static_cast<int>(OP_RemoveAllDoors)),
//			luabind::value("RemoveNimbusEffect", static_cast<int>(OP_RemoveNimbusEffect)),
			luabind::value("GuildCreate", static_cast<int>(OP_GuildCreate)),
			luabind::value("Untargetable", static_cast<int>(OP_Untargetable)),
			luabind::value("LFGuild", static_cast<int>(OP_LFGuild)),
			luabind::value("Weblink", static_cast<int>(OP_Weblink)),
			luabind::value("InspectMessageUpdate", static_cast<int>(OP_InspectMessageUpdate)),
			luabind::value("OpenInventory", static_cast<int>(OP_OpenInventory)),
			luabind::value("OpenContainer", static_cast<int>(OP_OpenContainer))
		];
}

#endif
