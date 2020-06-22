#pragma once

#include "../../libsql/AsyncSQL.h"
#include "any_function.h"

enum
{
	QUERY_TYPE_RETURN = 1,
	QUERY_TYPE_FUNCTION = 2,
	QUERY_TYPE_AFTER_FUNCTION = 3,
};

enum
{
	QID_SAFEBOX_SIZE,
	QID_AUTH_LOGIN,
	QID_AUTH_LOGIN_OPENID,
	QID_LOTTO,
	QID_HIGHSCORE_REGISTER,
	QID_HIGHSCORE_SHOW,
	QID_BILLING_GET_TIME,
	QID_BILLING_CHECK,

	QID_BLOCK_CHAT_LIST,

	QID_PCBANG_IP_LIST_CHECK,
	QID_PCBANG_IP_LIST_SELECT,

	QID_PROTECT_CHILD,

	QID_BRAZIL_CREATE_ID,
	QID_JAPAN_CREATE_ID,
};

typedef struct SUseTime
{
	uint32_t	dwLoginKey;
	char        szLogin[LOGIN_MAX_LEN+1];
	uint8_t        bBillType;
	uint32_t       dwUseSec;
	char        szIP[MAX_HOST_LENGTH+1];
} TUseTime;

class CQueryInfo
{
	public:
		int32_t	iQueryType;
};

class CReturnQueryInfo : public CQueryInfo
{
	public:
		int32_t	iType;
		uint32_t	dwIdent;
		void			*	pvData;
};

class CFuncQueryInfo : public CQueryInfo
{
	public:
		any_function f;
};

class CFuncAfterQueryInfo : public CQueryInfo
{
	public:
		any_void_function f;
};

class CLoginData;


class DBManager : public singleton<DBManager>
{
	public:
		DBManager();
		virtual ~DBManager();

		bool			IsConnected();

		bool			Connect(const char * host, const int32_t port, const char * user, const char * pwd, const char * db);
		void			Query(const char * c_pszFormat, ...);

		SQLMsg *		DirectQuery(const char * c_pszFormat, ...);
		void			ReturnQuery(int32_t iType, uint32_t dwIdent, void* pvData, const char * c_pszFormat, ...);

		void			Process();
		void			AnalyzeReturnQuery(SQLMsg * pmsg);

		void			SendMoneyLog(uint8_t type, uint32_t vnum, int64_t gold);

		void			LoginPrepare(uint8_t bBillType, uint32_t dwBillID, int32_t lRemainSecs, LPDESC d, uint32_t * pdwClientKey, int32_t * paiPremiumTimes = NULL);
		void			SendAuthLogin(LPDESC d);
		void			SendLoginPing(const char * c_pszLogin);

		void			InsertLoginData(CLoginData * pkLD);
		void			DeleteLoginData(CLoginData * pkLD);
		CLoginData *		GetLoginData(uint32_t dwKey);
		void			SetBilling(uint32_t dwKey, bool bOn, bool bSkipPush = false);
		void			PushBilling(CLoginData * pkLD);
		void			FlushBilling(bool bForce=false);
		void			CheckBilling();

		void			StopAllBilling();

		uint32_t			CountQuery()		{ return m_sql.CountQuery(); }
		uint32_t			CountQueryResult()	{ return m_sql.CountResult(); }
		void			ResetQueryResult()	{ m_sql.ResetQueryFinished(); }

		void			RequestBlockException(const char *login, int32_t cmd);


		template<class Functor> void FuncQuery(Functor f, const char * c_pszFormat, ...);
		template<class Functor> void FuncAfterQuery(Functor f, const char * c_pszFormat, ...);

		uint32_t EscapeString(char* dst, uint32_t dstSize, const char* src, uint32_t srcSize);

	private:
		SQLMsg *				PopResult();

		CAsyncSQL				m_sql;
		CAsyncSQL				m_sql_direct;
		bool					m_bIsConnect;

		std::map<uint32_t, CLoginData *>		m_map_pkLoginData;
		std::map<std::string, CLoginData *>	mapLDBilling;
		std::vector<TUseTime>			m_vec_kUseTime;
};

template <class Functor> void DBManager::FuncQuery(Functor f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	CFuncQueryInfo * p = M2_NEW CFuncQueryInfo;

	p->iQueryType = QUERY_TYPE_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

template <class Functor> void DBManager::FuncAfterQuery(Functor f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	CFuncAfterQueryInfo * p = M2_NEW CFuncAfterQueryInfo;

	p->iQueryType = QUERY_TYPE_AFTER_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

typedef struct SHighscoreRegisterQueryInfo
{
	char    szBoard[20+1]; 
	uint32_t   dwPID;
	int32_t     iValue;
	bool    bOrder;
} THighscoreRegisterQueryInfo;

extern void SendBillingExpire(const char * c_pszLogin, uint8_t bBillType, int32_t iSecs, CLoginData * pkLD);
extern void VCardUse(LPCHARACTER CardOwner, LPCHARACTER CardTaker, LPITEM item);

class AccountDB : public singleton<AccountDB>
{
	public:
		AccountDB();

		bool IsConnected();
		bool Connect(const char * host, const int32_t port, const char * user, const char * pwd, const char * db);
		bool ConnectAsync(const char * host, const int32_t port, const char * user, const char * pwd, const char * db, const char * locale);

		SQLMsg* DirectQuery(const char * query);		
		void ReturnQuery(int32_t iType, uint32_t dwIdent, void * pvData, const char * c_pszFormat, ...);
		void AsyncQuery(const char* query);

		void SetLocale(const std::string & stLocale);

		void Process();

	private:
		SQLMsg * PopResult();
		void AnalyzeReturnQuery(SQLMsg * pMsg);

		CAsyncSQL2	m_sql_direct;
		CAsyncSQL2	m_sql;
		bool		m_IsConnect;

};
