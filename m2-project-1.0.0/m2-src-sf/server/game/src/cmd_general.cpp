#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"

#ifdef NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif

#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "dev_log.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "threeway_war.h"
#include "log.h"
#include "../../common/VnumHelper.h"

#ifdef __AUCTION__
#include "auction_manager.h"
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#include "MountSystem.h"
#endif

extern int32_t g_server_id;

extern int32_t g_nPortalLimitTime;

ACMD(do_user_horse_ride)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		if (ch->GetMountVnum())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 탈것을 이용중입니다."));
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 먼저 소환해주세요."));
			return;
		}

		ch->StartRiding();
	}
	else
	{
		ch->StopRiding();
	}
}

ACMD(do_user_horse_back)
{
	if (ch->GetHorse() != NULL)
	{
		ch->HorseSummon(false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 돌려보냈습니다."));
	}
	else if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말에서 먼저 내려야 합니다."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 먼저 소환해주세요."));
	}
}

ACMD(do_user_horse_feed)
{
	if (ch->GetMyShop())
		return;

	if (ch->GetHorse() == NULL)
	{
		if (ch->IsHorseRiding() == false)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 먼저 소환해주세요."));
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 탄 상태에서는 먹이를 줄 수 없습니다."));
		return;
	}

	uint32_t dwFood = ch->GetHorseGrade() + 50054 - 1;

	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말에게 %s%s 주었습니다."), 
				ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName,
				g_iUseLocale ? "" : under_han(ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName) ? LC_TEXT("을") : LC_TEXT("를"));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 아이템이 필요합니다"), ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName);
	}
}

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int32_t		subcmd;
	int32_t         	left_second;
	char		szReason[MAX_REASON_LEN];

	TimedEventInfo() 
	: ch()
	, subcmd( 0 )
	, left_second( 0 )
	{
		::memset( szReason, 0, MAX_REASON_LEN );
	}
};

struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int32_t seconds;

	shutdown_event_data()
	: seconds( 0 )
	{}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>( event->info );

	if ( info == NULL )
	{
		sys_err( "shutdown_event> <Factor> Null pointer" );
		return 0;
	}

	int32_t * pSec = & (info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (--*pSec == -10)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--*pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf(buf, sizeof(buf), LC_TEXT("셧다운이 %d초 남았습니다."), *pSec);
		SendNotice(buf);

		--*pSec;
		return passes_per_sec;
	}
}

void Shutdown(int32_t iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();

	if(iSec != 0)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), LC_TEXT("%d초 후 게임이 셧다운 됩니다."), iSec);

		SendNotice(buf);
	}
	
	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	if (NULL == ch)
	{
		sys_err("Accept shutdown command from %s.", ch->GetName());
	}
	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(60);
}

EVENTFUNC(timed_event)
{
	TimedEventInfo * info = dynamic_cast<TimedEventInfo *>( event->info );
	
	if ( info == NULL )
	{
		sys_err( "timed_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) {
		return 0;
	}
	LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkTimedEvent = NULL;

		if (true == LC_IsEurope() || true == LC_IsYMIR() || true == LC_IsKorea())
		{
			switch (info->subcmd)
			{
				case SCMD_LOGOUT:
				case SCMD_QUIT:
				
				
				case SCMD_PHASE_SELECT:
					{
						TPacketNeedLoginLogInfo acc_info;
						acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;

						db_clientdesc->DBPacket( HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info) );

						LogManager::instance().DetailLoginLog( false, ch );
					}
					break;
			}
		}

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
				if (d)
					d->SetPhase(PHASE_CLOSE);
				break;

			case SCMD_QUIT:
				ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
				break;

			case SCMD_PHASE_SELECT:
				{
					ch->Disconnect("timed_event - SCMD_PHASE_SELECT");

					if (d)
					{
						d->SetPhase(PHASE_SELECT);
					}
				}
				break;
		}

		return 0;
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d초 남았습니다."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{


	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("취소 되었습니다."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("로그인 화면으로 돌아 갑니다. 잠시만 기다리세요."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("게임을 종료 합니다. 잠시만 기다리세요."));
			break;

		case SCMD_PHASE_SELECT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("캐릭터를 전환 합니다. 잠시만 기다리세요."));
			break;
	}

	int32_t nExitLimitTime = 10;

	if (ch->IsHack(false, true, nExitLimitTime) &&
		false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()) &&
	   	(!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}
	
	switch (subcmd)
	{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
			{
				TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

				{
					if (ch->IsPosition(POS_FIGHTING))
						info->left_second = 10;
					else
						info->left_second = 3;
				}

				info->ch		= ch;
				info->subcmd		= subcmd;
				strlcpy(info->szReason, argument, sizeof(info->szReason));

				ch->m_pkTimedEvent	= event_create(timed_event, info, 1);
			}
			break;
	}
}

ACMD(do_mount)
{
	
}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	int32_t iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (subcmd != SCMD_RESTART_TOWN && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		if (!test_server)
		{
			if (ch->IsHack())
			{
				if (false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 재시작 할 수 없습니다. (%d초 남음)"), iTimeToDead - (180 - g_nPortalLimitTime));
					return;
				}
			}

			if (iTimeToDead > 170)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 재시작 할 수 없습니다. (%d초 남음)"), iTimeToDead - 170);
				return;
			}
		}
	}

	if (subcmd == SCMD_RESTART_TOWN)
	{
		if (ch->IsHack())
		{
			if ((!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG) ||
			   	false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 재시작 할 수 없습니다. (%d초 남음)"), iTimeToDead - (180 - g_nPortalLimitTime));
				return;
			}
		}

		if (iTimeToDead > 173)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 마을에서 재시작 할 수 없습니다. (%d 초 남음)"), iTimeToDead - 173);
			return;
		}
	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
	ch->StartRecoveryEvent();

	if (1 == quest::CQuestManager::instance().GetEventFlag("threeway_war"))
	{
		if (subcmd == SCMD_RESTART_TOWN || subcmd == SCMD_RESTART_HERE)
		{
			if (true == CThreeWayWar::instance().IsThreeWayWarMapIndex(ch->GetMapIndex()) &&
					false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

				ch->ReviveInvisible(5);

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				ch->CheckMount();
#endif

				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());

				return;
			}

			if (true == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				if (CThreeWayWar::instance().GetReviveTokenForPlayer(ch->GetPlayerID()) <= 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성지에서 부활 기회를 모두 잃었습니다! 마을로 이동합니다!"));
					ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
				}
				else
				{
					ch->Show(ch->GetMapIndex(), GetSungziStartX(ch->GetEmpire()), GetSungziStartY(ch->GetEmpire()));
				}

				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ReviveInvisible(5);

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				ch->CheckMount();
#endif

				return;
			}
		}
	}

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap * pMap = ch->GetWarMap();
		uint32_t dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
				case SCMD_RESTART_TOWN:
					sys_log(0, "do_restart: restart town");
					PIXEL_POSITION pos;

					if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
						ch->Show(ch->GetMapIndex(), pos.x, pos.y);
					else
						ch->ExitToSavedLocation();

					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
	
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					ch->CheckMount();
#endif

					break;

				case SCMD_RESTART_HERE:
					sys_log(0, "do_restart: restart here");
					ch->RestartAtSamePos();
					
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					ch->CheckMount();
#endif

					break;
			}

			return;
		}
	}

	switch (subcmd)
	{
		case SCMD_RESTART_TOWN:
			sys_log(0, "do_restart: restart town");
			PIXEL_POSITION pos;

			if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
				ch->WarpSet(pos.x, pos.y);
			else
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(1);
			break;

		case SCMD_RESTART_HERE:
			sys_log(0, "do_restart: restart here");
			ch->RestartAtSamePos();
			
			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(0);
			ch->ReviveInvisible(5);

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				ch->CheckMount();
#endif

			break;
	}
}

