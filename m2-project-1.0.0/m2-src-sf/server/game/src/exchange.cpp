#include "stdafx.h"
#include "config.h"
#include "../../libgame/include/grid.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "locale_service.h"
#include "../../common/length.h"
#include "exchange.h"
#include "DragonSoul.h"
#include "constants.h"
#include "dev_log.h"
#include "db.h"
#include "log.h"
#ifdef NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif

void exchange_packet(LPCHARACTER ch, uint8_t sub_header, bool is_me, int64_t arg1, TItemPos arg2, uint32_t arg3, void * pvData = NULL);

void exchange_packet(LPCHARACTER ch, uint8_t sub_header, bool is_me, int64_t arg1, TItemPos arg2, uint32_t arg3, void * pvData)
{
	if (!ch->GetDesc())
		return;

	struct packet_exchange pack_exchg;

	pack_exchg.header 		= HEADER_GC_EXCHANGE;
	pack_exchg.sub_header 	= sub_header;
	pack_exchg.is_me		= is_me;
	pack_exchg.arg1		= arg1;
	pack_exchg.arg2		= arg2;
	pack_exchg.arg3		= arg3;

	if (sub_header == EXCHANGE_SUBHEADER_GC_ITEM_ADD && pvData)
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(((LPITEM) pvData)->GetWindow(), ((LPITEM) pvData)->GetCell());
#endif
		thecore_memcpy(&pack_exchg.alSockets, ((LPITEM) pvData)->GetSockets(), sizeof(pack_exchg.alSockets));
		thecore_memcpy(&pack_exchg.aAttr, ((LPITEM) pvData)->GetAttributes(), sizeof(pack_exchg.aAttr));
	}
	else
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(RESERVED_WINDOW, 0);
#endif
		memset(&pack_exchg.alSockets, 0, sizeof(pack_exchg.alSockets));
		memset(&pack_exchg.aAttr, 0, sizeof(pack_exchg.aAttr));
	}

	ch->GetDesc()->Packet(&pack_exchg, sizeof(pack_exchg));
}

bool CHARACTER::ExchangeStart(LPCHARACTER victim)
{
	if (this == victim)
		return false;

	if (IsObserverMode())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("관전 상태에서는 교환을 할 수 없습니다."));
		return false;
	}

	if (victim->IsNPC())
		return false;

	if (IsOpenSafebox() || GetShopOwner() || GetMyShop() || IsCubeOpen())
	{
		ChatPacket( CHAT_TYPE_INFO, LC_TEXT("다른 거래창이 열려있을경우 거래를 할수 없습니다." ) );
		return false;
	}

	if (victim->IsOpenSafebox() || victim->GetShopOwner() || victim->GetMyShop() || victim->IsCubeOpen())
	{
		ChatPacket( CHAT_TYPE_INFO, LC_TEXT("상대방이 다른 거래중이라 거래를 할수 없습니다." ) );
		return false;
	}


#ifdef NEW_PET_SYSTEM
	if (GetNewPetSystem()->IsActivePet())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("not open exchange window when new pet"));
		return false;
	}

	if (victim->GetNewPetSystem()->IsActivePet())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("not open victim exchange window when new pet"));
		return false;
	}
#endif

	int32_t iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (iDist >= EXCHANGE_MAX_DISTANCE)
		return false;

	if (GetExchange())
		return false;

	if (victim->GetExchange())
	{
		exchange_packet(this, EXCHANGE_SUBHEADER_GC_ALREADY, 0, 0, NPOS, 0);
		return false;
	}

	if (victim->IsBlockMode(BLOCK_EXCHANGE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 교환 거부 상태입니다."));
		return false;
	}

	SetExchange(M2_NEW CExchange(this));
	victim->SetExchange(M2_NEW CExchange(victim));

	victim->GetExchange()->SetCompany(GetExchange());
	GetExchange()->SetCompany(victim->GetExchange());

	SetExchangeTime();
	victim->SetExchangeTime();

	exchange_packet(victim, EXCHANGE_SUBHEADER_GC_START, 0, GetVID(), NPOS, 0);
	exchange_packet(this, EXCHANGE_SUBHEADER_GC_START, 0, victim->GetVID(), NPOS, 0);

	return true;
}

