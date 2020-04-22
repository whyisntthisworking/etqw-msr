Demonware master server variables seem to be in CAPITAL LETTERS eg; MAX_HOT_SERVERS
a class sdNetManager seems to handle any incoming demonware messages 
sd stands for serviceDemonware???
SS stands for Service State

================
sdHotServerList::IsServerValid
================
*/
bool sdHotServerList::IsServerValid( const sdNetSession& session, sdNetManager& manager ) {
	const int MIN_SENSIBLE_PLAYER_LIMIT		= 16;

	if ( manager.SessionIsFiltered( session, true ) ) {
		return false;
	}

	if( session.IsRepeater() ) {
		return false;
	}

	if ( session.GetServerInfo().GetBool( "si_needPass" ) ) {
		return false;
	}

	int maxPlayers = session.GetServerInfo().GetInt( "si_maxPlayers" );
	if ( maxPlayers < MIN_SENSIBLE_PLAYER_LIMIT ) {
		return false; // Gordon: ignore servers with silly player limit
	}
	if ( session.GetNumClients() == maxPlayers ) {
		return false;
	}

	return true;
}

================
sdNetManager::Connect
================
*/
bool sdNetManager::Connect() {
	assert( networkService->GetState() == sdNetService::SS_INITIALIZED );

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	return ( activeTask->Set( networkService->Connect(), &sdNetManager::OnConnect ) );
}

/*
================
sdNetManager::OnConnect
================
*/
void sdNetManager::OnConnect( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() != SDNET_NO_ERROR ) {
		properties.SetTaskResult( task->GetErrorCode(), declHolder.FindLocStr( va( "sdnet/error/%d", task->GetErrorCode() ) ) );
		properties.ConnectFailed();
	}
}

//looks like this contructs a network packet
/*
============
va
does a varargs printf into a temp buffer
NOTE: not thread safe
============
*/
#define VA_BUF_LEN 16384
wchar_t *va( const wchar_t *fmt, ... ) {
	va_list argptr;
	static int index = 0;
	static wchar_t string[4][VA_BUF_LEN];	// in case called by nested functions
	wchar_t *buf;

	buf = string[index];
	index = (index + 1) & 3;

	va_start( argptr, fmt );
	idWStr::vsnPrintf( buf, VA_BUF_LEN, fmt, argptr );
	va_end( argptr );

	return buf;
}

/*
============
va

does a varargs printf into a temp buffer
NOTE: not thread safe
============
*/
char *va( const char *fmt, ... ) {
	va_list argptr;
	static int index = 0;
	static char string[4][16384];	// in case called by nested functions
	char *buf;

	buf = string[index];
	index = (index + 1) & 3;

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}

char *vva( char *buf, const char *fmt, ... ) {
	va_list argptr;

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}

/*
=================
va_floatstring
=================
*/
char* va_floatstring( const char *fmt, ... ) {
	va_list argPtr;
	static int bufferIndex = 0;
	static char string[4][16384];	// in case called by nested functions
	char *buf;

	buf = string[bufferIndex];
	bufferIndex = (bufferIndex + 1) & 3;

	long i;
	unsigned long u;
	double f;
	char *str;
	int index;
	idStr tmp, format;

	index = 0;

	va_start( argPtr, fmt );
	while( *fmt ) {
		switch( *fmt ) {
			case '%':
				format = "";
				format += *fmt++;
				while ( (*fmt >= '0' && *fmt <= '9') ||
					*fmt == '.' || *fmt == '-' || *fmt == '+' || *fmt == '#') {
						format += *fmt++;
					}
					format += *fmt;
					switch( *fmt ) {
						case 'f':
						case 'e':
						case 'E':
						case 'g':
						case 'G':
							f = va_arg( argPtr, double );
							if ( format.Length() <= 2 ) {
								// high precision floating point number without trailing zeros
								sprintf( tmp, "%1.10f", f );
								tmp.StripTrailing( '0' );
								tmp.StripTrailing( '.' );
								index += sprintf( buf+index, "%s", tmp.c_str() );
							}
							else {
								index += sprintf( buf+index, format.c_str(), f );
							}
							break;
						case 'd':
						case 'i':
							i = va_arg( argPtr, long );
							index += sprintf( buf+index, format.c_str(), i );
							break;
						case 'u':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'o':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'x':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'X':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'c':
							i = va_arg( argPtr, long );
							index += sprintf( buf+index, format.c_str(), (char) i );
							break;
						case 's':
							str = va_arg( argPtr, char * );
							index += sprintf( buf+index, format.c_str(), str );
							break;
						case '%':
							index += sprintf( buf+index, format.c_str() );
							break;
						default:
							common->Error( "FS_WriteFloatString: invalid format %s", format.c_str() );
							break;
					}
					fmt++;
					break;
			case '\\':
				fmt++;
				switch( *fmt ) {
			case 't':
				index += sprintf( buf+index, "\t" );
				break;
			case 'v':
				index += sprintf( buf+index, "\v" );
				break;
			case 'n':
				index += sprintf( buf+index, "\n" );
				break;
			case '\\':
				index += sprintf( buf+index, "\\" );
				break;
			default:
				common->Error( "FS_WriteFloatString: unknown escape character \'%c\'", *fmt );
				break;
				}
				fmt++;
				break;
			default:
				index += sprintf( buf+index, "%c", *fmt );
				fmt++;
				break;
		}
	}
	va_end( argPtr );

	return buf;
}