#define MAX_STAT 128

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

#ifdef __GAYA__
ACMD(do_gaya_system)
{
	char arg1[255];
	char arg2[255];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (0 == arg1[0])
		return;

	const std::string& strArg1 = std::string(arg1);

	if (strArg1 == "craft"){

		if (0 == arg2[0])
			return;

		int32_t slot = atoi(arg2);
		ch->CraftGayaItems(slot);

	}
	else if(strArg1 == "market"){

		if (0 == arg2[0])
			return;

		int32_t slot = atoi(arg2);
		ch->MarketGayaItems(slot);

	}
	else if(strArg1 == "refresh")
	{
		ch->RefreshGayaItems();
	}
}
#endif

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	uint8_t idx = 0;
	
	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + 1);
	ch->SetPoint(idx, ch->GetPoint(idx) + 1);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -1);
	ch->ComputePoints();
}

ACMD(do_pvp)
{
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	uint32_t vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 대련중입니다."));
		return;
	}

	CPVPManager::instance().Insert(ch, pkVictim);
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드에 속해있지 않습니다."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		uint32_t vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 스킬 레벨을 변경할 권한이 없습니다."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint32_t vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch(vnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
			case SKILL_ADD_HP:
			case SKILL_RESIST_PENETRATE:
#ifdef __7AND8TH_SKILLS__
			case SKILL_ANTI_PALBANG:
			case SKILL_ANTI_AMSEOP:
			case SKILL_ANTI_SWAERYUNG:
			case SKILL_ANTI_YONGBI:
			case SKILL_ANTI_GIGONGCHAM:
			case SKILL_ANTI_HWAJO:
			case SKILL_ANTI_MARYUNG:
			case SKILL_ANTI_BYEURAK:
#ifdef __WOLFMAN_CHARACTER__
			case SKILL_ANTI_SALPOONG:
#endif
			case SKILL_HELP_PALBANG:
			case SKILL_HELP_AMSEOP:
			case SKILL_HELP_SWAERYUNG:
			case SKILL_HELP_YONGBI:
			case SKILL_HELP_GIGONGCHAM:
			case SKILL_HELP_HWAJO:
			case SKILL_HELP_MARYUNG:
			case SKILL_HELP_BYEURAK:
#ifdef __WOLFMAN_CHARACTER__
			case SKILL_HELP_SALPOONG:
#endif
#endif
				ch->SkillLevelUp(vnum);
				break;
		}
	}
}

ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	if (!*arg2 || strlen(arg2)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	if (LC_IsBrazil() == true)
	{
		for (int32_t i = 0; i < 6; ++i)
		{
			if (arg2[i] == '\0')
				break;

			if (isalpha(arg2[i]) == false)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 비밀번호는 영문자만 가능합니다."));
				return;
			}
		}
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	int32_t iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고가 이미 열려있습니다."));
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고를 닫은지 10초 안에는 열 수 없습니다."));
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던전 안에서는 파티에서 나갈 수 없습니다."));
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->GetMemberCount() == 2)
	{
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 나가셨습니다."));
		
		pParty->Quit(ch->GetPlayerID());
		
	}
}

ACMD(do_close_shop)
{
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

ACMD(do_channel_switch)
{

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
			return;
	
	int32_t new_ch;
	str_to_number(new_ch, arg1);
	if( new_ch <1 || new_ch > 3)
		return;
	if (!ch->IsPC())
		return;
	ch->ChannelSwitch(new_ch);
	
}

ACMD(do_war)
{
	CGuild * g = ch->GetGuild();

	if (!g)
		return;

	if (g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 다른 전쟁에 참전 중 입니다."));
		return;
	}

	char arg1[256], arg2[256];
	uint32_t type = GUILD_WAR_TYPE_FIELD;
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
            
		if(type < 0)
			return;
	}

	uint32_t gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
		return;
	}

	CGuild * opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그런 길드가 없습니다."));
		return;
	}

	switch (g->GetGuildWarState(opp_g->GetID()))
	{
		case GUILD_WAR_NONE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드가 이미 전쟁 중 입니다."));
					return;
				}

				int32_t iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

				if (g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 전비가 부족하여 길드전을 할 수 없습니다."));
					return;
				}

				if (opp_g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 전비가 부족하여 길드전을 할 수 없습니다."));
					return;
				}
			}
			break;

		case GUILD_WAR_SEND_DECLARE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 선전포고 중인 길드입니다."));
				return;
			}
			break;

		case GUILD_WAR_RECV_DECLARE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드가 이미 전쟁 중 입니다."));
					g->RequestRefuseWar(opp_g->GetID());
					return;
				}
			}
			break;

		case GUILD_WAR_RESERVE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 전쟁이 예약된 길드 입니다."));
				return;
			}
			break;

		case GUILD_WAR_END:
			return;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 전쟁 중인 길드입니다."));
			g->RequestRefuseWar(opp_g->GetID());
			return;
	}

	if (!g->CanStartWar(type))
	{
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 레더 점수가 모자라서 길드전을 할 수 없습니다."));
			sys_log(0, "GuildWar.StartError.NEED_LADDER_POINT");
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전을 하기 위해선 최소한 %d명이 있어야 합니다."), GUILD_WAR_MIN_MEMBER_COUNT);
			sys_log(0, "GuildWar.StartError.NEED_MINIMUM_MEMBER[%d]", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		else
		{
			sys_log(0, "GuildWar.StartError.UNKNOWN_ERROR");
		}
		return;
	}

	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 레더 점수가 모자라서 길드전을 할 수 없습니다."));
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 길드원 수가 부족하여 길드전을 할 수 없습니다."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 길드장이 접속중이 아닙니다."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;
		
		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());
		
		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 길드장이 접속중이 아닙니다."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint32_t gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그런 길드가 없습니다."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

ACMD(do_detaillog)
{
	ch->DetailLog();
}

ACMD(do_monsterlog)
{
	ch->ToggleMonsterLog();
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint8_t mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);

	if (answer != 'y')
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님으로 부터 친구 등록을 거부 당했습니다."), ch->GetName());
	}

	MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, answer == 'y' ? false : true); 
}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		uint8_t flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if(ch->GetWear(WEAR_COSTUME_MOUNT))
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		uint32_t mobVnum = 0;
		
		if (!mountSystem && !mount)
			return;
		