CExchange::CExchange(LPCHARACTER pOwner)
{
	m_pCompany = NULL;

	m_bAccept = false;

	for (int32_t i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		m_apItems[i] = NULL;
		m_aItemPos[i] = NPOS;
		m_abItemDisplayPos[i] = 0;
	}

	m_lGold = 0;
#ifdef ENABLE_CHEQUE_SYSTEM
	m_lCheque = 0;
#endif

	m_pOwner = pOwner;
	pOwner->SetExchange(this);

	m_pGrid = M2_NEW CGrid(4,3);
}

CExchange::~CExchange()
{
	M2_DELETE(m_pGrid);
}

bool CExchange::AddItem(TItemPos item_pos, uint8_t display_pos)
{
	assert(m_pOwner != NULL && GetCompany());

	if (!item_pos.IsValidItemPosition())
		return false;

	if (item_pos.IsEquipPosition())
		return false;

	LPITEM item;

	if (!(item = m_pOwner->GetItem(item_pos)))
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE))
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템을 건네줄 수 없습니다."));
		return false;
	}

	if (true == item->isLocked())
	{
		return false;
	}

	if (item->IsExchanging())
	{
		sys_log(0, "EXCHANGE under exchanging");
		return false;
	}

#ifdef __SOULBINDING_SYSTEM__
	if (item->IsBind() || item->IsUntilBind())
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, "You can't exchange this item because is binded!");
		return false;
	}
#endif

	if (!m_pGrid->IsEmpty(display_pos, 1, item->GetSize()))
	{
		sys_log(0, "EXCHANGE not empty item_pos %d %d %d", display_pos, 1, item->GetSize());
		return false;
	}

	Accept(false);
	GetCompany()->Accept(false);

	for (int32_t i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			continue;

		m_apItems[i]		= item;
		m_aItemPos[i]		= item_pos;
		m_abItemDisplayPos[i]	= display_pos;
		m_pGrid->Put(display_pos, 1, item->GetSize());

		item->SetExchanging(true);

		exchange_packet(m_pOwner, 
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				true,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		exchange_packet(GetCompany()->GetOwner(),
				EXCHANGE_SUBHEADER_GC_ITEM_ADD, 
				false, 
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		sys_log(0, "EXCHANGE AddItem success %s pos(%d, %d) %d", item->GetName(), item_pos.window_type, item_pos.cell, display_pos);

		return true;
	}

	return false;
}

bool CExchange::RemoveItem(uint8_t pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return false;

	if (!m_apItems[pos])
		return false;

	TItemPos PosOfInventory = m_aItemPos[pos];
	m_apItems[pos]->SetExchanging(false);

	m_pGrid->Get(m_abItemDisplayPos[pos], 1, m_apItems[pos]->GetSize());

	exchange_packet(GetOwner(),	EXCHANGE_SUBHEADER_GC_ITEM_DEL, true, pos, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ITEM_DEL, false, pos, PosOfInventory, 0);

	Accept(false);
	GetCompany()->Accept(false);

	m_apItems[pos]	    = NULL;
	m_aItemPos[pos]	    = NPOS;
	m_abItemDisplayPos[pos] = 0;
	return true;
}

bool CExchange::AddGold(int64_t gold)
{
	if (gold <= 0)
		return false;

	if (GetOwner()->GetGold() < gold)
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_GOLD, 0, 0, NPOS, 0);
		return false;
	}

	if ( LC_IsCanada() == true || LC_IsEurope() == true )
	{
		if ( m_lGold > 0 )
		{
			return false;
		}
	}

	Accept(false);
	GetCompany()->Accept(false);

	m_lGold = gold;

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, true, m_lGold, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, false, m_lGold, NPOS, 0);
	return true;
}

#ifdef ENABLE_CHEQUE_SYSTEM
bool CExchange::AddCheque(int32_t cheque)
{
	if (cheque <= 0)
		return false;
	if (GetOwner()->GetCheque() < cheque)
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_CHEQUE, 0, 0, NPOS, 0);
		return false;
	}
	Accept(false);
	GetCompany()->Accept(false);
	m_lCheque = cheque;
	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_CHEQUE_ADD, true, m_lCheque, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_CHEQUE_ADD, false, m_lCheque, NPOS, 0);
	return true;
}
#endif

bool CExchange::Check(int32_t * piItemCount)
{
	if (GetOwner()->GetGold() < m_lGold)
		return false;

#ifdef ENABLE_CHEQUE_SYSTEM
	if (GetOwner()->GetCheque() < m_lCheque)
		return false;
#endif
	int32_t item_count = 0;

	for (int32_t i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!m_apItems[i])
			continue;

		if (!m_aItemPos[i].IsValidItemPosition())
			return false;

		if (m_apItems[i] != GetOwner()->GetItem(m_aItemPos[i]))
			return false;

		++item_count;
	}

	*piItemCount = item_count;
	return true;
}

