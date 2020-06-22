#pragma once

#define WORD_MAX 0xffff
enum EMisc
{
	MAX_HOST_LENGTH			= 15,
	IP_ADDRESS_LENGTH		= 15,
	LOGIN_MAX_LEN			= 30,
	PASSWD_MAX_LEN			= 16,
	
	PLAYER_PER_ACCOUNT		= 5,
	ACCOUNT_STATUS_MAX_LEN	= 8,
	CHARACTER_NAME_MAX_LEN	= 24,
	SHOP_SIGN_MAX_LEN		= 32,
	INVENTORY_MAX_NUM		= 180,
	ABILITY_MAX_NUM			= 50,
	EMPIRE_MAX_NUM			= 4,
	BANWORD_MAX_LEN			= 24,
	SMS_MAX_LEN				= 80,
	MOBILE_MAX_LEN			= 32,
	SOCIAL_ID_MAX_LEN		= 18,

	GUILD_NAME_MAX_LEN		= 12,

	SHOP_HOST_ITEM_MAX_NUM	= 40,
	SHOP_GUEST_ITEM_MAX_NUM = 18,

	SHOP_PRICELIST_MAX_NUM	= 40,

	CHAT_MAX_LEN			= 512,

	QUICKSLOT_MAX_NUM		= 36,

	JOURNAL_MAX_NUM			= 2,

	QUERY_MAX_LEN			= 8192,

	FILE_MAX_LEN			= 128,

	PLAYER_EXP_TABLE_MAX	= 255,
	PLAYER_MAX_LEVEL_CONST	= 255,

	GUILD_MAX_LEVEL			= 99,
	MOB_MAX_LEVEL			= 100,

#ifdef ENABLE_DROP_FROM_TABLE
	
	DROP_TABLE_SCALE		= 1000000, 
#endif

	ATTRIBUTE_MAX_VALUE		= 20,
	CHARACTER_PATH_MAX_NUM	= 64,
	SKILL_MAX_NUM			= 255,
	SKILLBOOK_DELAY_MIN		= 64800,
	SKILLBOOK_DELAY_MAX		= 108000, 
	SKILL_MAX_LEVEL			= 40,

	APPLY_NAME_MAX_LEN		= 32,
	EVENT_FLAG_NAME_MAX_LEN = 32,

	MOB_SKILL_MAX_NUM		= 5,

    POINT_MAX_NUM = 255,
	DRAGON_SOUL_BOX_SIZE = 32,
	DRAGON_SOUL_BOX_COLUMN_NUM = 8,
	DRAGON_SOUL_BOX_ROW_NUM = DRAGON_SOUL_BOX_SIZE / DRAGON_SOUL_BOX_COLUMN_NUM,
	DRAGON_SOUL_REFINE_GRID_SIZE = 15,
	MAX_AMOUNT_OF_MALL_BONUS	= 20,

	WEAR_MAX_NUM				= 32,

	#ifdef __GAYA__
	GAYA_MAX = 1000000L,
	#endif

	
	GOLD_MAX = 10000000000LL,
	

#ifdef ENABLE_CHEQUE_SYSTEM
	CHEQUE_MAX = 1000L,
#endif

	MAX_PASSPOD = 8 ,

		
	

	OPENID_AUTHKEY_LEN = 32, 

	SHOP_TAB_NAME_MAX = 32,
	SHOP_TAB_COUNT_MAX = 3,

	BELT_INVENTORY_SLOT_WIDTH = 4,
	BELT_INVENTORY_SLOT_HEIGHT= 4,

	BELT_INVENTORY_SLOT_COUNT = BELT_INVENTORY_SLOT_WIDTH * BELT_INVENTORY_SLOT_HEIGHT,



};

enum EMatrixCard
{
	MATRIX_CODE_MAX_LEN		= 192,
	MATRIX_ANSWER_MAX_LEN	= 8,
};

#if defined(__SKILLBOOK_COMB_SYSTEM__)
enum ESkillBookCombination
{
	SKILLBOOK_COMB_SLOT_MAX = 10,
	SKILLBOOK_COMB_COST = 1000000,
};
#endif

