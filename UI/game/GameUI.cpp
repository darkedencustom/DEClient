//-----------------------------------------------------------------------------
// GameUI.cpp
//-----------------------------------------------------------------------------
// UI�� message�� ó���ϴ� �κ��̴�.
//-----------------------------------------------------------------------------
#include "Client_PCH.h"

#include <Winuser.h>

#include "Client.h"
#include "UIFunction.h"
#include "VS_UI.h" 
#include "VS_UI_Mouse_pointer.h"
#include "PacketDef.h"
#include "TalkBox.h"
#include "MShopShelf.h"
#include "MPriceManager.h"
#include "MMoneyManager.h"
#include "UIDialog.h"
#include "PCSlayerInfo.h"
#include "PCVampireInfo.h"
#include "PCOustersInfo.h"
#include "SkillDef.h"
#include "ClientFunction.h"
#include "MGameStringTable.h"
#include "MItemOptionTable.h"
#include "CToken.h"
#include "UserOption.h"
#include "UtilityFunction.h"
#include "MWeather.h"
#include "MStatusManager.h"
#include "MChatManager.h"
#include "UIMessageManager.h"
#include "ServerInformation.h"
#include "UserInformation.h"
#include "TempInformation.h"
#include "ClientConfig.h"
#include "MMusic.h"
//#include "MFileDef.h"
#include "Properties.h"
#include "ServerInfo.h"
#include "DebugInfo.h"
#include "GuildInfo.h"
#include "MGuildInfoMapper.h"
//#include "KeyAccelerator.h"
#include "Gpackets/GCWaitGuildList.h"
#include "Gpackets/GCActiveGuildList.h"
#include "Gpackets/GCGuildMemberList.h"
#include "Gpackets/GCShowWaitGuildInfo.h"
#include "Gpackets/GCShowGuildInfo.h"
#include "Gpackets/GCShowGuildMemberInfo.h"
#include "Gpackets/GCWarScheduleList.h"
#include "Gpackets/GCBloodBibleStatus.h"
#include "Gpackets/GCSelectQuestID.h"
#include "Gpackets/GCQuestStatus.h"
#include "Gpackets/GCGoodsList.h"
#include "Gpackets/GCShowUnionInfo.h"
#include "CMP3.h"
#include "AddonDef.h"
#include "VS_UI_filepath.h"
#include "PacketFunction.h"
#include "MFakeCreature.h"
#include "MMonsterKillQuestInfo.h"
#include "MZoneTable.h"
#include <string>
#include "SoundSetting.h"
#include "GPackets/GCMiniGameScores.h"
#include "SystemAvailabilities.h"
#include "VS_UI_GameCommon2.h"
#if __CONTENTS(__FRIEND_ADDITION)
#include "VS_UI_Friend_System.h"
#endif //__FRIEND_ADDITION
//#include "mintr.h"
#include "MGameTime.h"
#ifdef OUTPUT_DEBUG
	#include <io.h>
#endif

#include "Cpackets/CLGetWorldList.h"

#include "BGMManager.h"

extern Race g_eRaceInterface;

BOOL	g_bSetHotKey = FALSE;

BOOL g_bDrawProgress = FALSE;


//extern IWebBrowser2*			g_pWebBrowser; 
//-----------------------------------------------------------------------------
// �ϼ��� --> ������
//-----------------------------------------------------------------------------
/*
void
UI_WansungToJohap(const char* wansung, char* johap)
{
	C_HAN_VIEW::WansungToJohap(wansung, johap);
}
*/

//-----------------------------------------------------------------------------
//  ������--> �ϼ���
//-----------------------------------------------------------------------------
/*
void
UI_JohapToWansung(const char* johap, char* wansung)
{
	C_HAN_VIEW::JohabToWansung(wansung, johap);
}
*/

//-----------------------------------------------------------------------------
// SelectSkill
//-----------------------------------------------------------------------------
/*
void				
UI_SelectSkill( int selectSkill )
{
	if (g_pActionInfoTable!=NULL 
		&& selectSkill < g_pActionInfoTable->GetMinResultActionInfo())
	{
		gC_vs_ui.SelectSkill( selectSkill );
	}
}
*/

//-----------------------------------------------------------------------------
// Is Running SelectWayPoint
//-----------------------------------------------------------------------------
bool
UI_IsRunningSelectWayPoint()
{
	return gC_vs_ui.IsRunningSlayerPortal();
}

//-----------------------------------------------------------------------------
// Run SelectWayPoint
//-----------------------------------------------------------------------------
void
UI_RunSelectWayPoint( int nExtendLevel )
{
	gC_vs_ui.RunSlayerPortal( nExtendLevel );
}

//-----------------------------------------------------------------------------
// Close SelectWayPoint
//-----------------------------------------------------------------------------
void
UI_CloseSelectWayPoint()
{
	gC_vs_ui.CloseSlayerPortal();
}

//-----------------------------------------------------------------------------
// Save UserOption
//-----------------------------------------------------------------------------
// UI�� UserOption������ ȭ�Ϸ� �����Ѵ�.
//-----------------------------------------------------------------------------
void		
UI_SaveUserOption()
{			
	if (g_pUserOption!=NULL)
	{
//		g_pUserOption->OpenQuickSlot = gC_vs_ui.IsRunningQuickItemSlot();		
//		g_pUserOption->DrawMinimap = gC_vs_ui.IsRunningMinimap();
	
		g_pUserOption->SaveToFile( g_pFileDef->getProperty("FILE_INFO_USEROPTION").c_str());
	}	
}

//-----------------------------------------------------------------------------
// Add EffectStatus
//-----------------------------------------------------------------------------
void		
UI_AffectUserOption()
{
	if (g_pUserOption!=NULL)
	{
//		bool bOldMute = g_DXSound.IsMute();
//
//		g_DXSound.SetMute();

		//--------------------------------------------------------------
		// QuickSlot
		//--------------------------------------------------------------
		//if (g_pUserOption->OpenQuickSlot)
		{
//			gC_vs_ui.RunQuickItemSlot();	
		}
		//else
		{
		//	gC_vs_ui.CloseQuickItemSlot();
		}

		//--------------------------------------------------------------
		// Minimap
		//--------------------------------------------------------------
//		if (g_pUserOption->DrawMinimap)
//		{
//			gC_vs_ui.RunMinimap();	
//		}
//		else
//		{
//			gC_vs_ui.CloseMinimap();
//		}
//
//		if (!bOldMute)		
//		{
//			g_DXSound.UnSetMute();
//		}
//		if(g_pUserOption->UseEnterChat)
//			g_pKeyAccelerator->SetEnterChat();
//		else
//			g_pKeyAccelerator->UnsetEnterChat();
	}
}

//-----------------------------------------------------------------------------
// Add EffectStatus
//-----------------------------------------------------------------------------
void		
UI_AddEffectStatus(int es, DWORD delayFrame)
{
#if __CONTENTS(!__JAPAN_UI)
	if (delayFrame <= 0)
		return;
#endif

	if (es < g_pEffectStatusTable->GetSize())
	{
		TYPE_ACTIONINFO ai = (*g_pEffectStatusTable)[es].OriginalActionInfo;

		//---------------------------------------------------------------
		// EffectStatus�� ���õ� ActionInfo�� �ִ� ���
		//---------------------------------------------------------------
		if (ai < g_pActionInfoTable->GetMinResultActionInfo())
		{
			S_SLOT::UI_EFFECTSTATUS_TYPE& status = g_char_slot_ingame.STATUS;

			//-----------------------------------------------------------------
			// �̹� �ִ��� �˻�.. ��.. vector��.. - -;
			//-----------------------------------------------------------------
			S_SLOT::UI_EFFECTSTATUS_TYPE::iterator itr = status.begin(); 
			S_SLOT::UI_EFFECTSTATUS_TYPE::iterator endItr = status.end();

			while (itr != status.end())
			{
				//-----------------------------------------------------------------
				// �̹� ������ return�̴�..
				//-----------------------------------------------------------------
				if (itr->actionInfo == ai)
				{
					// 2004, 6, 11 sobeit add start - ����������,������ ���, delay ���׶����� delay�� ����
					if(delayFrame)
					{
						itr->delay100msec = timeGetTime() / 100 + (delayFrame * 10) /16;
					}
					// 2004, 6, 11 sobeit add end
					return;
				}

				itr++;
			}

			//-----------------------------------------------------------------
			// ������ �߰��Ѵ�.
			//-----------------------------------------------------------------
			S_SLOT::UI_EFFECTSTATUS_STRUCT efs;
			efs.actionInfo = ai;
			efs.delay100msec = timeGetTime() / 100 + (delayFrame * 10) / 16;

			status.push_back( efs );

//			2007.08.22 chyaya - ���� ��/�� ��ȯ ����
// 			if(ai == SKILL_BLOOD_DRAIN)
// 			{
// 				g_char_slot_ingame.bl_drained = true;
// 				// 2004, 5, 7, sobeit add start - ���� ������ �� ������ �����ش�.
// 			//	ExecuteHelpEvent( HELP_EVENT_DRAIN_BLOOD ); 
// 				// 2004, 5, 6, sobeit add end
// 			}
#if __CONTENTS(__EXPERT_PET_CHANGER)
			if(ai == SKILL_CLIENT_EXPERT_PET_CHANGER)
			{
					g_char_slot_ingame.bExpertPetUse	= true;
			}
#endif // __EXPERT_PET_CHANGER
		}
	}
}