//possibly sets up client-side packet responses
//ALLOC_FUNC format (for packet construction?)
// name - internal name for demonware message
// returntype - ?
// params - ?
// function - runs internal function for corresponding demonware message
/*
================
sdNetManager::InitFunctions
================
*/

#define ALLOC_FUNC( name, returntype, parms, function ) uiFunctions.Set( name, new uiFunction_t( returntype, parms, function ) )
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdNetProperties )
void sdNetManager::InitFunctions() {
	SD_UI_FUNC_TAG( connect, "Connecting to online service." )
		SD_UI_FUNC_RETURN_PARM( float, "True if connecting." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "connect",					'f', "",		&sdNetManager::Script_Connect );

	SD_UI_FUNC_TAG( validateUsername, "Validate the user name before creating an account." )
		SD_UI_FUNC_PARM( string, "username", "Username to be validated." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns valid/empty name/duplicate name/invalid name. See Username Validation defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "validateUsername",			'f', "s",		&sdNetManager::Script_ValidateUsername );

	SD_UI_FUNC_TAG( makeRawUsername, "Makes a raw username from the given username." )
		SD_UI_FUNC_PARM( string, "username", "Username to convert." )
		SD_UI_FUNC_RETURN_PARM( string, "Valid formatting for an account name." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "makeRawUsername",			's', "s",		&sdNetManager::Script_MakeRawUsername );

	SD_UI_FUNC_TAG( validateEmail, "Validate an email address." )
		SD_UI_FUNC_PARM( string, "email", "Email address." )
		SD_UI_FUNC_PARM( string, "confirmEmail", "Confirmation of email address." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns valid/empty name/duplicate name/invalid name. See Email Validation defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "validateEmail",			'f', "ss",		&sdNetManager::Script_ValidateEmail );

	SD_UI_FUNC_TAG( createUser, "Create a user." )
		SD_UI_FUNC_PARM( string, "username", "Username to create." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "createUser",				'v', "s",		&sdNetManager::Script_CreateUser );

	SD_UI_FUNC_TAG( deleteUser, "Delete a user. User should be logged out before trying to delete the account." )
		SD_UI_FUNC_PARM( string, "username", "username for user to delete." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deleteUser",				'v', "s",		&sdNetManager::Script_DeleteUser );

	SD_UI_FUNC_TAG( activateUser, "Log in as a user. No user should be logged in before calling this. Executes the users config." )
		SD_UI_FUNC_PARM( string, "username", "Username for user to log in." )
		SD_UI_FUNC_RETURN_PARM( float, "True if user activated." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "activateUser",				'f', "s",		&sdNetManager::Script_ActivateUser );

	SD_UI_FUNC_TAG( deactivateUser, "Log out, a user should be logged in before calling this." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deactivateUser",			'v', "",		&sdNetManager::Script_DeactivateUser );

	SD_UI_FUNC_TAG( saveUser, "Save user profile, config and bindings." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "saveUser",					'v', "",		&sdNetManager::Script_SaveUser );

	SD_UI_FUNC_TAG( setDefaultUser, "Sets a default user when starting up, skips login screen." )
		SD_UI_FUNC_PARM( string, "username", "Username for user." )
		SD_UI_FUNC_PARM( float, "set", "True if setting the username as default." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "setDefaultUser",			'v', "sf",		&sdNetManager::Script_SetDefaultUser );

	SD_UI_FUNC_TAG( getNumInterestedInServer, "Sets a default user when starting up, skips login screen." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( string, "address", "Address of the server." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns the number of players that are interested in joining a server." )
		SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getNumInterestedInServer",			'f', "fs",		&sdNetManager::Script_GetNumInterestedInServer );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( createAccount, "Create an account." )
		SD_UI_FUNC_PARM( string, "username", "Username for user." )
		SD_UI_FUNC_PARM( string, "password", "Password for account." )
		SD_UI_FUNC_PARM( string, "keyCode", "Valid Quake Wars key code." )
		SD_UI_FUNC_RETURN_PARM( float, "True if creating the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "createAccount",			'f', "sss",		&sdNetManager::Script_CreateAccount );

	SD_UI_FUNC_TAG( deleteAccount, "Delete an account." )
		SD_UI_FUNC_PARM( string, "password", "Password for account to delete." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deleteAccount",			'v', "s",		&sdNetManager::Script_DeleteAccount );

	SD_UI_FUNC_TAG( hasAccount, "Has account." )
		SD_UI_FUNC_RETURN_PARM( float, "True if logged in to an account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "hasAccountImmediate",				'f', "",		&sdNetManager::Script_HasAccount );

	SD_UI_FUNC_TAG( accountSetUsername, "Set a username for an account." )
		SD_UI_FUNC_PARM( string, "username", "Username to set." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountSetUsername",		'v', "s",		&sdNetManager::Script_AccountSetUsername );

	SD_UI_FUNC_TAG( accountPasswordSet, "Check if account requires a password." )
		SD_UI_FUNC_RETURN_PARM( float, "True if account has a password associated with it." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountPasswordSet",		'f', "",		&sdNetManager::Script_AccountIsPasswordSet );

	SD_UI_FUNC_TAG( accountSetPassword, "Set the password for an account." )
		SD_UI_FUNC_PARM( string, "password", "New password for the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountSetPassword",		'v', "s",		&sdNetManager::Script_AccountSetPassword );

	SD_UI_FUNC_TAG( accountResetPassword, "Reset an account password." )
		SD_UI_FUNC_PARM( string, "keyCode", "Key code associated with the account." )
		SD_UI_FUNC_PARM( string, "newPassword", "New password for the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountResetPassword",		'v', "ss",		&sdNetManager::Script_AccountResetPassword );

	SD_UI_FUNC_TAG( accountChangePassword, "Change account password." )
		SD_UI_FUNC_PARM( string, "password", "Password for the account." )
		SD_UI_FUNC_PARM( string, "newPassword", "New password for the account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "accountChangePassword",	'v', "ss",		&sdNetManager::Script_AccountChangePassword );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( signIn, "Sign in to demonware with the current account." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "signIn",					'v', "",		&sdNetManager::Script_SignIn );

	SD_UI_FUNC_TAG( signOut, "Sign out of demonware." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "signOut",					'v', "",		&sdNetManager::Script_SignOut );

	SD_UI_FUNC_TAG( getProfileString, "Get a key/val." )
		SD_UI_FUNC_PARM( string, "key", "Key." )
		SD_UI_FUNC_PARM( string, "defaultValue", "Default value." )
		SD_UI_FUNC_RETURN_PARM( string, "." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getProfileString",			's', "ss",		&sdNetManager::Script_GetProfileString );

	SD_UI_FUNC_TAG( setProfileString, "Set a key/val." )
		SD_UI_FUNC_PARM( string, "key", "Key." )
		SD_UI_FUNC_PARM( string, "value", "Value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "setProfileString",			'v', "ss",		&sdNetManager::Script_SetProfileString );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( assureProfileExists, "Make sure profile exists at demonware." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "assureProfileExists",		'v', "",		&sdNetManager::Script_AssureProfileExists );

	SD_UI_FUNC_TAG( storeProfile, "Store demonware profile." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "storeProfile",				'v', "",		&sdNetManager::Script_StoreProfile );

	SD_UI_FUNC_TAG( restoreProfile, "Restore demonware profile from demonware." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "restoreProfile",			'v', "",		&sdNetManager::Script_RestoreProfile );

#endif /* !SD_DEMO_BUILD */
	SD_UI_FUNC_TAG( validateProfile, "Validate user profile by requesting all profile properties." )
		SD_UI_FUNC_RETURN_PARM( float, "True on validated." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "validateProfile",			'f', "",		&sdNetManager::Script_ValidateProfile );

	SD_UI_FUNC_TAG( findServers, "Find servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "findServers",				'f', "f",		&sdNetManager::Script_FindServers );

	SD_UI_FUNC_TAG( addUnfilteredSession, "Add an unfiltered server with the specified IP:Port." )
		SD_UI_FUNC_PARM( string, "session", "Server IP:Port." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "addUnfilteredSession",		'v', "s",		&sdNetManager::Script_AddUnfilteredSession );

	SD_UI_FUNC_TAG( clearUnfilteredSessions, "Clear all unfiltered sessions." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "clearUnfilteredSessions",	'v', "",		&sdNetManager::Script_ClearUnfilteredSessions );

	SD_UI_FUNC_TAG( stopFindingServers, "Stop task for finding servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "stopFindingServers",		'v', "f",		&sdNetManager::Script_StopFindingServers );

	SD_UI_FUNC_TAG( refreshCurrentServers, "Refresh list of servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "True on success." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "refreshCurrentServers",	'f', "f",		&sdNetManager::Script_RefreshCurrentServers );

	SD_UI_FUNC_TAG( refreshServer, "Refresh a server." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "session index." )
		SD_UI_FUNC_PARM( string, "string", "Optional session as string (IP:Port)." )
		SD_UI_FUNC_RETURN_PARM( float, "True on success." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "refreshServer",			'f', "ffs",		&sdNetManager::Script_RefreshServer );

	SD_UI_FUNC_TAG( refreshHotServers, "Refresh all the hot servers." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( string, "string", "Optional session to ignore as string (IP:Port)." )
		SD_UI_FUNC_RETURN_PARM( float, "True on success." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "refreshHotServers",		'f', "fs",		&sdNetManager::Script_RefreshHotServers );

	SD_UI_FUNC_TAG( updateHotServers, "Updates the hot servers, removing any ones which are now invalid and finding new ones. Also tells the server the player is interested in them." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "updateHotServers",		'v', "f",		&sdNetManager::Script_UpdateHotServers );

	SD_UI_FUNC_TAG( joinBestServer, "Joins the server that is at the top of the hot server list." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "joinBestServer",		'v', "f",		&sdNetManager::Script_JoinBestServer );

	SD_UI_FUNC_TAG( getNumHotServers, "Gets the number of available hot servers" )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of hot servers." )
		SD_UI_END_FUNC_TAG
		ALLOC_FUNC( "getNumHotServers",		'f', "f",		&sdNetManager::Script_GetNumHotServers );

	SD_UI_FUNC_TAG( joinServer, "Join a server." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "session index." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "joinServer",				'v', "ff",		&sdNetManager::Script_JoinServer );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( checkKey, "Verify key against checksum." )
		SD_UI_FUNC_PARM( string, "keyCode", "Keycode." )
		SD_UI_FUNC_PARM( string, "checksum", "Checksum." )
		SD_UI_FUNC_RETURN_PARM( float, "True if keycode looks valid." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "checkKey",					'f', "ss",		&sdNetManager::Script_CheckKey );

	SD_UI_FUNC_TAG( initFriends, "Create friends task for initializing list of friends." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "initFriends",				'v', "",		&sdNetManager::Script_Friend_Init );

	SD_UI_FUNC_TAG( proposeFriendship, "Propose a friendship to another player." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to propose friendship to." )
		SD_UI_FUNC_PARM( wstring, "reason", "Reason for proposing a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "proposeFriendship",		'v', "sw",		&sdNetManager::Script_Friend_ProposeFriendShip );

	SD_UI_FUNC_TAG( acceptProposal, "Accept proposal for friendship." )
		SD_UI_FUNC_PARM( string, "username", "Username for player that proposed a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "acceptProposal",			'v', "s",		&sdNetManager::Script_Friend_AcceptProposal );

	SD_UI_FUNC_TAG( rejectProposal, "Reject proposal for friendship." )
		SD_UI_FUNC_PARM( string, "username", "Username for player that proposed a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "rejectProposal",			'v', "s",		&sdNetManager::Script_Friend_RejectProposal );

	SD_UI_FUNC_TAG( removeFriend, "Remove friendship." )
		SD_UI_FUNC_PARM( string, "username", "Username for player that proposed a friendship." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "removeFriend",				'v', "s",		&sdNetManager::Script_Friend_RemoveFriend );

	SD_UI_FUNC_TAG( setBlockedStatus, "Set block status for a user." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to set blocked status for." )
		SD_UI_FUNC_PARM( handle, "blockStatus", "See BS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "setBlockedStatus",			'v', "si",		&sdNetManager::Script_Friend_SetBlockedStatus );

	SD_UI_FUNC_TAG( getBlockedStatus, "Get blocked status for a user. See BS_* for defines." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to get blocked status for." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns blocked status." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getBlockedStatus",			'f', "s",		&sdNetManager::Script_Friend_GetBlockedStatus );

	SD_UI_FUNC_TAG( sendIM, "Send instant message." )
		SD_UI_FUNC_PARM( string, "username", "Username for player to send instant message to." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to send." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "sendIM",					'v', "sw",		&sdNetManager::Script_Friend_SendIM );

	SD_UI_FUNC_TAG( getIMText, "Get instant message text from user." )
		SD_UI_FUNC_PARM( string, "username", "Friend to get instant message from." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Text." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getIMText",				'w', "s",		&sdNetManager::Script_Friend_GetIMText );

	SD_UI_FUNC_TAG( getProposalText, "Get text for friendship proposal." )
		SD_UI_FUNC_PARM( string, "username", "User to get text from." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Text." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getProposalText",			'w', "s",		&sdNetManager::Script_Friend_GetProposalText );

	SD_UI_FUNC_TAG( inviteFriend, "Invite a friend to a session." )
		SD_UI_FUNC_PARM( string, "username", "User to invite." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "inviteFriend",				'v', "s",		&sdNetManager::Script_Friend_InviteFriend );

	SD_UI_FUNC_TAG( isFriend, "Check if a user is a friend." )
		SD_UI_FUNC_PARM( string, "username", "User of friend." )
		SD_UI_FUNC_RETURN_PARM( float, "True if friend." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isFriend",					'f', "s",		&sdNetManager::Script_IsFriend );

	SD_UI_FUNC_TAG( isPendingFriend, "See if a user is a pending friend." )
		SD_UI_FUNC_PARM( string, "username", "User to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if friendship is pending." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isPendingFriend",			'f', "s",		&sdNetManager::Script_IsPendingFriend );

	SD_UI_FUNC_TAG( isInvitedFriend, "Check if invited to a session by a friend." )
		SD_UI_FUNC_PARM( string, "username", "Friend name." )
		SD_UI_FUNC_RETURN_PARM( float, "True if invited by friend." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isInvitedFriend",			'f', "s",		&sdNetManager::Script_IsInvitedFriend );

	SD_UI_FUNC_TAG( isFriendOnServer, "Check if a friend is on a server. If friend is on a server the server port:ip is stored in profile key currentServer." )
		SD_UI_FUNC_PARM( string, "username", "Friend name." )
		SD_UI_FUNC_RETURN_PARM( float, "True if friend is on a server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isFriendOnServer",			'f', "s",		&sdNetManager::Script_IsFriendOnServer );

	SD_UI_FUNC_TAG( followFriendToServer, "Follow a friend to a server." )
		SD_UI_FUNC_PARM( string, "username", "Friend to follow." )
		SD_UI_FUNC_RETURN_PARM( float, "True if connecting." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "followFriendToServer",		'f', "s",		&sdNetManager::Script_FollowFriendToServer );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( isSelfOnServer, "Check if local player is on a server." )
		SD_UI_FUNC_RETURN_PARM( float, "True if on a server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isSelfOnServer",			'f', "",		&sdNetManager::Script_IsSelfOnServer );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( loadMessageHistory, "Load history with a user." )
		SD_UI_FUNC_PARM( float, "source", "Source for message history. See MHS_* defines." )
		SD_UI_FUNC_PARM( string, "username", "User to get message history from." )
		SD_UI_FUNC_RETURN_PARM( float, "True if message history loaded." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "loadMessageHistory",		'f', "fs",		&sdNetManager::Script_LoadMessageHistory );

	SD_UI_FUNC_TAG( unloadMessageHistory, "Unload a previously loaded message history." )
		SD_UI_FUNC_PARM( float, "source", "Source for message history. See MHS_* defines." )
		SD_UI_FUNC_PARM( string, "username", "User to get message history from." )
		SD_UI_FUNC_RETURN_PARM( float, "True if message history unloaded." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "unloadMessageHistory",		'f', "fs",		&sdNetManager::Script_UnloadMessageHistory );

	SD_UI_FUNC_TAG( addToMessageHistory, "Add a message to the message history." )
		SD_UI_FUNC_PARM( float, "source", "Source for message history. See MHS_* defines." )
		SD_UI_FUNC_PARM( string, "username", "User from which message history is with." )
		SD_UI_FUNC_PARM( string, "fromUser", "Message from user." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to add." )
		SD_UI_FUNC_RETURN_PARM( float, "True if message added to message history." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "addToMessageHistory",		'f', "fssw",	&sdNetManager::Script_AddToMessageHistory );

	SD_UI_FUNC_TAG( getUserNamesForKey, "Get usernames for keycode." )
		SD_UI_FUNC_PARM( string, "keyCode", "Keycode." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task was created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getUserNamesForKey",		'f', "s",		&sdNetManager::Script_GetUserNamesForKey );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( getPlayerCount, "Get number of players across all sessions (minus bots)." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of players." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getPlayerCount",			'f', "f",		&sdNetManager::Script_GetPlayerCount );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( chooseContextActionForFriend, "Get current context action for friend." )
		SD_UI_FUNC_PARM( string, "username", "Username for friend." )
		SD_UI_FUNC_RETURN_PARM( float, "Current context action. See FCA_*." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "chooseContextActionForFriend",	'f', "s",	&sdNetManager::Script_Friend_ChooseContextAction );

	SD_UI_FUNC_TAG( numAvailableIMs, "Get number of available instant messsages." )
		SD_UI_FUNC_PARM( string, "username", "Username of friend." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of available instant messages." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "numAvailableIMs",				'f', "s",	&sdNetManager::Script_Friend_NumAvailableIMs );

	SD_UI_FUNC_TAG( deleteActiveMessage, "Delete current active message." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "deleteActiveMessage",		'v', "",		&sdNetManager::Script_DeleteActiveMessage );

	SD_UI_FUNC_TAG( clearActiveMessage, "Clear active message." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "clearActiveMessage",		'v', "",		&sdNetManager::Script_ClearActiveMessage );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( queryServerInfo, "Query a server for key/val." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_PARM( string, "key", "Key to get." )
		SD_UI_FUNC_PARM( string, "defaultValue", "Default value for key." )
		SD_UI_FUNC_RETURN_PARM( string, "Value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryServerInfo",			's', "ffss",	&sdNetManager::Script_QueryServerInfo );

	SD_UI_FUNC_TAG( queryMapInfo, "Query a server map info for a key/val." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_PARM( string, "key", "Key to get." )
		SD_UI_FUNC_PARM( string, "defaultValue", "Default value for key." )
		SD_UI_FUNC_RETURN_PARM( string, "Value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryMapInfo",				's', "ffss",	&sdNetManager::Script_QueryMapInfo );

	SD_UI_FUNC_TAG( queryGameType, "Query the game type for a server." )
		SD_UI_FUNC_PARM( float, "source", "Source to get servers from. See FS_* defines." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Gametype for session." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryGameType",			'w', "ff",		&sdNetManager::Script_QueryGameType );

	SD_UI_FUNC_TAG( requestStats, "Create request stats task." )
		SD_UI_FUNC_PARM( float, "globalOnly", "If true only use profile stats retrieved from demonware." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "requestStats",				'f', "f",		&sdNetManager::Script_RequestStats );

	SD_UI_FUNC_TAG( getStat, "Get stat for player." )
		SD_UI_FUNC_PARM( string, "key", "Key for stat." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns float or handle depending on key." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getStat",					'f', "s",		&sdNetManager::Script_GetStat );

	SD_UI_FUNC_TAG( getPlayerAward, "Get player award." )
		SD_UI_FUNC_PARM( float, "rewardType", "Reward type." )
		SD_UI_FUNC_PARM( float, "statType", "Stat type, rank/xp/name." )
		SD_UI_FUNC_PARM( float, "isSelf", "If true, returns value for local player" )
		SD_UI_FUNC_RETURN_PARM( wstring, "Award converted to wide string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getPlayerAward",			'w', "fff",		&sdNetManager::Script_GetPlayerAward );

	SD_UI_FUNC_TAG( queryXPStats, "Query XP stats." )
		SD_UI_FUNC_PARM( string, "prof", "Proficiency category." )
		SD_UI_FUNC_PARM( float, "total", "True if total XP, false if per-map XP." )
		SD_UI_FUNC_RETURN_PARM( string, "XP converted to a string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryXPStats",				's', "sf",		&sdNetManager::Script_QueryXPStats );

	SD_UI_FUNC_TAG( queryXPTotals, "Query for total XP." )
		SD_UI_FUNC_PARM( float, "total", "True if total XP, false if per-map XP." )
		SD_UI_FUNC_RETURN_PARM( string, "Returns total XP." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryXPTotals",			's', "f",		&sdNetManager::Script_QueryXPTotals );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( initTeams, "Init team manager." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "initTeams",				'f', "",		&sdNetManager::Script_Team_Init );

	SD_UI_FUNC_TAG( createTeam, "Create a team." )
		SD_UI_FUNC_PARM( string, "teamName", "Team name." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "createTeam",				'v', "s",		&sdNetManager::Script_Team_CreateTeam );

	SD_UI_FUNC_TAG( proposeMembership, "Propose team membership to user." )
		SD_UI_FUNC_PARM( string, "username", "User to propose team membership to." )
		SD_UI_FUNC_PARM( wstring, "message", "Message text." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "proposeMembership",		'v', "sw",		&sdNetManager::Script_Team_ProposeMembership );	// username, reason

	SD_UI_FUNC_TAG( acceptMembership, "Accept team membership from username." )
		SD_UI_FUNC_PARM( string, "username", "User that sent membership." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "acceptMembership",			'f', "s",		&sdNetManager::Script_Team_AcceptMembership );
	
	SD_UI_FUNC_TAG( rejectMembership, "Reject a team membership proposal." )
		SD_UI_FUNC_PARM( string, "username", "User that sent membership." )
		SD_UI_FUNC_RETURN_PARM( float, "True if task created." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "rejectMembership",			'f', "s",		&sdNetManager::Script_Team_RejectMembership );

	SD_UI_FUNC_TAG( removeMember, "Remove a member from a team." )
		SD_UI_FUNC_PARM( string, "username", "User to remove from team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "removeMember",				'v', "s",		&sdNetManager::Script_Team_RemoveMember );

	SD_UI_FUNC_TAG( sendTeamMessage, "Send a message to a team member." )
		SD_UI_FUNC_PARM( string, "username", "Username of team player." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to send." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "sendTeamMessage",			'v', "sw",		&sdNetManager::Script_Team_SendMessage );

	SD_UI_FUNC_TAG( broadcastTeamMessage, "Broadcast message to all members of a team. Only team owner/admin may broadcast messages." )
		SD_UI_FUNC_PARM( wstring, "message", "Message to broadcast." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "broadcastTeamMessage",		'v', "w",		&sdNetManager::Script_Team_BroadcastMessage );

	SD_UI_FUNC_TAG( inviteMember, "Invite a member to a game." )
		SD_UI_FUNC_PARM( string, "username", "User to invite to the game." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "inviteMember",				'v', "s",		&sdNetManager::Script_Team_Invite );

	SD_UI_FUNC_TAG( promoteMember, "Promote a member to admin." )
		SD_UI_FUNC_PARM( string, "username", "User to promote." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "promoteMember",			'v', "s",		&sdNetManager::Script_Team_PromoteMember );

	SD_UI_FUNC_TAG( demoteMember, "Demote a player from being an admin." )
		SD_UI_FUNC_PARM( string, "username", "User to demote from admin status." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "demoteMember",				'v', "s",		&sdNetManager::Script_Team_DemoteMember );

	SD_UI_FUNC_TAG( transferOwnership, "Transfer ownership of a team." )
		SD_UI_FUNC_PARM( string, "username", "User to get ownership of team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "transferOwnership",		'v', "s",		&sdNetManager::Script_Team_TransferOwnership );

	SD_UI_FUNC_TAG( disbandTeam, "Create task to disband a team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "disbandTeam",				'v', "",		&sdNetManager::Script_Team_DisbandTeam );

	SD_UI_FUNC_TAG( leaveTeam, "Leave a team." )
		SD_UI_FUNC_PARM( string, "username", "User to leave the team." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "leaveTeam",				'v', "s",		&sdNetManager::Script_Team_LeaveTeam );

	SD_UI_FUNC_TAG( chooseTeamContextAction, "Let gamecode choose a team context action." )
		SD_UI_FUNC_PARM( string, "username", "User to choose a team context action on." )
		SD_UI_FUNC_RETURN_PARM( float, "Context action. See the TCA_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "chooseTeamContextAction",	'f', "s",		&sdNetManager::Script_Team_ChooseContextAction );

	SD_UI_FUNC_TAG( isTeamMember, "Check if a player is a team member." )
		SD_UI_FUNC_PARM( string, "membername", "Name of player to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if player is a team member." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isTeamMember",				'f', "s",		&sdNetManager::Script_Team_IsMember );

	SD_UI_FUNC_TAG( isPendingTeamMember, "Member waiting approval to join." )
		SD_UI_FUNC_PARM( string, "membername", "Member name of pending member." )
		SD_UI_FUNC_RETURN_PARM( float, "True if member is pending approval to join." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isPendingTeamMember",		'f', "s",		&sdNetManager::Script_Team_IsPendingMember );

	SD_UI_FUNC_TAG( getTeamIMText, "Get the instant message from team member." )
		SD_UI_FUNC_PARM( string, "teammember", "User to get instant message from." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Instant message received." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getTeamIMText",			'w', "s",		&sdNetManager::Script_Team_GetIMText );

	SD_UI_FUNC_TAG( numAvailableTeamIMs, "Number of available instant messages from team member." )
		SD_UI_FUNC_PARM( string, "membername", "Team member to check number of messages from." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of available instant messages." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "numAvailableTeamIMs",		'f', "s",		&sdNetManager::Script_Team_NumAvailableIMs );

	SD_UI_FUNC_TAG( getMemberStatus, "Get status of a team member." )
		SD_UI_FUNC_PARM( string, "membername", "member to get status of." )
		SD_UI_FUNC_RETURN_PARM( float, "Member status. See MS_* for defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMemberStatus",			'f', "s",		&sdNetManager::Script_Team_GetMemberStatus );

	SD_UI_FUNC_TAG( isMemberOnServer, "Check if member is on a server." )
		SD_UI_FUNC_PARM( string, "membmername", "Member to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if member is on a server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "isMemberOnServer",			'f', "s",		&sdNetManager::Script_IsMemberOnServer );

	SD_UI_FUNC_TAG( followMemberToServer, "Follow a member to a server." )
		SD_UI_FUNC_PARM( string, "membmername", "Member to check." )
		SD_UI_FUNC_RETURN_PARM( float, "True if trying to connecting to server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "followMemberToServer",		'f', "s",		&sdNetManager::Script_FollowMemberToServer );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( clearFilters, "Clear all server filters (both numeric and string filters)." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "clearFilters",				'v', "",		&sdNetManager::Script_ClearFilters );

	SD_UI_FUNC_TAG( applyNumericFilter, "Apply a numeric filter to servers." )
		SD_UI_FUNC_PARM( float, "type", "See FS_* defines." )
		SD_UI_FUNC_PARM( float, "state", "See SFS_* defines." )
		SD_UI_FUNC_PARM( float, "operation", "See SFO_* defines." )
		SD_UI_FUNC_PARM( float, "resultBin", "Operator when combining with other filters. See SFR_*." )
		SD_UI_FUNC_PARM( float, "value", "Optional additional value." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "applyNumericFilter",		'v', "fffff",	&sdNetManager::Script_ApplyNumericFilter );

	SD_UI_FUNC_TAG( applyStringFilter, "Apply a string filter to servers." )
		SD_UI_FUNC_PARM( float, "cvar", "Cvar to check for string filter." )
		SD_UI_FUNC_PARM( float, "state", "See SFS_* defines." )
		SD_UI_FUNC_PARM( float, "operation", "See SFO_* defines." )
		SD_UI_FUNC_PARM( float, "resultBin", "Operator when combining with other filters. See SFR_*." )
		SD_UI_FUNC_PARM( float, "value", "Optional additional value." )	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "applyStringFilter",		'v', "sfffs",	&sdNetManager::Script_ApplyStringFilter );

	SD_UI_FUNC_TAG( saveFilters, "Save filters in the profile under keys with name 'filter_<prfix>_*'." )
		SD_UI_FUNC_PARM( string, "prefix", "Prefix for filters." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "saveFilters",				'v', "s",		&sdNetManager::Script_SaveFilters );

	SD_UI_FUNC_TAG( loadFilters, "Load filters from profile with the specified prefix." )
		SD_UI_FUNC_PARM( string, "prefix", "Prefix for filters." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "loadFilters",				'v', "s",		&sdNetManager::Script_LoadFilters );

	SD_UI_FUNC_TAG( queryNumericFilterValue, "Query the value of a numeric filter." )
		SD_UI_FUNC_PARM( float, "type", "See SF_* defines." )
		SD_UI_FUNC_RETURN_PARM( float, "Value of filter." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryNumericFilterValue",	'f', "f",		&sdNetManager::Script_QueryNumericFilterValue );

	SD_UI_FUNC_TAG( queryNumericFilterState, "Query state of a numeric filter." )
		SD_UI_FUNC_PARM( float, "type", "See SF_* defines." )
		SD_UI_FUNC_PARM( float, "value", "Value to use for filter." )
		SD_UI_FUNC_RETURN_PARM( float, "See SFS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryNumericFilterState",	'f', "ff",		&sdNetManager::Script_QueryNumericFilterState );

	SD_UI_FUNC_TAG( queryStringFilterState, "Get state for string filter." )
		SD_UI_FUNC_PARM( string, "cvar", "CVar name for filter." )
		SD_UI_FUNC_PARM( float, "value", "Value to use for filter." )
		SD_UI_FUNC_RETURN_PARM( float, "See SFS_* defines." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryStringFilterState",	'f', "ss",		&sdNetManager::Script_QueryStringFilterState );

	SD_UI_FUNC_TAG( queryStringFilterValue, "Get value for string filter." )
		SD_UI_FUNC_PARM( string, "cvar", "CVar name for filter." )
		SD_UI_FUNC_RETURN_PARM( string, "Value for filter." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "queryStringFilterValue",	's', "s",		&sdNetManager::Script_QueryStringFilterValue );

	SD_UI_FUNC_TAG( joinSession, "Join a session based an invite." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "joinSession",				'v', "",		&sdNetManager::Script_JoinSession );

	SD_UI_FUNC_TAG( getMotdString, "Get message of the day string for current server." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Message of the day." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMotdString",			'w', "",		&sdNetManager::Script_GetMotdString );

	SD_UI_FUNC_TAG( getServerStatusString, "Get formatted server status (warmup/loading/running)." )
		SD_UI_FUNC_PARM( float, "source", "Source from which server is in." )
		SD_UI_FUNC_PARM( float, "index", "Session index." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Returns status strings." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getServerStatusString",	'w', "ff",		&sdNetManager::Script_GetServerStatusString );

	SD_UI_FUNC_TAG( toggleServerFavoriteState, "Toggle a server as favorite." )
		SD_UI_FUNC_PARM( string, "session", "IP:Port." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "toggleServerFavoriteState",'v', "s",		&sdNetManager::Script_ToggleServerFavoriteState );

#if !defined( SD_DEMO_BUILD )
	SD_UI_FUNC_TAG( getFriendServerIP, "Get friend server ip." )
		SD_UI_FUNC_PARM( string, "friend", "Friends name." )
		SD_UI_FUNC_RETURN_PARM( string, "IP:Port or empty string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getFriendServerIP",		's', "s",		&sdNetManager::Script_GetFriendServerIP );

	SD_UI_FUNC_TAG( getMemberServerIP, "Get member server ip." )
		SD_UI_FUNC_PARM( string, "member", "Members name." )
		SD_UI_FUNC_RETURN_PARM( string, "IP:Port or empty string." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMemberServerIP",		's', "s",		&sdNetManager::Script_GetMemberServerIP );

	SD_UI_FUNC_TAG( getMessageTimeStamp, "Get time left on server." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Time left on server." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getMessageTimeStamp",		'w', "",		&sdNetManager::Script_GetMessageTimeStamp );

	SD_UI_FUNC_TAG( getServerInviteIP, "Get IP of server invite." )
		SD_UI_FUNC_RETURN_PARM( string, "IP:Port of server invite." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "getServerInviteIP",		's', "",		&sdNetManager::Script_GetServerInviteIP );
#endif /* !SD_DEMO_BUILD */

	SD_UI_FUNC_TAG( cancelActiveTask, "Cancel currently active task." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "cancelActiveTask",			'v', "",		&sdNetManager::Script_CancelActiveTask );

	SD_UI_FUNC_TAG( formatSessionInfo, "Print out formatted session info." )
		SD_UI_FUNC_PARM( string, "session", "IP:Port of session." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Formatted session info." )
	SD_UI_END_FUNC_TAG
	ALLOC_FUNC( "formatSessionInfo",		'w', "s",		&sdNetManager::Script_FormatSessionInfo );			// IP:Port
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

#undef ALLOC_FUNC


//possibly pulls CD key from registry and sends it to demoneware???
idCVar net_accountName( "net_accountName", "", CVAR_INIT, "Auto login account name" );
idCVar net_accountPassword( "net_accountPassword", "", CVAR_INIT, "Auto login account password" );
idCVar net_autoConnectServer( "net_autoConnectServer", "", CVAR_INIT, "Server to connect to after auto login is complete" );