enum EWearPositions
{
	WEAR_BODY,		
	WEAR_HEAD,		
	WEAR_FOOTS,		
	WEAR_WRIST,		
	WEAR_WEAPON,	
	WEAR_NECK,		
	WEAR_EAR,		
	WEAR_UNIQUE1,	
	WEAR_UNIQUE2,	
	WEAR_ARROW,		
	WEAR_SHIELD,	
    WEAR_ABILITY1,  
    WEAR_ABILITY2,  
    WEAR_ABILITY3,  
    WEAR_ABILITY4,  
    WEAR_ABILITY5,  
    WEAR_ABILITY6,  
    WEAR_ABILITY7,  
    WEAR_ABILITY8,  
	WEAR_COSTUME_BODY,	
	WEAR_COSTUME_HAIR,	
#ifdef __WEAPON_COSTUME_SYSTEM__
	WEAR_COSTUME_WEAPON,	
#endif
	WEAR_COSTUME_ACCE,	
	WEAR_COSTUME_MOUNT,	
	WEAR_RING1,			
	WEAR_RING2,			

	WEAR_BELT,			

	WEAR_MAX = 32	
};

enum EDragonSoulDeckType
{
	DRAGON_SOUL_DECK_0,
	DRAGON_SOUL_DECK_1,
	DRAGON_SOUL_DECK_MAX_NUM = 2,

	DRAGON_SOUL_DECK_RESERVED_MAX_NUM = 3,	
};

enum ESex
{
	SEX_MALE,
	SEX_FEMALE
};

#ifdef ENABLE_EXTEND_INVEN_SYSTEM
enum EInventory
{
	
	INVENTORY_OPEN_PAGE_COUNT = 2,
	INVENTORY_OPEN_KEY_VNUM = 72319,
	INVENTORY_OPEN_KEY_VNUM2 = 72320,
	INVENTORY_START_DELETE_VNUM = INVENTORY_OPEN_KEY_VNUM, 
	INVENTORY_NEED_KEY_START = 2,
	INVENTORY_NEED_KEY_INCREASE = 3, 
	
	INVENTORY_WIDTH = 5,
	INVENTORY_HEIGHT = 9,
	INVENTORY_PAGE_SIZE = INVENTORY_WIDTH*INVENTORY_HEIGHT,
	INVENTORY_PAGE_COUNT = INVENTORY_MAX_NUM / INVENTORY_PAGE_SIZE,
	INVENTORY_OPEN_PAGE_SIZE = INVENTORY_OPEN_PAGE_COUNT*INVENTORY_PAGE_SIZE,
	INVENTORY_LOCKED_PAGE_COUNT = INVENTORY_PAGE_COUNT-INVENTORY_OPEN_PAGE_COUNT,
	INVENTORY_LOCK_COVER_COUNT = INVENTORY_LOCKED_PAGE_COUNT*INVENTORY_HEIGHT
};
#endif

enum EDirection
{
	DIR_NORTH,
	DIR_NORTHEAST,
	DIR_EAST,
	DIR_SOUTHEAST,
	DIR_SOUTH,
	DIR_SOUTHWEST,
	DIR_WEST,
	DIR_NORTHWEST,
	DIR_MAX_NUM
};

#define ABILITY_MAX_LEVEL	10  

enum EAbilityDifficulty
{
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_VERY_HARD,
	DIFFICULTY_NUM_TYPES
};

enum EAbilityCategory
{
	CATEGORY_PHYSICAL,	
	CATEGORY_MENTAL,	
	CATEGORY_ATTRIBUTE,	
	CATEGORY_NUM_TYPES
};

enum EJobs
{
	JOB_WARRIOR,
	JOB_ASSASSIN,
	JOB_SURA,
	JOB_SHAMAN,
	JOB_MAX_NUM
};

enum ESkillGroups
{
	SKILL_GROUP_MAX_NUM = 2,
};

