/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2010 EQEMu Development Team (http://eqemulator.net)

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
#include "Client.h"
#include "ErrorLog.h"
#include "LoginServer.h"
#include "LoginStructures.h"
#include "../common/md5.h"
#include "../common/MiscFunctions.h"
#include "EQCrypto.h"
#include "../common/sha1.h"

extern EQCrypto eq_crypto;
extern ErrorLog *server_log;
extern LoginServer server;

Client::Client(EQStreamInterface *c, ClientVersion v)
{
	connection = c;
	version = v;
	status = cs_not_sent_session_ready;
	account_id = 0;
	play_server_id = 0;
	play_sequence_id = 0;
}

bool Client::Process()
{
	EQApplicationPacket *app = connection->PopPacket();
	while(app)
	{
		if(server.options.IsTraceOn())
		{
			server_log->Log(log_network, "Application packet received from client (size %u)", app->Size());
		}

		if(server.options.IsDumpInPacketsOn())
		{
			DumpPacket(app);
		}

		switch(app->GetOpcode())
		{
		case OP_SessionReady:
			{
				if(server.options.IsTraceOn())
				{
					server_log->Log(log_network, "Session ready received from client.");
				}
				Handle_SessionReady((const char*)app->pBuffer, app->Size());
				break;
			}
		case OP_Login:
			{
				if(app->Size() < 20)
				{
					server_log->Log(log_network_error, "Login received but it is too small, discarding.");
					break;
				}

				if(server.options.IsTraceOn())
				{
					server_log->Log(log_network, "Login received from client.");
				}

				if(version != cv_old)
					Handle_Login((const char*)app->pBuffer, app->Size());
				else
					Handle_OldLogin((const char*)app->pBuffer, app->Size());
				break;
			}
		case OP_LoginComplete:
			{
				Handle_LoginComplete((const char*)app->pBuffer, app->Size());
				break;
			}
		case OP_LoginUnknown1: //Seems to be related to world status in older clients; we use our own logic for that though.
			{
				EQApplicationPacket *outapp = new EQApplicationPacket(OP_LoginUnknown2, 0);
				connection->QueuePacket(outapp);
				break;
			}
		case OP_ServerListRequest:
			{
				if(server.options.IsTraceOn())
				{
					server_log->Log(log_network, "Server list request received from client.");
				}

				SendServerListPacket();
				break;
			}
		case OP_PlayEverquestRequest:
			{
				if(app->Size() < sizeof(PlayEverquestRequest_Struct) && version != cv_old)
				{
					server_log->Log(log_network_error, "Play received but it is too small, discarding.");
					break;
				}

				Handle_Play((const char*)app->pBuffer);
				break;
			}
		default:
			{
				char dump[64];
				app->build_header_dump(dump);
				server_log->Log(log_network_error, "Recieved unhandled application packet from the client: %s.", dump);
			}
		}

		delete app;
		app = connection->PopPacket();
	}

	return true;
}

void Client::Handle_SessionReady(const char* data, unsigned int size)
{
	if(status != cs_not_sent_session_ready)
	{
		server_log->Log(log_network_error, "Session ready received again after already being received.");
		return;
	}

	if(version != cv_old)
	{
		if(size < sizeof(unsigned int))
		{
			server_log->Log(log_network_error, "Session ready was too small.");
			return;
		}

		unsigned int mode = *((unsigned int*)data);
		if(mode == (unsigned int)lm_from_world)
		{
			server_log->Log(log_network, "Session ready indicated logged in from world(unsupported feature), disconnecting.");
			connection->Close();
			return;
		}
	}

	status = cs_waiting_for_login;

	/**
	* The packets are mostly the same but slightly different between the two versions.
	*/
	if(version == cv_sod)
	{
		EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChatMessage, 17);
		outapp->pBuffer[0] = 0x02;
		outapp->pBuffer[10] = 0x01;
		outapp->pBuffer[11] = 0x65;

		if(server.options.IsDumpOutPacketsOn())
		{
			DumpPacket(outapp);
		}

		connection->QueuePacket(outapp);
		delete outapp;
	}
	else if (version == cv_titanium)
	{
		const char *msg = "ChatMessage";
		EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChatMessage, 16 + strlen(msg));
		outapp->pBuffer[0] = 0x02;
		outapp->pBuffer[10] = 0x01;
		outapp->pBuffer[11] = 0x65;
		strcpy((char*)(outapp->pBuffer + 15), msg);

		if(server.options.IsDumpOutPacketsOn())
		{
			DumpPacket(outapp);
		}

		connection->QueuePacket(outapp);
		delete outapp;
	}
	else if(version == cv_old)
	{
		//Special logic for old streams.
		char buf[20];
		strcpy(buf, "12-4-2002 1800");
		EQApplicationPacket *outapp = new EQApplicationPacket(OP_SessionReady, strlen(buf) + 1);
		strcpy((char*) outapp->pBuffer, buf);
		connection->QueuePacket(outapp);
		delete outapp;
	}
}

