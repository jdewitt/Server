/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2003 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "../common/debug.h"
#include "masterentity.h"
#include "worldserver.h"
#include "net.h"
#include "zonedb.h"
#include "../common/spdat.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "petitions.h"
#include "../common/serverinfo.h"
#include "../common/ZoneNumbers.h"
#include "../common/moremath.h"
#include "../common/guilds.h"
#include "../common/logsys.h"
#include "../common/StringUtil.h"
#include "StringIDs.h"
#include "NpcAI.h"
#include "QuestParserCollection.h"
extern WorldServer worldserver;

// @merth: this needs to be touched up
uint32 Client::NukeItem(uint32 itemnum, uint8 where_to_check) {
	if (itemnum == 0)
		return 0;
	uint32 x = 0;
	ItemInst *cur = nullptr;

	int i;
	if(where_to_check & invWhereWorn) {
		for (i=0; i<=21; i++) { // Equipped
			if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
				cur = m_inv.GetItem(i);
				if(cur && cur->GetItem()->Stackable) {
					x += cur->GetCharges();
				} else {
					x++;
				}

				DeleteItemInInventory(i, 0, true);
			}
		}

		// Power Source Slot
		if (GetItemIDAt(9999) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(9999) != INVALID_ID)) {
			cur = m_inv.GetItem(9999);
			if(cur && cur->GetItem()->Stackable) {
				x += cur->GetCharges();
			} else {
				x++;
			}
			DeleteItemInInventory(9999, 0, false);
		}
	}

	if(where_to_check & invWhereCursor) {
		if (GetItemIDAt(0) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(0) != INVALID_ID)) {
			cur = m_inv.GetItem(0);
			if(cur && cur->GetItem()->Stackable) {
				x += cur->GetCharges();
			} else {
				x++;
			}

			DeleteItemInInventory(0, 0, true);
		}

		for (i=330; i<=339; i++) { // cursor's containers
			if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
				cur = m_inv.GetItem(i);
				if(cur && cur->GetItem()->Stackable) {
					x += cur->GetCharges();
				} else {
					x++;
				}

				DeleteItemInInventory(i, 0, true);
			}
		}
	}

	if(where_to_check & invWherePersonal) {
		for (i=22; i<=29; i++) { // Equipped
			if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
				cur = m_inv.GetItem(i);
				if(cur && cur->GetItem()->Stackable) {
					x += cur->GetCharges();
				} else {
					x++;
				}

				DeleteItemInInventory(i, 0, true);
			}
		}

		for (i=250; i<=329; i++) { // Main inventory's containers
			if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
				cur = m_inv.GetItem(i);
				if(cur && cur->GetItem()->Stackable) {
					x += cur->GetCharges();
				} else {
					x++;
				}

				DeleteItemInInventory(i, 0, true);
			}
		}
	}

	if(where_to_check & invWhereBank) {
		for (i=2000; i<=2007; i++) { // Bank slots
			if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
				cur = m_inv.GetItem(i);
				if(cur && cur->GetItem()->Stackable) {
					x += cur->GetCharges();
				} else {
					x++;
				}

				DeleteItemInInventory(i, 0, true);
			}
		}

		for (i=2030; i<=2109; i++) { // Bank's containers
			if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
				cur = m_inv.GetItem(i);
				if(cur && cur->GetItem()->Stackable) {
					x += cur->GetCharges();
				} else {
					x++;
				}

				DeleteItemInInventory(i, 0, true);
			}
		}
	}

	return x;
}


bool Client::CheckLoreConflict(const Item_Struct* item) {
	if (!item)
		return false;
	if (!(item->LoreFlag))
		return false;

	if (item->LoreGroup == -1)	// Standard lore items; look everywhere except unused, return the result
		return (m_inv.HasItem(item->ID, 0, ~invWhereUnused) != SLOT_INVALID);

	//If the item has a lore group, we check for other items with the same group and return the result
	return (m_inv.HasItemByLoreGroup(item->LoreGroup, ~invWhereUnused) != SLOT_INVALID);
}

bool Client::SummonItem(uint32 item_id, int16 charges, uint32 aug1, uint32 aug2, uint32 aug3, uint32 aug4, uint32 aug5, bool attuned, uint16 to_slot) {
	// TODO: update calling methods and script apis to handle a failure return

	const Item_Struct* item = database.GetItem(item_id);

	// make sure the item exists
	if(item == nullptr) {
		Message(13, "Item %u does not exist.", item_id);
		mlog(INVENTORY__ERROR, "Player %s on account %s attempted to create an item with an invalid id.\n(Item: %u, Aug1: %u, Aug2: %u, Aug3: %u, Aug4: %u, Aug5: %u)\n",
			GetName(), account_name, item_id, aug1, aug2, aug3, aug4, aug5);

		return false;
	} 
	// check that there is not a lore conflict between base item and existing inventory
	else if(CheckLoreConflict(item)) {
		// DuplicateLoreMessage(item_id);
		Message(13, "You already have a lore %s (%i) in your inventory.", item->Name, item_id);

		return false;
	}

	// This code is ready to implement once the item load code is changed to process the 'minstatus' field.
	// Checking #iteminfo in-game verfies that item->MinStatus is set to '0' regardless of field value.
	// An optional sql script will also need to be added, once this goes live, to allow changing of the min status.

	// check to make sure we are a GM if the item is GM-only
	/*
	else if(item->MinStatus && ((this->Admin() < item->MinStatus) || (this->Admin() < RuleI(GM, MinStatusToSummonItem)))) {
		Message(13, "You are not a GM or do not have the status to summon this item.");
		mlog(INVENTORY__ERROR, "Player %s on account %s attempted to create a GM-only item with a status of %i.\n(Item: %u, Aug1: %u, Aug2: %u, Aug3: %u, Aug4: %u, Aug5: %u, MinStatus: %u)\n",
			GetName(), account_name, this->Admin(), item->ID, aug1, aug2, aug3, aug4, aug5, item->MinStatus);

		return false;
	}
	*/

	uint32 classes	= item->Classes;
	uint32 races	= item->Races;
	uint32 slots	= item->Slots;

	// validation passed..so, set the charges and create the actual item

	// if the item is stackable and the charge amount is -1 or 0 then set to 1 charge.
	// removed && item->MaxCharges == 0 if -1 or 0 was passed max charges is irrelevant 
	if(charges < 0)
		charges = 1;

	else if (charges == 0){
		//Item does not have charges and is not stackable (Normal item.)
		if (item->MaxCharges < 1 && (item->StackSize < 1 || !item->Stackable)) 
		{ 
			charges = 1;
		}
		//Item is not stackable, but has to use charges.
		else if(item->StackSize < 1 || !item->Stackable) 
		{
			charges = item->MaxCharges;
		}
		//Due to the previous checks, item has to stack.
		else
		{
			charges = item->StackSize;
		}
	}	
	// in any other situation just use charges as passed

	ItemInst* inst = database.CreateItem(item, charges);

	if(inst == nullptr) {
		Message(13, "An unknown server error has occurred and your item was not created.");
		// this goes to logfile since this is a major error
		LogFile->write(EQEMuLog::Error, "Player %s on account %s encountered an unknown item creation error.\n(Item: %u, Aug1: %u, Aug2: %u, Aug3: %u, Aug4: %u, Aug5: %u)\n",
			GetName(), account_name, item->ID, aug1, aug2, aug3, aug4, aug5);

		return false;
	}

	// attune item
	if(attuned && inst->GetItem()->Attuneable)
		inst->SetInstNoDrop(true);

	// check to see if item is usable in requested slot
	if(to_slot != SLOT_QUEST && (((to_slot >= 1) && (to_slot <= 21)) || (to_slot == 9999))) {
		uint32 slottest = (to_slot == 9999) ? 22 : to_slot;

		if(!(slots & ((uint32)1 << slottest))) {
			Message(0, "This item is not equipable at slot %u - moving to cursor.", to_slot);
			mlog(INVENTORY__ERROR, "Player %s on account %s attempted to equip an item unusable in slot %u - moved to cursor.\n(Item: %u, Aug1: %u, Aug2: %u, Aug3: %u, Aug4: %u, Aug5: %u)\n",
				GetName(), account_name, to_slot, item->ID, aug1, aug2, aug3, aug4, aug5);

			to_slot = SLOT_CURSOR;
		}
	}

	//We're coming from a quest method.
	if(to_slot == SLOT_QUEST)
	{
		bool stacking = TryStacking(inst);
		//If we were able to stack, there is no need to continue on as we're set.
		if(stacking)
		{
			safe_delete(inst);
			return true;
		}
		else
		{
			bool bag = false;
			if(inst->IsType(ItemClassContainer))
			{
				bag = true;
			}
			to_slot = m_inv.FindFreeSlot(bag, true, item->Size);

			//make sure we are not completely full...
			if(to_slot == SLOT_CURSOR || to_slot == SLOT_INVALID) {
				if(m_inv.GetItem(SLOT_CURSOR) != nullptr || to_slot == SLOT_INVALID) {
					Message(13,"You have no more room. The item falls to the ground."); 
					//This crashes the Intel client. But, we still need to put the item somewhere.
					DropInst(inst);
				}
			}
		}
	}

	if(to_slot == SLOT_CURSOR) {
		PushItemOnCursor(*inst);
		SendItemPacket(to_slot, inst, ItemPacketSummonItem);
	}
	else
		PutItemInInventory(to_slot, *inst, true);

	safe_delete(inst);

	// discover item
	if((RuleB(Character, EnableDiscoveredItems)) && !GetGM()) {
		if(!IsDiscovered(item_id))
			DiscoverItem(item_id);
	}

	return true;
}