enum ERaceFlags
{
	RACE_FLAG_ANIMAL	= (1 << 0),
	RACE_FLAG_UNDEAD	= (1 << 1),
	RACE_FLAG_DEVIL		= (1 << 2),
	RACE_FLAG_HUMAN		= (1 << 3),
	RACE_FLAG_ORC		= (1 << 4),
	RACE_FLAG_MILGYO	= (1 << 5),
	RACE_FLAG_INSECT	= (1 << 6),
	RACE_FLAG_FIRE		= (1 << 7),
	RACE_FLAG_ICE		= (1 << 8),
	RACE_FLAG_DESERT	= (1 << 9),
	RACE_FLAG_TREE		= (1 << 10),
	RACE_FLAG_ATT_ELEC	= (1 << 11),
	RACE_FLAG_ATT_FIRE	= (1 << 12),
	RACE_FLAG_ATT_ICE	= (1 << 13),
	RACE_FLAG_ATT_WIND	= (1 << 14),
	RACE_FLAG_ATT_EARTH	= (1 << 15),
	RACE_FLAG_ATT_DARK	= (1 << 16),
};

enum ELoads
{
	LOAD_NONE,
	LOAD_LIGHT,
	LOAD_NORMAL,
	LOAD_HEAVY,
	LOAD_MASSIVE
};

enum
{
	QUICKSLOT_TYPE_NONE,
	QUICKSLOT_TYPE_ITEM,
	QUICKSLOT_TYPE_SKILL,
	QUICKSLOT_TYPE_COMMAND,
	QUICKSLOT_TYPE_MAX_NUM,
};

enum EParts
{
	PART_MAIN,
	PART_WEAPON,
	PART_HEAD,
	PART_HAIR,
	#ifdef __ACCE_SYSTEM__
	PART_ACCE,
	#endif

	PART_MAX_NUM,
	
};

enum EChatType
{
	CHAT_TYPE_TALKING,	
	CHAT_TYPE_INFO,	
	CHAT_TYPE_NOTICE,	
	CHAT_TYPE_PARTY,	
	CHAT_TYPE_GUILD,	
	CHAT_TYPE_COMMAND,	
	CHAT_TYPE_SHOUT,	
	CHAT_TYPE_WHISPER,
	CHAT_TYPE_BIG_NOTICE,
	CHAT_TYPE_MONARCH_NOTICE,
	CHAT_TYPE_MAX_NUM
};

enum EWhisperType
{
	WHISPER_TYPE_NORMAL		= 0,
	WHISPER_TYPE_NOT_EXIST		= 1,
	WHISPER_TYPE_TARGET_BLOCKED	= 2,
	WHISPER_TYPE_SENDER_BLOCKED	= 3,
	WHISPER_TYPE_ERROR		= 4,
	WHISPER_TYPE_GM			= 5,
	WHISPER_TYPE_SYSTEM		= 0xFF
};

enum ECharacterPosition
{
	POSITION_GENERAL,
	POSITION_BATTLE,
	POSITION_DYING,
	POSITION_SITTING_CHAIR,
	POSITION_SITTING_GROUND,
	POSITION_INTRO,
	POSITION_MAX_NUM
};

enum EGMLevels
{
	GM_PLAYER,
	GM_LOW_WIZARD,
	GM_WIZARD,
	GM_HIGH_WIZARD,
	GM_GOD,
	GM_IMPLEMENTOR
};

enum EMobRank
{
	MOB_RANK_PAWN,
	MOB_RANK_S_PAWN,
	MOB_RANK_KNIGHT,
	MOB_RANK_S_KNIGHT,
	MOB_RANK_BOSS,
	MOB_RANK_KING,
	MOB_RANK_MAX_NUM
};

enum ECharType
{
	CHAR_TYPE_MONSTER,
	CHAR_TYPE_NPC,
	CHAR_TYPE_STONE,
	CHAR_TYPE_WARP,
	CHAR_TYPE_DOOR,
	CHAR_TYPE_BUILDING,
	CHAR_TYPE_PC,
	CHAR_TYPE_POLYMORPH_PC,
	CHAR_TYPE_HORSE,
	CHAR_TYPE_GOTO
};