#ifdef __CHANGELOOK_SYSTEM__	
		if(mount->GetTransmutation())
		{
			const TItemTable* itemTable = ITEM_MANAGER::instance().GetTable(mount->GetTransmutation());
			
			if (itemTable)
				mobVnum = itemTable->alValues[1];
			else
				mobVnum = mount->GetValue(1);
		}
		else
			mobVnum = mount->GetValue(1);
#else
		if(mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);
#endif

		if (ch->GetMountVnum())
		{
			if(mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		return;
	}
#endif
	
	LPITEM item = ch->GetWear(WEAR_UNIQUE1);
	LPITEM item2 = ch->GetWear(WEAR_UNIQUE2);
	LPITEM item3 = ch->GetWear(WEAR_COSTUME_MOUNT);

	if (item && item->IsRideItem())
		ch->UnequipItem(item);

	if (item2 && item2->IsRideItem())
		ch->UnequipItem(item2);

	if (item3 && item3->IsRideItem())
		ch->UnequipItem(item3);

	if (true == ch->UnEquipSpecialRideUniqueItem())
	{
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);

		if (ch->IsHorseRiding())
		{
			ch->StopRiding();
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("AI??Aa리? ?E A?� ??릴 ?o ???A??."));
	}

}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (ch->GetGMLevel() <= GM_PLAYER)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		uint32_t vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

		if (!tch->IsPC())
			return;
		
		tch->SendEquipment(ch);
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 파티에 속해 있으므로 가입신청을 할 수 없습니다."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint32_t vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint32_t vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint32_t vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

ACMD(do_monarch_warpto)
{
	if (true == LC_IsYMIR() || true == LC_IsKorea())
		return;

	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용 가능한 기능입니다"));
		return;
	}
	
	if (!ch->IsMCOK(CHARACTER::MI_WARP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_WARP));
		return;
	}

	const int32_t WarpPrice = 10000;
	
	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int32_t NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, WarpPrice);
		return;	
	}

	int32_t x = 0, y = 0;
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용법: warpto <character name>"));
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("타제국 유저에게는 이동할수 없습니다"));
				return;
			}

			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("해당 유저는 %d 채널에 있습니다. (현재 채널 %d)"), pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
				return;
			}

			PIXEL_POSITION pos;
	
			if (!SECTREE_MANAGER::instance().GetCenterPositionOfMap(pkCCI->lMapIndex, pos))
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pkCCI->lMapIndex);
			else
			{
				
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 에게로 이동합니다"), arg1);
				ch->WarpSet(pos.x, pos.y);
				
				CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

				ch->SetMC(CHARACTER::MI_WARP);
			}
		}
		else if (NULL == CHARACTER_MANAGER::instance().FindPC(arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
		}

		return;
	}
	else
	{
		if (tch->GetEmpire() != ch->GetEmpire())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("타제국 유저에게는 이동할수 없습니다"));
			return;
		}
		if (!IsMonarchWarpZone(tch->GetMapIndex()))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
			return;
		}
		x = tch->GetX();
		y = tch->GetY();
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 에게로 이동합니다"), arg1);
	ch->WarpSet(x, y);
	ch->Stop();

	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	ch->SetMC(CHARACTER::MI_WARP);
}

ACMD(do_monarch_transfer)
{
	if (true == LC_IsYMIR() || true == LC_IsKorea())
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용법: transfer <name>"));
		return;
	}
	
	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용 가능한 기능입니다"));
		return;
	}
	
	if (!ch->IsMCOK(CHARACTER::MI_TRANSFER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_TRANSFER));	
		return;
	}

	const int32_t WarpPrice = 10000;

	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int32_t NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, WarpPrice);
		return;	
	}


	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 제국 유저는 소환할 수 없습니다."));
				return;
			}
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님은 %d 채널에 접속 중 입니다. (현재 채널: %d)"), arg1, pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
				return;
			}
			if (!IsMonarchWarpZone(ch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 소환할 수 없습니다."));
				return;
			}

			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님을 소환하였습니다."), arg1);
			
			CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
	
			ch->SetMC(CHARACTER::MI_TRANSFER);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("입력하신 이름을 가진 사용자가 없습니다."));
		}

		return;
	}


	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("자신을 소환할 수 없습니다."));
		return;
	}

	if (tch->GetEmpire() != ch->GetEmpire())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 제국 유저는 소환할 수 없습니다."));
		return;
	}
	if (!IsMonarchWarpZone(tch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
		return;
	}
	if (!IsMonarchWarpZone(ch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 소환할 수 없습니다."));
		return;
	}

	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
		
	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	ch->SetMC(CHARACTER::MI_TRANSFER);
}

ACMD(do_monarch_info)
{
	if (CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))	
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("나의 군주 정보"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int32_t n = 1; n < 4; ++n)
		{
			if (n == ch->GetEmpire())
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s군주] : %s  보유금액 %lld "), EMPIRE_NAME(n), p->name[n], p->money[n]);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s군주] : %s  "), EMPIRE_NAME(n), p->name[n]);
				
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주 정보"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int32_t n = 1; n < 4; ++n)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s군주] : %s  "), EMPIRE_NAME(n), p->name[n]);
				
		}
	}
	
}	

ACMD(do_elect)
{
	db_clientdesc->DBPacketHeader(HEADER_GD_COME_TO_VOTE, ch->GetDesc()->GetHandle(), 0);
}

struct GotoInfo
{
	std::string 	st_name;

	uint8_t 	empire;
	int32_t 	mapIndex;
	uint32_t 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

extern void BroadcastNotice(const char * c_pszBuf);

ACMD(do_monarch_tax)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: monarch_tax <1-50>");
		return;
	}

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용할수 있는 기능입니다"));
		return;
	}

	int32_t tax = 0;
	str_to_number(tax,  arg1);

	if (tax < 1 || tax > 50)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1-50 사이의 수치를 선택해주세요"));

	quest::CQuestManager::instance().SetEventFlag("trade_tax", tax); 

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("세금이 %d %로 설정되었습니다"));

	char szMsg[1024];	

	snprintf(szMsg, sizeof(szMsg), "군주의 명으로 세금이 %d %% 로 변경되었습니다", tax);
	BroadcastNotice(szMsg);

	snprintf(szMsg, sizeof(szMsg), "앞으로는 거래 금액의 %d %% 가 국고로 들어가게됩니다.", tax);
	BroadcastNotice(szMsg);

	ch->SetMC(CHARACTER::MI_TAX); 
}

static const uint32_t cs_dwMonarchMobVnums[] =
{
	191,
	192,
	193,
	194,
	391,
	392,
	393,
	394,
	491,
	492,
	493,
	494,
	591,
	691,
	791,
	1304,
	1901,
	2091,
	2191,
	2206,
	0,
};

