//----------------------------------------------------------------------
// MPlayer.cpp
//----------------------------------------------------------------------
//
// m_nUsedActionInfo�� ���� turn�� �ٷ� �����ϰ� �Ǵ� �����̰�
//
// m_nNextUsedActionInfo�� 
//          ������ ������ ������ �����ϰ� �Ǵ� �����̴�.(buffer�� ����)
//
//----------------------------------------------------------------------
#include "Client_PCH.h"
#include <math.h>
#include "MZone.h"
#include "MTopView.h"
#include "MItem.h"
#include "MCreature.h"
#include "MInteractionObject.h"
#include "MLinearEffect.h"
#include "TempInformation.h"
#include "MParabolaEffect.h"
#include "MPlayer.h"
#include "MEffectStatusTable.h"
#include "MActionInfoTable.h"
#include "MEffectTarget.h"
#include "MEffectGeneratorTable.h"
#include "PacketDef.h" 
#include "ClientDef.h"
#include "MCreatureTable.h"
#include "MSoundTable.h"
#include "MItemTable.h"
#include "ClientConfig.h"
#include "MGameStringTable.h"
#include "MStatusManager.h"
#include "MItemOptionTable.h"
#include "TalkBox.h"
#include "UIDialog.h"
#include "MHelpManager.h"
#include "MPortal.h"
#include "PacketFunction.h"
#include "MGameTime.h"
#include "MObjectSelector.h"
#include "SoundDef.h"
#include "MItemFinder.h"
#include "Cpackets\CGSelectTileEffect.h"
#include "Cpackets\CGPartyInvite.h"
#include "EffectSpriteTypeDef.h"
#include "ModifyStatusManager.h"
#include "ClientCommunicationManager.h"
#include "MEventManager.h"
#include "MItemFinder.h"
#include "MZoneTable.h"

#include "Rpackets\RCPositionInfo.h"
#include "Rpackets\RCStatusHP.h"
#include "Rpackets\RCCharacterInfo.h"
#include "Cpackets\CGRequestInfo.h"
#include "Cpackets\CGUseItemFromGear.h"

#include "UserInformation.h"
#include "MParty.h"
#include "MJusticeAttackManager.h"
#include "RequestUserManager.h"
#include "MWarManager.h"

#include "MGuildType.h"
#include "ServerInfo.h"
#include "DebugInfo.h"
#include "MTestDef.h"
#include "RankBonusTable.h"
#include "RankBonusDef.h"
#include "MOustersGear.h"

#include "CPackets/CGAbsorbSoul.h"

#include "UIMessageManager.h"
#include "UIFunction.h"
#include "RequestClientPlayerManager.h"
#include "SystemAvailabilities.h"

#include "UserOption.h"
#include "OperatorOption.h"

#include "VS_UI_GameCommon2.h"

#include "Cpackets\CGPartyPosition.h"

// 2008.3.12 cheon ill zea - ��ް���ġ ��ȭ�� ǥ�ö���
#include "ExperienceTable.h"

extern void UI_RunPetInfo(struct PETINFO* pPetInfo);

//#include "Client.h"
bool		GetMakeItemFitPosition(MItem* pItem, ITEM_CLASS itemClass, int itemType, POINT& fitPoint);

// [�����]
void		SkillShadowDancing(MCreature* pUserCreature, MCreature* pTargetCreature, int skillID);
bool			IsCreatureMove(MCreature* pCreature);
bool			IsCreatureActionAttack(MCreature* pCreature);
bool			HasEffectStatusSummonSylph(MCreature* pCreature);
void			RemoveEffectStatusSummonSylph(MCreature* pCreature);
#if __CONTENTS(__FAST_TRANSFORTER)
bool			HasEffectStatusWingSylph(MCreature* pCreature);
void			RemoveEffectStatusWingSylph(MCreature* pCreature);
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
bool			HasEffectStatusUnicorn(MCreature* pCreature);
void			RemoveEffectStatusUnicorn(MCreature* pCreature);
#endif //__SECOND_TRANSFORTER
BYTE GetCreatureActionCountMax(const MCreature* pCreature, int action);

// #ifndef CONNECT_SERVER
#include "MFakeCreature.h"
// #endif


#include "MTestDef.h"

#ifdef __METROTECH_TEST__
//	#define OUTPUT_DEBUG_PLAYER_ACTION
#endif



#include "VS_UI.h" // KJTINC
#include "VS_UI_Mouse_pointer.h"

extern	DWORD	g_CurrentTime;
extern	int		g_x;
extern	int		g_y;
extern	bool	g_bNetStatusGood;
extern BOOL		g_bLButtonDown;
extern BOOL		g_bRButtonDown;
extern BOOL		g_bCButtonDown;
extern bool		g_bPreviousMove;

extern int		g_nZoneLarge;
extern int		g_nZoneSmall;
extern bool		g_bZonePlayerInLarge;
extern bool		g_bZoneSafe;
extern bool		g_bHolyLand;
extern POINT	g_SelectSector;
extern bool		g_bPlayerSkillSound;


//#define	new			DEBUG_NEW

#define				TRADE_DISTANCE_NPC		4
#define				TRADE_DISTANCE_PC		2


//Test - Skill Cool Time Check Test�Լ�

//SkillType���� .. ������ ��ų�� ���� ���� ��, ���ð� ���̸� SkillDelay.log�� ����� �ش�.
void TestCheckSkillCooltime(int SkillType, int WeaponSpeed)
{

	static DWORD _LastSkillTime = 0;
	static DWORD _LastSkillType = 0;

	if (_LastSkillType != SkillType)
	{
		_LastSkillType = SkillType;
		_LastSkillTime = g_CurrentTime;
	}
	else
	{
		ACTIONINFO_PACKET	packetType = (*g_pActionInfoTable)[SkillType].GetPacketType();		//� ����� ��ų�ΰ�?
		char strSkillInfo[256];
		switch (packetType)
		{
		case ACTIONINFO_PACKET_OTHER:
			wsprintf(strSkillInfo, "Ÿ�ο��� ����ϴ� ��ų");
			break;
		case ACTIONINFO_PACKET_SELF:
			wsprintf(strSkillInfo, "�ڽſ��� ����ϴ� ��ų");
			break;
		case ACTIONINFO_PACKET_ZONE:
			wsprintf(strSkillInfo, "Ÿ�Ͽ��� ����ϴ� ��ų");
			break;
		default:
			wsprintf(strSkillInfo, "�����? %d", packetType);
			break;
		}
		const char* pSkillName = (*g_pActionInfoTable)[SkillType].GetName();						//��ų �̸�
		int	StartFrame = (*g_pActionInfoTable)[SkillType].GetStartFrame(WeaponSpeed);	//���� ������

		std::ofstream file("SkillDelay.log", std::ios::out | std::ios::app);
		file << "*** SkillType = " << SkillType << "[" << pSkillName << "]" << " ***" << strSkillInfo << std::endl;
		file << "     WeaponSpeed=" << WeaponSpeed << "StartFrame =" << StartFrame << std::endl;
		file << "     Difftime=" << g_CurrentTime - _LastSkillTime << std::endl;

		file.close();

		_LastSkillTime = g_CurrentTime;
	}
}

//Test
void	MPlayer::SetWaitVerify(WAIT_VERIFY wv, TYPE_ACTIONINFO ai)
{
	m_WaitVerify = wv; m_WaitVerifyActionInfo = ai;
}
void	MPlayer::SetWaitVerifyNULL()
{
	m_WaitVerify = WAIT_VERIFY_NULL; m_WaitVerifyActionInfo = ACTIONINFO_NULL;
}

//----------------------------------------------------------------------
//
//							Global Player
//
//----------------------------------------------------------------------
MPlayer* g_pPlayer = NULL;

//----------------------------------------------------------------------
// define Functions
//----------------------------------------------------------------------
// �� ��ü�� �Ǵ� ����...
/*
#define GET_DYING_CREATURE( pCreature, id )								\
		if (pCreature==NULL)											\
		{																\
			MItem* pItem = g_pZone->GetItem( id );						\
																		\
			if (pItem!=NULL												\
				&& pItem->GetItemClass()==ITEM_CLASS_CORPSE)		\
			{															\
				pCreature = ((MCorpse*)pItem)->GetCreature();			\
																		\
				if (pCreature->GetActionCount() >= pCreature->GetActionCountMax())	\
				{														\
					pCreature = NULL;									\
				}														\
			}															\
		}
*/

//----------------------------------------------------------------------
// CHECK_REPEAT_LIMIT_TO_RETURN_FALSE
//----------------------------------------------------------------------
#define CHECK_REPEAT_LIMIT_TO_RETURN_FALSE(nActionInfo, repeatCount)		\
		if (nActionInfo < g_pActionInfoTable->GetSize())	\
		{													\
			if (repeatCount >= (*g_pActionInfoTable)[nActionInfo].GetRepeatLimit())	\
			{												\
				UnSetRepeatAction();						\
				return false;								\
			}												\
		}

//----------------------------------------------------------------------
// ���� �ڵ� �Ա�
//----------------------------------------------------------------------
#ifdef __METROTECH_TEST__

int  g_iAutoHealPotion = 1;
bool g_bAutoManaPotion = true;
int	 g_iAutoReload = 1;
bool g_bLight = false;
int	 g_iSpeed = 0;

void
AutoUseSkill(int skill)
{
	if (g_pSkillInfoTable != NULL
		&& g_pInventory != NULL
		&& g_pPlayer != NULL
		&& g_pPlayer->IsSlayer()
		&& g_pPlayer->IsItemCheckBufferNULL()
		&& !g_pPlayer->IsInSafeSector())
	{
		bool bUse = false;
		int skill_id = ACTIONINFO_NULL;

		if (skill == 0)	//healing
		{
			const int hp_gap = g_pPlayer->GetMAX_HP() - g_pPlayer->GetHP();

			if (hp_gap > 0)
			{
				if (g_iAutoHealPotion == 2)	// ������ �޸� �ܸ�ŭ ä���
				{
					if (g_pSkillAvailable->IsEnableSkill(MAGIC_CURE_CRITICAL_WOUNDS) &&
						(*g_pSkillInfoTable)[MAGIC_CURE_CRITICAL_WOUNDS].IsEnable() &&
						(*g_pSkillInfoTable)[MAGIC_CURE_CRITICAL_WOUNDS].GetExpLevel() / 2 + 30 <= hp_gap)
					{
						skill_id = MAGIC_CURE_CRITICAL_WOUNDS;
						bUse = true;
					}
					else
						if (g_pSkillAvailable->IsEnableSkill(MAGIC_CURE_SERIOUS_WOUNDS) &&
							(*g_pSkillInfoTable)[MAGIC_CURE_SERIOUS_WOUNDS].IsEnable() &&
							(*g_pSkillInfoTable)[MAGIC_CURE_SERIOUS_WOUNDS].GetExpLevel() / 4 + 30 <= hp_gap)
						{
							skill_id = MAGIC_CURE_SERIOUS_WOUNDS;
							bUse = true;
						}
						else
							if (g_pSkillAvailable->IsEnableSkill(MAGIC_CURE_LIGHT_WOUNDS) &&
								(*g_pSkillInfoTable)[MAGIC_CURE_LIGHT_WOUNDS].IsEnable() &&
								(*g_pSkillInfoTable)[MAGIC_CURE_LIGHT_WOUNDS].GetExpLevel() / 4 + 10 <= hp_gap)
							{
								skill_id = MAGIC_CURE_LIGHT_WOUNDS;
								bUse = true;
							}
				}
				else if (g_iAutoHealPotion == 3)	// ������ �޸� ä���
				{
					if (g_pSkillAvailable->IsEnableSkill(MAGIC_CURE_CRITICAL_WOUNDS) &&
						(*g_pSkillInfoTable)[MAGIC_CURE_CRITICAL_WOUNDS].IsEnable())
					{
						skill_id = MAGIC_CURE_CRITICAL_WOUNDS;
						bUse = true;
					}
					else
						if (g_pSkillAvailable->IsEnableSkill(MAGIC_CURE_SERIOUS_WOUNDS) &&
							(*g_pSkillInfoTable)[MAGIC_CURE_SERIOUS_WOUNDS].IsEnable())
						{
							skill_id = MAGIC_CURE_SERIOUS_WOUNDS;
							bUse = true;
						}
						else
							if (g_pSkillAvailable->IsEnableSkill(MAGIC_CURE_LIGHT_WOUNDS) &&
								(*g_pSkillInfoTable)[MAGIC_CURE_LIGHT_WOUNDS].IsEnable())
							{
								skill_id = MAGIC_CURE_LIGHT_WOUNDS;
								bUse = true;
							}
				}
			}
		}

		if (skill == 1)	// create holy water
		{
			if (g_pSkillAvailable->IsEnableSkill(MAGIC_CREATE_HOLY_WATER) &&
				(*g_pSkillInfoTable)[MAGIC_CREATE_HOLY_WATER].IsEnable() &&
				(*g_pSkillInfoTable)[MAGIC_CREATE_HOLY_WATER].IsAvailableTime())
			{
				skill_id = MAGIC_CREATE_HOLY_WATER;
				bUse = true;
			}
		}

		if (bUse == true && skill_id != ACTIONINFO_NULL)
		{
			// ��ų���
			g_pPlayer->UnSetRequestMode();

			TYPE_ACTIONINFO old_special = g_pPlayer->GetSpecialActionInfo();
			g_pPlayer->SetSpecialActionInfo(skill_id);
			g_pPlayer->SelfSpecialAction();
			g_pPlayer->SetSpecialActionInfo(old_special);

			//				g_bPreviousMove = false;
		}
	}
}

//------------------------------------------------------------
// Healing �Ա�
//------------------------------------------------------------
void
AutoUsePotion(int bHeal)	// 0:heal 1:mana 2:magazine
{
	if (g_pInventory != NULL
		&& g_pPlayer != NULL
		&& !g_pPlayer->IsVampire()
		&& g_pPlayer->IsItemCheckBufferNULL()
		&& !gbl_item_lock)
	{
		// Healing
		int badClass = -1;
		int badType = -1;

		int currentValue;
		int maxValue;
		float modifier = 1.0f;

		switch (bHeal)
		{
		case 0:	// heal
			currentValue = g_pPlayer->GetHP();
			maxValue = g_pPlayer->GetMAX_HP();
			break;

		case 1: // mana
			currentValue = g_pPlayer->GetMP();
			maxValue = g_pPlayer->GetMAX_MP();

			modifier = 1.0f + g_pPlayer->GetINT() / 300.0f;
			break;

		case 2: // magazine
		case 3: // magazine
		case 4: // magazien
		case 5: // magazine
			if (g_pCurrentMagazine == NULL)
				return;

			currentValue = g_pCurrentMagazine->GetNumber();
			maxValue = 1;
			break;
		}

		if (maxValue <= 0) return;

		int lost = maxValue - currentValue;
		int percent = currentValue * 100 / maxValue;

		bool forceUse = false;

		ITEM_CLASS itemClass = ITEM_CLASS_POTION;
		if (g_pPlayer->IsOusters())
			itemClass = ITEM_CLASS_PUPA;

		int itemType;
		int maxItemType = 4;

		if (bHeal >= 2)
		{
			if (g_pSlayerGear != NULL && !g_pSlayerGear->GetItem(MSlayerGear::GEAR_SLAYER_LEFTHAND)->IsGunItem())return;
			//				badType = (g_pSlayerGear->GetItem(MSlayerGear::GEAR_SLAYER_LEFTHAND)->GetItemClass()-ITEM_CLASS_SG)*2-1;
			badType = (g_pCurrentMagazine->GetItemType() & ~1) - 1;
			itemClass = ITEM_CLASS_MAGAZINE;

			if (currentValue > bHeal - 2)
				return;

			itemType = badType + 2;
		}
		else
		{
			if (lost >= 200.0f * modifier)		itemType = 4;
			else if (lost >= 100.0f * modifier)	itemType = 3;
			else if (lost >= 51.0f * modifier)	itemType = 2;
			else if (lost >= 24.0f * modifier)	itemType = 1;
			else if (lost >= 10.0f * modifier)	itemType = 0;
			else								itemType = badType;
		}

		switch (bHeal)
		{
		case 0:	// Heal
			// HP�� 50% �����϶� �Դ´�.
			forceUse = percent <= 50;
			break;

		case 1:	// Mana
			badType += 5;
			itemType += 5;
			maxItemType += 5;

			// Mana�� ���� ������� ����� ���ؼ� �Դ´�.
			if (g_pSkillInfoTable != NULL)
			{
				int skill = g_pPlayer->GetSpecialActionInfo();

				if (skill < g_pSkillInfoTable->GetSize())
				{
					forceUse = currentValue < 5
						|| (*g_pSkillInfoTable)[skill].GetMP() * 2 >= currentValue;
				}
			}
			break;

		}

		if (itemType != badType && itemClass != badClass)
		{
			MItem* pItem = NULL;
			int type = itemType;

			do
			{
				pItem = g_pInventory->FindItem(itemClass, type);

				// reload�϶��� �����Ѿ��� ã�ƺ���
				if (bHeal >= 2 && pItem == NULL) pItem = g_pInventory->FindItem(itemClass, type + 8);

				if (pItem != NULL) break;
			} while (--type > badType);

			// �� percent �����϶��� �׳� �Ծ�� �Ѵ�. �Ѿ��� ����
			if (bHeal != 2 && pItem == NULL && forceUse)
			{
				type = badType + 1;

				do
				{
					pItem = g_pInventory->FindItem(ITEM_CLASS_POTION, type);

					if (pItem != NULL) break;
				} while (++type <= maxItemType);
			}

			if (pItem != NULL && g_pPlayer->IsItemCheckBufferNULL())
			{
				if (bHeal >= 2)
				{
					CGReloadFromInventory _CGReloadFromInventory;
					_CGReloadFromInventory.setObjectID(pItem->GetID());
					_CGReloadFromInventory.setX(pItem->GetGridX());
					_CGReloadFromInventory.setY(pItem->GetGridY());

					g_pSocket->sendPacket(&_CGReloadFromInventory);
				}
				else
				{
					CGUsePotionFromInventory _CGUsePotionFromInventory;
					_CGUsePotionFromInventory.setObjectID(pItem->GetID());
					_CGUsePotionFromInventory.setX(pItem->GetGridX());
					_CGUsePotionFromInventory.setY(pItem->GetGridY());

					g_pSocket->sendPacket(&_CGUsePotionFromInventory);
				}

				g_pPlayer->SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_USE_FROM_INVENTORY);
			}
			else if (g_pQuickSlot != NULL)
			{
				// ��Ʈ���� ã��
				type = itemType;

				do
				{
					MItemClassTypeFinder itemFinder(itemClass, type);

					pItem = g_pQuickSlot->FindItem(itemFinder);

					// reload�϶��� �����Ѿ��� ã�ƺ���
					if (bHeal >= 2 && pItem == NULL)
					{
						MItemClassTypeFinder itemFinder(itemClass, type + 8);

						pItem = g_pQuickSlot->FindItem(itemFinder);
					}

					if (pItem != NULL) break;
				} while (--type > badType);

				// �� percent �����϶��� �׳� �Ծ�� �Ѵ�. �Ѿ��� ���� ����
				if (bHeal != 2 && pItem == NULL && forceUse)
				{
					type = badType + 1;

					do
					{
						MItemClassTypeFinder itemFinder(ITEM_CLASS_POTION, type);

						pItem = g_pQuickSlot->FindItem(itemFinder);

						if (pItem != NULL) break;
					} while (++type <= maxItemType);
				}

				if (pItem != NULL && g_pPlayer->IsItemCheckBufferNULL())
				{
					if (bHeal >= 2)
					{
						g_pPlayer->SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_USE_FROM_INVENTORY);

						CGReloadFromQuickSlot _CGReloadFromQuickSlot;
						_CGReloadFromQuickSlot.setObjectID(pItem->GetID());
						_CGReloadFromQuickSlot.setSlotID(pItem->GetItemSlot());

						g_pSocket->sendPacket(&_CGReloadFromQuickSlot);
					}
					else
					{
						g_pPlayer->SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_USE_FROM_INVENTORY);

						CGUsePotionFromQuickSlot _CGUsePotionFromQuickSlot;
						_CGUsePotionFromQuickSlot.setObjectID(pItem->GetID());
						_CGUsePotionFromQuickSlot.setSlotID(pItem->GetItemSlot());

						g_pSocket->sendPacket(&_CGUsePotionFromQuickSlot);
					}

					g_pPlayer->SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_USE_FROM_QUICKSLOT);

					UI_LockGear();
				}
			}

		}
	}
}

#endif

bool	CanActionByZoneInfo()
{
	if (g_pZone != NULL)
	{
		ZONETABLE_INFO* pZoneInfo = g_pZoneTable->Get(g_pZone->GetID());

		if (pZoneInfo->CannotAttackInSafe == true)					// �������� �ȿ��� ������ �� ������
		{
			if (g_pZone->GetSector(g_pPlayer->GetX(), g_pPlayer->GetY()).IsSafeComplete())
				return true;
			else
			{
				if (g_pPlayer->IsSlayer() && g_pZone->GetSector(g_pPlayer->GetX(), g_pPlayer->GetY()).IsSafeSlayer())
					return true;
				if (g_pPlayer->IsVampire() && g_pZone->GetSector(g_pPlayer->GetX(), g_pPlayer->GetY()).IsSafeVampire())
					return true;
				if (g_pPlayer->IsOusters() && g_pZone->GetSector(g_pPlayer->GetX(), g_pPlayer->GetY()).IsSafeOusters())
					return true;
			}
		}
	}
	return false;
}

//----------------------------------------------------------------------
// ��Ƽ���鿡�� �� ��ǥ�� ������.
//----------------------------------------------------------------------
void
SendPositionInfoToParty()
{
	//------------------------------------------------------
	// ���������� ��ǥ�� ������ ���
	//------------------------------------------------------		
	if (g_pParty->GetSize() != 0
		&& g_pClientCommunicationManager != NULL)
	{
		static int oldX = -1;
		static int oldY = -1;
		static int oldZoneID = -1;

		int x = g_pPlayer->GetX();
		int y = g_pPlayer->GetY();
		int zoneID = (g_bZonePlayerInLarge ? g_nZoneLarge : g_nZoneSmall);

		// ��ǥ�� �޶������� ������.
		if (oldX != x || oldY != y || oldZoneID != zoneID)
		{
			if (g_pUserInformation->bKorean == true)
			{
				RCPositionInfo _RCPositionInfo;

				// �̸��� �� ������ IP�� üũ�ϱ� ���� �ȴ�.
				//_RCPositionInfo.setName( g_pUserInformation->CharacterID.GetString() );
				_RCPositionInfo.setZoneX(x);
				_RCPositionInfo.setZoneY(y);
				_RCPositionInfo.setZoneID(zoneID);

				// ���� ���� ���
				oldX = x;
				oldY = y;
				oldZoneID = zoneID;

				for (int i = 0; i < g_pParty->GetSize(); i++)
				{
					PARTY_INFO* pInfo = g_pParty->GetMemberInfo(i);

					if (pInfo != NULL
						// �þ߿� ���� ��쿡�� ��ġ ������ ������.
						// �ǹ̰� ��������.. - -;
						//&& !pInfo->bInSight
						)
					{
						int port = g_pClientConfig->CLIENT_COMMUNICATION_UDP_PORT;

						RequestUserInfo* pUserInfo = g_pRequestUserManager->GetUserInfo(pInfo->Name.GetString());

						if (pUserInfo != NULL)
						{
							if (pUserInfo->UDPPort != 0)
								port = pUserInfo->UDPPort;
						}

						g_pClientCommunicationManager->sendPacket(pInfo->IP,
							port,
							&_RCPositionInfo);
					}
				}
			}
			else	// �ѱ�� �ƴ� ���
			{
				CGPartyPosition _CGPartyPosition;

				_CGPartyPosition.setZoneID(zoneID);
				_CGPartyPosition.setXY(x, y);
				_CGPartyPosition.setHP(g_pPlayer->GetMAX_HP(), g_pPlayer->GetHP());

				g_pSocket->sendPacket(&_CGPartyPosition);
			}
		}
	}
}

//----------------------------------------------------------------------
// Send StatusInfo To Party
//----------------------------------------------------------------------
// ��Ƽ���鿡�� �� ���� ������ ������. ����? - -;
//----------------------------------------------------------------------
void
SendStatusInfoToParty()
{
	static DWORD nextTime = g_CurrentTime;

	if (g_CurrentTime >= nextTime)
	{
		//------------------------------------------------------
		// ���������� ��ǥ�� ������ ���
		//------------------------------------------------------		
		if (g_pParty->GetSize() != 0
			&& g_pClientCommunicationManager != NULL)
		{
			static int oldHP = -1;
			static int oldMaxHP = -1;

			int HP = g_pPlayer->GetHP();
			int MaxHP = g_pPlayer->GetMAX_HP();

			// ��ǥ�� �޶������� ������.
			if (oldHP != HP || oldMaxHP != MaxHP)
			{
				if (g_pUserInformation->bKorean == true)
				{
					RCStatusHP _RCStatusHP;

					// �̸��� �� ������ IP�� üũ�ϱ� ���� �ȴ�.
					//_RCStatusHP.setName( g_pUserInformation->CharacterID.GetString() );
					_RCStatusHP.setCurrentHP(HP);
					_RCStatusHP.setMaxHP(MaxHP);

					// ���� ���� ���
					oldHP = HP;
					oldMaxHP = MaxHP;

					for (int i = 0; i < g_pParty->GetSize(); i++)
					{
						PARTY_INFO* pInfo = g_pParty->GetMemberInfo(i);

						if (pInfo != NULL
							// �þ߿� ���� ��쿡�� ���� ������ ������.
							// �ǹ̰� ��������.. - -;
							//&& !pInfo->bInSight
							)
						{
							int port = g_pClientConfig->CLIENT_COMMUNICATION_UDP_PORT;

							RequestUserInfo* pUserInfo = g_pRequestUserManager->GetUserInfo(pInfo->Name.GetString());

							if (pUserInfo != NULL)
							{
								if (pUserInfo->UDPPort != 0)
									port = pUserInfo->UDPPort;
							}

							g_pClientCommunicationManager->sendPacket(pInfo->IP,
								port,
								&_RCStatusHP);
						}
					}
				}
				else // �ѱ�� �ƴ� ���
				{
					int x = g_pPlayer->GetX();
					int y = g_pPlayer->GetY();
					int zoneID = (g_bZonePlayerInLarge ? g_nZoneLarge : g_nZoneSmall);

					CGPartyPosition _CGPartyPosition;

					_CGPartyPosition.setZoneID(zoneID);
					_CGPartyPosition.setXY(x, y);
					_CGPartyPosition.setHP(g_pPlayer->GetMAX_HP(), g_pPlayer->GetHP());

					g_pSocket->sendPacket(&_CGPartyPosition);
				}
			}
		}

		// �þ߿� ���� ����� HP�̱� ������..
		// 5�ʿ� �ѹ� �������ش�.
		nextTime = g_CurrentTime + g_pClientConfig->CLIENT_COMMUNICATION_STATUS_DELAY;
	}
}

//----------------------------------------------------------------------
// Send StatusInfo To Party
//----------------------------------------------------------------------
// ��Ƽ���鿡�� �� ���� ������ ������. ����? - -;
//----------------------------------------------------------------------
void
SendCharacterInfoToParty()
{
	//------------------------------------------------------
	// ���������� ��ǥ�� ������ ���
	//------------------------------------------------------		
	if (g_pParty->GetSize() != 0
		&& g_pClientCommunicationManager != NULL)
	{
		int guildID = g_pPlayer->GetGuildNumber();
		if (guildID > 0)
		{
			RCCharacterInfo _RCCharacterInfo;

			// �̸��� �� ������ IP�� üũ�ϱ� ���� �ȴ�.
			//_RCStatusHP.setName( g_pUserInformation->CharacterID.GetString() );
			_RCCharacterInfo.setGuildID(guildID);

			for (int i = 0; i < g_pParty->GetSize(); i++)
			{
				PARTY_INFO* pInfo = g_pParty->GetMemberInfo(i);

				if (pInfo != NULL
					// �þ߿� ���� ��쿡�� ���� ������ ������.
					// �ǹ̰� ��������.. - -;
					//&& !pInfo->bInSight
					)
				{
					int port = g_pClientConfig->CLIENT_COMMUNICATION_UDP_PORT;

					RequestUserInfo* pUserInfo = g_pRequestUserManager->GetUserInfo(pInfo->Name.GetString());

					if (pUserInfo != NULL)
					{
						if (pUserInfo->UDPPort != 0)
							port = pUserInfo->UDPPort;
					}

					g_pClientCommunicationManager->sendPacket(pInfo->IP,
						port,
						&_RCCharacterInfo);
				}
			}
		}
	}
}

//----------------------------------------------------------------------
// Is Block All Direction
//----------------------------------------------------------------------
extern POINT g_DirectionValue[MAX_DIRECTION];
bool
IsBlockAllDirection()
{
	if (g_pPlayer != NULL && g_pZone != NULL)
	{
		int x = g_pPlayer->GetX();
		int y = g_pPlayer->GetY();

		for (int i = 0; i < 8; i++)
		{
			const POINT& pt = g_DirectionValue[i];

			int sx = x + pt.x;
			int sy = y + pt.y;

			if (g_pZone->CanMove(g_pPlayer->GetMoveType(), sx, sy))
			{
				return false;
			}
		}
	}

	return true;
}

//----------------------------------------------------------------------
// 
// contructor/destructor
//
//----------------------------------------------------------------------
MPlayer::MPlayer()
{
	// ���� ����
	m_fTrace = FLAG_TRACE_NULL;
	m_TraceDistance = 0;
	m_TraceObjectAction = 0;
	m_fTraceBuffer = FLAG_TRACE_NULL;

	m_BasicAttackDistance = 1;

	// ���� ���� ���� 
	m_fNextTrace = FLAG_TRACE_NULL;
	m_NextTraceID = OBJECTID_NULL;
	m_NextTraceX = SECTORPOSITION_NULL;
	m_NextTraceY = SECTORPOSITION_NULL;
	m_NextTraceZ = 0;
	m_NextTraceObjectAction = 0;


	// ������	
	m_DestX = SECTORPOSITION_NULL;
	m_DestY = SECTORPOSITION_NULL;

	// ���� ������
	m_NextDestX = SECTORPOSITION_NULL;
	m_NextDestY = SECTORPOSITION_NULL;

	// �� ���� ��..
	m_BlockDestX = SECTORPOSITION_NULL;
	m_BlockDestY = SECTORPOSITION_NULL;

	// Server�� ���� �޼��� ����
	m_SendMove = 0;

	// Delay�ð�
	m_DelayTime = 0;

	// attack mode
	m_AttackMode = ATTACK_MODE_NORMAL;

	// ��� �ֿ� item
	m_ItemCheckBufferStatus = ITEM_CHECK_BUFFER_NULL;
	m_pItemCheckBuffer = NULL;
	m_dwSubItemIDCheckBuffer = OBJECTID_NULL;

	// ��� ����� ��ǥ
	m_pEffectTarget = NULL;

	// server���� ��ٸ���
	m_WaitVerify = WAIT_VERIFY_NULL;
	m_WaitVerifyActionInfo = ACTIONINFO_NULL;

	// ��� �����ϱ�..
	m_bKeepTraceCreature = true;

	// �ൿ �ݺ�
	m_bRepeatAction = FALSE;

	// �����̾�� ���ϴ� �ð�
	m_ConversionDelayTime = 0;

	m_nNoPacketUsedActionInfo = ACTIONINFO_NULL;

	m_bLockMode = false;

	//-------------------------------------------------------
	// ���� ũ�� �þ�
	//-------------------------------------------------------
	m_TimeLightSight = 0;
	//	m_TimeLightSightX	= SECTORPOSITION_NULL;
	//	m_TimeLightSightY	= SECTORPOSITION_NULL;	
	m_ItemLightSight = 0;
	m_LightSight = 0;

	// ĳ���� �þ�
	m_Sight = 0;
	m_PetDelayTime = 0;

	m_SweepVice_Value = 0;
	m_TempSelectPosX = SECTORPOSITION_NULL;
	m_TempSelectPosY = SECTORPOSITION_NULL;
	m_SweepViceX = SECTORPOSITION_NULL;
	m_SweepViceY = SECTORPOSITION_NULL;
	m_bShowAdamCloud = false;

	m_ResurrectZoneID = 0;

	m_byComboEndValue = 1;
	m_bInvincible = false;

	m_bEnforceResurrect = false;

	m_SkillCastingX = SECTORPOSITION_NULL;
	m_SkillCastingY = SECTORPOSITION_NULL;

	m_BatColorSet = 0xFFFF;
	m_AdvanceBatColorSet = 0xFFFF;
}

MPlayer::~MPlayer()
{
	// ����~~ �ʻ��� �ڵ� - -;;
	// Effect�� delete�Ǹ鼭
	// player�� EffectTarget�� �����ҷ��� g_pPlayer�� �����ϴ� �ٶ���..
	// ������ �� �ٲ�ߵ��ٵ�.. T_T;; ��ĥ �ð���.. - -;	
	g_pUserInformation->bCompetence = false;
	g_pUserInformation->bCompetenceShape = false;
	g_pPlayer = NULL;

	// remove priority queue 
	RemoveNodes();

	// remove list
	m_listDirection.clear();
	m_listSendDirection.clear();

	// ��� ��� ��ǥ
	if (m_pEffectTarget != NULL)
	{
		MEffectTarget* pTempEffectTarget = m_pEffectTarget;
		m_pEffectTarget = NULL;

		delete pTempEffectTarget;
	}
}


//----------------------------------------------------------------------
//
// member functions
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// MPlayer�� �����ϴ� Zone�� �����Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::SetZone(MZone* pZone)
{
	// base class functions
	MCreature::SetZone(pZone);

}

//----------------------------------------------------------------------
// Player�� ���߰� �Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::SetStop()
{
	// ��ã�� ����
	// 2004, 9, 1, sobeit add start - ���� �״°� ����?����
	m_bKnockBack = 0;

	// 2004, 9, 1, sobeit add end

	m_listDirection.clear();

	// ���� ����	
	m_fTrace = FLAG_TRACE_NULL;
	//m_fNextTrace	= FLAG_TRACE_NULL;

	// Action ����
	m_sX = 0;
	m_sY = 0;

	int tempAction = m_MoveAction == ACTION_SLAYER_MOTOR_MOVE ? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND;
	if (IsOusters() && HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this)))
		tempAction = ACTION_OUSTERS_FAST_MOVE_STAND;
#if __CONTENTS(__FAST_TRANSFORTER)
	else if (IsOusters() && HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this)))
		tempAction = ACTION_OUSTERS_WING_STAND;
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
	else if (IsOusters() && HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this)))
		tempAction = ACTION_OUSTERS_UNICORN_STAND;
#endif //__SECOND_TRANSFORTER
	m_Action = tempAction;
	// 2004, 11, 3, sobeit modify start - m_ActionCount�� �ٲ� �׼��� �ƽ� ī��Ʈ�� ���� ���� - �ƿ콺���� ����Ʈ ���� ����..�Ѥ�;
	m_ActionCount = m_ActionCountMax;
	//m_ActionCount	= (*g_pCreatureTable)[m_CreatureType].GetActionCount( m_Action );
	// 2004, 11, 3, sobeit modify end
	m_MoveCount = m_MoveCountMax;

	// ���� ���۵� ����
	m_bNextAction = false;
	m_NextX = SECTORPOSITION_NULL;
	m_NextY = SECTORPOSITION_NULL;

	//m_nNextUsedActionInfo = ACTIONINFO_NULL;
	//m_nUsedActionInfo = ACTIONINFO_NULL;


	// ��ǥ ����
	m_DestX = SECTORPOSITION_NULL;
	m_DestY = SECTORPOSITION_NULL;
	m_NextDestX = SECTORPOSITION_NULL;
	m_NextDestY = SECTORPOSITION_NULL;
}


//----------------------------------------------------------------------
// priority queue�� m_pqDNodes�� �����.
//----------------------------------------------------------------------
void
MPlayer::RemoveNodes()
{
	while (!m_pqDNodes.empty())
	{
		//DNode* pDNode = m_pqDNodes.top();
		m_pqDNodes.pop();
		//delete pDNode;		
	}


	// temp�� �����.
	DNODE_LIST::iterator iNode = m_listDNodes.begin();

	while (iNode != m_listDNodes.end())
	{
		DNode* pNode = *iNode;
		delete pNode;
		iNode++;
	}

	m_listDNodes.clear();
}

//----------------------------------------------------------------------
// (x,y)�� ���� m_Dest(X,Y)������ �Ÿ��� ���Ѵ�.
//----------------------------------------------------------------------
// ����,����,�밢�� ��� 1�̹Ƿ� ���� ū ���� �Ÿ��̰�
// ��ǥ�� ���� ��ǥ�� �ִ°� �� �����Ƿ� �ƴ� ��� +1�� �ǵ��� �Ѵ�.
//----------------------------------------------------------------------
int
MPlayer::CalculateDistance(int x, int y)
{
	//int xx = x-m_DestX,
	//	yy = y-m_DestY;

	//return abs(xx)+abs(yy);

	//return sqrt(xx*xx + yy*yy);
//	return xx*xx + yy*yy;

//	/*
	int xd = abs(x - m_DestX),
		yd = abs(y - m_DestY);

	int plus = !(m_DestX == x) + !(m_DestY == y);

	if (xd > yd)
	{
		return xd * xd + plus;
	}
	else return yd * yd + plus;

	return 0;
	//	*/
}

//----------------------------------------------------------------------
// Get Destination()
//----------------------------------------------------------------------
void
MPlayer::GetDestination(POINT& dest)
{
	dest.x = m_DestX;
	dest.y = m_DestY;
}

//----------------------------------------------------------------------
// Set Move Position
//----------------------------------------------------------------------
bool
MPlayer::SetMovePosition(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
{
	// ������� ���� ���..
	if (
		(
			!m_bAlive
			// ���ӿ� �ִ� ���
			|| m_bInCasket
			// �� �ӿ� �ִ� ���
			|| IsUndergroundCreature()
			|| HasEffectStatus(EFFECTSTATUS_SLEEP)
			)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		return false;
	}

	// ������ ����ؾ��� ����� �ִ� ���...
	//if (m_nUsedActionInfo!=ACTIONINFO_NULL)
	if (m_ActionCount >= m_ActionCountMax)
	{
		m_fTraceBuffer = FLAG_TRACE_NULL;
		m_TraceIDBuffer = OBJECTID_NULL;
		m_nNextUsedActionInfo = ACTIONINFO_NULL;

		//if (m_ActionCount==m_ActionCountMax)
		//{			
		m_nUsedActionInfo = ACTIONINFO_NULL;

		/*
		DEBUG_ADD("Clear m_nUsedActionInfo");
	}
	else
	{
		DEBUG_ADD_FORMAT("Not Clear : action=%d, %d/%d", (int)m_Action, (int)m_ActionCount, (int)m_ActionCountMax);
	}
	*/
		TraceNULL();
	}

	return SetNextDestination(sX, sY);
}

//----------------------------------------------------------------------
// Set Next Destination
//----------------------------------------------------------------------
// ������ �̵��� ��ǥ ��ġ�� �����Ѵ�.
// ��ǥ��ġ�� �� �� ���� ���� ���... ��� �� ���ΰ�?
//----------------------------------------------------------------------
bool
MPlayer::SetNextDestination(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
{
	//DEBUG_ADD_FORMAT("[Set] NextDestination (%d, %d) --> (%d, %d)", m_X, m_Y, sX, sY);

	// ������� ���� ���..
	if (
		(!m_bAlive || HasEffectStatus(EFFECTSTATUS_SLEEP))
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		return false;
	}

	// ������ ����ؾ��� ����� �ִ� ���...
	if (m_nUsedActionInfo != ACTIONINFO_NULL)
	{
		m_fTraceBuffer = FLAG_TRACE_NULL;
		m_TraceIDBuffer = OBJECTID_NULL;
		m_nNextUsedActionInfo = ACTIONINFO_NULL;

		if (m_ActionCount >= m_ActionCountMax)
		{
			m_nUsedActionInfo = ACTIONINFO_NULL;

			DEBUG_ADD("Clear m_nUsedActionInfo");
		}
		else
		{
			DEBUG_ADD_FORMAT("Not Clear : action=%d, %d/%d", (int)m_Action, (int)m_ActionCount, (int)m_ActionCountMax);
		}

		TraceNULL();
	}

	// ���� ���� �ƴϸ鼭
	// �̹� ��ǥ��ġ�� �ִ� ���� ��ã�� �ʿ����.
	if (m_fTrace == FLAG_TRACE_NULL && sX == m_X && sY == m_Y)
	{
		return false;
	}

	// �߸��� ��
	// ������ �� ���� ��..
	if (sX == SECTORPOSITION_NULL || sY == SECTORPOSITION_NULL
		|| sX == m_BlockDestX && sY == m_BlockDestY)
	{
		return false;
	}

	//---------------------------------------------------
	// [��� 1]
	// ��ǥ�������� �� �� ���ٸ�
	// ��ǥ ������ ���� ����� ������ �̵��ϰ� �Ѵ�.
	//---------------------------------------------------
	/*
	TYPE_SECTORPOSITION destX = sX;
	TYPE_SECTORPOSITION	destY = sY;

	//---------------------------------------------------
	// �� �� �ִ� ��ǥ������ �����Ѵ�.
	//---------------------------------------------------
	while (!m_pZone->CanMove(m_MoveType, destX, destY))
	{
		// ��ǥ(sX,sY)���� ���� ��ġ���� ����
		BYTE direction = MTopView::GetDirectionToPosition(sX, sY, m_X, m_Y);

		GetPositionToDirection(destX, destY, direction);

		// ������(!) �� �� ���� ���̴�.
		if (destX==m_X && destY==m_Y)
			return false;
	}
	m_NextDestX = destX;
	m_NextDestY = destY;
	*/

	if (m_fTrace == FLAG_TRACE_NULL)
	{
		BOOL bCanStand = m_pZone->CanMove(m_MoveType, sX, sY);
		if (!bCanStand)
		{
			extern POINT g_DirectionValue[MAX_DIRECTION];

			POINT MovePoint;

			if (sY > GetY())
				MovePoint.y = -1;
			else if (sY < GetY())
				MovePoint.y = 1;
			else
				MovePoint.y = 0;

			if (sX > GetX())
				MovePoint.x = -1;
			else if (sX < GetX())
				MovePoint.x = 1;
			else
				MovePoint.x = 0;

			int limit = 20;	// ��ȿ�� 20������ ��ã���� ����

			while (bCanStand == FALSE && !(GetX() == sX && GetY() == sY) &&
				sX >= 0 && sY >= 0
				&& sX < m_pZone->GetWidth() && sY < m_pZone->GetHeight()
				&& limit-- > 0
				)
			{
				if (GetX() != sX)sX += MovePoint.x;
				if (GetY() != sY)sY += MovePoint.y;
				bCanStand = m_pZone->CanMove(m_MoveType, sX, sY);
			};

			if (bCanStand == FALSE || GetX() == sX && GetY() == sY)
				return false;
		}
	}
	//	else
		//---------------------------------------------------
		// [��� 2]
		// ���� ��ġ���� ��ǥ �������� �� ĭ �̵�..
		//
		// ��, ���� ���� ����... �������.. 
		// �̵� �߿�����...
		//---------------------------------------------------
	//	if (m_fTrace==FLAG_TRACE_NULL && !m_pZone->CanMove(m_MoveType, sX, sY))
	//	{
	//		BYTE direction = MTopView::GetDirectionToPosition(m_X, m_Y, sX, sY);
	//
	//		sX = m_X;
	//		sY = m_Y;
	//
	//		GetPositionToDirection(sX, sY, direction);
	//
	//		// ���ο� ��ǥ �������� �� ���� ���..
	//		if (!m_pZone->CanMove(m_MoveType, sX, sY))
	//		{
	//			return false;
	//		}
	//	}



	m_NextDestX = sX;
	m_NextDestY = sY;

	return true;
}

//----------------------------------------------------------------------
// Reset Send Move
//----------------------------------------------------------------------
// server���� ��ٸ��� messasge�� ���ٰ� ǥ���Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::ResetSendMove()
{
	g_bNetStatusGood = true;
	m_SendMove = 0;
	m_listSendDirection.clear();
}

//----------------------------------------------------------------------
// Get DeadDelay Last
//----------------------------------------------------------------------
// Player�� �װ� �� ��, ��ٸ��� �ð�..
//----------------------------------------------------------------------
DWORD
MPlayer::GetDeadDelayLast() const
{
	int second = m_DeadDelayTime - g_CurrentTime;

	if (second < 0)
	{
		return 0;
	}

	return second / 1000;
}

//----------------------------------------------------------------------
// Get NextDestination()
//----------------------------------------------------------------------
void
MPlayer::GetNextDestination(POINT& dest)
{
	dest.x = m_NextDestX;
	dest.y = m_NextDestY;
}

//----------------------------------------------------------------------
// Get NextPosition()
//----------------------------------------------------------------------
// ��ã�⿡ ���ؼ� ������ ��� �߿���
// �ٷ� ������ ������ Sector�� ���� ��ǥ�� �Ѱ��ش�.
//
// '������ġ'���� '���� ����'�� ���� ������ ��ȭ�� �����ش�.
//----------------------------------------------------------------------
bool
MPlayer::GetNextPosition(POINT& next)
{
	// �� ���� �������� ���� ���
	if (m_listDirection.empty())
	{
		//next.x = SECTORPOSITION_NULL;
		//next.y = SECTORPOSITION_NULL;
		return false;
	}


	// �� ���� ������ ���, ���� ��ǥ�� return�Ѵ�.

	// ���� ��ġ���� ���� ���⿡ ���ؼ� ��ǥ ����
	next.x = m_X;
	next.y = m_Y;

	switch (m_listDirection.front())
	{
	case DIRECTION_LEFTDOWN: next.x--;	next.y++;	break;
	case DIRECTION_RIGHTUP: next.x++;	next.y--;	break;
	case DIRECTION_LEFTUP: next.x--;	next.y--;	break;
	case DIRECTION_RIGHTDOWN: next.x++;	next.y++;	break;
	case DIRECTION_LEFT: next.x--;				break;
	case DIRECTION_DOWN:			next.y++;	break;
	case DIRECTION_UP:			next.y--;	break;
	case DIRECTION_RIGHT: next.x++;				break;
	}

	return true;
}


//----------------------------------------------------------------------
// Set Destination(sX, sY)
//----------------------------------------------------------------------
// ��ǥ��ġ�� �����ϴ� ������ ��ǥ��ġ������
// Best Path�� �����ؼ� m_listDirection�� �����صд�.
//----------------------------------------------------------------------
bool
MPlayer::SetDestination(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
{
	if (HasEffectStatus(EFFECTSTATUS_SLEEP)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
		return false;

	// message : find path
	//DEBUG_ADD_FORMAT("........Find Path........");				

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD_FORMAT("SetDest(%d, %d)", sX, sY);
#endif

	//-------------------------------------------------------
	// ������� ���� ���..
	//-------------------------------------------------------
	if (!m_bAlive)
	{
#ifdef OUTPUT_DEBUG_PLAYER_ACTION
		DEBUG_ADD("dead");
#endif
		return false;
	}

	//-------------------------------------------------------
	// if not empty, delete all elements
	//-------------------------------------------------------
	if (!m_listDirection.empty())
	{
		m_listDirection.clear();
	}

	// ������ ��������� �ڱ��� ��������� ������!
	bool bHasRelic =
		(m_bEffectStatus[EFFECTSTATUS_HAS_VAMPIRE_RELIC] || m_bEffectStatus[EFFECTSTATUS_HAS_SLAYER_RELIC]) &&
		(g_pZone->GetSector(sX, sY).IsSafeSlayer() && IsSlayer() ||
			g_pZone->GetSector(sX, sY).IsSafeVampire() && IsVampire() ||
			g_pZone->GetSector(sX, sY).IsSafeOusters() && IsOusters()
			);

	bool bHasBloodBible =
		(
			m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_ARMEGA] || m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_MIHOLE] ||
			m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_KIRO] || m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_INI] ||
			m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_GREGORI] || m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_CONCILIA] ||
			m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_LEGIOS] || m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_HILLEL] ||
			m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_JAVE] || m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_NEMA] ||
			m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_AROSA] || m_bEffectStatus[EFFECTSTATUS_HAS_BLOOD_BIBLE_CHASPA]) &&
		(g_pZone->GetSector(sX, sY).IsSafeSlayer() && IsSlayer() ||
			g_pZone->GetSector(sX, sY).IsSafeOusters() && IsOusters() ||
			g_pZone->GetSector(sX, sY).IsSafeVampire() && IsVampire()
			);

	if (bHasBloodBible)
	{
		g_pSystemMessage->Add((*g_pGameStringTable)[STRING_MESSAGE_CANNOT_MOVE_SAFETY_ZONE_BLOOD_BIBLE].GetString());
	}

	if (bHasRelic)
	{
		g_pSystemMessage->Add((*g_pGameStringTable)[STRING_MESSAGE_CANNOT_MOVE_SAFETY_ZONE_RELIC].GetString());
	}

	if (HasEffectStatus(EFFECTSTATUS_HAS_FLAG) &&
		(g_pZone->GetSector(sX, sY).IsSafeSlayer() && IsSlayer() ||
			g_pZone->GetSector(sX, sY).IsSafeVampire() && IsVampire() ||
			g_pZone->GetSector(sX, sY).IsSafeOusters() && IsOusters()
			))
	{
		bHasRelic = true;
		g_pSystemMessage->Add((*g_pGameStringTable)[STRING_MESSAGE_CANNOT_MOVE_SAFETY_ZONE_FLAG].GetString());
	}

	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (sX < 0 || sY < 0
		|| sX >= m_pZone->GetWidth() || sY >= m_pZone->GetHeight()
		// [�����] Sanctuary �δ� �� �����δ�.
		|| g_pZone->GetSector(m_X, m_Y).HasSanctuary()
		|| bHasRelic
		|| bHasBloodBible
		)
	{
		// Zone���� ���̸� check �� �� ���
		//m_ActionCount = m_ActionCountMax;
		m_MoveCount = m_MoveCountMax;
		m_DestX = SECTORPOSITION_NULL;
		m_DestY = SECTORPOSITION_NULL;

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
		DEBUG_ADD("outOfZone or Sanctuary");
#endif

		return false;

		// ���� ���� �������� �̵��ϰ� �ϴ� ���
		/*
		POINT position = g_pTopView->GetSelectedSector(g_x, g_y);
		BYTE direction = GetDirectionToPosition(position.x, position.y);

		// ���⿡ ���� ��ġ�� �����Ѵ�.
		MCreature::GetNextPosition(direction, position);

		sX = position.x;
		sY = position.y;

		// ���� ���� ������ Zone�� ����� ���
		if (sX<0 || sY<0
			|| sX>=m_pZone->GetWidth() || sY>=m_pZone->GetHeight())
		{
			return false;
		}
		*/
	}

	//-------------------------------------------------------
	// ���� ���� ���, �̹� ���� �Ÿ��� ���� ��...
	//-------------------------------------------------------
	if (m_fTrace != FLAG_TRACE_NULL
		&& max(abs(m_TraceX - m_X), abs(m_TraceY - m_Y)) <= m_TraceDistance)
	{
		//m_ActionCount = m_ActionCountMax;
		m_MoveCount = m_MoveCountMax;

		m_DestX = SECTORPOSITION_NULL;
		m_DestY = SECTORPOSITION_NULL;

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
		DEBUG_ADD("already in dist");
#endif

		return false;
	}

	/*
	// priority queue test code
	ofstream file("test.txt", ios::app);
	CString str;

	m_pqDNodes.push( new DNode(0,0,0,1) );
	m_pqDNodes.push( new DNode(0,0,0,4) );
	m_pqDNodes.push( new DNode(0,0,0,3) );
	m_pqDNodes.push( new DNode(0,0,0,8) );
	m_pqDNodes.push( new DNode(0,0,0,2) );
	m_pqDNodes.push( new DNode(0,0,0,4) );
	m_pqDNodes.push( new DNode(0,0,0,7) );

	while (!m_pqDNodes.empty())
	{
		DNode*	pDNode = m_pqDNodes.top();
		m_pqDNodes.pop();

		str.Format("Pop : (%d,%d) , [%d], dist=%d", pDNode->x, pDNode->y, pDNode->direction, pDNode->distance);

		file << str << endl;
	}

	return;
	*/

	//--------------------------------------------------------------
	// �� �� �ִ� ���̸�
	//--------------------------------------------------------------
	// Best First Search
	//--------------------------------------------------------------
	BOOL	bCanStand = m_pZone->CanMove(m_MoveType, sX, sY);

#ifdef __EXPO_CLIENT__
	bCanStand |= (BOOL)g_UserInformation.Invisible;
#endif

	//--------------------------------------------------------------
	// �� �� �ִ� ���̰ų�
	// �������̸� Best Search~�� ���� ã�´�.
	//--------------------------------------------------------------
	if (bCanStand || m_fTrace != FLAG_TRACE_NULL)
	{
		//-------------------------------------------------------
		// ��ǥ��ġ�� ����
		//-------------------------------------------------------
		m_DestX = sX;
		m_DestY = sY;

		//-------------------------------------------------------
		// priority queue�� �ʱ�ȭ�Ѵ�.
		//-------------------------------------------------------
		RemoveNodes();

		//-------------------------------------------------------
		// visited�� ��� false�� �صΰ�
		// ������ġ���� �湮���� ���� ������ �湮�ϱ� �����Ѵ�.
		//-------------------------------------------------------
		POINT firstSector = g_pTopView->GetFirstSector();
		//int x0 = m_DestX - g_pClientConfig->MAX_FINDPATH_WIDTH, 
		//	x1 = m_DestX + g_pClientConfig->MAX_FINDPATH_WIDTH+1, 
		//	y0 = m_DestY - g_pClientConfig->MAX_FINDPATH_HEIGHT,
		//	y1 = m_DestY + g_pClientConfig->MAX_FINDPATH_HEIGHT+1;
		int x0 = firstSector.x - 1,
			x1 = firstSector.x + SECTOR_WIDTH + 5,
			y0 = firstSector.y - 1,
			y1 = firstSector.y + SECTOR_HEIGHT + 5;

		if (x0 < 0)
			x0 = 0;
		if (x1 > g_pZone->GetWidth())
			x1 = g_pZone->GetWidth();

		if (y0 < 0)
			y0 = 0;
		if (y1 > g_pZone->GetHeight())
			y1 = g_pZone->GetHeight();

		int size = x1 - x0;	// byte��
		for (int i = y0; i < y1; i++)
		{
			for (int j = x0; j < x1; j++)
			{
				//m_ppVisited[i][j] = false;
				g_pZone->UnSetVisitedFlag(j, i);
			}
			//memset( m_ppVisited[i]+x0, false, size );
		}


		//-------------------------------------------------------
		//
		// ���� ��ġ
		//
		//-------------------------------------------------------

		//-------------------------------------------------------
		// �������� ���� ������� ��ã�⸦ �Ѵ�.		
		//-------------------------------------------------------
		static BYTE FirstDirection = GetDirectionToPosition(m_DestX, m_DestY);
		static TYPE_SECTORPOSITION previousDestX = SECTORPOSITION_NULL;
		static TYPE_SECTORPOSITION previousDestY = SECTORPOSITION_NULL;

		//-------------------------------------------------------
		// ��� ���� ��ã�� �ߴ��Ŷ� ��ǥ ��ġ�� �ٸ� ��쿡�� 
		// �������� ���� ������ �����Ѵ�.
		//-------------------------------------------------------
		if (previousDestX != m_DestX || previousDestY != m_DestY)
		{
			FirstDirection = GetDirectionToPosition(m_DestX, m_DestY);
			previousDestX = m_DestX;
			previousDestY = m_DestY;
		}
		//-------------------------------------------------------
		// ���� ��ǥ�� ���� ���ư��� ���� 
		// ���� Player�� ������� ���캻��.
		//-------------------------------------------------------
		else
		{
			FirstDirection = m_Direction;
		}

		DNode* currentDNode = new DNode(
			m_X,
			m_Y,
			FirstDirection,
			CalculateDistance(m_X, m_Y),
			0
		);

		//-------------------------------------------------------
		// listDNodes�� �־ ���߿� �����ش�.
		//-------------------------------------------------------
		m_listDNodes.push_back(currentDNode);

		currentDNode->SetParent(NULL);
		m_pqDNodes.push(currentDNode);

		POINT	next;
		int		dist;
		bool	bFound = false;		// ���� �������� �ʾҴٴ� �ǹ�

		while (!bFound && !m_pqDNodes.empty())
		{
			//-------------------------------------------------------
			// ���� ������(!) Node�� ���(priority queue�̴�)
			// ���� ���������� ���ؼ� ����غ���.
			//-------------------------------------------------------
			DNode* pDNode = m_pqDNodes.top();
			m_pqDNodes.pop();

			//-------------------------------------------------------
			// ������ ������ ���
			//-------------------------------------------------------
			if (pDNode->distance == 0)
			{
				m_pqDNodes.push(pDNode);
				bFound = true;
				break;
			}

			//-------------------------------------------------------
			// ���� �Ÿ� �̳��� �����ϱ�
			//-------------------------------------------------------
			if (m_fTrace != FLAG_TRACE_NULL &&
				max(abs(pDNode->x - m_DestX), abs(pDNode->y - m_DestY)) <= m_TraceDistance)
			{
				m_pqDNodes.push(pDNode);
				bFound = true;
				break;
			}

			int Direction = pDNode->direction;

			//-------------------------------------------------------
			// 8������ ��� check
			//-------------------------------------------------------
			for (int i = 0; i < 8; Direction++, i++)
			{
				if (Direction == 8) Direction = 0;

				next.x = pDNode->x;
				next.y = pDNode->y;

				switch (Direction)
				{
				case DIRECTION_LEFTDOWN: next.x--;	next.y++;	break;
				case DIRECTION_RIGHTUP: next.x++;	next.y--;	break;
				case DIRECTION_LEFTUP: next.x--;	next.y--;	break;
				case DIRECTION_RIGHTDOWN: next.x++;	next.y++;	break;
				case DIRECTION_LEFT: next.x--;				break;
				case DIRECTION_DOWN: next.y++;				break;
				case DIRECTION_UP: next.y--;				break;
				case DIRECTION_RIGHT: next.x++;				break;
				}

				//-------------------------------------------------------
				// Zone�� ���� ���̸� check ���Ѵ�.
				//-------------------------------------------------------
				if (next.x < 0 || next.y < 0
					|| next.x >= m_pZone->GetWidth() || next.y >= m_pZone->GetHeight()) continue;

				//-------------------------------------------------------
				// ���� ���̸� �� ����.
				//-------------------------------------------------------
				if (g_pZone->IsVisitedFlag(next.x, next.y)) continue;

				// �� ȭ���� �Ѿ�� ���� check���� �ʴ´�.				
				if (next.x < x0 || next.y < y0
					|| next.x >= x1 || next.y >= y1) continue;

				//-------------------------------------------------------
				// �� �� ������ pqueue�� �ִ´�.
				//-------------------------------------------------------
				if (m_pZone->CanMove(m_MoveType, next.x, next.y)
					&& !bHasRelic && !bHasBloodBible

#ifdef __EXPO_CLIENT__
					|| g_UserInformation.Invisible
#endif

					)
				{
					g_pZone->SetVisitedFlag(next.x, next.y);
					//					m_ppVisited[next.y][next.x] = true;

										//-------------------------------------------------------
										// Node�� ���� priority queue�� �߰��Ѵ�.
										//-------------------------------------------------------
										// �Ÿ� ���
					dist = CalculateDistance(next.x, next.y);

					// Node�� ���� �߰�
					DNode* nextDNode = new DNode(
						next.x,
						next.y,
						Direction,
						dist,
						pDNode->step + 1);


					//-------------------------------------------------------
					// listDNodes�� �־ ���߿� �����ش�.
					//-------------------------------------------------------
					m_listDNodes.push_back(nextDNode);

					//CString str;
					//str.Format("NewNode : (%d,%d)-->(%d,%d) , [%d], dist=%d", next.x, next.y, m_Dest.x, m_Dest.y, i, dist);

					nextDNode->SetParent(pDNode);
					m_pqDNodes.push(nextDNode);
				}


			}
		}

		//-------------------------------------------------------
		// ã���� ��� ���� ���ؾ� �Ѵ�.
		//-------------------------------------------------------
		if (bFound)
		{
			//-------------------------------------------------------
			// m_pqDNodes�κ��� list�� ä���. (parent�� ���󰡾��Ѵ�)
			//-------------------------------------------------------

			// ��ǥ��ġ
			DNode* currentNode = m_pqDNodes.top();
			m_pqDNodes.pop();

			while (currentNode->pParent != NULL)
			{
				m_listDirection.insert(m_listDirection.begin(), currentNode->direction);
				currentNode = currentNode->pParent;
			}

			RemoveNodes();

			m_BlockDestX = SECTORPOSITION_NULL;
			m_BlockDestY = SECTORPOSITION_NULL;
		}
		//-------------------------------------------------------
		// ���� ���� ��� �����Ѵ�.
		//-------------------------------------------------------
		else
		{
			// ���� ����
			if (m_fTrace != FLAG_TRACE_NULL)
			{
				m_MoveCount = m_MoveCountMax;

				TraceNULL();

				// 2001.7.31 �߰�
				m_nUsedActionInfo = ACTIONINFO_NULL;
				m_nNextUsedActionInfo = ACTIONINFO_NULL;
			}

			m_BlockDestX = m_DestX;
			m_BlockDestY = m_DestY;

			m_DestX = SECTORPOSITION_NULL;
			m_DestY = SECTORPOSITION_NULL;
		}
	}
	// �� �� ���� ��
	else
	{
		//m_ActionCount = m_ActionCountMax;
		m_MoveCount = m_MoveCountMax;

		// �̹��� �� �� ������ �� ����
		//m_BlockDestX = m_DestX;
		//m_BlockDestY = m_DestY;

		m_DestX = SECTORPOSITION_NULL;
		m_DestY = SECTORPOSITION_NULL;

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
		DEBUG_ADD("no way!");
#endif

		return false;
	}

	/*
	else
	// �� �κп� �ణ�� ������ �ִ� �� ��!! -_-;;;;
	//--------------------------------------------------------------
	// �ƿ� �� �� ���� ���̸�..
	//--------------------------------------------------------------
	// Player��ġ���� ��ǥ�������� ����?���� �ɾ�ٰ�
	// ��ֹ��� ����� �����.
	// (m_X, m_Y)  --->  (sX, sY)
	//--------------------------------------------------------------
	{
		int	stepX = sX - m_X,
			stepY = sY - m_Y,
			x = m_X,
			y = m_Y,
			signX, signY;

		//-------------------------------------------------------
		// if not empty, delete all elements
		//-------------------------------------------------------
		if (!m_listDirection.empty())
		{
			m_listDirection.clear();
		}


		while (1)
		{
			// ��ȣ�� ����..
			signX = (stepX==0)? 0 : (stepX<0)? -1 : 1;
			signY = (stepY==0)? 0 : (stepY<0)? -1 : 1;

			x += signX;
			y += signY;

			// �� �� �ִ� ���̸� list�� �ִ´�.
			if (m_pZone->CanMove(x,y))
			{
				m_listDirection.insert( m_listDirection.end(), DetermineDirection(stepX, stepY));
			}
			else
			{
				// x,y ��ĭ ���� ��ġ�� ��ǥ�������� ���Ѵ�.
				m_DestX = x - signX;
				m_DestY = y - signY;
				break;
			}

			// �� ĭ ���������� ǥ��
			stepX -= signX;
			stepY -= signY;
		}

		return;
	}
	*/

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD("find way");
#endif

	return true;
}

//----------------------------------------------------------------------
// Self Special Action
//----------------------------------------------------------------------
// �ڽ����� Ư����� ���
//----------------------------------------------------------------------
bool
MPlayer::SelfSpecialAction()
{
	// Dracula Castle: no skills at all while carrying the Mihnea (the server refuses them)
	if (g_pEffectStatusTable->GetSize() > EFFECTSTATUS_HAS_MIHNEA && HasEffectStatus(EFFECTSTATUS_HAS_MIHNEA))
		return false;
	if (HasEffectStatus(EFFECTSTATUS_SLEEP)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
		return false;

	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	if (!IsNotDelay() || HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE))
	{
		return false;
	}

	// [�����3] �� �ӿ� �ִ� ���� Open casket�� ����� �� �ִ�.
	//if (IsInCasket())
//	{
//		m_nSpecialActionInfo = MAGIC_OPEN_CASKET;
//	}


	if (m_nSpecialActionInfo == ACTIONINFO_NULL
		// ������̸� Ÿ�� ������ Ư�������� �ȵȴ�.
		|| m_MoveDevice == MOVE_DEVICE_RIDE
		// ���� ��� ����� ���� �޾ƾ� �ϸ� ����� �� ����.
		//|| m_WaitVerify != WAIT_VERIFY_NULL)	// 2001.8.20 �ּ�ó��
		|| IsInSafeSector() == 2
		// ���� �������뿡���� ����� ����� �� ����.
		)
		return false;

	// 2006.08.22 sjheon
	if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WOLF))
	{
		if (!(m_nSpecialActionInfo == MAGIC_HOWL || m_nSpecialActionInfo == MAGIC_EAT_CORPSE || m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_BAT))
	{
		if (!(m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WERWOLF))
	{
		if (!(m_nSpecialActionInfo == SKILL_BITE_OF_DEATH ||
			m_nSpecialActionInfo == MAGIC_RAPID_GLIDING ||
			m_nSpecialActionInfo == SKILL_VIOLENT_PHANTOM ||
			m_nSpecialActionInfo == SKILL_WILD_WOLF ||
			m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}

	//	|| HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_BAT) || 
	//	   HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WERWOLF))
	//	{
	//		SKILL_BITE_OF_DEATH				MAGIC_RAPID_GLIDING
	//		if( !(m_nSpecialActionInfo == MAGIC_HOWL || m_nSpecialActionInfo == MAGIC_EAT_CORPSE || m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
	//			return false ;
	//	}

		// �ڽſ��� ����� �� �ִ� �͵鸸 ����Ѵ�.
	if ((*g_pActionInfoTable)[m_nSpecialActionInfo].IsTargetSelf())
	{

		//-------------------------------------------------------
		// ���� �����
		//-------------------------------------------------------
		// skillID            itemType
		// MINE_ANKLE_KILLER,	// 5
		// MINE_POMZ,			// 6
		// MINE_AP_C1,			// 7
		// MINE_DIAMONDBACK,	// 8
		// MINE_SWIFT_EX		// 9
		//
		// �ϵ��ϵ�...
		//			
		if (m_nSpecialActionInfo == SKILL_INSTALL_MINE
			|| m_nSpecialActionInfo >= MINE_ANKLE_KILLER
			&& m_nSpecialActionInfo <= MINE_SWIFT_EX)
		{
			int					useSkill = SKILL_INSTALL_MINE;

			//-------------------------------------------------------
			// ���� ����� �� �ִ� ������� üũ..
			// Passive ��ų�̸� ��� ���ϰ�..
			//-------------------------------------------------------
			if (g_pSkillAvailable->IsEnableSkill((ACTIONINFO)useSkill)
				&& (*g_pSkillInfoTable)[useSkill].IsEnable()
				&& (*g_pSkillInfoTable)[useSkill].IsAvailableTime()
				&& !(*g_pSkillInfoTable)[useSkill].IsPassive()
				&& IsSlayer()
				&& !IsWaitVerify()
				&& IsItemCheckBufferNULL())
			{

				ITEM_CLASS	itemClass = ITEM_CLASS_MINE;

				MItem* pItem = NULL;

				if (m_nSpecialActionInfo != SKILL_INSTALL_MINE)
				{
					// Ư�� type�� ���ڸ� ã�´�.
					int itemType = m_nSpecialActionInfo - MINE_ANKLE_KILLER;

					MItemClassTypeFinder itemFinder(itemClass, itemType);
					pItem = ((MItemManager*)g_pInventory)->FindItem(itemFinder);
				}
				else
				{
					// ���� �ϲ��� ã�´�.
					MItemClassFinder itemFinder(itemClass);
					pItem = ((MItemManager*)g_pInventory)->FindItem(itemFinder);
				}

				// ������ ��ź�� �ִ� ���
				if (pItem != NULL)
				{
					// ���� ��ġ �غ�..
					gC_vs_ui.StartInstallMineProgress(pItem->GetGridX(), pItem->GetGridY());
				}
			}

			// �ݺ������� �������� �ʴ´�.
			m_fNextTrace = FLAG_TRACE_NULL;
			UnSetRepeatAction();
			return false;
		}

		//-------------------------------------------------------
		// ���� ����� �� �ִ� ������� üũ..
		// Passive ��ų�̸� ��� ���ϰ�..
		//-------------------------------------------------------
		if (!g_pSkillAvailable->IsEnableSkill((ACTIONINFO)m_nSpecialActionInfo)
			|| !(*g_pSkillInfoTable)[m_nSpecialActionInfo].IsEnable()
			|| !(*g_pSkillInfoTable)[m_nSpecialActionInfo].IsAvailableTime()
			|| (*g_pSkillInfoTable)[m_nSpecialActionInfo].IsPassive())
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			UnSetRepeatAction();

			return false;
		}

		if (IsStop()) // || m_Action!=ACTION_ATTACK)
		{
			//-------------------------------------------------------
			// Ư���� ����� ����� üũ
			//-------------------------------------------------------
			switch (m_nSpecialActionInfo)
			{
				//-------------------------------------------------------
				// ��Ż
				//-------------------------------------------------------
			case MAGIC_BLOODY_MARK:
				if (IsVampire() && g_bHolyLand == false && g_pZoneTable->Get(m_pZone->GetID())->CannotUseSpecialItem == false)
				{
					//----------------------------------------------------
					// Server�� ������ ���
					//----------------------------------------------------
						//----------------------------------------------------
						// ���� ������ ���� ���..
						//----------------------------------------------------
					if (!IsWaitVerify()
						&& IsItemCheckBufferNULL())
					{
						// 2005, 3, 2, sobeit modify start - FindItem() ��� FindItemAll() ���
						// ���ȵ�(Marked=false) VampirePortalItem�� ã�´�.

						MItem* pSubInventory = NULL;
						MVampirePortalItemFinder finder_unmarked(false);
						MItem* pItem = ((MItemManager*)g_pInventory)->FindItemAll(finder_unmarked, pSubInventory);

						if (pItem != NULL && pItem->GetNumber() > 0 && pItem->IsAffectStatus())
						{
							DWORD SubInventoryItemID = 0;
							//----------------------------------------------------
							// Server�� ������ ���
							//----------------------------------------------------
							CGSkillToInventory _CGSkillToInventory;
							_CGSkillToInventory.setObjectID(pItem->GetID());
							_CGSkillToInventory.setX(pItem->GetGridX());
							_CGSkillToInventory.setY(pItem->GetGridY());
							_CGSkillToInventory.setSkillType(MAGIC_BLOODY_MARK);
							//_CGSkillToInventory.setCEffectID( 0 );	// -_-;;							

							if (NULL != pSubInventory)
							{
								SubInventoryItemID = pSubInventory->GetID();
							}
							_CGSkillToInventory.setInventoryItemObjectID(SubInventoryItemID);

							g_pSocket->sendPacket(&_CGSkillToInventory);

							//----------------------------------------------------
							// Inventory���� item�� ����ϴ� �� �����ޱ⸦ ��ٸ���.
							//----------------------------------------------------
							SetWaitVerify(WAIT_VERIFY_SKILL_SUCCESS);
							SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY, SubInventoryItemID);

							g_pSkillInfoTable->SetNextAvailableTime(m_nSpecialActionInfo);
						}
						// 2005, 3, 2, sobeit modify end
					}
				}

				return false;
				break;

				//-------------------------------------------------------
				// ��Ż
				//-------------------------------------------------------
			case MAGIC_BLOODY_TUNNEL:
				if (IsVampire() && g_bHolyLand == false && g_pZoneTable->Get(m_pZone->GetID())->CannotUseSpecialItem == false)
				{
					//----------------------------------------------------
					// ���� ������ ���� ���..
					//----------------------------------------------------
					if (!IsWaitVerify()
						&& IsItemCheckBufferNULL())
					{
						// 2005, 3, 2, sobeit modify start - FindItem() ��� FindItemAll() ���

						// ����(Marked=true) VampirePortalItem�� ã�´�.
//								MItem* pItem = ((MItemManager*)g_pInventory)->FindItem( MVampirePortalItemFinder( true ) );

						MItem* pSubInventory = NULL;
						MVampirePortalItemFinder finder_marked(true);
						MItem* pItem = ((MItemManager*)g_pInventory)->FindItemAll(finder_marked, pSubInventory);

						if (pItem != NULL && pItem->GetNumber() > 0 && pItem->IsAffectStatus())
						{
							DWORD SubInventoryItemID = 0;
							CGSkillToInventory _CGSkillToInventory;
							_CGSkillToInventory.setObjectID(pItem->GetID());
							_CGSkillToInventory.setX(pItem->GetGridX());
							_CGSkillToInventory.setY(pItem->GetGridY());
							_CGSkillToInventory.setSkillType(MAGIC_BLOODY_TUNNEL);
							//_CGSkillToInventory.setCEffectID( 0 );	// -_-;;							

							if (NULL != pSubInventory)
							{
								SubInventoryItemID = pSubInventory->GetID();
							}
							_CGSkillToInventory.setInventoryItemObjectID(SubInventoryItemID);

							g_pSocket->sendPacket(&_CGSkillToInventory);

							//----------------------------------------------------
							// Inventory���� item�� ����ϴ� �� �����ޱ⸦ ��ٸ���.
							//----------------------------------------------------
							SetWaitVerify(MPlayer::WAIT_VERIFY_SKILL_SUCCESS);
							SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY, SubInventoryItemID);

							g_pSkillInfoTable->SetNextAvailableTime(m_nSpecialActionInfo);

						}
					}
				}

				return false;
				break;

				//-------------------------------------------------------
				// ��Ż
				//-------------------------------------------------------
			case SUMMON_HELICOPTER:
				if (IsSlayer() && g_bHolyLand == false && g_pZoneTable->Get(m_pZone->GetID())->CannotUseSpecialItem == false)
				{
					//----------------------------------------------------
					// ���� ������ ���� ���..
					//----------------------------------------------------


				// 2005, 3, 2, sobeit modify start - FindItem() ��� FindItemAll() ���
					if (IsWaitVerifyNULL()
						&& IsItemCheckBufferNULL()
						&& g_pZone->GetHelicopter(GetID()) == NULL)
					{
						MItem* pSubInventory = NULL;
						MSlayerPortalItemFinder finder_slayer_portal;
						MItem* pItem = ((MItemManager*)g_pInventory)->FindItemAll(finder_slayer_portal, pSubInventory);

						if (pItem != NULL && pItem->GetNumber() > 0 && pItem->IsAffectStatus())
						{
							DWORD SubInventoryItemID = 0;
							CGUseItemFromInventory _CGUseItemFromInventory;
							_CGUseItemFromInventory.setObjectID(pItem->GetID());
							_CGUseItemFromInventory.setX(pItem->GetGridX());
							_CGUseItemFromInventory.setY(pItem->GetGridY());

							if (NULL != pSubInventory)
							{
								SubInventoryItemID = pSubInventory->GetID();
							}
							_CGUseItemFromInventory.setInventoryItemObjectID(SubInventoryItemID);

							g_pSocket->sendPacket(&_CGUseItemFromInventory);

							//----------------------------------------------------
							// Inventory���� item�� ����ϴ� �� �����ޱ⸦ ��ٸ���.
							//----------------------------------------------------
							SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_USE_FROM_INVENTORY, SubInventoryItemID);
						}
					}
					// 2005, 3, 2, sobeit modify end
				}

				return false;
				break;

				//-------------------------------------------------------
				// ���� �����
				//-------------------------------------------------------
			case MAGIC_CREATE_HOLY_WATER:
				if (
					IsSlayer()
					&& !IsWaitVerify()
					&& IsItemCheckBufferNULL())
				{
					MItem* pItem = NULL;

					for (int i = 0; i <= 2; i++)
					{
						pItem = g_pInventory->FindItem(ITEM_CLASS_WATER, i);
						if (pItem != NULL)
							break;
					}

					if (pItem != NULL)
					{
						//							POINT fitPoint;			// ������ �� �ڸ�

						//							if (GetMakeItemFitPosition(pItem, 
						//														ITEM_CLASS_HOLYWATER, 
						//														pItem->GetItemType(), 
						//														fitPoint))
						{
							CGSkillToInventory _CGSkillToInventory;
							_CGSkillToInventory.setObjectID(pItem->GetID());
							_CGSkillToInventory.setX(pItem->GetGridX());
							_CGSkillToInventory.setY(pItem->GetGridY());
							_CGSkillToInventory.setTargetX(pItem->GetGridX());
							_CGSkillToInventory.setTargetY(pItem->GetGridY());
							_CGSkillToInventory.setSkillType(MAGIC_CREATE_HOLY_WATER);
							//_CGSkillToInventory.setCEffectID( 0 );	// -_-;;							

							g_pSocket->sendPacket(&_CGSkillToInventory);


							// �ϴ�(!) �׳� ���ְ� ����.
							//(*g_pInventory).RemoveItem( pItem->GetID() );

							//----------------------------------------------------
							// Inventory���� item�� ����ϴ� �� �����ޱ⸦ ��ٸ���.
							//----------------------------------------------------
							SetWaitVerify(MPlayer::WAIT_VERIFY_SKILL_SUCCESS);
							SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY);

							g_pSkillInfoTable->SetNextAvailableTime(MAGIC_CREATE_HOLY_WATER);

							//----------------------------------------------------
							// ��� ��� �õ� ����
							//----------------------------------------------------
							AddNewInventoryEffect(pItem->GetID(),
								MAGIC_CREATE_HOLY_WATER, //+ (*g_pActionInfoTable).GetMinResultActionInfo(),
								g_pClientConfig->FPS * 3	// 3��
							);
							// �ݺ������� �������� �ʴ´�.
							m_fNextTrace = FLAG_TRACE_NULL;
							UnSetRepeatAction();
						}
						//							else
						//							{
						//								// g_pGameMessage( "�ڸ��� ������~" );
						//							}
					}
				}

				return false;
				break;
			case SKILL_CREATE_HOLY_POTION:
				if (
					IsSlayer()
					&& !IsWaitVerify()
					&& IsItemCheckBufferNULL())
				{

					MItem* pItem = NULL;

					for (int i = 3; i <= 6; i++)
					{
						pItem = g_pInventory->FindItem(ITEM_CLASS_WATER, i);
						if (pItem != NULL)
							break;
					}

					if (pItem != NULL)
					{
						POINT fitPoint;			// ������ �� �ڸ�

						if (GetMakeItemFitPosition(pItem,
							ITEM_CLASS_POTION,
							(*g_pItemTable)[pItem->GetItemClass()][pItem->GetItemType()].Value3,
							fitPoint))
						{
							CGSkillToInventory _CGSkillToInventory;
							_CGSkillToInventory.setObjectID(pItem->GetID());
							_CGSkillToInventory.setX(pItem->GetGridX());
							_CGSkillToInventory.setY(pItem->GetGridY());
							_CGSkillToInventory.setTargetX(fitPoint.x);
							_CGSkillToInventory.setTargetY(fitPoint.y);
							_CGSkillToInventory.setSkillType(SKILL_CREATE_HOLY_POTION);
							//_CGSkillToInventory.setCEffectID( 0 );	// -_-;;							

							g_pSocket->sendPacket(&_CGSkillToInventory);


							// �ϴ�(!) �׳� ���ְ� ����.
							//(*g_pInventory).RemoveItem( pItem->GetID() );

							//----------------------------------------------------
							// Inventory���� item�� ����ϴ� �� �����ޱ⸦ ��ٸ���.
							//----------------------------------------------------
							SetWaitVerify(MPlayer::WAIT_VERIFY_SKILL_SUCCESS);
							SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY);

							g_pSkillInfoTable->SetNextAvailableTime(SKILL_CREATE_HOLY_POTION);

							//----------------------------------------------------
							// ��� ��� �õ� ����
							//----------------------------------------------------
							AddNewInventoryEffect(pItem->GetID(),
								SKILL_CREATE_HOLY_POTION, //+ (*g_pActionInfoTable).GetMinResultActionInfo(),
								g_pClientConfig->FPS * 3	// 3��
							);
							// �ݺ������� �������� �ʴ´�.
							m_fNextTrace = FLAG_TRACE_NULL;
							UnSetRepeatAction();
						}
						else
						{
							// g_pGameMessage( "�ڸ��� ������~" );
						}
					}
				}

				return false;
				break;

				//-------------------------------------------------------
				// ��ź �����
				// ���� �����
				//-------------------------------------------------------
			case SKILL_MAKE_BOMB:
			case SKILL_MAKE_MINE:
				if (IsSlayer()
					&& !IsWaitVerify()
					&& IsItemCheckBufferNULL()
					&& !gC_vs_ui.IsInstallMineProgress())	// ���� ����� ���̸�..
					//|| gC_vs_ui.IsCreateMineProgress()	// ���� ����� ���̸�..
					//|| gC_vs_ui.IsCreateBombProgress()	// ���� ����� ���̸�..
				{

					int					skillID;
					ITEM_CLASS	itemClass;
					int					minItemType;
					int					maxItemType;
					int					itemTypeModifier;

					// itemType�� 0~4�����̸� ��ź�̴�.
					if (m_nSpecialActionInfo == SKILL_MAKE_BOMB)
					{
						skillID = SKILL_MAKE_BOMB;
						itemClass = ITEM_CLASS_BOMB;
						minItemType = 0;
						maxItemType = 4;
						itemTypeModifier = 0;
					}
					else
					{
						skillID = SKILL_MAKE_MINE;
						itemClass = ITEM_CLASS_MINE;
						minItemType = 5;
						maxItemType = 9;
						itemTypeModifier = -5;
					}

					MItemClassRangeTypeFinder itemFinder(ITEM_CLASS_BOMB_MATERIAL,
						minItemType, maxItemType);

					MItem* pItem = ((MItemManager*)g_pInventory)->FindItem(itemFinder);

					if (pItem != NULL)
					{
						POINT fitPoint;			// �� �ڸ�

						if (GetMakeItemFitPosition(pItem,
							itemClass,
							pItem->GetItemType() + itemTypeModifier,
							fitPoint))
						{
							// ����� �����ϴ� ����...
							if (skillID == SKILL_MAKE_BOMB)
							{
								gC_vs_ui.StartCreateBombProgress(pItem->GetGridX(), pItem->GetGridY());
							}
							else
							{
								gC_vs_ui.StartCreateMineProgress(pItem->GetGridX(), pItem->GetGridY());
							}

							// UI���� ����� �����ְ� ����
							// UI_ITEM_USE�� ���ƿ��� �κп��� ó���Ѵ�.

							// ���������� ����ϱ� ���ؼ�
//								m_fNextTrace	= FLAG_TRACE_SELF;
//				
//								m_bRepeatAction = FALSE;

								// �ݺ������� �������� �ʴ´�.
							m_fNextTrace = FLAG_TRACE_NULL;
							UnSetRepeatAction();

							/*
							CGSkillToInventory _CGSkillToInventory;
							_CGSkillToInventory.setObjectID( pItem->GetID() );
							_CGSkillToInventory.setX( pItem->GetGridX() );
							_CGSkillToInventory.setY( pItem->GetGridY() );
							_CGSkillToInventory.setTargetX( fitPoint.x );
							_CGSkillToInventory.setTargetY( fitPoint.y );
							_CGSkillToInventory.setSkillType( skillID );
							//_CGSkillToInventory.setCEffectID( 0 );	// -_-;;

							g_pSocket->sendPacket( &_CGSkillToInventory );


							// �ϴ�(!) �׳� ���ְ� ����.
							//(*g_pInventory).RemoveItem( pItem->GetID() );

							//----------------------------------------------------
							// Inventory���� item�� ����ϴ� �� �����ޱ⸦ ��ٸ���.
							//----------------------------------------------------
							SetWaitVerify( MPlayer::WAIT_VERIFY_SKILL_SUCCESS );
							SetItemCheckBuffer( pItem, MPlayer::ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY );

							(*g_pSkillInfoTable)[skillID].SetNextAvailableTime();

							//----------------------------------------------------
							// ��� ��� �õ� ����
							//----------------------------------------------------
							AddNewInventoryEffect( pItem->GetID(),
												skillID, //+ (*g_pActionInfoTable).GetMinResultActionInfo(),
												g_pClientConfig->FPS*3	// 3��
											);
							*/
						}
						else
						{
							// g_pGameMessage( "�ڸ��� ������~" );
						}
					}
				}

				return false;
				break;

				//-------------------------------------------------------
				// ���㳪 ���� ����
				//-------------------------------------------------------
			case SKILL_TRANSFORM_TO_WERWOLF:
			case MAGIC_TRANSFORM_TO_WOLF:
			case MAGIC_TRANSFORM_TO_BAT:
				//----------------------------------------------------
				//
				//					���ſ� ������ - vampire�� ���
				//
				//----------------------------------------------------
				if (IsVampire() && GetAction() == ACTION_VAMPIRE_DRAIN)
					return false;

				if (IsVampire() && g_pZoneTable->Get(m_pZone->GetID())->CannotUseSpecialItem == false)
				{
					//----------------------------------------------------
					// ���� ������ ���� ���..
					//----------------------------------------------------
					if (!IsWaitVerify()
						&& IsItemCheckBufferNULL())
					{
						MItem* pItem = NULL;
						MItem* pSubInventory = NULL;

						if (m_nSpecialActionInfo == MAGIC_TRANSFORM_TO_WOLF)
						{
							// ���� ���� ã��
							pItem = g_pInventory->FindItem(ITEM_CLASS_VAMPIRE_ETC, 0);
						}
						else if (m_nSpecialActionInfo == SKILL_TRANSFORM_TO_WERWOLF)
						{
							pItem = g_pInventory->FindItem(ITEM_CLASS_SKULL, 39);
						}
						else
						{
							/*
							#if __CONTENTS(__FAST_TRANSFORTER)
																pItem = g_pInventory->FindItemAll( MItemClassTypeFinder(ITEM_CLASS_VAMPIREWING_ITEM, 0), pSubInventory );
																// ���� ���� ã��
							#endif //__FAST_TRANSFORTER
							#if __CONTENTS(__SECOND_TRANSFORTER)
																if(pItem == NULL)
																	pItem = g_pInventory->FindItemAll( MItemClassTypeFinder(ITEM_CLASS_VAMPIREWING_ITEM, 1), pSubInventory );
							#endif //__SECOND_TRANSFORTER
							*/
#if __CONTENTS(__FAST_TRANSFORTER || __SECOND_TRANSFORTER)
							MVampireWingItemFinder finder_summon_gem;
							pItem = ((MItemManager*)g_pInventory)->FindItemOrderByIndex(finder_summon_gem);
#endif
							if (pItem == NULL)
							{
								MItemClassTypeFinder finder_vamp_etc(ITEM_CLASS_VAMPIRE_ETC, 1);
								pItem = g_pInventory->FindItemAll(finder_vamp_etc, pSubInventory);
							}
						}

						if (pItem != NULL)
						{
							DWORD dwSubInventoryID = 0;
							CGSkillToInventory _CGSkillToInventory;
							_CGSkillToInventory.setObjectID(pItem->GetID());
							_CGSkillToInventory.setX(pItem->GetGridX());
							_CGSkillToInventory.setY(pItem->GetGridY());
							_CGSkillToInventory.setSkillType(m_nSpecialActionInfo);
							if (NULL != pSubInventory)
								dwSubInventoryID = pSubInventory->GetID();

							_CGSkillToInventory.setInventoryItemObjectID(dwSubInventoryID);

							//_CGSkillToInventory.setCEffectID( 0 );	// -_-;;							

							g_pSocket->sendPacket(&_CGSkillToInventory);


							// �ϴ�(!) �׳� ���ְ� ����.
							//(*g_pInventory).RemoveItem( pItem->GetID() );

							//----------------------------------------------------
							// Inventory���� item�� ����ϴ� �� �����ޱ⸦ ��ٸ���.
							//----------------------------------------------------
							SetWaitVerify(MPlayer::WAIT_VERIFY_SKILL_SUCCESS);
							SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY, dwSubInventoryID);

							g_pSkillInfoTable->SetNextAvailableTime(m_nSpecialActionInfo);

							//----------------------------------------------------
							// ��� ��� �õ� ����
							//----------------------------------------------------
							AddNewInventoryEffect(pItem->GetID(),
								m_nSpecialActionInfo, //+ (*g_pActionInfoTable).GetMinResultActionInfo(),
								g_pClientConfig->FPS * 3	// 3��
							);
						}
					}
				}

				return false;
				break;

				//----------------------------------------------------
				//
				// ���� Ǯ��
				// 
				//----------------------------------------------------
			case MAGIC_UN_TRANSFORM:
				if (IsVampire())
				{
					const MSector& sector = m_pZone->GetSector(m_X, m_Y);

					// 2005, 2, 5, sobeit modify start - IsWaitVerify �߰� 
					//if (sector.IsBlockGround())
					if (sector.IsBlockGround() || IsWaitVerify())
						// 2005, 2, 5, sobeit modify end
					{
						m_fNextTrace = FLAG_TRACE_NULL;

						UnSetRepeatAction();

						return false;
					}
				}
				break;

			case SKILL_SUMMON_SYLPH:
				if (IsOusters() && !HasEffectStatus(EFFECTSTATUS_HAS_FLAG)
					//----------------------------------------------------
					// ���� ������ ���� ���..
					//----------------------------------------------------
					&& IsWaitVerifyNULL()
					&& IsItemCheckBufferNULL()
					)
				{
					//MItem* pItem = g_pInventory->FindItem( ITEM_CLASS_OUSTERS_SUMMON_ITEM );

					// 2005, 3, 2, sobeit modify start - FindItem() ��� FindItemAll() ���
					//MItem* pItem = ((MItemManager*)g_pInventory)->FindItem( MOustersSummonGemItemFinder() );

					MItem* pSubInventory = NULL;
					/*						MItem*
					#if __CONTENTS(__FAST_TRANSFORTER)
												pItem = g_pInventory->FindItemAll( MItemClassTypeFinder(ITME_CLASS_OUSTERSWING_ITEM , 0), pSubInventory );
											if(pItem == NULL)
					#endif //__FAST_TRANSFORTER
					#if __CONTENTS(__SECOND_TRANSFORTER)
												pItem = g_pInventory->FindItemAll( MItemClassTypeFinder(ITME_CLASS_OUSTERSWING_ITEM , 1), pSubInventory );
											if(pItem == NULL)
					#endif //__SECOND_TRANSFORTER
												pItem = ((MItemManager*)g_pInventory)->FindItemAll( MOustersSummonGemItemFinder(), pSubInventory );*/
					
					MItem* pItem = NULL;
#if __CONTENTS(__FAST_TRANSFORTER || __SECOND_TRANSFORTER)
					// 1. A wing transport takes priority over summon gems. With more than one wing,
					//    the first by inventory position (top to bottom, left to right) wins, which
					//    FindItemAll can't give: it goes by item map order.
					// 2. Wings can't be used from a sub-inventory, so only the main grid is searched.
					MOustersWingItemFinder finder_wing;
					pItem = ((MItemManager*)g_pInventory)->FindItemOrderByIndex(finder_wing);
					if (pItem == NULL)
#endif
					{
						MOustersSummonGemItemFinder finder_summon_gem;
						pItem = ((MItemManager*)g_pInventory)->FindItemAll(finder_summon_gem, pSubInventory);
					}

					if (pItem != NULL)
					{
						if ((pItem->IsAffectStatus() && pItem->IsChargeItem() && pItem->GetNumber() > 0)
#if __CONTENTS(__FAST_TRANSFORTER||__SECOND_TRANSFORTER)
							|| (pItem->IsAffectStatus() && pItem->GetItemClass() == ITEM_CLASS_OUSTERSWING_ITEM)
#endif //__FAST_TRANSFORTER||__SECOND_TRANSFORTER
							)
						{
							DWORD SubInventoryItemID = 0;

							CGUseItemFromInventory _CGUseItemFromInventory;
							_CGUseItemFromInventory.setObjectID(pItem->GetID());
							_CGUseItemFromInventory.setX(pItem->GetGridX());
							_CGUseItemFromInventory.setY(pItem->GetGridY());

							if (NULL != pSubInventory)
							{
								SubInventoryItemID = pSubInventory->GetID();
							}
							_CGUseItemFromInventory.setInventoryItemObjectID(SubInventoryItemID);

							g_pSocket->sendPacket(&_CGUseItemFromInventory);

							SetItemCheckBuffer(pItem, MPlayer::ITEM_CHECK_BUFFER_USE_FROM_INVENTORY, SubInventoryItemID);
							g_pSkillInfoTable->SetAvailableTime(SKILL_SUMMON_SYLPH, 4000);
						}
					}
					else
					{


					}
					// 2005, 3, 2, sobeit modify end
				}
				return false;
				break;
				//-----------------------------------------------------
				//
				// ����
				//
				//-----------------------------------------------------
			case SKILL_SOUL_CHAIN:
				// �����̾� �����̾� ������ ����.

				if (!IsWaitVerify())
				{
					// UI â�� ����ش�.
					// UI â�� ��� �� �޽����� ���ƿ��� �ű⼭ sendpacket �� ���ش�.

					gC_vs_ui.RunTraceWindow();
					g_pUIDialog->SetLockInputPCTalk();
					//						int skillID = SKILL_SOUL_CHAIN;
					//						
					//						if (skillID < MIN_RESULT_ACTIONINFO)
					//						{
					//							(*g_pSkillInfoTable)[skillID].SetAvailableTime();
					//						}

				}
				return false;

				break;

			case SKILL_LOVE_CHAIN:
				//----------------------------------------------------
				// ���� ������ ���� ���..
				//----------------------------------------------------
				if (!IsWaitVerify() && IsItemCheckBufferNULL() && CurPernalShop() != 1)
				{
					ITEM_CLASS ItemClass;
					switch (GetRace())
					{
					case RACE_SLAYER:
						ItemClass = ITEM_CLASS_COUPLE_RING;
						break;
					case RACE_VAMPIRE:
						ItemClass = ITEM_CLASS_VAMPIRE_COUPLE_RING;
						break;
					case RACE_OUSTERS:
						ItemClass = ITEM_CLASS_OUSTERS_HARMONIC_PENDENT; // ;
						break;
					default:
						ItemClass = ITEM_CLASS_COUPLE_RING;
						break;
					}
					MPlayerGear* pGear = GetGear();

					//���������� ������ ������ Ŀ��ü���� ����Ҽ� ����.
					MItem* pWarItem = g_pInventory->FindItem(ITEM_CLASS_WAR_ITEM);
					if (pWarItem != NULL)
						break;

					MItemClassFinder itemFinder(ItemClass);


					MItem* pItem = pGear->FindItem(itemFinder);
					if (pItem == NULL || !pItem->IsAffectStatus())
						return false;

					CGUseItemFromGear _CGUseItemFromGear;

					_CGUseItemFromGear.setObjectID(pItem->GetID());
					_CGUseItemFromGear.setPart(pItem->GetItemSlot());

					g_pSocket->sendPacket(&_CGUseItemFromGear);

					SetItemCheckBuffer(pItem, ITEM_CHECK_BUFFER_USE_FROM_GEAR);
					return false;
				}
				break;
			}

			if ((*g_pActionInfoTable)[m_nSpecialActionInfo].GetParentActionInfo() != ACTIONINFO_NULL)
				m_nSpecialActionInfo = (*g_pActionInfoTable)[m_nSpecialActionInfo].GetParentActionInfo();

			// ����� ����� ����
			m_nUsedActionInfo = m_nSpecialActionInfo;

			// ���� ���� �ʱ�ȭ
			SetTraceID(m_ID);
			m_fTrace = FLAG_TRACE_SELF;
			m_TraceX = m_X;//SECTORPOSITION_NULL;
			m_TraceY = m_Y;//SECTORPOSITION_NULL;
			m_TraceZ = 0;

			// ���� �����ϴ� ��� ���ؼ� ����صд�.
			m_fTraceBuffer = m_fTrace;

			//------------------------------------------------------------
			// BasicAction�� ���Ŀ� ����..
			// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶��� ���̴�.
			//------------------------------------------------------------

			m_TraceDistance = 0;

			//------------------------------------------------------------
			// ��� ��� Actionǥ��
			SetNextAction(GetActionInfoAction(m_nUsedActionInfo, true));

			// buffering�� ���ش�.
			m_fNextTrace = FLAG_TRACE_NULL;

			//------------------------------------------------------------
			// message���
			//------------------------------------------------------------
			DEBUG_ADD_FORMAT("Self SpecialAction");
		}
		else
		{
			//------------------------------------------------------------
			// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
			//------------------------------------------------------------
			if (m_bRepeatAction)
			{
				m_fNextTrace = FLAG_TRACE_NULL;

				return false;
			}
			else
			{
				m_fNextTrace = FLAG_TRACE_SELF;

				m_bRepeatAction = FALSE;

				return false;
			}
		}

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Keep Trace Creature
//----------------------------------------------------------------------
// �����ϴ� Creature�� ��� �����Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::KeepTraceCreature()
{
	if (IsInDarkness())
	{
		TraceNULL();
		SetStop();
		UnSetRepeatAction();

		return;
	}

	// �����ϴ� Creature�� ������ �о�´�.
	MCreature* pCreature = m_pZone->GetCreature(m_TraceID);

	//GET_DYING_CREATURE( pCreature, m_TraceID );

	bool	bTraceTimer = false;

	if (m_TraceTimer != 0)
	{
		if ((GetTickCount() - m_TraceTimer) / 1000 > g_pClientConfig->TRACE_CHARACTER_LIMIT_TIME)
		{
			if (pCreature != NULL &&					// ������ ��츸.
				(pCreature->IsSlayer() && IsSlayer()) ||
				(pCreature->IsVampire() && IsVampire()) ||
				(pCreature->IsOusters() && IsOusters())
				)
			{
				bTraceTimer = true;
				m_TraceTimer = 0;
			}
		}
	}

	//-------------------------------------------------------
	// �����ϴ� Creature�� ������� ��� --> ���� ����
	// ���� Slayer�� ���� Darkness�ȿ� �� ĳ���� �Ѿư� �� ����.
	//-------------------------------------------------------
	if ((pCreature == NULL ||
		pCreature->IsInDarkness() && !pCreature->IsNPC() &&
		(!IsVampire() && !(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) ||
			IsVampire() && g_pZone->GetID() == 3001)
		|| bTraceTimer
		|| pCreature->IsInGroundElemental() && pCreature->IsOusters() && !g_pPlayer->IsOusters())
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		// ���� ����
		TraceNULL();

		// �����.
		SetStop();

		UnSetRepeatAction();
		m_TraceTimer = 0;

		return;
	}

	if (pCreature)
	{
		if (max(abs(pCreature->GetX() - m_X),
			abs(pCreature->GetY() - m_Y)) > 1)
		{
			TraceCreatureToBasicAction(m_TraceID, false);

			g_pTopView->SetSelectedSectorNULL();
		}
	}
}


//----------------------------------------------------------------------
// Get ActionInfo Range
//----------------------------------------------------------------------
// nActionInfo�� ��� ���� �Ÿ���?
//----------------------------------------------------------------------
int
MPlayer::GetActionInfoRange(TYPE_ACTIONINFO nActionInfo)
{
	if (nActionInfo == SKILL_HEAD_SHOT)
		return 3;

	if ((*g_pActionInfoTable)[nActionInfo].GetParentActionInfo() != ACTIONINFO_NULL)
		nActionInfo = (*g_pActionInfoTable)[nActionInfo].GetParentActionInfo();

	//-------------------------------------------------------------
	// �⺻ action�� ������ �޴°�?
	//-------------------------------------------------------------
	if ((*g_pActionInfoTable)[nActionInfo].IsAffectCurrentWeaponRange())
	{
		return m_BasicAttackDistance;
		//nActionInfo = m_nBasicActionInfo;
	}

	SKILLINFO_NODE& skillInfo = (*g_pSkillInfoTable)[nActionInfo];

	int minRange = skillInfo.minRange;
	int maxRange = skillInfo.maxRange;
	int skillLevel = skillInfo.GetExpLevel();

#ifdef __METROTECH_TEST__
	if (g_bLight)
		return maxRange;
#endif

	int maxLevel = 100;	// �����̾�� ��ų������ 100���� �ø���.

	if (IsOusters())
		maxLevel = 30;	// �ƿ콺����� ��ų������ 30���� �ø���.

	// Ư�� ��ų�� ���� Range ���

	switch (nActionInfo)
	{
	case MAGIC_RAPID_GLIDING:
		return (min(6, (2 + (GetDEX() / 50))));
		break;

	case MAGIC_BLOODY_ZENITH:
		return min(6, 2 + GetSTR() / 80 + GetDEX() / 120 + GetINT() / 240);
		break;

	case SKILL_SOUL_REBIRTH:
		return min(5, 2 + (*g_pSkillInfoTable)[SKILL_SOUL_REBIRTH_MASTERY].GetExpLevel() / 10);
		break;
	}

	return (int)(minRange + (maxRange - minRange) * skillLevel / maxLevel);

}

//----------------------------------------------------------------------
// Trace Creature To BasicAction
//----------------------------------------------------------------------
// �⺻��������(L-Click) �� ���� Creature�� �����Ͽ� �����Ѵ�.
// �����ִ� ���°� �ƴϸ�.. Next�� �����صд�.
//----------------------------------------------------------------------
bool
MPlayer::TraceCreatureToBasicAction(TYPE_OBJECTID id, bool bForceAttack, bool bClick)
{
	//	if(bForceAttack && !g_pDXInput->KeyDown(DIK_LSHIFT))
	//	{
	//		bForceAttack = false;
	//	}

	if ((HasEffectStatus(EFFECTSTATUS_SLEEP) || HasEffectStatus(EFFECTSTATUS_BLOCK_HEAD))
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
		return false;

	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	if (!IsNotDelay()
		|| IsInCasket()	// [�����3]
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE)
		|| IsInDarkness() &&
		!(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		return false;
	}

	// Dracula Castle: the Mihnea's bearer cannot attack. The server refuses it, so do not start the swing at all.
	if (g_pEffectStatusTable->GetSize() > EFFECTSTATUS_HAS_MIHNEA && HasEffectStatus(EFFECTSTATUS_HAS_MIHNEA))
	{
		MCreature* pMihneaTarget = m_pZone->GetCreature(id);
		if (pMihneaTarget != NULL && !pMihneaTarget->IsNPC() && pMihneaTarget->GetID() != GetID())
			return false;
	}
	if (m_nBasicActionInfo == ACTIONINFO_NULL)
		return false;

	if (m_nUsedActionInfo != ACTIONINFO_NULL)
		return false;

	if (IsStop()) // || m_Action!=ACTION_ATTACK)
	{
		// Zone�� �����ϴ� Creature���� check�Ѵ�.
		MCreature* pCreature = m_pZone->GetCreature(id);
		if (pCreature == NULL)
		{
			MFakeCreature* pFakeCreature = (MFakeCreature*)m_pZone->GetFakeCreature(id);
			if (pFakeCreature != NULL && pFakeCreature->GetOwnerID() != OBJECTID_NULL)	// ���̴�
			{
				MPetItem* pPetItem = pFakeCreature->GetPetItem();
				if (pPetItem != NULL)
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

					if (pPetItem->GetItemOptionListCount() == 0)
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
						const std::list<TYPE_ITEM_OPTION>& optionList = pPetItem->GetItemOptionList();
						std::list<TYPE_ITEM_OPTION>::const_iterator itr = optionList.begin();

						for (; itr != optionList.end() && *itr < g_pItemOptionTable->GetSize(); itr++)
						{
							petInfo.OPTION_LIST.push_back(*itr);
						}
					}
#endif //__NEW_PET_INCUBUS

					petInfo.ITEM_TYPE = pPetItem->GetItemType();
					petInfo.NAME = pPetItem->GetPetName();
					petInfo.ITEM_ID = pPetItem->GetID();
					petInfo.ATTACK = pPetItem->IsCanAttack();
					petInfo.NICK_NAME = pFakeCreature->GetNickName();
					petInfo.ChangeAbleNickName = pFakeCreature->GetOwnerID() == g_pPlayer->GetID();
					petInfo.ENCHANTSKILLTYPE = pPetItem->GetEnchantSkillType();

#if __CONTENTS(__FUNCTION_PETFOOD)
					petInfo.FUNCTION_PETFOODTYPE = pPetItem->getFunctionPetfoodType();
#endif //__FUNCTION_PETFOOD
#if __CONTENTS(__PET_MIXINGFORGE)
					const std::list<TYPE_ITEM_OPTION>& petMixingOption = pPetItem->GetPetMixingOptionList();
					std::list<TYPE_ITEM_OPTION>::const_iterator itr = petMixingOption.begin();

					for (; itr != petMixingOption.end(); ++itr)
					{
						petInfo.MIXINGOPTION.push_back(*itr);
					}
#endif //__PET_MIXINGFORGE
					UI_RunPetInfo(&petInfo);
				}
				return false;
			}
		}

		// 2004,5, 28 sobeit add start - �ٸ� ĳ���� ����
		if (g_pTopView->GetRequestMode() == MRequestMode::REQUEST_INFO)
		{
			SetTraceID(id);
			SetRequestMode(g_pTopView->GetRequestMode());
			BasicActionToCreature();
			SetTraceID(NULL);
			return false;
		}
		// 2004,5, 28 sobeit add end
		//GET_DYING_CREATURE( pCreature, id );

		// creature�� ���� ���
		// creature�� �̹� ���� ���
		// ���� Slayer�� ���� Darkness�ȿ� �� ĳ���� �Ѿư� �� ����.	

		if (pCreature != NULL)
		{
			if (pCreature->GetObjectType() == MObject::TYPE_CREATURE)
			{
				if (IsSlayer() && g_pZone->GetSector(GetX(), GetY()).IsSafeSlayer() &&
					pCreature->IsSlayer() && !g_pZone->GetSector(pCreature->GetX(), pCreature->GetY()).IsSafeSlayer() && pCreature->IsPlayerOnly() ||
					IsVampire() && g_pZone->GetSector(GetX(), GetY()).IsSafeVampire() &&
					pCreature->IsVampire() && !g_pZone->GetSector(pCreature->GetX(), pCreature->GetY()).IsSafeVampire() && pCreature->IsPlayerOnly() ||
					IsOusters() && g_pZone->GetSector(GetX(), GetY()).IsSafeOusters() &&
					pCreature->IsOusters() && !g_pZone->GetSector(pCreature->GetX(), pCreature->GetY()).IsSafeOusters() && pCreature->IsPlayerOnly()
					)
				{
					//						UnSetLockMode();
					UnSetRepeatAction();
					//						g_pTopView->SetSelectedNULL();
					return false;
				}
			}
		}

		//		bool bOusters = IsOusters();
		//		MItem *pOustersItem = g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND);
		if (pCreature == NULL
			|| pCreature->IsDead()
			|| pCreature->IsInDarkness() && !pCreature->IsNPC() &&
			(!IsVampire() && !(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) ||
				IsVampire() && g_pZone->GetID() == 3001)
			|| pCreature->IsInGroundElemental() && pCreature->IsOusters() && !g_pPlayer->IsOusters()
			&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
			&& !g_bLight
#endif
			)
		{
			UnSetRepeatAction();

			m_fTraceBuffer = FLAG_TRACE_NULL;
			m_fNextTrace = FLAG_TRACE_NULL;
			return false;
		}


		bool	bTraceCreatureToForceAttack;
		bool	bKeepTraceCreature;

		bool bOusters = IsOusters();
		MItem* pOustersItem = g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND);

		//------------------------------------------------
		// Request�ҷ��� �Ѿư��� ���
		//------------------------------------------------
		if (g_pTopView->IsRequestMode()
			// Player�� �ȴ�.
			//&& pCreature->GetCreatureType()<=CREATURETYPE_VAMPIRE_FEMALE
			&& (*g_pCreatureSpriteTable)[(*g_pCreatureTable)[m_CreatureType].SpriteTypes[0]].IsPlayerOnlySprite()
			// �� �ӿ� �ִ� ���� �ȵȴ�.
			&& !pCreature->IsInCasket()
			// ���� �ִ� ���� �ȵȴ�.
			&& !pCreature->IsUndergroundCreature()
			&& pCreature->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR
			&& pCreature->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR
			&& pCreature->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR

			// ���� ���㳪 ���밡 �ƴ� ���
			//&& m_CreatureType!=CREATURETYPE_BAT
			//&& m_CreatureType!=CREATURETYPE_WOLF
			)
		{
			bTraceCreatureToForceAttack = false;
			bKeepTraceCreature = false;
			m_bNextForceAttack = false;

			SetRequestMode(g_pTopView->GetRequestMode());
		}
		//------------------------------------------------
		// ������̸� Ÿ�� ������ ������ �ȵȴ�.
		// ���� , ����Ʈ �� ��� ������ �ȵȴ�.
		//------------------------------------------------
		else if (m_MoveDevice == MOVE_DEVICE_RIDE
			|| m_MoveDevice == MOVE_DEVICE_SUMMON_SYLPH || HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this))
			|| m_CreatureType == CREATURETYPE_BAT
			|| m_CreatureType == CREATURETYPE_VAMPIRE_GHOST
#if __CONTENTS(__FAST_TRANSFORTER)
			|| (m_MoveDevice == MOVE_DEVICE_WING_SYLPH && HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this)))
			|| m_CreatureType == CREATURETYPE_FLITTERMOUSE
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
			|| (m_CreatureType == MOVE_DEVICE_WING_SYLPH && HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this)))
			|| m_CreatureType == CREATURETYPE_SHAPE_OF_DEMON
#endif //__SECOND_TRANSFORTER
			|| !(bOusters && pOustersItem != NULL && pOustersItem->GetItemClass() == ITEM_CLASS_OUSTERS_CHAKRAM || !bOusters)
			|| CanActionByZoneInfo()
			)
		{
			bTraceCreatureToForceAttack = false;
			bKeepTraceCreature = true;

			UnSetRequestMode();
		}
		//------------------------------------------------
		// ���� �����ΰ�?
		//------------------------------------------------		
		else if (bForceAttack)
		{
			//			if(! (IsSlayer() && g_pZone->GetSector( GetX(), GetY() ).IsSafeSlayer() && pCreature->IsSlayer() && pCreature->IsPlayerOnly()|| 
			//				IsVampire() && g_pZone->GetSector( GetX(), GetY() ).IsSafeVampire() && pCreature->IsVampire() && pCreature->IsPlayerOnly()) )
			bTraceCreatureToForceAttack = true;

			bKeepTraceCreature = false;

			UnSetRequestMode();
		}
		//------------------------------------------------
		// �ƴϸ�.. ������ ����..
		//------------------------------------------------
		else
		{
			bTraceCreatureToForceAttack = CanAttackTribe(pCreature)
				// �����̾��� ��쿡�� Guild�� ���� ������ �� �ְų�..
				|| IsVampire() && CanAttackGuild(pCreature)
				|| g_pObjectSelector->IsWarEnemy(pCreature);
			//	|| m_pZone->IsFreePKZone();			// -_- �ٳ���


// ��������� �Ǵ°�?
			bTraceCreatureToForceAttack |= g_pJusticeAttackManager->HasCreature(pCreature->GetName());

			// ��� �Ѿư��°�? - ������ �ƴ϶��..
			bKeepTraceCreature = !bTraceCreatureToForceAttack;
		}

		// 2005, 1, 8, sobeit add start - ���� �����̾� �� ���� ��æ�� �Ϲ� ������ �� �� ����. ������ ����..-_-
		// 2006.10.18 sjheon  ���⿡ �ּ�ó�� 

		if (IsAdvancementClass())
		{
			const MCreatureWear::ADDON_INFO& TempaddonInfo = GetAddonInfo(ADDON_RIGHTHAND);
			MCreature* pCreature = m_pZone->GetCreature(id);
			if (pCreature != NULL)
			{
				if (!pCreature->IsNPC() && IsSlayer() && (!TempaddonInfo.bAddon || TempaddonInfo.ItemClass == ITEM_CLASS_CROSS || TempaddonInfo.ItemClass == ITEM_CLASS_MACE)
					&& bTraceCreatureToForceAttack)
				{
					bTraceCreatureToForceAttack = false;
					bKeepTraceCreature = true;

					UnSetRequestMode();
					//return false;
				}
			}
		}

		// 2005, 1, 8, sobeit add end
		//------------------------------------------------
		// �� ������ ��� 
		// --> źâ�� �����Ǿ� �ִ� ��츦 üũ�ϸ� �ȴ�.
		//------------------------------------------------
		if (bKeepTraceCreature)
		{
			// �����ϴ� ��� üũ�� ��...
		}
		// NPC�� �ƴ϶�� �����ҷ��°ǵ�... �Ѿ� üũ�ؾ� �Ѵ�.
		else if (!pCreature->IsNPC())
		{
			if (!IsRequestMode() && IsInSafeSector() == 2)
				return false;

			// g_pCurrentMagazine�� Ȯ���� NULL�� �����ϸ� �ȴ�.
			//if (IsSlayer())
			{
				//	const MItem* pWeapon = g_pSlayerGear->GetItem( MSlayerGear::GEAR_SLAYER_RIGHTHAND );

					//------------------------------------------------
					// �Ѿ� üũ
					//------------------------------------------------
				//	if (pWeapon!=NULL					// ���Ⱑ �ְ�
				//		&& pWeapon->IsGunItem()			// ���̰�
					//	&&	

				if (
					(IsOusters() &&
						g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND) != NULL &&
						g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND)->GetItemClass() == ITEM_CLASS_OUSTERS_CHAKRAM ||
						!IsOusters())
					&& (*g_pActionInfoTable)[m_nBasicActionInfo].IsWeaponTypeGunAny()
					)
				{
					if (!IsRequestMode()

						&& g_pCurrentMagazine != NULL)		// �Ѿ��� �ʿ� ���� ���
					{
						if (g_pCurrentMagazine->GetNumber() == 0)	// �Ѿ��� ���� ���
						{
							// ���� ����ϴµ� źâ�� ���� ���
							PlaySound(SOUND_ITEM_NO_MAGAZINE);

							m_fNextTrace = FLAG_TRACE_NULL;

							// źâ ������ delay
							m_DelayTime = g_CurrentTime + GetActionInfoDelay(m_nBasicActionInfo);

							// [����] �Ѿ� �� ���� ��
//							__BEGIN_HELP_EVENT
//								ExecuteHelpEvent( HE_ITEM_EMPTY_BULLET );
//							__END_HELP_EVENT

							m_fNextTrace = FLAG_TRACE_NULL;

							UnSetRepeatAction();


							return false;
						}
					}
				}
			}

			// ����� ����� ����
			m_nNextUsedActionInfo = GetBasicActionInfo(); //m_nBasicActionInfo;
		}

		// ���� ���� ����
		SetTraceID(id);

		if (!bForceAttack && bClick)
		{
			m_TraceTimer = GetTickCount();
		}

		m_fTrace = FLAG_TRACE_CREATURE_BASIC;
		m_TraceX = pCreature->GetX();
		m_TraceY = pCreature->GetY();
		m_TraceZ = pCreature->GetZ();

		//------------------------------------------------------------
		// ���� �����ΰ�?
		// ��� �����ϴ°�?
		//------------------------------------------------------------
		m_bTraceCreatureToForceAttack = bTraceCreatureToForceAttack;
		m_bKeepTraceCreature = bKeepTraceCreature;

		//------------------------------------------------------------
		// ���� ��ǥ �Ÿ�..
		//------------------------------------------------------------
		// BasicAction�� ���Ŀ� ����..
		// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶��� ���̴�.
		//
		// ���󰡴� ���̸� 1�̰�
		// �ƴϸ� �⺻ ���ݿ� ����..
		//
		// NPC�� ���� �Ÿ��� �޶��� �� �ִ�.
		//------------------------------------------------------------
		if (m_bKeepTraceCreature)
		{
			m_TraceDistance = 1;
		}
		else
		{
			m_TraceDistance = m_BasicAttackDistance;
		}

		// NPC 
		if (pCreature->IsNPC())
		{
			if (pCreature->GetCreatureType() == 659)	// ���������� ���� ��ĭ
				m_TraceDistance = 1;
			else
				m_TraceDistance = TRADE_DISTANCE_NPC;
		}
		// Trade�ҷ��� �Ѿư��� ���
		else if (IsRequestMode())
		{
			m_TraceDistance = TRADE_DISTANCE_PC;
		}

		// ���� �����ϴ� ��� ���ؼ� ����صд�.
		m_fTraceBuffer = m_fTrace;
		m_TraceIDBuffer = id;

		//------------------------------------------------------------

		if (HasEffectStatus(EFFECTSTATUS_BLOCK_HEAD))
			m_bTraceCreatureToForceAttack = false;


		// �����ϴ� �� ����
		SetAction(m_MoveAction);
		SetNextDestination(m_TraceX, m_TraceY);

		// buffering�� ���ش�.
		m_fNextTrace = FLAG_TRACE_NULL;

		//------------------------------------------------------------
		// message���
		//------------------------------------------------------------
		DEBUG_ADD_FORMAT("Trace Creature To BasicAction : ID=%d, (%d, %d)", id, m_TraceX, m_TraceY);

		//------------------------------------------------------------
		// return false�ϸ� �ݺ� action�� ���ϰ� �ȴ�.
		//------------------------------------------------------------
		if (m_bKeepTraceCreature)// || m_bTraceCreatureToForceAttack)
		{
			return false;
		}

		// ���õ� sector ���ֱ�
		g_pTopView->SetSelectedSectorNULL();


		return true;
	}
	else
	{
		//------------------------------------------------------------
		// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
		//------------------------------------------------------------
		if (m_bRepeatAction)
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			return false;
		}
		else
		{
			m_fNextTrace = FLAG_TRACE_CREATURE_BASIC;
			m_NextTraceID = id;
			m_bNextForceAttack = bForceAttack;

			m_TraceIDBuffer = id;

			m_bRepeatAction = FALSE;

			m_fTraceBuffer = FLAG_TRACE_NULL;

			return true;
		}
	}

	return true;
}


//----------------------------------------------------------------------
// Trace Creature To SpecialAction
//----------------------------------------------------------------------
// Ư����������(R-Click) �� ���� Creature�� �����Ͽ� �����Ѵ�.
// �����ִ� ���°� �ƴϸ�.. Next�� �����صд�.
//----------------------------------------------------------------------
// [return��]
//
//  �ٸ� Creature���� ������� ���ϴ� ��츸 false�� return�Ѵ�. 
//
//----------------------------------------------------------------------
bool
MPlayer::TraceCreatureToSpecialAction(TYPE_OBJECTID id, bool bForceAttack)
{
	SetActionDelay(0);
	// Dracula Castle: the Mihnea's bearer cannot attack. The server refuses it, so do not start the swing at all.
	if (g_pEffectStatusTable->GetSize() > EFFECTSTATUS_HAS_MIHNEA && HasEffectStatus(EFFECTSTATUS_HAS_MIHNEA))
	{
		MCreature* pMihneaTarget = m_pZone->GetCreature(id);
		if (pMihneaTarget != NULL && !pMihneaTarget->IsNPC() && pMihneaTarget->GetID() != GetID())
			return false;
	}

	if ((HasEffectStatus(EFFECTSTATUS_SLEEP) || HasEffectStatus(EFFECTSTATUS_BLOCK_HEAD))
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
		return false;

	//	if(bForceAttack && !g_pDXInput->KeyDown(DIK_LSHIFT))
	//	{
	//		bForceAttack = false;
	//	}

		//-----------------------------------------------------
		// delay�� ������ �ȵȴ�.
		//-----------------------------------------------------
	if (!IsNotDelay()
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE)
		|| IsInCasket()	// [�����3]
		|| IsInDarkness() && (!(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || g_pZone->GetID() == 3001)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		return false;
	}

	if (m_nSpecialActionInfo == ACTIONINFO_NULL
		// ������̸� Ÿ�� ������ Ư�������� �ȵȴ�.
		|| m_MoveDevice == MOVE_DEVICE_RIDE
		// ���� ��� ����� ���� �޾ƾ� �ϸ� ����� �� ����.
		//|| m_WaitVerify != WAIT_VERIFY_NULL)	// 2001.8.20 �ּ�ó��
		|| IsInSafeSector() == 2
		// ���� �������뿡���� ����� ����� �� ����.
		|| CanActionByZoneInfo()
		|| HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this))				// �޶���� ���Ѵ�~ 
#if __CONTENTS(__FAST_TRANSFORTER)
		|| HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this))
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		|| HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this))
#endif //__SECOND_TRANSFORTER
		)
	{
		m_fNextTrace = FLAG_TRACE_NULL;

		UnSetRepeatAction();

		return false;
	}


	// �ٸ� ����� ������� ���
	if (m_nUsedActionInfo != ACTIONINFO_NULL)
		return false;

	// 2006.08.22 sjheon
	if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WOLF))
	{
		if (!(m_nSpecialActionInfo == MAGIC_HOWL || m_nSpecialActionInfo == MAGIC_EAT_CORPSE || m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_BAT))
	{
		if (!(m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WERWOLF))
	{
		if (!(m_nSpecialActionInfo == SKILL_BITE_OF_DEATH ||
			m_nSpecialActionInfo == MAGIC_RAPID_GLIDING ||
			m_nSpecialActionInfo == SKILL_VIOLENT_PHANTOM ||
			m_nSpecialActionInfo == SKILL_WILD_WOLF ||
			m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}

	//-------------------------------------------------------
	// Ÿ�ο��� ����� �� �ִ� �͵鸸 ����Ѵ�.
	//-------------------------------------------------------

	if ((*g_pActionInfoTable)[m_nSpecialActionInfo].IsTargetOther())
	{
		int useSkill = m_nSpecialActionInfo;
		int originalSkill = m_nSpecialActionInfo;

		//		// 2004, 6, 9 sobeit add start - �׽�Ʈ, ���� ���� �޼��� �´㿡 �����ϰ�..-_-
		//		if(m_nSpecialActionInfo == SKILL_BLOOD_DRAIN)
		//		{
		//			m_fNextTrace = FLAG_TRACE_NULL;
		//			//UnSetRepeatAction();
		//			if(g_pSkillAvailable->IsEnableSkill( (ACTIONINFO)m_nSpecialActionInfo ) && m_TraceID != -1)
		//			{
		//				CGBloodDrain _CGBloodDrain;
		//				_CGBloodDrain.setObjectID( m_TraceID );
		//				g_pSocket->sendPacket( &_CGBloodDrain );
		//				DEBUG_ADD("���� �޼��� ����");
		//			}
		//			return false;
		//		}
		//		// 2004, 6, 9 sobeit add end - �׽�Ʈ, ���� ���� �޼��� �´㿡 �����ϰ�..-_-

		if (m_nSpecialActionInfo == SKILL_BLITZ_SLIDING_ATTACK ||
			m_nSpecialActionInfo == SKILL_BLAZE_WALK_ATTACK
			)
		{
			m_fNextTrace = FLAG_TRACE_NULL;
			UnSetRepeatAction();

			return false;
		}

		int grade = 0;

		if (IsOusters())
		{
			grade = (*g_pSkillInfoTable)[originalSkill].GetExpLevel() / 15;
			DEBUG_ADD_FORMAT("OUSTERS LEVEL : %d %d", originalSkill, (*g_pSkillInfoTable)[originalSkill].GetExpLevel());
		}


		if (grade > 0 && (*g_pActionInfoTable)[originalSkill].IsUseActionStep())
			useSkill = (*g_pActionInfoTable)[originalSkill].GetActionStep(grade - 1);

		// Sjheon 2005.07.01 Combo ��� ó�� Add
		/*if((*g_pActionInfoTable)[originalSkill].GetComboAttackSkill())
		{
			m_ComboCnt = GetComboEndValue();
			//OutputDebugString("TraceCreatureToSpecialAction()\n") ;
		}
		else
		{
			//m_ComboCnt = 0 ;
			if(originalSkill != SKILL_HALO  && originalSkill != SKILL_SHARP_HAIL && originalSkill != SKILL_DRAGON_TORNADO &&
			   originalSkill != SKILL_HIT_CONVERT  && originalSkill != SKILL_SWORD_OF_THOR && originalSkill != SKILL_BLITZ_SLIDING &&
			   originalSkill != SKILL_MENTAL_SWORD  && originalSkill != SKILL_BLAZE_WALK && originalSkill != SKILL_BLAZE_WALK_2 && originalSkill != SKILL_BLITZ_SLIDING &&
			   originalSkill != SKILL_SHADOW_DANCING  && originalSkill != SKILL_POWER_OF_LAND && originalSkill != SKILL_TYPHOON
			)
				m_ComboCnt = 0 ;


			if( grade>0 && (*g_pActionInfoTable)[originalSkill].IsUseActionStep())
				useSkill = (*g_pActionInfoTable)[originalSkill].GetActionStep( grade - 1 );
		}*/
		// Sjheon 2005.07.01 Combo ��� ó�� End


		// 2004, 9, 20, sobeit add start
		if ((*g_pActionInfoTable)[useSkill].IsMasterySkillStep())
		{
			int TempSkill = (*g_pActionInfoTable)[useSkill].GetActionStep((*g_pActionInfoTable)[useSkill].GetMasterySkillStep());
			if (g_pSkillAvailable->IsEnableSkill((ACTIONINFO)TempSkill))
				useSkill = TempSkill;
		}
		// 2004, 9, 20, sobeit add end
		if ((*g_pActionInfoTable)[useSkill].GetParentActionInfo() != ACTIONINFO_NULL)
			originalSkill = (*g_pActionInfoTable)[useSkill].GetParentActionInfo();

		DEBUG_ADD_FORMAT("SKILL : %d %d", originalSkill, useSkill);
		//-------------------------------------------------------
		// ���� ����� �� �ִ� ������� üũ..
		// Passive ��ų�̸� ��� ���ϰ�..
		//-------------------------------------------------------
		if (!g_pSkillAvailable->IsEnableSkill((ACTIONINFO)originalSkill)
			|| !(*g_pSkillInfoTable)[originalSkill].IsEnable()
			|| !(*g_pSkillInfoTable)[originalSkill].IsAvailableTime()
			|| (*g_pSkillInfoTable)[originalSkill].IsPassive())
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			UnSetRepeatAction();

			return false;
		}

		if (IsStop()) // || m_Action!=ACTION_ATTACK)
		{
			// Zone�� �����ϴ� Creature���� check�Ѵ�.
			MCreature* pCreature = m_pZone->GetCreature(id);

			//GET_DYING_CREATURE( pCreature, id );

			// creature�� ���� ���
			// ���� Slayer�� ���� Darkness�ȿ� �� ĳ���� �Ѿư� �� ����.
			if (pCreature == NULL
				// ��ü���� �� �� �ִ� ��ų ������ �߰��ؾ��Ѵ�. FUCK!
				|| pCreature->IsDead()
				&& originalSkill != MAGIC_RESURRECT && originalSkill != SKILL_TRANSFUSION
				&& originalSkill != SKILL_SOUL_REBIRTH
				&& originalSkill != SKILL_BLOODY_WINGS
				|| pCreature->IsInGroundElemental() && pCreature->IsOusters() && !g_pPlayer->IsOusters()
				// 2004, 10, 14, sobiet modify start - ����Ʈ�Ͻ��ɷ����� ��ũ�Ͻ� �ȿ� �༮ ������ �ȵȴٰ� �ؼ� ����..
//				|| pCreature->IsInDarkness() && !pCreature->IsNPC() && 
//				!(IsVampire() && !(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))  ||
//				g_pZone->GetID() != 3001 && IsVampire() )
//				&& !g_pPlayer->HasEffectStatus( EFFECTSTATUS_GHOST ) 
|| pCreature->IsInDarkness() && !pCreature->IsNPC() && !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST) &&
(!IsVampire() && !(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || g_pZone->GetID() == 3001 && IsVampire())
// 2004, 10, 14, sobiet modify end- ����Ʈ�Ͻ��ɷ����� ��ũ�Ͻ� �ȿ� �༮ ������ �ȵȴٰ� �ؼ� ����..
#ifdef __METROTECH_TEST__
&& !g_bLight
#endif
|| (*g_pActionInfoTable)[originalSkill].GetUser() == FLAG_ACTIONINFO_USER_SLAYER && (!pCreature->IsSlayer() || !pCreature->IsPlayerOnly())
|| (*g_pActionInfoTable)[originalSkill].GetUser() == FLAG_ACTIONINFO_USER_VAMPIRE && (!pCreature->IsVampire() || !pCreature->IsPlayerOnly())
|| (*g_pActionInfoTable)[originalSkill].GetUser() == FLAG_ACTIONINFO_USER_OUSTERS && (!pCreature->IsOusters() || !pCreature->IsPlayerOnly())
|| (*g_pActionInfoTable)[originalSkill].GetUser() == FLAG_ACTIONINFO_USER_MONSTER && (!pCreature->IsVampire() || pCreature->IsPlayerOnly())

|| IsSlayer() && !(*g_pActionInfoTable)[originalSkill].IsUserSlayer() && pCreature->IsSlayer() && pCreature->IsPlayerOnly() && !bForceAttack
|| IsVampire() && !(*g_pActionInfoTable)[originalSkill].IsUserVampire() && pCreature->IsVampire() && pCreature->IsPlayerOnly() && !bForceAttack
|| IsOusters() && !(*g_pActionInfoTable)[originalSkill].IsUserOusters() && pCreature->IsOusters() && pCreature->IsPlayerOnly() && !bForceAttack
)
			{
				// ������� ���� ACTIONINFO_NULL�ε�..
				// UnSetRepeatAction()���� ����� ó���ϱ� ���ؼ�.. - -;;
				// ��ư �ᰡ���.. - -;;
				if (IsRepeatAction())
				{
					m_nUsedActionInfo = originalSkill;

					UnSetRepeatAction();

					m_nUsedActionInfo = ACTIONINFO_NULL;
				}
				else
				{
					UnSetRepeatAction();
				}

				m_fNextTrace = FLAG_TRACE_NULL;
				m_fTraceBuffer = FLAG_TRACE_NULL;
				return false;
			}

			if (pCreature != NULL)
			{
				if (pCreature->GetObjectType() == MObject::TYPE_CREATURE)
				{
					if (IsSlayer() && g_pZone->GetSector(GetX(), GetY()).IsSafeSlayer() &&
						pCreature->IsSlayer() && !g_pZone->GetSector(pCreature->GetX(), pCreature->GetY()).IsSafeSlayer() && pCreature->IsPlayerOnly() ||
						IsVampire() && g_pZone->GetSector(GetX(), GetY()).IsSafeVampire() &&
						pCreature->IsVampire() && !g_pZone->GetSector(pCreature->GetX(), pCreature->GetY()).IsSafeVampire() && pCreature->IsPlayerOnly() ||
						IsOusters() && g_pZone->GetSector(GetX(), GetY()).IsSafeOusters() &&
						pCreature->IsOusters() && !g_pZone->GetSector(pCreature->GetX(), pCreature->GetY()).IsSafeOusters() && pCreature->IsPlayerOnly())
					{
						//						UnSetLockMode();
						UnSetRepeatAction();
						//						g_pTopView->SetSelectedNULL();
						return false;
					}
				}
			}

			bool bTraceCreatureToForceAttack = true;
			bool bKeepTraceCreature = false;

			//-------------------------------------------------------
			// NPC�� ���.. �׳� �����ϰ� �Ѵ�.
			//-------------------------------------------------------
			if (pCreature->IsNPC())
			{
				TraceCreatureToBasicAction(id, false);

				return false;
			}
			else
				if (bForceAttack)
				{
					bTraceCreatureToForceAttack = true;
					bKeepTraceCreature = false;

					UnSetRequestMode();
				}


			//-------------------------------------------------------
			// Ư�� ����� ���� üũ - ���� ���ߵǴµ�. �� �׷�����.. --;
			//-------------------------------------------------------
			switch (originalSkill)
			{
				//-------------------------------------------------------
				// SKILL_TRANSFUSION
				//-------------------------------------------------------
			case SKILL_TRANSFUSION:

				//--------------------------------------------------
				// ����� �� ���� ���
				//--------------------------------------------------
				// �� HP�� 30���� ���� ���
				// �ٸ� ����� ���
				// ����� HP�� full�� ���
				if (GetHP() < 30
					|| pCreature->IsSlayer()
					//|| GetGuildNumber()!=pCreature->GetGuildNumber()	
					// vampire�� default ���
					//&& GetGuildNumber()!=0
				//	&& pCreature->GetGuildNumber()!=0
					|| pCreature->GetHP() == pCreature->GetMAX_HP()
					|| pCreature->IsUndergroundCreature()
					|| pCreature->IsFlyingCreature())
				{
					m_fNextTrace = FLAG_TRACE_NULL;

					UnSetRepeatAction();

					return false;
				}
				break;

				//-------------------------------------------------------
				// SKILL_THROW_BOMB
				//-------------------------------------------------------
			case SKILL_THROW_BOMB:
			{
				MItem* pItem = g_pInventory->FindItem(ITEM_CLASS_BOMB);

				if (pItem == NULL)
				{
					m_fNextTrace = FLAG_TRACE_NULL;
					UnSetRepeatAction();
					return false;
				}

				useSkill = pItem->GetUseActionInfo();
			}
			break;

			}

			//-------------------------------------------------------
			// Ư���� ��Ȳ üũ
			//-------------------------------------------------------
			if ((*g_pActionInfoTable)[originalSkill].HasOption())
			{
				//-------------------------------------------------------
				// bless �ɷ��� ����ϴ� ���
				//-------------------------------------------------------
				if ((*g_pActionInfoTable)[originalSkill].IsOptionUseWithBless())
				{
					if (!m_bEffectStatus[EFFECTSTATUS_BLESS] && !m_bEffectStatus[EFFECTSTATUS_BLESS_2])
					{
						m_fNextTrace = FLAG_TRACE_NULL;

						UnSetRepeatAction();

						return false;
					}
				}
			}
			//-------------------------------------------------------
			// item�� ����ϴ� ����̸� item�� üũ�ؾ� �Ѵ�.
			//-------------------------------------------------------
			WORD fWeaponType = (*g_pActionInfoTable)[originalSkill].GetWeaponType();
			if (fWeaponType & FLAG_ACTIONINFO_WEAPON_HOLY_WATER)
			{
				MItem* pItem = g_pInventory->FindItem(ITEM_CLASS_HOLYWATER);

				if (pItem == NULL)
				{
					m_fNextTrace = FLAG_TRACE_NULL;

					UnSetRepeatAction();

					return false;
				}

				// holy water�� �ִ� �����..
				// ���� ���ϵ���... item ����..
				// ��� ������ Ǯ��� �Ѵ�.
				//SetItemCheckBuffer( pItem, ITEM_CHECK_BUFFER_SKILL_FROM_ITEM );
			}

			//-------------------------------------------------------
			// item�� ����ϴ� ����̸� item�� üũ�ؾ� �Ѵ�.
			//-------------------------------------------------------
			if (fWeaponType & FLAG_ACTIONINFO_WEAPON_BOMB)
			{
				MItem* pItem = g_pInventory->FindItem(ITEM_CLASS_BOMB);

				if (pItem == NULL)
				{
					m_fNextTrace = FLAG_TRACE_NULL;

					UnSetRepeatAction();

					return false;
				}

				// holy water�� �ִ� �����..
				// ���� ���ϵ���... item ����..
				// ��� ������ Ǯ��� �Ѵ�.
				//SetItemCheckBuffer( pItem, ITEM_CHECK_BUFFER_SKILL_FROM_ITEM );
			}

			//-------------------------------------------------------
			// ���� ����ϴ� ����̸� 
			// �Ѿ��� üũ�ؾ� �Ѵ�.
			//-------------------------------------------------------
			if ((*g_pActionInfoTable)[originalSkill].IsWeaponTypeGunAny())
			{
				if (g_pCurrentMagazine == NULL)
				{
					// ���� ����ϴµ� źâ�� ���� ���
					PlaySound(SOUND_ITEM_NO_MAGAZINE);

					// źâ ������ delay
					m_DelayTime = g_CurrentTime + GetActionInfoDelay(m_nBasicActionInfo);

					m_fNextTrace = FLAG_TRACE_NULL;
					return false;
				}
				else
				{
					if (g_pCurrentMagazine->GetNumber() == 0)
					{
						// ���� ����ϴµ� źâ�� ���� ���
						PlaySound(SOUND_ITEM_NO_MAGAZINE);

						// źâ ������ delay
						m_DelayTime = g_CurrentTime + GetActionInfoDelay(m_nBasicActionInfo);

						if (IsRepeatAction())
						{
							m_nUsedActionInfo = originalSkill;

							UnSetRepeatAction();

							m_nUsedActionInfo = ACTIONINFO_NULL;
						}
						else
						{
							UnSetRepeatAction();
						}

						m_fNextTrace = FLAG_TRACE_NULL;

						// [����] �Ѿ� �� ���� ��
//							__BEGIN_HELP_EVENT
//								ExecuteHelpEvent( HE_ITEM_EMPTY_BULLET );
//							__END_HELP_EVENT


						return false;
					}
				}
			}

			// creature�� �̹� ���� ���
			//if (pCreature->IsDead()) return;

			// ����� ����� ����
			m_nNextUsedActionInfo = useSkill;

			// ���� ���� ����
			SetTraceID(id);
			m_fTrace = FLAG_TRACE_CREATURE_SPECIAL;
			m_TraceX = pCreature->GetX();
			m_TraceY = pCreature->GetY();
			m_TraceZ = pCreature->GetZ();

			// ��������� �Ǵ� ĳ������ ����
			//m_TraceCreatureTribe = pCreature->GetCreatureTribe();

			// ���� �����ϴ� ��� ���ؼ� ����صд�.
			m_fTraceBuffer = m_fTrace;
			m_TraceIDBuffer = id;

			//------------------------------------------------------------
			// SpecialAction�� ���Ŀ� ����.. 
			// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶��� ���̴�.
			//------------------------------------------------------------
			//------------------------------------------------------------
			// ���� �����ΰ�?
			// ��� �����ϴ°�?
			//------------------------------------------------------------
			m_bTraceCreatureToForceAttack = bTraceCreatureToForceAttack;
			m_bKeepTraceCreature = bKeepTraceCreature;

			m_TraceDistance = GetActionInfoRange(m_nNextUsedActionInfo);

			//------------------------------------------------------------

			// �����ϴ� �� ����
			SetAction(m_MoveAction);
			SetNextDestination(m_TraceX, m_TraceY);

			// buffering�� ���ش�.
			m_fNextTrace = FLAG_TRACE_NULL;

			// ��� �Ѿư��� ���� ����
			m_bKeepTraceCreature = false;

			//------------------------------------------------------------
			// message���
			//------------------------------------------------------------
			DEBUG_ADD_FORMAT("Trace Creature To SpecialAction: ID=%d, (%d, %d)", id, m_TraceX, m_TraceY);

			// ���õ� sector ���ֱ�
			g_pTopView->SetSelectedSectorNULL();
		}
		else
		{
			//------------------------------------------------------------
			// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
			//------------------------------------------------------------
			if (m_bRepeatAction)
			{
				m_fNextTrace = FLAG_TRACE_NULL;

				return false;
			}
			else
			{
				m_fNextTrace = FLAG_TRACE_CREATURE_SPECIAL;
				m_bNextForceAttack = bForceAttack;
				m_NextTraceID = id;

				m_TraceIDBuffer = id;

				m_bRepeatAction = FALSE;

				m_fTraceBuffer = FLAG_TRACE_NULL;

				return true;//false;
			}
		}

		return true;
	}

	// Target to creature�� �ƴ϶�� �ǹ�.
	return false;
}


//----------------------------------------------------------------------
// Trace Sector To BasicAction
//----------------------------------------------------------------------
// �⺻��������(L-Click)  Sector(sX,sY)�� �����Ѵ�.
// �����ִ� ���°� �ƴϸ�.. Next�� �����صд�.
//----------------------------------------------------------------------
bool
MPlayer::TraceSectorToBasicAction(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
{
	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	if (!IsNotDelay()
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE)
		|| IsInCasket()	// [�����3]
		|| g_bZoneSafe)
	{
		return false;
	}

	// ��ǥ�� �߸� �Էµ� ���
	if (sX == SECTORPOSITION_NULL || sY == SECTORPOSITION_NULL)
		return false;

	if (m_nBasicActionInfo == ACTIONINFO_NULL
		// ������̸� Ÿ�� ������ ���� �ȵ�
		|| m_MoveDevice == MOVE_DEVICE_RIDE
		|| CanActionByZoneInfo())
	{
		return false;
	}

	if (IsStop()) // || m_Action!=ACTION_ATTACK)
	{
		// ����� ����� ����
		m_nNextUsedActionInfo = GetBasicActionInfo(); //m_nBasicActionInfo;

		// ���� ���� ����
		m_fTrace = FLAG_TRACE_SECTOR_BASIC;
		m_TraceX = sX;
		m_TraceY = sY;
		m_TraceZ = 0;

		// ���� �����ϴ� ��� ���ؼ� ����صд�.
		m_fTraceBuffer = m_fTrace;

		//------------------------------------------------------------
		// BasicAction�� ���Ŀ� ����..
		// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶��� ���̴�.
		//------------------------------------------------------------

		m_TraceDistance = GetActionInfoRange(m_nNextUsedActionInfo);

		//------------------------------------------------------------		

		// �����ϴ� �� ����
		SetAction(m_MoveAction);
		SetNextDestination(m_TraceX, m_TraceY);

		// buffering�� ���ش�.
		m_fNextTrace = FLAG_TRACE_NULL;

		// ��� �Ѿư��� ���� ����
		m_bKeepTraceCreature = false;

		//------------------------------------------------------------
		// message���
		//------------------------------------------------------------
#ifdef	OUTPUT_DEBUG
		DEBUG_ADD_FORMAT("Trace Sector To BasicAction: (%d, %d)", sX, sY);
#endif
	}
	else
	{
		//------------------------------------------------------------
		// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
		//------------------------------------------------------------
		if (m_bRepeatAction)
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			return false;
		}
		else
		{
			m_fNextTrace = FLAG_TRACE_SECTOR_BASIC;
			m_NextTraceX = sX;
			m_NextTraceY = sY;

			m_bRepeatAction = FALSE;

			return false;
		}
	}

	return true;
}

//----------------------------------------------------------------------
// Trace Sector To SpecialAction
//----------------------------------------------------------------------
// Ư����������(R-Click)  Sector(sX,sY)�� �����Ѵ�.
// �����ִ� ���°� �ƴϸ�.. Next�� �����صд�.
//----------------------------------------------------------------------
// [return��]
//
//  Zone�� ������� ���ϴ� ��츸 false�� return�Ѵ�. 
//
//----------------------------------------------------------------------
bool
MPlayer::TraceSectorToSpecialAction(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
{
	// Dracula Castle: no skills at all while carrying the Mihnea (the server refuses them)
	if (g_pEffectStatusTable->GetSize() > EFFECTSTATUS_HAS_MIHNEA && HasEffectStatus(EFFECTSTATUS_HAS_MIHNEA))
		return false;
	if ((HasEffectStatus(EFFECTSTATUS_SLEEP) || HasEffectStatus(EFFECTSTATUS_BLOCK_HEAD))
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
		return false;

	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	// Ousters Village (zone 1311) is flagged Safety in zone.inf, which otherwise
	// blocks every tile skill in the zone.  Teleport is let through so Ousters can
	// still cross their own town; the server allows only that one skill there.
	bool bVillageTeleport = (m_nSpecialActionInfo == SKILL_TELEPORT
							&& m_pZone != NULL && m_pZone->GetID() == 1311);

	if (!IsNotDelay()
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE)
		|| IsInCasket()	// [�����3]
		|| (g_bZoneSafe && !bVillageTeleport)
		|| CanActionByZoneInfo()
		)
	{
		return false;
	}

	// ��ǥ�� �߸� �Էµ� ���
	if (sX == SECTORPOSITION_NULL || sY == SECTORPOSITION_NULL)
		return false;


	if (m_nSpecialActionInfo != ACTIONINFO_NULL)
	{
		MActionInfo& rActionInfo = (*g_pActionInfoTable)[m_nSpecialActionInfo];

		if (rActionInfo.GetCastingTime())
		{
			m_SkillCastingX = sX;
			m_SkillCastingY = sY;
		}
	}

	if (m_nSpecialActionInfo == ACTIONINFO_NULL
		// ������̸� Ÿ�� ������ ����� ����� �� ����.
		|| m_MoveDevice == MOVE_DEVICE_RIDE
		// ���� ��� ����� ���� �޾ƾ� �ϸ� ����� �� ����.
		//|| m_WaitVerify != WAIT_VERIFY_NULL)	// 2001.8.20 �ּ�ó��
		|| IsInSafeSector() == 2
		// ���� �������뿡���� ����� ����� �� ����.		
		)
	{
		m_fNextTrace = FLAG_TRACE_NULL;

		UnSetRepeatAction();

		return false;
	}
	// 2004, 9, 18, sobeit add start - �ͷ� ���̾�� �����Ÿ� ����� ���� ��.�� 
	if (m_nSpecialActionInfo == SKILL_TURRET_FIRE)
	{
		m_TraceDistance = GetActionInfoRange(m_nSpecialActionInfo);
		if (max(abs(m_X - sX), abs(m_Y - sY)) > m_TraceDistance || !g_pSkillAvailable->IsEnableSkill(SKILL_TURRET_FIRE))
		{
			m_fNextTrace = FLAG_TRACE_NULL;
			UnSetRepeatAction();
			InstallTurretStopAttack();
			return false;
		}
	}
	// 2004, 9, 18, sobeit add end

	// �ٸ� ����� ������� ���
	if (m_nUsedActionInfo != ACTIONINFO_NULL)
		return false;

	// 2004, 9, 21, sobeit add start - Ÿ�� ũ���� ��ų�� Ÿ�� �� ��ų�� ����� �� �ְ�
	bool bAbleTargetSector = false;
	//	if(m_nSpecialActionInfo == SKILL_DESTRUCTION_SPEAR || m_nSpecialActionInfo == SKILL_DESTRUCTION_SPEAR_MASTERY)
	//	{
	//		if(g_pSkillAvailable->IsEnableSkill( SKILL_DESTRUCTION_SPEAR_MASTERY ))
	//			bAbleTargetSector = true;
	//	}
	//	else 
	if (m_nSpecialActionInfo == SKILL_ICE_LANCE || m_nSpecialActionInfo == SKILL_ICE_LANCE_MASTERY)
	{
		if (g_pSkillAvailable->IsEnableSkill(SKILL_ICE_LANCE_MASTERY))
			bAbleTargetSector = true;
	}

	// 2006.08.22 sjheon
	if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WOLF))
	{
		if (!(m_nSpecialActionInfo == MAGIC_HOWL || m_nSpecialActionInfo == MAGIC_EAT_CORPSE || m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_BAT))
	{
		if (!(m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WERWOLF))
	{
		if (!(m_nSpecialActionInfo == SKILL_BITE_OF_DEATH ||
			m_nSpecialActionInfo == MAGIC_RAPID_GLIDING ||
			m_nSpecialActionInfo == SKILL_VIOLENT_PHANTOM ||
			m_nSpecialActionInfo == SKILL_WILD_WOLF ||
			m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}


	// 2004, 9, 21, sobeit add end
	// Zone�� ����� �� �ִ� �͵鸸 ����Ѵ�.
	if ((*g_pActionInfoTable)[m_nSpecialActionInfo].IsTargetZone() || bAbleTargetSector)
	{
		int grade = 0;
		int		useSkill = m_nSpecialActionInfo;

		if (IsOusters())
		{
			grade = (*g_pSkillInfoTable)[useSkill].GetExpLevel() / 15;
		}

		if ((*g_pActionInfoTable)[useSkill].IsUseActionStep() && grade > 0)
			useSkill = (*g_pActionInfoTable)[useSkill].GetActionStep(grade - 1);

		// 2004, 9, 20, sobeit add start
		if ((*g_pActionInfoTable)[useSkill].IsMasterySkillStep())
		{
			int TempSkill = (*g_pActionInfoTable)[useSkill].GetActionStep((*g_pActionInfoTable)[useSkill].GetMasterySkillStep());
			if (g_pSkillAvailable->IsEnableSkill((ACTIONINFO)TempSkill))
				useSkill = TempSkill;
		}
		// 2004, 9, 20, sobeit add end

//		if (useSkill==SKILL_TURRET_FIRE)
//		{
//			if (max(abs(m_X-sX), abs(m_Y-sY)) 
//				> m_TraceDistance)
//			{
//				m_fNextTrace = FLAG_TRACE_NULL;
//				UnSetRepeatAction();
//				return false;
//			}
//		}
		//-------------------------------------------------------
		// ��ź ��� ������
		//-------------------------------------------------------
		if (useSkill == SKILL_THROW_BOMB)
		{
			//-------------------------------------------------------
			// ���� ����� �� �ִ� ������� üũ..
			// Passive ��ų�̸� ��� ���ϰ�..
			//-------------------------------------------------------
			if (IsSlayer()
				&& !IsWaitVerify()
				&& IsItemCheckBufferNULL())
			{
				ITEM_CLASS	itemClass = ITEM_CLASS_BOMB;

				MItemClassFinder itemFinder(itemClass);
				MItem* pItem = ((MItemManager*)g_pInventory)->FindItem(itemFinder);

				if (pItem == NULL)
				{
					m_fNextTrace = FLAG_TRACE_NULL;
					UnSetRepeatAction();
					return false;
				}

				useSkill = pItem->GetUseActionInfo();
			}
		}


		//-------------------------------------------------------
		// ���� ����� �� �ִ� ������� üũ..
		// Passive ��ų�̸� ��� ���ϰ�..
		//-------------------------------------------------------
		if (
			(!g_pSkillAvailable->IsEnableSkill((ACTIONINFO)m_nSpecialActionInfo) && m_nSpecialActionInfo != SKILL_TURRET_FIRE)
			|| !(*g_pSkillInfoTable)[m_nSpecialActionInfo].IsEnable()
			|| !(*g_pSkillInfoTable)[m_nSpecialActionInfo].IsAvailableTime()
			|| (*g_pSkillInfoTable)[m_nSpecialActionInfo].IsPassive())
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			UnSetRepeatAction();

			return false;
		}

		if (IsStop()) // || m_Action!=ACTION_ATTACK)
		{

			// �ƿ콺���� ��ü ����Ŭ������ �ٷ� �����ϰ� �ϴ� �ڵ� �ּ� ó�� 
/*
#ifdef __METROTECH_TEST__
			if (IsOusters())
			{
				bool bAbsorbSoul = true;

				MItem* pItem = g_pZone->GetItem( g_pZone->GetItemID( sX, sY ) );

				if (pItem==NULL
					|| pItem->GetItemClass()!=ITEM_CLASS_CORPSE)
				{
					bAbsorbSoul = false;

					if(pItem == NULL)
					{
						MCreature *pCreature = g_pZone->GetCreature(g_pZone->GetCreatureID(sX, sY));

						if(pCreature != NULL && pCreature->HasEffectStatus(EFFECTSTATUS_COMA))
						{
							bAbsorbSoul = true;
						}
					}
				}

				else if(pItem != NULL )
				{
					// ������ �ȸ԰� �ϱ�
					MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();
					if(pCreature != NULL)
					{
						if(pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376 || pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563 || pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549 || pCreature->GetCreatureType() == 670 || pCreature->GetCreatureType() == 672 || pCreature->GetCreatureType() == 673)
							bAbsorbSoul = false;
					}
				}
				if(bAbsorbSoul)
					useSkill = SKILL_ABSORB_SOUL;
			}
#endif
			*/

			int targetX = sX;
			int targetY = sY;

			//-------------------------------------------------------
			// Ư���� ����� ����� üũ
			//-------------------------------------------------------
			switch (useSkill)
			{
				// Ʈ���� ǻ���� Ÿ�ϱ���� üũ�������� ������ ����� �� �� ����
			case SKILL_TRANSFUSION:
			case SKILL_WILD_TYPHOON:
			case SKILL_BLOODY_WINGS:
				return false;
				break;

				//-------------------------------------------------------
				// ����
				//-------------------------------------------------------
				// �ٴڿ� ��ü�� ���� ���� ��� ��� ���Ѵ�.
				//-------------------------------------------------------
			case SKILL_ABSORB_SOUL:
				if (IsOusters())
				{
					MItem* pItem = g_pZone->GetItem(g_pZone->GetItemID(sX, sY));

					if (pItem == NULL
						|| pItem->GetItemClass() != ITEM_CLASS_CORPSE)
					{
						if (pItem == NULL)
						{
							MCreature* pCreature = g_pZone->GetCreature(g_pZone->GetCreatureID(sX, sY));

							if (pCreature != NULL && pCreature->HasEffectStatus(EFFECTSTATUS_COMA))
								break;
						}

						m_fNextTrace = FLAG_TRACE_NULL;

						UnSetRepeatAction();

						return false;
					}

					// ������ �ȸ԰� �ϱ�
					MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();
					if (pCreature != NULL)
					{
						if (pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376 || pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563 || pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549 || pCreature->GetCreatureType() == 670 || pCreature->GetCreatureType() == 672 || pCreature->GetCreatureType() == 673)
							return false;
					}

				}
				else
				{
					m_fNextTrace = FLAG_TRACE_NULL;

					UnSetRepeatAction();

					return false;
				}
				break;

				//-------------------------------------------------------
				// ������ ��� ��ü �Ա�
				//-------------------------------------------------------
				// �ٴڿ� ��ü�� ���� ���� ��� ��� ���Ѵ�.
				//-------------------------------------------------------
			case SKILL_BITE_OF_DEATH:
				if (m_CreatureType != CREATURETYPE_WER_WOLF)
				{
					return false;
				}
				break;
			case MAGIC_EAT_CORPSE:
				if (m_CreatureType == CREATURETYPE_WOLF)
				{
					MItem* pItem = g_pZone->GetItem(g_pZone->GetItemID(sX, sY));

					if (pItem == NULL
						|| pItem->GetItemClass() != ITEM_CLASS_CORPSE)
					{
						m_fNextTrace = FLAG_TRACE_NULL;

						UnSetRepeatAction();

						return false;
					}

					// ������ �ȸ԰� �ϱ�
					MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();
					if (pCreature != NULL)
					{
						if (pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376 || pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563 || pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549 || pCreature->GetCreatureType() == 670 || pCreature->GetCreatureType() == 672 || pCreature->GetCreatureType() == 673)
							return false;
					}

				}
				else
				{
					m_fNextTrace = FLAG_TRACE_NULL;

					UnSetRepeatAction();

					return false;
				}
				break;
				//				// 2004, 8, 30, sobeit add start - sweep vice test
				//				case SKILL_SWEEP_VICE_1:
				//					if(!GetSweepViewValue()) //�������̽��� ���� ������� �ƴҶ�
				//					{
				//						SetSweepViewValue(1);
				//						g_SelectSector = g_pTopView->GetSelectedSector(g_x, g_y);
				//						m_TraceX	= 	g_SelectSector.x;
				//						m_TraceY	=	g_SelectSector.y;
				//						ExecuteActionInfoFromMainNode(SKILL_CLIENT_SWEEP_VICE,g_SelectSector.x, g_SelectSector.y, 0,g_pPlayer->GetDirection(),0	,	
				//							g_SelectSector.x, g_SelectSector.y, 0, 0xffff, NULL, false);
				//						UnSetRepeatAction();
				//					//	return false ;
				//					}
				//					else
				//					{
				//						if(!g_bRButtonDown)
				//						{
				//							if(2 == GetSweepViewValue())
				//								useSkill = SKILL_SWEEP_VICE_3;
				//							else if(3 == GetSweepViewValue())
				//								useSkill = SKILL_SWEEP_VICE_5;
				//						}
				//						else 
				//							return false;
				//					}
				//					break;
			case SKILL_TURRET_FIRE:
				if (!HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
					return false;
				break;
			}



			// 2004, 8, 30, sob


			WORD fWeaponType = (*g_pActionInfoTable)[useSkill].GetWeaponType();

			//------------------------------------------------
			// ��ź�� ���.. ������ ���� ��ǥ ��߳���
			//------------------------------------------------
			if (fWeaponType & FLAG_ACTIONINFO_WEAPON_BOMB)
			{
				int skillLevel = (*g_pSkillInfoTable)[SKILL_THROW_BOMB].GetExpLevel();

				int hitRoll = 50 + skillLevel / 2;

				if (rand() % 100 < hitRoll)
				{
					// ����� ���� ���
				}
				else
				{
					if (rand() % 100 < 25
						&& skillLevel < 30)		// 2Ÿ�� ��ó
					{
						// ���¾ƾ� �Ѵ�.
						targetX += rand() % 5 - 2;
						targetY += rand() % 5 - 2;
					}
					else						// 1Ÿ�� ��ó
					{
						// ���¾ƾ� �Ѵ�.
						targetX += rand() % 3 - 1;
						targetY += rand() % 3 - 1;
					}
				}
			}

			//------------------------------------------------
			// �Ѿ� üũ
			//------------------------------------------------
			if ((*g_pActionInfoTable)[useSkill].IsWeaponTypeGunAny())
			{
				if (!IsRequestMode() && g_pCurrentMagazine != NULL)		// �Ѿ��� �ʿ� ���� ���
				{
					if (g_pCurrentMagazine->GetNumber() == 0)	// �Ѿ��� ���� ���
					{
						// ���� ����ϴµ� źâ�� ���� ���
						PlaySound(SOUND_ITEM_NO_MAGAZINE);

						m_fNextTrace = FLAG_TRACE_NULL;



						// [����] �Ѿ� �� ���� ��
//						__BEGIN_HELP_EVENT
//							ExecuteHelpEvent( HE_ITEM_EMPTY_BULLET );
//						__END_HELP_EVENT

						m_fNextTrace = FLAG_TRACE_NULL;

						UnSetRepeatAction();
						// 2004, 9, 23, sobeit add start - �ͷ� ���� ������ ����
						if (HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
						{
							InstallTurretStopAttack();
						}
						else
							// źâ ������ delay
							m_DelayTime = g_CurrentTime + GetActionInfoDelay(m_nBasicActionInfo);
						// 2004, 9, 23, sobeit add end
						return false;
					}
				}
			}

			// ����� ����� ����
			m_nNextUsedActionInfo = useSkill;

			// ���� ���� ����
			m_fTrace = FLAG_TRACE_SECTOR_SPECIAL;
			m_TraceX = targetX;
			m_TraceY = targetY;
			m_TraceZ = 0;

			// ���� �����ϴ� ��� ���ؼ� ����صд�.
			m_fTraceBuffer = m_fTrace;

			//------------------------------------------------------------
			// BasicAction�� ���Ŀ� ����..
			// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶��� ���̴�.
			//------------------------------------------------------------

			m_TraceDistance = GetActionInfoRange(m_nNextUsedActionInfo);

			//------------------------------------------------------------		

			// �����ϴ� �� ����
			SetAction(m_MoveAction);
			SetNextDestination(m_TraceX, m_TraceY);

			// buffering�� ���ش�.
			m_fNextTrace = FLAG_TRACE_NULL;

			// ��� �Ѿư��� ���� ����
			m_bKeepTraceCreature = false;

			//------------------------------------------------------------
			// message���
			//------------------------------------------------------------
			DEBUG_ADD_FORMAT("Trace Sector To SpecialAction: (%d, %d)", sX, sY);
			if (useSkill == SKILL_TELEPORT)
			{
				return false;
			}
		}
		else
		{

			//------------------------------------------------------------
			// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
			//------------------------------------------------------------
			if (m_bRepeatAction)
			{
				m_fNextTrace = FLAG_TRACE_NULL;

				return false;
			}
			else
			{
				DEBUG_ADD_FORMAT("Trace Sector To SpecialAction: Buffering");

				m_fNextTrace = FLAG_TRACE_SECTOR_SPECIAL;
				m_NextTraceX = sX;
				m_NextTraceY = sY;

				m_bRepeatAction = FALSE;

				// 200411, 12, 21, sobeit modify start - �̵��� ��ų ���� �ݺ��� �ȵǴ� ���� ����
				//return false;
				return true;
				// 200411, 12, 21, sobeit modify end
			}
		}

		return true;
	}

	// Zone�� ����ϴ°� �ƴ϶�� �ǹ�
	return false;
}

//----------------------------------------------------------------------
// Trace Item
//----------------------------------------------------------------------
// Item���� �ٰ����� �ֿ��� �Ѵ�.
// �����ִ� ���°� �ƴϸ�.. Next�� �����صд�.
//----------------------------------------------------------------------
bool
MPlayer::TraceItem(TYPE_OBJECTID id)
{
	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	if (!IsNotDelay()
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE)
		|| IsInCasket()	// [�����3]
		|| IsInDarkness() && (!(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || g_pZone->GetID() == 3001)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		return false;
	}

	if (IsStop()) // || m_Action!=ACTION_ATTACK)
	{
		// Zone�� �����ϴ� Item���� check�Ѵ�.
		MItem* pItem = m_pZone->GetItem(id);

		// item�� zone�� ���� ���
		if (pItem == NULL
			|| g_pZone->GetSector(pItem->GetX(), pItem->GetY()).HasDarkness() &&
			(!IsVampire() && (!(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) ||
				IsVampire() && g_pZone->GetID() == 3001))
			&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
			&& !g_bLight
#endif
			)
		{
			return false;
		}

		//-------------------------------------------------------------
		// Item �ݱ� ���� �Ÿ��� �̹� �ִٸ�..
		//-------------------------------------------------------------
		if (max(abs(pItem->GetX() - m_X), abs(pItem->GetY() - m_Y)) <= 1)
		{
			DEBUG_ADD("Pickup Item in Trace Dist.");

			TraceNULL();

			// ������ �Ϸ������Ƿ� Item�� �ݴ´�.
			PickupItem(pItem);

			return true;
		}


		// ���� ���� ����
		SetTraceID(id);
		m_fTrace = FLAG_TRACE_ITEM;
		m_TraceX = pItem->GetX();
		m_TraceY = pItem->GetY();
		m_TraceZ = 0;

		// ���� �����ϴ� ��� ���ؼ� ����صд�.
		m_fTraceBuffer = m_fTrace;


		//------------------------------------------------------------		
		// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶�����?
		//------------------------------------------------------------

		m_TraceDistance = 1;

		//------------------------------------------------------------

		// �����ϴ� �� ����
		SetAction(m_MoveAction);
		SetNextDestination(m_TraceX, m_TraceY);

		// buffering�� ���ش�.
		m_fNextTrace = FLAG_TRACE_NULL;

		// ��� �Ѿư��� ���� ����
		m_bKeepTraceCreature = false;

		//------------------------------------------------------------
		// message���
		//------------------------------------------------------------
		DEBUG_ADD_FORMAT("Trace Item: %d", id);
	}
	else
	{
		//------------------------------------------------------------
		// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
		//------------------------------------------------------------
		if (m_bRepeatAction)
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			return false;
		}
		else
		{
			m_fNextTrace = FLAG_TRACE_ITEM;
			m_NextTraceID = id;

			m_bRepeatAction = FALSE;

			return false;
		}
	}

	return true;
}

//----------------------------------------------------------------------
// Trace InteractionObject
//----------------------------------------------------------------------
// InteractionObject�� �ٰ����� currentAction�� �����Ų��.
// --> �ٰ�������, currentAction�� 
//     "�̹�" ����Ǿ������� �������� �ʾƾ� �Ѵ�.
//
// �����ִ� ���°� �ƴϸ�.. Next�� �����صд�.
//----------------------------------------------------------------------
bool
MPlayer::TraceInteractionObject(TYPE_OBJECTID id, BYTE currentAction)
{
	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	if (!IsNotDelay()
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE)
		|| IsInCasket())	// [�����3]		
	{
		return false;
	}

	//------------------------------------------------
	// ������̸� Ÿ�� ������ ���� �� �� ����.
	//------------------------------------------------
	if (m_MoveDevice == MOVE_DEVICE_RIDE)
	{
		return false;
	}

	if (IsStop()) // || m_Action!=ACTION_ATTACK)
	{
		// Zone�� �����ϴ� Item���� check�Ѵ�.
		MImageObject* pImageObject = m_pZone->GetImageObject(id);

		// ImageObject�� ������ return
		if (pImageObject == NULL) return false;

		// InteractionObject�� �ƴϸ� return
		if (pImageObject->GetObjectType() != MObject::TYPE_INTERACTIONOBJECT)
			return false;

		// ���� ���� ����
		SetTraceID(id);
		m_fTrace = FLAG_TRACE_INTERACTIONOBJECT;
		m_TraceObjectAction = currentAction;
		m_TraceX = pImageObject->GetX();
		m_TraceY = pImageObject->GetY();
		m_TraceZ = 0;

		// ���� �����ϴ� ��� ���ؼ� ����صд�.
		m_fTraceBuffer = m_fTrace;

		//------------------------------------------------------------		
		// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶�����?
		//------------------------------------------------------------

		m_TraceDistance = 1;

		//------------------------------------------------------------

		// �����ϴ� �� ����
		SetAction(m_MoveAction);
		SetNextDestination(m_TraceX, m_TraceY);

		// buffering�� ���ش�.
		m_fNextTrace = FLAG_TRACE_NULL;

		// ��� �Ѿư��� ���� ����
		m_bKeepTraceCreature = false;

		//------------------------------------------------------------
		// message���
		//------------------------------------------------------------
		DEBUG_ADD_FORMAT("Trace InteractionObject: %d", id);
	}
	else
	{
		//------------------------------------------------------------
		// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
		//------------------------------------------------------------
		if (m_bRepeatAction)
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			return false;
		}
		else
		{
			m_fNextTrace = FLAG_TRACE_INTERACTIONOBJECT;
			m_NextTraceID = id;
			m_NextTraceObjectAction = currentAction;

			m_bRepeatAction = FALSE;

			return false;
		}
	}

	return true;
}

//----------------------------------------------------------------------
// Trace Effect
//----------------------------------------------------------------------
// SelectableEffect�� �����Ѵ�.
//----------------------------------------------------------------------
bool
MPlayer::TraceEffect(TYPE_OBJECTID id)
{
	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	if (!IsNotDelay()
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE)
		|| IsInCasket())	// [�����3]
	{
		return false;
	}

	//------------------------------------------------
	// ������̸� Ÿ�� ������ ���� �� �� ����.
	//------------------------------------------------
	if (m_MoveDevice == MOVE_DEVICE_RIDE)
	{
		return false;
	}

	if (IsStop()) // || m_Action!=ACTION_ATTACK)
	{
		MEffect* pEffect = m_pZone->GetEffect(id);

		if (pEffect == NULL)
		{
			pEffect = m_pZone->GetGroundEffect(id);

			if (pEffect == NULL)
			{
				return false;
			}
		}

		if (!pEffect->IsSelectable())
			return false;

		// ���� ���� ����
		SetTraceID(id);
		m_fTrace = FLAG_TRACE_EFFECT;
		//m_TraceObjectAction	= currentAction;
		m_TraceX = pEffect->GetX();
		m_TraceY = pEffect->GetY();
		m_TraceZ = 0;

		// ���� �����ϴ� ��� ���ؼ� ����صд�.
		m_fTraceBuffer = m_fTrace;

		//------------------------------------------------------------		
		// ���� Player�� �ɷ¿� ���� TraceDistance�� �޶�����?
		//------------------------------------------------------------

		m_TraceDistance = 1;

		//------------------------------------------------------------

		// �����ϴ� �� ����
		SetAction(m_MoveAction);
		SetNextDestination(m_TraceX, m_TraceY);

		// buffering�� ���ش�.
		m_fNextTrace = FLAG_TRACE_NULL;

		// ��� �Ѿư��� ���� ����
		m_bKeepTraceCreature = false;

		//------------------------------------------------------------
		// message���
		//------------------------------------------------------------
		DEBUG_ADD_FORMAT("Trace Effect: %d", id);
	}
	else
	{
		//------------------------------------------------------------
		// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
		//------------------------------------------------------------
		if (m_bRepeatAction)
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			return false;
		}
		else
		{
			m_fNextTrace = FLAG_TRACE_EFFECT;
			m_NextTraceID = id;
			//m_NextTraceObjectAction	= currentAction;

			m_bRepeatAction = FALSE;

			return false;
		}
	}

	return true;
}



//----------------------------------------------------------------------
// Set Trace Position
//----------------------------------------------------------------------
// Creature�� �ִ� sector�� �̹� Creature�� �ֱ� ������ �� �� ����.
// �׷��Ƿ�, Creature�� ���� �����ϸ鼭 ���� Player���� ����� 
// Sector�� �����ؾ� �Ѵ�.
//----------------------------------------------------------------------
/*
void
MPlayer::SetTraceDestination()
{
	// check�ϴ� ������ ������ �д�.
	int		stepX = m_X - m_TraceX,
			stepY = m_Y - m_TraceY;
	int		signX = (stepX==0)? 0 : (stepX<0)? -1 : 1,
			signY = (stepY==0)? 0 : (stepY<0)? -1 : 1;

	int		check[8];

	// signX,signY��
	switch (DetermineDirection(signX, signY))
	{
		case RIGHT :

		break;
	}


	//-------------------------------------------------------
	// 8������ ��� check
	//-------------------------------------------------------
	for (int i=0; i<8; i++)
	{
		switch (i)
		{
			case DIRECTION_LEFT			: next.x--;	next.y++;	break;
			case DIRECTION_RIGHT		: next.x++;	next.y--;	break;
			case DIRECTION_UP			: next.x--;	next.y--;	break;
			case DIRECTION_DOWN			: next.x++;	next.y++;	break;
			case DIRECTION_LEFTUP		: next.x--;				break;
			case DIRECTION_LEFTDOWN		: next.y++;				break;
			case DIRECTION_RIGHTUP		: next.y--;				break;
			case DIRECTION_RIGHTDOWN	: next.x++;				break;
		}

		//-------------------------------------------------------
		// Zone�� ���� ���̸� check ���Ѵ�.
		//-------------------------------------------------------
		if (next.x<0 || next.y<0
					|| next.x>=m_pZone->GetWidth() || next.y>=m_pZone->GetHeight()) continue;

		//-------------------------------------------------------
		// �� �� ������ �����Ѵ�.
		//-------------------------------------------------------
		if (m_pZone->CanMove(next.x, next.y))
		{
		}
	}
}
*/

//----------------------------------------------------------------------
// Can Attack Tribe
//----------------------------------------------------------------------
// � ������ ������ �� �ִ� AttackMode������ �˾ƺ���.
//----------------------------------------------------------------------
bool
MPlayer::CanAttackTribe(enum CREATURETRIBE to) const
{
	switch (m_AttackMode)
	{
		//--------------------------------------------
		// ��ȭ���� : �ƹ��� ���� �� �Ѵ�.
		//--------------------------------------------
	case ATTACK_MODE_PEACE:
		return false;
		break;

		//--------------------------------------------
		// ���ݻ��� : �ƹ��� ���� �Ѵ�.
		//--------------------------------------------
	case ATTACK_MODE_AGGRESS:
		return true;
		break;

		//--------------------------------------------
		// ���� : ���� �����Ѵ�.
		//--------------------------------------------
		// �� = Tribe�� ���� ���� ���..
		// NPC�� ���� ���Ѵ�.
	case ATTACK_MODE_NORMAL:
		return GetCreatureTribe() != to;// && to!=CREATURETRIBE_NPC;
		break;
	}

	return false;
}

//----------------------------------------------------------------------
// Can Attack Guild
//----------------------------------------------------------------------
// � guild�� ������ �� �ִ� AttackMode������ �˾ƺ���.
//----------------------------------------------------------------------
bool
MPlayer::CanAttackGuild(const MCreature* pCreature) const
{
	switch (m_AttackMode)
	{
		//--------------------------------------------
		// ��ȭ���� : �ƹ��� ���� �� �Ѵ�.
		//--------------------------------------------
	case ATTACK_MODE_PEACE:
		return false;
		break;

		//--------------------------------------------
		// ���ݻ��� : �ƹ��� ���� �Ѵ�.
		//--------------------------------------------
	case ATTACK_MODE_AGGRESS:
		return true;
		break;

		//--------------------------------------------
		// ���� : ���� �����Ѵ�.
		//--------------------------------------------
		// �� = Guild�� ���� ���� ���..
	case ATTACK_MODE_NORMAL:
	{
		int gn = pCreature->GetGuildNumber();

		return m_GuildNumber != gn
			// ����� ����ڳ����� �����ϸ� �ȵǱ� ������..
			// ����� ��尡 �ƴ� ��츸 ������ �� �ִ�.
			// ���߿��� �̰� �����ߵ��� ������?

			// vampire�� ���. �ϴ� vampire�� AttackByGuild�� ����ϱ� ����
			&& gn != GUILDID_VAMPIRE_DEFAULT && gn < MIN_USER_GUILDID
			|| GetCreatureTribe() != pCreature->GetCreatureTribe();
		// && to!=CREATURETRIBE_NPC;
	}
	break;
	}

	return false;
}

//----------------------------------------------------------------------
// Action In Trace Distance
//----------------------------------------------------------------------
// �ൿ�� ������ �Ÿ��� ���� ���� �ൿ ó��
//
// return true : �Ÿ��� ���� ��
//		  false : �Ÿ� ��.
//----------------------------------------------------------------------
bool
MPlayer::ActionInTraceDistance()
{
	int nextAction;
	if (m_nUsedActionInfo == ACTIONINFO_NULL)
	{
		if (m_MoveDevice == MOVE_DEVICE_RIDE)
		{
			if (IsSlayer())
			{
				nextAction = ACTION_SLAYER_MOTOR_STAND;
			}
			else
			{
				// �ᰡ��~~..
				//nextAction = ACTION_STAND;
			}
		}
		//-----------------------------------------------------
		// �ɾ�ٴҶ�
		//-----------------------------------------------------
		else if (m_MoveDevice == MOVE_DEVICE_SUMMON_SYLPH)
		{
			if (IsOusters())
			{
				nextAction = ACTION_OUSTERS_FAST_MOVE_STAND;
			}
		}
#if __CONTENTS(__FAST_TRANSFORTER||__SECOND_TRANSFORTER)
		else if (m_MoveDevice == MOVE_DEVICE_WING_SYLPH)
		{
			if (IsOusters())
			{
#if __CONTENTS(__SECOND_TRANSFORTER)
				if (0 == GetWingItemType())
#endif //__SECOND_TRANSFORTER
					nextAction = ACTION_OUSTERS_WING_STAND;
#if __CONTENTS(__SECOND_TRANSFORTER)
				else if (1 == GetWingItemType())
					nextAction = ACTION_OUSTERS_UNICORN_STAND;
#endif //__SECOND_TRANSFORTER
			}
		}
#endif //__FAST_TRANSFORTER||__SECOND_TRANSFORTER
		else
		{
			nextAction = ACTION_STAND;
		}
	}
	else
	{
		nextAction = GetActionInfoAction(m_nUsedActionInfo);
	}

	//-------------------------------------------------------
	//
	// [ ���� �Ϸ� ] 
	//
	// : ���� �Ÿ� �̳��� ��� --> ���� ���� --> ���� Action
	//
	//-------------------------------------------------------			
	if (max(abs(m_X - m_TraceX), abs(m_Y - m_TraceY))
		<= m_TraceDistance)
	{
		//----------------------------------------------------
		//
		// �����Ÿ��� ���������Ƿ� Action Mode��ȯ				
		//
		//----------------------------------------------------

		//----------------------------------------------------
		// ������ �Ϸ�����Ƿ� ������ �ൿ�� ���ϰ� �Ѵ�.
		// m_NextAction�� ������ �ൿ�� ����..
		//----------------------------------------------------
		//---------------------
		// �ڽſ��� �ൿ
		//---------------------
		if (m_fTrace & FLAG_TRACE_SELF)
		{
			//---------------------------------------------------------------
			// ���� ���õ� Ư������� ����..
			//---------------------------------------------------------------					
			//m_ActionCount = m_ActionCountMax;
			m_MoveCount = m_MoveCountMax;
			SetNextAction(nextAction);

			//--------------------
			// message
			//--------------------
			DEBUG_ADD("Do Self SpecialAction");

			return true;
		}
		//----------------------------------------------------
		//
		//					�⺻ �ൿ
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_CREATURE_BASIC)
		{
			//---------------------------------------------------------------
			// ���� ������� ���⿡ ����...
			//---------------------------------------------------------------
			// pCreature
			// m_TraceX, m_TraceY�� ������
			//---------------------------------------------------------------
			//m_ActionCount = m_ActionCountMax;
			m_MoveCount = m_MoveCountMax;

			BasicActionToCreature();

			return true;
		}
		//----------------------------------------------------
		//
		//						Ư�� �ൿ
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_CREATURE_SPECIAL)
		{
			//---------------------------------------------------------------
			// Slot�� ������ Ư������(������ �ƴҼ��� �ְ���)�� ����..
			//---------------------------------------------------------------
			// pCreature
			// m_TraceX, m_TraceY�� ������
			//---------------------------------------------------------------
			//m_ActionCount = m_ActionCountMax;
			m_MoveCount = m_MoveCountMax;
			SetNextAction(nextAction);

			/*
			#ifdef	CONNECT_SERVER
				CGAttackNormal _CGAttackNormal;
				_CGAttackNormal.setX( m_X );
				_CGAttackNormal.setY( m_Y );
				_CGAttackNormal.setDir( m_Direction );
				_CGAttackNormal.setObjectID( m_TraceID );
				g_pSocket->sendPacket( &_CGAttackNormal );

				#ifdef	OUTPUT_DEBUG
					DEBUG_ADD( _CGAttackNormal.toString().c_str() );
				#endif
			#endif
			*/

			//--------------------
			// message
			//--------------------
			DEBUG_ADD_FORMAT("Special Attack : %d", m_TraceID);

			return true;
		}
		//----------------------------------------------------
		//
		//				�⺻ �ൿ -> Sector
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_SECTOR_BASIC)
		{
			//---------------------------------------------------------------
			// ���� ������� ���⿡ ����...
			//---------------------------------------------------------------
			// pCreature
			// m_TraceX, m_TraceY�� ������
			//---------------------------------------------------------------
			//m_ActionCount = m_ActionCountMax;
			m_MoveCount = m_MoveCountMax;
			SetNextAction(nextAction);

			/*
			#ifdef	CONNECT_SERVER
				CGAttackNormal _CGAttackNormal;
				_CGAttackNormal.setX( m_X );
				_CGAttackNormal.setY( m_Y );
				_CGAttackNormal.setDir( m_Direction );
				_CGAttackNormal.setObjectID( m_TraceID );
				g_pSocket->sendPacket( &_CGAttackNormal );

				#ifdef	OUTPUT_DEBUG
					DEBUG_ADD( _CGAttackNormal.toString().c_str() );
				#endif
			#endif
			*/

			//--------------------
			// message
			//--------------------
			DEBUG_ADD_FORMAT("Basic Attack Sector : (%d, %d)", m_TraceX, m_TraceY);

			return true;
		}
		//----------------------------------------------------
		//
		//				Ư�� �ൿ -> Sector
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_SECTOR_SPECIAL)
		{
			//---------------------------------------------------------------
			// Slot�� ������ Ư������(������ �ƴҼ��� �ְ���)�� ����..
			//---------------------------------------------------------------
			// pCreature
			// m_TraceX, m_TraceY�� ������
			//---------------------------------------------------------------
			//m_NextAction = ACTION_SHOOT;
			//m_ActionCount = m_ActionCountMax; 
			m_MoveCount = m_MoveCountMax;
			SetNextAction(nextAction);

			/*
			#ifdef	CONNECT_SERVER
				CGAttackNormal _CGAttackNormal;
				_CGAttackNormal.setX( m_X );
				_CGAttackNormal.setY( m_Y );
				_CGAttackNormal.setDir( m_Direction );
				_CGAttackNormal.setObjectID( m_TraceID );
				g_pSocket->sendPacket( &_CGAttackNormal );

				#ifdef	OUTPUT_DEBUG
					DEBUG_ADD( _CGAttackNormal.toString().c_str() );
				#endif
			#endif
			*/

			//--------------------
			// message
			//--------------------
			DEBUG_ADD_FORMAT("Special Attack Sector : (%d, %d)", m_TraceX, m_TraceY);
			//*/

			return true;
		}
		//----------------------------------------------------
		//
		//					Item ����
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_ITEM)
		{
			MItem* pItem = m_pZone->GetItem(m_TraceID);

			// ������ �Ϸ������Ƿ� Item�� �ݴ´�.
			if (pItem != NULL
				&& (!g_pZone->GetSector(pItem->GetX(), pItem->GetY()).HasDarkness() || IsVampire() && g_pZone->GetID() != 3001 ||
					!IsVampire() && (HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))
					&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
					&& !g_bLight
#endif
					|| !IsOusters())
				)
			{
				PickupItem(pItem);
			}

			return true;
		}
		//----------------------------------------------------
		//
		//			Inventory�� Item�� ��� ���
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_INVENTORY)
		{
			MItem* pItem = g_pInventory->GetItemToModify(m_TraceID);

			// ������ �Ϸ������Ƿ� Item�� �ݴ´�.
			if (pItem != NULL)
			{
				// ActionEffect()�� ȣ���ϱ� ���ؼ�...
				m_MoveCount = m_MoveCountMax;
				SetNextAction(nextAction);
			}
			else
			{
				DEBUG_ADD_FORMAT("No such Item in Inventory: id=%d", m_TraceID);
			}

			return true;
		}
		//----------------------------------------------------
		//
		//			Effect����
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_EFFECT)
		{
			MEffect* pEffect = m_pZone->GetEffect(m_TraceID);

			if (pEffect == NULL)
			{
				pEffect = m_pZone->GetGroundEffect(m_TraceID);

				if (pEffect == NULL)
				{
					return true;
				}
			}

			if (pEffect->IsSelectable())
			{
				MEffectTarget* pEffectTarget = pEffect->GetEffectTarget();

				if (pEffectTarget != NULL)
				{
					bool bSendPacket = true;
					if (pEffectTarget->GetEffectTargetType() == MEffectTarget::EFFECT_TARGET_PORTAL)
					{
						if (IsSlayer())
						{
							bSendPacket = false;
						}
					}

					MPortalEffectTarget* pPortalEffectTarget
						= dynamic_cast<MPortalEffectTarget*>(pEffectTarget);

					if (bSendPacket && pPortalEffectTarget != NULL)
					{
						int serverID = pEffectTarget->GetServerID();

						CGSelectTileEffect _CGSelectTileEffect;
						_CGSelectTileEffect.setEffectObjectID(serverID);

						g_pSocket->sendPacket(&_CGSelectTileEffect);
					}
				}
			}

			return true;
		}
		//----------------------------------------------------
		//
		//			InteractionObject����
		//
		//----------------------------------------------------
		else if (m_fTrace & FLAG_TRACE_INTERACTIONOBJECT)
		{
			MImageObject* pImageObject = m_pZone->GetImageObject(m_TraceID);

			if (pImageObject->GetObjectType() == MObject::TYPE_INTERACTIONOBJECT)
			{
				MInteractionObject* pInteractionObject = (MInteractionObject*)pImageObject;
				pInteractionObject->SetNextAction(m_TraceObjectAction);

			}

			return true;
		}

		return false;
	}
	//-------------------------------------------------------
	// �� �����ؾ� �ϴ� ���
	//-------------------------------------------------------
	else
	{
		//------------------------------------------
		// ������ ���� ���� ��� --> ��ã��
		//------------------------------------------
		if (m_listDirection.empty())
		{
			//---------------------
			// �⺻ �ൿ
			//---------------------
			if (m_fTrace & FLAG_TRACE_CREATURE_BASIC)
			{
				TraceCreatureToBasicAction(m_TraceID, m_bTraceCreatureToForceAttack);
			}
			//---------------------
			// Ư�� �ൿ
			//---------------------
			else  if (m_fTrace & FLAG_TRACE_CREATURE_SPECIAL)
			{
				TraceCreatureToSpecialAction(m_TraceID, m_bTraceCreatureToForceAttack);
			}
			//---------------------
			// �⺻ �ൿ -> Sector
			//---------------------
			else if (m_fTrace & FLAG_TRACE_SECTOR_BASIC)
			{
				TraceSectorToBasicAction(m_TraceX, m_TraceY);
			}
			//---------------------
			// Ư�� �ൿ -> Sector
			//---------------------
			else if (m_fTrace & FLAG_TRACE_SECTOR_SPECIAL)
			{
				// 2004, 9, 18, sobeit add start - �ͷ� ���̾�� �����Ÿ� ����� ���� ��.�� 
				if (m_nUsedActionInfo == SKILL_TURRET_FIRE)
				{
					m_fNextTrace = FLAG_TRACE_NULL;
					UnSetRepeatAction();
					InstallTurretStopAttack();
					//SetAction(ACTION_STAND);
					return false;
				}
				// 2004, 9, 18, sobeit add end
				TraceSectorToSpecialAction(m_TraceX, m_TraceY);
			}
			//---------------------
			// Item ����
			//---------------------
			else if (m_fTrace & FLAG_TRACE_ITEM)
			{
				TraceItem(m_TraceID);
			}
			//---------------------
			// Inventory�� ��� ���
			//---------------------
			else if (m_fTrace & FLAG_TRACE_INVENTORY)
			{
				TraceInventoryItem(m_TraceID);
			}
			//---------------------
			// Effect����
			//---------------------
			else if (m_fTrace & FLAG_TRACE_EFFECT)
			{
				TraceEffect(m_TraceID);
			}
			//---------------------
			// InteractionObject����
			//---------------------
			else if (m_fTrace & FLAG_TRACE_INTERACTIONOBJECT)
			{
				TraceInteractionObject(m_TraceID, m_TraceObjectAction);
			}

			// �̵�~�ϸ鼭 ����..
			//SetAction( m_MoveAction );
		}


		//m_NextAction = m_MoveAction;
		SetNextAction(m_MoveAction);
	}

	return false;
}

//----------------------------------------------------------------------
// Move
//----------------------------------------------------------------------
// MCreature::Move�� ��������� MPlayer::Move�� 
//    m_listDirection���� ������ �ϳ��� �о �����̰� �ȴ�.
// 
// ��, ���� ���ٰ� ������ �ٽ� ���� ���ؾ��Ѵ�(SetDest...)
//----------------------------------------------------------------------
void
MPlayer::ActionMove()
{
#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD("ActionMove");
#endif

	//----------------------------------------------------
	//
	// ���� Sector�� ������ ���	
	//
	//----------------------------------------------------
	// m_listDirection���� ������ �ϳ� �о�ͼ� 
	// check�غ��� �����δ�.
	//----------------------------------------------------
	if (HasEffectStatus(EFFECTSTATUS_TRYING))
	{
		MZone::EFFECT_MAP::const_iterator itr = g_pZone->GetGroundEffects();
		bool bFind = false;

		for (int i = 0; i < g_pZone->GetGroundEffectCount(); itr++, i++)
		{
			const MEffect* pEffect = itr->second;

			if (GetX() == pEffect->GetX() &&
				GetY() == pEffect->GetY() &&
				pEffect->GetFrameID() == EFFECTSPRITETYPE_REGEN_TOWER_GROUND)
			{
				bFind = true;
				break;
			}
		}
		if (bFind == false)
			RemoveEffectStatus(EFFECTSTATUS_TRYING);
	}

	if (m_MoveCount == 0)//m_sX==0 && m_sY==0)
	{
		TYPE_SECTORPOSITION	x, y;

		//-------------------------------------------------------------
		// fast move�� ���� nextX, nextY�� ��ǥ�� �ִ�.
		//-------------------------------------------------------------
		if (m_bFastMove)
		{
			x = m_NextX;
			y = m_NextY;

			m_NextX = SECTORPOSITION_NULL;

			//m_CurrentDirection = 
			m_Direction = GetDirectionToPosition(x, y);

		}
		//-------------------------------------------------------------
		// �Ϲ����� ������
		//-------------------------------------------------------------
		else
		{
			//----------------------------------------------------
			// ��ǥ��ġ�� ������ �������� �ʴ´�.
			//----------------------------------------------------
			if (m_DestX == SECTORPOSITION_NULL && m_NextDestX == SECTORPOSITION_NULL)
			{
				DEBUG_ADD("### Don't need to move any more - Move Stop! ###");

				m_MoveCount = m_MoveCountMax;
				return;
			}

			//sprintf(g_pDebugMessage->GetCurrent(), "ListSize = %d", m_listDirection.size());

		//----------------------------------------------------
		// 
		// 1. ��ã�⸦ �ؼ� �̵��ϰ� �ϰ�..
		// 2. Server���� ���� �ޱ� ���� Send..
		//
		//----------------------------------------------------
		// Server�κ��� ��ٸ��� message�� ���� ���		
			if (m_SendMove != 0 && g_bNetStatusGood)
			{
				static int lastSendCount = 0;

				if (lastSendCount != m_SendMove)
				{
					DEBUG_ADD_FORMAT("Waiting MoveOK = %d", m_SendMove);

					lastSendCount = m_SendMove;
				}
			}

			//----------------------------------------------------
			//
			//
			//                 ���� ���̸�..
			//
			//
			//----------------------------------------------------		
			if (m_fTrace != FLAG_TRACE_NULL)
			{
				//----------------------------------------------------	
				// Creature ���� ���̸�..
				// Creature�� ������ų�
				// ���� ��ǥ��ǥ�� �ٲ� �� �����Ƿ� check���ش�.
				//----------------------------------------------------	
				if ((m_fTrace & FLAG_TRACE_CREATURE_BASIC)
					|| (m_fTrace & FLAG_TRACE_CREATURE_SPECIAL))
				{
					// �����ϴ� Creature�� ������ �о�´�.
					MCreature* pCreature = m_pZone->GetCreature(m_TraceID);

					//GET_DYING_CREATURE( pCreature, m_TraceID );

					//-------------------------------------------------------
					// �����ϴ� Creature�� ������� ��� --> ���� ����
					// ���� Slayer�� ���� Darkness�ȿ� �� ĳ���� �Ѿư� �� ����.					
					//-------------------------------------------------------								
					if (pCreature == NULL
						|| pCreature->IsInDarkness() && !pCreature->IsNPC() &&
						(!IsVampire() && !(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) ||
							g_pZone->GetID() == 3001 && IsVampire() ||
							pCreature->IsOusters() && pCreature->IsInGroundElemental() && !g_pPlayer->IsOusters())
						&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
						&& !g_bLight
#endif
						)
					{
						if (IsRepeatAction())
						{
							m_fNextTrace = FLAG_TRACE_NULL;

							// ��ư�� �������Ƿ� �ൿ �ݺ��� ����Ѵ�.
							UnSetRepeatAction();
							//TraceNextNULL();
						}
						else
						{
							// ���� ����
							TraceNULL();

							// �����.
							SetStop();
						}

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
						DEBUG_ADD("creature lost");
#endif

						return;
					}

					//-------------------------------------------------------			
					// �����ҷ��� Creature�� ��ǥ�� �޶����� ���
					//-------------------------------------------------------			
					if (pCreature->GetX() != m_TraceX ||
						pCreature->GetY() != m_TraceY)
					{
						m_NextDestX = pCreature->GetX();
						m_NextDestY = pCreature->GetY();
						// Z�� �������.
					}

					m_TraceX = pCreature->GetX();
					m_TraceY = pCreature->GetY();
					m_TraceZ = pCreature->GetZ();
				}
				//----------------------------------------------------	
				// Item ���� ���̸�..
				// Item�� ����� �� �����Ƿ� check���ش�.
				//----------------------------------------------------
				else if (m_fTrace & FLAG_TRACE_ITEM)
				{
					// �����ϴ� Creature�� ������ �о�´�.
					MItem* pItem = m_pZone->GetItem(m_TraceID);

					//-------------------------------------------------------
					// �����ϴ� Item�� ������� ��� --> ���� ����
					//-------------------------------------------------------
					if (pItem == NULL
						|| g_pZone->GetSector(pItem->GetX(), pItem->GetY()).HasDarkness() &&
						(!IsVampire() && (!(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) ||
							IsVampire() && g_pZone->GetID() == 3001))
						&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
						&& !g_bLight
#endif
						)
					{
						// ���� ����
						TraceNULL();

						// �����.
						SetStop();

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
						DEBUG_ADD("item lost");
#endif

						return;
					}
				}

				//-------------------------------------------------------
				// �ൿ ���� �Ÿ��� �ִ��� �Ǵ��Ѵ�.
				//-------------------------------------------------------
				if (ActionInTraceDistance())
				{
					// ��� ���~~
					if (m_nNextUsedActionInfo != ACTIONINFO_NULL)
					{
						m_nUsedActionInfo = m_nNextUsedActionInfo;
						m_nNextUsedActionInfo = ACTIONINFO_NULL;

						// ���� action����
						m_bNextAction = true;
						// 2004, 11, 23, sobeit add start - ��..����ɷ���..���� �ϰ͵� �ȵ��´�..��.��
						if (m_nUsedActionInfo == SKILL_SET_AFIRE)
						{
							if (max(abs(m_X - m_TraceX), abs(m_Y - m_TraceY)) < 2)
								SetNextAction(ACTION_ATTACK);
							else
								SetNextAction(ACTION_STAND);
						}
						else
							// 2004, 11, 23, sobeit add end
							SetNextAction(GetActionInfoAction(m_nUsedActionInfo));
					}

					// �� �̻� �� �ʿ䰡 ����.
					m_listDirection.clear();

					// Action����
					//m_ActionCount = m_ActionCountMax;
					m_MoveCount = m_MoveCountMax;

					// Player�� �ٶ󺸴� ������ ���� ���� �ִ� ������ �ǵ��� �Ѵ�.				
					SetDirectionToPosition(m_TraceX, m_TraceY);


					//----------------------------------------------------			
					// ��� �����ϴ� ��찡 �ƴϸ� ���� ����
					//----------------------------------------------------
					if (m_bKeepTraceCreature)
					{
						int fTrace = m_fTrace;

						// ����
						SetStop();

						m_fTrace = fTrace;
					}
					else
					{
						TraceNULL();

						// ����
						SetStop();
					}
				}

			}

			//----------------------------------------------------
			// ��ã�� Buffering�ߴ��� �����Ѵ�.
			//----------------------------------------------------
			if (m_NextDestX != SECTORPOSITION_NULL)
			{
				// ��ã��
				//SetDestination(m_NextDestX, m_NextDestY);
				m_DestX = m_NextDestX;	// TEST
				m_DestY = m_NextDestY;	// TEST
				m_NextDestX = SECTORPOSITION_NULL;
				m_NextDestY = SECTORPOSITION_NULL;
			}

			//----------------------------------------------------
			// ��ã�� : �� Tile �̵��ø��� ��ã�⸦ �Ѵ�.
			//----------------------------------------------------
			if (m_DestX != SECTORPOSITION_NULL)
				SetDestination(m_DestX, m_DestY);

			//----------------------------------------------------
			// Server���� �����ޱ� ���ؼ� Send
			//----------------------------------------------------
			//SetAction( m_MoveAction );


			//----------------------------------------------------
			// �������� ���� �������� 
			// ������ ���� ������ ��쿡�� �����̰� �Ѵ�.
			//----------------------------------------------------
			if (m_SendMove > g_pClientConfig->MAX_CLIENT_MOVE)
			{
				g_bNetStatusGood = false;

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
				DEBUG_ADD("max client move");
#endif

				return;
			}
			else
			{
				g_bNetStatusGood = true;
			}

			//----------------------------------------------------
			// �� ���� ���� ���
			//----------------------------------------------------
			if (m_listDirection.empty())
			{
				//--------------------------------------------
				// �������� ������ ���..
				//--------------------------------------------
				if (m_DestX == m_X && m_DestY == m_Y)
				{
					m_DestX = SECTORPOSITION_NULL;
					m_DestY = SECTORPOSITION_NULL;

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
					DEBUG_ADD("dest reached");
#endif

					return;
				}

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
				DEBUG_ADD("no way");
#endif

				return;
			}


			//----------------------------------------------------	
			//
			// ������ �����δ�.
			//
			//----------------------------------------------------			
			//----------------------------------------------------
			// m_listDirection���� ������ �ϳ� �о�´�.
			//----------------------------------------------------
			m_Direction = m_listDirection.front();
			m_listDirection.pop_front();

			//----------------------------------------------------
			// ������ ��ã�⸦ �ؾ��� ���...
			//----------------------------------------------------
			if (m_NextDestX != SECTORPOSITION_NULL)
			{
				//m_NextAction = m_MoveAction;
				SetNextAction(m_MoveAction);
			}

			//----------------------------------------------------
			// �о�� �������� �� Sector�� �����ϰ�
			// cX,cY, sX,sY�� �ٽ� ��������� �Ѵ�.
			//----------------------------------------------------

			//----------------------------------------------------
			// �̵� �������� check
			//----------------------------------------------------
			x = m_X;
			y = m_Y;

			switch (m_Direction)
			{
			case DIRECTION_LEFTDOWN: x--;	y++;	break;
			case DIRECTION_RIGHTUP: x++;	y--;	break;
			case DIRECTION_LEFTUP: x--;	y--;	break;
			case DIRECTION_RIGHTDOWN: x++;	y++;	break;
			case DIRECTION_LEFT: x--;			break;
			case DIRECTION_DOWN:		y++;	break;
			case DIRECTION_UP:		y--;	break;
			case DIRECTION_RIGHT: x++;			break;
			}
		}

		//--------------------------------------------------------------------
		// ������ �� ���� ���		
		//--------------------------------------------------------------------
		if (m_pZone->CanMove(m_MoveType, x, y)
			|| m_bFastMove		// Fast Move�� ���� ������ �̵� ����..

#ifdef __EXPO_CLIENT__
			|| g_UserInformation.Invisible
#endif
			)
		{
			//---------------------------------------------------------------
			// Fast Move�� ���..
			//---------------------------------------------------------------
			if (m_bFastMove)
			{
				// nothing
			}
			//---------------------------------------------------------------
			// �Ϲ����� ������ --> ���� �ʿ�
			//---------------------------------------------------------------
			else
			{
				//----------------------------------------------------
				// ���� ��ġ���̸� ����ϱ�
				//----------------------------------------------------
				if (gC_vs_ui.IsInstallMineProgress())
				{
					gC_vs_ui.EndInstallMineProgress();
				}

				CGMove _CGMove;
				_CGMove.setX(m_X);
				_CGMove.setY(m_Y);
				_CGMove.setDir(m_Direction);
				g_pSocket->sendPacket(&_CGMove);

				// Server�� ���� ������ ����صд�.
				m_listSendDirection.push_back(m_Direction);
				m_SendMove++;


				//----------------------------------------------------
				// NPC���� �� �� ���¿��� �����̴� ���..			
				//----------------------------------------------------
				if (m_WaitVerifyActionInfo == WAIT_VERIFY_NPC_ASK)
				{
					MCreature* pCreature = g_pZone->GetCreature(g_pPCTalkBox->GetNPCID());

					if (pCreature != NULL)
					{
						//----------------------------------------------------
						// �Ÿ��� �־����� ��ȭ ������.
						//----------------------------------------------------
						if ((max(abs(m_X - pCreature->GetX()), abs(m_Y - pCreature->GetY()))) > TRADE_DISTANCE_NPC)
						{
							// ���� ����
							SetWaitVerifyNULL();

							// Dialog ���ش�.
							g_pUIDialog->ClosePCTalkDlg();
						}
					}
				}
			}

			//-----------------------
			// �þ� �ٲ��ֱ�
			//-----------------------
			//m_pZone->MoveLightSight(m_X, m_Y, m_LightSight, m_Direction);
			//m_pZone->UnSetLightSight(m_X, m_Y, m_LightSight);
			//m_pZone->SetLightSight(x, y, m_LightSight);

			// ���� ��ǥ ���
			TYPE_SECTORPOSITION ox = m_X;
			TYPE_SECTORPOSITION oy = m_Y;

			// zone�� sector�� ������ �ٲ��ش�.
			MovePosition(x, y);

			//CheckInDarkness();

			//--------------------------------------------
			//
			// �̵��� ���� '��Ż'�� ���
			//
			//--------------------------------------------
			if (IsSlayer())
			{
				const MSector& sector = m_pZone->GetSector(x, y);
				if (sector.IsPortal())
				{
					int numPortal = sector.GetPortalSize();

					if (numPortal != 0)
					{
						PORTAL_LIST::const_iterator	iPortal = sector.GetPortalBegin();

						PORTAL_INFO portalInfo = *iPortal;

						//--------------------------------------------
						// ��Ƽ ��Ż�̸� elevator�� ����.
						//--------------------------------------------
						if (portalInfo.Type == MPortal::TYPE_MULTI_PORTAL)
						{
							DEBUG_ADD("[MPlayer] Run Elevator");

							gC_vs_ui.RunElevator();

							// ������ ǥ�ø� ���ش�.
							g_pTopView->SetSelectedSectorNULL();

							// ������ ����..
							m_DestX = SECTORPOSITION_NULL;
							m_DestY = SECTORPOSITION_NULL;
							m_NextDestX = SECTORPOSITION_NULL;
							m_NextDestY = SECTORPOSITION_NULL;
						}

						iPortal++;
					}
				}
			}

			//--------------------------------------------
			// 5 Frame�� �ִٰ� �� ���
			//--------------------------------------------
			//
			// [1] UP,DOWN,LEFT,RIGHT�� ��,
			//
			//     X��ȭ : 0 16 32 48 64   (+-16)
			//     Y��ȭ : 0  8 16 24 32   (+-8)
			// 
			//
			// [2] �밢��(LEFTUP,LEFTDOWN,RIGHTUP,RIGHTDOWN)���� �����϶�,
			//
			//     X��ȭ : 0  8 16 24 32   (+-8)
			//     Y��ȭ : 0  4  8 12 16   (+-4)
			//
			//--------------------------------------------
			// sX,sY : ���������� ��ü pixel(�� TILE)
			// cX,cY : �̵��ϴ� ���� pixel
			//--------------------------------------------		
			//--------------------------------------------
			// Fast Move �� ���
			//--------------------------------------------
			if (m_bFastMove)
			{
				// ���� �Ÿ��� �����϶��� pixel�Ÿ�
				// ex) ���� 2 --> 1 : ( 2 - 1 ) * TILE_X
				//     ���� 2 --> 1 : ( 2 - 1 ) * TILE_Y
				m_sX = (ox - m_X) * TILE_X;
				m_sY = (oy - m_Y) * TILE_Y;

				//------------------------------------------------
				// ���� �� Ÿ�� �̵� �ӵ��� ���� �ӵ��� ���������� �̵��Ѵ�.
				//------------------------------------------------
				// �� Ÿ�� �̵��Ҷ��� Frame ��
				int moveTimes_div_2 = (*g_pCreatureTable)[m_CreatureType].MoveTimes >> 1;
				// 2005, 1, 5, sobeit add start
				if (HasEffectStatus(EFFECTSTATUS_BIKE_CRASH))
				{
					//					moveTimes_div_2 = moveTimes_div_2/2;
					m_MoveCountMax = m_MoveCountMax * 2;
				}
				// 2005, 1, 5, sobeit add end
				m_cX = -m_sX / moveTimes_div_2;
				m_cY = -m_sY / moveTimes_div_2;

				m_MoveCount += 2;

				if (m_MoveCount > m_MoveCountMax)
				{
					m_MoveCount = m_MoveCountMax;
				}
			}
			//--------------------------------------------	
			// ���� �̵�
			//--------------------------------------------
			else
			{
				m_DirectionMoved = m_Direction;

				m_sX = m_sXTable[m_DirectionMoved];
				m_sY = m_sYTable[m_DirectionMoved];

				//????????????????????????????????????????????????????????
				//???                                                  ???
				//???   �� Frame�� �̵�...�ؾ��ϴ°�?? ���ƾ� �ϴ°�   ???
				//???                                                  ???
				//????????????????????????????????????????????????????????
				int moveTimes_1;

#ifdef OUTPUT_DEBUG
				if (m_CreatureType >= g_pCreatureTable->GetSize())
				{
					DEBUG_ADD_FORMAT("[Error] CreatureType is exceed MAX. id=%d, type=%d", m_ID, m_CreatureType);

					SetStop();

					return;
				}
#endif

				if (m_MoveDevice == MOVE_DEVICE_WALK)
				{
					moveTimes_1 = (*g_pCreatureTable)[m_CreatureType].MoveTimes - 1;
				}
				else if (m_MoveDevice == MOVE_DEVICE_RIDE)
				{
					moveTimes_1 = (*g_pCreatureTable)[m_CreatureType].MoveTimesMotor - 1;
#if __CONTENTS(__FAST_TRANSFORTER||__SECOND_TRANSFORTER)
					//�����̾��� �ű��̵�����(�����̾�)�� Ÿ�� ���� ��� -- moveTimes�� ������ 1���δ�. ��, ������ 4�������ڸ��� 3����������...
					if (IsRide_Fast_Transforter())
						moveTimes_1--;
#endif //__FAST_TRANSFORTER||__SECOND_TRANSFORTER
				}
				else if ((m_MoveDevice == MOVE_DEVICE_SUMMON_SYLPH || HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this)))
#if __CONTENTS(__FAST_TRANSFORTER)
					|| (m_MoveDevice == MOVE_DEVICE_WING_SYLPH && HasEffectStatusWingSylph(this))
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
					|| (m_MoveDevice == MOVE_DEVICE_WING_SYLPH && HasEffectStatusUnicorn(this))
#endif //__SECOND_TRANSFORTER
					)
				{
					moveTimes_1 = (*g_pCreatureTable)[m_CreatureType].MoveTimesMotor - 1;
				}
				else
				{
					DEBUG_ADD_FORMAT("[Error] What is MoveType of this Player??? id=%d", m_ID);

					SetStop();

					return;
				}

				m_MoveTableCount = 0;
				m_cX = m_cXTable[moveTimes_1][m_DirectionMoved][m_MoveTableCount];
				m_cY = m_cYTable[moveTimes_1][m_DirectionMoved][m_MoveTableCount];
			}

			m_sX += m_cX;
			m_sY += m_cY;

			m_MoveCount++;

			// ������ �̵��� count�� �����Ѵ�.
			m_NextMoveCount = (*g_pCreatureTable)[m_CreatureType].MoveRatio;

			//------------------------------------------------
			// ĳ������ MoveAction�� �´� Sound�� ������ش�.
			//------------------------------------------------
			TYPE_SOUNDID soundID = (*g_pCreatureTable)[m_CreatureType].GetActionSound(m_MoveAction);

			if (soundID != SOUNDID_NULL)
			{
				PlaySound(soundID,
					false,
					m_X,
					m_Y);
			}
		}
		//--------------------------------------------
		// ������ �� ���� ���		
		//--------------------------------------------
		// ��ã�⸦ �ٽ� ���ش�.
		else
		{
			m_NextDestX = m_DestX;
			m_NextDestY = m_DestY;
			m_listDirection.clear();


			DEBUG_ADD("�ٸ� ĳ���Ϳ� ���ؼ� Block����.");
		}

		// ���� Sector���� �����δ�.
		if (!m_listDirection.empty())
		{
			//m_NextAction = m_MoveAction;
			SetNextAction(m_MoveAction);
		}
	}
	//----------------------------------------------------
	//
	// ���� ���� Sector���� �� ������ ���
	//
	//----------------------------------------------------
	else
	{
		// �� Frame�� �̵��Ѵ�.
		/*
		switch (m_MoveDevice)
		{
			case MOVE_DEVICE_NULL :
				m_sX += m_cXTableDeviceNull[m_Direction][m_ActionCount];
				m_sY += m_cYTableDeviceNull[m_Direction][m_ActionCount];
			break;

			case MOVE_DEVICE_MOTOR1 :
				m_sX += m_cXTableDeviceMotor1[m_Direction][m_ActionCount];
				m_sY += m_cYTableDeviceMotor1[m_Direction][m_ActionCount];
			break;
		}
		*/

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
		DEBUG_ADD("going");
#endif

		// �̵��� count�� �������� �̵��Ѵ�.
		if (m_MoveCount >= m_NextMoveCount)
		{
			//--------------------------------------------
			// Fast Move �� ���
			//--------------------------------------------
			if (m_bFastMove)
			{
				// 2001.8.22 �߰� - �ȳ�
				if (m_sX != 0)	m_sX += m_cX;
				if (m_sY != 0)	m_sY += m_cY;

				m_MoveCount += 2;

				if (m_MoveCount > m_MoveCountMax)
				{
					m_MoveCount = m_MoveCountMax;
				}

				// ������ �̵��� count�� �����Ѵ�.
				m_NextMoveCount += (*g_pCreatureTable)[m_CreatureType].MoveRatio;

				if (m_MoveCount + 1 >= m_MoveCountMax
					&& m_nNoPacketUsedActionInfo == ACTIONINFO_NULL)
				{
					//----------------------------------------------------------
					// ������.. �׽�Ʈ �ڵ�.
					//----------------------------------------------------------
					// fast move �Ŀ� �ѹ� ������.. - -
					// Flash Sliding
					//----------------------------------------------------------
					//m_TraceID	= id;		
					MCreature* pCreature = g_pZone->GetCreature(m_TraceID);

					//GET_DYING_CREATURE( pCreature, m_TraceID );

					if (pCreature != NULL
						&& m_nSpecialActionInfo != MAGIC_RAPID_GLIDING
						&& m_nSpecialActionInfo != SKILL_TELEPORT
						&& !HasEffectStatus(EFFECTSTATUS_BIKE_CRASH)
						)		//-_- ���� ���õȰ�...��..
					{
						//						_MinTrace(" SpecialActionInfo : %d UsedAction : %d NextUsedActionInfo :%d\n",
						//							m_nSpecialActionInfo, m_nUsedActionInfo, m_nNextUsedActionInfo);
#ifdef OUTPUT_DEBUG
						DEBUG_ADD("[FastMovePosition] set next action to attack");
#endif

						UnSetRequestMode();

						// ����� ����� ����

						//---------------------------------------------------
						// ������..
						//---------------------------------------------------
						MActionResult* pResult = new MActionResult;

						// ��� ǥ��
						int ActionInfo = GetBasicActionInfo(); //m_nBasicActionInfo;
						if (m_nSpecialActionInfo == SKILL_BLITZ_SLIDING || m_nSpecialActionInfo == SKILL_BLAZE_WALK || m_nSpecialActionInfo == SKILL_BLAZE_WALK_2)
						{
							m_fTrace = FLAG_TRACE_CREATURE_SPECIAL;
							if (m_nSpecialActionInfo == SKILL_BLITZ_SLIDING)
								m_nNextUsedActionInfo = SKILL_BLITZ_SLIDING_ATTACK;
							else if (m_nSpecialActionInfo == SKILL_BLAZE_WALK || m_nSpecialActionInfo == SKILL_BLAZE_WALK_2)
								m_nNextUsedActionInfo = SKILL_BLAZE_WALK_ATTACK;
							else
								return;
							m_TraceX = pCreature->GetX();
							m_TraceY = pCreature->GetY();
							m_TraceZ = pCreature->GetZ();

							m_bTraceCreatureToForceAttack = false;
							m_bKeepTraceCreature = false;
							m_TraceDistance = m_BasicAttackDistance;

							m_bNextAction = true;
							SetNextAction(GetActionInfoAction(m_nNextUsedActionInfo));
							SetDirection(MTopView::GetDirectionToPosition(GetX(), GetY(), pCreature->GetX(), pCreature->GetY()));
							return;
						}
						if (m_nSpecialActionInfo == SKILL_SET_AFIRE)
						{
							m_nNextUsedActionInfo = SKILL_SET_AFIRE;
							m_nNoPacketUsedActionInfo = m_nNextUsedActionInfo;
							m_NextAction = ACTION_ATTACK;

							return;
						}
						//						if(m_nSpecialActionInfo == MAGIC_BLOODY_ZENITH)
						//						{
						//							SetBloodyZenith(16);
						//						}

						m_nNextUsedActionInfo = GetBasicActionInfo(); //m_nBasicActionInfo;

						pResult->Add(new MActionResultNodeActionInfo(
							ActionInfo,
							m_ID,
							pCreature->GetID(),
							pCreature->GetX(),
							pCreature->GetY(),
							0)
						);

						// Player�� ���� ���� ���� ����� ����� �߰���Ų��.
						if (m_nUsedActionInfo == SKILL_BLITZ_SLIDING_ATTACK ||
							m_nUsedActionInfo == SKILL_BLAZE_WALK_ATTACK
							)
							m_nUsedActionInfo = GetBasicActionInfo(); //m_nBasicActionInfo;

						int tempActionInfo = m_nUsedActionInfo;
						m_nUsedActionInfo = GetBasicActionInfo(); //m_nBasicActionInfo;	// ������ �ڵ� - -;

						// ������ ~~  --;
						if (m_listEffectTarget.size() == 0)
						{
							m_pEffectTarget = new MEffectTarget(0);
							m_pEffectTarget->NewEffectID();

							m_listEffectTarget.push_back(m_pEffectTarget);
						}

						PacketAddActionResult(0, pResult);

						m_nUsedActionInfo = tempActionInfo;
						//---------------------------------------------------
						// ��������... - -; 2001.12.10�� �߰�.. ����.
						//---------------------------------------------------

						// ���� ���� ����						
						m_fTrace = FLAG_TRACE_CREATURE_BASIC;
						m_TraceX = pCreature->GetX();
						m_TraceY = pCreature->GetY();
						m_TraceZ = pCreature->GetZ();

						m_bTraceCreatureToForceAttack = true;
						m_bKeepTraceCreature = false;
						m_TraceDistance = m_BasicAttackDistance;

						m_bNextAction = true;
						SetNextAction(GetActionInfoAction(m_nNextUsedActionInfo));
						SetDirection(MTopView::GetDirectionToPosition(GetX(), GetY(), pCreature->GetX(), pCreature->GetY()));

						// ������ �� �� ��Ŷ�� �� ������.
						m_nNoPacketUsedActionInfo = m_nNextUsedActionInfo;
					}
					else
					{
#ifdef OUTPUT_DEBUG
						DEBUG_ADD_FORMAT("[FastMovePosition] there is no such creature.id=%d", m_TraceID);
#endif
					}
				}

			}
			//--------------------------------------------
			// �Ϲ����� �̵��� ���
			//--------------------------------------------
			else
			{
				int moveTimes_1;

				if (m_MoveDevice == MOVE_DEVICE_WALK)
				{
					moveTimes_1 = (*g_pCreatureTable)[m_CreatureType].MoveTimes - 1;
				}
				else if (m_MoveDevice == MOVE_DEVICE_RIDE)
				{
					moveTimes_1 = (*g_pCreatureTable)[m_CreatureType].MoveTimesMotor - 1;
#if __CONTENTS(__FAST_TRANSFORTER||__SECOND_TRANSFORTER)
					if (IsRide_Fast_Transforter())
						moveTimes_1--;
#endif //__FAST_TRANSFORTER||__SECOND_TRANSFORTER
				}
				else if ((m_MoveDevice == MOVE_DEVICE_SUMMON_SYLPH || HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this)))
#if __CONTENTS(__FAST_TRANSFORTER)
					|| (m_MoveDevice == MOVE_DEVICE_WING_SYLPH && HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this)))
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
					|| (m_MoveDevice == MOVE_DEVICE_WING_SYLPH && HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this)))
#endif //__SECOND_TRANSFORTER
					)
				{
					moveTimes_1 = (*g_pCreatureTable)[m_CreatureType].MoveTimesMotor - 1;
				}
				else
				{
					DEBUG_ADD_FORMAT("[Error] What is MoveType of this Creature??? id=%d", m_ID);
					SetStop();
					return;
				}

				m_MoveTableCount++;
				// 2004, 12, 22, sobeit add start - max �� �Ѿ�� ������ ��ǥ�� ����.
				if (m_MoveTableCount > moveTimes_1)
					m_MoveTableCount = moveTimes_1;
				// 2004, 12, 22, sobeit add end
				m_cX = m_cXTable[moveTimes_1][m_DirectionMoved][m_MoveTableCount];
				m_cY = m_cYTable[moveTimes_1][m_DirectionMoved][m_MoveTableCount];

				// 2001.8.22 �߰� - �ȳ�
				if (m_sX != 0)	m_sX += m_cX;
				if (m_sY != 0)	m_sY += m_cY;

				// ������ �̵��� count�� �����Ѵ�.
				m_NextMoveCount += (*g_pCreatureTable)[m_CreatureType].MoveRatio;

				m_MoveCount++;
			}
		}
		else
		{
			m_MoveCount++;
		}

		//m_ActionCount++;
		// frame�� �ٲ��ش�. ������ Frame���� ������ 0��° Frame���� �ٲ۴�.
		//if (++m_ActionCount == (*m_pFrames)[m_Action][m_Direction].GetCount())
		//	m_ActionCount = 0;

		//if (m_ActionCount == (m_ActionCountMax>>1))
		//if (m_MoveCount == (m_MoveCountMax>>1))
		//{				
		//	SetLightSight( m_LightSight );
		//}
	}

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD("moveok");
#endif
}

//----------------------------------------------------------------------
// Attach CastingEffect
//----------------------------------------------------------------------
void
MPlayer::AttachCastingEffect(TYPE_ACTIONINFO nUsedActionInfo, BOOL bForceAttach)
{
	MCreature::AttachCastingEffect(nUsedActionInfo, bForceAttach);

	// ��..
}

//----------------------------------------------------------------------
// Affect UsedActionInfo
//----------------------------------------------------------------------
// m_nUsedActionInfo�� �����Ų��.
//----------------------------------------------------------------------
void
MPlayer::AffectUsedActionInfo(TYPE_ACTIONINFO nUsedActionInfo)
{
	if (nUsedActionInfo == ACTIONINFO_NULL)
	{
		return;
	}

	switch (nUsedActionInfo)
	{
	case SKILL_ACID_STORM:
	{
		if ((*g_pRankBonusTable)[RANK_BONUS_WIDE_STORM].GetStatus() == RankBonusInfo::STATUS_LEARNED)
			nUsedActionInfo = SKILL_ACID_STORM_WIDE;
	}
	break;

	case SKILL_POISON_STORM:
	{
		if ((*g_pRankBonusTable)[RANK_BONUS_WIDE_STORM].GetStatus() == RankBonusInfo::STATUS_LEARNED)
			nUsedActionInfo = SKILL_POISON_STORM_WIDE;
	}
	break;

	case SKILL_KASAS_ARROW:
		break;

	case SKILL_LARSLASH:

		break;

	case SKILL_SWEEP_VICE_1:
	case SKILL_SWEEP_VICE_3:
	case SKILL_SWEEP_VICE_5:
		if (g_pSkillAvailable->IsEnableSkill(SKILL_SWEEP_VICE_1))
		{
		}
		else
			return;
		break;

	// Blaze Walk 2 dashes with its own flame, the nodes of SKILL_BLAZE_WALK_2,
	// instead of Blaze Walk's fire
	case SKILL_BLAZE_WALK_ATTACK:
		if (m_nSpecialActionInfo == SKILL_BLAZE_WALK_2)
		{
			if (m_DelayActionInfo == nUsedActionInfo)
				m_DelayActionInfo = SKILL_BLAZE_WALK_2;

			nUsedActionInfo = SKILL_BLAZE_WALK_2;
		}
		break;
	}
	POINT point;
#ifdef __METROTECH_TEST__
	//	if(g_bLight)
	//	{
	//		int plusX = GetX()-m_TraceX;
	//		int plusY = GetY()-m_TraceY;
	//		int moveX = 0, moveY = 0;
	//		if(plusX != 0)
	//			moveX = plusX/abs(plusX);
	//		if(plusY != 0)
	//			moveY = plusY/abs(plusY);
	//		
	//		g_pPlayer->UnSetRequestMode();
	//		
	//		g_SelectSector.x = GetX()+moveX;
	//		g_SelectSector.y = GetY()+moveY;
	//		if(abs(g_SelectSector.x - m_TraceX) > GetActionInfoRange(nUsedActionInfo))
	//			g_SelectSector.x = GetX();
	//		if(abs(g_SelectSector.y - m_TraceY) > GetActionInfoRange(nUsedActionInfo))
	//			g_SelectSector.y = GetY();
	//		
	//		// ���� ��ǥ��ġ�� �����Ѵ�
	//		g_pPlayer->TraceNULL();
	//		
	//		if (g_pPlayer->SetMovePosition(g_SelectSector.x, g_SelectSector.y))
	//		{
	//			g_pPlayer->SetNextActionToMove();
	//		}
	//		
	//		// ���� �� ���� ������
	//		POINT point;
	//		g_pPlayer->GetNextDestination( point );		
	//		if (point.x==SECTORPOSITION_NULL || point.y==SECTORPOSITION_NULL)
	//		{
	//			// ���� ���� �ִ� ���� ������
	//			g_pPlayer->GetDestination( point );
	//			if (point.x==SECTORPOSITION_NULL || point.y==SECTORPOSITION_NULL)
	//			{
	//				//-_-;;
	//			}
	//			else
	//			{
	//				g_pTopView->SetSelectedSector( point );
	//			}
	//		}
	//		else
	//		{
	//			g_pTopView->SetSelectedSector( point );
	//		}		
	//	}
#endif

	//-----------------------------------------------------
	// �� ������� ����� EffectTarget
	//-----------------------------------------------------
	MEffectTarget* pEffectTarget = NULL;

	DEBUG_ADD_FORMAT("[MPlayer-StartEffect] ActionInfo=%d, ActionCount=%d/%d", nUsedActionInfo, m_ActionCount, m_ActionCountMax);

	//--------------------------------------------------------
	// Casting ActionInfo�ΰ�?
	//--------------------------------------------------------
	BOOL bCastingAction = (*g_pActionInfoTable)[nUsedActionInfo].IsCastingAction();

	//--------------------------------------------------------
	// ��ǥ ��ġ Pixel��ǥ
	//--------------------------------------------------------
	point = MTopView::MapToPixel(m_TraceX, m_TraceY);
	//point.x += m_sX;
	//point.y += m_sY;
	//point.x += TILE_X_HALF;

	//--------------------------------------------------------
	//
	//					Effect ��ǥ ����
	//Z
	//--------------------------------------------------------
	//MEffectTarget* m_pEffectTarget = new MEffectTarget( (*g_pActionInfoTable)[nUsedActionInfo].GetSize() );
	// 
	//--------------------------------------------------------
	// casting action�� ���
	//--------------------------------------------------------
	if (bCastingAction)
	{
		// casting�� EffectTarget�� �����Ѵ�.
		pEffectTarget = new MEffectTarget((*g_pActionInfoTable)[nUsedActionInfo].GetSize());
		pEffectTarget->NewEffectID();

		if (nUsedActionInfo == CASTING_FIRE || nUsedActionInfo == CASTING_WATER ||
			nUsedActionInfo == CASTING_GROUND || nUsedActionInfo == CASTING_FIRE2 ||
			nUsedActionInfo == CASTING_GROUND2 || nUsedActionInfo == CASTING_WATER2)
			pEffectTarget->Set(point.x, point.y, m_TraceZ, GetID());
		else
			pEffectTarget->Set(point.x, point.y, m_TraceZ, m_TraceID);

		//--------------------------------------------------------
		// castingAction������ delay�� ���������,
		// castingAction�� ���� delay�� �����ϸ� �ȵȴ�.
		//--------------------------------------------------------
		// �̰� ������ ����� ����.
		//if (m_DelayActionInfo==nUsedActionInfo) - -;
		{
			pEffectTarget->SetDelayFrame(m_EffectDelayFrame);
			m_DelayActionInfo = ACTIONINFO_NULL;
			m_EffectDelayFrame = 0;
		}

	}
	//--------------------------------------------------------
	// casting action�� �ƴ�.. ���� ����� ���..
	//--------------------------------------------------------
	else
	{
		//--------------------------------------------------------
		// ���� ����̸�.. �̹� new�� �����Ƿ�
		// ��� ����� ��쿡�� EffectTarget�� �����Ѵ�.
		//--------------------------------------------------------
		if (nUsedActionInfo < (*g_pActionInfoTable).GetMinResultActionInfo())
		{
		}
		else
		{
			if (m_pEffectTarget != NULL)
			{
				DEBUG_ADD_FORMAT("RemoveEffectTarget: action=(%d/%d) ActionInfo=%d, pEffectTarget=%x", m_ActionCount, m_ActionCountMax, nUsedActionInfo, m_pEffectTarget);

				RemoveEffectTarget(m_pEffectTarget->GetEffectID());
				MEffectTarget* pTempEffectTarget = m_pEffectTarget;
				m_pEffectTarget = NULL;

				// delete�Ǹ鼭 m_pEffectTarget�� ������ �ȴ� - -;
				delete pTempEffectTarget;
			}

			//----------------------------------------------------------
			// ����� �ϳ� �������� �д�.
			//----------------------------------------------------------
			m_pEffectTarget = new MEffectTarget((*g_pActionInfoTable)[nUsedActionInfo].GetSize());
			m_pEffectTarget->NewEffectID();

			// ���߿� �����ֱ� ���ؼ�..
			AddEffectTarget(m_pEffectTarget);
			//m_listEffectTarget.push_back( pEffectTarget );

			DEBUG_ADD_FORMAT("NewEffectTarget: action=(%d/%d) ActionInfo=%d, pEffectTarget=%x", m_ActionCount, m_ActionCountMax, nUsedActionInfo, m_pEffectTarget);
		}


		if (m_pEffectTarget == NULL)
		{
			DEBUG_ADD_FORMAT("EffectTarget is NULL, newEffectTarget : %x", m_pEffectTarget);

			m_pEffectTarget = new MEffectTarget((*g_pActionInfoTable)[nUsedActionInfo].GetSize());
			m_pEffectTarget->NewEffectID();

			DEBUG_ADD("new EffectTarget OK");
		}

		//		if( nUsedActionInfo == MAGIC_ACID_TOUCH || nUsedActionInfo == MAGIC_BLOODY_NAIL ||
		//			nUsedActionInfo == SKILL_FLOURISH || nUsedActionInfo == SKILL_ABSORB_SOUL)
		if (nUsedActionInfo != ACTIONINFO_NULL && (*g_pActionInfoTable)[nUsedActionInfo].IsAttachSelf())
		{
			//m_pEffectTarget->Set( point.x, point.y, m_TraceZ, m_TraceID );
			m_pEffectTarget->Set(point.x, point.y, m_TraceZ, GetID());
		}
		else
		{
			m_pEffectTarget->Set(point.x, point.y, m_TraceZ, m_TraceID);
		}

		//--------------------------------------------------------
		// Action�� �´� Sound ���
		//--------------------------------------------------------
		// ����� ��츸 ���⼭ ���� ����Ѵ�.
		if (nUsedActionInfo >= g_pActionInfoTable->GetMinResultActionInfo())
		{
			int soundID = (*g_pCreatureTable)[m_CreatureType].GetActionSound(
				GetActionInfoAction(nUsedActionInfo, false, true));

			PlaySound(soundID, false, m_X, m_Y);

		}

		//--------------------------------------------------------
		// ���� �ð� ����
		//--------------------------------------------------------
		if (m_DelayActionInfo == nUsedActionInfo)
		{
			m_pEffectTarget->SetDelayFrame(m_EffectDelayFrame);
			m_DelayActionInfo = ACTIONINFO_NULL;
			m_EffectDelayFrame = 0;
		}

		// �����صΰ�
		pEffectTarget = m_pEffectTarget;

		// ����.. 
		m_pEffectTarget = NULL;
	}

	// �������� Effect�μ� ����صд�.		
	//AddEffectTarget( m_pEffectTarget );

	// ����� ���� ������ ���� - delete�� ����� �����Ҷ�..
	/*
	MActionResult* pResult = new MActionResult;
	pResult->Add( new MActionResultNodeActionInfo( m_ID, m_TraceID, nUsedActionInfo ) );

	m_pEffectTarget->SetResult( pResult );
	*/


	//--------------------------------------------------------
	//
	//					���� ��ġ�� �����Ѵ�.
	//
	//--------------------------------------------------------
	int x, y, z, direction;

	//--------------------------------------------------------
	// User ��ġ���� �����ϴ� ���
	//--------------------------------------------------------
	if ((*g_pActionInfoTable)[nUsedActionInfo].IsStartUser())
	{
		point = MTopView::MapToPixel(m_X, m_Y);
		point.x += m_sX;
		point.y += m_sY;
		//point.x += TILE_X_HALF;

		x = point.x;
		y = point.y;
		z = m_Z + 1;//+60;			
	}
	//--------------------------------------------------------
	// Target ��ġ���� �����ϴ� ���
	//--------------------------------------------------------
	else if ((*g_pActionInfoTable)[nUsedActionInfo].IsStartTarget())
	{
		x = point.x;
		y = point.y;
		z = m_Z;//+60;			
	}

	//--------------------------------------------------------
	// ���߿��� �����ϴ� ���
	//--------------------------------------------------------
	if ((*g_pActionInfoTable)[nUsedActionInfo].IsStartSky())
	{
		z = m_Z + (*g_pActionInfoTable)[nUsedActionInfo].GetValue();

		//direction	= DIRECTION_DOWN;
	}
	//--------------------------------------------------------
	// ���󿡼� �����ϴ� ���
	//--------------------------------------------------------
	//else
	//{
	if (nUsedActionInfo == SKILL_ABSORB_SOUL)
	{
		// m_TraceID �� ���� ��ü�̹Ƿ� �������̴�.		
		direction = g_pTopView->GetDirectionToPosition(
			GetX(), GetY(), m_TraceX, m_TraceY);
	}
	else
		direction = m_Direction;
	//}

	DEBUG_ADD_FORMAT("[MPlayer-StartEffect] Before Generate");

	//--------------------------------------------------------
	//
	//                   Effect����		
	//
	//--------------------------------------------------------
	// the player's own Blaze Walk 2 sound (on its flame node) must not be dropped
	// by the per-second sound cap when it lands among several monsters
	g_bPlayerSkillSound = (nUsedActionInfo == SKILL_BLAZE_WALK_2);

	g_pEffectGeneratorTable->Generate(
		x, y, z,				// ���� ��ġ
		direction, 		// ����
		1,					// power
		nUsedActionInfo,		//	ActionInfoTable����,
		pEffectTarget		// ��ǥ ����
		, GetActionGrade()
	);
	g_bPlayerSkillSound = false;
	ClearActionGrade();

	DEBUG_ADD_FORMAT("[MPlayer-StartEffect] AffectUsedActionInfo OK");
}

//----------------------------------------------------------------------
// Action to Send Packet
//----------------------------------------------------------------------
// packet�� ������...
//----------------------------------------------------------------------
void
MPlayer::ActionToSendPacket(bool bImmediately)
{
	// 	char szBuf[256];
	// 	sprintf(szBuf, "Call ActionToSendPacket: bImmediately = %d\n", (int)bImmediately);
	// 	OutputDebugString(szBuf);

		//--------------------------------------------------------
		// ��� �ൿ�� �ƴ� ��츸 packet�� ������.
		//--------------------------------------------------------
	if ((m_nUsedActionInfo >= g_pActionInfoTable->GetMinResultActionInfo() ||
		m_nUsedActionInfo == ACTIONINFO_NULL))
	{
		return;
	}

	switch (m_nUsedActionInfo)
	{
	case SKILL_ACID_STORM_WIDE:
		m_nUsedActionInfo = SKILL_ACID_STORM;
		break;

	case SKILL_POISON_STORM_WIDE:
		m_nUsedActionInfo = SKILL_POISON_STORM;
		break;
	case SKILL_BURNING_SOUL_CHARGING:
		if (IsBurningSol())
			return;
		break;
	case SKILL_SWEEP_VICE_1:
	case SKILL_SWEEP_VICE_3:
	case SKILL_SWEEP_VICE_5:
		//		if(!GetSweepViewValue()) //�������̽��� ���� ������� �ƴҶ�
		//		{
		//			SetSweepViewValue(1);
		//			g_SelectSector = g_pTopView->GetSelectedSector(g_x, g_y);
		//			ExecuteActionInfoFromMainNode(SKILL_CLIENT_SWEEP_VICE,g_SelectSector.x, g_SelectSector.y, 0,g_pPlayer->GetDirection(),0	,	
		//				g_SelectSector.x, g_SelectSector.y, 0, 0xffff, NULL, false);
		//			return;
		//		}
		if (g_bRButtonDown/*||!GetSweepViewValue()*/)
			return;

		if (2 == GetSweepViewValue() || 3 == GetSweepViewValue())
			m_fTraceBuffer = FLAG_TRACE_SECTOR_SPECIAL;
		break;
	case SKILL_PLASMA_ROCKET_LAUNCHER:
#if __CONTENTS(__SECOND_RARE_SKILL)
	case SKILL_XRL:
	case SKILL_GAE_BULGA:
#endif //__SECOND_RARE_SKILL
		if (g_pCurrentMagazine != NULL)
		{
			int MagazineCount = g_pCurrentMagazine->GetNumber();
			if (MagazineCount > 0)
			{
				g_pCurrentMagazine->SetNumber(MagazineCount - 1);
			}
			else
				return;
		}
		else return;
		break;
	case SKILL_SATELLITE_BOMB:
	case SKILL_SATELLITE_BOMB_2:
		if (g_pCurrentMagazine != NULL)
		{
			int MagazineCount = g_pCurrentMagazine->GetNumber();
			if (MagazineCount > 9)
			{
				g_pCurrentMagazine->SetNumber(MagazineCount - 10);
			}
			else
				return;
		}
		else return;
	}

	/*
	if(m_nUsedActionInfo != MAGIC_TRANSFORM_TO_WOLF &&
	   m_nUsedActionInfo != MAGIC_TRANSFORM_TO_BAT &&
	   m_nUsedActionInfo != SKILL_TRANSFORM_TO_WERWOLF)
	{
		const int TransformDelay = 500;
		(*g_pSkillInfoTable)[MAGIC_TRANSFORM_TO_WOLF].SetAvailableTime(TransformDelay);
		(*g_pSkillInfoTable)[MAGIC_TRANSFORM_TO_BAT].SetAvailableTime(TransformDelay);
		(*g_pSkillInfoTable)[SKILL_TRANSFORM_TO_WERWOLF].SetAvailableTime(TransformDelay);
	}
	*/


	/*
	// [������ Ÿ�̹�]�� ..
	BOOL	bActionStand	= (m_Action==ACTION_STAND || IsSlayer() && m_Action==ACTION_SLAYER_MOTOR_STAND);
	int		actionCountMax_1 = m_ActionCountMax-1;
	BOOL	bEndAction		= (m_ActionCount==actionCountMax_1);// || bActionStand;
	int		StartFrame		= (*g_pActionInfoTable)[m_nUsedActionInfo].GetStartFrame( m_WeaponSpeed );

	int		packetSendTiming = min(actionCountMax_1, StartFrame);
	BOOL	bStartAction	= (m_ActionCount==packetSendTiming);	// packetSendTiming�� ������ 0�̾���.

	if (bActionStand)
	{
		bEndAction = bStartAction;
	}
	*/

	BOOL	bActionStand = (m_Action == ACTION_STAND || IsSlayer() && m_Action == ACTION_SLAYER_MOTOR_STAND || IsOusters() && m_Action == ACTION_OUSTERS_FAST_MOVE_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
		|| IsOusters() && m_Action == ACTION_OUSTERS_WING_STAND
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		|| IsOusters() && m_Action == ACTION_OUSTERS_UNICORN_STAND
#endif //__SECOND_TRANSFORTER
		);
	int		actionCountMax_1 = m_ActionCountMax - 1;
	BOOL	bEndAction = (m_ActionCount == actionCountMax_1) || bActionStand;
	int		StartFrame = (*g_pActionInfoTable)[m_nUsedActionInfo].GetStartFrame(m_WeaponSpeed);

	bool	bStartAction = (m_ActionCount == 0);
	//	BOOL	bStartAction	= (m_ActionCount == StartFrame && !IsGunShotGuidance());


		// Effect�� ���۵Ǵ� ����..
		// (1) StartFrame�� ���
		// (2) ������ ActionFrame�� ���
	bool	bStartEffect = m_ActionCount == StartFrame ||
		StartFrame >= m_ActionCountMax && bEndAction;

	//BOOL	bCastingEffect	= GetActionInfoCastingStartFrame(m_nUsedActionInfo)==m_ActionCount;
		// �ݺ� action�� ���� frame
	bStartAction |= m_bRepeatAction
		&& (*g_pActionInfoTable)[m_nUsedActionInfo].IsUseRepeatFrame()
		&& m_RepeatCount != 0
		&& m_ActionCount == GetActionInfoRepeatStartFrame(m_nUsedActionInfo);

	bStartAction |= bImmediately;

	//----------------------------------------------------------------------
	// [ TEST CODE ]
	//----------------------------------------------------------------------
	// ����� ������ ��������.. - -;;
	// ũ�� ������ ���� ��...
	//----------------------------------------------------------------------
	// EffectTarget�� casting������ ���� �ڿ� �����Ǵµ�,
	// EffectTarget�� ID�� casting���۶��� server�� ������ �ϱ� ������
	// �ӽ÷�... EffectTarget�� ���� �������ѵΰ� ����ߴ�.
	//----------------------------------------------------------------------
	// ��. data member�� �ٲ��. - -;;
	//static MEffectTarget* 


#ifdef OUTPUT_DEBUG
		//DEBUG_ADD_FORMAT("action=(%d/%d) ActionInfo=%d, pEffectTarget=%x", m_ActionCount, m_ActionCountMax, m_nUsedActionInfo, m_pEffectTarget);
#endif

//----------------------------------------------------------
//
//					���� ���� ����..
//
//----------------------------------------------------------
	if (//bStartEffect)
		bStartAction || m_nUsedActionInfo == SKILL_BURNING_SOUL_LAUNCH)
	{

		//----------------------------------------------------------
		// Packet�� �� ������ ����̴�.
		//----------------------------------------------------------
		if (m_nNoPacketUsedActionInfo == m_nUsedActionInfo)
		{
			m_nNoPacketUsedActionInfo = ACTIONINFO_NULL;

			int soundID = (*g_pCreatureTable)[m_CreatureType].GetActionSound(
				GetActionInfoAction(m_nUsedActionInfo, false, true));

			//------------------------------------------------
			// ĳ������ Action�� �´� Sound�� ������ش�.
			//------------------------------------------------
			PlaySound(soundID, false, m_X, m_Y);

			return;
		}

		//----------------------------------------------------------------------
		// �� �ܰ迡�� Packet�� ������ �Ѵ�.
		//----------------------------------------------------------------------
		// ��, ActionEffect()�� ȣ��ƴٰ� �ؼ� ��� ��쿡 
		// �� Packet������ �Ͼ�� ���� �ƴϴ�.
		//
		// Server���� �޾Ƽ� Player�� � Action�� ���� ��쵵 �ֱ� �����̴�.
		//
		// ActionEffect()�� �Ҹ��� ����..
		// Packet�� ������ ������ �����ϴ� ������ �ʿ��� ���̴�.
		//
		// (!) Packet���� ����� ActionInfo�� ���� Instance ID�� �ٿ���� �Ѵ�.
		//     ����� �޾��� �� check�� ���ؼ��̴�.
		//     �׸���, ���� �ֱ��� Instance ID�� �ϳ��� ����Ѵ�.		
		//----------------------------------------------------------------------

		// ����� �����ִ� Action�� �ƴϰ�
		// ������ Player�� ����� Action�� ��쿡�� Packet�� ������.
		if (m_nUsedActionInfo < (*g_pActionInfoTable).GetMinResultActionInfo())
		{

			//--------------------------------------------------------
			// ��� ��� �ð� Delay�� �������ش�.
			// ���� ����� ��쿡��..
			//--------------------------------------------------------
			if (m_Action == ACTION_STAND)
			{
				// ���� ���� ����� 0.3���� delay�� ������.
				m_DelayTime = g_CurrentTime + 300;
			}
			else if (m_bRepeatAction)
			{
			}
			else
			{
				m_DelayTime = g_CurrentTime
					+ GetActionInfoDelay(m_nUsedActionInfo);
				// [������ Ÿ�̹�]�� �������� - �ؾ� �Ѵ�.
				// - (m_ActionCount<<6);	// ������ ActionCount��ŭ delay�� ���ش�.
			}

			//----------------------------------------------------------
			//
			//			������ ������ �ִ� ���..
			//
			//----------------------------------------------------------

				// 
			if (m_pEffectTarget != NULL)
			{
				DEBUG_ADD_FORMAT("RemoveEffectTarget: action=(%d/%d) ActionInfo=%d, pEffectTarget=%x", m_ActionCount, m_ActionCountMax, m_nUsedActionInfo, m_pEffectTarget);

				RemoveEffectTarget(m_pEffectTarget->GetEffectID());

				MEffectTarget* pTempEffectTarget = m_pEffectTarget;
				m_pEffectTarget = NULL;

				delete pTempEffectTarget;
				pTempEffectTarget = NULL;
			}

			//----------------------------------------------------------
			// ����� �ϳ� �������� �д�.
			//----------------------------------------------------------
			m_pEffectTarget = new MEffectTarget((*g_pActionInfoTable)[m_nUsedActionInfo].GetSize());

			DEBUG_ADD_FORMAT("NewEffectTarget: action=(%d/%d) ActionInfo=%d, pEffectTarget=%x", m_ActionCount, m_ActionCountMax, m_nUsedActionInfo, m_pEffectTarget);

			//---------------------------------------------------------
			// �������� ����μ� �߰��Ѵ�.
			//----------------------------------------------------------
			AddEffectTarget(m_pEffectTarget);

			//----------------------------------------------------------
			// ���ο� EffectID�� �߱�?�޴´�.
			//----------------------------------------------------------
			m_pEffectTarget->NewEffectID();


			if (m_fTraceBuffer != FLAG_TRACE_NULL)
			{
				//----------------------------------------------------------
				// ������ �� ����� ����� �� �ִ� �ð� ����
				//----------------------------------------------------------
				int originalActionInfo = m_nUsedActionInfo;
				if ((*g_pActionInfoTable)[m_nUsedActionInfo].GetParentActionInfo() != ACTIONINFO_NULL)
					originalActionInfo = (*g_pActionInfoTable)[m_nUsedActionInfo].GetParentActionInfo();
				g_pSkillInfoTable->SetNextAvailableTime(originalActionInfo);

				//----------------------------------------------------------
				// ������ packet ���� ����
				//----------------------------------------------------------
				ACTIONINFO_PACKET	packetType = (*g_pActionInfoTable)[m_nUsedActionInfo].GetPacketType();

				//Test - 080125		: Skill Cool Time Check
				//				if (! (m_fTraceBuffer & FLAG_TRACE_CREATURE_BASIC) )
									//�⺻������ �ƴ� ���� ȣ���� ����.
				//					TestCheckSkillCooltime (m_nUsedActionInfo, m_WeaponSpeed);		
				//

								//----------------------------------------------------------
								//
								//			��뿡�� ����ϴ� ���̸�..
								//
								//----------------------------------------------------------
								// �⺻ ����
								//----------------------------------------------------------
				if (m_fTraceBuffer & FLAG_TRACE_CREATURE_BASIC)
				{
					MCreature* pCreature = g_pZone->GetCreature(m_TraceID);

					if (pCreature && g_pObjectSelector->CanAttack(pCreature))
					{
						CGAttack _CGAttack;
						_CGAttack.setX(m_X);
						_CGAttack.setY(m_Y);
						_CGAttack.setDir(m_Direction);
						_CGAttack.setObjectID(m_TraceID);
						_CGAttack.setActionStep(m_ActionStep);

						g_pSocket->sendPacket(&_CGAttack);
					}

					// [ TEST CODE ] - -;;
					// [ TEST CODE ] - -;;
					// [ TEST CODE ] - -;;
					/*
					#if defined(OUTPUT_DEBUG) && defined(__DEBUG_OUTPUT__) && defined(_DEBUG)
						if (g_DXInput.KeyDown(DIK_LMENU))
						{
							for (int i=0; i<200; i++)
							{
								g_pSocket->sendPacket( &_CGAttack );
							}
						}
					#endif
					*/

				}
				//----------------------------------------------------------
				//
				// Blood drain�� ���
				//
				//----------------------------------------------------------
				else if (packetType == ACTIONINFO_PACKET_BLOOD_DRAIN)
				{
					CGBloodDrain _CGBloodDrain;
					_CGBloodDrain.setObjectID(m_TraceID);
					g_pSocket->sendPacket(&_CGBloodDrain);
					//		DEBUG_ADD("���� �޼��� ����");									
							// 2001.8.20 �ּ�ó��
		//					SetWaitVerify( WAIT_VERIFY_SKILL_SUCCESS, m_nUsedActionInfo );
				}
				//----------------------------------------------------------
				//
				// ������ ���
				//
				//----------------------------------------------------------
				else if (packetType == ACTIONINFO_PACKET_ABSORB_SOUL)
				{
					CGAbsorbSoul _CGAbsorbSoul;
					_CGAbsorbSoul.setObjectID(g_pZone->GetItemID(m_TraceX, m_TraceY));
					_CGAbsorbSoul.setTargetZoneX(m_TraceX);
					_CGAbsorbSoul.setTargetZoneY(m_TraceY);

					int larvaSize = (*g_pItemTable)[ITEM_CLASS_LARVA].GetSize();
					MItem* pLarvaItem = NULL;
					for (int i = 0; i < larvaSize; i++)
					{
						pLarvaItem = g_pInventory->FindItem(ITEM_CLASS_LARVA, i);
						if (pLarvaItem != NULL)
							break;
					}

					if (pLarvaItem == NULL)
					{
						_CGAbsorbSoul.setInvenObjectID(OBJECTID_NULL);
						_CGAbsorbSoul.setInvenX(255);
						_CGAbsorbSoul.setInvenY(255);
						_CGAbsorbSoul.setTargetInvenX(255);
						_CGAbsorbSoul.setTargetInvenY(255);
					}
					else
					{
						_CGAbsorbSoul.setInvenObjectID(pLarvaItem->GetID());
						_CGAbsorbSoul.setInvenX(pLarvaItem->GetGridX());
						_CGAbsorbSoul.setInvenY(pLarvaItem->GetGridY());

						MOustersPupa pupa;

						// ��ٿ� ���� Ÿ������
						pupa.SetItemType(pLarvaItem->GetItemType());

						POINT p;
						g_pInventory->GetFitPosition(&pupa, p);
						_CGAbsorbSoul.setTargetInvenX(p.x);
						_CGAbsorbSoul.setTargetInvenY(p.y);

						// (!!!) ���� packet�� ���������� item�� �� �����̵��� �ؾ��Ѵ�.
						// Item���� ���� �ߴ�
						SetItemCheckBuffer(pLarvaItem, ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY);

						// ��� ��� �ߴ�
						// 2001.8.20 �ּ�ó��
						// 2001.10.26 �ּ����� - Item�� �����Ǿ�� �Ѵ�.
						SetWaitVerify(WAIT_VERIFY_SKILL_SUCCESS, m_nUsedActionInfo);
					}

					g_pSocket->sendPacket(&_CGAbsorbSoul);


					// 2001.8.20 �ּ�ó��
					SetWaitVerify(WAIT_VERIFY_SKILL_SUCCESS, m_nUsedActionInfo);
				}
				//----------------------------------------------------------
				//
				// �ٸ� ������� ����ϴ� packet�� ������ ���
				//
				//----------------------------------------------------------
				else if (packetType == ACTIONINFO_PACKET_OTHER
					|| (m_fTraceBuffer & FLAG_TRACE_CREATURE_SPECIAL))
				{
#ifndef __EXPO_CLIENT__

					//-----------------------------------------------
					// item�� ����ؼ� skill�� ����ϴ� ���
					//-----------------------------------------------					
					if ((*g_pActionInfoTable)[m_nUsedActionInfo].GetWeaponType()
						& FLAG_ACTIONINFO_WEAPON_HOLY_WATER)
					{
						MItem* pUsingItem = g_pInventory->FindItem(ITEM_CLASS_HOLYWATER);

						//-----------------------------------------------
						// ����� item�� ���� ���
						//-----------------------------------------------
						if (pUsingItem == NULL)
						{
							//-------------------------------------------------
							// Skill Icon üũ - �ʿ��ҷ���.. - -;
							//-------------------------------------------------
							g_pSkillAvailable->SetAvailableSkills();

							// ���� ����
							TraceNULL();

							//------------------------------------------
							// ���ڸ����� ���Ÿ��� ���
							//------------------------------------------
							SetAction(ACTION_STAND);

							m_ActionCount++;

							m_nUsedActionInfo = ACTIONINFO_NULL;

							return;
						}

						CGThrowItem	_CGThrowItem;
						_CGThrowItem.setObjectID(pUsingItem->GetID());	// item id
						_CGThrowItem.setTargetObjectID(m_TraceID);	//
						_CGThrowItem.setX(pUsingItem->GetGridX());
						_CGThrowItem.setY(pUsingItem->GetGridY());

						g_pSocket->sendPacket(&_CGThrowItem);

						//--------------------------------------------------
						// 1�� �̻��� ��쿡�� ������ ���δ�.
						//--------------------------------------------------
						if (pUsingItem->GetNumber() > 1)
						{
							pUsingItem->SetNumber(pUsingItem->GetNumber() - 1);
						}
						//--------------------------------------------------
						// 1���� ���� ���� Item�� ������ �����Ѵ�
						//--------------------------------------------------
						else
						{
							MItem* pRemovedItem = g_pInventory->RemoveItem(pUsingItem->GetGridX(), pUsingItem->GetGridY());

							if (pRemovedItem != NULL)
							{
								// item���� ����
								UI_RemoveDescriptor((void*)pRemovedItem);

								delete pRemovedItem;
							}
							else
							{
								DEBUG_ADD_FORMAT("[Error] Cannot Remove Item to use Skill. id=%d", pUsingItem->GetID());
							}

							//-------------------------------------------------
							// Skill Icon üũ
							//-------------------------------------------------
							g_pSkillAvailable->SetAvailableSkills();
						}
					}
					//-----------------------------------------------
					// ��ź�� ����ؼ� skill�� ����ϴ� ���
					//-----------------------------------------------					
					if ((*g_pActionInfoTable)[m_nUsedActionInfo].GetWeaponType()
						& FLAG_ACTIONINFO_WEAPON_BOMB)
					{
						int itemType;

						MItem* pUsingItem = NULL;

						if (m_nUsedActionInfo == SKILL_THROW_BOMB)
						{
							pUsingItem = g_pInventory->FindItem(ITEM_CLASS_BOMB);
						}
						else
						{
							itemType = m_nUsedActionInfo - BOMB_SPLINTER;

							pUsingItem = g_pInventory->FindItem(ITEM_CLASS_BOMB, itemType);
						}

						//-----------------------------------------------
						// ����� item�� ���� ���
						//-----------------------------------------------
						if (pUsingItem == NULL)
						{
							//-------------------------------------------------
							// Skill Icon üũ - �ʿ��ҷ���.. - -;
							//-------------------------------------------------
							g_pSkillAvailable->SetAvailableSkills();

							// ���� ����
							TraceNULL();

							//------------------------------------------
							// ���ڸ����� ���Ÿ��� ���
							//------------------------------------------
							SetAction(ACTION_STAND);

							m_ActionCount++;

							m_nUsedActionInfo = ACTIONINFO_NULL;

							return;
						}

						CGThrowBomb	cgThrowBomb;

						cgThrowBomb.setZoneX(m_TraceX);
						cgThrowBomb.setZoneY(m_TraceY);
						cgThrowBomb.setBombX(pUsingItem->GetGridX());
						cgThrowBomb.setBombY(pUsingItem->GetGridY());
						cgThrowBomb.setAttackSlayerFlag(m_TraceID != OBJECTID_NULL);

						g_pSocket->sendPacket(&cgThrowBomb);

						//--------------------------------------------------
						// 1�� �̻��� ��쿡�� ������ ���δ�.
						//--------------------------------------------------
						if (pUsingItem->GetNumber() > 1)
						{
							pUsingItem->SetNumber(pUsingItem->GetNumber() - 1);
						}
						//--------------------------------------------------
						// 1���� ���� ���� Item�� ������ �����Ѵ�
						//--------------------------------------------------
						else
						{
							MItem* pRemovedItem = g_pInventory->RemoveItem(pUsingItem->GetGridX(), pUsingItem->GetGridY());

							if (pRemovedItem != NULL)
							{
								// item���� ����
								UI_RemoveDescriptor((void*)pRemovedItem);

								delete pRemovedItem;
							}
							else
							{
								DEBUG_ADD_FORMAT("[Error] Cannot Remove Item to use Skill. id=%d", pUsingItem->GetID());
							}

							//-------------------------------------------------
							// Skill Icon üũ
							//-------------------------------------------------
							g_pSkillAvailable->SetAvailableSkills();
						}
					}
					else
					{
						int SkillType = m_nUsedActionInfo;
						if ((*g_pActionInfoTable)[SkillType].GetParentActionInfo() != ACTIONINFO_NULL)
							SkillType = (*g_pActionInfoTable)[SkillType].GetParentActionInfo();
						// 2005.07.13 Combo Skill Add Sjheon	
						//if(!(*g_pActionInfoTable)[SkillType].GetComboAttackSkill())  
						//	m_ComboCnt = 0  ;
						//else
						//	m_ComboCnt = GetComboEndValue(); 
						// 2005.07.13 Combo Skill End Sjheon


						if (SkillType == SKILL_TELEPORT || SkillType == SKILL_CHARGING_ATTACK)
						{
							g_pPlayer->SetDelay(1000);
						}

						CGSkillToObject _CGSkillToObject;
						_CGSkillToObject.setSkillType(SkillType);
						_CGSkillToObject.setCEffectID(m_pEffectTarget->GetEffectID());
						_CGSkillToObject.setTargetObjectID(m_TraceID);
						_CGSkillToObject.setActionStep(GetActionStep());

						//_CGSkillToObject.setCombo(m_ComboCnt); // 2005.07.13  // �޺� ī��Ʈ 	
						g_pSocket->sendPacket(&_CGSkillToObject);

						//Test - 071207
						/*
						#ifdef __DEBUG_OUTPUT__
										static test_LastTime = 0;
										static test_LastSkillType = 0;

										if (test_LastSkillType == SkillType)
										{
											char test_SkillName[128];
											switch                           (SkillType)
											{
											case 378: wsprintf (test_SkillName, " �� : �����̵��޺�(Blade Combo) : 10 ~ 10"); break;
											case 405: wsprintf (test_SkillName, " �� : �� ũ���� ��ũ(Bomb Crash Walk) : 300 ~ 300"); break;
											case 254: wsprintf (test_SkillName, " �� : �������� ��ũ(Blaze Walk) : 30 ~ 50"); break;
											case 129: wsprintf (test_SkillName, " �� : ������Ʈ ����(Moonlight Sever) : 8 ~ 8"); break;
											case 27: wsprintf (test_SkillName, " �� : ����̵� ����(Tornado Server) : 10 ~ 20"); break;
											case 26: wsprintf (test_SkillName, " �� : �㸮���� �޺�(Hurricane Combo) : 10 ~ 10"); break;
											case 21: wsprintf (test_SkillName, " �� : ���ϵ� ���Ž�(Wild Smash) : 10 ~ 10"); break;
											case 20: wsprintf (test_SkillName, " �� : Ʈ���� �극��ũ(Triple Break) : 10 ~ 10"); break;
											case 19: wsprintf (test_SkillName, " �� : �����̷� ������(Spiral Slay) : 10 ~ 10"); break;
											case 18: wsprintf (test_SkillName, " �� : �̱� ���ο�(Single Blow) : 10 ~ 10"); break;

											case 404: wsprintf (test_SkillName, " �� : ���� ������(Shine Swork) : 100 ~ 450"); break;
											case 377: wsprintf (test_SkillName, " �� : �ҵ� �޺�(Sword Combo) : 10 ~ 10"); break;
											case 253: wsprintf (test_SkillName, " �� : ������ �����̵�(Blitz Sliding) : 20 ~ 40"); break;
											case 12: wsprintf (test_SkillName, " �� : ����Ʈ�� �ڵ�(Lightning Hand) : 4 ~ 4"); break;
											case 14: wsprintf (test_SkillName, " �� : �ҵ� ���̺�(SWORD Wave) : 10 ~ 10"); break;
											case 13: wsprintf (test_SkillName, " �� : ������ũ �޺�(Snake Combo) : 10 ~ 10"); break;
											case 7: wsprintf (test_SkillName, " �� : ���κ��� ������(Rainbow Slasher) : 10 ~ 10"); break;
											case 10: wsprintf (test_SkillName, " �� : ũ�ν� ī����(Cross Counter) : 20 ~ 50"); break;
											case 8: wsprintf (test_SkillName, " �� : ��� ����ũ(Thunder Spark) : 10 ~ 10"); break;
											case 6: wsprintf (test_SkillName, " �� : Ʈ���� ������(Triple Slasher) : 10 ~ 10"); break;
											case 5: wsprintf (test_SkillName, " �� : ���� ����Ʈ(Double Impact) : 10 ~ 10"); break;

											case 406: wsprintf (test_SkillName, " �� : ���Ͷ���Ʈ ��(Satellite Bomb) : 100 ~ 450"); break;

											case 417: wsprintf (test_SkillName, " ���� : ���� �����(Tanning) : 400 ~ 400"); break;

											case 379: wsprintf (test_SkillName, " ���� : �����̾� �޺�(Vampire Combo) : 10 ~ 10"); break;
											case 328: wsprintf (test_SkillName, " ���� : ���̾Ʈ ����(Violent Phantom) : 0 ~ 0"); break;
											case 414: wsprintf (test_SkillName, " ���� : ���� ����(Blood Curse) : 450 ~ 450"); break;
											case 276: wsprintf (test_SkillName, " ���� : Ż�� ���� ũ�ο�(Talon OF Crow) : 6 ~ 6"); break;
											case 93: wsprintf (test_SkillName, " ���� : ������ ����(Bloody Nail) : 5 ~ 20"); break;
											case 85: wsprintf (test_SkillName, " ���� : �ֽõ� ��ġ(Acid Touch) : 6 ~ 6"); break;
											case 199: wsprintf (test_SkillName, " ���� : ���� ���� �����(Hands OF Wisdon) : 6 ~ 6"); break;
											case 80: wsprintf (test_SkillName, " ���� : ������ʽ� ����(Poisonous Hands) : 6 ~ 6"); break;

											case 365: wsprintf (test_SkillName, " �ƿ� : ���Ϸ�(Halo) : 150 ~ 50"); break;
											case 380: wsprintf (test_SkillName, " �ƿ� : �ƿ콺���� �޺�(Ousters Combo) : 10 ~ 10"); break;
											case 397: wsprintf (test_SkillName, " �ƿ� : �ֵθ���(Brandish) : 50 ~ 200"); break;
											case 219: wsprintf (test_SkillName, " �ƿ� : �÷�����(Flourish) : 10 ~ 10"); break;
											case 400: wsprintf (test_SkillName, " �ƿ� : ���̾� ���̺�(Fire Wave) : 50 ~ 200"); break;
											case 223: wsprintf (test_SkillName, " �ƿ� : �齺��(Back Stab) : 10 ~ 10"); break;
											case 304: wsprintf (test_SkillName, " �ƿ� : ���Ͻ� ������(Distance Blitz) : 10 ~ 70"); break;
											case 225: wsprintf (test_SkillName, " �ƿ� : ���� ��(Cammna Chop) : 10 ~ 40"); break;
											case 246: wsprintf (test_SkillName, " �ƿ� : ����(Absorb Soul) : 0 ~ 0"); break;
											case 221: wsprintf (test_SkillName, " �ƿ� : ���� ����(Sharp Round) : 10 ~ 10"); break;
											case 296: wsprintf (test_SkillName, " �ƿ� : ����Ʈ �극��ũ(Shift Break) : 10 ~ 50"); break;
											case 297: wsprintf (test_SkillName, " �ƿ� : ���� �ǵ�(Water Shield) : 10 ~ 10"); break;
											case 298: wsprintf (test_SkillName, " �ƿ� : ��Ʈ���� ���Ǿ�(Destruction Spear) : 10 ~ 50"); break;
											case 299: wsprintf (test_SkillName, " �ƿ� : ������ ���̾�(Bless Fire) : 10 ~ 10"); break;
											case 300: wsprintf (test_SkillName, " �ƿ� : ����Ʋ ����ũ(Fatal Snick) : 10 ~ 10"); break;
											case 301: wsprintf (test_SkillName, " �ƿ� : ���� ũ�ν�(Sand Cross) : 10 ~ 10"); break;
											case 302: wsprintf (test_SkillName, " �ƿ� : ��ŷ ������(Ducking Wallop) : 20 ~ 100"); break;
											case 303: wsprintf (test_SkillName, " �ƿ� : ��¡ ����(Charging Attack) : 50 ~ 100"); break;
											case 399: wsprintf (test_SkillName, " �ƿ� : ���İ���(Sound Wave) : 50 ~ 200"); break;

											default : wsprintf (test_SkillName, " �� ���"); break;
											}
											int		StartFrame		= (*g_pActionInfoTable)[m_nUsedActionInfo].GetStartFrame( m_WeaponSpeed );

											std::ofstream file("SkillDelay.log", std::ios::out | std::ios::app);
											file << "*** SkillType=" << SkillType << test_SkillName << " ***" << std::endl;//"LastTime=" << test_LastTime << ", CurrentTime=" << g_CurrentTime << std::endl;
											file << "     WeaponSpeed=" << m_WeaponSpeed << "StartFrame =" << StartFrame << std::endl;
											file << "     Difftime=" << g_CurrentTime - test_LastTime << std::endl;

											file.close();
											test_LastTime = g_CurrentTime;
										}
										else
										{
											test_LastSkillType = SkillType;
											test_LastTime = g_CurrentTime;
										}
						#endif
						*/
						//char cstrOutput[256] ; 
						//wsprintf(cstrOutput , "GCSkillToObjectOK1Handler _Seder() %d\n" ,SkillType  ) ; 

						//OutputDebugString(cstrOutput) ; 
					}

#else
					// �ӽ÷�..
					// BOMB����� target�� object�� tile�̹Ƿ�..
					// ������ tile�� �����ش�.
					if (m_nUsedActionInfo == BOMB_TWISTER)
					{
						int SkillType = m_nUsedActionInfo;
						if ((*g_pActionInfoTable)[SkillType].GetParentActionInfo() != ACTIONINFO_NULL)
							SkillType = (*g_pActionInfoTable)[SkillType].GetParentActionInfo();

						CGSkillToTile _CGSkillToTile;
						_CGSkillToTile.setSkillType(SkillType);
						_CGSkillToTile.setCEffectID(m_pEffectTarget->GetEffectID());
						_CGSkillToTile.setX(m_TraceX);
						_CGSkillToTile.setY(m_TraceY);

						g_pSocket->sendPacket(&_CGSkillToTile);
					}
					else
					{
						int SkillType = m_nUsedActionInfo;
						if ((*g_pActionInfoTable)[SkillType].GetParentActionInfo() != ACTIONINFO_NULL)
							SkillType = (*g_pActionInfoTable)[SkillType].GetParentActionInfo();

						if (SkillType == SKILL_TELEPORT || SkillType == SKILL_CHARGING_ATTACK)
						{
							g_pPlayer->SetDelay(1000);
						}

						CGSkillToObject _CGSkillToObject;
						_CGSkillToObject.setSkillType(SkillType);
						_CGSkillToObject.setCEffectID(m_pEffectTarget->GetEffectID());
						_CGSkillToObject.setTargetObjectID(m_TraceID);
						_CGSkillToObject.setSkillStep(GetSkillStep());

						g_pSocket->sendPacket(&_CGSkillToObject);
					}
#endif

					// 2001.8.20 �ּ�ó��
					SetWaitVerify(WAIT_VERIFY_SKILL_SUCCESS, m_nUsedActionInfo);
				}
				//----------------------------------------------------------
				//
				//			�ڽſ��� ����ϴ� ���̸�..
				//
				//----------------------------------------------------------
				else if (packetType == ACTIONINFO_PACKET_SELF
					|| (m_fTraceBuffer & FLAG_TRACE_SELF))
				{
					//----------------------------------------------------------
					// OLD_PACKET
					//----------------------------------------------------------
					///*
					int originalActionInfo = m_nUsedActionInfo;
					if ((*g_pActionInfoTable)[originalActionInfo].GetParentActionInfo() != ACTIONINFO_NULL)
						originalActionInfo = (*g_pActionInfoTable)[originalActionInfo].GetParentActionInfo();

					CGSkillToSelf _CGSkillToSelf;
					_CGSkillToSelf.setSkillType(originalActionInfo);
					_CGSkillToSelf.setCEffectID(m_pEffectTarget->GetEffectID());
					g_pSocket->sendPacket(&_CGSkillToSelf);
					// 2001.8.20 �ּ�ó��
					SetWaitVerify(WAIT_VERIFY_SKILL_SUCCESS, m_nUsedActionInfo);
					//*/

				}
				//----------------------------------------------------------
				//
				//				Tile�� ����ϴ� ���̸�..
				//
				//----------------------------------------------------------
				else if (packetType == ACTIONINFO_PACKET_ZONE
					|| (m_fTraceBuffer & FLAG_TRACE_SECTOR_BASIC)
					|| (m_fTraceBuffer & FLAG_TRACE_SECTOR_SPECIAL))
				{
					//----------------------------------------------------------
					// OLD_PACKET
					//----------------------------------------------------------
					///*
					int SkillType = m_nUsedActionInfo;
					if ((*g_pActionInfoTable)[SkillType].GetParentActionInfo() != ACTIONINFO_NULL)
						SkillType = (*g_pActionInfoTable)[SkillType].GetParentActionInfo();

					if (SkillType == SKILL_TELEPORT || SkillType == SKILL_CHARGING_ATTACK)
					{
						g_pPlayer->SetDelay(1000);
					}
					else if (SkillType == SKILL_SWEEP_VICE_1)
					{
						if (1 == GetSweepViewValue())
							SkillType = SKILL_SWEEP_VICE_1;
						else if (2 == GetSweepViewValue())
							SkillType = SKILL_SWEEP_VICE_3;
						else if (3 == GetSweepViewValue())
							SkillType = SKILL_SWEEP_VICE_5;
						m_TraceX = m_SweepViceX;
						m_TraceY = m_SweepViceY;
					}
					else 	if (SkillType == SKILL_BURNING_SOUL_LAUNCH)
					{
						m_TraceX = m_TempSelectPosX;
						m_TraceY = m_TempSelectPosY;
					}

					CGSkillToTile _CGSkillToTile;
					_CGSkillToTile.setSkillType(SkillType);
					_CGSkillToTile.setCEffectID(m_pEffectTarget->GetEffectID());
					_CGSkillToTile.setX(m_TraceX);
					_CGSkillToTile.setY(m_TraceY);
					g_pSocket->sendPacket(&_CGSkillToTile);

					// 					char szBuf[256];
					// 					sprintf(szBuf, "Send CGSkillToTile: TX = %d, TY = %d\n", m_TraceX, m_TraceY);
					// 					OutputDebugString(szBuf);

										// 2001.8.20 �ּ�ó��
					SetWaitVerify(WAIT_VERIFY_SKILL_SUCCESS, m_nUsedActionInfo);
					//*/
				}
				//----------------------------------------------------------
				//
				//				Inventory�� ����ϴ� ���̸�..
				//
				//----------------------------------------------------------
				else if (packetType == ACTIONINFO_PACKET_ITEM
					|| (m_fTraceBuffer & FLAG_TRACE_INVENTORY))
				{
					MItem* pItem = g_pInventory->GetItemToModify(m_TraceID);

					if (pItem != NULL && IsItemCheckBufferNULL())
					{
						// inventory�� Item�� ����� ����Ѵٰ� packetó���� ���ָ� �ȴ�.
						CGSkillToInventory _CGSkillToInventory;

						_CGSkillToInventory.setSkillType(m_nUsedActionInfo);
						//_CGSkillToInventory.setCEffectID( m_pEffectTarget->GetEffectID() );
						_CGSkillToInventory.setObjectID(m_TraceID);
						_CGSkillToInventory.setX(pItem->GetGridX());
						_CGSkillToInventory.setY(pItem->GetGridY());


						// (!!!) ���� packet�� ���������� item�� �� �����̵��� �ؾ��Ѵ�.
						// Item���� ���� �ߴ�
						SetItemCheckBuffer(pItem, ITEM_CHECK_BUFFER_SKILL_TO_INVENTORY);

						// ��� ��� �ߴ�
						// 2001.8.20 �ּ�ó��
						// 2001.10.26 �ּ����� - Item�� �����Ǿ�� �Ѵ�.
						SetWaitVerify(WAIT_VERIFY_SKILL_SUCCESS, m_nUsedActionInfo);
					}

				}


			}
		}


		//----------------------------------------------------------------------
		// Packet ����� ���� ��쿡..
		//----------------------------------------------------------------------
		// Client���� ���� Instance ID�� ���� �޾Ƽ�..
		// �� ID�� ���� ����� InstanceID�� ������ .. ���� �� ActionInfo��
		// �������̶�� �ǹ��̴�. Player�� ActionInfo�� ��������
		// Instance ID�� 0����? �ؾ����� ������..
		//
		// ������.. Server���� ���ƿ� ActionInfo�� ����� �����ѵΰ�  
		//          ���߿�~~~ ����ϰ�..(ActionInfo�� �����Ŀ�)
		// �ٸ���.. ��� ����ϸ� �ȴ�.
		//
		// �̷��� ������ �ð����� �ξ.. Packetó���� ���� �帧�� ������ �Ѵ�.
		//----------------------------------------------------------------------

		// casting������ ������ ���⼭ ǥ���ƴ�. - -;

		//------------------------------------------------
		// ����� ���ۿ� �´� sound�� ������ش�.
		//------------------------------------------------
		//g_Sound.Play( g_SoundTable[(*g_pActionInfoTable)[m_nUsedActionInfo].GetSoundID()].pDSBuffer );
		//PlaySound( (*g_pActionInfoTable)[m_nUsedActionInfo].GetSoundID() ,
		//			false,
		//			m_X, m_Y);	

		//------------------------------------------------
		// ĳ������ Action�� �´� Sound�� ������ش�.
		//------------------------------------------------
		/*int soundID  = 0 ;

		int 	UseAction = (*g_pActionInfoTable)[m_nUsedActionInfo].GetAction();

		if(UseAction == ACTION_MAGIC && IsAdvancementClass())
		{
			if(!(*g_pActionInfoTable)[m_nUsedActionInfo].IsTargetSelf())
			{
				soundID = (*g_pCreatureTable)[m_CreatureType].GetActionSound(UseAction );
			}
		}
		else
		{
			//int soundID = (*g_pCreatureTable)[m_CreatureType].GetActionSound(
			//							GetActionInfoAction(m_nUsedActionInfo) );

			soundID = (*g_pCreatureTable)[m_CreatureType].GetActionSound(
										GetActionInfoAction(m_nUsedActionInfo) );

		}*/

		int soundID = (*g_pCreatureTable)[m_CreatureType].GetActionSound(
			GetActionInfoAction(m_nUsedActionInfo, false, true));


		if (!HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET) && !m_bBurningSol)
			PlaySound(soundID, false, m_X, m_Y);

	}
}

//----------------------------------------------------------------------
// Set ServerPosition
//----------------------------------------------------------------------
void
MPlayer::SetServerPosition(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
{
	//--------------------------------------------------------
	// ��� ����..
	//--------------------------------------------------------
	m_listSendDirection.clear();

	m_SendMove = 0;

	g_bNetStatusGood = true;

	//--------------------------------------------------------
	// Server���� ������ ��ġ�� ����صд�.
	//--------------------------------------------------------
	m_ServerX = sX;
	m_ServerY = sY;

	//--------------------------------------------------------
	// (m_ServerX, m_ServerY)�������� 
	// Player�� �þ߿� ���Ե��� �ʴ� Creature�� Zone���� ���Ž�Ų��.
	//--------------------------------------------------------
	if (m_pZone != NULL)
	{
		m_pZone->KeepObjectInSight(m_ServerX, m_ServerY, m_Sight);
	}

	SendPositionInfoToParty();
}

//----------------------------------------------------------------------
// Action Effect
//----------------------------------------------------------------------
// ������ ��� �������� Effectȿ���� �����Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::ActionEffect()
{
	/*
	// [������ Ÿ�̹�]�� ��Ŷ�� ������ �ڵ�
	//
	BOOL	bActionStand	= (m_Action==ACTION_STAND || IsSlayer() && m_Action==ACTION_SLAYER_MOTOR_STAND);
	int		actionCountMax_1 = m_ActionCountMax-1;
	BOOL	bEndAction		= (m_ActionCount==actionCountMax_1);// || bActionStand;
	int		StartFrame		= (*g_pActionInfoTable)[m_nUsedActionInfo].GetStartFrame( m_WeaponSpeed );

	// ���� ���κп��� ��Ŷ ������. 2001.8.1
	int		packetSendTiming = min(actionCountMax_1, StartFrame);
	BOOL	bStartAction	= (m_ActionCount==packetSendTiming);	// packetSendTiming�� ������ 0�̾���.

	if (bActionStand)
	{
		bEndAction = bStartAction;
	}
	*/

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD("ActionEffect");
#endif
	/*
	#if __CONTENTS(__GEAR_SWAP_CHANGE)		//MPlayer::ActionEffect()
			bool	bGearChange	= false;
			switch(g_pPlayer->GetRace())
			{
			case RACE_SLAYER:
				bGearChange	= g_pSlayerGear->GetProcessingGearChange();
				break;

			case RACE_VAMPIRE:
				bGearChange	= g_pVampireGear->GetProcessingGearChange();
				break;

			case RACE_OUSTERS:
				bGearChange	= g_pOustersGear->GetProcessingGearChange();
				break;
			}

			if(bGearChange)
				return;
	#endif
	*/
	// 2002.3.12
	// packet�� �������ϴ� ���� ������ ����̸�..
	if (m_nUsedActionInfo < g_pActionInfoTable->GetMinResultActionInfo())
	{
		// �Ÿ��� �Ǵ� ��츸 ActionEffect�� �����Ѵ�.		
		if (max(abs(m_X - m_TraceX), abs(m_Y - m_TraceY))
			<= m_TraceDistance)
		{
#ifdef OUTPUT_DEBUG_PLAYER_ACTION
			DEBUG_ADD("inDist");
#endif
		}
		else
		{
#ifdef OUTPUT_DEBUG_PLAYER_ACTION
			DEBUG_ADD("not inDist");
#endif

			// �Ÿ��� �� �Ǹ�? 
			//if (m_listDirection.empty())
			{
				if (m_DestX != SECTORPOSITION_NULL
					&& m_DestY != SECTORPOSITION_NULL)
				{
					// ���� ���� �ִ� ��ǥ�� �ִ� ���
				}
				else if (abs((int)m_X - m_TraceX) < SECTOR_WIDTH
					&& abs((int)m_Y - m_TraceY) < SECTOR_HEIGHT)
				{
					// �����ϴ� ���
					m_DestX = m_TraceX;
					m_DestY = m_TraceY;
				}

				if (SetDestination(m_TraceX, m_TraceY))
				{
#ifdef OUTPUT_DEBUG_PLAYER_ACTION
					DEBUG_ADD("NewWayOK");
#endif

					// �� ���� �ִ� ���
					m_nNextUsedActionInfo = m_nUsedActionInfo;
					m_nUsedActionInfo = ACTIONINFO_NULL;

					SetAction(ACTION_STAND);
				}
				else
				{
					// ������ ���� ���
					// �� �̷� �ڵ尡 �þ���. ��.. �� �����ľߵǴµ� --;
					SetAction(((m_MoveDevice == MOVE_DEVICE_WALK) ? ACTION_STAND : ACTION_SLAYER_MOTOR_STAND));
					SetNextAction(ACTION_STAND);
					m_fTraceBuffer = 0;
					m_fNextTrace = 0;
					m_bTraceCreatureToForceAttack = false;
					SetStop();
					UnSetRepeatAction();
					TraceNULL();
					m_nUsedActionInfo = ACTIONINFO_NULL;
					m_nNextUsedActionInfo = ACTIONINFO_NULL;

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
					DEBUG_ADD("Stop!");
#endif

					return;
				}

				return;
			}
		}
	}

	BOOL	bActionStand = (m_Action == ACTION_STAND || IsSlayer() && m_Action == ACTION_SLAYER_MOTOR_STAND);
	int		actionCountMax_1 = m_ActionCountMax - 1;
	BOOL	bEndAction = (m_ActionCount == actionCountMax_1) || bActionStand;
	int		StartFrame = (*g_pActionInfoTable)[m_nUsedActionInfo].GetStartFrame(m_WeaponSpeed);

	BOOL	bStartAction = (m_ActionCount == 0 && !IsGunShotGuidance());
	bool	bTest = IsGunShotGuidance();
	//	BOOL	bStartAction	= (m_ActionCount == StartFrame && !IsGunShotGuidance());

		// Effect�� ���۵Ǵ� ����..
		// (1) StartFrame�� ���
		// (2) ������ ActionFrame�� ���

	BOOL	bStartEffect = m_ActionCount == StartFrame || StartFrame >= m_ActionCountMax && bEndAction;

	BOOL	bRepeatAction = m_bRepeatAction
		&& (*g_pActionInfoTable)[m_nUsedActionInfo].IsUseRepeatFrame()
		&& m_RepeatCount != 0
		&& m_ActionCount == GetActionInfoRepeatStartFrame(m_nUsedActionInfo);

	// �ݺ� action�� ���� frame
	bStartAction |= bRepeatAction;

	//BOOL	bCastingEffect	= GetActionInfoCastingStartFrame(m_nUsedActionInfo)==m_ActionCount;


	//----------------------------------------------------------------------
	// [ TEST CODE ]
	//----------------------------------------------------------------------
	// ����� ������ ��������.. - -;;
	// ũ�� ������ ���� ��...
	//----------------------------------------------------------------------
	// EffectTarget�� casting������ ���� �ڿ� �����Ǵµ�,
	// EffectTarget�� ID�� casting���۶��� server�� ������ �ϱ� ������
	// �ӽ÷�... EffectTarget�� ���� �������ѵΰ� ����ߴ�.
	//----------------------------------------------------------------------
	// ��. data member�� �ٲ��. - -;;
	//static MEffectTarget* 


#ifdef OUTPUT_DEBUG
		//DEBUG_ADD_FORMAT("action=(%d/%d) ActionInfo=%d, pEffectTarget=%x", m_ActionCount, m_ActionCountMax, m_nUsedActionInfo, m_pEffectTarget);
#endif

	int SkillInfo = m_nUsedActionInfo;
	if (SkillInfo == SKILL_SWEEP_VICE_1)
	{
		if (!GetSweepViewValue() && g_bRButtonDown) //�������̽��� ���� ������� �ƴҶ�
		{
			if (g_pSkillAvailable->IsEnableSkill(SKILL_SWEEP_VICE_1)
				&& !IsDead()
				&& !(*g_pSkillInfoTable)[SKILL_SWEEP_VICE_1].GetAvailableTimeLeft()
				)
			{
				SetSweepViewValue(1);
				g_SelectSector = g_pTopView->GetSelectedSector(g_x, g_y);

				m_TempSelectPosX = g_SelectSector.x;
				m_TempSelectPosY = g_SelectSector.y;
				m_SweepViceX = m_TraceX;
				m_SweepViceY = m_TraceY;
				ExecuteActionInfoFromMainNode(SKILL_CLIENT_SWEEP_VICE, m_SweepViceX, m_SweepViceY, 0, g_pPlayer->GetDirection(), 0,
					m_SweepViceX, m_SweepViceY, 0, 0xffff, NULL, false);
			}
			return;
		}
		else if (g_bRButtonDown)
			return;

		//	bStartEffect = true;

		if (1 == GetSweepViewValue())
			SkillInfo = SKILL_SWEEP_VICE_1;
		else if (2 == GetSweepViewValue())
			SkillInfo = SKILL_SWEEP_VICE_3;
		else if (3 == GetSweepViewValue())
			SkillInfo = SKILL_SWEEP_VICE_5;
		else
		{
			SkillInfo = SKILL_SWEEP_VICE_1;
			SetSweepViewValue(0);
		}
	}

	//----------------------------------------------------------
	//
	//					���� ���� ����..
	//
	//----------------------------------------------------------

	// Effect�� ������ packet�� ������.
	// ���� �����Ҷ� packet�� ������.
	if (bStartAction)
	{
		const MActionInfo* pUsedActionInfo = NULL;
		if (m_nUsedActionInfo != ACTIONINFO_NULL)
			pUsedActionInfo = &(*g_pActionInfoTable)[m_nUsedActionInfo];

		bool bNoCastingTime = true;
		//		bool bCastingAction	= false;
		WORD wCastingTime = 0;

		if (pUsedActionInfo)
		{
			//			bCastingAction	= pUsedActionInfo->IsCastingAction();
			wCastingTime = pUsedActionInfo->GetCastingTime();
			bNoCastingTime = /*!bCastingAction || */wCastingTime == 0;
		}

		if (bNoCastingTime)
		{
			if (HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
			{
				if (bStartEffect && g_pCurrentMagazine != NULL)
				{
					int MagazineCount = g_pCurrentMagazine->GetNumber();
					if (MagazineCount > 0)
					{
						g_pCurrentMagazine->SetNumber(MagazineCount - 1);
						ActionToSendPacket();
					}
				}
			}
			else
			{
				ActionToSendPacket();
			}
		}
		else
		{
			if (!gC_vs_ui.IsSkillCastingProgress())
			{
				// 				m_SkillCastingX = m_TraceX;
				// 				m_SkillCastingY = m_TraceY;
				gC_vs_ui.StartSkillCastingProgress(
					m_nUsedActionInfo, wCastingTime * 100);
			}
		}
	}

	//-------------------------------------------------------------
	//
	// Casting effect
	//
	//-------------------------------------------------------------
	AttachCastingEffect(SkillInfo);

	//-------------------------------------------------------------
	// ���� �����̸�..
	// �ٷ� effect�� �����ش�.
	//-------------------------------------------------------------

	// [������ Ÿ�̹�] �°� �������� �̰� �ּ�ó�� �ؾ��Ѵ�.
	if (bActionStand)
	{
		m_ActionCount = actionCountMax_1;
	}

	//----------------------------------------------------------
	// action count ����
	//----------------------------------------------------------
	// �� ���⼭ �����ϳĸ�?
	// �ؿ���.. actionCount�� �ٲ� �� �ֱ� ������..
	//----------------------------------------------------------
	m_ActionCount++;


	//--------------------------------------------------------	
	// �ӽ÷� �������� �����ϴ°�..
	//--------------------------------------------------------	
	if (m_bStopBloodDrain)
	{
		StopBloodDrain();
	}
	if (m_bStopAbsorbSoul)
	{
		StopAbsorbSoul();
	}


	//----------------------------------------------------------
	//
	//					Effect ����
	//
	//----------------------------------------------------------
	// affect �Ŀ� �̰� �ٲ� ���� �ִ� T_T;
	TYPE_ACTIONINFO currentUsedActionInfo = SkillInfo;

	// Lar Stroke: every repeat loop of a held attack starts a new slash (the start frame is 0)
	if (bRepeatAction && SkillInfo == SKILL_LAR_STROKE)
	{
		bStartEffect = TRUE;
	}

	if (bStartEffect)
	{
		AffectUsedActionInfo(SkillInfo);
	}





	//----------------------------------------------------------
	//
	//					Action������ ���� ��
	//
	//----------------------------------------------------------
	if (bEndAction)
		//&& (*g_pActionInfoTable)[ m_nUsedActionInfo ].GetSize()!=0)		
	{
		//--------------------------------------------------------
		// ���� ������ �����ش�.
		//--------------------------------------------------------
		//m_TraceID	= OBJECTID_NULL;
		//m_Action	= ACTION_STAND;

		//--------------------------------------------------------
		// ��� ��� �ð� Delay�� �������ش�.
		// ���� ����� ��쿡��..
		//--------------------------------------------------------
		// [ TEST CODE ]
		//if (//currentUsedActionInfo!=ACTIONINFO_NULL && 
		//	currentUsedActionInfo < (*g_pActionInfoTable).GetMinResultActionInfo())
		//{
		//	m_DelayTime	= g_CurrentTime + GetActionInfoDelay(currentUsedActionInfo);
		//}

		//m_bAffectUsedActionInfo = true;


		//------------------------------------------------------------
		// �ݺ� �ൿ üũ..
		//------------------------------------------------------------
			///*
		if (CheckRepeatAction())
		{
			CheckBufferAction();
		}

		// 2002.3.24 checkRepeatAction() ���� �ִ��� ����� ���ȴ�.
		m_nUsedActionInfo = ACTIONINFO_NULL;

		//*/
	}

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD("AEEnd");
#endif
}


//----------------------------------------------------------------------
// �þ� ����
//----------------------------------------------------------------------
//void	
//MPlayer::SetLightSight(char LightSight)
//{
//	// �þ߰� �����Ǿ� ���� ��, �þ� ����..
////	if (m_TimeLightSightX != SECTORPOSITION_NULL)
////	{
//////		m_pZone->UnSetLight(m_LightSightX, m_LightSightY, m_LightSight);	
////	}
//
//	// ����
//	m_TimeLightSight = LightSight;
//
//	// ������ �þ߿� ���� ��ǥ���� Set..	
////	m_TimeLightSightX = m_X;
////	m_TimeLightSightY = m_Y;
//
//	// ���ο� �þ� ���� ����.		
////	m_pZone->SetLight(m_X, m_Y, m_LightSight);			
//}

//----------------------------------------------------------------------
// �þ� ����
//----------------------------------------------------------------------
//void	
//MPlayer::UnSetLightSight()
//{
//	// �þ߰� �����Ǿ� ���� ��, �þ� ����..
////	if (m_LightSightX != SECTORPOSITION_NULL)
////	{
////		m_pZone->UnSetLight(m_LightSightX, m_LightSightY, m_LightSight);	
////	}
//	
//	//m_LightSight = 0;
//
////	m_LightSightX = SECTORPOSITION_NULL;
////	m_LightSightY = SECTORPOSITION_NULL;
//}
//
//----------------------------------------------------------------------
// Set ItemCheckBuffer
//----------------------------------------------------------------------
void
MPlayer::SetItemCheckBuffer(MItem* pItem, enum ITEM_CHECK_BUFFER status, TYPE_OBJECTID SubItem)
{
	m_pItemCheckBuffer = pItem;

	m_dwSubItemIDCheckBuffer = SubItem;

	m_ItemCheckBufferStatus = status;

#ifdef OUTPUT_DEBUG
	DEBUG_ADD_FORMAT("[SetItemCheckBuffer] status %d", status);
	//		switch (status)
	//		{
	//			case ITEM_CHECK_BUFFER_PICKUP_TO_INVENTORY :
	//				DEBUG_ADD("ITEM_CHECK_BUFFER_PICKUP_TO_INVENTORY");
	//			break;
	//
	//			case ITEM_CHECK_BUFFER_PICKUP_TO_MOUSE :
	//				DEBUG_ADD("ITEM_CHECK_BUFFER_PICKUP_TO_MOUSE");
	//			break;
	//
	//			case ITEM_CHECK_BUFFER_DROP_TO_ZONE :
	//				DEBUG_ADD("ITEM_CHECK_BUFFER_DROP_TO_ZONE");
	//			break;
	//		}
#endif

	UI_LockItem();
	UI_LockGear();
	UI_LockItemTrade();
}

//----------------------------------------------------------------------
// Clear ItemCheckBuffer
//----------------------------------------------------------------------
void
MPlayer::ClearItemCheckBuffer()
{
	DEBUG_ADD("Set Item Check Buffer to NULL");

	m_pItemCheckBuffer = NULL;

	m_dwSubItemIDCheckBuffer = OBJECTID_NULL;

	m_ItemCheckBufferStatus = ITEM_CHECK_BUFFER_NULL;

	// 2004, 8, 27, sobeit add start - ����ȯ ���¿��� �κ������� ����� ��������� ������ �Ծ��� �ϸ� 
	// Ŭ���̾�Ʈ �ʿ��� �����°� Ǯ����.-_- ���� �� ��ȯ�϶� UI_UnlockGear()�� ȣ������ �ʴ´�.
	if (!IsInCasket())
		UI_UnlockGear();
	// 2004, 8, 27, sobeit add end - 
	UI_UnlockItem();
	UI_UnlockItemTrade();
}

//----------------------------------------------------------------------
// �ൿ ����
//----------------------------------------------------------------------
void
MPlayer::SetAction(BYTE action)
{
	//	if ( m_nSpecialActionInfo == MAGIC_BLOODY_ZENITH && m_Action == ACTION_ATTACK &&
	//		m_ActionCount >= m_ActionCountMax && GetActionDelay() > 0)
	//	{
	//		SetActionDelay( GetActionDelay() - 1 );
	//		m_ActionCount = m_ActionCountMax - 2;
	//		return;
	//	}	
		// 2004, 10, 6, sobeit add start

	if (HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET) && (action == ACTION_DAMAGED || action == ACTION_DRAINED))
		return;
	// 2004, 10, 6, sobeit add end
	if (HasEffectStatus(EFFECTSTATUS_SLEEP)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		//		return;
		action = ACTION_STAND;
	}

	// ������ ���ش�.
	m_bFastMove = false;
	//m_bFastMove = false;

	//-----------------------------------------------------------------
	// ���� ����� action�� �´� ���. 2001.10.5
	//-----------------------------------------------------------------
	if (action < GetActionMax())
	{
		//-----------------------------------------------------
		// ���� �غ� �ڼ�
		//-----------------------------------------------------
		UpdateBattleStand(action);

		//-----------------------------------------------------
		// ������� Ÿ�� ������
		//-----------------------------------------------------
		if (m_MoveDevice == MOVE_DEVICE_RIDE)
		{
			if (IsSlayer())
			{
				if (action == ACTION_STAND)
				{
					action = ACTION_SLAYER_MOTOR_STAND;
				}
				else if (action == ACTION_MOVE)
				{
					action = ACTION_SLAYER_MOTOR_MOVE;
				}
			}
		}
		//-----------------------------------------------------
		// �ɾ�ٴҶ�
		//-----------------------------------------------------
		else
		{
			if (IsSlayer())
			{
				if (action == ACTION_SLAYER_MOTOR_STAND)
				{
					action = ACTION_STAND;
				}
				else if (action == ACTION_SLAYER_MOTOR_MOVE)
				{
					action = ACTION_MOVE;
				}
			}
		}

		if (IsOusters() && (HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this)) || m_MoveDevice == MOVE_DEVICE_SUMMON_SYLPH))
		{
			if (action == ACTION_STAND)
			{
				action = ACTION_OUSTERS_FAST_MOVE_STAND;
			}
			else if (action == ACTION_MOVE)
			{
				action = ACTION_OUSTERS_FAST_MOVE_MOVE;
			}
		}

#if __CONTENTS(__FAST_TRANSFORTER||__SECOND_TRANSFORTER)
		if (IsOusters() && (HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this)) && m_MoveDevice == MOVE_DEVICE_WING_SYLPH)
#if __CONTENTS(__SECOND_TRANSFORTER)
			|| (HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this)) && m_MoveDevice == MOVE_DEVICE_WING_SYLPH)
#endif //__SECOND_TRANSFORTER
			)
		{
			if (action == ACTION_STAND)
			{
#if __CONTENTS(__SECOND_TRANSFORTER)
				if (0 == GetWingItemType())
#endif //__SECOND_TRANSFORTER
					action = ACTION_OUSTERS_WING_STAND;
#if __CONTENTS(__SECOND_TRANSFORTER)
				else if (1 == GetWingItemType())
					action = ACTION_OUSTERS_UNICORN_STAND;
#endif //__SECOND_TRANSFORTER
			}
			else if (action == ACTION_MOVE)
			{
#if __CONTENTS(__SECOND_TRANSFORTER)
				if (0 == GetWingItemType())
#endif //__SECOND_TRANSFORTER
					action = ACTION_OUSTERS_WING_MOVE;
#if __CONTENTS(__SECOND_TRANSFORTER)
				else if (1 == GetWingItemType())
					action = ACTION_OUSTERS_UNICORN_MOVE;
#endif //__SECOND_TRANSFORTER
			}
		}
#endif //__FAST_TRANSFORTER||__SECOND_TRANSFORTER
#ifdef OUTPUT_DEBUG
		if (IsSlayer())
		{
			// slayer�� ���
			if (action >= GetActionMax())
			{
				DEBUG_ADD_FORMAT("[Error] Slayer's Action exceed MAX : id=%d, action=%d", m_ID, action);

				return;
			}
		}
		else if (IsVampire())
		{
			if (action >= GetActionMax())
			{
				DEBUG_ADD_FORMAT("[Error] Vampire's Action exceed MAX : id=%d, action=%d", m_ID, action);

				return;
			}
		}
		else if (IsOusters())
		{
			if (action >= GetActionMax())
			{
				DEBUG_ADD_FORMAT("[Error] Ousters's Action exceed MAX : id=%d, action=%d", m_ID, action);

				return;
			}
		}
		else if (IsNPC())
		{
			if (action >= GetActionMax())
			{
				DEBUG_ADD_FORMAT("[Error] NPC's Action exceed MAX : id=%d, action=%d", m_ID, action);

				return;
			}
		}
		else
		{
			// - -;;
			DEBUG_ADD("[Error] What is this Creature????");

			return;
		}
#endif

		m_bNextAction = false;

		//------------------------------------------------
		// Move�� ���
		//------------------------------------------------
		if (action == m_MoveAction)
		{
			// �׳� ���ִ� ����... ���������� �����.
			// Action�� �����ٰ� ǥ�����ֱ� ���ؼ�..
			if (m_Action == ACTION_STAND
				|| IsSlayer() && m_Action == ACTION_SLAYER_MOTOR_STAND
				|| IsOusters() && m_Action == ACTION_OUSTERS_FAST_MOVE_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
				|| IsOusters() && m_Action == ACTION_OUSTERS_WING_STAND
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
				|| IsOusters() && m_Action == ACTION_OUSTERS_UNICORN_STAND
#endif //__SECOND_TRANSFORTER
				)
			{
				m_ActionCount = 0;
				m_ActionCountMax = 0;
			}

			//------------------------------------------------
			// ������ ��츸 �����δ�.
			//------------------------------------------------
			if (IsStop())
			{
				m_Action = action;
				//SetAction( action );

				SetNextAction(ACTION_STAND);

				// Move ù ����
				m_MoveCount = 0;
				m_NextMoveCount = 0;

				// ����  :  m_sX = 0; m_sY = 0;
				// �����̴� ������ ����..
				//m_MoveCountMax = (*g_pCreatureTable)[m_CreatureType].GetActionCount( action );
				m_MoveCountMax = GetCreatureActionCountMax(this, action);
				//g_pTopView->m_CreatureFPK[(*g_pCreatureTable)[m_CreatureType].FrameID]
				//									[action][m_Direction].GetSize();		
#if __CONTENTS(__FAST_TRANSFORTER||__SECOND_TRANSFORTER)
				//���� �ű��̵�����(�����̾�)�� Ÿ�� �ִٸ�... 
				if (m_MoveDevice == MOVE_DEVICE_RIDE && IsRide_Fast_Transforter())
				{
					m_MoveCountMax -= 1;
				}
#endif //__FAST_TRANSFORTER||__SECOND_TRANSFORTER
				//---------------------------------------------
				// UI���� gear�� item �̵��� ���ϰ� �Ѵ�. 
				//---------------------------------------------
				//gC_vs_ui.LockItem();
				gC_vs_ui.LockGear();
			}
			//------------------------------------------------
			// �����̰� �ִ� �����̸� Buffering�Ѵ�.
			//------------------------------------------------
			else
			{
				//---------------------------------------------
				// UI���� gear item �̵��� �� �� �ֵ��� �Ѵ�.
				//---------------------------------------------
				//gC_vs_ui.UnlockItem();
				gC_vs_ui.UnlockGear();

				// buffering
				SetNextAction(action);
			}
		}
		//------------------------------------------------
		// Action�� ���
		//------------------------------------------------
		else
		{
			//------------------------------------------------
			// extreme�� Ư�� ���ۿ��� Ǯ����.
			//------------------------------------------------
//			if (HasEffectStatus(EFFECTSTATUS_EXTREME))
//			{
//				if (action==ACTION_MAGIC
//					|| action==ACTION_DIE
//					|| action==ACTION_VAMPIRE_DRAIN
//					|| action==ACTION_DRAINED)
//				{
//					RemoveEffectStatus(EFFECTSTATUS_EXTREME);
//				}
//			}

			if (// ������� Ÿ�� �������� action �� �����ش�.
				m_MoveDevice == MOVE_DEVICE_RIDE && action != ACTION_SLAYER_MOTOR_STAND
				// damaged�� ���..
				|| action == ACTION_DAMAGED
				&& (// ���ݹ޴� ���� �ƴϰų� ���� ������ �ƴϸ� ���� �޴� ��� ǥ�� �� �Ѵ�
					m_Action != ACTION_DAMAGED && m_Action != ACTION_STAND && m_Action != ACTION_MOVE

					// ���� ���ϴ� ���� ���̰ų� 
//						m_Action==ACTION_DRAINED
						// �����̾��϶�, �����ϴ� ���� �߿��� damaged�� �� �����ش�.
//						|| IsVampire() && m_Action==ACTION_VAMPIRE_DRAIN
)
)
			{
				SetNextAction(ACTION_STAND);
				return;
			}

			//---------------------------------------------
			// UI���� gear item �̵��� �� �� �ֵ��� �Ѵ�.
			//---------------------------------------------
			//gC_vs_ui.UnlockItem();
			if (!IsInCasket())
				gC_vs_ui.UnlockGear();

			// ���� ����		
			SetNextAction(ACTION_STAND);

			//m_ActionCountMax = (*g_pCreatureTable)[m_CreatureType].GetActionCount( action );
			m_ActionCountMax = GetCreatureActionCountMax(this, action);

			if (m_ActionCountMax == 0)
			{
				// ������ ���� ���
				m_Action = ((m_MoveDevice == MOVE_DEVICE_WALK) ? ACTION_STAND : ACTION_SLAYER_MOTOR_STAND);
				//SetAction( ((m_MoveDevice==MOVE_DEVICE_WALK)? ACTION_STAND : ACTION_SLAYER_MOTOR_STAND) );
				m_ActionCount = 0;
				m_RepeatCount = 0;
				//m_ActionCountMax = (*g_pCreatureTable)[m_CreatureType].GetActionCount( m_Action );
				m_ActionCountMax = GetCreatureActionCountMax(this, m_Action);
			}
			else
			{
				//---------------------------------------------
				// action�� �ݺ��ϴ� ���
				//---------------------------------------------
				if (m_nUsedActionInfo != ACTIONINFO_NULL
					&& m_bRepeatAction
					&& (*g_pActionInfoTable)[m_nUsedActionInfo].IsUseRepeatFrame())// m_WeaponSpeed ))
				{
					m_Action = action;
					//SetAction( action );

					// �ݺ�ȸ�� ������ �ɸ���..
					if (m_RepeatCount >= (*g_pActionInfoTable)[m_nUsedActionInfo].GetRepeatLimit())
					{
						m_RepeatCount = 0;

						// ������ ù ���ۺ���..
						m_ActionCount = 0;
					}
					else
					{
						//---------------------------------------------
						// ��� �ݺ��ϴ� ���
						//---------------------------------------------
						BOOL bSlayer = IsSlayer();

						if (m_RepeatCount != 0

							&& action != ACTION_STAND
							&& (!bSlayer || bSlayer && action != ACTION_SLAYER_MOTOR_STAND))
						{
							// count ����..
							//m_ActionCount = (*g_pActionInfoTable)[m_nUsedActionInfo].GetRepeatStartFrame( m_WeaponSpeed );
							m_ActionCount = GetActionInfoRepeatStartFrame(m_nUsedActionInfo);
						}
						//---------------------------------------------
						// ó�� �ݺ��� �����ҷ��� ���
						//---------------------------------------------
						else
						{
							m_ActionCount = 0;
						}
					}

					//m_ActionCountMax = (*g_pActionInfoTable)[m_nUsedActionInfo].GetRepeatEndFrame( m_WeaponSpeed );			
					m_ActionCountMax = GetActionInfoRepeatEndFrame(m_nUsedActionInfo);
				}
				//---------------------------------------------
				// �Ϲ����� ���..
				//---------------------------------------------
				else
				{
					if (m_ActionCount != m_ActionCountMax
						&& (action == ACTION_STAND && m_Action == ACTION_STAND
							|| IsSlayer() && action == ACTION_SLAYER_MOTOR_STAND && m_Action == ACTION_SLAYER_MOTOR_STAND
							|| IsOusters() && action == ACTION_OUSTERS_FAST_MOVE_STAND && m_Action == ACTION_OUSTERS_FAST_MOVE_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
							|| IsOusters() && action == ACTION_OUSTERS_WING_MOVE && m_Action == ACTION_OUSTERS_WING_STAND
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
							|| IsOusters() && action == ACTION_OUSTERS_UNICORN_MOVE && m_Action == ACTION_OUSTERS_UNICORN_STAND
#endif //__SECOND_TRANSFORTER
							)
						)
					{
						// ���� ���� �����̸� �������� �ʴ´�.
						//m_ActionCount = 0;
						//m_ActionCountMax = (*g_pCreatureTable)[m_CreatureType].GetActionCount( m_Action );
					}
					else
					{
						m_Action = action;
						//SetAction( action );

						// count ����..
						m_ActionCount = 0;

						//g_pTopView->m_CreatureFPK[(*g_pCreatureTable)[m_CreatureType].FrameID]
						//												[m_Action][m_Direction].GetSize();
					}
				}
			}
		}
	}


	// ������ �����Ҷ� �ΰ������� �ٴ� Effect ��.
	// ��򰡷� ���� �ϴµ�.-_-
	if (HasEffectStatus(EFFECTSTATUS_REDIANCE))
	{
		if (
			(m_Action == ACTION_SLAYER_SWORD_2 || m_Action == ACTION_SLAYER_SWORD_2_SLOW || m_Action == ACTION_SLAYER_SWORD_2_FAST)
			)
		{
			MAttachEffect* pEffect = CreateAttachEffect(EFFECTSPRITETYPE_NEW_REDIANCE_MALE_NORMAL, 0, 0);
			if (pEffect != NULL)
			{
				pEffect->SetDirection(GetDirection());
				pEffect->SetLink(ACTIONINFO_NULL, NULL);
				pEffect->SetAttachCreatureID(GetID());
			}
		}
		else if (
			(m_Action == ACTION_SLAYER_SWORD || m_Action == ACTION_SLAYER_SWORD_FAST || m_Action == ACTION_SLAYER_SWORD_SLOW))
		{
			MAttachEffect* pEffect = CreateAttachEffect(EFFECTSPRITETYPE_NEW_REDIANCE_MALE_NORMAL_ATTACK_NORMAL, 0, 0);
			if (pEffect != NULL)
			{
				pEffect->SetDirection(GetDirection());
				pEffect->SetLink(ACTIONINFO_NULL, NULL);
				pEffect->SetAttachCreatureID(GetID());
			}
		}
	}

	//------------------------------
	// ������ �����̸�
	//------------------------------
	/*
	if (IsStop())
	{
		m_Action = action;
		m_NextAction = ACTION_STAND;

		// Creature�� ������ ���� Action�� ���� frame���� �޶�� �ϳ�?
		switch (m_Action)
		{
			case ACTION_MOVE :
				switch (m_MoveDevice)
				{
					case MOVE_DEVICE_NULL :
						m_ActionCountMax = ACTIONCOUNT_MOVE;
					break;

					case MOVE_DEVICE_MOTOR1 :
						m_ActionCountMax = ACTIONCOUNT_MOVE_HALF;
					break;
				}
			break;

			default :
				m_ActionCountMax = g_pTopView->m_CreatureFPK[(*g_pCreatureTable)[m_CreatureType].FrameID]
															[m_Action][m_Direction].GetSize();
		}

		m_ActionCount = 0;
	}
	//------------------------------
	// �����̰� �ִ� �����̸�
	//------------------------------
	else
	{
		// buffering
		m_NextAction = action;
	}
	*/
}

//----------------------------------------------------------------------
// Set AddonItem
//----------------------------------------------------------------------
// AddonItem�� �����Ѵ�.
//----------------------------------------------------------------------
bool
MPlayer::SetAddonItem(MItem* pItem)
{
	DEBUG_ADD("MPlayer::SetAddonItem");

	//-------------------------------------------------
	// ����� ������ ���
	//-------------------------------------------------
	if (MCreatureWear::SetAddonItem(pItem))
	{
		//-------------------------------------------------
		// ������ Item�� 
		// �⺻���ݿ� �������̶��...
		// �⺻����ActionInfo�� �ٲ��� �Ѵ�.
		//-------------------------------------------------
		if (pItem->IsBasicWeapon())
		{
			//-------------------------------------------------
			// ���� ���ɰŸ� ����
			// ���� ��츸 �ٸ���.. 
			// �������� ��� ���������̹Ƿ� 1�̴�.							
			//-------------------------------------------------
			//if (pItem->IsGunItem())
			//{
			SetBasicAttackDistance(pItem->GetReach());
			//}
			//else
			//{								
			//	SetBasicAttackDistance( 1 );
			//}

			//-------------------------------------------------
			// Skill Icon üũ
			//-------------------------------------------------
			//(*g_pSkillAvailable).SetAvailableSkills();
		}

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Remove AddonItem
//----------------------------------------------------------------------
// AddonItem�� ���������Ѵ�.
//----------------------------------------------------------------------
bool
MPlayer::RemoveAddonItem(MItem* pItem)
{
	DEBUG_ADD("MCreatureWear::RemoveAddonItem");

	//-------------------------------------------------
	// ����� ������ ���
	//-------------------------------------------------
	if (MCreatureWear::RemoveAddonItem(pItem))
	{
		//-------------------------------------------------
		// ������ Item�� 
		// �⺻���ݿ� �������̶��...
		// �⺻����ActionInfo�� �Ǽ� �������� �ٲ��� �Ѵ�.
		//-------------------------------------------------
		if (pItem->IsBasicWeapon())
		{
			// �⺻ �Ǽ� ���� �������� ��ȯ�Ѵ�.
			SetBasicActionInfo(SKILL_ATTACK_MELEE);


			// ���� ���ɰŸ� ����
			SetBasicAttackDistance(1);

			//-------------------------------------------------
			// Skill Icon üũ
			//-------------------------------------------------
			//(*g_pSkillAvailable).SetAvailableSkills();
		}

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Set Next Action
//----------------------------------------------------------------------
// ������ �� �ൿ ����.. ���� �׳� ��������(STAND) �ٷ� action ����
// �ƴϸ�, buffering
//----------------------------------------------------------------------
void
MPlayer::SetNextAction(BYTE action)
{
	if (HasEffectStatus(EFFECTSTATUS_SLEEP)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
	{
		//		return;
		action = ACTION_STAND;
	}

	if (action == ACTION_STAND)
	{
		action = ((m_MoveAction == ACTION_SLAYER_MOTOR_MOVE) ? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND);
		if (IsOusters() && HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this)))
			action = ACTION_OUSTERS_FAST_MOVE_STAND;
#if __CONTENTS(__FAST_TRANSFORTER)
		else if (IsOusters() && HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this)))
			action = ACTION_OUSTERS_WING_STAND;
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		else if (IsOusters() && HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this)))
			action = ACTION_OUSTERS_UNICORN_STAND;
#endif //__SECOND_TRANSFORTER
	}
	else if (action == ACTION_MOVE)
	{
		if (IsOusters() && HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this)))
			action = ACTION_OUSTERS_FAST_MOVE_MOVE;
#if __CONTENTS(__FAST_TRANSFORTER)
		else if (IsOusters() && HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this)))
			action = ACTION_OUSTERS_WING_MOVE;
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		else if (IsOusters() && HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this)))
			action = ACTION_OUSTERS_UNICORN_MOVE;
#endif //__SECOND_TRANSFORTER
	}

	if (m_Action == ACTION_STAND || IsOusters() && m_Action == ACTION_OUSTERS_FAST_MOVE_STAND
		|| IsSlayer() && m_Action == ACTION_SLAYER_MOTOR_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
		|| IsOusters() && m_Action == ACTION_OUSTERS_WING_STAND
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		|| IsOusters() && m_Action == ACTION_OUSTERS_UNICORN_STAND
#endif //__SECOND_TRANSFORTER
		)
	{
		//SetAction( action );
		m_ActionCount = m_ActionCountMax;
	}

	//else
	//{
	m_NextAction = action;
	//}
}

//----------------------------------------------------------------------
// Set NextAction To Move
//----------------------------------------------------------------------
// ���� action���� �̵��Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::SetNextActionToMove()
{
	if (m_Action == ACTION_STAND || IsOusters() && m_Action == ACTION_OUSTERS_FAST_MOVE_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
		|| IsOusters() && m_Action == ACTION_OUSTERS_WING_STAND
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		|| IsOusters() && m_Action == ACTION_OUSTERS_UNICORN_STAND
#endif //__SECOND_TRANSFORTER
		|| IsSlayer() && m_Action == ACTION_SLAYER_MOTOR_STAND)
	{
		//SetAction( action );
		m_ActionCount = m_ActionCountMax;
	}
	//else
	//{
	SetNextAction(m_MoveAction);
	//}
}

//----------------------------------------------------------------------
// Set Alive
//----------------------------------------------------------------------
// ��Ҵ�~ 
//----------------------------------------------------------------------
void
MPlayer::SetAlive()
{
	m_bAlive = true;

	// item �̵� ����
	UI_UnlockItem();
	UI_UnlockGear();
	UI_UnlockItemTrade();

	SetAction(ACTION_STAND);

	//-------------------------------------------------------
	// Effect���µ� ����
	//-------------------------------------------------------
	/*
	if (g_pEffectStatusTable!=NULL)
	{
		for (int e=0; e<g_pEffectStatusTable->GetSize(); e++)
		{
			RemoveEffectStatus( (EFFECTSTATUS)e );
		}
	}
	*/

	if (g_pTopView != NULL)
	{
		g_pTopView->SetFadeEnd();
	}

	if (m_MoveType == CREATURE_FLYING)
	{
		m_Z = (*g_pCreatureTable)[m_CreatureType].FlyingHeight;
	}
}

//----------------------------------------------------------------------
// Set Dead
//----------------------------------------------------------------------
// �׾���~ 
// ServerPosition���� ���� ����..
//----------------------------------------------------------------------
void
MPlayer::SetDead()
{
	// ��� �ִ� ��츸 ���δ�. -_-;;
	if (m_bAlive)
	{
		if (HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
		{
			RemoveEffectStatus(EFFECTSTATUS_INSTALL_TURRET);
			SetAction(ACTION_STAND);
		}
		if (HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this)))
		{
			RemoveEffectStatusSummonSylph(dynamic_cast<MCreature*>(this));
			SetAction(ACTION_STAND);
		}
#if __CONTENTS(__FAST_TRANSFORTER)
		if (HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this)))
		{
			RemoveEffectStatusWingSylph(dynamic_cast<MCreature*>(this));
			SetAction(ACTION_STAND);
		}
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		if (HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this)))
		{
			RemoveEffectStatusUnicorn(dynamic_cast<MCreature*>(this));
			SetAction(ACTION_STAND);
		}
#endif //__SECOND_TRANSFORTER
		if (HasEffectStatus(EFFECTSTATUS_GLACIER))
		{
			RemoveEffectStatus(EFFECTSTATUS_GLACIER);
			SetAction(ACTION_STAND);
			ClearAttachEffect();
		}
		g_pUIMessageManager->Execute(UI_FINISH_LEVELUP_BUTTON, 0, 0, NULL);

		StopDrain();
		StopAbsorb();

		// item �̵� ���ϰ�..
		UI_LockItem();
		UI_LockGear();
		UI_LockItemTrade();

		SetStop();
		//		StopCauseCriticalWounds();

		MovePosition(m_ServerX, m_ServerY);

		DEBUG_ADD_FORMAT("PlayerDeadPosition(%d, %d)", m_ServerX, m_ServerY);

		// ����
		TraceNULL();

		// ���� ���� ActionInfo
		m_nUsedActionInfo = (*g_pCreatureTable)[m_CreatureType].DeadActionInfo;

		if (m_nUsedActionInfo != ACTIONINFO_NULL)
		{
			// ���� ���� ����
			SetAction(GetActionInfoAction(m_nUsedActionInfo));

			// ������ �ϴ� ����
			m_NextTraceID = m_ID;
			m_NextTraceX = m_X;
			m_NextTraceY = m_Y;
			m_NextTraceZ = 0;

			SetTraceID(m_ID);
			m_TraceX = m_X;
			m_TraceY = m_Y;
			m_TraceZ = 0;
		}

		// Delay �ð�
		m_DeadDelayTime = g_CurrentTime + g_pClientConfig->DELAY_PLAYER_DEAD;


		//-------------------------------------------------------
		// ���̰� �ϱ�
		//-------------------------------------------------------
		SetVisibleSoon();

		// ������ ���� ���°� ���ֱ� ���ؼ�.. Ȯ�λ��.. - -; 2001.7.23
		m_bNextAction = false;
		m_bKeepTraceCreature = false;
		SetNextAction(ACTION_STAND);
		UnSetRepeatAction();

		// HP
		StopRecoveryHP();
		StopRecoveryMP();
		SetStatus(MODIFY_CURRENT_HP, 0);

		// LockMode ����
		UnSetLockMode();

		// ������ ������...
		//g_DXMusic.SetCurrentTempo( g_DXMusic.GetOriginalTempo() * 0.8f );

		m_Z = 0;
		// Ȯ��.. 
		UI_CloseSelectWayPoint();

		// Logout�ð� ����
		g_pUserInformation->LogoutTime = 0;

#if __CONTENTS(__GAMEMENU_QUITEXIT)
		g_pUserInformation->QuitExitTime = 0;
#endif //__GAMEMENU_QUITEXIT

		SetWaitVerifyNULL();

		g_pEventManager->RemoveEvent(EVENTID_LOGOUT);
		g_pEventManager->RemoveEvent(EVENTID_LOVECHAIN);

		// �׾���.
		m_bAlive = false;
		m_bInCasket = false;

		//-------------------------------------------------------
		// Effect���µ� ����
		//-------------------------------------------------------
		if (g_pEffectStatusTable != NULL)
		{
			for (int e = 0; e < g_pEffectStatusTable->GetSize(); e++)
			{
				// ������ �������� �ʴ´�.
				if (e != EFFECTSTATUS_BLOOD_DRAIN &&
					// 2004, 11, 16, sobeit add start - ������ ������ �༮�� ���°� �ʿ��ؼ� ���� ����..
					e != EFFECTSTATUS_SIEGE_ATTACKER_1 &&
					e != EFFECTSTATUS_SIEGE_ATTACKER_2 &&
					e != EFFECTSTATUS_SIEGE_ATTACKER_3 &&
					e != EFFECTSTATUS_SIEGE_ATTACKER_4 &&
					e != EFFECTSTATUS_SIEGE_ATTACKER_5 &&
					// 2004, 11, 16, sobeit add end
					HasEffectStatus((EFFECTSTATUS)e))
				{
					RemoveEffectStatus((EFFECTSTATUS)e);
				}
			}
		}
		ClearAttachEffect();

		//		g_pTempInformation->SetMode( TempInformation::MODE_NULL );

				//---------------------------------------------------------------
				// �� ������ ��Ƽ���� �����ش�.
				//---------------------------------------------------------------
		SendStatusInfoToParty();

		// 2004, 5, 7 sobeit add start - �׾��� �� ����
		ExecuteHelpEvent(HELP_EVENT_DIE);
		// 2004, 5, 7 sobeit add end
	}
}

//----------------------------------------------------------------------
// Check Repeat Action
//----------------------------------------------------------------------
bool
MPlayer::CheckRepeatAction()
{
	//----------------------------------------------------------------------
	// �ݺ� action�� �ȵǴ� ���
	//----------------------------------------------------------------------
	if (m_bKeepTraceCreature || IsRequestMode() || IsInDarkness())// || m_bTraceCreatureToForceAttack)
	{
		UnSetRepeatAction();
		return false;
	}

	//----------------------------------------------------------------------
	// action�� ������ �����̰ų� ���������� ��쿡 �ݺ� action�� �����Ѵ�.
	//----------------------------------------------------------------------
	if (m_ActionCount >= m_ActionCountMax - 1
		|| m_Action == ACTION_STAND
		|| IsSlayer() && m_Action == ACTION_SLAYER_MOTOR_STAND)
	{
		static DWORD repeatFrameIncTurn = 0;

		//------------------------------------------------------------
		// �ݺ� �ൿ�� ������ ���
		//------------------------------------------------------------
		if (m_bRepeatAction)
		{
			//------------------------------------------------------------
			// �ڽſ��� Ư�� ���
			//------------------------------------------------------------
			if (m_fTraceBuffer & FLAG_TRACE_SELF)
			{
				//SelfSpecialAction();
				m_fNextTrace = m_fTraceBuffer;

				if (repeatFrameIncTurn != g_CurrentFrame)
				{
					repeatFrameIncTurn = g_CurrentFrame;
					m_RepeatCount++;
				}

				//CHECK_REPEAT_LIMIT_TO_RETURN_FALSE( m_nUsedActionInfo, m_RepeatCount )

				return true;
			}
			//------------------------------------------------------------
			// �⺻ �ൿ			
			//------------------------------------------------------------
			else if (m_fTraceBuffer & FLAG_TRACE_CREATURE_BASIC)
			{
				//TraceCreatureToBasicAction( m_TraceID, m_bTraceCreatureToForceAttack );
				m_fNextTrace = m_fTraceBuffer;

				m_NextTraceID = m_TraceIDBuffer;

				m_bNextForceAttack = m_bTraceCreatureToForceAttack;

				if (repeatFrameIncTurn != g_CurrentFrame)
				{
					repeatFrameIncTurn = g_CurrentFrame;
					m_RepeatCount++;
				}

				//CHECK_REPEAT_LIMIT_TO_RETURN_FALSE( m_nUsedActionInfo, m_RepeatCount )

				return true;
			}
			//------------------------------------------------------------
			// Ư�� �ൿ
			//------------------------------------------------------------
			else  if (m_fTraceBuffer & FLAG_TRACE_CREATURE_SPECIAL)
			{
				//TraceCreatureToSpecialAction( m_TraceID );
				m_fNextTrace = m_fTraceBuffer;

				m_NextTraceID = m_TraceIDBuffer;

				m_bNextForceAttack = m_bTraceCreatureToForceAttack;

				if (repeatFrameIncTurn != g_CurrentFrame)
				{
					repeatFrameIncTurn = g_CurrentFrame;
					m_RepeatCount++;
				}

				//CHECK_REPEAT_LIMIT_TO_RETURN_FALSE( m_nUsedActionInfo, m_RepeatCount )				
				if (m_nUsedActionInfo < g_pActionInfoTable->GetSize())
				{
					// �ݺ�ȸ�� ���ѿ� �ɸ���..
					if (m_RepeatCount >= (*g_pActionInfoTable)[m_nUsedActionInfo].GetRepeatLimit())
					{
						// ���� ������ ������.
						//m_RepeatCount = 0;

						BOOL bSlayer = IsSlayer();

						if ((*g_pActionInfoTable)[m_nUsedActionInfo].IsUseRepeatFrame() //m_WeaponSpeed )
							&& m_Action != ACTION_STAND
							&& (!bSlayer || bSlayer && m_Action != ACTION_SLAYER_MOTOR_STAND)
							&& m_ActionCountMax != 0)	// �̰ɷ� �ɷ���.. - -;
						{
							//m_ActionCountMax = (*g_pCreatureTable)[m_CreatureType].GetActionCount( m_Action );
							m_ActionCountMax = GetCreatureActionCountMax(this, m_Action);

							DEBUG_ADD_FORMAT("unsetrepeat: actionCount=%d / %d", m_ActionCount, m_ActionCountMax);
						}
					}
				}

				return true;
			}
			//------------------------------------------------------------
			// Ư�� �ൿ -> Sector
			//------------------------------------------------------------
			else if (m_fTraceBuffer & FLAG_TRACE_SECTOR_SPECIAL)
			{
				//TraceSectorToSpecialAction( m_TraceX, m_TraceY );
				m_fNextTrace = m_fTraceBuffer;

				//m_NextTraceX = m_TraceX;
				//m_NextTraceY = m_TraceY;

				//------------------------------------------------------------
				// ������ mouse ��ǥ�� �о map�� ��ǥ�� �ٲ۴�.
				//------------------------------------------------------------
				// �쾾 -_-;; �� ���⼭ ���콺 ��ǥ�� �о���İ� ��
				if (!gC_vs_ui.IsSkillCastingProgress())
				{
					POINT temp = g_pTopView->ScreenToPixel(g_x, g_y);
					temp = MTopView::PixelToMap(temp.x, temp.y);

					m_NextTraceX = temp.x;
					m_NextTraceY = temp.y;
				}
				else
				{
					m_NextTraceX = m_SkillCastingX;
					m_NextTraceY = m_SkillCastingY;
				}

				if (repeatFrameIncTurn != g_CurrentFrame)
				{
					repeatFrameIncTurn = g_CurrentFrame;
					m_RepeatCount++;
				}

				//CHECK_REPEAT_LIMIT_TO_RETURN_FALSE( m_nUsedActionInfo, m_RepeatCount )

				return true;
			}
		}
		//------------------------------------------------------------
		// �ݺ� �ൿ�� ���� �� �� ���
		//------------------------------------------------------------
		else
		{
			//------------------------------------------------------------
			// buffering�� ���� �ൿ�� �ִٸ�..
			//------------------------------------------------------------
			if (m_fNextTrace != FLAG_TRACE_NULL)
			{
				//m_fTraceBuffer = m_fNextTrace;
				return true;
			}
		}
	}
	else
	{
	}

	return false;
}

//----------------------------------------------------------------------
// Check BufferAction
//----------------------------------------------------------------------
// ������ ó���ؾ��� buffering�� action�� �ִ����� �˾ƺ���.
// ���� ���� return true
// ���� ���� return false
//----------------------------------------------------------------------
bool
MPlayer::CheckBufferAction()
{
	//DEBUG_ADD("Before CheckBufferAction");

	//------------------------------------------		
	//     �����ؾ� �ϴ� ���..		
	//------------------------------------------
	if (m_fNextTrace != FLAG_TRACE_NULL)
	{
		//SetAction( (m_MoveAction==ACTION_SLAYER_MOTOR_MOVE)? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND );

		//---------------------
		// �ڽſ��� Ư�� ���
		//---------------------
		if (m_fNextTrace & FLAG_TRACE_SELF)
		{
			SelfSpecialAction();
		}
		//---------------------
		// �⺻ �ൿ			
		//---------------------
		if (m_fNextTrace & FLAG_TRACE_CREATURE_BASIC)
		{
			TraceCreatureToBasicAction(m_NextTraceID, m_bNextForceAttack);
		}
		//---------------------
		// Ư�� �ൿ
		//---------------------
		else  if (m_fNextTrace & FLAG_TRACE_CREATURE_SPECIAL)
		{
			TraceCreatureToSpecialAction(m_NextTraceID, m_bNextForceAttack);
		}
		//---------------------
		// �⺻ �ൿ -> Sector
		//---------------------
		else if (m_fNextTrace & FLAG_TRACE_SECTOR_BASIC)
		{
			TraceSectorToBasicAction(m_NextTraceX, m_NextTraceY);
		}
		//---------------------
		// Ư�� �ൿ -> Sector
		//---------------------
		else if (m_fNextTrace & FLAG_TRACE_SECTOR_SPECIAL)
		{
			TraceSectorToSpecialAction(m_NextTraceX, m_NextTraceY);
		}
		//---------------------
		// Item ����
		//---------------------
		else if (m_fNextTrace & FLAG_TRACE_ITEM)
		{
			TraceItem(m_NextTraceID);
		}
		//---------------------
		// Inventory�� Item �� ��� ���
		//---------------------
		else if (m_fNextTrace & FLAG_TRACE_INVENTORY)
		{
			TraceInventoryItem(m_NextTraceID);
		}
		//---------------------
		// Effect����
		//---------------------
		else if (m_fNextTrace & FLAG_TRACE_EFFECT)
		{
			TraceEffect(m_NextTraceID);
		}
		//---------------------
		// InteractionObject����
		//---------------------
		else if (m_fNextTrace & FLAG_TRACE_INTERACTIONOBJECT)
		{
			TraceInteractionObject(m_NextTraceID, m_NextTraceObjectAction);
		}

		//DEBUG_ADD("After CheckBufferAction : true");

		return true;
	}

	//------------------------------------------		
	//     Buffering�� ���� �ൿ
	//------------------------------------------
	BOOL bSlayer = IsSlayer();

	if (m_bNextAction ||
		m_NextAction != ACTION_STAND
		&& (!IsOusters() || IsOusters() && m_NextAction != ACTION_OUSTERS_FAST_MOVE_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
			|| IsOusters() && m_NextAction != ACTION_OUSTERS_WING_STAND
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
			|| IsOusters() && m_NextAction != ACTION_OUSTERS_UNICORN_STAND
#endif //__SECOND_TRANSFORTER
			)// && m_fNextTrace==FLAG_TRACE_NULL)
		&& (!bSlayer || bSlayer && m_NextAction != ACTION_SLAYER_MOTOR_STAND))// && m_fNextTrace==FLAG_TRACE_NULL)
	{
		//SetSpecialActionInfo(SKILL_UNTERFELDWEBEL_FIRE) ; 
		SetAction(m_NextAction);


		//SetAction( ACTION_ADVANCEMENT_SLAYER_COMBO_SWARD_SLOW);	
		DEBUG_ADD("After CheckBufferAction : true - SetAction");
		return true;
	}

	//DEBUG_ADD("After CheckBufferAction - false");

	return false;
}

//----------------------------------------------------------------------
// UnSet Repeat Action
//----------------------------------------------------------------------
void
MPlayer::UnSetRepeatAction()
{
	// �ݺ��� �����Ǹ� ���� buffering�ൿ�� ���־� �ɱ�?

	m_bRepeatAction = FALSE;

	m_RepeatCount = 0;


	m_nNextUsedActionInfo = ACTIONINFO_NULL;

	BOOL bSlayer = IsSlayer();

	//m_fTraceBuffer = FLAG_TRACE_NULL;
	/*
	#ifdef OUTPUT_DEBUG
		if (g_pDebugMessage)
			{
				if (m_nUsedActionInfo != ACTIONINFO_NULL)
				{
					DEBUG_ADD_FORMAT("UnSetRepeatAction:ai=%d, ir=%d, at=%d, sm=%d, acm=%d",

					(int)m_nUsedActionInfo,
					(int)(*g_pActionInfoTable)[m_nUsedActionInfo].IsUseRepeatFrame(),
					(int)m_Action!=ACTION_STAND,
					(int)(!bSlayer || bSlayer && m_Action!=ACTION_SLAYER_MOTOR_STAND),
					(int)m_ActionCountMax!=0);
				}
				else
				{
					DEBUG_ADD("UnSetRepeatAction: m_nUsedActionInfo is ACTIONINFO_NULL");
				}
			}
	#endif
	*/

	if (m_nUsedActionInfo != ACTIONINFO_NULL
		&& (*g_pActionInfoTable)[m_nUsedActionInfo].IsUseRepeatFrame() //m_WeaponSpeed )
		&& m_Action != ACTION_STAND
		&& (!bSlayer || bSlayer && m_Action != ACTION_SLAYER_MOTOR_STAND)
		&& m_ActionCountMax != 0)	// �̰ɷ� �ɷ���.. - -;
	{
		//m_ActionCountMax = (*g_pCreatureTable)[m_CreatureType].GetActionCount( m_Action );		
		m_ActionCountMax = GetCreatureActionCountMax(this, m_Action);

		DEBUG_ADD_FORMAT("unsetrepeat: actionCount=%d / %d", m_ActionCount, m_ActionCountMax);
	}



	//m_fNextTrace = FLAG_TRACE_NULL; 
}

//----------------------------------------------------------------------
// Add EffectStatus
//----------------------------------------------------------------------
bool
MPlayer::AddEffectStatus(EFFECTSTATUS status, DWORD delayFrame)
{
	__BEGIN_HELP_EVENT
		switch (status)
		{
		case EFFECTSTATUS_SUMMON_SYLPH_GREEN:
		case EFFECTSTATUS_SUMMON_SYLPH_ORANGE:
		case EFFECTSTATUS_SUMMON_SYLPH_RED:
		case EFFECTSTATUS_SUMMON_SYLPH_YELLOW:
		case EFFECTSTATUS_SUMMON_SYLPH_WHITE:
		case EFFECTSTATUS_SUMMON_SYLPH_BROWN:
		case EFFECTSTATUS_SUMMON_SYLPH_LILAC:
		case EFFECTSTATUS_SUMMON_SYLPH_BLACK:
		case EFFECTSTATUS_SUMMON_SYLPH:
		case EFFECTSTATUS_SUMMON_WISP_RED:
		case EFFECTSTATUS_SUMMON_WISP_YELLOW:
		case EFFECTSTATUS_SUMMON_WISP_GREEN:
			SetMoveDevice(MOVE_DEVICE_SUMMON_SYLPH);
			break;
#if __CONTENTS(__FAST_TRANSFORTER)
		case EFFECTSTATUS_CLIENT_WING_SYLPH:
#if __CONTENTS(__SECOND_TRANSFORTER)
		case EFFECTSTATUS_CLIENT_UNICORN:
#endif //__SECOND_TRANSFORTER
			SetMoveDevice(MOVE_DEVICE_WING_SYLPH);
			break;
#endif //__FAST_TRANSFORTER
			//--------------------------------------------------
			// Bless
			//--------------------------------------------------
		case EFFECTSTATUS_BLESS:
			// [����] striking
//				ExecuteHelpEvent( HE_EFFECT_BLESS );
			break;

			//--------------------------------------------------
			// Striking
			//--------------------------------------------------
		case EFFECTSTATUS_STRIKING:
			// [����] striking
//				ExecuteHelpEvent( HE_EFFECT_STRIKING );
			break;

			//--------------------------------------------------
			// Green Poison
			//--------------------------------------------------
		case EFFECTSTATUS_GREEN_POISON:
		case EFFECTSTATUS_POISON:
			// [����] green poison
//				ExecuteHelpEvent( HE_EFFECT_GREEN_POISON );
			break;

			//--------------------------------------------------
			// Purple Wall
			//--------------------------------------------------
			//case EFFECTSTATUS_PURPLE_WALL :
		case EFFECTSTATUS_CURSE_PARALYSIS:
			// [����] purple wall
//				ExecuteHelpEvent( HE_EFFECT_PURPLE_WALL );
			break;

			//--------------------------------------------------
			// Yellow Poison
			//--------------------------------------------------
		case EFFECTSTATUS_YELLOW_POISON:
		case EFFECTSTATUS_YELLOW_POISON_TO_CREATURE:
			// [����] yellow poison
//				ExecuteHelpEvent( HE_EFFECT_YELLOW_POISON );
			break;

			//--------------------------------------------------
			// BLOOD DRAIN 
			//--------------------------------------------------
		case EFFECTSTATUS_BLOOD_DRAIN:
		{
			// 2004, 5, 6 sobeit add start [����] ������������
			ExecuteHelpEvent(HELP_EVENT_DRAIN_BLOOD);
			// 2004, 5, 6 sobeit add end
		}
		break;

		//--------------------------------------------------
		// OBSERVING_EYE [�����6]
		//--------------------------------------------------
		case EFFECTSTATUS_OBSERVING_EYE:
			// ���� �� �ɷ��ִ� ���
			if (!m_bEffectStatus[EFFECTSTATUS_OBSERVING_EYE])
			{
				CalculateLightSight();
			}
			break;

		case EFFECTSTATUS_GHOST:
			SetFlyingCreature();
			//				g_pUserInformation->bCompetence = true;
			break;
		case EFFECTSTATUS_REQUEST_RESURRECT:
			// ��������� ����Ʈ�� �����Ѵ�.
			if (IsDead())	gC_vs_ui.RequestResurrectSetButton(4);
			else			return true;
			break;
		}
	__END_HELP_EVENT

		//--------------------------------------------------
		// �����̾�� ���ϴ� effect�̸�.. �ð� ������ �صд�.
		//--------------------------------------------------
		switch (status)
		{
			//--------------------------------------------------
			// ����
			//--------------------------------------------------
		case EFFECTSTATUS_BLOOD_DRAIN:
		{
			DWORD conversionTime = delayFrame * 1000 / 16;// / g_pClientConfig->FPS;

			SetConversionDelay(conversionTime);

			DEBUG_ADD_FORMAT("[BloodDrained] %ld --> %ld", g_CurrentTime, m_ConversionDelayTime);

			//			delayFrame *= 16;
			//			UI_SetHP( GetHP(), GetMAX_HP() );
		}
		break;

		//--------------------------------------------------
		// hallu
		//--------------------------------------------------
		case EFFECTSTATUS_HALLUCINATION:
			g_pZone->ChangeToHalluCreature();
			break;

		case EFFECTSTATUS_LIGHTNESS:

		case EFFECTSTATUS_FLAME_SIGHT:
		case EFFECTSTATUS_YELLOW_POISON:
		case EFFECTSTATUS_FLARE:
		case EFFECTSTATUS_YELLOW_POISON_TO_CREATURE:
			//		case EFFECTSTATUS_BLINDNESS:
			CalculateSight();
			break;
		case EFFECTSTATUS_CAN_ENTER_GDR_LAIR:
			delayFrame = delayFrame * 10;
			break;
		case EFFECTSTATUS_INSTALL_TURRET:
			//			g_pSkillAvailable->SetAvailableSkills();
			SetDelay(1000);
			gC_vs_ui.UnselectSkill();
			//gC_vs_ui.SelectSkill( SKILL_TURRET_FIRE );
			break;
		case EFFECTSTATUS_HARPOON_BOMB:
			SetEnforceResurrect(true);
			break;
		}

	bool re = MCreature::AddEffectStatus(status, delayFrame);
	//--------------------------------------------------
	// UI�� �˸���. �� �̰� re�� �Ǵ��ϸ� ��쳪��-_-; 2002.5.5 ����
	//--------------------------------------------------
	if (!(IsSlayer() && status == EFFECTSTATUS_CAUSE_CRITICAL_WOUNDS))
		UI_AddEffectStatus(status, delayFrame);

	if (status == EFFECTSTATUS_INSTALL_TURRET)
	{
		g_pSkillAvailable->SetAvailableSkills();
		gC_vs_ui.SelectSkill(SKILL_TURRET_FIRE);
	}
	CheckRegen();

	return re;
}

//----------------------------------------------------------------------
// Remove EffectStatus
//----------------------------------------------------------------------
bool
MPlayer::RemoveEffectStatus(EFFECTSTATUS status)
{
	bool bOK = MCreatureWear::RemoveEffectStatus(status);

	//--------------------------------------------------
	// UI�� �˸���.
	//--------------------------------------------------
	UI_RemoveEffectStatus(status);

	switch (status)
	{
	case EFFECTSTATUS_SUMMON_SYLPH_GREEN:
	case EFFECTSTATUS_SUMMON_SYLPH_ORANGE:
	case EFFECTSTATUS_SUMMON_SYLPH_RED:
	case EFFECTSTATUS_SUMMON_SYLPH_YELLOW:
	case EFFECTSTATUS_SUMMON_SYLPH_WHITE:
	case EFFECTSTATUS_SUMMON_SYLPH_BROWN:
	case EFFECTSTATUS_SUMMON_SYLPH_LILAC:
	case EFFECTSTATUS_SUMMON_SYLPH_BLACK:
	case EFFECTSTATUS_SUMMON_SYLPH:
	case EFFECTSTATUS_SUMMON_WISP_RED:
	case EFFECTSTATUS_SUMMON_WISP_YELLOW:
	case EFFECTSTATUS_SUMMON_WISP_GREEN:
		SetMoveDevice(MOVE_DEVICE_WALK);
		if (GetWaitVerify() == MPlayer::WAIT_VERIFY_SYLPH_SUMMON_GETOFF)
			SetWaitVerifyNULL();
		break;
#if __CONTENTS(__FAST_TRANSFORTER)
	case EFFECTSTATUS_CLIENT_WING_SYLPH:
		SetMoveDevice(MOVE_DEVICE_WALK);
		if (GetWaitVerify() == MPlayer::WAIT_VERIFY_SYLPH_SUMMON_GETOFF)
			SetWaitVerifyNULL();
		break;
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
	case EFFECTSTATUS_CLIENT_UNICORN:
		SetMoveDevice(MOVE_DEVICE_WALK);
		if (GetWaitVerify() == MPlayer::WAIT_VERIFY_SYLPH_SUMMON_GETOFF)
			SetWaitVerifyNULL();
		break;
#endif //__SECOND_TRANSFORTER
		//--------------------------------------------------
		// hallu
		//--------------------------------------------------
	case EFFECTSTATUS_HALLUCINATION:
		g_pZone->RemoveHalluCreature();
		break;

		//--------------------------------------------------
		// �����̾�� ���ϴ� effect�̸�.. �ð� ���� ����
		//--------------------------------------------------
	case EFFECTSTATUS_BLOOD_DRAIN:
		UnSetConversionDelay();

		UI_SetHP(GetHP(), GetMAX_HP());
		break;

		//--------------------------------------------------
		// �״ٰ� ��Ƴ� ���
		//--------------------------------------------------		
	case EFFECTSTATUS_COMA:
		// ���� ����
		if (g_pTempInformation->GetMode() == TempInformation::MODE_WAIT_RESURRECT)
		{
			g_pTempInformation->SetMode(TempInformation::MODE_NULL);
		}

		if (GetWaitVerify() == WAIT_VERIFY_RESURRECT)
		{
			SetWaitVerifyNULL();
		}

		gC_vs_ui.FinishRequestDie();
		gC_vs_ui.FinishRequestResurrect();
		gC_vs_ui.CloseRequestShrineMinimap();
		break;

		//--------------------------------------------------
		// OBSERVING_EYE [�����6]
		//--------------------------------------------------
	case EFFECTSTATUS_OBSERVING_EYE:
		CalculateStatus();
		break;

	case EFFECTSTATUS_GHOST:
		if (!(*g_pCreatureTable)[m_CreatureType].bFlyingCreature)	// ������ ���
			SetGroundCreature();
		break;
	case EFFECTSTATUS_WILL_OF_LIFE:
		if (IsVampire())
			MCreature::CheckRegen();
		break;
	case EFFECTSTATUS_FLARE:
	case EFFECTSTATUS_YELLOW_POISON:
	case EFFECTSTATUS_YELLOW_POISON_TO_CREATURE:
	case EFFECTSTATUS_LIGHTNESS:
	case EFFECTSTATUS_FLAME_SIGHT:
		// 2004, 6, 21 sobeit add start - about blindness
//		case EFFECTSTATUS_BLINDNESS:
			// 2004, 6, 21 sobeit add end - about blindness
		if (bOK)
			CalculateSight();
		break;
		//		case EFFECTSTATUS_GLACIER:
		//			if (m_bAlive)
		//			{
		//				SetStop();
		//				StopCauseCriticalWounds();
		//			}			
		//		break;
	case EFFECTSTATUS_INSTALL_TURRET:
		g_pSkillAvailable->SetAvailableSkills();
		ExecuteActionInfoFromMainNode(RESULT_SKILL_INSTALL_TURRET, GetX(), GetY(), 0, 0, GetID(),
			GetX(), GetY(), 0, 0, NULL, false);
		SetDelay(1000);
		break;
	case EFFECTSTATUS_HARPOON_BOMB:
		// �÷��̾ ���� �ʾ��� ��쿡�� ����
		if (!IsDead()) SetEnforceResurrect(false);
		break;
	}

	return bOK;
}

//----------------------------------------------------------------------
// Set CreatureType
//----------------------------------------------------------------------
void
MPlayer::SetCreatureType(TYPE_CREATURETYPE type)
{
	MCreatureWear::SetCreatureType(type);

	//---------------------------------------------------
	// Skill Icon�� �ٽ� üũ�Ѵ�.
	//---------------------------------------------------
	if (g_pSkillAvailable != NULL)
	{
		g_pSkillAvailable->SetAvailableSkills();
	}
	if (type == CREATURETYPE_SLAYER_OPERATOR
		|| type == CREATURETYPE_VAMPIRE_OPERATOR
		|| type == CREATURETYPE_OUSTERS_OPERATOR)
		g_pUserInformation->bCompetenceShape = true;
	else
		g_pUserInformation->bCompetenceShape = false;
}

//----------------------------------------------------------------------
// UpdateConversionTime
//----------------------------------------------------------------------
void
MPlayer::UpdateConversionTime()
{
	//--------------------------------------------------------
	// �����̾�� ���ϴ� Effectüũ
	// �ڵ� ���� �߰��ϱ�.. īī...- -;
	//--------------------------------------------------------

	// ���� ���� �ð� üũ

	if (IsSlayer())
	{
		if ((int)m_ConversionDelayTime > (int)g_CurrentTime)
		{
			//--------------------------------------------------------
			// �����̾�� ���ϴ� ���� �ð��� UI�� �����Ѵ�.
			//--------------------------------------------------------
			// msec --> min���� �ٲ۴�.   * GetTimeRatio / 1000 / 60 
			int changeTime = ((int)m_ConversionDelayTime - (int)g_CurrentTime)/* * g_pGameTime->GetTimeRatio()*/ / 60000;
			g_char_slot_ingame.CHANGE_VAMPIRE = changeTime;

			static DWORD enableBlinkTime = 0;	// �����ŷ��� �Ǵ� �ð�

			//const DWORD sixHour = 6*60*60*1000;

			// { �����ð�, �����̴� �ð� }
			const int maxBlink = 12;
			const DWORD blinkValue[maxBlink][2] =
			{
				{ 4 * 60 * 60 * 1000, 6 * 60 * 1000 },	// 4�ð� �̻� --> 5�и���			
				{ 3 * 60 * 60 * 1000, 4 * 60 * 1000 },	// 3�ð� �̻� --> 4�и���			
				{ 2 * 60 * 60 * 1000, 3 * 60 * 1000 },	// 2�ð� �̻� --> 3�и���			
				{ 1 * 60 * 60 * 1000, 2 * 60 * 1000 },	// 1�ð� �̻� --> 2�и���
				{ 1 * 60 * 60 * 1000, 1 * 60 * 1000 },	// 10�� �̻� --> 1��
				{ 3 * 60 * 1000, 30 * 1000 },			// 3�� �̻� --> 30��
				{ 2 * 60 * 1000, 10 * 1000 },			// 2�� �̻� --> 10��
				{ 1 * 60 * 1000, 5 * 1000 },			// 1�� �̻� --> 5��
				{ 30 * 60 * 1000, 3 * 1000 },			// 30�� �̻� --> 3��
				{ 10 * 1000, 2 * 1000 },			// 10�� �̻� --> 2��
				{ 10 * 1000, 2 * 1000 },			// 5�� �̻� --> 1��
				{ 0 * 1000, 300 }					// 0�� �̻� --> 0.3��
			};

			//--------------------------------------------------------
			// �������� �����̰� ����.. 
			// �ٽ� ������ �� �ִ� �ð����� üũ�Ѵ�.
			//--------------------------------------------------------
			if (g_CurrentTime > enableBlinkTime)
			{
				// ���ϱ���� ���� �ð�.. /1000�ϸ� '��'�� ���´�.
				DWORD timeGap = m_ConversionDelayTime - g_CurrentTime;

				// ������ ���. 6�ð��� �� ���� ��쿡�� effect���
				if (1)//timeGap < sixHour)
				{
					BOOL bBlink = FALSE;

					for (int i = 0; i < maxBlink; i++)
					{
						if (timeGap > blinkValue[i][0])
						{
							g_pTopView->SetFadeStart(25, 31, 2, 31, 0, 0);

							// ������ ������ �� �ִ� �ð��� �����Ѵ�.
							enableBlinkTime = g_CurrentTime + blinkValue[i][1];

							bBlink = TRUE;

							break;
						}
					}

					if (!bBlink)
					{
						UnSetConversionDelay();
					}
				}
			}
		}
		else
		{
			m_ConversionDelayTime = 0;

			//--------------------------------------------------------
			// �����̾�� ���ϴ� ���� �ð��� UI�� �����Ѵ�.
			//--------------------------------------------------------
			g_char_slot_ingame.CHANGE_VAMPIRE = 0;
		}
	}
}

//----------------------------------------------------------------------
// ���� ������ ���Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::Action()
{
	UpdateActionStep();

	if (!HasEffectStatus(EFFECTSTATUS_FIRE_ELEMENTAL) && !HasEffectStatus(EFFECTSTATUS_WATER_ELEMENTAL) && GetElementalID() != OBJECTID_NULL && g_pZone != NULL)
	{
		g_pZone->RemoveFakeCreature(GetElementalID());
	}

	// ���� �ְ� �� �������� 1�� �Ѿ�����
	if (GetPetID() != OBJECTID_NULL)
	{
		MFakeCreature* pFakeCreature = (MFakeCreature*)g_pZone->GetFakeCreature(GetPetID());
		if (pFakeCreature != NULL)
		{
			MPetItem* pPetItem = pFakeCreature->GetPetItem();
			if (pPetItem != NULL)
			{
				const int petLevelMax = 50;
				int petLevel = pPetItem->GetNumber();
				int petDelayTime = 0;
				if (pPetItem->GetItemType() >= 3 && pPetItem->GetItemType() <= 5)// 2 �� ���̸�
					petDelayTime = 3200 + ((petLevelMax - petLevel) * 2800) / petLevelMax;
				else
					petDelayTime = 1200 + ((petLevelMax - petLevel) * 2800) / petLevelMax;
#if __CONTENTS(__EXPERT_PET_CHANGER)
				//
				if (g_char_slot_ingame.bExpertPetUse)
				{
					petDelayTime = max(petDelayTime * 0.5, 900);
				}
#endif //__EXPERT_PET_CHANGER
				if (m_PetDelayTime + petDelayTime < GetTickCount())
				{
					int itemLimit = 1;	// �Ӹ��� ����

					if (pPetItem->IsCanCutHead())	// �Ӹ����� ����� ������ �Ӹ� �����ؼ�
						itemLimit = 0;

					MItem* pItem = g_pZone->GetCorpseKilledByMe(itemLimit);

					if (pItem != NULL)
					{
						CGDissectionCorpse _CGDissectionCorpse;
						_CGDissectionCorpse.setX(pItem->GetX());
						_CGDissectionCorpse.setY(pItem->GetY());
						_CGDissectionCorpse.setPet(true);
						_CGDissectionCorpse.setObjectID(pItem->GetID());

						g_pSocket->sendPacket(&_CGDissectionCorpse);

						// �Ѳ����� ������ limit�� ����������
						DEBUG_ADD_FORMAT("[PETPET] itemLimit %d", itemLimit);
						pItem->SetNumber(itemLimit);
					}

					m_PetDelayTime = GetTickCount();
				}
			}
		}
	}

	// �ʹ� ���� �ݺ� ��Ű�� �ʰ�
	if (m_nUsedActionInfo == SKILL_BLITZ_SLIDING_ATTACK || m_nUsedActionInfo == SKILL_BLAZE_WALK_ATTACK)
		UnSetRepeatAction();

	if (IsRepeatAction())
	{
		if (m_RepeatTimer + g_pClientConfig->REPEAT_TIME < GetTickCount())
			UnSetRepeatAction();
	}
	// �ʹ� ���� �ݺ� ��Ű�� �ʰ�
	if (IsLockMode())
	{
		if (m_LockTimer + g_pClientConfig->LOCK_TIME < GetTickCount())
			UnSetLockMode();
	}
	//if (m_ActionCount==m_ActionCountMax)
	//{
	//	int a =0;
	//}

	//----------------------------------------------------------------------
	// ��Ƽ���鿡�� ���� ���������� ������.
	//----------------------------------------------------------------------
	SendStatusInfoToParty();

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD_FORMAT("[MPLayer::Action] Action[%d]=%d/%d. Move=%d/%d"
		, m_Action, m_ActionCount, m_ActionCountMax, m_MoveCount, m_MoveCountMax);
#endif

	/* Test�������� �ڵ����� ���� �Դ� ��� ���� -_-
	// �׽�Ʈ�� ������ �� ������!
#ifdef __METROTECH_TEST__
	if(!IsVampire())
	{
		if (g_iAutoHealPotion == 1 && m_RecoveryHPTimes==0)	AutoUsePotion(0);		// HP
		if (g_bAutoManaPotion && m_RecoveryMPTimes==0)	AutoUsePotion(1);		// MP
		if (g_iAutoReload)	AutoUsePotion(2+g_iAutoReload-1);					// Magazine
//		if (g_iAutoHealPotion == 2 || g_iAutoHealPotion == 3)  AutoUseSkill(0);									// Healing
//		if (g_bHolyWater)  AutoUseSkill(1);									// CreateHolyWater
	}
#endif
	*/

	/*
	// [ TEST CODE ]
	// EffectTarget�� ��� �߰��ϱ� Test
	if (m_listEffectTarget.size()!=0 && m_ActionCount==m_ActionCountMax)
	{
		MActionResult* pResult = new MActionResult;
		pResult->Add( new MActionResultNodeActionInfo( 1, 1001, GUIDANCE_FIRE ) );

		// ����� ���õ� Effect�� ���� ���..
		EFFECTTARGET_LIST::iterator iEffectTarget = m_listEffectTarget.begin();
		MEffectTarget* pEffectTarget = *iEffectTarget;

		BYTE id = pEffectTarget->GetEffectID();

		if (!AddActionResult(id, pResult))
		{
			// ����� �ٷ� �����ϰ�..
			// �޸𸮿��� �����.
			delete pResult;
		}

	}
	*/

#ifdef OUTPUT_DEBUG
	if (m_ActionCount > m_ActionCountMax)
	{
		DEBUG_ADD("So Terrible!");
	}
#endif

	//--------------------------------------------------------
	// ���� --;
	//--------------------------------------------------------
//	__BEGIN_HELP_EVENT
//		static DWORD nextHelpTime = g_CurrentTime+20*1000;	// 20�� �� ó������ �޼��� ���
//		if (g_CurrentTime > nextHelpTime)
//		{
//			ExecuteHelpEvent( HE_TIME_PASSED );
//
//			nextHelpTime = g_CurrentTime + 20*1000;	// 20��
//		}
//	__END_HELP_EVENT

	//--------------------------------------------------------
	// Status ���� ��ȭ	- HP, MP ��ȭ
	//--------------------------------------------------------
	UpdateStatus();

	//--------------------------------------------------------
	// �����̾�� ���ϴ� Effectüũ
	//--------------------------------------------------------
	UpdateConversionTime();

	//--------------------------------------------------------
	// ä�� String Scroll ��ų �ð�
	//--------------------------------------------------------
	if (m_NextChatFadeTime < g_CurrentTime)
	{
		FadeChatString();
	}

	//--------------------------------------------------------
	// Ư���� ȿ����
	//--------------------------------------------------------
	if (UpDateInstallTurret())
		return;
	UpdateInvisible();
	UpdateCasket();
	UpdateCutHeight();
	UpdateTurning();
	UpdateCauseCriticalWounds();
	//	UpdateBloodyZenith();
	UpdateGunShotGuidance();
	UpdateFakeDie();
	UpdateGhost();

	UpdateDarkness();

	UpdateSweepVice();
	UpdateBurningSol();

	UpdateBikeCrash();
	//--------------------------------------------------------
	// ���� ��ȯ�� �ڿ�������
	//--------------------------------------------------------
	ChangeNearDirection();
	//m_CurrentDirection = m_Direction;

	//--------------------------------------------------------
	// ����� ���� ��� ǥ�� üũ
	//--------------------------------------------------------
#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD("beforeCheckListEffect");
#endif

	if (m_nUsedActionInfo < g_pActionInfoTable->GetMinResultActionInfo())
	{
		int numTarget = m_listEffectTarget.size();

		if (numTarget != 0)
		{
			//------------------------------------------------------------
			// ���� ����� ����ϰ� �ִٸ� 
			// �� ����� ��� �������� ����� �����ְ� �ҷ���...
			//------------------------------------------------------------
			BOOL	bActionStand = (m_Action == ACTION_STAND || IsSlayer() && m_Action == ACTION_SLAYER_MOTOR_STAND);
			//int		actionCountMax_1 = m_ActionCountMax-1;
			//BOOL	bEndAction		= (m_ActionCount==actionCountMax_1) || bActionStand;
			int		StartFrame = (*g_pActionInfoTable)[m_nUsedActionInfo].GetStartFrame(m_WeaponSpeed);

			BOOL	bStartResult = bActionStand
				|| (m_ActionCount >= StartFrame)
				|| m_ActionCount >= m_ActionCountMax;

			bStartResult |= m_bRepeatAction
				&& (*g_pActionInfoTable)[m_nUsedActionInfo].IsUseRepeatFrame()
				&& m_RepeatCount != 0
				&& m_ActionCount >= GetActionInfoRepeatStartFrame(m_nUsedActionInfo);

			// ����� ���� �����ع�����.
			if (bStartResult)
			{
				EFFECTTARGET_LIST::iterator iEffectTarget = m_listEffectTarget.begin();

				while (iEffectTarget != m_listEffectTarget.end())
				{
					MEffectTarget* pEffectTarget = *iEffectTarget;

					if (pEffectTarget->IsResultTime())// || pEffectTarget->IsEnd())
					{
						MActionResult* pResult = pEffectTarget->GetResult();

						if (pResult != NULL)
						{
							// ������ ����� ���ش�.
							int numTargetBeforeExecute = m_listEffectTarget.size();

							pEffectTarget->SetResultNULL();

							pResult->Execute();

							delete pResult;

							int numTargetAfterExecute = m_listEffectTarget.size();

							// pResult->Execute()���� m_listEffectTarget�� ���ŵ� ���� �ִ� - -;
							// �ᰡ�� ����... ������ �ʿ��ϴ� - -;;
							if (numTargetBeforeExecute != numTargetAfterExecute)
							{
								// ó������ �ٽ� üũ
								iEffectTarget = m_listEffectTarget.begin();
								continue;
							}

							// delete�� �ܺο��� �Ѵ�...
							// �Ƹ�(-_-;;) Effect�� �ٿ��� Effect ���ŵɶ� ����� ��..
							// �׷���.. ���� ����� �ȳ��µ�.. ��.��;;
							// �ϴ� ����� �ٿ�Ǵµ�. �� ����� �ٿ�ȵǴ°ɺ���.. �´µ�.. - -;;
							//delete pEffectTarget;
						}
					}

					iEffectTarget++;
				}

				m_listEffectTarget.clear();
			}
		}
	}

#ifdef OUTPUT_DEBUG_PLAYER_ACTION
	DEBUG_ADD("bcleok");
#endif

	//--------------------------------------------------------
	// ��� ������� delay�� ��� ���� �����̸�...
	//--------------------------------------------------------	
	if (!m_bTurning	// [�����]
		&& !HasEffectStatus(EFFECTSTATUS_CURSE_PARALYSIS)
		&& !HasEffectStatus(EFFECTSTATUS_STUN)
		&& !HasEffectStatus(EFFECTSTATUS_CHAIN_OF_DEMON)
		&& !HasEffectStatus(EFFECTSTATUS_CURSE_OF_BLOOD)
		&& !HasEffectStatus(EFFECTSTATUS_FREEZE)
		&& !IsCauseCriticalWounds()
		//		&& !IsBloodyZenith()
		&& !IsGunShotGuidance()
		&& !HasEffectStatus(EFFECTSTATUS_SOUL_CHAIN)
		&& !HasEffectStatus(EFFECTSTATUS_TRAP_TRIGGERED)
		&& !HasEffectStatus(EFFECTSTATUS_TRAPPED)
		//		&& g_pUserInformation->LogoutTime == 0
		|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		|| g_bLight
#endif
		)// && !HasEffectStatus(EFFECTSTATUS_CAUSE_CRITICAL_WOUNDS))	//IsNotDelay())
	{
		//--------------------------------------------------------
		// ���� buffering�� actioninfo
		//--------------------------------------------------------
		/*
		if (m_nNextUsedActionInfo!=ACTIONINFO_NULL
			&& m_nUsedActionInfo==ACTIONINFO_NULL)
		{
			m_nUsedActionInfo = m_nNextUsedActionInfo;
			m_nNextUsedActionInfo = ACTIONINFO_NULL;
		}
		*/
		//--------------------------------------------------------
		// ���� effect�� �ɷ��ִٸ� 2�� ������ �����δ�. // IsInFuryOfGnome �� �̵� , ���ӵ� �������� ������ �Ѵ�.
		//--------------------------------------------------------
		bool bAttackSlow = false, bMoveSlow = false;
		if (HasEffectStatus(EFFECTSTATUS_HAS_SLAYER_RELIC) || HasEffectStatus(EFFECTSTATUS_HAS_VAMPIRE_RELIC) || HasEffectStatus(EFFECTSTATUS_ABERRATION) ||
			HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_GREGORI) || HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_NEMA) ||
			HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_LEGIOS) || HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_MIHOLE) ||
			HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_AROSA) || HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_ARMEGA) ||
			HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_INI) || HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_JAVE) ||
			HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_CHASPA) || HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_CONCILIA) ||
			HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_KIRO) || HasEffectStatus(EFFECTSTATUS_HAS_BLOOD_BIBLE_HILLEL) ||
			HasEffectStatus(EFFECTSTATUS_AMBER_OF_GUARD) || HasEffectStatus(EFFECTSTATUS_AMBER_OF_AVENGER) ||
			HasEffectStatus(EFFECTSTATUS_AMBER_OF_IMMORTAL) || HasEffectStatus(EFFECTSTATUS_AMBER_OF_CURSE) ||
			HasEffectStatus(EFFECTSTATUS_HAS_CASTLE_SYMBOL_5) || HasEffectStatus(EFFECTSTATUS_HAS_CASTLE_SYMBOL_6) ||
			(g_pEffectStatusTable->GetSize() > EFFECTSTATUS_HAS_MIHNEA && HasEffectStatus(EFFECTSTATUS_HAS_MIHNEA)))
		{
			bAttackSlow = true; bMoveSlow = true;
		}
		if (IsCreatureActionAttack(this) && (HasEffectStatus(EFFECTSTATUS_ICE_OF_SOUL_STONE) || (IsInFuryOfGnome() && !IsOusters())))
		{
			bAttackSlow = true;
		}

		if (IsCreatureMove(dynamic_cast<MCreature*>(this)) && HasEffectStatus(EFFECTSTATUS_JABBING_VEIN) ||
			HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER) || HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_2) ||
			HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_3) || HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_4) ||
			HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_5) || HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_6) ||
			HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_7) || HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_8) ||
			HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_9) || HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_10) ||
			HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_11) || HasEffectStatus(EFFECTSTATUS_HAS_SWEEPER_12) ||
			HasEffectStatus(EFFECTSTATUS_NATURAL_PEACE) || HasEffectStatus(EFFECTSTATUS_ICE_FIELD_TO_CREATURE) ||
			HasEffectStatus(EFFECTSTATUS_FROZEN_ARMOR_TO_ENEMY))
		{
			bMoveSlow = true;
		}

		/*|| (IsInFuryOfGnome() && !IsOusters()) // ǻ�������� �̼� �������� ȿ�� ���� - 2006.11.21 by chyaya*/
		if (!g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
			&& !g_bLight
#endif
			&& (IsCreatureActionAttack(this) && bAttackSlow == true) || (IsCreatureMove(dynamic_cast<MCreature*>(this)) && bMoveSlow == true)
			)
		{
			if (g_CurrentFrame & 0x01)
			{
				return;
			}
		}


		//--------------------------------------------------------
		// KnockBack ó�� 2001.10.9
		//--------------------------------------------------------
		if (m_bKnockBack > 0)
		{
			m_sX += m_cX;
			m_sY += m_cY;

			// knockBack ��x ������ ��������.
			if (--m_bKnockBack == 0)
			{
				m_sX = 0;
				m_sY = 0;

				SetAction(ACTION_STAND);

				return;
			}
		}
		//--------------------------------------------------------
		// FastMove�߿��� action�� ������ �ʴ´�. 2001.8.10
		//--------------------------------------------------------
		else if (!m_bFastMove)
		{
			//--------------------------------------------------------
			// Lock Mode ���
			//--------------------------------------------------------
			if (IsLockMode() && !IsInDarkness())
			{
				MCreature* pCreature = m_pZone->GetCreature(GetTraceID());

				int creatureID;

				// �� �ѳ� ��
				if (pCreature == NULL ||
					(pCreature != NULL && (pCreature->IsDead() || pCreature->GetName() == NULL))
					)
					creatureID = FindEnemy();
				else
				{
					creatureID = GetTraceID();
					//20070618 by diesirace 
					//����� ������ ���� �ִ� ���� ���� ����� ���� ���� �̵��ϴ� ����
					//����� �����̰� ���� ������ �Ǿ� ������ �����Ѱ��� ������ ��� �ൿ�� 
					//������ �Ǿ� ������ �ȴ�. ���� �ڱ� �ڽſ��� ����� 
					//�� ��� �϶� �ڱ� �ڽſ��� ����� ���� ���ٰ� ���� �����ν� ���� Ÿ���� �Ǵ� ����
					//�����ߴ� ũ��ó�� ������ �ִ�. ���� �ڻ���׵� ���� ����.
					if (creatureID == g_pPlayer->GetID())
					{
						m_fNextTrace = FLAG_TRACE_SELF;
						m_NextTraceID = creatureID;
					}
				}



				//--------------------------------------------------------
				// �����Ÿ� �̳��� ���� ���� ���
				//--------------------------------------------------------
				if (creatureID == OBJECTID_NULL)
				{
					m_listDirection.clear();

					UnSetRepeatAction();
					TraceNULL();
					m_fTraceBuffer = FLAG_TRACE_NULL;
					m_fNextTrace = FLAG_TRACE_NULL;

					// ��ã�� �ߴ��� �����ش�.
					m_DestX = SECTORPOSITION_NULL;
					m_DestY = SECTORPOSITION_NULL;
					m_NextDestX = SECTORPOSITION_NULL;
					m_NextDestY = SECTORPOSITION_NULL;

					g_pTopView->SetSelectedNULL();
				}
				//--------------------------------------------------------
				// ���� �ִ� ���
				//--------------------------------------------------------
				else
				{
					int bBasicAction = g_bLButtonDown;
					int bSpecialAction = g_bRButtonDown;

					//---------------------------------------------------------------
					// �⺻ �ൿ
					//-------------------------------------------------------------
					if (bBasicAction)
					{
						if (TraceCreatureToBasicAction(
							creatureID,
							true))		// ���� ����
						{
							//----------------------------------
							// �ݺ� action�� ��츸 �ݺ� ����
							//----------------------------------
							//if (m_nBasicActionInfo!=ACTIONINFO_NULL 
							//	&& (*g_pActionInfoTable)[m_nBasicActionInfo].IsUseRepeatFrame()
							//	&& !IsRepeatAction())
							//{
							//	SetRepeatAction();
							//}

							g_pTopView->SetSelectedCreature(creatureID);
							gpC_mouse_pointer->SetCursorAttack();

							//g_bPreviousMove = false;
						}
					}
					//-------------------------------------------------------------
					// Ư�� �ൿ
					//-------------------------------------------------------------
					else if (bSpecialAction)
					{
						if (TraceCreatureToSpecialAction(creatureID, true))
						{
							//----------------------------------
							// �ݺ� action�� ��츸 �ݺ� ����
							//----------------------------------
							//if (m_nSpecialActionInfo!=ACTIONINFO_NULL 
							//	&& (*g_pActionInfoTable)[m_nSpecialActionInfo].IsUseRepeatFrame()
							//	&& !IsRepeatAction()
							//	)
							//{
							SetRepeatAction();
							g_bPreviousMove = false;
							//}							

							g_pTopView->SetSelectedCreature(creatureID);
							gpC_mouse_pointer->SetCursorAttack();

							//g_bPreviousMove = false;
						}
					}

					// ������ ǥ�ø� ���ش�.
					g_pTopView->SetSelectedSectorNULL();
				}

			}

			//--------------------------------------------------------
			// �� �긮�°� üũ
			//--------------------------------------------------------
			if (m_bAlive)
			{
				CheckDropBlood();
			}

			// delayTime�� ���� �ɷ� setting�Ѵ�.
			//m_DelayTime	= 0;
			//----------------------------------------------------
			// ��� �����ϴ� ���
			// ��.. �������� �ڵ尡 �þ��... - -;;
			//----------------------------------------------------
			if (m_bKeepTraceCreature && (m_fTrace & FLAG_TRACE_CREATURE_BASIC))
			{
				KeepTraceCreature();
			}

			//------------------------------------------		
			//     Buffering�� ���� �ൿ
			//------------------------------------------
			BOOL bSlayer = IsSlayer();
			if (m_bNextAction ||
				(!IsOusters() || IsOusters() && m_NextAction != ACTION_OUSTERS_FAST_MOVE_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
					&& m_NextAction != ACTION_OUSTERS_WING_STAND
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
					&& m_NextAction != ACTION_OUSTERS_UNICORN_STAND
#endif //__SECOND_TRANSFORTER
					)
				&& m_NextAction != ACTION_STAND
				&& (!bSlayer || bSlayer && m_NextAction != ACTION_SLAYER_MOTOR_STAND)
				&& m_NextAction != m_MoveAction)
			{
				SetAction(m_NextAction);

#ifdef OUTPUT_DEBUG
				if (m_ActionCount > m_ActionCountMax)
				{
					DEBUG_ADD("So Terrible! return1");
				}
#endif

				return;
			}

			//------------------------------------------
			// [ TEST CODE ]
			//------------------------------------------
			// �����̴� ���߿�..
			// �����ϴ� ��� ������ǥ�Ÿ��� �Ǵ� ��쿡��..
			// �ٷ� �̹���.. action�� �����ϰ� �ؾ��Ѵ�.
			// �׷���, return false
			if (m_NextAction == m_MoveAction &&
				m_fTrace != FLAG_TRACE_NULL &&
				(max(abs(m_X - m_TraceX), abs(m_Y - m_TraceY))) <= m_TraceDistance)
			{
				SetAction(m_NextAction);
			}

			//------------------------------------------
			// CastingTime�� ������
			// Action ����.. ���� Frame����...
			//------------------------------------------
			if (m_ActionCount < m_ActionCountMax)
			{
				if (m_nUsedActionInfo != ACTIONINFO_NULL)
				{
					ActionEffect();
				}
				else
				{
					//m_ActionCount--;

					// ������ Action ����
					//switch (m_Action)
					//

					BOOL bSlayer = IsSlayer();

					// ���� ����
					if (m_Action == ACTION_STAND || IsOusters() && m_Action == ACTION_OUSTERS_FAST_MOVE_MOVE
						|| bSlayer && m_Action == ACTION_SLAYER_MOTOR_STAND
#if __CONTENTS(__FAST_TRANSFORTER)
						|| IsOusters() && m_Action == ACTION_OUSTERS_WING_MOVE
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
						|| IsOusters() && m_Action == ACTION_OUSTERS_UNICORN_MOVE
#endif //__SECOND_TRANSFORTER
						)
					{

						m_ActionCount++;

						//------------------------------------------------------------
						// �ݺ� �ൿ üũ..
						//------------------------------------------------------------
						if (CheckRepeatAction())
						{
							if (CheckBufferAction())
							{
								if (m_nNextUsedActionInfo != ACTIONINFO_NULL)
								{
									// ��.. �򰥷���.. - -;
									m_nUsedActionInfo = m_nNextUsedActionInfo;
									m_nNextUsedActionInfo = ACTIONINFO_NULL;

									if (ActionInTraceDistance())
									{
										if (m_nNextUsedActionInfo != ACTIONINFO_NULL)
										{
											// ��.. �򰥷���.. - -;
											m_nUsedActionInfo = m_nNextUsedActionInfo;
											m_nNextUsedActionInfo = ACTIONINFO_NULL;
										}
										else
										{
											// �� �̻� �� �ʿ䰡 ����.
											m_listDirection.clear();

											// �̵� ����
											m_MoveCount = m_MoveCountMax;

											// Player�� �ٶ󺸴� ������ ���� ���� �ִ� ������ �ǵ��� �Ѵ�.				
											SetDirectionToPosition(m_TraceX, m_TraceY);

											SetAction(m_NextAction);

											//ActionToSendPacket();
										}
									}
								}
#ifdef OUTPUT_DEBUG
								if (m_ActionCount > m_ActionCountMax)
								{
									DEBUG_ADD("So Terrible! return2");
								}
#endif

								return;
							}
						}

						/*
						else if (m_Action==ACTION_MOVE
								|| bSlayer && ACTION_SLAYER_MOTOR_MOVE)
						{
							// Move�� ���� ó���Ѵ�.
							//ActionMove();
						}
						*/
					}
					else
					{
						//default :
						if (m_nUsedActionInfo == ACTIONINFO_NULL)
						{
							m_ActionCount++;

							if (m_bStopBloodDrain)
							{
								StopBloodDrain();
							}
							if (m_bStopAbsorbSoul)
							{
								StopAbsorbSoul();
							}
						}
						else
						{
							ActionEffect();
						}
					}
				}
			}
		}



		//------------------------------------------
		// Action�� ���� �����̸�
		//------------------------------------------
		if (m_bAlive)
		{
			// �ε�ȣ �߰� 2001.10.7
			if (m_ActionCount >= m_ActionCountMax) //if (IsStop())
			{
				//--------------------------------------------------------
				// ��ǥ Ÿ�Ͽ� ������ ���¿���..
				// ��� �� �ְų�.. �� ���� ����.. 
				// ���ڸ����� ���Ÿ��� ����� ǥ�����ش�.
				//--------------------------------------------------------
				// �ε�ȣ �߰� 2001.10.7
				if (m_MoveCount >= m_MoveCountMax)
				{
					//if (m_bRepeatAction && m_RepeatCount!=0 && (*g_pActionInfoTable)[m_nUsedActionInfo].IsRepeatFrame())
					//{						
					//	int a =0;
					//}
					//------------------------------------------------------------
					// �ݺ� �ൿ üũ..
					//------------------------------------------------------------
					//if (
					bool bCheckRepeat = CheckRepeatAction();
					//)
				//{
				//}

					if ((!m_bRepeatAction || !bCheckRepeat)
						&& m_fNextTrace != FLAG_TRACE_NULL)
					{
						if (m_ActionCount >= m_ActionCountMax)
						{
							SetAction(ACTION_STAND);
						}
					}

					if (CheckBufferAction())
					{
						/*
						if (m_NextAction==m_MoveAction &&
							//m_fTrace != FLAG_TRACE_NULL &&
							(max(abs(m_X-m_TraceX), abs(m_Y-m_TraceY))) <= m_TraceDistance)
						{
							SetAction( m_NextAction );
						}
						*/
						if (m_nNextUsedActionInfo != ACTIONINFO_NULL)
						{
							// ��.. �򰥷���.. - -;
							m_nUsedActionInfo = m_nNextUsedActionInfo;
							m_nNextUsedActionInfo = ACTIONINFO_NULL;

							if (ActionInTraceDistance())
							{
								if (m_nNextUsedActionInfo != ACTIONINFO_NULL)
								{
									// ��.. �򰥷���.. - -;
									m_nUsedActionInfo = m_nNextUsedActionInfo;
									m_nNextUsedActionInfo = ACTIONINFO_NULL;
								}
								else
								{
									// �� �̻� �� �ʿ䰡 ����.
									m_listDirection.clear();

									// �̵� ����
									m_MoveCount = m_MoveCountMax;

									// Player�� �ٶ󺸴� ������ ���� ���� �ִ� ������ �ǵ��� �Ѵ�.				
									SetDirectionToPosition(m_TraceX, m_TraceY);

									SetAction(m_NextAction);

									//ActionToSendPacket();
								}
							}
						}

#ifdef OUTPUT_DEBUG
						if (m_ActionCount > m_ActionCountMax)
						{
							DEBUG_ADD("So Terrible! return3");
						}
#endif

						return;
					}

					//------------------------------------------
					// ���ڸ����� ���Ÿ��� ���
					//------------------------------------------
					if (m_bRepeatAction)// && m_RepeatCount!=0)// && (*g_pActionInfoTable)[m_nUsedActionInfo].IsRepeatFrame())
					{
					}
					else
					{
						SetAction(ACTION_STAND);
					}

				}
				//--------------------------------------------------------
				// �ȴٰ� �ٸ� action�� ������ų� ��� �ȴ� ���̴�.
				// �ٽ� �ȴ´�.
				//--------------------------------------------------------					
				else
				{
					m_Action = m_MoveAction;
					ActionMove();
#if __CONTENTS(__HERO_SYSTEM)
					if (HasEffectStatus(EFFECTSTATUS_SLAYER_HERO) ||
						HasEffectStatus(EFFECTSTATUS_VAMPIRE_HERO) ||
						HasEffectStatus(EFFECTSTATUS_OUSTERS_HERO) ||
						HasEffectStatus(EFFECTSTATUS_LEVELWAR_SLAYER_HERO) ||
						HasEffectStatus(EFFECTSTATUS_LEVELWAR_VAMPIRE_HERO) ||
						HasEffectStatus(EFFECTSTATUS_LEVELWAR_OUSTERS_HERO)
						)
					{
						ActionMove();
					}
#endif //__HERO_SYSTEM

					//----------------------------------------------------
					// �̵��� ��� ��������..
					// ���ڸ����� ���Ÿ��� ��� 
					//----------------------------------------------------
					if (m_MoveCount == m_MoveCountMax)
					{
						if (m_fNextTrace != FLAG_TRACE_NULL)
						{
							if (m_ActionCount >= m_ActionCountMax)
							{
								SetAction(ACTION_STAND);
							}
						}

						if (CheckBufferAction())
						{
							if (m_nNextUsedActionInfo != ACTIONINFO_NULL)
							{
								// ��.. �򰥷���.. - -;
								m_nUsedActionInfo = m_nNextUsedActionInfo;
								m_nNextUsedActionInfo = ACTIONINFO_NULL;

								if (ActionInTraceDistance())
								{
									if (m_nNextUsedActionInfo != ACTIONINFO_NULL)
									{
										// ��.. �򰥷���.. - -;
										m_nUsedActionInfo = m_nNextUsedActionInfo;
										m_nNextUsedActionInfo = ACTIONINFO_NULL;
									}
									else
									{
										// �� �̻� �� �ʿ䰡 ����.
										m_listDirection.clear();

										// �̵� ����
										m_MoveCount = m_MoveCountMax;

										// Player�� �ٶ󺸴� ������ ���� ���� �ִ� ������ �ǵ��� �Ѵ�.				
										SetDirectionToPosition(m_TraceX, m_TraceY);

										SetAction(m_NextAction);

										//ActionToSendPacket();
									}
								}
							}

#ifdef OUTPUT_DEBUG
							if (m_ActionCount > m_ActionCountMax)
							{
								DEBUG_ADD("So Terrible! return4");
							}
#endif

							return;
						}

						//------------------------------------------
						// ���ڸ����� ���Ÿ��� ���
						//------------------------------------------				
						if (m_ActionCount >= m_ActionCountMax)
						{
							SetAction(ACTION_STAND);
						}
					}
				}
			}
		}
	}


	//--------------------------------------------------------
	// Delay�� �ִ� ���...  
	// ��� �ִ� ��쿡�� --> ���������� �����ش�.
	//--------------------------------------------------------
	/*
	else if (m_bAlive)
	{
		if (m_ActionCount < m_ActionCountMax)
		{
			if (m_Action==ACTION_STAND
				|| IsSlayer() && m_Action==ACTION_SLAYER_MOTOR_STAND)
			{
				m_ActionCount++;
			}

			// action�� ��������..
			if (m_ActionCount==m_ActionCountMax)
			{
				// �ٽ� ���� ����
				SetAction( (m_MoveAction==ACTION_SLAYER_MOTOR_MOVE)? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND );
			}		hflh
		}
		else
		{
			SetAction( (m_MoveAction==ACTION_SLAYER_MOTOR_MOVE)? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND );
		}
	}
	*/

}

//----------------------------------------------------------------------
// Add EffectTarget
//----------------------------------------------------------------------
// �������� EffectTarget�� �߰��Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::AddEffectTarget(MEffectTarget* pEffectTarget)
{
	// �������� �� ����������. ����
	m_listEffectTarget.clear();

	// ���� �ϳ� �߰�
	m_listEffectTarget.push_back(pEffectTarget);
}

//----------------------------------------------------------------------
// Remove EffectTarget
//----------------------------------------------------------------------
// �������̴� Effect�� ���� ���...
// �׿� �����ִ� Target�� �����Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::RemoveEffectTarget(BYTE id)
{
	EFFECTTARGET_LIST::iterator iEffectTarget = m_listEffectTarget.begin();

	// id�� ���� ��츦 ã�Ƽ� �����Ѵ�.
	// list�̱� ������.. �� ���ؾ� ������...
	// ������ Player�� ������� ����� �~ �ۿ� �ȵǱ� ������...
	while (iEffectTarget != m_listEffectTarget.end())
	{
		// id�� ���� ���
		if ((*iEffectTarget)->GetEffectID() == id)
		{
			// �޸𸮿��� ���Ŵ� �ܺο��� �Ѵ�.
			//delete (*iEffectTarget);

			// list���� �����Ѵ�.			
			m_listEffectTarget.erase(iEffectTarget);

			break;
		}

		iEffectTarget++;
	}
}

//----------------------------------------------------------------------
// Packet Back
//----------------------------------------------------------------------
// �̵� ���̸�..
// �ٷ� ���� ĭ���� Back�Ѵ�.
// ���� ���������� ó���̴�.
// ��ŷ�� ������ ���� ������.. ����.. - -;;
//----------------------------------------------------------------------
/*
void
MPlayer::PacketBack()
{
	// ������ �����̸� �ʿ䰡 ����.
	if (m_sX==0 && m_sY==0)
		return;

	// �ݴ����..
	//m_sX = -m_sX;
	//m_sY = -m_sY;
	//m_cX = -m_cX;
	//m_cY = -m_cY;
	//m_ActionCount = m_ActionCountMax - m_ActionCount;


	// ������ ���°� �ƴϸ�.. ���� ��Ų��.
	SetStop();

	// �ٷ� ���� ��ġ��...
	// ���� ��ġ���� ��������� �ݴ�������� �ǵ��� ���� �ȴ�.
	m_Direction = GetCounterDirection( m_Direction );

	// �������� ������ ��ǥ�� ��´�.
	GetPositionToDirection(m_X, m_Y, m_Direction);

	// Back�̵��� �ߴٰ� Server�� ������.
	#ifdef	CONNECT_SERVER
		CGMove _CGMove;
		_CGMove.setX( m_X );
		_CGMove.setY( m_Y );
		_CGMove.setDir( m_Direction );
		g_pSocket->sendPacket( &_CGMove );

		#if defined(OUTPUT_DEBUG) && defined(__DEBUG_OUTPUT__)
				sprintf(g_pDebugMessage->GetCurrent(), "[Send] [Back]%s", _CGMove.toString().c_str());
				g_pDebugMessage->Next();
		#endif

		// Server�� ���� ������ ����صд�.
		m_listSendDirection.push_back( m_Direction );
		m_SendMove++;

	#endif
}
*/

//----------------------------------------------------------------------
// Packet MoveOK
//----------------------------------------------------------------------
// Server���� MOVE_OK�� ���� ���
//
// ������ġ���� direction������ (x,y) sector�� �̵������ϴٴ� �ǹ��̴�.
//----------------------------------------------------------------------
void
MPlayer::PacketMoveOK(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, BYTE direction)
{
	// 2004, 9, 16, sobeit add start - �ͷ� ���� �ٲٱ�, ����..�Ѥ�;
//	if(HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
//	{
//		SetCurrentDirection(direction);
//		return;
//	}
	// 2004, 9, 16, sobeit add end
	// 2004, 9, 21, sobeit add start - �ƴ� �� ����
	m_bShowAdamCloud = false;
	if (NULL != g_pZone && g_pZone->GetID() == 74)
	{
		if (sY < 60)
			m_bShowAdamCloud = true;
	}
	// 2004, 9, 21, sobeit add end
	//--------------------------------------------------------
	// ����ص� �� �߿���..
	// �������� ������ �����Ѵ�.
	//--------------------------------------------------------
	if (!m_listSendDirection.empty())
	{
		m_listSendDirection.pop_front();
	}

	//--------------------------------------------------------
	// MoveError������ MoveOK�� �ٷ� ���ƿ��� ���
	//--------------------------------------------------------
	if (m_SendMove == 0)
	{
		//--------------------------------------------------------
		// MoveError������ �ٷ� MoveOK�� ���ƿ��� ��찡 �־���.
		// �� ���.. (�Ƹ� - -;) ��ǥ�� �߲�~�ؼ� 
		// ��� �������� ��ǥ ������ ���޴°� ����.
		// �׷���.. �ӽ�(?) ��å����... Server���� ������ ��ǥ�� �ٷ� �̵���Ų��.
		//--------------------------------------------------------
		MovePosition(sX, sY);

		// �̵��� �����Ѵ�.
		m_sX = 0;
		m_sY = 0;

		m_MoveCount = m_MoveCountMax;
		m_ActionCount = m_ActionCountMax;

		// ��ã�� ����
		m_listDirection.clear();

		//----------------------------------------------------
		// Creature�� ��� �����ϴ� ���̸�..
		//----------------------------------------------------
		if (m_bKeepTraceCreature						// ��� �����ϴ� ���
			&& (m_fTrace & FLAG_TRACE_CREATURE_BASIC)	// creature�� �����ϴ� ���
			&& !m_bTraceCreatureToForceAttack)			// �׳� ���󰡴� ���
		{
			KeepTraceCreature();
		}
		//----------------------------------------------------
		// ���� �̵��ÿ� ������ ���
		//----------------------------------------------------
		else
		{
			TraceNULL();

			// ��ã�� �ߴ��� �����ش�.
			m_DestX = SECTORPOSITION_NULL;
			m_DestY = SECTORPOSITION_NULL;
			m_NextDestX = SECTORPOSITION_NULL;
			m_NextDestY = SECTORPOSITION_NULL;
		}
	}
	//--------------------------------------------------------
	// �������� MoveOK�� ���
	//--------------------------------------------------------
	else if (m_SendMove > 0)
	{
		m_SendMove--;
	}

	g_bNetStatusGood = true;

	//--------------------------------------------------------
	// Server���� ������ ��ġ�� ����صд�.
	//--------------------------------------------------------
	//POINT nextPoint;
	//MCreature::GetNextPosition(direction, nextPoint);
//	m_ServerX	= nextPoint.x;
//	m_ServerY	= nextPoint.y;

	m_ServerX = sX;
	m_ServerY = sY;


	//--------------------------------------------------------
	// (m_ServerX, m_ServerY)�������� 
	// Player�� �þ߿� ���Ե��� �ʴ� Creature�� Zone���� ���Ž�Ų��.
	//--------------------------------------------------------
	if (m_pZone != NULL)
	{
		m_pZone->KeepObjectInSight(m_ServerX, m_ServerY, m_Sight);
	}

	SendPositionInfoToParty();

	return;
}

//----------------------------------------------------------------------
// Packet MoveNO
//----------------------------------------------------------------------
// Server���� MOVE_NO�� ���� ���
//----------------------------------------------------------------------
void
MPlayer::PacketMoveNO()
{
	//m_listSendDirection.front();
	if (!m_listSendDirection.empty())
	{
		m_listSendDirection.pop_front();
	}

	m_SendMove = 0;

	//------------------------------------------------------------
	// ���� ��ġ�� �� �� �ִ� ���� �ִ��� ���캻��.
	//
	// ���� ���� ����̰ų� ���� �� �� ���?�� 
	//		�ֱ��� �������� ��ġ(ServerX,Y)�� �̵��Ѵ�.
	//
	// ���� �ִٸ�, ã�� ��ġ������ �̵� packet�� ������.
	//------------------------------------------------------------
	//if (!SetDestination( m_X, m_Y ) || m_listDirection.empty()	// ���� ���� ���
		//|| m_SendMove > g_pClientConfig->MAX_CLIENT_MOVE)		// ���� �� �� ���
	{
		// Zone���� ���� �ִ� Sector���� �����ϰ� 
		// Server���� ������ �ֱ��� Sector�� �̵��Ѵ�.
		MovePosition(m_ServerX, m_ServerY);


		// �̵��� �����Ѵ�.
		m_sX = 0;
		m_sY = 0;

		m_MoveCount = m_MoveCountMax;
		//m_ActionCount	= m_ActionCountMax; 

		//----------------------------------------------------
		// Creature�� ��� �����ϴ� ���̸�..
		//----------------------------------------------------
		if (m_bKeepTraceCreature						// ��� �����ϴ� ���
			&& (m_fTrace & FLAG_TRACE_CREATURE_BASIC)	// creature�� �����ϴ� ���
			&& !m_bTraceCreatureToForceAttack			// �׳� ���󰡴� ���
			&& m_ActionCount >= m_ActionCountMax)			// Ư���� ���۾��� �����̰� �ִ� ���
		{
			KeepTraceCreature();
		}
		//----------------------------------------------------
		// ���� �̵��ÿ� ������ ���
		//----------------------------------------------------
		else
		{
			// ��ã�� �ߴ��� �����ش�.
			m_listDirection.clear();

			/*
			// �� �ּ�ó���ϸ� ���� �ɰŰ���... - -;  2001.8.1
			m_DestX		= SECTORPOSITION_NULL;
			m_DestY		= SECTORPOSITION_NULL;
			m_NextDestX	= SECTORPOSITION_NULL;
			m_NextDestY	= SECTORPOSITION_NULL;

			TraceNULL();

			// 2001.7.27 �߰�
			UnSetRepeatAction();
			*/
		}

		// 2001.8.22
		if (m_Action == m_MoveAction)
		{
			// �̵� ���̸� �׳� ����
			/*
			SetStop();

			m_DestX		= SECTORPOSITION_NULL;
			m_DestY		= SECTORPOSITION_NULL;
			m_NextDestX	= SECTORPOSITION_NULL;
			m_NextDestY	= SECTORPOSITION_NULL;

			TraceNULL();

			UnSetRepeatAction();
			*/
			// ��ã�� ����
			m_listDirection.clear();

			// ���� ����	
			//m_fTrace		= FLAG_TRACE_NULL;
			//m_fNextTrace	= FLAG_TRACE_NULL;

			// Action ����
			//m_Action		= (m_MoveAction==ACTION_SLAYER_MOTOR_MOVE)? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND;
			//m_ActionCount	= m_ActionCountMax; 	
			m_MoveCount = m_MoveCountMax - 1;
			m_NextMoveCount = m_MoveCount;

			// ���� ���۵� ����
			m_bNextAction = false;
			m_NextX = SECTORPOSITION_NULL;
			m_NextY = SECTORPOSITION_NULL;

			//m_nNextUsedActionInfo = ACTIONINFO_NULL;
			//m_nUsedActionInfo = ACTIONINFO_NULL;


			// ��ǥ ����
			m_DestX = SECTORPOSITION_NULL;
			m_DestY = SECTORPOSITION_NULL;
			m_NextDestX = SECTORPOSITION_NULL;
			m_NextDestY = SECTORPOSITION_NULL;
		}
		// 2001.8.6
		// �ӽ� ��å.. - �����̴ٰ� �ε����� ƨ��� ���..
		else if (m_ActionCount >= m_ActionCountMax)
		{
			if (m_Action == m_MoveAction)
			{
				/*
				SetStop();

				m_DestX		= SECTORPOSITION_NULL;
				m_DestY		= SECTORPOSITION_NULL;
				m_NextDestX	= SECTORPOSITION_NULL;
				m_NextDestY	= SECTORPOSITION_NULL;

				TraceNULL();

				UnSetRepeatAction();
				*/
				m_sX = 0;//-m_cX;
				m_sY = 0;//-m_cY;

				m_MoveCount = m_MoveCountMax - 1;
				m_NextMoveCount = m_MoveCount;
			}
		}

		//m_SendMove = 0;
		// ���⼭ ���� SX, SY���� �̻��ϰ� �Ǵµ�~~!!!!!!!!!!!!!!!!
	}
	/*
	// ��ĭ ���̷� ��ü�� ���������� ������
	// ���� �ǹ̰� ��� �ּ�ó���ص״�.
	// �׸���.. �׽�Ʈ�� �ѹ��� ���غ� �ڵ��̴�. - -;
	else
	{
		DEBUG_ADD_FORMAT("[MoveNo] Find New Way. num=%d", m_listDirection.size());

		//----------------------------------------------------
		//
		// (m_ServerX, m_ServerY)����
		//	(m_X, m_Y)���� ������ �����δ�.
		//
		//----------------------------------------------------
		int x = m_ServerX;
		int y = m_ServerY;

		while (!m_listDirection.empty())
		{
			//----------------------------------------------------
			// m_listDirection���� ������ �ϳ� �о�´�.
			//----------------------------------------------------
			int direction = m_listDirection.front();
			m_listDirection.pop_front();

			switch (direction)
			{
				case DIRECTION_LEFTDOWN		: x--;	y++;	break;
				case DIRECTION_RIGHTUP		: x++;	y--;	break;
				case DIRECTION_LEFTUP		: x--;	y--;	break;
				case DIRECTION_RIGHTDOWN	: x++;	y++;	break;
				case DIRECTION_LEFT			: x--;			break;
				case DIRECTION_DOWN			:		y++;	break;
				case DIRECTION_UP			:		y--;	break;
				case DIRECTION_RIGHT		: x++;			break;
			}

			//----------------------------------------------------
			// Server���� ������ �޾ƾ� �Ѵ�.
			//----------------------------------------------------
			#ifdef	CONNECT_SERVER
				CGMove _CGMove;
				_CGMove.setX( x );
				_CGMove.setY( y );
				_CGMove.setDir( direction );
				g_pSocket->sendPacket( &_CGMove );

				// Server�� ���� ������ ����صд�.
				m_listSendDirection.push_back( direction );
				m_SendMove++;
			#endif
		}
	}
	*/
}


//----------------------------------------------------------------------
// Packet SpecialAction Result
//----------------------------------------------------------------------
// this Creature�� Damage�� �޾����Ƿ�
// �̵� ���̸� ��� ���� Tile�� Back���Ѿ� �Ѵ�.
//
// 0. ��� ������ �����ϰ�..
// 1. Action --> Damage�޴� ����
// 2. Damage ǥ��
//----------------------------------------------------------------------
void
MPlayer::PacketSpecialActionResult(TYPE_ACTIONINFO nResultActionInfo, TYPE_OBJECTID id, TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, BYTE temp)
{
	//--------------------------------------------------
	// �׾����� return
	//--------------------------------------------------
	if (!m_bAlive)
		return;

	//----------------------------------------------------------------------
	// ��� action�� �ƴ� ���..  - �ǹ̰� �ֳ�?? ��..
	//----------------------------------------------------------------------
	if (nResultActionInfo < (*g_pActionInfoTable).GetMinResultActionInfo()
		// fast move�߿��� action�� �������� �ʴ´�... // 2001.8.10
		// ������ ������ �ʿ䰡 �ִ� �͵� �ִ� --;;
		|| (m_bFastMove &&
			(nResultActionInfo != RESULT_SKILL_BOMB_CRASH_WALK &&
				nResultActionInfo != RESULT_SKILL_BLOODS_SYMPOSION_ATTACK))
		|| m_bKnockBack)
	{
		return;
	}

	//----------------------------------------------------------------------
	// �� ȿ��.. - -;
	//----------------------------------------------------------------------
	int actionInfoAction = GetActionInfoAction(nResultActionInfo);

	if (actionInfoAction == ACTION_DAMAGED)
	{
		//if (HasEffectStatus(EFFECTSTATUS_AURA_PRISM))
		{
			//	nResultActionInfo = REFLECT_AURA_PRISM;
			//	actionInfoAction = GetActionInfoAction(nResultActionInfo);
		}
		//else 
		if (HasEffectStatus(EFFECTSTATUS_AURA_SHIELD))
		{
			nResultActionInfo = REFLECT_AURA_PRISM;//REFLECT_AURA_SHIELD;
			actionInfoAction = GetActionInfoAction(nResultActionInfo);
		}

	}

	//-------------------------------------------------
	// �ݺ� action ������ ����صд�.
	//-------------------------------------------------
	int oldTraceID = m_TraceID;

	//----------------------------------------------------------------------
	// ������ ����ҷ��� ���� �־��ٸ�...
	//----------------------------------------------------------------------
	///*
	m_nNextUsedActionInfo = ACTIONINFO_NULL;
	TraceNextNULL();

	if (m_nUsedActionInfo != ACTIONINFO_NULL)
	{
		BOOL	bStartActionInfo = m_nUsedActionInfo < g_pActionInfoTable->GetMinResultActionInfo();
		BOOL	bSentPacket = (m_ActionCount > 0);
		int		StartFrame = (*g_pActionInfoTable)[m_nUsedActionInfo].GetStartFrame(m_WeaponSpeed);

		// Effect�� ���۵Ǵ� ����..
		// (1) StartFrame�� ���
		// (2) ������ ActionFrame�� ���
		BOOL	bStartedEffect = m_ActionCount >= StartFrame;

		//--------------------------------------------------
		// �̹� effect�� ������ �ƴ϶�� ���δ�.
		//--------------------------------------------------		
		if (!bStartActionInfo						// ��� ��Ŷ�̰ų�..
			|| (bSentPacket && !bStartedEffect))	// ��Ŷ�� ���´µ� Effect�� �� ���� ���..
		{
			TYPE_ACTIONINFO oldUsedActionInfo = m_nUsedActionInfo;

			m_nUsedActionInfo = ACTIONINFO_NULL;

			AffectUsedActionInfo(oldUsedActionInfo);

			// AffectUsedActionInfo�� ���ؼ� ���� ��� ǥ��..
			if (m_nUsedActionInfo != ACTIONINFO_NULL)
			{
				AttachCastingEffect(m_nUsedActionInfo, TRUE);
				AffectUsedActionInfo(m_nUsedActionInfo);
			}
		}
		//--------------------------------------------------		
		// �ٷ� ������ Effect�� ��� �� �ϴ� ���.
		//--------------------------------------------------		
		else
		{
			m_nUsedActionInfo = ACTIONINFO_NULL;
		}

		//-------------------------------------------------------------
		// castingEffect�� ���� ��� �ȵ����� ��½�Ų��.
		//-------------------------------------------------------------
		if (m_nUsedActionInfo != ACTIONINFO_NULL
			&& GetActionInfoCastingStartFrame(m_nUsedActionInfo) >= m_ActionCount)
		{
			AttachCastingEffect(m_nUsedActionInfo, TRUE);
		}

	}
	//*/

	// �̵��� �����Ѵ�.
	//m_sX = 0;	
	//m_sY = 0;
	// Back��Ų��.	
	////SetStop();
	//PacketBack();		

	//m_ActionCount = m_ActionCountMax; 

	// ���� ����
	//-------------------------------------------------
	// 2000.09.22 �� m_fTrace�� NULL�� �ٲ�..
	//-------------------------------------------------
	//TraceNULL();


	m_fTrace = FLAG_TRACE_NULL;


	SetTraceID(id);
	m_TraceX = sX;
	m_TraceY = sY;

	// ��ã�� �ߴ��� �����ش�.
	m_listDirection.clear();
	m_DestX = m_X;//SECTORPOSITION_NULL;
	m_DestY = m_Y;//SECTORPOSITION_NULL;
	m_NextDestX = SECTORPOSITION_NULL;
	m_NextDestY = SECTORPOSITION_NULL;

	// nResultActionInfo�� �ش��ϴ� ActionInfo�� ã�ƾ� �Ѵ�.
	// ����ActionInfo + MIN_RESULT_ACTIONINFO�� �ϸ� �ȴ�.
	//	m_nUsedActionInfo	= nResultActionInfo;	// + (*g_pActionInfoTable).GetMinResultActionInfo()
	//SetAction( (*g_pActionInfoTable)[m_nUsedActionInfo].GetAction() );

	// �̷��� �ع�����..
	// ���� �����ϴ� Tile���� ����.. ������ �ޱ� ������
	// �� ĭ �������� ���ݹ޴� ��찡 �߻��Ѵ�.
	// ��� �ؾ��ұ�??
	//m_NextAction = (*g_pActionInfoTable)[m_nUsedActionInfo].GetAction();

	//SetNextAction( GetActionInfoAction(m_nUsedActionInfo) );
	//m_bNextAction = true;

	if (!m_bFastMove)
	{

		// 2001.05.21 �߰�
		// ��� ���ۿ��� ACTION_STAND�� �������� �ʴ´�.
		if (actionInfoAction != ACTION_STAND)
		{
			SetAction(actionInfoAction);
		}

		if (temp != 0)
			SetActionGrade(temp);

	}

	// �ٷ� ����
	//-------------------------------------------------------------
	//
	// Casting effect
	//
	//-------------------------------------------------------------
	//-------------------------------------------------------------
	// Casting effect
	//-------------------------------------------------------------
	AttachCastingEffect(nResultActionInfo, TRUE);
	AffectUsedActionInfo(nResultActionInfo);


	//-------------------------------------------------
	// �ݺ� action ������...
	//-------------------------------------------------
	SetTraceID(oldTraceID);


	//--------------------------------------------------------
	// ������� �� �´� ��е� (���� / ���� ���� )�� ������ش�. 
	//--------------------------------------------------------
	// 2005.08.12 Sjheon ������ ���� / ���� / �ƿ콺���� �� ���� ���� ���� ��  �޶�����.
	int soundID = 0;
	if (IsOusters())
	{
		soundID = (*g_pActionInfoTable)[nResultActionInfo].GetMaleSoundID();
	}
	else
	{
		if (IsMale())
			soundID = (*g_pActionInfoTable)[nResultActionInfo].GetMaleSoundID();
		else
			soundID = (*g_pActionInfoTable)[nResultActionInfo].GetFemaleSoundID();
	}

	if (soundID != SOUNDID_NULL)
		PlaySound(soundID, false, sX, sY);
	//--------------------------------------------------------

	// �ڽſ��� ���?..
	//-------------------------------------------------
	// 2000.09.22 �� �ּ� ó��.. -_-;
	//-------------------------------------------------
	//m_TraceID = m_ID;

	// Test Code
	/*
	if (rand()%2)
		SetChatString( "Shit!" );
	else
		SetChatString( "Oops!" );
	*/
}

//----------------------------------------------------------------------
// Packet Add ActionResult
//----------------------------------------------------------------------
// Player�� ����� Action�� ���õ� ����� Server�� ���� �޾Ƽ�
// �ܺο��� ActionResult�� ������ ���� 
// Player�� ActionResultList�� �߰��Ѵ�.
//
// ��, InstanceID�� id�� MEffectTarget�� Result�� �����Ѵ�.
//----------------------------------------------------------------------
// return���� 
//     true�̸� �߰��Ǿ��ٴ� �ǹ�
//     false�̸� ���õ� EffectTarget�� �����Ƿ� 
//               ����� �ٷ� ��������� �Ѵ�.
//----------------------------------------------------------------------
bool
MPlayer::PacketAddActionResult(WORD effectID, MActionResult* pActionResult)
{
	/*
	MActionResult* pResult = new MActionResult;
	pResult->Add( new MActionResultNodeActionInfo( m_ID, m_TraceID, m_nUsedActionInfo ) );

	pEffectTarget->SetResult( pResult );
	*/

	// ó���� ����� ���� ���
	if (pActionResult == NULL)
	{
		return false;
	}


	if (m_nUsedActionInfo < g_pActionInfoTable->GetMinResultActionInfo())
	{
		//------------------------------------------------------------
		// �������� Effect�� EffectTarget�� ���� ��쿡
		//------------------------------------------------------------
		if (m_listEffectTarget.size() != 0)
		{
			// ������ EffectTarget�� ����� Ȯ���غ���..
			MEffectTarget* pEffectTarget = m_listEffectTarget.back();

			// �̹� ����� �ִ� �����..
			MActionResult* pResult = pEffectTarget->GetResult();

			if (pResult != NULL)
			{
				pEffectTarget->SetResultNULL();
			}

			// ���ο� ����� �������ش�.
			pEffectTarget->SetResult(pActionResult);

			if (pResult != NULL)
			{
				// ������ ����� ������� ������.
				pResult->Execute();

				delete pResult;
			}

			return true;

			/*
			EFFECTTARGET_LIST::iterator iEffectTarget = m_listEffectTarget.begin();

			while (iEffectTarget != m_listEffectTarget.end())
			{
				//------------------------------------------------------------
				// id�� ���� ���
				//------------------------------------------------------------
				// EffectID�� üũ�ؾ�������,
				// ����(!)�� .. ��� �ϳ� ���� �ϳ� �����ޱ� ������
				// EffectID�� üũ ���ص� �ȴ�.
				//------------------------------------------------------------
				// ������ �̹� �ൿ�� ���缭 ����� ǥ������ �����̹Ƿ�
				// ����� �־�д�.
				//if ((*iEffectTarget)->GetEffectID() == effectID)
				{
					// EffectTarget�� ����� �߰���Ų��.
					// �̹� ����� �ִ� ���..(?)
					// �� ���� ����� ������ѹ�����.
					if ((*iEffectTarget)->IsExistResult())
					{
						(*iEffectTarget)->GetResult()->Execute();
						//break; // ������.. - -;
					}

					// ���ο�� set�ϸ� �������� delete�ȴ�.
					(*iEffectTarget)->SetResult( pActionResult );

					// ����� ���� �� ���
					return true;
				}

				iEffectTarget++;
			}
			*/
		}
	}

	//------------------------------------------------------------
	// �������� Effect�� EffectTarget�� ���� ��쿡
	//------------------------------------------------------------			
	// ����� �ٷ� �����ϰ�..
	// �޸𸮿��� �����.
	// (!) ��� ������ ���� Manager class�� �ʿ��ϴ�.
	pActionResult->Execute();

	delete pActionResult;

	return false;
}

//----------------------------------------------------------------------
// On Attacking
//----------------------------------------------------------------------
// �ݺ� �ൿ �߿� �����ϴ� ���..
//----------------------------------------------------------------------
bool
MPlayer::OnAttacking() const
{
	return m_bRepeatAction && m_bTraceCreatureToForceAttack;
}

//----------------------------------------------------------------------
// Basic Action To Creature
//----------------------------------------------------------------------
// ������ �Ϸ��� ���.. Creature���� �⺻action�� ���Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::BasicActionToCreature()
{
	if (IsInDarkness())
	{
		return;
	}

	MCreature* pCreature = g_pZone->GetCreature(m_TraceID);

	if (pCreature == NULL)// || pCreature->IsDead())
	{
		return;
	}
	if (pCreature->CurPernalShop() == 1) // by csm 12/4
		return;

	//---------------------------------------------------------------
	// ����� NPC�� ���..
	//---------------------------------------------------------------
	if (pCreature->IsNPC())
	{
		// ���㳪 ���밡 �ƴ� ���.. �ŷ� �����ϴ�.
		if (//m_CreatureType!=CREATURETYPE_BAT
			//&& m_CreatureType!=CREATURETYPE_WOLF)
			(*g_pCreatureSpriteTable)[(*g_pCreatureTable)[m_CreatureType].SpriteTypes[0]].IsPlayerOnlySprite())
		{
			if (pCreature->GetCreatureType() == 659)	// ���������� ���̸� UI����
				UI_RunHorn(g_pZone->GetID());
			else
			{
#if __CONTENTS(__MENEGROTH)
				if (pCreature->HasEffectStatus(EFFECTSTATUS_MENEGROTH_CABRACAM_ALTAR))
				{
					int creatureType = pCreature->GetCreatureType();
					if (creatureType > 948 && creatureType < 1021)
					{
					}
				}
				else
#endif //__MENEGROTH
					if (IsWaitVerifyNULL())
					{
						// NPC ó�� packet�� ������.
						CGNPCTalk _CGNPCTalk;
						_CGNPCTalk.setObjectID(m_TraceID);

						g_pSocket->sendPacket(&_CGNPCTalk);

						SetWaitVerify(WAIT_VERIFY_NPC_ASK);
					}
			}
		}

		// �ʿ��Ϸ���.. - -;
		m_bTraceCreatureToForceAttack = false;
		m_bKeepTraceCreature = false;

		// �����Ѵ�.
		SetNextAction(ACTION_STAND);
		m_nUsedActionInfo = ACTIONINFO_NULL;
		m_nNextUsedActionInfo = ACTIONINFO_NULL;
		m_fNextTrace = FLAG_TRACE_NULL;
		UnSetRepeatAction();
	}
	//---------------------------------------------------------------
	// Request�ҷ��� ���
	//---------------------------------------------------------------
	else if (IsRequestMode())
	{
		switch (m_RequestMode)
		{
			//---------------------------------------------------------------
			// Trade �ҷ��� ���
			//---------------------------------------------------------------
		case REQUEST_TRADE:
		{
			if (IsWaitVerifyNULL()
				&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
				&& (IsSlayer() && pCreature->IsSlayer() ||
					IsVampire() && pCreature->IsVampire() ||
					IsOusters() && pCreature->IsOusters())
				)
			{
				// ��ȯ ��û
				CGTradePrepare _CGTradePrepare;
				_CGTradePrepare.setTargetObjectID(m_TraceID);
				_CGTradePrepare.setCode(CG_TRADE_PREPARE_CODE_REQUEST);

				g_pSocket->sendPacket(&_CGTradePrepare);

				SetWaitVerify(WAIT_VERIFY_TRADE);

				// temp information�� ����.. (��..�ణ �����ϴ�.- -;)
				g_pTempInformation->SetMode(TempInformation::MODE_TRADE_REQUEST);
				g_pTempInformation->Value1 = m_TraceID;

				//-----------------------------------------------------
				// ��ȯ ����ұ�?
				//-----------------------------------------------------
				UI_RunExchangeCancel(pCreature->GetName());
			}
		}
		break;

		//---------------------------------------------------------------
		// Party �ҷ��� ���
		//---------------------------------------------------------------
		case REQUEST_PARTY:
		{
			if (IsWaitVerifyNULL()
				&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
				&& g_pParty != NULL
				// �� ��Ƽ���� �ƴ� ��쿡�� ��û �����ϴ�.
				&& !g_pParty->HasMember(pCreature->GetName())
				&& (IsSlayer() && pCreature->IsSlayer() || IsVampire() && pCreature->IsVampire() || IsOusters() && pCreature->IsOusters())
				&& g_pSystemAvailableManager->IsAvailablePartySystem()
				)
			{
				// ��Ƽ ��û
				CGPartyInvite _CGPartyInvite;
				_CGPartyInvite.setTargetObjectID(m_TraceID);
				_CGPartyInvite.setCode(CG_PARTY_INVITE_REQUEST);

				g_pSocket->sendPacket(&_CGPartyInvite);

				SetWaitVerify(WAIT_VERIFY_PARTY);

				// temp information�� ����.. (��..�ణ �����ϴ�.- -;)
				g_pTempInformation->SetMode(TempInformation::MODE_PARTY_REQUEST);
				g_pTempInformation->Value1 = m_TraceID;

				//-----------------------------------------------------
				// ��ȯ ����ұ�?
				//-----------------------------------------------------
				UI_RunPartyCancel(pCreature->GetName());
			}
		}
		break;

		//---------------------------------------------------------------
		// OtherInfo ������ ���
		//---------------------------------------------------------------
		case REQUEST_INFO:
		{
			//				MFakeCreature *pFakeCreature = (MFakeCreature *)g_pZone->GetFakeCreature(m_TraceID);
			//				if(pFakeCreature != NULL && pFakeCreature->GetOwnerID() != OBJECTID_NULL)	// ���̴�
			//				{
			//					UI_RunPetInfo(pFakeCreature->GetPetItem());
			//				}
			//				else
			if (IsWaitVerifyNULL()
				&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
				&& (IsSlayer() && pCreature->IsSlayer() ||
					IsVampire() && pCreature->IsVampire() ||
					IsOusters() && pCreature->IsOusters() ||
					GetCreatureType() == CREATURETYPE_SLAYER_OPERATOR ||
					GetCreatureType() == CREATURETYPE_VAMPIRE_OPERATOR ||
					GetCreatureType() == CREATURETYPE_OUSTERS_OPERATOR ||
					GetCompetence() == 0)
				)
			{
				const char* str = gC_vs_ui.GetInputString();
				if (strlen(str) > 0)
				{
					if (str[0] == '*')
					{
						gC_vs_ui.AddInputString(pCreature->GetName());
						break;

					}
				}
				// ���� ��û
				CGRequestInfo _CGRequestInfo;
				_CGRequestInfo.setValue(m_TraceID);
				_CGRequestInfo.setCode(CGRequestInfo::REQUEST_CHARACTER_INFO);

				g_pSocket->sendPacket(&_CGRequestInfo);

				MCreature* pCreature = g_pZone->GetCreature(m_TraceID);

				if (pCreature != NULL)
				{
					UI_RunOtherInfo(pCreature);

					if (g_pProfileManager != NULL
						&& g_pRequestUserManager != NULL
						&& g_pRequestClientPlayerManager != NULL)
						//&& !g_pProfileManager->HasProfile(pName)
						//&& !g_pProfileManager->HasProfileNULL(pName)
						//&& !g_pRequestUserManager->HasRequestingUser(pName)
						//&& !g_pRequestClientPlayerManager->HasConnection(pName)
						//&& !g_pRequestClientPlayerManager->HasTryingConnection(pName))
					{
						g_pProfileManager->RequestProfile(pCreature->GetName());
					}
				}


			}
		}
		break;

		}

		// �ʿ��Ϸ���.. - -;
		m_bTraceCreatureToForceAttack = false;
		m_bKeepTraceCreature = false;

		// �����Ѵ�.
		SetNextAction(ACTION_STAND);
		m_nUsedActionInfo = ACTIONINFO_NULL;
		m_nNextUsedActionInfo = ACTIONINFO_NULL;
		m_fNextTrace = FLAG_TRACE_NULL;
		UnSetRepeatAction();

		UnSetRequestMode();
	}
	//---------------------------------------------------------------
	// �ƴϸ�.. �����̰ų� ����..
	//---------------------------------------------------------------
	else
	{
		//---------------------------------------------------------------
		// ���� attack�̰ų�..
		// ��������� �Ǵ� ĳ������ ������ ���� ���������� �����Ѵ�.
		//---------------------------------------------------------------
		bool bOusters = IsOusters();
		MItem* pOustersItem = NULL;
		if (bOusters)
			pOustersItem = g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND);

		if (m_bTraceCreatureToForceAttack
			&& (bOusters && pOustersItem != NULL && pOustersItem->GetItemClass() == ITEM_CLASS_OUSTERS_CHAKRAM
				|| !bOusters)
			)
		{
			SetNextAction(GetActionInfoAction(m_nBasicActionInfo));

			//--------------------
			// message
			//--------------------
			DEBUG_ADD_FORMAT("Basic Action : %d", m_TraceID);

			// 
			//m_bTraceCreatureToForceAttack = false;
		}
		else
		{
			DEBUG_ADD_FORMAT("Basic Action : %d - But don't attack", m_TraceID);

			// ���� ���Ѵ�.
			m_nUsedActionInfo = ACTIONINFO_NULL;

			//---------------------------------
			// ��� �����ؾ� �ϸ�..
			//---------------------------------
			if (m_bKeepTraceCreature)
			{
				return;
			}
		}
	}
	/*
	#ifdef	CONNECT_SERVER
		CGAttackNormal _CGAttackNormal;
		_CGAttackNormal.setX( m_X );
		_CGAttackNormal.setY( m_Y );
		_CGAttackNormal.setDir( m_Direction );
		_CGAttackNormal.setObjectID( m_TraceID );
		g_pSocket->sendPacket( &_CGAttackNormal );

		#ifdef	OUTPUT_DEBUG
			DEBUG_ADD( _CGAttackNormal.toString().c_str() );
		#endif
	#endif
	*/
}
//void
//MPlayer::BasicActionToCreature()
//{
//	if (IsInDarkness())
//	{
//		return;
//	}
//
//	MCreature* pCreature = g_pZone->GetCreature( m_TraceID );
//
//	if (pCreature==NULL)// || pCreature->IsDead())
//	{
//		return;
//	}
//
//	//---------------------------------------------------------------
//	// ����� NPC�� ���..
//	//---------------------------------------------------------------
//	if (pCreature->IsNPC())
//	{
//		// ���㳪 ���밡 �ƴ� ���.. �ŷ� �����ϴ�.
//		if (//m_CreatureType!=CREATURETYPE_BAT
//			//&& m_CreatureType!=CREATURETYPE_WOLF)
//			(*g_pCreatureSpriteTable)[(*g_pCreatureTable)[m_CreatureType].SpriteType].IsPlayerOnlySprite())
//		{		
//			if(pCreature->GetCreatureType() == 659)	// ���������� ���̸� UI����
//				UI_RunHorn(g_pZone->GetID());
//			else
//				
//				if (IsWaitVerifyNULL())
//				{
//					// NPC ó�� packet�� ������.
//					CGNPCTalk _CGNPCTalk;
//					_CGNPCTalk.setObjectID( m_TraceID );
//
//					g_pSocket->sendPacket( &_CGNPCTalk );
//
//					SetWaitVerify( WAIT_VERIFY_NPC_ASK );
//				}
//		}
//
//		// �ʿ��Ϸ���.. - -;
//		m_bTraceCreatureToForceAttack = false;
//		m_bKeepTraceCreature = false;
//
//		// �����Ѵ�.
//		SetNextAction(ACTION_STAND);
//		m_nUsedActionInfo = ACTIONINFO_NULL;
//		m_nNextUsedActionInfo = ACTIONINFO_NULL;
//		m_fNextTrace = FLAG_TRACE_NULL;
//		UnSetRepeatAction();
//	}
//	//---------------------------------------------------------------
//	// Request�ҷ��� ���
//	//---------------------------------------------------------------
//	else if (IsRequestMode())
//	{
//		switch (m_RequestMode)
//		{
//			//---------------------------------------------------------------
//			// Trade �ҷ��� ���
//			//---------------------------------------------------------------
//			case REQUEST_TRADE :
//			{
//				if (IsWaitVerifyNULL() 
//					&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
//					&& (IsSlayer() && pCreature->IsSlayer() || 
//					IsVampire() && pCreature->IsVampire() || 
//					IsOusters() && pCreature->IsOusters())
//					)
//				{
//						// ��ȯ ��û
//						CGTradePrepare _CGTradePrepare;
//						_CGTradePrepare.setTargetObjectID( m_TraceID );
//						_CGTradePrepare.setCode( CG_TRADE_PREPARE_CODE_REQUEST );
//
//						g_pSocket->sendPacket( &_CGTradePrepare );
//
//					SetWaitVerify( WAIT_VERIFY_TRADE );
//					
//					// temp information�� ����.. (��..�ణ �����ϴ�.- -;)
//					g_pTempInformation->SetMode(TempInformation::MODE_TRADE_REQUEST);
//					g_pTempInformation->Value1 = m_TraceID;
//
//					//-----------------------------------------------------
//					// ��ȯ ����ұ�?
//					//-----------------------------------------------------
//					UI_RunExchangeCancel( pCreature->GetName() );
//				}				
//			}
//			break;
//
//			//---------------------------------------------------------------
//			// Party �ҷ��� ���
//			//---------------------------------------------------------------
//			case REQUEST_PARTY :
//			{
//				if (IsWaitVerifyNULL() 
//					&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
//					&& g_pParty!=NULL
//					// �� ��Ƽ���� �ƴ� ��쿡�� ��û �����ϴ�.
//					&& !g_pParty->HasMember( pCreature->GetName() )
//					&& (IsSlayer() && pCreature->IsSlayer() || IsVampire() && pCreature->IsVampire() || IsOusters() && pCreature->IsOusters())
//					&& g_pSystemAvailableManager->IsAvailablePartySystem()
//					)
//				{
//						// ��Ƽ ��û
//						CGPartyInvite _CGPartyInvite;
//						_CGPartyInvite.setTargetObjectID( m_TraceID );
//						_CGPartyInvite.setCode( CG_PARTY_INVITE_REQUEST );
//
//						g_pSocket->sendPacket( &_CGPartyInvite );
//
//					SetWaitVerify( WAIT_VERIFY_PARTY );
//
//					// temp information�� ����.. (��..�ణ �����ϴ�.- -;)
//					g_pTempInformation->SetMode(TempInformation::MODE_PARTY_REQUEST);
//					g_pTempInformation->Value1 = m_TraceID;
//
//					//-----------------------------------------------------
//					// ��ȯ ����ұ�?
//					//-----------------------------------------------------
//					UI_RunPartyCancel( pCreature->GetName() );
//				}				
//			}
//			break;
//
//			//---------------------------------------------------------------
//			// OtherInfo ������ ���
//			//---------------------------------------------------------------
//			case REQUEST_INFO:
//			{
////				MFakeCreature *pFakeCreature = (MFakeCreature *)g_pZone->GetFakeCreature(m_TraceID);
////				if(pFakeCreature != NULL && pFakeCreature->GetOwnerID() != OBJECTID_NULL)	// ���̴�
////				{
////					UI_RunPetInfo(pFakeCreature->GetPetItem());
////				}
////				else
//				if (IsWaitVerifyNULL() 
//					&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
//					&& (IsSlayer() && pCreature->IsSlayer() || 
//					IsVampire() && pCreature->IsVampire() || 
//					IsOusters() && pCreature->IsOusters() ||
//					GetCreatureType() == CREATURETYPE_SLAYER_OPERATOR ||
//					GetCreatureType() == CREATURETYPE_VAMPIRE_OPERATOR ||
//					GetCreatureType() == CREATURETYPE_OUSTERS_OPERATOR ||
//					GetCompetence() == 0 )
//					)
//				{
//						const char* str = gC_vs_ui.GetInputString();
//						if(strlen(str)>0)
//						{
//							if(str[0] == '*')
//							{
//								gC_vs_ui.AddInputString(pCreature->GetName());
//								break;
//
//							}
//						}
//						// ���� ��û
//						CGRequestInfo _CGRequestInfo;
//						_CGRequestInfo.setValue( m_TraceID );
//						_CGRequestInfo.setCode( CGRequestInfo::REQUEST_CHARACTER_INFO );
//
//						g_pSocket->sendPacket( &_CGRequestInfo );
//
//						MCreature *pCreature = g_pZone->GetCreature(m_TraceID);
//						
//						if(pCreature != NULL)
//						{
//							UI_RunOtherInfo(pCreature);
//
//							if (g_pProfileManager!=NULL 
//								&& g_pRequestUserManager!=NULL
//								&& g_pRequestClientPlayerManager!=NULL)
//								//&& !g_pProfileManager->HasProfile(pName)
//								//&& !g_pProfileManager->HasProfileNULL(pName)
//								//&& !g_pRequestUserManager->HasRequestingUser(pName)
//								//&& !g_pRequestClientPlayerManager->HasConnection(pName)
//								//&& !g_pRequestClientPlayerManager->HasTryingConnection(pName))
//							{
//								g_pProfileManager->RequestProfile(pCreature->GetName());
//							}
//						}
//
//
//				}
//			}
//			break;
//	
//		}
//
//		// �ʿ��Ϸ���.. - -;
//		m_bTraceCreatureToForceAttack = false;
//		m_bKeepTraceCreature = false;
//
//		// �����Ѵ�.
//		SetNextAction(ACTION_STAND);
//		m_nUsedActionInfo = ACTIONINFO_NULL;
//		m_nNextUsedActionInfo = ACTIONINFO_NULL;
//		m_fNextTrace = FLAG_TRACE_NULL;
//		UnSetRepeatAction();
//
//		UnSetRequestMode();
//	}	
//	//---------------------------------------------------------------
//	// �ƴϸ�.. �����̰ų� ����..
//	//---------------------------------------------------------------
//	else
//	{
//		//---------------------------------------------------------------
//		// ���� attack�̰ų�..
//		// ��������� �Ǵ� ĳ������ ������ ���� ���������� �����Ѵ�.
//		//---------------------------------------------------------------
//		bool bOusters = IsOusters();
//		MItem *pOustersItem = NULL;
//		if( bOusters) 
//			pOustersItem = g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND);
//
//		if (m_bTraceCreatureToForceAttack 
//			&& (bOusters && pOustersItem != NULL && pOustersItem->GetItemClass() == ITEM_CLASS_OUSTERS_CHAKRAM
//				|| !bOusters)
//			)
//		{
//			SetNextAction(GetActionInfoAction(m_nBasicActionInfo));
//
//			//--------------------
//			// message
//			//--------------------
//			DEBUG_ADD_FORMAT("Basic Action : %d", m_TraceID);					
//			
//			// 
//			//m_bTraceCreatureToForceAttack = false;
//		}
//		else
//		{
//			DEBUG_ADD_FORMAT("Basic Action : %d - But don't attack", m_TraceID);					
//			
//			// ���� ���Ѵ�.
//			m_nUsedActionInfo = ACTIONINFO_NULL;
//			
//			//---------------------------------
//			// ��� �����ؾ� �ϸ�..
//			//---------------------------------
//			if (m_bKeepTraceCreature)
//			{
//				return;
//			}
//		}
//	}
//	/*
//	#ifdef	CONNECT_SERVER
//		CGAttackNormal _CGAttackNormal;
//		_CGAttackNormal.setX( m_X );
//		_CGAttackNormal.setY( m_Y );
//		_CGAttackNormal.setDir( m_Direction );
//		_CGAttackNormal.setObjectID( m_TraceID );
//		g_pSocket->sendPacket( &_CGAttackNormal );
//
//		#ifdef	OUTPUT_DEBUG
//			DEBUG_ADD( _CGAttackNormal.toString().c_str() );							
//		#endif
//	#endif
//	*/	
//}

//----------------------------------------------------------------------
// Pickup Item
//----------------------------------------------------------------------
// Item�ݱ⸦ �����Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::PickupItem(MItem* pItem)
{
	//----------------------------------------------------
	// ���� ���� �������� ����..
	// ���� ��� �ִ� Item�� ���� ���
	//----------------------------------------------------

	if (HasEffectStatusSummonSylph(dynamic_cast<MCreature*>(this))
#if __CONTENTS(__FAST_TRANSFORTER)
		|| HasEffectStatusWingSylph(dynamic_cast<MCreature*>(this))
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
		|| HasEffectStatusUnicorn(dynamic_cast<MCreature*>(this))
#endif //__SECOND_TRANSFORTER
		)
		return;

	if (IsItemCheckBufferNULL()
		&& gpC_mouse_pointer->GetPickUpItem() == NULL)
	{
		//----------------------------------------------------------------
		// ��ġ ������ �� ����
		//----------------------------------------------------------------
		//if (m_SendMove==0)
		{
			// [ TEST CODE ]
			// Item ������ �о�ͼ� Item�ݱ��Ҷ� Sound�� ����.
			// �� �κ��� ���߿�.. 
			// Server Packet���� Ȯ���� "�ֿ���"��� �� ��
			// �ϴ� ���� ���� �� ���⵵ �ϴ�.
			//MItem*	pItem = m_pZone->GetItem( m_TraceID );

			//g_Sound.Play( g_SoundTable[ g_ItemTable[pItem->GetItemType()].PickupSoundID ].pDSBuffer );
			//PlaySound( g_ItemTable[pItem->GetItemClass()][pItem->GetItemType()].TileSoundID ,
			//			false,
			//			m_X, m_Y);

			// item class ������ ����..
			// item class���ٰ� virtual pickup()�� �δ°� ���� ������.
			// �������.. -_-;;

			//----------------------------------------------------------------
			//
			//	 Motorcycle�� ��� --> Ÿ��
			//
			//----------------------------------------------------------------
			if (pItem->GetItemClass() == ITEM_CLASS_MOTORCYCLE)
			{
				if (IsSlayer()
					&& m_CreatureType != CREATURETYPE_SLAYER_OPERATOR)
				{
					RideMotorcycle((MMotorcycle*)pItem);
				}
			}

			//----------------------------------------------------------------
			//
			// ��ü�� ��� --> Looting
			//
			//----------------------------------------------------------------
			else if (pItem->GetItemClass() == ITEM_CLASS_CORPSE)
			{
				//----------------------------------------------------
				// Server�� ������ ���� ��,
				//----------------------------------------------------
				MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();

				if (pCreature != NULL &&
					(
						pCreature->GetActionCount() >= pCreature->GetActionCountMax() ||
						pCreature->GetCreatureType() == 672
						)
					)
				{
					if (pItem->GetNumber() > 0 ||
						pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376 ||
						pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563 ||
						pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549 ||
						pCreature->GetCreatureType() == 670 ||			// ����
						pCreature->GetCreatureType() == 672 ||			// ������ �������̸�
						pCreature->GetCreatureType() == 673
						)
					{
						// ���㰡 �ƴ� ��쿡�� ������.
						// ������̸� Ÿ�� ���� ���� ������ ��쿡��
						// ��ȭ �Ȱɷ�����
						if (m_CreatureType != CREATURETYPE_BAT
							&& m_CreatureType != CREATURETYPE_VAMPIRE_GHOST
#if __CONTENTS(__FAST_TRANSFORTER)
							&& m_CreatureType != CREATURETYPE_FLITTERMOUSE
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
							&& m_CreatureType != CREATURETYPE_SHAPE_OF_DEMON
#endif //__SECOND_TRANSFORTER
							&& m_MoveDevice == MOVE_DEVICE_WALK
							&& !m_bEffectStatus[EFFECTSTATUS_CURSE_PARALYSIS])
						{
							CGDissectionCorpse _CGDissectionCorpse;
							_CGDissectionCorpse.setX(pItem->GetX());
							_CGDissectionCorpse.setY(pItem->GetY());
							_CGDissectionCorpse.setObjectID(pItem->GetID());

							g_pSocket->sendPacket(&_CGDissectionCorpse);

							if (pItem->GetNumber() > 0)
								pItem->SetNumber(pItem->GetNumber() - 1);
						}
					}
					// item�� ���� ��쿣..
					else
					{
						// ������ ��ü �Ա�
						// ������ �ƴ� ��쿡��, ��ȣ������ �ƴ� ��쿡��.
						if (m_CreatureType == CREATURETYPE_WOLF &&
							!(
								pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376 ||
								pCreature->GetCreatureType() == 670 ||			// ����
								pCreature->GetCreatureType() == 672 ||			// ������ �������̸�
								pCreature->GetCreatureType() == 673 ||
								pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549 ||
								pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563
								)
							)
						{
							// ����� ����� ����
							m_nNextUsedActionInfo = MAGIC_EAT_CORPSE;

							// ���� ���� ����
							m_fTraceBuffer = m_fTrace = FLAG_TRACE_SECTOR_SPECIAL;
							m_TraceX = pItem->GetX();
							m_TraceY = pItem->GetY();
							m_TraceZ = 0;

							m_TraceDistance = GetActionInfoRange(m_nNextUsedActionInfo);

							SetAction(m_MoveAction);
							SetNextDestination(m_TraceX, m_TraceY);

							m_fNextTrace = FLAG_TRACE_NULL;

							m_bKeepTraceCreature = false;
						}
					}
				}
			}

			//----------------------------------------------------------------
			//
			// ���� ��� --> ������ inventory��..
			//
			//----------------------------------------------------------------
			else if (pItem->GetItemClass() == ITEM_CLASS_MONEY || pItem->GetItemClass() == ITEM_CLASS_CHECK_MONEY)
			{
				PickupMoney((MMoney*)pItem);
			}

			else //if (m_CreatureType!=CREATURETYPE_BAT || m_MoveDevice!=MOVE_DEVICE_WALK)
			{
				//----------------------------------------------------------------
				// ��ġ�� ������ ���� ���� �� ����.
				//----------------------------------------------------------------
				if (pItem->GetItemClass() == ITEM_CLASS_MINE)
				{
					if (((MMine*)pItem)->IsInstalled())
					{
						return;
					}
				}
				//----------------------------------------------------------------
				//
				//	 Event �������� ���
				//
				//----------------------------------------------------------------
//				else if (pItem->GetItemClass()==ITEM_CLASS_EVENT_GIFT_BOX)
//				{
//					// ���� type�� �������� �ִ��� üũ�Ѵ�.
//					if (NULL != g_pInventory->FindItem( ITEM_CLASS_EVENT_GIFT_BOX, pItem->GetItemType() ))
//					{
//						// �ִٸ� �� �ݴ´�.
//						return;
//					}
//				}

				//----------------------------------------------------------------
				//
				//	Gear�� �� �� �ִ� Item�� ���..
				//
				//----------------------------------------------------------------
				if (pItem->IsGearItem())
				{
					//------------------------------------------
					// gear�� Inventory�� �����ִٸ� --> mouse��
					//------------------------------------------
					if (gC_vs_ui.GetGearOpenState() ||
						gC_vs_ui.GetInventoryOpenState())
					{
						PickupItemToMouse(pItem);
					}
					//------------------------------------------
					// �ƴϸ� 
					//------------------------------------------
					else
					{
						// gear�� �־�� �� �� ���ٸ�
						// inventory�� �־��.

						// �� �� �� ���� �� ���´�.

						// �ϴ���.. --;
						PickupItemToInventory(pItem);
					}
				}
				//----------------------------------------------------------------
				//
				//	Inventory�� �� �� �ִ� Item�� ���..
				//
				//----------------------------------------------------------------
				else if (pItem->IsInventoryItem())
				{
					//----------------------------------------------------------------
					// inventory�� �����ִٸ� --> mouse��
					//----------------------------------------------------------------
					if (gC_vs_ui.GetInventoryOpenState())
					{
						PickupItemToMouse(pItem);
					}
					//----------------------------------------------------------------
					// �ƴϸ� �׳� inventory��
					//----------------------------------------------------------------
					else
					{
						if (pItem->IsQuickItem()
							&& PickupItemToQuickslot(pItem))
						{
							// quickslot�� �� �� �־ ���� �Ǵ� ����.
							// �ɳ�..
						}
						else
						{
							PickupItemToInventory(pItem);
						}
					}
				}
			}
		}
		//----------------------------------------------------------------
		// ��ġ ������ �ȵ� ����
		//----------------------------------------------------------------
		//DEBUG_ADD_FORMAT("Cannot Pickup item (Position not verified) sendMove=%d", m_SendMove);		
	}
#ifdef OUTPUT_DEBUG
	else
	{
		if (gpC_mouse_pointer->GetPickUpItem() != NULL)
		{
			DEBUG_ADD_FORMAT("[UI] Already Handle a Item id=%d", gpC_mouse_pointer->GetPickUpItem()->GetID());
		}
	}
#endif

}

//----------------------------------------------------------------------
// Pickup Item to Inventory
//----------------------------------------------------------------------
// Zone�� Item�� �ֿ��� inventory�� �ִ´�.
//----------------------------------------------------------------------
void
MPlayer::PickupItemToInventory(MItem* pItem)
{
	//------------------------------------------
	// Inventory�� �� �ڸ��� �ִ� ���
	// : Server���� �����ޱ� ���� packet�� ������.
	//------------------------------------------

	// sjheon 2005.05.03  ������ �� �������� ������� �������� ���� ���ϰ� �Ѵ�.Add
	if (pItem->GetItemClass() == ITEM_CLASS_EVENT_ITEM && (pItem->GetItemType() >= 32 && pItem->GetItemType() <= 36))
	{
		MItem* pItemCharm = NULL;
		for (int iItemType = 0; iItemType < 5; iItemType++)
		{
			pItemCharm = g_pInventory->FindItem(pItem->GetItemClass(), iItemType + 32);
			if (pItemCharm) return;
		}
	}
	// sjheon 2005.05.03  ������ �� �������� ������� �������� ���� ���ϰ� �Ѵ�.End
	else if (pItem->GetItemClass() == ITEM_CLASS_QUEST_ITEM && pItem->GetItemType() == 4) {
		MItem* pSubInven = NULL;
		MItemClassTypeFinder finder_quest_item1(pItem->GetItemClass(), 4);
		if (g_pInventory->FindItemAll(finder_quest_item1, pSubInven))
			return;
	}

	POINT fitPoint;
	//------------------------------------------
	// ������ ó���ҷ��� item�� ���� ���..
	//------------------------------------------
	if (IsItemCheckBufferNULL())
	{

		if (g_pInventory->GetFitPosition(pItem, fitPoint) &&
			(pItem->IsSlayerItem() && IsSlayer() ||
				pItem->IsVampireItem() && IsVampire() ||
				pItem->IsOustersItem() && IsOusters()))
		{
			CGAddZoneToInventory _CGAddZoneToInventory;

			_CGAddZoneToInventory.setObjectID(pItem->GetID());
			_CGAddZoneToInventory.setZoneX(pItem->GetX());
			_CGAddZoneToInventory.setZoneY(pItem->GetY());
			_CGAddZoneToInventory.setInvenX(fitPoint.x);
			_CGAddZoneToInventory.setInvenY(fitPoint.y);

			g_pSocket->sendPacket(&_CGAddZoneToInventory);

			// �ֿ���� item�� ����Ѵ�.
			pItem->SetGridXY(fitPoint.x, fitPoint.y);
			SetItemCheckBuffer(pItem, ITEM_CHECK_BUFFER_PICKUP_TO_INVENTORY);
		}
		else
		{
			//------------------------------------------------------------
			// inventory�� ������ ������ ��� �ֿ� �� ���� ����̴�.				
			//------------------------------------------------------------
			pItem->SetDropping();

			// 2004, 5, 7 , sobeit add start - �κ��� �ڸ��� ������ ������ ���� ���� ������
			ExecuteHelpEvent(HELP_EVENT_STORAGE_BUY);
			// 2004, 5, 6, sobeit add end
		}
	}
	//------------------------------------------
	// ���� �ٸ� item�� ó�� ���̱� ������
	// item�� ���� ���ϴ� ����̴�.
	//------------------------------------------
	else
	{
		// message���
		// "���� item�� �ֿ� �� �����ϴ�."
	}
}

//----------------------------------------------------------------------
// Pickup Money
//----------------------------------------------------------------------
// Zone�� Item�� �ݴ´�.
//----------------------------------------------------------------------
void
MPlayer::PickupMoney(MMoney* pItem)
{
	//------------------------------------------
	// ������ ó���ҷ��� item�� ���� ���..
	//------------------------------------------
	if (IsItemCheckBufferNULL())
	{
		CGPickupMoney _CGPickupMoney;

		_CGPickupMoney.setObjectID(pItem->GetID());
		_CGPickupMoney.setZoneX(pItem->GetX());
		_CGPickupMoney.setZoneY(pItem->GetY());

		g_pSocket->sendPacket(&_CGPickupMoney);

		// �ֿ���� item�� ����Ѵ�.
		SetItemCheckBuffer(pItem, ITEM_CHECK_BUFFER_PICKUP_MONEY);

	}
	//------------------------------------------
	// ���� �ٸ� item�� ó�� ���̱� ������
	// item�� ���� ���ϴ� ����̴�.
	//------------------------------------------
	else
	{
		// message���
		// "���� item�� �ֿ� �� �����ϴ�."
	}
}

//----------------------------------------------------------------------
// Pickup Item to Mouse
//----------------------------------------------------------------------
// Zone�� Item�� �ֿ��� Mouse�� ���δ�. - -
//----------------------------------------------------------------------
void
MPlayer::PickupItemToMouse(MItem* pItem)
{
	//------------------------------------------
	// Server�� ���ӵ� ����..
	//------------------------------------------
	// item�� �ֿ� �� �ִ��� server���� 
	// ������ �޾ƾ� �Ѵ�.
	//------------------------------------------

	// sjheon 2005.05.03  ������ �� �������� ������� �������� ���� ���ϰ� �Ѵ�.Add
	if (pItem->GetItemClass() == ITEM_CLASS_EVENT_ITEM && (pItem->GetItemType() >= 32 && pItem->GetItemType() <= 36))
	{
		MItem* pItemCharm = NULL;
		for (int iItemType = 0; iItemType < 5; iItemType++)
		{
			pItemCharm = g_pInventory->FindItem(pItem->GetItemClass(), iItemType + 32);
			if (pItemCharm) return;
		}
	}
	// sjheon 2005.05.03  ������ �� �������� ������� �������� ���� ���ϰ� �Ѵ�.End 


	// �� �����۵� �Ѱ� �̻� �� �ݰ� �Ѵ�.
	if (pItem->GetItemClass() == ITEM_CLASS_QUEST_ITEM && pItem->GetItemType() == 4) {
		MItem* pSubInven = NULL;
		MItemClassTypeFinder finder_quest_item2(pItem->GetItemClass(), 4);
		if (g_pInventory->FindItemAll(finder_quest_item2, pSubInven))
			return;
	}

	//*
	// chyaya
	if (pItem->GetItemClass() == ITEM_CLASS_COMMON_QUEST_ITEM) {
		MItem* pSubInven = NULL;
		POINT FitPoint;
		if (false == GetMakeItemFitPosition(
			pItem, pItem->GetItemClass(), pItem->GetItemType(), FitPoint))
			return;
	}
	/**/

	//------------------------------------------
	// ������ ó���ҷ��� item�� ���� ���..
	//------------------------------------------
	if (IsItemCheckBufferNULL())
	{


		if (pItem->IsSlayerItem() && IsSlayer() ||
			pItem->IsVampireItem() && IsVampire() ||
			pItem->IsOustersItem() && IsOusters())
		{
			CGAddZoneToMouse _CGAddZoneToMouse;

			_CGAddZoneToMouse.setObjectID(pItem->GetID());
			_CGAddZoneToMouse.setZoneX(pItem->GetX());
			_CGAddZoneToMouse.setZoneY(pItem->GetY());

			g_pSocket->sendPacket(&_CGAddZoneToMouse);

			// �ֿ���� item�� ����Ѵ�.
			SetItemCheckBuffer(pItem, ITEM_CHECK_BUFFER_PICKUP_TO_MOUSE);
		}
	}
	//------------------------------------------
	// item�� ���� ���ϴ� ����̴�.
	//------------------------------------------
	else
	{
		// message���
		// "���� item�� �ֿ� �� �����ϴ�."
	}
}

//----------------------------------------------------------------------
// Pickup Item to Quickslot
//----------------------------------------------------------------------
// Zone�� Item�� �ֿ��� Mouse�� ���δ�. - -
//----------------------------------------------------------------------
bool
MPlayer::PickupItemToQuickslot(MItem* pItem)
{
	//------------------------------------------------------------------
	// �ϴ� QuickSlot�� �� �� �ִ����� Ȯ���ؾ� �Ѵ�.
	//------------------------------------------------------------------
	if ((g_pQuickSlot != NULL && IsSlayer()) || ((g_pArmsBand1 != NULL || g_pArmsBand2 != NULL) && IsOusters())
		&& IsItemCheckBufferNULL()
		&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
		&& (pItem->IsSlayerItem() && IsSlayer() ||
			pItem->IsVampireItem() && IsVampire() ||
			pItem->IsOustersItem() && IsOusters()))
	{
		BOOL FirstArmsband = TRUE;
		int slot;

		//---------------------------------------------------------
		// QuickSlot�� ��� �� �� ������?
		//---------------------------------------------------------
		if (IsSlayer() && !g_pQuickSlot->FindSlotToAddItem(pItem, slot))
		{
			return false;
		}
		else if (IsOusters())
		{
			// ù��°������ ������ ������ �ι�°�� ������.

			if (g_pArmsBand1 == NULL || !g_pArmsBand1->FindSlotToAddItem(pItem, slot))
			{
				if (g_pArmsBand2 == NULL || !g_pArmsBand2->FindSlotToAddItem(pItem, slot))
					return false;
				else FirstArmsband = FALSE;
			}
			else
			{
				FirstArmsband = TRUE;
			}
		}

		//---------------------------------------------------------
		// �ϴ� ���콺�� �� �� �ִ°� �����޾ƾ� �Ѵ�...
		//---------------------------------------------------------
		CGAddZoneToMouse _CGAddZoneToMouse;

		_CGAddZoneToMouse.setObjectID(pItem->GetID());
		_CGAddZoneToMouse.setZoneX(pItem->GetX());
		_CGAddZoneToMouse.setZoneY(pItem->GetY());

		g_pSocket->sendPacket(&_CGAddZoneToMouse);

		//---------------------------------------------------------
		// slot�� �� �� �ִ�.
		//---------------------------------------------------------
		g_pTempInformation->SetMode(TempInformation::MODE_TRADE_VERIFY_PICKUP_TO_QUICKSLOT);
		g_pTempInformation->Value1 = FirstArmsband ? slot : slot + 3;
		g_pTempInformation->pValue = (void*)pItem;

		SetItemCheckBuffer(pItem, ITEM_CHECK_BUFFER_PICKUP_TO_QUICKSLOT);


		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Pickup Item to Mouse
//----------------------------------------------------------------------
// Zone�� Item�� �ֿ��� Mouse�� ���δ�. - -
//----------------------------------------------------------------------
void
MPlayer::RideMotorcycle(MMotorcycle* pMotorcycle)
{
	if (pMotorcycle == NULL)
	{
		DEBUG_ADD("[Error] Player::Motorcycle to ride is NULL");

		return;
	}

	//------------------------------------------
	// �Ȱ� �ִ� ���� ��� ������̿� Ż �� �ִ�.
	//------------------------------------------
	if (m_MoveDevice == MOVE_DEVICE_WALK)
	{
		CGRideMotorCycle _CGRideMotorCycle;

		_CGRideMotorCycle.setObjectID(pMotorcycle->GetID());
		_CGRideMotorCycle.setX(pMotorcycle->GetX());
		_CGRideMotorCycle.setY(pMotorcycle->GetY());

		g_pSocket->sendPacket(&_CGRideMotorCycle);

		// Motorcycle�� Ÿ�� ���� �����ޱ⸦ ��ٸ���.
		SetWaitVerify(WAIT_VERIFY_MOTORCYCLE_GETON);
	}
	//------------------------------------------
	// �̹� ������̸� Ÿ�� �ִ� ����̴�.
	//------------------------------------------
	else
	{
		// ������� Ż �� ����. 
	}
}

//----------------------------------------------------------------------
// Skill To Inventory Item
//----------------------------------------------------------------------
// Inventory�� �ִ� item�� ����� ����Ѵ�.
// ���⼭�� casting���۸� �����ϰ� �ϸ� �ȴ�.
//
// ����� packet�� �޾Ƽ� ó��..
// ����� (global) AddNewInventoryEffect(...)���� �������� �ϸ� �ȴ�.
//----------------------------------------------------------------------
bool
MPlayer::TraceInventoryItem(TYPE_OBJECTID id)
{
	//-----------------------------------------------------
	// delay�� ������ �ȵȴ�.
	//-----------------------------------------------------
	if (!IsNotDelay()
		|| HasEffectStatus(EFFECTSTATUS_ETERNITY_PAUSE))
	{
		return false;
	}

	//--------------------------------------------------------
	// ����� �� ���� ���
	//--------------------------------------------------------
	if (m_nSpecialActionInfo == ACTIONINFO_NULL
		// ������̸� Ÿ�� ������ Ư�������� �ȵȴ�.
		|| m_MoveDevice == MOVE_DEVICE_RIDE
		// ���� ��� ����� ���� �޾ƾ� �ϸ� ����� �� ����.
		//|| m_WaitVerify != WAIT_VERIFY_NULL)	// 2001.8.20 �ּ�ó��
		)
	{
		return false;
	}

	//--------------------------------------------------------
	// �ٸ� ����� ������� ���
	//--------------------------------------------------------
	if (m_nUsedActionInfo != ACTIONINFO_NULL)
		return false;

	// 2006.08.22 sjheon
	if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WOLF))
	{
		if (!(m_nSpecialActionInfo == MAGIC_HOWL || m_nSpecialActionInfo == MAGIC_EAT_CORPSE || m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_BAT))
	{
		if (!(m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}
	else if (HasEffectStatus(EFFECTSTATUS_TRANSFORM_TO_WERWOLF))
	{
		if (!(m_nSpecialActionInfo == SKILL_BITE_OF_DEATH ||
			m_nSpecialActionInfo == MAGIC_RAPID_GLIDING ||
			m_nSpecialActionInfo == SKILL_VIOLENT_PHANTOM ||
			m_nSpecialActionInfo == SKILL_WILD_WOLF ||
			m_nSpecialActionInfo == MAGIC_UN_TRANSFORM))
			return false;
	}

	//--------------------------------------------------------
	// ���� ����� item�� ����ϴ°� �³�?
	//--------------------------------------------------------
	if ((*g_pActionInfoTable)[m_nSpecialActionInfo].IsTargetItem())
	{
		//-------------------------------------------------------
		// ���� ����� �� �ִ� ������� üũ..
		// Passive ��ų�̸� ��� ���ϰ�..
		//-------------------------------------------------------
		if (!g_pSkillAvailable->IsEnableSkill((ACTIONINFO)m_nSpecialActionInfo)
			|| !(*g_pSkillInfoTable)[m_nSpecialActionInfo].IsEnable()
			|| !(*g_pSkillInfoTable)[m_nSpecialActionInfo].IsAvailableTime()
			|| (*g_pSkillInfoTable)[m_nSpecialActionInfo].IsPassive())
		{
			m_fNextTrace = FLAG_TRACE_NULL;

			UnSetRepeatAction();

			return false;
		}

		// ���� ���¿����� ����� �� �ִ�.
		if (IsStop() && IsItemCheckBufferNULL()) // || m_Action!=ACTION_ATTACK)
		{
			//--------------------------------------------------------
			// pItem�� inventory�� �ִ� item���� üũ�Ѵ�.
			//--------------------------------------------------------
			const MItem* pItem = ((MItemManager)(*g_pInventory)).GetItem(id);

			if (pItem == NULL)
			{
				return false;
			}

			// ����� ����� ����
			m_nNextUsedActionInfo = m_nSpecialActionInfo;

			// ���� ���� ����
			SetTraceID(id);
			m_fTrace = FLAG_TRACE_INVENTORY;
			m_TraceX = pItem->GetGridX();
			m_TraceY = pItem->GetGridY();
			m_TraceZ = 0;

			// ���� �����ϴ� ��� ���ؼ� ����صд�.
			m_fTraceBuffer = m_fTrace;


			//------------------------------------------------------------		
			// Inventory Item�� ���ؼ��� �ǹ� ����. - -;
			//------------------------------------------------------------	
			m_TraceDistance = 1;
			SetAction(m_MoveAction);
			SetNextDestination(m_X, m_Y);

			// buffering�� ���ش�.
			m_fNextTrace = FLAG_TRACE_NULL;

			// ��� �Ѿư��� ���� ����
			m_bKeepTraceCreature = false;

			//------------------------------------------------------------
			// message���
			//------------------------------------------------------------
			DEBUG_ADD_FORMAT("Trace Item: %d", id);
		}
		else
		{
			//------------------------------------------------------------
			// �ݺ� action�ϴ� ���� �ƴ� ��쿡 ���� ���� buffering
			//------------------------------------------------------------
			if (m_bRepeatAction)
			{
				m_fNextTrace = FLAG_TRACE_NULL;

				return false;
			}
			else
			{
				m_fNextTrace = FLAG_TRACE_INVENTORY;
				m_NextTraceID = id;

				m_bRepeatAction = FALSE;

				return false;
			}
		}

		return true;
	}

	// item�� ����ϴ� ����� �ƴ϶�� �ǹ�
	return false;
}

//----------------------------------------------------------------------
// Change To Slayer
//----------------------------------------------------------------------
// slayer�� �����ϴµ�..
// �̹� slayer�̸�.. return false
//----------------------------------------------------------------------
bool
MPlayer::ChangeToSlayer()
{
	if (MCreature::ChangeToSlayer())
	{
		m_ConversionDelayTime = 0;

		g_pCurrentMagazine = NULL;

		//-----------------------------------------------------
		// �����Ե� �����ش�.
		//-----------------------------------------------------
		if (g_pStorage != NULL)
		{
			delete g_pStorage;
			g_pStorage = NULL;
		}

		//-----------------------------------------------------
		// ��½
		//-----------------------------------------------------
		g_pTopView->SetFadeStart(1, 31, 6, 5, 5, 31);

		UI_ChangeInterfaceRace(RACE_SLAYER);

		SetWaitVerifyNULL();

		ResetSendMove();

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Change To Vampire
//----------------------------------------------------------------------
// vampire�� �����ϴµ�..
// �̹� vampire�̸�.. return false
//----------------------------------------------------------------------
bool
MPlayer::ChangeToVampire()
{
	if (MCreature::ChangeToVampire())
	{
		m_ConversionDelayTime = 0;

		g_pCurrentMagazine = NULL;


		//-----------------------------------------------------
		// �����Ե� �����ش�.
		//-----------------------------------------------------
		if (g_pStorage != NULL)
		{
			delete g_pStorage;
			g_pStorage = NULL;
		}

		g_pTopView->SetFadeStart(1, 31, 6, 31, 0, 0);

		UI_ChangeInterfaceRace(RACE_VAMPIRE);

		SetWaitVerifyNULL();

		ResetSendMove();

		DEBUG_ADD("MPlayer::Change to Vampire OK");

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Set Status
//----------------------------------------------------------------------
// ĳ������ ���°��� �ٲ۴�.
//----------------------------------------------------------------------
void
MPlayer::SetStatus(DWORD n, DWORD value)
{
	if (n >= MODIFY_MAX)
	{
		DEBUG_ADD_FORMAT("[Error] Modify Part is Wrong : part=%d, value=%d", n, value);

		return;
	}

	MODIFY_VALUE modifyValue;
	modifyValue.oldValue = m_Status[n];
	modifyValue.newValue = value;

	/* �Ϻ��� ���� -_- ���߿� �ʿ��ұ��
	if( GetWaitVerify() == WAIT_VERIFY_LOGOUT	&&
		n == MODIFY_CURRENT_HP					&&
		modifyValue.oldValue > modifyValue.newValue ) {

		g_pSystemMessage->Add("���� ���ݿ� ���� �α׾ƿ��� ��ҵǾ����ϴ�.");
		g_pEventManager->RemoveEvent(EVENTID_LOGOUT);
		g_pUserInformation->LogoutTime = 0;

		SetWaitVerify(WAIT_VERIFY_NULL);
	}
	*/

	// �ϴ� �� ����
	m_Status[n] = value;

	// ����� ���� ���õ� ó��
	g_pModifyStatusManager->Execute(n, (void*)&modifyValue);

	// �ٽ�.. - -;
	m_Status[n] = modifyValue.newValue;


	if (g_pUserInformation->attrOperator.GetValue() &&
		g_pOperatorOption->bShowModifyHP)
	{
		/////////////////////////////////////////////////////////////////////////
		// 2006.11.10 chyaya - �� ��ȭ�� ǥ��! (���߿� �����ؾ���)
		if (n == MODIFY_CURRENT_HP)
		{
			int modifyHP = modifyValue.newValue - modifyValue.oldValue;

			if (modifyHP != 0)
			{
				char szNameBuf[64];
				sprintf(szNameBuf, "%s(%d,%d)",
					g_pUserInformation->CharacterID.GetString(), GetX(), GetY());

				int curHP = GetHP();
				int maxHP = max(GetMAX_HP() - GetSilverDamage(), curHP);

				char szInfoBuf[128];
				sprintf(szInfoBuf, "MAX/CUR=%4d/%4d, %s=%d",
					curHP, maxHP, (modifyHP > 0) ? "HEAL" : "DAM", abs(modifyHP));

				if (curHP == 0)
					strcat(szInfoBuf, " (DIE)");
				else if (modifyHP > 0 && curHP - modifyHP == 0)
					strcat(szInfoBuf, " (CREATE)");

				DWORD color = (modifyHP > 0) ? RGB_GREEN : RGB_RED;

				UI_AddChatToHistory(szInfoBuf, szNameBuf, 6, color);
			}
		}
		//
		/////////////////////////////////////////////////////////////////////////
	}

	if (g_pUserInformation->attrOperator.GetValue() &&
		g_pOperatorOption->bShowModifyRANKEXP)
	{
		/////////////////////////////////////////////////////////////////////////
		// 2008.3.11 cheon ill zea - ��ް���ġ ��ȭ�� ǥ��!
		if (n == MODIFY_RANK_EXP_REMAIN)
		{
			int modifyRANKEXP = modifyValue.newValue - modifyValue.oldValue;

			if (modifyRANKEXP != 0)
			{
				char szNameBuf[256];

				sprintf(szNameBuf, "%s(%d,%d)",
					g_pUserInformation->CharacterID.GetString(), GetX(), GetY());

				DOUBLE oldRANKEXP = (DOUBLE)modifyValue.oldValue;
				DOUBLE curRANKEXP = (DOUBLE)modifyValue.newValue;

				__int64	goal_exp = max(1, g_pExperienceTable->GetRankInfo(g_char_slot_ingame.GRADE, g_eRaceInterface).GoalExp);

				char szInfoBuf[128];
				sprintf(szInfoBuf, "(srt=%4d) (old=%4d) (-%d) (cur=%4d) (%0.2f%%)",
					(int)goal_exp, (int)oldRANKEXP, abs((int)modifyRANKEXP), (int)curRANKEXP,
					(goal_exp - curRANKEXP) * 100 / goal_exp);

				if (curRANKEXP == 0)
					strcat(szInfoBuf, " (UPGRADE)");

				DWORD color = RGB_GOLD;
				UI_AddChatToHistory(szInfoBuf, szNameBuf, 6, color);
			}
		}
		//
		/////////////////////////////////////////////////////////////////////////
	}

	//20070705 by diesirace
	if (g_pUserInformation->attrOperator.GetValue() &&
		g_pOperatorOption->bShowModifyEXP)
	{
		if ((n >= MODIFY_SWORD_DOMAIN_EXP_REMAIN &&
			n <= MODIFY_GUN_DOMAIN_EXP_REMAIN &&
			((n - MODIFY_SWORD_DOMAIN_EXP_REMAIN) % 3 == 0)) ||
			n == MODIFY_VAMP_EXP_REMAIN ||
			n == MODIFY_OUSTERS_EXP_REMAIN ||
			n == MODIFY_ADVANCEMENT_CLASS_GOAL_EXP)
		{
			int modifyEXP = modifyValue.oldValue - modifyValue.newValue;
			// ������ ���� �ʿ��� �༮���� �Ǻ�
			bool isDomain = (g_pPlayer->GetRace() == RACE_SLAYER &&
				n >= MODIFY_SWORD_DOMAIN_EXP_REMAIN &&
				n <= MODIFY_GUN_DOMAIN_EXP_REMAIN);

			if (modifyEXP > 0)
			{
				char szNameBuf[64];
				sprintf(szNameBuf, "%s(%d,%d)",
					g_pUserInformation->CharacterID.GetString(), GetX(), GetY());

				int remainEXP = m_Status[n];
				char szInfoBuf[128];
				sprintf(szInfoBuf, "REMAIN =%6d GETEXP =%6d ",
					remainEXP, modifyEXP);
				if (isDomain)
				{
					const char* domainName[] = { "(Sword)", "(Blade)", "(Heal)", "(Enchant)", "(Gun)" };
					//�� ����ġ���� 3���� ������ �־ �ε�ȣ�� ������ üũ�ؼ� 
					//�� ������ ���� ������ ������ 3�����̱� ������ 3���� ������.
					int domainNameNum = (n - MODIFY_SWORD_DOMAIN_EXP_REMAIN) / 3;
					strcat(szInfoBuf, domainName[domainNameNum]);
				}

				DWORD color = RGB_GOLD;
				UI_AddChatToHistory(szInfoBuf, szNameBuf, 6, color);
			}

			//�����Ҷ� ���̵� �Ҷ�
			else if (modifyEXP < 0)
			{
				char szNameBuf[64];
				sprintf(szNameBuf, "%s(%d,%d)",
					g_pUserInformation->CharacterID.GetString(), GetX(), GetY());

				int remainEXP = m_Status[n];
				char szInfoBuf[128];
				if (!(modifyEXP + modifyValue.newValue))
				{
					sprintf(szInfoBuf, "REMAIN =%6d GETEXP =%6d (CREATE)",
						remainEXP, modifyValue.oldValue);
				}
				else
				{
					sprintf(szInfoBuf, "REMAIN =%6d GETEXP =%6d (LEVELUP)",
						remainEXP, modifyValue.oldValue);
				}
				DWORD color = RGB_GOLD;
				UI_AddChatToHistory(szInfoBuf, szNameBuf, 6, color);
			}
		}
	}
}


//----------------------------------------------------------------------
// Affect Item Option
//----------------------------------------------------------------------
// ������ �ɼ� ����
//----------------------------------------------------------------------
void
MPlayer::AffectItemOption(const MItem* pItem)
{
	DWORD& DV = m_Status[MODIFY_DEFENSE];
	DWORD& PV = m_Status[MODIFY_PROTECTION];
	DWORD& TOHIT = m_Status[MODIFY_TOHIT];
	DWORD& MinDAM = m_Status[MODIFY_MIN_DAMAGE];
	DWORD& MaxDAM = m_Status[MODIFY_MAX_DAMAGE];
	DWORD& AttackSpeed = m_Status[MODIFY_ATTACK_SPEED];

	DWORD& ElementalFire = m_Status[MODIFY_ELEMENTAL_FIRE];
	DWORD& ElementalWater = m_Status[MODIFY_ELEMENTAL_WATER];
	DWORD& ElementalEarth = m_Status[MODIFY_ELEMENTAL_EARTH];
	DWORD& ElementalWind = m_Status[MODIFY_ELEMENTAL_WIND];

	//----------------------------------------------------------
	// ��ġ�� ����Ǵ� item�� ��츸..
	//----------------------------------------------------------
	if (pItem != NULL && pItem->IsAffectStatus())
	{
		if (pItem->GetItemClass() == ITEM_CLASS_OUSTERS_WRISTLET || pItem->GetItemClass() == ITEM_CLASS_OUSTERS_STONE)
		{
			ITEMTABLE_INFO iInfo = (*g_pItemTable)[pItem->GetItemClass()][pItem->GetItemType()];
			if (iInfo.ElementalType == ITEMTABLE_INFO::ELEMENTAL_TYPE_FIRE)
				ElementalFire += iInfo.Elemental;
			else if (iInfo.ElementalType == ITEMTABLE_INFO::ELEMENTAL_TYPE_WATER)
				ElementalWater += iInfo.Elemental;
			else if (iInfo.ElementalType == ITEMTABLE_INFO::ELEMENTAL_TYPE_EARTH)
				ElementalEarth += iInfo.Elemental;
			else if (iInfo.ElementalType == ITEMTABLE_INFO::ELEMENTAL_TYPE_WIND)
				ElementalWind += iInfo.Elemental;

		}
		//---------------------------------------------------
		// �⺻���� ����ġ ����
		//---------------------------------------------------
		int def = pItem->GetDefenseValue();

		if (def != -1)
		{
			DV += def;
		}

		int pro = pItem->GetProtectionValue();

		if (pro != -1)
		{
			PV += pro;
		}

		const std::list<TYPE_ITEM_OPTION>& optionList = pItem->GetItemOptionList();

		std::list<TYPE_ITEM_OPTION>::const_iterator optionListItr = optionList.begin();

		while (optionListItr != optionList.end())
		{

			ITEMOPTION_INFO& optionInfo = (*g_pItemOptionTable)[*optionListItr];
			//---------------------------------------------------
			// �ΰ����� Option
			//---------------------------------------------------
			switch ((ITEMOPTION_TABLE::ITEMOPTION_PART)optionInfo.Part)
			{
			case ITEMOPTION_TABLE::PART_DEFENSE: DV += optionInfo.PlusPoint; break;
			case ITEMOPTION_TABLE::PART_TOHIT: TOHIT += optionInfo.PlusPoint; break;
			case ITEMOPTION_TABLE::PART_PROTECTION: PV += optionInfo.PlusPoint; break;
				//case ITEMOPTION_INFO::PART_DAMAGE		: MinDAM	+= optionInfo.PlusPoint; 
				//										  MaxDAM	+= optionInfo.PlusPoint; break;

				//case ITEMOPTION_TABLE::PART_ATTACK_SPEED : AttackSpeed += optionInfo.PlusPoint; break;

			case ITEMOPTION_TABLE::PART_VISION:	SetItemLightSight(GetItemLightSight() + optionInfo.PlusPoint); break;
			}
			optionListItr++;
		}

		std::list<TYPE_ITEM_OPTION>::const_iterator itr = pItem->GetItemDefaultOptionList().begin();

		while (itr != pItem->GetItemDefaultOptionList().end())
		{
			TYPE_ITEM_OPTION option = *itr;

			ITEMOPTION_INFO& optionInfo = (*g_pItemOptionTable)[option];
			//---------------------------------------------------
			// �ΰ����� Option
			//---------------------------------------------------
			switch ((ITEMOPTION_TABLE::ITEMOPTION_PART)optionInfo.Part)
			{
			case ITEMOPTION_TABLE::PART_DEFENSE: DV += optionInfo.PlusPoint; break;
			case ITEMOPTION_TABLE::PART_TOHIT: TOHIT += optionInfo.PlusPoint; break;
			case ITEMOPTION_TABLE::PART_PROTECTION: PV += optionInfo.PlusPoint; break;
				//case ITEMOPTION_INFO::PART_DAMAGE		: MinDAM	+= optionInfo.PlusPoint; 
				//										  MaxDAM	+= optionInfo.PlusPoint; break;

				//case ITEMOPTION_TABLE::PART_ATTACK_SPEED : AttackSpeed += optionInfo.PlusPoint; break;

			case ITEMOPTION_TABLE::PART_VISION:	SetItemLightSight(GetItemLightSight() + optionInfo.PlusPoint); break;
			}

			itr++;
		}
	}
}



//----------------------------------------------------------------------
// Check Status
//----------------------------------------------------------------------
// ��ġ ���ϴ°� üũ
//----------------------------------------------------------------------
void
MPlayer::CalculateStatus()
{
	//-----------------------------------------------------------------
	// item �����Ѱſ� ����...
	//-----------------------------------------------------------------
	MItemManager* pGear = nullptr;

	int weaponSpeed = 0;
	int weaponTohit = 0;

	switch (GetRace())
	{
	case RACE_SLAYER:
	{
		pGear = g_pSlayerGear;

		const MItem* pWeapon = g_pSlayerGear->GetItem(MSlayerGear::GEAR_SLAYER_RIGHTHAND);

		SKILLDOMAIN domain = MAX_SKILLDOMAIN;
		int domainLevel = 0;

		//------------------------------------------------------
		// �ѵ������ üũ
		//------------------------------------------------------
		if (pWeapon != NULL && pWeapon->IsAffectStatus())
		{
			if (pWeapon->IsGunItem())
			{
				domain = SKILLDOMAIN_GUN;

				weaponTohit = pWeapon->GetToHit();
			}
			else if (pWeapon->GetItemClass() == ITEM_CLASS_SWORD)
			{
				domain = SKILLDOMAIN_SWORD;
			}
			else if (pWeapon->GetItemClass() == ITEM_CLASS_BLADE)
			{
				domain = SKILLDOMAIN_BLADE;
			}
			else if (pWeapon->GetItemClass() == ITEM_CLASS_CROSS)
			{
				domain = SKILLDOMAIN_HEAL;
			}
			else if (pWeapon->GetItemClass() == ITEM_CLASS_MACE)
			{
				domain = SKILLDOMAIN_ENCHANT;
			}

			if (domain < MAX_SKILLDOMAIN)
			{
				domainLevel = (*g_pSkillManager)[domain].GetDomainLevel();
			}


			//----------------------------------------------------
			// ������ �⺻ �ӵ�
			//----------------------------------------------------			
			if (pWeapon->IsAffectStatus())
				weaponSpeed = pWeapon->GetOriginalSpeed();
		}

		g_StatusManager.SetCurrentWeaponDomain(domain, domainLevel);
	}
	break;

	case RACE_VAMPIRE:
	{
		pGear = g_pVampireGear;

		const MItem* pWeapon = g_pVampireGear->GetItem(MVampireGear::GEAR_VAMPIRE_RIGHTHAND);

		if (pWeapon != NULL)
		{
			//----------------------------------------------------
			// ������ �⺻ �ӵ�
			//----------------------------------------------------			
			if (pWeapon->IsAffectStatus())
				weaponSpeed = pWeapon->GetOriginalSpeed();
		}
		g_StatusManager.SetCurrentWeaponDomain(SKILLDOMAIN_VAMPIRE, GetLEVEL());
	}
	break;

	case RACE_OUSTERS:
	{
		pGear = g_pOustersGear;

		const MItem* pWeapon = g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND);

		if (pWeapon != NULL)
		{
			//----------------------------------------------------
			// ������ �⺻ �ӵ�
			//----------------------------------------------------			
			if (pWeapon->IsAffectStatus())
				weaponSpeed = pWeapon->GetOriginalSpeed();
		}
		g_StatusManager.SetCurrentWeaponDomain(SKILLDOMAIN_OUSTERS, GetLEVEL());
	}
	break;
	}

	//-----------------------------------------------------------------
	// Gear�� ��� item�� ���ؼ�..
	//
	// DAM, AC, TOHIT, CC option�� �����Ų��.
	//-----------------------------------------------------------------
	g_StatusManager.Set(GetSTR(), GetDEX(), GetINT());

	DWORD& DV = m_Status[MODIFY_DEFENSE];
	DWORD& PV = m_Status[MODIFY_PROTECTION];
	DWORD& TOHIT = m_Status[MODIFY_TOHIT];
	DWORD& MinDAM = m_Status[MODIFY_MIN_DAMAGE];
	DWORD& MaxDAM = m_Status[MODIFY_MAX_DAMAGE];
	DWORD& AttackSpeed = m_Status[MODIFY_ATTACK_SPEED];

	//-----------------------------------------------------------------
	// DV, PV, TOHIT�� ����� ���� �� �����ؾ��Ѵ�.
	//-----------------------------------------------------------------
	DWORD OldDV = DV;
	DWORD OldPV = PV;
	DWORD OldTOHIT = TOHIT;

	DV = g_StatusManager.GetDefense();
	PV = g_StatusManager.GetProtection();
	TOHIT = g_StatusManager.GetTOHIT() + weaponTohit;
	//int CC		= g_StatusManager.GetCC();
	//MinDAM		= m_Status[MODIFY_MIN_DAMAGE];	//g_StatusManager.GetMinDAM();
	//MaxDAM		= m_Status[MODIFY_MAX_DAMAGE];	//g_StatusManager.GetMaxDAM();
//	AttackSpeed = g_StatusManager.GetAttackSpeed() + weaponSpeed;
//
//	if((*g_pRankBonusTable)[RANK_BONUS_HAWK_WING].GetStatus() == RankBonusInfo::STATUS_LEARNED)
//		AttackSpeed += (*g_pRankBonusTable)[RANK_BONUS_HAWK_WING].GetPoint();
//
//	if((*g_pRankBonusTable)[RANK_BONUS_CROW_WING].GetStatus() == RankBonusInfo::STATUS_LEARNED)
//		AttackSpeed += (*g_pRankBonusTable)[RANK_BONUS_CROW_WING].GetPoint();
//	
//	if((*g_pRankBonusTable)[RANK_BONUS_SWIFT_ARM].GetStatus() == RankBonusInfo::STATUS_LEARNED)
//		AttackSpeed += (*g_pRankBonusTable)[RANK_BONUS_SWIFT_ARM].GetPoint();


	//-----------------------------------------------------------------
	// �����ۿ� ���ؼ� �þ߰� ������°� üũ.
	//-----------------------------------------------------------------
	SetItemLightSight(0);

	// Pet Bonus
	if (GetPetID() != OBJECTID_NULL)
	{
		MFakeCreature* pFakeCreature = (MFakeCreature*)g_pZone->GetFakeCreature(GetPetID());
		if (pFakeCreature != NULL)
		{
			MPetItem* pPetItem = pFakeCreature->GetPetItem();
			if (pPetItem != NULL)
			{
				ITEMOPTION_TABLE::ITEMOPTION_PART attr = (ITEMOPTION_TABLE::ITEMOPTION_PART)pPetItem->GetEnchantLevel();
				int PlusPoint = pPetItem->GetSilver();

				switch (attr)
				{
				case ITEMOPTION_TABLE::PART_VISION:		SetItemLightSight(GetItemLightSight() + PlusPoint); break;
				}

				AffectItemOption(pPetItem);
			}
		}
	}

	DWORD& ElementalFire = m_Status[MODIFY_ELEMENTAL_FIRE];
	DWORD& ElementalWater = m_Status[MODIFY_ELEMENTAL_WATER];
	DWORD& ElementalEarth = m_Status[MODIFY_ELEMENTAL_EARTH];
	DWORD& ElementalWind = m_Status[MODIFY_ELEMENTAL_WIND];

	ElementalFire = 0;
	ElementalWater = 0;
	ElementalEarth = 0;
	ElementalWind = 0;

	pGear->SetBegin();

	while (pGear->IsNotEnd())
	{
		const MItem* pItem = pGear->Get();

		AffectItemOption(pItem);

		pGear->Next();
	}

	// ���� ��� �ɼ� ���� add by chaya
	for (int i = 0; i < 3; ++i)
	{
		POINT ptContractOfBlood = { 8, i << 1 };

		const MItem* pSlot0 = g_pInventory->GetItem(ptContractOfBlood.x + 1, ptContractOfBlood.y);
		const MItem* pSlot1 = g_pInventory->GetItem(ptContractOfBlood.x + 1, ptContractOfBlood.y + 1);

		if (pSlot0 && pSlot1 &&
			pSlot0->GetID() == pSlot1->GetID() &&
			pSlot0->GetItemClass() == ITEM_CLASS_CONTRACT_OF_BLOOD &&
			pSlot1->GetItemClass() == ITEM_CLASS_CONTRACT_OF_BLOOD)
			//		   pItem->GetX() == ptContractOfBlood.x && pItem->GetY() == ptContractOfBlood.y)
			AffectItemOption(pSlot0);
	}

	// 2004, 9, 20, sobeit add start - ���� ���� �÷��ִ� ��޽�ų�� �ֳ�..
	if ((*g_pRankBonusTable)[RANK_BONUS_SALAMANDERS_KNOWLEDGE].GetStatus() == RankBonusInfo::STATUS_LEARNED)
		ElementalFire++;
	if ((*g_pRankBonusTable)[RANK_BONUS_UNDINES_KNOWLEDGE].GetStatus() == RankBonusInfo::STATUS_LEARNED)
		ElementalWater++;
	if ((*g_pRankBonusTable)[RANK_BONUS_GNOMES_KNOWLEDGE].GetStatus() == RankBonusInfo::STATUS_LEARNED)
		ElementalEarth++;
	// 2004, 9, 20, sobeit add end
	// ���� ��ġ ����
	SetStatus(MODIFY_ELEMENTAL_FIRE, ElementalFire);
	SetStatus(MODIFY_ELEMENTAL_WATER, ElementalWater);
	SetStatus(MODIFY_ELEMENTAL_EARTH, ElementalEarth);
	SetStatus(MODIFY_ELEMENTAL_WIND, ElementalWind);
	if (IsOusters())
		g_pSkillAvailable->SetAvailableSkills();

	//-----------------------------------------------------------------
	// ���� �ӵ� ����
	//-----------------------------------------------------------------
	SetStatus(MODIFY_ATTACK_SPEED, AttackSpeed);

	//-----------------------------------------------------------------
	// DAM, AC, TOHI, CC option�� �����Ų��.
	//-----------------------------------------------------------------
	// 2001.9.28 ����
	//SetStatus(MODIFY_DEFENSE, DV);
	//SetStatus(MODIFY_PROTECTION, PV);
	//SetStatus(MODIFY_TOHIT, TOHIT);
//	SetStatus(MODIFY_CARRYWEIGHT, CC);	

	//-----------------------------------------------------------------
	// DV, PV, TOHIT�� ����
	//-----------------------------------------------------------------
	DV = OldDV;
	PV = OldPV;
	TOHIT = OldTOHIT;

	//-----------------------------------------------------------------
	// DAM�� ���� üũ.. �ȳ�..
	//-----------------------------------------------------------------	
//	m_Status[MODIFY_MIN_DAMAGE] = MinDAM; 
//	m_Status[MODIFY_MAX_DAMAGE] = MaxDAM; 
	int maxDAM, minDAM;
	int maxSilverDAM = 0, minSilverDAM = 0;

	//-----------------------------------------------------------------
	// Slayer�� ��� --> ���� damage ����
	//-----------------------------------------------------------------
	switch (GetRace())
	{
	case RACE_SLAYER:
	{
		const MItem* pWeapon = g_pSlayerGear->GetItem(MSlayerGear::GEAR_SLAYER_RIGHTHAND);

		if (pWeapon == NULL || !pWeapon->IsAffectStatus())
		{
			minDAM = MinDAM;
			maxDAM = MaxDAM;
		}
		else //if (pWeapon->IsAffectStatus())
		{
			minDAM = MinDAM + pWeapon->GetMinDamage();
			maxDAM = MaxDAM + pWeapon->GetMaxDamage();

			//-----------------------------------------------------------------
			// ���ڰ��� ���̽��� ���  : �������� +10%
			//-----------------------------------------------------------------
			if (pWeapon->GetItemClass() == ITEM_CLASS_MACE
				|| pWeapon->GetItemClass() == ITEM_CLASS_CROSS)
			{
				minSilverDAM = pWeapon->GetMinDamage() / 10;
				maxSilverDAM = pWeapon->GetMaxDamage() / 10;
			}

			//-----------------------------------------------------------------
			// �������� �Ǿ� �ִ� ��� : �������� +10%
			// ���� ��� ���Ѿ� üũ
			//-----------------------------------------------------------------
			if (!pWeapon->IsGunItem() && pWeapon->GetSilver() > 0 ||
				pWeapon->IsGunItem() && g_pCurrentMagazine &&
				g_pCurrentMagazine->IsSilverMagazine())

			{
				minSilverDAM += pWeapon->GetMinDamage() / 10;
				maxSilverDAM += pWeapon->GetMaxDamage() / 10;
			}
		}
	}
	break;

	case RACE_VAMPIRE:
		//-----------------------------------------------------------------
		// �����̾�..
		//-----------------------------------------------------------------
	{
		const MItem* pWeapon = g_pVampireGear->GetItem(MVampireGear::GEAR_VAMPIRE_RIGHTHAND);

		if (pWeapon == NULL || !pWeapon->IsAffectStatus())
		{
			minDAM = MinDAM;
			maxDAM = MaxDAM;
		}
		else //if (pWeapon->IsAffectStatus())
		{
			minDAM = MinDAM + pWeapon->GetMinDamage();
			maxDAM = MaxDAM + pWeapon->GetMaxDamage();

		}
	}
	break;

	case RACE_OUSTERS:
		//-----------------------------------------------------------------
		// �ƿ콺����..
		//-----------------------------------------------------------------
	{
		const MItem* pWeapon = g_pOustersGear->GetItem(MOustersGear::GEAR_OUSTERS_RIGHTHAND);

		if (pWeapon == NULL || !pWeapon->IsAffectStatus() || pWeapon->GetItemClass() == ITEM_CLASS_OUSTERS_WRISTLET)
		{
			minDAM = MinDAM;
			maxDAM = MaxDAM;
		}
		else //if (pWeapon->IsAffectStatus())
		{
			minDAM = MinDAM + pWeapon->GetMinDamage();
			maxDAM = MaxDAM + pWeapon->GetMaxDamage();

		}
	}
	break;
	}

	UI_SetCharInfoDAM(maxDAM, minDAM);

	g_char_slot_ingame.SILVER_DAM2 = minSilverDAM;
	g_char_slot_ingame.SILVER_DAM = maxSilverDAM;

	// ���� �߿��� �������� �����ֱ� ������..
	if (g_Mode == MODE_WAIT_PCLIST)
	{
		UI_SetCharInfoDefense(DV);
		UI_SetCharInfoProtection(PV);
		UI_SetCharInfoTOHIT(TOHIT);
	}
	else
	{
		UI_SetCharInfoDefense(GetDefense());
		UI_SetCharInfoProtection(GetProtection());
		UI_SetCharInfoTOHIT(GetTOHIT());
	}
}

//----------------------------------------------------------------------
// Fast Move Position
//----------------------------------------------------------------------
// (m_X, m_Y) --> (x, y)
//----------------------------------------------------------------------
bool
MPlayer::FastMovePosition(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y, bool server)
{
	if (g_pTopView != NULL)
	{
		g_pTopView->SetSelectedSectorNULL();
	}

	if (MCreature::FastMovePosition(x, y))
	{
		if (server == true)
			SetServerPosition(x, y);
		// 2001.8.8  ��� �����ϰ� �غ��� ���� �ּ�ó��
		// �׷���.. ���� ������ �־.. �ٽ�... - -;;
		//m_nUsedActionInfo = ACTIONINFO_NULL;
		//m_Action		= (m_MoveAction==ACTION_SLAYER_MOTOR_MOVE)? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND;

		///*
		m_fNextTrace = FLAG_TRACE_NULL;

		m_bKeepTraceCreature = false;
		m_Action = (m_MoveAction == ACTION_SLAYER_MOTOR_MOVE) ? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND;

		UnSetRepeatAction();
		//*/

		// �� �����ƴٰ� ����.
		ResetSendMove();

		// 2001.8.10�� �߰�
		m_nUsedActionInfo = ACTIONINFO_NULL;

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// KnockBackPosition
//----------------------------------------------------------------------
bool
MPlayer::KnockBackPosition(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	if (MCreature::KnockBackPosition(x, y))
	{
		// 2001.8.8  ��� �����ϰ� �غ��� ���� �ּ�ó��
		// �׷���.. ���� ������ �־.. �ٽ�... - -;;
		//m_nUsedActionInfo = ACTIONINFO_NULL;
		//m_Action		= (m_MoveAction==ACTION_SLAYER_MOTOR_MOVE)? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND;

		///*
		//m_fNextTrace = FLAG_TRACE_NULL;

		//m_bKeepTraceCreature = false;
		//m_Action		= (m_MoveAction==ACTION_SLAYER_MOTOR_MOVE)? ACTION_SLAYER_MOTOR_STAND : ACTION_STAND;

		//UnSetRepeatAction();
		//*/

		// �� �����ƴٰ� ����.
		ResetSendMove();

		// 2001.8.10�� �߰�
		m_nUsedActionInfo = ACTIONINFO_NULL;

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Find Enemy
//----------------------------------------------------------------------
// ��ó�� ���� ã�´�.
// ���� ���ٸ� OBJECTID_NULL�� return�Ѵ�.
//----------------------------------------------------------------------
int
MPlayer::FindEnemy()
{
	//---------------------------------------------------------------
	// ���� L, R Button�� ���� ���·� LockMode�� �ൿ�� �Ǵ��Ѵ�.
	// ���� �� ���������.. ����..
	//---------------------------------------------------------------
	int bBasicAction = g_bLButtonDown;
	int bSpecialAction = g_bRButtonDown;
	int actionDistance = 0;

	//---------------------------------------------------------------
	// �⺻ �ൿ
	//-------------------------------------------------------------
	if (bBasicAction)
	{
		actionDistance = m_BasicAttackDistance;
	}
	//-------------------------------------------------------------
	// Ư�� �ൿ
	//-------------------------------------------------------------
	else if (bSpecialAction)
	{
		// Ư�� ����� �������� ���� ���
		if (m_nSpecialActionInfo == ACTIONINFO_NULL)
		{
			return NULL;
		}

		actionDistance = GetActionInfoRange(m_nSpecialActionInfo);
	}

	//-------------------------------------------------------------
	// Zone�� ��� Creature�� �˻��Ѵ�.
	//-------------------------------------------------------------
	int	numCreature = g_pZone->GetCreatureNumber();

	MZone::CREATURE_MAP::const_iterator iCreature = g_pZone->GetCreatureBegin();

	int targetCreatureID = OBJECTID_NULL;
	int targetMinDistance = 99999;

	for (int i = 0; i < numCreature; i++)
	{
		MCreature* pCreature = iCreature->second;

		//--------------------------------------------------
		// NPC�� �ƴϰ�, ���� �ƴ� ���..
		//--------------------------------------------------
		if (!pCreature->IsNPC()
			&& pCreature->GetID() != m_ID)
		{


			int tx = pCreature->GetServerX();
			int ty = pCreature->GetServerY();

			int tdistance = max(abs(tx - m_X), abs(ty - m_Y));

			//--------------------------------------------------
			// ���̰�..
			// �ൿ ���� �Ÿ��̸�...
			//--------------------------------------------------
			if (g_pObjectSelector->CanAttack(pCreature)
				&& tdistance <= actionDistance)
				//&& g_pObjectSelector->IsAblePKbyZone(pCreature))					
			{
				//*
				//--------------------------------------------------
				// ���� ����� �� ģ��. -chyaya
				//--------------------------------------------------
				if (tdistance < targetMinDistance)
				{
					targetMinDistance = tdistance;
					targetCreatureID = pCreature->GetID();
				}
				/*/
				//--------------------------------------------------
				// ó�� ���õ� ĳ������ ���
				//--------------------------------------------------
				if (targetCreatureID==OBJECTID_NULL)
				{
					targetCreatureID = pCreature->GetID();
				}
				//--------------------------------------------------
				// ó�� ���õ� ĳ���Ͱ� �ƴ� ���
				//--------------------------------------------------
				else
				{
					//--------------------------------------------------
					// �ݹ��� Ȯ���� ��ǥ�� �ٽ� �����Ѵ�.
					//--------------------------------------------------
					if (rand()%2)
					{
						targetCreatureID = pCreature->GetID();
					}
					//--------------------------------------------------
					// �ٽ� �������� ���� ���
					// ������ ������ ĳ���͸� �����Ѵ�.
					//--------------------------------------------------
					else
					{
						return targetCreatureID;
					}
				}
				//*/
			}
		}

		iCreature++;
	}

	return targetCreatureID;
}

//----------------------------------------------------------------------
// Stop Blood Drain
//----------------------------------------------------------------------
// m_bStopBloodDrain�̸� �̰� ȣ���ؾ� �Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::StopBloodDrain()
{
	if (m_Action == ACTION_VAMPIRE_DRAIN
		&& IsVampire()
		&& m_ActionCountMax > 60)
	{
		if (m_ActionCount > 9 && m_ActionCount < 60)
		{
			m_ActionCount = 60;

			// delay �ٽ� ����
			//m_DelayTime	= g_CurrentTime + (m_ActionCountMax-m_ActionCount)*60;		

			UnSetStopBloodDrain();
			//			char TempBuffer[128];
			//			sprintf(TempBuffer, "StopBD: D:%d, m:%d, C:%d", m_DelayTime,m_bStopBloodDrain, m_ActionCount);
			//			g_pSystemMessage->Add(TempBuffer);

						//DEBUG_ADD_FORMAT("[Player] StopBloodDrain1(%d)", m_bStopBloodDrain);

		}
	}
	else
	{
		//UnSetStopBloodDrain();
		//DEBUG_ADD_FORMAT("[Player] StopBloodDrain2(%d), action=%d(%d)", m_bStopBloodDrain, m_Action, m_ActionCountMax);
	}
}

//----------------------------------------------------------------------
// Stop Absorb Soul
//----------------------------------------------------------------------
// m_bStopAbsorbSoul�̸� �̰� ȣ���ؾ� �Ѵ�.
//----------------------------------------------------------------------
void
MPlayer::StopAbsorbSoul()
{
	if (m_Action == ACTION_OUSTERS_DRAIN
		&& IsOusters()
		&& m_ActionCountMax > 30)
	{
		if (m_ActionCount > 9 && m_ActionCount < 30)
		{
			m_ActionCount = 30;

			ATTACHEFFECT_LIST::iterator itr = m_listEffect.begin();
			ATTACHEFFECT_LIST::iterator endItr = m_listEffect.end();

			while (itr != endItr)
			{
				MAttachEffect* pEffect = *itr;

				if (pEffect->GetEffectSpriteType() == EFFECTSPRITETYPE_ABSORB_SOUL)
				{
					bool bUseEffectSprite = pEffect->IsEffectSprite();

					// flag����
					if (bUseEffectSprite)
					{
						m_bAttachEffect[pEffect->GetEffectSpriteType()] = false;	// flag����
					}

					delete pEffect;
					m_listEffect.erase(itr);
					break;
				}
				itr++;
			}

			UnSetStopAbsorbSoul();
		}
	}
	else
	{
		//UnSetStopBloodDrain();
	}
}

//----------------------------------------------------------------------
// Check In Darkness
//----------------------------------------------------------------------
// darkness�� ��Ÿ����/������� ����
//            player�� �����̴� ������ üũ�ϸ� �ȴ�.
//----------------------------------------------------------------------
void
MPlayer::CheckInDarkness()
{
	if (g_pZone != NULL &&
		(!IsVampire() && (!(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || g_pZone->GetID() == 3001) ||
			g_pZone->GetID() == 3001 && IsVampire())
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		&& !HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET)
		)
	{
		//-------------------------------------------------------
		// Darkness�� ���Դ��� üũ
		//-------------------------------------------------------
		// ���� ��ǥ���ٴ� ���� ��ǥ�� ���⿡ ����.. ����..
		const MSector& sector = g_pZone->GetSector(m_X, m_Y);

		if (sector.HasDarkness())
		{
			// darkness�� �ִ� ���
			PlaceInDarkness();
		}
		else
		{
			// darkness�� �������.
			PlaceNotInDarkness();
		}
	}
	else
	{
		PlaceNotInDarkness();
	}
}

//----------------------------------------------------------------------
// Set SpecialActionInfo
//----------------------------------------------------------------------
void
MPlayer::SetSpecialActionInfo(TYPE_ACTIONINFO n)
{

	if (g_pActionInfoTable != NULL
		&& n < g_pActionInfoTable->GetMinResultActionInfo())
	{
		m_nSpecialActionInfo = n;

		// �ݺ� �߿���..
		// ����� Target�� �ٲ�� ��찡 �ִ�.
		if (IsRepeatAction())
		{
			if (m_fTraceBuffer != FLAG_TRACE_NULL
				// �⺻ ���� �߿��� �������.
				&& !(m_fTraceBuffer & FLAG_TRACE_CREATURE_BASIC)
				&& !(m_fTraceBuffer & FLAG_TRACE_SECTOR_BASIC)
				&& !(m_fTraceBuffer & FLAG_TRACE_ITEM)
				&& !(m_fTraceBuffer & FLAG_TRACE_INTERACTIONOBJECT)
				&& !(m_fTraceBuffer & FLAG_TRACE_EFFECT)
				)
			{
				// ��� ������� ���..
				BOOL bOther = ((*g_pActionInfoTable)[n].IsTargetOther());
				BOOL bSelf = ((*g_pActionInfoTable)[n].IsTargetSelf());
				BOOL bZone = ((*g_pActionInfoTable)[n].IsTargetZone());
				BOOL bItem = ((*g_pActionInfoTable)[n].IsTargetItem());

				// �������� ���...
				if (((m_fTraceBuffer & FLAG_TRACE_SELF) && bSelf)
					|| ((m_fTraceBuffer & FLAG_TRACE_CREATURE_SPECIAL) && bOther)
					|| ((m_fTraceBuffer & FLAG_TRACE_SECTOR_SPECIAL) && bZone)
					|| ((m_fTraceBuffer & FLAG_TRACE_INVENTORY) && bItem)
					)
				{
				}
				// ���������� ���.. target ������ �ٲ�� �Ѵ�.
				else
				{
					if (bOther)
					{
						m_fTraceBuffer = FLAG_TRACE_CREATURE_SPECIAL;
					}
					else if (bSelf)
					{
						m_fTraceBuffer = FLAG_TRACE_SELF;
					}
					else if (bZone)
					{
						m_fTraceBuffer = FLAG_TRACE_SECTOR_SPECIAL;
					}
					else if (bItem)
					{
						m_fTraceBuffer = FLAG_TRACE_INVENTORY;
					}
				}
			}
		}
	}

	//	MessageBox(NULL, "DASfadsf" ,"dSFA" ,MB_OK) ;  
		//if((*g_pActionInfoTable)[m_nSpecialActionInfo].GetComboAttackSkill())
		//	gC_vs_ui.SetSelectedAttackComboSkill(TRUE); 
		//else
		//	gC_vs_ui.SetSelectedAttackComboSkill(); 

}

//----------------------------------------------------------------------
// Is In SafeSector
//----------------------------------------------------------------------
BOOL
MPlayer::IsInSafeSector() const
{
	if (g_pWarManager->IsExist(m_pZone->GetID()))
		return FALSE;

	if (m_pZone != NULL)
	{
		if (m_pZone->GetSector(m_X, m_Y).IsSafeComplete() || g_bZoneSafe)
			return 2;

		if (IsSlayer())
		{
			return (m_pZone->GetSector(m_X, m_Y).IsSafeSlayer()) ? 1 : 0;
		}
		else
			if (IsVampire())
			{
				return (m_pZone->GetSector(m_X, m_Y).IsSafeVampire()) ? 1 : 0;
			}
			else
				if (IsOusters())
				{
					return (m_pZone->GetSector(m_X, m_Y).IsSafeOusters()) ? 1 : 0;
				}
	}

	return FALSE;
}

//----------------------------------------------------------------------
// Is In Sector Safe Zone
//
// The sectors the zone file flags safe for the player's race, or for
// everyone - what the minimap paints green. A zone that is safe as a
// whole (a town's Safety flag) does not count here, and neither does
// any sector while a war is on in the zone.
//----------------------------------------------------------------------
bool
MPlayer::IsInSectorSafeZone() const
{
	if (m_pZone == NULL)
		return false;

	if (g_pWarManager->IsExist(m_pZone->GetID()))
		return false;

	const MSector& sector = m_pZone->GetSector(m_X, m_Y);

	if (IsSlayer())
		return sector.IsSafeSlayer() != 0;

	if (IsVampire())
		return sector.IsSafeVampire() != 0;

	if (IsOusters())
		return sector.IsSafeOusters() != 0;

	return false;
}

//-------------------------------------------------------------------------
// Soul Chain File
//-------------------------------------------------------------------------
void
MPlayer::SetCannotTrace()
{
	gC_vs_ui.SetCannotTrace();
	g_pUIDialog->SetLockInputPCTalk();
}

void	MPlayer::SetTimeLightSight(char s)
{
	m_TimeLightSight = s;
	CalculateLightSight();
}

void	MPlayer::SetItemLightSight(int s)
{
	m_ItemLightSight = s;
	CalculateLightSight();
}

void	MPlayer::CalculateLightSight()
{
	//	CalculateSight();
	m_LightSight = GetTimeLightSight() + GetItemLightSight();

	// [�����6] observing eye�� �ɷ������� �þ߰� �о�����.
	if (m_bEffectStatus[EFFECTSTATUS_OBSERVING_EYE])
	{
		m_LightSight += 1 + (*g_pSkillInfoTable)[SKILL_OBSERVING_EYE].GetLevel() / 50;
	}
	// Pet Bonus
	/*
	if( GetPetID() != OBJECTID_NULL)
	{
		MFakeCreature *pFakeCreature = (MFakeCreature *)g_pZone->GetFakeCreature(GetPetID());
		if(pFakeCreature != NULL)
		{
			MPetItem *pPetItem = pFakeCreature->GetPetItem();
			if(pPetItem != NULL)
			{
				ITEMOPTION_TABLE::ITEMOPTION_PART attr = (ITEMOPTION_TABLE::ITEMOPTION_PART)pPetItem->GetEnchantLevel();
				if(attr == ITEMOPTION_TABLE::PART_VISION)
				{
					m_LightSight += pPetItem->GetSilver();
				}
			}
		}
	}
	*/

	if (g_pSkillAvailable->IsEnableSkill(SKILL_HOLYLAND_BLOOD_BIBLE_GREGORI))
	{
		m_LightSight += 5;
	}

	if (g_pSkillAvailable->IsEnableSkill(SKILL_SWEEPER_BONUS_12))
	{
		m_LightSight += 5;
	}

	if (IsFlyingCreature())
	{
		m_LightSight += 3;
	}

	// 2004, 6, 24, sobeit add start
	if (HasEffectStatus(EFFECTSTATUS_BLINDNESS))
	{
		//m_LightSight = 1;
		PlaceInDarkness(true);
	}
	// 2004, 6, 24, sobeit add end

	if (HasEffectStatus(EFFECTSTATUS_LIGHT) ||
		(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)))
	{
		m_LightSight += 6;
	}
}

void	MPlayer::SetUndergroundCreature()
{
	MCreature::SetUndergroundCreature();
	CalculateLightSight();
}

void	MPlayer::SetGroundCreature()
{
	MCreature::SetGroundCreature();
	CalculateLightSight();
}

void	MPlayer::SetFlyingCreature()
{
	MCreature::SetFlyingCreature();
	CalculateLightSight();
}

void	MPlayer::CalculateSight()
{
	// -_----; �þ� ���� ������ ���װ� ���Ƽ� �� �ǵ���

	//-----------------------------------------------------------------
	// item �����Ѱſ� ����...
	//-----------------------------------------------------------------
	MItemManager* pGear = GetGear();

	//-----------------------------------------------------------------
	// Slayer�� ���
	//-----------------------------------------------------------------	
	if (pGear == NULL)
		return;

	SetItemLightSight(0);

	pGear->SetBegin();

	while (pGear->IsNotEnd())
	{
		const MItem* pItem = pGear->Get();

		//----------------------------------------------------------
		// ��ġ�� ����Ǵ� item�� ��츸..
		//----------------------------------------------------------
		if (pItem->IsAffectStatus())
		{
			const std::list<TYPE_ITEM_OPTION>& optionList = pItem->GetItemOptionList();
			std::list<TYPE_ITEM_OPTION>::const_iterator optionListItr = optionList.begin();
			while (optionListItr != optionList.end())
			{
				ITEMOPTION_INFO& optionInfo = (*g_pItemOptionTable)[*optionListItr];
				//---------------------------------------------------
				// �ΰ����� Option
				//---------------------------------------------------
				switch ((ITEMOPTION_TABLE::ITEMOPTION_PART)optionInfo.Part)
				{
				case ITEMOPTION_TABLE::PART_VISION:
					SetItemLightSight(GetItemLightSight() + optionInfo.PlusPoint);
					break;
				}
				optionListItr++;
			}

			std::list<TYPE_ITEM_OPTION>::const_iterator itr = pItem->GetItemDefaultOptionList().begin();

			while (itr != pItem->GetItemDefaultOptionList().end())
			{
				TYPE_ITEM_OPTION option = *itr;
				ITEMOPTION_INFO& optionInfo = (*g_pItemOptionTable)[option];
				//---------------------------------------------------
				// �ΰ����� Option
				//---------------------------------------------------
				switch ((ITEMOPTION_TABLE::ITEMOPTION_PART)optionInfo.Part)
				{
				case ITEMOPTION_TABLE::PART_VISION:
					SetItemLightSight(GetItemLightSight() + optionInfo.PlusPoint);
					break;
				}
				itr++;
			}
		}
		pGear->Next();
	}
	if ( //(HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || 
		g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		|| g_bLight
#endif
		)
	{
		// �ְ�~~
		SetItemLightSight(GetItemLightSight() + 15);
	}
	else
	{
		if (HasEffectStatus(EFFECTSTATUS_FLARE) && IsVampire() ||
			HasEffectStatus(EFFECTSTATUS_YELLOW_POISON) && IsSlayer() ||
			HasEffectStatus(EFFECTSTATUS_YELLOW_POISON_TO_CREATURE))//||
			//			HasEffectStatus( EFFECTSTATUS_BLINDNESS ))
			SetItemLightSight(1);
	}
}
// 2004, 8, 30, sobeit add start - sweep vice test
void
MPlayer::UpdateSweepVice()
{
	if (GetSweepViewValue())	// ���� ���̽� ���ϋ�
	{
		//if(IsDead() || m_nUsedActionInfo != SKILL_SWEEP_VICE_1 || m_SweepViceX == SECTORPOSITION_NULL ||	m_SweepViceY == SECTORPOSITION_NULL	)
		if (IsDead() || !IsSlayer() || m_SweepViceX == SECTORPOSITION_NULL || m_SweepViceY == SECTORPOSITION_NULL)
		{
			g_pZone->RemoveSwapViceType();
			SetSweepViewValue(0);
			return;
		}
		POINT CurrentPos = g_pTopView->GetSelectedSector(g_x, g_y);
		int TempVal = 0;
		int Distance = max(abs(m_TempSelectPosX - CurrentPos.x), abs(m_TempSelectPosY - CurrentPos.y));

		if (Distance < 2)
		{
			TempVal = 1;
			if (GetSweepViewValue() != TempVal)
			{
				g_pZone->ChangeSwapViceType(m_SweepViceX, m_SweepViceY, EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_1);
				PlaySound(SOUND_SLAYER_SWEEPVICE_RESIZE);
			}

		}
		else if (Distance < 3)
		{
			TempVal = 2;
			if (GetSweepViewValue() != TempVal)
			{
				g_pZone->ChangeSwapViceType(m_SweepViceX, m_SweepViceY, EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_2);
				PlaySound(SOUND_SLAYER_SWEEPVICE_RESIZE);
			}
		}
		else
		{
			TempVal = 3;
			if (GetSweepViewValue() != TempVal)
			{
				g_pZone->ChangeSwapViceType(m_SweepViceX, m_SweepViceY, EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_3);
				PlaySound(SOUND_SLAYER_SWEEPVICE_RESIZE);
			}
		}
		SetSweepViewValue(TempVal);
	}
}

bool
MPlayer::CheckRbuttonUpSkill()
{
	if (HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
	{
		m_fNextTrace = FLAG_TRACE_NULL;
		UnSetRepeatAction();
		InstallTurretStopAttack();
	}
	if (GetSweepViewValue())
	{
		g_pZone->RemoveSwapViceType();
		m_nUsedActionInfo = SKILL_SWEEP_VICE_1;
		ActionEffect();
		SetSweepViewValue(0);
		m_nUsedActionInfo = SKILL_SWEEP_VICE_1;
		m_TempSelectPosX = SECTORPOSITION_NULL;
		m_TempSelectPosY = SECTORPOSITION_NULL;
		m_SweepViceX = SECTORPOSITION_NULL;
		m_SweepViceY = SECTORPOSITION_NULL;

		return true;
	}
	//	else if(IsBurningSol())
	//	{
	//		return true;
	//	}
	return false;
}
// 2004, 8, 30, sobeit add end - sweep vice test



bool MPlayer::UpdateBurningSol()
{
	if (m_bBurningSol)
	{
		if (IsDead() || !IsSlayer())
		{
			SetBurningSol(0);
			StopBurningSol();
			SetStop();
			m_nUsedActionInfo = ACTIONINFO_NULL;
			m_fNextTrace = FLAG_TRACE_NULL;
			UnSetRepeatAction();
			//	m_ActionCount = m_ActionCountMax;
			SetAction(ACTION_DIE);
			return false;
		}
		else if (HasEffectStatus(EFFECTSTATUS_CURSE_PARALYSIS)
			|| m_MoveDevice == MOVE_DEVICE_RIDE)
		{
			SetBurningSol(0);
			StopBurningSol();
			//SetStop();
			m_nUsedActionInfo = ACTIONINFO_NULL;
			m_fNextTrace = FLAG_TRACE_NULL;
			UnSetRepeatAction();
			RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_1);
			RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_2);
			RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_3);
			RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_4);
			return false;
		}
		if (m_TempSelectPosX == SECTORPOSITION_NULL || m_TempSelectPosY == SECTORPOSITION_NULL)
		{
			//SetBurningSol(0);
			StopBurningSol();
			m_nUsedActionInfo = ACTIONINFO_NULL;
			m_fNextTrace = FLAG_TRACE_NULL;
			//			UnSetRepeatAction();
						//m_ActionCount = m_ActionCountMax;
			SetAction(ACTION_STAND);
			return false;
		}
		int bCheckFrame = 0;
		if (!IsAdvancementClass())
			bCheckFrame = 3;
		else
			bCheckFrame = 7;

		m_bBurningSolCount++;

		//if(g_bRButtonDown)
		if (HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_1) ||
			HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_2) ||
			HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_3) ||
			HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_4))
		{
			if (m_bBurningSolCount >= bCheckFrame)
				m_bBurningSolCount = bCheckFrame;
		}
		POINT  TempPos = g_pTopView->GetSelectedSector(g_x, g_y);
		m_TempSelectPosX = TempPos.x;
		m_TempSelectPosY = TempPos.y;
		m_Direction = MTopView::GetDirectionToPosition(g_pPlayer->GetX(), g_pPlayer->GetY(), m_TempSelectPosX, m_TempSelectPosY);
		ChangeNearDirection();
		//else
		if (!g_bRButtonDown)
		{
			if (m_bBurningSolCount >= bCheckFrame)
			{
				//				POINT  TempPos = g_pTopView->GetSelectedSector(g_x, g_y);
				//				m_TempSelectPosX = TempPos.x;
				//				m_TempSelectPosY = TempPos.y;
				//				m_CurrentDirection = MTopView::GetDirectionToPosition(g_pPlayer->GetX(), g_pPlayer->GetY(),m_TempSelectPosX, m_TempSelectPosY);
				//				

				bool ShowLunchEffect = true;
				int TempDistance = GetActionInfoRange(SKILL_BURNING_SOUL_LAUNCH);
				if (max(abs(m_X - m_TempSelectPosX), abs(m_Y - m_TempSelectPosY)) > TempDistance)
				{
					ShowLunchEffect = false;
				}

				if (HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_1))
				{
					if (ShowLunchEffect)
						ExecuteActionInfoFromMainNode(SKILL_CLIENT_BURNING_SOL_1, g_pPlayer->GetX(), g_pPlayer->GetY(), 0, g_pPlayer->GetDirection(), g_pPlayer->GetID(),
							m_TempSelectPosX, m_TempSelectPosY, 0, 10, NULL, false);
					RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_1);
				}
				else if (HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_2))
				{
					if (ShowLunchEffect)
						ExecuteActionInfoFromMainNode(SKILL_CLIENT_BURNING_SOL_2, g_pPlayer->GetX(), g_pPlayer->GetY(), 0, g_pPlayer->GetDirection(), g_pPlayer->GetID(),
							m_TempSelectPosX, m_TempSelectPosY, 0, 10, NULL, false);
					RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_2);
				}
				else if (HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_3))
				{
					if (ShowLunchEffect)
						ExecuteActionInfoFromMainNode(SKILL_CLIENT_BURNING_SOL_3, g_pPlayer->GetX(), g_pPlayer->GetY(), 0, g_pPlayer->GetDirection(), g_pPlayer->GetID(),
							m_TempSelectPosX, m_TempSelectPosY, 0, 10, NULL, false);
					RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_3);
				}
				else if (HasEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_4))
				{
					if (ShowLunchEffect)
						ExecuteActionInfoFromMainNode(SKILL_CLIENT_BURNING_SOL_4, g_pPlayer->GetX(), g_pPlayer->GetY(), 0, g_pPlayer->GetDirection(), g_pPlayer->GetID(),
							m_TempSelectPosX, m_TempSelectPosY, 0, 10, NULL, false);
					RemoveEffectStatus(EFFECTSTATUS_BURNING_SOL_CHARGE_4);
				}

				m_nUsedActionInfo = SKILL_BURNING_SOUL_LAUNCH;
				m_fTraceBuffer = FLAG_TRACE_SECTOR_SPECIAL;
				ActionToSendPacket();
				m_nNextUsedActionInfo = ACTIONINFO_NULL;
				//SetBurningSol(0);
				StopBurningSol();

				UnSetRepeatAction();
				m_TempSelectPosX = SECTORPOSITION_NULL;
				m_TempSelectPosY = SECTORPOSITION_NULL;
			}
		}
		m_ActionCount = m_bBurningSolCount;

		return true;
#ifdef OUTPUT_DEBUG					
		DEBUG_ADD_FORMAT("[UpdateBurningSol] ActionCount  : %d", m_bBurningSolCount);
#endif
	}
	return false;
}


bool MPlayer::UpDateInstallTurret()
{
	if (HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
	{
		if (!IsNotDelay())
		{
			m_Action = ACTION_STAND;
			//m_CurrentDirection = 2; 
			return true;
		}
		if (IsAlive() && (m_Action == ACTION_STAND))
		{
			if (m_bInstallTurretDirect != m_CurrentDirection)
			{
				if (((m_bInstallTurretDirect - m_CurrentDirection + 8) % 8) < 4)
				{
					m_bInstallTurretCount = ((m_bInstallTurretCount - 1) + 16) % 16;
				}
				else
				{
					m_bInstallTurretCount = (m_bInstallTurretCount + 1) % 16;
				}

				if (!(m_bInstallTurretCount & 0x01))
					m_bInstallTurretDirect = ((m_bInstallTurretCount / 2) + 2) % 8;
			}
			return true;
		}


		SetInstallTurretDirect(m_CurrentDirection);
		SetInstallTurretCount(((m_CurrentDirection + 6) % 8) * 2);
	}
	return false;
}
// 2005, 1, 4, sobeit add start
void
MPlayer::UseWildWolf_Corpse(MItem* pItem)
{
	if (pItem == NULL) return;
	m_TraceDistance = GetActionInfoRange(SKILL_WILD_WOLF);
	if (max(abs(m_X - pItem->GetX()), abs(m_Y - pItem->GetY())) <= m_TraceDistance && g_pSkillAvailable->IsEnableSkill(SKILL_WILD_WOLF))
	{
		SetAction(ACTION_MAGIC);
		CGSkillToObject _CGSkillToObject;
		_CGSkillToObject.setSkillType(SKILL_WILD_WOLF);
		_CGSkillToObject.setCEffectID(0);
		_CGSkillToObject.setTargetObjectID(pItem->GetID());
		g_pSocket->sendPacket(&_CGSkillToObject);
	}
}
// 2005, 1, 4, sobeit add end



// 2005.06.30 combo Skill State  My Creature  �� ����  Sjheon
void
MPlayer::AddComboEndValue()
{
	m_byComboEndValue;
	m_byComboEndValue = (m_byComboEndValue & 3); ///%  4) ; 
	++m_byComboEndValue;
	gC_vs_ui.SetComboCnt(m_byComboEndValue);
}

// 2005.06.30 combo Skill State  My Creature �� ���� Sjheon

void
MPlayer::UpdateActionStep()
{
	if (m_ActionCount == m_ActionCountMax - 1)
	{
		m_ActionStep = (++m_ActionStep) % 3;

		//		OutputDebugString("UpdateSkillStep()\n");
	}
}

void
MPlayer::RecordCharName(TYPE_OBJECTID id)
{
	std::ofstream RecordCharName("RecordCharName.txt", std::ios::app);
	RecordCharName << m_pZone->GetCreature(id)->GetName() << std::endl;
	RecordCharName.close();
}

#if __CONTENTS(__FRIEND_ADDITION)
void
MPlayer::RequestModeExec()
{
	if (IsInDarkness())
		return;

	MCreature* pCreature = g_pZone->GetCreature(m_TraceID);

	if (pCreature == NULL)
		return;
	if (pCreature->CurPernalShop() == 1)
		return;
	if (pCreature->IsNPC())
		return;

	if (IsRequestMode())
	{
		switch (m_RequestMode)
		{
			//---------------------------------------------------------------
			// Trade �ҷ��� ���
			//---------------------------------------------------------------
		case REQUEST_TRADE:
		{
			if (IsWaitVerifyNULL()
				&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
				&& (IsSlayer() && pCreature->IsSlayer() ||
					IsVampire() && pCreature->IsVampire() ||
					IsOusters() && pCreature->IsOusters())
				)
			{
				// ��ȯ ��û
				CGTradePrepare _CGTradePrepare;
				_CGTradePrepare.setTargetObjectID(m_TraceID);
				_CGTradePrepare.setCode(CG_TRADE_PREPARE_CODE_REQUEST);

				g_pSocket->sendPacket(&_CGTradePrepare);

				SetWaitVerify(WAIT_VERIFY_TRADE);

				// temp information�� ����.. (��..�ణ �����ϴ�.- -;)
				g_pTempInformation->SetMode(TempInformation::MODE_TRADE_REQUEST);
				g_pTempInformation->Value1 = m_TraceID;

				//-----------------------------------------------------
				// ��ȯ ����ұ�?
				//-----------------------------------------------------
				UI_RunExchangeCancel(pCreature->GetName());
			}
		}
		break;

		//---------------------------------------------------------------
		// Party �ҷ��� ���
		//---------------------------------------------------------------
		case REQUEST_PARTY:
		{
			if (IsWaitVerifyNULL()
				&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
				&& g_pParty != NULL
				// �� ��Ƽ���� �ƴ� ��쿡�� ��û �����ϴ�.
				&& !g_pParty->HasMember(pCreature->GetName())
				&& (IsSlayer() && pCreature->IsSlayer() || IsVampire() && pCreature->IsVampire() || IsOusters() && pCreature->IsOusters())
				&& g_pSystemAvailableManager->IsAvailablePartySystem()
				)
			{
				// ��Ƽ ��û
				CGPartyInvite _CGPartyInvite;
				_CGPartyInvite.setTargetObjectID(m_TraceID);
				_CGPartyInvite.setCode(CG_PARTY_INVITE_REQUEST);

				g_pSocket->sendPacket(&_CGPartyInvite);

				SetWaitVerify(WAIT_VERIFY_PARTY);

				// temp information�� ����.. (��..�ణ �����ϴ�.- -;)
				g_pTempInformation->SetMode(TempInformation::MODE_PARTY_REQUEST);
				g_pTempInformation->Value1 = m_TraceID;

				//-----------------------------------------------------
				// ��ȯ ����ұ�?
				//-----------------------------------------------------
				UI_RunPartyCancel(pCreature->GetName());
			}
		}
		break;

		//---------------------------------------------------------------
		// OtherInfo ������ ���
		//---------------------------------------------------------------
		case REQUEST_INFO:
		{
			if (IsWaitVerifyNULL()
				&& g_pTempInformation->GetMode() == TempInformation::MODE_NULL
				&& (IsSlayer() && pCreature->IsSlayer() ||
					IsVampire() && pCreature->IsVampire() ||
					IsOusters() && pCreature->IsOusters() ||
					GetCreatureType() == CREATURETYPE_SLAYER_OPERATOR ||
					GetCreatureType() == CREATURETYPE_VAMPIRE_OPERATOR ||
					GetCreatureType() == CREATURETYPE_OUSTERS_OPERATOR ||
					GetCompetence() == 0)
				)
			{
				const char* str = gC_vs_ui.GetInputString();
				if (strlen(str) > 0)
				{
					if (str[0] == '*')
					{
						gC_vs_ui.AddInputString(pCreature->GetName());
						break;

					}
				}
				// ���� ��û
				CGRequestInfo _CGRequestInfo;
				_CGRequestInfo.setValue(m_TraceID);
				_CGRequestInfo.setCode(CGRequestInfo::REQUEST_CHARACTER_INFO);

				g_pSocket->sendPacket(&_CGRequestInfo);

				MCreature* pCreature = g_pZone->GetCreature(m_TraceID);

				if (pCreature != NULL)
				{
					UI_RunOtherInfo(pCreature);

					if (g_pProfileManager != NULL
						&& g_pRequestUserManager != NULL
						&& g_pRequestClientPlayerManager != NULL)
					{
						g_pProfileManager->RequestProfile(pCreature->GetName());
					}
				}
			}
		}
		break;
		}
		UnSetRequestMode();
	}
}
#endif //__FRIEND_ADDITION