enum EBattleType
{
	BATTLE_TYPE_MELEE,
	BATTLE_TYPE_RANGE,
	BATTLE_TYPE_MAGIC,
	BATTLE_TYPE_SPECIAL,
	BATTLE_TYPE_POWER,
	BATTLE_TYPE_TANKER,
	BATTLE_TYPE_SUPER_POWER,
	BATTLE_TYPE_SUPER_TANKER,
	BATTLE_TYPE_MAX_NUM
};

enum EApplyTypes
{
	APPLY_NONE,			
	APPLY_MAX_HP,		
	APPLY_MAX_SP,		
	APPLY_CON,			
	APPLY_INT,			
	APPLY_STR,			
	APPLY_DEX,			
	APPLY_ATT_SPEED,	
	APPLY_MOV_SPEED,	
	APPLY_CAST_SPEED,	
	APPLY_HP_REGEN,		
	APPLY_SP_REGEN,		
	APPLY_POISON_PCT,	
	APPLY_STUN_PCT,		
	APPLY_SLOW_PCT,		
	APPLY_CRITICAL_PCT,		
	APPLY_PENETRATE_PCT,	
	APPLY_ATTBONUS_HUMAN,	
	APPLY_ATTBONUS_ANIMAL,	
	APPLY_ATTBONUS_ORC,		
	APPLY_ATTBONUS_MILGYO,	
	APPLY_ATTBONUS_UNDEAD,	
	APPLY_ATTBONUS_DEVIL,	
	APPLY_STEAL_HP,			
	APPLY_STEAL_SP,			
	APPLY_MANA_BURN_PCT,	
	APPLY_DAMAGE_SP_RECOVER,	
	APPLY_BLOCK,			
	APPLY_DODGE,			
	APPLY_RESIST_SWORD,		
	APPLY_RESIST_TWOHAND,	
	APPLY_RESIST_DAGGER,	
	APPLY_RESIST_BELL,		
	APPLY_RESIST_FAN,		
	APPLY_RESIST_BOW,		
	APPLY_RESIST_FIRE,		
	APPLY_RESIST_ELEC,		
	APPLY_RESIST_MAGIC,		
	APPLY_RESIST_WIND,		
	APPLY_REFLECT_MELEE,	
	APPLY_REFLECT_CURSE,	
	APPLY_POISON_REDUCE,	
	APPLY_KILL_SP_RECOVER,	
	APPLY_EXP_DOUBLE_BONUS,	
	APPLY_GOLD_DOUBLE_BONUS,	
	APPLY_ITEM_DROP_BONUS,	
	APPLY_POTION_BONUS,		
	APPLY_KILL_HP_RECOVER,	
	APPLY_IMMUNE_STUN,		
	APPLY_IMMUNE_SLOW,		
	APPLY_IMMUNE_FALL,		
	APPLY_SKILL,			
	APPLY_BOW_DISTANCE,		
	APPLY_ATT_GRADE_BONUS,	
	APPLY_DEF_GRADE_BONUS,	
	APPLY_MAGIC_ATT_GRADE,	
	APPLY_MAGIC_DEF_GRADE,	
	APPLY_CURSE_PCT,		
	APPLY_MAX_STAMINA,		
	APPLY_ATTBONUS_WARRIOR,	
	APPLY_ATTBONUS_ASSASSIN,	
	APPLY_ATTBONUS_SURA,	
	APPLY_ATTBONUS_SHAMAN,	
	APPLY_ATTBONUS_MONSTER,	
	APPLY_MALL_ATTBONUS,			
	APPLY_MALL_DEFBONUS,			
	APPLY_MALL_EXPBONUS,			
	APPLY_MALL_ITEMBONUS,			
	APPLY_MALL_GOLDBONUS,			
	APPLY_MAX_HP_PCT,				
	APPLY_MAX_SP_PCT,				
	APPLY_SKILL_DAMAGE_BONUS,		
	APPLY_NORMAL_HIT_DAMAGE_BONUS,	
	APPLY_SKILL_DEFEND_BONUS,		
	APPLY_NORMAL_HIT_DEFEND_BONUS,	
	APPLY_PC_BANG_EXP_BONUS,		
	APPLY_PC_BANG_DROP_BONUS,		

