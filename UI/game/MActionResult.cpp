//----------------------------------------------------------------------
// MActionResult.cpp
//----------------------------------------------------------------------
#include "Client_PCH.h"
#include "MActionResult.h"
#include "MTopView.h"
#include "ClientDef.h"
#include "MEffectGeneratorTable.h"
#include "UIFunction.h"
#include "MCreatureTable.h"
#include "VS_UI.h"
#include "MEventManager.h"
#include "MItemOptionTable.h"

#include "DebugInfo.h"
//#define	new			DEBUG_NEW
//#define	delete		DEBUG_DELETE

#define SAFE_DELETE(x)		{ if(x!=NULL) delete x; x=NULL; }

extern int g_MorphCreatureType;

// [새기술]
void		SkillShadowDancing(MCreature* pUserCreature, MCreature* pTargetCreature, int skillID);

//----------------------------------------------------------------------
// 
// ActionResultNode :: constructor/destructor
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// ActionResultNode - ActionInfo
//
//----------------------------------------------------------------------
MActionResultNodeActionInfo::MActionResultNodeActionInfo(TYPE_ACTIONINFO nActionInfo, TYPE_OBJECTID uid, TYPE_OBJECTID tid, TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, DWORD delayFrame, BYTE temp)
{ 
	m_UserID		= uid;
	m_TargetID		= tid;
	m_nActionInfo	= nActionInfo;
	m_DelayFrame	= delayFrame;

	m_sX			= sX;
	m_sY			= sY;

	m_Temp			= temp;
}