// Drop item from inventory to ground (generally only dropped from SLOT_CURSOR)
void Client::DropItem(int16 slot_id)
{
	if(GetInv().CheckNoDrop(slot_id) && RuleI(World, FVNoDropFlag) == 0 ||
		RuleI(Character, MinStatusForNoDropExemptions) < Admin() && RuleI(World, FVNoDropFlag) == 2) {
		database.SetHackerFlag(this->AccountName(), this->GetCleanName(), "Tried to drop an item on the ground that was nodrop!");
		GetInv().DeleteItem(slot_id);
		return;
	}

	// Take control of item in client inventory
	ItemInst *inst = m_inv.PopItem(slot_id);
	if(inst) {
		int i = parse->EventItem(EVENT_DROP_ITEM, this, inst, nullptr, "", 0);
		if(i != 0) {
			safe_delete(inst);
		}
	} else {
		// Item doesn't exist in inventory!
		Message(13, "Error: Item not found in slot %i", slot_id);
		return;
	}

	// Save client inventory change to database
	if(slot_id == SLOT_CURSOR) {
		std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
		database.SaveCursor(CharacterID(), s, e);
	} else {
		database.SaveInventory(CharacterID(), nullptr, slot_id);
	}

	if(!inst)
		return;

	// Package as zone object
	Object* object = new Object(this, inst);
	entity_list.AddObject(object, true);
	object->StartDecay();

	safe_delete(inst);
}

// Drop inst
void Client::DropInst(const ItemInst* inst)
{
	if (!inst) {
		// Item doesn't exist in inventory!
		Message(13, "Error: Item not found");
		return;
	}


	if (inst->GetItem()->NoDrop == 0)
	{
		Message(13, "This item is NODROP. Deleting.");
		return;
	}

	// Package as zone object
	Object* object = new Object(this, inst);
	entity_list.AddObject(object, true);
	object->StartDecay();
}

// Returns a slot's item ID (returns INVALID_ID if not found)
uint32 Client::GetItemIDAt(int16 slot_id) {
	const ItemInst* inst = m_inv[slot_id];
	if (inst)
		return inst->GetItem()->ID;

	// None found
	return INVALID_ID;
}

// Remove item from inventory
void Client::DeleteItemInInventory(int16 slot_id, int8 quantity, bool client_update, bool update_db) {
	#if (EQDEBUG >= 5)
		LogFile->write(EQEMuLog::Debug, "DeleteItemInInventory(%i, %i, %s)", slot_id, quantity, (client_update) ? "true":"false");
	#endif

	// Added 'IsSlotValid(slot_id)' check to both segments of client packet processing.
	// - cursor queue slots were slipping through and crashing client
	if(!m_inv[slot_id]) {
		// Make sure the client deletes anything in this slot to match the server.
		if(client_update && IsValidSlot(slot_id)) {
			EQApplicationPacket* outapp;
			outapp = new EQApplicationPacket(OP_MoveItem, sizeof(MoveItem_Struct));
			MoveItem_Struct* delitem	= (MoveItem_Struct*)outapp->pBuffer;
			delitem->from_slot			= slot_id;
			delitem->to_slot			= 0xFFFFFFFF;
			delitem->number_in_stack	= 0xFFFFFFFF;
			QueuePacket(outapp);
			safe_delete(outapp);
			
		}
		return;
	}

	// start QS code
	if(RuleB(QueryServ, PlayerLogDeletes)) {
		uint16 delete_count = 0;

		if(m_inv[slot_id]) { delete_count += m_inv.GetItem(slot_id)->GetTotalItemCount(); }

		ServerPacket* qspack = new ServerPacket(ServerOP_QSPlayerLogDeletes, sizeof(QSPlayerLogDelete_Struct) + (sizeof(QSDeleteItems_Struct) * delete_count));
		QSPlayerLogDelete_Struct* qsaudit = (QSPlayerLogDelete_Struct*)qspack->pBuffer;
		uint16 parent_offset = 0;

		qsaudit->char_id	= character_id;
		qsaudit->stack_size = quantity;
		qsaudit->char_count = delete_count;

		qsaudit->items[parent_offset].char_slot = slot_id;
		qsaudit->items[parent_offset].item_id	= m_inv[slot_id]->GetID();
		qsaudit->items[parent_offset].charges	= m_inv[slot_id]->GetCharges();

		if(m_inv[slot_id]->IsType(ItemClassContainer)) {
			for(uint8 bag_idx = 0; bag_idx < m_inv[slot_id]->GetItem()->BagSlots; bag_idx++) {
				ItemInst* bagitem = m_inv[slot_id]->GetItem(bag_idx);

				if(bagitem) {
					int16 bagslot_id = Inventory::CalcSlotId(slot_id, bag_idx);

					qsaudit->items[++parent_offset].char_slot	= bagslot_id;
					qsaudit->items[parent_offset].item_id		= bagitem->GetID();
					qsaudit->items[parent_offset].charges		= bagitem->GetCharges();
				}
			}
		}

		qspack->Deflate();
		if(worldserver.Connected()) { worldserver.SendPacket(qspack); }
		safe_delete(qspack);
	}
	// end QS code

	bool isDeleted = m_inv.DeleteItem(slot_id, quantity);

	const ItemInst* inst=nullptr;
	if (slot_id==SLOT_CURSOR) {
		std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
		if(update_db)
			database.SaveCursor(character_id, s, e);
	}
	else {
		// Save change to database
		inst = m_inv[slot_id];
		if(update_db)
			database.SaveInventory(character_id, inst, slot_id);
	}

	if(client_update && IsValidSlot(slot_id)) {
		EQApplicationPacket* outapp;
		if(inst) {
			if((!inst->IsStackable() && !isDeleted) || (GetClientVersion() == EQClientMac && !isDeleted)){
				// Non stackable item with charges = Item with clicky spell effect ? Delete a charge.
				outapp = new EQApplicationPacket(OP_DeleteCharge, sizeof(MoveItem_Struct));
				MoveItem_Struct* delitem = (MoveItem_Struct*)outapp->pBuffer;
				delitem->from_slot			= slot_id;
				delitem->to_slot			= 0xFFFFFFFF;
				delitem->number_in_stack	= 0xFFFFFFFF;
				QueuePacket(outapp);
				safe_delete(outapp);
				
			}
			else {
				outapp = new EQApplicationPacket(OP_MoveItem, sizeof(MoveItem_Struct));
				MoveItem_Struct* delitem	= (MoveItem_Struct*)outapp->pBuffer;
				delitem->from_slot			= slot_id;
				delitem->to_slot			= 0xFFFFFFFF;
				delitem->number_in_stack	= 0xFFFFFFFF;
				QueuePacket(outapp);
				safe_delete(outapp);
			}
				
		}
		else {
			outapp = new EQApplicationPacket(OP_MoveItem, sizeof(MoveItem_Struct));
			MoveItem_Struct* delitem	= (MoveItem_Struct*)outapp->pBuffer;
			delitem->from_slot			= slot_id;
			delitem->to_slot			= 0xFFFFFFFF;
			delitem->number_in_stack	= 0xFFFFFFFF;
			QueuePacket(outapp);
			safe_delete(outapp);
			
		}
	}
}

// Puts an item into the person's inventory
// Any items already there will be removed from user's inventory
// (Also saves changes back to the database: this may be optimized in the future)
// client_update: Sends packet to client
bool Client::PushItemOnCursor(const ItemInst& inst, bool client_update)
{
	mlog(INVENTORY__SLOTS, "Putting item %s (%d) on the cursor", inst.GetItem()->Name, inst.GetItem()->ID);
	m_inv.PushCursor(inst);

	if (client_update) {
		SendItemPacket(SLOT_CURSOR, &inst, ItemPacketSummonItem);
	}

	std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
	return database.SaveCursor(CharacterID(), s, e);
}

bool Client::PutItemInInventory(int16 slot_id, const ItemInst& inst, bool client_update)
{
	mlog(INVENTORY__SLOTS, "Putting item %s (%d) into slot %d", inst.GetItem()->Name, inst.GetItem()->ID, slot_id);
	if (slot_id==SLOT_CURSOR)
	{
		return PushItemOnCursor(inst,client_update);
	}
	else
		m_inv.PutItem(slot_id, inst);

	if (client_update) {
		SendItemPacket(slot_id, &inst, (slot_id==SLOT_CURSOR)?ItemPacketSummonItem:ItemPacketTrade);
	}

	if (slot_id==SLOT_CURSOR) {
		std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
		return database.SaveCursor(this->CharacterID(), s, e);
	} else
		return database.SaveInventory(this->CharacterID(), &inst, slot_id);

	CalcBonuses();
}

