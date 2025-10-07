#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

class CGameContext;

class CSQL
{
private:
	class CGameContext *m_pGameServer;
	
public:
	sql::Driver *driver;
	sql::Connection *connection;
	sql::Statement *statement;
	sql::ResultSet *results;

	char database[64];
	char prefix[64];
	char user[64];
	char pass[64];
	char ip[64];
	int port;

	CSQL(class CGameContext *pGameServer);
	
	// Accessor method for GameServer
	class CGameContext *GameServer() { return m_pGameServer; }

	bool Connect();
	bool IsConnected();
	void Disconnect();
	void CreateTables();
	void CleanupSQL();

	void CreateAccount(const char* name, const char* pass, int m_ClientID);
	void Login(const char* name, const char* pass, int m_ClientID);
	void Logout(int m_ClientID);
	bool IsLoggedIn(int ClientID);
	
	void ChangePassword(int m_ClientID, const char* new_pass);
	
	bool UpdateUser(int UserID, int Level, int Exp, char* UsernameBuf, int BufSize);
	void UpdatePlayer(int ClientID);
	void UpdatePlayers();
	
	void CreateClan(const char* name, int m_ClientID);
};

struct CSqlData
{
	int m_ClientID;
	char name[32];
	char pass[32];
	int UserID[64];
	int m_ExpPoints[64];
	int m_Level[64];
	class CSQL *m_SqlData;
};

#endif