#ifdef ENABLE_EXTEND_INVEN_SYSTEM
bool CExchange::CheckSpace()
{	
	std::array<std::unique_ptr<CGrid>, static_cast<const int32_t>(INVENTORY_PAGE_COUNT)> s_grid;
	static std::vector <uint16_t> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);
	LPITEM item;
	bool bDSInitialized = false;
	int32_t new_size, invenpoint = GetCompany()->GetOwner()->Inven_Point();

	for (uint32_t i = 0; i < s_grid.size(); i++) {
		if (i < INVENTORY_OPEN_PAGE_COUNT) {
			s_grid[i] = std::make_unique<CGrid>(INVENTORY_WIDTH,INVENTORY_HEIGHT);
			continue;
		}
		if (invenpoint > 0 && invenpoint <= INVENTORY_HEIGHT)
			new_size = invenpoint;
		else if (invenpoint >= INVENTORY_HEIGHT)
			new_size = INVENTORY_HEIGHT;
		else
			new_size = 0;
		s_grid[i] = std::make_unique<CGrid>(INVENTORY_WIDTH,new_size);
		invenpoint -= INVENTORY_HEIGHT;
	}
	for (uint32_t j = 0; j < s_grid.size(); j++) {
		s_grid[j]->Clear();
		for (auto i = INVENTORY_PAGE_SIZE * j; i < INVENTORY_PAGE_SIZE * (j+1); ++i) 
		{
			if (!(item = GetCompany()->GetOwner()->GetInventoryItem(i))) 
				continue;
			s_grid[j]->Put(i - INVENTORY_PAGE_SIZE * j, 1, item->GetSize());
		}
	}
	for (auto i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i) 
	{
		if (!(item = m_apItems[i])) 
			continue;
		if (item->IsDragonSoul()) 
		{
			if (!GetCompany()->GetOwner()->DragonSoul_IsQualified()) 
				return false;
			if (!bDSInitialized) 
			{
				bDSInitialized = true;
				GetCompany()->GetOwner()->CopyDragonSoulItemGrid(s_vDSGrid);
			}
			bool bExistEmptySpace = false;
			uint16_t wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM) 
				return false;
			for (int32_t i = 0; i < DRAGON_SOUL_BOX_SIZE; i++) {
				uint16_t wPos = wBasePos + i;
				if (0 == s_vDSGrid[wBasePos]) 
				{
					bool bEmpty = true;
					for (int32_t j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM]) 
						{
							bEmpty = false;
							break;
						}
					} 
					if (bEmpty)	
					{
						for (int32_t j = 0; j < item->GetSize(); j++)
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] =  wPos + 1;
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace) 
					break;
            } 
			if (!bExistEmptySpace) 
				return false;
		}
		else 
		{
			bool existspace = false;
			for (uint32_t i = 0; i < s_grid.size(); i++)
			{
				const auto& iPos = s_grid[i]->FindBlank(1, item->GetSize());
				if (iPos >= 0) {
					s_grid[i]->Put(iPos, 1, item->GetSize());
					existspace = true;
					break;
				}
			}
			if (!existspace)
				return false;
		}
	}
	return true;
}