//----------------------------------------------------------------------
// ActionResultNode - ActionInfo::Execute
//----------------------------------------------------------------------
void
MActionResultNodeActionInfo::Execute()
{
	DEBUG_ADD_FORMAT("ActionResultNodeAIExecute. u=%d, t=%d", m_UserID, m_TargetID);
	///*
	//POINT point;
	if (g_pZone==NULL)
	{
		return;
	}


	//--------------------------------------------------------
	// 목표 위치 Pixel좌표
	//--------------------------------------------------------
	MCreature*	pTargetCreature = g_pZone->GetCreature( m_TargetID );
	MCreature*	pUserCreature	= g_pZone->GetCreature( m_UserID );

	// 캐릭터가 없으면... 끝~
	if (pTargetCreature==NULL)// || pUserCreature==NULL)
	{
		//MItem* pItem = g_pZone->GetItem( m_TargetID );	
		
		return;		
	}

	TYPE_ACTIONINFO nUsedActionInfo = m_nActionInfo;

/*
	point = MTopView::MapToPixel(pTargetCreature->GetX(), pTargetCreature->GetY());

	
	if ((*g_pActionInfoTable)[nUsedActionInfo].GetSize()!=0)
	{
		//--------------------------------------------------------
		//
		//					Effect 목표 설정
		//
		//--------------------------------------------------------
		MEffectTarget* pEffectTarget = new MEffectTarget( (*g_pActionInfoTable)[nUsedActionInfo].GetSize() );

		pEffectTarget->Set( point.x, point.y, pTargetCreature->GetZ(), pTargetCreature->GetID() );		

		//--------------------------------------------------------
		//
		//					시작 위치를 결정한다.
		//
		//--------------------------------------------------------
		int x,y,z, direction;

		//--------------------------------------------------------
		// User 위치에서 시작하는 경우
		//--------------------------------------------------------
		if ((*g_pActionInfoTable)[nUsedActionInfo].IsStartUser())
		{
			point = MTopView::MapToPixel(pUserCreature->GetX(), pUserCreature->GetY());

			x			= point.x;
			y			= point.y;
			z			= pUserCreature->GetZ();//+60;
		}
		//--------------------------------------------------------
		// Target 위치에서 시작하는 경우
		//--------------------------------------------------------
		else if ((*g_pActionInfoTable)[nUsedActionInfo].IsStartTarget())
		{
			x			= point.x;
			y			= point.y;
			z			= pUserCreature->GetZ();//+60;			
		}

		//--------------------------------------------------------
		// 공중에서 시작하는 경우
		//--------------------------------------------------------
		if ((*g_pActionInfoTable)[nUsedActionInfo].IsStartSky())
		{
			z	= pUserCreature->GetZ() + (*g_pActionInfoTable)[nUsedActionInfo].GetValue();

			direction	= DIRECTION_DOWN;
		}
		//--------------------------------------------------------
		// 지상에서 시작하는 경우
		//--------------------------------------------------------
		else
		{
			direction	= pUserCreature->GetDirection();
		}

		//--------------------------------------------------------
		//
		//                   Effect생성		
		//
		//--------------------------------------------------------
		g_EffectGeneratorTable.Generate(
				x,y,z,				// 시작 위치
				direction, 			// 방향
				1,					// power
				nUsedActionInfo,	//	ActionInfoTable종류,
				pEffectTarget		// 목표 정보
		);
	}
	*/
	//--------------------------------------------------------
	// 대상이 되는 캐릭터는 특정한 Action을 취하게 된다.
	//--------------------------------------------------------
	//pTargetCreature->SetStop();
	//pTargetCreature->SetAction( (*g_pActionInfoTable)[nUsedActionInfo].GetAction() );

	int resultActionInfo;
	resultActionInfo = nUsedActionInfo + (*g_pActionInfoTable).GetMinResultActionInfo();	// 결과

	switch( nUsedActionInfo )
	{
	case SKILL_SHADOW_DANCING :
		resultActionInfo = SKILL_SINGLE_BLOW + (*g_pActionInfoTable).GetMinResultActionInfo();
		break;

	case SKILL_ILLENDUE :
		resultActionInfo = SKILL_LIGHT_BALL + (*g_pActionInfoTable).GetMinResultActionInfo();
		break;
#if __CONTENTS(__ONIBLA)
	case SKILL_AGONY_CHILD_SHADOW_FOUR :
		resultActionInfo = SKILL_ATTACK_MELEE + (*g_pActionInfoTable).GetMinResultActionInfo();
		break;
	case SKILL_VILLAIN_LADY_GHOST_SHADOW :
		resultActionInfo = MAGIC_RAPID_GLIDING + (*g_pActionInfoTable).GetMinResultActionInfo();
		break;
#endif //__ONIBLA
	// SKILL_BLAZE_WALK_2 keeps its own hit, RESULT_SKILL_BLAZE_WALK_2 (blood splash)
	case SKILL_BLAZE_WALK :
		resultActionInfo = SKILL_BLAZE_WALK_ATTACK + (*g_pActionInfoTable).GetMinResultActionInfo();
		break;
	}
	
	if( resultActionInfo >= g_pActionInfoTable->GetSize() )
	{
		DEBUG_ADD_FORMAT("[Execute-ResultActionInfo] exceed resultAction %d/%d",nUsedActionInfo,
			resultActionInfo);
		return;
	}
	
	#ifdef OUTPUT_DEBUG
		if (g_pDebugMessage)
		{
			DEBUG_ADD_FORMAT("[Execute-ResultActionInfo] ID=%d, ai=%d", m_TargetID, resultActionInfo);
		}
	#endif

	// [새기술]

	
	pTargetCreature->SetEffectDelayFrame(resultActionInfo, m_DelayFrame );

 	if( pUserCreature != NULL)
 	{
		if(nUsedActionInfo == MAGIC_BLOODY_ZENITH)
			pUserCreature->SetBloodyZenith(8);
//		else if( nUsedActionInfo == SKILL_SET_AFIRE)
//			pUserCreature->SetBloodyZenith(20);
//		pUserCreature->SetActionDelay( 16 );
//		if( pUserCreature == g_pPlayer )
//			g_pPlayer->SetDelay( 2000 );
 	}
	
	if(nUsedActionInfo == SKILL_BLITZ_SLIDING_ATTACK )
	{
		if( pUserCreature != NULL )
		{
			pUserCreature->PacketSpecialActionResult( resultActionInfo, m_TargetID, m_sX, m_sY );
		}
	}
	else
	{
		if( (*g_pActionInfoTable)[nUsedActionInfo].IsUseActionGrade() && m_Temp != 0 )
			pTargetCreature->PacketSpecialActionResult( resultActionInfo, m_TargetID, m_sX, m_sY, m_Temp );
		else
			pTargetCreature->PacketSpecialActionResult( resultActionInfo, m_TargetID, m_sX, m_sY);
	}
	
	//--------------------------------------------------------
	// 기술의 동작에 맞는 sound를 출력해준다.
	//--------------------------------------------------------
	//g_Sound.Play( g_SoundTable[(*g_pActionInfoTable)[nUsedActionInfo].GetSoundID()].pDSBuffer );
	//PlaySound( (*g_pActionInfoTable)[nUsedActionInfo].GetSoundID(),
	//			false,
	//			pTargetCreature->GetX(),
	//			pTargetCreature->GetY());

	//--------------------------------------------------------
	// Effect붙이는 결과이면 ..
	//--------------------------------------------------------
	/*
	EFFECTSTATUS	status		= (*g_pActionInfoTable)[resultActionInfo].GetEffectStatus();

	if (status!=EFFECTSTATUS_NULL)
	{
		// 시간이 남아 있는 경우에만...
		if (m_DelayFrame > 0)
		{
			pTargetCreature->AddEffectStatus( status, m_DelayFrame );
		}
	}
	*/
}