void Client::PutLootInInventory(int16 slot_id, const ItemInst &inst, ServerLootItem_Struct** bag_item_data)
{
	mlog(INVENTORY__SLOTS, "Putting loot item %s (%d) into slot %d", inst.GetItem()->Name, inst.GetItem()->ID, slot_id);
	m_inv.PutItem(slot_id, inst);

	SendLootItemInPacket(&inst, slot_id);

	if (slot_id==SLOT_CURSOR) {
		std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
		database.SaveCursor(this->CharacterID(), s, e);
	} else
		database.SaveInventory(this->CharacterID(), &inst, slot_id);

	if(bag_item_data)	// bag contents
	{
		int16 interior_slot;
		// solar: our bag went into slot_id, now let's pack the contents in
		for(int i = 0; i < 10; i++)
		{
			if(bag_item_data[i] == nullptr)
				continue;
			const ItemInst *bagitem = database.CreateItem(bag_item_data[i]->item_id, bag_item_data[i]->charges);
			interior_slot = Inventory::CalcSlotId(slot_id, i);
			mlog(INVENTORY__SLOTS, "Putting bag loot item %s (%d) into slot %d (bag slot %d)", inst.GetItem()->Name, inst.GetItem()->ID, interior_slot, i);
			PutLootInInventory(interior_slot, *bagitem);
			safe_delete(bagitem);
		}
	}

	CalcBonuses();
}
bool Client::TryStacking(ItemInst* item, uint8 type, bool try_worn, bool try_cursor){
	if(!item || !item->IsStackable())
		return false;
	int16 i;
	uint32 item_id = item->GetItem()->ID;
	for (i = 22; i <= 29; i++)
	{
		ItemInst* tmp_inst = m_inv.GetItem(i);
		if(tmp_inst && tmp_inst->GetItem()->ID == item_id && tmp_inst->GetCharges() < tmp_inst->GetItem()->StackSize){
			MoveItemCharges(*item, i, type);
			CalcBonuses();
			if(item->GetCharges())	// we didn't get them all
				return AutoPutLootInInventory(*item, try_worn, try_cursor, 0);
			return true;
		}
	}
	for (i = 22; i <= 29; i++)
	{
		for (uint8 j = 0; j < 10; j++)
		{
			uint16 slotid = Inventory::CalcSlotId(i, j);
			ItemInst* tmp_inst = m_inv.GetItem(slotid);

			if(tmp_inst && tmp_inst->GetItem()->ID == item_id && tmp_inst->GetCharges() < tmp_inst->GetItem()->StackSize){
				MoveItemCharges(*item, slotid, type);
				CalcBonuses();
				if(item->GetCharges())	// we didn't get them all
					return AutoPutLootInInventory(*item, try_worn, try_cursor, 0);
				return true;
			}
		}
	}
	return false;
}
// Locate an available space in inventory to place an item
// and then put the item there
// The change will be saved to the database
bool Client::AutoPutLootInInventory(ItemInst& inst, bool try_worn, bool try_cursor, ServerLootItem_Struct** bag_item_data)
{
	// #1: Try to auto equip
	if (try_worn && inst.IsEquipable(GetBaseRace(), GetClass()) && inst.GetItem()->ReqLevel<=level && !inst.GetItem()->Attuneable)
	{
		for (int16 i = 0; i < 9999; i++) // originally (i < 22)
		{
			if (i == 22) {
				break;
			}

			if (!m_inv[i])
			{
				if( i == SLOT_PRIMARY && inst.IsWeapon() ) // If item is primary slot weapon
				{
					if( (inst.GetItem()->ItemType == ItemType2HSlash) || (inst.GetItem()->ItemType == ItemType2HBlunt) || (inst.GetItem()->ItemType == ItemType2HPiercing) ) // and uses 2hs \ 2hb \ 2hp
					{
						if( m_inv[SLOT_SECONDARY] ) // and if secondary slot is not empty
						{
							continue; // Can't auto-equip
						}
					}
				}
				if( i== SLOT_SECONDARY && m_inv[SLOT_PRIMARY]) // check to see if primary slot is a two hander
				{
					uint8 use = m_inv[SLOT_PRIMARY]->GetItem()->ItemType;
					if(use == ItemType2HSlash || use == ItemType2HBlunt || use == ItemType2HPiercing)
						continue;
				}
				if
				(
					i == SLOT_SECONDARY &&
					inst.IsWeapon() &&
					!CanThisClassDualWield()
				)
				{
					continue;
				}

				if (inst.IsEquipable(i))	// Equippable at this slot?
				{
					//send worn to everyone...
					PutLootInInventory(i, inst);
					uint8 worn_slot_material = Inventory::CalcMaterialFromSlot(i);
					if(worn_slot_material != 0xFF)
					{
						SendWearChange(worn_slot_material);
					}
					
					parse->EventItem(EVENT_EQUIP_ITEM, this, &inst, nullptr, "", i);
					return true;
				}
			}
		}
	}

	// #2: Stackable item?
	if (inst.IsStackable())
	{
		if(TryStacking(&inst, ItemPacketTrade, try_worn, try_cursor))
			return true;
	}

	// #3: put it in inventory
	bool is_arrow = (inst.GetItem()->ItemType == ItemTypeArrow) ? true : false;
	int16 slot_id = m_inv.FindFreeSlot(inst.IsType(ItemClassContainer), try_cursor, inst.GetItem()->Size, is_arrow);
	if (slot_id != SLOT_INVALID)
	{
		PutLootInInventory(slot_id, inst, bag_item_data);
		return true;
	}

	return false;
}

// solar: helper function for AutoPutLootInInventory
void Client::MoveItemCharges(ItemInst &from, int16 to_slot, uint8 type)
{
	ItemInst *tmp_inst = m_inv.GetItem(to_slot);

	if(tmp_inst && tmp_inst->GetCharges() < tmp_inst->GetItem()->StackSize)
	{
		// this is how much room is left on the item we're stacking onto
		int charge_slots_left = tmp_inst->GetItem()->StackSize - tmp_inst->GetCharges();
		// this is how many charges we can move from the looted item to
		// the item in the inventory
		int charges_to_move =
			from.GetCharges() < charge_slots_left ?
				from.GetCharges() :
				charge_slots_left;

		tmp_inst->SetCharges(tmp_inst->GetCharges() + charges_to_move);
		from.SetCharges(from.GetCharges() - charges_to_move);
		SendLootItemInPacket(tmp_inst, to_slot);
		if (to_slot==SLOT_CURSOR){
			std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
			database.SaveCursor(this->CharacterID(), s, e);
		} else
			database.SaveInventory(this->CharacterID(), tmp_inst, to_slot);
	}
}

bool Client::MakeItemLink(char* &ret_link, const ItemInst *inst) {
	//we're sending back the entire "link", minus the null characters & item name
	//that way, we can use it for regular links & Task links
	//note: initiator needs to pass us ret_link

/*
	--- Usage ---
	Chat: "%c" "%s" "%s" "%c", 0x12, ret_link, inst->GetItem()->name, 0x12
	Task: "<a WndNotify=\"27," "%s" "\">" "%s" "</a>", ret_link, inst->GetItem()->name
		<a WndNotify="27,00960F000000000000000000000000000000000000000">Master's Book of Wood Elven Culture</a>
		http://eqitems.13th-floor.org/phpBB2/viewtopic.php?p=510#510
*/

	if (!inst) //have to have an item to make the link
		return false;

	const Item_Struct* item = inst->GetItem();

	static char itemid[7];
	sprintf(itemid, "%06d", item->ID);
	MakeAnyLenString(&ret_link, "%1X" "%s",
			0,
			itemid
	);

	return true;
}

int Client::GetItemLinkHash(const ItemInst* inst) {
	//pre-Titanium: http://eqitems.13th-floor.org/phpBB2/viewtopic.php?t=70&postdays=0&postorder=asc
	//Titanium: http://eqitems.13th-floor.org/phpBB2/viewtopic.php?t=145
	if (!inst)	//have to have an item to make the hash
		return 0;

	const Item_Struct* item = inst->GetItem();
	char* hash_str = 0;
	/*register */int hash = 0;

	//now the fun part, since different types of items use different hashes...
	if (item->ItemClass == 0 && item->CharmFileID) {	//charm
		MakeAnyLenString(&hash_str, "%d%s-1-1-1-1-1%d %d %d %d %d %d %d %d %d",
			item->ID,
			item->Name,
			item->Light,
			item->Icon,
			item->Price,
			item->Size,
			item->Weight,
			item->ItemClass,
			item->ItemType,
			item->Favor,
			item->GuildFavor);
	} else if (item->ItemClass == 2) {	//book
		MakeAnyLenString(&hash_str, "%d%s%d%d%09X",
			item->ID,
			item->Name,
			item->Weight,
			item->BookType,
			item->Price);
	} else if (item->ItemClass == 1) {	//bag
		MakeAnyLenString(&hash_str, "%d%s%x%d%09X%d",
			item->ID,
			item->Name,
			item->BagSlots,
			item->BagWR,
			item->Price,
			item->Weight);
	} else {	//everything else
		MakeAnyLenString(&hash_str, "%d%s-1-1-1-1-1%d %d %d %d %d %d %d %d %d %d %d %d %d",
			item->ID,
			item->Name,
			item->Mana,
			item->HP,
			item->Favor,
			item->Light,
			item->Icon,
			item->Price,
			item->Weight,
			item->ReqLevel,
			item->Size,
			item->ItemClass,
			item->ItemType,
			item->AC,
			item->GuildFavor);
	}

	//this currently crashes zone, so someone feel free to fix this so we can work with hashes:
	//*** glibc detected *** double free or corruption (out): 0xb2403470 ***

	/*
	while (*hash_str != '\0') {
		register int c = toupper(*hash_str);

		asm volatile("\
			imul $31, %1, %1;\
			movzx %%ax, %%edx;\
			addl %%edx, %1;\
			movl %1, %0;\
			"
			:"=r"(hash)
			:"D"(hash), "a"(c)
			:"%edx"
			);

		// This is what the inline asm is doing:
		// hash *= 0x1f;
		// hash += (int)c;

		hash_str++;
	}
	*/

	safe_delete_array(hash_str);
	return hash;
}