void Client::Handle_Login(const char* data, unsigned int size)
{
	if(status != cs_waiting_for_login)
	{
		server_log->Log(log_network_error, "Login received after already having logged in.");
		return;
	}

	if((size - 12) % 8 != 0)
	{
		server_log->Log(log_network_error, "Login received packet of size: %u, this would cause a block corruption, discarding.", size);
		return;
	}

	status = cs_logged_in;

	string e_user;
	string e_hash;
	char *e_buffer = nullptr;
	unsigned int d_account_id = 0;
	string d_pass_hash;

#ifdef WIN32
	e_buffer = server.eq_crypto->DecryptUsernamePassword(data, size, server.options.GetEncryptionMode());

	int buffer_len = strlen(e_buffer);
	e_hash.assign(e_buffer, buffer_len);
	e_user.assign((e_buffer + buffer_len + 1), strlen(e_buffer + buffer_len + 1));

	if(server.options.IsTraceOn())
	{
		server_log->Log(log_client, "User: %s", e_user.c_str());
		server_log->Log(log_client, "Hash: %s", e_hash.c_str());
	}

	server.eq_crypto->DeleteHeap(e_buffer);
#else
	e_buffer = DecryptUsernamePassword(data, size, server.options.GetEncryptionMode());

	int buffer_len = strlen(e_buffer);
	e_hash.assign(e_buffer, buffer_len);
	e_user.assign((e_buffer + buffer_len + 1), strlen(e_buffer + buffer_len + 1));

	if(server.options.IsTraceOn())
	{
		server_log->Log(log_client, "User: %s", e_user.c_str());
		server_log->Log(log_client, "Hash: %s", e_hash.c_str());
	}

	_HeapDeleteCharBuffer(e_buffer);
#endif

	bool result;
	in_addr in;
	in.s_addr = connection->GetRemoteIP();

	if(server.db->GetLoginDataFromAccountName(e_user, d_pass_hash, d_account_id) == false)
	{
		server_log->Log(log_client_error, "Error logging in, user %s does not exist in the database.", e_user.c_str());


		if (server.options.IsLoginFailsOn() && !server.options.IsCreateOn())
		{
			server.db->UpdateAccessLog(d_account_id, e_user, string(inet_ntoa(in)), time(NULL), "Account not exist, PC");
		}
		if (server.options.IsCreateOn() && server.options.IsLoginFailsOn())
		{
			if (server.options.IsLoginFailsOn())
			{
				server.db->UpdateAccessLog(d_account_id, e_user, string(inet_ntoa(in)), time(NULL), "Account created, PC");
			}
			/*eventually add a unix time stamp calculator from last id in log that matches IP
			to limit account creations per time specified by an interval set in the ini.*/
			server.db->UpdateLSAccountInfo(NULL, e_user, e_hash, "PC_generated@server.com", string(inet_ntoa(in)));

			result = false;
		}
		result = false;
	}
	else
	{
		if(d_pass_hash.compare(e_hash) == 0)
		{
			if (server.db->GetStatusWorldAccountTable(e_user))
			{
				result = true;
			}
			else
			{
				if (server.options.IsLoginFailsOn())
				{
					server.db->UpdateAccessLog(d_account_id, e_user, string(inet_ntoa(in)), time(NULL), "Unauthorized client login attempt.");
				}
				result = false;
			}
		}
		else
		{
			if (server.options.IsLoginFailsOn())
			{
				server.db->UpdateAccessLog(d_account_id, e_user, string(inet_ntoa(in)), time(NULL), "PC bad password");
			}
			result = false;
		}
	}

	if(result)
	{
		server.CM->RemoveExistingClient(d_account_id);
		server.db->UpdateLSAccountData(d_account_id, string(inet_ntoa(in)));
		GenerateKey();
		account_id = d_account_id;
		account_name = e_user;

		if (server.options.IsIDnormalsOn())
		{
			server.db->UpdateLSWorldAccountInfo(d_account_id, e_user, e_hash, d_account_id);
		}

		EQApplicationPacket *outapp = new EQApplicationPacket(OP_LoginAccepted, 10 + 80);
		const LoginLoginRequest_Struct* llrs = (const LoginLoginRequest_Struct *)data;
		LoginLoginAccepted_Struct* llas = (LoginLoginAccepted_Struct *)outapp->pBuffer;
		llas->unknown1 = llrs->unknown1;
		llas->unknown2 = llrs->unknown2;
		llas->unknown3 = llrs->unknown3;
		llas->unknown4 = llrs->unknown4;
		llas->unknown5 = llrs->unknown5;

		Login_ReplyBlock_Struct * lrbs = new Login_ReplyBlock_Struct;
		memset(lrbs, 0, sizeof(Login_ReplyBlock_Struct));

		lrbs->failed_attempts = 0;
		lrbs->message = 0x01;
		lrbs->lsid = d_account_id;
		lrbs->unknown3[3] = 0x03;
		lrbs->unknown4[3] = 0x02;
		lrbs->unknown5[0] = 0xe7;
		lrbs->unknown5[1] = 0x03;
		lrbs->unknown6[0] = 0xff;
		lrbs->unknown6[1] = 0xff;
		lrbs->unknown6[2] = 0xff;
		lrbs->unknown6[3] = 0xff;
		lrbs->unknown7[0] = 0xa0;
		lrbs->unknown7[1] = 0x05;
		lrbs->unknown8[3] = 0x02;
		lrbs->unknown9[0] = 0xff;
		lrbs->unknown9[1] = 0x03;
		lrbs->unknown11[0] = 0x63;
		lrbs->unknown12[0] = 0x01;
		memcpy(lrbs->key, key.c_str(), key.size());

#ifdef WIN32
		unsigned int e_size;
		char *encrypted_buffer = server.eq_crypto->Encrypt((const char*)lrbs, 75, e_size);
		memcpy(llas->encrypt, encrypted_buffer, 80);
		server.eq_crypto->DeleteHeap(encrypted_buffer);
#else
		unsigned int e_size;
		char *encrypted_buffer = Encrypt((const char*)lrbs, 75, e_size);
		memcpy(llas->encrypt, encrypted_buffer, 80);
		_HeapDeleteCharBuffer(encrypted_buffer);
#endif

		if(server.options.IsDumpOutPacketsOn())
		{
			DumpPacket(outapp);
		}

		connection->QueuePacket(outapp);
		delete outapp;
	}
	else
	{
		EQApplicationPacket *outapp = new EQApplicationPacket(OP_LoginAccepted, sizeof(LoginLoginFailed_Struct));
		const LoginLoginRequest_Struct* llrs = (const LoginLoginRequest_Struct *)data;
		LoginLoginFailed_Struct* llas = (LoginLoginFailed_Struct *)outapp->pBuffer;
		llas->unknown1 = llrs->unknown1;
		llas->unknown2 = llrs->unknown2;
		llas->unknown3 = llrs->unknown3;
		llas->unknown4 = llrs->unknown4;
		llas->unknown5 = llrs->unknown5;
		memcpy(llas->unknown6, FailedLoginResponseData, sizeof(FailedLoginResponseData));

		if(server.options.IsDumpOutPacketsOn())
		{
			DumpPacket(outapp);
		}

		connection->QueuePacket(outapp);
		delete outapp;
	}
}