//----------------------------------------------------------------------
//
// ActionResultNode - ChangePosition
//
//----------------------------------------------------------------------
MActionResultNodeChangePosition::MActionResultNodeChangePosition(TYPE_OBJECTID uid, TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
{ 
	m_UserID	= uid;
	m_X			= sX;
	m_Y			= sY;
}

//----------------------------------------------------------------------
// ActionResultNode - ChangePosition::Execute
//----------------------------------------------------------------------
void
MActionResultNodeChangePosition::Execute()
{
	if (g_pPlayer==NULL
		|| g_pZone==NULL)
	{
		return;
	}

	//-----------------------------------------------------------
	// Player인 경우
	//-----------------------------------------------------------
	if (m_UserID==g_pPlayer->GetID())
	{
		g_pPlayer->MovePosition( m_X, m_Y );
		g_pPlayer->SetServerPosition( m_X, m_Y );
	}
	//-----------------------------------------------------------
	// 다른 애들인 경우
	//-----------------------------------------------------------
	else
	{
		MCreature* pCreature = g_pZone->GetCreature( m_UserID );

		// player인 경우를 따로 체크해야 할까??

		if (pCreature!=NULL)
		{
			//------------------------------------------------
			// 기존의 MoveBuffer를 다 제거시킨다.
			//------------------------------------------------
			pCreature->ReleaseMoveBuffer();

			// 좌표를 바꾼다.
			pCreature->MovePosition( m_X, m_Y );
			pCreature->SetServerPosition( m_X, m_Y );
			//pCreature->SetStop();
		}
	}
}

//----------------------------------------------------------------------
//
// ActionResultNode - ChangeStatus
//
//----------------------------------------------------------------------
MActionResultNodeChangeStatus::MActionResultNodeChangeStatus(TYPE_OBJECTID tid, MStatus* pStatus)
{
	m_TargetID = tid;
	m_pStatus = pStatus;
}

MActionResultNodeChangeStatus::~MActionResultNodeChangeStatus()
{
	if (m_pStatus != NULL)
	{
		delete m_pStatus;
	}
		
}

//----------------------------------------------------------------------
// ActionResultNode - ChangeStatus::Execute
//----------------------------------------------------------------------
void
MActionResultNodeChangeStatus::Execute()
{
	if (g_pPlayer==NULL)
	{
		return;
	}

	//---------------------------------------------------------
	// Player만 MStatus를 가지고 있다.. - -;;
	// 그렇다면.. m_TargetID는 필요없는데.. 음.. - -;; 
	//---------------------------------------------------------
	if (m_TargetID==g_pPlayer->GetID()
		&& m_pStatus!=NULL)
	{
		g_pPlayer->ApplyStatus( *m_pStatus );		
	}
}

//----------------------------------------------------------------------
// ActionResultNodeCreatureDie - CreatureDie::Execute
//----------------------------------------------------------------------
void
MActionResultNodeCreatureDie::Execute()
{
	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

	//---------------------------------------------------------
	// Player 인 경우
	//---------------------------------------------------------
	if (m_TargetID == g_pPlayer->GetID())
	{
		g_pPlayer->SetDead();
	}
	//---------------------------------------------------------
	// 다른 Creature인 경우
	//---------------------------------------------------------
	else
	{
		MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );

		//---------------------------------------------------------
		// 캐릭터가 없으면... 의미없겠지..
		//---------------------------------------------------------
		if (pCreature==NULL)
		{
			return;
		}
		
		//---------------------------------------------------------
		// 죽인다. - -;
		//---------------------------------------------------------		
		pCreature->SetDead();	
	}
}

//----------------------------------------------------------------------
// ActionResultNodeCreatureBurrow - CreatureBurrow::Execute
//----------------------------------------------------------------------
void
MActionResultNodeCreatureBurrow::Execute()
{
	if (g_pPlayer==NULL
		|| g_pZone==NULL)
	{
		return;
	}

	//---------------------------------------------------------
	// Player 인 경우
	//---------------------------------------------------------
	if (m_TargetID == g_pPlayer->GetID())
	{
		// Underground Creature로 만든다.
		g_pPlayer->SetUndergroundCreature();

		// [도움말] burrow
//		__BEGIN_HELP_EVENT
//			ExecuteHelpEvent( HE_EFFECT_BURROW );
//		__END_HELP_EVENT
	}
	//---------------------------------------------------------
	// 다른 Creature인 경우
	//---------------------------------------------------------
	else
	{
		MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );

		//---------------------------------------------------------
		// 캐릭터가 없으면... 의미없겠지..
		//---------------------------------------------------------
		if (pCreature==NULL)
		{
			return;
		}
		
		//---------------------------------------------------------
		// 숨긴다. 
		//---------------------------------------------------------		
		pCreature->SetUndergroundCreature();
	}
}