void Client::SendLootItemInPacket(const ItemInst* inst, int16 slot_id)
{
	SendItemPacket(slot_id,inst, ItemPacketTrade);
}

bool Client::IsValidSlot(uint32 slot)
{
	if((slot == (uint32)SLOT_INVALID) ||	// Destroying/Dropping item
		(slot >= 0 && slot <= 30) ||		// Worn inventory, normal inventory, and cursor
		(slot >= 250 && slot <= 339) ||		// Normal inventory bags and cursor bag
		(slot >= 2000 && slot <= 2007) ||	// Bank
		(slot >= 2030 && slot <= 2109) ||	// Bank bags
		(slot >= 3000 && slot <= 3007) ||	// Trade window
		(slot >= 4000 && slot <= 4009) ||	// Tradeskill container
		(slot == 9999))						// Power Source
	{
		return true;
	}
	else {
		return false;
	}
}

bool Client::IsBankSlot(uint32 slot)
{
	if ((slot >= 2000 && slot <= 2007) || // Bank
		(slot >= 2030 && slot <= 2109)) // Bank bags
	{
		return true;
	}

	return false;
}

// Moves items around both internally and in the database
// In the future, this can be optimized by pushing all changes through one database REPLACE call
int Client::SwapItem(MoveItem_Struct* move_in) {

	uint32 src_slot_check = move_in->from_slot;
	uint32 dst_slot_check = move_in->to_slot;
	uint32 stack_count_check = move_in->number_in_stack;

	if(!IsValidSlot(src_slot_check)){
		// SoF+ sends a Unix timestamp (should be int32) for src and dst slots every 10 minutes for some reason.
		if(src_slot_check < 2147483647)
			Message(13, "Warning: Invalid slot move from slot %u to slot %u with %u charges!", src_slot_check, dst_slot_check, stack_count_check);
		_log(INVENTORY__ERROR, "Invalid slot move from slot %u to slot %u with %u charges!", src_slot_check, dst_slot_check, stack_count_check);
		return 0;
	}

	if(!IsValidSlot(dst_slot_check)) {
		// SoF+ sends a Unix timestamp (should be int32) for src and dst slots every 10 minutes for some reason.
		if(src_slot_check < 2147483647)
			Message(13, "Warning: Invalid slot move from slot %u to slot %u with %u charges!", src_slot_check, dst_slot_check, stack_count_check);
		_log(INVENTORY__ERROR, "Invalid slot move from slot %u to slot %u with %u charges!", src_slot_check, dst_slot_check, stack_count_check);
		return 0;
	}

	// This could be expounded upon at some point to let the server know that
	// the client has moved a buffered cursor item onto the active cursor -U
	if (move_in->from_slot == move_in->to_slot) { // Item summon, no further proccessing needed
		if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in); } // QS Audit
		return 1;
	}

	if (move_in->to_slot == (uint32)SLOT_INVALID) {
		if(move_in->from_slot == (uint32)SLOT_CURSOR) {
			mlog(INVENTORY__SLOTS, "Client destroyed item from cursor slot %d", move_in->from_slot);
			if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in); } // QS Audit

			ItemInst *inst = m_inv.GetItem(SLOT_CURSOR);
			if(inst) {
				parse->EventItem(EVENT_DESTROY_ITEM, this, inst, nullptr, "", 0);
			}

			DeleteItemInInventory(move_in->from_slot);
			return 1; // Item destroyed by client
		}
		else {
			mlog(INVENTORY__SLOTS, "Deleted item from slot %d as a result of an inventory container tradeskill combine.", move_in->from_slot);
			if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in); } // QS Audit
			DeleteItemInInventory(move_in->from_slot);
			return 1; // Item deletetion
		}
	}
	if(auto_attack && (move_in->from_slot == SLOT_PRIMARY || move_in->from_slot == SLOT_SECONDARY || move_in->from_slot == SLOT_RANGE))
		SetAttackTimer();
	else if(auto_attack && (move_in->to_slot == SLOT_PRIMARY || move_in->to_slot == SLOT_SECONDARY || move_in->to_slot == SLOT_RANGE))
		SetAttackTimer();
	// Step 1: Variables
	int16 src_slot_id = (int16)move_in->from_slot;
	int16 dst_slot_id = (int16)move_in->to_slot;

	if(IsBankSlot(src_slot_id) ||
		IsBankSlot(dst_slot_id) ||
		IsBankSlot(src_slot_check) ||
		IsBankSlot(dst_slot_check))
	{
		uint32 distance = 0;
		NPC *banker = entity_list.GetClosestBanker(this, distance);

		if(!banker || distance > USE_NPC_RANGE2)
		{
			char *hacked_string = nullptr;
			MakeAnyLenString(&hacked_string, "Player tried to make use of a banker(items) but %s is non-existant or too far away (%u units).",
				banker ? banker->GetName() : "UNKNOWN NPC", distance);
			database.SetMQDetectionFlag(AccountName(), GetName(), hacked_string, zone->GetShortName());
			safe_delete_array(hacked_string);
			Kick();	// Kicking player to avoid item loss do to client and server inventories not being sync'd
			_log(INVENTORY__ERROR, "Banker error");
			return 0;
		}
	}

	//Setup
	uint32 srcitemid = 0;
	uint32 dstitemid = 0;
	ItemInst* src_inst = m_inv.GetItem(src_slot_id);
	ItemInst* dst_inst = m_inv.GetItem(dst_slot_id);
	if (src_inst){
		mlog(INVENTORY__SLOTS, "Src slot %d has item %s (%d) with %d charges in it.", src_slot_id, src_inst->GetItem()->Name, src_inst->GetItem()->ID, src_inst->GetCharges());
		srcitemid = src_inst->GetItem()->ID;
		//SetTint(dst_slot_id,src_inst->GetColor());
		if (src_inst->GetCharges() > 0 && (src_inst->GetCharges() < (int16)move_in->number_in_stack || move_in->number_in_stack > src_inst->GetItem()->StackSize))
		{
			//Damn Intel client sending SwapItem multiple times :I
			_log(INVENTORY__ERROR, "Insufficent number in stack. Ignore this if on EQMac.");
			return 0;
		}
	}
	if (dst_inst) {
		mlog(INVENTORY__SLOTS, "Dest slot %d has item %s (%d) with %d charges in it.", dst_slot_id, dst_inst->GetItem()->Name, dst_inst->GetItem()->ID, dst_inst->GetCharges());
		dstitemid = dst_inst->GetItem()->ID;
	}
	if (Trader && srcitemid>0){
		ItemInst* srcbag;
		ItemInst* dstbag;
		uint32 srcbagid =0;
		uint32 dstbagid = 0;
		if (src_slot_id>=250 && src_slot_id<330){
			srcbag=m_inv.GetItem(((int)(src_slot_id/10))-3);
			if(srcbag)
				srcbagid=srcbag->GetItem()->ID;
		}
		if (dst_slot_id>=250 && dst_slot_id<330){
			dstbag=m_inv.GetItem(((int)(dst_slot_id/10))-3);
			if(dstbag)
				dstbagid=dstbag->GetItem()->ID;
		}
		if (srcitemid==17899 || srcbagid==17899 || dstitemid==17899 || dstbagid==17899){
			this->Trader_EndTrader();
			this->Message(13,"You cannot move your Trader Satchels, or items inside them, while Trading.");
		}
	}

	bool recursive_si = false;
	// Step 2: Validate item in from_slot
	// After this, we can assume src_inst is a valid ptr
	if (!src_inst && (src_slot_id<4000 || src_slot_id>4009)) {
		if (dst_inst) {
			// If there is no source item, but there is a destination item,
			// move the slots around before deleting the invalid source slot item,
			// which is now in the destination slot.
			move_in->from_slot = dst_slot_check;
			move_in->to_slot = src_slot_check;
			move_in->number_in_stack = dst_inst->GetCharges();
			if(!SwapItem(move_in)) { 
				_log(INVENTORY__ERROR, "Recursive SwapItem call failed due to non-existent destination item (charid: %i, fromslot: %i, toslot: %i)", CharacterID(), src_slot_id, dst_slot_id); 
				//Intel EQMac sends 2 SwapItem packets when moving things to the cursor. Handle this here, and figure out a better way in the future. 
				return 2;
			}
			else
				recursive_si = true;
		}
		if(!recursive_si)
		{
			_log(INVENTORY__ERROR, "From slot is invalid and Recursive SwapItem call failed. (charid: %i, fromslot: %i, toslot: %i)", CharacterID(), src_slot_id, dst_slot_id); 
			return 0;
		}
	}

	// Check for No Drop Hacks
	Mob* with = trade->With();
	if (((with && with->IsClient() && dst_slot_id>=3000 && dst_slot_id<=3007)) // Trade
	&& GetInv().CheckNoDrop(src_slot_id)
	&& RuleI(World, FVNoDropFlag) == 0 || RuleI(Character, MinStatusForNoDropExemptions) < Admin() && RuleI(World, FVNoDropFlag) == 2) {
		DeleteItemInInventory(src_slot_id);
		WorldKick();
		_log(INVENTORY__ERROR, "No drop hack.");
		return 0;
	}

	// Step 3: Check for interaction with World Container (tradeskills)
	if(m_tradeskill_object != nullptr) {
		if (src_slot_id>=4000 && src_slot_id<=4009) {
			// Picking up item from world container
			ItemInst* inst = m_tradeskill_object->PopItem(Inventory::CalcBagIdx(src_slot_id));
			if (inst) {
				PutItemInInventory(dst_slot_id, *inst, false);
				safe_delete(inst);
			}

			if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in, true); } // QS Audit

			return true;
		}
		else if (dst_slot_id>=4000 && dst_slot_id<=4009) {
			// Putting item into world container, which may swap (or pile onto) with existing item
			uint8 world_idx = Inventory::CalcBagIdx(dst_slot_id);
			ItemInst* world_inst = m_tradeskill_object->PopItem(world_idx);

			// Case 1: No item in container, unidirectional "Put"
			if (world_inst == nullptr) {
				m_tradeskill_object->PutItem(world_idx, src_inst);
				m_inv.DeleteItem(src_slot_id);
			}
			else {
				const Item_Struct* world_item = world_inst->GetItem();
				const Item_Struct* src_item = src_inst->GetItem();
				if (world_item && src_item) {
					// Case 2: Same item on cursor, stacks, transfer of charges needed
					if ((world_item->ID == src_item->ID) && src_inst->IsStackable()) {
						int16 world_charges = world_inst->GetCharges();
						int16 src_charges = src_inst->GetCharges();

						// Fill up destination stack as much as possible
						world_charges += src_charges;
						if (world_charges > world_inst->GetItem()->StackSize) {
							src_charges = world_charges - world_inst->GetItem()->StackSize;
							world_charges = world_inst->GetItem()->StackSize;
						}
						else {
							src_charges = 0;
						}

						world_inst->SetCharges(world_charges);
						m_tradeskill_object->Save();

						if (src_charges == 0) {
							m_inv.DeleteItem(src_slot_id); // DB remove will occur below
						}
						else {
							src_inst->SetCharges(src_charges);
						}
					}
					else {
						// Case 3: Swap the item on user with item in world container
						// World containers don't follow normal rules for swapping
						ItemInst* inv_inst = m_inv.PopItem(src_slot_id);
						m_tradeskill_object->PutItem(world_idx, inv_inst);
						m_inv.PutItem(src_slot_id, *world_inst);
						safe_delete(inv_inst);
					}
				}
			}

			safe_delete(world_inst);
			if (src_slot_id==SLOT_CURSOR) {
				std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
				database.SaveCursor(character_id, s, e);
			} else
				database.SaveInventory(character_id, m_inv[src_slot_id], src_slot_id);

			if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in, true); } // QS Audit

			return 1;
		}
	}

	// Step 4: Check for entity trade
	if (dst_slot_id>=3000 && dst_slot_id<=3007) {
		if (src_slot_id != SLOT_CURSOR) {
			Kick();
			_log(INVENTORY__ERROR, "Trading item no on cursor.");
			return 0;
		}
		if (with) {
			mlog(INVENTORY__SLOTS, "Trade item move from slot %d to slot %d (trade with %s)", src_slot_id, dst_slot_id, with->GetName());
			// Fill Trade list with items from cursor
			if (!m_inv[SLOT_CURSOR]) {
				Message(13, "Error: Cursor item not located on server!");
				_log(INVENTORY__ERROR, "Error: Cursor item not located on server!");
				return 0;
			}

			// Add cursor item to trade bucket
			// Also sends trade information to other client of trade session
			if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in); } // QS Audit

			trade->AddEntity(src_slot_id, dst_slot_id);

			return 1;
		} else {
			if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in); } // QS Audit

			SummonItem(src_inst->GetID(), src_inst->GetCharges());
			DeleteItemInInventory(SLOT_CURSOR);

			return 1;
		}
	}

	// Step 5: Swap (or stack) items
	if (move_in->number_in_stack > 0) {
		// Determine if charged items can stack
		if(src_inst && !src_inst->IsStackable()) {
			_log(INVENTORY__ERROR, "Move from %d to %d with stack size %d. %s is not a stackable item. (charname: %s)", src_slot_id, dst_slot_id, move_in->number_in_stack, src_inst->GetItem()->Name, GetName());
			return 0;
		}

		if (src_inst && dst_inst) {
			if(src_inst->GetID() != dst_inst->GetID()) {
				_log(INVENTORY__ERROR, "Move from %d to %d with stack size %d. Incompatible item types: %d != %d", src_slot_id, dst_slot_id, move_in->number_in_stack, src_inst->GetID(), dst_inst->GetID());
				return 0;
			}
			if(dst_inst->GetCharges() < dst_inst->GetItem()->StackSize) {
				//we have a chance of stacking.
				mlog(INVENTORY__SLOTS, "Move from %d to %d with stack size %d. dest has %d/%d charges", src_slot_id, dst_slot_id, move_in->number_in_stack, dst_inst->GetCharges(), dst_inst->GetItem()->StackSize);
				// Charges can be emptied into dst
				uint16 usedcharges = dst_inst->GetItem()->StackSize - dst_inst->GetCharges();
				if (usedcharges > move_in->number_in_stack)
					usedcharges = move_in->number_in_stack;

				dst_inst->SetCharges(dst_inst->GetCharges() + usedcharges);
				src_inst->SetCharges(src_inst->GetCharges() - usedcharges);

				// Depleted all charges?
				if (src_inst->GetCharges() < 1)
				{
					mlog(INVENTORY__SLOTS, "Dest (%d) now has %d charges, source (%d) was entirely consumed. (%d moved)", dst_slot_id, dst_inst->GetCharges(), src_slot_id, usedcharges);
					database.SaveInventory(CharacterID(),nullptr,src_slot_id);
					m_inv.DeleteItem(src_slot_id);
				} else {
					mlog(INVENTORY__SLOTS, "Dest (%d) now has %d charges, source (%d) has %d (%d moved)", dst_slot_id, dst_inst->GetCharges(), src_slot_id, src_inst->GetCharges(), usedcharges);
				}
			} else {
				_log(INVENTORY__ERROR, "Move from %d to %d with stack size %d. Exceeds dest maximum stack size: %d/%d", src_slot_id, dst_slot_id, move_in->number_in_stack, (src_inst->GetCharges()+dst_inst->GetCharges()), dst_inst->GetItem()->StackSize);
				return 0;
			}
		
		}
		else if(!src_inst)
		{
			//I also believe this is being caused by EQMac double packets. I cannot get this to happen on PC client at all, but happens on Intel client all the time.
			_log(INVENTORY__ERROR, "src_inst has become invalid somewhere.");
			Message(CC_Yellow, "Caution: You may have de-synced. Stopped what you're doing, and log out now to avoid item loss.");
			//return 2;
		}
		else {
			// Nothing in destination slot: split stack into two
			if ((int8)move_in->number_in_stack >= src_inst->GetCharges()) {
				// Move entire stack
				if(!m_inv.SwapItem(src_slot_id, dst_slot_id)) { 
					mlog(INVENTORY__ERROR, "Could not move entire stack from %d to %d with stack size %d. Dest empty.", src_slot_id, dst_slot_id, move_in->number_in_stack);
					return 0; 
				}
				mlog(INVENTORY__SLOTS, "Move entire stack from %d to %d with stack size %d. Dest empty.", src_slot_id, dst_slot_id, move_in->number_in_stack);
			}
			else {
				// Split into two
				src_inst->SetCharges(src_inst->GetCharges() - move_in->number_in_stack);
				mlog(INVENTORY__SLOTS, "Split stack of %s (%d) from slot %d to %d with stack size %d. Src keeps %d.", src_inst->GetItem()->Name, src_inst->GetItem()->ID, src_slot_id, dst_slot_id, move_in->number_in_stack, src_inst->GetCharges());
				ItemInst* inst = database.CreateItem(src_inst->GetItem(), move_in->number_in_stack);
				m_inv.PutItem(dst_slot_id, *inst);
				safe_delete(inst);
			}
		}
	}
	else {
		// Not dealing with charges - just do direct swap
		if(src_inst && dst_slot_id < 22 && dst_slot_id >= 1) {
			if (src_inst->GetItem()->Attuneable) {
				src_inst->SetInstNoDrop(true);
			}
			SetMaterial(dst_slot_id,src_inst->GetItem()->ID);
		}
		if(!m_inv.SwapItem(src_slot_id, dst_slot_id)) {
			mlog(INVENTORY__ERROR, "Destination slot is not valid for item %s from slot %d to slot %d", src_inst->GetItem()->Name, src_slot_id, dst_slot_id);
			return 0;
		}
		mlog(INVENTORY__SLOTS, "Moving entire item from slot %d to slot %d", src_slot_id, dst_slot_id);

		if(src_slot_id > 0 && src_slot_id < 22) {
			if(src_inst) {
				parse->EventItem(EVENT_UNEQUIP_ITEM, this, src_inst, nullptr, "", src_slot_id);
			}

			if(dst_inst) {
				parse->EventItem(EVENT_EQUIP_ITEM, this, dst_inst, nullptr, "", src_slot_id);
			}
		}

		if(dst_slot_id > 0 && dst_slot_id < 22) {
			if(dst_inst) {
				parse->EventItem(EVENT_UNEQUIP_ITEM, this, dst_inst, nullptr, "", dst_slot_id);
			}

			if(src_inst) {
				parse->EventItem(EVENT_EQUIP_ITEM, this, src_inst, nullptr, "", dst_slot_id);
			}
		}
	}

	int matslot = SlotConvert2(dst_slot_id);
	if (dst_slot_id > 0 && dst_slot_id<22 && matslot != 0) {
		SendWearChange(matslot);
	}

	// Step 7: Save change to the database
	if (src_slot_id==SLOT_CURSOR){
		std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
		database.SaveCursor(character_id, s, e);
	} else
		database.SaveInventory(character_id, m_inv.GetItem(src_slot_id), src_slot_id);
	if (dst_slot_id==SLOT_CURSOR) {
		std::list<ItemInst*>::const_iterator s=m_inv.cursor_begin(),e=m_inv.cursor_end();
		database.SaveCursor(character_id, s, e);
	} else
		database.SaveInventory(character_id, m_inv.GetItem(dst_slot_id), dst_slot_id);

	if(RuleB(QueryServ, PlayerLogMoves)) { QSSwapItemAuditor(move_in, true); } // QS Audit

	// Step 8: Re-calc stats
	CalcBonuses();
	return 1;
}