void Client::FatalError(const char* message) {
	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ClientError, strlen(message) + 1);
	if (strlen(message) > 1) {
		strcpy((char*)outapp->pBuffer, message);
	}
	connection->QueuePacket(outapp);
	delete outapp;
	return;
}

void Client::Handle_LoginComplete(const char* data, unsigned int size) {
	EQApplicationPacket *outapp = new EQApplicationPacket(OP_LoginComplete, 20);
	outapp->pBuffer[0] = 1;
	connection->QueuePacket(outapp);
	delete outapp;
	return;
}

void Client::Handle_OldLogin(const char* data, unsigned int size)
{
	if(status != cs_waiting_for_login)
	{
		server_log->Log(log_network_error, "Login received after already having logged in.");
		return;
	}

	if (size < sizeof(LoginServerInfo_Struct)) {
		return;
	}

	status = cs_logged_in;

	string e_user;
	string e_hash;
	char *e_buffer = nullptr;
	unsigned int d_account_id = 0;
	string d_pass_hash;
	uchar eqlogin[40];
	uchar sha1pass[40];
	char sha1hash[40];
	eq_crypto.DoEQDecrypt((unsigned char*)data, eqlogin, 40);
	LoginCrypt_struct* lcs = (LoginCrypt_struct*)eqlogin;

	bool result;
	in_addr in;
	in.s_addr = connection->GetRemoteIP();

	if(server.db->GetLoginDataFromAccountName(lcs->username, d_pass_hash, d_account_id) == false)
	{
		server_log->Log(log_client_error, "Error logging in, user %s does not exist in the database.", e_user.c_str());
		
		if (server.options.IsLoginFailsOn() && !server.options.IsCreateOn())
		{
			server.db->UpdateAccessLog(d_account_id, lcs->username, string(inet_ntoa(in)), time(NULL), "Account not exist, Mac");
		}
		if (server.options.IsCreateOn())
		{
			if (server.options.IsLoginFailsOn())
			{
				server.db->UpdateAccessLog(d_account_id, lcs->username, string(inet_ntoa(in)), time(NULL), "Account created, Mac");
			}
			/*eventually add a unix time stamp calculator from last id that matches IP
			to limit account creations per time specified by an interval set in the ini.*/
			server.db->UpdateLSAccountInfo(NULL, lcs->username, lcs->password, "MAC_generated@server.com", string(inet_ntoa(in)));
			FatalError("Account did not exist so it was created. Hit connect again to login.");

			return;
		}
		result = false;
	}
	else
	{
		sha1::calc(lcs->password, strlen(lcs->password), sha1pass);
		sha1::toHexString(sha1pass,sha1hash);
		if(d_pass_hash.compare((char*)sha1hash) == 0)
		{
			result = true;
		}
		else
		{
			if (server.options.IsLoginFailsOn())
			{
				server.db->UpdateAccessLog(d_account_id, lcs->username, string(inet_ntoa(in)), time(NULL), "Mac bad password");
			}
			server_log->Log(log_client_error, "%s", sha1hash);
			result = false;
		}
	}

	if(result)
	{
		server.CM->RemoveExistingClient(d_account_id);
		server.db->UpdateLSAccountData(d_account_id, string(inet_ntoa(in)));
		GenerateKey();
		account_id = d_account_id;
		account_name = e_user;
		EQApplicationPacket *outapp = new EQApplicationPacket(OP_LoginAccepted, sizeof(SessionId_Struct));
		SessionId_Struct* s_id = (SessionId_Struct*)outapp->pBuffer;
		// this is submitted to world server as "username"
		sprintf(s_id->session_id, "LS#%i", account_id);
		strcpy(s_id->unused, "unused");
		s_id->unknown = 4;
		connection->QueuePacket(outapp);
		delete outapp;
		if (server.options.IsIDnormalsOn())
		{
			server.db->UpdateLSWorldAccountInfo(account_id, lcs->username, lcs->password, account_id);
		}
	}
	else
	{
		FatalError("Game over, Insert coin to try again.");
	}
}

