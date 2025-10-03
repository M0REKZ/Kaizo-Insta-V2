/* SQL class 0.5 by Sushi */
/* SQL class 0.6 by FFS   */
#include "accounts.h"
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <game/server/player.h>

static LOCK sql_lock = 0;
class CGameContext *m_pGameServer;
CGameContext *GameServer() { return m_pGameServer; }

CSQL::CSQL(class CGameContext *pGameServer)
{
	if(sql_lock == 0)
		sql_lock = lock_create();

	m_pGameServer = pGameServer;

	// set database info
	database = g_Config.m_SvSqlDatabase;
	prefix = g_Config.m_SvSqlPrefix;
	user = g_Config.m_SvSqlUser;
	pass = g_Config.m_SvSqlPw;
	ip = g_Config.m_SvSqlIp;
	port = g_Config.m_SvSqlPort;
}

bool CSQL::Connect()
{
	try 
	{
		// Create connection
		driver = get_driver_instance();
		char buf[256];
		str_format(buf, sizeof(buf), "tcp://%s:%d", ip, port);
		connection = driver->connect(buf, user, pass);
		
		// Create Statement
		statement = connection->createStatement();
		
		// Create database if not exists
		str_format(buf, sizeof(buf), "CREATE DATABASE IF NOT EXISTS %s", database);
		statement->execute(buf);
		
		// Connect to specific database
		connection->setSchema(database);
		dbg_msg("SQL", "SQL connection established");

		return true;
	} 
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: SQL connection failed (MySQL Error: %s)", e.what());
		return false;
	}
}

void CSQL::Disconnect()
{
	try
	{
		delete connection;
		dbg_msg("SQL", "SQL connection disconnected");
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: No SQL connection (MySQL Error: %s)", e.what());
	}
}

// create tables... should be done only once
void CSQL::CreateTables()
{
	// create connection
	if(Connect())
	{
		try
		{
			// create tables
			char buf[2048];
			str_format(buf, sizeof(buf), 
			"CREATE TABLE IF NOT EXISTS `%s_Account` "
			"(`UserID` INT AUTO_INCREMENT PRIMARY KEY,"
			"`Username` VARCHAR(31) NOT NULL,"
			"`Password` VARCHAR(32) NOT NULL,"
			"`Exp` BIGINT DEFAULT 0,"
			"`Level` BIGINT DEFAULT 0,"
			"`AutologinToken` VARCHAR(31) DEFAULT 'none',"
			"`LastName` VARCHAR(31) DEFAULT 'nameless tee',"
			"`LastIP` VARCHAR(31) DEFAULT 'none');", prefix);
			statement->execute(buf);

			str_format(buf, sizeof(buf), 
			"CREATE TABLE IF NOT EXISTS `%s_Clans` "
			"(`ClanID` INT AUTO_INCREMENT PRIMARY KEY,"
			"`ClanName` VARCHAR(31) NOT NULL,"
			"`Owner` INT NOT NULL,"
			"`Exp` BIGINT DEFAULT 0,"
			"`Level` BIGINT DEFAULT 0"
			");", prefix);
			statement->execute(buf);

			dbg_msg("SQL", "Tables were created successfully");

			// delete statement
			delete statement;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Tables were NOT created (MySQL Error: %s)", e.what());
		}
		
		// disconnect from database
		Disconnect();
	}	
}