void Client::SwapItemResync(MoveItem_Struct* move_slots) {
	// resync the 'from' and 'to' slots on an as-needed basis
	// Not as effective as the full process, but less intrusive to gameplay -U
	mlog(INVENTORY__ERROR, "Inventory desyncronization. (charname: %s, source: %i, destination: %i)", GetName(), move_slots->from_slot, move_slots->to_slot);
	Message(15, "Inventory Desyncronization detected: Resending slot data...");

	if((move_slots->from_slot >= 0 && move_slots->from_slot <= 340) || move_slots->from_slot == 9999) {
		int16 resync_slot = (Inventory::CalcSlotId(move_slots->from_slot) == SLOT_INVALID) ? move_slots->from_slot : Inventory::CalcSlotId(move_slots->from_slot);
		if(IsValidSlot(resync_slot) && resync_slot != SLOT_INVALID) {
			// This prevents the client from crashing when closing any 'phantom' bags -U
			const Item_Struct* token_struct = database.GetItem(22292); // 'Copper Coin'
			ItemInst* token_inst = database.CreateItem(token_struct, 1);

			SendItemPacket(resync_slot, token_inst, ItemPacketTrade);

			if(m_inv[resync_slot]) { SendItemPacket(resync_slot, m_inv[resync_slot], ItemPacketTrade); }
			else {
				EQApplicationPacket* outapp		= new EQApplicationPacket(OP_MoveItem, sizeof(MoveItem_Struct));
				MoveItem_Struct* delete_slot	= (MoveItem_Struct*)outapp->pBuffer;
				delete_slot->from_slot			= resync_slot;
				delete_slot->to_slot			= 0xFFFFFFFF;
				delete_slot->number_in_stack	= 0xFFFFFFFF;

				QueuePacket(outapp);
				safe_delete(outapp);
			}
			Message(14, "Source slot %i resyncronized.", move_slots->from_slot);
		}
		else { Message(13, "Could not resyncronize source slot %i.", move_slots->from_slot); }
	}
	else {
		int16 resync_slot = (Inventory::CalcSlotId(move_slots->from_slot) == SLOT_INVALID) ? move_slots->from_slot : Inventory::CalcSlotId(move_slots->from_slot);
		if(IsValidSlot(resync_slot) && resync_slot != SLOT_INVALID) {
			if(m_inv[resync_slot]) {
				const Item_Struct* token_struct = database.GetItem(22292); // 'Copper Coin'
				ItemInst* token_inst = database.CreateItem(token_struct, 1);

				SendItemPacket(resync_slot, token_inst, ItemPacketTrade);
				SendItemPacket(resync_slot, m_inv[resync_slot], ItemPacketTrade);

				Message(14, "Source slot %i resyncronized.", move_slots->from_slot);
			}
			else { Message(13, "Could not resyncronize source slot %i.", move_slots->from_slot); }
		}
		else { Message(13, "Could not resyncronize source slot %i.", move_slots->from_slot); }
	}

	if((move_slots->to_slot >= 0 && move_slots->to_slot <= 340) || move_slots->to_slot == 9999) {
		int16 resync_slot = (Inventory::CalcSlotId(move_slots->to_slot) == SLOT_INVALID) ? move_slots->to_slot : Inventory::CalcSlotId(move_slots->to_slot);
		if(IsValidSlot(resync_slot) && resync_slot != SLOT_INVALID) {
			const Item_Struct* token_struct = database.GetItem(22292); // 'Copper Coin'
			ItemInst* token_inst = database.CreateItem(token_struct, 1);

			SendItemPacket(resync_slot, token_inst, ItemPacketTrade);

			if(m_inv[resync_slot]) { SendItemPacket(resync_slot, m_inv[resync_slot], ItemPacketTrade); }
			else {
				EQApplicationPacket* outapp		= new EQApplicationPacket(OP_MoveItem, sizeof(MoveItem_Struct));
				MoveItem_Struct* delete_slot	= (MoveItem_Struct*)outapp->pBuffer;
				delete_slot->from_slot			= resync_slot;
				delete_slot->to_slot			= 0xFFFFFFFF;
				delete_slot->number_in_stack	= 0xFFFFFFFF;

				QueuePacket(outapp);
				safe_delete(outapp);
			}
			Message(14, "Destination slot %i resyncronized.", move_slots->to_slot);
		}
		else { Message(13, "Could not resyncronize destination slot %i.", move_slots->to_slot); }
	}
	else {
		int16 resync_slot = (Inventory::CalcSlotId(move_slots->to_slot) == SLOT_INVALID) ? move_slots->to_slot : Inventory::CalcSlotId(move_slots->to_slot);
		if(IsValidSlot(resync_slot) && resync_slot != SLOT_INVALID) {
			if(m_inv[resync_slot]) {
				const Item_Struct* token_struct = database.GetItem(22292); // 'Copper Coin'
				ItemInst* token_inst = database.CreateItem(token_struct, 1);

				SendItemPacket(resync_slot, token_inst, ItemPacketTrade);
				SendItemPacket(resync_slot, m_inv[resync_slot], ItemPacketTrade);

				Message(14, "Destination slot %i resyncronized.", move_slots->to_slot);
			}
			else { Message(13, "Could not resyncronize destination slot %i.", move_slots->to_slot); }
		}
		else { Message(13, "Could not resyncronize destination slot %i.", move_slots->to_slot); }
	}
}