ACMD(do_monarch_mob)
{
	char arg1[256];
	LPCHARACTER	tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용할수 있는 기능입니다"));
		return;
	}
	
	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mmob <mob name>");
		return;
	}

	uint8_t pcEmpire = ch->GetEmpire();
	uint8_t mapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(ch->GetMapIndex());

	if (LC_IsYMIR() == true || LC_IsKorea() == true)
	{
		if (mapEmpire != pcEmpire && mapEmpire != 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("자국 영토에서만 사용할 수 있는 기능입니다"));
			return;
		}
	}

	const int32_t SummonPrice = 5000000;

	if (!ch->IsMCOK(CHARACTER::MI_SUMMON))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_SUMMON));	
		return;
	}
	
	if (!CMonarch::instance().IsMoneyOk(SummonPrice, ch->GetEmpire()))
	{
		int32_t NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, SummonPrice);
		return;	
	}

	const CMob * pkMob;
	uint32_t vnum = 0;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	uint32_t count;

	for (count = 0; cs_dwMonarchMobVnums[count] != 0; ++count)
		if (cs_dwMonarchMobVnums[count] == vnum)
			break;

	if (0 == cs_dwMonarchMobVnums[count])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소환할수 없는 몬스터 입니다. 소환가능한 몬스터는 홈페이지를 참조하세요"));
		return;
	}

	tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum, 
			ch->GetMapIndex(),
			ch->GetX() - number(200, 750), 
			ch->GetY() - number(200, 750), 
			ch->GetX() + number(200, 750), 
			ch->GetY() + number(200, 750), 
			true,
			pkMob->m_table.bType == CHAR_TYPE_STONE,
			true);

	if (tch)
	{
		CMonarch::instance().SendtoDBDecMoney(SummonPrice, ch->GetEmpire(), ch);

		ch->SetMC(CHARACTER::MI_SUMMON); 
	}
}

static const char* FN_point_string(int32_t apply_number)
{
	switch (apply_number)
	{
		case POINT_MAX_HP:	return LC_TEXT("최대 생명력 +%d");
		case POINT_MAX_SP:	return LC_TEXT("최대 정신력 +%d");
		case POINT_HT:		return LC_TEXT("체력 +%d");
		case POINT_IQ:		return LC_TEXT("지능 +%d");
		case POINT_ST:		return LC_TEXT("근력 +%d");
		case POINT_DX:		return LC_TEXT("민첩 +%d");
		case POINT_ATT_SPEED:	return LC_TEXT("공격속도 +%d");
		case POINT_MOV_SPEED:	return LC_TEXT("이동속도 %d");
		case POINT_CASTING_SPEED:	return LC_TEXT("쿨타임 -%d");
		case POINT_HP_REGEN:	return LC_TEXT("생명력 회복 +%d");
		case POINT_SP_REGEN:	return LC_TEXT("정신력 회복 +%d");
		case POINT_POISON_PCT:	return LC_TEXT("독공격 %d");
		case POINT_STUN_PCT:	return LC_TEXT("스턴 +%d");
		case POINT_SLOW_PCT:	return LC_TEXT("슬로우 +%d");
		case POINT_CRITICAL_PCT:	return LC_TEXT("%d%% 확률로 치명타 공격");
		case POINT_RESIST_CRITICAL:	return LC_TEXT("상대의 치명타 확률 %d%% 감소");
		case POINT_PENETRATE_PCT:	return LC_TEXT("%d%% 확률로 관통 공격");
		case POINT_RESIST_PENETRATE: return LC_TEXT("상대의 관통 공격 확률 %d%% 감소");
		case POINT_ATTBONUS_HUMAN:	return LC_TEXT("인간류 몬스터 타격치 +%d%%");
		case POINT_ATTBONUS_ANIMAL:	return LC_TEXT("동물류 몬스터 타격치 +%d%%");
		case POINT_ATTBONUS_ORC:	return LC_TEXT("웅귀족 타격치 +%d%%");
		case POINT_ATTBONUS_MILGYO:	return LC_TEXT("밀교류 타격치 +%d%%");
		case POINT_ATTBONUS_UNDEAD:	return LC_TEXT("시체류 타격치 +%d%%");
		case POINT_ATTBONUS_DEVIL:	return LC_TEXT("악마류 타격치 +%d%%");
		case POINT_STEAL_HP:		return LC_TEXT("타격치 %d%% 를 생명력으로 흡수");
		case POINT_STEAL_SP:		return LC_TEXT("타력치 %d%% 를 정신력으로 흡수");
		case POINT_MANA_BURN_PCT:	return LC_TEXT("%d%% 확률로 타격시 상대 전신력 소모");
		case POINT_DAMAGE_SP_RECOVER:	return LC_TEXT("%d%% 확률로 피해시 정신력 회복");
		case POINT_BLOCK:			return LC_TEXT("물리타격시 블럭 확률 %d%%");
		case POINT_DODGE:			return LC_TEXT("활 공격 회피 확률 %d%%");
		case POINT_RESIST_SWORD:	return LC_TEXT("한손검 방어 %d%%");
		case POINT_RESIST_TWOHAND:	return LC_TEXT("양손검 방어 %d%%");
		case POINT_RESIST_DAGGER:	return LC_TEXT("두손검 방어 %d%%");
		case POINT_RESIST_BELL:		return LC_TEXT("방울 방어 %d%%");
		case POINT_RESIST_FAN:		return LC_TEXT("부채 방어 %d%%");
		case POINT_RESIST_BOW:		return LC_TEXT("활공격 저항 %d%%");
		case POINT_RESIST_FIRE:		return LC_TEXT("화염 저항 %d%%");
		case POINT_RESIST_ELEC:		return LC_TEXT("전기 저항 %d%%");
		case POINT_RESIST_MAGIC:	return LC_TEXT("마법 저항 %d%%");
		case POINT_RESIST_WIND:		return LC_TEXT("바람 저항 %d%%");
		case POINT_RESIST_ICE:		return LC_TEXT("냉기 저항 %d%%");
		case POINT_RESIST_EARTH:	return LC_TEXT("대지 저항 %d%%");
		case POINT_RESIST_DARK:		return LC_TEXT("어둠 저항 %d%%");
		case POINT_REFLECT_MELEE:	return LC_TEXT("직접 타격치 반사 확률 : %d%%");
		case POINT_REFLECT_CURSE:	return LC_TEXT("저주 되돌리기 확률 %d%%");
		case POINT_POISON_REDUCE:	return LC_TEXT("독 저항 %d%%");
		case POINT_KILL_SP_RECOVER:	return LC_TEXT("%d%% 확률로 적퇴치시 정신력 회복");
		case POINT_EXP_DOUBLE_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 경험치 추가 상승");
		case POINT_GOLD_DOUBLE_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 돈 2배 드롭");
		case POINT_ITEM_DROP_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 아이템 2배 드롭");
		case POINT_POTION_BONUS:	return LC_TEXT("물약 사용시 %d%% 성능 증가");
		case POINT_KILL_HP_RECOVERY:	return LC_TEXT("%d%% 확률로 적퇴치시 생명력 회복");





		case POINT_ATT_GRADE_BONUS:	return LC_TEXT("공격력 +%d");
		case POINT_DEF_GRADE_BONUS:	return LC_TEXT("방어력 +%d");
		case POINT_MAGIC_ATT_GRADE:	return LC_TEXT("마법 공격력 +%d");
		case POINT_MAGIC_DEF_GRADE:	return LC_TEXT("마법 방어력 +%d");

		case POINT_MAX_STAMINA:	return LC_TEXT("최대 지구력 +%d");
		case POINT_ATTBONUS_WARRIOR:	return LC_TEXT("무사에게 강함 +%d%%");
		case POINT_ATTBONUS_ASSASSIN:	return LC_TEXT("자객에게 강함 +%d%%");
		case POINT_ATTBONUS_SURA:		return LC_TEXT("수라에게 강함 +%d%%");
		case POINT_ATTBONUS_SHAMAN:		return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_MONSTER:	return LC_TEXT("몬스터에게 강함 +%d%%");
		case POINT_MALL_ATTBONUS:		return LC_TEXT("공격력 +%d%%");
		case POINT_MALL_DEFBONUS:		return LC_TEXT("방어력 +%d%%");
		case POINT_MALL_EXPBONUS:		return LC_TEXT("경험치 %d%%");
		case POINT_MALL_ITEMBONUS:		return LC_TEXT("아이템 드롭율 %.1f배");
		case POINT_MALL_GOLDBONUS:		return LC_TEXT("돈 드롭율 %.1f배");
		case POINT_MAX_HP_PCT:			return LC_TEXT("최대 생명력 +%d%%");
		case POINT_MAX_SP_PCT:			return LC_TEXT("최대 정신력 +%d%%");
		case POINT_SKILL_DAMAGE_BONUS:	return LC_TEXT("스킬 데미지 %d%%");
		case POINT_NORMAL_HIT_DAMAGE_BONUS:	return LC_TEXT("평타 데미지 %d%%");
		case POINT_SKILL_DEFEND_BONUS:		return LC_TEXT("스킬 데미지 저항 %d%%");
		case POINT_NORMAL_HIT_DEFEND_BONUS:	return LC_TEXT("평타 데미지 저항 %d%%");



		case POINT_RESIST_WARRIOR:	return LC_TEXT("무사공격에 %d%% 저항");
		case POINT_RESIST_ASSASSIN:	return LC_TEXT("자객공격에 %d%% 저항");
		case POINT_RESIST_SURA:		return LC_TEXT("수라공격에 %d%% 저항");
		case POINT_RESIST_SHAMAN:	return LC_TEXT("무당공격에 %d%% 저항");
#ifdef __ANTI_RESIST_MAGIC_BONUS__
		case POINT_ANTI_RESIST_MAGIC:
			return LC_TEXT("Anti Magic Resistance: %d%%");
#endif
		default:					return NULL;
	}
}