	APPLY_EXTRACT_HP_PCT,			

	APPLY_RESIST_WARRIOR,			
	APPLY_RESIST_ASSASSIN,			
	APPLY_RESIST_SURA,				
	APPLY_RESIST_SHAMAN,			
	APPLY_ENERGY,					
	APPLY_DEF_GRADE,				
	APPLY_COSTUME_ATTR_BONUS,		
	APPLY_MAGIC_ATTBONUS_PER,		
	APPLY_MELEE_MAGIC_ATTBONUS_PER,			
	
	APPLY_RESIST_ICE,		
	APPLY_RESIST_EARTH,		
	APPLY_RESIST_DARK,		

	APPLY_ANTI_CRITICAL_PCT,	
	APPLY_ANTI_PENETRATE_PCT,	
#ifdef __ANTI_RESIST_MAGIC_BONUS__
	APPLY_ANTI_RESIST_MAGIC,	
#endif

	MAX_APPLY_NUM,              
};

enum EOnClickEvents
{
	ON_CLICK_NONE,
	ON_CLICK_SHOP,
	ON_CLICK_TALK,
	ON_CLICK_MAX_NUM
};

enum EOnIdleEvents
{
	ON_IDLE_NONE,
	ON_IDLE_GENERAL,
	ON_IDLE_MAX_NUM
};

enum EWindows
{
	RESERVED_WINDOW,
	INVENTORY,
	EQUIPMENT,
	SAFEBOX,
	MALL,
	DRAGON_SOUL_INVENTORY,
	BELT_INVENTORY,
#ifdef __AUCTION__
	AUCTION,
#endif
	GROUND
};

enum EMobSizes
{
	MOBSIZE_RESERVED,
	MOBSIZE_SMALL,
	MOBSIZE_MEDIUM,
	MOBSIZE_BIG
};

enum EAIFlags
{
	AIFLAG_AGGRESSIVE	= (1 << 0),
	AIFLAG_NOMOVE	= (1 << 1),
	AIFLAG_COWARD	= (1 << 2),
	AIFLAG_NOATTACKSHINSU	= (1 << 3),
	AIFLAG_NOATTACKJINNO	= (1 << 4),
	AIFLAG_NOATTACKCHUNJO	= (1 << 5),
	AIFLAG_ATTACKMOB = (1 << 6 ),
	AIFLAG_BERSERK	= (1 << 7),
	AIFLAG_STONESKIN	= (1 << 8),
	AIFLAG_GODSPEED	= (1 << 9),
	AIFLAG_DEATHBLOW	= (1 << 10),
	AIFLAG_REVIVE		= (1 << 11),
};

enum EMobStatType
{
	MOB_STATTYPE_POWER,
	MOB_STATTYPE_TANKER,
	MOB_STATTYPE_SUPER_POWER,
	MOB_STATTYPE_SUPER_TANKER,
	MOB_STATTYPE_RANGE,
	MOB_STATTYPE_MAGIC,
	MOB_STATTYPE_MAX_NUM
};

enum EImmuneFlags
{
	IMMUNE_STUN		= (1 << 0),
	IMMUNE_SLOW		= (1 << 1),
	IMMUNE_FALL		= (1 << 2),
	IMMUNE_CURSE	= (1 << 3),
	IMMUNE_POISON	= (1 << 4),
	IMMUNE_TERROR	= (1 << 5),
	IMMUNE_REFLECT	= (1 << 6),
};

enum EMobEnchants
{
	MOB_ENCHANT_CURSE,
	MOB_ENCHANT_SLOW,
	MOB_ENCHANT_POISON,
	MOB_ENCHANT_STUN,
	MOB_ENCHANT_CRITICAL,
	MOB_ENCHANT_PENETRATE,
	MOB_ENCHANTS_MAX_NUM
};

enum EMobResists
{
	MOB_RESIST_SWORD,
	MOB_RESIST_TWOHAND,
	MOB_RESIST_DAGGER,
	MOB_RESIST_BELL,
	MOB_RESIST_FAN,
	MOB_RESIST_BOW,
	MOB_RESIST_CLAW,
	MOB_RESIST_FIRE,
	MOB_RESIST_ELECT,
	MOB_RESIST_MAGIC,
	MOB_RESIST_WIND,
	MOB_RESIST_POISON,
	MOB_RESIST_BLEEDING,
	MOB_RESISTS_MAX_NUM
};