void Client::QSSwapItemAuditor(MoveItem_Struct* move_in, bool postaction_call) {
	int16 from_slot_id = static_cast<int16>(move_in->from_slot);
	int16 to_slot_id	= static_cast<int16>(move_in->to_slot);
	int16 move_amount	= static_cast<int16>(move_in->number_in_stack);

	if(!m_inv[from_slot_id] && !m_inv[to_slot_id]) { return; }

	uint16 move_count = 0;

	if(m_inv[from_slot_id]) { move_count += m_inv[from_slot_id]->GetTotalItemCount(); }
	if(to_slot_id != from_slot_id) { if(m_inv[to_slot_id]) { move_count += m_inv[to_slot_id]->GetTotalItemCount(); } }

	ServerPacket* qspack = new ServerPacket(ServerOP_QSPlayerLogMoves, sizeof(QSPlayerLogMove_Struct) + (sizeof(QSMoveItems_Struct) * move_count));
	QSPlayerLogMove_Struct* qsaudit = (QSPlayerLogMove_Struct*)qspack->pBuffer;

	qsaudit->char_id	= character_id;
	qsaudit->stack_size = move_amount;
	qsaudit->char_count = move_count;
	qsaudit->postaction = postaction_call;
	qsaudit->from_slot	= from_slot_id;
	qsaudit->to_slot	= to_slot_id;

	move_count = 0;

	const ItemInst* from_inst = m_inv[postaction_call?to_slot_id:from_slot_id];

	if(from_inst) {
		qsaudit->items[move_count].from_slot	= from_slot_id;
		qsaudit->items[move_count].to_slot		= to_slot_id;
		qsaudit->items[move_count].item_id		= from_inst->GetID();
		qsaudit->items[move_count++].charges		= from_inst->GetCharges();

		if(from_inst->IsType(ItemClassContainer)) {
			for(uint8 bag_idx = 0; bag_idx < from_inst->GetItem()->BagSlots; bag_idx++) {
				const ItemInst* from_baginst = from_inst->GetItem(bag_idx);

				if(from_baginst) {
					qsaudit->items[move_count].from_slot	= Inventory::CalcSlotId(from_slot_id, bag_idx);
					qsaudit->items[move_count].to_slot		= Inventory::CalcSlotId(to_slot_id, bag_idx);
					qsaudit->items[move_count].item_id		= from_baginst->GetID();
					qsaudit->items[move_count++].charges		= from_baginst->GetCharges();
				}
			}
		}
	}

	if(to_slot_id != from_slot_id) {
		const ItemInst* to_inst = m_inv[postaction_call?from_slot_id:to_slot_id];

		if(to_inst) {
			qsaudit->items[move_count].from_slot	= to_slot_id;
			qsaudit->items[move_count].to_slot		= from_slot_id;
			qsaudit->items[move_count].item_id		= to_inst->GetID();
			qsaudit->items[move_count++].charges		= to_inst->GetCharges();

			if(to_inst->IsType(ItemClassContainer)) {
				for(uint8 bag_idx = 0; bag_idx < to_inst->GetItem()->BagSlots; bag_idx++) {
					const ItemInst* to_baginst = to_inst->GetItem(bag_idx);

					if(to_baginst) {
						qsaudit->items[move_count].from_slot	= Inventory::CalcSlotId(to_slot_id, bag_idx);
						qsaudit->items[move_count].to_slot		= Inventory::CalcSlotId(from_slot_id, bag_idx);
						qsaudit->items[move_count].item_id		= to_baginst->GetID();
						qsaudit->items[move_count++].charges		= to_baginst->GetCharges();
					}
				}
			}
		}
	}

	if(move_count && worldserver.Connected()) {
		qspack->Deflate();
		worldserver.SendPacket(qspack);
	}

	safe_delete(qspack);
}