//----------------------------------------------------------------------
// ActionResultNodeCreatureInvisible - CreatureInvisible::Execute
//----------------------------------------------------------------------
void
MActionResultNodeCreatureInvisible::Execute()
{
	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

	//---------------------------------------------------------
	// Player 인 경우
	//---------------------------------------------------------
	if (m_TargetID == g_pPlayer->GetID())
	{
		// Underground Creature로 만든다.
		g_pPlayer->SetInvisible();

		// [도움말] burrow
//		__BEGIN_HELP_EVENT
//			ExecuteHelpEvent( HE_EFFECT_BURROW );
//		__END_HELP_EVENT
	}
	//---------------------------------------------------------
	// 다른 Creature인 경우
	//---------------------------------------------------------
	else
	{
		MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );

		//---------------------------------------------------------
		// 캐릭터가 없으면... 의미없겠지..
		//---------------------------------------------------------
		if (pCreature==NULL)
		{
			return;
		}
		
		//---------------------------------------------------------
		// invisible
		//---------------------------------------------------------		
		pCreature->SetInvisible();
	}
}

//----------------------------------------------------------------------
// ActionResultNodeCreatureVisible - CreatureVisible::Execute
//----------------------------------------------------------------------
void
MActionResultNodeCreatureVisible::Execute()
{
	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

	//---------------------------------------------------------
	// Player 인 경우
	//---------------------------------------------------------
	if (m_TargetID == g_pPlayer->GetID())
	{
		// Underground Creature로 만든다.
		g_pPlayer->SetVisible();

		// [도움말] burrow
//		__BEGIN_HELP_EVENT
//			//ExecuteHelpEvent( HE_EFFECT_BURROW );
//		__END_HELP_EVENT
	}
	//---------------------------------------------------------
	// 다른 Creature인 경우
	//---------------------------------------------------------
	else
	{
		MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );

		//---------------------------------------------------------
		// 캐릭터가 없으면... 의미없겠지..
		//---------------------------------------------------------
		if (pCreature==NULL)
		{
			return;
		}
		
		//---------------------------------------------------------
		// invisible
		//---------------------------------------------------------		
		pCreature->SetVisible();
	}
}

//----------------------------------------------------------------------
// ActionResultNodeCreatureSnipping - CreatureSnipping::Execute
//----------------------------------------------------------------------
void
MActionResultNodeCreatureSnipping::Execute()
{
	#ifdef OUTPUT_DEBUG
		if (g_pDebugMessage)
		{
			DEBUG_ADD_FORMAT("[Execute-CreatureSnipping] ID=%d", m_TargetID);
		}
	#endif

	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

	//---------------------------------------------------------
	// Player 인 경우
	//---------------------------------------------------------
	if (m_TargetID == g_pPlayer->GetID())
	{		
		g_pPlayer->AddEffectStatus( EFFECTSTATUS_SNIPPING_MODE, 0xFFFF );
		g_pPlayer->SetInvisible();

		// [도움말] burrow
//		__BEGIN_HELP_EVENT
//			//ExecuteHelpEvent( HE_EFFECT_BURROW );
//		__END_HELP_EVENT
	}
	//---------------------------------------------------------
	// 다른 Creature인 경우
	//---------------------------------------------------------
	else
	{
		MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );

		//---------------------------------------------------------
		// 캐릭터가 없으면... 의미없겠지..
		//---------------------------------------------------------
		if (pCreature==NULL)
		{
			return;
		}
		
		//---------------------------------------------------------
		// invisible
		//---------------------------------------------------------		
		pCreature->AddEffectStatus( EFFECTSTATUS_SNIPPING_MODE, 0xFFFF );
		pCreature->SetInvisible();
	}
}

//----------------------------------------------------------------------
// ActionResultNodeCreatureTurning - CreatureTurning::Execute
//----------------------------------------------------------------------
void
MActionResultNodeCreatureTurning::Execute()
{
	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

	//---------------------------------------------------------
	// Player 인 경우
	//---------------------------------------------------------
	if (m_TargetID == g_pPlayer->GetID())
	{
		// Underground Creature로 만든다.
		g_pPlayer->SetTurning( m_TurnFrame );
	}
	//---------------------------------------------------------
	// 다른 Creature인 경우
	//---------------------------------------------------------
	else
	{
		MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );

		//---------------------------------------------------------
		// 캐릭터가 없으면... 의미없겠지..
		//---------------------------------------------------------
		if (pCreature==NULL)
		{
			return;
		}
		
		//---------------------------------------------------------
		// invisible
		//---------------------------------------------------------		
		pCreature->SetTurning( m_TurnFrame );
	}
}