enum
{
	SKILL_ATTR_TYPE_NORMAL = 1,
	SKILL_ATTR_TYPE_MELEE,
	SKILL_ATTR_TYPE_RANGE,
	SKILL_ATTR_TYPE_MAGIC
		
};

enum
{
	SKILL_NORMAL,
	SKILL_MASTER,
	SKILL_GRAND_MASTER,
	SKILL_PERFECT_MASTER,
};

enum EGuildWarType
{
	GUILD_WAR_TYPE_FIELD,
	GUILD_WAR_TYPE_BATTLE,
	GUILD_WAR_TYPE_FLAG,
	GUILD_WAR_TYPE_MAX_NUM
};

enum EGuildWarState
{
	GUILD_WAR_NONE,
	GUILD_WAR_SEND_DECLARE,
	GUILD_WAR_REFUSE,
	GUILD_WAR_RECV_DECLARE,
	GUILD_WAR_WAIT_START,
	GUILD_WAR_CANCEL,
	GUILD_WAR_ON_WAR,
	GUILD_WAR_END,
	GUILD_WAR_OVER,
	GUILD_WAR_RESERVE,

	GUILD_WAR_DURATION = 30 * 60,
	GUILD_WAR_WIN_POINT = 1000,
	GUILD_WAR_LADDER_HALF_PENALTY_TIME = 12 * 60 * 60,
};

enum EAttributeSet 
{            
	ATTRIBUTE_SET_WEAPON,
	ATTRIBUTE_SET_BODY, 
	ATTRIBUTE_SET_WRIST, 
	ATTRIBUTE_SET_FOOTS,
	ATTRIBUTE_SET_NECK,
	ATTRIBUTE_SET_HEAD,
	ATTRIBUTE_SET_SHIELD,
	ATTRIBUTE_SET_EAR,
	ATTRIBUTE_SET_MAX_NUM
};  

enum EPrivType
{
	PRIV_NONE,
	PRIV_ITEM_DROP,
	PRIV_GOLD_DROP,
	PRIV_GOLD10_DROP,
	PRIV_EXP_PCT,
	MAX_PRIV_NUM,
};

enum EMoneyLogType
{
	MONEY_LOG_RESERVED,
	MONEY_LOG_MONSTER,
	MONEY_LOG_SHOP,
	MONEY_LOG_REFINE,
	MONEY_LOG_QUEST,
	MONEY_LOG_GUILD,
	MONEY_LOG_MISC,
	MONEY_LOG_MONSTER_KILL,
	MONEY_LOG_DROP,
	MONEY_LOG_TYPE_MAX_NUM,
};

enum EPremiumTypes
{
	PREMIUM_EXP,		
	PREMIUM_ITEM,		
	PREMIUM_SAFEBOX,		
	PREMIUM_AUTOLOOT,		
	PREMIUM_FISH_MIND,		
	PREMIUM_MARRIAGE_FAST,	
	PREMIUM_GOLD,		
	PREMIUM_MAX_NUM = 9
};

enum SPECIAL_EFFECT
{
	SE_NONE,

	SE_HPUP_RED,
	SE_SPUP_BLUE,
	SE_SPEEDUP_GREEN,
	SE_DXUP_PURPLE,
	SE_CRITICAL,
	SE_PENETRATE,
	SE_BLOCK,
	SE_DODGE,
	SE_CHINA_FIREWORK,
	SE_SPIN_TOP,
	SE_SUCCESS,
	SE_FAIL,
	SE_FR_SUCCESS,
	SE_LEVELUP_ON_14_FOR_GERMANY,
	SE_LEVELUP_UNDER_15_FOR_GERMANY,
	SE_PERCENT_DAMAGE1,
	SE_PERCENT_DAMAGE2,
	SE_PERCENT_DAMAGE3,