#else
bool CExchange::CheckSpace()
{
	static CGrid s_grid1(5, INVENTORY_MAX_NUM/5 / 4);
	static CGrid s_grid2(5, INVENTORY_MAX_NUM/5 / 4);
	static CGrid s_grid3(5, INVENTORY_MAX_NUM/5 / 4);
	static CGrid s_grid4(5, INVENTORY_MAX_NUM/5 / 4);

	s_grid1.Clear();
	s_grid2.Clear();
	s_grid3.Clear();
	s_grid4.Clear();

	LPCHARACTER	victim = GetCompany()->GetOwner();
	LPITEM item;

	int32_t i;

	for (i = 0; i < INVENTORY_MAX_NUM / 4; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid1.Put(i, 1, item->GetSize());
	}
	for (i = INVENTORY_MAX_NUM / 4; i < (INVENTORY_MAX_NUM / 4) * 2; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid2.Put(i - INVENTORY_MAX_NUM / 4, 1, item->GetSize());
	}
	for (i = (INVENTORY_MAX_NUM / 4) * 2; i < (INVENTORY_MAX_NUM / 4) * 3; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid3.Put(i - (INVENTORY_MAX_NUM / 4) * 2, 1, item->GetSize());
	}
	for (i = (INVENTORY_MAX_NUM / 4) * 3; i < (INVENTORY_MAX_NUM / 4) * 4; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid4.Put(i - (INVENTORY_MAX_NUM / 4) * 3, 1, item->GetSize());
	}

	static std::vector <uint16_t> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);

	bool bDSInitialized = false;

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
		{
			if (!victim->DragonSoul_IsQualified())
			{
				return false;
			}

			if (!bDSInitialized)
			{
				bDSInitialized = true;
				victim->CopyDragonSoulItemGrid(s_vDSGrid);
			}

			bool bExistEmptySpace = false;
			uint16_t wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			for (int32_t i = 0; i < DRAGON_SOUL_BOX_SIZE; i++)
			{
				uint16_t wPos = wBasePos + i;
				if (0 == s_vDSGrid[wBasePos])
				{
					bool bEmpty = true;
					for (int32_t j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM])
						{
							bEmpty = false;
							break;
						}
					}
					if (bEmpty)
					{
						for (int32_t j = 0; j < item->GetSize(); j++)
						{
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] = wPos + 1;
						}
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace)
					break;
			}
			if (!bExistEmptySpace)
				return false;
		}
		else
		{
			int32_t iPos = s_grid1.FindBlank(1, item->GetSize());

			if (iPos >= 0)
			{
				s_grid1.Put(iPos, 1, item->GetSize());
			}
			else
			{
				iPos = s_grid2.FindBlank(1, item->GetSize());

				if (iPos >= 0)
				{
					s_grid2.Put(iPos, 1, item->GetSize());
				}
				else
				{
					iPos = s_grid3.FindBlank(1, item->GetSize());

					if (iPos >= 0)
					{
						s_grid3.Put(iPos, 1, item->GetSize());
					}
					else
					{
						iPos = s_grid4.FindBlank(1, item->GetSize());

						if (iPos >= 0)
						{
							s_grid4.Put(iPos, 1, item->GetSize());
						}
						else
						{
							return false;
						}
					}
				}
			}
		}
	}
	return true;
}
#endif



bool CExchange::Done()
{
	int32_t		empty_pos, i;
	LPITEM	item;

	LPCHARACTER	victim = GetCompany()->GetOwner();

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
			empty_pos = victim->GetEmptyDragonSoulInventory(item);
		else
			empty_pos = victim->GetEmptyInventory(item->GetSize());

		if (empty_pos < 0)
		{
			sys_err("Exchange::Done : Cannot find blank position in inventory %s <-> %s item %s", 
					m_pOwner->GetName(), victim->GetName(), item->GetName());
			continue;
		}

		assert(empty_pos >= 0);

		if (item->GetVnum() == 90008 || item->GetVnum() == 90009)
		{
			VCardUse(m_pOwner, victim, item);
			continue;
		}

		int32_t iCount = item->GetCount();
		int32_t iCountBeforeFrom = GetOwner()->CountSpecifyItem(item->GetVnum());
		int32_t iCountAfterFrom = iCountBeforeFrom - iCount;
		int32_t iCountBeforeTo = victim->CountSpecifyItem(item->GetVnum());
		int32_t iCountAfterTo = iCountBeforeTo + iCount;



		m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);
		item->RemoveFromCharacter();
		
		if (item->IsDragonSoul())
		{
			item->AddToCharacter(victim, TItemPos(DRAGON_SOUL_INVENTORY, empty_pos));
		}
		else
		{
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		}
		
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		item->SetExchanging(false);
		{
			char exchange_buf[51];

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), GetOwner()->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(victim, item, "EXCHANGE_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), victim->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(GetOwner(), item, "EXCHANGE_GIVE", exchange_buf);

			if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
			{
				LogManager::instance().GoldBarLog(victim->GetPlayerID(), item->GetID(), EXCHANGE_TAKE, "");
				LogManager::instance().GoldBarLog(GetOwner()->GetPlayerID(), item->GetID(), EXCHANGE_GIVE, "");
			}

			LogManager::instance().Exchange(
				GetOwner()->GetPlayerID(),
				victim->GetPlayerID(),
				item->GetVnum(),
				iCountBeforeFrom,
				iCountAfterFrom,
				iCountBeforeTo,
				iCountAfterTo,
				iCount,
				GetOwner()->GetMapIndex(),
				GetOwner()->GetX(),
				GetOwner()->GetY(),
				GetOwner()->GetZ()
			);
		}

		m_apItems[i] = NULL;
	}

	if (m_lGold)
	{
		LogManager::instance().ExchangeGold(
			GetOwner()->GetPlayerID(),
			victim->GetPlayerID(),
			GetOwner()->GetGold(),
			GetOwner()->GetGold() - m_lGold,
			victim->GetGold(),
			victim->GetGold() + m_lGold,
			m_lGold,
			GetOwner()->GetMapIndex(),
			GetOwner()->GetX(),
			GetOwner()->GetY(),
			GetOwner()->GetZ()
		);

		GetOwner()->PointChange(POINT_GOLD, -m_lGold, true);
		victim->PointChange(POINT_GOLD, m_lGold, true);

		if (m_lGold > 1000)
		{
			char exchange_buf[51];
			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", GetOwner()->GetPlayerID(), GetOwner()->GetName());
			LogManager::instance().CharLog(victim, m_lGold, "EXCHANGE_GOLD_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", victim->GetPlayerID(), victim->GetName());
			LogManager::instance().CharLog(GetOwner(), m_lGold, "EXCHANGE_GOLD_GIVE", exchange_buf);
		}
	}