static bool FN_hair_affect_string(LPCHARACTER ch, char *buf, uint32_t bufsiz)
{
	if (NULL == ch || NULL == buf)
		return false;

	CAffect* aff = NULL;
	time_t expire = 0;
	struct tm ltm;
	int32_t	year, mon, day;
	int32_t	offset = 0;

	aff = ch->FindAffect(AFFECT_HAIR);

	if (NULL == aff)
		return false;

	expire = ch->GetQuestFlag("hair.limit_time");

	if (expire < get_global_time())
		return false;

	offset = snprintf(buf, bufsiz, FN_point_string(aff->bApplyOn), aff->lApplyValue);

	if (offset < 0 || offset >= (int32_t) bufsiz)
		offset = bufsiz - 1;

	localtime_r(&expire, &ltm);

	year	= ltm.tm_year + 1900;
	mon		= ltm.tm_mon + 1;
	day		= ltm.tm_mday;

	snprintf(buf + offset, bufsiz - offset, LC_TEXT(" (만료일 : %d년 %d월 %d일)"), year, mon, day);

	return true;
}

ACMD(do_costume)
{
	#ifdef __ACCE_SYSTEM__
	char buf[768];
	#else
	char buf[512];
	#endif
	
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	
	CItem * pBody = ch->GetWear(WEAR_COSTUME_BODY);
	CItem * pHair = ch->GetWear(WEAR_COSTUME_HAIR);
	#ifdef __ACCE_SYSTEM__
	CItem * pAcce = ch->GetWear(WEAR_COSTUME_ACCE);
	#endif
	
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	CItem* pMount = ch->GetWear(WEAR_COSTUME_MOUNT);
#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "COSTUME status:");
	if (pBody)
	{
		const char* itemName = pBody->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  BODY: %s", itemName);
		
		if (pBody->IsEquipped() && arg1[0] == 'b')
			ch->UnequipItem(pBody);
	}
	
	if (pHair)
	{
		const char* itemName = pHair->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  HAIR: %s", itemName);

		for (int32_t i = 0; i < pHair->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pHair->GetAttribute(i);
			if (attr.bType > 0)
			{
				const char * pAttrName = FN_point_string(attr.bType);
				if (pAttrName == NULL)
					continue;
				
				snprintf(buf, sizeof(buf), FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}

		if (pHair->IsEquipped() && arg1[0] == 'h')
			ch->UnequipItem(pHair);
	}
	
	#ifdef __ACCE_SYSTEM__
	if (pAcce)
	{
		const char * itemName = pAcce->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  ACCE: %s", itemName);
		for (int32_t i = 0; i < pAcce->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pAcce->GetAttribute(i);
			if (attr.bType > 0)
			{
				const char * pAttrName = FN_point_string(attr.bType);
				if (pAttrName == NULL)
					continue;
				
				snprintf(buf, sizeof(buf), FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}

		if (pAcce->IsEquipped() && arg1[0] == 's')
			ch->UnequipItem(pAcce);
	}
	#endif
	
	
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (pMount)
	{
		const char* itemName = pMount->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  MOUNT : %s", itemName);

		if (pMount->IsEquipped() && arg1[0] == 'm')
			ch->UnequipItem(pMount);
	}
#endif
}

ACMD(do_hair)
{
	char buf[256];

	if (false == FN_hair_affect_string(ch, buf, sizeof(buf)))
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

#ifdef ENABLE_NEW_PET_SYSTEM_FEED
ACMD(do_feedpetitem)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (ch->IsDead() || ch->GetShopOwner() || ch->GetExchange() || ch->GetMyShop() || ch->IsCubeOpen() || ch->IsOpenSafebox() || ch->GetShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("PENCERE_KAPAT"));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	int32_t itemslot = 0;
	str_to_number(itemslot, arg1);
	if (!itemslot)
		return;

	if (itemslot >= 0 && itemslot <= 1000)
	{
		if (ch->GetNewPetSystem()->IsActivePet()) {
			ch->GetNewPetSystem()->ItemFeed(itemslot);
		}
	}
}
#endif

ACMD(do_inventory)
{
	int32_t	index = 0;
	int32_t	count		= 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int32_t i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s",
						index, item ? item->GetName() : "<NONE>");
		++index;
	}
}

ACMD(do_gift)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift");
}



#ifdef ENABLE_PET_ATTR_DETERMINE
ACMD(do_determine_pet)
{
	if (!ch)
		return;

	if (!ch->IsPC())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHack())
		return;

	if (ch->CountSpecifyItem(55032) < 1)
		return;

	if (ch->GetNewPetSystem()->IsActivePet())
	{
		ch->RemoveSpecifyItem(55032, 1);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "OnResultPetAttrDetermine %d", ch->GetNewPetSystem()->GetPetType());
	}
}