//----------------------------------------------------------------------
// ActionResultNodeAddEffectStatus
//----------------------------------------------------------------------
void
MActionResultNodeAddEffectStatus::Execute()
{
	if (g_pZone==NULL)
	{
		return;
	}

	MCreature*	pTargetCreature = g_pZone->GetCreature( m_TargetID );

	// 캐릭터가 없으면... 끝~
	if (pTargetCreature==NULL)// || pUserCreature==NULL)
		return;

	//--------------------------------------------------------
	// EffectStatus를 붙인다.
	//--------------------------------------------------------
	pTargetCreature->AddEffectStatus(m_EffectStatus, m_DelayFrame);	
}

//----------------------------------------------------------------------
//
// ActionResultNode - ChangeCreatureType
//
//----------------------------------------------------------------------
MActionResultNodeChangeCreatureType::MActionResultNodeChangeCreatureType(TYPE_OBJECTID tid, int ctype)
{
	m_TargetID = tid;
	m_CreatureType = ctype;
}

MActionResultNodeChangeCreatureType::~MActionResultNodeChangeCreatureType()
{
}

//----------------------------------------------------------------------
// ActionResultNode - ChangeStatus::Execute
//----------------------------------------------------------------------
void
MActionResultNodeChangeCreatureType::Execute()
{
	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

	MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );
	
	// 캐릭터가 없으면... 끝~
	if (pCreature==NULL)
		return;


	if (pCreature->IsUndergroundCreature())
	{
		// 그냥 묻어둔다. - -;
	}
	else
	{
		// 성별을 다시 설정해줘야 한다.
		bool bMale = pCreature->IsMale();
		WORD cs1 = pCreature->GetBodyColor1();
		WORD cs2 = pCreature->GetBodyColor2();

		pCreature->SetCreatureType( m_CreatureType );

		pCreature->SetMale( bMale );
		if( m_CreatureType == CREATURETYPE_WER_WOLF )
			pCreature->SetBodyColor1( cs2 );
		else
			pCreature->SetBodyColor1( cs1 );

		pCreature->SetBodyColor2( cs2 );

		// 무기 차고 있는지 체크
		MCreatureWear *pCreatureWear = dynamic_cast<MCreatureWear*>(pCreature);

		if(pCreatureWear)
		{
			MItem *pCoatItem = g_pVampireGear->GetItem(MVampireGear::GEAR_VAMPIRE_COAT);
			if(pCoatItem)
			{
//20080904 변신,변신해제 용으로 뱀파의 경우 크리처와 뱀파 팩이 나뉨으로 해서 튕길수 있다
//따라서 적용 불가능한 아이템은 외형 적용되 되지 않으므로 걍 벗겨 버린다. 
//아래 코드만으론 남에게는 보이지 않으며 재장비 할경우 자기 자신에게만 보인다.
//add start
				if(!pCoatItem->IsAffectStatus())
				{
					pCreatureWear->RemoveAddon(ADDON_COAT);
				}
//add end				
				pCreatureWear->SetAddonItem(pCoatItem);
			}

			MItem *pRightItem = g_pVampireGear->GetItem(MVampireGear::GEAR_VAMPIRE_RIGHTHAND);

			if(pRightItem && pRightItem->GetItemClass() == ITEM_CLASS_VAMPIRE_WEAPON)
			{
				pCreatureWear->SetAddonItem(pRightItem);
			}
		}

		// 이동방법에 따라..
		if ((*g_pCreatureTable)[m_CreatureType].IsFlyingCreature())
		{
			pCreature->SetFlyingCreature();
		}
		else
		{
			pCreature->SetGroundCreature();
		}
	}

	// player인 경우는 skill취소
	if (m_TargetID == g_pPlayer->GetID())
	{
		int selectSkill = ACTIONINFO_NULL;

		switch (m_CreatureType)
		{
			case CREATURETYPE_BAT :
			case CREATURETYPE_VAMPIRE_GHOST :
			case CREATURETYPE_FLITTERMOUSE :
#if __CONTENTS(__SECOND_TRANSFORTER)
			case CREATURETYPE_SHAPE_OF_DEMON :
#endif //__SECOND_TRANSFORTER
				selectSkill = MAGIC_UN_TRANSFORM;
			break;
		
			case CREATURETYPE_WOLF :
				selectSkill = MAGIC_EAT_CORPSE;
			break;

			case CREATURETYPE_WER_WOLF :
				selectSkill = MAGIC_UN_TRANSFORM;
			break;
			
			// 2004, 9, 13, sobeit add start - 총슬 130 skill
			case CREATURETYPE_INSTALL_TURRET:
				selectSkill = MAGIC_UN_TRANSFORM;
			break;
			// 2004, 9, 13, sobeit add end
		}
		
		if (selectSkill!=ACTIONINFO_NULL)
		{
			gC_vs_ui.SelectSkill( selectSkill );
		}
		else
		{
			gC_vs_ui.UnselectSkill();
		}

		// 이전에 설정해둔거 없앤다.
//		if (g_MorphCreatureType==m_CreatureType)
		{
			g_MorphCreatureType = 0;
		}
	}
}