//-----------------------------------------------------------------------------
// Remove EffectStatus
//-----------------------------------------------------------------------------
void		
UI_RemoveEffectStatus(int es)
{
	if (es < g_pEffectStatusTable->GetSize())
	{
		int ai = (*g_pEffectStatusTable)[es].OriginalActionInfo;

		//---------------------------------------------------------------
		// EffectStatus�� ���õ� ActionInfo�� �ִ� ���
		//---------------------------------------------------------------
		if (ai < g_pActionInfoTable->GetMinResultActionInfo())
		{
			S_SLOT::UI_EFFECTSTATUS_TYPE& status = g_char_slot_ingame.STATUS;

			//-----------------------------------------------------------------
			// �̹� �ִ��� �˻�.. map���� �ٲٰ� ������.. ��.. - -;
			//-----------------------------------------------------------------
			S_SLOT::UI_EFFECTSTATUS_TYPE::iterator itr = status.begin();
			S_SLOT::UI_EFFECTSTATUS_TYPE::iterator endItr = status.end();

			while (itr != endItr)
			{
				//-----------------------------------------------------------------
				// �ִ� �Ÿ�..
				//-----------------------------------------------------------------
				if (itr->actionInfo == ai)
				{
					status.erase( itr );

//				2007.08.22 chyaya - ���� ��/�� ��ȯ ����
// 				if(ai == SKILL_BLOOD_DRAIN)
// 					g_char_slot_ingame.bl_drained = false;
 
 					// �� üũ�� �ʿ����.
#if __CONTENTS(__EXPERT_PET_CHANGER)
					if(ai == SKILL_CLIENT_EXPERT_PET_CHANGER)
						g_char_slot_ingame.bExpertPetUse = false;
 					return;
#endif //__EXPERT_PET_CHANGER

 				}
				itr++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Get EffectStatus sjheon
//-----------------------------------------------------------------------------
bool		
UI_GetEffectStatus(int es)
{
	//if (es < g_pEffectStatusTable->GetSize())
	//{
	TYPE_ACTIONINFO ai = (*g_pEffectStatusTable)[es].OriginalActionInfo;

	//---------------------------------------------------------------
	// EffectStatus�� ���õ� ActionInfo�� �ִ� ���
	//---------------------------------------------------------------
	if (ai < g_pActionInfoTable->GetMinResultActionInfo())
	{
		S_SLOT::UI_EFFECTSTATUS_TYPE& status = g_char_slot_ingame.STATUS;

		//-----------------------------------------------------------------
		// �̹� �ִ��� �˻�.. ��.. vector��.. - -;
		//-----------------------------------------------------------------
		S_SLOT::UI_EFFECTSTATUS_TYPE::iterator itr = status.begin(); 
		S_SLOT::UI_EFFECTSTATUS_TYPE::iterator endItr = status.end();

		while (itr != status.end())
		{
			//-----------------------------------------------------------------
			// �̹� ������ return�̴�..
			//-----------------------------------------------------------------
			if (itr->actionInfo == ai)
			{
				return true;
			}

			itr++;
		}
	}
	//}
	return false;
}


//-----------------------------------------------------------------------------
// Set WorldList
//-----------------------------------------------------------------------------
void
UI_SetWorldList()
{	
	// �ݸ�����
#ifdef __DESIGNED_JAPAN
	if(g_pUserInformation->IsNetmarble && !g_pUserInformation->IsNetmarbleJapan)
#else
	if(g_pUserInformation->IsNetmarble && g_pUserInformation->IsNetmarbleLogin)
#endif
	{
		gpC_base->SendMessage(UI_CONNECT_SERVER, true, g_pUserInformation->WorldID);
		return;
	}

	if (g_pServerInformation!=NULL)
	{
		// true�� group�� �����ϴ� �Ŵ�.
		gC_vs_ui.StartServerSelect( true );

//		gC_vs_ui.CharManagerEnable();

		const int numGroup = g_pServerInformation->size();

		std::string* groupName = new std::string[numGroup];
		int* groupID = new int [numGroup];
		int* groupStatus = new int [numGroup];

		CServerInformation::const_iterator iGroup = g_pServerInformation->begin();

		//-----------------------------------------------------
		// UI�� �Ѱ��� server���� ����
		//-----------------------------------------------------
		for (int i=0; i<numGroup; i++)
		{			
			const char* groupNameOrg = iGroup->second->GetGroupName();
			int status = iGroup->second->GetGroupStatus();
		
			groupID[i] = iGroup->first;
			groupName[i] = groupNameOrg;
			groupStatus[i] = status;

			iGroup ++;
		}			
		//-----------------------------------------------------
		// UI�� server list�� ������.
		//-----------------------------------------------------		
		int defaultID = g_pServerInformation->GetServerGroupID();
		gC_vs_ui.SetServerList(groupName, groupID, groupStatus, numGroup, defaultID);

		//-----------------------------------------------------
		// �޸𸮿��� ����
		//-----------------------------------------------------
		delete [] groupName;
		delete [] groupID;
		delete [] groupStatus;

	}
	
}

//-----------------------------------------------------------------------------
// Set ServerList
//-----------------------------------------------------------------------------
void
UI_SetServerList()
{
	// �ݸ�����
#ifdef __DESIGNED_JAPAN
	if(g_pUserInformation->IsNetmarble && !g_pUserInformation->IsNetmarbleJapan)
#else
	if(g_pUserInformation->IsNetmarble && g_pUserInformation->IsNetmarbleLogin)
#endif
	{
		gpC_base->SendMessage(UI_CONNECT_SERVER, false, g_pUserInformation->ServerID);
		return;
	}

	if (g_pServerInformation!=NULL)
	{
		// false�� server�� �����ϴ� �Ŵ�.
		gC_vs_ui.StartServerSelect( false );

//		gC_vs_ui.CharManagerEnable();
	
		int groupID = g_pServerInformation->GetServerGroupID();
		ServerGroup* pServerGroup = g_pServerInformation->GetData( groupID );

		const int numServer = pServerGroup->size();

		std::string* serverName = new std::string[numServer];
		int* serverID = new int [numServer];
		int* serverStatus = new int [numServer];
		bool* serverNonPK = new bool [numServer];

#if __CONTENTS(__SERVER_SELECT_REVEWAL)
		WORD* serverSlayerNum	= new WORD [numServer];
		WORD* serverVampireNum	= new WORD [numServer];
		WORD* serverOustersNum	= new WORD [numServer];
#endif

		ServerGroup::const_iterator iServer = pServerGroup->begin();

		//-----------------------------------------------------
		// UI�� �Ѱ��� server���� ����
		//-----------------------------------------------------		
		for (int i=0; i<numServer; i++)
		{			
			SERVER_INFO* pServerInfo = iServer->second;

			const char* serverNameOrg = pServerInfo->ServerName.GetString();
			int status = pServerInfo->ServerStatus;

			serverID[i] = iServer->first;
			serverName[i] = serverNameOrg;
			serverStatus[i] = status;
			serverNonPK[i]	= pServerInfo->ServerNonPK;
#if __CONTENTS(__SERVER_SELECT_REVEWAL)
			serverSlayerNum[i]	= pServerInfo->ServerSlayer;
			serverVampireNum[i]	= pServerInfo->ServerVampire;
			serverOustersNum[i]	= pServerInfo->ServerOusters;
#endif
			iServer++;
		}			

		//-----------------------------------------------------
		// UI�� server list�� ������.
		//-----------------------------------------------------		
		int defaultID = g_pServerInformation->GetServerID();
#if __CONTENTS(__SERVER_SELECT_REVEWAL)
		gC_vs_ui.SetServerList(serverName, serverID, serverStatus, numServer, defaultID, serverNonPK, serverSlayerNum, serverVampireNum, serverOustersNum);
#else
		gC_vs_ui.SetServerList(serverName, serverID, serverStatus, numServer, defaultID, serverNonPK);
#endif
		//-----------------------------------------------------
		// �޸𸮿��� ����
		//-----------------------------------------------------
		delete [] serverName;
		delete [] serverID;
		delete [] serverStatus;
		delete [] serverNonPK;

#if __CONTENTS(__SERVER_SELECT_REVEWAL)
		delete [] serverSlayerNum;
		delete [] serverVampireNum;
		delete [] serverOustersNum;
#endif //__SERVER_SELECT_REVEWAL
	}
	
}

//-----------------------------------------------------------------------------
// Set Current ServerName
//-----------------------------------------------------------------------------
void	
UI_SetCurrentServerName(const char *name, int status)
{
//	gC_vs_ui.SetCurrentServerName(name, (C_VS_UI_SERVER_SELECT::SERVER_STATUS)status);
}

//-----------------------------------------------------------------------------
// IsRunningSelectExplosive
//-----------------------------------------------------------------------------
BOOL		
UI_IsRunningSelectExplosive()
{
//	return gC_vs_ui.IsRunningSelectExplosive();
	return FALSE;
}

//-----------------------------------------------------------------------------
// UI_RunSelectExplosive
//-----------------------------------------------------------------------------
void		
UI_RunSelectExplosive()
{
//	gC_vs_ui.RunSelectExplosive();
}

//-----------------------------------------------------------------------------
// UI_RunSelectBomb
//-----------------------------------------------------------------------------
void		
UI_RunSelectBomb()
{
//	gC_vs_ui.RunSelectBomb();
}

//-----------------------------------------------------------------------------
// UI_RunSelectMine
//-----------------------------------------------------------------------------
void		
UI_RunSelectMine()
{
//	gC_vs_ui.RunSelectMine();
}

//-----------------------------------------------------------------------------
// UI_CloseSelectExplosive
//-----------------------------------------------------------------------------
void		
UI_CloseSelectExplosive()
{
//	gC_vs_ui.CloseSelectExplosive();
}

//-----------------------------------------------------------------------------
// RemoveDescriptor
//-----------------------------------------------------------------------------
void		
UI_RemoveDescriptor(void* pPtr)
{
	g_descriptor_manager.Unset( pPtr );
}

//-----------------------------------------------------------------------------
// Is Running Interface
//-----------------------------------------------------------------------------
BOOL
UI_IsRunningExchange()
{
	return gC_vs_ui.IsRunningExchange();
}

//-----------------------------------------------------------------------------
// Is Running Storage
//-----------------------------------------------------------------------------
BOOL
UI_IsRunningStorage()
{
	return gC_vs_ui.IsRunningStorage();
}

//-----------------------------------------------------------------------------
// Is Running Shop
//-----------------------------------------------------------------------------
BOOL
UI_IsRunningShop()
{
	return gC_vs_ui.IsRunningShop();
}

//-----------------------------------------------------------------------------
// Is Running Bookcase
//-----------------------------------------------------------------------------
BOOL
UI_IsRunningBookcase()
{
	return gC_vs_ui.IsRunningBookcase();
}

//-----------------------------------------------------------------------------
// Is Running Repair
//-----------------------------------------------------------------------------
BOOL 
UI_IsRunningRepair()
{
	return gC_vs_ui.IsRunningRepair();
}

//-----------------------------------------------------------------------------
// Is Running Silvering
//-----------------------------------------------------------------------------
BOOL 
UI_IsRunningSilvering()
{
	return gC_vs_ui.IsRunningSilvering();
}


//-----------------------------------------------------------------------------
// Is Running Sell
//-----------------------------------------------------------------------------
BOOL
UI_IsRunningSell()
{
	return gC_vs_ui.IsRunningSell();
}

//-----------------------------------------------------------------------------
// Is Running Tutorial Exit
//-----------------------------------------------------------------------------
BOOL		
UI_IsRunningTutorialExit()
{
	//return gC_vs_ui.IsRunningTutorialExit();
	return TRUE;
}

//-----------------------------------------------------------------------------
// Is Running Briefing
//-----------------------------------------------------------------------------
BOOL		
UI_IsRunningBriefing()
{
	//return gC_vs_ui.IsRunningBriefing();
	return TRUE;
}

//-----------------------------------------------------------------------------
// Is Running Computer
//-----------------------------------------------------------------------------
BOOL		
UI_IsRunningComputer()
{
	return gC_vs_ui.IsRunningComputer();
}

//-----------------------------------------------------------------------------
// Is Run Briefing
//-----------------------------------------------------------------------------
void		
UI_RunBriefing()
{
	if (g_pUIDialog)
	{
	//	g_pUIDialog->HidePCTalkDlg();
	}

	//gC_vs_ui.RunBriefing();
}

//-----------------------------------------------------------------------------
// Is Run Computer
//-----------------------------------------------------------------------------
void		
UI_RunComputer()
{
	DEBUG_ADD("UI_RunComputer()");

	if (g_pUIDialog)
	{
		DEBUG_ADD("HidePCTalkDlg");
		g_pUIDialog->HidePCTalkDlg();
	}

	DEBUG_ADD("gC_vs_ui.RunComputer()");
	gC_vs_ui.RunComputer();

	DEBUG_ADD("g_pUserInformation keep");

	// ���� ����
	g_pUserInformation->KeepConnection = TRUE;

	DEBUG_ADD("UI_RunComputer OK");
}

//-----------------------------------------------------------------------------
// Is Run Tutorial Exit
//-----------------------------------------------------------------------------
void		
UI_RunTutorialExit()
{
	//gC_vs_ui.RunTutorialExit();
}

//-----------------------------------------------------------------------------
// Is Close Briefing
//-----------------------------------------------------------------------------
void		
UI_CloseBriefing()
{
	if (g_pUIDialog)
	{
		//g_pUIDialog->ShowPCTalkDlg();
	}

	//gC_vs_ui.CloseBriefing();
}

//-----------------------------------------------------------------------------
// Is Close Computer
//-----------------------------------------------------------------------------
void		
UI_CloseComputer()
{
	gC_vs_ui.CloseComputer();

	if (g_pUIDialog)
	{
		g_pUIDialog->ShowPCTalkDlg();
	}

	// ���� ����
	g_pUserInformation->KeepConnection = FALSE;
}

//-----------------------------------------------------------------------------
// Is Close Tutorial Exit
//-----------------------------------------------------------------------------
void		
UI_CloseTutorialExit()
{
	//gC_vs_ui.CloseTutorialExit();
}

//--------------------------------------------------------------------------------
// UI_BackupID
//--------------------------------------------------------------------------------
void		
UI_RunExchangeAsk(TYPE_OBJECTID otherID)
{
	// by csm 12/4
	MCreature* pCreature = g_pZone->GetCreature( otherID );
	if(pCreature == NULL)
	{
		DEBUG_ADD_FORMAT( "[Error] UI_RunExchangeAsk - There is no such Creature. id=%d", otherID);
		return;
	}
	if(pCreature->CurPernalShop() != 1)
	{
		gC_vs_ui.RunExchangeAsk( pCreature->GetName() );
		
		// ��ȯ���̶�� �ǹ�..
		g_pPlayer->SetWaitVerify( MPlayer::WAIT_VERIFY_TRADE );
		
		g_pTempInformation->SetMode(TempInformation::MODE_TRADE_REQUEST);
		g_pTempInformation->Value1 = otherID;
		
		// [����] ��ȯ��û ���� ��
		//		__BEGIN_HELP_EVENT
		//			ExecuteHelpEvent( HE_TRADE_REQUESTED );	
		//		__END_HELP_EVENT
	}
	else
	{
		DEBUG_ADD_FORMAT( "[Error] UI_RunExchangeAsk - Failed , pCreature is using persnal shop.");
	}
}

//-----------------------------------------------------------------------------
// Run ExchangeCancel
//-----------------------------------------------------------------------------
void
UI_RunExchangeCancel(const char* pName)
{
	gC_vs_ui.RunExchangeCancel(pName);

	// [����] ��ȯ��û �ϰ� ����
//	__BEGIN_HELP_EVENT
//		ExecuteHelpEvent( HE_TRADE_REQUEST );	
//	__END_HELP_EVENT
}

//-----------------------------------------------------------------------------
// Run Exchange
//-----------------------------------------------------------------------------
void
UI_RunExchange(TYPE_OBJECTID otherID)
{	
	UI_CloseExchangeCancel();

	//-----------------------------------------------------------
	// ���� ����̸� �ȵȴ�..
	//-----------------------------------------------------------
	MCreature* pCreature = g_pZone->GetCreature( otherID );

	if (pCreature!=NULL)
	{
		if (g_pTradeManager!=NULL)
		{
			delete g_pTradeManager;			
		}
		
		g_pTradeManager = new MTradeManager;
		
		g_pTradeManager->Init();
		
		g_pTradeManager->SetOtherID( otherID );
		g_pTradeManager->SetOtherName( pCreature->GetName() );
		
		//-----------------------------------------------------------
		// mouse item�� �������� ���� ���·�..
		//-----------------------------------------------------------
		MItem* pMouseItem = UI_GetMouseItem();

		if (pMouseItem!=NULL)
		{
			pMouseItem->UnSetTrade();
		}

		//-----------------------------------------------------------
		// inventory�� ��� �������� �������� ���� ���·� �����.
		//-----------------------------------------------------------
		g_pInventory->SetBegin();

		while (g_pInventory->IsNotEnd())
		{
			MItem* pItem = g_pInventory->Get();

			pItem->UnSetTrade();
			
			g_pInventory->Next();
		}

		//-----------------------------------------------------------
		// gear�� ��� �������� �������� ���� ���·� �����.
		//-----------------------------------------------------------
		MPlayerGear* pGear = g_pPlayer->GetGear();

		pGear->SetBegin();

		while (pGear->IsNotEnd())
		{
			MItem* pItem = pGear->Get();

			pItem->UnSetTrade();
			
			pGear->Next();
		}

	
		//-----------------------------------------------------------
		// ��ȯâ ����
		//-----------------------------------------------------------
		gC_vs_ui.RunExchange();

		// ��ȯ���̶�� �ǹ̷� �����صд�...
		g_pPlayer->SetWaitVerify( MPlayer::WAIT_VERIFY_TRADE );
	}
	else
	{
		DEBUG_ADD_FORMAT( "[Error] There is no such Creature. id=%d", otherID);
	}
	
}

//-----------------------------------------------------------------------------
// Run Bookcase
//-----------------------------------------------------------------------------
void
UI_RunBookcase()
{
	gC_vs_ui.RunBookcase();
}

//-----------------------------------------------------------------------------
// Close Bookcase
//-----------------------------------------------------------------------------
void
UI_CloseBookcase()
{
	gC_vs_ui.CloseBookcase();
}

//-----------------------------------------------------------------------------
// Close Shop
//-----------------------------------------------------------------------------
void
UI_CloseShop()
{
	gC_vs_ui.CloseShop();
}

//-----------------------------------------------------------------------------
// Close Storage
//-----------------------------------------------------------------------------
void
UI_CloseStorage()
{
	gC_vs_ui.CloseStorage();

	// ������ ����
	if (g_pStorage!=NULL)
	{
		g_pStorage->UnSetActive();
	}

	// storage�� ���ش�.
	gC_vs_ui.SetStorage( NULL );

	// �����Կ��� skill icon�� ����� �������� �� �� �ִ�.
	// PacketFunction.cpp�� �ִ�. compile �ð� �����..
	if (g_pSkillAvailable!=NULL)
	{
		g_pSkillAvailable->SetAvailableSkills();
	}
}


//-----------------------------------------------------------------------------
// Close Exchange
//-----------------------------------------------------------------------------
void
UI_CloseExchange()
{
	if (g_pPlayer!=NULL
		&& g_pClientConfig!=NULL
		&& g_pTempInformation!=NULL
		&& g_pUserInformation!=NULL)
	{
		gC_vs_ui.CloseExchange();
		gC_vs_ui.CloseOtherInfo();

		// ��ȯ��
		if (g_pPlayer->GetWaitVerify()==MPlayer::WAIT_VERIFY_TRADE)
		{
			g_pPlayer->SetWaitVerifyNULL();
		}

		g_pTempInformation->SetMode(TempInformation::MODE_NULL);
		
		if (g_pTradeManager!=NULL)
		{
			delete g_pTradeManager;			
			g_pTradeManager = NULL;

			// trade �� item�������Ⱑ ���������� �ð�
			g_pUserInformation->ItemDropEnableTime = g_CurrentTime 
													+ g_pClientConfig->AFTER_TRADE_ITEM_DROP_DELAY;
		}
	}
}

//-----------------------------------------------------------------------------
// Close Exchange Ask
//-----------------------------------------------------------------------------
void
UI_CloseExchangeAsk()
{
	gC_vs_ui.CloseExchangeAsk();

	// ��ȯ��
	if (g_pPlayer->GetWaitVerify()==MPlayer::WAIT_VERIFY_TRADE)
	{
		g_pPlayer->SetWaitVerifyNULL();
	}

	if (g_pTempInformation->GetMode() == TempInformation::MODE_TRADE_REQUEST)
	{
		g_pTempInformation->SetMode(TempInformation::MODE_NULL);
	}

	if (g_pTradeManager!=NULL)
	{
		delete g_pTradeManager;			
		g_pTradeManager = NULL;
	}
}

//-----------------------------------------------------------------------------
// Close Exchange Cancel
//-----------------------------------------------------------------------------
void
UI_CloseExchangeCancel()
{	
	if (g_pTempInformation!=NULL)
	{
		if (g_pTempInformation->GetMode() == TempInformation::MODE_TRADE_REQUEST)
		{
			g_pTempInformation->SetMode(TempInformation::MODE_NULL);
		}

		gC_vs_ui.CloseExchangeCancel();
	}
}

//--------------------------------------------------------------------------------
// UI_BackupID
//--------------------------------------------------------------------------------
void		
UI_BackupLoginID(char *id)
{
	if( id != NULL && strlen( id ) < 20 )
		gC_vs_ui.BackupPrevId( id );
}

//--------------------------------------------------------------------------------
// Close All Dialog
//--------------------------------------------------------------------------------
void
UI_CloseAllDialog()
{
	if (g_pUIDialog!=NULL)
	{
		g_pUIDialog->ClosePCTalkDlg();
		g_pUIDialog->CloseMessageDlg();
//		g_pUIDialog->CloseHelpDlg();
	}

	gC_vs_ui.CloseAllDialog();

	// dialog�ݴ� �Լ��� �̿�ǰ� �ִ� ��..
	//gC_vs_ui.ServerDisconnectMessage();
}

//--------------------------------------------------------------------------------
// ���� ��
//--------------------------------------------------------------------------------
void
UI_SaveHotKeyToServer()
{
//	#ifdef	CONNECT_SERVER
//		if (g_pPlayer!=NULL)
//		{
//			//---------------------------------------------------------------
//			// Slayer�� ���
//			//---------------------------------------------------------------
//			if (g_pPlayer->IsSlayer())
//			{
//				CGSetSlayerHotKey _CGSetSlayerHotKey;
//					
//				for (int i=0; i < 4; i++)
//				{
//					int id = gC_vs_ui.GetHotkey((C_VS_UI_SKILL::HOTKEY)i);
//
//					//if (id != NOT_SELECTED)
//					//{
//						// save(i, id)
//						// ���� �ȵ� ���̶�.. �״�� ������״ٰ� �޾ƿ��� �ȴ�.
//						_CGSetSlayerHotKey.setHotKey( i, id );
//					//}
//				}
//
//				g_pSocket->sendPacket( &_CGSetSlayerHotKey );
//			}
//			//---------------------------------------------------------------
//			// Vampire�� ���
//			//---------------------------------------------------------------
//			else
//			{
//				CGSetVampireHotKey _CGSetVampireHotKey;
//					
//				for (int i=0; i < 8; i++)
//				{
//					int id = gC_vs_ui.GetHotkey((C_VS_UI_SKILL::HOTKEY)i);
//
//					//if (id != NOT_SELECTED)
//					//{
//						// save(i, id)
//						// ���� �ȵ� ���̶�.. �״�� ������״ٰ� �޾ƿ��� �ȴ�.
//						_CGSetVampireHotKey.setHotKey( i, id );
//					//}
//				}
//
//				g_pSocket->sendPacket( &_CGSetVampireHotKey );
//			}
//
//		}
//	#endif
}

//--------------------------------------------------------------------------------
// UI_SetHotkey - ��ų ����Ű �����Ҷ�,
//--------------------------------------------------------------------------------
void
UI_SetHotKey(int hotkey, int id)
{
//	if (id>0 && id < (*g_pActionInfoTable).GetMinResultActionInfo())
//	{
//		gC_vs_ui.SetHotkey((C_VS_UI_SKILL::HOTKEY)hotkey, (ACTIONINFO)id); 
//	}
}

//--------------------------------------------------------------------------------
// Get Hotkey
//--------------------------------------------------------------------------------
int		
UI_GetHotKey(int hotkey)
{
//	return gC_vs_ui.GetHotkey((C_VS_UI_SKILL::HOTKEY)hotkey);
	return -1;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoName
//--------------------------------------------------------------------------------
void
UI_LevelUp()
{
	if(g_pPlayer->IsDead() || g_pPlayer->HasEffectStatus(EFFECTSTATUS_COMA))
		return;

	gC_vs_ui.LevelUp();
}

//--------------------------------------------------------------------------------
// UI_FinishLevelUp
//--------------------------------------------------------------------------------
// The level-up button used to be removed by the old character window while it was
// open; it is removed as soon as no bonus point is left.
//--------------------------------------------------------------------------------
void
UI_FinishLevelUp()
{
	gC_vs_ui.FinishLevelUp();
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoName
//--------------------------------------------------------------------------------
void
UI_SetCharInfoName(char* pName)
{
	g_char_slot_ingame.sz_name = pName;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoCreatureType
//--------------------------------------------------------------------------------
void
UI_SetCharInfoCreatureType(Race eRace)
{
	g_char_slot_ingame.Race = eRace;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoSex
//--------------------------------------------------------------------------------
void
UI_SetCharInfoSex(bool female)
{
	g_char_slot_ingame.bl_female = female;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoLevel
//--------------------------------------------------------------------------------
void
UI_SetCharInfoLevel(int value)
{
	g_char_slot_ingame.level = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoSTR
//--------------------------------------------------------------------------------
void
UI_SetCharInfoSTR(int value)
{
	g_char_slot_ingame.STR_CUR = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoDEX
//--------------------------------------------------------------------------------
void
UI_SetCharInfoDEX(int value)
{
	g_char_slot_ingame.DEX_CUR = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoINT
//--------------------------------------------------------------------------------
void
UI_SetCharInfoINT(int value)
{
	g_char_slot_ingame.INT_CUR = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoSTR MAX
//--------------------------------------------------------------------------------
void
UI_SetCharInfoSTR_MAX(int value)
{
	g_char_slot_ingame.STR_MAX = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoDEX MAX
//--------------------------------------------------------------------------------
void
UI_SetCharInfoDEX_MAX(int value)
{
	g_char_slot_ingame.DEX_MAX = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoINT MAX
//--------------------------------------------------------------------------------
void
UI_SetCharInfoINT_MAX(int value)
{
	g_char_slot_ingame.INT_MAXX = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoSTR_PURE
//--------------------------------------------------------------------------------
void
UI_SetCharInfoSTR_PURE(int value)
{
	g_char_slot_ingame.STR_PURE = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoDEX_PURE
//--------------------------------------------------------------------------------
void
UI_SetCharInfoDEX_PURE(int value)
{
	g_char_slot_ingame.DEX_PURE = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoINT_PURE
//--------------------------------------------------------------------------------
void
UI_SetCharInfoINT_PURE(int value)
{
	g_char_slot_ingame.INT_PURE = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoDAM ( Max, Min )
//--------------------------------------------------------------------------------
void
UI_SetCharInfoDAM(int value, int value2)
{
	if (value2==-1)
	{
		value2 = value;
	}
	
	g_char_slot_ingame.DAM = value;
	g_char_slot_ingame.DAM2 = value2;	
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoDefense
//--------------------------------------------------------------------------------
void
UI_SetCharInfoDefense(int value)
{
	g_char_slot_ingame.DEFENSE = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoProtection
//--------------------------------------------------------------------------------
void
UI_SetCharInfoProtection(int value)
{
	g_char_slot_ingame.PROTECTION = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoTOHIT
//--------------------------------------------------------------------------------
void
UI_SetCharInfoTOHIT(int value)
{
	g_char_slot_ingame.TOHIT = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoSilverDamage
//--------------------------------------------------------------------------------
void
UI_SetCharInfoSilverDamage(int value)
{
	g_char_slot_ingame.SILVER_HP = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoSilverDamage
//--------------------------------------------------------------------------------
void
UI_SetCharInfoSilverDamage(int min_damage, int max_damage)
{
	g_char_slot_ingame.SILVER_DAM = max_damage;
	g_char_slot_ingame.SILVER_DAM2 = min_damage;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoHP
//--------------------------------------------------------------------------------
void
UI_SetCharInfoHP(int value)
{
	g_char_slot_ingame.HP = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoMP
//--------------------------------------------------------------------------------
void
UI_SetCharInfoMP(int value)
{
	g_char_slot_ingame.MP = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoHP_max
//--------------------------------------------------------------------------------
void
UI_SetCharInfoHP_max(int value)
{
	g_char_slot_ingame.HP_MAX = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoMP_max
//--------------------------------------------------------------------------------
void
UI_SetCharInfoMP_max(int value)
{
	g_char_slot_ingame.MP_MAX = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoCC
//--------------------------------------------------------------------------------
/*
void
UI_SetCharInfoCC(int value)
{
	g_char_slot_ingame.CC = value;
}
*/

//--------------------------------------------------------------------------------
// UI_SetCharInfoFAME
//--------------------------------------------------------------------------------
void
UI_SetCharInfoFAME(int value)
{
	g_char_slot_ingame.FAME = value;
}

//--------------------------------------------------------------------------------
// UI_SetCharInfoNOTERITY
//--------------------------------------------------------------------------------
/*
void
UI_SetCharInfoNOTERITY(int value)
{
	g_char_slot_ingame.NOTERITY = value;
}
*/


//-----------------------------------------------------------------------------
// UI_SetBonusPoint()
//-----------------------------------------------------------------------------
void
UI_SetBonusPoint(int bp)
{
	g_char_slot_ingame.bonus_point = bp;
}

//-----------------------------------------------------------------------------
// UI_SetSkillBonusPoint()
//-----------------------------------------------------------------------------
void
UI_SetSkillPoint(int sp)
{
	g_char_slot_ingame.skill_point = sp;
}


//-----------------------------------------------------------------------------
// Is Vampire Interface
//-----------------------------------------------------------------------------
Race
UI_GetInterfaceRace()
{
	switch(g_eRaceInterface)
	{
	case RACE_SLAYER:
		return RACE_SLAYER;
		break;

	case RACE_VAMPIRE:
		return RACE_VAMPIRE;
		break;

	case RACE_OUSTERS:
		return RACE_OUSTERS;
		break;
	}
	return RACE_SLAYER;
}
//int
//UI_IsVampireInterface()
//{
//	return gbl_vampire_interface;
//}

//-----------------------------------------------------------------------------
// Already Exist ID Message
//-----------------------------------------------------------------------------
void
UI_AlreadyExistIDMessage()
{
	gC_vs_ui.AleadyExistIdMessage(); // ���Ұ�		
}

//-----------------------------------------------------------------------------
// No Already Exist ID Message
//-----------------------------------------------------------------------------
void
UI_NoAlreadyExistIDMessage()
{
	gC_vs_ui.NoAleadyExistIdMessage(); // ��밡��
}

//-----------------------------------------------------------------------------
// Disconnect PCS
//-----------------------------------------------------------------------------
/*
void
UI_DisconnectPCS(int slot)
{
	gC_vs_ui.DisconnectPCS(slot);
}
*/

//-----------------------------------------------------------------------------
// PCS Connection Failed
//-----------------------------------------------------------------------------
/*
void
UI_PCSConnectionFailed()
{
	gC_vs_ui.ConnectionFailed();
}
*/

//-----------------------------------------------------------------------------
// CloseUserRegistrationWindow
//-----------------------------------------------------------------------------
void
UI_CloseUserRegistrationWindow()
{
//	gC_vs_ui.CloseUserRegistrationWindow();
}

//-----------------------------------------------------------------------------
// UI_AcquireQuitPCSOnlineModeMessage
//-----------------------------------------------------------------------------
/*
void	
UI_AcquireQuitPCSOnlineModeMessage()
{
	gC_vs_ui.AcquireQuitPCSOnlineModeMessage();
}
*/

//-----------------------------------------------------------------------------
// Please Connect PCS
//-----------------------------------------------------------------------------
/*
void	
UI_PleaseConnectPCS(char * sz_name, long pcs_number, int slot)
{
	gC_vs_ui.PleaseConnectPCS(sz_name, pcs_number, slot);
}
*/

//-----------------------------------------------------------------------------
// OnLine PCS
//-----------------------------------------------------------------------------
/*
void	
UI_OnLinePCS(char* pName, int pcsNumber)
{
	gC_vs_ui.OnLinePCS(pName, pcsNumber);
}
*/

//-----------------------------------------------------------------------------
// AcquirePDSClosedMessage
//-----------------------------------------------------------------------------
/*
void	
UI_AcquirePDSClosedMessage()
{
	gC_vs_ui.AcquirePDSClosedMessage();
}
*/

//-----------------------------------------------------------------------------
// Change To Vampire Interface
//-----------------------------------------------------------------------------
void		
UI_ChangeInterfaceRace(Race race)
{
	//UI_SaveUserOption();

	bool bLevelUp = gC_vs_ui.IsRunningLevelUp();

	switch(race)
	{
	case RACE_SLAYER:
		gC_vs_ui.ChangeToSlayerInterface();
		break;

	case RACE_VAMPIRE:
		gC_vs_ui.ChangeToVampireInterface();
		break;

	case RACE_OUSTERS:
		gC_vs_ui.ChangeToOustersInterface();
		break;
	}

	if(bLevelUp)
		gC_vs_ui.LevelUp();

	// UI�� ���� ����
	int zoneID = (g_bZonePlayerInLarge? g_nZoneLarge : g_nZoneSmall);

	if (g_pZone!=NULL)
	{
		SIZE zoneSize = { g_pZone->GetWidth(), g_pZone->GetHeight() };
		gC_vs_ui.SetZone( zoneID );
		gC_vs_ui.SetSize( zoneSize );

		// �������� ���� ��.. �ٽ� �������ش�.
		LoadZoneInfo(zoneID);
	}

	UI_AffectUserOption();
}

//-----------------------------------------------------------------------------
// Change To Slayer Interface
//-----------------------------------------------------------------------------
//void		
//UI_ChangeToSlayerInterface()
//{
//	//UI_SaveUserOption();
//
//	gC_vs_ui.ChangeToSlayerInterface();
//
//	// UI�� ���� ����
//	int zoneID = (g_bZonePlayerInLarge? g_nZoneLarge : g_nZoneSmall);
//
//	if (g_pZone!=NULL)
//	{
//		SIZE zoneSize = { g_pZone->GetWidth(), g_pZone->GetHeight() };
//		gC_vs_ui.SetZone( zoneID );
//		gC_vs_ui.SetSize( zoneSize );
//
//		// �������� ���� ��.. �ٽ� �������ش�.
//		LoadZoneInfo(zoneID);		
//	}
//	
//	UI_AffectUserOption();
//}

//-----------------------------------------------------------------------------
// UI_GetMouseItem
//-----------------------------------------------------------------------------
MItem*
UI_GetMouseItem()
{
	return gpC_mouse_pointer->GetPickUpItem();
}

//-----------------------------------------------------------------------------
// Get Inventory GridPosition
//-----------------------------------------------------------------------------
POINT	
UI_GetInventoryGridPosition(int x, int y)
{
	Point point = gC_vs_ui.GetInventoryGridPosition(x, y);
	
	POINT pt = { point.x, point.y };

	return pt;
}

//-----------------------------------------------------------------------------
// Get Inventory Position
//-----------------------------------------------------------------------------
POINT	
UI_GetInventoryPosition()
{
	Point point = gC_vs_ui.GetInventoryPosition();	

	POINT pt = { point.x, point.y };

	return pt;
}

//-----------------------------------------------------------------------------
// Lock Item Trade
//-----------------------------------------------------------------------------
void
UI_LockItemTrade()
{
	DEBUG_ADD("[UI] Lock ItemTrade");

	gC_vs_ui.LockItemTrade();
}

//-----------------------------------------------------------------------------
// UnLock Item Trade
//-----------------------------------------------------------------------------
void
UI_UnlockItemTrade()
{
	DEBUG_ADD("[UI] Unlock ItemTrade");
	
	gC_vs_ui.UnlockItemTrade();
}

//-----------------------------------------------------------------------------
//  Run Skill Tree ( domain )
//-----------------------------------------------------------------------------
void
UI_RunSkillTree(int domain, int maxLevel)
{
	/*
	BOOL bExistSkillTree = FALSE;
	// ������ SkillTree�� ����.
	SKILLDOMAIN sd = (enum SKILLDOMAIN)domain;

	switch (sd)
	{	
		//-----------------------------------------------------
		// ��
		//-----------------------------------------------------
		case SKILLDOMAIN_BLADE : gC_vs_ui.RunBladeSkillTree(); bExistSkillTree=TRUE; break;

		//-----------------------------------------------------
		// ��
		//-----------------------------------------------------
		case SKILLDOMAIN_SWORD : gC_vs_ui.RunSwordSkillTree(); bExistSkillTree=TRUE; break;

		//-----------------------------------------------------
		// ��
		//-----------------------------------------------------
		case SKILLDOMAIN_GUN : 
		//case SKILLDOMAIN_RIFLE : 
			gC_vs_ui.RunGunSkillTree(); bExistSkillTree=TRUE; break;
			
		//-----------------------------------------------------
		// ��æƮ
		//-----------------------------------------------------
		case SKILLDOMAIN_ENCHANT : gC_vs_ui.RunEnchantSkillTree(); bExistSkillTree=TRUE; break;

		//-----------------------------------------------------
		// ��
		//-----------------------------------------------------
		case SKILLDOMAIN_HEAL : gC_vs_ui.RunHealSkillTree(); bExistSkillTree=TRUE; break;

		//-----------------------------------------------------
		// �����̾�
		//-----------------------------------------------------
		case SKILLDOMAIN_VAMPIRE : gC_vs_ui.RunVampireSkillTree(); bExistSkillTree=TRUE; break;

		case SKILLDOMAIN_ETC : break;		
	}	
	*/

	//gC_vs_ui.RunSkillView();

	//----------------------------------------------------
	// NPC��ȭ�ϴ� dialog�� ����� 
	//----------------------------------------------------
	//if (bExistSkillTree)
	//{
	//	g_pUIDialog->HidePCTalkDlg();
	//}	

	//if (g_pPlayer->GetWaitVerify()==MPlayer::WAIT_VERIFY_NPC_ASK)
	{
		g_pPlayer->SetWaitVerifyNULL();

		g_pPCTalkBox->Release();		// 

		//---------------------------------------------------
		// normal
		//---------------------------------------------------
		g_pPCTalkBox->SetType( PCTalkBox::SKILL_LEARN );

		//---------------------------------------------------
		// PC Talk Box�� ���� ����
		//---------------------------------------------------
		char str[192];

		//g_pPCTalkBox->SetNPCID( pPacket->getObjectID() );
		//g_pPCTalkBox->SetCreatureType( pCreature->GetCreatureType() );	// �̹� �����Ǿ� �ִٰ� ����.
		//g_pPCTalkBox->SetScriptID( pPacket->getScriptID() );

		//---------------------------------------------------
		// Domain���� ��� �� �ִ� ����� �߰�
		//---------------------------------------------------
		MSkillDomain& domainInfo = (*g_pSkillManager)[domain];

		int domainLevel;

		if (g_pPlayer->IsSlayer())
		{
			domainLevel = domainInfo.GetDomainLevel();
		}
		else
		{
			domainLevel = g_pPlayer->GetLEVEL();
		}


		int availableSkills = 0;

		domainInfo.SetBegin();

		while (domainInfo.IsNotEnd())
		{
			int skillID = domainInfo.GetSkillID();

			MSkillDomain::SKILLSTATUS status = domainInfo.GetSkillStatus( (ACTIONINFO)skillID );

			const SKILLINFO_NODE& skillNode = (*g_pSkillInfoTable)[skillID];

			int		skillLevel = skillNode.GetLearnLevel();
			bool	hideInSkillTree = skillNode.IsHideInSkillTree();

			// ����� ��ų�� �ƴ� �͸�
			if(!hideInSkillTree)
			{
				//---------------------------------------------------
				// ���� �� ��� ����� �߿���..
				// ��� �� �ִ� ������ �� �����..
				//---------------------------------------------------
				if ((status==MSkillDomain::SKILLSTATUS_NEXT || status==MSkillDomain::SKILLSTATUS_OTHER)
					&& skillLevel>0 && skillLevel<150 && skillLevel <= domainLevel)
				{
					const char* pSkillName = (*g_pSkillInfoTable)[skillID].GetHName();

					sprintf(str, "%3d%s (%s)", skillID, pSkillName, (*g_pSkillInfoTable)[skillID].GetName() );
					
					if( g_pSystemAvailableManager->GetLimitLearnSkillLevel() >= skillLevel )
						g_pPCTalkBox->AddString( str );

					availableSkills++;
				}
			}
			
			domainInfo.Next();
		}

		// ������ �߰�
		std::string szMsg;
		szMsg += "999";
		szMsg += (*g_pGameStringTable)[UI_STRING_MESSAGE_CANCEL_LEARN_SKILL].GetString();
		g_pPCTalkBox->AddString( szMsg.c_str() );

		//---------------------------------------------------
		// ��ų �� ��﷡?
		//---------------------------------------------------
		if (availableSkills==0)
		{
			strcpy(str, (*g_pGameStringTable)[STRING_MESSAGE_NO_SKILL_TO_LEARN].GetString());
		}
		else
		{
			sprintf(str, (*g_pGameStringTable)[STRING_MESSAGE_WHAT_SKILL_TO_LEARN].GetString(), 
							(*g_pGameStringTable)[SKILLDOMAIN_NAME[domain]].GetString());			
		}
		
		g_pPCTalkBox->SetContent( str );

		g_pUIDialog->PopupPCTalkDlg();
	}

}

//-----------------------------------------------------------------------------
// Open Inventory To Sell
//-----------------------------------------------------------------------------
void
UI_OpenInventoryToSell()
{
	// NPC��ȭ�ϴ� dialog�� ����� 
	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.OpenInventoryToSell();
}

//-----------------------------------------------------------------------------
// Open Inventory To Repair
//-----------------------------------------------------------------------------
void
UI_OpenInventoryToRepair()
{
	// NPC��ȭ�ϴ� dialog�� ����� 
	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.OpenInventoryToRepair();
}

//-----------------------------------------------------------------------------
// Open Inventory To Silvering
//-----------------------------------------------------------------------------
void
UI_OpenInventoryToSilvering()
{
	// NPC��ȭ�ϴ� dialog�� ����� 
	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.OpenInventoryToSilvering();
}


//-----------------------------------------------------------------------------
// Close Repair
//-----------------------------------------------------------------------------
void
UI_CloseRepair()
{
	gC_vs_ui.FinishItemRepairing();
}

//-----------------------------------------------------------------------------
// Close Silvering
//-----------------------------------------------------------------------------
void
UI_CloseSilvering()
{
	gC_vs_ui.FinishItemSilvering();
}

//-----------------------------------------------------------------------------
// Close Sell
//-----------------------------------------------------------------------------
void
UI_CloseSell()
{
	gC_vs_ui.FinishItemSelling();
}

//-----------------------------------------------------------------------------
// Run Shop
//-----------------------------------------------------------------------------
void
UI_RunShop()
{
	// NPC��ȭ�ϴ� dialog�� �����
	g_pUIDialog->HidePCTalkDlg();
	// 2004, 8, 17, sobeit add start - ����â ���� ���ÿ� escŰ ������ ġ���ȭ�� �̿��ϴ� ���� ������ �߰�
	g_pUIDialog->SetLockInputPCTalk();
	// 2004, 8, 17, sobeit add end
	gC_vs_ui.RunShop();
}

//-----------------------------------------------------------------------------
// Set Shop
//-----------------------------------------------------------------------------
void
UI_SetShop(MShop* pShop)
{
	gC_vs_ui.SetShop( pShop );
}

//-----------------------------------------------------------------------------
// Run Storage Buy
//-----------------------------------------------------------------------------
// ������ �췡����? ������ price
//-----------------------------------------------------------------------------
void		
UI_RunStorageBuy(int price)
{
	// NPC��ȭ�ϴ� dialog�� �����
	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.RunStorageBuy( price );
}

//-----------------------------------------------------------------------------
// Run Storage
//-----------------------------------------------------------------------------
void
UI_RunStorage()
{
	// NPC��ȭ�ϴ� dialog�� �����
	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.RunStorage();

	// ������ �۵����̶�� �����Ѵ�.
	g_pStorage->SetActive();
}
//-----------------------------------------------------------------------------
// Run Pet Storage
//-----------------------------------------------------------------------------
void
UI_RunPetStorage()
{
//	// NPC��ȭ�ϴ� dialog�� �����
//	hide ��ų��� ���� ȭ�� ���콺 Ŭ���� ������ �ϳ� �������� npc�� ��ȭ���̴� ��� �������� �ʱ⿡ 
//	m_pPCTalkDlg�� �ʱ�ȭ �ʿ� ���� ���콺 ������ ������ �ʴ´�. ���� setlockinputpctalk�� �Ѵ�.
//	g_pUIDialog->HidePCTalkDlg();
	g_pUIDialog->SetLockInputPCTalk();
	gC_vs_ui.RunPetStorage();

	// ������ �۵����̶�� �����Ѵ�.
	g_pStorage->SetActive();
}
//-----------------------------------------------------------------------------
// Set Storage
//-----------------------------------------------------------------------------
void
UI_SetStorage(MStorage* pStorage)
{
	gC_vs_ui.SetStorage( pStorage );
}
//-----------------------------------------------------------------------------
// Set Storage
//-----------------------------------------------------------------------------
void
UI_SetPetStorage(MStorage* pStorage)
{
	gC_vs_ui.SetPetStorage( pStorage );
}

//-----------------------------------------------------------------------------
// StartCharacterManager
//-----------------------------------------------------------------------------
void
UI_StartCharacterManager( bool bAgree )	// �ݸ����� ��� ��� ���� �޽���
{
	gC_vs_ui.StartCharacterManager();
#if __CONTENTS(__JAPAN_UI)
	if( bAgree && !g_pUserInformation->IsNetmarbleJapan)
#else
	if( bAgree )
#endif
		gC_vs_ui.PopupNetmarbleAgreementMessage();
}

//-----------------------------------------------------------------------------
// New Character Create OK
//-----------------------------------------------------------------------------
// �� ĳ���� ���� ����
//-----------------------------------------------------------------------------
void	
UI_NewCharacterCreateOk()
{
	gC_vs_ui.NewCharacterCreateOk();
}

//-----------------------------------------------------------------------------
// New Character Create Failed
//-----------------------------------------------------------------------------
// �� ĳ���� ������ ������ ���
//-----------------------------------------------------------------------------
void	
UI_NewCharacterCreateFailed(int error)
{
	gC_vs_ui.NewCharacterCreateFailed( error );
}

//-----------------------------------------------------------------------------
// Delete Character OK
//-----------------------------------------------------------------------------
// ĳ���� ���� ����
//-----------------------------------------------------------------------------
void	
UI_DeleteCharacterOK()
{	
	// ĳ���� ����
	gC_vs_ui.DeleteCharacter( g_pUserInformation->Slot );	
}

//-----------------------------------------------------------------------------
// Delete Character Failed
//-----------------------------------------------------------------------------
// ĳ���� ���� ���� - �ֹε�Ϲ�ȣ.. ����. - -;
//-----------------------------------------------------------------------------
void	
UI_DeleteCharacterFailed()
{
	// �ֹε�Ϲ�ȣ�� Ʋ���� ���� message
	gC_vs_ui.Invalid_SSN_Message();	
}

//-----------------------------------------------------------------------------
// StartGame
//-----------------------------------------------------------------------------
void
UI_StartGame()
{
#if __CONTENTS(__EXPERT_PET_CHANGER)
	g_char_slot_ingame.bExpertPetUse		= false ;
#endif //__EXPERT_PET_CHANGER
	DEBUG_ADD("STARTGAME2");
	gC_vs_ui.StartGame();
	DEBUG_ADD("STARTGAME");

	g_pQuickSlot = NULL;
	g_pArmsBand1 = NULL;
	g_pArmsBand2 = NULL;
	UI_AffectUserOption();
	DEBUG_ADD("AFFECT");

	// minimap�� ������ �ѵд�.
	gC_vs_ui.RunMinimap();	
	DEBUG_ADD("MINIMAP");
}


//-----------------------------------------------------------------------------
// Set NPC Info
//-----------------------------------------------------------------------------
// Zone�� �����ϴ� NPC�� ���� ������ �����Ѵ�.
//-----------------------------------------------------------------------------
void
UI_SetNPCInfo( const char* pName, int npcID, int x, int y )
{
	DEBUG_ADD_FORMAT("[UI_SetNPCInfo] %s %d %d %d", pName, npcID, x, y);
	gC_vs_ui.SetNPC( x, y, npcID, pName );
}

//-----------------------------------------------------------------------------
// Set Character
//-----------------------------------------------------------------------------
void
UI_SetCharacter(int slotID, PCSlayerInfo * pInfo)
{
	// set character
	S_SLOT slot;
	slot.Init();

	slot.sz_name = g_pUserInformation->Character[slotID].GetString();
	slot.sz_guild_name = "";
	slot.Race = RACE_SLAYER;

	short alignment = pInfo->getAlignment();
	if (alignment > 10000)
	{	
		alignment = - (short)(~alignment + 1);
	}

	slot.alignment_num = alignment;
	slot.alignment = (ALIGNMENT)ConvertAlignment( pInfo->getAlignment() );

	slot.bl_female = pInfo->getSex()==FEMALE;	

	slot.HP_MAX = pInfo->getHP(ATTR_MAX);
	slot.HP		= pInfo->getHP(ATTR_MAX);
	slot.MP_MAX = pInfo->getMP(ATTR_MAX);	
	slot.MP		= pInfo->getMP(ATTR_MAX);

	slot.STR_PURE	= pInfo->getSTR();
	slot.DEX_PURE	= pInfo->getDEX();
	slot.INT_PURE	= pInfo->getINT();
	slot.STR_EXP_REMAIN = pInfo->getSTRExp();
	slot.DEX_EXP_REMAIN = pInfo->getDEXExp();
	slot.INT_EXP_REMAIN = pInfo->getINTExp();
	slot.STATUS.clear();

	SKILLDOMAIN weaponDomain[] =
	{
		MAX_SKILLDOMAIN,
		SKILLDOMAIN_SWORD,
		SKILLDOMAIN_BLADE,
		SKILLDOMAIN_GUN,
		SKILLDOMAIN_HEAL,
		SKILLDOMAIN_ENCHANT,
	};

	// �ӽ�!!
	g_StatusManager.SetCurrentWeaponDomain( weaponDomain[pInfo->getWeaponType()], 1 );

	// status ���� ����.
	g_StatusManager.Set(pInfo->getSTR(), pInfo->getDEX(), pInfo->getINT());

	//slot.CC		= g_StatusManager.GetCC();
	slot.DAM2	= g_StatusManager.GetMinDAM();
	slot.DAM	= g_StatusManager.GetMaxDAM();
	slot.DEFENSE	= g_StatusManager.GetDefense();
	slot.PROTECTION	= g_StatusManager.GetProtection();
	slot.TOHIT	= g_StatusManager.GetTOHIT();
	slot.m_AdvancementLevel = pInfo->getAdvancementLevel();

	slot.FAME	= pInfo->getFame();
	slot.GRADE	= pInfo->getRank();
	//slot.NOTERITY = pInfo->getNotoriety();

	// ���� ���� domain level�� ã�´�.
	int maxDomainLevel = 1;

	/*
	for (int sd=0; sd<SKILL_DOMAIN_MAX; sd++)
	{
		maxDomainLevel = max( maxDomainLevel, pInfo->getSkillDomainLevel( (SkillDomain)sd ) );
	}
	*/

	slot.level = maxDomainLevel;

	//------------------------------------------------------------
	// domain level ����
	//------------------------------------------------------------
	slot.DOMAIN_SWORD	= pInfo->getSkillDomainLevel( SKILL_DOMAIN_SWORD );
	slot.DOMAIN_BLADE	= pInfo->getSkillDomainLevel( SKILL_DOMAIN_BLADE );
	slot.DOMAIN_GUN		= pInfo->getSkillDomainLevel( SKILL_DOMAIN_GUN );
	slot.DOMAIN_HEAL	= pInfo->getSkillDomainLevel( SKILL_DOMAIN_HEAL );
	slot.DOMAIN_ENCHANT = pInfo->getSkillDomainLevel( SKILL_DOMAIN_ENCHANT );
	

	//------------------------------------------------------------
	// ���� ���� ����
	//------------------------------------------------------------
	const CHAR_MAN uiFaceMale[] =
	{			
		M_FACE1,
		M_FACE2,
		M_FACE3
	};
	
	const CHAR_MAN uiHairMale[] =
	{			
		M_HAIR1,
		M_HAIR2,
		M_HAIR3
	};
	
	const CHAR_MAN uiHelmMale[] = 
	{
		M_NO_WEAR,
		M_HELMET1,
		M_HELMET2,
		M_HELMET3,		// 3�� ��� by chyaya
		M_HELMET3,		// HELMET_OSIRIS
	};
	
	const CHAR_MAN uiCoatMale[] =
	{
		M_COAT1,
		M_COAT1,
		M_COAT2,
		M_COAT3,
		M_COAT4,		// 4�� ���� by chyaya
		M_COAT4,		// JACKET_OSIRIS
	};
	
	const CHAR_MAN uiTrouserMale[] =
	{
		M_TROUSER1,
		M_TROUSER1,
		M_TROUSER2,
		M_TROUSER3,
		M_TROUSER4,	// 4�� ���� by chyaya
		M_TROUSER4,		// PANTS_OSIRIS
	};
	
	const CHAR_MAN uiWeaponMale[] =
	{
		M_NO_WEAR,
		M_SWORD,
		M_BLADE,
		M_SR,
		M_AR,
		M_SG,
		M_SMG,
		M_CROSS,
		M_MACE,
	};
	
	const CHAR_MAN uiShieldMale[] =
	{
		M_NO_WEAR,
		M_SHIELD1,
		M_SHIELD2,
		M_SHIELD2,		// SHIELD_OSIRIS
	};
	
	slot.man_info.helmet	= uiHelmMale[pInfo->getHelmetType()];
	slot.man_info.coat		= uiCoatMale[pInfo->getJacketType()];
	slot.man_info.trouser	= uiTrouserMale[pInfo->getPantsType()];
	slot.man_info.face		= uiFaceMale[pInfo->getHairStyle()];
	slot.man_info.hair		= uiHairMale[pInfo->getHairStyle()];
	slot.man_info.right		= uiWeaponMale[pInfo->getWeaponType()];
	slot.man_info.left		= uiShieldMale[pInfo->getShieldType()];

	if (pInfo->getHelmetType() == HELMET_OSIRIS)
		slot.OsirisLook |= OSIRIS_LOOK_HELMET;
	if (pInfo->getJacketType() == JACKET_OSIRIS)
		slot.OsirisLook |= OSIRIS_LOOK_COAT;
	if (pInfo->getPantsType() == PANTS_OSIRIS)
		slot.OsirisLook |= OSIRIS_LOOK_TROUSER;
	if (pInfo->getShieldType() == SHIELD_OSIRIS)
		slot.OsirisLook |= OSIRIS_LOOK_SHIELD;

	static const BYTE weaponTierLook[] = { 0, OSIRIS_LOOK_WEAPON_TIER1, OSIRIS_LOOK_WEAPON_TIER2, OSIRIS_LOOK_WEAPON };
	slot.OsirisLook |= weaponTierLook[ pInfo->getWeaponTier() ];
	

	// ���� ����
	/*
	pInfo->getHairStyle();	//	HAIR_STYLE1 , 	HAIR_STYLE2 , 	HAIR_STYLE3 
	pInfo->getHelmetType();	// HELMET_NONE , HELMET1 , HELMET2 ,
	pInfo->getJacketType();	// 	JACKET_BASIC , 	JACKET1 , 	JACKET2 ,
	pInfo->getPantsType(); 	// PANTS_BASIC , 	PANTS1 , 	PANTS2 ,
	*/

	//------------------------------------------------------------
	// ���� ����
	//------------------------------------------------------------
	slot.skin_color	= pInfo->getSkinColor();
	slot.hair_color	= pInfo->getHairColor();

	slot.helmet_color = 0;
	slot.left_color = 0;
	slot.right_color = 0;

	if (pInfo->getHelmetType()!=HELMET_NONE)
	{
		if(pInfo->getHelmetColor() == UNIQUE_ITEM_COLOR)
			slot.helmet_color = MItem::GetUniqueItemColorset();
		else
		if(pInfo->getHelmetColor() == QUEST_ITEM_COLOR)
			slot.helmet_color = QUEST_ITEM_COLOR;
		else
			slot.helmet_color = (*g_pItemOptionTable)[pInfo->getHelmetColor()].ColorSet;			
	}
	
	if(pInfo->getPantsColor() == UNIQUE_ITEM_COLOR )
		slot.trouser_color = MItem::GetUniqueItemColorset();
	else
	if(pInfo->getPantsColor() == QUEST_ITEM_COLOR)
		slot.trouser_color = QUEST_ITEM_COLOR;
	else
		slot.trouser_color = (*g_pItemOptionTable)[pInfo->getPantsColor()].ColorSet;	

	if(pInfo->getJacketColor() == UNIQUE_ITEM_COLOR)
		slot.coat_color = MItem::GetUniqueItemColorset( );
	else
	if(pInfo->getJacketColor() == QUEST_ITEM_COLOR)
		slot.coat_color = QUEST_ITEM_COLOR;
	else
		slot.coat_color = (*g_pItemOptionTable)[pInfo->getJacketColor()].ColorSet;
	
	if (pInfo->getShieldType()!=SHIELD_NONE)
	{		
		if(pInfo->getShieldColor() == UNIQUE_ITEM_COLOR)			
			slot.left_color = MItem::GetUniqueItemColorset( );
		else
		if(pInfo->getShieldColor() == QUEST_ITEM_COLOR)
			slot.left_color = QUEST_ITEM_COLOR;
		else
			slot.left_color = (*g_pItemOptionTable)[pInfo->getShieldColor()].ColorSet;
	}
	

	if (pInfo->getWeaponType()!=WEAPON_NONE)
	{
		if( pInfo->getWeaponColor() == UNIQUE_ITEM_COLOR)
			slot.right_color = MItem::GetUniqueItemColorset();
		else		
		if( pInfo->getWeaponColor() == QUEST_ITEM_COLOR)
			slot.right_color = QUEST_ITEM_COLOR;
		else
			slot.right_color = (*g_pItemOptionTable)[pInfo->getWeaponColor()].ColorSet;
	}

	// ���� Ȯ��
	if(slot.skin_color != UNIQUE_ITEM_COLOR && slot.skin_color != QUEST_ITEM_COLOR)
		slot.skin_color = max(0, min(slot.skin_color, MAX_COLORSET-1));
	if(slot.helmet_color != UNIQUE_ITEM_COLOR && slot.helmet_color != QUEST_ITEM_COLOR)
		slot.helmet_color = max(0, min(slot.helmet_color, MAX_COLORSET-1));
	if(slot.coat_color != UNIQUE_ITEM_COLOR && slot.coat_color != QUEST_ITEM_COLOR)
		slot.coat_color = max(0, min(slot.coat_color, MAX_COLORSET-1));
	if(slot.hair_color != UNIQUE_ITEM_COLOR && slot.hair_color != QUEST_ITEM_COLOR)
		slot.hair_color = max(0, min(slot.hair_color, MAX_COLORSET-1));
	if(slot.left_color != UNIQUE_ITEM_COLOR && slot.left_color != QUEST_ITEM_COLOR)
		slot.left_color = max(0, min(slot.left_color, MAX_COLORSET-1));
	if(slot.right_color != UNIQUE_ITEM_COLOR && slot.right_color != QUEST_ITEM_COLOR)
		slot.right_color = max(0, min(slot.right_color, MAX_COLORSET-1));	
	
	gC_vs_ui.SetCharacter(slotID, slot);
}

//-----------------------------------------------------------------------------
// Set Character
//-----------------------------------------------------------------------------
void
UI_SetCharacter(int slotID, PCVampireInfo * pInfo)
{
	S_SLOT slot;
	slot.Init();

	slot.sz_name = g_pUserInformation->Character[slotID];
	slot.sz_guild_name = "";
//	slot.bl_vampire = true;
	slot.Race = RACE_VAMPIRE;

	slot.bl_female = pInfo->getSex()==FEMALE;	
	
	short alignment = pInfo->getAlignment();
	if (alignment > 10000)
	{	
		alignment = - (short)(~alignment + 1);
	}

	slot.alignment_num = alignment;
	slot.alignment = (ALIGNMENT)ConvertAlignment( pInfo->getAlignment() );

	slot.HP_MAX = pInfo->getHP(ATTR_MAX);		
	slot.HP		= pInfo->getHP(ATTR_MAX);
	slot.MP_MAX = 1;
	slot.MP		= 1;
	
	slot.STR_PURE	= pInfo->getSTR();
	slot.DEX_PURE	= pInfo->getDEX();
	slot.INT_PURE	= pInfo->getINT();
	
	slot.STATUS.clear();

	// status ���� ����.
	g_StatusManager.SetCurrentWeaponDomain( SKILLDOMAIN_VAMPIRE, pInfo->getExp() );

	g_StatusManager.Set(pInfo->getSTR(), pInfo->getDEX(), pInfo->getINT());

	//slot.CC		= g_StatusManager.GetCC();
	slot.DAM2	= g_StatusManager.GetMinDAM();
	slot.DAM	= g_StatusManager.GetMaxDAM();
	slot.DEFENSE		= g_StatusManager.GetDefense();
	slot.PROTECTION		= g_StatusManager.GetProtection();
	slot.TOHIT	= g_StatusManager.GetTOHIT();

	slot.level			= pInfo->getLevel();
	slot.bonus_point	= pInfo->getBonus();

	slot.EXP_REMAIN		= pInfo->getExp();
	
	slot.FAME	= pInfo->getFame();
	slot.GRADE	= pInfo->getRank();
//	slot.NOTERITY = pInfo->getNotoriety();

	// coat
	int coatType = pInfo->getCoatType();
	int coatColor = pInfo->getCoatColor();
	

	// arm
	int armType = M_NO_WEAR;

	if(pInfo->getArmType() == VAMPIRE_ARM_WEAPON)
	{
		armType = slot.bl_female;
	}

	// color
	slot.skin_color	= pInfo->getSkinColor();
	slot.hair_color = pInfo->getBatColor();
	slot.right_color = pInfo->getArmColor();

	slot.m_AdvancementLevel = pInfo->getAdvancementLevel();

	if (coatType == VAMPIRE_OSIRIS_COAT)
		slot.OsirisLook |= OSIRIS_LOOK_COAT;
	if (pInfo->getArmType() == VAMPIRE_ARM_OSIRIS_WEAPON)
		slot.OsirisLook |= OSIRIS_LOOK_WEAPON;
	if (pInfo->getArmType() == VAMPIRE_ARM_TIER1_WEAPON)
		slot.OsirisLook |= OSIRIS_LOOK_WEAPON_TIER1;
	if (pInfo->getArmType() == VAMPIRE_ARM_TIER2_WEAPON)
		slot.OsirisLook |= OSIRIS_LOOK_WEAPON_TIER2;

	slot.left_color = 0;
	slot.helmet_color = 0;
	slot.trouser_color = 0;

	/*
	// ���� ���߰�
	int creatureType = 0;
	if(slot.bl_female)
	{
		creatureType = (*g_pItemTable)[ITEM_CLASS_VAMPIRE_COAT][coatType].AddonFemaleFrameID;
	}
	else
	{
		creatureType = (*g_pItemTable)[ITEM_CLASS_VAMPIRE_COAT][coatType].AddonMaleFrameID;
	}
	*/

//	int spriteType = (*g_pCreatureTable)[creatureType].SpriteTypes[0];
//	int coat = (*g_pCreatureSpriteTable)[spriteType].FrameID;
	
	slot.man_info.helmet = M_NO_WEAR;
	slot.man_info.coat = (CHAR_MAN)coatType;			// coatType�� ���� �ٲ�� �Ѵ�.. ���߿�~
	slot.man_info.trouser = M_NO_WEAR;
	slot.man_info.face = M_FACE1;	// default
	slot.man_info.hair = M_NO_WEAR;
	slot.man_info.right = (CHAR_MAN)armType;
	slot.man_info.left = M_NO_WEAR;

	//------------------------------------------------------------
	// ����
	//------------------------------------------------------------
	slot.coat_color = coatColor;//(*g_pItemOptionTable)[coatOption].ColorSet;
	/*
	if (slot.bl_female)
	{
		// default .. ���߿� �ٲ�� �Ѵ�.
		slot.coat_color = 38;
	}
	//------------------------------------------------------------
	// ����
	//------------------------------------------------------------
	else
	{
		// default 
		slot.coat_color = 91;
	}
	*/

	// ���� Ȯ��
	if(slot.skin_color != UNIQUE_ITEM_COLOR && slot.skin_color != QUEST_ITEM_COLOR)
		slot.skin_color = max(0, min(slot.skin_color, MAX_COLORSET-1));
	if(slot.helmet_color != UNIQUE_ITEM_COLOR && slot.helmet_color != QUEST_ITEM_COLOR)
		slot.helmet_color = max(0, min(slot.helmet_color, MAX_COLORSET-1));
	if(slot.coat_color != UNIQUE_ITEM_COLOR && slot.coat_color != QUEST_ITEM_COLOR)
		slot.coat_color = max(0, min(slot.coat_color, MAX_COLORSET-1));
	if(slot.hair_color != UNIQUE_ITEM_COLOR && slot.hair_color != QUEST_ITEM_COLOR)
		slot.hair_color = max(0, min(slot.hair_color, MAX_COLORSET-1));
	if(slot.left_color != UNIQUE_ITEM_COLOR && slot.left_color != QUEST_ITEM_COLOR)
		slot.left_color = max(0, min(slot.left_color, MAX_COLORSET-1));
	if(slot.right_color != UNIQUE_ITEM_COLOR && slot.right_color != QUEST_ITEM_COLOR)
		slot.right_color = max(0, min(slot.right_color, MAX_COLORSET-1));

	DEBUG_ADD_FORMAT("[UI_SETCHARACTER] %d Before", slotID);
	gC_vs_ui.SetCharacter(slotID, slot);
	DEBUG_ADD_FORMAT("[UI_SETCHARACTER] %d After", slotID);
}

//-----------------------------------------------------------------------------
// Set Character
//-----------------------------------------------------------------------------
void
UI_SetCharacter(int slotID, PCOustersInfo * pInfo)
{
	S_SLOT slot;
	slot.Init();

	slot.sz_name = g_pUserInformation->Character[slotID];
	slot.sz_guild_name = "";
	slot.Race = RACE_OUSTERS;

	short alignment = pInfo->getAlignment();
	if (alignment > 10000)
	{	
		alignment = - (short)(~alignment + 1);
	}

	slot.alignment_num = alignment;
	slot.alignment = (ALIGNMENT)ConvertAlignment( pInfo->getAlignment() );

	slot.HP_MAX = pInfo->getHP(ATTR_MAX);		
	slot.HP		= pInfo->getHP(ATTR_MAX);
	slot.MP_MAX = pInfo->getMP(ATTR_MAX);
	slot.MP		= pInfo->getMP(ATTR_MAX);
	
	slot.STR_PURE	= pInfo->getSTR();
	slot.DEX_PURE	= pInfo->getDEX();
	slot.INT_PURE	= pInfo->getINT();
	
	slot.STATUS.clear();

	// status ���� ����.
	g_StatusManager.SetCurrentWeaponDomain( SKILLDOMAIN_OUSTERS, pInfo->getLevel() );

	g_StatusManager.Set(pInfo->getSTR(), pInfo->getDEX(), pInfo->getINT());

	//slot.CC		= g_StatusManager.GetCC();
	slot.DAM2	= g_StatusManager.GetMinDAM();
	slot.DAM	= g_StatusManager.GetMaxDAM();
	slot.DEFENSE		= g_StatusManager.GetDefense();
	slot.PROTECTION		= g_StatusManager.GetProtection();
	slot.TOHIT	= g_StatusManager.GetTOHIT();

	slot.level			= pInfo->getLevel();
	slot.bonus_point	= pInfo->getBonus();
	slot.skill_point	= pInfo->getSkillBonus();

	slot.EXP_REMAIN		= pInfo->getExp();
	
	slot.FAME	= pInfo->getFame();
	slot.GRADE	= pInfo->getRank();
	slot.m_AdvancementLevel = pInfo->getAdvancementLevel();
//	slot.NOTERITY = pInfo->getNotoriety();

	// coat
	int coatType = pInfo->getCoatType();
	int coatColor = pInfo->getCoatColor();
	int weaponType = pInfo->getArmType();
	int weaponColor = pInfo->getArmColor();
	int bootsColor = pInfo->getBootsColor();
	int armColor = pInfo->getArmColor();

	if (coatType == OUSTERS_OSIRIS_COAT)
		slot.OsirisLook |= OSIRIS_LOOK_COAT;
	if (weaponType == OUSTERS_ARM_OSIRIS_CHAKRAM)
		slot.OsirisLook |= OSIRIS_LOOK_WEAPON;
	if (weaponType == OUSTERS_ARM_TIER1_CHAKRAM)
		slot.OsirisLook |= OSIRIS_LOOK_WEAPON_TIER1;
	if (weaponType == OUSTERS_ARM_TIER2_CHAKRAM)
		slot.OsirisLook |= OSIRIS_LOOK_WEAPON_TIER2;

	// color
//	slot.skin_color	= pInfo->getSkinColor();
	slot.hair_color = pInfo->getHairColor();

	slot.left_color = 0;
	slot.right_color = 0;
	slot.helmet_color = 0;
	slot.trouser_color = 0;

	// ���� ���߰�
//	int creatureType = 0;
//	if(slot.bl_female)
//		creatureType = (*g_pItemTable)[ITEM_CLASS_VAMPIRE_COAT][coatType].AddonFemaleFrameID;
//	else
//		creatureType = (*g_pItemTable)[ITEM_CLASS_VAMPIRE_COAT][coatType].AddonMaleFrameID;
//
//	int spriteType = (*g_pCreatureTable)[creatureType].SpriteType;
	// coatType comes off the wire; an unknown look falls back to the basic coat.
	MItem* pCoatItem = (coatType >= 0 && coatType < OUSTERS_COAT_MAX)? g_pPacketItemOustersCoat[coatType] : NULL;
	if (pCoatItem == NULL)
		pCoatItem = g_pPacketItemOustersCoat[OUSTERS_COAT_BASIC];
	int spriteType = (*g_pItemTable)[ITEM_CLASS_OUSTERS_COAT][pCoatItem->GetItemType()].AddonMaleFrameID;
	
	slot.man_info.helmet = M_NO_WEAR;
	slot.man_info.coat = (CHAR_MAN)(spriteType);		// coatType�� ���� �ٲ�� �Ѵ�.. ���߿�~
	slot.man_info.trouser = M_NO_WEAR;
	slot.man_info.face = M_FACE1;	// default
	slot.man_info.hair = M_NO_WEAR;
	slot.man_info.right = (CHAR_MAN)weaponType;
	slot.man_info.left = (CHAR_MAN)weaponType;

	//------------------------------------------------------------
	// ����
	//------------------------------------------------------------
	slot.coat_color = coatColor;//(*g_pItemOptionTable)[coatOption].ColorSet;
	slot.left_color = weaponColor;
	slot.right_color = weaponColor;
	slot.trouser_color = bootsColor;
	slot.skin_color = armColor;
	/*
	if (slot.bl_female)
	{
		// default .. ���߿� �ٲ�� �Ѵ�.
		slot.coat_color = 38;
	}
	//------------------------------------------------------------
	// ����
	//------------------------------------------------------------
	else
	{
		// default 
		slot.coat_color = 91;
	}
	*/

	// ���� Ȯ��
	if(slot.skin_color != UNIQUE_ITEM_COLOR && slot.skin_color != QUEST_ITEM_COLOR)
		slot.skin_color = max(0, min(slot.skin_color, MAX_COLORSET-1));
	if(slot.helmet_color != UNIQUE_ITEM_COLOR && slot.helmet_color != QUEST_ITEM_COLOR)
		slot.helmet_color = max(0, min(slot.helmet_color, MAX_COLORSET-1));
	if(slot.coat_color != UNIQUE_ITEM_COLOR && slot.coat_color != QUEST_ITEM_COLOR)
		slot.coat_color = max(0, min(slot.coat_color, MAX_COLORSET-1));
	if(slot.trouser_color != UNIQUE_ITEM_COLOR && slot.trouser_color != QUEST_ITEM_COLOR)
		slot.trouser_color = max(0, min(slot.trouser_color, MAX_COLORSET-1));
	if(slot.hair_color != UNIQUE_ITEM_COLOR && slot.hair_color != QUEST_ITEM_COLOR)
		slot.hair_color = max(0, min(slot.hair_color, MAX_COLORSET-1));
	if(slot.left_color != UNIQUE_ITEM_COLOR && slot.left_color != QUEST_ITEM_COLOR)
		slot.left_color = max(0, min(slot.left_color, MAX_COLORSET-1));
	if(slot.right_color != UNIQUE_ITEM_COLOR && slot.right_color != QUEST_ITEM_COLOR)
		slot.right_color = max(0, min(slot.right_color, MAX_COLORSET-1));

	DEBUG_ADD_FORMAT("[UI_SETCHARACTER] %d Before", slotID);
	gC_vs_ui.SetCharacter(slotID, slot);
	DEBUG_ADD_FORMAT("[UI_SETCHARACTER] %d After", slotID);
}


//-----------------------------------------------------------------------------
// Start Progress
//-----------------------------------------------------------------------------
void
UI_StartProgress(int zoneID)
{
	DEBUG_ADD("[UI] Start Progress");
	
	//---------------------------------------
	// ���� �����.
	//---------------------------------------
	if (g_pUserOption!=NULL)
	{
		if (g_pUserOption->PlayWaveMusic)
		{
#ifdef __USE_MP3__
			if(g_pMP3 != NULL)
			{
				DEBUG_ADD("MP3 STOP6");
				g_pMP3->Stop();
				DEBUG_ADD("MP3 STOP6 OK");
			}
//			if (g_pDXSoundStream!=NULL)
//			{
//				g_pDXSoundStream->Stop();
//			}
#else
//			if(g_pOGG != NULL )
//			{
				if( g_DXSound.IsInit() )
					BGMManager::Instance()->StopMusic();
//					g_pOGG->streamClose();
//			}
#endif
		}
		else
		{
			g_Music.Stop();
		}

		DEBUG_ADD("[UI] Start Progress : Stop Music OK");
	}

	g_bDrawProgress = TRUE;

	DEBUG_ADD("[UI] Start Progress : gC_vs_ui.StartProgress()");
	
#if __CONTENTS(__TIPOJYU_CASTLE)
	gC_vs_ui.StartProgress(zoneID);
#else
	gC_vs_ui.StartProgress();
#endif // __TIPOJYU_CASTLE

	DEBUG_ADD("[UI] Start Progress : gC_vs_ui.StartProgress() OK");

	if (g_pServerInformation!=NULL)
	{
		DEBUG_ADD_FORMAT("[UI] SetCurrentServerName : %x %s %s %d", g_pServerInformation, g_pServerInformation->GetServerGroupName(), g_pServerInformation->GetServerName(), g_pServerInformation->GetServerStatus());
		gC_vs_ui.SetCurrentServerName( g_pServerInformation->GetServerGroupName(),
										g_pServerInformation->GetServerName(),
										g_pServerInformation->GetServerStatus() );
		DEBUG_ADD("[UI] SetCurrentServerName OK");
#ifdef OUTPUT_DEBUG
//	extern int CLogFile;
//
//	_write( CLogFile, "<<<", 3);
//	_write( CLogFile, g_pServerInformation->GetServerGroupName(), strlen(g_pServerInformation->GetServerGroupName()) );
//	_write( CLogFile, " - ", 3);
//	_write( CLogFile, g_pServerInformation->GetServerName(), strlen(g_pServerInformation->GetServerName()) );
//	_write( CLogFile, ">>>", 3);
//	_write( CLogFile, "\n", 1 );
#endif
	}

	DEBUG_ADD("[UI] Start Progress : StartProgress OK");
	
	int oldZoneID = (g_bZonePlayerInLarge? g_nZoneLarge : g_nZoneSmall);

	/*
	PROGRESS_SPK_INDEX index;
			
	//----------------------------------------------------------------
	// tutorial 
	//----------------------------------------------------------------
	if (oldZoneID==2106 
		|| zoneID==2106)
	{
		index = TUTORIAL_HELIPORT;
	}
	//----------------------------------------------------------------
	// dungeon
	//----------------------------------------------------------------
	else if (oldZoneID==1002 || oldZoneID==1003 || oldZoneID==1004
			|| zoneID==1002 || zoneID==1003 || zoneID==1004)
	{
		index = DUNGEON_EXIT;
	}
	//----------------------------------------------------------------
	// Slayer random
	//----------------------------------------------------------------
	else if (g_pPlayer!=NULL && g_pPlayer->IsSlayer())
	{
		DEBUG_ADD("[UI] Start Progress : Slayer Random");
		
		const int numSlayerProgress = 2;
		PROGRESS_SPK_INDEX slayerProgress[numSlayerProgress] =
		{
			SLAYER_PROGRESS_1,
			SLAYER_PROGRESS_2
		};
	
		index = slayerProgress[ (PROGRESS_SPK_INDEX)(rand()%numSlayerProgress) ];

		DEBUG_ADD("[UI] Start Progress : Slayer Random OK");
	}
	//----------------------------------------------------------------
	// Vampire random
	//----------------------------------------------------------------
	else
	{
		DEBUG_ADD("[UI] Start Progress : Vampire Random");
		
		const int numVampireProgress = 3;
		PROGRESS_SPK_INDEX vampireProgress[numVampireProgress] =
		{
			VAMPIRE_PROGRESS_1,
			VAMPIRE_PROGRESS_2,
			VAMPIRE_PROGRESS_3
		};

		index = vampireProgress[ (PROGRESS_SPK_INDEX)(rand()%numVampireProgress) ];

		DEBUG_ADD("[UI] Start Progress : Vampire Random OK");
	}
	*/
	DEBUG_ADD("[UI] Start Progress : Set Progress Style");
	
	//gC_vs_ui.SetProgressStyle(index);	
	
	DEBUG_ADD("[UI] Start Progress OK");
}

//-----------------------------------------------------------------------------
// Draw Progess
//-----------------------------------------------------------------------------
void		
UI_DrawProgress(int percent)
{
	DEBUG_ADD_FORMAT("[UI] Draw Progress : %d", percent);
	
	if (g_bDrawProgress)
	{
		gC_vs_ui.SetProgress(percent, 100);
		g_pBack->FillSurface(CDirectDraw::Color(0,0,0));		

		//if (CDirect3D::GetDevice()->BeginScene()!=D3D_OK)
		//{
		//	return;
		//}

//		if (CDirect3D::IsHAL())
//		{
//			if (CDirect3D::GetDevice()->BeginScene()!=D3D_OK)
//			{
//				return;
//			}
//
//			DEBUG_ADD("3D-Before UI.Show()");
//			
//			gC_vs_ui.Show();
//
//			DEBUG_ADD("3D-After UI.Show()");
//			
//			CDirect3D::GetDevice()->EndScene();
//		}
//		else
		{
			DEBUG_ADD("2D-Before UI.Show()");
			gC_vs_ui.Show();
			DEBUG_ADD("2D-After UI.Show()");
			
			POINT point = { 0, 0 };
			RECT rect = { 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT };

			g_pBack->BltNoColorkey( &point, g_pLast, &rect );
		}

		//CDirect3D::GetDevice()->EndScene();

		CDirectDraw::Flip();
	}
}

//-----------------------------------------------------------------------------
// End Progess
//-----------------------------------------------------------------------------
void		
UI_EndProgress()
{
	DEBUG_ADD("[UI] End Progress");
	
	gC_vs_ui.EndProgress();

	g_bDrawProgress = FALSE;
}

//-----------------------------------------------------------------------------
// Set HP
//-----------------------------------------------------------------------------
void
UI_SetHP(int current, int max)
{
	if (current < 0)
	{
		current = 0;
	}

	if (max <= 0)
	{
		max = 1;
	}

	if (current > max)
	{
		current = max;
	}

	//---------------------------------------------------------------
	// HP ���� ��� üũ
	//---------------------------------------------------------------
//	__BEGIN_HELP_EVENT
		static int lastPercent = 100;
		int percent = ((max==0)?0 : current*100 / max);

		// ��� ������ 30%�̻��̾��µ�.. ���� ���ϰ� �� ���
		if (percent <= 30 && lastPercent > 30)
		{
			// [����] HP ���� ���
			ExecuteHelpEvent( HELP_EVENT_USE_POTION );
		}
		lastPercent = percent;
//	__END_HELP_EVENT

	DEBUG_ADD_FORMAT("[UI] Set HP  (%d / %d)", current, max);
	
	//---------------------------------------------------------------
	// �������� HP bar ����
	//---------------------------------------------------------------
//	if (g_pPlayer->IsSlayer())
//	{
//		if (g_pPlayer->GetConversionDelayTime() > g_CurrentTime)
//		{
//			// ���ȴ�..
//			gC_vs_ui.SetHP(current, max, TRUE);
//		}
//		else
//		{
//			gC_vs_ui.SetHP(current, max, FALSE);
//		}
//	}
//	else
//	{
//		gC_vs_ui.SetHP(current, max, FALSE, g_pPlayer->GetSilverDamage());
//	}

	/*
	if (g_Mode==MODE_GAME)
	{
		float ratio = (max==0)? 1 : (float)current/max;

		// hp�� 30%���ϸ�..
		if (ratio < 0.3f)
		{
			//g_DXMusic.SetCurrentTempo( g_DXMusic.GetOriginalTempo() * (float)(1.3f - ratio) );
			g_DXMusic.SetCurrentTempo( g_DXMusic.GetOriginalTempo() * 1.3f );
		}	
		// 100�� �ƴϸ�..
		else if (g_DXMusic.GetCurrentTempo()!=g_DXMusic.GetOriginalTempo())	
		{
			g_DXMusic.SetOriginalTempo();
		}
	}
	else
	{
		if (g_DXMusic.GetCurrentTempo()!=g_DXMusic.GetOriginalTempo())	
		{		
			g_DXMusic.SetOriginalTempo();
		}
	}
	*/
}

//-----------------------------------------------------------------------------
// Set MP
//-----------------------------------------------------------------------------
void
UI_SetMP(int current, int max)
{
	if (current < 0)
	{
		current = 0;
	}

	if (max < 0)
	{
		max = 0;
	}

	if (current > max)
	{
		current = max;
	}

	//---------------------------------------------------------------
	// MP ���� ��� üũ
	//---------------------------------------------------------------
//	__BEGIN_HELP_EVENT
		static int lastPercent = 100;
		int percent = ((max==0)?0 : current*100 / max);

		// ��� ������ 30%�̻��̾��µ�.. ���� ���ϰ� �� ���
		if (percent <= 30 && lastPercent > 30)
		{
			// [����] MP ���� ���
			ExecuteHelpEvent( HELP_EVENT_USE_POTION );
			ExecuteHelpEvent( HELP_EVENT_ABSORB_SOUL );

			lastPercent = percent;
		}
		
//	__END_HELP_EVENT

	DEBUG_ADD_FORMAT("[UI] Set MP  (%d / %d)", current, max);
	
//	gC_vs_ui.SetMP(current, max);
}

//-----------------------------------------------------------------------------
// Add Chat To History
//-----------------------------------------------------------------------------
void
UI_AddChatToHistory(const char* str, const char* sz_id, int cond, DWORD color)
{
	if (g_pUserOption->ChatWhite)
		color = gpC_base->m_chatting_pi.text_color;

	if (str == NULL || cond >= CLD_TOTAL)
	{
		return;
	}

	CHAT_LINE_CONDITION condition = (CHAT_LINE_CONDITION)cond;

	//-----------------------------------------------------------
	// �̸��� �ִٸ�.. ����ص� �Ǵ� ������� Ȯ���Ѵ�.
	//-----------------------------------------------------------
	/*
	if (sz_id!=NULL)
	{
		// ����� �ȵǴ� ID�̸� ������� �ʴ´�.
		if (!g_pChatManager->IsAcceptID( sz_id ))
		{
			return;
		}
	}
	*/

	DEBUG_ADD_FORMAT("[UI_AddChatToHistory][%d:%s] %s, %x", cond, sz_id, str, color);
	/*
	#ifdef OUTPUT_DEBUG
	const char g_sz_chat_id_divisor[CLD_TOTAL][3] =
	{
		">",
		">",
		"->",
		")",
		"}",
		":",
	};

	//	extern int CLogFile;
	//
	//	if(sz_id != NULL)
	//	{
	//	if(strcmp(sz_id, g_char_slot_ingame.sz_name.c_str()) == 0 || strstr(str, g_char_slot_ingame.sz_name.c_str()) != NULL)
	//		_write( CLogFile, "]]]", 3);
	//	_write( CLogFile, sz_id, strlen(sz_id) );
	//	_write( CLogFile, g_sz_chat_id_divisor[cond], strlen(g_sz_chat_id_divisor[cond]) );
	//	_write( CLogFile, str, strlen(str));
	//	_write( CLogFile, "\n", 1 );
	//	}
	#endif
	*/

	gC_vs_ui.AddChatToHistory(const_cast<char*>(str), sz_id, condition, color);
}

//-----------------------------------------------------------------------------
// Drop Item
//-----------------------------------------------------------------------------
void
UI_DropItem()
{
	gC_vs_ui.DropItem();
}

//-----------------------------------------------------------------------------
// Pickup Item
//-----------------------------------------------------------------------------
void
UI_PickUpItem(MItem* pItem)
{
	// �Ҹ� ����..
	PlaySound( pItem->GetInventorySoundID() );

	gC_vs_ui.PickUpItem( pItem );
}

//-----------------------------------------------------------------------------
// Lock Item
//-----------------------------------------------------------------------------
void
UI_LockItem()
{
	DEBUG_ADD("[UI] Lock Item");
	
	gC_vs_ui.LockItem();
}

//-----------------------------------------------------------------------------
// UnLock Item
//-----------------------------------------------------------------------------
void
UI_UnlockItem()
{
	DEBUG_ADD("[UI] Unlock Item");
	
	gC_vs_ui.UnlockItem();
}

//-----------------------------------------------------------------------------
// Lock Gear
//-----------------------------------------------------------------------------
void
UI_LockGear()
{
	DEBUG_ADD("[UI] Lock Gear");
	
	gC_vs_ui.LockGear();
}

//-----------------------------------------------------------------------------
// Unlock Gear
//-----------------------------------------------------------------------------
void
UI_UnlockGear()
{
	DEBUG_ADD("[UI] Unlock Gear");
	
	gC_vs_ui.UnlockGear();
}

//-----------------------------------------------------------------------------
// UI �޼��� ó��
//-----------------------------------------------------------------------------
void UI_ResultReceiver(DWORD message, int left, int right, void *void_ptr)
{
	//DEBUG_ADD("[UI_ResultReceiver] Start");

	if (g_pUIMessageManager==NULL)
	{
		g_pUIMessageManager = new UIMessageManager;
	}
	g_pUIMessageManager->Execute(message, left, right, void_ptr);

	//DEBUG_ADD("[UI_ResultReceiver] End");
}

//-----------------------------------------------------------------------------
// Run Party
//-----------------------------------------------------------------------------
void		
UI_RunParty()
{
	// by csm 12/4
	if (!UI_IsRunningParty())
	{	
		//gC_vs_ui.RunPartyManager();
		gC_vs_ui.HotKey_Party();
	}
}

//-----------------------------------------------------------------------------
// Request Party
//-----------------------------------------------------------------------------
void
UI_RunPartyRequest(TYPE_OBJECTID otherID)
{
	if (g_pZone==NULL)
	{
		return;
	}

	//-------------------------------------------
	// ��Ƽ ����â ����.
	//-------------------------------------------
	MCreature* pCreature = g_pZone->GetCreature( otherID );

	if (pCreature!=NULL)
	{
		gC_vs_ui.RequestParty( pCreature->GetName(), 30000 );

		// ��Ƽ ��û���̶�� �ǹ�..
		// ���� ���� ó���Ѵ�.
		//g_pPlayer->SetWaitVerify( MPlayer::WAIT_VERIFY_PARTY );

		//g_pTempInformation->Mode = TempInformation::MODE_PARTY_REQUEST;
		//g_pTempInformation->Value1 = otherID;

		g_pTempInformation->PartyInviter = otherID;

		// [����] ��ȯ��û ���� ��
		//__BEGIN_HELP_EVENT
		//	ExecuteHelpEvent( HE_TRADE_REQUESTED );	
		//__END_HELP_EVENT

	}
	else
	{
		DEBUG_ADD_FORMAT( "[Error] There is no such Creature. id=%d", otherID);
	}	
}


//-----------------------------------------------------------------------------
// Run PartyAsk
//-----------------------------------------------------------------------------
void		
UI_RunPartyAsk(TYPE_OBJECTID otherID)
{
	// by csm 12/4
	MCreature* TempCreature = g_pZone->GetCreature( otherID );
	if(TempCreature->CurPernalShop() != 1)
	{
		
		
		if (g_pZone==NULL || !g_pSystemAvailableManager->IsAvailablePartySystem())
		{
			return;
		}
		
		//-------------------------------------------
		// ��Ƽ ����â ����.
		//-------------------------------------------
		MCreature* pCreature = g_pZone->GetCreature( otherID ); 
		
		if (pCreature!=NULL)
		{
			// C_VS_UI_REQUEST_PARTY::REQUEST
			gC_vs_ui.RunPartyAsk( pCreature->GetName(), C_VS_UI_REQUEST_PARTY::INVITE );
			
			// ��Ƽ �������Ѵ�.
			//g_pPlayer->SetWaitVerify( MPlayer::WAIT_VERIFY_PARTY );
			
			//g_pTempInformation->Mode = TempInformation::MODE_PARTY_REQUEST;
			g_pTempInformation->PartyInviter = otherID;
			
			// [����] ��ȯ��û ���� ��
			//__BEGIN_HELP_EVENT
			//	ExecuteHelpEvent( HE_TRADE_REQUESTED );	
			//__END_HELP_EVENT
			
		}
		else
		{
			DEBUG_ADD_FORMAT( "[Error] There is no such Creature. id=%d", otherID);
		}
	}
}

//-----------------------------------------------------------------------------
// Run PartyCancel
//-----------------------------------------------------------------------------
void		
UI_RunPartyCancel(const char* pName)
{
	gC_vs_ui.RunPartyCancel(pName);

	// [����] ��ȯ��û �ϰ� ����
	//__BEGIN_HELP_EVENT
	//	ExecuteHelpEvent( HE_TRADE_REQUEST );	
	//__END_HELP_EVENT
}

//-----------------------------------------------------------------------------
// Close PartyRequest
//-----------------------------------------------------------------------------
void		
UI_ClosePartyRequest()
{
	gC_vs_ui.FinishRequestParty();
}


//-----------------------------------------------------------------------------
// Close Party
//-----------------------------------------------------------------------------
void		
UI_CloseParty()
{
	gC_vs_ui.ClosePartyManager();
}

//-----------------------------------------------------------------------------
// Close PartyAsk
//-----------------------------------------------------------------------------
void		
UI_ClosePartyAsk()
{
	gC_vs_ui.ClosePartyAsk();

	// ��ȯ��
	if (g_pPlayer->GetWaitVerify()==MPlayer::WAIT_VERIFY_PARTY)
	{
		g_pPlayer->SetWaitVerifyNULL();
	}

	if (g_pTempInformation->GetMode() == TempInformation::MODE_PARTY_REQUEST)
	{
		g_pTempInformation->SetMode(TempInformation::MODE_NULL);
	}
}

//-----------------------------------------------------------------------------
// Close PartyCancel
//-----------------------------------------------------------------------------
void		
UI_ClosePartyCancel()
{
	if (g_pPlayer->IsWaitVerify()==MPlayer::WAIT_VERIFY_PARTY)
	{
		g_pPlayer->SetWaitVerifyNULL();		
	}

	if (g_pTempInformation->GetMode() == TempInformation::MODE_PARTY_REQUEST)
	{
		g_pTempInformation->SetMode(TempInformation::MODE_NULL);
	}

	gC_vs_ui.ClosePartyCancel();
}

//-----------------------------------------------------------------------------
// IsRunningPartyManager
//-----------------------------------------------------------------------------
bool
UI_IsRunningParty() 
{
	return gC_vs_ui.IsRunningPartyManager();
}

//-----------------------------------------------------------------------------
// IsRunningPartyManager
//-----------------------------------------------------------------------------
bool
UI_SetWaitGuildList() 
{
	return gC_vs_ui.IsRunningPartyManager();
}

//-----------------------------------------------------------------------------
// UI_SetWaitGuildList
//-----------------------------------------------------------------------------
void
UI_ShowWaitGuildList( GCWaitGuildList *pPacket)
{
	// NPC��ȭ�ϴ� dialog�� ����� 
	g_pUIDialog->HidePCTalkDlg();

	DEBUG_ADD_FORMAT( "[GCWaitGuildList] Start");
	gC_vs_ui.RunTeamList(true);
	DEBUG_ADD_FORMAT( "[GCWaitGuildList] Run gC_vs_ui.RunTeamList(true)");

	GuildInfo *pInfo = NULL;
	C_VS_UI_TEAM_LIST::READY_TEAM_LIST ready_team_info;

	while(pInfo = pPacket->popFrontGuildInfoList())
	{
		ready_team_info.guild_id = pInfo->getGuildID();
		ready_team_info.TEAM_NAME = pInfo->getGuildName();
		ready_team_info.LEADER_NAME = pInfo->getGuildMaster();
		ready_team_info.EXPIRE_DATE = pInfo->getGuildExpireDate();
		ready_team_info.MEMBERS = pInfo->getGuildMemberCount();
		ready_team_info.MEMBERS_MAX = 5;

		gC_vs_ui.AddReadyTeamInfo(ready_team_info);
	}

	DEBUG_ADD_FORMAT( "[GCWaitGuildList] Finish");

}

void		
UI_ShowActiveGuildList( GCActiveGuildList *pPacket)
{
	// NPC��ȭ�ϴ� dialog�� ����� 
	g_pUIDialog->HidePCTalkDlg();

	DEBUG_ADD_FORMAT( "[GCActiveGuildList] Start");
	gC_vs_ui.RunTeamList(false);
	DEBUG_ADD_FORMAT( "[GCActiveGuildList] Run gC_vs_ui.RunTeamList(false)");

	GuildInfo *pInfo = NULL;
	C_VS_UI_TEAM_LIST::REGIST_TEAM_LIST regist_team_info;

	while(pInfo = pPacket->popFrontGuildInfoList())
	{
		regist_team_info.guild_id = pInfo->getGuildID();
		regist_team_info.TEAM_NAME = pInfo->getGuildName();
		regist_team_info.LEADER_NAME = pInfo->getGuildMaster();
		// ��ŷ ���߿� �����.��
		regist_team_info.RANKING = 0;
		regist_team_info.MEMBERS = pInfo->getGuildMemberCount();

		gC_vs_ui.AddRegistTeamInfo(regist_team_info);
	}

	DEBUG_ADD_FORMAT( "[GCActiveGuildList] Finish");
	
}

void		
UI_ShowGuildMemberList( GCGuildMemberList *pPacket)
{
	DEBUG_ADD_FORMAT( "[GCShowGuildMemberList] Start");

	if(false == gC_vs_ui.IsRunningTeamMemberList())
		gC_vs_ui.RunTeamMemberList();
	GuildMemberInfo *pInfo = NULL;

	C_VS_UI_TEAM_MEMBER_LIST::TEAM_MEMBER_LIST member_info;
	
	int groupID = g_pServerInformation->GetServerGroupID();
	ServerGroup* pServerGroup = g_pServerInformation->GetData( groupID );

	bool SetServerName;
	BYTE bAvailableRecall = pPacket->getType();
	while(pInfo = pPacket->popFrontGuildMemberInfoList())
	{
		SetServerName = false;
		member_info.MEMBER_NAME = pInfo->getName();
		member_info.member_grade = pInfo->getRank();
		member_info.bLogOn = pInfo->getLogOn();
		// 2004, 10, 8, sobeit add start 
//		member_info.server_type = pInfo->getServerID();  
		if(pServerGroup && member_info.bLogOn)
		{
			ServerGroup::const_iterator iServer = pServerGroup->find(pInfo->getServerID());
			if(iServer != pServerGroup->end())
			{
				SERVER_INFO* pServerInfo = iServer->second;
				if(NULL != pServerInfo)
				{
					member_info.SERVER_NAME = pServerInfo->ServerName.GetString(); 
					SetServerName = true;
				}
			}
		}
		if(false == SetServerName)
		{
			member_info.SERVER_NAME = (*g_pGameStringTable)[UI_STRING_MESSAGE_NOT_LOGINED].GetString();
			member_info.bLogOn = false; // �������� ã���� ���°Ŵ� �׳� �� �α������� ����..����
		}
		// 2004, 10, 8, sobeit add end
		gC_vs_ui.AddTeamMemberInfo(member_info, bAvailableRecall); 
	}

	DEBUG_ADD_FORMAT( "[GCShowGuildMemberList] Finish");	
}


void		
UI_ShowGuildRegist(DWORD reg_fee)
{
	// NPC��ȭ�ϴ� dialog�� ����� 
	g_pUIDialog->HidePCTalkDlg();
	
	gC_vs_ui.RunTeamRegist(false, reg_fee);
}

void		
UI_ShowWaitGuildInfo(GCShowWaitGuildInfo *pPacket)
{
	C_VS_UI_TEAM_INFO::READY_TEAM_INFO team_info;
	team_info.TEAM_NAME = pPacket->getGuildName();
	team_info.LEADER_NAME = pPacket->getGuildMaster();
	team_info.INTRODUCTION = pPacket->getGuildIntro();
	team_info.guild_id = pPacket->getGuildID();
	team_info.MEMBERS = pPacket->getMemberNum();
	team_info.MEMBERS_MAX = 5;
	for(int i = 0; i < team_info.MEMBERS; i++)
	{
		team_info.MEMBERS_NAME.push_back(pPacket->popMember());
	}
	team_info.REG_FEE = pPacket->getJoinFee();
	team_info.EXPIRE_DATE = "";

	//20090130 ��� ������ �弳 ���� �ɱ� 
	char* teamInfoIntroduction = const_cast<char *>(team_info.INTRODUCTION.c_str());
	g_pChatManager->RemoveCurse(teamInfoIntroduction);
	team_info.INTRODUCTION = teamInfoIntroduction;

	gC_vs_ui.RunTeamInfo(true, (void *)&team_info);
}

void		
UI_ShowGuildStartJoin(DWORD join_fee)
{
	gC_vs_ui.RunTeamRegist(true, join_fee);
}

void		
UI_ShowGuildJoin(DWORD join_fee, int rank, const char *date, const char *team_name, int guild_id)
{
	gC_vs_ui.RunTeamRegist(true, join_fee, rank, (char *)date, (char *)team_name, guild_id);
}

void		
UI_ShowGuildInfo(GCShowGuildInfo *pPacket)
{
	C_VS_UI_TEAM_INFO::REGIST_TEAM_INFO team_info;
	team_info.TEAM_NAME = pPacket->getGuildName();
	team_info.LEADER_NAME = pPacket->getGuildMaster();
	team_info.INTRODUCTION = pPacket->getGuildIntro();
	team_info.guild_id = pPacket->getGuildID();
	team_info.MEMBERS = pPacket->getGuildMemberCount();
	team_info.REG_FEE = pPacket->getJoinFee();
	team_info.REGISTERED_DATE = "";
	team_info.RANKING = 0;

//20090130 ��� ������ �弳 ���� �ɱ� 
	char* teamInfoIntroduction = const_cast<char *>(team_info.INTRODUCTION.c_str());
	g_pChatManager->RemoveCurse(teamInfoIntroduction);
	team_info.INTRODUCTION = teamInfoIntroduction;

	// 2004, 10, 19, sobeit add start - ���� �� �ִ� ��� ����Ʈ�� ���ո���Ʈ ���� �׳� ��� ����Ʈ ����..����..
	bool IsUnionTeamInfo = gC_vs_ui.IsRunningTeamList(true); //  
	// 2004, 10, 19, sobeit add end
	gC_vs_ui.RunTeamInfo(false, (void *)&team_info, IsUnionTeamInfo);
}

void		
UI_ShowGuildMemberInfo(GCShowGuildMemberInfo *pPacket)
{
	C_VS_UI_TEAM_MEMBER_INFO::MEMBER_INFO member_info;
	member_info.guild_id = pPacket->getGuildID();
	member_info.NAME = pPacket->getName();
	member_info.GRADE = pPacket->getGuildMemberRank();
	member_info.INTRODUCTION = pPacket->getGuildMemberIntro().c_str();
	gC_vs_ui.RunTeamMemberInfo(&member_info);
}

void		
UI_ShowUnionGuildMemberInfo(GCShowUnionInfo *pPacket)
{
//	C_VS_UI_TEAM_MEMBER_INFO::MEMBER_INFO member_info;
//	member_info.guild_id = pPacket->getGuildID();
//	member_info.NAME = pPacket->getName();
//	member_info.GRADE = pPacket->getGuildMemberRank();
//	member_info.INTRODUCTION = pPacket->getGuildMemberIntro().c_str();
//	gC_vs_ui.RunTeamMemberInfo(&member_info);

	DEBUG_ADD_FORMAT( "[GCShowUnionInfo] Start");
	gC_vs_ui.RunTeamList(false, true);
	DEBUG_ADD_FORMAT( "[GCShowUnionInfo] Run gC_vs_ui.RunTeamList(false)");
	C_VS_UI_TEAM_LIST::REGIST_TEAM_LIST regist_team_info;
	SingleGuildInfo&pmaster = pPacket->getMasterGuildInfo();
	
	regist_team_info.guild_id = pmaster.getGuildID();
	regist_team_info.TEAM_NAME = pmaster.getGuildName();
	regist_team_info.LEADER_NAME = pmaster.getGuildMaster();
	regist_team_info.RANKING = 0;
	regist_team_info.MEMBERS = pmaster.getGuildMemberCount();
	gC_vs_ui.AddRegistTeamInfo(regist_team_info);

	SingleGuildInfo *pInfo = NULL;
	while(pInfo = pPacket->popFrontUnionGuildInfoList())
	{
		regist_team_info.guild_id = pInfo->getGuildID();
		regist_team_info.TEAM_NAME = pInfo->getGuildName();
		regist_team_info.LEADER_NAME = pInfo->getGuildMaster();
		// ��ŷ ���߿� �����.��
		regist_team_info.RANKING = 0;
		regist_team_info.MEMBERS = pInfo->getGuildMemberCount();
	
		gC_vs_ui.AddRegistTeamInfo(regist_team_info);
	}
	DEBUG_ADD_FORMAT( "[GCActiveGuildList] Finish");
}

void
UI_ShowGuildQuit()
{
}

void		UI_ResetQuickItemSlot()
{
	if( g_pPlayer->IsSlayer() )
		gC_vs_ui.ResetSlayerQuickItemSize();
	else if( g_pPlayer->IsOusters() )
		gC_vs_ui.ResetOustersQuickItemSize();
}

void		UI_SetWeaponSpeed(int speed, BYTE type)
{
	g_char_slot_ingame.WS = type;
	g_char_slot_ingame.WeaponSpeed = speed;
}

void		UI_CloseEnchant()
{
	gC_vs_ui.CloseEnchant();
}

void	UI_SetGuild(WORD guild_id, BYTE guild_rank, std::string guild_name)
{
	g_char_slot_ingame.GUILD_ID = guild_id;
	g_char_slot_ingame.GUILD_GRADE = guild_rank;
	g_char_slot_ingame.sz_guild_name = guild_name;
	gC_vs_ui.CloseTeamMemberList();
}


void	UI_RunOtherInfo(MCreature *pCreature)
{
	if(gC_vs_ui.IsRunningHorn() || pCreature->IsHallu())
		return;

	DEBUG_ADD("[UI_RunOtherInfo] START");

	if(pCreature == NULL)
	{
		DEBUG_ADD("[UI_RunOtherInfo] pCreature == NULL");
		return;
	}


	C_VS_UI_OTHER_INFO::PLAYER_INFO info;
	if(pCreature->GetName() != NULL)
		info.PLAYER_NAME = pCreature->GetName();
	else
		DEBUG_ADD("[UI_RunOtherInfo] Name == NULL");

//	GUILD_INFO *guildInfo = g_pGuildInfoMapper->Get(pCreature->GetGuildNumber());

//	if(guildInfo != NULL && guildInfo->GetGuildName() != NULL)
//		info.TEAM_NAME = guildInfo->GetGuildName();

	info.DOMAINLEVEL[SKILLDOMAIN_BLADE] = pCreature->GetBLADE_DOMAIN_LEVEL();
	info.DOMAINLEVEL[SKILLDOMAIN_SWORD] = pCreature->GetSWORD_DOMAIN_LEVEL();
	info.DOMAINLEVEL[SKILLDOMAIN_GUN] = pCreature->GetGUN_DOMAIN_LEVEL();
	info.DOMAINLEVEL[SKILLDOMAIN_HEAL] = pCreature->GetHEAL_DOMAIN_LEVEL();
	info.DOMAINLEVEL[SKILLDOMAIN_ENCHANT] = pCreature->GetENCHANT_DOMAIN_LEVEL();

	info.bl_female = !pCreature->IsMale();

	if(pCreature->IsSlayer())
	{
		MCreatureWear *pCreatureWear = (MCreatureWear *)pCreature;
		const CHAR_MAN uiFace[] =
		{
			M_FACE1,
			M_FACE2,
			M_FACE3
		};

		int faceFrameID = pCreatureWear->GetHairFrameID();
		
		info.face_type = uiFace[faceFrameID - ADDONID_HAIR1];
	}
	info.GRADE = pCreature->GetGrade();
	info.LEVEL = pCreature->GetLEVEL();
	info.FAME = pCreature->GetFAME();
	info.ALIGNMENT = pCreature->GetAlignment();
	info.STR_CUR = pCreature->GetSTR();
	info.DEX_CUR = pCreature->GetDEX();
	info.INT_CUR = pCreature->GetINT();
	info.STR_PURE = pCreature->GetBASIC_STR();
	info.DEX_PURE = pCreature->GetBASIC_DEX();
	info.INT_PURE = pCreature->GetBASIC_INT();
	info.guild_id = pCreature->GetGuildNumber();

#if __CONTENTS(__CONTRIBUTE_SYSTEM)
	info.contributePoint = pCreature->GetContributePoint();
#endif
	
	info.Origin_ServerIdx = pCreature->GetOriginServerNum();
	if(pCreature->GetOldName() != NULL)
		info.PLAYER_OLDNAME = pCreature->GetOldName() ;
	
	DEBUG_ADD_FORMAT("[UI_RunOtherInfo] Name : %s, guild_id : %d, BLADE : %d, SWORD : %d, GUN : %d, HEAL : %d, ENCHANT : %d, GRADE : %d, LEVEL : %d, FAME : %d, ALIGNMENT : %d, STR_PURE : %d, STR_CUR : %d, DEX_PURE : %d, DEX_CUR : %d, INT_PURE : %d, INT_CUR : %d, face_type : %d, bl_female : %d", 
		info.PLAYER_NAME.c_str(), info.guild_id, info.DOMAINLEVEL[0], info.DOMAINLEVEL[1], info.DOMAINLEVEL[2], info.DOMAINLEVEL[3], info.DOMAINLEVEL[4], info.GRADE, info.LEVEL, info.FAME, info.ALIGNMENT, info.STR_PURE, info.STR_CUR, info.DEX_PURE, info.DEX_CUR, info.INT_PURE, info.INT_CUR, info.face_type, info.bl_female);
	
	if(!gC_vs_ui.IsRunningOtherInfo())
		gC_vs_ui.RunOtherInfo();
	gC_vs_ui.SetOtherInfo(info);
	gC_vs_ui.SetOtherPetInfo(NULL);
	if(pCreature->GetPetID() != OBJECTID_NULL)
	{
		MFakeCreature *pFakeCreature = (MFakeCreature *)g_pZone->GetFakeCreature(pCreature->GetPetID());
		if(pFakeCreature != NULL)
		{
			MPetItem *pPetItem = pFakeCreature->GetPetItem();
			if(pPetItem != NULL)
			{
				PETINFO petInfo;
				petInfo.HP = pPetItem->GetCurrentDurability();
				petInfo.ATTR = pPetItem->GetEnchantLevel();
				petInfo.ATTR_VALUE = pPetItem->GetSilver();
				petInfo.LEVEL = pPetItem->GetNumber();
				petInfo.FOODTYPE = pPetItem->GetPetFoodType();
				petInfo.EXP_REMAIN = pPetItem->GetPetExpRemain();
				petInfo.CUT_HEAD = pPetItem->IsCanCutHead();
				petInfo.GAMBLE = pPetItem->IsCanGamble();

				if(pPetItem->GetItemOptionListCount() == 0)
#if __CONTENTS(!__NEW_PET_INCUBUS)
					petInfo.OPTION = -1;
				else
					petInfo.OPTION = pPetItem->GetItemOptionList().front();
#else
				{
					petInfo.OPTION_LIST.clear();
				}	
				else
				{
					const std::list<TYPE_ITEM_OPTION> &optionList = pPetItem->GetItemOptionList();
					std::list<TYPE_ITEM_OPTION>::const_iterator itr= optionList.begin();
				
					for(; itr!=optionList.end() && *itr < g_pItemOptionTable->GetSize(); itr++)
					{
						petInfo.OPTION_LIST.push_back( *itr );
					}
				}
#endif //__NEW_PET_INCUBUS
				
				petInfo.ITEM_TYPE = pPetItem->GetItemType();
				petInfo.NAME = pPetItem->GetPetName();
				petInfo.ITEM_ID = pPetItem->GetID();
				petInfo.ChangeAbleNickName = pFakeCreature->GetOwnerID() == g_pPlayer->GetID();
				petInfo.ENCHANTSKILLTYPE = pPetItem->GetEnchantSkillType();
#if __CONTENTS(__FUNCTION_PETFOOD)
				petInfo.FUNCTION_PETFOODTYPE = pPetItem->getFunctionPetfoodType();
#endif //__FUNCTION_PETFOOD
				#if __CONTENTS(__PET_MIXINGFORGE)
				const std::list<TYPE_ITEM_OPTION> &petMixingOption = pPetItem->GetPetMixingOptionList();
				std::list<TYPE_ITEM_OPTION>::const_iterator itr = petMixingOption.begin();
				
				for(;itr != petMixingOption.end(); ++itr)
				{
					petInfo.MIXINGOPTION.push_back( *itr );
				}
#endif //__PET_MIXINGFORGE
				gC_vs_ui.SetOtherPetInfo(&petInfo);
			}
		}
	}

	DEBUG_ADD("[UI_RunOtherInfo] END");	
}

const char *UI_GetOtherInfoName()
{
	if(gC_vs_ui.IsRunningOtherInfo())
		return gC_vs_ui.GetCurrentOtherInfoName();

	return NULL;
}

extern std::string g_CpCookie;
#ifdef __DESIGNED_JAPAN
//�Ϻ� ���񽺿� - �Ϻ� �ݸ����� ������ ����� ��� ����Ͽ� �α���
extern std::string g_CpPassword;
#endif

// �ݸ�����
void	UI_RunConnect()
{
	if(g_pUserInformation->IsNetmarble && g_pUserInformation->IsNetmarbleLogin)
	{
		static LOGIN	login;
		// ��� �ٲ�.   2003.12.12 by sonee
		//login.sz_id = new char [g_pUserInformation->NetmarbleID.GetLength()+1];
		//strcpy(login.sz_id, g_pUserInformation->NetmarbleID.GetString());
		//		login.sz_password = new char [g_pUserInformation->NetmarblePassword.GetLength()+1];
//		strcpy(login.sz_password, g_pUserInformation->NetmarblePassword.GetString());
		
		login.sz_id = new char[ g_CpCookie.length() + 1 ];
		login.sz_password = new char[ 10 ];
		
		strcpy( login.sz_id, g_CpCookie.c_str() );		
#ifdef __DESIGNED_JAPAN
		strcpy( login.sz_password, g_CpPassword.c_str() );
#else
		strcpy( login.sz_password, "DarkEden" );
#endif	
		gpC_base->SendMessage(UI_LOGIN, 0, 0, (void *)&login);
	}
	else
	{
		// 2004, 7, 15, sobeit modify start
		// 20070518 by diesirace �����α��� ������ �Ǹ鼭 Ÿ��Ʋ�� ����
		//if(false == g_pUserInformation->IsAutoLogIn) // ���� �α���
		//	gC_vs_ui.RunConnect();
		//else // �� �α���
		//{
			if(NULL != g_pUserInformation->pLogInClientPlayer)
			{
				const int numGroup = g_pServerInformation->size();
				if(numGroup != 0)
				{
					UI_SetWorldList();
					SetMode( MODE_WAIT_SELECT_WORLD );
				}
				else
				{
					CLGetWorldList clGetWorldList;
					ClientPlayer * TempClient = (ClientPlayer *)(g_pUserInformation->pLogInClientPlayer);
					TempClient->sendPacket( &clGetWorldList );
		
					SetMode( MODE_WAIT_WORLD_LIST );
				}			
			}
//			static LOGIN	login;
//			login.sz_id = new char[  g_pUserInformation->UserID.GetLength() + 1 ];
//			login.sz_password = new char[ 20 ];
//			
//			strcpy( login.sz_id, g_pUserInformation->UserID.GetString() );		
//			strcpy( login.sz_password, g_pUserInformation->AutoLogInKeyValue.GetString() );
//			
//			gpC_base->SendMessage(UI_LOGIN, 0, 0, (void *)&login);
//		}
		// 2004, 7, 15, sobeit modify end
	}
}

void	UI_OpenBringFee(UINT totalfee)
{
	DEBUG_ADD("[UI_OpenBringFee] START");
	g_pUIDialog->HidePCTalkDlg();
	gC_vs_ui.RunBringFeeWindow(totalfee);
	DEBUG_ADD("[UI_OpenBringFee] END");
//	gC_vs_ui.OpenInventoryToSell();	
}

void	UI_SetTotalFee(UINT fee)
{
	// ���� â�� �����ִ� �� �ݾװ� �����κ��� ���� ���� ������°� ���ؼ� �� ���̸�ŭ ������ �����ش�.		
	gC_vs_ui.SetTotalFee(fee);
	gC_vs_ui.SetBringFee(0);
}

void	UI_ClearNPC()
{
	DEBUG_ADD("[UI_CLEARNPC]");
	gC_vs_ui.ClearNPC();
}

void	UI_RunWarList(GCWarScheduleList* pPacket)
{
	DEBUG_ADD("[UI_RunWarList] Start");
	g_pUIDialog->UnSetLockInputPCTalk();
	g_pUIDialog->ClosePCTalkDlg();
	g_pPlayer->SetWaitVerifyNULL();

	DEBUG_ADD("[UI_RunWarList] OK close PCDialog");

	gC_vs_ui.RunWarListWindow();

	DEBUG_ADD("[UI_RunWarList] OK RunWarListWindow");

	while(1)
	{
		WarScheduleInfo *info = pPacket->popWarScheduleInfo();

		if(info == NULL)
			break;


		C_VS_UI_WAR_LIST::WarInfo wi;

		wi.warType	= info->warType;
		wi.year		= info->year;
		wi.month	= info->month;
		wi.day		= info->day;
		wi.hour		= info->hour;
		if(0 == info->warType) // ������϶� - 2004,10,2,sobeit modify
		{
			for(int i = 0; i<5; i++)
			{
				wi.challengerGuildID[i] = info->challengerGuildID[i];
				wi.challengerGuildName[i] = info->challengerGuildName[i];
			}
			wi.reinforceGuildID = info->reinforceGuildID;
			wi.reinforceGuildName = info->reinforceGuildName;
		}
		
		
		gC_vs_ui.AddWarList(wi);

		delete info;
	}
	DEBUG_ADD("[UI_RunWarList] END");
}

void	UI_CloseBloodBibleStatus()
{
	if(gC_vs_ui.IsRunningBloodBibleStatus())
	{
		gC_vs_ui.CloseBloodBibleStatus();
	}
}

void	UI_RunBloodBibleStatus()
{
	gC_vs_ui.RunBloodBibleStatus();
	gC_vs_ui.SetBloodBibleStatusTimer(0);
}

void	UI_SetBloodBibleStatus(GCBloodBibleStatus * pPacket)
{
	C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_STATUS status;
	
	status.zone_id = pPacket->getZoneID();
	status.x = pPacket->getX();
	status.y = pPacket->getY();
	status.name = pPacket->getOwnerName();
	status.shrine_race = pPacket->getShrineRace();
	
	switch(pPacket->getStorage())
	{
	case STORAGE_INVENTORY:
		if(pPacket->getRace() == RACE_SLAYER)	// Slayer
			status.status = C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_HAS_SLAYER;
		else if( pPacket->getRace() == RACE_VAMPIRE )
			status.status = C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_HAS_VAMPIRE;
		else if (pPacket->getRace() == RACE_OUSTERS )
			status.status = C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_HAS_OUSTERS;
		break;

	case STORAGE_ZONE:
		status.status = C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_DROP;
		break;

	case STORAGE_CORPSE:
		if(pPacket->getRace() == RACE_SLAYER)	// Slayer
			status.status = C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_SHRINE_SLAYER;
		else if(pPacket->getRace() == RACE_VAMPIRE )
			status.status = C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_SHRINE_VAMPIRE;
		else if (pPacket->getRace() == RACE_OUSTERS )
			status.status = C_VS_UI_BLOOD_BIBLE_STATUS::BLOOD_BIBLE_SHRINE_OUSTERS;
		break;
	}

	int itemType = pPacket->getItemType();
	gC_vs_ui.SetBloodBibleStatus(itemType, status);
}

void	UI_SetBloodBibleStatusTimer(DWORD sec)
{
	gC_vs_ui.SetBloodBibleStatusTimer(sec);
}

void	UI_RunInputNameWindow(int ID)
{
	if(gC_vs_ui.IsRunningInputNameWindow())
		gC_vs_ui.CloseInputNameWindow();

	g_pUIDialog->HidePCTalkDlg();
	gC_vs_ui.RunInputNameWindow((C_VS_UI_INPUT_NAME::INPUT_NAME_MODE_LIST) ID );
	gC_vs_ui.SetEnableInputName();
}

void	UI_CloseInputNameWindow()
{	
	gC_vs_ui.CloseInputNameWindow();	
	g_pUIDialog->UnSetLockInputPCTalk();
	g_pUIDialog->ClosePCTalkDlg();
	g_pPlayer->SetWaitVerify( MPlayer::WAIT_VERIFY_NULL );	
	g_pTempInformation->SetMode ( TempInformation::MODE_NULL );
}

void		UI_RunAskGoBilingPage(int n)
{
	switch(n)
	{
	case 0 :
		gC_vs_ui.RunCannotPlayMessage();
		break;
	case 1 :
		gC_vs_ui.RunCannotPlayByAttrMessage();
		break;
	}
}

void		UI_RunQuestList(GCSelectQuestID *pPacket)
{		
	if( g_pQuestInfoManager == NULL )
		return;

	// No quest fits the player's grade (Slayer) or level: say so and leave the NPC
	// menu usable, instead of waiting for an answer that never comes.
	if( pPacket->empty() )
	{
		g_pPlayer->SetWaitVerifyNULL();
		g_pUIDialog->PopupFreeMessageDlg( (*g_pGameStringTable)[UI_STRING_MESSAGE_FAIL_MONSTER_KILL_QUEST_BY_STATUS].GetString() );
		return;
	}

	g_pPlayer->SetWaitVerifyNULL();
	g_pPCTalkBox->Release();	
	g_pPCTalkBox->SetType( PCTalkBox::SELECT_QUEST );

	if( g_pPCTalkBox->GetCreatureType() == 17 || g_pPCTalkBox->GetCreatureType() == 255 || g_pPCTalkBox->GetCreatureType() == 653)			// ī������, ����ī��
	{
		char tempstr[128] = {0,};
			
		while( !pPacket->empty() )
		{
			char str[256];
			QuestID_t	ID = pPacket->popQuestID();		
			QUEST_INFO* mkq = g_pQuestInfoManager->GetInfo( ID );		
			if(NULL != mkq )
			{
				DWORD time = mkq->GetTimeLimit();				
				DWORD minute = (time / 60) % 60;
				DWORD hour = (time / 60 / 60);
				bool bContinue = false;
				
				if(hour > 0 || bContinue)
				{
					bContinue = true;
					char temp[100];
					sprintf(temp,(*g_pGameStringTable)[UI_STRING_MESSAGE_HOUR].GetString(), hour );
					strcat(tempstr, temp);
					strcat(tempstr, " ");
				}
				if(minute > 0 || bContinue)
				{
					bContinue = true;
					char temp[100];
					sprintf(temp,(*g_pGameStringTable)[UI_STRING_MESSAGE_MINUTE].GetString(), minute );
					strcat(tempstr, temp);			
				}
				char selectType = 0;
				switch( mkq->GetType() )
				{
				case QUEST_INFO_MONSTER_KILL :
					if( g_pPlayer->IsVampire() )
					{
						wsprintf(str,(*g_pGameStringTable)[STRING_MESSAGE_MONSTER_KILL_QUEST_STRING_SET_VAMPIRE],mkq->GetName(),mkq->GetGoal(),tempstr);
					} else
					{
						wsprintf(str,(*g_pGameStringTable)[STRING_MESSAGE_MONSTER_KILL_QUEST_STRING_SET],mkq->GetName(),mkq->GetGoal(),tempstr);
					}						
					break;
				case QUEST_INFO_MEET_NPC :
					if( g_pPlayer->IsVampire() )
					{
						wsprintf(str, (*g_pGameStringTable)[STRING_MESSAGE_MEET_NPC_VAMPIRE].GetString() );
					} else
					{
						wsprintf(str, (*g_pGameStringTable)[STRING_MESSAGE_MEET_NPC_SLAYER].GetString() );				
					}
					break;
				case QUEST_INFO_GATHER_ITEM :
					break;
				case QUEST_INFO_MINI_GAME :
					wsprintf(str, (*g_pGameStringTable)[STRING_MESSAGE_SELECT_MINI_GAME].GetString() );
					selectType = 1;
					break;
				}		
				g_pPCTalkBox->SetContent( str );

				if( selectType == 0 )
				{
					wsprintf(tempstr,"%4d%s",ID,(*g_pGameStringTable)[STRING_MESSAGE_YES_I_SEE].GetString() );		
					g_pPCTalkBox->AddString( tempstr );				
				} else
				{
					wsprintf(tempstr,"%4d",ID);
					
					switch( mkq->GetGameType() )
					{
					case GAME_MINE :
						strcat(tempstr,(*g_pGameStringTable)[STRING_MESSAGE_SELECT_CRAZY_MINE].GetString() );
						g_pPCTalkBox->AddString( tempstr );
						break;
					case GAME_ARROW :
						strcat(tempstr,(*g_pGameStringTable)[STRING_MESSAGE_SELECT_ARROW_TILES].GetString() );
						g_pPCTalkBox->AddString( tempstr );
						break;
					}
				}
			}
		}
//		wsprintf(tempstr,"9999%s",g_pPlayer->IsSlayer() ? 
//					(*g_pGameStringTable)[STRING_MESSAGE_CANCEL_MONSTER_KILL_QUEST].GetString() :
//					(*g_pGameStringTable)[STRING_MESSAGE_CANCEL_QUEST_VAMPIRE].GetString() );		
//				g_pPCTalkBox->AddString( tempstr );	

		wsprintf(tempstr,"9999%s",(*g_pGameStringTable)[UI_STRING_MESSAGE_CANCEL_SELECT_QUEST].GetString() );
		g_pPCTalkBox->AddString( tempstr );
	} else
	{
		if( g_pPlayer->IsVampire() )
		{
			g_pPCTalkBox->SetContent( (*g_pGameStringTable)[UI_STRING_MESSAGE_SELECT_QUEST_VAMPIRE].GetString() );
		} else if( g_pPlayer->IsSlayer() )
		{
			g_pPCTalkBox->SetContent( (*g_pGameStringTable)[UI_STRING_MESSAGE_SELECT_QUEST_SLAYER].GetString() );
		} else if( g_pPlayer->IsOusters())
		{
			g_pPCTalkBox->SetContent( (*g_pGameStringTable)[UI_STRING_MESSAGE_SELECT_QUEST_OUSTERS].GetString() );
		}
		else
			return;				// �ƿ콺����� ����
		
		char str[256];
		
		while ( ! pPacket->empty() )
		{
			char timestr[128] = {0,};
			QuestID_t	ID = pPacket->popQuestID();
			
			QUEST_INFO* mkq = g_pQuestInfoManager->GetInfo( ID );
			if(NULL != mkq) 
			{
				DWORD time = mkq->GetTimeLimit();				
				DWORD minute = (time / 60) % 60;
				DWORD hour = (time / 60 / 60);
				bool bContinue = false;
				
				if(hour > 0 || bContinue)
				{
					bContinue = true;
					char temp[100];
					sprintf(temp,(*g_pGameStringTable)[UI_STRING_MESSAGE_HOUR].GetString(), hour );
					strcat(timestr, temp);
					strcat(timestr, " ");
				}
				if(minute > 0 || bContinue)
				{
					bContinue = true;
					char temp[100];
					sprintf(temp,(*g_pGameStringTable)[UI_STRING_MESSAGE_MINUTE].GetString(), minute );
					strcat(timestr, temp);			
				}		
				switch(mkq->GetType() )
				{
				case QUEST_INFO_MONSTER_KILL :
					wsprintf(str,"%4d%s %d%s %s%s",ID,mkq->GetName(), mkq->GetGoal(), (*g_pGameStringTable)[UI_STRING_MESSAGE_NUMBER_OF_ANIMALS].GetString(),(*g_pGameStringTable)[UI_STRING_MESSAGE_TIME_LIMIT].GetString(),timestr);			
					break;			
				default :
					break;
				}		
				
				g_pPCTalkBox->AddString( str );
			}
		}
		
		std::string szMsg;
		
		szMsg += "9999";
		szMsg += (*g_pGameStringTable)[UI_STRING_MESSAGE_CANCEL_SELECT_QUEST].GetString();
		
		g_pPCTalkBox->AddString( szMsg.c_str() );
	}
	g_pUIDialog->PopupPCTalkDlg();
}

void		UI_RunWelcomeMessage()
{
	if(g_pPlayer == NULL)
		return;

	if(g_pPlayer->IsSlayer())
	{
		gC_vs_ui.RunPopupMessage(TXT_TUTORIAL_WELCOME_SLAYER, C_VS_UI_POPUP_MESSAGE::POPUP_WELCOME );
	} else
	if(g_pPlayer->IsVampire())
	{
		gC_vs_ui.RunPopupMessage(TXT_TUTORIAL_WELCOME_VAMPIRE, C_VS_UI_POPUP_MESSAGE::POPUP_WELCOME );
	} else
	if(g_pPlayer->IsOusters())
	{
		gC_vs_ui.RunPopupMessage(TXT_TUTORIAL_WELCOME_OUSTERS, C_VS_UI_POPUP_MESSAGE::POPUP_WELCOME );
	}
}

void		UI_RunQuestStatus( GCQuestStatus * pPacket )
{
	if( pPacket == NULL )
		return;
	
	C_VS_UI_QUEST_STATUS::QUEST_STATUS QuestStatus;
	QUEST_INFO* Quest_Info = g_pQuestInfoManager->GetInfo( pPacket->getQuestID() );
	if(NULL != Quest_Info)
	{
		if( Quest_Info->GetEventQuest() )
		{
			QuestStatus.Title = (*g_pGameStringTable)[UI_STRING_MESSAGE_EVENT_QUEST2_1+Quest_Info->GetQuestLevel()].GetString();
		} else
		{
			QuestStatus.Title			= (*g_pGameStringTable)[UI_STRING_MESSAGE_QUEST_MONSTER_KILL].GetString();
		}
		QuestStatus.QuestID			= pPacket->getQuestID();
		QuestStatus.quest_time		= pPacket->getRemainTime() + (timeGetTime() / 1000);
		QuestStatus.current_point	= pPacket->getCurrentNum();	

		gC_vs_ui.RunQuestStatusWindow();
		gC_vs_ui.SetQuestStatus( QuestStatus );
	}
}

void		UI_InitQuestStatus()
{
	gC_vs_ui.SetQuestStatusInit();
}

void		UI_CloseQuestStatus()
{
	if( gC_vs_ui.IsRunningQuestStatusWindow() )
		gC_vs_ui.CloseQuestStatusWindow();
}

void		UI_RunPopupMessage(int type)
{
}

void		UI_LotteryResult(bool bSuccess)
{
	gC_vs_ui.SetResultLotteryCard( bSuccess );	
}

void		UI_RunLottery(int step)
{
	gC_vs_ui.RunLotteryCard( step );
}

void		UI_RunImageNotice(const char* name, DWORD id )
{
	gC_vs_ui.AddNotice( name, id );
}

void		UI_REQUEST_DIE_TIMER_RESET()
{
	gC_vs_ui.RequestDieTimerReset();
}

void		UI_RunItemShop( GCGoodsList *pPacket )
{
	g_pUIDialog->HidePCTalkDlg();
	g_pPlayer->SetWaitVerify( MPlayer::WAIT_VERIFY_NULL );	
	g_pTempInformation->SetMode( TempInformation::MODE_NULL );


	gC_vs_ui.RunItemListWindow();
	while( !pPacket->isEmpty() )
	{
		GoodsInfo *pInfo = pPacket->popGoodsInfo();
		
		if(pInfo == NULL)
			return;

		MItem *pItem = MItem::NewItem( (ITEM_CLASS)pInfo->itemClass );
		
		pItem->SetID( pInfo->objectID );
		pItem->SetItemType( pInfo->itemType );
		// 2004, 5, 18 sobeit add start
		pItem->SetGrade( pInfo->grade );
		// 2004, 5, 18 sobeit add end
		pItem->SetItemOptionList( pInfo->optionType );
		if( pItem->IsGunItem() )
		{
			MMagazine* pMagazine = (MMagazine*)MItem::NewItem( (ITEM_CLASS)ITEM_CLASS_MAGAZINE );
			pMagazine->SetID( 0 );
			for (int j=0; j<(*g_pItemTable)[ITEM_CLASS_MAGAZINE].GetSize(); j++)			
			{
				pMagazine->SetItemType(	j );
				
				if (pMagazine->IsInsertToItem( pItem ))
				{
					break;
				}
			}			
			// �ǹ� ����
			pMagazine->ClearItemOption();			
			// źâ ����
			pMagazine->SetNumber( pInfo->num );			
			//------------------------------------
			// źâ ����
			//------------------------------------
			MGunItem* pGunItem = (MGunItem*)pItem;
			pGunItem->SetMagazine( pMagazine );
		} else
		{
			pItem->SetNumber( pInfo->num );
		}
		
		if(pInfo->timeLimit > 0)
			pItem->SetQuestFlag();

		C_VS_UI_ITEM_LIST::ItemList Item;
		Item.pItem = pItem;
		Item.TimeLimit = pInfo->timeLimit;		
		gC_vs_ui.AddItemToItemList( Item );
		
		delete pInfo;
		pInfo = NULL;
	}	
}

void		UI_RemoveItemFromItemShop( DWORD id )
{
	if(gC_vs_ui.IsRunningItemListWindow() )
		gC_vs_ui.RemoveItemInItemList( id );
}

void		UI_CloseTransItem()
{
	gC_vs_ui.CloseTransItem();
}

void		UI_OkMixingForge(DWORD parameter, MItem* pItem, MItem* pItem2)
{
	WORD	FirstOption = (parameter & 0xffff0000) >> 16;		// paramter >> 16 �ϵ���-_- ���� 
	WORD	SecondOption = parameter & 0xffff;
	
	if( pItem == NULL || pItem2 == NULL || 
		pItem->IsEmptyItemOptionList() || 
		pItem2->IsEmptyItemOptionList()||
		pItem->GetItemOptionListCount() > 1 ||
		pItem2->GetItemOptionListCount() > 1)
	{
		g_pUIDialog->PopupFreeMessageDlg((*g_pGameStringTable)[STRING_ERROR_ETC_ERROR].GetString() );
		return;
	}

	const std::list<TYPE_ITEM_OPTION>& OptionList1  = pItem->GetItemOptionList();
	const std::list<TYPE_ITEM_OPTION>& OptionList2  = pItem2->GetItemOptionList();

	std::list<TYPE_ITEM_OPTION>::const_iterator itr1 = OptionList1.begin();
	std::list<TYPE_ITEM_OPTION>::const_iterator itr2 = OptionList2.begin();
	
	if( *itr1 != FirstOption || *itr2 != SecondOption )
	{
		g_pUIDialog->PopupFreeMessageDlg((*g_pGameStringTable)[STRING_ERROR_ETC_ERROR].GetString() );
		return;		
	}
	
	// �ɼ� ������ ������.
	
	MItem* pModifyItem = g_pInventory->GetItem( pItem->GetID() );
	if(pModifyItem != NULL)
	{
		pModifyItem->AddItemOption( SecondOption );
	}
	else
	{
		g_pUIDialog->PopupFreeMessageDlg((*g_pGameStringTable)[STRING_ERROR_ETC_ERROR].GetString() );
		return;
	}
	//2004,04,27 sobeit set grade add start
	int nMaxGrade = max(pItem->GetGrade(), pItem2->GetGrade());
	pModifyItem->SetGrade(nMaxGrade);
	//2004,04,27 sobeit set grade add end

	MItem* pRemoveItem = g_pInventory->RemoveItem( pItem2->GetID() );
	SAFE_DELETE( pRemoveItem );
	//delete pRemoveItem;	
		
	PlaySound(SOUND_EVENT_FANFARE);
	gC_vs_ui.CloseMixingForge();	
	g_pUIDialog->PopupFreeMessageDlg( (*g_pGameStringTable)[UI_STRING_MESSAGE_COMPLETE_MERGE_ITEM].GetString() );
	g_pInventory->CheckAffectStatus( pModifyItem );
}

void		UI_CloseMixingForge()
{
	gC_vs_ui.CloseMixingForge();
	
	g_pTempInformation->SetMode( TempInformation::MODE_NULL );
}

void		UI_OkRemoveOption( DWORD parameter, MItem *pItem)
{
	if( pItem == NULL )
	{
		g_pUIDialog->PopupFreeMessageDlg( (*g_pGameStringTable)[STRING_ERROR_ETC_ERROR].GetString() );
		return;
	}
	BYTE OptionType = (BYTE)parameter;
	
	pItem->ClearItemOption();	
	pItem->AddItemOption( OptionType );
	PlaySound(SOUND_XMAS_STAR);
	g_pInventory->CheckAffectStatus( pItem );
	
	g_pUIDialog->PopupFreeMessageDlg((*g_pGameStringTable)[STRING_MESSAGE_SUCCESS_REMOVE_OPTION].GetString());
}

void		UI_SetElementalValue(BYTE type, int value)
{
	switch(type)
	{
	case ITEMTABLE_INFO::ELEMENTAL_TYPE_FIRE:
		g_char_slot_ingame.ElementalFire = value;
		break;

	case ITEMTABLE_INFO::ELEMENTAL_TYPE_WATER:
		g_char_slot_ingame.ElementalWater = value;
		break;
		
	case ITEMTABLE_INFO::ELEMENTAL_TYPE_EARTH:
		g_char_slot_ingame.ElementalEarth = value;
		break;
		
	case ITEMTABLE_INFO::ELEMENTAL_TYPE_WIND:
		g_char_slot_ingame.ElementalWind = value;
		break;		
	}
}

void	UI_RunHorn(int currentZoneID)
{
	if(g_pPlayer->IsOusters())
		gC_vs_ui.RunHorn(currentZoneID);
}

void		UI_MasterLairMessage(BYTE type, short value, TYPE_ZONEID ZoneID)
{
	if( g_pUserOption->DoNotShowLairMsg || g_pSystemMessage == NULL || g_pZoneTable == NULL || g_pGameStringTable == NULL )
		return;	

	MString msg;
	
	switch( type ) 
	{
	case 0 :			// �����ͷ�� ���Ƚ��ϴ�.
		msg.Format((*g_pGameStringTable)[STRING_MESSAGE_OPEN_LAIR].GetString(), g_pZoneTable->Get( ZoneID )->Name.GetString() );
		break;
	case 1 :			// �����ͷ�� �������ϴ�.
		msg.Format((*g_pGameStringTable)[STRING_MESSAGE_CLOSED_LAIR].GetString(), g_pZoneTable->Get( ZoneID )->Name.GetString() );
		break;
	case 2 :			// �����ͷ����� ���԰��� �ð��� %d �� ���ҽ��ϴ�.
		msg.Format((*g_pGameStringTable)[STRING_MESSAGE_LEFT_TIME_LAIR].GetString(), g_pZoneTable->Get( ZoneID )->Name.GetString() , value );
		break;
	default :
		return;
	}
	//O/X������������ �����ͷ��� �޼����� .. �����Ѵ�.
	if (g_pZone->GetID() == 1005)
		return;

	g_pSystemMessage->Add( msg.GetString() );
}

void		UI_PopupMessage(int gamestringID, int x, int y, WORD fButton, bool IsAfterExit)
{
	if( gamestringID < 0 || gamestringID >= MAX_GAME_STRING || g_pUIDialog == NULL || g_pGameStringTable == NULL)
		return;
	
	g_pUIDialog->PopupFreeMessageDlg((*g_pGameStringTable)[gamestringID].GetString(), x, y, fButton, IsAfterExit);
}

void		UI_DownSkill()
{
	g_pUIDialog->ClosePCTalkDlg();
	gC_vs_ui.CloseAllDialog();
	gC_vs_ui.HotKey_SkillInfo();
	gC_vs_ui.SetOustersSkillDown();
}

void		UI_RunMinigame(int game)
{

	switch(game)
	{
	case 0:
		gC_vs_ui.RunFindingMine();
		break;

	case 1:
//		gC_vs_ui.NemoNemo();
		break;
	
	case 2:
//		gC_vs_ui.PushPush();
		break;
	}
}

void		UI_MiniGameScores(GCMiniGameScores* pPacket)
{
	BYTE Type = pPacket->getGameType();
	BYTE Level = pPacket->getLevel();	
	int Size = pPacket->getSize();

	// Size �� 0�̸� �ƹ��� ���Ѱ�.
	// 1�̸� top score �� �ִ°�	
	
	switch(Type)
	{
	case GAME_MINE :
		if( Level == 0 )
			gC_vs_ui.RunCrazyMine();
		
		break;
	case GAME_ARROW :
		if( Level == 0 )
			gC_vs_ui.RunArrowTile();
		break;
	}

	std::string name = "";
	WORD score = 0;

	std::pair<std::string,WORD> scores ,score2;

	if( Size > 0 )
		scores = pPacket->popScore();
	switch( Size )
	{
	case 0 :
		gC_vs_ui.SetMiniGameScore( Type, name, score, score);
		break;
	case 1 :
		gC_vs_ui.SetMiniGameScore( Type, scores.first, scores.second, score );
		break;
	case 2 : 
		score2 = pPacket->popScore();
		gC_vs_ui.SetMiniGameScore( Type, scores.first, scores.second, score2.second);
		break;
	}	
}

void		UI_SetCTFStatus( DWORD endTime, int flag_s, int flag_v, int flag_o )
{
	gC_vs_ui.SetCTFStatus( endTime, flag_s, flag_v, flag_o );
}

void		UI_CloseCTFStatusWindow()
{
	gC_vs_ui.CloseCTFStatusWindow();
}

void		UI_SetDelayEternity()
{
	gC_vs_ui.SetDelayRequestResurrect( C_VS_UI_REQUEST_RESURRECT::RESURRECT_MODE_ETERNITY, 2000 );
}

void		UI_CloseRequestResurrectWindow()
{
	gC_vs_ui.FinishRequestResurrect();
}

void		UI_SetGuildName(WORD guildID, const std::string& guildName)
{
	DEBUG_ADD_FORMAT("[SetGuildName] %d %s", guildID, guildName.c_str() );

	if( !guildName.empty() )
		g_pGuildInfoMapper->SetGuildName( guildID, guildName );
}

void		UI_RefreshInfoImage()
{
	gC_vs_ui.RefreshInfoImage();
}

int			GetMyLevelWarStair()
{
	int MyStat;
	if( g_pPlayer->IsSlayer() )
		MyStat = g_char_slot_ingame.STR_PURE + g_char_slot_ingame.DEX_PURE + g_char_slot_ingame.INT_PURE;
	else
		MyStat = g_pPlayer->GetLEVEL();

	switch( g_pPlayer->GetRace() )
	{
	case RACE_SLAYER :
		if( MyStat >= 0 && MyStat <= 150 )
			return 1;
		else if (MyStat <= 210 )
			return 2;
		else if (MyStat <= 260 )
			return 3;
		else if (MyStat <= 300 )
			return 4;	
		break;
	
	case RACE_VAMPIRE :
	case RACE_OUSTERS :
		if( MyStat >= 1 && MyStat <= 30 )
			return 1;
		else if ( MyStat <= 50 )
			return 2;
		else if (MyStat <= 70 )
			return 3;
		else if (MyStat <= 90 )
			return 4;
		break;
	}

	return 0;
}

void
UI_RunNotice(DWORD sendID, DWORD parameter)
{
	std::string FileName;

	SIZE windowSize = { 0, 0 };
	bool bOpen = false;
	char szDate[128];
	switch(sendID)
	{
	// �ʺ��� �޼���
	case 1:
		FileName = "BeginnerZone";
//		bOpen = true;
		break;

	// START CTF
	case 2:
		FileName = "StartCTF";
		bOpen = true;
		break;

	// ���� ���� ����
	case 3:
		if(parameter/100000000 != GetMyLevelWarStair())
			return;
		parameter %= 100000000;
		FileName = "LevelWar";
		FileName += '0'+GetMyLevelWarStair();
		break;
	
	// ���� ���� ����
	case 4:
		if(parameter/100000000 != GetMyLevelWarStair())
			return;
		parameter %= 100000000;
		FileName = "StartLevelWar";
		FileName += '0'+GetMyLevelWarStair();
		bOpen = true;
		break;

	// ���� ���� ����
	case 5:
		FileName = "RaceWar";
		break;

	// ���� ���� ����
	case 6:
		FileName = "StartRaceWar";
		bOpen = true;
		break;

	// 2�� �� ����Ʈ
	case 7:
	{
		// today, not the game world's calendar, like the notices the server dates
		SYSTEMTIME now;
		GetLocalTime(&now);
		sprintf(szDate, "%02d%02d%02d", now.wYear % 100, now.wMonth, now.wDay);
		parameter = (atoi(szDate))*100;
		FileName = "2ndPetQuest";
		break;
	}
	case 8: 
		gC_vs_ui.RunSMSMessage();
		return;
	}

	if(sendID != 2)
	{
		switch( g_pPlayer->GetRace() )
		{
		case RACE_SLAYER :	
			FileName += "Slayer.txt" ;	
			break;

		case RACE_VAMPIRE :	
			FileName += "Vampire.txt";	
			break;

		case RACE_OUSTERS :	
			FileName += "Ousters.txt";	
			break;
		}
	}
	else
	{
		FileName += ".txt";
	}

//	style.Type = C_VS_UI_POPUP_MESSAGE::POPUP_NORMAL;	
//	style.SendID = 2;

	CRarFile PackFile;
	char szLine[4096];
		
	PackFile.SetRAR( RPK_TUTORIAL_ETC, RPK_PASSWORD );
	std::string sender, title, contents;
	if( PackFile.IsSet() )
	{
		// no letter when its text is not in the archive
		if( !PackFile.Open( FileName.c_str() ) )
		{
			PackFile.Release();
			return;
		}
		ZeroMemory( szLine, 4096 );
		PackFile.GetString(szLine, 4096);
		sscanf(szLine, "%d %d", &windowSize.cx, &windowSize.cy);
		ZeroMemory( szLine, 4096 );
		PackFile.GetString(szLine, 4096);
		sender = szLine;
		ZeroMemory( szLine, 4096 );
		PackFile.GetString(szLine, 4096);
		title = szLine;
		ZeroMemory( szLine, 4096 );
		PackFile.GetString(szLine, 4096);
		contents = szLine;
		
		PackFile.Release();
	}
	else return;
	

	gC_vs_ui.AddMail(0, sendID, windowSize, sender.c_str(), title.c_str(), parameter, bOpen);
	gC_vs_ui.AddMailContents(0, sendID, contents.c_str());
//	gC_vs_ui.AddMail(0, 2, sender, title, 030000, windowSize);
//	gC_vs_ui.AddMailContents(0, 2, contents, true);
//	gC_vs_ui.RunPopupMessage( style );
}

void
UI_DeleteNotice(DWORD sendID)
{
	gC_vs_ui.DeleteMail(0, sendID);
}

void
UI_RunPetInfo(struct PETINFO *pPetInfo)
{
	gC_vs_ui.RunPetInfo(pPetInfo);
}

void 
UI_SetSMSList(void* pVoid)
{
	gC_vs_ui.SetSMSAddressList(pVoid);

}

void 
UI_AddSMSList(DWORD element, char* name, char* id, char* num)
{
	gC_vs_ui.AddSMSAddressList(element,name, id, num);
}

void 
UI_DeleteSMSList(DWORD id)
{
	gC_vs_ui.DeleteSMSAddressList(id);

}
void 
UI_SMS_Set_Charge(int i)
{
	gC_vs_ui.SetSMSCharage(i);
}
void 
UI_SMS_ERROR(DWORD param)
{
	switch(param)
	{
	case 1:
		UI_PopupMessage(UI_STRING_MESSAGE_SMS_FAIL_MAX_NUM_EXCEEDED);
		break;
	case 2:
		UI_PopupMessage(UI_STRING_MESSAGE_SMS_FAIL_INVALID_DATA);
		break;
	case 3:
		UI_PopupMessage(UI_STRING_MESSAGE_SMS_FAIL_NO_SUCH_EID);
		break;
	case 4:
		UI_PopupMessage(UI_STRING_MESSAGE_SMS_FAIL_NOT_ENOUGH_CHARGE);
		break;

	default:
		UI_PopupMessage(STRING_ERROR_ETC_ERROR);
		break;
	}
}
void 
UI_RunModifyTax(int	TexRate)
{
	// NPC��ȭ�ϴ� dialog�� �����
	g_pUIDialog->HidePCTalkDlg();

	//gC_vs_ui.RunModifyTax();

	gC_vs_ui.RunTaxRate(TexRate)	;
}


void 
UI_RunRangerChat()
{
	gC_vs_ui.RunRangerChat();
}
void 
UI_CloseRangerChat()
{
	gC_vs_ui.CloseRangerChat();
}

void		
UI_SetRangerChatString(char* str)
{
	gC_vs_ui.SetRangerChatString(str);
}

bool UI_IsMonsterKillQuest_Monster(DWORD nID)
{
	QUEST_INFO* Quest_Info = g_pQuestInfoManager->GetInfo( gC_vs_ui.GetQuestStatusID() );
	
	if(NULL != Quest_Info)
	{
		if( Quest_Info->GetType() == QUEST_INFO_MONSTER_KILL && Quest_Info->GetSpriteType() == nID)
		{
			return true;
		}
	}
	return false;
}

void UI_ShowTargetArrow(int x, int y)
{
	gC_vs_ui.DrawTargetArrow(x,y);
}

// 2005, 1, 3, sobeit add start - ���� ������ ��ȯ
void
UI_Show_Swap_Advancement_Item()
{
	// NPC��ȭ�ϴ� dialog�� ����� 
	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.OpenInventoryToSwapAdvanceItem();
}
bool		UI_IsRunningSwapAdvancementItem()
{
	return gC_vs_ui.IsRunningSwapAdvancementItem();
}
// 2005, 1, 3, sobeit add end

// 2005, 1, 11, sobeit add start - �ҿ� �̿����� ���� â
void 
UI_Run_Campaign_Help_Unfortunate_Neighbors(int value)
{
	// NPC��ȭ�ϴ� dialog�� �����
//	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.Run_Campaign_Help_Unfortunate_Neighbors(value);
}
// 2005, 1, 11, sobeit add end

// 2005, 1, 24, sobeit add start - �̺�Ʈ ������ �ޱ�


// 2005, 4, 21, sjheon add start - ��ȥ���Ǳ�  â
void 
UI_Run_Wedding_Contribution_Unfortunate_Neighbors(int value)
{
	// NPC��ȭ�ϴ� dialog�� �����
//	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.Run_Wedding_Contribution_Unfortunate_Neighbors(value);
}
// 2005, 4, 21, sjheon add end - ��ȥ���Ǳ� â


void 
UI_Run_Confirm_GetEventItem(int value)
{
	// NPC��ȭ�ϴ� dialog�� �����
//	g_pUIDialog->HidePCTalkDlg();

	gC_vs_ui.Run_Confirm_GetItemEvent(value);
}
// 2005, 1, 11, sobeit add end

void 
UI_Run_WebBrowser(char* szURL)
{
	if(UI_IsRunning_WebBrowser())
	{
		UI_Close_WebBrowser();
	
	}

	gC_vs_ui.RunWebBrowser(g_hWnd, szURL, g_hInstance) ; //(void*)g_pWebBrowser);
	//gC_vs_ui.RunWebBrowser(g_hWnd, szURL, (void*)g_x);
	if(gC_vs_ui.IsRunningWebBrowser())
	{
//		CDirectDraw::CreateClipper();
	//	SetCursor(LoadCursor(NULL, IDC_ARROW));

//	
//		CURSORINFO _CursorInfo;
//		GetCursorInfo(&_CursorInfo);
//		if(_CursorInfo.flags != CURSOR_SHOWING)
//		{
//		}
		if(gC_vs_ui.IsInRectPointWebBrowser(g_x, g_y))
		{
			UI_ShowWindowCursor();
		}
	}
}
bool
UI_IsRunning_WebBrowser()
{
	return gC_vs_ui.IsRunningWebBrowser();
}
int
UI_GetMouseCursorInfo_WebBrowser()
{
	return gC_vs_ui.GetMouseCursorInfo();
}
void 
UI_Close_WebBrowser()
{
	if(gC_vs_ui.IsRunningWebBrowser())
	{
		if(gC_vs_ui.IsInRectPointWebBrowser(g_x, g_y))
		{
			UI_HiddenWindowCursor();
		}

		gC_vs_ui.CloseWebBrowser();

		SetFocus(g_hWnd);
	}
}

void 
UI_ShowWindowCursor()
{
	// ���������� ó�� ���� ��� TempCount ���� 0�� ���´�.
	int TempCount = ShowCursor( TRUE );
		
	// Ȥ�ö� ������ �� Cursor ���� ���� ����. ShowCursor�� ���� ���������� �ߺ��Ǳ� ����..
	if(TempCount != 0)
	{
		int MaxChange = 10;
		if(TempCount > 0)
		{
			while(TempCount != 0 && MaxChange-- > 0)
			{
				TempCount = ShowCursor( FALSE );
			}
		}
		else if(TempCount < 0)
		{
			while(TempCount != 0 && MaxChange-- > 0)
			{
				TempCount = ShowCursor( TRUE );
			}
		}
	}
}

void 
UI_HiddenWindowCursor()
{
	// ���������� ó�� ���� ��� TempCount ���� -1�� ���´�.
	int TempCount = ShowCursor( FALSE );

	// Ȥ�ö� ������ �� Cursor ���� ���� ����. ShowCursor�� ���� ���������� �ߺ��Ǳ� ����..
	if(TempCount != -1)
	{
		int MaxChange = 10;
		if(TempCount > -1)
		{
			while(TempCount != -1 && MaxChange-- > 0)
			{
				TempCount = ShowCursor( FALSE );
			}
		}
		else if(TempCount < -1)
		{
			while(TempCount != -1 && MaxChange-- > 0)
			{
				TempCount = ShowCursor( TRUE );
			}
		}
	}
}

void 
UI_RunSubInventory(MItem* pItem)
{
//	if(gC_vs_ui.IsRunningSubInventory())
// 		gC_vs_ui.CloseSubInventory();
	gC_vs_ui.RunSubInventory(pItem);
}

void 
UI_CloseSubInventory(TYPE_OBJECTID subInventory)
{
	gC_vs_ui.CloseSubInventory(subInventory);
}

void 
UI_CloseAllSubInventory()
{
	gC_vs_ui.CloseAllSubInventory();
}

void 
UI_ClearSubInventory()
{
	gC_vs_ui.ClearSubInventory();
}

bool 
UI_AddItemToSubInventory(TYPE_OBJECTID subInventory, MItem *pItem, int X, int Y)
{
	return gC_vs_ui.AddItemToSubInventory(subInventory, pItem, X, Y);
}

bool 
UI_IsRunAskUseItemDialog()
{
	return gC_vs_ui.IsRunAskUseItemDialog();
}

void
UI_RunAskUseItemDialog(int AskType, int value, void* pValue)
{
	gC_vs_ui.RunAskUseItemDialog(AskType, value, pValue);
}

void
UI_SetUseAskIitemRunning(bool bRunning)
{
	gC_vs_ui.SetUseAskIitemRunning(bRunning);
}


void
UI_PopupSummerComBack(int	value)
{
	gC_vs_ui.PopupSummerComBack(value);
}

void
UI_SetBloodBurstSetGage()
{
//	gC_vs_ui.SetGage() ; 

}


void
UI_SetMagicRemainSec(int MagicRemainSec)
{
	g_char_slot_ingame.MagicRemainSec = MagicRemainSec;
	g_char_slot_ingame.MagicRemainTick  =GetTickCount() ; 

}


void
UI_BLOODBURST_ATTACKGAGEFULL()
{
	if(gC_vs_ui.GetBloodBurstAttackGageFull())
		gpC_base->SendMessage(UI_BLOOD_BURST ,  1) ;
}

void
UI_BLOODBURST_DEFENSEGAGEFULL()
{
	if(gC_vs_ui.GetBloodBurstDefenseGageFull())
		gpC_base->SendMessage(UI_BLOOD_BURST ,  2) ;

}
void
UI_BLOODBURST_PARTYGAGEFULL()
{
	if(gC_vs_ui.GetBloodBurstPartyGageFull())
		gpC_base->SendMessage(UI_BLOOD_BURST ,  3) ;
}


void
UI_RunAskCheckToEnterServerDialog(int AskType , int value)
{
	gC_vs_ui.RunAskCheckToEnterServerDialog(AskType , value);
}


void
UI_RunTimeCount(DWORD sec, LPCTSTR desc)
{
	gC_vs_ui.RunTimeCount(sec, desc);
}

void
UI_CloseTimeCount()
{
	gC_vs_ui.CloseTimeCount();
}

void
UI_OKPetMixingForge(MItem* pItem)
{
#if __CONTENTS(__PET_MIXINGFORGE)
	gC_vs_ui.ClosePetMixingForge();
	MItem* pRemoveItem = g_pInventory->RemoveItem( pItem->GetID() );
	SAFE_DELETE( pRemoveItem );
	PlaySound(SOUND_EVENT_FANFARE);
	g_pUIDialog->PopupFreeMessageDlg( (*g_pGameStringTable)[UI_STRING_MESSAGE_COMPLETE_MERGE_ITEM].GetString() );
#endif //__PET_MIXINGFORGE
}

void
UI_ClosePetMixingForge()
{
#if __CONTENTS(__PET_MIXINGFORGE)
	gC_vs_ui.ClosePetMixingForge();
#endif //__PET_MIXINGFORGE
}

#if __CONTENTS(__FRIEND_ADDITION)
/*
	�ŷ� ��û ��û
*/
void execf_trade_to_other_player(void* void_ptr)
{
	TYPE_OBJECTID* id = (TYPE_OBJECTID*)void_ptr;
	g_pTopView->SetRequestMode( MTopView::REQUEST_TRADE );
	g_pPlayer->SetTraceID( *id );
	g_pPlayer->SetRequestMode( g_pTopView->GetRequestMode() );
	g_pPlayer->RequestModeExec();
	g_pPlayer->SetTraceID( NULL );
}

/*
	��Ƽ ��û ��û
*/
void execf_party_req_to_player(void* void_ptr)
{
	TYPE_OBJECTID* id = (TYPE_OBJECTID*)void_ptr;
	g_pTopView->SetRequestMode( MTopView::REQUEST_PARTY );
	g_pPlayer->SetTraceID( *id );
	g_pPlayer->SetRequestMode( g_pTopView->GetRequestMode() );
	g_pPlayer->RequestModeExec();
	g_pPlayer->SetTraceID( NULL );
}

/*
	ģ�� �߰�â
*/
void execf_add_friend(void* void_ptr)
{
	C_VS_UI_FRINEND_ADD_IS_PLAYER* pFriend = (C_VS_UI_FRINEND_ADD_IS_PLAYER*)gpC_window_manager->GetWindowbyName("FriendAddIsPlayer");
	if (pFriend)
	{
		if (pFriend->Running())
		{
			pFriend->Finish();
			DeleteNew(pFriend);
		}
		else
		{
			DeleteNew(pFriend);
		}
	}

	pFriend = new C_VS_UI_FRINEND_ADD_IS_PLAYER((char*)void_ptr);
	pFriend->SetWindowName("FriendAddIsPlayer");
	pFriend->Start();
}

/*
	ģ�� ���� ��û
*/
void execf_character_req_info(void* void_ptr)
{
	TYPE_OBJECTID* id = (TYPE_OBJECTID*)void_ptr;
	g_pTopView->SetRequestMode( MTopView::REQUEST_INFO );
	g_pPlayer->SetTraceID( *id );
	g_pPlayer->SetRequestMode( g_pTopView->GetRequestMode() );
	g_pPlayer->RequestModeExec();
	g_pPlayer->SetTraceID( NULL );
}

void UI_GeneratePlayerPopupMenu(int _x, int _y, TYPE_OBJECTID id)
{
	std::string wnd_name = "PlayerPopupMenu";
	C_VS_UI_CHARACTER_POPUP_MENU* pPlayerPopup = (C_VS_UI_CHARACTER_POPUP_MENU*)gpC_window_manager->GetWindowbyName(wnd_name.c_str());
	if (pPlayerPopup)
	{
		if (pPlayerPopup->Running())
		{
			pPlayerPopup->Finish();
			DeleteNew(pPlayerPopup);
		}
		else
		{
			DeleteNew(pPlayerPopup);
		}
	}

	pPlayerPopup = new C_VS_UI_CHARACTER_POPUP_MENU(_x+2,_y+2,id);
	pPlayerPopup->SetWindowName(wnd_name.c_str());

	// Use spk image
	C_SPRITE_PACK* spk = new C_SPRITE_PACK(SPK_PLAYER_POPUP);
	pPlayerPopup->SetSpkImage(spk,4);
	pPlayerPopup->AddCommand("�ŷ���û",0,execf_trade_to_other_player);
	pPlayerPopup->AddCommand("��Ƽ��û",1,execf_party_req_to_player);
	pPlayerPopup->AddCommand("ģ����û",2,execf_add_friend);
	pPlayerPopup->AddCommand("��������",3,execf_character_req_info);
	pPlayerPopup->Start();
}

void UI_ReleasePlayerPopupMenu(int _x, int _y)
{
	std::string wnd_name = "PlayerPopupMenu";
	C_VS_UI_CHARACTER_POPUP_MENU* pPlayerPopup = (C_VS_UI_CHARACTER_POPUP_MENU*)gpC_window_manager->GetWindowbyName(wnd_name.c_str());
	if (pPlayerPopup)
	{
		if (!pPlayerPopup->IsPixel(_x,_y))
		{
			pPlayerPopup->Finish();
			DeleteNew( pPlayerPopup );
		}
	}
}
#endif //__FRIEND_ADDTION