ACMD(do_change_pet)
{
	char arg1[256], arg2[256], arg3[256], arg4[256];
	four_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
		return;

	int32_t firstInvenType = atoi(arg1);
	int32_t firstInvenPos = atoi(arg2);
	int32_t secondInvenType = atoi(arg3);
	int32_t secondInvenPos = atoi(arg4);

	if (firstInvenType < 0)
		return;

	if (secondInvenType < 0)
		return;

	if (firstInvenType == INVENTORY)
	{
		if (firstInvenPos >= ch->Inventory_Size())
			return;
	}
	else
	{
		if (firstInvenPos >= 225)
			return;
	}

	if (secondInvenType == INVENTORY)
	{
		if (secondInvenPos >= ch->Inventory_Size())
			return;
	}
	else
	{
		if (secondInvenPos >= 225)
			return;
	}

	LPITEM petItem = ch->GetItem(TItemPos(firstInvenType, firstInvenPos));
	LPITEM secondItem = ch->GetItem(TItemPos(secondInvenType, secondInvenPos));

	if (!petItem)
		return;

	if (!secondItem)
		return;

	if (secondItem->GetVnum() != 55033)
		return;

	int32_t	m_dwbonuspet[3][2];

	for (int32_t x = 0; x < 3; ++x)
	{
		int32_t btype[3] = { 1, 54, 53 };
		m_dwbonuspet[x][0] = btype[x];
		m_dwbonuspet[x][1] = 0;
	}

	int32_t newpettype = number(0, 6);
	int32_t level = petItem->GetSocket(1);
	int32_t age = petItem->GetSocket(3);
	int32_t tmpage = (time(0) + age) / 86400;

	if (newpettype == 0)
	{
		m_dwbonuspet[0][1] = number(3, 3);
		m_dwbonuspet[1][1] = number(3, 3);
		m_dwbonuspet[2][1] = number(3, 3);
		for (int32_t i = 0; i < level; ++i)
		{
			int32_t lvbonus = 0;
			if (level >= 60)
				lvbonus = 1;
			if (i % 5 == 0)
			{
				m_dwbonuspet[0][1] += number(1 + lvbonus, 5 + lvbonus);
				m_dwbonuspet[2][1] += number(1 + lvbonus, 5 + lvbonus);
			}
			if (i % 7 == 0)
			{
				m_dwbonuspet[1][1] += number(1 + lvbonus, 5 + lvbonus);
			}
		}
	}
	else if (newpettype == 1)
	{
		m_dwbonuspet[0][1] = number(3, 3);
		m_dwbonuspet[1][1] = number(3, 3);
		m_dwbonuspet[2][1] = number(3, 3);
		for (int32_t i = 0; i < level; ++i)
		{
			int32_t lvbonus = 0;
			if (tmpage >= 60)
				lvbonus = 1;
			if (i % 5 == 0)
			{
				m_dwbonuspet[0][1] += number(2 + lvbonus, 3 + lvbonus);
				m_dwbonuspet[2][1] += number(2 + lvbonus, 3 + lvbonus);
			}
			if (i % 7 == 0)
			{
				m_dwbonuspet[1][1] += number(2 + lvbonus, 3 + lvbonus);
			}
		}
	}
	else if (newpettype == 2)
	{
		m_dwbonuspet[0][1] = number(3, 3);
		m_dwbonuspet[1][1] = number(3, 3);
		m_dwbonuspet[2][1] = number(3, 3);
		for (int i = 0; i < level; ++i)
		{
			int lvbonus = 0;
			if (level >= 60)
				lvbonus = 1;
			if (i % 5 == 0)
			{
				m_dwbonuspet[0][1] += number(1 + lvbonus, 7 + lvbonus);
				m_dwbonuspet[2][1] += number(1 + lvbonus, 7 + lvbonus);
			}
			if (i % 7 == 0)
			{
				m_dwbonuspet[1][1] += number(1 + lvbonus, 7 + lvbonus);
			}
		}
	}
	else if (newpettype == 3)
	{
		m_dwbonuspet[0][1] = number(3, 3);
		m_dwbonuspet[1][1] = number(3, 3);
		m_dwbonuspet[2][1] = number(3, 3);
		for (int32_t i = 0; i < level; ++i)
		{
			int32_t lvbonus = 0;
			if (tmpage >= 60)
				lvbonus = 1;
			if (i % 5 == 0)
			{
				m_dwbonuspet[0][1] += number(3 + lvbonus, 4 + lvbonus);
				m_dwbonuspet[2][1] += number(3 + lvbonus, 4 + lvbonus);
			}
			if (i % 7 == 0)
			{
				m_dwbonuspet[1][1] += number(3 + lvbonus, 4 + lvbonus);
			}
		}
	}
	else if (newpettype == 4)
	{
		m_dwbonuspet[0][1] = number(3, 3);
		m_dwbonuspet[1][1] = number(3, 3);
		m_dwbonuspet[2][1] = number(3, 3);
		for (int32_t i = 0; i < level; ++i)
		{
			int32_t lvbonus = 0;
			if (level >= 60)
				lvbonus = 1;
			if (i % 5 == 0)
			{
				m_dwbonuspet[0][1] += number(1 + lvbonus, 9 + lvbonus);
				m_dwbonuspet[2][1] += number(1 + lvbonus, 9 + lvbonus);
			}
			if (i % 7 == 0)
			{
				m_dwbonuspet[1][1] += number(1 + lvbonus, 9 + lvbonus);
			}
		}
	}
	else if (newpettype == 5)
	{
		m_dwbonuspet[0][1] = number(3, 3);
		m_dwbonuspet[1][1] = number(3, 3);
		m_dwbonuspet[2][1] = number(3, 3);
		for (int i = 0; i < level; ++i)
		{
			int lvbonus = 0;
			if (tmpage >= 60)
				lvbonus = 1;
			if (i % 5 == 0)
			{
				m_dwbonuspet[0][1] += number(5 + lvbonus, 6 + lvbonus);
				m_dwbonuspet[2][1] += number(5 + lvbonus, 6 + lvbonus);
			}
			if (i % 7 == 0)
			{
				m_dwbonuspet[1][1] += number(5 + lvbonus, 6 + lvbonus);
			}
		}
	}
	else if (newpettype == 6)
	{
		m_dwbonuspet[0][1] = number(3, 3);
		m_dwbonuspet[1][1] = number(3, 3);
		m_dwbonuspet[2][1] = number(3, 3);
		for (int32_t i = 0; i < level; ++i)
		{
			int32_t lvbonus = 0;
			if (tmpage >= 60)
				lvbonus = 1;
			if (i % 5 == 0)
			{
				m_dwbonuspet[0][1] += number(6 + lvbonus, 9 + lvbonus);
				m_dwbonuspet[2][1] += number(6 + lvbonus, 9 + lvbonus);
			}
			if (i % 7 == 0)
			{
				m_dwbonuspet[1][1] += number(6 + lvbonus, 9 + lvbonus);
			}
		}
	}
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("UPDATE `new_petsystem` SET `bonus0` = '%d', `bonus1` = '%d', `bonus2` = '%d', `pet_type` = '%d' WHERE `id` = '%d'", m_dwbonuspet[0][1], m_dwbonuspet[1][1], m_dwbonuspet[2][1], newpettype, petItem->GetID()));
	for (int32_t b = 0; b < 3; b++) {
		petItem->SetForceAttribute(b, 1, m_dwbonuspet[b][1]);
	}

	secondItem->SetCount(secondItem->GetCount() - 1);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "OnResultPetAttrChange %d", newpettype);
}
#endif