// change password
static void change_password_thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	// Connect to database
	if(Data->m_SqlData->Connect())
	{
		try
		{
			// Connect to database
			Data->m_SqlData->Connect();
			
			// check if Account exists
			char buf[512];
			str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE UserID=%d;", Data->m_SqlData->prefix, Data->UserID[Data->m_ClientID]);
			Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
			if(Data->m_SqlData->results->next())
			{
				// update Account data
				str_format(buf, sizeof(buf), "UPDATE %s_Account SET Password='%s' WHERE UserID=%d", Data->m_SqlData->prefix, Data->pass, Data->UserID[Data->m_ClientID]);
				Data->m_SqlData->statement->execute(buf);
				
				// get Account name from database
				str_format(buf, sizeof(buf), "SELECT Username FROM %s_Account WHERE UserID=%d;", Data->m_SqlData->prefix, Data->UserID[Data->m_ClientID]);
				
				// create results
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

				// jump to result
				Data->m_SqlData->results->next();
				
				// finally the name is there \o/
				char acc_name[32];
				str_copy(acc_name, Data->m_SqlData->results->getString("Username").c_str(), sizeof(acc_name));	
				dbg_msg("SQL", "Account '%s' changed password.", acc_name);
				
				// Success
				str_format(buf, sizeof(buf), "Successfully changed your password to '%s'.", Data->pass);
				GameServer()->SendBroadcast(buf, Data->m_ClientID);
				GameServer()->SendChatTarget(Data->m_ClientID, buf);
			}
			else
				dbg_msg("SQL", "Account seems to be deleted");
			
			// delete statement and results
			delete Data->m_SqlData->statement;
			delete Data->m_SqlData->results;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update Account");
		}
		
		// disconnect from database
		Data->m_SqlData->Disconnect();
	}
	
	delete Data;
	
	lock_release(sql_lock);
}

void CSQL::ChangePassword(int m_ClientID, const char* new_pass)
{
	CSqlData *tmp = new CSqlData();
	tmp->m_ClientID = m_ClientID;
	tmp->UserID[m_ClientID] = GameServer()->m_apPlayers[m_ClientID]->m_AccData.m_UserID;
	str_copy(tmp->pass, new_pass, sizeof(tmp->pass));
	tmp->m_SqlData = this;
	
	void *change_pw_thread = thread_init(change_password_thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)change_pw_thread);
#endif
}

static void Create_Account_Thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	if(GameServer()->m_apPlayers[Data->m_ClientID])
	{
		// Connect to database
		if(Data->m_SqlData->Connect())
		{
			try
			{
				char buf[512];
				str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE Username='%s';", Data->m_SqlData->prefix, Data->name);
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
				if(Data->m_SqlData->results->next())
				{
					// already exists
					dbg_msg("SQL", "Account creation failed same username '%s'", Data->name);
					GameServer()->SendChatTarget(Data->m_ClientID, "Failed to create an account. Please choose another username");
				}
				else
				{
					str_format(buf, sizeof(buf), "INSERT INTO %s_Account(Username, Password) VALUES ('%s', '%s');", 
								Data->m_SqlData->prefix, 
								Data->name, Data->pass);
					
					Data->m_SqlData->statement->execute(buf);
					dbg_msg("SQL", "Created account '%s'", Data->name);
					
					GameServer()->SendChatTarget(Data->m_ClientID, "Acoount was created successfully, You may login now.");
				}

				// delete statement
				delete Data->m_SqlData->statement;
				delete Data->m_SqlData->results;
			}
			catch (sql::SQLException &e)
			{
				dbg_msg("SQL", "ERROR: Could not create Account (MySQL Error: %s)", e.what());
			}
			
			// disconnect from database
			Data->m_SqlData->Disconnect();
		}
	}
	
	delete Data;
	lock_release(sql_lock);
}

void CSQL::CreateAccount(const char* name, const char* pass, int m_ClientID)
{
	CSqlData *tmp = new CSqlData();
	str_copy(tmp->name, name, sizeof(tmp->name));
	str_copy(tmp->pass, pass, sizeof(tmp->pass));
	tmp->m_ClientID = m_ClientID;
	tmp->m_SqlData = this;

	void *register_thread = thread_init(Create_Account_Thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)register_thread);
#endif
}

bool CSQL::IsLoggedIn(int ClientID)
{
	if(!GameServer()->m_apPlayers[ClientID])
		return false;
	return GameServer()->m_apPlayers[ClientID]->m_AccData.m_UserID;
}

