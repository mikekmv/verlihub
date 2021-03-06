/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include "ccustomredirects.h"
#include "cconfigitembase.h"
#include "cserverdc.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	namespace nTables {

	cRedirects::cRedirects(cServerDC *server):
		tMySQLMemoryList<cRedirect, cServerDC>(server->mMySQL, server, "custom_redirects")
	{
		SetClassName("nDC::cRedirects");
	}

	void cRedirects::AddFields()
	{
		AddCol("address", "varchar(125)", "", false, mModel.mAddress);
		AddPrimaryKey("address");
		AddCol("flag", "smallint(5)", "", false, mModel.mFlag);
		AddCol("enable", "tinyint(1)", "1", true, mModel.mEnable);
		mMySQLTable.mExtra = "PRIMARY KEY(address)";
		SetBaseTo(&mModel);
	}

	int cRedirects::MapTo(unsigned int Type)
	{
		switch (Type) {
			case eCR_INVALID_USER:
			case eCR_KICKED:
				return eKick;

			case eCR_USERLIMIT:
				return eUserLimit;

			case eCR_SHARE_LIMIT:
				return eShareLimit;

			case eCR_TAG_INVALID:
			case eCR_TAG_NONE:
				return eTag;

			case eCR_PASSWORD:
				return eWrongPasswd;

			case eCR_INVALID_KEY:
				return eInvalidKey;

			case eCR_HUB_LOAD:
				return eHubBusy;

			case eCR_RECONNECT:
				return eReconnect;

			case eCR_CLONE:
				return eClone;

			case eCR_SELF:
				return eSelf;

			case eCR_BADNICK:
				return eBadNick;

			case eCR_NOREDIR:
				return -1;

			default:
				return 0;
		}
	}

	/*
		find redirect url from a given type
	*/
	const char* cRedirects::MatchByType(unsigned int Type)
	{
		iterator it;
		cRedirect *redirect;
		const char *redirects[10];
		const char *DefaultRedirect[10];
		int i = 0, j = 0, iType = MapTo(Type);

		if (iType == -1) // do not redirect, special reason
			return "";

		for (it = begin(); it != end(); ++it) {
			if (i >= 10)
				break;

			redirect = (*it);

			if (redirect) {
				if (redirect->mEnable && (redirect->mFlag & iType)) {
					redirects[i] = redirect->mAddress.c_str();
					i++;
				}

				if (redirect->mEnable && !redirect->mFlag && (j < 10)) {
					DefaultRedirect[j] = redirect->mAddress.c_str();
					j++;
				}
			}
		}

		if (!i) { // use default redirect
			if (!j)
				return "";

			Random(j);
			CountPlusPlus(DefaultRedirect[j]);
			return DefaultRedirect[j];
		}

		Random(i);
		CountPlusPlus(redirects[i]);
		return redirects[i];
	}

	void cRedirects::Random(int &key)
	{
		srand (time(NULL));
		int temp = int(1.0 * key * rand() / (RAND_MAX + 1.0));

		if (temp < key)
			key = temp;
		else
			key -= 1;
	}

	void cRedirects::CountPlusPlus(const char *addr)
	{
		iterator it;
		cRedirect *redirect;

		for (it = begin(); it != end(); ++it) {
			redirect = (*it);

			if (redirect && (addr == redirect->mAddress.c_str())) {
				redirect->mCount++; // increase counter
				break;
			}
		}
	}

	bool cRedirects::CompareDataKey(const cRedirect &D1, const cRedirect &D2)
	{
		return (D1.mAddress == D2.mAddress);
	}

	cRedirectConsole::cRedirectConsole(cDCConsole *console):
		tRedirectConsoleBase(console)
	{
		this->AddCommands();
	}

	cRedirectConsole::~cRedirectConsole()
	{}

	void cRedirectConsole::GetHelpForCommand(int cmd, ostream &os)
	{
		string help_str;

		switch (cmd) {
			case eLC_LST:
				help_str = "!lstredirect\r\n" + string(_("Show list of redirects"));
				break;

			case eLC_ADD:
			case eLC_MOD:
				help_str = "!(add|mod)redirect <url>"
					"[ -f <flags>]"
					"[ -e <1/0>]";

				break;

			case eLC_DEL:
				help_str = "!delredirect <url>";
				break;

			default:
				break;
		}

		if (help_str.size()) {
			cDCProto::EscapeChars(help_str, help_str);
			os << help_str;
		}
	}

	void cRedirectConsole::GetHelp(ostream &os)
	{
		string help("https://github.com/verlihub/verlihub/wiki/redirects/\r\n\r\n");

		help += " Available redirect flags:\r\n\r\n";
		help += " 0\t\t\t- For any other reason\r\n";
		help += " 1\t\t\t- Ban and kick\r\n";
		help += " 2\t\t\t- Hub is full\r\n";
		help += " 4\t\t\t- Too low or too high share\r\n";
		help += " 8\t\t\t- Wrong or unknown tag\r\n";
		help += " 16\t\t\t- Wrong password\r\n";
		help += " 32\t\t\t- Invalid key\r\n";
		help += " 64\t\t\t- Hub is busy\r\n";
		help += " 128\t\t\t- Too fast reconnect\r\n";
		help += " 256\t\t\t- Bad nick, already used, too short, etc\r\n";
		help += " 512\t\t\t- Clone detection\r\n";
		help += " 1024\t\t\t- Same user connects twice\r\n\r\n";
		help += " Remember to make the sum of selected above flags.\r\n";

		cDCProto::EscapeChars(help, help);
		os << help;
	}

	const char* cRedirectConsole::GetParamsRegex(int cmd)
	{
		switch (cmd) {
			case eLC_ADD:
			case eLC_MOD:
				return "^(\\S+)("
						"( -f ?(\\d+))?|"
						"( -e ?(1|0))?|"
						")*\\s*$";

			case eLC_DEL:
				return "(\\S+)";

			default:
				return "";
		}
	}

	bool cRedirectConsole::ReadDataFromCmd(cfBase *cmd, int CmdID, cRedirect &data)
	{
		enum {
			eADD_ALL,
   			eADD_ADDRESS,
			eADD_CHOICE,
   			eADD_FLAGp, eADD_FLAG,
			eADD_ENABLEp, eADD_ENABLE
		};

		cmd->GetParStr(eADD_ADDRESS, data.mAddress);
		cmd->GetParInt(eADD_FLAG, data.mFlag);
		cmd->GetParInt(eADD_ENABLE, data.mEnable);
		return true;
	}

	cRedirects *cRedirectConsole::GetTheList()
	{
		return mOwner->mRedirects;
	}

	const char* cRedirectConsole::CmdSuffix() { return "redirect"; }
	const char* cRedirectConsole::CmdPrefix() { return "!"; }

	void cRedirectConsole::ListHead(ostream *os)
	{
		(*os) << "\r\n\r\n\t" << _("Hits");
		(*os) << "\t" << _("URL");
		(*os) << "\t\t\t\t" << _("Status");
		(*os) << "\t" << _("Type");
		(*os) << "\r\n\t" << string(110, '-') << "\r\n";
	}

	bool cRedirectConsole::IsConnAllowed(cConnDC *conn, int cmd)
	{
		return (conn && conn->mpUser && (conn->mpUser->mClass >= eUC_ADMIN));
	}

	}; // namespace nTables
}; // namespace nVerliHub