//----------------------------------------------------------------------
// [새기술]
//----------------------------------------------------------------------
MActionResultNodeSummonCasket::MActionResultNodeSummonCasket(TYPE_OBJECTID tid, int casketType)
{
	m_TargetID = tid;
	m_CasketType = casketType;
}

MActionResultNodeSummonCasket::~MActionResultNodeSummonCasket()
{
}

//----------------------------------------------------------------------
// ActionResultNode - SummonCasket::Execute
//----------------------------------------------------------------------
void
MActionResultNodeSummonCasket::Execute()
{
	if (g_pZone==NULL)
	{
		return;
	}

	MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );
	
	// 캐릭터가 없으면... 끝~
	if (pCreature==NULL)
		return;

	//
	if (!pCreature->IsInCasket())
	{
		pCreature->SetStop();
		pCreature->MovePosition( pCreature->GetServerX(), pCreature->GetServerY() );
		pCreature->AddCasket( m_CasketType );
	}
}

//----------------------------------------------------------------------
// ActionResultNode - OpenCasket::Execute
//----------------------------------------------------------------------
void
MActionResultNodeOpenCasket::Execute()
{
	if (g_pZone==NULL)
	{
		return;
	}

	MCreature*	pCreature = g_pZone->GetCreature( m_TargetID );
	
	// 캐릭터가 없으면... 끝~
	if (pCreature==NULL)
		return;

	if (pCreature->IsInCasket())
	{
		pCreature->RemoveCasket();
	}
}

//----------------------------------------------------------------------
// ActionResultNode - OpenCasket::Execute
//----------------------------------------------------------------------
void
MActionResultNodeRemoveItemInInventory::Execute()
{
	if (g_pInventory==NULL)
	{
		return;
	}

	MItem* pRemovedItem = g_pInventory->RemoveItem( m_TargetID );
	SAFE_DELETE( pRemovedItem );

	MItem*	pMouseItem = NULL;

	if(g_pPlayer->IsItemCheckBufferItemToItem())
	{

		pMouseItem = g_pPlayer->GetItemCheckBuffer();

		g_pPlayer->ClearItemCheckBuffer();

		if(pMouseItem != NULL)
			delete pMouseItem;
	}
	
}