void Client::DyeArmor(DyeStruct* dye){
	int16 slot=0;
	for(int i=0;i<7;i++){
		if(m_pp.item_tint[i].rgb.blue!=dye->dye[i].rgb.blue ||
			m_pp.item_tint[i].rgb.red!=dye->dye[i].rgb.red ||
			m_pp.item_tint[i].rgb.green != dye->dye[i].rgb.green){
			slot = m_inv.HasItem(32557, 1, invWherePersonal);
			if(slot != SLOT_INVALID){
				DeleteItemInInventory(slot,1,true);
				uint8 slot2=SlotConvert(i);
				ItemInst* inst = this->m_inv.GetItem(slot2);
				if(inst){
					inst->SetColor((dye->dye[i].rgb.red*65536)+(dye->dye[i].rgb.green*256)+(dye->dye[i].rgb.blue));
					database.SaveInventory(CharacterID(),inst,slot2);
					if(dye->dye[i].rgb.use_tint)
						m_pp.item_tint[i].rgb.use_tint = 0xFF;
					else
						m_pp.item_tint[i].rgb.use_tint=0x00;
				}
				m_pp.item_tint[i].rgb.blue=dye->dye[i].rgb.blue;
				m_pp.item_tint[i].rgb.red=dye->dye[i].rgb.red;
				m_pp.item_tint[i].rgb.green=dye->dye[i].rgb.green;
				SendWearChange(i);
			}
			else{
				Message(13,"Could not locate A Vial of Prismatic Dye.");
				return;
			}
		}
	}
	EQApplicationPacket* outapp=new EQApplicationPacket(OP_Dye,0);
	QueuePacket(outapp);
	safe_delete(outapp);
	Save();
}

bool Client::DecreaseByID(uint32 type, uint8 amt) {
	const Item_Struct* TempItem = 0;
	ItemInst* ins;
	int x;
	int num = 0;
	for(x=0; x < 330; x++)
	{
		if (x == 31)
			x = 250;
		TempItem = 0;
		ins = GetInv().GetItem(x);
		if (ins)
			TempItem = ins->GetItem();
		if (TempItem && TempItem->ID == type)
		{
			num += ins->GetCharges();
			if (num >= amt)
				break;
		}
	}
	if (num < amt)
		return false;
	for(x=0; x < 330; x++)
	{
		if (x == 31)
			x = 250;
		TempItem = 0;
		ins = GetInv().GetItem(x);
		if (ins)
			TempItem = ins->GetItem();
		if (TempItem && TempItem->ID == type)
		{
			if (ins->GetCharges() < amt)
			{
				amt -= ins->GetCharges();
				DeleteItemInInventory(x,amt,true);
			}
			else
			{
				DeleteItemInInventory(x,amt,true);
				amt = 0;
			}
			if (amt < 1)
				break;
		}
	}
	return true;
}