void Client::Handle_Play(const char* data)
{
	if(status != cs_logged_in)
	{
		server_log->Log(log_client_error, "Client sent a play request when they either were not logged in, discarding.");
		return;
	}

	if(version == cv_old)
	{
		if(data)
		{
		server.SM->SendOldUserToWorldRequest(data, account_id);
		}
	}
	else
	{
		const PlayEverquestRequest_Struct *play = (const PlayEverquestRequest_Struct*)data;
		unsigned int server_id_in = (unsigned int)play->ServerNumber;
		unsigned int sequence_in = (unsigned int)play->Sequence;

		if(server.options.IsTraceOn())
		{
			server_log->Log(log_network, "Play received from client, server number %u sequence %u.", server_id_in, sequence_in);
		}

		this->play_server_id = (unsigned int)play->ServerNumber;
		play_sequence_id = sequence_in;
		play_server_id = server_id_in;
		server.SM->SendUserToWorldRequest(server_id_in, account_id);
	}
}

void Client::SendServerListPacket()
{
	if(version == cv_old)
	{
		EQApplicationPacket *outapp = server.SM->CreateOldServerListPacket(this);

		if(server.options.IsDumpOutPacketsOn())
		{
			DumpPacket(outapp);
		}

		connection->QueuePacket(outapp);
		delete outapp;
	}
	else
	{
		EQApplicationPacket *outapp = server.SM->CreateServerListPacket(this);

		if(server.options.IsDumpOutPacketsOn())
		{
			DumpPacket(outapp);
		}

		connection->QueuePacket(outapp);
		delete outapp;
	}
}

void Client::SendPlayResponse(EQApplicationPacket *outapp)
{
	if(server.options.IsTraceOn())
	{
		server_log->Log(log_network_trace, "Sending play response for %s.", GetAccountName().c_str());
		server_log->LogPacket(log_network_trace, (const char*)outapp->pBuffer, outapp->size);
	}
	connection->QueuePacket(outapp);
	status = cs_logged_in;
}

void Client::GenerateKey()
{
	key.clear();
	int count = 0;
	while(count < 10)
	{
		static const char key_selection[] =
		{
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			'Y', 'Z', '0', '1', '2', '3', '4', '5',
			'6', '7', '8', '9'
		};

		key.append((const char*)&key_selection[MakeRandomInt(0, 35)], 1);
		count++;
	}
}