//----------------------------------------------------------------------
// ActionResultNode - OpenCasket::Execute
//----------------------------------------------------------------------
void
MActionResultNodeChangeItemOptionInInventory::Execute()
{
	if (g_pInventory==NULL)
	{
		return;
	}

	MItem*	pInvenItem = g_pInventory->GetItem(m_TargetID);
	MItem*	pMouseItem = NULL;

	bool bSecondPetEnchant = false;
	bool bRevivalPet = false;
	bool bRemovePetOption = false;
	bool bGradePlus = false;
	bool bPetNoChange = false;
	bool bGradeMinus = false;
	bool bItemOptChange	= false;

#if __CONTENTS(__TUNING_ITEM)
	bool bItemTuning = false;
#endif //__TUNING_ITEM

	if(g_pPlayer->IsItemCheckBufferItemToItem())
	{

		pMouseItem = g_pPlayer->GetItemCheckBuffer();

		g_pPlayer->ClearItemCheckBuffer();

		if(pMouseItem != NULL)
		{
			if(pMouseItem->GetItemClass() == ITEM_CLASS_PET_ENCHANT_ITEM)
			{
				if(pMouseItem->GetItemType() == 13 ||
					pMouseItem->GetItemType() == 14)	// 펫 부활 짜잔~
				{
					bRevivalPet = true;
				}
				else
				if(pMouseItem->GetItemType() == 15 ||	// 레드 버드
				   pMouseItem->GetItemType() == 26)		// 레드 버드 2
				{
					bSecondPetEnchant = true;
				}
				else
				if(pMouseItem->GetItemType() >= 27 &&	// 캡슐
				   pMouseItem->GetItemType() <= 29)
				{
					bPetNoChange = true;
				}
			}
			if(pMouseItem->GetItemClass() == ITEM_CLASS_EVENT_STAR && pMouseItem->GetItemType() >=54 && 
				pMouseItem->GetItemType() <=56)
			{
				bGradeMinus = true;
			}

#if __CONTENTS(__IMI_NEW_ENCHANT_ITEM)
			if(pMouseItem->GetItemClass() == ITEM_CLASS_EVENT_STAR && pMouseItem->GetItemType() >=70 && 
				pMouseItem->GetItemType() <=72)
			{
				bGradeMinus = true;
			}
#endif //__IMI_NEW_ENCHANT_ITEM

			if(pMouseItem->GetItemClass() == ITEM_CLASS_MIXING_ITEM &&
				((pMouseItem->GetItemType() >= 9 && pMouseItem->GetItemType() <= 18) || pMouseItem->GetItemType() == 25))
			{
				bRemovePetOption = true;
			}

			if(pMouseItem->GetItemClass() == ITEM_CLASS_EVENT_STAR && 
				(pMouseItem->GetItemType() == 22 || pMouseItem->GetItemType() == 48 ))// 옐로우 드롭
			{
				bGradePlus = true;
			}
			if(pMouseItem->GetItemClass() == ITEM_CLASS_EVENT_STAR && (pMouseItem->GetItemType() == 57 || pMouseItem->GetItemType() == 69))
			{		//옐로우 드롭 3
				bGradePlus = true;
			}
#if __CONTENTS(__2008_FIRST_CHARGE_ITEM)
			if(pMouseItem->GetItemClass() == ITEM_CLASS_EVENT_STAR && (pMouseItem->GetItemType() == 58 || pMouseItem->GetItemType() == 68))
			{		//크리스탈
				bItemOptChange = true;
			}
#endif	//__2008_FIRST_CHARGE_ITEM
			
#if __CONTENTS(__TUNING_ITEM)
			if(pMouseItem->IsTuningItem())
			{
				bItemTuning = true;
			}
#endif	//__TUNING_ITEM

			delete pMouseItem;
		}
	}
	
	// 아이템이 없으면... 끝~
	if (pInvenItem==NULL)
		return;

	bool bSucces = false;

	if(bRemovePetOption)
	{
		pInvenItem->ClearItemOption();
		bSucces = true;
	}
	else if(bGradePlus)
	{
		pInvenItem->SetGrade(m_Type);
		bSucces = true;
	}
	else if(bGradeMinus)
	{
		pInvenItem->SetGrade(m_Type);
		bSucces = false;
	}

	else if(pInvenItem->GetItemClass() == ITEM_CLASS_PET_ITEM)
	{
		if(bPetNoChange)
		{
			bSucces = true;
		}
		else
		if(bRevivalPet == true)
		{
			pInvenItem->SetCurrentDurability(m_Type);

			bSucces = true;
		}
		else
		if(bSecondPetEnchant == false && pMouseItem->GetItemType() != 24)
		{
			// 펫 1차 인첸트 필살! 속성 붙이기
#if __CONTENTS(!__PET_VISION_AMPLE)
			WORD AttrType = HIWORD(m_Type);
			WORD AttrLevel = LOWORD(m_Type);
			pInvenItem->SetEnchantLevel(AttrType);	// Attr Type
			pInvenItem->SetSilver(AttrLevel);	// AttrLevel
			pInvenItem->SetItemColorSet(0xFFFF);
#endif //__PET_VISION_AMPLE
			bSucces = true;
		}
		else
		{
			// 펫 2차 인첸트 옵션 붙이기, 아래 코드 그대로 써도 될거 같은데 양군이 쫄랐다.-ㅅ-
#if __CONTENTS(!__PET_VISION_AMPLE)
			pInvenItem->ClearItemOption();	// 일단 머가 있을지 모르니 지우고 본다-0-
			pInvenItem->AddItemOption(m_Type);
#endif //__PET_VISION_AMPLE
			bSucces = true;
		}
	}
	else if(bItemOptChange)
	{
		pInvenItem->ClearItemOption();
		pInvenItem->AddItemOption(m_Type);
		bSucces	= true;
	}
#if __CONTENTS(__TUNING_ITEM)
	else if(bItemTuning)
	{
		pInvenItem->SetItemType(m_Type);
		bSucces	= true;
	}
#endif //__TUNING_ITEM
	else
	{

		DWORD Option = m_Type;
	//	int Option2 = LOWORD(m_Type);

	//	if(oriOption < newOption)
		
		for(; Option != NULL; Option >>= 16)	// 2바이트 쉬프트 하면 LOWORD, HIWORD순으로 되나
		{
			int oriOption = (Option & 0xff00) >> 8;
			int newOption = (Option & 0xff);

			if(newOption == NULL && oriOption == NULL)
				continue;

	//			continue;
			
			if(oriOption)
				pInvenItem->ChangeItemOption(oriOption, newOption);
			else
				pInvenItem->AddItemOption(newOption);

			if((*g_pItemOptionTable)[oriOption].UpgradeOptionType == newOption
				|| newOption != 0 && ITEMOPTION_TABLE::GetThirdUpgradeOptionType(oriOption) == newOption)
				bSucces |= true;
		}
	}

	if(bSucces)
		PlaySound(SOUND_EVENT_FANFARE);

}

MActionResultNodeChangeItemThirdOptionInInventory::MActionResultNodeChangeItemThirdOptionInInventory(tag_ThirdOptionData* type, TYPE_OBJECTID tid)
{
	m_TargetID = tid;
	m_ThirdOptionList = *(type->thirdOptionList);
	m_ThirdOptionType	 = type->thirdOptionType;
}

