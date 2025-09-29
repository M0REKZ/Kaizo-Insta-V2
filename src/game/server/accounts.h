/* SQL Class 0.5 by Sushi */
/* SQL Class 0.6 by FFS*/
#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <engine/shared/protocol.h>
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

class CSQL
{
public:
	CSQL(class CGameContext *pGameServer);

	sql::Driver *driver;
	sql::Connection *connection;
	sql::Statement *statement;
	sql::ResultSet *results;

	// copy of config vars
	const char* database;
	const char* prefix;
	const char* user;
	const char* pass;
	const char* ip;
	int port;

	bool Connect();
	void Disconnect();
	
	void CreateTables();
	void CreateAccount(const char* name, const char* pass, int client_id);
	void ChangePassword(int client_id, const char* new_pass);

	void CreateClan(const char* name, int m_ClientID);

	void Login(const char* name, const char* pass, int client_id);
	bool UpdateUser(int UserID, int Level, int Exp, int Money, char* UsernameBuf, int BufSize);
	void update(int client_id);
	void update_all();
};

struct CSqlData
{
	CSQL *m_SqlData;
	char name[32];
	char pass[32];
	int m_ClientID;
	int UserID[MAX_CLIENTS];

	unsigned int m_Level[MAX_CLIENTS];
	unsigned long long int m_ExpPoints[MAX_CLIENTS];
	unsigned long long m_Money[MAX_CLIENTS];
};

#endif