	SE_AUTO_HPUP,
	SE_AUTO_SPUP,

	SE_EQUIP_RAMADAN_RING,		
	SE_EQUIP_HALLOWEEN_CANDY,		
	SE_EQUIP_HAPPINESS_RING,		
	SE_EQUIP_LOVE_PENDANT,		

#ifdef ENABLE_AGGREGATE_MONSTER_EFFECT
	SE_AGGREGATE_MONSTER_EFFECT,
#endif

	#ifdef __ACCE_SYSTEM__
	SE_EFFECT_ACCE_SUCCEDED,
	SE_EFFECT_ACCE_EQUIP,
	#endif
	
	
	
	
	
} ;

enum ETeenFlags
{
	TEENFLAG_NONE = 0,
	TEENFLAG_1HOUR,
	TEENFLAG_2HOUR,
	TEENFLAG_3HOUR,
	TEENFLAG_4HOUR,
	TEENFLAG_5HOUR,
};

#include "item_length.h"






enum EDragonSoulRefineWindowSize
{
	DRAGON_SOUL_REFINE_GRID_MAX = 15,
};

enum EMisc2
{
	DRAGON_SOUL_EQUIP_SLOT_START = INVENTORY_MAX_NUM + WEAR_MAX_NUM,
	DRAGON_SOUL_EQUIP_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_START + (DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM),
	DRAGON_SOUL_EQUIP_RESERVED_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_END + (DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM),

	BELT_INVENTORY_SLOT_START = DRAGON_SOUL_EQUIP_RESERVED_SLOT_END,
	BELT_INVENTORY_SLOT_END = BELT_INVENTORY_SLOT_START + BELT_INVENTORY_SLOT_COUNT,

	INVENTORY_AND_EQUIP_SLOT_MAX = BELT_INVENTORY_SLOT_END,
};

#pragma pack(push, 1)

typedef struct SItemPos
{
	uint8_t window_type;
	uint16_t cell;
    SItemPos ()
    {
        window_type = INVENTORY;
		cell = WORD_MAX;
    }

	SItemPos (uint8_t _window_type, uint16_t _cell)
    {
        window_type = _window_type;
        cell = _cell;
    }

	bool IsValidItemPosition() const
	{
		switch (window_type)
		{
		case RESERVED_WINDOW:
			return false;
		case INVENTORY:
		case EQUIPMENT:
		case BELT_INVENTORY:
			return cell < INVENTORY_AND_EQUIP_SLOT_MAX;
		case DRAGON_SOUL_INVENTORY:
			return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);
		
		case SAFEBOX:
		case MALL:
			return false;
		default:
			return false;
		}
		return false;
	}
	
	bool IsEquipPosition() const
	{
		return ((INVENTORY == window_type || EQUIPMENT == window_type) && cell >= INVENTORY_MAX_NUM && cell < INVENTORY_MAX_NUM + WEAR_MAX_NUM)
			|| IsDragonSoulEquipPosition();
	}

	bool IsDragonSoulEquipPosition() const
	{
		return (DRAGON_SOUL_EQUIP_SLOT_START <= cell) && (DRAGON_SOUL_EQUIP_SLOT_END > cell);
	}

	bool IsBeltInventoryPosition() const
	{
		return (BELT_INVENTORY_SLOT_START <= cell) && (BELT_INVENTORY_SLOT_END > cell);
	}

	bool IsDefaultInventoryPosition() const
	{
		return INVENTORY == window_type && cell < INVENTORY_MAX_NUM;
	}

	bool operator==(const struct SItemPos& rhs) const
	{
		return (window_type == rhs.window_type) && (cell == rhs.cell);
	}
	bool operator<(const struct SItemPos& rhs) const
	{
		return (window_type < rhs.window_type) || ((window_type == rhs.window_type) && (cell < rhs.cell));
	}
} TItemPos;

const TItemPos NPOS (RESERVED_WINDOW, WORD_MAX);

typedef enum
{
	SHOP_COIN_TYPE_GOLD, 
	SHOP_COIN_TYPE_SECONDARY_COIN,
} EShopCoinType;

#pragma pack(pop)