#ifdef ENABLE_CHEQUE_SYSTEM
	if (m_lCheque)
	{

		LogManager::instance().ExchangeCheque(
			GetOwner()->GetPlayerID(),
			victim->GetPlayerID(),
			GetOwner()->GetCheque(),
			GetOwner()->GetCheque() - m_lCheque,
			victim->GetCheque(),
			victim->GetCheque() + m_lCheque,
			m_lCheque,
			GetOwner()->GetMapIndex(),
			GetOwner()->GetX(),
			GetOwner()->GetY(),
			GetOwner()->GetZ()
		);

		GetOwner()->PointChange(POINT_CHEQUE, -m_lCheque, true);
		victim->PointChange(POINT_CHEQUE, m_lCheque, true);
	}
#endif

	m_pGrid->Clear();
	return true;
}

bool CExchange::Accept(bool bAccept)
{
	if (m_bAccept == bAccept)
		return true;

	m_bAccept = bAccept;

	if (m_bAccept && GetCompany()->m_bAccept)
	{
		int32_t	iItemCount;

		LPCHARACTER victim = GetCompany()->GetOwner();

		GetOwner()->SetExchangeTime();
		victim->SetExchangeTime();		

		if (!Check(&iItemCount))
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈이 부족하거나 아이템이 제자리에 없습니다."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 돈이 부족하거나 아이템이 제자리에 없습니다."));
			goto EXCHANGE_END;
		}

		if (!CheckSpace())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
			goto EXCHANGE_END;
		}

		if (!GetCompany()->Check(&iItemCount))
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈이 부족하거나 아이템이 제자리에 없습니다."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 돈이 부족하거나 아이템이 제자리에 없습니다."));
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpace())
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
			goto EXCHANGE_END;
		}

		if (db_clientdesc->GetSocket() == INVALID_SOCKET)
		{
			sys_err("Cannot use exchange feature while DB cache connection is dead.");
			victim->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			goto EXCHANGE_END;
		}

		if (Done())
		{
			if (m_lGold)
				GetOwner()->Save();


#ifdef ENABLE_CHEQUE_SYSTEM
			if (m_lCheque)
				GetOwner()->Save();
#endif

			if (GetCompany()->Done())
			{
				if (GetCompany()->m_lGold)
					victim->Save();


#ifdef ENABLE_CHEQUE_SYSTEM
				if (GetCompany()->m_lCheque)
					victim->Save();
#endif

				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님과의 교환이 성사 되었습니다."), victim->GetName());
				victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님과의 교환이 성사 되었습니다."), GetOwner()->GetName());
			}
		}

EXCHANGE_END:
		Cancel();
		return false;
	}
	else
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, true, m_bAccept, NPOS, 0);
		exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, false, m_bAccept, NPOS, 0);
		return true;
	}
}

void CExchange::Cancel()
{
	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_END, 0, 0, NPOS, 0);
	GetOwner()->SetExchange(NULL);

	for (int32_t i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			m_apItems[i]->SetExchanging(false);
	}

	if (GetCompany())
	{
		GetCompany()->SetCompany(NULL);
		GetCompany()->Cancel();
	}

	M2_DELETE(this);
}