void Client::RemoveNoRent(bool client_update) {

	int16 slot_id;

	// personal
	for(slot_id = 0; slot_id <= 30; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if(inst && !inst->GetItem()->NoRent) {
			mlog(INVENTORY__SLOTS, "NoRent Timer Lapse: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
			DeleteItemInInventory(slot_id, 0, client_update);
		}
	}

	// containers
	for(slot_id = 250; slot_id <= 339; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if(inst && !inst->GetItem()->NoRent) {
			mlog(INVENTORY__SLOTS, "NoRent Timer Lapse: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
			DeleteItemInInventory(slot_id, 0, client_update);
		}
	}

	// bank
	for(slot_id = 2000; slot_id <= 2007; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if(inst && !inst->GetItem()->NoRent) {
			mlog(INVENTORY__SLOTS, "NoRent Timer Lapse: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
			DeleteItemInInventory(slot_id, 0, false); // Can't delete from client Bank slots
		}
	}

	// bank containers
	for(slot_id = 2030; slot_id <= 2109; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if(inst && !inst->GetItem()->NoRent) {
			mlog(INVENTORY__SLOTS, "NoRent Timer Lapse: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
			DeleteItemInInventory(slot_id, 0, false); // Can't delete from client Bank Container slots
		}
	}
}

// Two new methods to alleviate perpetual login desyncs
void Client::RemoveDuplicateLore(bool client_update) {
	// Split-charge stacking may be added at some point -U
	int16 slot_id;

	// personal
	for(slot_id = 0; slot_id <= 30; slot_id++) {
		ItemInst* inst = m_inv.PopItem(slot_id);
		if(inst) {
			if(CheckLoreConflict(inst->GetItem())) {
				mlog(INVENTORY__ERROR, "Lore Duplication Error: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
				database.SaveInventory(character_id, nullptr, slot_id);
			}
			else {
				m_inv.PutItem(slot_id, *inst);
			}
			safe_delete(inst);
		}
	}

	// power source
	ItemInst* inst = m_inv.PopItem(9999);
	if(inst) {
		if(CheckLoreConflict(inst->GetItem())) {
			mlog(INVENTORY__ERROR, "Lore Duplication Error: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
			database.SaveInventory(character_id, nullptr, 9999);
		}
		else {
			m_inv.PutItem(9999, *inst);
		}
		safe_delete(inst);
	}

	// containers
	for(slot_id = 250; slot_id <= 339; slot_id++) {
		ItemInst* inst = m_inv.PopItem(slot_id);
		if(inst) {
			if(CheckLoreConflict(inst->GetItem())) {
				mlog(INVENTORY__ERROR, "Lore Duplication Error: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
				database.SaveInventory(character_id, nullptr, slot_id);
			}
			else {
				m_inv.PutItem(slot_id, *inst);
			}
			safe_delete(inst);
		}
	}

	// bank
	for(slot_id = 2000; slot_id <= 2007; slot_id++) {
		ItemInst* inst = m_inv.PopItem(slot_id);
		if(inst) {
			if(CheckLoreConflict(inst->GetItem())) {
				mlog(INVENTORY__ERROR, "Lore Duplication Error: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
				database.SaveInventory(character_id, nullptr, slot_id);
			}
			else {
				m_inv.PutItem(slot_id, *inst);
			}
			safe_delete(inst);
		}
	}

	// bank containers
	for(slot_id = 2030; slot_id <= 2109; slot_id++) {
		ItemInst* inst = m_inv.PopItem(slot_id);
		if(inst) {
			if(CheckLoreConflict(inst->GetItem())) {
				mlog(INVENTORY__ERROR, "Lore Duplication Error: Deleting %s from slot %i", inst->GetItem()->Name, slot_id);
				database.SaveInventory(character_id, nullptr, slot_id);
			}
			else {
				m_inv.PutItem(slot_id, *inst);
			}
			safe_delete(inst);
		}
	}
}

void Client::MoveSlotNotAllowed(bool client_update) {

	int16 slot_id;

	// equipment
	for(slot_id = 0; slot_id <= 21; slot_id++) {
		if(m_inv[slot_id] && !m_inv[slot_id]->IsSlotAllowed(slot_id)) {
			ItemInst* inst = m_inv.PopItem(slot_id);
			bool is_arrow = (inst->GetItem()->ItemType == ItemTypeArrow) ? true : false;
			int16 free_slot_id = m_inv.FindFreeSlot(inst->IsType(ItemClassContainer), true, inst->GetItem()->Size, is_arrow);
			mlog(INVENTORY__ERROR, "Slot Assignment Error: Moving %s from slot %i to %i", inst->GetItem()->Name, slot_id, free_slot_id);
			PutItemInInventory(free_slot_id, *inst, client_update);
			database.SaveInventory(character_id, nullptr, slot_id);
			safe_delete(inst);
		}
	}

	// No need to check inventory, cursor, bank or shared bank since they allow max item size and containers -U
	// Code can be added to check item size vs. container size, but it is left to attrition for now.
}

// these functions operate with a material slot, which is from 0 to 8
uint32 Client::GetEquipment(uint8 material_slot) const
{
	int invslot;
	const ItemInst *item;

	if(material_slot > 8)
	{
		return 0;
	}

	invslot = Inventory::CalcSlotFromMaterial(material_slot);
	if(invslot == -1)
	{
		return 0;
	}

	item = m_inv.GetItem(invslot);

	if(item && item->GetItem())
	{
		return item->GetItem()->ID;
	}

	return 0;
}

uint32 Client::GetEquipmentColor(uint8 material_slot) const
{
	const Item_Struct *item;

	if(material_slot > 8)
	{
		return 0;
	}

	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return m_pp.item_tint[material_slot].rgb.use_tint ?
			m_pp.item_tint[material_slot].color :
			item->Color;
	}

	return 0;
}

// Send an item packet (including all subitems of the item)
void Client::SendItemPacket(int16 slot_id, const ItemInst* inst, ItemPacketType packet_type, int16 fromid)
{
	if (!inst)
		return;

	// Serialize item into |-delimited string
	std::string packet = inst->Serialize(slot_id);

	EmuOpcode opcode = OP_Unknown;
	EQApplicationPacket* outapp = nullptr;
	ItemPacket_Struct* itempacket = nullptr;

	// Construct packet
	if(packet_type==ItemPacketViewLink)
		opcode = OP_ItemLinkResponse;
	else if(packet_type==ItemPacketTradeView)
		opcode = OP_TradeItemPacket;
	else
		opcode = OP_ItemPacket;
	//opcode = (packet_type==ItemPacketViewLink) ? OP_ItemLinkResponse : OP_ItemPacket;
	outapp = new EQApplicationPacket(opcode, packet.length()+sizeof(ItemPacket_Struct));
	itempacket = (ItemPacket_Struct*)outapp->pBuffer;
	memcpy(itempacket->SerializedItem, packet.c_str(), packet.length());
	itempacket->PacketType = packet_type;
	itempacket->fromid = fromid;

#if EQDEBUG >= 9
		DumpPacket(outapp);
#endif
	FastQueuePacket(&outapp);
}

EQApplicationPacket* Client::ReturnItemPacket(int16 slot_id, const ItemInst* inst, ItemPacketType packet_type)
{
	if (!inst)
		return 0;

	// Serialize item into |-delimited string
	std::string packet = inst->Serialize(slot_id);

	EmuOpcode opcode = OP_Unknown;
	EQApplicationPacket* outapp = nullptr;
	BulkItemPacket_Struct* itempacket = nullptr;

	// Construct packet
	opcode = OP_ItemPacket;
	outapp = new EQApplicationPacket(opcode, packet.length()+1);
	itempacket = (BulkItemPacket_Struct*)outapp->pBuffer;
	memcpy(itempacket->SerializedItem, packet.c_str(), packet.length());

#if EQDEBUG >= 9
		DumpPacket(outapp);
#endif

	return outapp;
}

bool Client::MoveItemToInventory(ItemInst *ItemToReturn, bool UpdateClient) {

	// This is a support function for Client::SetBandolier, however it can be used anywhere it's functionality is required.
	//
	// When the client moves items around as Bandolier sets are activated, it does not send details to the
	// server of what item it has moved to which slot. It assumes the server knows what it will do.
	//
	// The standard EQEmu auto inventory routines do not behave as the client does when manipulating bandoliers.
	// The client will look in each main inventory slot. If it finds a bag in a slot, it will then look inside
	// the bag for a free slot.
	//
	// This differs from the standard EQEmu method of looking in all 8 inventory slots first to find an empty slot, and
	// then going back and looking in bags. There are also other differences related to how it moves stackable items back
	// to inventory.
	//
	// Rather than alter the current auto inventory behaviour, just in case something
	// depends on current behaviour, this routine operates the same as the client when moving items back to inventory when
	// swapping bandolier sets.

	if(!ItemToReturn) return false;

	_log(INVENTORY__SLOTS,"Char: %s Returning %s to inventory", GetName(), ItemToReturn->GetItem()->Name);

	uint32 ItemID = ItemToReturn->GetItem()->ID;

	// If the item is stackable (ammo in range slot), try stacking it with other items of the same type
	//
	if(ItemToReturn->IsStackable()) {

		for (int16 i=22; i<=30; i++) { // changed slot max to 30 from 29. client will stack into slot 30 (bags too) before moving.

			ItemInst* InvItem = m_inv.GetItem(i);

			if(InvItem && (InvItem->GetItem()->ID == ItemID) && (InvItem->GetCharges() < InvItem->GetItem()->StackSize)) {

				int ChargeSlotsLeft = InvItem->GetItem()->StackSize - InvItem->GetCharges();

				int ChargesToMove = ItemToReturn->GetCharges() < ChargeSlotsLeft ? ItemToReturn->GetCharges() :
													ChargeSlotsLeft;

				InvItem->SetCharges(InvItem->GetCharges() + ChargesToMove);

				if(UpdateClient)
					SendItemPacket(i, InvItem, ItemPacketTrade);

				database.SaveInventory(character_id, m_inv.GetItem(i), i);

				ItemToReturn->SetCharges(ItemToReturn->GetCharges() - ChargesToMove);

				if(!ItemToReturn->GetCharges())
					return true;
			}
			// If there is a bag in this slot, look inside it.
			//
			if (InvItem && InvItem->IsType(ItemClassContainer)) {

				int16 BaseSlotID = Inventory::CalcSlotId(i, 0);

				uint8 BagSize=InvItem->GetItem()->BagSlots;

				uint8 BagSlot;
				for (BagSlot=0; BagSlot<BagSize; BagSlot++) {
					InvItem = m_inv.GetItem(BaseSlotID + BagSlot);
					if (InvItem && (InvItem->GetItem()->ID == ItemID) &&
						(InvItem->GetCharges() < InvItem->GetItem()->StackSize)) {

						int ChargeSlotsLeft = InvItem->GetItem()->StackSize - InvItem->GetCharges();

						int ChargesToMove = ItemToReturn->GetCharges() < ChargeSlotsLeft
									? ItemToReturn->GetCharges() : ChargeSlotsLeft;

						InvItem->SetCharges(InvItem->GetCharges() + ChargesToMove);

						if(UpdateClient)
							SendItemPacket(BaseSlotID + BagSlot, m_inv.GetItem(BaseSlotID + BagSlot),
										ItemPacketTrade);

						database.SaveInventory(character_id, m_inv.GetItem(BaseSlotID + BagSlot),
										BaseSlotID + BagSlot);

						ItemToReturn->SetCharges(ItemToReturn->GetCharges() - ChargesToMove);

						if(!ItemToReturn->GetCharges())
							return true;
					}
				}
			}
		}
	}

	// We have tried stacking items, now just try and find an empty slot.

	for (int16 i=22; i<=30; i++) { // changed slot max to 30 from 29. client will move into slot 30 (bags too) before pushing onto cursor.

		ItemInst* InvItem = m_inv.GetItem(i);

		if (!InvItem) {
			// Found available slot in personal inventory
			m_inv.PutItem(i, *ItemToReturn);

			if(UpdateClient)
				SendItemPacket(i, ItemToReturn, ItemPacketTrade);

			database.SaveInventory(character_id, m_inv.GetItem(i), i);

			_log(INVENTORY__SLOTS, "Char: %s Storing in main inventory slot %i", GetName(), i);

			return true;
		}
		if(InvItem->IsType(ItemClassContainer) && Inventory::CanItemFitInContainer(ItemToReturn->GetItem(), InvItem->GetItem())) {

			int16 BaseSlotID = Inventory::CalcSlotId(i, 0);

			uint8 BagSize=InvItem->GetItem()->BagSlots;

			for (uint8 BagSlot=0; BagSlot<BagSize; BagSlot++) {

				InvItem = m_inv.GetItem(BaseSlotID + BagSlot);

				if (!InvItem) {
					// Found available slot within bag
					m_inv.PutItem(BaseSlotID + BagSlot, *ItemToReturn);

					if(UpdateClient)
						SendItemPacket(BaseSlotID + BagSlot, ItemToReturn, ItemPacketTrade);

					database.SaveInventory(character_id, m_inv.GetItem(BaseSlotID + BagSlot), BaseSlotID + BagSlot);

					_log(INVENTORY__SLOTS, "Char: %s Storing in bag slot %i", GetName(), BaseSlotID + BagSlot);

					return true;
				}
			}
		}
	}

	// Store on the cursor
	//
	_log(INVENTORY__SLOTS, "Char: %s No space, putting on the cursor", GetName());

	PushItemOnCursor(*ItemToReturn, UpdateClient);

	return true;
}

void Inventory::SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, std::string value) {
	ItemInst *inst = GetItem(slot_id);
	if(inst) {
		inst->SetCustomData(identifier, value);
		database.SaveInventory(character_id, inst, slot_id);
	}
}

void Inventory::SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, int value) {
	ItemInst *inst = GetItem(slot_id);
	if(inst) {
		inst->SetCustomData(identifier, value);
		database.SaveInventory(character_id, inst, slot_id);
	}
}

void Inventory::SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, float value) {
	ItemInst *inst = GetItem(slot_id);
	if(inst) {
		inst->SetCustomData(identifier, value);
		database.SaveInventory(character_id, inst, slot_id);
	}
}

void Inventory::SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, bool value) {
	ItemInst *inst = GetItem(slot_id);
	if(inst) {
		inst->SetCustomData(identifier, value);
		database.SaveInventory(character_id, inst, slot_id);
	}
}

std::string Inventory::GetCustomItemData(int16 slot_id, std::string identifier) {
	ItemInst *inst = GetItem(slot_id);
	if(inst) {
		return inst->GetCustomData(identifier);
	}
	return "";
}