#ifdef NEW_PET_SYSTEM
ACMD(do_CubePetAdd) {

	int32_t pos = 0;
	int32_t invpos = 0;

	const char *line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
		return;

	switch (LOWER(arg1[0]))
	{
	case 'a':
	{
		if (0 == arg2[0] || !isdigit(*arg2) ||
			0 == arg3[0] || !isdigit(*arg3))
			return;

		str_to_number(pos, arg2);
		str_to_number(invpos, arg3);

	}
	break;

	default:
		return;
	}

	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->SetItemCube(pos, invpos);
	else
		return;

}

ACMD(do_PetSkill) {
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	uint32_t skillslot = 0;
	str_to_number(skillslot, arg1);
	if (skillslot > 2 || skillslot < 0)
		return;

	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->DoPetSkill(skillslot);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Devi aver evocato il pet per procedere");
}

ACMD(do_FeedCubePet) {
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	uint32_t feedtype = 0;
	str_to_number(feedtype, arg1);
	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->ItemCubeFeed(feedtype);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Nu aveti un insotitor chemat");
}
ACMD(do_PetEvo){
	if (ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() || ch->IsCubeOpen()) {
		ch->ChatPacket(CHAT_TYPE_INFO, "[PetEvolution] You cannot evolve your pet now!");
		return;
	}

	if (ch->GetNewPetSystem()->IsActivePet()) {

		int32_t it[3][7] = { 
						{ 55003, 30058, 30073, 30041, 30017, 30074, 30088 },
						{ 55004, 30035, 30038, 30031, 30080, 30074, 27994 },
						{ 55005, 30083, 27992, 27993, 30086, 30077, 27994 }
		};
		int32_t ic[3][7] = {
						{ 10, 10, 10, 10, 10, 5, 5 },
						{ 10, 10, 10, 10, 5, 10, 8 },
						{ 10, 10, 2, 10, 10, 10, 5 }
		};
		int32_t tmpevo = ch->GetNewPetSystem()->GetEvolution();

		if (ch->GetNewPetSystem()->GetLevel() == 40 && tmpevo == 0 ||
			ch->GetNewPetSystem()->GetLevel() == 80 && tmpevo == 1 ||
			ch->GetNewPetSystem()->GetLevel() >= 82 && tmpevo == 2) {
			for (int32_t b = 0; b < 7; b++) {
				if (ch->CountSpecifyItem(it[tmpevo][b]) < ic[tmpevo][b]) {
					ch->ChatPacket(CHAT_TYPE_INFO, "[PetEvolution] Items Required:");
					for (int32_t c = 0; c < 7; c++) {
						uint32_t vnum = it[tmpevo][c];
						ch->ChatPacket(CHAT_TYPE_INFO, "%s X%d", ITEM_MANAGER::instance().GetTable(vnum)->szLocaleName , ic[tmpevo][c]);
					}
					return;
				}
			}
			for (int32_t c = 0; c < 7; c++) {
				ch->RemoveSpecifyItem(it[tmpevo][c], ic[tmpevo][c]);
			}
			ch->GetNewPetSystem()->IncreasePetEvolution();

		}
		else {
			ch->ChatPacket(CHAT_TYPE_INFO, "You can't evolve your pet!");
			return;
		}

	}else
		ch->ChatPacket(CHAT_TYPE_INFO, "Your pet should be summoned!");

}
#endif

#ifdef ENABLE_NEW_PET_SYSTEM_NAME_CHANGE
ACMD(do_PetChangeName)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	int32_t bCell = 0;
	str_to_number(bCell, arg1);
	LPITEM item = ch->GetInventoryItem(bCell);
	if (!item)
		return;
	if (ch->CountSpecifyItem(55030) < 1)
		return;

	if (ch->GetNewPetSystem()->IsActivePet())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ONCE_PETINI_GONDER"));
		return;
	}

	DBManager::instance().DirectQuery("UPDATE new_petsystem SET name = '%s' WHERE id = '%d'", arg2, item->GetID());
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("PET_ISMI_DEGISTIRILDI"));
	ch->RemoveSpecifyItem(55030, 1);
}
#endif

ACMD(do_cube)
{
	if (!ch->CanDoCube())
		return;

	dev_log(LOG_DEB0, "CUBE COMMAND <%s>: %s", ch->GetName(), argument);
	int32_t cube_index = 0, inven_index = 0;
	const char *line;

	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: cube open");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube close");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube delete <cube_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube list");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube make [all]");
		return;
	}

	const std::string& strArg1 = std::string(arg1);

	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Cube_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int32_t listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);

				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);

				Cube_request_material_info(ch, listIndex, requestCount);
			}
		}

		return;
	}

	switch (LOWER(arg1[0]))
	{
		case 'o':
			Cube_open(ch);
			break;

		case 'c':
			Cube_close(ch);
			break;

		case 'l':
			Cube_show_list(ch);
			break;

		case 'a':
			{
				if (0 == arg2[0] || !isdigit(*arg2) ||
					0 == arg3[0] || !isdigit(*arg3))
					return;

				str_to_number(cube_index, arg2);
				str_to_number(inven_index, arg3);
				Cube_add_item (ch, cube_index, inven_index);
			}
			break;

		case 'd':
			{
				if (0 == arg2[0] || !isdigit(*arg2))
					return;

				str_to_number(cube_index, arg2);
				Cube_delete_item (ch, cube_index);
			}
			break;

		case 'm':
			if (0 != arg2[0])
			{
				while (true == Cube_make(ch))
					dev_log (LOG_DEB0, "cube make success");
			}
			else
				Cube_make(ch);
			break;

		default:
			return;
	}
}