static void Login_Thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	if(GameServer()->m_apPlayers[Data->m_ClientID])
	{
		if(Data->m_SqlData->IsLoggedIn(Data->m_ClientID))
		{
			GameServer()->SendChatTarget(Data->m_ClientID, "You are already logged in.");
		} 	else
		{
			if(Data->m_SqlData->Connect())
			{
				try
				{
					char buf[1024];
					str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE Username='%s' AND Password='%s';", Data->m_SqlData->prefix, Data->name, Data->pass);
					Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
					if(Data->m_SqlData->results->next())
					{
						GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_UserID = Data->m_SqlData->results->getInt("UserID");
						GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_ExpPoints = (float)Data->m_SqlData->results->getInt("Exp");
						GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_Level = Data->m_SqlData->results->getInt("Level");			

						dbg_msg("SQL", "Account '%s' logged in sucessfully", Data->name);
						GameServer()->SendChatTarget(Data->m_ClientID, "You are now logged in.");
						GameServer()->m_apPlayers[Data->m_ClientID]->SetTeam(TEAM_RED);
					}
					else
					{
						// no Account
						dbg_msg("SQL", "Failed login attempt '%s'", Data->name);
						GameServer()->SendChatTarget(Data->m_ClientID, "Wrong username or password");
					}
					
					// delete statement and results
					delete Data->m_SqlData->statement;
					delete Data->m_SqlData->results;
				}
				catch (sql::SQLException &e)
				{
					dbg_msg("SQL", "ERROR: Could not login Account (MySQL Error: %s)", e.what());
				}
				
				// disconnect from database
				Data->m_SqlData->Disconnect();
			}
		}
	}
	
	delete Data;
	
	lock_release(sql_lock);
}

void CSQL::Login(const char* name, const char* pass, int m_ClientID)
{
	CSqlData *tmp = new CSqlData();
	str_copy(tmp->name, name, sizeof(tmp->name));
	str_copy(tmp->pass, pass, sizeof(tmp->pass));
	tmp->m_ClientID = m_ClientID;
	tmp->m_SqlData = this;
	
	void *login_account_thread = thread_init(Login_Thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)login_account_thread);
#endif
}

void CSQL::Logout(int m_ClientID)
{
	if(!IsLoggedIn(m_ClientID))
	{
		GameServer()->SendChatTarget(m_ClientID, "Please login first.");
		return;
	}

	UpdatePlayer(m_ClientID);
	GameServer()->m_apPlayers[m_ClientID]->m_AccData = {};
	GameServer()->SendChatTarget(m_ClientID, "You have logged out.");
}

bool CSQL::UpdateUser(int UserID, int Level, int Exp, int Money, char* UsernameBuf, int BufSize)
{
	char buf[512];

	// Check if account exists
	str_format(buf, sizeof(buf), "SELECT 1 FROM %s_Account WHERE UserID=%d;", prefix, UserID);
	results = statement->executeQuery(buf);

	if (!results->next())
	{
		delete results;
		return false;
	}
	delete results;

	// Update account data
	str_format(buf, sizeof(buf),
		"UPDATE %s_Account SET Level=%d, Exp=%d WHERE UserID=%d;",
		prefix, Level, Exp, UserID);
	statement->execute(buf);

	// Get username if buffer is provided
	if (UsernameBuf && BufSize > 0)
	{
		str_format(buf, sizeof(buf), "SELECT Username FROM %s_Account WHERE UserID=%d;", prefix, UserID);
		results = statement->executeQuery(buf);
		if (results->next())
		{
			str_copy(UsernameBuf, results->getString("Username").c_str(), BufSize);
		}
		delete results;
	}

	return true;
}

static void Update_Thread(void *user)
{
	lock_wait(sql_lock);

	CSqlData *Data = (CSqlData *)user;

	if (Data->m_SqlData->Connect())
	{
		try
		{
			char username[32];
			bool success = Data->m_SqlData->UpdateUser(
				Data->UserID[Data->m_ClientID],
				Data->m_Level[Data->m_ClientID],
				Data->m_ExpPoints[Data->m_ClientID],
				Data->m_Money[Data->m_ClientID],
				username, sizeof(username)
			);

			if (success)
				dbg_msg("SQL", "Account '%s' was saved successfully", username);
			else
				dbg_msg("SQL", "Account seems to be deleted");

			delete Data->m_SqlData->statement;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update Account (MySQL Error: %s)", e.what());
		}

		Data->m_SqlData->Disconnect();
	}

	delete Data;
	lock_release(sql_lock);
}