void
MActionResultNodeChangeItemThirdOptionInInventory::SetOption(tag_ThirdOptionData* type)
{
	m_ThirdOptionList = *(type->thirdOptionList);
	m_ThirdOptionType	 = type->thirdOptionType;
}


void 
MActionResultNodeChangeItemThirdOptionInInventory::Execute()
{
	if (g_pInventory==NULL)
	{
		return;
	}

	MItem*	pInvenItem = g_pInventory->GetItem(m_TargetID);
	MItem*	pMouseItem = NULL;

	//카피엔 패이스트에 붙어 왔다. 나중에 필요 할수도 있으니 걍 둔다.
	bool bSecondPetEnchant = false;
	bool bRevivalPet = false;
	bool bRemovePetOption = false;
	bool bGradePlus = false;
	bool bPetNoChange = false;
	bool bThirdOptionSecces = false;

	if(m_ThirdOptionList.size())
	{
		pInvenItem->ChangeItemThirdOption(m_ThirdOptionList);
		pInvenItem->SetItemThirdOptionType(m_ThirdOptionType);
		bThirdOptionSecces = true;
	}
	else
	{
		pInvenItem->ClearItemThirdOption();
		pInvenItem->SetItemThirdOptionType(99);
	}
	if(bThirdOptionSecces)
	{
		PlaySound(SOUND_EVENT_FANFARE);
	}
}
//----------------------------------------------------------------------
// ActionResultNodeFakeDie - FakeDie::Execute
//----------------------------------------------------------------------
void
MActionResultNodeFakeDie::Execute()
{
	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

}

//----------------------------------------------------------------------
// ActionResultNodeEvent - Event::Execute
//----------------------------------------------------------------------
void
MActionResultNodeEvent::Execute()
{
	if (g_pZone==NULL
		|| g_pPlayer==NULL)
	{
		return;
	}

}

//----------------------------------------------------------------------
// 
// MActionResult :: constructor/destructor
//
//----------------------------------------------------------------------
MActionResult::MActionResult()
{
}

MActionResult::~MActionResult()
{
	ACTIONRESULTNODE_LIST::iterator	iNode = m_List.begin();

	// 모든 node를 delete해준다.
	while (iNode != m_List.end())
	{
		MActionResultNode* pResultNode = *iNode;
		
		// 결과 실행..
		//pResultNode->Execute();

		delete pResultNode;
		
		iNode++;
	}

	m_List.clear();
}

//----------------------------------------------------------------------
//
// member functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Release
//----------------------------------------------------------------------
void
MActionResult::Release()
{
	ACTIONRESULTNODE_LIST::iterator	iNode = m_List.begin();

	// 모든 node를 delete해준다.
	while (iNode != m_List.end())
	{
		MActionResultNode* pResultNode = *iNode;
		
		delete pResultNode;
		
		iNode++;
	}

	m_List.clear();
}

//----------------------------------------------------------------------
// Add : 결과 하나를 추가한다.
//----------------------------------------------------------------------
void		
MActionResult::Add(MActionResultNode* pNode)
{
	if (pNode==NULL)
		return;

	// list에 추가
	m_List.push_back( pNode );
}

//----------------------------------------------------------------------
// ExecuteResult
//----------------------------------------------------------------------
// ActionInfo(Effect)에 따른 결과를 실행한다.
//----------------------------------------------------------------------
void
MActionResult::Execute()
{
	//------------------------------------------------
	// 모두~~ 처리한다.
	//------------------------------------------------
	while (!m_List.empty())
	{	
		MActionResultNode* pResultNode = m_List.front();
		
		m_List.pop_front();

		if (pResultNode!=NULL)
		{
			pResultNode->Execute();

			delete pResultNode;
		}
	}
}

//----------------------------------------------------------------------
// ActionResultNode - OpenCasket::Execute
//----------------------------------------------------------------------
void
MActionResultNodeChangeItemGenderInInventory::Execute()
{
	if (g_pInventory==NULL)
	{
		return;
	}

	MItem*	pInvenItem = g_pInventory->GetItem(m_TargetID);
	MItem*	pMouseItem = NULL;

	if(g_pPlayer->IsItemCheckBufferItemToItem())
	{
		pMouseItem = g_pPlayer->GetItemCheckBuffer();

		g_pPlayer->ClearItemCheckBuffer();
		if(pMouseItem != NULL)
			delete pMouseItem;
	}
	
	// 아이템이 없으면... 끝~
	if (pInvenItem==NULL)
		return;

	pInvenItem->SetItemType( pInvenItem->GetItemType() ^ 0x1 );

	g_pInventory->CheckAffectStatus( pInvenItem );
	PlaySound(SOUND_EVENT_FANFARE);
}