ACMD(do_in_game_mall)
{
	if (LC_IsYMIR() == true || LC_IsKorea() == true)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://metin2.co.kr/04_mall/mall/login.htm");
		return;
	}

	if (true == LC_IsTaiwan())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://203.69.141.203/mall/mall/item_main.htm");
		return;
	}

	if (true == LC_IsWE_Korea())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://metin2.co.kr/50_we_mall/mall/login.htm");
		return;
	}

	if (LC_IsJapan() == true)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://mt2.oge.jp/itemmall/itemList.php");
		return;
	}
	
	if (LC_IsNewCIBN() == true && test_server)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://218.99.6.51/04_mall/mall/login.htm");
		return;
	}

	if (LC_IsSingapore() == true)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "mall http://www.metin2.sg/ishop.php");
		return;
	}
	
	if (LC_IsEurope() == true)
	{
		char country_code[3];

		switch (LC_GetLocalType())
		{
			case LC_GERMANY:	country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_FRANCE:		country_code[0] = 'f'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_ITALY:		country_code[0] = 'i'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_SPAIN:		country_code[0] = 'e'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_UK:			country_code[0] = 'e'; country_code[1] = 'n'; country_code[2] = '\0'; break;
			case LC_TURKEY:		country_code[0] = 't'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_POLAND:		country_code[0] = 'p'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_PORTUGAL:	country_code[0] = 'p'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_GREEK:		country_code[0] = 'g'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_RUSSIA:		country_code[0] = 'r'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_DENMARK:	country_code[0] = 'd'; country_code[1] = 'k'; country_code[2] = '\0'; break;
			case LC_BULGARIA:	country_code[0] = 'b'; country_code[1] = 'g'; country_code[2] = '\0'; break;
			case LC_CROATIA:	country_code[0] = 'h'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_MEXICO:		country_code[0] = 'm'; country_code[1] = 'x'; country_code[2] = '\0'; break;
			case LC_ARABIA:		country_code[0] = 'a'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_CZECH:		country_code[0] = 'c'; country_code[1] = 'z'; country_code[2] = '\0'; break;
			case LC_ROMANIA:	country_code[0] = 'r'; country_code[1] = 'o'; country_code[2] = '\0'; break;
			case LC_HUNGARY:	country_code[0] = 'h'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_NETHERLANDS: country_code[0] = 'n'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_USA:		country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_CANADA:	country_code[0] = 'c'; country_code[1] = 'a'; country_code[2] = '\0'; break;
			default:
				if (test_server == true)
				{
					country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0';
				}
				break;
		}

		char buf[512+1];
		char sas[33];
		MD5_CTX ctx;
		const char sas_key[] = "GF9001";

		snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

		MD5Init(&ctx);
		MD5Update(&ctx, (const uint8_t *) buf, strlen(buf));
#ifdef __FreeBSD__
		MD5End(&ctx, sas);
#else
		static const char hex[] = "0123456789abcdef";
		uint8_t digest[16];
		MD5Final(digest, &ctx);
		int32_t i;
		for (i = 0; i < 16; ++i) {
			sas[i+i] = hex[digest[i] >> 4];
			sas[i+i+1] = hex[digest[i] & 0x0f];
		}
		sas[i+i] = '\0';
#endif

		snprintf(buf, sizeof(buf), "mall %s", g_strWebMallURL.c_str());

		ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
	}
}

ACMD(do_dice) 
{
	int32_t start = 1, end = 100;

	end = MAX(start, end);
	start = MIN(start, end);

	int32_t n = number(start, end);

	if (ch->GetParty())
	{
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_TEXT("%d"), n);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d"), n);
	}
}

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_ride)
{
    if (ch->IsDead() || ch->IsStun())
	{
		return;
	}

	if (ch->GetMapIndex() == 113)
	{
		return;
	}

	if (ch->GetMapIndex() == 252 || ch->GetMapIndex() >= 2520000 && ch->GetMapIndex() <= 2529999)
	{
		return;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ch->IsPolymorphed() == true){
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MSG-7859231624221071"));
		return;
	}

	if(ch->GetWear(WEAR_COSTUME_MOUNT))
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		uint32_t mobVnum = 0;
		
		if (!mountSystem && !mount)
		{
			return;
		}
		
#ifdef __CHANGELOOK_SYSTEM__	
		if(mount->GetTransmutation())
		{
			const TItemTable* itemTable = ITEM_MANAGER::instance().GetTable(mount->GetTransmutation());
			
			if (itemTable)
			{
				mobVnum = itemTable->alValues[1];
			}
			else
			{
				mobVnum = mount->GetValue(1);
			}
		}
		else
		{
			mobVnum = mount->GetValue(1);
		}
#else
		if(mount->GetValue(1) != 0)
		{
			mobVnum = mount->GetValue(1);
		}
#endif

		if (ch->GetMountVnum())
		{
			if(mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		else
		{
			if(mountSystem->CountSummoned() == 1)
			{
				mountSystem->Mount(mobVnum, mount);
			}
		}
		
		return;
	}
#endif

	if (ch->IsHorseRiding())
	{
		ch->StopRiding();
		return;
	}

	if (ch->GetHorse() != NULL)
	{
	    ch->StartRiding();
	    return;
	}

	for (uint8_t i = 0; i<INVENTORY_MAX_NUM; ++i)
	{
	    LPITEM item = ch->GetInventoryItem(i);
	    if (NULL == item)
		{
			continue;
		}

		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_MOUNT)
		{
			ch->UseItem(TItemPos (INVENTORY, i));
		    return;
		}
	}

    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MSG-7312474338483286"));
}

#ifdef __AUCTION__
ACMD(do_get_item_id_list)
{
	for (int32_t i = 0; i < INVENTORY_MAX_NUM; i++)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "name : %s id : %d", item->GetProto()->szName, item->GetID());
	}
}

ACMD(do_enroll_auction)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	char arg4[256];
	two_arguments (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));
	
	uint32_t item_id = strtoul(arg1, NULL, 10);
	uint8_t empire = strtoul(arg2, NULL, 10);
	int32_t bidPrice = strtol(arg3, NULL, 10);
	int32_t immidiatePurchasePrice = strtol(arg4, NULL, 10);

	LPITEM item = ITEM_MANAGER::instance().Find(item_id);
	if (item == NULL)
		return;

	AuctionManager::instance().enroll_auction(ch, item, empire, bidPrice, immidiatePurchasePrice);
}

ACMD(do_enroll_wish)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	one_argument (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));
	
	uint32_t item_num = strtoul(arg1, NULL, 10);
	uint8_t empire = strtoul(arg2, NULL, 10);
	int32_t wishPrice = strtol(arg3, NULL, 10);

	AuctionManager::instance().enroll_wish(ch, item_num, empire, wishPrice);
}

ACMD(do_enroll_sale)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	one_argument (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));
	
	uint32_t item_id = strtoul(arg1, NULL, 10);
	uint32_t wisher_id = strtoul(arg2, NULL, 10);
	int32_t salePrice = strtol(arg3, NULL, 10);

	LPITEM item = ITEM_MANAGER::instance().Find(item_id);
	if (item == NULL)
		return;

	AuctionManager::instance().enroll_sale(ch, item, wisher_id, salePrice);
}

ACMD(do_get_auction_list)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	two_arguments (one_argument (argument, arg1, sizeof(arg1)), arg2, sizeof(arg2), arg3, sizeof(arg3));

	AuctionManager::instance().get_auction_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10), strtoul(arg3, NULL, 10));
}

ACMD (do_get_my_auction_list)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_my_auction_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_get_my_purchase_list)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_my_purchase_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_auction_bid)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().bid (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_auction_impur)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().immediate_purchase (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_get_auctioned_item)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_auctioned_item (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_buy_sold_item)
{
	char arg1[256];
	char arg2[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().get_auctioned_item (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_cancel_auction)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_auction (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_cancel_wish)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_wish (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_cancel_sale)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_sale (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_rebid)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().rebid (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_bid_cancel)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().bid_cancel (ch, strtoul(arg1, NULL, 10));
}
#endif