void CSQL::UpdatePlayer(int ClientID)
{
	if (!GameServer()->m_apPlayers[ClientID] || !IsLoggedIn(ClientID))
		return;

	CSqlData *tmp = new CSqlData();
	tmp->m_ClientID = ClientID;
	tmp->UserID[ClientID] = GameServer()->m_apPlayers[ClientID]->m_AccData.m_UserID;
	tmp->m_ExpPoints[ClientID] = GameServer()->m_apPlayers[ClientID]->m_AccData.m_ExpPoints;
	tmp->m_Level[ClientID] = GameServer()->m_apPlayers[ClientID]->m_AccData.m_Level;
	tmp->m_SqlData = this;

	void *thread = thread_init(Update_Thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)thread);
#endif
}

void CSQL::UpdatePlayers()
{
	lock_wait(sql_lock);

	if (!Connect())
	{
		lock_release(sql_lock);
		return;
	}

	try
	{
		statement = connection->createStatement();

		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			CPlayer *pPlayer = GameServer()->m_apPlayers[i];
			if (!pPlayer || !pPlayer->m_AccData.m_UserID)
				continue;

			int UserID = pPlayer->m_AccData.m_UserID;
			int Level = pPlayer->m_AccData.m_Level;
			int Exp = pPlayer->m_AccData.m_ExpPoints;

			char acc_name[32];
			bool success = UpdateUser(UserID, Level, Exp, 0, acc_name, sizeof(acc_name));

			if (success)
				dbg_msg("SQL", "Account '%s' was saved successfully", acc_name);
			else
				dbg_msg("SQL", "Account for UserID=%d seems to be deleted", UserID);
		}

		delete statement;
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: Could not update accounts (MySQL Error: %s)", e.what());
	}

	Disconnect();
	lock_release(sql_lock);
}


static void Create_Clan_Thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	if(GameServer()->m_apPlayers[Data->m_ClientID])
	{
		// Connect to database
		if(Data->m_SqlData->Connect())
		{
			try
			{
				char buf[512];
				str_format(buf, sizeof(buf), "SELECT * FROM %s_Clans WHERE ClanName='%s';", Data->m_SqlData->prefix, Data->name);
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
				if(Data->m_SqlData->results->next())
				{
					// already exists
					dbg_msg("SQL", "Clan creation failed same name '%s'", Data->name);
					GameServer()->SendChatTarget(Data->m_ClientID, "Failed to create an clan. Please choose another clanname");
				}
				else
				{
					str_format(buf, sizeof(buf), "INSERT INTO %s_Clans(ClanName, Owner) VALUES ('%s', %d);", 
								Data->m_SqlData->prefix, 
								Data->name, Data->m_ClientID);
					
					Data->m_SqlData->statement->execute(buf);
					dbg_msg("SQL", "Created clan '%s'", Data->name);
					
					GameServer()->SendChatTarget(Data->m_ClientID, "Clan was created successfully");
				}

				// delete statement
				delete Data->m_SqlData->statement;
				delete Data->m_SqlData->results;
			}
			catch (sql::SQLException &e)
			{
				dbg_msg("SQL", "ERROR: Could not create clan (MySQL Error: %s)", e.what());
			}
			
			// disconnect from database
			Data->m_SqlData->Disconnect();
		}
	}
	
	delete Data;
	lock_release(sql_lock);
}

void CSQL::CreateClan(const char* name, int m_ClientID)
{
	CSqlData *tmp = new CSqlData();
	str_copy(tmp->name, name, sizeof(tmp->name));
	tmp->m_ClientID = m_ClientID;
	tmp->m_SqlData = this;

	void *register_thread = thread_init(Create_Clan_Thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)register_thread);
#endif
}