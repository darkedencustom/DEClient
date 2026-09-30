//#define __3D_IMAGE_OBJECT__					// by sonee

//----------------------------------------------------------------------
// MTopView.cpp
//----------------------------------------------------------------------
// [ Effect Layer ]  - ??? ?????? ????..
// 
//		-- Tile ???
//	* Tile?? ??? ???? Effect
//
// 
//	* Sector?? Effect			
//		-- ?????? Character ???
//	* Character?? ???? AttachEffect
// 
//----------------------------------------------------------------------
#include "Client_PCH.h"
#pragma warning(disable:4786)

#include <time.h>
#include <math.h>
#include <list>
#include <stdio.h>
#include <fstream>
#include <numeric>
#include <random>

#include "DX3D.h"
#include "MZone.h"
#include "MCreature.h"
#include "MFakeCreature.h"
#include "MPlayer.h"
#include "MItem.h"
#include "MImageObject.h"
#include "MAnimationObject.h"
#include "MShadowAnimationObject.h"
//#include "MInteractionObject.h"
#include "MMovingEffect.h"
#include "MScreenEffect.h"
#include "MTopView.h"
#include "MItemTable.h"
#include "MCreatureTable.h"
#include "MWeather.h"
#include "CMessageArray.h"
#include "DXLib.h"
#include "SP.h"
#include "FL2.h"
//#include "2D.h"
#include "ClientConfig.h"
#include "VS_UI_BASE.h"
#include "VS_UI_Mouse_Pointer.h"
#include "UIFunction.h"
#include "AddonDef.h"
#include "MZoneTable.h"
#include "MGameTime.h"
#include "MSlayerGear.h"
#include "MVampireGear.h"
#include "MOustersGear.h"
#include "UserOption.h"
#include "UserInformation.h"
#include "SpriteIDDef.h"
//#include "MWorkThread.h"
//#include "MLoadingSPKWorkNode.h"
#include "MGameStringTable.h"
//#include "MZoneInfo.h"
#include "MObjectSelector.h"
#include "MPortal.h"
#include "EffectSpriteTypeDef.h"
#include "MScreenEffectManager.h"
#include "MEffectSpriteTypeTable.h"
#include "RequestServerPlayerManager.h"
#include "RequestClientPlayerManager.h"
#include "MJusticeAttackManager.h"
#include "Profiler.h"
//#include "MFileDef.h"
#include "Properties.h"
#include "MGuildMarkManager.h"
#include "MEventManager.h"
#include "MAttachOrbitEffect.h"
#include "MTestDef.h"
#include "ServerInfo.h"
#include "DebugInfo.h"
#include "MTestDef.h"
#include "MItemOptionTable.h"
#include "MMonsterKillQuestInfo.h"
#include "Client.h"
#include "cmp3.h"
#include "CSprite555.h"
#include "CSprite565.h"
#include "SoundSetting.h"
#include "SystemAvailabilities.h"
#include "SkillDef.h"
#include "vs_ui_item.h"
#include "NicknameInfo.h"

#include "MTopViewDraw.inl"
#include "VS_UI_GameCommon2.h"

// The skill box on screen (VS_UI_GameCommon.cpp); game messages stack above it.
void g_GetSkillBoxRect(RECT* rect);

#include "OperatorOption.h"

#include "BGMManager.h"
//#undef OUTPUT_DEBUG

//#define	new			DEBUG_NEW
//#define	delete		DEBUG_DELETE

#if defined(OUTPUT_DEBUG)//defined(_DEBUG) || 
//	#define OUTPUT_DEBUG_DRAW_3DBOX
#ifdef __METROTECH_TEST__
//		#define OUTPUT_DEBUG_DRAW_PROCESS
//		#define OUTPUT_DEBUG_DRAW_PROCESS_INPUT
#endif
//	#define OUTPUT_DEBUG_DRAW_SHADOW
#endif

void	StartTitleLoading();
void	DrawTitleLoading();
void	EndTitleLoading(bool SendLogin = false);

#ifdef __METROTECH_TEST__
extern bool g_bLight;
#endif

extern void PlaySoundForce(TYPE_SOUNDID soundID);

extern HWND							g_hWnd;
extern POINT						g_SelectSector;
extern BOOL							g_bLButtonDown;
extern BOOL							g_bRButtonDown;
extern BOOL							g_bCButtonDown;
extern BOOL					g_bUIInput;
//extern MInput						g_Input;
//extern BOOL							g_WindowsNT;

extern int			g_nZoneLarge;
extern int			g_nZoneSmall;
extern bool			g_bZonePlayerInLarge;
//extern HANDLE		g_hFileThread;

extern CMessageArray* g_pSystemMessage;
extern CMessageArray* g_pGameMessage;
extern CMessageArray* g_pNoticeMessage;

extern MScreenEffectManager* g_pInventoryEffectManager;

extern bool FileOpenBinary(const char* filename, ivfstream& file);

//----------------------------------------------------------------------
// Global
//----------------------------------------------------------------------
MTopView* g_pTopView = NULL;
extern bool g_bFrameChanged;

bool g_bMouseInPortal = false;

// ??? ????.. ??????
POINT g_MouseSector = { 0, 0 };		// mouse?? ??????? sector???

extern int	g_x;
extern int	g_y;

extern int	g_UpdateDelay;

//extern MWorkThread*	g_pLoadingThread;

int g_ShowImageObjectID = 0;

// ???????
//std::map<int, bool> g_mapPremiumZone;

#ifdef OUTPUT_DEBUG
extern RECT g_rectScreenShot;

extern int gtx_op;
extern int gtx_src;
extern int gtx_dest;

MString g_ProfileInfoName("DrawZone");
#endif

//----------------------------------------------------------------------
// ?? filter????? ?????? ????..
//----------------------------------------------------------------------
float MTopView::s_LightWidth = 0.0f;//(float)CLIPSURFACE_WIDTH / SCREENLIGHT_WIDTH;
float MTopView::s_LightHeight = 0.0f;//(float)CLIPSURFACE_HEIGHT / SCREENLIGHT_HEIGHT;

//----------------------------------------------------------------------
//
// defines
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// ??ü Creature?? ??? ??????
//----------------------------------------------------------------------
#define	MAX_CREATURE_BODY		3
#define	MAX_CREATURE_ADDON		4
#define	MAX_ANIMATION			2
#define	MAX_ALPHAEFFECT			10
#define	MAX_NORMALEFFECT		1

//----------------------------------------------------------------------
// Light 3D?? ???? ??
//-----------------------------------------------------------------------
#define	MAX_LIGHT_SETSIZE		12

//-----------------------------------------------------------------------
// ??????????? ???? ???????? shift??
//-----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// FONT ????
//
//----------------------------------------------------------------------
enum FONTID
{
	FONTID_INFO,
	FONTID_SLAYER_NAME,
	FONTID_VAMPIRE_NAME,
	FONTID_NPC_NAME,
	FONTID_SLAYER_CHAT,
	FONTID_VAMPIRE_CHAT,
	FONTID_NPC_CHAT,
	FONTID_ITEM,
	FONTID_LARGE_CHAT,
	FONTID_USER_ID,
	FONTID_EVENTMSG,
	MAX_FONTID
};

PrintInfo* g_ClientPrintInfo[MAX_FONTID];

//----------------------------------------------------------------------
//
//		Text Comparison
//
//----------------------------------------------------------------------
// ?ð??? ??????? ??????? ???.		
// left < right ?? ???? return??
//
// ?? ?????? ????????? false??
// ?????? ?????? true?? return??? ???.
//
// ??? ???? ???? ??µ? ??????.. ?ð??? ???? ??(???? ??)?? ??? ???? ???.
// 
// PQ?? ?????? ??? ??????..
// ??????... ?ð??? ???? ??(???)?? ??? ???????.
bool
TextComparison::operator () (DRAWTEXT_NODE* left, DRAWTEXT_NODE* right) const
{
	// ???? ?????? ?????? ???.. OK
	// ?ð??? ?????? ??? ??? ?????. 
	return left->GetTextTime() >= right->GetTextTime();
}

inline		bool			IsAffectFromDarkness()
{
	if (g_pPlayer->IsVampire() ||
		(g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) ||
		g_pZone->GetID() == 3001)
		return false;

	return true;
}

//----------------------------------------------------------------------
//
// constructor/destructor
//
//----------------------------------------------------------------------
MTopView::MTopView()
{
	m_bInit = false;

	m_pZone = NULL;

	m_FirstSector.x = 0;
	m_FirstSector.y = 0;
	m_FirstZonePixel.x = 0;
	m_FirstZonePixel.y = 0;

	m_SelectSector.x = SECTORPOSITION_NULL;
	m_SelectSector.y = SECTORPOSITION_NULL;

	// ???õ? ???
	m_SelectCreatureID = OBJECTID_NULL;
	m_SelectItemID = OBJECTID_NULL;
	//	m_SelectInteractionObjectID	= OBJECTID_NULL;
	m_SelectEffectID = OBJECTID_NULL;
	//m_bSelectModeAttack			= false;

	//m_ppSurface			= NULL;

	// Tile Surface
	m_pTileSurface = NULL;
	m_bFirstTileDraw = true;
	m_TileSurfaceFirstZonePixelX = 0;
	m_TileSurfaceFirstZonePixelY = 0;


	//m_SelectSector.x = 0;
	//m_SelectSector.y = 0;

	// BlindEffect
	m_bBlindEffect = false;

	// ?þ?
	m_DarkBits = 0;

#if __CONTENTS(__BLITZ_COUPON_EVENT)
	m_UIEffect_Frame = 0;
#endif //__BLITZ_COUPON_EVENT

	// imageobject texture manager
#ifdef __3D_IMAGE_OBJECT__
//	m_pImageObjectTextureManager = NULL;
#endif

	// Texture PartManager
//	m_pAlphaEffectTextureManager = NULL;
//	m_pScreenEffectTextureManager = NULL;

	// Shadow PartManager
//	m_pImageObjectShadowManager = NULL;
//	m_pCreatureShadowManager = NULL;
//	m_pAddonShadowManager = NULL;
//	m_pOustersShadowManager = NULL;
//	m_pEffectShadowManager = NULL;

	// LightBuffer Texture
	m_pLightBufferTexture = NULL;

	// 3D Box Texture Surface
//	m_p3DBoxSurface = NULL;	
//	m_p3DBoxBlackSurface = NULL;	
//	m_p3DBoxHPSurface = NULL;	
//	m_3DBoxCurrentPixel =  0x7000;
//
//	// 2001.7.14 ???ó??
//	m_pMinimapSPR = NULL;	
//	m_pMinimapTexture = NULL;

	m_p2DLightPixelWidth = NULL;
	m_p2DLightPixelHeight = NULL;


	m_bDrawItemNameList = false;

	//
	m_pTileSFPArrayLargeZone = NULL;
	m_pImageObjectSFPArrayLargeZone = NULL;
	m_pTileSFPArraySmallZone = NULL;
	m_pImageObjectSFPArraySmallZone = NULL;

	m_bDrawRequest = false;

	//-----------------------------------------------
	// AlphBox?? g_pTopView->Init()???? ?????? ?????.
	//-----------------------------------------------
//	if (CDirect3D::IsHAL())
//	{
//		m_p3DBoxSurface = new CSpriteSurface;
//		m_p3DBoxSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
//
//		Set3DBoxColor( m_3DBoxCurrentPixel );
//
//		m_p3DBoxBlackSurface = new CSpriteSurface;
//		m_p3DBoxBlackSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
//		m_p3DBoxBlackSurface->Restore();
//
//		m_p3DBoxHPSurface = new CSpriteSurface;
//		m_p3DBoxHPSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
//		m_p3DBoxHPSurface->Restore();
//	}

//	m_bTileSearchForCreature = true;
	m_bTileSearchForCreature = false;

	// ???????
////	g_mapPremiumZone[31] = true;
////	g_mapPremiumZone[32] = true;
////	g_mapPremiumZone[33] = true;
////	g_mapPremiumZone[34] = true;
//	g_mapPremiumZone[51] = true;
//	g_mapPremiumZone[52] = true;
//	g_mapPremiumZone[53] = true;
//	g_mapPremiumZone[54] = true;
//	g_mapPremiumZone[61] = true;
//	g_mapPremiumZone[62] = true;
//	g_mapPremiumZone[63] = true;
//	g_mapPremiumZone[64] = true;
////	g_mapPremiumZone[1002] = true;
//	g_mapPremiumZone[1007] = true;
//	g_mapPremiumZone[1008] = true;
//	g_mapPremiumZone[1009] = true;
////	g_mapPremiumZone[1011] = true;
////	g_mapPremiumZone[1012] = true;
//	g_mapPremiumZone[1013] = true;
//	g_mapPremiumZone[1111] = true;
//	g_mapPremiumZone[1112] = true;
//	g_mapPremiumZone[1113] = true;
//	g_mapPremiumZone[1104] = true;
//	g_mapPremiumZone[1106] = true;
//	g_mapPremiumZone[1114] = true;
//	g_mapPremiumZone[1115] = true;
#if __CONTENTS(__TITLE_UI_RENWEAL)
#endif // __TITLE_UI_RENWEAL
	InitMapingExceptionProc();
}


MTopView::~MTopView()
{
	Release();
}

//----------------------------------------------------------------------
//
// member functions
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Init
//----------------------------------------------------------------------
// View?? ????? ?? ?????? ???? ?????.
//----------------------------------------------------------------------
bool
MTopView::Init()
{
	//------------------------------------------
	// ??? init???? init???? ??´?.
	//------------------------------------------
	if (m_bInit)
	{
		return false;
	}

	//----------------------------------------------------------------------
	// ?? filter????? ?????? ????..
	//----------------------------------------------------------------------
	s_LightWidth = CLIPSURFACE_WIDTH / (float)SCREENLIGHT_WIDTH;
	s_LightHeight = CLIPSURFACE_HEIGHT / (float)SCREENLIGHT_HEIGHT;


	//------------------------------------------
	// ?????? Texture Memory ???
	//------------------------------------------
	DDSCAPS2 ddsCaps2;
	DWORD dwTotal;
	DWORD dwFree;
	ZeroMemory(&ddsCaps2, sizeof(ddsCaps2));
	ddsCaps2.dwCaps = DDSCAPS_TEXTURE;
	HRESULT hr = CDirectDraw::GetDD()->GetAvailableVidMem(&ddsCaps2, &dwTotal, &dwFree);

	DEBUG_ADD_FORMAT("[TextureMemory] Before Init View = %d/%d", dwFree, dwTotal);

	//------------------------------------------
	// ?????? effect texture????
	//------------------------------------------
	// 1500000?????? ????? ?????? ????.
	// ??????? ?? Texture?? memory size = 65536 - -;;
	//------------------------------------------
	int freeMemory = dwFree - 1500000;

	if (freeMemory < 0) freeMemory = 0;

	int num = freeMemory / 20000 / 12;

	num = max(num, 30);	// 30????? ??????.
	num = min(num, 60);	// 60?? ?????.

	//num = 20;

	// ????.. - -;
	//g_pClientConfig->MAX_TEXTUREPART_EFFECT				= num*3/2;
	//g_pClientConfig->MAX_TEXTUREPART_CREATURESHADOW		= num*3;
	//g_pClientConfig->MAX_TEXTUREPART_IMAGEOBJECTSHADOW	= num;
	//g_pClientConfig->MAX_TEXTUREPART_ADDON_SHADOW			= 240 + num*7;

	//------------------------------------------------------------------------
	// ????? ?????? ?????? ??? ??????? ???? ???? ???
	//------------------------------------------------------------------------
	g_pClientConfig->MAX_TEXTUREPART_EFFECT = num * 2;	//num*10;
	g_pClientConfig->MAX_TEXTUREPART_SCREENEFFECT = num * 5;		//num*10;
	g_pClientConfig->MAX_TEXTUREPART_CREATURESHADOW = 1;//num*3;
	g_pClientConfig->MAX_TEXTUREPART_IMAGEOBJECTSHADOW = num * 2;
	g_pClientConfig->MAX_TEXTUREPART_ADDON_SHADOW = 1;//240 + num*7;

	DEBUG_ADD_FORMAT("[MTopView::TexturePart] AlphaEffect = %d", g_pClientConfig->MAX_TEXTUREPART_EFFECT);
	DEBUG_ADD_FORMAT("[MTopView::TexturePart] ScreenEffect = %d", g_pClientConfig->MAX_TEXTUREPART_SCREENEFFECT);
	DEBUG_ADD_FORMAT("[MTopView::TexturePart] CreatureShadow = %d", g_pClientConfig->MAX_TEXTUREPART_CREATURESHADOW);
	DEBUG_ADD_FORMAT("[MTopView::TexturePart] ImageObjectShadow = %d", g_pClientConfig->MAX_TEXTUREPART_IMAGEOBJECTSHADOW);
	DEBUG_ADD_FORMAT("[MTopView::TexturePart] AddonShadow = %d", g_pClientConfig->MAX_TEXTUREPART_ADDON_SHADOW);



	if (InitSurfaces() &&
		InitColors() &&
		//		Init3DBoxSurface() &&
		InitSprites() &&
		InitFilters() &&
		InitCreatureFrames() &&
		InitImageFrames() &&
		InitAnimationFrames() &&
		InitEffectFrames() &&
		InitFonts())
	{
		//------------------------------------------
		// ?????? Texture Memory ???
		//------------------------------------------
#ifdef OUTPUT_DEBUG
		DDSCAPS2 ddsCaps2;
		DWORD dwTotal;
		DWORD dwFree;
		ZeroMemory(&ddsCaps2, sizeof(ddsCaps2));
		ddsCaps2.dwCaps = DDSCAPS_TEXTURE;
		HRESULT hr = CDirectDraw::GetDD()->GetAvailableVidMem(&ddsCaps2, &dwTotal, &dwFree);

		DEBUG_ADD_FORMAT("[TextureMemory] After Init View = %d/%d", dwFree, dwTotal);
#endif

		m_bInit = true;

		PrecalculateAdvancementClassCreatureFrames();

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Init Changes
//----------------------------------------------------------------------
// 2D <--> 3D ??? ???..
//----------------------------------------------------------------------
bool
MTopView::InitChanges()
{
	//------------------------------------------
	// ???? init???? ????? return
	//------------------------------------------
	if (!m_bInit)
	{
		return false;
	}

	if (InitSurfaces() &&
		InitColors() &&
		//		Init3DBoxSurface() &&
		InitSprites() &&
		InitFilters() &&
		InitFonts())
	{
		//---------------------------------------------------------
		// ????? loading?? ????? ????? ü?????.
		// ????? ???????.. ???.. - -; 2001.10.6
		//---------------------------------------------------------
		if (g_pCreatureSpriteTable != NULL)
		{
			int numSize = g_pCreatureSpriteTable->GetSize();

			for (int i = 0; i < numSize; i++)
			{
				(*g_pCreatureSpriteTable)[i].bLoad = false;
			}
		}

		//		if (g_pAddonSpriteTable!=NULL)
		//		{
		//			int numSize = g_pAddonSpriteTable->GetSize();
		//
		//			for (int i=0; i<numSize; i++)
		//			{
		//				(*g_pAddonSpriteTable)[i].bLoad = false;
		//			}
		//		}

		//		if (g_pCreatureActionSpriteTable!=NULL)
		//		{
		//			int numSize = g_pCreatureActionSpriteTable->GetSize();
		//
		//			for (int i=0; i<numSize; i++)
		//			{
		//				(*g_pCreatureActionSpriteTable)[i].bLoad = false;
		//			}
		//		}

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Init Fonts
//----------------------------------------------------------------------
bool
MTopView::InitFonts()
{
	//------------------------------------------
	// UI?? Font???
	//------------------------------------------	
	g_ClientPrintInfo[FONTID_INFO] = &gpC_base->m_info_pi;			// info
	g_ClientPrintInfo[FONTID_SLAYER_NAME] = &gpC_base->m_user_id_pi; //m_char_name_pi;	// slayer
	g_ClientPrintInfo[FONTID_VAMPIRE_NAME] = &gpC_base->m_user_id_pi; //m_char_name_pi;	// vampire
	g_ClientPrintInfo[FONTID_NPC_NAME] = &gpC_base->m_user_id_pi; // m_char_name_pi;	// npc
	g_ClientPrintInfo[FONTID_SLAYER_CHAT] = &gpC_base->m_char_chat_pi;	// slayer
	g_ClientPrintInfo[FONTID_VAMPIRE_CHAT] = &gpC_base->m_char_chat_pi;	// vampire
	g_ClientPrintInfo[FONTID_NPC_CHAT] = &gpC_base->m_char_chat_pi;	// npc
	g_ClientPrintInfo[FONTID_ITEM] = &gpC_base->m_item_pi;			// item
	g_ClientPrintInfo[FONTID_LARGE_CHAT] = &gpC_base->m_char_chat_large_pi;
	g_ClientPrintInfo[FONTID_USER_ID] = &gpC_base->m_user_id_pi;
	g_ClientPrintInfo[FONTID_EVENTMSG] = &gpC_base->m_char_chat_event_pi;
	return true;
}

//----------------------------------------------------------------------
// Release
//----------------------------------------------------------------------
void
MTopView::Release()
{
	//------------------------------------------------
	// File ???
	//------------------------------------------------
//	if (m_TileSPKFile.is_open())
//	{
//		m_TileSPKFile.close();
//	}
//
//	if (m_ImageObjectSPKFile.is_open())
//	{
//		m_ImageObjectSPKFile.close();
//	}
//
//	if (m_EffectAlphaSPKFile.is_open())
//	{
//		m_EffectAlphaSPKFile.close();
//	}
//
//	if (m_EffectScreenSPKFile.is_open())
//	{
//		m_EffectScreenSPKFile.close();
//	}
//
//	if (m_AddonSPKFile.is_open())
//	{
//		m_AddonSPKFile.close();
//	}
//
//	if (m_AddonSSPKFile.is_open())
//	{
//		m_AddonSSPKFile.close();
//	}
//
//	if (m_CreatureSPKFile.is_open())
//	{
//		m_CreatureSPKFile.close();
//	}
//
//	if (m_CreatureSSPKFile.is_open())
//	{
//		m_CreatureSSPKFile.close();
//	}
//
	DEBUG_ADD("MTV-Rel-File closed");

	//----------------------------------------------------------------------
	// Tile Surface????
	//----------------------------------------------------------------------
	if (m_pTileSurface != NULL)
	{
		delete m_pTileSurface;
		m_pTileSurface = NULL;

		DEBUG_ADD("MTV-Rel-TileSur");
	}

	// imageobject TextureManager
	//if (m_pImageObjectTextureManager!=NULL)
	//{
	//	delete m_pImageObjectTextureManager;
	//	m_pImageObjectTextureManager = NULL;
	//}
#ifdef __3D_IMAGE_OBJECT__
	SAFE_DELETE(m_pImageObjectTextureManager);
#endif

	DEBUG_ADD("MTV-Rel-StartRelTex");

	//----------------------------------------------------------------------
	// Texture Manager
	//----------------------------------------------------------------------
//	if (m_pAlphaEffectTextureManager!=NULL)
//	{
//		delete m_pAlphaEffectTextureManager;
//		m_pAlphaEffectTextureManager = NULL;
//		DEBUG_ADD("MTV-Rel-AlphaTex");
//	}	
//
//	if (m_pScreenEffectTextureManager!=NULL)
//	{
//		delete m_pScreenEffectTextureManager;
//		m_pScreenEffectTextureManager = NULL;
//		DEBUG_ADD("MTV-Rel-ScreenTex");
//	}	
//
//	//----------------------------------------------------------------------
//	// Shadow Manager
//	//----------------------------------------------------------------------
//	if (m_pEffectShadowManager!=NULL)
//	{
//		delete m_pEffectShadowManager;
//		m_pEffectShadowManager = NULL;
//		DEBUG_ADD("MTV-Rel-EffectShadow");
//	}	

//	if (m_pImageObjectShadowManager!=NULL)
//	{
//		delete m_pImageObjectShadowManager;
//		m_pImageObjectShadowManager = NULL;
//		DEBUG_ADD("MTV-Rel-IMShadow");
//	}	
//
//	if (m_pAddonShadowManager!=NULL)
//	{
//		delete m_pAddonShadowManager;
//		m_pAddonShadowManager = NULL;
//		DEBUG_ADD("MTV-Rel-AddonShadow");
//	}
//
//	if (m_pOustersShadowManager!=NULL)
//	{
//		delete m_pOustersShadowManager;
//		m_pOustersShadowManager = NULL;
//		DEBUG_ADD("MTV-Rel-OustersShadow");
//	}
//	
//	if (m_pCreatureShadowManager!=NULL)
//	{
//		delete m_pCreatureShadowManager;
//		m_pCreatureShadowManager = NULL;
//		DEBUG_ADD("MTV-Rel-CreatureShadow");
//	}

	//----------------------------------------------------------------------
	// LightBuffer Texture
	//----------------------------------------------------------------------
	if (m_pLightBufferTexture != NULL)
	{
		delete m_pLightBufferTexture;
		m_pLightBufferTexture = NULL;
		DEBUG_ADD("MTV-Rel-LightBuffer");
	}

	//----------------------------------------------------------------------
	// 3D Box Texture Surface
	//----------------------------------------------------------------------
//	if (m_p3DBoxSurface!=NULL)
//	{
//		delete m_p3DBoxSurface;
//		m_p3DBoxSurface = NULL;
//		DEBUG_ADD("MTV-Rel-3DBox");
//	}	
//
//	if (m_p3DBoxBlackSurface!=NULL)
//	{
//		delete m_p3DBoxBlackSurface;
//		m_p3DBoxBlackSurface = NULL;
//		DEBUG_ADD("MTV-Rel-3DBoxBl");
//	}	
//
//	if (m_p3DBoxHPSurface!=NULL)
//	{
//		delete m_p3DBoxHPSurface;
//		m_p3DBoxHPSurface = NULL;
//		DEBUG_ADD("MTV-Rel-3DBoxHP");
//	}	
//	
//	//----------------------------------------------------------------------
//	// Minimap
//	//----------------------------------------------------------------------
//	// 2001.7.14 ???ó??
//	if (m_pMinimapSPR != NULL)
//	{
//		delete m_pMinimapSPR;
//		m_pMinimapSPR = NULL;
//	}
//
//	if (m_pMinimapTexture != NULL)
//	{
//		delete m_pMinimapTexture;
//		m_pMinimapTexture = NULL;
//	}


	//----------------------------------------------------------------------
	// 2D Light Pixel
	//----------------------------------------------------------------------
	if (m_p2DLightPixelWidth != NULL)
	{
		delete[] m_p2DLightPixelWidth;
		m_p2DLightPixelWidth = NULL;
		DEBUG_ADD("MTV-Rel-2DLightPixelW");
	}

	if (m_p2DLightPixelHeight != NULL)
	{
		delete[] m_p2DLightPixelHeight;
		m_p2DLightPixelHeight = NULL;
		DEBUG_ADD("MTV-Rel-2DLightPixelH");
	}

	DEBUG_ADD("MTV-Rel-ClearList");

	//----------------------------------------------------------------------
	// list , map ????
	//----------------------------------------------------------------------
	ClearItemNameList();
	ClearOutputCreature();
	ClearTextList();

	DEBUG_ADD("MTV-Rel-Loaded");

	m_listLoadedCreatureSprite.Release();
	m_listLoadedMonsterSprite.Release();
	m_mapImageObject.clear();

	//----------------------------------------------------------------------
	// FPK ?????
	//----------------------------------------------------------------------
	DEBUG_ADD("MTV-Rel-FPK");

	m_CreatureFPK.Release();			// Creature frames

	m_AddonManFPK.Release();				// Player addon frames
	m_AddonWomanFPK.Release();				// Player addon frames

	m_OustersFPK.Release();				// Player addon frames
	m_VampireFPK.Release();				// Player addon frames
	m_ItemTileFPK.Release();			// Tile?? ??? Item?? ????..
	m_ItemDropFPK.Release();			// ??????? ???????? Item?? ????..
	m_ImageObjectFPK.Release();		// ImageObject?? ???? frames
	m_ImageObjectShadowFPK.Release();

	// ???? ???? fpk ?????
	m_AdvancementSlayerManFPK.Release();
	m_AdvancementSlayerWomanFPK.Release();
	m_OsirisSlayerManFPK.Release();
	m_OsirisSlayerWomanFPK.Release();
	m_Tier2OustersFPK.Release();
	m_Tier2VampireManFPK.Release();
	m_Tier2VampireWomanFPK.Release();
	m_Tier2SlayerManFPK.Release();
	m_Tier2SlayerWomanFPK.Release();
	m_AdvancementVampireManFPK.Release();
	m_OsirisVampireManFPK.Release();
	m_OsirisVampireWomanFPK.Release();
	m_AdvancementVampireWomanFPK.Release();
	m_AdvancementOustersFPK.Release();
	m_OsirisOustersFPK.Release();
	m_AdvancementSlayerManShadowFPK.Release();
	m_AdvancementSlayerWomanShadowFPK.Release();
	m_OsirisSlayerManShadowFPK.Release();
	m_OsirisSlayerWomanShadowFPK.Release();
	m_Tier2OustersShadowFPK.Release();
	m_Tier2VampireManShadowFPK.Release();
	m_Tier2VampireWomanShadowFPK.Release();
	m_Tier2SlayerManShadowFPK.Release();
	m_Tier2SlayerWomanShadowFPK.Release();
	m_AdvancementVampireManShadowFPK.Release();
	m_AdvancementVampireWomanShadowFPK.Release();
	m_OsirisVampireManShadowFPK.Release();
	m_OsirisVampireWomanShadowFPK.Release();
	m_AdvancementOustersShadowFPK.Release();
	m_OsirisOustersShadowFPK.Release();
	//	m_InteractionObjectFPK.Release();		// ImageObject?? ???? frames

	DEBUG_ADD("MTV-Rel-EffectFPK");

	m_EffectAlphaFPK.Release();				// Effect?? ???? frames
	m_EffectScreenFPK.Release();
	m_EffectShadowFPK.Release();			// Effect?? ???? frames
	m_EffectNormalFPK.Release();			// Effect?? ???? frames

	DEBUG_ADD("MTV-Rel-CFPK");

	m_CreatureShadowFPK.Release();			// Creature frames

	m_AddonManShadowFPK.Release();				// Player addon frames	
	m_AddonWomanShadowFPK.Release();				// Player addon frames	

	m_OustersShadowFPK.Release();			// Player addon frames	
	m_VampireShadowFPK.Release();			// Player addon frames	

	//------------------------------------------------------
	// SpritePack ????
	//------------------------------------------------------		
	DEBUG_ADD("MTV-Rel-SPK");

	m_TileSPK.Release();				// Tile
	m_ImageObjectSPK.Release();		// ImageObject Sprite??
	//	m_InteractionObjectSPK.Release();		// ImageObject Sprite??
	m_CreatureSPK.Release();			// Creature Sprite??
	//#if __CONTENTS(__ONIBLA)
	m_CreatureSPK2.Release();			// Creature Sprite??
	//#endif //__ONIBLA
	m_AddonManSPK.Release();		// Sprite??		
	m_AddonWomanSPK.Release();		// Sprite??		

	m_OustersSPK.Release();
	m_VampireSPK.Release();

	DEBUG_ADD("MTV-Rel-ItemSPK");

	m_ItemTileISPK.Release();			// Sprite??		
	m_ItemDropISPK.Release();			//
	m_ItemBrokenSPK.Release();			//
	m_ItemRealSPK.Release();			// Sprite??				

	DEBUG_ADD("MTV-Rel-EffectSPk");
	m_EffectAlphaSPK.Release();		// AlphaSprite??
	m_EffectScreenSPK.Release();		// AlphaSprite??
	m_EffectShadowSPK.Release();		// ShadowSprite??
	m_EffectNormalSPK.Release();		// NormalSprite??
	m_EffectScreenPPK.Release();
	m_EffectAlphaPPK.Release();

	DEBUG_ADD("MTV-Rel-Other");

	m_WeatherSPK.Release();			// WeatherSprite??
	m_GuildSPK.Release();

	m_EtcSPK.Release();				// ??Ÿ...
	m_OustersFinSPK.Release();
	m_AdvacementQuestEnding.Release();

	// ???? ???? SPK ????
	m_AdvancementSlayerManSPK.Release();
	m_AdvancementSlayerWomanSPK.Release();
	m_OsirisSlayerManSPK.Release();
	m_OsirisSlayerWomanSPK.Release();
	m_Tier2OustersSPK.Release();
	m_Tier2VampireManSPK.Release();
	m_Tier2VampireWomanSPK.Release();
	m_Tier2SlayerManSPK.Release();
	m_Tier2SlayerWomanSPK.Release();
	m_AdvancementVampireManSPK.Release();
	m_AdvancementVampireWomanSPK.Release();
	m_OsirisVampireManSPK.Release();
	m_OsirisVampireWomanSPK.Release();
	m_AdvancementOustersSPK.Release();
	m_OsirisOustersSPK.Release();

	m_AdvancementSlayerManSSPK.Release();
	m_AdvancementSlayerWomanSSPK.Release();
	m_OsirisSlayerManSSPK.Release();
	m_OsirisSlayerWomanSSPK.Release();
	m_Tier2OustersSSPK.Release();
	m_Tier2VampireManSSPK.Release();
	m_Tier2VampireWomanSSPK.Release();
	m_Tier2SlayerManSSPK.Release();
	m_Tier2SlayerWomanSSPK.Release();
	m_AdvancementVampireManSSPK.Release();
	m_AdvancementVampireWomanSSPK.Release();
	m_OsirisVampireManSSPK.Release();
	m_OsirisVampireWomanSSPK.Release();
	m_AdvancementOustersSSPK.Release();
	m_OsirisOustersSSPK.Release();

	//------------------------------------------------------		
	// Shadow SpritePack
	//------------------------------------------------------
	DEBUG_ADD("MTV-Rel-SSPK");

	m_AddonManSSPK.Release();			// Sprite??		
	m_AddonWomanSSPK.Release();			// Sprite??		

	m_OustersSSPK.Release();			// Sprite??		
	m_VampireSSPK.Release();
	m_ImageObjectSSPK.Release();
	//	m_InteractionObjectSSPK.Release();
	m_CreatureSSPK.Release();
	//#if __CONTENTS(__ONIBLA)
	m_CreatureSSPK2.Release();
	//#endif //__ONIBLA

		//----------------------------------------------------------------------
		// File Index Table ?????
		//----------------------------------------------------------------------
	DEBUG_ADD("MTV-Rel-SSPKI");

	m_TileSPKI.Release();
	m_ImageObjectSPKI.Release();
	//	m_EffectAlphaSPKI.Release();
	//	m_EffectScreenSPKI.Release();

		//----------------------------------------------------------------------
		// Filter
		//----------------------------------------------------------------------
	DEBUG_ADD("MTV-Rel-Filter");

	m_ImageObjectFilter.Release();
	m_LightFTP.Release();
	m_LightBufferFilter.Release();;

	//----------------------------------------------------------------------
	// SFP Array????
	//----------------------------------------------------------------------
	if (m_pTileSFPArrayLargeZone != NULL)
	{
		delete m_pTileSFPArrayLargeZone;
		m_pTileSFPArrayLargeZone = NULL;
		DEBUG_ADD("MTV-Rel-TileSFPAL");
	}

	if (m_pImageObjectSFPArrayLargeZone != NULL)
	{
		delete m_pImageObjectSFPArrayLargeZone;
		m_pImageObjectSFPArrayLargeZone = NULL;
		DEBUG_ADD("MTV-Rel-IMSFPAL");
	}

	if (m_pTileSFPArraySmallZone != NULL)
	{
		delete m_pTileSFPArraySmallZone;
		m_pTileSFPArraySmallZone = NULL;
		DEBUG_ADD("MTV-Rel-TileSFPS");
	}

	if (m_pImageObjectSFPArraySmallZone != NULL)
	{
		delete m_pImageObjectSFPArraySmallZone;
		m_pImageObjectSFPArraySmallZone = NULL;
		DEBUG_ADD("MTV-Rel-IMSFPS");
	}

	m_bInit = false;

	DEBUG_ADD("MTV-Rel-All OK");
}


//----------------------------------------------------------------------
// Thread Load?? ???????.
//----------------------------------------------------------------------
void
MTopView::ClearWaitToLoadSet()
{
	//	m_EffectScreenSPK.ClearWaitToLoadSet();
	//	m_EffectAlphaSPK.ClearWaitToLoadSet();

	//	m_ImageObjectSPK.ClearWaitToLoadSet();
	//	m_ImageObjectSSPK.ClearWaitToLoadSet();

	//	m_TileSPK.ClearWaitToLoadSet();

	// 	m_AddonSPK.FinishThreadLoad();
	// 	m_OustersSPK.FinishThreadLoad();
	// 	m_VampireSPK.FinishThreadLoad();
	// 	m_AdvancementSlayerManSPK.FinishThreadLoad();
	// 	m_AdvancementSlayerWomanSPK.FinishThreadLoad();
	// 	m_AdvancementOustersSPK.FinishThreadLoad();
	// 	m_AdvancementVampireManSPK.FinishThreadLoad();
	// 	m_AdvancementVampireWomanSPK.FinishThreadLoad();
}

//----------------------------------------------------------------------
// ??? ????? ??? Surface?? ???????.
//----------------------------------------------------------------------
void
MTopView::SetSurface(CSpriteSurface*& pSurface)
{
	m_pSurface = pSurface;
}

//----------------------------------------------------------------------
// Clear ShadowManager
//----------------------------------------------------------------------
void
MTopView::ClearShadowManager()
{
	if (CDirect3D::IsHAL())
	{
		//		if (m_pImageObjectShadowManager!=NULL) m_pImageObjectShadowManager->Clear();
		//		if (m_pAddonShadowManager!=NULL) m_pAddonShadowManager->Clear();
		//		if (m_pOustersShadowManager!=NULL) m_pOustersShadowManager->Clear();
		//		if (m_pCreatureShadowManager!=NULL) m_pCreatureShadowManager->Clear();
	}
}

//----------------------------------------------------------------------
// Set 3D Box Color
//----------------------------------------------------------------------
//void			
//MTopView::Set3DBoxColor(WORD pixel)
//{
//	#ifdef OUTPUT_DEBUG_DRAW_3DBOX
//		DEBUG_ADD_FORMAT("Set3DBoxColor = %x", pixel);
//	#endif
//
//	if (m_p3DBoxSurface->Lock())
//	{	
//		WORD *lpSurface = (WORD*)m_p3DBoxSurface->GetSurfacePointer();
//		WORD *lpSurfaceTemp = lpSurface;
//		*lpSurfaceTemp = pixel;
//		m_p3DBoxSurface->Unlock();
//		
//		m_3DBoxCurrentPixel = pixel;
//
//		#ifdef OUTPUT_DEBUG_DRAW_3DBOX
//			DEBUG_ADD("OK");
//		#endif
//	}
//}
//
////----------------------------------------------------------------------
//// Init 3D BoxSurface
////----------------------------------------------------------------------
//bool
//MTopView::Init3DBoxSurface()
//{
//	//-----------------------------------------------
//	// 3D Box Texture Surface
//	//-----------------------------------------------
//	if (CDirect3D::IsHAL())
//	{	
//		//-----------------------------------------------
//		// ??????
//		//-----------------------------------------------
//		if (m_p3DBoxBlackSurface!=NULL)
//		{
//			if (m_p3DBoxBlackSurface->Lock())
//			{	
//				WORD *lpSurface = (WORD*)m_p3DBoxBlackSurface->GetSurfacePointer();
//				*lpSurface = m_ColorBlackHalf;
//				m_p3DBoxBlackSurface->Unlock();
//			}
//		}
//
//		//-----------------------------------------------
//		// HP????
//		//-----------------------------------------------
//		if (m_p3DBoxHPSurface!=NULL)
//		{
//			if (m_p3DBoxHPSurface->Lock())
//			{	
//				WORD *lpSurface = (WORD*)m_p3DBoxHPSurface->GetSurfacePointer();
//				*lpSurface = m_ColorHPBar;
//				m_p3DBoxHPSurface->Unlock();
//			}
//		}
//	}
//
//	return true;
//}

//----------------------------------------------------------------------
// Restore Surface
//----------------------------------------------------------------------
void
MTopView::RestoreSurface()
{
	m_bDrawRequest = false;

	//for (int i=0; i<4; i++)
	{
		CDirectDraw::RestoreAllSurfaces();

		if (CDirect3D::IsHAL())
		{
			CDirect3D::Restore();

			/*
			D3DRECT rect3d;
			rect3d.x1 = 0;
			rect3d.y1 = 0;
			rect3d.x2 = CLIPSURFACE_WIDTH;
			rect3d.y2 = CLIPSURFACE_HEIGHT;

			CDirect3D::GetDevice()->Clear( 1UL, (D3DRECT *)&rect3d,
										D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
										0x00000000, 1.0f, 0L );
			*/
		}

		//	CDirectDraw::Flip();		
	}

	if (m_pSurface != NULL)
	{
		m_pSurface->Restore();
	}

	if (m_pTileSurface != NULL)
	{
		m_pTileSurface->Restore();
	}

	// ??? ??? ??????.
	//m_bFirstTileDraw = true;

	ClearOutputCreature();

	ClearItemNameList();

	//------------------------------------------------------------
	// ?????? ?????? ????? 
	// Texture?? VideoMemory?? ?ö? ??????? Restore??? ???.
	// ??? Load.. - -;;
	//------------------------------------------------------------
	if (CDirect3D::IsHAL())
	{
		//------------------------------------------------------------
		// Part Manager
		//------------------------------------------------------------
		DEBUG_ADD("Restore Surfaces - m_pAlphaEffectTextureManager");

#ifdef __3D_IMAGE_OBJECT__
		//		if( m_pImageObjectTextureManager != NULL )
		//			m_pImageObjectTextureManager->Clear();
#endif

//		if (m_pAlphaEffectTextureManager!=NULL)
//		{
//			m_pAlphaEffectTextureManager->Clear();
//		}
//
//		if (m_pScreenEffectTextureManager!=NULL)
//		{
//			m_pScreenEffectTextureManager->Clear();
//		}

//		if (m_pImageObjectShadowManager!=NULL)
//		{
//			m_pImageObjectShadowManager->Clear();
//		}

//		if (m_pEffectShadowManager!=NULL)
//		{
//			m_pEffectShadowManager->Clear();
//		}
//
//		if (m_pAddonShadowManager!=NULL)
//		{
//			m_pAddonShadowManager->Clear();
//		}
//
//		if (m_pOustersShadowManager!=NULL)
//		{
//			m_pOustersShadowManager->Clear();
//		}
//		
//		if (m_pCreatureShadowManager!=NULL)
//		{
//			m_pCreatureShadowManager->Clear();
//		}

		//------------------------------------------------------------
		// Light Buffer Texture
		//------------------------------------------------------------
		DEBUG_ADD("Restore Surfaces - m_pLightBufferTexture");

		InitFilters();
		/*
	if (m_pLightBufferTexture!=NULL)
	{
		m_pLightBufferTexture->Restore();
		//delete m_pLightBufferTexture;
	}
	*/

	//m_pLightBufferTexture = new CSpriteSurface;
	//m_pLightBufferTexture->InitTextureSurface(SCREENLIGHT_WIDTH, SCREENLIGHT_HEIGHT, 0, CDirect3D::GetPixelFormat4444());
	//m_pLightBufferTexture->Restore();

//		if (m_p3DBoxSurface!=NULL)
//		{
//			//m_p3DBoxSurface->Restore();
//			delete m_p3DBoxSurface;	
//			m_p3DBoxSurface = NULL;
//		}
//
//		if (m_p3DBoxBlackSurface!=NULL)
//		{
//			//m_p3DBoxBlackSurface->Restore();
//			delete m_p3DBoxBlackSurface;	
//			m_p3DBoxBlackSurface = NULL;
//		}
//
//		if (m_p3DBoxHPSurface!=NULL)
//		{
//			//m_p3DBoxHPSurface->Restore();
//			delete m_p3DBoxHPSurface;	
//			m_p3DBoxHPSurface = NULL;
//		}


		//------------------------------------------------------------
		// 3DBox Surface
		//------------------------------------------------------------
		DEBUG_ADD("Restore Surfaces - m_p3DBoxSurface");

		//		m_p3DBoxSurface = new CSpriteSurface;
		//		m_p3DBoxSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
		//		m_p3DBoxSurface->Restore();
		//
		//		m_p3DBoxBlackSurface = new CSpriteSurface;
		//		m_p3DBoxBlackSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
		//		m_p3DBoxBlackSurface->Restore();
		//
		//		m_p3DBoxHPSurface = new CSpriteSurface;
		//		m_p3DBoxHPSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
		//		m_p3DBoxHPSurface->Restore();
		//		
		//		Set3DBoxColor( m_3DBoxCurrentPixel );
		//
		//		Init3DBoxSurface();

				//------------------------------------------------------------
				// minimap texture????
				//------------------------------------------------------------
				// 2001.7.14 ???ó??
		//		if (m_pMinimapSPR!=NULL && m_pMinimapTexture!=NULL)
		//		{
		//			m_pMinimapTexture->Restore();
		////			InitMinimapTexture();
		//		}		
	}
	/*
	if (CDirect3D::IsHAL())
	{
		//------------------------------------------------------------
		//
		// Load  EffectPack
		//
		//------------------------------------------------------------
		std::ifstream	effectFile2(FILE_ASPRITE_ALPHAEFFECT, ios::binary);
		m_EffectAlphaSPK.LoadFromFile(effectFile2);
		effectFile2.close();

		//------------------------------------------------------------
		//
		// Init Effect TextureSurface
		//
		//------------------------------------------------------------
		//--------------------------------------------
		// AlphaSPK?? ?????? TexturePack?? ???????.
		//--------------------------------------------
		if (!m_EffectTPK.Init( m_EffectAlphaSPK ))
		{
			MessageBox(g_hWnd, "Can't Init EffectTexture!", NULL, MB_OK);
			return;
		}

		//------------------------------------------------------------
		// EffectAlphaSprite?? ??????? ???????.
		// --> TextureSurface?? ????? ??????.. ??? ????.
		//------------------------------------------------------------
		m_EffectAlphaSPK.Release();
	}
	*/
}


//----------------------------------------------------------------------
// ????? Surface???? ???? ?????.
//----------------------------------------------------------------------
bool
MTopView::InitSurfaces()
{
	//-----------------------------------------------
	// Tile Surface
	//-----------------------------------------------
	if (m_pTileSurface != NULL)
	{
		delete m_pTileSurface;
	}

	m_pTileSurface = new CSpriteSurface;



	//
	// 3D?????? ??? VideoMemory??.. (???????. - -;)
	//
	// ????.. system memory??... 
	// (??? ??.. ????? ???õ? ??? ???? ?????? ã?? ??????. ????)
	//
	//----------------------------------------------------------------
	// 3D
	//----------------------------------------------------------------
	//-----------------------------------------------
	//
	//-----------------------------------------------
//	if (m_p3DBoxSurface!=NULL)
//	{
//		delete m_p3DBoxSurface;	
//		m_p3DBoxSurface = NULL;
//	}
//
//	//-----------------------------------------------
//	//
//	//-----------------------------------------------
//	if (m_p3DBoxBlackSurface!=NULL)
//	{
//		delete m_p3DBoxBlackSurface;	
//		m_p3DBoxBlackSurface = NULL;
//	}
//
//
//	//-----------------------------------------------
//	//
//	//-----------------------------------------------
//	if (m_p3DBoxHPSurface!=NULL)
//	{
//		delete m_p3DBoxHPSurface;	
//		m_p3DBoxHPSurface = NULL;
//	}


	if (CDirect3D::IsHAL())
	{
		// ??? video memory?? ?????? ????? ?????..
		// ???? system memory?? ???????.
//		if (!m_pTileSurface->InitOffsurface(TILESURFACE_WIDTH, TILESURFACE_HEIGHT))
		{
			DEBUG_ADD("[ InitGame ]  MTopView::InitSurface() - TileSurface vidmem failed");

			m_pTileSurface->InitOffsurface16(TILESURFACE_WIDTH,
				TILESURFACE_HEIGHT,
				DDSCAPS_SYSTEMMEMORY);

			DEBUG_ADD("[ InitGame ]  MTopView::InitSurface() - TileSurface sysmem OK");
		}

		//-----------------------------------------------
		// free
		//-----------------------------------------------
//		m_p3DBoxSurface = new CSpriteSurface;
//		m_p3DBoxSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
//
//		Set3DBoxColor( m_3DBoxCurrentPixel );
//
//		m_p3DBoxBlackSurface = new CSpriteSurface;
//		m_p3DBoxBlackSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
//		m_p3DBoxBlackSurface->Restore();
//
//		m_p3DBoxHPSurface = new CSpriteSurface;
//		m_p3DBoxHPSurface->InitTextureSurface(1, 1, 0, CDirect3D::GetPixelFormat4444());
//		m_p3DBoxHPSurface->Restore();


	}
	//----------------------------------------------------------------
	// 2D
	//----------------------------------------------------------------
	else
	{
		m_pTileSurface->InitOffsurface16(TILESURFACE_WIDTH,
			TILESURFACE_HEIGHT,
			DDSCAPS_SYSTEMMEMORY);
	}

	//m_pTileSurface->InitOffsurface(TILESURFACE_WIDTH, 
	//								TILESURFACE_HEIGHT,
	//								DDSCAPS_SYSTEMMEMORY);
	//,(CDirect3D::IsHAL()? 0 : DDSCAPS_SYSTEMMEMORY));

	m_pTileSurface->SetTransparency(0);



	UI_DrawProgress(1);
	DrawTitleLoading();

	return true;
}

//----------------------------------------------------------------------
// Init Colors
//----------------------------------------------------------------------
// ??????? ???? ???.
//----------------------------------------------------------------------
bool
MTopView::InitColors()
{
	//---------------------------------------------------
	// ?? ???? ??
	//---------------------------------------------------
	// item
	m_ColorNameItem = g_pClientConfig->COLOR_NAME_ITEM;
	m_ColorNameItemOption = g_pClientConfig->COLOR_NAME_ITEM_OPTION;

	// ??????
	m_ColorNameVampire = g_pClientConfig->COLOR_NAME_VAMPIRE;
	m_ColorNameSlayer = g_pClientConfig->COLOR_NAME_SLAYER;
	m_ColorNameNPC = g_pClientConfig->COLOR_NAME_NPC;

	m_ColorNameAlignment[0] = g_pClientConfig->COLOR_NAME_EVIL_MORE;
	m_ColorNameAlignment[1] = g_pClientConfig->COLOR_NAME_EVIL;
	m_ColorNameAlignment[2] = g_pClientConfig->COLOR_NAME_NEUTRAL;
	m_ColorNameAlignment[3] = g_pClientConfig->COLOR_NAME_GOOD;
	m_ColorNameAlignment[4] = g_pClientConfig->COLOR_NAME_GOOD_MORE;


	//---------------------------------------------------
	// 5:6:5?? ???? ??? ???? ?????? ???.
	//---------------------------------------------------
	if (CDirectDraw::Is565())
	{
		// interactionObject
//		m_ColorOutlineInteractionObject	= g_pClientConfig->COLOR_OUTLINE_INTERACTIONOBJECT;

		m_ColorOutlineItem = g_pClientConfig->COLOR_OUTLINE_ITEM;

		// ???? ?????
		m_ColorOutlineNPC = g_pClientConfig->COLOR_OUTLINE_NPC;
		m_ColorOutlineAttackPossible = g_pClientConfig->COLOR_OUTLINE_ATTACK_POSSIBLE;
		m_ColorOutlineAttackImpossible = g_pClientConfig->COLOR_OUTLINE_ATTACK_IMPOSSIBLE;
	}
	//---------------------------------------------------
	// 5:5:5?? ???? ???? ???
	//---------------------------------------------------
	else
	{
		// interactionObject
//		m_ColorOutlineInteractionObject	= CDirectDraw::Convert565to555(g_pClientConfig->COLOR_OUTLINE_INTERACTIONOBJECT);

		// item
		m_ColorOutlineItem = CDirectDraw::Convert565to555(g_pClientConfig->COLOR_OUTLINE_ITEM);

		// ???? ?????
		m_ColorOutlineNPC = CDirectDraw::Convert565to555(g_pClientConfig->COLOR_OUTLINE_NPC);
		m_ColorOutlineAttackPossible = CDirectDraw::Convert565to555(g_pClientConfig->COLOR_OUTLINE_ATTACK_POSSIBLE);
		m_ColorOutlineAttackImpossible = CDirectDraw::Convert565to555(g_pClientConfig->COLOR_OUTLINE_ATTACK_IMPOSSIBLE);
	}


	//---------------------------------------------------
	// 3D ????? ?????? ??? ????...
	//---------------------------------------------------
//	if (CDirect3D::IsHAL())
//	{	
//		// 4:4:4:4 format
//		m_ColorHPBar = 0xA000 
//						| ((g_pClientConfig->COLOR_HP_BAR_R >> 1) << 8)
//						| ((g_pClientConfig->COLOR_HP_BAR_G >> 1) << 4)
//						| (g_pClientConfig->COLOR_HP_BAR_B >> 1);			
//
//		m_ColorBlackHalf = 0xA000;
//	}
//	else
	{
		m_ColorHPBar = CDirectDraw::Color(g_pClientConfig->COLOR_HP_BAR_R,
			g_pClientConfig->COLOR_HP_BAR_G,
			g_pClientConfig->COLOR_HP_BAR_B);

		m_ColorBlackHalf = 0;
	}

	return true;
}


//----------------------------------------------------------------------
// Optional filedef entry: "" when the key is missing (older data folders).
//----------------------------------------------------------------------
static std::string
GetOptionalFileDef(const char* key)
{
	try
	{
		return g_pFileDef->getProperty(key);
	}
	catch (...)
	{
		return "";
	}
}

//----------------------------------------------------------------------
// SpritePack?? ???  memory?? Load???.
//----------------------------------------------------------------------
bool
MTopView::InitSprites()
{
	// Sprite
	/*
	WORD		spriteID = 0;

	WORD	*lpSurface,
			*lpSurfaceTemp,
			*lpSurfaceTemp2;

	WORD	lPitch;

	int i,j;

	// Temp Surface
	CSpriteSurface TempSurface;
	*/



	/*
	m_ImageObjectSSPK.Init( 42 );

	CShadowSprite::SetColorkey( 0x001F );
	//------------------------------------------------------------
	// Shadow
	//------------------------------------------------------------
	TempSurface.InitFromBMP("BuildingShadow0.bmp", 1024,768, DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_ImageObjectSSPK[ 0 ].SetPixel(lpSurface, lPitch, 874, 360);
	TempSurface.Unlock();

	TempSurface.InitFromBMP("BuildingShadow1.bmp", 1024,768, DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_ImageObjectSSPK[ 1 ].SetPixel(lpSurface, lPitch, 906, 494);
	TempSurface.Unlock();

	TempSurface.InitFromBMP("BuildingShadow2.bmp", 1024,768, DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_ImageObjectSSPK[ 2 ].SetPixel(lpSurface, lPitch, 928, 436);
	TempSurface.Unlock();

	TempSurface.InitFromBMP("BuildingShadow3.bmp", 1024,768, DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_ImageObjectSSPK[ 3 ].SetPixel(lpSurface, lPitch, 804, 502);

	for (i=4; i<=31; i++)
	{
		m_ImageObjectSSPK[ i ].SetPixel(lpSurface, lPitch, 804, 502);
	}

	TempSurface.Unlock();

	TempSurface.InitFromBMP("ImageObjectSomeShadow.bmp", 800,600, DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	lpSurfaceTemp = lpSurface;
	m_ImageObjectSSPK[ 32 ].SetPixel(lpSurfaceTemp, lPitch, 87, 82);
	lpSurfaceTemp = lpSurface + 89;
	m_ImageObjectSSPK[ 33 ].SetPixel(lpSurfaceTemp, lPitch, 47, 50);
	lpSurfaceTemp = lpSurface + 157;
	m_ImageObjectSSPK[ 34 ].SetPixel(lpSurfaceTemp, lPitch, 58, 50);
	m_ImageObjectSSPK[ 35 ].SetPixel(lpSurfaceTemp, lPitch, 58, 50);

	lpSurfaceTemp = lpSurface + 223;
	m_ImageObjectSSPK[ 36 ].SetPixel(lpSurfaceTemp, lPitch, 49, 50);
	m_ImageObjectSSPK[ 38 ].SetPixel(lpSurfaceTemp, lPitch, 49, 50);
	m_ImageObjectSSPK[ 40 ].SetPixel(lpSurfaceTemp, lPitch, 49, 50);

	lpSurfaceTemp = lpSurface + 286;
	m_ImageObjectSSPK[ 37 ].SetPixel(lpSurfaceTemp, lPitch, 51, 41);
	m_ImageObjectSSPK[ 39 ].SetPixel(lpSurfaceTemp, lPitch, 51, 41);
	m_ImageObjectSSPK[ 41 ].SetPixel(lpSurfaceTemp, lPitch, 51, 41);
	TempSurface.Unlock();


	//------------------------------------------------------------
	// Save  ImageObject Shadow SpritePack
	//------------------------------------------------------------
	std::ofstream	ImageObjectSFile(FILE_SSPRITE_IMAGEOBJECT, ios::binary);
	std::ofstream	ImageObjectSIndexFile(FILE_SSPRITEINDEX_IMAGEOBJECT, ios::binary);

	m_ImageObjectSSPK.SaveToFile(ImageObjectSFile, ImageObjectSIndexFile);

	ImageObjectSFile.close();
	ImageObjectSIndexFile.close();
	*/

	//------------------------------------------------------------	
	// Load  ImageObject ShadowSpritePack	
	//------------------------------------------------------------
	//------------------------------------------------------------
	//
	// Load  EffectPack
	//
	//------------------------------------------------------------
	// 3d?????? ??? m_pImageObjectShadowManager?? ??????
	// ????, m_ImageObjectSSPK?? ??????.
	/*
	std::ifstream	ImageObjectShadowFile2;//(FILE_SSPRITE_IMAGEOBJECT, ios::binary);
	if (!FileOpenBinary(FILE_SSPRITE_IMAGEOBJECT, ImageObjectShadowFile2))
		return false;
	m_ImageObjectSSPK.LoadFromFile(ImageObjectShadowFile2);
	ImageObjectShadowFile2.close();
	*/
	//	if (m_pImageObjectShadowManager!=NULL)
	//	{
	//		delete m_pImageObjectShadowManager;
	//		m_pImageObjectShadowManager = NULL;
	//	}		
	//
	//	//m_ImageObjectSSPK.Release();
	//
	//	if (CDirect3D::IsHAL())
	//	{	
	//		//m_pAlphaEffectTextureManager = new CTexturePartManager( FILE_ASPRITE_ALPHAEFFECT, FILE_ASPRITEINDEX_ALPHAEFFECT, 70 );
	//		//m_pImageObjectShadowManager = new CShadowPartManager( 
	//		//										&m_ImageObjectSSPK, 
	//		//										g_pClientConfig->MAX_TEXTUREPART_IMAGEOBJECTSHADOW );
	//		m_pImageObjectShadowManager = new CShadowPartManager(g_pFileDef->getProperty("FILE_SSPRITE_IMAGEOBJECT").c_str(),
	////															FILE_SSPRITEINDEX_IMAGEOBJECT, 
	//															g_pClientConfig->MAX_TEXTUREPART_IMAGEOBJECTSHADOW );
	//	}
	//	else
	{
		// 		std::ifstream	ImageObjectShadowFile2;//(FILE_SSPRITE_IMAGEOBJECT, ios::binary);
		//		if (!FileOpenBinary(FILE_SSPRITE_IMAGEOBJECT, ImageObjectShadowFile2))
		//			return false;

		m_ImageObjectSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_IMAGEOBJECT").c_str());
		//		m_ImageObjectSSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_SSPRITE_IMAGEOBJECT").c_str());

		// 		ImageObjectShadowFile2.close();
	}

	//	UI_DrawProgress(2);
	//	DrawTitleLoading();

		//------------------------------------------------------------
		//
		//
		//                  Creature SpritePack
		//
		//
		//------------------------------------------------------------	
		//------------------------------------------------------------	
		// Init  Creature SpritePack	
		//------------------------------------------------------------
		// ??? ??ü ???? memory?? ?????.
		//------------------------------------------------------------
		//  Init Creature SpriteSet
		//------------------------------------------------------------	
	if (m_CreatureSPK.GetSize() == 0)
	{
		/*
		WORD size;
		std::ifstream CreaturePackIndexFile;//(FILE_ISPRITEINDEX_CREATURE, ios::binary);
		if (!FileOpenBinary(FILE_ISPRITEINDEX_CREATURE, CreaturePackIndexFile))
			return false;
		CreaturePackIndexFile.read((char*)&size, 2);	// Sprite?? ????
		CreaturePackIndexFile.close();

		m_CreatureSPK.Init( size, CDirectDraw::Is565() );
		*/
		//		if (m_CreatureSPKFile.is_open())
		//		{
		//			m_CreatureSPKFile.close();
		//		}
		//		
		//		// ???? ??????.
		//		//------------------------------------------------------------
		//		// sprite load
		//		//------------------------------------------------------------
		//		if (!FileOpenBinary(FILE_ISPRITE_CREATURE, m_CreatureSPKFile))
		//			return false;	
		//
		//		TYPE_SPRITEID	numCreatureSPK;
		//		m_CreatureSPKFile.read((char*)&numCreatureSPK, SIZE_SPRITEID);
		//		m_CreatureSPK.Init( numCreatureSPK, CDirectDraw::Is565() );			
		m_CreatureSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_CREATURE").c_str());
		//		m_CreatureSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_ISPRITE_CREATURE").c_str());

	}
	//#if __CONTENTS(__ONIBLA)
	if (m_CreatureSPK2.GetSize() == 0)
	{
		m_CreatureSPK2.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_CREATURE2").c_str());
	}
	//#endif //__ONIBLA
	//	UI_DrawProgress(3);
	//	DrawTitleLoading();

		//m_CreatureSSPK.Init( size );
		//m_CreatureTPK.Init( size );

		//------------------------------------------------------------	
		//
		//			Creature Shadow SpritePack	
		//
		//------------------------------------------------------------	
		/*
		m_CreatureSSPK.Init( 1 );

		CShadowSprite::SetColorkey( 0x001F );
		//------------------------------------------------------------
		// Shadow
		//------------------------------------------------------------
		TempSurface.InitFromBMP("CreatureShadow.bmp", DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);
		m_CreatureSSPK[ 0 ].SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
		TempSurface.Unlock();

		//------------------------------------------------------------
		// Save  Creature SpritePack
		//------------------------------------------------------------
		std::ofstream	creatureShadowFile(FILE_SSPRITE_CREATURE, ios::binary);
		std::ofstream	creatureShadowIndexFile(FILE_SSPRITEINDEX_CREATURE, ios::binary);
		m_CreatureSSPK.SaveToFile(creatureShadowFile, creatureShadowIndexFile);
		creatureShadowFile.close();
		creatureShadowIndexFile.close();
		*/

		//------------------------------------------------------------
		// ???? : ???SpritePack --> ShadowSpritePack
		//------------------------------------------------------------
		// ??
		/*
		CSpritePack CreatureShadowSPK;
		std::ifstream	CreatureShadowFile2;//(FILE_ISPRITE_ADDON, ios::binary);
		if (!FileOpenBinary("Data\\Image\\CreatureShadow.spk", CreatureShadowFile2))
			return false;
		CreatureShadowSPK.LoadFromFile(CreatureShadowFile2);
		CreatureShadowFile2.close();

		m_CreatureSSPK.Init( CreatureShadowSPK.GetSize() );

		for (int i=0; i<CreatureShadowSPK.GetSize(); i++)
		{
			m_CreatureSSPK[i].SetPixel( CreatureShadowSPK[i] );
		}

		//------------------------------------------------------------
		// ?? - Save  ShadowSpritePack
		//------------------------------------------------------------
		std::ofstream	CreatureShadowFile(FILE_SSPRITE_CREATURE, ios::binary);
		std::ofstream	CreatureShadowIndexFile(FILE_SSPRITEINDEX_CREATURE, ios::binary);

		m_CreatureSSPK.SaveToFile(CreatureShadowFile, CreatureShadowIndexFile);

		CreatureShadowFile.close();
		CreatureShadowIndexFile.close();
		*/

		//------------------------------------------------------------
		// Load Creature Shadow SpritePack
		//------------------------------------------------------------
		// 3d?????? ??? m_pCreatureShadowManager?? ??????
		// ????, m_CreatureSSPK?? ??????.
		//------------------------------------------------------------
		// 3d?????? ??? m_pAlphaEffectTextureManager?? ??????
		// ????, m_EffectAlphaSPK?? ??????.

		// ??ü ?????? ?????.
		/*
		std::ifstream CreatureShadowPackIndexFile;//(FILE_ISPRITEINDEX_CREATURE, ios::binary);
		if (!FileOpenBinary(FILE_SSPRITEINDEX_CREATURE, CreatureShadowPackIndexFile))
			return false;
		CreatureShadowPackIndexFile.read((char*)&size, 2);	// Sprite?? ????
		CreatureShadowPackIndexFile.close();

		m_CreatureSSPK.Init( size );
		*/
		/*
		// ??÷? Load
		// ??ü loading???? ???
		std::ifstream	CreatureShadowFile2;//(FILE_SSPRITE_Creature, ios::binary);
		if (!FileOpenBinary(FILE_SSPRITE_CREATURE, CreatureShadowFile2))
			return false;
		m_CreatureSSPK.LoadFromFile(CreatureShadowFile2);
		CreatureShadowFile2.close();
		*/
		//	if (m_CreatureSSPKFile.is_open())
		//	{
		//		m_CreatureSSPKFile.close();
		//	}		
		//
		//	if (m_pCreatureShadowManager!=NULL)
		//	{
		//		delete m_pCreatureShadowManager;
		//		m_pCreatureShadowManager = NULL;
		//	}
		//	//m_CreatureSSPK.Release();
		//	
		//	if (CDirect3D::IsHAL())
		//	{	
		//		//m_pAlphaEffectTextureManager = new CTexturePartManager( FILE_ASPRITE_ALPHAEFFECT, FILE_ASPRITEINDEX_ALPHAEFFECT, 70 );
		//		//m_pCreatureShadowManager = new CShadowPartManager( 
		//		//									&m_CreatureSSPK,  
		//		//									g_pClientConfig->MAX_TEXTUREPART_CREATURESHADOW);
		//		m_pCreatureShadowManager = new CShadowPartManager( 
		//											g_pFileDef->getProperty("FILE_SSPRITE_CREATURE").c_str(),
		////											FILE_SSPRITEINDEX_CREATURE,  
		//											g_pClientConfig->MAX_TEXTUREPART_CREATURESHADOW);
		//	}
		//	else
	{
		/*
		std::ifstream	CreatureShadowFile2;//(FILE_SSPRITE_Creature, ios::binary);
		if (!FileOpenBinary(FILE_SSPRITE_CREATURE, CreatureShadowFile2))
			return false;
		m_CreatureSSPK.LoadFromFile(CreatureShadowFile2);
		CreatureShadowFile2.close();
		*/
		/*
		std::ifstream indexFile;//(indexFilename, ios::binary);
		if (!FileOpenBinary(FILE_SSPRITEINDEX_CREATURE, indexFile))
			return false;

		int packSize;
		indexFile.read((char*)&packSize, 2);
		indexFile.close();

		m_CreatureSSPK.Init( packSize );
		*/
		//------------------------------------------------------------
		// ????? load
		//------------------------------------------------------------
//		if (!FileOpenBinary(FILE_SSPRITE_CREATURE, m_CreatureSSPKFile))
//			return false;
//
//		TYPE_SPRITEID	numCreatureSSPK;
//
//		m_CreatureSSPKFile.read((char*)&numCreatureSSPK, SIZE_SPRITEID);
// 		m_CreatureSSPK.Init( numCreatureSSPK );			
		m_CreatureSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_CREATURE").c_str());
	}
	//#if __CONTENTS(__ONIBLA)
	if (m_CreatureSSPK2.GetSize() == 0)
	{
		m_CreatureSSPK2.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_CREATURE2").c_str());
	}
	//#endif //__ONIBLA
	//	UI_DrawProgress(4);
	//	DrawTitleLoading();

		//------------------------------------------------------------
		//
		//
		//                   Clothes SpritePack
		//
		//
		//------------------------------------------------------------
		/*
		CSpritePack	ClothesSPK;
		ClothesSPK.Init( 160, CDirectDraw::Is565() );

		//------------------------------------------------------------
		// Shirts
		//------------------------------------------------------------
		TempSurface.InitFromBMP("Shirts.bmp", 640,480, DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);

		spriteID = 0;
		for (i=0; i<4; i++)
		{
			lpSurfaceTemp = lpSurface;
			for (j=0; j<10; j++)
			{
				ClothesSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 64);

				lpSurfaceTemp += 64;
			}
			lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*64);
		}
		TempSurface.Unlock();

		//------------------------------------------------------------
		// Pants
		//------------------------------------------------------------
		TempSurface.InitFromBMP("Pants.bmp", 640,480, DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);

		for (i=0; i<4; i++)
		{
			lpSurfaceTemp = lpSurface;
			for (j=0; j<10; j++)
			{
				ClothesSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 64);

				lpSurfaceTemp += 64;
			}
			lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*64);
		}
		TempSurface.Unlock();


		//------------------------------------------------------------
		// Boots
		//------------------------------------------------------------
		TempSurface.InitFromBMP("Boots.bmp", 640,480, DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);

		for (i=0; i<4; i++)
		{
			lpSurfaceTemp = lpSurface;
			for (j=0; j<10; j++)
			{
				ClothesSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 64);

				lpSurfaceTemp += 64;
			}
			lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*64);
		}
		TempSurface.Unlock();

		//------------------------------------------------------------
		// Cloak
		//------------------------------------------------------------
		TempSurface.InitFromBMP("Cloak.bmp", 640,480, DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);

		for (i=0; i<4; i++)
		{
			lpSurfaceTemp = lpSurface;
			for (j=0; j<10; j++)
			{
				ClothesSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 64);

				lpSurfaceTemp += 64;
			}
			lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*64);
		}
		TempSurface.Unlock();

		//------------------------------------------------------------
		// Save  Clothes SpritePack
		//------------------------------------------------------------
		std::ofstream	clothesFile("Clothes.spk", ios::binary);
		std::ofstream	clothesIndexFile("Clothes.spki", ios::binary);

		ClothesSPK.SaveToFile(clothesFile, clothesIndexFile);

		clothesFile.close();
		clothesIndexFile.close();
		*/

		//------------------------------------------------------------	
		// Load  Clothes SpritePack	
		//------------------------------------------------------------
		// slayer
	/*
		if (m_AddonSPK.GetSize()==0)
		{
			std::ifstream	AddonFile2;//(FILE_ISPRITE_ADDON, ios::binary);
			if (!FileOpenBinary(FILE_ISPRITE_ADDON, AddonFile2))
				return false;

			// 2001.8.20 ???ó??..
			//m_AddonSPK.LoadFromFile(AddonFile2);
			//m_AddonSPK.Init( 20000, CDirectDraw::Is565() );

			// ?????? ?????.
			TYPE_SPRITEID addonSize;
			AddonFile2.read((char*)&addonSize, SIZE_SPRITEID);
			m_AddonSPK.Init( addonSize, CDirectDraw::Is565() );
			AddonFile2.close();
		}
	*/
	//	if (m_AddonSPKFile.is_open())
	//	{
	//		m_AddonSPKFile.close();
	//	}
	//
	//	if (m_AddonSSPKFile.is_open())
	//	{
	//		m_AddonSSPKFile.close();
	//	}
	//
	//	// ???? ??????.
	//	//------------------------------------------------------------
	//	// sprite load
	//	//------------------------------------------------------------
	//	if (!FileOpenBinary(FILE_ISPRITE_ADDON, m_AddonSPKFile))
	//		return false;	
	//
	//	if (m_AddonSPK.GetSize()==0)
	//	{
	//		TYPE_SPRITEID	numAddonSPK;
	//		m_AddonSPKFile.read((char*)&numAddonSPK, SIZE_SPRITEID);
	//		m_AddonSPK.Init( numAddonSPK, CDirectDraw::Is565() );	
	//	}

	m_AddonManSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ADDON_MAN").c_str());
	m_AddonWomanSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ADDON_WOMAN").c_str());

	m_OustersSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_OUSTERS").c_str());
	m_VampireSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_VAMPIRE").c_str());

	m_AdvancementSlayerManSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_MAN").c_str());
	m_AdvancementSlayerWomanSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN").c_str());
	// advancedslayer* hold the Osiris Slayer parts (GetAdvancementSlayerLook).
	std::string osirisSlayerManSPK = GetOptionalFileDef("FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_MAN_171");
	if (!osirisSlayerManSPK.empty())
		m_OsirisSlayerManSPK.LoadFromFileRunning(osirisSlayerManSPK.c_str());
	std::string osirisSlayerWomanSPK = GetOptionalFileDef("FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN_171");
	if (!osirisSlayerWomanSPK.empty())
		m_OsirisSlayerWomanSPK.LoadFromFileRunning(osirisSlayerWomanSPK.c_str());
	// secondadvanced* give the tier-2 weapon art (GetWeaponArtTier).
	struct { const char* key; CIndexSpritePack* pPack; } tier2SPK[] =
	{
		{ "FILE_ISPRITE_ADVANCEMENT_CLASS_OUSTERS_201",	&m_Tier2OustersSPK },
		{ "FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN_201",	&m_Tier2VampireManSPK },
		{ "FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_201",	&m_Tier2VampireWomanSPK },
		{ "FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_MAN_201",	&m_Tier2SlayerManSPK },
		{ "FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN_201",	&m_Tier2SlayerWomanSPK },
	};
	for (int i = 0; i < (int)(sizeof(tier2SPK) / sizeof(tier2SPK[0])); ++i)
	{
		std::string path = GetOptionalFileDef(tier2SPK[i].key);
		if (!path.empty())
			tier2SPK[i].pPack->LoadFromFileRunning(path.c_str());
	}
	m_AdvancementOustersSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ADVANCEMENT_CLASS_OUSTERS").c_str());
	// advancedousters holds the Osiris Ousters bodies (GetAdvancementOustersLook).
	std::string osirisOustersSPK = GetOptionalFileDef("FILE_ISPRITE_ADVANCEMENT_CLASS_OUSTERS_171");
	if (!osirisOustersSPK.empty())
		m_OsirisOustersSPK.LoadFromFileRunning(osirisOustersSPK.c_str());
	m_AdvancementVampireManSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN").c_str());
	m_AdvancementVampireWomanSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN").c_str());
	// advancedvampire* hold the Osiris Vampire bodies (GetAdvancementVampireLook).
	std::string osirisVampireManSPK = GetOptionalFileDef("FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN_171");
	if (!osirisVampireManSPK.empty())
		m_OsirisVampireManSPK.LoadFromFileRunning(osirisVampireManSPK.c_str());
	std::string osirisVampireWomanSPK = GetOptionalFileDef("FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_171");
	if (!osirisVampireWomanSPK.empty())
		m_OsirisVampireWomanSPK.LoadFromFileRunning(osirisVampireWomanSPK.c_str());

	// Sprite Pack
// 	m_AddonSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_ISPRITE_ADDON").c_str());
// 	m_AddonSPK.LoadFromFilePart(0, m_AddonSPK.GetSize());
// 
// 	m_OustersSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_ISPRITE_OUSTERS").c_str());
// 	m_OustersSPK.LoadFromFilePart(0, m_OustersSPK.GetSize());
// 	
// 	m_VampireSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_ISPRITE_VAMPIRE").c_str());
// 	m_VampireSPK.LoadFromFilePart(0, m_VampireSPK.GetSize());
// 
// 	m_AdvancementSlayerManSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_MAN" ).c_str() );
// 	m_AdvancementSlayerManSPK.LoadFromFilePart(0, m_AdvancementSlayerManSPK.GetSize());
// 
// 	m_AdvancementSlayerWomanSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN" ).c_str() );
// 	m_AdvancementSlayerWomanSPK.LoadFromFilePart(0, m_AdvancementSlayerWomanSPK.GetSize());
// 
// 	m_AdvancementOustersSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_ISPRITE_ADVANCEMENT_CLASS_OUSTERS" ).c_str() );
// 	m_AdvancementOustersSPK.LoadFromFilePart(0, m_AdvancementOustersSPK.GetSize());
// 
// 	m_AdvancementVampireManSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN" ).c_str() );
// 	m_AdvancementVampireManSPK.LoadFromFilePart(0, m_AdvancementVampireManSPK.GetSize());
// 
// 	m_AdvancementVampireWomanSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_ISPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN" ).c_str() );
// 	m_AdvancementVampireWomanSPK.LoadFromFilePart(0, m_AdvancementVampireWomanSPK.GetSize());


	// Shadow Sprite Pack
	m_AddonManSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_ADDON_MAN").c_str());
	m_AddonWomanSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_ADDON_WOMAN").c_str());

	m_OustersSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_OUSTERS").c_str());
	m_VampireSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_VAMPIRE").c_str());

	m_AdvancementSlayerManSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_MAN").c_str());
	m_AdvancementSlayerWomanSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN").c_str());
	std::string osirisSlayerManSSPK = GetOptionalFileDef("FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_MAN_171");
	if (!osirisSlayerManSSPK.empty())
		m_OsirisSlayerManSSPK.LoadFromFileRunning(osirisSlayerManSSPK.c_str());
	std::string osirisSlayerWomanSSPK = GetOptionalFileDef("FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN_171");
	if (!osirisSlayerWomanSSPK.empty())
		m_OsirisSlayerWomanSSPK.LoadFromFileRunning(osirisSlayerWomanSSPK.c_str());
	struct { const char* key; CShadowSpriteTypePack* pPack; } tier2SSPK[] =
	{
		{ "FILE_SSPRITE_ADVANCEMENT_CLASS_OUSTERS_201",	&m_Tier2OustersSSPK },
		{ "FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN_201",	&m_Tier2VampireManSSPK },
		{ "FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_201",	&m_Tier2VampireWomanSSPK },
		{ "FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_MAN_201",	&m_Tier2SlayerManSSPK },
		{ "FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN_201",	&m_Tier2SlayerWomanSSPK },
	};
	for (int i = 0; i < (int)(sizeof(tier2SSPK) / sizeof(tier2SSPK[0])); ++i)
	{
		std::string path = GetOptionalFileDef(tier2SSPK[i].key);
		if (!path.empty())
			tier2SSPK[i].pPack->LoadFromFileRunning(path.c_str());
	}
	m_AdvancementOustersSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_ADVANCEMENT_CLASS_OUSTERS").c_str());
	std::string osirisOustersSSPK = GetOptionalFileDef("FILE_SSPRITE_ADVANCEMENT_CLASS_OUSTERS_171");
	if (!osirisOustersSSPK.empty())
		m_OsirisOustersSSPK.LoadFromFileRunning(osirisOustersSSPK.c_str());
	m_AdvancementVampireManSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN").c_str());
	m_AdvancementVampireWomanSSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN").c_str());
	std::string osirisVampireManSSPK = GetOptionalFileDef("FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN_171");
	if (!osirisVampireManSSPK.empty())
		m_OsirisVampireManSSPK.LoadFromFileRunning(osirisVampireManSSPK.c_str());
	std::string osirisVampireWomanSSPK = GetOptionalFileDef("FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_171");
	if (!osirisVampireWomanSSPK.empty())
		m_OsirisVampireWomanSSPK.LoadFromFileRunning(osirisVampireWomanSSPK.c_str());


	// 	m_AddonSSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_SSPRITE_ADDON").c_str());
	// 	m_AddonSSPK.LoadFromFilePart(0, m_AddonSSPK.GetSize());
	// 
	// 	m_OustersSSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_SSPRITE_OUSTERS").c_str());
	// 	m_OustersSSPK.LoadFromFilePart(0, m_OustersSSPK.GetSize());
	// 
	// 	m_VampireSSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_SSPRITE_VAMPIRE").c_str());
	// 	m_VampireSSPK.LoadFromFilePart(0, m_VampireSSPK.GetSize());
	// 
	// 	m_AdvancementSlayerManSSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_MAN" ).c_str() );
	// 	m_AdvancementSlayerManSSPK.LoadFromFilePart(0, m_AdvancementSlayerManSSPK.GetSize());
	// 
	// 	m_AdvancementSlayerWomanSSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER_WOMAN" ).c_str() );
	// 	m_AdvancementSlayerWomanSSPK.LoadFromFilePart(0, m_AdvancementSlayerWomanSSPK.GetSize());
	// 
	// 	m_AdvancementOustersSSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_SSPRITE_ADVANCEMENT_CLASS_OUSTERS" ).c_str() );// ***
	// 	m_AdvancementOustersSSPK.LoadFromFilePart(0, m_AdvancementOustersSSPK.GetSize());
	// 
	// 	m_AdvancementVampireManSSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_MAN" ).c_str() );
	// 	m_AdvancementVampireManSSPK.LoadFromFilePart(0, m_AdvancementVampireManSSPK.GetSize());
	// 
	// 	m_AdvancementVampireWomanSSPK.LoadFromFileThread( g_pFileDef->getProperty( "FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE_WOMAN" ).c_str() );
	// 	m_AdvancementVampireWomanSSPK.LoadFromFilePart(0, m_AdvancementVampireWomanSSPK.GetSize());



	/*
		m_AdvancementSlayerSPK.LoadFromFileRunning( g_pFileDef->getProperty( "FILE_ISPRITE_ADVANCEMENT_CLASS_SLAYER" ).c_str() );

		m_AdvancementSlayerSSPK.LoadFromFileRunning( g_pFileDef->getProperty( "FILE_SSPRITE_ADVANCEMENT_CLASS_SLAYER" ).c_str() );
		m_AdvancementVampireSSPK.LoadFromFileRunning( g_pFileDef->getProperty( "FILE_SSPRITE_ADVANCEMENT_CLASS_VAMPIRE" ).c_str() );

	*/

	//------------------------------------------------------------	
	// ????? - Load  Clothes SpritePack	
	//------------------------------------------------------------
	// ??
	/*
	std::ifstream	AddonMaleShadowFile2;//(FILE_ISPRITE_ADDON, ios::binary);
	if (!FileOpenBinary(FILE_SSPRITE_ADDON_MALE, AddonMaleShadowFile2))
		return false;
	m_AddonMaleShadowSPK.LoadFromFile(AddonMaleShadowFile2);
	AddonMaleShadowFile2.close();
	*/

	//------------------------------------------------------------
	// ???? : ???SpritePack --> ShadowSpritePack
	//------------------------------------------------------------
	/*
	// ??
	CSpritePack AddonMaleShadowSPK;
	std::ifstream	AddonMaleShadowFile2;//(FILE_ISPRITE_ADDON, ios::binary);
	if (!FileOpenBinary("Data\\Image\\addonMaleShadow.spk", AddonMaleShadowFile2))
		return false;
	AddonMaleShadowSPK.LoadFromFile(AddonMaleShadowFile2);
	AddonMaleShadowFile2.close();

	m_AddonMaleSSPK.Init( AddonMaleShadowSPK.GetSize() );

	for (int i=0; i<AddonMaleShadowSPK.GetSize(); i++)
	{
		m_AddonMaleSSPK[i].SetPixel( AddonMaleShadowSPK[i] );
	}

	//------------------------------------------------------------
	// ?? - Save  ShadowSpritePack
	//------------------------------------------------------------
	std::ofstream	AddonMaleShadowFile(FILE_SSPRITE_ADDON_MALE, ios::binary);
	std::ofstream	AddonMaleShadowIndexFile(FILE_SSPRITEINDEX_ADDON_MALE, ios::binary);

	m_AddonMaleSSPK.SaveToFile(AddonMaleShadowFile, AddonMaleShadowIndexFile);

	AddonMaleShadowFile.close();
	AddonMaleShadowIndexFile.close();
	*/

	//------------------------------------------------------------
	// Load Addon Shadow SpritePack
	//------------------------------------------------------------
	// Male
	/*
	std::ifstream	AddonShadowFile2;//(FILE_ISPRITE_ADDON, ios::binary);
	if (!FileOpenBinary(FILE_SSPRITE_ADDON, AddonShadowFile2))
		return false;
	m_AddonSSPK.LoadFromFile(AddonShadowFile2);
	AddonShadowFile2.close();
	*/
	//	if (m_pAddonShadowManager!=NULL)
	//	{
	//		delete m_pAddonShadowManager;
	//		m_pAddonShadowManager = NULL;
	//	}
	//	if (m_pOustersShadowManager!=NULL)
	//	{
	//		delete m_pOustersShadowManager;
	//		m_pOustersShadowManager = NULL;
	//	}
		//m_AddonSSPK.Release();

	//	if (CDirect3D::IsHAL())
	//	{	
	//		//m_pAlphaEffectTextureManager = new CTexturePartManager( FILE_ASPRITE_ALPHAEFFECT, FILE_ASPRITEINDEX_ALPHAEFFECT, 70 );
	//		//m_pAddonShadowManager = new CShadowPartManager( 
	//		//									&m_AddonSSPK, 
	//		//									g_pClientConfig->MAX_TEXTUREPART_ADDON_SHADOW );
	//		m_pAddonShadowManager = new CShadowPartManager( 
	//											g_pFileDef->getProperty("FILE_SSPRITE_ADDON").c_str(), 
	////											FILE_SSPRITEINDEX_ADDON,
	//											g_pClientConfig->MAX_TEXTUREPART_ADDON_SHADOW );
	//		m_pOustersShadowManager = new CShadowPartManager( 
	//											g_pFileDef->getProperty("FILE_SSPRITE_OUSTERS").c_str(), 
	//											//											FILE_SSPRITEINDEX_ADDON,
	//											g_pClientConfig->MAX_TEXTUREPART_ADDON_SHADOW );
	//	}
	//	else
	{
		/*
		std::ifstream	AddonShadowFile2;//(FILE_ISPRITE_ADDON, ios::binary);
		if (!FileOpenBinary(FILE_SSPRITE_ADDON, AddonShadowFile2))
			return false;
		m_AddonSSPK.LoadFromFile(AddonShadowFile2);
		AddonShadowFile2.close();
		*/
	}

	//	UI_DrawProgress(7);
	//	DrawTitleLoading();

		//------------------------------------------------------------
		//
		//
		//                  ItemTile SpritePack
		//
		//
		//------------------------------------------------------------	
		/*
		m_ItemTileSPK.Init( 5, CDirectDraw::Is565() );


		TempSurface.InitFromBMP("ItemTile.bmp", 800,600, DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);

		lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*96);

		CSprite::SetColorkey(0x07E0);

		spriteID = 0;
		for (i=0; i<1; i++)
		{
			lpSurfaceTemp = lpSurface;
			for (j=0; j<5; j++)
			{
				m_ItemTileSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 64);

				lpSurfaceTemp += 64;
			}
			lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*64);
		}

		CSprite::SetColorkey(0);

		TempSurface.Unlock();

		//------------------------------------------------------------
		// Save  ItemTile SpritePack
		//------------------------------------------------------------
		std::ofstream	itemTileFile(FILE_SPRITE_ITEMTILE, ios::binary);
		std::ofstream	itemTileIndexFile(FILE_SPRITEINDEX_ITEMTILE, ios::binary);

		m_ItemTileSPK.SaveToFile(itemTileFile, itemTileIndexFile);

		itemTileFile.close();
		itemTileIndexFile.close();
		*/

		//------------------------------------------------------------	
		// Load  ItemTile SpritePack	
		//------------------------------------------------------------
	if (m_ItemTileISPK.GetSize() == 0)
	{
		//		std::ifstream	itemTileFile2;//(FILE_SPRITE_ITEMTILE, ios::binary);
		//		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_ISPRITE_ITEMTILE").c_str(), itemTileFile2))
		//			return false;
		//		m_ItemTileISPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ITEMTILE").c_str());
		//		itemTileFile2.close();
		m_ItemTileISPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ITEMTILE").c_str());
	}

	//------------------------------------------------------------
	//
	//
	//                    ItemDrop SpritePack
	//
	//
	//------------------------------------------------------------		

	//------------------------------------------------------------	
	// Load  itemDrop SpritePack	
	//------------------------------------------------------------
	if (m_ItemDropISPK.GetSize() == 0)
	{
		//		std::ifstream	itemDropFile2;//(FILE_SPRITE_itemDrop, ios::binary);
		//		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_ISPRITE_ITEMDROP").c_str(), itemDropFile2))
		//			return false;
		//		m_ItemDropISPK.LoadFromFileRunning(itemDropFile2);
		//		itemDropFile2.close();
		m_ItemDropISPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ISPRITE_ITEMDROP").c_str());
	}

	//------------------------------------------------------------
	//
	//
	//                    ItemBroken SpritePack
	//
	//
	//------------------------------------------------------------		

	//------------------------------------------------------------	
	// Load  itemBroken SpritePack	
	//------------------------------------------------------------
	if (m_ItemBrokenSPK.GetSize() == 0)
	{
		//		std::ifstream	itemBrokenFile2;//(FILE_SPRITE_itemBroken, ios::binary);
		//		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_ITEMBROKEN").c_str(), itemBrokenFile2))
		//			return false;
		//		m_ItemBrokenSPK.LoadFromFileRunning(itemBrokenFile2);
		//		itemBrokenFile2.close();
		m_ItemBrokenSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SPRITE_ITEMBROKEN").c_str());
	}

	//	UI_DrawProgress(8);
	//	DrawTitleLoading();

		//------------------------------------------------------------
		//
		//
		//                    Tile Sprite
		//
		//
		//------------------------------------------------------------		
		/*

		//------------------------------------------------------------
		//
		//                 Tile SpritePack
		//
		//------------------------------------------------------------
		m_TileSPK.Init( 20, CDirectDraw::Is565() );


		TempSurface.InitFromBMP("Tile.bmp", 800,600, DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);

		lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*96);

		spriteID = 0;
		for (i=0; i<2; i++)
		{
			lpSurfaceTemp = lpSurface;
			for (j=0; j<10; j++)
			{
				m_TileSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 32);

				lpSurfaceTemp += 64;
			}
			lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*32);
		}

		TempSurface.Unlock();

		//------------------------------------------------------------
		// Save  Tile SpritePack
		//------------------------------------------------------------
		std::ofstream	tileFile(FILE_SPRITE_TILE, ios::binary);
		std::ofstream	tileIndexFile(FILE_SPRITEINDEX_TILE, ios::binary);

		m_TileSPK.SaveToFile(tileFile, tileIndexFile);

		tileFile.close();
		tileIndexFile.close();

		//------------------------------------------------------------
		// Load  Tile SpritePack
		//------------------------------------------------------------
		std::ifstream	tileFile2(FILE_SPRITE_TILE, ios::binary);
		m_TileSPK.LoadFromFile(tileFile2);
		tileFile2.close();
		*/


		//------------------------------------------------------------	
		//
		//                 Tile SpriteSet
		//
		//------------------------------------------------------------
		/*
		CSpriteFilePositionArray	TileSFPArray;
		TileSFPArray.Init( 6 );

		// TileSPK Index?? Load???.
		CFileIndexTable		TileIndex;

		std::ifstream TilePackIndexFile(FILE_SPRITEINDEX_TILE, ios::binary);
		TileIndex.LoadFromFile( TilePackIndexFile );
		TilePackIndexFile.close();

		// TileSPK???? ????? SpriteID?? Tile?????? Set?? ???????.
		TYPE_SPRITEID	TileSpriteID[6] = { 9, 10, 11, 12, 13, 19 };
		for (TYPE_SPRITEID t=0; t<6; t++)
		{
			TileSFPArray[t].SpriteID		= TileSpriteID[t];
			TileSFPArray[t].FilePosition	= TileIndex[ TileSpriteID[t] ];
		}
		*/

		//------------------------------------------------------------
		//  Init Tile SpriteSet
		//------------------------------------------------------------
		//int size;
		///*
	//	if (m_TileSPK.GetSize()==0)
	//	{
	//		std::ifstream TilePackIndexFile;//(FILE_SPRITEINDEX_TILE, ios::binary);
	//		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITEINDEX_TILE").c_str(), TilePackIndexFile))
	//			return false;
	//		
	//		//-----------------------------------------------------------
	//		// Index?? Load???.
	//		//-----------------------------------------------------------
	//		m_TileSPKI.LoadFromFile( TilePackIndexFile );
	//		TilePackIndexFile.close();	
	//
	//		m_TileSPK.Init( m_TileSPKI.GetSize() );
	//
	//		/*
	//		for (int q=0; q<m_TileSPKI.GetSize(); q++)
	//		{
	//			DEBUG_ADD_FORMAT("[%d] %ld", q, m_TileSPKI[q]);
	//		}
	//		*/
	//
	//		//m_TileSPKFile.open(FILE_SPRITE_TILE, ios::binary);
	////		if (!FileOpenBinary(FILE_SPRITE_TILE, m_TileSPKFile))
	////			return false;
	//	}

	//	UI_DrawProgress(9);
	//	DrawTitleLoading();

		//*/

		/* TILE INDEX ?????
		CSpritePack spk;
		std::ifstream TilePackIndexFile(FILE_SPRITE_TILE, ios::binary);
		spk.LoadFromFile(TilePackIndexFile);
		TilePackIndexFile.close();

		std::ofstream TilePackIndexFile2(FILE_SPRITE_TILE, ios::binary);
		std::ofstream TilePackIndexFile3(FILE_SPRITEINDEX_TILE, ios::binary);
		spk.SaveToFile(TilePackIndexFile2, TilePackIndexFile3);
		TilePackIndexFile2.close();
		TilePackIndexFile3.close();

		m_TileSPK.Init( spk.GetSize(), CDirectDraw::Is565() );
		*/


		/*
		std::ifstream	TilePackFile(FILE_SPRITE_TILE, ios::binary);
		m_TileSPK.LoadFromFilePart(TilePackFile, TileSFPArray);
		TilePackFile.close();
		*/
	m_TileSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SPRITE_TILE").c_str());
	//	m_TileSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_SPRITE_TILE").c_str() );


		//------------------------------------------------------------	
		//
		//                 ImageObject SpriteSet
		//
		//------------------------------------------------------------	
		//
		// Index???? SPK?? Index???????
		/*
		CSpritePack tempSPK;
		std::ifstream	ioFile2(FILE_SPRITE_IMAGEOBJECT, ios::binary);
		tempSPK.LoadFromFile(ioFile2);
		ioFile2.close();

		std::ofstream	ioFile(FILE_SPRITE_IMAGEOBJECT, ios::binary);
		std::ofstream	ioIndexFile(FILE_SPRITEINDEX_IMAGEOBJECT, ios::binary);

		tempSPK.SaveToFile(ioFile, ioIndexFile);

		ioFile.close();
		ioIndexFile.close();
		*/

		/*
		CSpriteFilePositionArray	ImageObjectSFPArray;
		ImageObjectSFPArray.Init( 21 );

		// ImageObjectSPK Index?? Load???.
		CFileIndexTable		ImageObjectIndex;

		std::ifstream ImageObjectPackIndexFile(FILE_SPRITEINDEX_IMAGEOBJECT, ios::binary);
		ImageObjectIndex.LoadFromFile( ImageObjectPackIndexFile );
		ImageObjectPackIndexFile.close();

		// ImageObjectSPK???? ????? SpriteID?? ImageObject?????? Set?? ???????.
		TYPE_SPRITEID	ImageObjectSpriteID[21] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
		for (TYPE_SPRITEID io=0; io<21; io++)
		{
			ImageObjectSFPArray[io].SpriteID		= ImageObjectSpriteID[io];
			ImageObjectSFPArray[io].FilePosition	= ImageObjectIndex[ ImageObjectSpriteID[io] ];
		}
		*/

		//------------------------------------------------------------
		//  Init ImageObject SpriteSet
		//------------------------------------------------------------	
		//int size;
	if (m_ImageObjectSPK.GetSize() == 0)
	{

		//		std::ifstream ImageObjectPackIndexFile;//(FILE_SPRITEINDEX_IMAGEOBJECT, ios::binary);
		//		if (!FileOpenBinary(FILE_SPRITEINDEX_IMAGEOBJECT, ImageObjectPackIndexFile))
		//			return false;
		//
		//		//-----------------------------------------------------------
		//		// Index?? Load???.
		//		//-----------------------------------------------------------
		//		m_ImageObjectSPKI.LoadFromFile( ImageObjectPackIndexFile );
		//		ImageObjectPackIndexFile.close();	
		//
		//		m_ImageObjectSPK.Init( m_ImageObjectSPKI.GetSize(), CDirectDraw::Is565() );
		//
		//		m_ImageObjectSPKFile.open(FILE_SPRITE_IMAGEOBJECT, ios::binary);
		m_ImageObjectSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SPRITE_IMAGEOBJECT").c_str());
		//		m_ImageObjectSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_SPRITE_IMAGEOBJECT").c_str());
				// ??.. ??? ?? ?? ???????. - -;;
				// 2001.8.20 ???ó??
				//if (!FileOpenBinary(FILE_SPRITE_IMAGEOBJECT, m_ImageObjectSPKFile))
				//	return false;

				//m_ImageObjectSPK.LoadFromFile( m_ImageObjectSPKFile );

	}

	//-----------------------------------------------------------
	// ImageObject TextureManager
	//-----------------------------------------------------------

#ifdef __3D_IMAGE_OBJECT__
//	if (CDirect3D::IsHAL())
//	{	
//		//m_pAlphaEffectTextureManager = new CTexturePartManager( FILE_ASPRITE_ALPHAEFFECT, FILE_ASPRITEINDEX_ALPHAEFFECT, 70 );
//		SAFE_DELETE( m_pImageObjectTextureManager );
//		
//		m_pImageObjectTextureManager = new CNormalSpriteTexturePartManager(
//												&m_ImageObjectSPK, 
//												g_pClientConfig->MAX_TEXTUREPART_IMAGEOBJECTSPK );
//	}
#endif


//	UI_DrawProgress(10);
//	DrawTitleLoading();

	/*
	std::ifstream	ImageObjectPackFile(FILE_SPRITE_IMAGEOBJECT, ios::binary);
	m_ImageObjectSPK.LoadFromFilePart(ImageObjectPackFile, ImageObjectSFPArray);
	ImageObjectPackFile.close();
	*/

	//------------------------------------------------------------
	//
	//
	//                   EffectSPK ????
	//
	//
	//------------------------------------------------------------
	/*
	m_EffectAlphaSPK.Init( 67, CDirectDraw::Is565() );

	spriteID = 0;
	CAlphaSprite::SetColorkey( 0x001F );
	//------------------------------------------------------------
	// FireEffect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("FireTest.bmp", DDSCAPS_SYSTEMMEMORY);
	//TempSurface.InitFromBMP("FireEffect.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*154);

	for (i=0; i<8; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								110, 154);
		lpSurface += 110;
		lpSurfaceTemp += 110;
	}

	TempSurface.Unlock();
	CAlphaSprite::SetColorkey( 0 );

	//------------------------------------------------------------
	// Lightning Effect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("Lightning.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*96);

	for (i=0; i<8; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								64, 96);
		lpSurface += 64;
		lpSurfaceTemp += 64;
	}

	TempSurface.Unlock();

	//------------------------------------------------------------
	// Light + Darkness Effect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("LightEffect.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*64);

	CAlphaSprite::SetColorkey( 0x001F );
	for (i=0; i<4; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								64, 64);
		lpSurface += 64;
		lpSurfaceTemp += 64;
	}
	CAlphaSprite::SetColorkey( 0 );

	TempSurface.Unlock();

	//------------------------------------------------------------
	// Aura Effect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("Aura.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp2 = lpSurface;
	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*32*4);

	CAlphaSprite::SetColorkey( 0xFFFF );
	for (i=0; i<3; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								64*3, 32*4);
		lpSurface += 64*3;
		lpSurfaceTemp += 64*3;
	}


	lpSurface = (WORD*)((BYTE*)lpSurfaceTemp2 + lPitch*32*8);
	lpSurfaceTemp = (WORD*)((BYTE*)lpSurfaceTemp2 + lPitch*32*4);

	for (i=0; i<3; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								64*3, 32*4);
		lpSurface += 64*3;
		lpSurfaceTemp += 64*3;
	}
	CAlphaSprite::SetColorkey( 0 );

	TempSurface.Unlock();

	//------------------------------------------------------------
	// Sword Effect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("SwordEffect.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*64);

	CAlphaSprite::SetColorkey( 0xF800 );
	for (i=0; i<12; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								64, 64);
		lpSurface += 64;
		lpSurfaceTemp += 64;
	}
	CAlphaSprite::SetColorkey( 0 );

	TempSurface.Unlock();

	//------------------------------------------------------------
	// Comet Effect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("CometEffect.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*256);

	CAlphaSprite::SetColorkey( 0xFFFF );
	for (i=0; i<7; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								256, 256);
		lpSurface += 256;
		lpSurfaceTemp += 256;
	}
	CAlphaSprite::SetColorkey( 0 );

	TempSurface.Unlock();


	//------------------------------------------------------------
	// Star Effect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("StarEffect.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*120);

	CAlphaSprite::SetColorkey( 0x001F );
	for (i=0; i<12; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								160, 120);
		lpSurface += 160;
		lpSurfaceTemp += 160;
	}
	TempSurface.Unlock();

	//------------------------------------------------------------
	// Flare Effect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("FlareEffect.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*100);

	CAlphaSprite::SetColorkey( 0xF800 );
	for (i=0; i<10; i++)
	{
		m_EffectAlphaSPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								100, 100);
		lpSurface += 100;
		lpSurfaceTemp += 100;
	}
	TempSurface.Unlock();


	//------------------------------------------------------------
	//
	// Save  EffectSPK
	//
	//------------------------------------------------------------
	std::ofstream	effectFile(FILE_ASPRITE_ALPHAEFFECT, ios::binary);
	std::ofstream	effectIndexFile(FILE_ASPRITEINDEX_ALPHAEFFECT, ios::binary);

	m_EffectAlphaSPK.SaveToFile(effectFile, effectIndexFile);

	effectFile.close();
	effectIndexFile.close();
	*/

	//------------------------------------------------------------
	//
	// Load  EffectPack
	//
	//------------------------------------------------------------
	// 3d?????? ??? m_pAlphaEffectTextureManager?? ??????
	// ????, m_EffectAlphaSPK?? ??????.
/*
	std::ifstream	effectFile2;//(FILE_ASPRITE_ALPHAEFFECT, ios::binary);
	if (!FileOpenBinary(FILE_ASPRITE_ALPHAEFFECT, effectFile2))
		return false;
	m_EffectAlphaSPK.LoadFromFile(effectFile2);
	effectFile2.close();
	*/
	//------------------------------------------------------------
	// AlphaEffect
	//------------------------------------------------------------

//	SAFE_DELETE( m_pAlphaEffectTextureManager );
//	SAFE_DELETE( m_pImageObjectTextureManager );
//	m_EffectAlphaSPKI.Release();
//	m_EffectAlphaSPK.Release();

//	if (m_EffectAlphaSPKFile.is_open())
//	{
//		m_EffectAlphaSPKFile.close();
//	}

//	if (CDirect3D::IsHAL())
//	{	
//		//m_pAlphaEffectTextureManager = new CTexturePartManager( FILE_ASPRITE_ALPHAEFFECT, FILE_ASPRITEINDEX_ALPHAEFFECT, 70 );
//		//m_pAlphaEffectTextureManager = new CTexturePartManager( 
//		//								&m_EffectAlphaSPK, 
//		//								g_pClientConfig->MAX_TEXTUREPART_EFFECT );
//		m_pAlphaEffectTextureManager = new CTexturePartManager( 
//										g_pFileDef->getProperty("FILE_ASPRITE_ALPHAEFFECT").c_str(),
//// 										FILE_ASPRITEINDEX_ALPHAEFFECT, 
//										g_pClientConfig->MAX_TEXTUREPART_EFFECT );
////		m_EffectAlphaPPK.LoadFromFile( FILE_PALETTE_ALPHAEFFECT, CDirectDraw::Is565() );
//
//		
//	}
//	else
	{
		//std::ifstream	effectFile2;//(FILE_ASPRITE_ALPHAEFFECT, ios::binary);
		//if (!FileOpenBinary(FILE_ASPRITE_ALPHAEFFECT, effectFile2))
		//	return false;
		//m_EffectAlphaSPK.LoadFromFile(effectFile2);
		//effectFile2.close();	
//		std::ifstream effectFileIndex;//(FILE_ISPRITEINDEX_CREATURE, ios::binary);
//		if (!FileOpenBinary(FILE_ASPRITEINDEX_ALPHAEFFECT, effectFileIndex))
//			return false;
//		
//		// Index File?? loading??? 
//		m_EffectAlphaSPKI.LoadFromFile( effectFileIndex );
//
//		// ??????? ?????? ????.
//		m_EffectAlphaSPK.Init( m_EffectAlphaSPKI.GetSize(), CDirectDraw::Is565() );
//
//		effectFileIndex.close();	
//
//		//------------------------------------------------------------
//		// Run-time loading?? ????? file?? ??????.
//		//------------------------------------------------------------
//		if (!FileOpenBinary(FILE_ASPRITE_ALPHAEFFECT, m_EffectAlphaSPKFile))
//			return false;
		m_EffectAlphaSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_ASPRITE_ALPHAEFFECT").c_str());

		//m_EffectAlphaSPK.LoadFromFile(g_pFileDef->getProperty("FILE_ASPRITE_ALPHAEFFECT").c_str());

//		m_EffectAlphaSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_ASPRITE_ALPHAEFFECT").c_str());

		m_EffectAlphaPPK.LoadFromFile(g_pFileDef->getProperty("FILE_PALETTE_ALPHAEFFECT").c_str());
	}

	//------------------------------------------------------------
	// ScreenEffect
	//------------------------------------------------------------
//	if (m_pScreenEffectTextureManager!=NULL)
//	{
//		delete m_pScreenEffectTextureManager;
//		m_pScreenEffectTextureManager = NULL;
//	}
//	m_EffectScreenSPKI.Release();
//	m_EffectScreenSPK.Release();
//	m_EffectScreenPPK.Release();

//	if (m_EffectScreenSPKFile.is_open())
//	{
//		m_EffectScreenSPKFile.close();
//	}

//	if (CDirect3D::IsHAL())
//	{	
//		//m_pAlphaEffectTextureManager = new CTexturePartManager( FILE_ASPRITE_ALPHAEFFECT, FILE_ASPRITEINDEX_ALPHAEFFECT, 70 );
//		//m_pAlphaEffectTextureManager = new CTexturePartManager( 
//		//								&m_EffectAlphaSPK, 
//		//								g_pClientConfig->MAX_TEXTUREPART_EFFECT );
//		m_pScreenEffectTextureManager = new CSpriteTexturePartManager( 
//										g_pFileDef->getProperty("FILE_SPRITE_SCREENEFFECT").c_str(),
////										FILE_SPRITEINDEX_SCREENEFFECT, 
//										g_pClientConfig->MAX_TEXTUREPART_SCREENEFFECT );
////		m_EffectScreenPPK.LoadFromFile( FILE_PALETTE_SCREENEFFECT, CDirectDraw::Is565() );
//	}
//	else
	{
		//std::ifstream	effectFile2;//(FILE_ASPRITE_ALPHAEFFECT, ios::binary);
		//if (!FileOpenBinary(FILE_ASPRITE_ALPHAEFFECT, effectFile2))
		//	return false;
		//m_EffectAlphaSPK.LoadFromFile(effectFile2);
		//effectFile2.close();	
//		std::ifstream effectFileIndex;//(FILE_ISPRITEINDEX_CREATURE, ios::binary);
//		if (!FileOpenBinary(FILE_SPRITEINDEX_SCREENEFFECT, effectFileIndex))
//			return false;
//		
//		// Index File?? loading??? 
//		m_EffectScreenSPKI.LoadFromFile( effectFileIndex );
//
//		// ??????? ?????? ????.
//		m_EffectScreenSPK.Init( m_EffectScreenSPKI.GetSize(), CDirectDraw::Is565() );
//
//		effectFileIndex.close();	
//
//		//------------------------------------------------------------
//		// Run-time loading?? ????? file?? ??????.
//		//------------------------------------------------------------
//		if (!FileOpenBinary(FILE_SPRITE_SCREENEFFECT, m_EffectScreenSPKFile))
//			return false;
		m_EffectScreenSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SPRITE_SCREENEFFECT").c_str());
		//		m_EffectScreenSPK.LoadFromFileThread(g_pFileDef->getProperty("FILE_SPRITE_SCREENEFFECT").c_str());
				//m_EffectScreenSPK.LoadFromFile(g_pFileDef->getProperty("FILE_SPRITE_SCREENEFFECT").c_str());
		m_EffectScreenPPK.LoadFromFile(g_pFileDef->getProperty("FILE_PALETTE_SCREENEFFECT").c_str());
	}

	//------------------------------------------------------------
	//
	//		ShadowEffect
	//
	//------------------------------------------------------------
//	if (m_pEffectShadowManager!=NULL)
//	{
//		delete m_pEffectShadowManager;
//		m_pEffectShadowManager = NULL;
//	}
//	if (CDirect3D::IsHAL())
//	{	
//		m_pEffectShadowManager = new CShadowPartManager( 
//										g_pFileDef->getProperty("FILE_SSPRITE_SHADOWEFFECT").c_str(),
////										FILE_SSPRITEINDEX_SHADOWEFFECT, 
//										g_pClientConfig->MAX_TEXTUREPART_EFFECTSHADOW );
//	}
//	else
	{
		//		std::ifstream	effectFile2;
		//		if (!FileOpenBinary(FILE_SSPRITE_SHADOWEFFECT, effectFile2))
		//			return false;
		//		m_EffectShadowSPK.LoadFromFile(effectFile2);
		//		effectFile2.close();			
		m_EffectShadowSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SSPRITE_SHADOWEFFECT").c_str());
	}

	//	UI_DrawProgress(12);
	//	DrawTitleLoading();

		//std::ifstream	effectFile2(FILE_ASPRITE_ALPHAEFFECT, ios::binary);
		//m_EffectAlphaSPK.LoadFromFile(effectFile2);
		//effectFile2.close();	

		//------------------------------------------------------------
		//
		// Init Effect TextureSurface
		//
		//------------------------------------------------------------
		/*
		if (CDirect3D::IsHAL())
		{
			//--------------------------------------------
			// AlphaSPK?? ?????? TexturePack?? ???????.
			//--------------------------------------------
			if (!m_EffectTPK.Init( m_EffectAlphaSPK ))
			{
				MessageBox(g_hWnd, "Can't Init EffectTexture!", NULL, MB_OK);
				return false;
			}

			//------------------------------------------------------------
			// EffectAlphaSprite?? ??????? ???????.
			// --> TextureSurface?? ????? ??????.. ??? ????.
			//------------------------------------------------------------
			m_EffectAlphaSPK.Release();
		}
		*/

		//------------------------------------------------------------		
		//
		//                 Etc SpritePack
		//
		//------------------------------------------------------------	
		/*
		m_EtcSPK.Init( 13, CDirectDraw::Is565() );


		TempSurface.InitFromBMP("Etc.bmp", DDSCAPS_SYSTEMMEMORY);
		TempSurface.LockW(lpSurface, lPitch);

		lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*96);

		spriteID = 0;
		CSprite::SetColorkey( 0x001F );
		for (i=0; i<2; i++)
		{
			lpSurfaceTemp = lpSurface;
			int endj = (i==0)? 5 : 8;
			for (j=0; j<endj; j++)
			{
				//if (spriteID==0)
					m_EtcSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 32);
				//else
				//	m_EtcSPK[spriteID++].SetPixel(lpSurfaceTemp, lPitch, 64, 96);

				lpSurfaceTemp += 64;
			}
			lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*32);
		}
		CSprite::SetColorkey( 0 );

		TempSurface.Unlock();

		//------------------------------------------------------------
		// Save  Etc SpritePack
		//------------------------------------------------------------
		std::ofstream	EtcFile(FILE_SPRITE_ETC, ios::binary);
		std::ofstream	EtcIndexFile(FILE_SPRITEINDEX_ETC, ios::binary);

		m_EtcSPK.SaveToFile(EtcFile, EtcIndexFile);

		EtcFile.close();
		EtcIndexFile.close();
		*/

		//------------------------------------------------------------	
		// Load  Etc SpritePack	
		//------------------------------------------------------------	
	if (m_EtcSPK.GetSize() == 0)
	{
		//ivfstream	EtcFile2;//(FILE_SPRITE_ETC, ios::binary);
		//if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_ETC").c_str(), EtcFile2))
		//	return false;
		//m_EtcSPK.LoadFromFile(EtcFile2);
		//EtcFile2.close();

		if (!m_EtcSPK.LoadFromFile(g_pFileDef->getProperty("FILE_SPRITE_ETC").c_str()))
			return false;
	}

	//------------------------------------------------------------		
	//
	//                 InteractionObject SpritePack
	//
	//------------------------------------------------------------
//	if (m_InteractionObjectSPK.GetSize()==0)
//	{
//		std::ifstream	ioFile2;//(FILE_SPRITE_ETC, ios::binary);
//		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_INTERACTIONOBJECT").c_str(), ioFile2))
//			return false;
//		m_InteractionObjectSPK.LoadFromFile(ioFile2);
//		ioFile2.close();
//	}

//	if (m_InteractionObjectSSPK.GetSize()==0)
//	{
//		/*
//		std::ifstream	ioFile3;//(FILE_SPRITE_ETC, ios::binary);
//		if (!FileOpenBinary(FILE_SSPRITE_INTERACTIONOBJECT, ioFile3))
//			return false;
//		m_InteractionObjectSSPK.LoadFromFile(ioFile2);
//		ioFile3.close();
//		*/
//	}

	/*
	//------------------------------------------------------------
	//
	//      Effect Normal
	//
	//------------------------------------------------------------
	m_EffectNormalSPK.Init( 3, CDirectDraw::Is565() );

	CSprite::SetColorkey( 0 );
	//------------------------------------------------------------
	// EffectNormalSPK
	//------------------------------------------------------------
	TempSurface.InitFromBMP("Blood1.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_EffectNormalSPK[ 0 ].SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// EffectNormalSPK
	//------------------------------------------------------------
	TempSurface.InitFromBMP("Blood2.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_EffectNormalSPK[ 1 ].SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// EffectNormalSPK
	//------------------------------------------------------------
	TempSurface.InitFromBMP("Blood3.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_EffectNormalSPK[ 2 ].SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// Save  NormalEffect Shadow SpritePack
	//------------------------------------------------------------

	std::ofstream	NormalEffectFile(FILE_SPRITE_NORMALEFFECT, ios::binary);
	std::ofstream	NormalEffectIndexFile(FILE_SPRITEINDEX_NORMALEFFECT, ios::binary);

	m_EffectNormalSPK.SaveToFile(NormalEffectFile, NormalEffectIndexFile);

	NormalEffectFile.close();
	NormalEffectIndexFile.close();
	*/

	//------------------------------------------------------------	
	// Load  Clothes SpritePack	
	//------------------------------------------------------------
	if (m_EffectNormalSPK.GetSize() == 0)
	{
		//		std::ifstream	NormalEffectFile2;//(FILE_SPRITE_NORMALEFFECT, ios::binary);
		//		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_NORMALEFFECT").c_str(), NormalEffectFile2))
		//			return false;
		//		m_EffectNormalSPK.LoadFromFile(NormalEffectFile2);
		//		NormalEffectFile2.close();	
		m_EffectNormalSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SPRITE_NORMALEFFECT").c_str());
	}


	//------------------------------------------------------------
	//
	//
	//                   Weather SpritePack ????
	//
	//
	//------------------------------------------------------------
	//------------------------------------------------------------	
	// Load  Clothes SpritePack	
	//------------------------------------------------------------
	if (m_WeatherSPK.GetSize() == 0)
	{
		//ivfstream	WeatherFile2;//(FILE_SPRITE_WEATHER, ios::binary);
		//if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_WEATHER").c_str(), WeatherFile2))
		//	return false;
		//m_WeatherSPK.LoadFromFile(WeatherFile2);
		//WeatherFile2.close();

		if (!m_WeatherSPK.LoadFromFile(g_pFileDef->getProperty("FILE_SPRITE_WEATHER").c_str()))
			return false;
	}

	//------------------------------------------------------------	
	// Load  Weather FullScreen SpritePack	
	//------------------------------------------------------------
	/*
	if (m_WeatherFullScreenSPK.GetSize()==0)
	{
		ivfstream	WeatherFullScreenFile;
		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_WEATHER_FULLSCREEN").c_str(), WeatherFullScreenFile))
			return false;
		m_WeatherFullScreenSPK.LoadFromFile(WeatherFullScreenFile);
		WeatherFullScreenFile.close();
	}
	*/

	//------------------------------------------------------------
	//
	//
	//                   Shadow Test ????
	//
	//
	//------------------------------------------------------------
	/*
	CAlphaSpritePack	ShadowASPK;
	ShadowASPK.Init( 1, CDirectDraw::Is565() );

	spriteID = 0;
	CAlphaSprite::SetColorkey( 0xFFFF );
	//------------------------------------------------------------
	// FireEffect
	//------------------------------------------------------------
	TempSurface.InitFromBMP("Shadow.bmp", DDSCAPS_SYSTEMMEMORY);
	//TempSurface.InitFromBMP("FireEffect.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch*32);

	for (i=0; i<1; i++)
	{
		ShadowASPK[spriteID++].SetPixel(lpSurface, lPitch,
								lpSurfaceTemp, lPitch,
								32, 32);
		lpSurface += 32;
		lpSurfaceTemp += 32;
	}

	TempSurface.Unlock();
	CAlphaSprite::SetColorkey( 0 );

	//--------------------------------------------
	// AlphaSPK?? ?????? TexturePack?? ???????.
	//--------------------------------------------
	if (!m_ShadowTPK.Init( ShadowASPK ))
	{
		MessageBox(g_hWnd, "Can't Init ShadowTexture!", NULL, MB_OK);
		return false;
	}

	//------------------------------------------------------------
	// EffectAlphaSprite?? ??????? ???????.
	// --> TextureSurface?? ????? ??????.. ??? ????.
	//------------------------------------------------------------
	ShadowASPK.Release();
	*/

	//------------------------------------------------------------
	//
	//				Minimap	
	//
	//------------------------------------------------------------
	 // 2001.7.14 ???ó??
//	if (m_pMinimapSPR!=NULL)
//	{
//		delete m_pMinimapSPR;
//		m_pMinimapSPR = NULL;
//	}
//
//	if (CDirectDraw::Is565())
//	{
//		m_pMinimapSPR = new CSprite565;
//	}
//	else
//	{
//		m_pMinimapSPR = new CSprite555;
//	}


	/*
	CSprite::SetColorkey( 0x001F );


	//------------------------------------------------------------
	// MinimapSprite
	//------------------------------------------------------------
	TempSurface.InitFromBMP("a-minimap.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_pMinimapSPR->SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// Save  Minimap Sprite
	//------------------------------------------------------------
	std::ofstream	MapTest1("Data\\Image\\map_a.spr", ios::binary);
	m_pMinimapSPR->SaveToFile( MapTest1 );
	MapTest1.close();

	//------------------------------------------------------------
	// MinimapSprite
	//------------------------------------------------------------
	TempSurface.InitFromBMP("h-minimap.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_pMinimapSPR->SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// Save  Minimap Sprite
	//------------------------------------------------------------
	std::ofstream	MapTest2("Data\\Image\\map_h.spr", ios::binary);
	m_pMinimapSPR->SaveToFile( MapTest2 );
	MapTest2.close();

	//------------------------------------------------------------
	// MinimapSprite
	//------------------------------------------------------------
	TempSurface.InitFromBMP("e-minimap.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_pMinimapSPR->SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// Save  Minimap Sprite
	//------------------------------------------------------------
	std::ofstream	MapTest3("Data\\Image\\map_e.spr", ios::binary);
	m_pMinimapSPR->SaveToFile( MapTest3 );
	MapTest3.close();

	//------------------------------------------------------------
	// MinimapSprite
	//------------------------------------------------------------
	TempSurface.InitFromBMP("c-minimap.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_pMinimapSPR->SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// Save  Minimap Sprite
	//------------------------------------------------------------
	std::ofstream	MapTest4("Data\\Image\\map_c.spr", ios::binary);
	m_pMinimapSPR->SaveToFile( MapTest4 );
	MapTest4.close();

	//------------------------------------------------------------
	// MinimapSprite
	//------------------------------------------------------------
	TempSurface.InitFromBMP("d-minimap.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);
	m_pMinimapSPR->SetPixel(lpSurface, lPitch, TempSurface.GetWidth(), TempSurface.GetHeight());
	TempSurface.Unlock();

	//------------------------------------------------------------
	// Save  Minimap Sprite
	//------------------------------------------------------------
	std::ofstream	MapTest5("Data\\Image\\map_d.spr", ios::binary);
	m_pMinimapSPR->SaveToFile( MapTest5 );
	MapTest5.close();

	*/

	//------------------------------------------------------------
	//
	//
	//                   Guild SpritePack ????
	//
	//
	//------------------------------------------------------------
	//------------------------------------------------------------	
	// Load  Guild SpritePack	
	//------------------------------------------------------------
	/*
	if (m_GuildSPK.GetSize()==0)
	{
		std::ifstream	guildFile2;//(FILE_SPRITE_WEATHER, ios::binary);
		if (!FileOpenBinary(FILE_SPRITE_GUILD, guildFile2))
			return false;
		m_GuildSPK.LoadFromFile(guildFile2);
		guildFile2.close();
	}
	*/

	//	UI_DrawProgress(13);
	//	DrawTitleLoading();

	return true;
}

//----------------------------------------------------------------------
// Filter ????
//----------------------------------------------------------------------
bool
MTopView::InitFilters()
{
	/*
	TYPE_FILTERID	filterID;

	CSpriteSurface	TempSurface;

	WORD	*lpSurface,
			*lpSurfaceTemp;

	WORD	lPitch;

	int i,j;
	*/

	//------------------------------------------------------------		
	//
	//             Filter Tile SpritePack
	//
	//------------------------------------------------------------	
	/*
	m_Filter.Init( 105 );

	TempSurface.InitFromBMP("FogTile48.bmp", DDSCAPS_SYSTEMMEMORY);
	TempSurface.LockW(lpSurface, lPitch);

	lpSurfaceTemp = (WORD*)((BYTE*)lpSurface + lPitch * TILE_Y*3);

	filterID = 0;

	for (i=0; i<12; i++)
	{
		lpSurfaceTemp = lpSurface;
		for (j=0; j<10; j++)
		{
			m_Filter[filterID++].SetFilter(lpSurfaceTemp, lPitch, TILE_X, TILE_Y);

			lpSurfaceTemp += TILE_X;

			if (filterID%35==0) break;
		}
		lpSurface = (WORD*)((BYTE*)lpSurface + lPitch*TILE_Y);
	}

	TempSurface.Unlock();

	//------------------------------------------------------------
	// Save  Light2D FilterPack
	//------------------------------------------------------------
	std::ofstream	FilterLight2DFile(FILE_FILTER_LIGHT2D, ios::binary);
	m_Filter.SaveToFile(FilterLight2DFile);
	FilterLight2DFile.close();
	*/

	//------------------------------------------------------------	
	// Load  Light2D FilterPack
	//------------------------------------------------------------	
	//std::ifstream	FilterLight2DFile2(FILE_FILTER_LIGHT2D, ios::binary);
	//m_Filter.LoadFromFile(FilterLight2DFile2);
	//FilterLight2DFile2.close();


	//------------------------------------------------------------	
	//
	//  3D ?þ?ó???? ???? Light Filter
	//
	//------------------------------------------------------------	
	//-----------------------------------------------
	// LightBuffer Texture ????
	//-----------------------------------------------	
	if (m_pLightBufferTexture != NULL)
	{
		//m_pLightBufferTexture->Restore();
		delete m_pLightBufferTexture;
		m_pLightBufferTexture = NULL;
	}
	//------------------------------------------------------------	
	// 2D light???? ?? ???? ??????? ????? pixel???
	//------------------------------------------------------------	
	if (m_p2DLightPixelWidth != NULL)
	{
		delete[] m_p2DLightPixelWidth;
		m_p2DLightPixelWidth = NULL;
	}

	if (m_p2DLightPixelHeight != NULL)
	{
		delete[] m_p2DLightPixelHeight;
		m_p2DLightPixelHeight = NULL;
	}


	if (CDirect3D::IsHAL())
	{
		/*
		//------------------------------------------------------
		// Light FilterPack ????
		//------------------------------------------------------
		m_LightFTP.Init( MAX_LIGHT_SETSIZE );

		//------------------------------------------------------
		// LightFilter ????
		//------------------------------------------------------
		int maxLight = 14;
		int maxCenter = 9;
		int lightRange[MAX_LIGHT_SETSIZE] = { 20, 26, 36, 46, 56, 66, 76, 86, 96, 106, 116, 126};
		int l,i,j;

		// MAX_LIGHT_SETSIZE??? ????? ??? ?????? ???.
		for (l=0; l<MAX_LIGHT_SETSIZE; l++)
		{
			int lightRangeHalf = lightRange[l]>>1;
			int maxK = maxCenter,maxLight;
			float gap = (float)(maxK,1)/(float)lightRangeHalf;	// k???? 0~24????..

			m_LightFTP[l].Init( lightRange[l], lightRange[l] );
			BYTE* pLight;
			for (i=0; i<lightRange[l]; i++)
			{
				pLight = m_LightFTP[l].GetFilter( i );
				for (j=0; j<lightRange[l]; j++)
				{
					int k = sqrt(
								abs(i-lightRangeHalf)*abs(i-lightRangeHalf)
								, abs(j-lightRangeHalf)*abs(j-lightRangeHalf)
							);

					k = (float)k * gap;

					if (k <= maxCenter) k = 0;			// maxCenter?????? 0
					else if (k >= maxK) k = maxLight;	// ?????? maxLight
					else k = k - maxCenter;				// ???????? - maxCenter

					// ????? SET??? ?????.
					//k >>= shiftValue;

					//*pLight = k;
					*pLight = maxLight - k;
					pLight ++;
				}
			}
		}

		//------------------------------------------------------------
		// Save  Light3D FilterPack
		//------------------------------------------------------------
		std::ofstream	LightFilter3DFile(FILE_FILTER_LIGHT3D, ios::binary);
		m_LightFTP.SaveToFile(LightFilter3DFile);
		LightFilter3DFile.close();

		*/

		//------------------------------------------------------------	
		// Load  Light3D FilterPack
		//------------------------------------------------------------	
		//ivfstream	LightFilter3DFile2;//(FILE_FILTER_LIGHT3D, ios::binary);
		//if (!FileOpenBinary(g_pFileDef->getProperty("FILE_FILTER_LIGHT3D").c_str(), LightFilter3DFile2))
		//	return false;
		if (!m_LightFTP.LoadFromFile(g_pFileDef->getProperty("FILE_FILTER_LIGHT3D").c_str()))
			return false;
		//LightFilter3DFile2.close();

		//------------------------------------------------------
		// lightBuffer ????
		//------------------------------------------------------		
		m_LightBufferFilter.Init(SCREENLIGHT_WIDTH, SCREENLIGHT_HEIGHT);

		m_pLightBufferTexture = new CSpriteSurface;
		m_pLightBufferTexture->InitTextureSurface(
			SCREENLIGHT_WIDTH, SCREENLIGHT_HEIGHT,
			0, CDirect3D::GetPixelFormat4444());

	}
	//------------------------------------------------------------	
	//
	//  2D ?þ?ó???? ???? Light Filter
	//
	//------------------------------------------------------------	
	else
	{
		/*
		//------------------------------------------------------
		// Light FilterPack ????
		//------------------------------------------------------
		m_LightFTP.Init( MAX_LIGHT_SETSIZE );

		//------------------------------------------------------
		// LightFilter ????
		//------------------------------------------------------
		int maxLight = 14;
		int maxCenter = 9;
		int lightRange[MAX_LIGHT_SETSIZE] = { 20, 26, 36, 46, 56, 66, 76, 86, 96, 106, 116, 126 };
		int l,i,j;

		// MAX_LIGHT_SETSIZE??? ????? ??? ?????? ???.
		for (l=0; l<MAX_LIGHT_SETSIZE; l++)
		{
			int lightRangeHalf = lightRange[l]>>1;
			int maxK = maxCenter,maxLight;
			float gap = (float)(maxK,1)/(float)lightRangeHalf;	// k???? 0~24????..

			m_LightFTP[l].Init( lightRange[l], lightRange[l] );
			BYTE* pLight;
			for (i=0; i<lightRange[l]; i++)
			{
				pLight = m_LightFTP[l].GetFilter( i );
				for (j=0; j<lightRange[l]; j++)
				{
					int k = sqrt(
								abs(i-lightRangeHalf)*abs(i-lightRangeHalf)
								, abs(j-lightRangeHalf)*abs(j-lightRangeHalf)
							);

					k = (float)k * gap;

					if (k <= maxCenter) k = 0;			// maxCenter?????? 0
					else if (k >= maxK) k = maxLight;	// ?????? maxLight
					else k = k - maxCenter;				// ???????? - maxCenter

					// ????? SET??? ?????.
					//k >>= shiftValue;

					//*pLight = k;
					*pLight = (maxLight-k)<<1;//31 - ((maxLight - k)<<1);
					pLight ++;
				}
			}
		}

		//------------------------------------------------------------
		// Save  Light2D FilterPack
		//------------------------------------------------------------
		std::ofstream	LightFilter2DFile(FILE_FILTER_LIGHT2D, ios::binary);
		m_LightFTP.SaveToFile(LightFilter2DFile);
		LightFilter2DFile.close();
		*/

		//------------------------------------------------------
		// lightBuffer ????
		//------------------------------------------------------		
		m_LightBufferFilter.Init(SCREENLIGHT_WIDTH, SCREENLIGHT_HEIGHT);

		//------------------------------------------------------------	
		// Load  Light2D FilterPack
		//------------------------------------------------------------	
		//ivfstream	LightFilter2DFile2;//(FILE_FILTER_LIGHT2D, ios::binary);
		//if (!FileOpenBinary(g_pFileDef->getProperty("FILE_FILTER_LIGHT2D").c_str(), LightFilter2DFile2))
		//	return false;
		if (!m_LightFTP.LoadFromFile(g_pFileDef->getProperty("FILE_FILTER_LIGHT2D").c_str()))
			return false;
		//LightFilter2DFile2.close();

		m_p2DLightPixelWidth = new int[SCREENLIGHT_WIDTH];
		m_p2DLightPixelHeight = new int[SCREENLIGHT_HEIGHT];


		//----------------------------------------------------------------
		// ?????? SCREENLIGHT_HEIGHT	
		// total???? CLIPSURFACE_HEIGHT?? ?????? ???.
		// ????? 600.  10*24 + 9*40	
		//----------------------------------------------------------------
		// ?????? ?? = 100 * 8 = 800

		const int LIGHT_PIXEL_BUF_SIZE = 100;


		//----------------------------------------------------------------
		// LightBuffer?? ?? ???? ?????? ?????? pixel???? ????
		//----------------------------------------------------------------
		// 1024 = 16 * 64
		// 1024 = 12*29 + 13*52 
		std::vector<int> pixelWidth(SCREENLIGHT_WIDTH, 12);
		int pixelWidthCount = std::accumulate(pixelWidth.begin(), pixelWidth.end(), 0);
		int pixelWidthMargin = SURFACE_WIDTH - pixelWidthCount;

		{
			std::vector<int>::iterator iter = pixelWidth.begin();
			int i = 0;

			for (; i < pixelWidthMargin && iter != pixelWidth.end(); ++i, ++iter)
				++(*iter);
		}

		pixelWidthCount = std::accumulate(pixelWidth.begin(), pixelWidth.end(), 0);
		if (pixelWidthCount != SURFACE_WIDTH)
			MessageBox(NULL, "Init light filter failed", "InitFilters", MB_OK);

		std::shuffle(pixelWidth.begin(), pixelWidth.end(), std::mt19937{std::random_device{}()});

		//----------------------------------------------------------------
		// LightBuffer?? ?? ???? ?????? ?????? pixel???? ????
		//----------------------------------------------------------------
		// 768 = 12 * 64
		// 768 = 9*42 + 10*39
		std::vector<int> pixelHeight(SCREENLIGHT_HEIGHT, 9);
		int pixelHeightCount = std::accumulate(pixelHeight.begin(), pixelHeight.end(), 0);
		int pixelHeightMargin = SURFACE_HEIGHT - pixelHeightCount;

		{
			std::vector<int>::iterator iter = pixelHeight.begin();
			int i = 0;

			for (; i < pixelHeightMargin && iter != pixelHeight.end(); ++i, ++iter)
				++(*iter);
		}

		pixelHeightCount = std::accumulate(pixelHeight.begin(), pixelHeight.end(), 0);
		if (pixelHeightCount != SURFACE_HEIGHT)
			MessageBox(NULL, "Init light filter failed", "InitFilters", MB_OK);

		std::shuffle(pixelHeight.begin(), pixelHeight.end(), std::mt19937{std::random_device{}()});

		{
			std::vector<int>::iterator iter = pixelWidth.begin();
			int i = 0;

			for (; i < SCREENLIGHT_WIDTH && iter != pixelWidth.end(); ++i, ++iter)
			{
				m_p2DLightPixelWidth[i] = *iter;
			}
		}

		{
			std::vector<int>::iterator iter = pixelHeight.begin();
			int i = 0;

			for (; i < SCREENLIGHT_HEIGHT && iter != pixelHeight.end(); ++i, ++iter)
			{
				m_p2DLightPixelHeight[i] = *iter;
			}
		}



		// 		if(1/*g_pUserInformation->IsResolution1024*/)
		// 		{
		// 
		// 			//----------------------------------------------------------------
		// 			// LightBuffer?? ?? ???? ?????? ?????? pixel???? ????
		// 			//----------------------------------------------------------------
		// 			// 1024 = 16 * 64
		// 			// 1024 = 12*29 + 13*52 
		// 			int pPixelWidth[LIGHT_PIXEL_BUF_SIZE] = 
		// 			{
		// 				12, 13, 13, 13, 12, 13, 13, 13, 12,//12*3
		// 				13, 13, 13, 12, 13, 12, 13, 13, 13,//12*2
		// 				13, 13, 12, 13, 12, 13, 12, 13, 13,//12*3
		// 				13, 12, 13, 12, 13, 12, 13, 12, 13,//12*4
		// 				12, 13, 12, 13, 12, 13, 12, 13, 12,//12*5
		// 				13, 12, 13, 12, 13, 12, 13, 12, 13,//12*4
		// 				13, 13, 12, 13, 12, 13, 12, 13, 13,//12*3
		// 				13, 13, 13, 12, 13, 12, 13, 13, 13,//12*2 12*29 + 13*52 
		// 				12, 13, 13, 13, 12, 13, 13, 13, 12, //12*3 tot == 29
		// 			};
		// 
		// 			//----------------------------------------------------------------
		// 			// LightBuffer?? ?? ???? ?????? ?????? pixel???? ????
		// 			//----------------------------------------------------------------
		// 			// 768 = 12 * 64
		// 			// 768 = 9*42 + 10*39
		// 			int pPixelHeight[LIGHT_PIXEL_BUF_SIZE] = 
		// 			{
		// 				10,9,9,10, 9,9,10,10,9,
		// 				10,9,9,10, 9,9,10,10,9,
		// 				10,9,9,10, 9,9,10,10,10,
		// 				10,9,9,10, 9,9,10,10,9,
		// 				10,9,9,10, 9,9,10,10,9,
		// 				10,9,9,10, 9,9,10,10,10,
		// 				10,9,9,10, 9,9,10,10,9,
		// 				10,9,9,10, 9,9,10,10,9,
		// 				10,9,9,10, 9,9,10,10,10,
		// 			};
		// 
		// 			for (int i=0; i<SCREENLIGHT_WIDTH; i++)
		// 			{
		// 				m_p2DLightPixelWidth[i] = pPixelWidth[i];
		// 			}
		// 
		// 			for (i=0; i<SCREENLIGHT_HEIGHT; i++)
		// 			{
		// 				m_p2DLightPixelHeight[i] = pPixelHeight[i];
		// 			}
		// 
		// 		}
		// 		else
		// 		{
		// 			
		// 			//----------------------------------------------------------------
		// 			// LightBuffer?? ?? ???? ?????? ?????? pixel???? ????
		// 			//----------------------------------------------------------------
		// 			int pPixelWidth[LIGHT_PIXEL_BUF_SIZE] = 
		// 			{
		// 				12, 13, 12, 13, 12,	13, 12, 13, 
		// 				12, 13, 12, 13, 12, 13, 12, 13, 
		// 				12, 13, 12, 13, 12, 13, 12, 13, 
		// 				12, 13, 12, 13, 12, 13, 12, 13, 
		// 				12, 13, 12, 13, 12, 13, 12, 13, 
		// 				12, 13, 12, 13, 12, 13, 12, 13, 
		// 				12, 13, 12, 13, 12, 13, 12, 13, 
		// 				12, 13, 12, 13, 12, 13, 12, 13 
		// 			};
		// 			
		// 			//----------------------------------------------------------------
		// 			// LightBuffer?? ?? ???? ?????? ?????? pixel???? ????
		// 			//----------------------------------------------------------------
		// 			int pPixelHeight[LIGHT_PIXEL_BUF_SIZE] = 
		// 			{
		// 				10,9,9,10, 9,9,10,9,
		// 				10,9,9,10, 9,9,10,9,
		// 				10,9,9,10, 9,9,10,9,
		// 				10,9,9,10, 9,9,10,9,
		// 				10,9,9,10, 9,9,10,9,
		// 				10,9,9,10, 9,9,10,9,
		// 				10,9,9,10, 9,9,10,9,
		// 				10,9,9,10, 9,9,10,9,
		// 			};
		// 
		// 				
		// 			for (int i=0; i<SCREENLIGHT_WIDTH; i++)
		// 			{
		// 				m_p2DLightPixelWidth[i] = pPixelWidth[i];
		// 			}
		// 
		// 			for (i=0; i<SCREENLIGHT_HEIGHT; i++)
		// 			{
		// 				m_p2DLightPixelHeight[i] = pPixelHeight[i];
		// 			}
		// 		}
	}



	//------------------------------------------------------------
	//
	// Player?? ?????? ImageObject???? ó???? Filter
	//
	//------------------------------------------------------------
	/*
	int i;
	int k;

	//
	m_ImageObjectFilter.Init(200,200);
	for (i=0; i<200; i++)
	{
		for (int j=0; j<200; j++)
		{
			k = sqrt(abs(i-100)*abs(i-100) ,
					abs(j-100)*abs(j-100));
			//k = sqrt(abs(i-100)*abs(i-100) ,
					//abs(j-200)/2*abs(j-200)/2);

			//k -= 50;	// ?? ????

			k >>= 2;	// ?????(?) ????

			k += 5;


			if (k>32) k=32;
			else if (k<1) k=1;

			// [ TEST CODE ]
			//if (k>25 && k<31)
			//	k = (rand()%(k-22))? 32:26;
			//else if (k>20 && k<26)
			//	k = (rand()%(77 - k*3))? 26:32;
			//else if (k>15 && k<21)
			//	k = (rand()%(102 - k*5))? 26:32;

			// 0?? 1????...
			if (k > 26)
				k = 1;
			else
				k = 0;

			m_ImageObjectFilter.SetFilter(j,i, k);
		}
	}

	//------------------------------------------------------------
	// Save  Light3D FilterPack
	//------------------------------------------------------------
	std::ofstream	ImageObjectFilterFile(FILE_FILTER_IMAGEOBJECT, ios::binary);
	m_ImageObjectFilter.SaveToFile(ImageObjectFilterFile);
	ImageObjectFilterFile.close();
	*/

	//------------------------------------------------------------	
	// Load  Light3D FilterPack
	//------------------------------------------------------------	
	if (m_ImageObjectFilter.IsNotInit())
	{
		ivfstream	ImageObjectFilterFile2;//(FILE_FILTER_IMAGEOBJECT, ios::binary);
		if (!FileOpenBinary(g_pFileDef->getProperty("FILE_FILTER_IMAGEOBJECT").c_str(), ImageObjectFilterFile2))
			return false;
		m_ImageObjectFilter.LoadFromFile(ImageObjectFilterFile2);
		ImageObjectFilterFile2.close();
	}

	return true;
}

//----------------------------------------------------------------------
// Character?? ?????? ???????.
//----------------------------------------------------------------------
// m_CreatureFrame[????][Action][Direction][Frame]
//----------------------------------------------------------------------
bool
MTopView::InitCreatureFrames()
{
	//------------------------------------------------------------
	//
	//  Creature Body?? Frame ?????? ???????.
	//
	//------------------------------------------------------------

	/*
	m_CreatureFPK.Init(MAX_CREATURE_BODY);

	//-----------------------
	// ù??° Creature(Woman)
	//-----------------------
	// ???? ???? ????
	m_CreatureFPK[0].Init(ACTION_MAX);

	// n??° Sprite
	WORD n = 0;
	int i,j,k;

	// ?? ????? 8?????? FrameArray?? ???.
	for (k=0; k<ACTION_MAX; k++)
		m_CreatureFPK[0][k].Init(8);

	// 0~8??° ?????? ???? 8???? Frame?? ???.
	for (i=0; i<8; i++)
	{
		for (k=0; k<ACTION_MAX; k++)
			m_CreatureFPK[0][k][i].Init(8);

		// 8???? Frame?? ???? ?????? Set???.
		for (j=0; j<8; j++)
		{
			for (k=0; k<ACTION_MAX; k++)
				m_CreatureFPK[0][k][i][j].Set(n, 0,-40);

			n ++;
		}
	}

	//-----------------------
	// ???° Creature(Skeleton)
	//-----------------------
	// ???? ???? ????
	m_CreatureFPK[1].Init(ACTION_MAX);

	// ?? ????? 8?????? FrameArray?? ???.
	for (k=0; k<ACTION_MAX; k++)
		m_CreatureFPK[1][k].Init(8);

	// sprite ????
	int step[] = { 2,3,4,3,2,1,0,1 };


	// n??° Sprite
	n = 64;

	// 0~8??° ?????? ???? 8???? Frame?? ???.
	for (i=0; i<8; i++)
	{
		for (k=0; k<ACTION_MAX; k++)
			m_CreatureFPK[1][k][i].Init(8);

		// 8???? Frame?? ???? ?????? Set???.
		for (j=0; j<8; j++)
		{
			for (k=0; k<ACTION_MAX; k++)
				m_CreatureFPK[1][k][i][j].Set(n,step[j], 0,-40);
		}

		n += 5;
	}


	//-----------------------
	// ????° Creature(Vamp1)
	//-----------------------
	// ???? ???? ????
	m_CreatureFPK[2].Init(ACTION_MAX_VAMPIRE);

	// n??° Sprite
	n = 104;

	// ?? ????? 8?????? FrameArray?? ???.
	for (k=0; k<ACTION_MAX_VAMPIRE; k++)
		m_CreatureFPK[2][k].Init(8);

	//------------------------------------------------
	// ACTION_STAND - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_STAND][i].Init(16);

		int index = 0;
		// 4???? Frame?? ???? ?????? Set???.
		for (j=0; j<4; j++)
		{
			m_CreatureFPK[2][ACTION_STAND][i][index++].Set(n, 15,-60);
			m_CreatureFPK[2][ACTION_STAND][i][index++].Set(n, 15,-60);
			m_CreatureFPK[2][ACTION_STAND][i][index++].Set(n, 15,-60);
			m_CreatureFPK[2][ACTION_STAND][i][index++].Set(n, 15,-60);
			n ++;
		}
	}

	//------------------------------------------------
	// ACTION_MOVE - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_MOVE][i].Init(8);

		// 8???? Frame?? ???? ?????? Set???.
		for (j=0; j<7; j++)
		{
			// 0~7 frame
			m_CreatureFPK[2][ACTION_MOVE][i][j].Set(n, 15,-60);
			n ++;
		}
		// 8??° frame
		m_CreatureFPK[2][ACTION_MOVE][i][7].Set(n-7, 15,-60);
	}

	//------------------------------------------------
	// ACTION_ATTACK - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_ATTACK][i].Init(8);

		// 8???? Frame?? ???? ?????? Set???.
		for (j=0; j<8; j++)
		{
			m_CreatureFPK[2][ACTION_ATTACK][i][j].Set(n, 15,-60);
			n ++;
		}
	}

	//------------------------------------------------
	// ACTION_MAGIC - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_MAGIC][i].Init(8);

		// 8???? Frame?? ???? ?????? Set???.
		k=0;
		int kk=6;
		//int index = 0;
		for (j=0; j<8; j++)
		{
			//m_CreatureFPK[2][ACTION_MAGIC][i][index++].Set(n, 15,-60-k);
			//m_CreatureFPK[2][ACTION_MAGIC][i][index++].Set(n, 15,-60-k);
			m_CreatureFPK[2][ACTION_MAGIC][i][j].Set(n, 15,-60-k);
			n ++;

			if (j==4) kk=-6;
			k += kk;
		}
	}

	//------------------------------------------------
	// ACTION_DAMAGED - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_DAMAGED][i].Init(6);

		// 6???? Frame?? ???? ?????? Set???.
		for (j=0; j<6; j++)
		{
			m_CreatureFPK[2][ACTION_DAMAGED][i][j].Set(n, 15,-60);
			n ++;
		}
	}

	//------------------------------------------------
	// ACTION_DRAINED - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_DRAINED][i].Init(7);

		// 7???? Frame?? ???? ?????? Set???.
		for (j=0; j<5; j++)
		{
			m_CreatureFPK[2][ACTION_DRAINED][i][j].Set(n, 15,-60);
			n ++;
		}

		m_CreatureFPK[2][ACTION_DRAINED][i][5].Set(n-3, 15,-60);
		m_CreatureFPK[2][ACTION_DRAINED][i][6].Set(n-4, 15,-60);
	}

	//------------------------------------------------
	// ACTION_DIE - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_DIE][i].Init(4);

		// 4???? Frame?? ???? ?????? Set???.
		for (j=0; j<4; j++)
		{
			m_CreatureFPK[2][ACTION_DIE][i][j].Set(n, 15,-60);
			//n ++;
		}
	}

	//------------------------------------------------
	// ACTION_VAMPIRE_DRAIN - 8???? ?????
	//------------------------------------------------
	for (i=0; i<8; i++)
	{
		// ????? Frame??
		m_CreatureFPK[2][ACTION_VAMPIRE_DRAIN][i].Init(7);

		// 7???? Frame?? ???? ?????? Set???.
		for (j=0; j<5; j++)
		{
			m_CreatureFPK[2][ACTION_VAMPIRE_DRAIN][i][j].Set(n, 15,-60);
			n ++;
		}

		m_CreatureFPK[2][ACTION_VAMPIRE_DRAIN][i][5].Set(n-3, 15,-60);
		m_CreatureFPK[2][ACTION_VAMPIRE_DRAIN][i][6].Set(n-4, 15,-60);
	}


	std::ofstream packFile(FILE_CFRAME_CREATURE, ios::binary);
	std::ofstream indexFile(FILE_CFRAMEINDEX_CREATURE, ios::binary);
	m_CreatureFPK.SaveToFile(packFile, indexFile);
	packFile.close();
	indexFile.close();
	*/

	///*
	// Load from File
	ivfstream file;//(FILE_CFRAME_CREATURE, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_CREATURE").c_str(), file))
		return false;
	m_CreatureFPK.LoadFromFile(file);
	file.close();


	/*
	#ifdef OUTPUT_DEBUG
		const char* strAction[] = {
				"????",				// ACTION_STOP					0
				"???",				// ACTION_MOVE					1
				"????",				// ACTION_ATTACK				2
				"????????",			// ACTION_MAGIC					3
				"?????",			// ACTION_DAMAGED				4
				"?????????",		// ACTION_DRAINED				5
				"???",				// ACTION_DIE					6
				"????",				//
				"????2",
				"????3",
				"?????",
		};

		std::ofstream AddonSpriteDefFile("SpriteDef.txt");
		int CFASize = m_CreatureFPK.GetSize();
		for(int k = 0; k < CFASize; k++)
		{

				ACTION_FRAME_ARRAY &AFA = m_CreatureFPK[k];

				int AFASize = AFA.GetSize();
				//?????? 10?? ???? ????? ?? ???? ???? ????? ???ó??
				if(10 == AFASize) {
				AddonSpriteDefFile << "MonsterType=" << k << "	MonsterName="<< (*g_pCreatureTable)[k].Name.GetString()<<"\n";
				for(int i = 0; i < AFASize; ++i)
				{
					// ??? Action?? ù??° Direction FRAME_ARRAY?? ?????´?.
					FRAME_ARRAY FA = AFA[i][0];

					int FASize = FA.GetSize();
					AddonSpriteDefFile << "Action=" << strAction[i] << ", FrameCount=" << FASize;

					std::set<int> frameSet;

					for(int j = 0; j < FASize; ++j)
					{
						CFrame &frame = FA[j];
						frameSet.insert(frame.GetSpriteID());
					}

					AddonSpriteDefFile << ", ImageCount=" << frameSet.size() << '\n';

					int minSpriteID = *frameSet.begin();

					for(j = 0; j < FASize; ++j)
					{
						CFrame &frame = FA[j];

						int id = std::distance(frameSet.begin(), frameSet.find(frame.GetSpriteID()));

						AddonSpriteDefFile <<  id + 1 << ' ';
					}

					AddonSpriteDefFile << "\n\n";
				}
			}
		}
		AddonSpriteDefFile.close();
	#endif
	//*/


	//*/	
	DrawTitleLoading();
	//m_CreatureFPK.InfoToFile("log\\CreatureFPK.txt");

	// Frame2 (????)?? ??? ?????? y??? ,40?? ?????.
	/*
	ACTION_FRAME_ARRAY &zombie = m_CreatureFPK[2];

	DIRECTION_FRAME_ARRAY &dead = zombie[ACTION_DIE];

	for (int d=0; d<8; d++)
	{
		FRAME_ARRAY &fr = dead[d];

		for (int f=0; f<fr.GetSize(); f++)
		{
			fr[f].Set( fr[f].GetSpriteID(), fr[f].GetCX(), fr[f].GetCY(),40 );
		}
	}

	std::ofstream packFile(FILE_CFRAME_CREATURE, ios::binary);
	std::ofstream indexFile(FILE_CFRAMEINDEX_CREATURE, ios::binary);
	m_CreatureFPK.SaveToFile(packFile, indexFile);
	packFile.close();
	indexFile.close();
	*/


	/*
	// Damaged 0,1?? 1,0???? ????.
	for (int ct=0; ct<m_CreatureFPK.GetSize(); ct++)
	{
		DIRECTION_FRAME_ARRAY& damaged = m_CreatureFPK[ct][ACTION_DAMAGED];

		for (int d=0; d<8; d++)
		{
			FRAME_ARRAY& dfr = damaged[d];

			// 0?? 1 framd?? ??????.
			CFrame frame0 = dfr[0];
			CFrame frame1 = dfr[1];

			CFrame temp = frame0;
			frame0 = frame1;
			frame1 = temp;
		}
	}
	*/

	//m_CreatureFPK.InfoToFile("creature.txt");


	// ???? 4frame?? 6frame???? ??? ??? 18frame????..
	/*
	DIRECTION_FRAME_ARRAY& stand = m_CreatureFPK[1][ACTION_STAND];
	DIRECTION_FRAME_ARRAY standTemp;
	DIRECTION_FRAME_ARRAY standTemp2;
	standTemp = stand;
	standTemp2.Init( 8 );
	for (int d=0; d<8; d++)
	{
		// 4frame??  6frame????..
		standTemp2[d].Init( standTemp[d].GetSize(),2 );
		for (int f=0; f<standTemp[d].GetSize(); f++)
		{
			standTemp2[d][f] = standTemp[d][f];
		}
		standTemp2[d][4] = standTemp[d][2];
		standTemp2[d][5] = standTemp[d][1];

		// 6frame?? 18frame????..
		stand[d].Init( standTemp2[d].GetSize()*3 );
		int ff=0;
		for (f=0; f<standTemp2[d].GetSize(); f++)
		{
			stand[d][ff++] = standTemp2[d][f];
			stand[d][ff++] = standTemp2[d][f];
			stand[d][ff++] = standTemp2[d][f];
		}
	}

	// ??? ???? 6frame?? 12frame????...
	DIRECTION_FRAME_ARRAY& move = m_CreatureFPK[1][ACTION_MOVE];
	DIRECTION_FRAME_ARRAY moveTemp;
	moveTemp = move;
	for (d=0; d<8; d++)
	{
		move[d].Init( moveTemp[d].GetSize()*3 );

		int ff = 0;
		for (int f=0; f<moveTemp[d].GetSize(); f++)
		{
			move[d][ff++] = moveTemp[d][f];
			move[d][ff++] = moveTemp[d][f];
			move[d][ff++] = moveTemp[d][f];
		}
	}

	std::ofstream packFile(FILE_CFRAME_CREATURE, ios::binary);
	std::ofstream indexFile(FILE_CFRAMEINDEX_CREATURE, ios::binary);
	m_CreatureFPK.SaveToFile(packFile, indexFile);
	packFile.close();
	indexFile.close();
	*/

	//------------------------------------------------------------
	//
	// Creature Shadow FPK - Loading
	//
	//------------------------------------------------------------
	ivfstream fileShadow;//(FILE_CFRAME_CREATURE, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_CREATURE_SHADOW").c_str(), fileShadow))
		return false;
	m_CreatureShadowFPK.LoadFromFile(fileShadow);
	fileShadow.close();



	DrawTitleLoading();
	//m_CreatureShadowFPK.InfoToFile("log\\CreatureShadowFPK.txt");

	//------------------------------------------------------------
	//
	//  addon?? Frame ?????? ???????.
	//
	//------------------------------------------------------------
	//
	/*
	m_AddonFPK.Init(MAX_CREATURE_ADDON);


	//--------------
	// ù??° ??
	//--------------
	// 1???? ????? ???.
	m_AddonFPK[0].Init(1);
	m_AddonFPK[1].Init(1);
	m_AddonFPK[2].Init(1);
	m_AddonFPK[3].Init(1);

	// ?? 1???? ????? 8?????? FrameArray?? ???.
	m_AddonFPK[0][0].Init(8);
	m_AddonFPK[1][0].Init(8);
	m_AddonFPK[2][0].Init(8);
	m_AddonFPK[3][0].Init(8);

	// n??° tile
	//n = n + 80;
	n = 0;

	// 0~8??° ?????? ???? 8???? Frame?? ???.
	for (i=0; i<8; i++)
	{
		m_AddonFPK[0][0][i].Init(8);
		m_AddonFPK[1][0][i].Init(8);
		m_AddonFPK[2][0][i].Init(8);
		m_AddonFPK[3][0][i].Init(8);

		// 8???? Frame?? ???? ?????? Set???.
		for (int j=0; j<8; j++)
		{
			// 83~122 : Shirt ????? ???? ???? ???
			m_AddonFPK[0][0][i][j].Set(n,step[j], 0,-40);

			// 123~162 Pants ????? ???? ???? ???
			m_AddonFPK[1][0][i][j].Set(n,40,step[j], 0,-40);

			// Boots
			m_AddonFPK[2][0][i][j].Set(n,80,step[j], 0,-40);

			// Cloak
			m_AddonFPK[3][0][i][j].Set(n,120,step[j], 0,-40);
		}

		n+=5;
	}

	packFile.open(FILE_CFRAME_ADDON, ios::binary);
	indexFile.open(FILE_CFRAMEINDEX_ADDON, ios::binary);
	m_AddonFPK.SaveToFile(packFile, indexFile);
	packFile.close();
	indexFile.close();
	*/

	///*

	ivfstream AdvancementOustersFile;//(FILE_CFRAME_ADDON_MALE, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS").c_str(), AdvancementOustersFile))
		return false;
	m_AdvancementOustersFPK.LoadFromFile(AdvancementOustersFile);
	AdvancementOustersFile.close();

	ivfstream AdvancementOustersShadowFile;//(FILE_CFRAME_ADDON_MALE, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS_SHADOW").c_str(), AdvancementOustersShadowFile))
		return false;
	m_AdvancementOustersShadowFPK.LoadFromFile(AdvancementOustersShadowFile);
	AdvancementOustersFile.close();

	// Optional: without advancedousters the Osiris items keep the ACOusters look.
	ivfstream OsirisOustersFile;
	std::string osirisOustersCFPK = GetOptionalFileDef("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS_171");
	if (!osirisOustersCFPK.empty() && FileOpenBinary(osirisOustersCFPK.c_str(), OsirisOustersFile))
	{
		m_OsirisOustersFPK.LoadFromFile(OsirisOustersFile);
		OsirisOustersFile.close();
	}

	ivfstream OsirisOustersShadowFile;
	std::string osirisOustersShadowCFPK = GetOptionalFileDef("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS_SHADOW_171");
	if (!osirisOustersShadowCFPK.empty() && FileOpenBinary(osirisOustersShadowCFPK.c_str(), OsirisOustersShadowFile))
	{
		m_OsirisOustersShadowFPK.LoadFromFile(OsirisOustersShadowFile);
		OsirisOustersShadowFile.close();
	}

	// Optional: without the advanced* packs the Osiris items keep the AC* look.
	struct { const char* key; CCreatureFramePack* pFPK; } osirisFPK[] =
	{
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_MAN_171",			&m_OsirisVampireManFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_171",			&m_OsirisVampireWomanFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_MAN_SHADOW_171",		&m_OsirisVampireManShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_SHADOW_171",	&m_OsirisVampireWomanShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN_171",				&m_OsirisSlayerManFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN_171",			&m_OsirisSlayerWomanFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN_SHADOW_171",		&m_OsirisSlayerManShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN_SHADOW_171",		&m_OsirisSlayerWomanShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS_201",	&m_Tier2OustersFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_MAN_201",	&m_Tier2VampireManFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_201",	&m_Tier2VampireWomanFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN_201",	&m_Tier2SlayerManFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN_201",	&m_Tier2SlayerWomanFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS_SHADOW_201",	&m_Tier2OustersShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_MAN_SHADOW_201",	&m_Tier2VampireManShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_SHADOW_201",	&m_Tier2VampireWomanShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN_SHADOW_201",	&m_Tier2SlayerManShadowFPK },
		{ "FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN_SHADOW_201",	&m_Tier2SlayerWomanShadowFPK },
	};
	for (int i = 0; i < (int)(sizeof(osirisFPK) / sizeof(osirisFPK[0])); ++i)
	{
		ivfstream osirisFile;
		std::string path = GetOptionalFileDef(osirisFPK[i].key);
		if (!path.empty() && FileOpenBinary(path.c_str(), osirisFile))
		{
			osirisFPK[i].pFPK->LoadFromFile(osirisFile);
			osirisFile.close();
		}
	}

	ivfstream AdvancementVampireManFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_MAN").c_str(),
		AdvancementVampireManFile))
		return false;
	m_AdvancementVampireManFPK.LoadFromFile(AdvancementVampireManFile);
	AdvancementVampireManFile.close();


	/*
#ifdef OUTPUT_DEBUG


	const char* strAddon[] = {
		"??",
		"????",
	};


	const char* strAction[] = {
		"STOP",
			"MOVE",
			"DAMAGED",
			"DIE",
			"DRAIN",
			"DRAINED",

			"ATTACK_SLOW",
			"ATTACK_NORMAL",
			"ATTACK_FAST",

			"SKILL_SLOW",
			"SKILL_NORMAL",
			"SKILL_FAST",

			"MAGIC",
			"MAGIC_ATTACK",
			"SPECIAL",
			"CREATE_WEAPON",
			"DESTROY_WEAPON",

			"MAX",
	};

	std::ofstream AddonSpriteDefFile("AdvVampireSpriteDef.txt");
	int CFASize = m_AdvancementVampireManFPK.GetSize();
	for(int k = 0; k < CFASize; k++)
	{
		ACTION_FRAME_ARRAY &AFA = m_AdvancementVampireManFPK[k];

		int AFASize = AFA.GetSize();
		//?????? 10?? ???? ????? ?? ???? ???? ????? ???ó??

		AddonSpriteDefFile << "["<< strAddon[k] << "]\n";
		for(int i = 0; i < AFASize; ++i)
		{
			// ??? Action?? ù??° Direction FRAME_ARRAY?? ?????´?.
			FRAME_ARRAY FA = AFA[i][0];

			int FASize = FA.GetSize();
			AddonSpriteDefFile << "Action=" << strAction[i] << ", FrameCount=" << FASize;

			std::set<int> frameSet;

			for(int j = 0; j < FASize; ++j)
			{
				CFrame &frame = FA[j];
				frameSet.insert(frame.GetSpriteID());
			}

			AddonSpriteDefFile << ", ImageCount=" << frameSet.size() << '\n';

			int minSpriteID = *frameSet.begin();

			for(j = 0; j < FASize; ++j)
			{
				CFrame &frame = FA[j];

				int id = std::distance(frameSet.begin(), frameSet.find(frame.GetSpriteID()));

				AddonSpriteDefFile <<  id + 1 << ' ';
			}

			AddonSpriteDefFile << "\n\n";
		}
	}
	AddonSpriteDefFile.close();
#endif
//*/


	ivfstream AdvancementVampireManShadowFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_MAN_SHADOW").c_str(),
		AdvancementVampireManShadowFile))
		return false;
	m_AdvancementVampireManShadowFPK.LoadFromFile(AdvancementVampireManShadowFile);
	AdvancementVampireManShadowFile.close();

	ivfstream AdvancementVampireWomanFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_WOMAN").c_str(),
		AdvancementVampireWomanFile))
		return false;
	m_AdvancementVampireWomanFPK.LoadFromFile(AdvancementVampireWomanFile);
	AdvancementVampireWomanFile.close();

	ivfstream AdvancementVampireWomanShadowFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_WOMAN_SHADOW").c_str(),
		AdvancementVampireWomanShadowFile))
		return false;
	m_AdvancementVampireWomanShadowFPK.LoadFromFile(AdvancementVampireWomanShadowFile);
	AdvancementVampireWomanShadowFile.close();


	ivfstream AdvancementSlayerManFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN").c_str(),
		AdvancementSlayerManFile))
		return false;

	m_AdvancementSlayerManFPK.LoadFromFile(AdvancementSlayerManFile);

	AdvancementSlayerManFile.close();


	/*
#ifdef OUTPUT_DEBUG


	const char* strAddon[] = {
		"??",
		"???",
		"??",
		"??",
		"?????",
		"?????",
		"?? AR",
		"?? SR",
		"????",
		"BIKE 1",
		"BIKE 2",
		"???",
		"BIKE 2 ????",
		"WING BIKE",
		"HOVER VEHICLE",
	};


	const char* strAction[] = {
	"STOP_SWORD",
	"STOP_BLADE",
	"STOP_GUN",
	"STOP_MACE_AND_CROSS",

	"MOVE_SWORD",
	"MOVE_BLADE",
	"MOVE_GUN",
	"MOVE_MACE_AND_CROSS",

	"ATTACK_SWORD_SLOW",
	"ATTACK_SWORD_NORMAL",
	"ATTACK_SWORD_FAST",

	"ATTACK_BLADE_SLOW",
	"ATTACK_BLADE_NORMAL",
	"ATTACK_BLADE_FAST",

	"ATTACK_AR_GUN_SLOW",
	"ATTACK_AR_GUN_NORMAL",
	"ATTACK_AR_GUN_FAST",

	"ATTACK_SR_GUN_SLOW",
	"ATTACK_SR_GUN_NORMAL",
	"ATTACK_SR_GUN_FAST",

	"SKILL_SWORD_SLOW",
	"SKILL_SWORD_NORMAL",
	"SKILL_SWORD_FAST",

	"SKILL_BLADE_SLOW",
	"SKILL_BLADE_NORMAL",
	"SKILL_BLADE_FAST",

	"SKILL_GUN_SLOW",
	"SKILL_GUN_NORMAL",
	"SKILL_GUN_FAST",

	"MAGIC",
	"MAGIC_ATTACK",

	"DRAINED",
	"DAMAGED_SWORD",
	"DAMAGED_BLADE",
	"DAMAGED_GUN",
	"DAMAGED_CROSS_MACE",
	"DIE",

	"BIKE_MOVE",
	"BIKE_STOP",

	"SPECIAL",
	"SPECIAL_2",

	"SLIDING",
	"SLIDING_END",


	"MAX",
	};

	std::ofstream AddonSpriteDefFile("AdvSlayerSpriteDef.txt");
	int CFASize = m_AdvancementSlayerManFPK.GetSize();
	for(int k = 0; k < CFASize; k++)
	{
		ACTION_FRAME_ARRAY &AFA = m_AdvancementSlayerManFPK[k];

		int AFASize = AFA.GetSize();
		//?????? 10?? ???? ????? ?? ???? ???? ????? ???ó??

		AddonSpriteDefFile << "["<< strAddon[k] << "]\n";
		for(int i = 0; i < AFASize; ++i)
		{
			// ??? Action?? ù??° Direction FRAME_ARRAY?? ?????´?.
			FRAME_ARRAY FA = AFA[i][0];

			int FASize = FA.GetSize();
			AddonSpriteDefFile << "Action=" << strAction[i] << ", FrameCount=" << FASize;

			std::set<int> frameSet;

			for(int j = 0; j < FASize; ++j)
			{
				CFrame &frame = FA[j];
				frameSet.insert(frame.GetSpriteID());
			}

			AddonSpriteDefFile << ", ImageCount=" << frameSet.size() << '\n';

			int minSpriteID = *frameSet.begin();

			for(j = 0; j < FASize; ++j)
			{
				CFrame &frame = FA[j];

				int id = std::distance(frameSet.begin(), frameSet.find(frame.GetSpriteID()));

				AddonSpriteDefFile <<  id + 1 << ' ';
			}

			AddonSpriteDefFile << "\n\n";
		}
	}
	AddonSpriteDefFile.close();
#endif
//*/






	ivfstream AdvancementSlayerManShadowFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN_SHADOW").c_str(),
		AdvancementSlayerManShadowFile))
		return false;
	m_AdvancementSlayerManShadowFPK.LoadFromFile(AdvancementSlayerManShadowFile);
	AdvancementSlayerManShadowFile.close();
	ivfstream AdvancementSlayerWomanFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN").c_str(),
		AdvancementSlayerWomanFile))
		return false;
	m_AdvancementSlayerWomanFPK.LoadFromFile(AdvancementSlayerWomanFile);
	AdvancementSlayerWomanFile.close();
	ivfstream AdvancementSlayerWomanShadowFile;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN_SHADOW").c_str(),
		AdvancementSlayerWomanShadowFile))
		return false;
	m_AdvancementSlayerWomanShadowFPK.LoadFromFile(AdvancementSlayerWomanShadowFile);

	/*/ afpk?????? action?????? ??? ????? ???
	// ???????? ????

	#define CFPK_OBJECT_ID 14

	DIRECTION_FRAME_ARRAY afa25;
	afa25.operator=(m_AdvancementSlayerManFPK[CFPK_OBJECT_ID][15]);
	DIRECTION_FRAME_ARRAY afa26;
	afa26.operator=(m_AdvancementSlayerManFPK[CFPK_OBJECT_ID][16]);

	m_AdvancementSlayerManFPK[CFPK_OBJECT_ID].Init(43);

	for(int i = 0; i < 43; ++i)
		m_AdvancementSlayerManFPK[CFPK_OBJECT_ID][i].Init(8);

	m_AdvancementSlayerManFPK[CFPK_OBJECT_ID][37] = afa25;
	m_AdvancementSlayerManFPK[CFPK_OBJECT_ID][38] = afa26;

	std::ofstream AdvancementSlayerMan(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN").c_str(), std::ios::binary);
	std::string str = g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN");
	str += "i";
	std::ofstream indexAdvancementSlayerMan(str.c_str(), std::ios::binary);
	m_AdvancementSlayerManFPK.SaveToFile(AdvancementSlayerMan, indexAdvancementSlayerMan);
////???????? ???? ?????
	afa25.operator=(m_AdvancementSlayerManShadowFPK[CFPK_OBJECT_ID][15]);
	afa26.operator=(m_AdvancementSlayerManShadowFPK[CFPK_OBJECT_ID][16]);

	m_AdvancementSlayerManShadowFPK[CFPK_OBJECT_ID].Init(43);

	for( i = 0; i < 43; ++i)
		m_AdvancementSlayerManShadowFPK[CFPK_OBJECT_ID][i].Init(8);

	m_AdvancementSlayerManShadowFPK[CFPK_OBJECT_ID][37] = afa25;
	m_AdvancementSlayerManShadowFPK[CFPK_OBJECT_ID][38] = afa26;

	std::ofstream AdvancementSlayerMan1(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN_SHADOW").c_str(), std::ios::binary);
	std::string str1 = g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_MAN_SHADOW");
	str1 += "i";
	std::ofstream indexAdvancementSlayerMan1(str1.c_str(), std::ios::binary);
	m_AdvancementSlayerManShadowFPK.SaveToFile(AdvancementSlayerMan1, indexAdvancementSlayerMan1);

////???????? ????
	afa25.operator=(m_AdvancementSlayerWomanFPK[CFPK_OBJECT_ID][15]);
	afa26.operator=(m_AdvancementSlayerWomanFPK[CFPK_OBJECT_ID][16]);

	m_AdvancementSlayerWomanFPK[CFPK_OBJECT_ID].Init(43);

	for( i = 0; i < 43; ++i)
		m_AdvancementSlayerWomanFPK[CFPK_OBJECT_ID][i].Init(8);

	m_AdvancementSlayerWomanFPK[CFPK_OBJECT_ID][37] = afa25;
	m_AdvancementSlayerWomanFPK[CFPK_OBJECT_ID][38] = afa26;

	std::ofstream AdvancementSlayerMan2(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN").c_str(), std::ios::binary);
	std::string str2 = g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN");
	str2 += "i";
	std::ofstream indexAdvancementSlayerMan2(str2.c_str(), std::ios::binary);
	m_AdvancementSlayerWomanFPK.SaveToFile(AdvancementSlayerMan2, indexAdvancementSlayerMan2);
	AdvancementSlayerWomanShadowFile.close();

////???????? ???? ?????
	afa25.operator=(m_AdvancementSlayerWomanShadowFPK[CFPK_OBJECT_ID][15]);
	afa26.operator=(m_AdvancementSlayerWomanShadowFPK[CFPK_OBJECT_ID][16]);

	m_AdvancementSlayerWomanShadowFPK[CFPK_OBJECT_ID].Init(43);
	for(  i = 0; i < 43; ++i)
		m_AdvancementSlayerWomanShadowFPK[CFPK_OBJECT_ID][i].Init(8);

	m_AdvancementSlayerWomanShadowFPK[CFPK_OBJECT_ID][37] = afa25;
	m_AdvancementSlayerWomanShadowFPK[CFPK_OBJECT_ID][38] = afa26;

	std::ofstream AdvancementSlayerMan3(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN_SHADOW").c_str(), std::ios::binary);
	std::string str3 = g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_WOMAN_SHADOW");
	str3 += "i";
	std::ofstream indexAdvancementSlayerMan3(str3.c_str(), std::ios::binary);
	m_AdvancementSlayerWomanShadowFPK.SaveToFile(AdvancementSlayerMan3, indexAdvancementSlayerMan3);
//*/





/*
//------------------------------------------------
// Load from File
//------------------------------------------------
std::ifstream AdvancementSlayerFile;//(FILE_CFRAME_ADDON_MALE, ios::binary);
if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER").c_str(), AdvancementSlayerFile))
	return false;
m_AdvancementSlayerFPK.LoadFromFile(AdvancementSlayerFile);
AdvancementSlayerFile.close();

std::ifstream AdvancementVampireFile;//(FILE_CFRAME_ADDON_MALE, ios::binary);
if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE").c_str(), AdvancementVampireFile))
	return false;
m_AdvancementVampireFPK.LoadFromFile(AdvancementVampireFile);
AdvancementVampireFile.close();

std::ifstream AdvancementSlayerShadowFile;//(FILE_CFRAME_ADDON_MALE, ios::binary);
if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_SLAYER_SHADOW").c_str(), AdvancementSlayerFile))
	return false;
m_AdvancementSlayerShadowFPK.LoadFromFile(AdvancementSlayerFile);
AdvancementSlayerFile.close();

std::ifstream AdvancementVampireShadowFile;//(FILE_CFRAME_ADDON_MALE, ios::binary);
if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_VAMPIRE_SHADOW").c_str(), AdvancementVampireFile))
	return false;
m_AdvancementVampireShadowFPK.LoadFromFile(AdvancementVampireFile);
AdvancementVampireFile.close();

*/
//------------------------------------------------
// Load from File
//------------------------------------------------
	ivfstream AddonManFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADDON_MAN").c_str(), AddonManFile2))
		return false;
	m_AddonManFPK.LoadFromFile(AddonManFile2);
	AddonManFile2.close();

	ivfstream AddonWomanFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADDON_WOMAN").c_str(), AddonWomanFile2))
		return false;
	m_AddonWomanFPK.LoadFromFile(AddonWomanFile2);
	AddonWomanFile2.close();


	/*
	#ifdef OUTPUT_DEBUG
		// ??????????? Slayer?????? ??? Frame?? ?????? ???? TXT?? ??´?.

		const char* strAction[] = {
				"????",				// ACTION_STOP					0
				"???",				// ACTION_MOVE					1
				"????",				// ACTION_ATTACK				2
				"????????",			// ACTION_MAGIC					3
				"?????",			// ACTION_DAMAGED				4
				"?????????",		// ACTION_DRAINED				5
				"???",				// ACTION_DIE					6

				"?????(SR)",		// ACTION_SLAYER_GUN_SR
				"????(AG)",			// ACTION_SLAYER_GUN_AR
				"????(SG)",			// ACTION_SLAYER_GUN_SG
				"???????(SMG)",	// ACTION_SLAYER_GUN_SMG

				"???",			// ACTION_SLAYER_SWORD
				"????",			// ACTION_SLAYER_BLADE
				"????",			// ACTION_SLAYER_SWORD_2
				"????",			// ACTION_SLAYER_BLADE_2

				"??????????",		// ACTION_SLAYER_MOTOR_MOVE
				"???????????",		// ACTION_SLAYER_MOTOR_STAND

				"?????(SR)SLOW",	// ACTION_SLAYER_GUN_SR_SLOW
				"?????(SR)FAST",	// ACTION_SLAYER_GUN_SR_FAST
				"????(AG)SLOW",		// ACTION_SLAYER_GUN_AR_SLOW
				"????(AG)FAST",		// ACTION_SLAYER_GUN_AR_FAST
				"????(SG)SLOW",		// ACTION_SLAYER_GUN_SG_SLOW
				"????(SG)FAST",		// ACTION_SLAYER_GUN_SG_FAST
				"???????(SMG)SLOW",// ACTION_SLAYER_GUN_SMG_SLOW
				"???????(SMG)FAST",// ACTION_SLAYER_GUN_SMG_FAST

				"???SLOW",		// ACTION_SLAYER_SWORD_SLOW
				"???FAST",		// ACTION_SLAYER_SWORD_FAST
				"????SLOW",		// ACTION_SLAYER_BLADE_SLOW
				"????FAST",		// ACTION_SLAYER_BLADE_FAST
				"????SLOW",		// ACTION_SLAYER_SWORD_2_SLOW
				"????FAST",		// ACTION_SLAYER_SWORD_2_FAST
				"?????SLOW",		// ACTION_SLAYER_BLADE_2_SLOW
				"?????FAST",		// ACTION_SLAYER_BLADE_2_FAST

				"????????(??)",		// ACTION_SLAYER_BATTLE_STAND_GUN
				"????????(??)",		// ACTION_SLAYER_BATTLE_STAND_SWORD
				"????????(??)",		// ACTION_SLAYER_BATTLE_STAND_BLADE

				"???(??)",			// ACTION_SLAYER_MOVE_GUN

				"????????",			// ACTION_SLAYER_MAGIC_CASTING
				"????",
				"?????",
		};

		std::ofstream AddonSpriteDefFile("AddonSpriteDef.txt");

		ACTION_FRAME_ARRAY &AFA = m_AddonManFPK[0];

		int AFASize = AFA.GetSize();

		for(int i = 0; i < AFASize; ++i)
		{
			// ??? Action?? ù??° Direction FRAME_ARRAY?? ?????´?.
			FRAME_ARRAY FA = AFA[i][0];

			int FASize = FA.GetSize();

			AddonSpriteDefFile << "Action=" << strAction[i] << ", FrameCount=" << FASize;

			std::set<int> frameSet;

			for(int j = 0; j < FASize; ++j)
			{
				CFrame &frame = FA[j];
				frameSet.insert(frame.GetSpriteID());
			}

			AddonSpriteDefFile << ", ImageCount=" << frameSet.size() << '\n';

			int minSpriteID = *frameSet.begin();

			for(j = 0; j < FASize; ++j)
			{
				CFrame &frame = FA[j];

				int id = std::distance(frameSet.begin(), frameSet.find(frame.GetSpriteID()));

				AddonSpriteDefFile <<  id + 1 << ' ';
			}

			AddonSpriteDefFile << "\n\n";
		}

		AddonSpriteDefFile.close();

	#endif
	*/

	ivfstream OustersFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_OUSTERS").c_str(), OustersFile2))
		return false;
	m_OustersFPK.LoadFromFile(OustersFile2);
	OustersFile2.close();

	ivfstream VampireFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_VAMPIRE").c_str(), VampireFile2))
		return false;
	m_VampireFPK.LoadFromFile(VampireFile2);
	VampireFile2.close();

	DrawTitleLoading();

	//------------------------------------------------
	// ????? - Load from File
	//------------------------------------------------
	ivfstream AddonManShadowFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADDON_MAN_SHADOW").c_str(), AddonManShadowFile2))
		return false;
	m_AddonManShadowFPK.LoadFromFile(AddonManShadowFile2);
	AddonManShadowFile2.close();

	ivfstream AddonWomanShadowFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_ADDON_WOMAN_SHADOW").c_str(), AddonWomanShadowFile2))
		return false;
	m_AddonWomanShadowFPK.LoadFromFile(AddonWomanShadowFile2);
	AddonWomanShadowFile2.close();

	ivfstream OustersShadowFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_OUSTERS_SHADOW").c_str(), OustersShadowFile2))
		return false;
	m_OustersShadowFPK.LoadFromFile(OustersShadowFile2);
	OustersShadowFile2.close();

	ivfstream VampireShadowFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_CFRAME_VAMPIRE_SHADOW").c_str(), VampireShadowFile2))
		return false;
	m_VampireShadowFPK.LoadFromFile(VampireShadowFile2);
	VampireShadowFile2.close();

	DrawTitleLoading();
	//* ??????? ??? ???? ???
		//??? ?????

	/*/

		#define CFPK_TOTAL_SHAPE_NUM 5
		#define CFPK_TOTAL_ACTION_NUM 22
		#define CFPK_TOTAL_ADD_NUM 2

		// ????? ???
		DIRECTION_FRAME_ARRAY wingaction_stop;
		DIRECTION_FRAME_ARRAY wingaction_move;

		DIRECTION_FRAME_ARRAY tempOustersFPK[CFPK_TOTAL_SHAPE_NUM][CFPK_TOTAL_ACTION_NUM];

		for(int i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			wingaction_stop.operator=(m_OustersFPK[CFPK_TOTAL_SHAPE_NUM][0]);
			wingaction_move.operator=(m_OustersFPK[CFPK_TOTAL_SHAPE_NUM][1]);
			for(int k = 0; k < CFPK_TOTAL_ACTION_NUM-CFPK_TOTAL_ADD_NUM; ++k)
			{
				tempOustersFPK[i][k] = m_OustersFPK[i][k];
			}
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-2] = wingaction_stop;
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-1] = wingaction_move;
		}
		m_OustersFPK.Init(CFPK_TOTAL_SHAPE_NUM);
		for(i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			m_OustersFPK[i].Init(CFPK_TOTAL_ACTION_NUM);
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_OustersFPK[i][j].Init(8); // ????
			}
		}
		for(i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_OustersFPK[i][j] = tempOustersFPK[i][j];
			}
		}

		std::ofstream OustersCFPK(g_pFileDef->getProperty("FILE_CFRAME_OUSTERS").c_str(), std::ios::binary);
		std::string str = g_pFileDef->getProperty("FILE_CFRAME_OUSTERS");
		str += "i";
		std::ofstream indexOustersCFPK(str.c_str(), std::ios::binary);
		m_OustersFPK.SaveToFile(OustersCFPK, indexOustersCFPK);

		//??? ????? ?????

		for( i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			wingaction_stop.operator=(m_OustersShadowFPK[CFPK_TOTAL_SHAPE_NUM][0]);
			wingaction_move.operator=(m_OustersShadowFPK[CFPK_TOTAL_SHAPE_NUM][1]);
			for(int k = 0; k < CFPK_TOTAL_ACTION_NUM-CFPK_TOTAL_ADD_NUM; ++k)
			{
				tempOustersFPK[i][k] = m_OustersShadowFPK[i][k];
			}
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-2] = wingaction_stop;
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-1] = wingaction_move;
		}
		m_OustersShadowFPK.Init(CFPK_TOTAL_SHAPE_NUM);
		for(i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			m_OustersShadowFPK[i].Init(CFPK_TOTAL_ACTION_NUM);
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_OustersShadowFPK[i][j].Init(8); // ????
			}
		}
		for(i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_OustersShadowFPK[i][j] = tempOustersFPK[i][j];
			}
		}

		std::ofstream OustersCFPK1(g_pFileDef->getProperty("FILE_CFRAME_OUSTERS_SHADOW").c_str(), std::ios::binary);
		str = g_pFileDef->getProperty("FILE_CFRAME_OUSTERS_SHADOW");
		str += "i";
		std::ofstream indexOustersCFPK1(str.c_str(), std::ios::binary);
		m_OustersShadowFPK.SaveToFile(OustersCFPK1, indexOustersCFPK1);
	///
		//??? ????
		#define CFPK_TOTAL_SHAPE_NUM 2
		#define CFPK_TOTAL_ACTION_NUM 23
		#define CFPK_TOTAL_ADD_NUM 2

		DIRECTION_FRAME_ARRAY wingaction_stop;
		DIRECTION_FRAME_ARRAY wingaction_move;
		DIRECTION_FRAME_ARRAY tempOustersFPK[CFPK_TOTAL_SHAPE_NUM][CFPK_TOTAL_ACTION_NUM];

		for(int i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			wingaction_stop.operator=(m_AdvancementOustersFPK[CFPK_TOTAL_SHAPE_NUM][0]);
			wingaction_move.operator=(m_AdvancementOustersFPK[CFPK_TOTAL_SHAPE_NUM][1]);
			for(int k = 0; k < CFPK_TOTAL_ACTION_NUM-CFPK_TOTAL_ADD_NUM; ++k)
			{
				tempOustersFPK[i][k] = m_AdvancementOustersFPK[i][k];
			}
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-2] = wingaction_stop;
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-1] = wingaction_move;
		}
		m_AdvancementOustersFPK.Init(CFPK_TOTAL_SHAPE_NUM);
		for(i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			m_AdvancementOustersFPK[i].Init(CFPK_TOTAL_ACTION_NUM);
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_AdvancementOustersFPK[i][j].Init(8);
			}
		}
		for(i = 0; i < CFPK_TOTAL_ADD_NUM; ++i)
		{
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_AdvancementOustersFPK[i][j] = tempOustersFPK[i][j];
			}
		}

		std::ofstream OustersCFPK(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS").c_str(), std::ios::binary);
		std::string str = g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS");
		str += "i";
		std::ofstream indexOustersCFPK(str.c_str(), std::ios::binary);
		m_AdvancementOustersFPK.SaveToFile(OustersCFPK, indexOustersCFPK);

		//??? ???? ?????

		for( i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			wingaction_stop.operator=(m_AdvancementOustersShadowFPK[CFPK_TOTAL_SHAPE_NUM][0]);
			wingaction_move.operator=(m_AdvancementOustersShadowFPK[CFPK_TOTAL_SHAPE_NUM][1]);
			for(int k = 0; k < CFPK_TOTAL_ACTION_NUM-CFPK_TOTAL_ADD_NUM; ++k)
			{
				tempOustersFPK[i][k] = m_AdvancementOustersShadowFPK[i][k];
			}
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-2] = wingaction_stop;
			tempOustersFPK[i][CFPK_TOTAL_ACTION_NUM-1] = wingaction_move;
		}
		m_AdvancementOustersShadowFPK.Init(CFPK_TOTAL_SHAPE_NUM);
		for(i = 0; i < CFPK_TOTAL_SHAPE_NUM; ++i)
		{
			m_AdvancementOustersShadowFPK[i].Init(CFPK_TOTAL_ACTION_NUM);
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_AdvancementOustersShadowFPK[i][j].Init(8);
			}
		}
		for(i = 0; i < CFPK_TOTAL_ADD_NUM; ++i)
		{
			for(int j = 0; j < CFPK_TOTAL_ACTION_NUM; ++j)
			{
				m_AdvancementOustersShadowFPK[i][j] = tempOustersFPK[i][j];
			}
		}

		std::ofstream OustersCFPK1(g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS_SHADOW").c_str(), std::ios::binary);
		str = g_pFileDef->getProperty("FILE_CFRAME_ADVANCEMENT_CLASS_OUSTERS_SHADOW");
		str += "i";
		std::ofstream indexOustersCFPK1(str.c_str(), std::ios::binary);
		m_AdvancementOustersShadowFPK.SaveToFile(OustersCFPK1, indexOustersCFPK1);

	//*/
	// ?????? ?????? ?????? ???
	// ????? ???? ????!!
	/*
	int add;
	for (add=0; add<MAX_ADDONID_MALE; add++)
	{
		for (int i=0; i<8; i++)
		{
			if (add!=ADDONID_GUN_AR_MALE)
				m_AddonMaleFPK[add][ACTION_SLAYER_GUN_AR][i] = m_AddonMaleFPK[add][ACTION_SLAYER_GUN_TR][i];

			if (add!=ADDONID_GUN_SMG_MALE)
				m_AddonMaleFPK[add][ACTION_SLAYER_GUN_SMG][i] = m_AddonMaleFPK[add][ACTION_SLAYER_GUN_TR][i];
		}
	}

	for (add=0; add<MAX_ADDONID_FEMALE; add++)
	{
		for (int i=0; i<8; i++)
		{
			if (add!=ADDONID_GUN_AR_FEMALE)
				m_AddonFemaleFPK[add][ACTION_SLAYER_GUN_AR][i] = m_AddonMaleFPK[add][ACTION_SLAYER_GUN_TR][i];

			if (add!=ADDONID_GUN_SMG_FEMALE)
				m_AddonFemaleFPK[add][ACTION_SLAYER_GUN_SMG][i] = m_AddonMaleFPK[add][ACTION_SLAYER_GUN_TR][i];
		}
	}
	*/

	// ????..
	/*
	int add, a, d, f;
	for (add=0; add<MAX_ADDONID_MALE; add++)
	{
		ACTION_FRAME_ARRAY& AFA = m_AddonMaleFPK[add];

		// motorcycle???? action???
		if (add==ADDONID_MOTORCYCLE_MALE)
		{
			for (a=0; a<ACTION_MAX_SLAYER; a++)
			{
				if (a==1)
				{
					DIRECTION_FRAME_ARRAY& DFA = AFA[a];

					for (d=0; d<8; d++)
					{
						FRAME_ARRAY& FA = DFA[d];
						FRAME_ARRAY newFA;
						newFA.Init( 18 );

						newFA[0] = FA[0];
						newFA[1] = FA[1];
						newFA[2] = FA[2];
						newFA[3] = FA[1];
						for (f=4; f<18; f++)
						{
							newFA[f] = FA[f%4];
						}

						FA = newFA;
					}
				}
				else
				{
					DIRECTION_FRAME_ARRAY& DFA = AFA[a];

					for (d=0; d<8; d++)
					{
						FRAME_ARRAY& FA = DFA[d];
						FA.Init( 18 );

						FA[0] = AFA[1][d][0];
						FA[1] = AFA[1][d][1];
						FA[2] = AFA[1][d][2];
						FA[3] = AFA[1][d][1];
						for (f=4; f<18; f++)
						{
							FA[f] = FA[f%4];
						}
					}
				}
			}
		}
		else
		{
			for (a=0; a<AFA.GetSize(); a++)
			{
				DIRECTION_FRAME_ARRAY& DFA = AFA[a];

				for (d=0; d<DFA.GetSize(); d++)
				{
					FRAME_ARRAY& FA = DFA[d];

					// motor?????? 3 --> 4 frame???? ????.
					if (a==ACTION_SLAYER_MOTOR_MOVE)
					{
						if (FA.GetSize()!=0)
						{
							FRAME_ARRAY newFA;
							newFA.Init( 4 );
							// 3??¥?? FA?? 4???? ?????? ???.
							for (f=0; f<FA.GetSize(); f++)
							{
								CFrame& frame = FA[f];

								newFA[f] = FA[f];
							}

							newFA[3] = FA[2];

							FA = newFA;
						}
					}

				}
			}
		}
	}
	*/

	//m_AddonMaleFPK.InfoToFile("log\\addonMale.txt");
	//m_AddonFemaleFPK.InfoToFile("log\\addonFemale.txt");
	//m_AddonMaleShadowFPK.InfoToFile("log\\addonMaleShadow.txt");
	//m_AddonFemaleShadowFPK.InfoToFile("log\\addonFemaleShadow.txt");

	/*
	// [????] ??????? 6 frame???? ????...
	for (int add=0; add<m_AddonMaleFPK.GetSize(); add++)
	{
		ACTION_FRAME_ARRAY &addon = m_AddonMaleFPK[add];

		DIRECTION_FRAME_ARRAY &SWORD = addon[ACTION_SLAYER_SWORD];
		DIRECTION_FRAME_ARRAY &BLADE = addon[ACTION_SLAYER_BLADE];

		addon[ACTION_SLAYER_SWORD_2] = SWORD;
		addon[ACTION_SLAYER_BLADE_2] = BLADE;

		for (int d=0; d<8; d++)
		{
			FRAME_ARRAY &fr1 = SWORD[d];
			FRAME_ARRAY &fr2 = BLADE[d];

			if (fr1.GetSize()!=0)
			{
				FRAME_ARRAY temp;
				temp.Init( 6 );

				temp[0] = fr1[0];
				temp[1] = fr1[1];
				temp[2] = fr1[2];
				temp[3] = fr1[3];
				temp[4] = fr1[4];
				temp[5] = fr1[fr1.GetSize()-1];

				fr1 = temp;
			}

			if (fr2.GetSize()!=0)
			{
				FRAME_ARRAY temp;
				temp.Init( 6 );

				temp[0] = fr2[0];
				temp[1] = fr2[1];
				temp[2] = fr2[2];
				temp[3] = fr2[3];
				temp[4] = fr2[4];
				temp[5] = fr2[fr2.GetSize()-1];

				fr2 = temp;
			}
		}
	}

	// [????] ??????? 6 frame???? ????...
	for (add=0; add<m_AddonFemaleFPK.GetSize(); add++)
	{
		ACTION_FRAME_ARRAY &addon = m_AddonFemaleFPK[add];

		DIRECTION_FRAME_ARRAY &SWORD = addon[ACTION_SLAYER_SWORD];
		DIRECTION_FRAME_ARRAY &BLADE = addon[ACTION_SLAYER_BLADE];

		addon[ACTION_SLAYER_SWORD_2] = SWORD;
		addon[ACTION_SLAYER_BLADE_2] = BLADE;

		for (int d=0; d<8; d++)
		{
			FRAME_ARRAY &fr1 = SWORD[d];
			FRAME_ARRAY &fr2 = BLADE[d];

			if (fr1.GetSize()!=0)
			{
				FRAME_ARRAY temp;
				temp.Init( 6 );

				temp[0] = fr1[0];
				temp[1] = fr1[1];
				temp[2] = fr1[2];
				temp[3] = fr1[3];
				temp[4] = fr1[4];
				temp[5] = fr1[fr1.GetSize()-1];

				fr1 = temp;
			}

			if (fr2.GetSize()!=0)
			{
				FRAME_ARRAY temp;
				temp.Init( 6 );

				temp[0] = fr2[0];
				temp[1] = fr2[1];
				temp[2] = fr2[2];
				temp[3] = fr2[3];
				temp[4] = fr2[4];
				temp[5] = fr2[fr2.GetSize()-1];

				fr2 = temp;
			}
		}
	}

	std::ofstream packFileMale(FILE_CFRAME_ADDON_MALE, ios::binary);
	std::ofstream indexFileMale(FILE_CFRAMEINDEX_ADDON_MALE, ios::binary);
	m_AddonMaleFPK.SaveToFile(packFileMale, indexFileMale);
	packFileMale.close();
	indexFileMale.close();

	std::ofstream packFileFemale(FILE_CFRAME_ADDON_FEMALE, ios::binary);
	std::ofstream indexFileFemale(FILE_CFRAMEINDEX_ADDON_FEMALE, ios::binary);
	m_AddonFemaleFPK.SaveToFile(packFileFemale, indexFileFemale);
	packFileFemale.close();
	indexFileFemale.close();

	*/


	// ???? ?? ???? ????
	/*
	m_AddonFemaleFPK[ADDONID_GUN_TR_FEMALE][14] = m_AddonFemaleFPK[ADDONID_GUN_TR_FEMALE][7];
	m_AddonFemaleFPK[ADDONID_GUN_TR_FEMALE][7].Release();

	m_AddonFemaleFPK[ADDONID_GUN_SG_FEMALE][13] = m_AddonFemaleFPK[ADDONID_GUN_SG_FEMALE][8];
	m_AddonFemaleFPK[ADDONID_GUN_SG_FEMALE][8].Release();

	m_AddonFemaleFPK[ADDONID_GUN_AR_FEMALE][8] = m_AddonFemaleFPK[ADDONID_GUN_AR_FEMALE][13];
	m_AddonFemaleFPK[ADDONID_GUN_AR_FEMALE][13].Release();

	m_AddonFemaleFPK[ADDONID_GUN_SMG_FEMALE][7] = m_AddonFemaleFPK[ADDONID_GUN_SMG_FEMALE][14];
	m_AddonFemaleFPK[ADDONID_GUN_SMG_FEMALE][14].Release();
	*/

	//*/

	//m_AddonMaleFPK.InfoToFile("log\\addonMale.txt");
	//m_AddonFemaleFPK.InfoToFile("log\\addonFemale.txt");


	return true;
}

//----------------------------------------------------------------------
// Init Image Frames
//----------------------------------------------------------------------
// CImageFramePack			m_ItemTileFPK;			// Tile?? ??? Item?? ????..
// m_ImageFrame[????]
//----------------------------------------------------------------------
bool
MTopView::InitImageFrames()
{
	//------------------------------------------------------------
	//
	//  Item on Tile ?????? ???????.
	//
	//------------------------------------------------------------
	/*
	m_ItemTileFPK.Init( 28 );

	// Item 0
	for (int i=0; i<28; i++)
		m_ItemTileFPK[i].Set(i, 10, 10);

	std::ofstream packFile(FILE_IFRAME_ITEMTILE, ios::binary);
	std::ofstream indexFile(FILE_IFRAMEINDEX_ITEMTILE, ios::binary);

	m_ItemTileFPK.SaveToFile(packFile, indexFile);

	packFile.close();
	indexFile.close();
	*/

	///*
	// Load from File
	ivfstream file2;//(FILE_IFRAME_ITEMTILE, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_IFRAME_ITEMTILE").c_str(), file2))
		return false;
	m_ItemTileFPK.LoadFromFile(file2);
	file2.close();
	//*/



	return true;
}

//----------------------------------------------------------------------
// Init Animation Frames
//----------------------------------------------------------------------
// m_ImageObjectFPK[????][Frame]
//----------------------------------------------------------------------
bool
MTopView::InitAnimationFrames()
{
	//------------------------------------------------------------
	//
	//  Animation Frame ?????? ???????.
	//
	//------------------------------------------------------------

	//------------------------------------------------------------
	//
	//  ?? ?????? FrameSet???.
	//
	//  SpriteSet?????? SpriteID?? ??????? ???? ???.
	//
	//------------------------------------------------------------

	ivfstream file2;//(FILE_AFRAME_ANIMATIONOBJECT, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_AFRAME_ANIMATIONOBJECT").c_str(), file2))
		return false;
	m_ImageObjectFPK.LoadFromFile(file2);
	file2.close();

	ivfstream file3;//(FILE_AFRAME_ANIMATIONOBJECT, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_AFRAME_ANIMATIONOBJECT_SHADOW").c_str(), file3))
		return false;
	m_ImageObjectShadowFPK.LoadFromFile(file3);
	file3.close();

	//#ifdef OUTPUT_DEBUG
	//	m_InteractionObjectFPK.Init( 1 );
	//	
	//	//------------------------------------------
	//	// ù??° Animation Object
	//	//------------------------------------------
	//	m_InteractionObjectFPK[0].Init(10);	
	//	m_InteractionObjectFPK[0][0].Set(0, 0, -48);
	//	m_InteractionObjectFPK[0][1].Set(1, 0, -48);
	//	m_InteractionObjectFPK[0][2].Set(2, 0, -48);
	//	m_InteractionObjectFPK[0][3].Set(3, 0, -48);
	//	m_InteractionObjectFPK[0][4].Set(4, 0, -48);
	//	m_InteractionObjectFPK[0][5].Set(4, 0, -48);
	//	m_InteractionObjectFPK[0][6].Set(3, 0, -48);
	//	m_InteractionObjectFPK[0][7].Set(2, 0, -48);
	//	m_InteractionObjectFPK[0][8].Set(1, 0, -48);
	//	m_InteractionObjectFPK[0][9].Set(0, 0, -48);
	//	
	//
	//	std::ofstream iopackFile(g_pFileDef->getProperty("FILE_AFRAME_INTERACTIONOBJECT").c_str(), ios::binary);
	//	std::ofstream ioindexFile(g_pFileDef->getProperty("FILE_AFRAMEINDEX_INTERACTIONOBJECT").c_str(), ios::binary);
	//	m_InteractionObjectFPK.SaveToFile(iopackFile, ioindexFile);
	//	iopackFile.close();
	//	ioindexFile.close();	
	//#endif

		///*
		// Load from File
	//	std::ifstream iofile2;//(FILE_AFRAME_ANIMATIONOBJECT, ios::binary);
	//	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_AFRAME_INTERACTIONOBJECT").c_str(), iofile2))
	//		return false;
	//	m_InteractionObjectFPK.LoadFromFile(iofile2);
	//	iofile2.close();
		//*/

		//------------------------------------------------------------
		//
		// Item Drop
		//
		//------------------------------------------------------------	
	ivfstream itemdropfile2;//(FILE_AFRAME_ANIMATIONOBJECT, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_AFRAME_ITEMDROP").c_str(), itemdropfile2))
		return false;
	m_ItemDropFPK.LoadFromFile(itemdropfile2);
	//std::ofstream itemdropfile3(g_pFileDef->getProperty("FILE_AFRAME_ITEMDROP").c_str(), ios::binary);
	itemdropfile2.close();

	//	for(int kkk = 803; kkk <= 804; kkk++)
	//	{
	//		for(int bbb = 0; bbb < 6; bbb++)
	//		{
	//			CFrame &Frame = m_ItemDropFPK[ kkk ][ bbb ];
	//			int spriteID = Frame.GetSpriteID()+(kkk-802)*4;
	//			Frame.Set(spriteID, Frame.GetCX(), Frame.GetCY());
	//		}
	//
	//	}
	//	std::ofstream itemdropfile3(g_pFileDef->getProperty("FILE_AFRAME_ITEMDROP").c_str(), ios::binary);
	//	std::ofstream itemdropfile4(g_pFileDef->getProperty("FILE_AFRAMEINDEX_ITEMDROP").c_str(), ios::binary);
	//	m_ItemDropFPK.SaveToFile(itemdropfile3, itemdropfile4);
	//	itemdropfile4.close();
	//	itemdropfile3.close();

		//------------------------------------------------------------	
		// ??? ????
		//------------------------------------------------------------	
		/*
		const int numItems	= 121;
		const int numFrames	= 6;

		// frame ??? ??
		// sprite?? item???? 4??????? 6 frame?? ?????? ???.
		int nFrame[numFrames] = { 3, 0, 1, 2, 3, 0 };

		m_ItemDropFPK.Init( numItems );	// ??ü item ????????? ani frame

		int spriteID = 0;
		for (int i=0; i<numItems; i++)
		{
			m_ItemDropFPK[i].Init( numFrames );		// ???? 6 frame??

			for (int f=0; f<numFrames; f++)
			{
				int sid = spriteID+nFrame[f];

				// ???? ?????? ??? ????..
				int cx = TILE_X_HALF - (m_ItemDropSPK[sid].GetWidth()>>1);
				int cy = TILE_Y_HALF - (m_ItemDropSPK[sid].GetHeight()>>1);

				m_ItemDropFPK[i][f].Set( sid, cx, cy );
			}

			spriteID += 4;
		}

		//------------------------------------------------------------
		// ????
		//------------------------------------------------------------
		std::ofstream packFile(FILE_AFRAME_ITEMDROP, ios::binary);
		std::ofstream indexFile(FILE_AFRAMEINDEX_ITEMDROP, ios::binary);
		m_ItemDropFPK.SaveToFile(packFile, indexFile);
		packFile.close();
		indexFile.close();
		*/

		//------------------------------------------------------------
	//
	//  Item Broken ?????? ???????.
	//
	//------------------------------------------------------------
	///*
#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
	//chyaya 2006.09.04 Item???? ???
	const int maxSlayerItemBroken = (MSlayerGear::MAX_GEAR_SLAYER + 2 - 17) * 3;
	const int maxVampireItemBroken = (MVampireGear::MAX_GEAR_VAMPIRE - 17) * 3;
	const int maxOustersItemBroken = (MOustersGear::MAX_GEAR_OUSTERS + 1 - 17) * 3;
	/*
	const int maxSlayerItemBroken = (MSlayerGear::MAX_GEAR_SLAYER+2-13) * 3;
	const int maxVampireItemBroken = (MVampireGear::MAX_GEAR_VAMPIRE-13) * 3;
	const int maxOustersItemBroken = (MOustersGear::MAX_GEAR_OUSTERS+1-13) * 3;
	*/
	/*
	const int maxSlayerItemBroken = (MSlayerGear::MAX_GEAR_SLAYER+2-12) * 3;
	const int maxVampireItemBroken = (MVampireGear::MAX_GEAR_VAMPIRE+2-12) * 3;
	const int maxOustersItemBroken = (MOustersGear::MAX_GEAR_OUSTERS+1-12) * 3;
	*/

	m_ItemBrokenFPK.Init(5);	// ??????? ??,?? + ??????? ??,??


	// { SpriteID, CX, CY }
	//------------------------------------------------------------
	// ??????? ????
	//------------------------------------------------------------
	int slayerMale[maxSlayerItemBroken][3] =
	{
		// ?????????
		{ SPRITEID_NULL,	33, 1 },	// ????,
		{ SPRITEID_NULL,	 38, 18 },	// ?????
		{ SPRITEID_NULL,	16, 19 },	// ????
		{ 1,	62, 19 },	// ????	
		{ 2,	19, 11 },	// ?		
		{ SPRITEID_NULL,	9, 49 },	// ??
		{ SPRITEID_NULL,	27, 54 },	// ???
		{ SPRITEID_NULL,	26, 59 },	// ????
		{ 5,	59, 51 },	// ????2		
		{ 5,	24, 51 },	// ????1
		{ 6,	20, 60 },	// ????1
		{ 6,	28, 60 },	// ????2
		{ 6,	55, 60 },	// ????3
		{ 6,	63, 60 },	// ????4
		{ SPRITEID_NULL,	20, 122 },	// ???
		{ 3,	18, 12 },	// ?????
		{ 4,	20, 12 },	// ??

		// ?? ???????
		{ 9,	40, 10 },	// ????,
		{ 10,	42, 19 },	// ?????
		{ 7,	33, 18 },	// ????
		{ 14,	62, 19 },	// ????	
		{ 15,	19, 11 },	// ?		
		{ 11,	30, 35 },	// ??
		{ 12,	37, 36 },	// ???
		{ 8,	36, 35 },	// ????
		{ 18,	59, 51 },	// ????2		
		{ 18,	24, 51 },	// ????1
		{ 19,	20, 60 },	// ????1
		{ 19,	28, 60 },	// ????2
		{ 19,	55, 60 },	// ????3
		{ 19,	63, 60 },	// ????4
		{ 13,	36, 72 },	// ???
		{ 16,	18, 12 },	// ?????
		{ 17,	20, 12 },	// ??

		// ?? ????? ???°?
		{ 22,	40, 10 },	// ????,
		{ 23,	42, 19 },	// ?????
		{ 20,	33, 18 },	// ????
		{ 27,	62, 19 },	// ????	
		{ 28,	19, 11 },	// ?		
		{ 24,	30, 35 },	// ??
		{ 25,	37, 36 },	// ???
		{ 21,	36, 35 },	// ????
		{ 31,	59, 51 },	// ????2		
		{ 31,	24, 51 },	// ????1
		{ 32,	20, 60 },	// ????1
		{ 32,	28, 60 },	// ????2
		{ 32,	55, 60 },	// ????3
		{ 32,	63, 60 },	// ????4
		{ 26,	36, 72 },	// ???
		{ 29,	18, 12 },	// ?????
		{ 30,	20, 12 },	// ??
	};

	//------------------------------------------------------------
	// ??????? ????
	//------------------------------------------------------------
	int slayerFemale[maxSlayerItemBroken][3] =
	{
		// ?????????
		{ SPRITEID_NULL,	33, 1 },	// ????,
		{ SPRITEID_NULL,	 38, 18 },	// ?????
		{ SPRITEID_NULL,	16, 19 },	// ????
		{ 1,	62, 19 },	// ????	
		{ 2,	19, 11 },	// ?		
		{ SPRITEID_NULL,	9, 49 },	// ??
		{ SPRITEID_NULL,	27, 54 },	// ???
		{ SPRITEID_NULL,	26, 59 },	// ????
		{ 5,	59, 51 },	// ????2		
		{ 5,	24, 51 },	// ????1
		{ 6,	20, 60 },	// ????1
		{ 6,	28, 60 },	// ????2
		{ 6,	55, 60 },	// ????3
		{ 6,	63, 60 },	// ????4
		{ SPRITEID_NULL,	20, 122 },	// ???
		{ 3,	18, 12 },	// ?????
		{ 4,	20, 12 },	// ??

		// ?? ???????
		{ 36,	40, 10 },	// ????,
		{ 37,	41, 20 },	// ?????
		{ 34,	33, 20 },	// ????
		{ 14,	62, 19 },	// ????	
		{ 15,	19, 11 },	// ?		
		{ 38,	30, 37 },	// ??
		{ 39,	40, 32 },	// ???
		{ 35,	38, 35 },	// ????
		{ 18,	59, 51 },	// ????2		
		{ 18,	24, 51 },	// ????1
		{ 19,	20, 60 },	// ????1
		{ 19,	28, 60 },	// ????2
		{ 19,	55, 60 },	// ????3
		{ 19,	63, 60 },	// ????4
		{ 40,	39, 55 },	// ???
		{ 16,	18, 12 },	// ?????
		{ 17,	20, 12 },	// ??

		// ?? ????? ???°?
		{ 43,	40, 10 },	// ????,
		{ 44,	41, 20 },	// ?????
		{ 41,	33, 20 },	// ????
		{ 14,	62, 19 },	// ????	
		{ 15,	19, 11 },	// ?		
		{ 45,	30, 37 },	// ??
		{ 46,	40, 32 },	// ???
		{ 42,	38, 35 },	// ????
		{ 18,	59, 51 },	// ????2		
		{ 18,	24, 51 },	// ????1
		{ 19,	20, 60 },	// ????1
		{ 19,	28, 60 },	// ????2
		{ 19,	55, 60 },	// ????3
		{ 19,	63, 60 },	// ????4
		{ 47,	39, 55 },	// ???
		{ 16,	18, 12 },	// ?????
		{ 17,	20, 12 },	// ??
	};

	//------------------------------------------------------------
	// ??????? ????
	//------------------------------------------------------------
	int vampireMale[maxVampireItemBroken][3] =
	{
		// ?????????
		{ SPRITEID_NULL,	40, 22 },	// ?????
		{ SPRITEID_NULL,	19, 10 },	// ????
		{ 49,	22, 45 },	// ????1
		{ 49,	62, 45 },	// ????2
		{ 50,	18, 54 },	// ????1
		{ 50,	26, 54 },	// ????2
		{ 50,	58, 54 },	// ????3
		{ 50,	66, 54 },	// ????4
		{ 62,	22, 18 },	// ?????1
		{ 62,	62, 18 },	// ?????2
		{ 65,	16, 22 },	// Weapon1
		{ 65,	64, 22 },	// Weapon2
		{ 68,	18, 63 },	// Amulet1
		{ 68,	26, 63 },	// Amulet2
		{ 68,	58, 63 },	// Amulet3
		{ 68,	66, 63 },	// Amulet4

		// ?? ???????
		{ 52,	43, 18 },	// ?????
		{ 51,	30, 14 },	// ????
		{ 53,	22, 45 },	// ????1
		{ 53,	62, 45 },	// ????2
		{ 54,	18, 54 },	// ????1
		{ 54,	26, 54 },	// ????2
		{ 54,	58, 54 },	// ????3
		{ 54,	66, 54 },	// ????4
		{ 63,	22, 18 },	// ?????1
		{ 63,	62, 18 },	// ?????2
		{ 66,	16, 22 },	// Weapon1
		{ 66,	64, 22 },	// Weapon2
		{ 69,	18, 63 },	// Amulet1
		{ 69,	26, 63 },	// Amulet2
		{ 69,	58, 63 },	// Amulet3
		{ 69,	66, 63 },	// Amulet4

		// ?? ???????
		{ 56,	43, 18 },	// ?????
		{ 55,	30, 14 },	// ????
		{ 57,	22, 45 },	// ????1
		{ 57,	62, 45 },	// ????2
		{ 58,	18, 54 },	// ????1
		{ 58,	26, 54 },	// ????2
		{ 58,	58, 54 },	// ????3
		{ 58,	66, 54 },	// ????4
		{ 64,	22, 18 },	// ?????1
		{ 64,	62, 18 },	// ?????2
		{ 67,	16, 22 },	// Weapon1
		{ 67,	64, 22 },	// Weapon2
		{ 70,	18, 63 },	// Amulet1
		{ 70,	26, 63 },	// Amulet2
		{ 70,	58, 63 },	// Amulet3
		{ 70,	66, 63 },	// Amulet4
	};

	//------------------------------------------------------------
	// ??????? ????
	//------------------------------------------------------------
	int vampireFemale[maxVampireItemBroken][3] =
	{
		// ?????????
		{ SPRITEID_NULL,	40, 22 },	// ?????
		{ SPRITEID_NULL,	19, 10 },	// ????
		{ 49,	22, 45 },	// ????1
		{ 49,	62, 45 },	// ????2
		{ 50,	18, 54 },	// ????1
		{ 50,	58, 54 },	// ????3
		{ 50,	26, 54 },	// ????2		
		{ 50,	66, 54 },	// ????4
		{ 62,	22, 18 },	// ?????1
		{ 62,	62, 18 },	// ?????2
		{ 65,	16, 22 },	// Weapon1
		{ 65,	64, 22 },	// Weapon2
		{ 68,	18, 63 },	// Amulet1
		{ 68,	26, 63 },	// Amulet2
		{ 68,	58, 63 },	// Amulet3
		{ 68,	66, 63 },	// Amulet4

		// ?? ???????
		{ 52,	42, 18 },	// ?????
		{ 60,	31, 19 },	// ????
		{ 53,	22, 45 },	// ????1
		{ 53,	62, 45 },	// ????2
		{ 54,	18, 54 },	// ????1
		{ 54,	58, 54 },	// ????3
		{ 54,	26, 54 },	// ????2		
		{ 54,	66, 54 },	// ????4
		{ 63,	22, 18 },	// ?????1
		{ 63,	62, 18 },	// ?????2
		{ 66,	16, 22 },	// Weapon1
		{ 66,	64, 22 },	// Weapon2
		{ 69,	18, 63 },	// Amulet1
		{ 69,	26, 63 },	// Amulet2
		{ 69,	58, 63 },	// Amulet3
		{ 69,	66, 63 },	// Amulet4

		// ?? ???????
		{ 56,	42, 18 },	// ?????
		{ 61,	31, 19 },	// ????
		{ 57,	22, 45 },	// ????1
		{ 57,	62, 45 },	// ????2
		{ 58,	18, 54 },	// ????1
		{ 58,	58, 54 },	// ????3
		{ 58,	26, 54 },	// ????2		
		{ 58,	66, 54 },	// ????4
		{ 64,	22, 18 },	// ?????1
		{ 64,	62, 18 },	// ?????2
		{ 67,	16, 22 },	// Weapon1
		{ 67,	64, 22 },	// Weapon2
		{ 70,	18, 63 },	// Amulet1
		{ 70,	26, 63 },	// Amulet2
		{ 70,	58, 63 },	// Amulet3
		{ 70,	66, 63 },	// Amulet4
	};

	//------------------------------------------------------------
	// ???????
	//------------------------------------------------------------
	int Ousters[maxOustersItemBroken][3] =
	{
		// ?????????
		{ 77,	39, 8 },				// ?????
		{ SPRITEID_NULL ,	34, 18 },				// ??
		{ 89,	17, 33 },				// ???				// ????????? 92
		{ 89,	61, 33 },				// ??????
		{ SPRITEID_NULL,	40, 69 },				// ???
		{ 74,	19, 18},				// ??????1
		{ 74,	57, 18},				// ??????2
		{ 83,	19, 63 },				// ??1
		{ 83,	65, 63 },				// ??2
		{ 86,	35, 22 },				// ?????1
		{ 86,	42, 22 },				// ?????2
		{ 86,	49, 22 },				// ?????3
		{ 80,	26, 63 },				// ?????1		
		{ 80,	58, 63 },				// ?????2
		{ 80,	26, 71 },				// ?????3
		{ 80,	58, 71 },				// ????? 4 
		{ 92,	61, 33 },				// ??????? 

		// ???
		{ 78,	40, 22 },				// ?????
		{ 72,	19, 10 },	// ??
		{ 90,	22, 45 },				// ???				// ????????? 92
		{ 90,	62, 45 },				// ??????
		{ 95,	18, 54 },				// ???
		{ 75,	26, 54 },				// ??????1
		{ 75,	58, 54 },				// ??????2
		{ 84,	66, 54 },				// ??1
		{ 84,	22, 18 },				// ??2
		{ 87,	62, 18 },				// ?????1
		{ 87,	16, 22 },				// ?????2
		{ 87,	16, 22 },				// ?????3

		{ 81,	26, 63 },				// ?????1		
		{ 81,	58, 63 },				// ?????2
		{ 81,	26, 71 },				// ?????3
		{ 81,	58, 71 },				// ????? 4 

		{ 93,	61, 33 },				// ???????  

		// ?????
		{ 79,	40, 22 },				// ?????
		{ 73,	19, 10 },	// ??
		{ 91,	22, 45 },				// ???				// ????????? 92
		{ 91,	62, 45 },				// ??????
		{ 96,	18, 54 },				// ???
		{ 76,	26, 54 },				// ??????1
		{ 76,	58, 54 },				// ??????2
		{ 85,	66, 54 },				// ??1
		{ 85,	22, 18 },				// ??2
		{ 88,	62, 18 },				// ?????1
		{ 88,	16, 22 },				// ?????2
		{ 88,	16, 22 },				// ?????3
		{ 82,	26, 63 },				// ?????1		
		{ 82,	58, 63 },				// ?????2
		{ 82,	26, 71 },				// ?????3
		{ 82,	58, 71 },				// ????? 4 
		{ 94,	61, 33 },				// ???????
	};


	int j;

	m_ItemBrokenFPK[0].Init(maxSlayerItemBroken);
	m_ItemBrokenFPK[1].Init(maxSlayerItemBroken);
	m_ItemBrokenFPK[2].Init(maxVampireItemBroken);
	m_ItemBrokenFPK[3].Init(maxVampireItemBroken);
	m_ItemBrokenFPK[4].Init(maxOustersItemBroken);

	for (j = 0; j < maxSlayerItemBroken; j++)
	{
		m_ItemBrokenFPK[0][j].Set(slayerMale[j][0], slayerMale[j][1] - 30, slayerMale[j][2] - 10);
	}

	for (j = 0; j < maxSlayerItemBroken; j++)
	{
		m_ItemBrokenFPK[1][j].Set(slayerFemale[j][0], slayerFemale[j][1] - 30, slayerFemale[j][2] - 10);
	}

	for (j = 0; j < maxVampireItemBroken; j++)
	{
		m_ItemBrokenFPK[2][j].Set(vampireMale[j][0], vampireMale[j][1] - 30, vampireMale[j][2] - 10);
	}

	for (j = 0; j < maxVampireItemBroken; j++)
	{
		m_ItemBrokenFPK[3][j].Set(vampireFemale[j][0], vampireFemale[j][1] - 30, vampireFemale[j][2] - 10);
	}

	for (j = 0; j < maxOustersItemBroken; j++)
	{
		m_ItemBrokenFPK[4][j].Set(Ousters[j][0], Ousters[j % (maxOustersItemBroken / 3)][1] - 30, Ousters[j % (maxOustersItemBroken / 3)][2] - 10);
	}

	std::ofstream packFile(g_pFileDef->getProperty("FILE_AFRAME_ITEMBROKEN").c_str(), std::ios::binary);
	std::ofstream indexFile(g_pFileDef->getProperty("FILE_AFRAMEINDEX_ITEMBROKEN").c_str(), std::ios::binary);

	m_ItemBrokenFPK.SaveToFile(packFile, indexFile);

	packFile.close();
	indexFile.close();
#endif
	//*/

	///*
	// Load from File
	ivfstream fileItemBroken2;//(FILE_IFRAME_ITEMTILE, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_AFRAME_ITEMBROKEN").c_str(), fileItemBroken2))
		return false;
	m_ItemBrokenFPK.LoadFromFile(fileItemBroken2);
	fileItemBroken2.close();
	//*/


	return true;
}

//----------------------------------------------------------------------
// Init Effect Frames
//----------------------------------------------------------------------
// m_EffectAlphaFPK[????][Direction][Frame]
//----------------------------------------------------------------------
bool
MTopView::InitEffectFrames()
{
	//------------------------------------------------
	//
	//   Effect NormalSPK
	//
	//------------------------------------------------	
	// 0?? : HolyWater ??????? effect
	// 1?? : Bomb ??????? effect
	/*
	m_EffectNormalFPK.Init( 2 );

	for (int e=0; e<2; e++)
	{
		m_EffectNormalFPK[e].Init( 8 );

		for (int d=0; d<8; d++)
		{
			m_EffectNormalFPK[e][d].Init( 4 );	// ?? 4 frame

			for (int f=0; f<4; f++)
			{
				int spriteID = e*4 + f;
				int cx = -(m_EffectNormalSPK[spriteID].GetWidth()>>1);
				int cy = -(m_EffectNormalSPK[spriteID].GetHeight()>>1);

				m_EffectNormalFPK[e][d][f].Set(spriteID, cx, cy, 0);
			}
		}
	}

	std::ofstream packNormalFile(FILE_EFRAME_NORMALEFFECT, ios::binary);
	std::ofstream indexNormalFile(FILE_EFRAMEINDEX_NORMALEFFECT, ios::binary);
	m_EffectNormalFPK.SaveToFile(packNormalFile, indexNormalFile);
	packNormalFile.close();
	indexNormalFile.close();
	*/
	/*
	m_EffectNormalFPK.Init(MAX_NORMALEFFECT);

	//-------------------
	// ù??° Effect
	//-------------------
	// 8?????? ???.
	m_EffectNormalFPK[0].Init(8);		// Bomb

	for (int d=0; d<8; d++)
	{
		// Bomb
		m_EffectNormalFPK[0][d].Init(5);
		m_EffectNormalFPK[0][d][0].Set(0, 0, -48);
		m_EffectNormalFPK[0][d][1].Set(1, 0, -48);
		m_EffectNormalFPK[0][d][2].Set(2, 0, -48);
		m_EffectNormalFPK[0][d][3].Set(1, 0, -48);
		m_EffectNormalFPK[0][d][4].Set(0, 0, -48);
	}

	std::ofstream packNormalFile(FILE_EFRAME_NORMALEFFECT, ios::binary);
	std::ofstream indexNormalFile(FILE_EFRAMEINDEX_NORMALEFFECT, ios::binary);
	m_EffectNormalFPK.SaveToFile(packNormalFile, indexNormalFile);
	packNormalFile.close();
	indexNormalFile.close();
	*/

	//--------------------------------------------------
	// ????? FRR?? ???.. 
	// EFRR?? ????????..
	// EFPK?? ???????.
	//--------------------------------------------------
	// [ TEST CODE ]
	/*
	std::ifstream fileFA;//(FILE_EFRAME_NORMALEFFECT, ios::binary);
	if (!FileOpenBinary("Data\\bomb.frr", fileFA))
		return false;

	FRAME_ARRAY FA;
	FA.LoadFromFile( fileFA );
	fileFA.close();

	m_EffectNormalFPK.Init( 1 );
	// FRR --> EFRR
	m_EffectNormalFPK[0].Init( 8 );
	for (int d=0; d<8; d++)
	{
		m_EffectNormalFPK[0][d].Init( FA.GetSize() );
		for (int f=0; f<FA.GetSize(); f++)
		{
			CFrame& frame = FA[f];
			CEffectFrame& eframe = m_EffectNormalFPK[0][d][f];

			eframe.Set( frame.GetSpriteID(), frame.GetCX(), frame.GetCY(), 0);
		}
	}

	std::ofstream packNormalFile(FILE_EFRAME_NORMALEFFECT, ios::binary);
	std::ofstream indexNormalFile(FILE_EFRAMEINDEX_NORMALEFFECT, ios::binary);
	m_EffectNormalFPK.SaveToFile(packNormalFile, indexNormalFile);
	packNormalFile.close();
	indexNormalFile.close();
	*/

	///*
	// Load from NormalFile
	ivfstream NormalFile2;//(FILE_EFRAME_NORMALEFFECT, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_EFRAME_NORMALEFFECT").c_str(), NormalFile2))
		return false;
	m_EffectNormalFPK.LoadFromFile(NormalFile2);
	NormalFile2.close();
	//*/

	// ???????? ??????
	/*
	std::ofstream infoFile("Log\\EffectNormal.txt");

	for (int type=0; type<m_EffectNormalFPK.GetSize(); type++)
	{
		for (int dir=0; dir<m_EffectNormalFPK[type].GetSize(); dir++)
		{
			for (int frame=0; frame<m_EffectNormalFPK[type][dir].GetSize(); frame++)
			{
				int spriteID = m_EffectNormalFPK[type][dir][frame].GetSpriteID();
				int cx = m_EffectNormalFPK[type][dir][frame].GetCX();
				int cy = m_EffectNormalFPK[type][dir][frame].GetCY();
				infoFile << "[" << type << "][" << dir << "][" << frame << "] = ( " << spriteID << ",  " << cx << ", " << cy << " ) " << endl;
			}
		}
	}
	infoFile.close();
	*/

	//------------------------------------------------
	//
	//   Effect AlphaSPK
	//
	//------------------------------------------------
	/*
	m_EffectAlphaFPK.Init(MAX_ALPHAEFFECT);

	//-------------------
	// ù??° Effect
	//-------------------
	// 8?????? ???.
	m_EffectAlphaFPK[0].Init(8);		// Fire
	m_EffectAlphaFPK[1].Init(8);		// Lightning
	m_EffectAlphaFPK[2].Init(8);		// Light
	m_EffectAlphaFPK[3].Init(8);		// Darkness
	m_EffectAlphaFPK[4].Init(8);		// Aura(Blue)
	m_EffectAlphaFPK[5].Init(8);		// Aura(Red)
	m_EffectAlphaFPK[6].Init(8);		// Sword
	m_EffectAlphaFPK[7].Init(8);		// Comet
	m_EffectAlphaFPK[8].Init(8);		// Star
	m_EffectAlphaFPK[9].Init(8);		// Flare

	for (d=0; d<8; d++)
	{
		// ?? 8??????  8 Frames
		m_EffectAlphaFPK[0][d].Init(8);
		m_EffectAlphaFPK[1][d].Init(8);

		m_EffectAlphaFPK[0][d][0].Set(0, -35,-120, 1);		// Fire
		m_EffectAlphaFPK[0][d][1].Set(1, -35,-120, 1);		// Fire
		m_EffectAlphaFPK[0][d][2].Set(2, -35,-120, 1);		// Fire
		m_EffectAlphaFPK[0][d][3].Set(3, -35,-120, 2);		// Fire
		m_EffectAlphaFPK[0][d][4].Set(4, -35,-120, 2);		// Fire
		m_EffectAlphaFPK[0][d][5].Set(5, -35,-120, 1);		// Fire
		m_EffectAlphaFPK[0][d][6].Set(6, -35,-120, 1);		// Fire
		m_EffectAlphaFPK[0][d][7].Set(7, -35-120, 1);		// Fire


		for (int f=0; f<8; f++)
		{
			m_EffectAlphaFPK[1][d][f].Set(f,8, 0,-64, 1);	// Lightning
		}

		// Light
		m_EffectAlphaFPK[2][d].Init(2);
		m_EffectAlphaFPK[2][d][0].Set(16, 0, -48, 1);
		m_EffectAlphaFPK[2][d][1].Set(17, 0, -48, 1);
		//m_EffectAlphaFPK[2][d][1].Set(17, 0, -50);
		//m_EffectAlphaFPK[2][d][2].Set(16, 0, -54);
		//m_EffectAlphaFPK[2][d][3].Set(17, 0, -60);
		//m_EffectAlphaFPK[2][d][4].Set(16, 0, -62);
		//m_EffectAlphaFPK[2][d][5].Set(17, 0, -58);
		//m_EffectAlphaFPK[2][d][6].Set(16, 0, -50);
		//m_EffectAlphaFPK[2][d][7].Set(17, 0, -48);

		// Darkness
		m_EffectAlphaFPK[3][d].Init(2);
		m_EffectAlphaFPK[3][d][0].Set(18, 0, -48, 0);
		m_EffectAlphaFPK[3][d][1].Set(19, 0, -48, 0);

		// Aura(Blue)
		m_EffectAlphaFPK[4][d].Init(5);
		m_EffectAlphaFPK[4][d][0].Set(20, -60, -80, 2);
		m_EffectAlphaFPK[4][d][1].Set(22, -60, -80, 2);
		m_EffectAlphaFPK[4][d][2].Set(21, -60, -80, 2);
		m_EffectAlphaFPK[4][d][3].Set(22, -60, -80, 2);
		m_EffectAlphaFPK[4][d][4].Set(20, -60, -80, 2);

		// Aura(Red)
		m_EffectAlphaFPK[5][d].Init(5);
		m_EffectAlphaFPK[5][d][0].Set(23, -60, -80, 2);
		m_EffectAlphaFPK[5][d][1].Set(25, -60, -80, 2);
		m_EffectAlphaFPK[5][d][2].Set(24, -60, -80, 2);
		m_EffectAlphaFPK[5][d][3].Set(25, -60, -80, 2);
		m_EffectAlphaFPK[5][d][4].Set(23, -60, -80, 2);

		// Sword
		m_EffectAlphaFPK[6][d].Init(12);
		for (f=0; f<12; f++)
		{
			m_EffectAlphaFPK[6][d][f].Set(26,f, 0, -48, 1);
		}

		// Comet
		m_EffectAlphaFPK[7][d].Init(7);
		for (f=0; f<7; f++)
		{
			m_EffectAlphaFPK[7][d][f].Set(38,f, -20, -160, 2);
		}

		// Star
		m_EffectAlphaFPK[8][d].Init(24);
		for (f=0; f<24; f++)
		{
			m_EffectAlphaFPK[8][d][f].Set(45,(f>>1), -50, -70, 1);
		}

		// Flare
		m_EffectAlphaFPK[9][d].Init(10);
		m_EffectAlphaFPK[9][d][0].Set(57, -21, -105, 4);
		m_EffectAlphaFPK[9][d][1].Set(58, -21, -106, 4);
		m_EffectAlphaFPK[9][d][2].Set(59, -21, -108, 4);
		m_EffectAlphaFPK[9][d][3].Set(60, -21, -112, 4);
		m_EffectAlphaFPK[9][d][4].Set(61, -21, -115, 5);
		m_EffectAlphaFPK[9][d][5].Set(62, -21, -112, 4);
		m_EffectAlphaFPK[9][d][6].Set(63, -21, -108, 4);
		m_EffectAlphaFPK[9][d][7].Set(64, -21, -106, 4);
		m_EffectAlphaFPK[9][d][8].Set(65, -21, -105, 4);
		m_EffectAlphaFPK[9][d][9].Set(66, -21, -104, 4);
	}

	std::ofstream packAlphaFile(FILE_EFRAME_ALPHAEFFECT, ios::binary);
	std::ofstream indexAlphaFile(FILE_EFRAMEINDEX_ALPHAEFFECT, ios::binary);
	m_EffectAlphaFPK.SaveToFile(packAlphaFile, indexAlphaFile);
	packAlphaFile.close();
	indexAlphaFile.close();
	*/

	///*
	// Load from AlphaFile
	ivfstream AlphaFile2;//(FILE_EFRAME_ALPHAEFFECT, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_EFRAME_ALPHAEFFECT").c_str(), AlphaFile2))
		return false;
	//std::ifstream AlphaFile2("effect.efpk", ios::binary);
	m_EffectAlphaFPK.LoadFromFile(AlphaFile2);
	AlphaFile2.close();
	//*/

	/*
	// ???? ???? ??????
	//m_EffectAlphaFPK.InfoToFile("Log\\Effect.txt");
	std::ofstream infoFile("Log\\Effect.txt");

	for (int type=0; type<m_EffectAlphaFPK.GetSize(); type++)
	{
		for (int dir=0; dir<m_EffectAlphaFPK[type].GetSize(); dir++)
		{
			for (int frame=0; frame<m_EffectAlphaFPK[type][dir].GetSize(); frame++)
			{
				int spriteID = m_EffectAlphaFPK[type][dir][frame].GetSpriteID();
				int cx = m_EffectAlphaFPK[type][dir][frame].GetCX();
				int cy = m_EffectAlphaFPK[type][dir][frame].GetCY();
				infoFile << "[" << type << "][" << dir << "][" << frame << "] = ( " << spriteID << ",  " << cx << ", " << cy << " ) " << endl;
			}
		}
	}
	infoFile.close();
	*/
	/*
	//---------------------------------------------------------------
	// ?? effect?? frame?? ??????
	//---------------------------------------------------------------
	std::ofstream file("log\\EffectList.txt");
	for (int e=0; e<m_EffectAlphaFPK.GetSize(); e++)
	{
		DIRECTION_EFFECTFRAME_ARRAY& DEA = m_EffectAlphaFPK[e];

		file << "[" << e << "] " << (int)DEA[0].GetSize() << endl;
	}
	file.close();
	*/


	//---------------------------------------------------------------
	// ?? ???????
	//---------------------------------------------------------------
	/*
	int light;

	// effect ????
	for (int e=0; e<m_EffectAlphaFPK.GetSize(); e++)
	{
		DIRECTION_EFFECTFRAME_ARRAY& effect = m_EffectAlphaFPK[e];

		// ????
		for (int d=0; d<8; d++)
		{
			EFFECTFRAME_ARRAY& EA = effect[d];

			// frame??
			for (int f=0; f<EA.GetSize(); f++)
			{
				CEffectFrame &frame = EA[f];

				switch (e)
				{
					case EFFECTSPRITETYPE_LIGHT_1 :
						light = 5 + (f & 0x01);
					break;
					case EFFECTSPRITETYPE_LIGHT_2 :
						light = 6 + (f & 0x01);
					break;
					case EFFECTSPRITETYPE_LIGHT_3 :
						light = 4 + (f & 0x01);
					break;
					default :
						light = 3;
				}

				frame.Set( frame.GetSpriteID(), frame.GetCX(), frame.GetCY(), light);
			}
		}
	}

	std::ofstream packAlphaFile(FILE_EFRAME_ALPHAEFFECT, ios::binary);
	std::ofstream indexAlphaFile(FILE_EFRAMEINDEX_ALPHAEFFECT, ios::binary);
	m_EffectAlphaFPK.SaveToFile(packAlphaFile, indexAlphaFile);
	packAlphaFile.close();
	indexAlphaFile.close();
	*/

	/*
	//---------------------------------------------------------------
	// ?? sprite ??? frame???
	//---------------------------------------------------------------
	DIRECTION_EFFECTFRAME_ARRAY& effect = m_EffectAlphaFPK[EFFECTSPRITETYPE_SLAYER_DIE];

	for (int d=0; d<8; d++)
	{
		EFFECTFRAME_ARRAY& EA = effect[d];

		int frameMax = EA.GetSize()*3 + 30;

		EFFECTFRAME_ARRAY newEA;

		newEA.Init( frameMax );

		// ??? ?? ????...
		for (int i=0; i<frameMax; i++)
		{
			newEA[i] = EA[EA.GetSize()-1];
		}

		// ??frame??, ???? frame ???, ?? frame????
		for (i=0; i<EA.GetSize(); i++)
		{
			newEA[i*2] = EA[i];
			newEA[i*2,1] = EA[i];

			newEA[frameMax-1 - i] = EA[i];
		}

		EA = newEA;
	}

	std::ofstream packAlphaFile(FILE_EFRAME_ALPHAEFFECT, ios::binary);
	std::ofstream indexAlphaFile(FILE_EFRAMEINDEX_ALPHAEFFECT, ios::binary);
	m_EffectAlphaFPK.SaveToFile(packAlphaFile, indexAlphaFile);
	packAlphaFile.close();
	indexAlphaFile.close();
	*/


	//	#ifdef OUTPUT_DEBUG
	//
	//		m_EffectShadowFPK.Init( 1 );
	//
	//		for (int se=0; se<1; se++)
	//		{
	//			m_EffectShadowFPK[se].Init( 8 );
	//
	//			for (int d=0; d<8; d++)
	//			{
	//				m_EffectShadowFPK[se][d].Init( 3 );
	//
	//				for (int f=0; f<3; f++)
	//				{
	//					m_EffectShadowFPK[se][d][f].Set( f, -110, -115, 6 );
	//				}
	//			}
	//		}
	//		
	//
	//		std::ofstream packshadowEffectFile(g_pFileDef->getProperty("FILE_EFRAME_SHADOWEFFECT").c_str(), ios::binary);
	//		std::ofstream indexshadowEffectFile(g_pFileDef->getProperty("FILE_EFRAMEINDEX_SHADOWEFFECT").c_str(), ios::binary);
	//		m_EffectShadowFPK.SaveToFile(packshadowEffectFile, indexshadowEffectFile);
	//		packshadowEffectFile.close();
	//		indexshadowEffectFile.close();
	//	#endif

		///*
		// Load from shadowEffectFile
	ivfstream shadowEffectFile2;
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_EFRAME_SHADOWEFFECT").c_str(), shadowEffectFile2))
		return false;
	m_EffectShadowFPK.LoadFromFile(shadowEffectFile2);
	shadowEffectFile2.close();

	// Load from ScreenFile
	ivfstream ScreenFile2;//(FILE_EFRAME_ScreenEFFECT, ios::binary);
	if (!FileOpenBinary(g_pFileDef->getProperty("FILE_EFRAME_SCREENEFFECT").c_str(), ScreenFile2))
		return false;
	//ivfstream ScreenFile2("effect.efpk", ios::binary);
	m_EffectScreenFPK.LoadFromFile(ScreenFile2);
	ScreenFile2.close();

	return true;
}

//----------------------------------------------------------------------
// Load Minimap
//----------------------------------------------------------------------
void
MTopView::LoadMinimap(const char* filename)//, MZoneInfo* pZoneInfo)
{
	/* // 2001.7.14 ???ó??
	//------------------------------------------------------------
	//
	//				Minimap
	//
	//------------------------------------------------------------
	//------------------------------------------------------------
	// Load  Clothes SpritePack
	//------------------------------------------------------------
	std::ifstream	MapTest1;
	if (!FileOpenBinary(filename, MapTest1))
		return;
	m_pMinimapSPR->LoadFromFile( MapTest1 );
	*/

	//------------------------------------------------------------	
	// ???????? ZoneInfo?? ??? ???
	//------------------------------------------------------------	
	/*
	if (pZoneInfo!=NULL)
	{
		int i;
		int j;

		int spWidth = m_pMinimapSPR->GetWidth();
		int spHeight = m_pMinimapSPR->GetHeight();
		int zoneWidth = pZoneInfo->GetWidth();
		int zoneHeight = pZoneInfo->GetHeight();

		WORD* pPixel;
		int	count, transCount, colorCount;

		int spX, spY, zoneX, zoneY;

		// (256,128) --> (256,256)
		// (spX, spX) ---> (zoneX, zoneY)
		// zoneX = spX * zoneHeight / spHeight
		CSpriteSurface::s_Value1 = 16;
		CSpriteSurface::s_Value2 = 16;
		WORD green = CDirectDraw::Color(0, 20, 0);

		//----------------------------------------------------------------
		// Sprite??ü?? ????????. - -;
		//----------------------------------------------------------------
		for (spY=0; spY<spHeight; spY++)
		{
			zoneY = spY * zoneHeight / spHeight;

			spX = 0;
			pPixel = m_pMinimapSPR->GetPixelLine( spY );

			count = *pPixel++;		// ??? ???

			//----------------------------------------------------------------
			// ?? ??? ???? ü?..
			//----------------------------------------------------------------
			for (i=0; i<count; i++)
			{
				transCount = *pPixel++;
				colorCount = *pPixel++;

				spX += transCount;

				for (j=0; j<colorCount; j++)
				{
					zoneX = spX * zoneWidth / spWidth;

					//---------------------------------------------------------
					// ??????????? ??????? ??????..
					//---------------------------------------------------------
					if (pZoneInfo->IsSafeSector(zoneX, zoneY))
					{
						// 1?? ??????? alpha blending - -;
						CSpriteSurface::memcpyAlpha(pPixel, &green, 1);
					}

					// ???? ??
					pPixel++;
					spX++;
				}
			}
		}
	}
	*/

	/* // 2001.7.14 ???ó??
	//------------------------------------------------------------
	// 3D ???????...
	//------------------------------------------------------------
	if (CDirect3D::IsHAL())
	{
		InitMinimapTexture();
	}

	// m_SectorToMinimapWidth - ?? sector?? ???? ?? pixel????
	// m_SectorToMinimapHeight - ?? sector?? ???? ?? pixel????
	if (g_pZone->GetWidth()==0 || g_pZone->GetHeight()==0)
	{
		m_SectorToMinimapWidth	= 0;
		m_SectorToMinimapHeight = 0;
	}
	else
	{
		m_SectorToMinimapWidth = (float)m_pMinimapSPR->GetWidth() / (float)g_pZone->GetWidth();
		m_SectorToMinimapHeight = (float)m_pMinimapSPR->GetHeight() / (float)g_pZone->GetHeight();
	}

	MapTest1.close();
	*/
}

//----------------------------------------------------------------------
// Init MinimapTexture
//----------------------------------------------------------------------
//void
//MTopView::InitMinimapTexture(CSpriteSurface *minimap_surface)
//{
//	// ??????????? ?????? by ????
//	if(minimap_surface != NULL)
//	{
//		if(minimap_surface->Lock())
//		{
//			WORD *pSurface = (WORD*)minimap_surface->GetSurfacePointer();
//			m_pMinimapSPR->SetPixelNoColorkey(pSurface, minimap_surface->GetSurfacePitch(), minimap_surface->GetWidth(), minimap_surface->GetHeight());
//			
//			minimap_surface->Unlock();
//		}
//	}
//
//	//------------------------------------------------------------
//	// 3D ???????...
//	//------------------------------------------------------------
//	if (!CDirect3D::IsHAL())
//		return;
//
//	// 2001.7.14 ???ó??
//	if (m_pMinimapTexture!=NULL)
//	{
//		delete m_pMinimapTexture;
//		m_pMinimapTexture = NULL;
//	}
//
//	m_pMinimapTexture = new CSpriteSurface;
//
//	if (m_pMinimapTexture==NULL)
//	{
//		DEBUG_ADD("[Error] m_pMinimapTexture is NULL");
//		
//		return;
//	}
//
//	// texture size ????
//	int spWidth, spHeight;
//	int width, height;
//	BYTE smallShift = 0;
//
//	spWidth = m_pMinimapSPR->GetWidth();
//	spHeight = m_pMinimapSPR->GetHeight();
//	width = spWidth;
//	height = spHeight;
//	CDirect3D::GetTextureSize(width, height);		
//
//	//-----------------------------------------------------------
//	//
//	// ?? ??? ??? ???? ???.. ????... - -;;
//	//
//	//-----------------------------------------------------------
//	//
//	// m_pMinimapTexture?? minimap?? ??????.
//	//
//	//-----------------------------------------------------------
//	//-----------------------------------------------------------
//	// 2?? ?¼??? ??????? ???..
//	//-----------------------------------------------------------
//	bool bDifferentSize;	// sprite?? texture?? ??? ??????
//	if (CDirect3D::IsTexturePow2())
//	{
//		bDifferentSize = true;
//
//		// width?? height?? ??? ???...
//		// ???? ??? ???? ????? ???.
//		// Square?? ?????..?? ?? ?? ????.. ????..
//		if (width < height)
//		{
//			height = width;
//		}
//		else if (width > height)
//		{
//			width = height;
//		}
//
//		//-----------------------------------------------------------
//		// Texture????? Sprite ??? ?? ? ???..
//		//-----------------------------------------------------------
//		// ??, ???????? Sprite?????? Texture?? ???????? ????? ??????.		
//		// shift?? ?????? ??? ?????.
//		while (spWidth > width || spHeight > height)
//		{
//			smallShift ++;
//
//			spWidth >>= 1;
//			spHeight >>= 1;
//		}
//	}
//	//-----------------------------------------------------------
//	// ????? size?? ???? ???? ???
//	//-----------------------------------------------------------
//	else
//	{
//		if (CDirect3D::IsTextureSquareOnly())
//		{
//			// width?? height?? ??? ???...
//			// ? ??? ???? ????? ???.
//			// Square?? ?????..?? ?? ?? ????.. ????..
//			if (width > height)
//			{
//				height = width;
//				bDifferentSize = true;
//			}
//			else if (width < height)
//			{
//				width = height;
//				bDifferentSize = true;
//			}		
//			
//			m_MinimapTextureWidth = width;
//			m_MinimapTextureHeight = height;
//		}
//		else
//		{
//			bDifferentSize = false;
//		}			
//	}		
//
//	//---------------------------------------------------
//	// TextureSurface ????
//	//---------------------------------------------------		
//	m_pMinimapTexture->InitTextureSurface(width, height, 0, CDirect3D::GetPixelFormat4444());
//	POINT point;
//	point.x = 0;
//	point.y = 0;
//	m_pMinimapTexture->Lock();
//
////	int i;
//	
//
//	//---------------------------------------------------
//	// ??? ???? ???..
//	//---------------------------------------------------
//	//if (bDifferentSize)
//	{
//		/*
//		// Sprite?? ??????? ?????? ?????? ????? ???~~
//		DWORD width2 = (width - spWidth) << 1;	// *2 
//		pSurface += spWidth;
//
//		// ?????? ?????
//		if (width2 > 0)
//		{
//			i = spHeight;		
//			do
//			{			
//				memset(pSurface, 0, width2);
//				pSurface = (WORD*)((BYTE*)pSurface + pitch);
//			} while (--i);
//		}
//
//		// ?????
//		pSurface -= spWidth;
//		width2 = width << 1;
//		i = height - spHeight;
//
//		if (i>0)
//		{
//			do
//			{			
//				memset(pSurface, 0, width2);
//				pSurface = (WORD*)((BYTE*)pSurface + pitch);
//			} while (--i);
//		}
//		*/
//		
//		//---------------------------------------------------
//		// Texture Surface ????
//		//---------------------------------------------------
//		/*// 2001.7.14 ???ó??
//		WORD *pSurface = (WORD*)m_pMinimapTexture->GetSurfacePointer();
//				//,	*pSurfaceTemp;
//		long pitch	= m_pMinimapTexture->GetSurfacePitch();
//
//		int width2 = width << 1;
//
//		for (i=0; i<height; i++)
//		{
//			memset(pSurface, 0, width2);
//			pSurface = (WORD*)((BYTE*)pSurface + pitch);
//		}
//		*/
//		
//	}
//	
//	//---------------------------------------------------
//	// Sprite???
//	//---------------------------------------------------
//	// ??? ???????..
//
//	 // 2001.7.14 ???ó??
//	// alpha???? 16 == ??????
//	BYTE alpha = 16;
//
//	if (smallShift==0)
//	{
//		m_pMinimapTexture->BltSpriteAlpha4444NotTrans(&point, m_pMinimapSPR, alpha);
//	}
//	else
//	{
//		m_pMinimapTexture->BltSpriteAlpha4444SmallNotTrans(&point, m_pMinimapSPR, alpha, smallShift);
//	}
//
//	//---------------------------------------------------------------
//	// ???????? ???
//	//---------------------------------------------------------------
//	spWidth = m_pMinimapSPR->GetWidth();
//	spHeight = m_pMinimapSPR->GetHeight();
//	if (CDirect3D::IsTexturePow2())	
//	{
//		width = 1;
//		height = 1;
//
//		// width?? height?? spWidth?? spHeight???? ??? ???.
//		// square?? ???????.
//		while (width < spWidth || height < spHeight)
//		{
//			width <<= 1;
//			height <<= 1;
//		}
//	}
//	else
//	{
//		width = spWidth;
//		height = spHeight;
//
//		// ? ???? ????? Square?? ?????.
//		if (CDirect3D::IsTextureSquareOnly())
//		{				
//			if (width > height)
//			{
//				height = width;				
//			}
//			else if (width < height)
//			{
//				width = height;				
//			}	
//		}
//	}
//
//	m_MinimapTextureWidth = width;
//	m_MinimapTextureHeight = height;
//	
//	m_pMinimapTexture->Unlock();
//	
//}

//----------------------------------------------------------------------
// Use HalfFrame
//----------------------------------------------------------------------
/*
void
MTopView::UseHalfFrame(bool bUse)
{
	// Addon?? ???? ????µ? - -;
	// Effect??... - -;

	//----------------------------------------------------------
	// ??? Loading?? ??? ????
	//----------------------------------------------------------
	m_CreatureSPK.Release();
	m_CreatureSSPK.Release();

	if (CDirect3D::IsHAL())
	{
		m_pCreatureShadowManager->Release();

		m_pCreatureShadowManager->Init( FILE_SSPRITE_CREATURE,
							FILE_SSPRITEINDEX_CREATURE,
							g_pClientConfig->MAX_TEXTUREPART_CREATURESHADOW);
	}

	//----------------------------------------------------------
	// Loading ??? ????? ???
	//----------------------------------------------------------
	int num = g_pCreatureSpriteTable->GetSize();

	for (int i=0; i<num; i++)
	{
		(*g_pCreatureSpriteTable)[i].bLoad = false;
	}

	if (bUse)
	{
		// ??? Loading?? ??? ????
		std::ifstream file(FILE_CFRAME_CREATURE2, ios::binary);
		m_CreatureFPK.LoadFromFile(file);
		file.close();
	}
	else
	{
		// ??? Loading?? ??? ????
		std::ifstream file(FILE_CFRAME_CREATURE, ios::binary);
		m_CreatureFPK.LoadFromFile(file);
		file.close();
	}
}
*/

//----------------------------------------------------------------------
// Load From File CreatureSPK
//----------------------------------------------------------------------
// ???? Zone???? ?????? Creature?? ???? Sprite???? Load???.
//----------------------------------------------------------------------
void
MTopView::LoadFromFileCreatureSPK(int spriteType)
{
	if (spriteType < 0 || spriteType >= (*g_pCreatureSpriteTable).GetSize())
	{
		DEBUG_ADD_FORMAT("[Error] Wrong SpriteType=%d", spriteType);

		return;
	}

	//int spriteType = (*g_pCreatureTable)[n].SpriteType;

	//----------------------------------------------------------------------
	// Load??????? ü??????.
	//----------------------------------------------------------------------
	if ((*g_pCreatureSpriteTable)[spriteType].bLoad)
	{
		// ??? Load?? ???
	}
	//----------------------------------------------------------------------
	// Load????? ???
	//----------------------------------------------------------------------
	// CreatureAction?? ???? g_pCreatureActionSpriteTable?? ????? ??????
	// ?????? loading???? ??´?.  2001.11.21
	//----------------------------------------------------------------------			
	else
	{
		if (g_pUserOption->IsPreLoadMonster)
		{

			// ???? Zone???? ????? Sprite???? Load??? ???.
//			std::ifstream	creatureFile;//(FILE_ISPRITE_CREATURE, ios::binary);
//			std::ifstream	creatureShadowFile;
//
//			//------------------------------------------------------------
//			// sprite load
//			//------------------------------------------------------------
//			if (!FileOpenBinary(FILE_ISPRITE_CREATURE, creatureFile))
//				return;	


			//------------------------------------------------------------
			// Half Frame?? ??????
			//------------------------------------------------------------
			if (0)//g_pUserOption->UseHalfFrame)
			{
				/*
				int frameID = (*g_pCreatureSpriteTable)[spriteType].FrameID;

				//------------------------------------------------------------
				// ????? EFPK?? ??????? ???? SpriteID?? ?????.
				//------------------------------------------------------------
				COrderedList<int> intList;
				ACTION_FRAME_ARRAY& AFA = CreatureFPK[frameID];
				int numAction = AFA.GetSize();

				// orderedList?? ????? ?????? ????? ???? ..
				for (int a=AFA.GetSize()-1; a>=0; a--)
				{
					DIRECTION_FRAME_ARRAY& DFA = AFA[a];

					for (int d=DFA.GetSize()-1; d>=0; d--)
					{
						FRAME_ARRAY& EA = DFA[d];

						for (int f=FA.GetSize()-1; f>=0; f--)
						{
							CFrame& Frame = FA[f];

							int spriteID = Frame.GetSpriteID();

							intList.Add( spriteID );
						}
					}
				}

				COrderedList<int>::DATA_LIST::const_iterator iID = intList.GetIterator();
				int numID = intList.GetSize();

				CSpriteFilePositionArray SFPA;
				SFPA.Init( numID );

				std::ifstream CreaturePackIndexFile(FILE_ISPRITEINDEX_CREATURE, ios::binary);

				long fp;
				for (int i=0; i<numID; i++)
				{
					int id = *iID

					CreaturePackIndexFile.seekg( 2 + (id<<2));	// 2 + id*4
					CreaturePackIndexFile.read((char*)&fp, 4);


					SFPA[id].SpriteID		= id;
					SFPA[id].FilePosition	= fp;
				}

				m_CreatureSPK.LoadFromFile(creatureFile, SFPA);

				CreaturePackIndexFile.close();
				*/
			}
			//------------------------------------------------------------
			// ??ü Frame ?? ???
			//------------------------------------------------------------
			else
			{


				TYPE_SPRITEID first = (*g_pCreatureSpriteTable)[spriteType].FirstSpriteID;
				TYPE_SPRITEID last = (*g_pCreatureSpriteTable)[spriteType].LastSpriteID;
				long			fp = (*g_pCreatureSpriteTable)[spriteType].SpriteFilePosition;

				int a = (*g_pCreatureSpriteTable)[spriteType].FrameID;
				if (a < 287)
				{
					m_CreatureSPK.LoadFromFilePart(first, last);
				}
				//#if __CONTENTS(__ONIBLA)
				else
				{
					m_CreatureSPK2.LoadFromFilePart(first, last);
				}
				//#endif //__ONIBLA
								// n??° creature load	
				//				m_CreatureSPK.LoadFromFilePart(creatureFile, 
				//												fp,
				//												first, 
				//												last
				//												);

			}

			// 			creatureFile.close();

						//------------------------------------------------------------
						// ????? load
						//------------------------------------------------------------
			//			if (!FileOpenBinary(FILE_SSPRITE_CREATURE, creatureShadowFile))
			//				return;	


			//			if (!CDirect3D::IsHAL())
			{
				//------------------------------------------------------------
				// Half Frame ???
				//------------------------------------------------------------
				if (g_pUserOption->UseHalfFrame)
				{
				}
				//------------------------------------------------------------
				// ??ü Frame ?? ???
				//------------------------------------------------------------
				else
				{
					TYPE_SPRITEID first = (*g_pCreatureSpriteTable)[spriteType].FirstShadowSpriteID;
					TYPE_SPRITEID last = (*g_pCreatureSpriteTable)[spriteType].LastShadowSpriteID;

					int a = (*g_pCreatureSpriteTable)[spriteType].FrameID;
					if (a < 287)
					{
						m_CreatureSSPK.LoadFromFilePart(first, last);
					}
					//#if __CONTENTS(__ONIBLA)
					else
					{
						m_CreatureSSPK2.LoadFromFilePart(first, last);
					}
					//#endif //__ONIBLA


					//					m_CreatureSSPK.LoadFromFilePart(creatureShadowFile, 
					//													(*g_pCreatureSpriteTable)[spriteType].SpriteShadowFilePosition,
					//													first, 
					//													last
					//													);
				}
			}
		}

		// Load???? ??????.
		(*g_pCreatureSpriteTable)[spriteType].bLoad = TRUE;

		//--------------------------------------------------------
		// Player???????? ??????? ??´?.
		//--------------------------------------------------------
		//if (spriteType!=(*g_pCreatureTable)[CREATURETYPE_SLAYER_MALE].SpriteType
		//	&& spriteType!=(*g_pCreatureTable)[CREATURETYPE_SLAYER_FEMALE].SpriteType
		//	&& spriteType!=(*g_pCreatureTable)[CREATURETYPE_VAMPIRE_MALE].SpriteType
		//	&& spriteType!=(*g_pCreatureTable)[CREATURETYPE_VAMPIRE_FEMALE].SpriteType
		//	&& spriteType!=(*g_pCreatureTable)[CREATURETYPE_WOLF].SpriteType
		//	&& spriteType!=(*g_pCreatureTable)[CREATURETYPE_BAT].SpriteType
		//	&& spriteType!=(*g_pCreatureTable)[CREATURETYPE_SLAYER_OPERATOR].SpriteType
		//	&& spriteType!=(*g_pCreatureTable)[CREATURETYPE_VAMPIRE_OPERATOR].SpriteType)
		if (!(*g_pCreatureSpriteTable)[spriteType].IsPlayerSprite())
		{
			m_listLoadedCreatureSprite.Add(spriteType);

			if ((*g_pCreatureSpriteTable)[spriteType].IsMonsterSprite())
			{
				m_listLoadedMonsterSprite.Add(spriteType);
			}
		}


		/*
		//------------------------------------------------------------
		// ????? Pack????
		//------------------------------------------------------------
		m_CreatureSSPK.InitPart(m_CreatureSPK,
								(*g_pCreatureTable)[n].FirstSpriteID,
								(*g_pCreatureTable)[n].LastSpriteID);

		//------------------------------------------------------------
		//
		// Init ????? TextureSurface
		//
		//------------------------------------------------------------
		if (CDirect3D::IsHAL())
		{
			//--------------------------------------------
			// AlphaSPK?? ?????? TexturePack?? ???????.
			//--------------------------------------------
			if (!m_CreatureTPK.InitPart( m_CreatureSSPK,
										(*g_pCreatureTable)[n].FirstSpriteID,
										(*g_pCreatureTable)[n].LastSpriteID))
			{
				MessageBox(g_hWnd, "Can't Init ShadowTexture!", NULL, MB_OK);
				return;
			}

			//------------------------------------------------------------
			// EffectAlphaSprite?? ??????? ???????.
			// --> TextureSurface?? ????? ??????.. ??? ????.
			//------------------------------------------------------------
			m_CreatureSSPK.ReleasePart((*g_pCreatureTable)[n].FirstSpriteID,
										(*g_pCreatureTable)[n].LastSpriteID);
		}
		*/
	}
}

//----------------------------------------------------------------------
// Load From File AddonSPK
//----------------------------------------------------------------------
// ???? ????? Addon?? Load???.
//----------------------------------------------------------------------
//void
//MTopView::LoadFromFileAddonSPK(int frame, int action)
//{
//	int frameID = frame*ACTION_MAX_SLAYER + action;
//
//	if (frameID<0 || frameID >= (*g_pAddonSpriteTable).GetSize())
//	{
//		DEBUG_ADD_FORMAT("[Error] Wrong AddonType=%d, %d", frame, action);
//		
//		return;
//	}
//
//	//int spriteType = (*g_pCreatureTable)[n].SpriteType;
//
//	//----------------------------------------------------------------------
//	// Load??????? ü??????.
//	//----------------------------------------------------------------------
//	if ((*g_pAddonSpriteTable)[frameID].bLoad)
//	{
//		// ??? Load?? ???
//	}
//	//----------------------------------------------------------------------
//	// Load????? ???
//	//----------------------------------------------------------------------
//	else
//	{		
//		TYPE_SPRITEID first = (*g_pAddonSpriteTable)[frameID].FirstSpriteID;
//		TYPE_SPRITEID last	= (*g_pAddonSpriteTable)[frameID].LastSpriteID;
//		long			fp	= (*g_pAddonSpriteTable)[frameID].SpriteFilePosition;
//
//		DEBUG_ADD_FORMAT("[LoadAddonAction] frameID=%d, action=%d, spr=%d~%d", frameID, action, first, last);
//
//		__BEGIN_PROFILE("LoadAddonAction")
//
//		m_AddonSPK.LoadFromFilePart(first, last);
////		m_AddonSPK.LoadFromFilePart(m_AddonSPKFile, 
////										fp,
////										first, 
////										last
////										);
//
//		__END_PROFILE("LoadAddonAction")
//
//		if (!CDirect3D::IsHAL())
//		{			
//			TYPE_SPRITEID first = (*g_pAddonSpriteTable)[frameID].FirstShadowSpriteID;
//			TYPE_SPRITEID last	= (*g_pAddonSpriteTable)[frameID].LastShadowSpriteID;
//			long			fp	= (*g_pAddonSpriteTable)[frameID].SpriteShadowFilePosition;
//
//			DEBUG_ADD_FORMAT("[LoadAddonActionShadow] spr=%d~%d", frameID, action, first, last);
//
//			__BEGIN_PROFILE("LoadAddonActionShadow")
//
////			m_AddonSSPK.LoadFromFilePart(m_AddonSSPKFile, 
////											fp,
////											first, 
////											last
////											);			
//			m_AddonSSPK.LoadFromFilePart(first, last);
//
//			__END_PROFILE("LoadAddonActionShadow")
//		}
//
//		(*g_pAddonSpriteTable)[frameID].bLoad = TRUE;
//	}
//}

//----------------------------------------------------------------------
// Load From File CreatureActionSPK
//----------------------------------------------------------------------
// ???? ????? CreatureAction?? Load???.
//----------------------------------------------------------------------
//void
//MTopView::LoadFromFileCreatureActionSPK(int frame, int action)
//{
//	int frameID = frame*ACTION_MAX_VAMPIRE + action;
//
//	if (frameID<0 || frameID >= (*g_pCreatureActionSpriteTable).GetSize())
//	{
//		DEBUG_ADD_FORMAT("[Error] Wrong CreatureActionType=%d, %d", frame, action);
//		
//		return;
//	}
//
//	//int spriteType = (*g_pCreatureTable)[n].SpriteType;
//
//	//----------------------------------------------------------------------
//	// Load??????? ü??????.
//	//----------------------------------------------------------------------
//	if ((*g_pCreatureActionSpriteTable)[frameID].bLoad)
//	{
//		// ??? Load?? ???
//	}
//	//----------------------------------------------------------------------
//	// Load????? ???
//	//----------------------------------------------------------------------
//	else
//	{		
//		TYPE_SPRITEID first = (*g_pCreatureActionSpriteTable)[frameID].FirstSpriteID;
//		TYPE_SPRITEID last	= (*g_pCreatureActionSpriteTable)[frameID].LastSpriteID;
//		long			fp	= (*g_pCreatureActionSpriteTable)[frameID].SpriteFilePosition;
//
//		DEBUG_ADD_FORMAT("[LoadCreatureAction] frameID=%d, action=%d, spr=%d~%d", frameID, action, first, last);
//
//		__BEGIN_PROFILE("LoadCreatureAction")
//
////		m_CreatureSPK.LoadFromFilePart(m_CreatureSPKFile, 
////										fp,
////										first, 
////										last
////										);
//		m_CreatureSPK.LoadFromFilePart(first, last);
//
//		__END_PROFILE("LoadCreatureAction")
//
//		if (!CDirect3D::IsHAL())
//		{			
//			TYPE_SPRITEID first = (*g_pCreatureActionSpriteTable)[frameID].FirstShadowSpriteID;
//			TYPE_SPRITEID last	= (*g_pCreatureActionSpriteTable)[frameID].LastShadowSpriteID;
//			long			fp	= (*g_pCreatureActionSpriteTable)[frameID].SpriteShadowFilePosition;
//
//			DEBUG_ADD_FORMAT("[LoadCreatureActionShadow] spr=%d~%d", first, last);
//
//			__BEGIN_PROFILE("LoadCreatureActionShadow")
//
////			m_CreatureSSPK.LoadFromFilePart(m_CreatureSSPKFile, 
////											fp,
////											first, 
////											last
////											);		
//			m_CreatureSSPK.LoadFromFilePart(first, last);
//			
//			__END_PROFILE("LoadCreatureActionShadow")
//		}
//
//		(*g_pCreatureActionSpriteTable)[frameID].bLoad = TRUE;
//
//		// frameID --> spriteType
//		//int spriteType = frameID + 2;	// mapping table?? ?????? ??µ?.
//										// slayer???? ??????? ????????..
//
//		// loading???? ??????? ?? ??? ?? release?? ?? ???.
//		// map???? ??????.
//		//m_listLoadedCreatureSprite.push_back( spriteType );
//	}
//}
//
////----------------------------------------------------------------------
//// Release CreatureActionSPK
////----------------------------------------------------------------------
//void
//MTopView::ReleaseCreatureActionSPK(int frame, int action)
//{
//	int frameID = frame*ACTION_MAX_VAMPIRE + action;
//
//	if (frameID<0 || frameID >= (*g_pCreatureActionSpriteTable).GetSize())
//	{
//		DEBUG_ADD_FORMAT("[Error] Wrong CreatureActionType=%d, %d", frame, action);
//		
//		return;
//	}
//
//	//int spriteType = (*g_pCreatureTable)[n].SpriteType;
//
//	//----------------------------------------------------------------------
//	// Load??????? ü??????.
//	//----------------------------------------------------------------------
//	if ((*g_pCreatureActionSpriteTable)[frameID].bLoad)
//	{
//		TYPE_SPRITEID first = (*g_pCreatureActionSpriteTable)[frameID].FirstSpriteID;
//		TYPE_SPRITEID last	= (*g_pCreatureActionSpriteTable)[frameID].LastSpriteID;
//		
//		m_CreatureSPK.ReleasePart(first, last);
//
//		if (!CDirect3D::IsHAL())
//		{			
//			TYPE_SPRITEID first = (*g_pCreatureActionSpriteTable)[frameID].FirstShadowSpriteID;
//			TYPE_SPRITEID last	= (*g_pCreatureActionSpriteTable)[frameID].LastShadowSpriteID;
//			
// 			m_CreatureSSPK.ReleasePart(first, last);			
//		}
//
//		(*g_pCreatureActionSpriteTable)[frameID].bLoad = FALSE;
//	}
//}

//----------------------------------------------------------------------
// Release From CreatureSPK
//----------------------------------------------------------------------
// ???? Load?? Creature Sprite?? ??? ?????????.
//----------------------------------------------------------------------
void
MTopView::ReleaseCreatureSPK(int n)
{
	//	//--------------------------------------------------------
	//	// Player???????? ???????? ??´?.
	//	//--------------------------------------------------------
	//	//if (n==CREATURETYPE_SLAYER_MALE
	//	//	|| n==CREATURETYPE_SLAYER_FEMALE
	//	//	|| n==CREATURETYPE_VAMPIRE_MALE
	//	//	|| n==CREATURETYPE_VAMPIRE_FEMALE
	//	//	|| n==CREATURETYPE_WOLF
	//	//	|| n==CREATURETYPE_BAT
	//	//	|| n==CREATURETYPE_SLAYER_OPERATOR
	//	//	|| n==CREATURETYPE_VAMPIRE_OPERATOR)
	//	//{
	//	//	return;
	//	//{
	//
	//	int spriteType = (*g_pCreatureTable)[n].SpriteType;
	//
	//	// Player???????? ???????? ??´?.
	//	if ((*g_pCreatureSpriteTable)[spriteType].IsPlayerSprite())
	//	{
	//		return;
	//	}
	//
	//	//----------------------------------------------------------------------
	//	// Load??????? ü??????.
	//	//----------------------------------------------------------------------
	//	if ((*g_pCreatureSpriteTable)[spriteType].bLoad)
	//	{
	//		/*
	//		// ??? Load?? ???
	//		// n??° creature?? sprite?? ??? ????
	//		m_CreatureSPK.ReleasePart((*g_pCreatureSpriteTable)[spriteType].FirstSpriteID,
	//									(*g_pCreatureSpriteTable)[spriteType].LastSpriteID);
	//
	//		m_CreatureSSPK.ReleasePart((*g_pCreatureSpriteTable)[spriteType].FirstSpriteID,
	//									(*g_pCreatureSpriteTable)[spriteType].LastSpriteID);
	//
	//		*/
	////		int frameID = (*g_pCreatureSpriteTable)[spriteType].FrameID;
	////
	////		if (frameID!=FRAMEID_NULL)
	////		{
	////			for (int a=0; a<ACTION_MAX_VAMPIRE; a++)
	////			{
	////				ReleaseCreatureActionSPK( frameID, a );
	////			}
	//			TYPE_SPRITEID first = (*g_pCreatureSpriteTable)[spriteType].FirstSpriteID;
	//			TYPE_SPRITEID last	= (*g_pCreatureSpriteTable)[spriteType].LastSpriteID;
	//			long			fp	= (*g_pCreatureSpriteTable)[spriteType].SpriteFilePosition;
	//			
	//			m_CreatureSPK.ReleasePart(first, last);
	//// 		}
	//
	//		// load???? ????? ??????.
	//		(*g_pCreatureSpriteTable)[spriteType].bLoad = FALSE;
	//
	//		m_listLoadedCreatureSprite.Remove( spriteType );
	//
	//		if ((*g_pCreatureSpriteTable)[spriteType].IsMonsterSprite())
	//		{
	//			m_listLoadedMonsterSprite.Remove( spriteType );
	//		}
	//	}
	//	else
	//	{
	//		// Load???? ???? ???
	//	}
		//--------------------------------------------------------
		// Player???????? ???????? ??´?.
		//--------------------------------------------------------
		//if (n==CREATURETYPE_SLAYER_MALE
		//	|| n==CREATURETYPE_SLAYER_FEMALE
		//	|| n==CREATURETYPE_VAMPIRE_MALE
		//	|| n==CREATURETYPE_VAMPIRE_FEMALE
		//	|| n==CREATURETYPE_WOLF
		//	|| n==CREATURETYPE_BAT
		//	|| n==CREATURETYPE_SLAYER_OPERATOR
		//	|| n==CREATURETYPE_VAMPIRE_OPERATOR)
		//{
		//	return;
		//{

		//int spriteType = (*g_pCreatureTable)[n].SpriteTypes[0];
	int spriteTypeCount = (*g_pCreatureTable)[n].SpriteTypes.GetSize();

	for (int FrameIndex = 0; FrameIndex < spriteTypeCount; FrameIndex++)
	{

		int spriteType = (*g_pCreatureTable)[n].SpriteTypes[FrameIndex];
		// Player???????? ???????? ??´?.
		if ((*g_pCreatureSpriteTable)[spriteType].IsPlayerSprite())
		{
			return;
		}

		//----------------------------------------------------------------------
		// Load??????? ü??????.
		//----------------------------------------------------------------------
		if ((*g_pCreatureSpriteTable)[spriteType].bLoad)
		{
			/*
			// ??? Load?? ???
			// n??° creature?? sprite?? ??? ????
			m_CreatureSPK.ReleasePart((*g_pCreatureSpriteTable)[spriteType].FirstSpriteID,
			(*g_pCreatureSpriteTable)[spriteType].LastSpriteID);

			  m_CreatureSSPK.ReleasePart((*g_pCreatureSpriteTable)[spriteType].FirstSpriteID,
			  (*g_pCreatureSpriteTable)[spriteType].LastSpriteID);

				*/
				//		int frameID = (*g_pCreatureSpriteTable)[spriteType].FrameID;
				//
				//		if (frameID!=FRAMEID_NULL)
				//		{
				//			for (int a=0; a<ACTION_MAX_VAMPIRE; a++)
				//			{
				//				ReleaseCreatureActionSPK( frameID, a );
				//			}
			TYPE_SPRITEID first = (*g_pCreatureSpriteTable)[spriteType].FirstSpriteID;
			TYPE_SPRITEID last = (*g_pCreatureSpriteTable)[spriteType].LastSpriteID;
			long			fp = (*g_pCreatureSpriteTable)[spriteType].SpriteFilePosition;

			int a = (*g_pCreatureSpriteTable)[spriteType].FrameID;

			if (a < 287)
			{
				m_CreatureSPK.ReleasePart(first, last);
			}
			//#if __CONTENTS(__ONIBLA)
			else
			{
				m_CreatureSPK2.ReleasePart(first, last);
			}
			//#endif //__ONIBLA
						// 		}

						// load???? ????? ??????.
			(*g_pCreatureSpriteTable)[spriteType].bLoad = FALSE;

			m_listLoadedCreatureSprite.Remove(spriteType);

			if ((*g_pCreatureSpriteTable)[spriteType].IsMonsterSprite())
			{
				m_listLoadedMonsterSprite.Remove(spriteType);
			}
		}
		else
		{
			// Load???? ???? ???
		}




	}
}

//----------------------------------------------------------------------
// Add MonsterSpriteTypes
//----------------------------------------------------------------------
// Hallu???? ??????? ?? ?????...
//----------------------------------------------------------------------
void
MTopView::AddMonsterSpriteTypes(const INT_ORDERED_LIST& listUse)
{
	INT_ORDERED_LIST::DATA_LIST::const_iterator iUse = listUse.GetIterator();

	for (int i = 0; i < listUse.GetSize(); i++)
	{
		int useSpriteType = *iUse;

		if ((*g_pCreatureSpriteTable)[useSpriteType].IsMonsterSprite())
		{
			m_listLoadedMonsterSprite.Add(useSpriteType);
		}

		iUse++;
	}
}

//----------------------------------------------------------------------
// Release Useless CreatureSPK Except
//----------------------------------------------------------------------
// list?? ?????? creature???? sprite type ???.
//----------------------------------------------------------------------
void
MTopView::ReleaseUselessCreatureSPKExcept(const INT_ORDERED_LIST& listUse)
{
	// ?????? ??+NPC ??????????? ?? ??????? release???? ??´?.
	//return;

	//--------------------------------------------------------
	// load?? ??? ????? ????????? ????..
	//--------------------------------------------------------
	m_listLoadedCreatureSprite -= listUse;

	//--------------------------------------------------------
	// ???????? ???????.
	//--------------------------------------------------------
	INT_ORDERED_LIST::DATA_LIST::const_iterator iSpriteType = m_listLoadedCreatureSprite.GetIterator();

	for (int i = 0; i < m_listLoadedCreatureSprite.GetSize(); i++)
	{
		int spriteType = *iSpriteType;

		//--------------------------------------------------------
		// Player???????? ???????? ??´?.
		//--------------------------------------------------------
		//if (spriteType==(*g_pCreatureTable)[CREATURETYPE_SLAYER_MALE].SpriteType
		//	|| spriteType==(*g_pCreatureTable)[CREATURETYPE_SLAYER_FEMALE].SpriteType
		//	|| spriteType==(*g_pCreatureTable)[CREATURETYPE_VAMPIRE_MALE].SpriteType
		//	|| spriteType==(*g_pCreatureTable)[CREATURETYPE_VAMPIRE_FEMALE].SpriteType
		//	|| spriteType==(*g_pCreatureTable)[CREATURETYPE_WOLF].SpriteType
		//	|| spriteType==(*g_pCreatureTable)[CREATURETYPE_BAT].SpriteType
		//	|| spriteType==(*g_pCreatureTable)[CREATURETYPE_SLAYER_OPERATOR].SpriteType
		//	|| spriteType==(*g_pCreatureTable)[CREATURETYPE_VAMPIRE_OPERATOR].SpriteType)
		if ((*g_pCreatureSpriteTable)[spriteType].IsPlayerSprite())
		{
			DEBUG_ADD_FORMAT("[ReleaseUselessCreatureSPK] Skip spriteType=%d", spriteType);

			iSpriteType++;
			continue;
		}

		DEBUG_ADD_FORMAT("[ReleaseUselessCreatureSPK] Release spriteType=%d", spriteType);


		if ((*g_pCreatureSpriteTable)[spriteType].bLoad)
		{
			/*
			int first, last;
			// ??? Load?? ???
			// n??° creature?? sprite?? ??? ????

			//--------------------------------------------------------
			// Creature SPK ????
			//--------------------------------------------------------
			first = (*g_pCreatureSpriteTable)[spriteType].FirstSpriteID;
			last = (*g_pCreatureSpriteTable)[spriteType].LastSpriteID;

			if (last > first && first!=SPRITEID_NULL && last!=SPRITEID_NULL)
			{
				m_CreatureSPK.ReleasePart(first, last);
			}

			//--------------------------------------------------------
			// Creature SSPK ????
			//--------------------------------------------------------
			first = (*g_pCreatureSpriteTable)[spriteType].FirstSpriteID;
			last = (*g_pCreatureSpriteTable)[spriteType].LastSpriteID;

			if (last > first && first!=SPRITEID_NULL && last!=SPRITEID_NULL)
			{
				m_CreatureSSPK.ReleasePart( first, last );
			}
			*/
			// loading?? action?? ????
			// 2001.11.21
			int frameID = (*g_pCreatureSpriteTable)[spriteType].FrameID;

			if (frameID != FRAMEID_NULL)
			{
				ReleaseCreatureSPK(frameID);
			}

			// load???? ????? ??????.
			(*g_pCreatureSpriteTable)[spriteType].bLoad = FALSE;
		}

		iSpriteType++;
	}

	//--------------------------------------------------------
	// load?? ?? ??? ????
	//--------------------------------------------------------	
	m_listLoadedCreatureSprite.Release();
	m_listLoadedMonsterSprite.Release();

	INT_ORDERED_LIST::DATA_LIST::const_iterator iUse = listUse.GetIterator();

	for (int i = 0; i < listUse.GetSize(); i++)
	{
		int useSpriteType = *iUse;

		m_listLoadedCreatureSprite.Add(useSpriteType);

		if ((*g_pCreatureSpriteTable)[useSpriteType].IsMonsterSprite())
		{
			m_listLoadedMonsterSprite.Add(useSpriteType);
		}

		iUse++;
	}
}

//----------------------------------------------------------------------
// Load From File  Tile & ImageObject Set
//----------------------------------------------------------------------
// ????? Tile?? ImageObject?? Load???.
//----------------------------------------------------------------------
bool
MTopView::LoadFromFileTileAndImageObjectSet(CSpriteSetManager& TileSSM, CSpriteSetManager& ImageObjectSSM)
{
	//--------------------------------------------------------
	//
	// Tile ??? Load
	//
	//--------------------------------------------------------
//	std::ifstream	TileSPKFile;//(FILE_SPRITE_TILE, ios::binary);	
//	if (!FileOpenBinary(FILE_SPRITE_TILE, TileSPKFile))
//		return false;
//	bool bLoad = m_TileSPK.LoadFromFilePart(TileSPKFile, m_TileSPKI, TileSSM);
//	TileSPKFile.close();
//
//	if (!bLoad)
//		return false;

	bool bLoad = true;

	//bLoad &= m_TileSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SPRITE_TILE").c_str());
	bLoad = m_TileSPK.LoadFromFilePart(TileSSM);

	// 	CSpriteSetManager::DATA_LIST::const_iterator iID = TileSSM.GetIterator();
	// 	int t;
	// 
	// 	for(t = 0; t < TileSSM.GetSize(); ++t, ++iID)
	// 	{
	// 		if(*iID != 0xFFFF && !m_TileSPK.IsLoadedData(*iID))
	// 			m_TileSPK.LoadDataRunning(*iID);
	// 	}

		//--------------------------------------------------------
		//
		// ImageObject ??? Load
		//
		//--------------------------------------------------------
	//	std::ifstream	ImageObjectSPKFile;//(FILE_SPRITE_IMAGEOBJECT, ios::binary);	
	//	if (!FileOpenBinary(FILE_SPRITE_IMAGEOBJECT, ImageObjectSPKFile))
	//		return false;
	//	bLoad = m_ImageObjectSPK.LoadFromFilePart(ImageObjectSPKFile, m_ImageObjectSPKI, ImageObjectSSM);
	//	ImageObjectSPKFile.close();	

	//	bLoad &= m_ImageObjectSPK.LoadFromFileRunning(g_pFileDef->getProperty("FILE_SPRITE_IMAGEOBJECT").c_str());
	bLoad &= m_ImageObjectSPK.LoadFromFilePart(ImageObjectSSM);

	// 	iID = ImageObjectSSM.GetIterator();
	// 
	// 	for(t = 0; t < ImageObjectSSM.GetSize(); ++t, ++iID)
	// 	{
	// 		if(*iID != 0xFFFF)
	// 		{
	// 			if(!m_ImageObjectSPK.IsLoadedData(*iID))
	// 				m_ImageObjectSPK.LoadDataRunning(*iID);
	// 
	// 			if(!m_ImageObjectSSPK.IsLoadedData(*iID))
	// 				m_ImageObjectSSPK.LoadDataRunning(*iID);
	// 		}
	// 	}

	return bLoad;
}




//----------------------------------------------------------------------
// Load From File  Tile & ImageObject Set Thread
//----------------------------------------------------------------------
// ????? Tile?? ImageObject?? Thread Load???.
//----------------------------------------------------------------------
bool
MTopView::LoadFromFileTileAndImageObjectSetThread(CSpriteSetManager& TileSSM, CSpriteSetManager& ImageObjectSSM)
{
	//--------------------------------------------------------
	//
	// Tile ??? Load
	//
	//--------------------------------------------------------
	bool bLoad = true;
	// 
	// 	CSpriteSetManager::DATA_LIST::const_iterator iID = TileSSM.GetIterator();
	// 	int t;
	// 
	// 	for(t = 0; t < TileSSM.GetSize(); ++t, ++iID)
	// 	{
	// 		if(*iID != 0xFFFF && !m_TileSPK.IsLoadedData(*iID))
	// 			m_TileSPK.LoadDataThread(*iID);
	// 	}
	// 
	// 	
	// 	//--------------------------------------------------------
	// 	//
	// 	// ImageObject ??? Load
	// 	//
	// 	//--------------------------------------------------------
	// 	iID = ImageObjectSSM.GetIterator();
	// 
	// 	for(t = 0; t < ImageObjectSSM.GetSize(); ++t, ++iID)
	// 	{
	// 		if(*iID != 0xFFFF)
	// 		{
	// 			if(!m_ImageObjectSPK.IsLoadedData(*iID))
	// 				m_ImageObjectSPK.LoadDataThread(*iID);
	// 
	// 			if(!m_ImageObjectSSPK.IsLoadedData(*iID))
	// 				m_ImageObjectSSPK.LoadDataThread(*iID);
	// 		}
	// 	}

	return bLoad;
}


//----------------------------------------------------------------------
// Load From File Tile LargeZone
//----------------------------------------------------------------------
// File???? LargeZone?????? ????? TileSprite?? Load???.
//----------------------------------------------------------------------
bool
MTopView::ReleaseTileSPKLargeZone(ivfstream& file)
{
	//------------------------------------------------------------
	// File???? TileSprite?? ???? ?????? Load???.
	//------------------------------------------------------------
	CSpriteFilePositionArray* pOldTileSFPA = m_pTileSFPArrayLargeZone;

	m_pTileSFPArrayLargeZone = new CSpriteFilePositionArray;

	m_pTileSFPArrayLargeZone->LoadFromFile(file);

	//------------------------------------------------------------
	// ??? ???°? ????
	//------------------------------------------------------------

	DEBUG_ADD("Remove Old-Tile SFPArray LargeZone");

	COrderedList<TYPE_SPRITEID>	oldTileID;
	COrderedList<TYPE_SPRITEID>	newTileID;

	int i;

	DEBUG_ADD("Sort Part1");

	// Sort
	if (pOldTileSFPA)
	{
		for (i = pOldTileSFPA->GetSize() - 1; i >= 0; i--)
		{
			oldTileID.Add((*pOldTileSFPA)[i].SpriteID);
		}
	}

	DEBUG_ADD("Sort Part2");

	// Sort
	for (i = m_pTileSFPArrayLargeZone->GetSize() - 1; i >= 0; i--)
	{
		newTileID.Add((*m_pTileSFPArrayLargeZone)[i].SpriteID);
	}

	DEBUG_ADD("Subtract");

	// oldTileID???? newTileID ???????.
	oldTileID -= newTileID;

	DEBUG_ADD("Release Part");

	// ?????? zone???? ??????? TileID???? ???????.
	m_TileSPK.ReleasePart(oldTileID);

	DEBUG_ADD("Delete pOldTilSFPA");

	delete pOldTileSFPA;
	pOldTileSFPA = NULL;

	return true;
}

//----------------------------------------------------------------------
// Load From File ImageObject LargeZone
//----------------------------------------------------------------------
// File???? LargeZone?????? ????? ImageObjectSprite???? Load???.
//----------------------------------------------------------------------
bool
MTopView::ReleaseImageObjectSPKLargeZone(ivfstream& file)
{
	//------------------------------------------------------------
	// File???? ImageObjectSprite?? ???? ?????? Load???.
	//------------------------------------------------------------
	CSpriteFilePositionArray* pOldImageObjectSFPA = m_pImageObjectSFPArrayLargeZone;

	m_pImageObjectSFPArrayLargeZone = new CSpriteFilePositionArray;

	m_pImageObjectSFPArrayLargeZone->LoadFromFile(file);

	//------------------------------------------------------------
	// ??? ???°? ????
	//------------------------------------------------------------

	DEBUG_ADD("Remove Old-ImageObject SFPArray LargeZone");

	COrderedList<TYPE_SPRITEID>	oldImageObjectID;
	COrderedList<TYPE_SPRITEID>	newImageObjectID;

	int i;

	// Sort
	if (pOldImageObjectSFPA)
	{
		for (i = pOldImageObjectSFPA->GetSize() - 1; i >= 0; i--)
		{
			oldImageObjectID.Add((*pOldImageObjectSFPA)[i].SpriteID);

			//DEBUG_ADD_FORMAT( "Old: %d", (*pOldImageObjectSFPA)[i].SpriteID );			
		}
	}

	// Sort
	for (i = m_pImageObjectSFPArrayLargeZone->GetSize() - 1; i >= 0; i--)
	{
		newImageObjectID.Add((*m_pImageObjectSFPArrayLargeZone)[i].SpriteID);

		//	DEBUG_ADD_FORMAT( "New: %d", (*m_pImageObjectSFPArrayLargeZone)[i].SpriteID );

	}

	// oldImageObjectID???? newImageObjectID ???????.
	oldImageObjectID -= newImageObjectID;


	// ?????? zone???? ??????? ImageObjectID???? ???????.
	m_ImageObjectSPK.ReleasePart(oldImageObjectID);
	m_ImageObjectSSPK.ReleasePart(oldImageObjectID);

	delete pOldImageObjectSFPA;
	pOldImageObjectSFPA = NULL;

	return true;
}

//----------------------------------------------------------------------
// Load From File Tile SmallZone
//----------------------------------------------------------------------
// File???? SmallZone?????? ????? TileSprite?? Load???.
//----------------------------------------------------------------------
bool
MTopView::ReleaseTileSPKSmallZone(ivfstream& file)
{
	//------------------------------------------------------------
	// File???? TileSprite?? ???? ?????? Load???.
	//------------------------------------------------------------
	CSpriteFilePositionArray* pOldTileSFPA = m_pTileSFPArraySmallZone;

	m_pTileSFPArraySmallZone = new CSpriteFilePositionArray;

	m_pTileSFPArraySmallZone->LoadFromFile(file);

	//------------------------------------------------------------
	// ??? ???°? ????
	//------------------------------------------------------------

	DEBUG_ADD("Remove Old-Tile SFPArray SmallZone");

	COrderedList<TYPE_SPRITEID>	oldTileID;
	COrderedList<TYPE_SPRITEID>	newTileID;

	int i;

	DEBUG_ADD("Sort Part1");

	// Sort
	if (pOldTileSFPA != NULL)
	{
		for (i = pOldTileSFPA->GetSize() - 1; i >= 0; i--)
		{
			oldTileID.Add((*pOldTileSFPA)[i].SpriteID);
		}
	}

	DEBUG_ADD("Sort Part2");

	// Sort
	for (i = m_pTileSFPArraySmallZone->GetSize() - 1; i >= 0; i--)
	{
		newTileID.Add((*m_pTileSFPArraySmallZone)[i].SpriteID);
	}

	DEBUG_ADD("Subtract");

	// oldTileID???? newTileID ???????.
	oldTileID -= newTileID;

	DEBUG_ADD("Release");

	// ?????? zone???? ??????? TileID???? ???????.
	m_TileSPK.ReleasePart(oldTileID);

	DEBUG_ADD("Delete pOldTilSFPA");

	delete pOldTileSFPA;
	pOldTileSFPA = NULL;

	return true;
}

//----------------------------------------------------------------------
// Load From File ImageObject SmallZone
//----------------------------------------------------------------------
// File???? SmallZone?????? ????? ImageObjectSprite???? Load???.
//----------------------------------------------------------------------
bool
MTopView::ReleaseImageObjectSPKSmallZone(ivfstream& file)
{
	//------------------------------------------------------------
	// File???? ImageObjectSprite?? ???? ?????? Load???.
	//------------------------------------------------------------
	CSpriteFilePositionArray* pOldImageObjectSFPA = m_pImageObjectSFPArraySmallZone;

	m_pImageObjectSFPArraySmallZone = new CSpriteFilePositionArray;

	m_pImageObjectSFPArraySmallZone->LoadFromFile(file);

	//------------------------------------------------------------
	// ??? ???°? ????
	//------------------------------------------------------------

	DEBUG_ADD("Remove Old-ImageObject SFPArray SmallZone");

	COrderedList<TYPE_SPRITEID>	oldImageObjectID;
	COrderedList<TYPE_SPRITEID>	newImageObjectID;

	int i;

	// Sort
	if (pOldImageObjectSFPA != NULL)
	{
		for (i = pOldImageObjectSFPA->GetSize() - 1; i >= 0; i--)
		{
			oldImageObjectID.Add((*pOldImageObjectSFPA)[i].SpriteID);
		}
	}

	// Sort
	for (i = m_pImageObjectSFPArraySmallZone->GetSize() - 1; i >= 0; i--)
	{
		newImageObjectID.Add((*m_pImageObjectSFPArraySmallZone)[i].SpriteID);
	}

	// oldImageObjectID???? newImageObjectID ???????.
	oldImageObjectID -= newImageObjectID;

	// ?????? zone???? ??????? ImageObjectID???? ???????.
	m_ImageObjectSPK.ReleasePart(oldImageObjectID);
	m_ImageObjectSSPK.ReleasePart(oldImageObjectID);

	delete pOldImageObjectSFPA;
	pOldImageObjectSFPA = NULL;

	return true;
}

//----------------------------------------------------------------------
// Release TileSPK LargeZone
//----------------------------------------------------------------------
// void	
// MTopView::ReleaseTileSPKLargeZone()
// {
// 	DEBUG_ADD( "Release TileSPK LargeZone" );
// 
// 	// m_pTileSFPArraySmallZone?? m_pTileSFPArrayLargeZone?? sort???
// 	// LargeZone - SmallZone??? ???? ???? ???????.
// 	COrderedList<TYPE_SPRITEID>	SmallZoneTileID;
// 	COrderedList<TYPE_SPRITEID>	LargeZoneTileID;
// 	
// 	int i;
// 	
// 	// Sort
// 	if(m_pTileSFPArraySmallZone)
// 	{
// 		for (i=m_pTileSFPArraySmallZone->GetSize()-1; i>=0; i--)
// 		{
// 			SmallZoneTileID.Add( (*m_pTileSFPArraySmallZone)[i].SpriteID );
// 		}
// 	}
// 	
// 	// Sort
// 	if(m_pTileSFPArrayLargeZone)
// 	{
// 		for (i=m_pTileSFPArrayLargeZone->GetSize()-1; i>=0; i--)
// 		{
// 			LargeZoneTileID.Add( (*m_pTileSFPArrayLargeZone)[i].SpriteID );
// 		}
// 	}
// 	
// 	// LargeZoneTileID???? SmallZoneTileID?? ???????.
// 	LargeZoneTileID -= SmallZoneTileID;
// 	
// 	// LargeZone???? ??????? TileID???? ???????.
// 	m_TileSPK.ReleasePart( LargeZoneTileID );
// 	
// 	if(m_pTileSFPArrayLargeZone)
// 	{
// 		m_pTileSFPArrayLargeZone->Release();
// 	}
// 
// 	DEBUG_ADD( "Release TileSPK LargeZone OK" );
// 
// 
// 	/*
// 	if (m_pTileSFPArrayLargeZone!=NULL)
// 	{		
// 		DEBUG_ADD_FORMAT( "Release TileSPK LargeZone : size=%d", m_pTileSFPArrayLargeZone->GetSize() );
// 		
// 		m_TileSPK.ReleasePart( m_pTileSFPArrayLargeZone );	
// 
// 		DEBUG_ADD( "ReleasePart OK" );
// 		
// 		delete m_pTileSFPArrayLargeZone;		
// 
// 		m_pTileSFPArrayLargeZone = NULL;
// 	}
// 	//*/
// 	//DEBUG_ADD( "Release TileSPK LargeZone OK" );
// }

//----------------------------------------------------------------------
// Release ImageObjectSPK LargeZone
//----------------------------------------------------------------------
// void	
// MTopView::ReleaseImageObjectSPKLargeZone()
// {
// 	DEBUG_ADD( "Release ImageObjectSPK LargeZone" );
// 	
// 	// m_pImageObjectSFPArraySmallZone?? m_pImageObjectSFPArrayLargeZone?? sort???
// 	// SmallZone - LargeZone??? ???? ???? ???????.
// 	COrderedList<TYPE_SPRITEID>	SmallZoneImageObjectID;
// 	COrderedList<TYPE_SPRITEID>	LargeZoneImageObjectID;
// 
// 	int i;
// 
// 	// Sort
// 	if(m_pImageObjectSFPArraySmallZone)
// 	{
// 		for (i=m_pImageObjectSFPArraySmallZone->GetSize()-1; i>=0; i--)
// 		{
// 			SmallZoneImageObjectID.Add( (*m_pImageObjectSFPArraySmallZone)[i].SpriteID );
// 		}
// 	}
// 
// 	// Sort
// 	if(m_pImageObjectSFPArrayLargeZone)
// 	{
// 		for (i=m_pImageObjectSFPArrayLargeZone->GetSize()-1; i>=0; i--)
// 		{
// 			LargeZoneImageObjectID.Add( (*m_pImageObjectSFPArrayLargeZone)[i].SpriteID );
// 		}
// 	}
// 
// 	// SmallZoneImageObjectID???? LargeZoneImageObjectID?? ???????.
// 	LargeZoneImageObjectID -= SmallZoneImageObjectID;
// 	
// 	// SmallZone???? ??????? ImageObjectID???? ???????.
// 	m_ImageObjectSPK.ReleasePart(LargeZoneImageObjectID);
// 
// 	if(m_pImageObjectSFPArrayLargeZone)
// 	{
// 		m_pImageObjectSFPArrayLargeZone->Release();
// 	}
// 	
// 	DEBUG_ADD( "Release ImageObjectSPK LargeZone OK" );
// 
// 	/*
// 	if (m_pImageObjectSFPArrayLargeZone!=NULL)
// 	{
// 		DEBUG_ADD_FORMAT( "ReleaseImageObjectSPKLargeZone size=%d", m_pImageObjectSFPArrayLargeZone->GetSize() );
// 		
// 		m_ImageObjectSPK.ReleasePart( m_pImageObjectSFPArrayLargeZone );
// 
// 		DEBUG_ADD( "ReleasePart OK" );
// 		
// 		delete m_pImageObjectSFPArrayLargeZone;
// 
// 		m_pImageObjectSFPArrayLargeZone = NULL;
// 	}
// 	//*/
// 	//DEBUG_ADD( "ReleaseImageObjectSPKLargeZone OK" );
// }

//----------------------------------------------------------------------
// Release TileSPK SmallZone
//----------------------------------------------------------------------
// SmallZone???? ??????? Tile???? ???????.
//----------------------------------------------------------------------
// void	
// MTopView::ReleaseTileSPKSmallZone()
// {
// 	DEBUG_ADD( "Release TileSPK SmallZone" );
// 	
// 	// m_pTileSFPArraySmallZone?? m_pTileSFPArrayLargeZone?? sort???
// 	// SmallZone - LargeZone??? ???? ???? ???????.
// 	COrderedList<TYPE_SPRITEID>	SmallZoneTileID;
// 	COrderedList<TYPE_SPRITEID>	LargeZoneTileID;
// 
// 	int i;
// 
// 	// Sort
// 	if(m_pTileSFPArraySmallZone)
// 	{
// 		for (i=m_pTileSFPArraySmallZone->GetSize()-1; i>=0; i--)
// 		{
// 			SmallZoneTileID.Add( (*m_pTileSFPArraySmallZone)[i].SpriteID );
// 		}
// 	}
// 
// 	// Sort
// 	if(m_pTileSFPArrayLargeZone)
// 	{
// 		for (i=m_pTileSFPArrayLargeZone->GetSize()-1; i>=0; i--)
// 		{
// 			LargeZoneTileID.Add( (*m_pTileSFPArrayLargeZone)[i].SpriteID );
// 		}
// 	}
// 
// 	// SmallZoneTileID???? LargeZoneTileID?? ???????.
// 	SmallZoneTileID -= LargeZoneTileID;
// 	
// 	// SmallZone???? ??????? TileID???? ???????.
// 	m_TileSPK.ReleasePart( SmallZoneTileID );
// 
// 	if(m_pTileSFPArraySmallZone)
// 	{
// 		m_pTileSFPArraySmallZone->Release();
// 	}
// 	
// 	DEBUG_ADD( "Release TileSPK SmallZone OK" );
// }

//----------------------------------------------------------------------
// Release ImageObjectSPK SmallZone
//----------------------------------------------------------------------
// SmallZone???? ??????? ImageObject???? ???????.
//----------------------------------------------------------------------
// void	
// MTopView::ReleaseImageObjectSPKSmallZone()
// {
// 	DEBUG_ADD( "Release ImageObject SPKSmall Zone" );
// 	
// 	// m_pImageObjectSFPArraySmallZone?? m_pImageObjectSFPArrayLargeZone?? sort???
// 	// SmallZone - LargeZone??? ???? ???? ???????.
// 	COrderedList<TYPE_SPRITEID>	SmallZoneImageObjectID;
// 	COrderedList<TYPE_SPRITEID>	LargeZoneImageObjectID;
// 
// 	int i;
// 
// 	// Sort
// 	if(m_pImageObjectSFPArraySmallZone)
// 	{
// 		for (i=m_pImageObjectSFPArraySmallZone->GetSize()-1; i>=0; i--)
// 		{
// 			SmallZoneImageObjectID.Add( (*m_pImageObjectSFPArraySmallZone)[i].SpriteID );
// 		}
// 	}
// 
// 	// Sort
// 	if(m_pImageObjectSFPArrayLargeZone)
// 	{
// 		for (i=m_pImageObjectSFPArrayLargeZone->GetSize()-1; i>=0; i--)
// 		{
// 			LargeZoneImageObjectID.Add( (*m_pImageObjectSFPArrayLargeZone)[i].SpriteID );
// 		}
// 	}
// 
// 	// SmallZoneImageObjectID???? LargeZoneImageObjectID?? ???????.
// 	SmallZoneImageObjectID -= LargeZoneImageObjectID;
// 	
// 	// SmallZone???? ??????? ImageObjectID???? ???????.
// 	m_ImageObjectSPK.ReleasePart( SmallZoneImageObjectID );
// 
// 	if(m_pImageObjectSFPArraySmallZone)
// 	{
// 		m_pImageObjectSFPArraySmallZone->Release();
// 	}
// 	
// 	DEBUG_ADD( "Release ImageObjectSPK SmallZone OK" );
// }

//----------------------------------------------------------------------
// ?????? Zone?? ???????.
//----------------------------------------------------------------------
void
MTopView::SetZone(MZone* pZone)
{
	m_pZone = pZone;

	// ??? ??? ??????.
	m_bFirstTileDraw = true;

	ClearOutputCreature();

	ClearItemNameList();

	// ??? ????
	//----------------------------------------
	// ?þ? ????..
	//----------------------------------------
//	int x, y;
//	for (y=0; y<m_pZone->GetHeight(); y++)
//	{
//		for (x=0; x<m_pZone->GetWidth(); x++)
//		{			
//			m_PreviousFogSpriteID[y][x] = SPRITEID_NULL;
//		}
//	}
//	m_bFogChanged = true;
}

//----------------------------------------------------------------------
// Map to Pixel  :  (Xm, Ym) ---> (Xp, Yp)
//----------------------------------------------------------------------
// inline

//----------------------------------------------------------------------
// Pixel to Map  :  (Xp, Yp) ---> (Xm, Ym)
//----------------------------------------------------------------------
// inline


//----------------------------------------------------------------------
// Get Screen Point
//----------------------------------------------------------------------
// ??? ???? ??? (x,y)?? Zone?????? pixel?????? ??? ??????
//----------------------------------------------------------------------
POINT
MTopView::ScreenToPixel(int x, int y)
{
	// ???? ????? (x,y)?? ????? ?????.
	POINT zonePixel;	// ?????? ó?? Sector?? ??Ÿ???? Zone?? pixel???

	// ????? ù Sector?? ??Ÿ???? Map?????? Pixel????? ?????
	// ?????? (x,y)?? ??????? 
	// ??? ???? ????? ??Ÿ???? Map?????? Pixel????? ????? ?????

	//zonePixel = MapToPixel(m_FirstSector.x, m_FirstSector.y);
	//zonePixel.x += m_PlusPoint.x  + x;
	//zonePixel.y += m_PlusPoint.y  + y;

	zonePixel.x = m_FirstZonePixel.x + x;
	zonePixel.y = m_FirstZonePixel.y + y;

	return zonePixel;
}

//----------------------------------------------------------------------
// Pixel To Screen
//----------------------------------------------------------------------
// Zone?? pixel????? ????? ????? ????.
//----------------------------------------------------------------------
POINT
MTopView::PixelToScreen(int x, int y)
{
	POINT screenPixel;

	// zone?? pixel??????? 
	// ????? ù???? ??????? zone?? ????? ???? ???.
	screenPixel.x = x - m_FirstZonePixel.x;
	screenPixel.y = y - m_FirstZonePixel.y;

	return screenPixel;
}

//----------------------------------------------------------------------
// Map To Screen
//----------------------------------------------------------------------
// Map?? sector????? ????? ????? ????.
//----------------------------------------------------------------------
POINT
MTopView::MapToScreen(int sX, int sY)
{
	POINT screenPixel;

	// Map?? Sector????? Pixel????? ????.
	screenPixel = MapToPixel(sX, sY);

	// Pixel????? Screen????? ????.
	screenPixel.x -= m_FirstZonePixel.x;
	screenPixel.y -= m_FirstZonePixel.y;

	return screenPixel;
}

//----------------------------------------------------------------------
// ??? ??? (x,y)?? ??????? ????? 
// Zone?????? ??(sX,sY) Sector????
//----------------------------------------------------------------------
POINT
MTopView::GetSelectedSector(int x, int y)
{
	POINT point;

	//-------------------------------------------------
	// ???? ????
	//-------------------------------------------------
	// ??? ???? ù Sector : m_FirstSector
	// ?????????          : m_PlusPoint
	//-------------------------------------------------

	//-------------------------------------------------
	// ???
	//-------------------------------------------------
	// 1. ????? (0,0)?? Zone???? ??Ÿ???? 
	//    pixel????????? ??????.(pX,pY) - MapToPixel
	//
	// 2. (pX,pY) + (x,y)?? ?????.
	//
	// 3. (pX,x, pY,y)?? Zone???? ??Ÿ????
	//    Sector????? ??????.           - PixelToMap
	//-------------------------------------------------

	point = MapToPixel(m_FirstSector.x, m_FirstSector.y);
	point.x += m_PlusPoint.x;
	point.y += m_PlusPoint.y;

	point.x += x;
	point.y += y;

	point = PixelToMap(point.x, point.y);

	if (g_pZone->IsPositionInZone(point.x, point.y))
	{
		return point;
	}

	point.x = SECTORPOSITION_NULL;
	point.y = SECTORPOSITION_NULL;

	return point;
}

//----------------------------------------------------------------------
// Set Direction To Creature
//----------------------------------------------------------------------
// ??? Creature?? ????? ?????.
//----------------------------------------------------------------------
BYTE
MTopView::GetDirectionToPosition(int originX, int originY, int destX, int destY)
{
	int	stepX = destX - originX,
		stepY = destY - originY;

	// 0?? ?? check
	float	k = (stepX == 0) ? 0 : (float)(stepY) / stepX;	// ????


	//--------------------------------------------------
	// ?????? ????? ???.	
	//--------------------------------------------------
	if (stepY == 0)
	{
		// X??
		// - -;;
		if (stepX == 0)
			return DIRECTION_DOWN;
		else if (stepX > 0)
			return DIRECTION_RIGHT;
		else
			return DIRECTION_LEFT;
	}
	else
		if (stepY < 0)	// UP??????
		{
			// y?? ??
			if (stepX == 0)
			{
				return DIRECTION_UP;
			}
			// 1????
			else if (stepX > 0)
			{
				if (k < -BASIS_DIRECTION_HIGH)
					return DIRECTION_UP;
				else if (k <= -BASIS_DIRECTION_LOW)
					return DIRECTION_RIGHTUP;
				else
					return DIRECTION_RIGHT;
			}
			// 2????
			else
			{
				if (k > BASIS_DIRECTION_HIGH)
					return DIRECTION_UP;
				else if (k >= BASIS_DIRECTION_LOW)
					return DIRECTION_LEFTUP;
				else
					return DIRECTION_LEFT;
			}
		}
	// ?????
		else
		{
			// y?? ???
			if (stepX == 0)
			{
				return DIRECTION_DOWN;
			}
			// 4????
			else if (stepX > 0)
			{
				if (k > BASIS_DIRECTION_HIGH)
					return DIRECTION_DOWN;
				else if (k >= BASIS_DIRECTION_LOW)
					return DIRECTION_RIGHTDOWN;
				else
					return DIRECTION_RIGHT;
			}
			// 3????
			else
			{
				if (k < -BASIS_DIRECTION_HIGH)
					return DIRECTION_DOWN;
				else if (k <= -BASIS_DIRECTION_LOW)
					return DIRECTION_LEFTDOWN;
				else
					return DIRECTION_LEFT;
			}
		}
}

//----------------------------------------------------------------------
// Set Fade Start
//----------------------------------------------------------------------
// start???? end???? step??..
// (r,g,b)????? Fade in ?????.
// 0(????)~31(????)
//----------------------------------------------------------------------
void
MTopView::SetFadeStart(char start, char end, char step, BYTE r, BYTE g, BYTE b, WORD delay, BYTE delayPostion)
{
	step = abs(step);

	m_FadeValue = start;
	m_FadeEnd = end;
	m_FadeInc = (start < end) ? step : -step;
	m_bFade = true;
	m_delayFrame = delay;
	m_delayPostion = delayPostion;

	if (CDirect3D::IsHAL())
	{
		// 3D ?????? ??? 4:4:4???.
		m_FadeColor = (((WORD)r >> 1) << 8)
			| (((WORD)g >> 1) << 4)
			| ((WORD)b >> 1);
	}
	else
	{
		m_FadeColor = CDirectDraw::Color(r, g, b);
	}
}


//----------------------------------------------------------------------
// Draw Fade
//----------------------------------------------------------------------
// Fade In/Out ó??
//----------------------------------------------------------------------	
void
MTopView::DrawFade()
{
	if (g_pEventManager->GetEventByFlag(EVENTFLAG_NOT_FADE_SCREEN))
		return;

	//--------------------------------------------------------
	// Fade In/Out ????? ????? ???..
	//--------------------------------------------------------
	if (m_bFade)// || bEvent)
	{
		static DWORD TempFadeFrame = g_CurrentFrame;
		//		if(bEvent)
		//		{
		//			const MEvent *event = g_pEventManager->GetEventByFlag(EVENTFLAG_FADE_SCREEN);
		//
		//			m_FadeValue = event->parameter1 >> 24;
		//			
		//			m_bFade = true;
		//			m_FadeInc = 1;
		//			m_FadeEnd = 31;
		//
		//			int r = (event->parameter1 >> 16)&0xff, g = (event->parameter1 >> 8)&0xff, b = (event->parameter1)&0xff;
		//			
		//			if (CDirect3D::IsHAL())
		//			{
		//				// 3D ?????? ??? 4:4:4???.
		//				m_FadeColor = (((WORD)r>>1) << 8)
		//					| (((WORD)g>>1) << 4)
		//					| ((WORD)b>>1);
		//			}
		//			else
		//			{
		//				m_FadeColor	= CDirectDraw::Color(r,g,b);
		//			}
		//		}

		RECT rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = SURFACE_WIDTH;
		rect.bottom = SURFACE_HEIGHT;

		//		//--------------------------------------------------------
		//		// 3D ?????? ???
		//		//--------------------------------------------------------
		//		if (CDirect3D::IsHAL())
		//		{
		//			WORD alpha = (32-m_FadeValue)>>1;			
		//			WORD pixel = alpha << 12;
		//
		//			// m_FadeColor?? ????? ???´?.
		//			pixel |= m_FadeColor;
		//
		//			DrawBox3D(&rect, pixel);
		//		}
		//		//--------------------------------------------------------
		//		// 2D
		//		//--------------------------------------------------------
		//		else
		{
			m_pSurface->Lock();

			//-------------------------------------------------
			// ?????????.. ???? ???~
			//-------------------------------------------------
			if (m_FadeColor == 0)
			{
				// 2D 5:6:5
				if (CDirectDraw::Is565())
				{
					m_pSurface->GammaBox565(&rect, m_FadeValue);
				}
				// 2D 5:5:5
				else
				{
					m_pSurface->GammaBox555(&rect, m_FadeValue);
				}
			}
			//-------------------------------------------------
			// ????...
			//-------------------------------------------------
			else
			{
				m_pSurface->BltColorAlpha(&rect, m_FadeColor, m_FadeValue);
			}

			m_pSurface->Unlock();
		}

		//------------------------------------------------
		// ???? fade??
		//------------------------------------------------
		// 2004, 6, 21, sobeit add start - ???? ???? ???? ???..
		if (m_delayFrame)
		{
#if __CONTENTS(__HIDDEN_BOSS_SYSTEM)
			if (m_delayPostion & FADE_DELAY_POSTION_FRONT)
			{
				if (g_CurrentFrame - TempFadeFrame >= m_delayFrame)
				{
					if (m_FadeEnd == 0 && 1 == m_FadeValue) // ???? ?????? ???? ???? ??? ????..^^;
					{
						if (g_CurrentFrame - TempFadeFrame > 16 * m_delayFrame) // ??°???? ?????? 5??? ????
							m_bFade = false;
					}
					else
					{
						TempFadeFrame = g_CurrentFrame;
						m_FadeValue += m_FadeInc;
					}
				}
			}
			else
				if (m_delayPostion & FADE_DELAY_POSTION_BACK)
#endif //__HIDDEN_BOSS_SYSTEM
				{
					if (g_CurrentFrame - TempFadeFrame >= m_delayFrame)
					{
						if (m_FadeEnd == -1 && 1 == m_FadeValue) // ???? ?????? ???? ???? ??? ????..^^;
						{
							if (g_CurrentFrame - TempFadeFrame > 16 * m_delayFrame) // ??°???? ?????? 5??? ????
								m_bFade = false;
						}
						else
						{
							TempFadeFrame = g_CurrentFrame;
							m_FadeValue += m_FadeInc;
						}
					}
				}
		}
		else
		{
			// 2004, 6, 21, sobeit add end - ???? ???? ???? ???..
			m_FadeValue += m_FadeInc;
		}



		//------------------------------------------------
		// ??????
		//------------------------------------------------
		// ??????? ??? ???
		if (m_FadeInc > 0)
		{
			if (m_FadeValue > m_FadeEnd || m_FadeValue > 31)
			{
				m_bFade = false;
			}
		}
		else
		{
			if (m_FadeValue < m_FadeEnd || m_FadeValue < 1)
			{
				m_bFade = false;
			}
		}
	}
}
//----------------------------------------------------------------------
// ??? ??? (x,y)?? ??????? ????? ??????? 
// Zone?? ??(sX,sY) Object?? ???õ???
//----------------------------------------------------------------------
MObject*
MTopView::GetSelectedObject(int x, int y, BYTE_ATTRIBUTE actionTarget)
{
	//--------------------------------------------------------------	
	// 1. ?????? (x,y)?? Zone?????? Sector(sX,sY)????? ?????.
	// 
	// 2. (sX,sY) ??ó?? Object?? ?????? 
	//    ?? Object?? ??? ???? ????? ?????
	//    (x,y)?? ????? "???????"
	//    ????, ?? ?????? ??ó Sector?? check????? ???.
	//
	// (!) Player?? ?????. ????? ??????? check???.
	//--------------------------------------------------------------

	//--------------------------------------------------------------
	// ??ó Sector?? ã?? ????
	//--------------------------------------------------------------	
	// - ????? ????? Sector?? ??? ??????? ??? ??µ???
	//   ???õ? ???????? ????.
	// 
	//      .  .  .  
	//      .  X  . 
	//      .  .  .  
	//      .  .  .  
	//      .  .  .     X?? ???õ??? ???, Ground,Item ??... check????
	//
	//
	//               
	//         X    
	//               
	//      .  .  .  
	//      .  .  .  
	//      .  .  .  
	//      .  .  .     Flying?? 64 pixel(?? Ÿ??) ???? ?? ???..
	//      .  .  .     X?? ???õ??? ???, Flying... check????
	//
	//--------------------------------------------------------------

	//--------------------------------------------------------------
	// ?? Sector?? ????? check????? ??? ??
	//--------------------------------------------------------------
	/*

	   while ( ExistNextSector )
	   {
			if ( IsExistObject )
			{
				if ( IsObjectCreature && ClickPoint In TheCreatureRect)
					Select Creature

				else if ( IsObjectItem && ClickPoint In TheItemRect )
					Select Item
			}

			CurrentSector = NextSector
		}

		if ( Not Selected )
			return NULL;

		return Object;
	*/
	//--------------------------------------------------------------

	//--------------------------------------------------------------
	// ???? ???? ????? ?? ????? ??????..
	// Player?? ?????? ??? ????..
	// ??? return
	//--------------------------------------------------------------
//	bool bSlayerPlayer = g_pPlayer->IsSlayer();

	if ((m_bFirstTileDraw
		|| !g_pPlayer->IsVampire() && g_pPlayer->IsInDarkness() && (!(g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || g_pZone->GetID() == 3001)
		&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
		&& !g_bLight
#endif
		)
		)
	{
		return NULL;
	}


	POINT firstZonePixel;	// ?????? ó?? Sector?? ??Ÿ???? Zone?? pixel???
	firstZonePixel = MapToPixel(m_FirstSector.x, m_FirstSector.y);
	firstZonePixel.x += m_PlusPoint.x;
	firstZonePixel.y += m_PlusPoint.y;


	POINT	pixelPoint;			// Pixel Position In Zone
	POINT	sectorPoint;		// Sector Position In Zone

	//--------------------------------------------------------------
	// (x,y)?? Zone?????? pixel????? ?????.
	//--------------------------------------------------------------
	pixelPoint = MapToPixel(m_FirstSector.x, m_FirstSector.y);
	pixelPoint.x += m_PlusPoint.x + x;
	pixelPoint.y += m_PlusPoint.y + y;

	//--------------------------------------------------------------
	// (x,y)?? Zone?????? sector????? ?????.
	//--------------------------------------------------------------						
	sectorPoint = PixelToMap(pixelPoint.x, pixelPoint.y);

	g_MouseSector = sectorPoint;

	//--------------------------------------------------------------
	// ??? ü?
	//--------------------------------------------------------------
	// ???? ??°? ????????.. -_-;;
	// ó???? ????? ?????.. ??÷?... ????... ??.??;;
	//--------------------------------------------------------------
	g_bMouseInPortal = false;

	if (sectorPoint.x >= 0
		&& sectorPoint.y >= 0
		&& sectorPoint.x < g_pZone->GetWidth()
		&& sectorPoint.y < g_pZone->GetHeight())
	{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
		DEBUG_ADD("ptChk");
#endif

		const MSector& sector = m_pZone->GetSector(sectorPoint.x, sectorPoint.y);

		// Dracula Castle: while the Mihnea door (EST 2282) stands on the 2F stairs, that portal is closed
		bool bDoorNear = m_pZone->IsGroundEffectNear(sectorPoint.x, sectorPoint.y,
			(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_TEMP2282].FrameID, 6);
		if (sector.IsPortal() && !bDoorNear)
		{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
			DEBUG_ADD("hasPt");
#endif

			int numPortal = sector.GetPortalSize();


			/*
			for (int i=0; i<numPortal; i++)
			{
				// ??? ?????? zone?? ID
				PORTAL_INFO portalInfo = *iPortal;

				//-------------------------------------------------------
				// ??? ???
				//-------------------------------------------------------
				DEBUG_ADD_FORMAT( "[Portal] Type=%d, ZoneID=%d", portalInfo.Type, portalInfo.ZoneID );

				iPortal++;
			}
			*/

			if (numPortal != 0)
			{
				PORTAL_LIST::const_iterator	iPortal = sector.GetPortalBegin();

				PORTAL_INFO portalInfo = *iPortal;

				bool canMove = false;
				bool bSiegePotal = false;
				//-------------------------------------------------------
				// portal?? ?????? ???? ???? ??°????? ü?
				//-------------------------------------------------------			
				switch (portalInfo.Type)
				{
				case MPortal::TYPE_BATTLE_PORTAL:
				case MPortal::TYPE_NOMAL:
				case MPortal::TYPE_CLIENT_ONLY:
					canMove = true;
					break;

				case MPortal::TYPE_SLAYER_ONLY:
					canMove = g_pPlayer->IsSlayer();
					break;

				case MPortal::TYPE_VAMPIRE_ONLY:
					canMove = g_pPlayer->IsVampire();
					break;

				case MPortal::TYPE_OUSTERS_ONLY:
					canMove = g_pPlayer->IsOusters();
					break;

				case MPortal::TYPE_MULTI_PORTAL:
					canMove = g_pPlayer->IsSlayer();
					break;

				case MPortal::TYPE_GUILD_PORTAL:
					canMove = (g_pUserInformation->GuildName.GetLength() > 0);
					break;
				case MPortal::TYPE_CLIENT_SIEGE_POTAL_1:
				case MPortal::TYPE_CLIENT_SIEGE_POTAL_2:
				case MPortal::TYPE_CLIENT_SIEGE_POTAL_3:
				case MPortal::TYPE_CLIENT_SIEGE_POTAL_4:
					canMove = true;
					bSiegePotal = true;
					break;

				}

				//-------------------------------------------------------			
				// ?? ?? ?????? ??? ??? ???
				//-------------------------------------------------------			
				if (canMove)
				{
					ZONETABLE_INFO* pZoneInfo = g_pZoneTable->Get(portalInfo.ZoneID);

					if (pZoneInfo != NULL)
					{
						//char pZoneName[80];
						int zoneID = portalInfo.ZoneID;

						if (bSiegePotal)
						{
							if (g_pSystemAvailableManager->ZoneFiltering(zoneID) && zoneID == 1500)
								gpC_mouse_pointer->SetCursorPortal(zoneID, portalInfo.Type);
						}
						else
						{
							if (numPortal <= 1)
							{
								//strcpy(pZoneName, pZoneInfo->Name.GetString());
							}
							else
							{
								// ??? ??????~~
								zoneID = 60001;
								//strcpy(pZoneName, "??????????");
							}

							//if (!g_bMouseInPortal)
							//{
							//	gpC_mouse_pointer->SetCursorPortal( pZoneName );
							//}

							if (// (!g_pUserInformation->IsNetmarble || g_mapPremiumZone.find(zoneID) == g_mapPremiumZone.end())
								/*&&*/ g_pSystemAvailableManager->ZoneFiltering(zoneID))
								gpC_mouse_pointer->SetCursorPortal(zoneID);
						}
					}
				}
				else
				{

					if (!g_bMouseInPortal)
					{
						//char pZoneName[80];
						//strcpy(pZoneName, "?? ?? ???? ??");
						//gpC_mouse_pointer->SetCursorPortal( -1 );
					}
				}
			}

			g_bMouseInPortal = true;

#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
			DEBUG_ADD("ptK");
#endif
		}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
		DEBUG_ADD("ptK2");
#endif
	}


	//--------------------------------------------------------------
	//
	// check
	//
	//--------------------------------------------------------------

	MObject* pObject = NULL;
	MCreature* pCreature = NULL;
	MItem* pItem = NULL;
	CFrame			frame;
	//	TYPE_SPRITEID	sprite;

		// Object?? ????
	//	POINT	objectPixelPoint;
		//RECT	rect;

		// Player Creature's ID
	TYPE_OBJECTID	pid = g_pPlayer->GetID();

	//std::ofstream file("log.txt");


	// ???? check??? ???
	int currentX;
	int currentY;

	int numCreature = 0;

	//--------------------------------------------------------------
	//
	// Flying Creature Check 
	//
	// : ???? ????? ??? ??? 
	//   ??????? Object???? ????? Sector?? ?????? ?? ???.
	//
	//--------------------------------------------------------------
	int	sX1 = sectorPoint.x - 3,		// ?? ??? ?? ?????????. ?????~
		sX2 = sectorPoint.x + 3;

	// ???? ???? ??????.. +?? ?????.
	int	sY1 = sectorPoint.y + 4,
		sY2 = sectorPoint.y + 8;

	//------------------------------------------------------
	// Zone?? ?????? ??? ??? Skip...
	//------------------------------------------------------
	if (sX1 < 0)
	{
		sX1 = 0;
	}

	if (sX2 >= m_pZone->GetWidth())
	{
		sX2 = m_pZone->GetWidth() - 1;
	}

	if (sY1 < 0)
	{
		sY1 = 0;
	}

	if (sY2 >= m_pZone->GetHeight())
	{
		sY2 = m_pZone->GetHeight() - 1;
	}


	//------------------------------------------------------	
	// ???????? Sector?? check???.
	//------------------------------------------------------	
	for (currentY = sY2; currentY >= sY1; currentY--)
	{
		for (currentX = sX1; currentX <= sX2; currentX++)
		{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
			DEBUG_ADD_FORMAT("ckF1(%d,%d)", currentX, currentY);
#endif

			const MSector& sector = m_pZone->GetSector(currentX, currentY);

			// ?? Object?? ???? ???
			if (sector.IsExistObject())
			{

				//file << "Exist Object!" << endl;

				// Object?? ?????? ???? ??? check???.
				// ?????? ?? ??? ???? Creature?? Item?????.

				//------------------------------------------------
				//
				// Flying Creature?? ??? ???
				//
				//------------------------------------------------
				numCreature = sector.GetFlyingCreatureSize();

				if (numCreature > 0)
				{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
					DEBUG_ADD("hasFC");
#endif

					OBJECT_MAP::const_iterator iCreature = sector.GetFlyingCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_FLYINGCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{
							// FlyingCreature?? ??????? ?????? ?????.
							// FRAME_PACK???? ?????? ???? ?? ??!!
							// FlyingCreature?? Sector????? Pixel????? ???? ???.

							// FlyingCreature?? Zone?? ????? ??????.
							/*
							objectPixelPoint = MapToPixel(pCreature->GetX(), pCreature->GetY());
							objectPixelPoint.x += m_PlusPoint.x + pCreature->GetSX();
							objectPixelPoint.y += m_PlusPoint.y + pCreature->GetSY();

							// Zone?? ????? ????? ????? ????.
							objectPixelPoint.x -= firstZonePixel.x;
							objectPixelPoint.y -= firstZonePixel.y;

							// Creature?? ??? ??????? ?????? ????????.
							frame = m_CreatureFPK[(*g_pCreatureTable)[pCreature->GetCreatureType()].FrameID][pCreature->GetAction()][pCreature->GetDirection()][pCreature->GetFrame()];

							// ???? Sprite?? ??? ??µ?? ???
							objectPixelPoint.x += frame.GetCX();
							objectPixelPoint.y += frame.GetCY() - FLYINGCREATURE_HEIGHT;
												// FramePack???? ????? ????????,
												// Flying?? ??? ?????.

							// Creature?? ??? ??????? ?????? ????????.
							rect.left	= objectPixelPoint.x;
							rect.top	= objectPixelPoint.y;
							rect.right	= rect.left + m_CreatureSPK[ frame.GetSpriteID() ].GetWidth();
							rect.bottom	= rect.top + m_CreatureSPK[ frame.GetSpriteID() ].GetHeight();
							*/

							// ???????? ?????? 
							// ?????? ????? Creature?? ?????? ?????
							// ???? mode?? ???? ????????? ?????? ????
							// ???? ?????.
							if (
								(g_pPlayer->IsVampire() && g_pZone->GetID() != 3001 ||
									!g_pPlayer->IsVampire() && !(!pCreature->IsNPC() && pCreature->IsInDarkness()) ||
									!g_pPlayer->IsVampire() && (g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))
									|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
									|| g_bLight
#endif
									)
								&& !pCreature->IsCutHeight()	// ??????? ???¸?
								&& g_pObjectSelector->CanSelect(pCreature)
								&& pCreature->IsPointInScreenRect(x, y)
								&& !(pCreature->IsOusters() && pCreature->IsInGroundElemental() && !g_pPlayer->IsOusters()))
								//if (x > rect.left && x < rect.right
									//&& y > rect.top && y < rect.bottom)
							{
								return pCreature;
							}
						}
					}
				}
			}
		}
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
	DEBUG_ADD("chk2");
#endif

	//--------------------------------------------------------------
	//
	// Creature / Item Check data
	//
	//--------------------------------------------------------------
	sY1 = sectorPoint.y - 3,
		//sY2 = sectorPoint.y + 4;		// ?? ??? ?? ?????????.
		sY2 = sectorPoint.y + 8;		// ??? ???? ?????? ?? ?????????.

	//------------------------------------------------------
	// Zone?? ?????? ??? ??? Skip...
	//------------------------------------------------------	
	if (sY1 < 0)
	{
		sY1 = 0;
	}

	if (sY2 >= m_pZone->GetHeight())
	{
		sY2 = m_pZone->GetHeight() - 1;
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
	DEBUG_ADD("ckIO9");
#endif

	//------------------------------------------------------
	// Check????? ??? ImageObject Iterator
	//------------------------------------------------------
	IMAGEOBJECT_OUTPUT_MAP::reverse_iterator iImageObjectCheck
		= m_mapImageObject.rbegin();

	//------------------------------------------------------
	// ??? ?????? sY1???? ???? ????? 
	// ImageObject???? Check???.
	// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (iImageObjectCheck != m_mapImageObject.rend())
	{
		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectCheck).second);

		// InteractionObject?? ??? check???.
//		if (pImageObject->GetObjectType()==MObject::TYPE_INTERACTIONOBJECT)
//		{
//			MInteractionObject* const pInteractionObject = (MInteractionObject* const)pImageObject;
//
//			// ??? ?????? sY1???? ???? ???..?? Check
//			if (pInteractionObject->GetViewpoint() > sY2)
//			{
//				/*
//				// ????? ????? ???? ????? ????? ?????								
//				objectPixelPoint.x = pInteractionObject->GetPixelX() - m_FirstZonePixel.x;
//				objectPixelPoint.y = pInteractionObject->GetPixelY() - m_FirstZonePixel.y;
//
//				// frame?? ????? sprite?? ?????.
//				frame = m_ImageObjectFPK[ pInteractionObject->GetFrameID() ][ pInteractionObject->GetFrame() ];
//				sprite = frame.GetSpriteID();
//
//				// ???? ???? ???? ????..
//				if (m_ImageObjectSPK[ sprite ].IsNotInit())
//				{
//					iImageObjectCheck ++;
//					continue;
//				}
//						
//				// ??? ????
//				objectPixelPoint.x += frame.GetCX();
//				objectPixelPoint.y += frame.GetCY();
//
//				// ??? ??????? ?????? ????????.
//				rect.left	= objectPixelPoint.x;
//				rect.top	= objectPixelPoint.y;
//				rect.right	= rect.left + m_ImageObjectSPK[ sprite ].GetWidth();
//				rect.bottom	= rect.top + m_ImageObjectSPK[ sprite ].GetHeight();
//				*/		
//				// ?????? ????? ImageObject?? ?????? ?????
//				if (pInteractionObject->IsPointInScreenRect(x,y))
//				//if (x > rect.left && x < rect.right
//				//	&& y > rect.top && y < rect.bottom)
//				{
//					#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
//						DEBUG_ADD("reIO9");
//					#endif
//
//					//pInteractionObject->SetAction( 1 );
//					return pInteractionObject;
//				}
//			}
//			// ????.. Object check??? Check????? ??????.
//			else
//			{
//				break;
//			}
//		}

		iImageObjectCheck++;
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
	DEBUG_ADD("ckSe");
#endif



	//NPC ???? ?????? ???? ??? NPC?? ??? ???? ???? ????? ?????? ???????? ??? ?? ?? ????? ?? ?????? NPC???? ?????.	
	for (currentY = sY2; currentY >= sY1; currentY--)
	{
		for (currentX = sX1; currentX <= sX2; currentX++)
		{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
			DEBUG_ADD_FORMAT("gS(%d,%d)", currentX, currentY);
#endif

			const MSector& sector = m_pZone->GetSector(currentX, currentY);

			// ?? Object?? ???? ???
			if (sector.IsExistObject())
			{
				//------------------------------------------------
				//
				// Creature?? ??? ???
				//
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
				DEBUG_ADD("ckGc");
#endif

				numCreature = sector.GetGroundCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetGroundCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_GROUNDCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{
							BOOL bMouseInCreature = false;

							// Creature Table?? Width ?????? ????
							if (pCreature->GetWidth() != -1)
							{
								// Width?? ???? ??? ?????? ??ü????.
								RECT	rcScreenRect = pCreature->GetScreenRect();
								POINT	ptMouse = { x, y };

								int		nWidth = pCreature->GetWidth();
								int		nCenterY = rcScreenRect.left + (rcScreenRect.right - rcScreenRect.left) / 2;

								// ??????????? ??? ???? ????
								rcScreenRect.left = nCenterY - (nWidth >> 1);
								rcScreenRect.right = nCenterY + (nWidth >> 1);

								// ??ü????.
								// A creature whose body sprite never draws -- the Hell Garden crystals
								// are rendered entirely by the effect system -- leaves m_ScreenRect in
								// its reset, inverted state (left/top = screen size, right/bottom = 0).
								// Overriding left/right above cannot help because top/bottom are still
								// inverted, so PtInRect always fails and the click falls through to the
								// ground. Fall back to a tile hit test for that case.
								if (rcScreenRect.bottom <= rcScreenRect.top)
								{
									// The sprite contributed no bounds, so build a hit box from the
									// creature tile position plus the Width/Height in the creature table.
									// This inverts the conversion GetSelectedSector() performs.
									POINT ptOrigin = MapToPixel(m_FirstSector.x, m_FirstSector.y);
									POINT ptBase   = MapToPixel(pCreature->GetX(), pCreature->GetY());
								
									int sx = ptBase.x - ptOrigin.x - m_PlusPoint.x;
									int sy = ptBase.y - ptOrigin.y - m_PlusPoint.y;
									int nHeight = pCreature->GetHeight();
								
									rcScreenRect.left   = sx - (nWidth >> 1);
									rcScreenRect.right  = sx + (nWidth >> 1);
									rcScreenRect.bottom = sy + 24;
									rcScreenRect.top    = sy - nHeight;
								
									// accept the sprite area, and still accept the ground tile itself
									POINT ptTile = GetSelectedSector(x, y);
									bMouseInCreature = PtInRect(&rcScreenRect, ptMouse)
										|| (ptTile.x == pCreature->GetX() && ptTile.y == pCreature->GetY());
								}
								else
								{
									bMouseInCreature = PtInRect(&rcScreenRect, ptMouse);
								}
							}
							else
							{
								// ?????? ??????? ??ü?
								bMouseInCreature = pCreature->IsPointInScreenRect(x, y);
							}


							// ?????? ????? Creature?? ?????? ?????
							if ((g_pPlayer->IsVampire() && g_pZone->GetID() != 3001 ||
								!g_pPlayer->IsVampire() && !(!pCreature->IsNPC() && pCreature->IsInDarkness()) ||
								!g_pPlayer->IsVampire() && (g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))
								|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
								|| g_bLight
#endif
								)
								&& !pCreature->IsCutHeight()	// ??????? ???¸?
								&& g_pObjectSelector->CanSelect(pCreature)
								&& bMouseInCreature
								&& !(pCreature->IsOusters() && pCreature->IsInGroundElemental() && !g_pPlayer->IsOusters()))
								//if (x > rect.left && x < rect.right
								//	&& y > rect.top && y < rect.bottom)
							{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
								DEBUG_ADD("reGc");
#endif

								if (pCreature->IsNPC())	//????? ???? NPC?? ?????? NPC ?????? ??????.
									return pCreature;
							}
						}
					}
				}
			}
		}
	}


	//------------------------------------------------------	
	// ???????? Sector?? check???.
	//------------------------------------------------------	
	for (currentY = sY2; currentY >= sY1; currentY--)
	{
		for (currentX = sX1; currentX <= sX2; currentX++)
		{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
			DEBUG_ADD_FORMAT("gS(%d,%d)", currentX, currentY);
#endif

			const MSector& sector = m_pZone->GetSector(currentX, currentY);

			// ?? Object?? ???? ???
			if (sector.IsExistObject())
			{

				//file << "Exist Object!" << endl;

				// Object?? ?????? ???? ??? check???.
				// ?????? ?? ??? ???? Creature?? Item?????.

				//------------------------------------------------
				//
				// Flying Creature?? ??? ???
				//
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
				DEBUG_ADD("ckFc");
#endif

				numCreature = sector.GetFlyingCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetFlyingCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_FLYINGCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{
							/*
							// FlyingCreature?? ??????? ?????? ?????.
							// FRAME_PACK???? ?????? ???? ?? ??!!
							// FlyingCreature?? Sector????? Pixel????? ???? ???.

							// FlyingCreature?? Zone?? ????? ??????.
							objectPixelPoint = MapToPixel(pCreature->GetX(), pCreature->GetY());
							objectPixelPoint.x += m_PlusPoint.x + pCreature->GetSX();
							objectPixelPoint.y += m_PlusPoint.y + pCreature->GetSY();

							// Zone?? ????? ????? ????? ????.
							objectPixelPoint.x -= firstZonePixel.x;
							objectPixelPoint.y -= firstZonePixel.y;

							// Creature?? ??? ??????? ?????? ????????.
							frame = m_CreatureFPK[(*g_pCreatureTable)[pCreature->GetCreatureType()].FrameID][pCreature->GetAction()][pCreature->GetDirection()][pCreature->GetFrame()];

							// ???? Sprite?? ??? ??µ?? ???
							objectPixelPoint.x += frame.GetCX();
							objectPixelPoint.y += frame.GetCY() - FLYINGCREATURE_HEIGHT;
												// FramePack???? ????? ????????,
												// Flying?? ??? ?????.

							// ??? ??????? ?????? ????????.
							rect.left	= objectPixelPoint.x;
							rect.top	= objectPixelPoint.y;
							rect.right	= rect.left + m_CreatureSPK[ frame.GetSpriteID() ].GetWidth();
							rect.bottom	= rect.top + m_CreatureSPK[ frame.GetSpriteID() ].GetHeight();
							*/
							BOOL bMouseInCreature = false;

							// Creature Table?? Width ?????? ????
							if (pCreature->GetWidth() != -1)
							{
								// Width?? ???? ??? ?????? ??ü????.
								RECT	rcScreenRect = pCreature->GetScreenRect();
								POINT	ptMouse = { x, y };

								int		nWidth = pCreature->GetWidth();
								int		nCenterY = rcScreenRect.left + (rcScreenRect.right - rcScreenRect.left) / 2;

								// ??????????? ??? ???? ????
								rcScreenRect.left = nCenterY - (nWidth >> 1);
								rcScreenRect.right = nCenterY + (nWidth >> 1);

								// ??ü????.
								// A creature whose body sprite never draws -- the Hell Garden crystals
								// are rendered entirely by the effect system -- leaves m_ScreenRect in
								// its reset, inverted state (left/top = screen size, right/bottom = 0).
								// Overriding left/right above cannot help because top/bottom are still
								// inverted, so PtInRect always fails and the click falls through to the
								// ground. Fall back to a tile hit test for that case.
								if (rcScreenRect.bottom <= rcScreenRect.top)
								{
									// The sprite contributed no bounds, so build a hit box from the
									// creature tile position plus the Width/Height in the creature table.
									// This inverts the conversion GetSelectedSector() performs.
									POINT ptOrigin = MapToPixel(m_FirstSector.x, m_FirstSector.y);
									POINT ptBase   = MapToPixel(pCreature->GetX(), pCreature->GetY());
								
									int sx = ptBase.x - ptOrigin.x - m_PlusPoint.x;
									int sy = ptBase.y - ptOrigin.y - m_PlusPoint.y;
									int nHeight = pCreature->GetHeight();
								
									rcScreenRect.left   = sx - (nWidth >> 1);
									rcScreenRect.right  = sx + (nWidth >> 1);
									rcScreenRect.bottom = sy + 24;
									rcScreenRect.top    = sy - nHeight;
								
									// accept the sprite area, and still accept the ground tile itself
									POINT ptTile = GetSelectedSector(x, y);
									bMouseInCreature = PtInRect(&rcScreenRect, ptMouse)
										|| (ptTile.x == pCreature->GetX() && ptTile.y == pCreature->GetY());
								}
								else
								{
									bMouseInCreature = PtInRect(&rcScreenRect, ptMouse);
								}
							}
							else
							{
								// ?????? ??????? ??ü?
								bMouseInCreature = pCreature->IsPointInScreenRect(x, y);
							}


							// ?????? ????? Creature?? ?????? ?????
							if ((
								g_pPlayer->IsVampire() && g_pZone->GetID() != 3001 ||
								!g_pPlayer->IsVampire() && !(!pCreature->IsNPC() && pCreature->IsInDarkness()) ||
								!g_pPlayer->IsVampire() && (g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))
								|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
								|| g_bLight
#endif
								)
								&& !pCreature->IsCutHeight()	// ??????? ???¸?
								&& g_pObjectSelector->CanSelect(pCreature)
								&& bMouseInCreature				// ?????? ???ó?? ??????? ??°??
								&& !(pCreature->IsOusters() && pCreature->IsInGroundElemental() && !g_pPlayer->IsOusters()))
								//if (x > rect.left && x < rect.right
								//	&& y > rect.top && y < rect.bottom)
							{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
								DEBUG_ADD("reFc");
#endif

								return pCreature;
							}
						}
					}
				}

				//------------------------------------------------
				//
				// Creature?? ??? ???
				//
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
				DEBUG_ADD("ckGc");
#endif

				numCreature = sector.GetGroundCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetGroundCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_GROUNDCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{
							/*
							// Creature?? ??????? ?????? ?????.
							// FRAME_PACK???? ?????? ???? ?? ??!!
							// Creature?? Sector????? Pixel????? ???? ???.

							// Creature?? Zone?? ????? ??????.
							objectPixelPoint = MapToPixel(pCreature->GetX(), pCreature->GetY());
							objectPixelPoint.x += m_PlusPoint.x + pCreature->GetSX();
							objectPixelPoint.y += m_PlusPoint.y + pCreature->GetSY();

							// Zone?? ????? ????? ????? ????.
							objectPixelPoint.x -= firstZonePixel.x;
							objectPixelPoint.y -= firstZonePixel.y;

							// Creature?? ??? ??????? ?????? ????????.
							frame = m_CreatureFPK[(*g_pCreatureTable)[pCreature->GetCreatureType()].FrameID][pCreature->GetAction()][pCreature->GetDirection()][pCreature->GetFrame()];

							// ???? Sprite?? ??? ??µ?? ???
							objectPixelPoint.x += frame.GetCX();
							objectPixelPoint.y += frame.GetCY();

							// ??? ??????? ?????? ????????.
							rect.left	= objectPixelPoint.x;
							rect.top	= objectPixelPoint.y;
							rect.right	= rect.left + m_CreatureSPK[ frame.GetSpriteID() ].GetWidth();
							rect.bottom	= rect.top + m_CreatureSPK[ frame.GetSpriteID() ].GetHeight();
							*/


							BOOL bMouseInCreature = false;

							// Creature Table?? Width ?????? ????
							if (pCreature->GetWidth() != -1)
							{
								// Width?? ???? ??? ?????? ??ü????.
								RECT	rcScreenRect = pCreature->GetScreenRect();
								POINT	ptMouse = { x, y };

								int		nWidth = pCreature->GetWidth();
								int		nCenterY = rcScreenRect.left + (rcScreenRect.right - rcScreenRect.left) / 2;

								// ??????????? ??? ???? ????
								rcScreenRect.left = nCenterY - (nWidth >> 1);
								rcScreenRect.right = nCenterY + (nWidth >> 1);

								// ??ü????.
								// A creature whose body sprite never draws -- the Hell Garden crystals
								// are rendered entirely by the effect system -- leaves m_ScreenRect in
								// its reset, inverted state (left/top = screen size, right/bottom = 0).
								// Overriding left/right above cannot help because top/bottom are still
								// inverted, so PtInRect always fails and the click falls through to the
								// ground. Fall back to a tile hit test for that case.
								if (rcScreenRect.bottom <= rcScreenRect.top)
								{
									// The sprite contributed no bounds, so build a hit box from the
									// creature tile position plus the Width/Height in the creature table.
									// This inverts the conversion GetSelectedSector() performs.
									POINT ptOrigin = MapToPixel(m_FirstSector.x, m_FirstSector.y);
									POINT ptBase   = MapToPixel(pCreature->GetX(), pCreature->GetY());
								
									int sx = ptBase.x - ptOrigin.x - m_PlusPoint.x;
									int sy = ptBase.y - ptOrigin.y - m_PlusPoint.y;
									int nHeight = pCreature->GetHeight();
								
									rcScreenRect.left   = sx - (nWidth >> 1);
									rcScreenRect.right  = sx + (nWidth >> 1);
									rcScreenRect.bottom = sy + 24;
									rcScreenRect.top    = sy - nHeight;
								
									// accept the sprite area, and still accept the ground tile itself
									POINT ptTile = GetSelectedSector(x, y);
									bMouseInCreature = PtInRect(&rcScreenRect, ptMouse)
										|| (ptTile.x == pCreature->GetX() && ptTile.y == pCreature->GetY());
								}
								else
								{
									bMouseInCreature = PtInRect(&rcScreenRect, ptMouse);
								}
							}
							else
							{
								// ?????? ??????? ??ü?
								bMouseInCreature = pCreature->IsPointInScreenRect(x, y);
							}


							// ?????? ????? Creature?? ?????? ?????
							if ((g_pPlayer->IsVampire() && g_pZone->GetID() != 3001 ||
								!g_pPlayer->IsVampire() && !(!pCreature->IsNPC() && pCreature->IsInDarkness()) ||
								!g_pPlayer->IsVampire() && (g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))
								|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
								|| g_bLight
#endif
								)
								&& !pCreature->IsCutHeight()	// ??????? ???¸?
								&& g_pObjectSelector->CanSelect(pCreature)
								&& bMouseInCreature
								&& !(pCreature->IsOusters() && pCreature->IsInGroundElemental() && !g_pPlayer->IsOusters()))
								//if (x > rect.left && x < rect.right
								//	&& y > rect.top && y < rect.bottom)
							{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
								DEBUG_ADD("reGc");
#endif

								return pCreature;
							}
						}
					}
				}

				//------------------------------------------------
				//
				// Effect?? ??? ???
				//
				//------------------------------------------------				
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
				DEBUG_ADD("ckE");
#endif

				if (sector.IsExistEffect())
				{
					int	effectNum = sector.GetEffectSize();

					EFFECT_LIST::const_iterator iEffect = sector.GetEffectIterator();

					for (int e = 0; e < effectNum; e++)
					{
						MEffect* pEffect = *iEffect;

						if (pEffect->IsSelectable())
						{
							if (pEffect->IsPointInScreenRect(x, y))
							{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
								DEBUG_ADD("reE");
#endif

								return pEffect;
							}
						}

						iEffect++;
					}
				}

				//------------------------------------------------
				//
				// UndergroundCreature?? ??? ???
				//
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
				DEBUG_ADD("ckUc");
#endif

				numCreature = sector.GetUndergroundCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetUndergroundCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_UNDERGROUNDCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{
							// ?????? ????? Creature?? ?????? ?????
							if ((g_pPlayer->IsVampire() && g_pZone->GetID() != 3001 ||
								!g_pPlayer->IsVampire() && !(!pCreature->IsNPC() && pCreature->IsInDarkness())
								|| !g_pPlayer->IsVampire() && (g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT))
								|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
								|| g_bLight
#endif
								)
								&& g_pObjectSelector->CanSelect(pCreature)
								&& pCreature->IsPointInScreenRect(x, y)
								&& !(pCreature->IsOusters() && pCreature->IsInGroundElemental() && !g_pPlayer->IsOusters())
								)
							{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
								DEBUG_ADD("uC");
#endif

								return pCreature;
							}
						}
					}
				}

				//------------------------------------------------
				//
				// Item?? ??? ???
				//
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
				DEBUG_ADD("ckIt");
#endif

				MItem* const pItem = sector.GetItem();

				if (pItem != NULL)
				{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
					DEBUG_ADD("IT");
#endif

					//file << "Exist Item!" << endl;

					// Item?? ??????? ?????? ?????.
					// FRAME_PACK???? ?????? ???? ?? ??!!
					// Item?? Sector????? Pixel????? ???? ???.
					/*
					// Item?? Zone?? ????? ??????.
					objectPixelPoint = MapToPixel(pItem->GetX(), pItem->GetY());
					objectPixelPoint.x += m_PlusPoint.x;// + pItem->GetSX();
					objectPixelPoint.y += m_PlusPoint.y;// + pItem->GetSY();

					// Zone?? ????? ????? ????? ????.
					objectPixelPoint.x -= firstZonePixel.x;
					objectPixelPoint.y -= firstZonePixel.y;

					// Item?? ??? ??????? ?????? ????????.
					frame = m_ItemTileFPK[ pItem->GetTileFrameID() ];

					// ???? Sprite?? ??? ??µ?? ???
					objectPixelPoint.x += frame.GetCX();
					objectPixelPoint.y += frame.GetCY();

					// ??? ??????? ?????? ????????.
					rect.left	= objectPixelPoint.x;
					rect.top	= objectPixelPoint.y;
					rect.right	= rect.left + m_ItemTileSPK[ frame.GetSpriteID() ].GetWidth();
					rect.bottom	= rect.top + m_ItemTileSPK[ frame.GetSpriteID() ].GetHeight();
					*/

					//------------------------------------------------
					// ?????????
					// ????????? ???? ?? ?? ??? ???????.. 
					//------------------------------------------------
					if (g_pPlayer->IsVampire() && g_pZone->GetID() != 3001
						|| !g_pPlayer->IsVampire() && g_pPlayer->ShowInDarkness(currentX, currentY)
						|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)
#ifdef __METROTECH_TEST__
						|| g_bLight
#endif
						)
					{
						//------------------------------------------------
						// ??ü?? ???? 
						// ??ü Creature?? ?????? ???? ???.
						//------------------------------------------------
						if (pItem->GetItemClass() == ITEM_CLASS_CORPSE)
						{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
							DEBUG_ADD("corp");
#endif

							MCorpse* pCorpse = (MCorpse*)pItem;

							MCreature* pCreature = pCorpse->GetCreature();

							if (pCreature != NULL)
							{
								if (pCreature->IsPointInScreenRect(x, y))
								{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
									DEBUG_ADD("reIT0");
#endif

									return pItem;
								}
							}
						}
						else
						{
							// ?????? ????? Item?? ?????? ?????
							if (pItem->IsPointInScreenRect(x, y))
								//if (x > rect.left && x < rect.right
									//&& y > rect.top && y < rect.bottom)
							{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
								DEBUG_ADD("reIT");
#endif

								return pItem;
							}
						}
					}
				}

			}
		}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
		DEBUG_ADD("ckIO0");
#endif

		//------------------------------------------------------
		// ??? ?????? currentY???? ???? ????? 
		// ImageObject???? Check???.
		// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
		//------------------------------------------------------
//		while (iImageObjectCheck != m_mapImageObject.rend())
//		{
//			MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectCheck).second);
//
			// InteractionObject?? ??? check???.
//			if (pImageObject->GetObjectType()==MObject::TYPE_INTERACTIONOBJECT)
//			{
//				MInteractionObject* const pInteractionObject = (MInteractionObject* const)pImageObject;
//
//				// ??? ?????? sY1???? ???? ???..?? Check
//				if (pInteractionObject->GetViewpoint() == currentY)
//				{
//					/*
//					// ????? ????? ???? ????? ????? ?????								
//					objectPixelPoint.x = pInteractionObject->GetPixelX() - m_FirstZonePixel.x;
//					objectPixelPoint.y = pInteractionObject->GetPixelY() - m_FirstZonePixel.y;
//
//					// frame?? ????? sprite?? ?????.
//					frame = m_ImageObjectFPK[ pInteractionObject->GetFrameID() ][ pInteractionObject->GetFrame() ];
//					sprite = frame.GetSpriteID();
//
//					// ???? ???? ???? ????..
//					if (m_ImageObjectSPK[ sprite ].IsNotInit())
//					{
//						iImageObjectCheck ++;
//						continue;
//					}
//							
//					// ??? ????
//					objectPixelPoint.x += frame.GetCX();
//					objectPixelPoint.y += frame.GetCY();
//
//					// ??? ??????? ?????? ????????.
//					rect.left	= objectPixelPoint.x;
//					rect.top	= objectPixelPoint.y;
//					rect.right	= rect.left + m_ImageObjectSPK[ sprite ].GetWidth();
//					rect.bottom	= rect.top + m_ImageObjectSPK[ sprite ].GetHeight();
//					*/			
//					// ?????? ????? ImageObject?? ?????? ?????
//					if (pInteractionObject->IsPointInScreenRect(x,y))
//					//if (x > rect.left && x < rect.right
//					//	&& y > rect.top && y < rect.bottom)
//					{			
//						#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
//							DEBUG_ADD("reIO");
//						#endif
//
//						return pInteractionObject;
//					}					
//				}
//				// ????.. Object check??? Check????? ??????.
//				else
//				{
//					break;
//				}
//			}
//
//			iImageObjectCheck ++;
//		}
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
	DEBUG_ADD("ckIO");
#endif

	//------------------------------------------------------
	// ??? ?????? currentY???? ???? ????? 
	// ImageObject???? Check???.
	// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
//	while (iImageObjectCheck != m_mapImageObject.rend())
//	{
//		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectCheck).second);
//
		// InteractionObject?? ??? check???.
//		if (pImageObject->GetObjectType()==MObject::TYPE_INTERACTIONOBJECT)
//		{
//			MInteractionObject* const pInteractionObject = (MInteractionObject* const)pImageObject;
//		
//			// ????? ????? ???? ????? ????? ?????								
//			/*
//			objectPixelPoint.x = pInteractionObject->GetPixelX() - m_FirstZonePixel.x;
//			objectPixelPoint.y = pInteractionObject->GetPixelY() - m_FirstZonePixel.y;
//
//			// frame?? ????? sprite?? ?????.
//			frame = m_ImageObjectFPK[ pInteractionObject->GetFrameID() ][ pInteractionObject->GetFrame() ];
//			sprite = frame.GetSpriteID();
//
//			// ???? ???? ???? ????..
//			if (m_ImageObjectSPK[ sprite ].IsNotInit())
//			{
//				iImageObjectCheck ++;
//				continue;
//			}
//							
//			// ??? ????
//			objectPixelPoint.x += frame.GetCX();
//			objectPixelPoint.y += frame.GetCY();
//
//			// ??? ??????? ?????? ????????.
//			rect.left	= objectPixelPoint.x;
//			rect.top	= objectPixelPoint.y;
//			rect.right	= rect.left + m_ImageObjectSPK[ sprite ].GetWidth();
//			rect.bottom	= rect.top + m_ImageObjectSPK[ sprite ].GetHeight();
//			*/				
//			// ?????? ????? ImageObject?? ?????? ?????
//			if (pInteractionObject->IsPointInScreenRect(x,y))
//			//if (x > rect.left && x < rect.right
//			//	&& y > rect.top && y < rect.bottom)
//			{
//				#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
//					DEBUG_ADD("reIO");
//				#endif
//
//				return pInteractionObject;
//			}
//		}
//
//		iImageObjectCheck ++;
//	}

	//------------------------------------------------------
	// ?????? ?????.. GroundEffect?? ü????.
	//------------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
	DEBUG_ADD("chkGE");
#endif

	int effectNum = g_pZone->GetGroundEffectCount();
	MZone::EFFECT_MAP::const_iterator iEffect = g_pZone->GetGroundEffects();
	//	MZone::EFFECT_MAP::const_iterator iEffectEnd = g_pZone->GetGroundEffectsEnd();
	//
	//	while(iEffect != iEffectEnd)
	//	{
	//		MEffect* pEffect = iEffect->second;
	//		if(pEffect != NULL)
	//		{
	//			if (pEffect->IsSelectable()
	//				&& pEffect->IsPointInScreenRect(x, y))
	//			{
	//				#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
	//					DEBUG_ADD("reGE");
	//				#endif
	//
	//				return pEffect;			
	//			}
	//		}
	//		iEffect++;
	//	}
	for (int e = 0; e < effectNum; e++)
	{
		MEffect* pEffect = iEffect->second;
		if (pEffect != NULL)
		{
			if (pEffect->IsSelectable()
				&& pEffect->IsPointInScreenRect(x, y))
			{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS_INPUT
				DEBUG_ADD("reGE");
#endif

				return pEffect;
			}
		}
		iEffect++;
	}

	if (IsRequestInfo())
	{
		//------------------------------------------------------
		// ?????? ?????.. FakeCreature(Pet)?? ü????.
		//------------------------------------------------------
		int fakeCreatureNum = g_pZone->GetFakeCreatureNumber();
		MZone::CREATURE_MAP::const_iterator iFakeCreature = g_pZone->GetFakeCreatureBegin();
		for (int fc = 0; fc < fakeCreatureNum; fc++)
		{
			MFakeCreature* pFakeCreature = (MFakeCreature*)iFakeCreature->second;

			// ?????? ???
			if (pFakeCreature->GetOwnerID() != OBJECTID_NULL && pFakeCreature->IsPointInScreenRect(x, y))
			{
				return pFakeCreature;
			}

			iFakeCreature++;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------
// ?? ????????? actionTarget ????? ??????? ??
// Target???? ?????? ?????? ???????????
//----------------------------------------------------------------------
bool
MTopView::IsAbleToActionTarget(MObject* pObject, BYTE_ATTRIBUTE actionTarget)
{
	// ??? ??? ??????? ??????? true
	if (actionTarget.GetAttr(FLAG_ACTIONINFO_SELECT_ALL))
		return true;

	bool bAbleToTarget = false;

	// ??ü?? ?????? BoA??~
	if (actionTarget.GetAttr(FLAG_ACTIONINFO_SELECT_CORPSE))
	{
		if (pObject->GetObjectType() == MObject::TYPE_CREATURE)
		{
			MCreature* pCreature = dynamic_cast<MCreature*>(pObject);
			if (pCreature)
				bAbleToTarget |= pCreature->IsDead();
		}
	}

	return bAbleToTarget;
}


//----------------------------------------------------------------------
// ??? ??? (x,y)?? ??????? ????? ??????? 
// Zone?? ??(sX,sY) Object?? ???õ???
//
// Sprite???? pixel?????? ??? ?????.
//
//----------------------------------------------------------------------
MObject*
MTopView::GetSelectedObjectSprite(int x, int y, BYTE_ATTRIBUTE actionTarget)
{
	//--------------------------------------------------------------	
	// 1. ?????? (x,y)?? Zone?????? Sector(sX,sY)????? ?????.
	// 
	// 2. (sX,sY) ??ó?? Object?? ?????? 
	//    ?? Object?? ??? ???? ????? ?????
	//    (x,y)?? ????? "???????"
	//    ????, ?? ?????? ??ó Sector?? check????? ???.
	//
	// (!) Player?? ?????. ????? ??????? check???.
	//--------------------------------------------------------------

	//--------------------------------------------------------------
	// ??ó Sector?? ã?? ????
	//--------------------------------------------------------------	
	// - ????? ????? Sector?? ??? ??????? ??? ??µ???
	//   ???õ? ???????? ????.
	// 
	//      .  .  .  
	//      .  X  . 
	//      .  .  .  
	//      .  .  .  
	//      .  .  .     X?? ???õ??? ???, Ground,Item ??... check????
	//
	//
	//               
	//         X    
	//               
	//      .  .  .  
	//      .  .  .  
	//      .  .  .  
	//      .  .  .     Flying?? 64 pixel(?? Ÿ??) ???? ?? ???..
	//      .  .  .     X?? ???õ??? ???, Flying... check????
	//
	//--------------------------------------------------------------

	//--------------------------------------------------------------
	// ?? Sector?? ????? check????? ??? ??
	//--------------------------------------------------------------
	/*

	   while ( ExistNextSector )
	   {
			if ( IsExistObject )
			{
				if ( IsObjectCreature && ClickPoint In TheCreatureRect)
					Select Creature

				else if ( IsObjectItem && ClickPoint In TheItemRect )
					Select Item
			}

			CurrentSector = NextSector
		}

		if ( Not Selected )
			return NULL;

		return Object;
	*/
	//--------------------------------------------------------------

	POINT firstZonePixel;	// ?????? ó?? Sector?? ??Ÿ???? Zone?? pixel???
	firstZonePixel = MapToPixel(m_FirstSector.x, m_FirstSector.y);
	firstZonePixel.x += m_PlusPoint.x;
	firstZonePixel.y += m_PlusPoint.y;


	POINT	pixelPoint;			// Pixel Position In Zone
	POINT	sectorPoint;		// Sector Position In Zone

	//--------------------------------------------------------------
	// (x,y)?? Zone?????? pixel????? ?????.
	//--------------------------------------------------------------
	pixelPoint = MapToPixel(m_FirstSector.x, m_FirstSector.y);
	pixelPoint.x += m_PlusPoint.x + x;
	pixelPoint.y += m_PlusPoint.y + y;

	//--------------------------------------------------------------
	// (x,y)?? Zone?????? sector????? ?????.
	//--------------------------------------------------------------						
	sectorPoint = PixelToMap(pixelPoint.x, pixelPoint.y);

	//--------------------------------------------------------------
	//
	// check
	//
	//--------------------------------------------------------------

	MObject* pObject = NULL;
	MCreature* pCreature = NULL;
	MItem* pItem = NULL;
	CFrame			frame;
	//	TYPE_SPRITEID	sprite;

		// Object?? ????
	POINT	objectPixelPoint;

	// Player Creature's ID
	TYPE_OBJECTID	pid = g_pPlayer->GetID();

	//std::ofstream file("log.txt");


	// ???? check??? ???
	int currentX;
	int currentY;

	int numCreature = 0;

	//--------------------------------------------------------------
	//
	// Flying Creature Check 
	//
	// : ???? ????? ??? ??? 
	//   ??????? Object???? ????? Sector?? ?????? ?? ???.
	//
	//--------------------------------------------------------------
	int	sX1 = sectorPoint.x - 1,
		sX2 = sectorPoint.x + 1;
	int	sY1 = sectorPoint.y + 4,
		sY2 = sectorPoint.y + 5;

	//------------------------------------------------------
	// Zone?? ?????? ??? ??? Skip...
	//------------------------------------------------------
	if (sX1 < 0)
	{
		sX1 = 0;
	}

	if (sX2 >= m_pZone->GetWidth())
	{
		sX2 = m_pZone->GetWidth() - 1;
	}

	if (sY1 < 0)
	{
		sY1 = 0;
	}

	if (sY2 >= m_pZone->GetHeight())
	{
		sY2 = m_pZone->GetHeight() - 1;
	}


	//------------------------------------------------------	
	// ???????? Sector?? check???.
	//------------------------------------------------------	
	for (currentY = sY2; currentY >= sY1; currentY--)
	{
		for (currentX = sX1; currentX <= sX2; currentX++)
		{
			const MSector& sector = m_pZone->GetSector(currentX, currentY);

			// ?? Object?? ???? ???
			if (sector.IsExistObject())
			{

				//file << "Exist Object!" << endl;

				// Object?? ?????? ???? ??? check???.
				// ?????? ?? ??? ???? Creature?? Item?????.

				//------------------------------------------------
				//
				// Flying Creature?? ??? ???
				//
				//------------------------------------------------
				numCreature = sector.GetFlyingCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetFlyingCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_FLYINGCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{
							// FlyingCreature?? ??????? ?????? ?????.
							// FRAME_PACK???? ?????? ???? ?? ??!!
							// FlyingCreature?? Sector????? Pixel????? ???? ???.

							// FlyingCreature?? Zone?? ????? ??????.
							objectPixelPoint = MapToPixel(pCreature->GetX(), pCreature->GetY());
							objectPixelPoint.x += m_PlusPoint.x + pCreature->GetSX();
							objectPixelPoint.y += m_PlusPoint.y + pCreature->GetSY();

							// Zone?? ????? ????? ????? ????.
							objectPixelPoint.x -= firstZonePixel.x;
							objectPixelPoint.y -= firstZonePixel.y;

							// Creature?? ??? ??????? ?????? ????????.	
							frame = m_CreatureFPK[pCreature->GetCreatureFrameID(0)][pCreature->GetAction()][pCreature->GetDirection()][pCreature->GetFrame()];

							// ???? Sprite?? ??? ??µ?? ???
							objectPixelPoint.x += frame.GetCX();
							objectPixelPoint.y += frame.GetCY() - pCreature->GetZ();//FLYINGCREATURE_HEIGHT;
							// FramePack???? ????? ????????,
							// Flying?? ??? ?????.

		// mouse????? Sprite?? ??????? ??????
		// Sprite?????? ????? ?????? ??? ??????
							if (GetCreatureSprite(frame.GetSpriteID(), pCreature->GetCreatureFrameID(0)).IsColorPixel(
								x - objectPixelPoint.x,
								y - objectPixelPoint.y
							))
							{
								return pCreature;
							}
						}
					}
				}
			}
		}
	}


	//--------------------------------------------------------------
	//
	// Creature / Item Check data
	//
	//--------------------------------------------------------------
	sY1 = sectorPoint.y - 1,
		sY2 = sectorPoint.y + 3;

	//------------------------------------------------------
	// Zone?? ?????? ??? ??? Skip...
	//------------------------------------------------------	
	if (sY1 < 0)
	{
		sY1 = 0;
	}

	if (sY2 >= m_pZone->GetHeight())
	{
		sY2 = m_pZone->GetHeight() - 1;
	}

	//------------------------------------------------------
	// Check????? ??? ImageObject Iterator
	//------------------------------------------------------
	IMAGEOBJECT_OUTPUT_MAP::reverse_iterator iImageObjectCheck
		= m_mapImageObject.rbegin();

	//------------------------------------------------------
	// ??? ?????? sY1???? ???? ????? 
	// ImageObject???? Check???.
	// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (iImageObjectCheck != m_mapImageObject.rend())
	{
		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectCheck).second);

		// InteractionObject?? ??? check???.
//		if (pImageObject->GetObjectType()==MObject::TYPE_INTERACTIONOBJECT)
//		{
//			MInteractionObject* const pInteractionObject = (MInteractionObject* const)pImageObject;
//
//			// ??? ?????? sY1???? ???? ???..?? Check
//			if (pInteractionObject->GetViewpoint() > sY2)
//			{
//				// ????? ????? ???? ????? ????? ?????								
//				objectPixelPoint.x = pInteractionObject->GetPixelX() - m_FirstZonePixel.x;
//				objectPixelPoint.y = pInteractionObject->GetPixelY() - m_FirstZonePixel.y;
//
//				// frame?? ????? sprite?? ?????.
//				frame = m_ImageObjectFPK[ pInteractionObject->GetFrameID() ][ pInteractionObject->GetFrame() ];
//				sprite = frame.GetSpriteID();
//						
//				// ??? ????
//				objectPixelPoint.x += frame.GetCX();
//				objectPixelPoint.y += frame.GetCY();
//									
//				// InteractionObject?? (point.x, point.y)?? ??µ?? ???? ?????.
//				// mouse????? Sprite?? ??????? ??????
//				// Sprite?????? ????? ?????? ??? ??????
//				if (m_ImageObjectSPK[ sprite ].IsColorPixel( 
//						x - objectPixelPoint.x, 
//						y - objectPixelPoint.y 
//					))
//				{	
//					pInteractionObject->SetAction( 1 );
//					return pInteractionObject;
//				}
//			}
//			// ????.. Object check??? Check????? ??????.
//			else
//			{
//				break;
//			}
//		}

		iImageObjectCheck++;
	}

	//------------------------------------------------------	
	// ???????? Sector?? check???.
	//------------------------------------------------------	
	for (currentY = sY2; currentY >= sY1; currentY--)
	{
		for (currentX = sX1; currentX <= sX2; currentX++)
		{
			const MSector& sector = m_pZone->GetSector(currentX, currentY);

			// ?? Object?? ???? ???
			if (sector.IsExistObject())
			{

				//file << "Exist Object!" << endl;

				// Object?? ?????? ???? ??? check???.
				// ?????? ?? ??? ???? Creature?? Item?????.

				//------------------------------------------------
				//
				// Flying Creature?? ??? ???
				//
				//------------------------------------------------
				numCreature = sector.GetFlyingCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetFlyingCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_FLYINGCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{

							// FlyingCreature?? ??????? ?????? ?????.
							// FRAME_PACK???? ?????? ???? ?? ??!!
							// FlyingCreature?? Sector????? Pixel????? ???? ???.

							// FlyingCreature?? Zone?? ????? ??????.
							objectPixelPoint = MapToPixel(pCreature->GetX(), pCreature->GetY());
							objectPixelPoint.x += m_PlusPoint.x + pCreature->GetSX();
							objectPixelPoint.y += m_PlusPoint.y + pCreature->GetSY();

							// Zone?? ????? ????? ????? ????.
							objectPixelPoint.x -= firstZonePixel.x;
							objectPixelPoint.y -= firstZonePixel.y;

							// Creature?? ??? ??????? ?????? ????????.	
							frame = m_CreatureFPK[pCreature->GetCreatureFrameID(0)][pCreature->GetAction()][pCreature->GetDirection()][pCreature->GetFrame()];

							// ???? Sprite?? ??? ??µ?? ???
							objectPixelPoint.x += frame.GetCX();
							objectPixelPoint.y += frame.GetCY() - pCreature->GetZ();//FLYINGCREATURE_HEIGHT;
							// FramePack???? ????? ????????,
							// Flying?? ??? ?????.

		// mouse????? Sprite?? ??????? ??????
		// Sprite?????? ????? ?????? ??? ??????
							if (GetCreatureSprite(frame.GetSpriteID(), pCreature->GetCreatureFrameID(0)).IsColorPixel(
								x - objectPixelPoint.x,
								y - objectPixelPoint.y
							))
							{
								return pCreature;
							}
						}
					}
				}

				//------------------------------------------------
				//
				// Creature?? ??? ???
				//
				//------------------------------------------------
				numCreature = sector.GetGroundCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetGroundCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_GROUNDCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{

							// Creature?? ??????? ?????? ?????.
							// FRAME_PACK???? ?????? ???? ?? ??!!
							// Creature?? Sector????? Pixel????? ???? ???.

							// Creature?? Zone?? ????? ??????.
							objectPixelPoint = MapToPixel(pCreature->GetX(), pCreature->GetY());
							objectPixelPoint.x += m_PlusPoint.x + pCreature->GetSX();
							objectPixelPoint.y += m_PlusPoint.y + pCreature->GetSY();

							// Zone?? ????? ????? ????? ????.
							objectPixelPoint.x -= firstZonePixel.x;
							objectPixelPoint.y -= firstZonePixel.y;

							// Creature?? ??? ??????? ?????? ????????.	
							frame = m_CreatureFPK[pCreature->GetCreatureFrameID(0)][pCreature->GetAction()][pCreature->GetDirection()][pCreature->GetFrame()];

							// ???? Sprite?? ??? ??µ?? ???
							objectPixelPoint.x += frame.GetCX();
							objectPixelPoint.y += frame.GetCY();

							// mouse????? Sprite?? ??????? ??????
							// Sprite?????? ????? ?????? ??? ??????
							if (GetCreatureSprite(frame.GetSpriteID(), pCreature->GetCreatureFrameID(0)).IsColorPixel(
								x - objectPixelPoint.x,
								y - objectPixelPoint.y
							))
							{
								return pCreature;
							}
						}
					}
				}

				//------------------------------------------------
				//
				// Underground Creature?? ??? ???
				//
				//------------------------------------------------
				numCreature = sector.GetUndergroundCreatureSize();

				if (numCreature > 0)
				{
					OBJECT_MAP::const_iterator iCreature = sector.GetUndergroundCreatureIterator();

					for (int c = 0; c < numCreature; c++, iCreature++)
					{
						if (iCreature == sector.GetObjectEnd()
							|| iCreature->first > MSector::POSITION_UNDERGROUNDCREATURE_MAX)
							break;

						pCreature = (MCreature*)iCreature->second;

						// Player??? ???????? ??´?.
						// Target???? ?????? ????????
						if (pCreature->GetID() != pid &&
							IsAbleToActionTarget(pCreature, actionTarget))
						{

							// Creature?? ??????? ?????? ?????.
							// FRAME_PACK???? ?????? ???? ?? ??!!
							// Creature?? Sector????? Pixel????? ???? ???.

							// Creature?? Zone?? ????? ??????.
							objectPixelPoint = MapToPixel(pCreature->GetX(), pCreature->GetY());
							objectPixelPoint.x += m_PlusPoint.x + pCreature->GetSX();
							objectPixelPoint.y += m_PlusPoint.y + pCreature->GetSY();

							// Zone?? ????? ????? ????? ????.
							objectPixelPoint.x -= firstZonePixel.x;
							objectPixelPoint.y -= firstZonePixel.y;

							// Creature?? ??? ??????? ?????? ????????.	
							// ??? ????
							//frame = m_CreatureFPK[pCreature->GetCreatureFrameID()][pCreature->GetAction()][pCreature->GetDirection()][pCreature->GetFrame()];

							// ???? Sprite?? ??? ??µ?? ???
							//objectPixelPoint.x += frame.GetCX();
							//objectPixelPoint.y += frame.GetCY();

							// mouse????? Sprite?? ??????? ??????
							// Sprite?????? ????? ?????? ??? ??????
							if (m_EtcSPK[SPRITEID_CREATURE_BURROW].IsColorPixel(
								x - objectPixelPoint.x,
								y - objectPixelPoint.y
							))
							{
								return pCreature;
							}
						}
					}
				}

				//------------------------------------------------
				//
				// Item?? ??? ???
				//
				//------------------------------------------------				
				pObject = (MObject*)sector.GetItem();

				if (pObject != NULL)
				{
					//file << "Exist Item!" << endl;

					pItem = (MItem*)pObject;

					// Item?? ??????? ?????? ?????.
					// FRAME_PACK???? ?????? ???? ?? ??!!
					// Item?? Sector????? Pixel????? ???? ???.

					// Item?? Zone?? ????? ??????.
					objectPixelPoint = MapToPixel(pItem->GetX(), pItem->GetY());
					objectPixelPoint.x += m_PlusPoint.x;// + pItem->GetSX();
					objectPixelPoint.y += m_PlusPoint.y;// + pItem->GetSY();

					// Zone?? ????? ????? ????? ????.
					objectPixelPoint.x -= firstZonePixel.x;
					objectPixelPoint.y -= firstZonePixel.y;

					// Item?? ??? ??????? ?????? ????????.				
					frame = m_ItemTileFPK[pItem->GetTileFrameID()];

					// ???? Sprite?? ??? ??µ?? ???
					objectPixelPoint.x += frame.GetCX();
					objectPixelPoint.y += frame.GetCY();

					// mouse????? Sprite?? ??????? ??????
					// Sprite?????? ????? ?????? ??? ??????
					if (m_ItemTileISPK[frame.GetSpriteID()].IsColorPixel(
						x - objectPixelPoint.x,
						y - objectPixelPoint.y
					))
					{
						//file << "Select Item!" << endl;					
						//#ifdef	OUTPUT_DEBUG					
							//sprintf(g_pDebugMessage->GetCurrent(), "SelectItem(%d,%d)", x - objectPixelPoint.x, y - objectPixelPoint.y );
							//g_pDebugMessage->Next()
						//#endif

						return pItem;
					}
				}

			}
		}

		//------------------------------------------------------
		// ??? ?????? currentY???? ???? ????? 
		// ImageObject???? Check???.
		// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
		//------------------------------------------------------
		while (iImageObjectCheck != m_mapImageObject.rend())
		{
			MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectCheck).second);

			// InteractionObject?? ??? check???.
//			if (pImageObject->GetObjectType()==MObject::TYPE_INTERACTIONOBJECT)
//			{
//				MInteractionObject* const pInteractionObject = (MInteractionObject* const)pImageObject;
//
//				// ??? ?????? sY1???? ???? ???..?? Check
//				if (pInteractionObject->GetViewpoint() == currentY)
//				{
//					// ????? ????? ???? ????? ????? ?????								
//					objectPixelPoint.x = pInteractionObject->GetPixelX() - m_FirstZonePixel.x;
//					objectPixelPoint.y = pInteractionObject->GetPixelY() - m_FirstZonePixel.y;
//
//					// frame?? ????? sprite?? ?????.
//					frame = m_ImageObjectFPK[ pInteractionObject->GetFrameID() ][ pInteractionObject->GetFrame() ];
//					sprite = frame.GetSpriteID();
//							
//					// ??? ????
//					objectPixelPoint.x += frame.GetCX();
//					objectPixelPoint.y += frame.GetCY();
//										
//					// InteractionObject?? (point.x, point.y)?? ??µ?? ???? ?????.
//					// mouse????? Sprite?? ??????? ??????
//					// Sprite?????? ????? ?????? ??? ??????
//					if (m_ImageObjectSPK[ sprite ].IsColorPixel( 
//							x - objectPixelPoint.x, 
//							y - objectPixelPoint.y 
//						))
//					{							
//						return pInteractionObject;
//					}
//				}
//				// ????.. Object check??? Check????? ??????.
//				else
//				{
//					break;
//				}
//			}

			iImageObjectCheck++;
		}
	}

	//------------------------------------------------------
	// ??? ?????? currentY???? ???? ????? 
	// ImageObject???? Check???.
	// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (iImageObjectCheck != m_mapImageObject.rend())
	{
		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectCheck).second);

		// InteractionObject?? ??? check???.
//		if (pImageObject->GetObjectType()==MObject::TYPE_INTERACTIONOBJECT)
//		{
//			MInteractionObject* const pInteractionObject = (MInteractionObject* const)pImageObject;
//		
//			// ????? ????? ???? ????? ????? ?????								
//			objectPixelPoint.x = pInteractionObject->GetPixelX() - m_FirstZonePixel.x;
//			objectPixelPoint.y = pInteractionObject->GetPixelY() - m_FirstZonePixel.y;
//
//			// frame?? ????? sprite?? ?????.
//			frame = m_ImageObjectFPK[ pInteractionObject->GetFrameID() ][ pInteractionObject->GetFrame() ];
//			sprite = frame.GetSpriteID();
//							
//			// ??? ????
//			objectPixelPoint.x += frame.GetCX();
//			objectPixelPoint.y += frame.GetCY();
//									
//			// InteractionObject?? (point.x, point.y)?? ??µ?? ???? ?????.
//			// mouse????? Sprite?? ??????? ??????
//			// Sprite?????? ????? ?????? ??? ??????
//			if (m_ImageObjectSPK[ sprite ].IsColorPixel( 
//					x - objectPixelPoint.x, 
//					y - objectPixelPoint.y 
//				))
//			{					
//				return pInteractionObject;
//			}
//		}

		iImageObjectCheck++;
	}

	return NULL;
}

//----------------------------------------------------------------------
// Clear ItemNameList
//----------------------------------------------------------------------
// ?? ??????? ???.
//----------------------------------------------------------------------
void
MTopView::ClearItemNameList()
{
	DRAWITEMNAME_LIST::iterator iItemName = m_listDrawItemName.begin();

	// ??? node?? ?????.
	while (iItemName != m_listDrawItemName.end())
	{
		DRAWITEMNAME_NODE* pNode = *iItemName;

		// ??????? node????
		delete pNode;

		iItemName++;
	}

	m_listDrawItemName.clear();

}

//----------------------------------------------------------------------
// Add ItemName
//----------------------------------------------------------------------
// ?????? new?? ????? ???.
//
// pNode->GetDistance?? sort??? ?????? ???.
// ????????..
//----------------------------------------------------------------------
// item?? ???? ???? ????????? ü???°? ??????.
// MAX???? ?????? ??? ??????...
// ?????.. ????? ??????.. --;
// ??? ??? ?ð??? ??????.. ????..
//----------------------------------------------------------------------
void
MTopView::AddItemName(DRAWITEMNAME_NODE* pNode)
{
	DRAWITEMNAME_LIST::iterator iItemName = m_listDrawItemName.begin();

	BOOL bInsertEnd = TRUE;

	//-------------------------------------------------------
	// ?Ÿ??? ????? ItemName?? ??????.
	//-------------------------------------------------------
	while (iItemName != m_listDrawItemName.end())
	{
		DRAWITEMNAME_NODE* pOldNode = *iItemName;

		//------------------------------------------
		// ?? ??~
		//------------------------------------------
		if (pNode->GetDistance() < pOldNode->GetDistance())
		{
			m_listDrawItemName.insert(iItemName, pNode);

			bInsertEnd = FALSE;

			break;
		}

		iItemName++;
	}

	if (bInsertEnd)
	{
		// ?? ???? ???
		m_listDrawItemName.push_back(pNode);
	}

	//-------------------------------------------------------
	// MAX?? ??? ???
	//-------------------------------------------------------
	if (m_listDrawItemName.size() > g_pClientConfig->MAX_DRAWITEMNAME)
	{
		// ?????? ?? ???????.
		DRAWITEMNAME_NODE* pDeleteNode = m_listDrawItemName.back();

		delete pDeleteNode;

		m_listDrawItemName.pop_back();
	}
}

//----------------------------------------------------------------------
// DrawItemNameList
//----------------------------------------------------------------------
// ?????? Item???? AddText?? ???.
//----------------------------------------------------------------------
void
MTopView::DrawItemNameList()
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawItemNameList");
#endif


	DRAWITEMNAME_LIST::iterator iItemName = m_listDrawItemName.begin();

	int x, x2;
	int y = 0, y2;

	// minimap?? ????? ???.. 
	// ??? ????...
	// ???????? ?????..

	// 2001.7.14 ???ó??
	//if (g_pUserOption->DrawMinimap)// && g_pPlayer->IsSlayer())
	{
		// ?????? ??? ???~~
		y = 160;
	}

	while (iItemName != m_listDrawItemName.end())
	{
		DRAWITEMNAME_NODE* pNode = *iItemName;

		MItem* pItem = pNode->GetItem();


		const char* pItemName = pItem->GetName();

		// option?? ??????? ?????..
		COLORREF nameColor;
#if __CONTENTS(__TUNING_ITEM)
		if (pItem->IsTuningItem())
		{
			WORD temp_color = CIndexSprite::ColorSet[MItem::GetTuningItemColorSet()][8];
			nameColor = RGB(CDirectDraw::Red(temp_color) << 3, CDirectDraw::Green(temp_color) << 3, CDirectDraw::Blue(temp_color) << 3);
		}
		else
#endif //__TUNING_ITEM
			if (pItem->IsSpecialColorItem())
			{
				WORD temp_color;
				//			if(pItem->IsQuestItem() )
				//				temp_color = CIndexSprite::ColorSet[MItem::GetQuestItemColorset()][16];
				//			else
				//				temp_color = CIndexSprite::ColorSet[MItem::GetUniqueItemColorset()][16];				

				temp_color = CIndexSprite::ColorSet[pItem->GetSpecialColorItemColorset()][16];
				nameColor = RGB(CDirectDraw::Red(temp_color) << 3, CDirectDraw::Green(temp_color) << 3, CDirectDraw::Blue(temp_color) << 3);
			}
			else if (pItem->IsItemNameColorChange())
			{
				WORD temp_color = CIndexSprite::ColorSet[pItem->GetItemNameColorSet()][15];
				nameColor = RGB(CDirectDraw::Red(temp_color) << 3, CDirectDraw::Green(temp_color) << 3, CDirectDraw::Blue(temp_color) << 3);
			}
			else if ((*g_pItemTable)[pItem->GetItemClass()][pItem->GetItemType()].DropItemNameTag)
			{
				nameColor = (*g_pItemTable)[pItem->GetItemClass()][pItem->GetItemType()].DropItemNameTag;
			}
			else if (pItem->IsEmptyItemOptionList())
			{
				nameColor = m_ColorNameItem;
			}
			else if (pItem->GetItemOptionListCount() > 1)
			{
				nameColor = g_pClientConfig->COLOR_NAME_ITEM_RARE_OPTION;
			}
			else
			{
				nameColor = m_ColorNameItemOption;
			}

		if (pItemName != NULL)
		{
			// ??? ??? ????? ????????..
			x = CLIPSURFACE_WIDTH - g_GetStringWidth(pItemName, g_ClientPrintInfo[FONTID_ITEM]->hfont);
			x2 = CLIPSURFACE_WIDTH;

			y2 = y + g_pClientConfig->FONT_ITEM_HEIGHT;

			// ?????? ??? ????
			RECT rect = {
				x,
				y,
				x2,
				y2
			};

			// mouse?? ???õ? Item?? ???
			if (m_SelectItemID == pItem->GetID())
			{
				DRAWTEXT_NODE* pTextNodeBK = new DRAWTEXT_NODE(
					x,
					y + 1,
					pItemName,
					0,		// black
					FONTID_ITEM
				);

				AddText(pTextNodeBK);

				DRAWTEXT_NODE* pTextNode = new DRAWTEXT_NODE(
					x + 1,
					y + 1 + 1,
					pItemName,
					nameColor,
					FONTID_ITEM
				);

				AddText(pTextNode);
			}
			else
			{
				DRAWTEXT_NODE* pTextNode = new DRAWTEXT_NODE(
					x,
					y + 1,
					pItemName,
					nameColor,
					FONTID_ITEM
				);

				pTextNode->SetBox(rect, m_ColorBlackHalf);

				AddText(pTextNode);
			}

			// item Name?? ???? ????
			pNode->SetRect(rect);
		}

		// ????..
		iItemName++;
		y += g_pClientConfig->FONT_ITEM_HEIGHT;
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawItemNameList");
#endif	

}

//----------------------------------------------------------------------
// Select Item Name
//----------------------------------------------------------------------
// ??? ???(x,y)?? ?????? ???
// ?? ????? ItemName?? ??°??
//----------------------------------------------------------------------
MItem*
MTopView::SelectItemName(int x, int y)
{
	DRAWITEMNAME_LIST::iterator iItemName = m_listDrawItemName.begin();

	while (iItemName != m_listDrawItemName.end())
	{
		DRAWITEMNAME_NODE* pNode = *iItemName;

		//--------------------------------------------
		// ItemName?? ???ÿ????? ??????
		//--------------------------------------------
		if (pNode->IsPointInRect(x, y))
		{
			// item pointer?? ??????.
			// zone?? ???? ??? ???? ?? ???????...
			// zone???? ????? ???.
			return m_pZone->GetItem(pNode->GetID());
		}

		// ???? item name
		iItemName++;
	}

	return NULL;
}

//----------------------------------------------------------------------
// Clear TextList
//----------------------------------------------------------------------
// ?? ??????? ???.
//----------------------------------------------------------------------
void
MTopView::ClearTextList()
{
	while (!m_pqDrawText.empty())
	{
		DRAWTEXT_NODE* pNode = m_pqDrawText.top();

		// ??????? node????
		delete pNode;

		m_pqDrawText.pop();
	}
}

//----------------------------------------------------------------------
// Add Text
//----------------------------------------------------------------------
// DrawTextList?? ????? string?? ?????? ??????.
// ?????? new?? ????? ???.
//----------------------------------------------------------------------
void
MTopView::AddText(DRAWTEXT_NODE* pNode)
{
	/*
	const char* str = pNode->GetString();

	if (str==NULL)
	{
		delete pNode;
		return;
	}
	else if (str[0]=='\0')
	{
		delete pNode;
		return;
	}
	*/

	// ????? ?????? ???
	if (pNode->GetTextTime() == 0)
	{
		QWORD time = ((QWORD)(g_CurrentFrame + 160) << 41)
			+ m_pqDrawText.size();

		pNode->SetTextTime(time);
	}

	m_pqDrawText.push(pNode);
}
//----------------------------------------------------------------------
// Floating damage numbers
//
// The game server sends GCGetDamage to the player who swung, one per monster
// hit, carrying the final damage. Each number appears on top of that monster's
// head, rides along with it, and rises straight up for a moment.
//
// The number is anchored in zone pixels, not screen pixels: the view scrolls
// whenever the player walks, so a screen anchor left the number following the
// player instead of the monster. The head's offset from the monster's position
// is measured ONCE, when the number spawns: a monster's sprite rect changes
// width from one animation frame to the next, so re-measuring it every frame
// made the number jitter sideways. 0xFFFF is reserved (it once meant a miss)
// and is ignored.
//----------------------------------------------------------------------
namespace
{
	struct FLOATING_DAMAGE
	{
		TYPE_OBJECTID	id;
		int				damage;
		DWORD			start;
		int				zoneX, zoneY;	// the monster's zone pixel position, last seen
		int				headX, headY;	// top centre of its sprite, relative to that
		bool			placed;
		int				fan;		// 0..2: stacks a burst of hits on one monster, upwards
	};

	std::list<FLOATING_DAMAGE>	s_FloatingDamage;

	const DWORD	FLOATING_DAMAGE_MS		= 900;		// on screen this long
	const int	FLOATING_DAMAGE_RISE	= 36;		// and rises this far
	const int	FLOATING_DAMAGE_MISS	= 0xFFFF;	// reserved: ignored, no Miss indicator
	const int	FLOATING_DAMAGE_LIFT	= 16;		// the text's height: it sits ON the head
	const int	FLOATING_DAMAGE_STACK	= 14;		// between hits that land together

	// firstZonePixel is the zone pixel at screen (0,0) in the frame that
	// recorded the creature's screen rect
	bool PlaceFloatingDamage(FLOATING_DAMAGE& fd, const POINT& firstZonePixel)
	{
		MCreature* pCreature = (g_pZone != NULL) ? g_pZone->GetCreature(fd.id) : NULL;

		if (pCreature == NULL)
			return false;

		const RECT& r = pCreature->GetScreenRect();

		if (r.right <= r.left)		// never drawn yet
			return false;

		fd.zoneX	= pCreature->GetPixelX();
		fd.zoneY	= pCreature->GetPixelY();
		fd.headX	= (r.left + r.right) / 2 + firstZonePixel.x - fd.zoneX;
		fd.headY	= r.top + firstZonePixel.y - fd.zoneY;
		fd.placed	= true;
		return true;
	}

	// tracks the monster while it is still in the zone; once it is gone the
	// number stays where the monster was last seen
	void FollowFloatingDamage(FLOATING_DAMAGE& fd)
	{
		MCreature* pCreature = (g_pZone != NULL) ? g_pZone->GetCreature(fd.id) : NULL;

		if (pCreature == NULL)
			return;

		fd.zoneX = pCreature->GetPixelX();
		fd.zoneY = pCreature->GetPixelY();
	}
}

void
MTopView::AddFloatingDamage(TYPE_OBJECTID id, int damage)
{
	if (damage == FLOATING_DAMAGE_MISS)		// the user wants hits only
		return;

	FLOATING_DAMAGE fd;
	fd.id		= id;
	fd.damage	= damage;
	fd.start	= g_CurrentTime;
	fd.zoneX	= 0;
	fd.zoneY	= 0;
	fd.headX	= 0;
	fd.headY	= 0;
	fd.placed	= false;

	// several hits on one monster at once fan out instead of stacking
	int live = 0;

	for (std::list<FLOATING_DAMAGE>::const_iterator it = s_FloatingDamage.begin(); it != s_FloatingDamage.end(); ++it)
	{
		if (it->id == id)
			live++;
	}

	fd.fan = live % 3;

	// place it now: a killing blow can remove the monster before the next frame
	PlaceFloatingDamage(fd, m_FirstZonePixel);

	s_FloatingDamage.push_back(fd);

	while (s_FloatingDamage.size() > 64)
		s_FloatingDamage.pop_front();
}

void
MTopView::DrawFloatingDamage()
{
	PrintInfo* pInfo = g_ClientPrintInfo[FONTID_LARGE_CHAT];

	std::list<FLOATING_DAMAGE>::iterator it = s_FloatingDamage.begin();

	while (it != s_FloatingDamage.end())
	{
		const DWORD age = g_CurrentTime - it->start;

		if (age >= FLOATING_DAMAGE_MS)
		{
			it = s_FloatingDamage.erase(it);
			continue;
		}

		// the head offset is measured once; after that only the monster's
		// position is followed - re-measuring the sprite rect made it jitter
		if (!it->placed)
			PlaceFloatingDamage(*it, m_FirstZonePixel);
		else
			FollowFloatingDamage(*it);

		if (!it->placed || pInfo == NULL)
		{
			++it;
			continue;
		}

		char szText[16];
		sprintf(szText, "%d", it->damage);

		// zone pixels back to screen pixels with this frame's scroll
		const int width	= g_GetStringWidth(szText, pInfo->hfont);
		const int rise	= (int)(age * FLOATING_DAMAGE_RISE / FLOATING_DAMAGE_MS);
		const int headX	= it->zoneX + it->headX - m_FirstZonePixel.x;
		const int headY	= it->zoneY + it->headY - m_FirstZonePixel.y;
		const int x		= headX - width / 2;
		const int y		= headY - FLOATING_DAMAGE_LIFT - it->fan * FLOATING_DAMAGE_STACK - rise;

		// the node must own its string: DRAWTEXT_NODE keeps only the pointer
		AddText(new DRAWTEXT_NODE_HEAP(x, y, szText, RGB(255, 225, 110),
			FONTID_LARGE_CHAT, FLAG_DRAWTEXT_OUTLINE));

		++it;
	}
}


//----------------------------------------------------------------------
// DrawTextList
//----------------------------------------------------------------------
// Unlock???¿??? ??????? ???????.
//----------------------------------------------------------------------
void
MTopView::DrawTextList()
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawTextList");
#endif

	//DRAWTEXT_PQ::iterator iText = m_pqDrawText.begin();

	//--------------------------------------------------------
	// 3D ???? ??????...
	// ??? ??~~ ????.. --;
	// Lock/Unlock?? ?? ???? ?????..
	//
	// ?????! 
	// ???? ???? ??, ??? ?????? box?? ?????? ???. ??...
	//--------------------------------------------------------
	/*
	if (CDirect3D::IsHAL())
	{
		m_pSurface->Unlock();

		// ??? node?? ?????.
		while (iText != m_pqDrawText.end())
		{
			DRAWTEXT_NODE* pNode = *iText;

			//--------------------------------------------------------
			// ??????µ?? ???? ?????? ???? ???
			//--------------------------------------------------------
			RECT rect = { pNode->GetX(), pNode->GetY(),
						pNode->GetXPlusWidth(), pNode->GetY(),15 };

			// ???? ??? ???..
			DrawBox3D(&rect, 0x7000);

			iText++;
		}

		m_pSurface->Lock();

		// ??? ó??????..
		iText = m_pqDrawText.begin();
	}
	*/

	//m_pSurface->Unlock();
	//--------------------------------------------------------
	// ??? node?? ?????? ?????.
	//--------------------------------------------------------
	RECT rect2;

	while (!m_pqDrawText.empty())
	{
		DRAWTEXT_NODE* pNode = m_pqDrawText.top();

		//--------------------------------------------------------
		// Box?? ???????
		//--------------------------------------------------------
		if (pNode->IsExistBox())
		{
			//--------------------------------------------------------
			// ??????µ?? ???? ?????? ???? ???
			//--------------------------------------------------------
			RECT rect = pNode->GetBox();

			//--------------------------------------------------------
			// ?????? ????? ???
			//--------------------------------------------------------
			if (pNode->IsTransBox())
			{
				//--------------------------------------------------------
				// 3D ?????? ???
				//--------------------------------------------------------
//				if (CDirect3D::IsHAL())
//				{
//					// Unlock / lock... ????~~ ??? ?ð? ???? ?????????..
//					//m_pSurface->Unlock();
//
//					// ??? ???..
//					#ifdef OUTPUT_DEBUG_DRAW_3DBOX
//						DEBUG_ADD_FORMAT("TextList-Box3D( %d, %d, %d, %d ), color=%x",
//														rect.left, rect.top, rect.right, rect.bottom, pNode->GetBoxColor());
//					#endif
//
//					// ??? ???
//					WORD boxColor = pNode->GetBoxColor();
//
//					rect2 = rect;
//					DrawBox3D(&rect2, boxColor);
//
//					if (pNode->IsExistBoxOutline())
//					{
//						// ??? ??? ?????
//						rect2 = rect;
//						DRAW_CHAT_BOX_OUTLINE( rect2, pNode->GetBoxOutlineColor() )
//
//						
//						// XXXXXX
//						// XXXXXX
//						// .XXXX.
//						// .XXXX.
//						// ..XX..
//						// ..XX..
//
//						rect2.left	= rect.left + CHAT_BOX_TAIL_X;
//						rect2.top	= rect.bottom;
//						rect2.right = rect.left + CHAT_BOX_TAIL_X + 6;
//						rect2.bottom = rect.bottom + 2;		
//						DrawBox3D(&rect2, boxColor);
//
//						rect2.left	= rect.left + CHAT_BOX_TAIL_X+1;
//						rect2.top	= rect.bottom + 2;
//						rect2.right = rect.left + CHAT_BOX_TAIL_X+1 + 4;
//						rect2.bottom = rect.bottom + 2 + 2;
//						DrawBox3D(&rect2, boxColor);
//
//						rect2.left	= rect.left + CHAT_BOX_TAIL_X+2;
//						rect2.top	= rect.bottom + 4;
//						rect2.right = rect.left + CHAT_BOX_TAIL_X+2 + 2;
//						rect2.bottom = rect.bottom + 4 + 2;
//						DrawBox3D(&rect2, boxColor);
//
//						// ??
//						m_pSurface->Lock();
//						DRAW_CHAT_BOX_TAIL_LOCKED( rect, pNode->GetBoxOutlineColor() );
//						m_pSurface->Unlock();
//					}
//
//					//m_pSurface->Lock();
//				}
//				else
				{
					WORD boxColor = pNode->GetBoxColor();

					if (pNode->IsExistBoxOutline())
					{
						WORD color = pNode->GetBoxOutlineColor();

						// ??? ??? ?????
						rect2 = rect;
						DRAW_CHAT_BOX_OUTLINE(rect2, color)

							// Lock
							m_pSurface->Lock();

						WORD boxColor = pNode->GetBoxColor();

						// ??? ?????
						rect2 = rect;
						DRAW_ALPHA_BOX_2D(rect2, boxColor);

						// ???????? ???
						if (boxColor == 0)
						{
							// XXXXXX
							// XXXXXX
							// .XXXX.
							// .XXXX.
							// ..XX..
							// ..XX..

							// 2D 5:6:5
							if (CDirectDraw::Is565())
							{
								rect2 = rect;
								m_pSurface->GammaBox565(&rect2, 15);

								rect2.left = rect.left + CHAT_BOX_TAIL_X;
								rect2.top = rect.bottom;
								rect2.right = rect.left + CHAT_BOX_TAIL_X + 6;
								rect2.bottom = rect.bottom + 2;
								m_pSurface->GammaBox565(&rect2, 15);

								rect2.left = rect.left + CHAT_BOX_TAIL_X + 1;
								rect2.top = rect.bottom + 2;
								rect2.right = rect.left + CHAT_BOX_TAIL_X + 1 + 4;
								rect2.bottom = rect.bottom + 2 + 2;
								m_pSurface->GammaBox565(&rect2, 15);

								rect2.left = rect.left + CHAT_BOX_TAIL_X + 2;
								rect2.top = rect.bottom + 4;
								rect2.right = rect.left + CHAT_BOX_TAIL_X + 2 + 2;
								rect2.bottom = rect.bottom + 4 + 2;
								m_pSurface->GammaBox565(&rect2, 15);
							}
							// 2D 5:5:5
							else
							{
								rect2 = rect;
								m_pSurface->GammaBox555(&rect2, 15);

								rect2.left = rect.left + CHAT_BOX_TAIL_X;
								rect2.top = rect.bottom;
								rect2.right = rect.left + CHAT_BOX_TAIL_X + 6;
								rect2.bottom = rect.bottom + 2;
								m_pSurface->GammaBox555(&rect2, 15);

								rect2.left = rect.left + CHAT_BOX_TAIL_X + 1;
								rect2.top = rect.bottom + 2;
								rect2.right = rect.left + CHAT_BOX_TAIL_X + 1 + 4;
								rect2.bottom = rect.bottom + 2 + 2;
								m_pSurface->GammaBox555(&rect2, 15);

								rect2.left = rect.left + CHAT_BOX_TAIL_X + 2;
								rect2.top = rect.bottom + 4;
								rect2.right = rect.left + CHAT_BOX_TAIL_X + 2 + 2;
								rect2.bottom = rect.bottom + 4 + 2;
								m_pSurface->GammaBox555(&rect2, 15);
							}


						}
						// ??? ???? ???
						else
						{
							m_pSurface->BltColorAlpha(&rect, boxColor, 15);
						}

						// ??
						DRAW_CHAT_BOX_TAIL_LOCKED(rect, color);

						m_pSurface->Unlock();
					}
					else
					{
						m_pSurface->Lock();

						DRAW_ALPHA_BOX_2D(rect, boxColor);

						m_pSurface->Unlock();
					}
				}
			}
			//--------------------------------------------------------
			// ?????? ????? ??? ???
			//--------------------------------------------------------
			else
			{
				m_pSurface->FillRect(&rect, pNode->GetBoxColor());
			}
		}

		//--------------------------------------------------------
		// ????? ????? ??? ???..
		//--------------------------------------------------------
		const char* pString = pNode->GetString();

		if (pString != NULL && pString[0] != '\0')
		{
			//--------------------------------------------------------
			// ??? ???
			//--------------------------------------------------------
			int x = pNode->GetX();
			int y = pNode->GetY();

			//--------------------------------------------------------
			// font ????
			//--------------------------------------------------------	
			PrintInfo* pPrintInfo = g_ClientPrintInfo[pNode->GetFont()];

			//--------------------------------------------------------
			// flag?? ???? ó???? ????? ???....				
			//--------------------------------------------------------
			if (pNode->IsOutline())
			{
				// ???? color????
				pPrintInfo->text_color = (~pNode->GetColor()) & 0x00FFFFFF;

				// ???
				g_Print(x - 1, y - 1, pString, pPrintInfo);
				g_Print(x + 1, y + 1, pString, pPrintInfo);
			}

			// text color????
			pPrintInfo->text_color = pNode->GetColor();

			//--------------------------------------------------------
			// string ???
			//--------------------------------------------------------
			g_Print(x, y, pString, pPrintInfo);
		}

		// ??????? ???????.
		delete pNode;

		// ????..
		m_pqDrawText.pop();
	}

	// ??? node?? ???????.
	//m_pqDrawText.clear();

	// Lock????
	//m_pSurface->Lock();

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawTextList");
#endif
}

//----------------------------------------------------------------------
// Clear LightBufferFilter
//----------------------------------------------------------------------
// LightBuffer?? ???????.
// ???????? Player?? ?þ?? ??????? ???.
//----------------------------------------------------------------------
void
MTopView::ClearLightBufferFilter3D()
{
	//if (CDirect3D::IsHAL() && 
	//if	(m_DarkBits)
	{
		//--------------------------------------------------
		// ???? ?? ????
		//--------------------------------------------------
		BYTE DarkColor;// = 0;
		///*
		//int maxLight[MAX_LIGHT3D_SET]	= { 7, 11, 14 };
		//int maxCenter[MAX_LIGHT3D_SET]	= { 3, 6, 9 };
		/*
		switch (m_DarkBits)
		{
			case 0 : DarkColor = 0; break;
			case 1 : DarkColor = 4; break;
			case 2 : DarkColor = 8; break;
			case 3 : DarkColor = 14; break;
			case 4 : DarkColor = 14; break;
		}
		*/

		//DarkColor = m_DarkBits;
		int darkBits = m_DarkBits;

		if (g_pPlayer->IsInDarkness() && (!(g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || g_pZone->GetID() == 3001))
		{
			darkBits = 15;
		}

		DarkColor = max(0, darkBits - (g_pPlayer->GetLightSight() - g_pPlayer->GetTimeLightSight()));

		//--------------------------------------------------
		// ???? - ??? ??ü?? ä?? ??..
		//--------------------------------------------------
		BYTE* pBuffer;

		int i = SCREENLIGHT_HEIGHT - 1;
		do {
			pBuffer = m_LightBufferFilter.GetFilter(i);
			memset(pBuffer, DarkColor, SCREENLIGHT_WIDTH);
		} while (i--);

		m_nLight = 0;
	}
}

//----------------------------------------------------------------------
// Clear LightBufferFilter
//----------------------------------------------------------------------
// LightBuffer?? ???????.
// ???????? Player?? ?þ?? ??????? ???.
//----------------------------------------------------------------------
void
MTopView::ClearLightBufferFilter2D()
{
	//if (1)//CDirect3D::IsHAL() && m_b3DLight && m_DarkBits)
	{
		//--------------------------------------------------
		// ???? ?? ????
		//--------------------------------------------------
		BYTE DarkColor;// = 0;
		///*
		//int maxLight[MAX_LIGHT3D_SET]	= { 7, 11, 14 };
		//int maxCenter[MAX_LIGHT3D_SET]	= { 3, 6, 9 };
		/*
		switch (m_DarkBits)
		{
			case 0 : DarkColor = 31; break;		// 31-0
			case 1 : DarkColor = 23; break;	// 31-4*2
			case 2 : DarkColor = 15; break;	// 31-8*2
			case 3 : DarkColor = 5; break;	// 31-14*2
			case 4 : DarkColor = 5; break;	// 31-14*2
		}
		*/
		int darkBits = m_DarkBits;

		if (g_pPlayer->IsInDarkness() && (!(g_pPlayer->HasEffectStatus(EFFECTSTATUS_LIGHTNESS) || g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLAME_SIGHT)) || g_pZone->GetID() == 3001))
		{
			darkBits = 15;
		}

		DarkColor = 31 - (darkBits << 1);

		DarkColor = max(0, DarkColor - ((g_pPlayer->GetLightSight() - g_pPlayer->GetTimeLightSight()) << 1));

		//--------------------------------------------------
		// ???? - ??? ??ü?? ä?? ??..
		//--------------------------------------------------
		BYTE* pBuffer;

		int i = SCREENLIGHT_HEIGHT - 1;
		do {
			pBuffer = m_LightBufferFilter.GetFilter(i);
			memset(pBuffer, DarkColor, SCREENLIGHT_WIDTH);
		} while (i--);

		m_nLight = 0;
	}
}

//----------------------------------------------------------------------
// Add LightFilter To Screen
//----------------------------------------------------------------------
// LightFilter?? ??????.
// Screen????? LightBuffer?? ?µ??? ???????.
//
// (CLIPSURFACE_WIDTH, CLIPSURFACE_WIDTH) 
//  --> (SCREENLIGHT_WIDTH, SCREENLIGHT_HEIGHT)??..
//----------------------------------------------------------------------
void
MTopView::AddLightFilter2D(int x, int y, BYTE range, bool bMapPixel, bool bForceLight)
{
	if (!bForceLight)
	{
		range = min(max(g_pPlayer->GetSight() - 3, 0), range);
	}

	if (//CDirect3D::IsHAL() && m_b3DLight && 
		!g_pPlayer->IsInDarkness()
		&& range != 0
		&& (m_DarkBits || g_pPlayer->IsInDarkness())
		&& m_nLight < g_pClientConfig->MAX_LIGHT_DRAW
		|| bForceLight) // ?????? ????????? ??
	{
		// tile ??? ???? 
		 //x += TILE_X_HALF;
		 //y += TILE_Y_HALF;

		// Light Filter?? ?????? ???? ????? ???.
		if (range >= m_LightFTP.GetSize())
		{
			range = m_LightFTP.GetSize() - 1;
		}

		//-----------------------------------------------------
		// range?? ???? ????? lightFilter?? ??????????.
		//-----------------------------------------------------		
		int filterID = range;

		//-----------------------------------------------------
		// LightFilter ??? ???? ????
		/// ?????, filter??? (fx,fy)?? ???? ???..
		// filter?? ????? (x,y)?? ?????? ????? ???.
		//-----------------------------------------------------
		int fx, fy;

		// (x,y)?? ?? ????????? ???? ????? ????? ?????.
		if (bMapPixel)
		{
			// mapPixel --> screen
			POINT screenPoint = PixelToScreen(x, y);

			// screen -> filter
			fx = (float)screenPoint.x / s_LightWidth;
			fy = (float)screenPoint.y / s_LightHeight;
		}
		else
		{
			// screen -> filter
			fx = (float)x / s_LightWidth;
			fy = (float)y / s_LightHeight;
		}

		// filter ??? ?????
		fx = fx - (m_LightFTP[filterID].GetWidth() >> 1);
		fy = fy - (m_LightFTP[filterID].GetHeight() >> 1);

		//-----------------------------------------------------
		// BufferFilter?? Light filter?? ??????.
		//-----------------------------------------------------
		m_LightBufferFilter.BltFilterAdd(fx, fy, m_LightFTP[filterID]);

		m_nLight++;
	}
}

//----------------------------------------------------------------------
// Add LightFilter To Screen
//----------------------------------------------------------------------
// LightFilter?? ??????.
// Screen????? LightBuffer?? ?µ??? ???????.
//
// (CLIPSURFACE_WIDTH, CLIPSURFACE_WIDTH) 
//  --> (SCREENLIGHT_WIDTH, SCREENLIGHT_HEIGHT)??..
//----------------------------------------------------------------------
void
MTopView::AddLightFilter3D(int x, int y, BYTE range, bool bMapPixel, bool bForceLight)
{
	if (!bForceLight)
	{
		range = min(max(g_pPlayer->GetSight() - 3, 0), range);
	}

	if (//CDirect3D::IsHAL() && 
		!g_pPlayer->IsInDarkness()
		&& range != 0
		&& (m_DarkBits || g_pPlayer->IsInDarkness())
		&& m_nLight < g_pClientConfig->MAX_LIGHT_DRAW
		|| bForceLight)	// ?????? ????????? ??
	{
		// tile ??? ???? 
		//x += TILE_X_HALF;
		//y += TILE_Y_HALF;

		// Light Filter?? ?????? ???? ????? ???.
		if (range >= m_LightFTP.GetSize())
		{
			range = m_LightFTP.GetSize() - 1;
		}

		//-----------------------------------------------------
		// range?? ???? ????? lightFilter?? ??????????.
		//-----------------------------------------------------		
		int filterID = range;

		//-----------------------------------------------------
		// LightFilter ??? ???? ????
		/// ?????, filter??? (fx,fy)?? ???? ???..
		// filter?? ????? (x,y)?? ?????? ????? ???.
		//-----------------------------------------------------
		int fx, fy;

		// (x,y)?? ?? ????????? ???? ????? ????? ?????.
		if (bMapPixel)
		{
			// mapPixel --> screen
			POINT screenPoint = PixelToScreen(x, y);

			// screen -> filter
			fx = (float)screenPoint.x / s_LightWidth;
			fy = (float)screenPoint.y / s_LightHeight;
		}
		else
		{
			// screen -> filter
			fx = (float)x / s_LightWidth;
			fy = (float)y / s_LightHeight;
		}

		// filter ??? ?????
		fx = fx - (m_LightFTP[filterID].GetWidth() >> 1);
		fy = fy - (m_LightFTP[filterID].GetHeight() >> 1);

		//-----------------------------------------------------
		// BufferFilter?? Light filter?? ??????.
		//-----------------------------------------------------
		m_LightBufferFilter.BltFilterSub(fx, fy, m_LightFTP[filterID]);

		m_nLight++;
	}
}

//----------------------------------------------------------------------
// Add CreatureOutput All
//----------------------------------------------------------------------
// m_pZone?? Creature map?? ??°??~ ???
//----------------------------------------------------------------------
// OutputMap?????? ID?? Creature??..
//
//    2 bytes + 2 bytes    = 4 bytes ?? ??????? ???.
// [pixel y???][object id]
//
// ??? creature?? ??????? ?????? ????? ???????..
// FakeCreature?? tile???? ????????.. ?????? ???? ?????????.
//----------------------------------------------------------------------
void
MTopView::AddOutputCreatureAll()
{
	// ????
	m_mapCreature.clear();

	// [?????3] ?? ??? ???????? ??? ?????? ???.
	if (g_pPlayer->IsInCasket())
	{
		m_bTileSearchForCreature = false;

		QWORD key = GetOutputCreatureID(g_pPlayer);

		// (key??, pCreature)?? ??????.
		m_mapCreature.insert(
			CREATURE_OUTPUT_MAP::value_type(key, g_pPlayer)
		);

		return;
	}


	MZone::CREATURE_MAP::const_iterator iCreature;

	MCreature* pCreature;

	int num = m_pZone->GetCreatureNumber();

	// 20?? ??? ?????? ... 
	// ??? ?????°? ????? ???.. ???????? - -;
	if (num >= 20)
	{
		//		m_bTileSearchForCreature = true;		
		m_bTileSearchForCreature = false;
	}
	else
	{
		m_bTileSearchForCreature = false;
	}

	//---------------------------------------------------------
	// Tile?? ?????? ?????? ??? ????..
	// ??? Creature?? OutputMap?? ?????? ???.
	//---------------------------------------------------------
	if (!m_bTileSearchForCreature)
	{
		iCreature = m_pZone->GetCreatureBegin();

		for (int i = 0; i < num; i++)
		{
			pCreature = iCreature->second;

			// key ?? = [pixel Y???][object id]
			QWORD key = GetOutputCreatureID(pCreature);

			// (key??, pCreature)?? ??????.
			m_mapCreature.insert(
				CREATURE_OUTPUT_MAP::value_type(key, pCreature)
			);

			// ???? Creature
			iCreature++;
		}
	}

	//---------------------------------------------------------
	// Fake Creature
	//---------------------------------------------------------
	iCreature = m_pZone->GetFakeCreatureBegin();
	num = m_pZone->GetFakeCreatureNumber();

	//---------------------------------------------------------
	// ??? Creature?? OutputMap?? ?????? ???.
	//---------------------------------------------------------
	for (int i = 0; i < num; i++)
	{
		pCreature = iCreature->second;

		// key ?? = [pixel Y???][object id]
		QWORD key = GetOutputCreatureID(pCreature);

		// (key??, pCreature)?? ??????.
		m_mapCreature.insert(
			CREATURE_OUTPUT_MAP::value_type(key, pCreature)
		);

		// ???? Creature
		iCreature++;
	}
}

//----------------------------------------------------------------------
// Add OutputCreature
//----------------------------------------------------------------------
// pCreature?? ??????.
//----------------------------------------------------------------------
bool
MTopView::AddOutputCreature(MCreature* pCreature)
{
	// key?? ????
	QWORD key = GetOutputCreatureID(pCreature);

	// ??? ????? ???????? ?????? ???.
	if (m_mapCreature.find(key) == m_mapCreature.end())
	{
		// (key??, pCreature)?? ??????.
		m_mapCreature.insert(
			CREATURE_OUTPUT_MAP::value_type(key, pCreature)
		);

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// Remove OutputCreature
//----------------------------------------------------------------------
// id?? ??????? creature?? ???????.
//----------------------------------------------------------------------
bool
MTopView::RemoveOutputCreature(MCreature* pCreature)
{
	// ??? ????? ???????? ?????? ????
	CREATURE_OUTPUT_MAP::iterator iCreature
		= m_mapCreature.find(GetOutputCreatureID(pCreature));

	if (iCreature != m_mapCreature.end())
	{
		m_mapCreature.erase(iCreature);

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// ID?? id?? Creature?? ????? y0???? y1???? ???? update???.
//----------------------------------------------------------------------
// id?? ??????? creature?? ????(??????????id)?? Update???.
// ??????.. ???????.. ??? ?????? ???.
//----------------------------------------------------------------------
bool
MTopView::UpdateOutputCreature(TYPE_OBJECTID id, int y0, int y1)
{
	// ??? ????? ???????? ?????? update
	CREATURE_OUTPUT_MAP::iterator iCreature
		= m_mapCreature.find(GetOutputCreatureID(id, y0));

	if (iCreature != m_mapCreature.end())
	{
		MCreature* pCreature = (*iCreature).second;

		// ????
		m_mapCreature.erase(iCreature);

		// (key??, pCreature)?? ??? ??????.
		m_mapCreature.insert(
			CREATURE_OUTPUT_MAP::value_type(
				GetOutputCreatureID(id, y1),	// key??
				pCreature
			)
		);

		return true;
	}

	return false;
}

//----------------------------------------------------------------------
// DrawBox3D
//----------------------------------------------------------------------
// ?????? ?????? ????? ?????. 3D
//----------------------------------------------------------------------
//void
//MTopView::DrawBox3D(RECT* pRect, WORD pixel)
//{
//	#ifdef OUTPUT_DEBUG_DRAW_PROCESS
//		DEBUG_ADD( "Start DrawBox3D" );
//	#endif
//
//
//	CSpriteSurface* pUsingSurface = NULL;
//
//	//-------------------------------------------------
//	// Color Black Bar
//	//-------------------------------------------------
//	if (pixel==m_ColorBlackHalf)
//	{
//		pUsingSurface = m_p3DBoxBlackSurface;
//	}
//	//-------------------------------------------------
//	// Color HP Bar
//	//-------------------------------------------------
//	else if (pixel==m_ColorHPBar)
//	{
//		pUsingSurface = m_p3DBoxHPSurface;
//	}
//	//-------------------------------------------------
//	// ??? ?????? ???.. ??? ????..
//	//------------------------------------------------		
//	else
//	{		
//		pUsingSurface = m_p3DBoxSurface;
//
//		if (pixel != m_3DBoxCurrentPixel)
//		{
//			Set3DBoxColor( pixel );
//
//			//m_3DBoxCurrentPixel = pixel;
//		}
//	}
//	
//	DRAW_TEXTURE_SURFACE( pUsingSurface, pRect )
//	else
//	{
//		#ifdef OUTPUT_DEBUG_DRAW_PROCESS
//			DEBUG_ADD( "Failed DrawBox3D" );
//		#endif	
//	}
//
//	#ifdef OUTPUT_DEBUG_DRAW_PROCESS
//		DEBUG_ADD( "End DrawBox3D" );
//	#endif
//}

//----------------------------------------------------------------------
// Draw Alpha Box
//----------------------------------------------------------------------
// 2D/3D????? ???????...
// ?????? Lock/Unlock ???¿? ???????... ??????.
//
// m_pSurface?? 
// (r,g,b)?? alpha??(1~31)?? ???? pRect?????? ?????? box?? ?????.
//----------------------------------------------------------------------
void
MTopView::DrawAlphaBox(RECT* pRect, BYTE r, BYTE g, BYTE b, BYTE alpha)
{
	WORD color;

	//--------------------------------------------------------
	// Win2000?? ???.. ???? ??û?? ???????? ?????.. ?? ??????
	// ?????.. AlphaBox?? ??????? ??´?.
	// ???? ???.. ??? ?????? ??????... ???? ???????. - -;
	//--------------------------------------------------------
	/*
	if (g_WindowsNT)
	{
		//------------------------------------------------
		// Unlock ???·? ?????.
		//------------------------------------------------
		BOOL bLock = m_pSurface->IsLock();
		if (bLock)
		{
			m_pSurface->Unlock();
		}

		color = CDirectDraw::Color(r,g,b);

		m_pSurface->FillRect(pRect, color);

		//------------------------------------------------
		// ???? lock ?? ???¸? ??? ???????.
		//------------------------------------------------
		if (bLock)
		{
			m_pSurface->Lock();
		}
	}
	*/
	//--------------------------------------------------------
	// 98?? ???..
	//--------------------------------------------------------
	//else
	{
		//--------------------------------------------------------
		// 3D ?????? ???
		//--------------------------------------------------------
//		if (CDirect3D::IsHAL())
//		{
//			//------------------------------------------------
//			// Unlock ???·? ?????.
//			//------------------------------------------------
//			BOOL bLock = m_pSurface->IsLock();
//			if (bLock)
//			{
//				m_pSurface->Unlock();
//			}
//
//			//------------------------------------------------
//			// 4:4:4:4
//			//------------------------------------------------
//			//						1 1110b
//			WORD color = ((32-alpha) & 0x1E) << 11;		// ((32-alpha)>>1) << 12
//
//			color = color 
//					| (((WORD)r>>1) << 8)
//					| (((WORD)g>>1) << 4)
//					| ((WORD)b>>1);
//
//			
//			CSpriteSurface* pUsingSurface = NULL;
//
//			//-------------------------------------------------
//			// Color Black Bar
//			//-------------------------------------------------
//			if (color==m_ColorBlackHalf)
//			{
//				pUsingSurface = m_p3DBoxBlackSurface;
//			}
//			//-------------------------------------------------
//			// Color HP Bar
//			//-------------------------------------------------
//			else if (color==m_ColorHPBar)
//			{
//				pUsingSurface = m_p3DBoxHPSurface;
//			}
//			//-------------------------------------------------
//			// ??? ?????? ???.. ??? ????..
//			//------------------------------------------------		
//			else
//			{		
//				pUsingSurface = m_p3DBoxSurface;
//
//				if (color != m_3DBoxCurrentPixel)
//				{
//					Set3DBoxColor( color );
//
//					//m_3DBoxCurrentPixel = pixel;
//				}
//			}
//
//			DRAW_TEXTURE_SURFACE( pUsingSurface, pRect )			
//
//			//------------------------------------------------
//			// ???? lock ?? ???¸? ??? ???????.
//			//------------------------------------------------
//			if (bLock)
//			{
//				m_pSurface->Lock();
//			}
//		}
//		//--------------------------------------------------------
//		// 2D
//		//--------------------------------------------------------
//		else
		{
			//------------------------------------------------
			// Lock ???·? ?????.
			//------------------------------------------------
			BOOL bUnlock = !m_pSurface->IsLock();
			if (bUnlock)
			{
				m_pSurface->Lock();
			}

			color = CDirectDraw::Color(r, g, b);

			//-------------------------------------------------
			// ?????????.. ???? ???~
			//-------------------------------------------------
			if (color == 0)
			{
				// 2D 5:6:5
				if (CDirectDraw::Is565())
				{
					m_pSurface->GammaBox565(pRect, alpha);
				}
				// 2D 5:5:5
				else
				{
					m_pSurface->GammaBox555(pRect, alpha);
				}
			}
			//-------------------------------------------------
			// ????...
			//-------------------------------------------------
			else
			{
				m_pSurface->BltColorAlpha(pRect, color, alpha);
			}

			//------------------------------------------------
			// ?????? Lock ???·? ???????.
			//------------------------------------------------
			if (bUnlock)
			{
				m_pSurface->Unlock();
			}
		}
	}
}

//----------------------------------------------------------------------
// Draw Information
//----------------------------------------------------------------------
// Debug?? ????? ?????? ??????.
//
// ???? ???????? Unlock????..
//----------------------------------------------------------------------
void
MTopView::DrawInformation()
{
	if (g_pEventManager->GetEventByFlag(EVENTFLAG_NOT_DRAW_INFORMATION))
		return;

	char str[128];

	//-----------------------------------------------------------------
	//							Lock
	//-----------------------------------------------------------------	
	//if (!m_pSurface->Lock()) return;
	// Surface?? ?????? ????????.
	//S_SURFACEINFO		SurfaceInfo;
	//SetSurfaceInfo(&SurfaceInfo, m_pSurface->GetDDSD());

	//-----------------------------------------------------------------
	// Font ????
	//-----------------------------------------------------------------
	PrintInfo* pPrintInfo = g_ClientPrintInfo[FONTID_INFO];

	int y;//2;

	if (g_pPlayer->IsSlayer())
	{
		y = 45;
	}
	else
	{
		y = 48;
	}


	//-----------------------------------------------------------------
	// [ TEST CODE ]
	//-----------------------------------------------------------------
	// Zone??? ???
	//-----------------------------------------------------------------
	/* // 2001.7.14 ???ó??
	if (g_pUserOption->DrawZoneName)
	{
		if (g_bZonePlayerInLarge)
		{
			sprintf(str, (*g_pGameStringTable)[STRING_DRAW_ZONE_NAME].GetString(),
							g_pZoneTable->Get( g_nZoneLarge )->Name.GetString(),
							g_pPlayer->GetX(), g_pPlayer->GetY());
		}
		else
		{
			sprintf(str, (*g_pGameStringTable)[STRING_DRAW_ZONE_NAME].GetString(),
							g_pZoneTable->Get( g_nZoneSmall )->Name.GetString(),
							g_pPlayer->GetX(), g_pPlayer->GetY());
		}
		const COLORREF color	= RGB(18<<3, 14<<3, 30<<3);	//CDirectDraw::Color(18,14,30);
		const COLORREF bkcolor = RGB(0, 0, 0);				//CDirectDraw::Color(0,0,0);

		pPrintInfo->text_color	= bkcolor;
		g_Print(6, y+1, str, pPrintInfo);

		pPrintInfo->text_color	= color;
		g_Print(5, y, str, pPrintInfo);

		y += 18;
	}
	*/


	//-----------------------------------------------------------------
	// ???? ?ð? ???
	//-----------------------------------------------------------------
	/*
	static DWORD lastDisplayGameTime = g_CurrentTime;

	if (g_pUserOption->DrawGameTime)
	{
		//static WORD dayColor = CDirectDraw::Color(5,2,24);
		const COLORREF timeColor	= RGB(12<<3, 8<<3, 25<<3);	//CDirectDraw::Color(12,8,25);
		const COLORREF timeBkColor = RGB(0, 0, 0);				//CDirectDraw::Color(0,0,0);

		g_pGameTime->SetCurrentTime( g_CurrentTime );


		sprintf(str, (*g_pGameStringTable)[STRING_DRAW_GAME_TIME].GetString(),
						g_pGameTime->GetHour(),
						g_pGameTime->GetMinute(),
						g_pGameTime->GetSecond()
				);

		pPrintInfo->text_color = timeBkColor;
		g_Print(6, y+1, str, pPrintInfo);

		pPrintInfo->text_color = timeColor;
		g_Print(5, y, str, pPrintInfo);

		lastDisplayGameTime = g_CurrentTime;

		y += 18;
	}
	else
	{
		// 30??? ??????? ?ð??? ??????.
		if (g_CurrentTime - lastDisplayGameTime > 30000)
		{
			g_pGameTime->SetCurrentTime( g_CurrentTime );

			lastDisplayGameTime = g_CurrentTime;
		}
	}
	*/

	//-----------------------------------------------------------------
	//
	// Item ??????? ???
	//
	//-----------------------------------------------------------------
	y += 10;
	DrawItemBroken(5, y);

	y += 70;	// ItemBroken?? ???.. ??????? ???.. ????

	//-----------------------------------------------------------------
	//
	// Event Message ???
	//
	//-----------------------------------------------------------------
	int strY = 30;
	int strX = SURFACE_WIDTH / 2;


	// Event String ???
	DrawEventString(strX, strY);

	//-----------------------------------------------------------------
	//
	// System Message ???
	//
	//-----------------------------------------------------------------

	strY = y + 20;
	for (int c = 0; c < g_pSystemMessage->GetSize(); c++)
	{
		if ((*g_pSystemMessage)[c][0] != NULL)
		{
			const COLORREF color = RGB(29 << 3, 8 << 3, 12 << 3);	//CDirectDraw::Color(29,8,12);

			pPrintInfo->text_color = 0;
			g_Print(11, strY + 1, (*g_pSystemMessage)[c], pPrintInfo);

			pPrintInfo->text_color = color;
			g_Print(10, strY, (*g_pSystemMessage)[c], pPrintInfo);

			strY += 20;
		}
	}

	// 2008.07.17 zzi - EventMessage ???
	//		EventMessage?? ??µ? ?????? static???? ???? .. ??????? ???????? ???.
	static int EventMessageWidth = g_pUserInformation->iResolution_x / 2;
	static int EventMessageHeight = 20;	// ??????.. ??µ? ??????? ????? ????~~ ?????? ?????.
	static int EventMessageX = (g_pUserInformation->iResolution_x - EventMessageWidth) / 2;
	static int EventMessageY = g_pUserInformation->iResolution_y / 4;		// ???????.
	static RECT EventMessageRt = { EventMessageX, EventMessageY, EventMessageX + EventMessageWidth, EventMessageY + EventMessageHeight };

	PrintInfo* pEventInfo = g_ClientPrintInfo[FONTID_EVENTMSG];
	for (int c = 0; c < g_pEventMessage->GetSize(); c++)
	{
		if ((*g_pEventMessage)[c][0] != NULL)
		{
			const COLORREF color = RGB(128, 255, 0);
			pEventInfo->text_color = color;
			g_DrawText(&EventMessageRt, (*g_pEventMessage)[c], pEventInfo);
		}
	}
	//	GMMessage ???
	PrintInfo* pGMInfo = g_ClientPrintInfo[FONTID_LARGE_CHAT];
	for (int c = 0; c < g_pGMMessage->GetSize(); c++)
	{
		if ((*g_pGMMessage)[c][0] != NULL)
		{
			const COLORREF color = RGB(255, 255, 81);

			pGMInfo->text_color = 0;
			g_Print(11, strY + 1, (*g_pGMMessage)[c], pGMInfo);
			pGMInfo->text_color = color;
			g_Print(10, strY, (*g_pGMMessage)[c], pGMInfo);
			strY += 16;
		}
	}
	// 10????? ?????.. scroll
	static DWORD lastGMTime = g_CurrentTime;
	if (g_CurrentTime - lastGMTime >= (g_pClientConfig->DELAY_SYSTEMMESSAGE + 5000))
	{
		g_pGMMessage->Add("\0");
		lastGMTime = g_CurrentTime;
	}
	//-

	PrintInfo* pNoticeInfo = g_ClientPrintInfo[FONTID_LARGE_CHAT];
	for (int c = 0; c < g_pNoticeMessage->GetSize(); c++)
	{
		if ((*g_pNoticeMessage)[c][0] != NULL)
		{
			const COLORREF color = RGB(20 << 3, 31 << 3, 12 << 3);

			pNoticeInfo->text_color = 0;
			g_Print(11, strY + 1, (*g_pNoticeMessage)[c], pNoticeInfo);
			pNoticeInfo->text_color = color;
			g_Print(10, strY, (*g_pNoticeMessage)[c], pNoticeInfo);
			strY += 16;
		}
	}

	// 5????? ?????.. scroll
	static DWORD lastTime = g_CurrentTime;
	if (g_CurrentTime - lastTime >= g_pClientConfig->DELAY_SYSTEMMESSAGE)
	{
		g_pSystemMessage->Add("\0");
		lastTime = g_CurrentTime;
	}

	// 7????? ?????.. scroll
	static DWORD lastNoticeTime = g_CurrentTime;
	if (g_CurrentTime - lastNoticeTime >= (g_pClientConfig->DELAY_SYSTEMMESSAGE + 2000))
	{
		g_pNoticeMessage->Add("\0");
		lastNoticeTime = g_CurrentTime;
	}

	//-----------------------------------------------------------------
	//
	// Game Message ???
	//
	//-----------------------------------------------------------------
	// Right-aligned above the skill box, newest at the bottom.
	RECT skill_box;
	g_GetSkillBoxRect(&skill_box);

	// Clear of the skill box's tooltip, which opens upward from the box: the
	// tallest, Bless (two names, level and MP cost), is 80px.
	strY = skill_box.top - 80 - 24;

	for (int c = g_pGameMessage->GetSize() - 1; c >= 0; c--)
	{
		if ((*g_pGameMessage)[c][0] != NULL)
		{
			const COLORREF color = RGB(8 << 3, 28 << 3, 8 << 3);	//CDirectDraw::Color(29,8,12);

			strX = g_pUserInformation->iResolution_x - 10 - g_GetStringWidth((*g_pGameMessage)[c], pPrintInfo->hfont);

			pPrintInfo->text_color = 0;
			g_Print(strX + 1, strY + 1, (*g_pGameMessage)[c], pPrintInfo);

			pPrintInfo->text_color = color;
			g_Print(strX, strY, (*g_pGameMessage)[c], pPrintInfo);

			strY -= 20;
		}
	}

	// 5????? ?????.. scroll
	static DWORD gamelastTime = g_CurrentTime;
	if (g_CurrentTime - gamelastTime >= g_pClientConfig->DELAY_GAMEMESSAGE)
	{
		g_pGameMessage->Add("\0");
		gamelastTime = g_CurrentTime;
	}

	//-----------------------------------------------------------------
	//
	// ??? ?????? ???
	//
	//-----------------------------------------------------------------
	if (g_pCurrentMagazine != NULL)
	{
		POINT point = { 225, 9 };

		m_pSurface->Lock();
		m_pSurface->BltSprite(&point, &m_EtcSPK[SPRITEID_BULLET]);
		m_pSurface->Unlock();

		point.x += m_EtcSPK[SPRITEID_BULLET].GetWidth() + 2;
		point.y += m_EtcSPK[SPRITEID_BULLET].GetHeight() - 18;

		sprintf(str, "%d", g_pCurrentMagazine->GetNumber());

		const COLORREF bulletColor = RGB(30 << 3, 28 << 3, 30 << 3);

		pPrintInfo->text_color = 0;
		g_Print(point.x + 1, point.y + 1, str, pPrintInfo);

		pPrintInfo->text_color = bulletColor;
		g_Print(point.x, point.y, str, pPrintInfo);
	}

	//-----------------------------------------------------------------
	// ????? ???? ???
	//-----------------------------------------------------------------
#ifdef OUTPUT_DEBUG
	if (g_pDXInput->KeyDown(DIK_P) &&
		(g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
	{
		const int statusNum = 3;
		char statusName[statusNum][5] =
		{
			"STR",
			"DEX",
			"INT"
		};

		int statusValue[statusNum];

		statusValue[0] = g_pPlayer->GetSTR();
		statusValue[1] = g_pPlayer->GetDEX();
		statusValue[2] = g_pPlayer->GetINT();

		const COLORREF color = RGB(8 << 3, 28 << 3, 8 << 3);	//CDirectDraw::Color(29,8,12);


		strY = 50;

		for (int status = 0; status < statusNum; status++)
		{
			sprintf(str, "%s = %d", statusName[status], statusValue[status]);

			pPrintInfo->text_color = 0;
			g_Print(701, strY + 1, str, pPrintInfo);

			pPrintInfo->text_color = color;
			g_Print(700, strY, str, pPrintInfo);

			strY += 20;
		}
	}

#endif

	//-----------------------------------------------------------------
	//							UnLock
	//-----------------------------------------------------------------
	//m_pSurface->Unlock();
}

#if defined(OUTPUT_DEBUG) && defined(_LIB)
extern int g_SurfaceCount;
extern int g_EffectCount;
#endif

void
MTopView::DrawEventString(int& strX, int& strY)
{
	char str[256] = { 0, };
	char str2[256] = { 0, };

	// Delay String
	PrintInfo* pPrintInfo = g_ClientPrintInfo[FONTID_INFO];

	const MEvent* event = g_pEventManager->GetEventByFlag(EVENTFLAG_SHOW_DELAY_STRING, 0);

	if (event != NULL)
	{
		for (int eventMessageCount = 0; eventMessageCount < (event->m_StringsID.empty() ? 1 : event->m_StringsID.size()); eventMessageCount++)
		{
			str[0] = NULL;

			switch (event->eventFlag & EVENTFLAG_SHOW_DELAY_STRING)
			{
			case EVENTFLAG_SHOW_STRING:
				if (!event->m_StringsID.empty())
				{
					if (event->m_StringsID[eventMessageCount] < MAX_GAME_STRING && (*g_pGameStringTable)[event->m_StringsID[eventMessageCount]].GetString() != NULL)
						strcpy(str, (*g_pGameStringTable)[event->m_StringsID[eventMessageCount]].GetString());
				}
				break;

			case EVENTFLAG_SHOW_DELAY:
				sprintf(str, "%d", (event->eventDelay - (GetTickCount() - event->eventStartTickCount) + 999) / 1000);
				break;

			case EVENTFLAG_SHOW_DELAY_STRING:
				if (!event->m_StringsID.empty())
				{
					if (event->m_StringsID[eventMessageCount] < MAX_GAME_STRING && (*g_pGameStringTable)[event->m_StringsID[eventMessageCount]].GetString() != NULL)
						sprintf(str, (*g_pGameStringTable)[event->m_StringsID[eventMessageCount]].GetString(), (event->eventDelay - (GetTickCount() - event->eventStartTickCount) + 999) / 1000);
				}
				break;
			}
			if (str[0] != NULL)
			{
				const COLORREF color = RGB(29 << 3, 29 << 3, 12 << 3);	//CDirectDraw::Color(29,8,12);

				const int strWidth = g_GetStringWidth(str, pPrintInfo->hfont) / 2;
				pPrintInfo->text_color = 0;
				g_Print(strX - strWidth + 1, strY + 1, str, pPrintInfo);

				pPrintInfo->text_color = color;
				g_Print(strX - strWidth, strY, str, pPrintInfo);

				strY += 20;
			}
		}
	}

	ZeroMemory(str, 256);
	ZeroMemory(str2, 256);


	// ????? ????? ????? ??????????? ????. ???????? ?????? ??????..
	const MEvent* QuestEvent = g_pEventManager->GetEventByFlag(EVENTFLAG_QUEST_INFO, 0);

	if (QuestEvent != NULL)
	{
		COLORREF color = RGB(116, 243, 73);
		COLORREF color2 = RGB(80, 150, 40);

		str[0] = NULL;
		str2[0] = NULL;

		bool bColor;
		if (QuestEvent->parameter1 > timeGetTime())
		{
			if (((QuestEvent->parameter1 - timeGetTime()) / 300) & 0x1)
				bColor = true;
			else
				bColor = false;
		}
		else
		{
			bColor = false;
		}

		//if( QuestEvent->parameter1 == 0 )
		{
			QUEST_INFO* mkq = g_pQuestInfoManager->GetInfo(QuestEvent->parameter2);

			if (NULL != mkq)
			{
				switch (mkq->GetType())
				{
				case QUEST_INFO_GATHER_ITEM:
				{
					// ?????? ??????
					sprintf(str, "?????? ?????? ???????? ?????? %d/%d %d", QuestEvent->parameter3, mkq->GetGoal(), QuestEvent->parameter4 - (timeGetTime() / 1000));
				}
				break;
				case QUEST_INFO_MONSTER_KILL:
					if (QuestEvent->parameter4 <= (timeGetTime() / 1000))
					{
						sprintf(str, "%s", (*g_pGameStringTable)[UI_STRING_MESSAGE_EXPIRED_TIME_MONSTER_KILL_QUEST].GetString());
						color = RGB(255, 181, 181);
						color2 = RGB(255, 181, 181);
					}
					else
						if (QuestEvent->parameter3 >= mkq->GetGoal())
						{
							sprintf(str, "%s", (*g_pGameStringTable)[UI_STRING_MESSAGE_FINISH_MONSTER_KILL_QUEST].GetString());
							memset(str2, 0, sizeof(str2));
							DWORD time = QuestEvent->parameter4 - (timeGetTime() / 1000);
							DWORD sec = time % 60;
							DWORD minute = (time / 60) % 60;
							DWORD hour = (time / 60 / 60);
							bool bContinue = false;
							if (hour > 0 || bContinue)
							{
								bContinue = true;
								char temp[100];
								sprintf(temp, (*g_pGameStringTable)[UI_STRING_MESSAGE_HOUR].GetString(), hour);
								strcat(str2, temp);
								strcat(str2, " ");
							}
							if (minute > 0 || bContinue)
							{
								bContinue = true;
								char temp[100];
								sprintf(temp, (*g_pGameStringTable)[UI_STRING_MESSAGE_MINUTE].GetString(), minute);
								strcat(str2, temp);
								strcat(str2, " ");
							}
							if (sec > 0 && minute <= 0 && hour <= 0)
							{
								char temp[100];
								sprintf(temp, (*g_pGameStringTable)[UI_STRING_MESSAGE_SECOND].GetString(), sec);
								strcat(str2, temp);
							}
							//sprintf(str2,"%s",(*g_pGameStringTable)[UI_STRING_MESSAGE_CAN_REQUITAL_FROM_NPC].GetString() );
							strcat(str2, (*g_pGameStringTable)[UI_STRING_MESSAGE_CAN_REQUITAL_FROM_NPC].GetString());
							color = RGB(142, 200, 200);
							color2 = RGB(180, 220, 220);
						}

						else
						{
							sprintf(str, "%s %d/%d ", mkq->GetName(), QuestEvent->parameter3, mkq->GetGoal());
							DWORD time = QuestEvent->parameter4 - (timeGetTime() / 1000);


							DWORD sec = time % 60;
							DWORD minute = (time / 60) % 60;
							DWORD hour = (time / 60 / 60);
							bool bContinue = false;

							if (hour > 0 || bContinue)
							{
								bContinue = true;
								char temp[100];
								sprintf(temp, (*g_pGameStringTable)[UI_STRING_MESSAGE_HOUR].GetString(), hour);
								strcat(str, temp);
								strcat(str, " ");
							}
							if (minute > 0 || bContinue)
							{
								bContinue = true;
								char temp[100];
								sprintf(temp, (*g_pGameStringTable)[UI_STRING_MESSAGE_MINUTE].GetString(), minute);
								strcat(str, temp);
								strcat(str, " ");
							}
							if (sec > 0 || bContinue)
							{
								bContinue = true;
								char temp[100];
								sprintf(temp, (*g_pGameStringTable)[UI_STRING_MESSAGE_SECOND].GetString(), sec);
								strcat(str, temp);
								strcat(str, " ");
							}
						}
					break;
				case QUEST_INFO_MEET_NPC:
					break;
				}
			}
		}

		if (str[0] != NULL)
		{

			const int strWidth = g_GetStringWidth(str, pPrintInfo->hfont) / 2;
			pPrintInfo->text_color = 0;
			g_Print(strX - strWidth + 1, strY + 1, str, pPrintInfo);

			pPrintInfo->text_color = bColor ? color2 : color;
			g_Print(strX - strWidth, strY, str, pPrintInfo);

			strY += 20;
		}
		if (str2[0] != NULL)
		{
			const int strWidth = g_GetStringWidth(str2, pPrintInfo->hfont) / 2;
			pPrintInfo->text_color = 0;
			g_Print(strX - strWidth + 1, strY + 1, str2, pPrintInfo);

			pPrintInfo->text_color = bColor ? color2 : color;
			g_Print(strX - strWidth, strY, str2, pPrintInfo);

			strY += 20;
		}
	}
}

//----------------------------------------------------------------------
// Draw Information
//----------------------------------------------------------------------
// Debug?? ????? ?????? ??????.
//----------------------------------------------------------------------
void
MTopView::DrawDebugInfo(CDirectDrawSurface* pSurface)
{
#ifdef	OUTPUT_DEBUG		
	//----------------------------------------------------------------
	// debug?? code
	//----------------------------------------------------------------
	///*
	char str[128];
	char attackMode[20];

	if (g_pPlayer->IsAttackModeNormal())
	{
		strcpy(attackMode, "Normal");
	}
	else if (g_pPlayer->IsAttackModeAggress())
	{
		strcpy(attackMode, "Aggress");
	}
	else if (g_pPlayer->IsAttackModePeace())
	{
		strcpy(attackMode, "Peace");
	}

	switch (g_pPlayer->GetWeaponSpeed())
	{
	case MCreature::WEAPON_SPEED_SLOW: sprintf(attackMode, "Slow(%d)", g_pPlayer->GetAttackSpeed()); break;
	case MCreature::WEAPON_SPEED_NORMAL: sprintf(attackMode, "Normal(%d)", g_pPlayer->GetAttackSpeed()); break;
	case MCreature::WEAPON_SPEED_FAST: sprintf(attackMode, "Fast(%d)", g_pPlayer->GetAttackSpeed()); break;
	}

	sprintf(attackMode, "gtx(%d. %d %d)", gtx_op, gtx_src, gtx_dest);

	int zoneID = (g_bZonePlayerInLarge) ? g_nZoneLarge : g_nZoneSmall;

	sprintf(str, "ID = %d / %d?? [Weapon=%s] [align=%d]", g_pPlayer->GetID(), m_pZone->GetCreatureNumber(), attackMode, g_pPlayer->GetAlignment());
	pSurface->GDI_Text(10, 35, str, RGB(220, 220, 220));

	sprintf(str, " [ZoneID=%d] XY=(%d, %d), sXY=(%d, %d), SerXY=(%d, %d)", zoneID, g_pPlayer->GetX(), g_pPlayer->GetY(), g_pPlayer->GetSX(), g_pPlayer->GetSY(), g_pPlayer->GetServerX(), g_pPlayer->GetServerY());
	pSurface->GDI_Text(10, 56, str, RGB(220, 220, 220));

	sprintf(str, "SX = %d, SY = %d, HP=%d, MP=%d", g_pPlayer->GetSX(), g_pPlayer->GetSY(), g_pPlayer->GetHP(), g_pPlayer->GetMP());
	//sprintf(str, "[Wait Move = %d]", g_pPlayer->GetSendMove());
	pSurface->GDI_Text(10, 72, str, RGB(220, 220, 220));

	sprintf(str, "ActionCount = [%d] %d/%d", g_pPlayer->GetAction(), g_pPlayer->GetActionCount(), g_pPlayer->GetActionCountMax());
	pSurface->GDI_Text(10, 88, str, RGB(220, 220, 220));

	sprintf(str, "MoveCount = %d/%d", g_pPlayer->GetMoveCount(), g_pPlayer->GetMoveCountMax());
	pSurface->GDI_Text(10, 104, str, RGB(220, 220, 220));

	sprintf(str, "Dark/Light/Sight = %d/%d/%d", m_DarkBits, g_pPlayer->GetLightSight()/* + g_pPlayer->GetItemLightSight()*/, g_pPlayer->GetSight());
	pSurface->GDI_Text(10, 120, str, RGB(220, 220, 220));

	if (g_pProfiler != NULL && g_pProfiler->HasProfileInfo(g_ProfileInfoName.GetString()))
	{
		DWORD totalTime = g_pProfiler->GetTotalTime(g_ProfileInfoName.GetString());
		DWORD times = g_pProfiler->GetTimes(g_ProfileInfoName.GetString());
		float avgTime = g_pProfiler->GetAverageTime(g_ProfileInfoName.GetString());

		sprintf(str, "%s = %d / %d = %7.3f", g_ProfileInfoName.GetString(), totalTime, times, avgTime);
		pSurface->GDI_Text(10, 136, str, RGB(220, 220, 220));
	}


	//		if (m_pAlphaEffectTextureManager!=NULL)
	//		{
	//			sprintf(str, "Effect Used/Part/Max = %d/%d/%d", m_pAlphaEffectTextureManager->GetUsed(), m_pAlphaEffectTextureManager->GetMaxPartIndex(), m_pAlphaEffectTextureManager->GetMaxIndex());
	//			pSurface->GDI_Text(10,150, str, RGB(220,220,220));	
	//		}
	//
	//		if (m_pScreenEffectTextureManager!=NULL)
	//		{
	//			sprintf(str, "ScreenEffect Used/Part/Max = %d/%d/%d", m_pScreenEffectTextureManager->GetUsed(), m_pScreenEffectTextureManager->GetMaxPartIndex(), m_pScreenEffectTextureManager->GetMaxIndex());
	//			pSurface->GDI_Text(10,165, str, RGB(220,220,220));	
	//		}
	//
	//		if (m_pImageObjectShadowManager!=NULL)
	//		{
	//			sprintf(str, "ImageObjectShadow Used/Part/Max = %d/%d/%d", m_pImageObjectShadowManager->GetUsed(), m_pImageObjectShadowManager->GetMaxPartIndex(), m_pImageObjectShadowManager->GetMaxIndex());
	//			pSurface->GDI_Text(10,180, str, RGB(220,220,220));	
	//		}
	//
	//		if (m_pAddonShadowManager!=NULL)
	//		{
	//			sprintf(str, "AddonShadow Used/Part/Max = %d/%d/%d", m_pAddonShadowManager->GetUsed(), m_pAddonShadowManager->GetMaxPartIndex(), m_pAddonShadowManager->GetMaxIndex());
	//			pSurface->GDI_Text(10,195, str, RGB(220,220,220));	
	//		}
	//
	//		if (m_pEffectShadowManager!=NULL)
	//		{
	//			sprintf(str, "EffectShadow Used/Part/Max = %d/%d/%d", m_pEffectShadowManager->GetUsed(), m_pEffectShadowManager->GetMaxPartIndex(), m_pEffectShadowManager->GetMaxIndex());
	//			pSurface->GDI_Text(10,210, str, RGB(220,220,220));	
	//		}

	if (g_pRequestServerPlayerManager != NULL
		&& g_pRequestClientPlayerManager != NULL)
	{
		sprintf(str, "Request Other/My = %d/%d", g_pRequestServerPlayerManager->GetSize(), g_pRequestClientPlayerManager->GetSize());
		pSurface->GDI_Text(10, 225, str, RGB(220, 220, 220));
	}

	//#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
	//	sprintf(str, "Repeat = %d, UsedActionInfo = %d", (g_pPlayer->IsRepeatAction()?"True":"False"), g_pPlayer->GetUsedActionInfo());
		//pSurface->GDI_Text(10,195, str, RGB(220,220,220));	
	//#endif


	//if (g_pCurrentMagazine!=NULL)
	//{
	//	sprintf(str, "Bullet = %d", g_pCurrentMagazine->GetNumber());
	//	pSurface->GDI_Text(10,136, str, RGB(220,220,220));
	//}

	//sprintf(str, "FirstPoint : X = %d, Y = %d", firstPointX, firstPointY);
	//pSurface->GDI_Text(9,109, str, RGB(220,220,220));
	//*/	

	// Timer ???
	//sprintf(str, "Timer = %ld", g_CurrentTime);	
	//g_pBack->GDI_Text(11,11, str, RGB(20,20,20));
	//g_pBack->GDI_Text(10,20, str, 0xFFFFFF);


	// Mouse??? ???	
	//sprintf(str, "%d, %d", g_x, g_y);	
	//g_pBack->GDI_Text(11,101, str, RGB(20,20,20));
	//g_pBack->GDI_Text(10,100, str, RGB(255,255,255));

	//sprintf(str, "ImageObject=%d??", m_mapImageObject.size());
	//pSurface->GDI_Text(100,580, str, 0xFFFFFF);

	char infoStr[4][2][20] =
	{
		// FALSE, TRUE
		{ "Mode=555", "Mode=565" },
		{ "WinMM", "SWSynth" },
		{ "Game", "UI" },
		{ "One", "Slide" }
	};

	sprintf(str, "%s, %s, InputFocus=%s, SCShot=%s",
		infoStr[0][CDirectDraw::Is565()],
		infoStr[1][g_DXMusic.IsSoftwareSynth()],
		infoStr[2][g_bUIInput],
		infoStr[3][g_pOperatorOption->bSlideScreenShot]
	);

	pSurface->GDI_Text(150, 580, str, 0xFFFFFF);

	// ???? Texture??? test
	DDSCAPS2 ddsCaps2;
	DWORD dwTotal;
	DWORD dwFree;
	ZeroMemory(&ddsCaps2, sizeof(ddsCaps2));
	ddsCaps2.dwCaps = DDSCAPS_TEXTURE;
	HRESULT hr = CDirectDraw::GetDD()->GetAvailableVidMem(&ddsCaps2, &dwTotal, &dwFree);

	sprintf(str, "Texture = %ld/%ld", dwFree, dwTotal);
	pSurface->GDI_Text(500, 580, str, 0xFFFFFF);


#endif
}


//----------------------------------------------------------------------
// Draw Test Help
//----------------------------------------------------------------------
// ClientTest?? Help
//----------------------------------------------------------------------
void
MTopView::DrawTestHelp()
{
#ifdef OUTPUT_DEBUG
	//--------------------------------------------------------------------
	//
	// F1?? ???? ???? --> ???? ???
	//
	//--------------------------------------------------------------------	
	if (g_pDXInput->KeyDown(DIK_F11)
		&& (g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
	{
		RECT rect = { 250, 25, 530, 410 };

		if (CDirect3D::IsHAL())
		{
			//DrawBox3D(&rect, 0x7000);
			DrawAlphaBox(&rect, 0, 0, 0, 20);
		}
		else
		{
			m_pSurface->Lock();

			if (CDirectDraw::Is565())
			{
				m_pSurface->GammaBox565(&rect, 15);
			}
			else
			{
				m_pSurface->GammaBox555(&rect, 15);
			}

			m_pSurface->Unlock();
		}

		const int maxHelp = 19;
		const char helpStr[maxHelp][128] =
		{
			"---------------------------------------------------------------------",
			"         ALPHA-TEST CLIENT ????",
			"---------------------------------------------------------------------",
			"  [ESC] Menu",
			"  [F1~F8] ??? ?????? ???",
			"  [Scroll Lock] ??? ???(--> \\ScreenShot)",
			"  [Ctrl + M/T/Z] ????/????ð?/?? ???",
			"  [Tab] ??????â on/off",
			"  [Ctrl + Tab] ????â on/off",
			"  [L-Ctrl + ~] ??? on/off",
			"  [L-Ctrl + S] ??? ?????? ????",
			"  [L-Ctrl + Space] ??????â, ????â on/off",
			"---------------------------------------------------------------------",
			"  [L-Button] ???, ????, ????",
			"  [L-Shift + L-Button] ???? ????",
			"  [L-Ctrl + L-Button] ???? ???? ????",
			"  [R-Button] ????, ???",
			"---------------------------------------------------------------------",
			"  [ä?????] ! ?? ?????? ?? ä??"
		};

		POINT point;
		point.x = 250;
		point.y = 25;
		for (int i = 0; i < maxHelp; i++)
		{
			//pSurface->GDI_Text(point.x,1, point.y,1, helpStr[i], RGB(0,0,0));
			m_pSurface->GDI_Text(point.x, point.y, helpStr[i], RGB(220, 220, 220));
			point.y += 20;
		}
	}
#endif
}

//----------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------
// ???? ??? ???????   
// ?????? Tile?? ?????? ???? ????????? ???????.
//----------------------------------------------------------------------
void
MTopView::Draw(int firstPointX, int firstPointY)
{
	if (g_pZone == NULL)
	{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("g_pZone is NULL");
#endif

		return;
	}

#ifdef OUTPUT_DEBUG
	/*		if (g_pDXInput->KeyDown(DIK_RCONTROL) && g_pDXInput->KeyDown(DIK_RSHIFT))
			{
				if (g_WipeValue==0 || g_WipeValue==64)
				{
					if (g_WipeStep==0)
					{
						if (g_WipeValue==0)
							g_WipeStep = 1;
						else
							g_WipeStep = -1;
					}
					else g_WipeStep *= -1;
				}
			}
			else
			{
				g_WipeValue += g_WipeStep;

				if (g_WipeValue<0) g_WipeValue = 0;
				else if (g_WipeValue>64) g_WipeValue=64;
			}
	*/
	// Ÿ??? ??? ???? ??????
	if (g_pDXInput->KeyDown(DIK_T) && g_pDXInput->KeyDown(DIK_LCONTROL))
	{
		SetFirstDraw();
	}
#endif	

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Draw");
#endif

	/*
	// Work Thread?? ????? loading test
	static int first = 1;
	if (first)
	{
		//18336
		MLoadingSPKWorkNode4* pNode = new MLoadingSPKWorkNode4(0, 10000, 2);
		pNode->SetSPK( &m_AddonSPK, FILE_ISPRITE_ADDON );
		pNode->SetType( 1 );
		g_pLoadingThread->SetPriority( THREAD_PRIORITY_BELOW_NORMAL );
		g_pLoadingThread->AddLast( pNode );

		first = 0;
	}
	*/

	//------------------------------------------------------------
	// ?????(-_-;) ???? ??? ?????? - -;
	//------------------------------------------------------------
	//gC_font.NotUseNoPublicEng();		

	//#ifdef OUTPUT_DEBUG
		//	DEBUG_ADD("[TempDebug] Before Draw");
	//#endif	

	//------------------------------------------------------------
	// Clip???? ??????? ??????.
	//------------------------------------------------------------
	int clipRight = m_pSurface->GetClipRight();
	int clipBottom = m_pSurface->GetClipBottom();
	m_pSurface->SetClipRightBottom(
		CLIPSURFACE_WIDTH, CLIPSURFACE_HEIGHT);

	//------------------------------------------------------------
	// Tile?? ó?? ??? ??, ??~ ?????.
	//------------------------------------------------------------
	if (m_bFirstTileDraw)
	{
		DEBUG_ADD("Draw m_bFirstTileDraw: Restore");

		CDirectDraw::RestoreAllSurfaces();
		CDirect3D::Restore();

		DEBUG_ADD("Draw m_bFirstTileDraw: Clear");

		m_mapImageObject.clear();
		m_mapCreature.clear();

		DEBUG_ADD("Draw m_bFirstTileDraw: DrawTileSurface");

		__BEGIN_PROFILE("DrawTileSurface")

			DrawTileSurface();

		__END_PROFILE("DrawTileSurface")

			DEBUG_ADD("Draw m_bFirstTileDraw: DetermineImageObject");

		DetermineImageObject();
		m_bFirstTileDraw = false;

		DEBUG_ADD("Draw m_bFirstTileDraw: OK");

		/*
		CDirect3D::GetDevice()->LightEnable( 0, false );
		CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_LIGHTING, false );


		D3DLIGHT7 light;
		D3DUtil_InitLight( light, D3DLIGHT_POINT, 0.0f, 0.0f, -10.0f );

		light.dltType      = D3DLIGHT_SPOT;
		light.dcvDiffuse.r   = 1.0f;
		light.dcvDiffuse.g   = 1.0f;
		light.dcvDiffuse.b   = 1.0f;
		light.dcvSpecular = light.dcvDiffuse;
		light.dvPosition.x = 0.0f;
		light.dvPosition.y = 0.0f;
		light.dvPosition.z = -10.0f;
		light.dvDirection.x = 0.0f;
		light.dvDirection.y = 0.0f;
		light.dvDirection.z = 1.0f;
		light.dvTheta =       0.5f; //?????? ??? ???
		light.dvPhi =         0.8f; //?????? ??? ???
		light.dvAttenuation0 = 1.0f;
		light.dvFalloff		= 1.0f;

		CDirect3D::GetDevice()->SetLight( 0, &light );
		*/
		//*/
	}
	//------------------------------------------------------------
	// ?????? ??? Tile?? ??? ??????.
	//------------------------------------------------------------
	//else
	{
		if (CDirect3D::IsHAL())// && m_b3DLight)
		{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("IsLost?");
#endif

			// ?????? ???..
			if (CDirect3D::IsLost())
			{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
				DEBUG_ADD("CDirect3D::Restore");
#endif

				CDirect3D::Restore();

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
				DEBUG_ADD("RestoreSurface");
#endif

				RestoreSurface();
			}

			//------------------------------------------------
			// BeginScene
			//------------------------------------------------
			/*
			if (CDirect3D::GetDevice()->BeginScene()!=D3D_OK)
			{
				CDirectDraw::RestoreAllSurfaces();
				CDirect3D::Restore();

				RestoreSurface();

				m_pSurface->SetClipNULL();
				return;
			}
			*/


			///*
			///*
			/*
			D3DRECT rect3d;
			rect3d.x1 = 0;
			rect3d.y1 = 0;
			rect3d.x2 = CLIPSURFACE_WIDTH;
			rect3d.y2 = CLIPSURFACE_HEIGHT;
			CDirect3D::GetDevice()->Clear( 1UL, (D3DRECT *)&rect3d,
										D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
										0x00000000, 1.0f, 0L );
			*/
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("Clear 3D");
#endif

			CDirect3D::GetDevice()->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0L);
			//*/


			//DrawZone(firstPointX, firstPointY);

			//*/


			/*
			if (g_pDXInput->KeyDown(DIK_Z))
			{
				D3DLIGHT7 light;
				CDirect3D::GetDevice()->GetLight( 0, &light );

				light.dvTheta -=       0.01f; //?????? ??? ???
				light.dvPhi -=         0.01f; //?????? ??? ???

				CDirect3D::GetDevice()->SetLight( 0, &light );
			}

			if (g_pDXInput->KeyDown(DIK_A))
			{
				D3DLIGHT7 light;
				CDirect3D::GetDevice()->GetLight( 0, &light );

				light.dvTheta +=       0.01f; //?????? ??? ???
				light.dvPhi +=         0.01f; //?????? ??? ???

				CDirect3D::GetDevice()->SetLight( 0, &light );
			}
			*/

			///*
			//test.Rotate( 0.1f, 0.1f, 0 );

			//------------------------------------------------
			// LightBuffer ????
			//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("Clear LightBufferFilter3D");
#endif

			__BEGIN_PROFILE("ClearLBF3D")

				ClearLightBufferFilter3D();

			__END_PROFILE("ClearLBF3D")

				//------------------------------------------------
				// Draw Zone
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
				DEBUG_ADD("DrawZone");
#endif

			__BEGIN_PROFILE("DrawZone")

				DrawZone(firstPointX, firstPointY);

			__END_PROFILE("DrawZone")


				// mouse????? ??			
				//AddLightFilter( m_FirstZonePixel.x,g_x, m_FirstZonePixel.y,g_y, 4 );
				//------------------------------------------------
				// Fade
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
				DEBUG_ADD("DrawFade");
#endif

			__BEGIN_PROFILE("DrawFade")

				DrawFade();

			__END_PROFILE("DrawFade")

				//------------------------------------------------
				// Minimap
				//------------------------------------------------
				/* // 2001.7.14 ???ó??
				if (g_pUserOption->DrawMinimap)
				{
					DrawMinimap( CLIPSURFACE_WIDTH-m_pMinimapSPR->GetWidth(),
								0,//440-m_pMinimapSPR->GetHeight(),
								1);	// scale
				}
				*/

				//------------------------------------------------
				// test?? ????
				//------------------------------------------------
				if (g_pDXInput->KeyDown(DIK_F11))
				{
					DrawTestHelp();
				}


			//------------------------------------------------
			// EndScene
			//------------------------------------------------
			//CDirect3D::GetDevice()->EndScene();

			//------------------------------------------------
			// ????? Surface???? ??????? ???.
			// ????, EndScene()?? ?????? ?????? ???.
			//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("Remove RemovedTexture");
#endif

			__BEGIN_PROFILE("ClearTexture")

				//			if (m_pAlphaEffectTextureManager)				m_pAlphaEffectTextureManager->DeleteRemoved();
				//			if (m_pScreenEffectTextureManager)	m_pScreenEffectTextureManager->DeleteRemoved();
				//			if (m_pImageObjectShadowManager)	m_pImageObjectShadowManager->DeleteRemoved();
				//			if (m_pAddonShadowManager)			m_pAddonShadowManager->DeleteRemoved();
				//			if (m_pOustersShadowManager)		m_pOustersShadowManager->DeleteRemoved();
				//			if (m_pEffectShadowManager)			m_pEffectShadowManager->DeleteRemoved();
				//			if (m_pCreatureShadowManager)		m_pCreatureShadowManager->DeleteRemoved();

				__END_PROFILE("ClearTexture")
		}
		else
		{
			//------------------------------------------------
			// LightBuffer ????
			//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("Clear LightBufferFilter2D");
#endif

			__BEGIN_PROFILE("ClearLBF2D")

				ClearLightBufferFilter2D();

			__END_PROFILE("ClearLBF2D")


				//------------------------------------------------
				// Draw Zone
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
				DEBUG_ADD("DrawZone");
#endif

			__BEGIN_PROFILE("DrawZone")

				DrawZone(firstPointX, firstPointY);

			__END_PROFILE("DrawZone")

				//------------------------------------------------
				// Fade
				//------------------------------------------------
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
				DEBUG_ADD("DrawFade");
#endif

			__BEGIN_PROFILE("DrawFade")

				DrawFade();

			__BEGIN_PROFILE("DrawFade")

				//------------------------------------------------
				// Minimap
				//------------------------------------------------
				/* // 2001.7.14 ???ó??
				if (g_pUserOption->DrawMinimap)
				{
					DrawMinimap( CLIPSURFACE_WIDTH-m_pMinimapSPR->GetWidth(),
								0, //440-m_pMinimapSPR->GetHeight(),
								1);	// scale
				}
				*/

				//------------------------------------------------
				// test?? ????
				//------------------------------------------------
	//			if (g_pDXInput->KeyDown(DIK_F11))
	//			{
	//				DrawTestHelp();				
	//			}
		}
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Draw Information");
#endif

	//------------------------------------------------
	// ???? ???
	//------------------------------------------------
	DrawInformation();

	//#ifdef OUTPUT_DEBUG
		//	DEBUG_ADD("[TempDebug] After Draw");
	//#endif


	//char str[80];
	//sprintf(str, "ImageObject=%d??", m_mapImageObject.size());
	//m_pSurface->GDI_Text(100,580, str, 0xFFFFFF);

	//------------------------------------------------------------
	// ???????? ???
	//------------------------------------------------------------
	//m_pSurface->SetClipNULL();	
	m_pSurface->SetClipRightBottom(clipRight, clipBottom);

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Draw OK");
#endif
}

//----------------------------------------------------------------------
// Draw LightBuffer
//----------------------------------------------------------------------
// m_LightBufferFilter?? m_pLightBufferTexture?? blt???
// m_pLightBufferTexture?? texture?? ???????
// ??? LightBufferFilter?? ?????? ???.
//----------------------------------------------------------------------
void
MTopView::DrawLightBuffer3D()
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawLightBuffer3D");
#endif

	if (//CDirect3D::IsHAL() && 
		m_DarkBits || g_pPlayer->IsInDarkness())
	{
		//------------------------------------------------
		// LightBufferFilter --> Texture
		//------------------------------------------------
		WORD* lpSurface, pitch;
		m_pLightBufferTexture->Lock();
		lpSurface = (WORD*)m_pLightBufferTexture->GetSurfacePointer();
		pitch = m_pLightBufferTexture->GetSurfacePitch();

		m_LightBufferFilter.Blt4444(lpSurface, pitch);
		//m_LightBufferFilter.Blt4444Color(lpSurface, pitch, 0x0200);

		m_pLightBufferTexture->Unlock();
		//*/

		//------------------------------------------------
		// Texture??????? ???
		//------------------------------------------------
		RECT rect = { 0, 0, CLIPSURFACE_WIDTH, CLIPSURFACE_HEIGHT };

		DRAW_TEXTURE_SURFACE(m_pLightBufferTexture, &rect)
	else
	{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("Failed DrawLightBuffer3D");
#endif
	}


#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawLightBuffer3D");
#endif
	}


#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawLightBuffer3D");
#endif


}

//----------------------------------------------------------------------
// Draw LightBuffer
//----------------------------------------------------------------------
// m_LightBufferFilter?? m_pLightBufferTexture?? blt???
// m_pLightBufferTexture?? texture?? ???????
// ??? LightBufferFilter?? ?????? ???.
//----------------------------------------------------------------------
void
MTopView::DrawLightBuffer2D()
{
	//if (CDirect3D::IsHAL() && m_b3DLight && 
	if (m_DarkBits || g_pPlayer->IsInDarkness())
	{
		//------------------------------------------------
		// LightBufferFilter --> m_pSurface
		//------------------------------------------------
		WORD* lpSurface, pitch;

		WORD* lpSurfaceTemp1,
			* lpSurfaceTemp2,
			* lpSurfaceTemp3,
			* lpSurfaceTemp4,
			* lpSurfaceTemp5,
			* lpSurfaceTemp6,
			* lpSurfaceTemp7,
			* lpSurfaceTemp8,
			* lpSurfaceTemp9,
			/*#ifdef CONVERT_1024_768
							*lpSurfaceTemp10,
							*lpSurfaceTemp11,
							*lpSurfaceTemp12;
			#else*/
			* lpSurfaceTemp10;
		//#endif

				//m_pSurface->Lock();
		lpSurface = (WORD*)m_pSurface->GetSurfacePointer();
		pitch = m_pSurface->GetSurfacePitch();

		//--------------------------------------------
		// 5:6:5
		//--------------------------------------------
		if (CDirectDraw::Is565())
		{
			int* pPH = m_p2DLightPixelHeight;
			for (int y = 0; y < m_LightBufferFilter.GetHeight(); y++)
			{
				BYTE* pFilter = m_LightBufferFilter.GetFilter(y);
				int* pPW = m_p2DLightPixelWidth;


				//--------------------------------------------
				// 9??? ??????...
				//--------------------------------------------
				if (*pPH == 9)
				{
					lpSurfaceTemp1 = lpSurface;
					lpSurfaceTemp2 = (WORD*)((BYTE*)lpSurfaceTemp1 + pitch);
					lpSurfaceTemp3 = (WORD*)((BYTE*)lpSurfaceTemp2 + pitch);
					lpSurfaceTemp4 = (WORD*)((BYTE*)lpSurfaceTemp3 + pitch);
					lpSurfaceTemp5 = (WORD*)((BYTE*)lpSurfaceTemp4 + pitch);
					lpSurfaceTemp6 = (WORD*)((BYTE*)lpSurfaceTemp5 + pitch);
					lpSurfaceTemp7 = (WORD*)((BYTE*)lpSurfaceTemp6 + pitch);
					lpSurfaceTemp8 = (WORD*)((BYTE*)lpSurfaceTemp7 + pitch);
					lpSurfaceTemp9 = (WORD*)((BYTE*)lpSurfaceTemp8 + pitch);

					// ????..
					lpSurface = (WORD*)((BYTE*)lpSurfaceTemp9 + pitch);

					for (int x = 0; x < m_LightBufferFilter.GetWidth(); x++)
					{
						int light = *pFilter;
						int	len = *pPW;

						//if (light!=31)
						//{
							// len?? ???
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp1, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp2, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp3, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp4, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp5, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp6, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp7, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp8, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp9, len, light);
						//}


						// ???? filter??
						pFilter++;
						pPW++;

						// ???? ??? ??? --> 8 pixel ??
						lpSurfaceTemp1 += len;
						lpSurfaceTemp2 += len;
						lpSurfaceTemp3 += len;
						lpSurfaceTemp4 += len;
						lpSurfaceTemp5 += len;
						lpSurfaceTemp6 += len;
						lpSurfaceTemp7 += len;
						lpSurfaceTemp8 += len;
						lpSurfaceTemp9 += len;
					}
				}
				//--------------------------------------------
				// 10??? ???
				//--------------------------------------------
				//else //if (*pPH==8)
				else if (*pPH == 10)
				{
					lpSurfaceTemp1 = lpSurface;
					lpSurfaceTemp2 = (WORD*)((BYTE*)lpSurfaceTemp1 + pitch);
					lpSurfaceTemp3 = (WORD*)((BYTE*)lpSurfaceTemp2 + pitch);
					lpSurfaceTemp4 = (WORD*)((BYTE*)lpSurfaceTemp3 + pitch);
					lpSurfaceTemp5 = (WORD*)((BYTE*)lpSurfaceTemp4 + pitch);
					lpSurfaceTemp6 = (WORD*)((BYTE*)lpSurfaceTemp5 + pitch);
					lpSurfaceTemp7 = (WORD*)((BYTE*)lpSurfaceTemp6 + pitch);
					lpSurfaceTemp8 = (WORD*)((BYTE*)lpSurfaceTemp7 + pitch);
					lpSurfaceTemp9 = (WORD*)((BYTE*)lpSurfaceTemp8 + pitch);
					lpSurfaceTemp10 = (WORD*)((BYTE*)lpSurfaceTemp9 + pitch);
					/*#ifdef CONVERT_1024_768
										lpSurfaceTemp11 = (WORD*)((BYTE*)lpSurfaceTemp10 + pitch);
										lpSurfaceTemp12 = (WORD*)((BYTE*)lpSurfaceTemp11 + pitch);
										lpSurface = (WORD*)((BYTE*)lpSurfaceTemp12 + pitch);
					#else*/
					// ????..
					lpSurface = (WORD*)((BYTE*)lpSurfaceTemp10 + pitch);
					//#endif

					for (int x = 0; x < m_LightBufferFilter.GetWidth(); x++)
					{
						int light = *pFilter;
						int	len = *pPW;

						//if (light!=31)
						//{
							// len?? ???
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp1, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp2, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp3, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp4, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp5, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp6, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp7, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp8, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp9, len, light);
						m_pSurface->Gamma4Pixel565(lpSurfaceTemp10, len, light);
						/*#ifdef CONVERT_1024_768
													m_pSurface->Gamma4Pixel565(lpSurfaceTemp11, len, light);
													m_pSurface->Gamma4Pixel565(lpSurfaceTemp12, len, light);
						#endif*/
						//}


						// ???? filter??
						pFilter++;
						pPW++;

						// ???? ??? ??? --> 8 pixel ??
						lpSurfaceTemp1 += len;
						lpSurfaceTemp2 += len;
						lpSurfaceTemp3 += len;
						lpSurfaceTemp4 += len;
						lpSurfaceTemp5 += len;
						lpSurfaceTemp6 += len;
						lpSurfaceTemp7 += len;
						lpSurfaceTemp8 += len;
						lpSurfaceTemp9 += len;
						lpSurfaceTemp10 += len;
						/*#ifdef CONVERT_1024_768
												lpSurfaceTemp11 += len;
												lpSurfaceTemp12 += len;
						#endif*/
					}
				}

				pPH++;
			}
		}
		//--------------------------------------------
		// 5:5:5
		//--------------------------------------------
		else
		{
			int* pPH = m_p2DLightPixelHeight;
			for (int y = 0; y < m_LightBufferFilter.GetHeight(); y++)
			{
				BYTE* pFilter = m_LightBufferFilter.GetFilter(y);
				int* pPW = m_p2DLightPixelWidth;


				//--------------------------------------------
				// 9??? ??????...
				//--------------------------------------------
				if (*pPH == 9)
				{
					lpSurfaceTemp1 = lpSurface;
					lpSurfaceTemp2 = (WORD*)((BYTE*)lpSurfaceTemp1 + pitch);
					lpSurfaceTemp3 = (WORD*)((BYTE*)lpSurfaceTemp2 + pitch);
					lpSurfaceTemp4 = (WORD*)((BYTE*)lpSurfaceTemp3 + pitch);
					lpSurfaceTemp5 = (WORD*)((BYTE*)lpSurfaceTemp4 + pitch);
					lpSurfaceTemp6 = (WORD*)((BYTE*)lpSurfaceTemp5 + pitch);
					lpSurfaceTemp7 = (WORD*)((BYTE*)lpSurfaceTemp6 + pitch);
					lpSurfaceTemp8 = (WORD*)((BYTE*)lpSurfaceTemp7 + pitch);
					lpSurfaceTemp9 = (WORD*)((BYTE*)lpSurfaceTemp8 + pitch);

					// ????..
					lpSurface = (WORD*)((BYTE*)lpSurfaceTemp9 + pitch);

					for (int x = 0; x < m_LightBufferFilter.GetWidth(); x++)
					{
						int light = *pFilter;
						int	len = *pPW;

						//if (light!=31)
						//{
							// len?? ???
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp1, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp2, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp3, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp4, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp5, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp6, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp7, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp8, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp9, len, light);
						//}


						// ???? filter??
						pFilter++;
						pPW++;

						// ???? ??? ??? --> 8 pixel ??
						lpSurfaceTemp1 += len;
						lpSurfaceTemp2 += len;
						lpSurfaceTemp3 += len;
						lpSurfaceTemp4 += len;
						lpSurfaceTemp5 += len;
						lpSurfaceTemp6 += len;
						lpSurfaceTemp7 += len;
						lpSurfaceTemp8 += len;
						lpSurfaceTemp9 += len;
					}
				}
				//--------------------------------------------
				// 10??? ???
				//--------------------------------------------
				else if (*pPH == 10)
					//else //if (*pPH==8)
				{
					lpSurfaceTemp1 = lpSurface;
					lpSurfaceTemp2 = (WORD*)((BYTE*)lpSurfaceTemp1 + pitch);
					lpSurfaceTemp3 = (WORD*)((BYTE*)lpSurfaceTemp2 + pitch);
					lpSurfaceTemp4 = (WORD*)((BYTE*)lpSurfaceTemp3 + pitch);
					lpSurfaceTemp5 = (WORD*)((BYTE*)lpSurfaceTemp4 + pitch);
					lpSurfaceTemp6 = (WORD*)((BYTE*)lpSurfaceTemp5 + pitch);
					lpSurfaceTemp7 = (WORD*)((BYTE*)lpSurfaceTemp6 + pitch);
					lpSurfaceTemp8 = (WORD*)((BYTE*)lpSurfaceTemp7 + pitch);
					lpSurfaceTemp9 = (WORD*)((BYTE*)lpSurfaceTemp8 + pitch);
					lpSurfaceTemp10 = (WORD*)((BYTE*)lpSurfaceTemp9 + pitch);
					/*#ifdef CONVERT_1024_768
										lpSurfaceTemp11 = (WORD*)((BYTE*)lpSurfaceTemp10 + pitch);
										lpSurfaceTemp12 = (WORD*)((BYTE*)lpSurfaceTemp11 + pitch);
										lpSurface = (WORD*)((BYTE*)lpSurfaceTemp12 + pitch);
					#else*/
					// ????..
					lpSurface = (WORD*)((BYTE*)lpSurfaceTemp10 + pitch);
					//#endif

					for (int x = 0; x < m_LightBufferFilter.GetWidth(); x++)
					{
						int light = *pFilter;
						int	len = *pPW;

						//if (light!=31)
						//{
							// len?? ???
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp1, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp2, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp3, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp4, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp5, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp6, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp7, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp8, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp9, len, light);
						m_pSurface->Gamma4Pixel555(lpSurfaceTemp10, len, light);
						/*#ifdef CONVERT_1024_768
													m_pSurface->Gamma4Pixel555(lpSurfaceTemp11, len, light);
													m_pSurface->Gamma4Pixel555(lpSurfaceTemp12, len, light);
						#endif*/
						//}


						// ???? filter??
						pFilter++;
						pPW++;

						// ???? ??? ??? --> 8 pixel ??
						lpSurfaceTemp1 += len;
						lpSurfaceTemp2 += len;
						lpSurfaceTemp3 += len;
						lpSurfaceTemp4 += len;
						lpSurfaceTemp5 += len;
						lpSurfaceTemp6 += len;
						lpSurfaceTemp7 += len;
						lpSurfaceTemp8 += len;
						lpSurfaceTemp9 += len;
						lpSurfaceTemp10 += len;
						/*#ifdef CONVERT_1024_768
												lpSurfaceTemp11 += len;
												lpSurfaceTemp12 += len;
						#endif*/
					}
				}

				pPH++;
			}
		}
		//m_pSurface->Unlock();
		//*/	
	}
}


//----------------------------------------------------------------------
// Determine ImageObject
//----------------------------------------------------------------------
// ??? ??? ImageObject???? ??? ??????.
//----------------------------------------------------------------------
void
MTopView::DetermineImageObject()
{
	//---------------------------------------------------------------	
	// firstPoint?? ??µ? Zone?????? ù??° Sector
	//---------------------------------------------------------------	
	// Player?? ??????? ??? ????????? ??????
	// ????~~~~ ??~ ???? ??????.
	//---------------------------------------------------------------	
	POINT	firstSector;
	firstSector.x = g_pPlayer->GetX() + SECTOR_SKIP_PLAYER_LEFT;
	firstSector.y = g_pPlayer->GetY() + SECTOR_SKIP_PLAYER_UP;

	//---------------------------------------------------------------	
	// Zone?? ??? ???? ??,
	// ???? ??? ?? ????? ???..
	//---------------------------------------------------------------	
	// X ??? ?????..
	//---------------------------------------------------------------	
	if (firstSector.x <= 0)
	{
		firstSector.x = 0;
	}
	else if (firstSector.x + SECTOR_WIDTH + 1 >= m_pZone->GetWidth())
	{
		firstSector.x = m_pZone->GetWidth() - SECTOR_WIDTH - 1;
	}

	//---------------------------------------------------------------	
	// Y ??? ?????
	//---------------------------------------------------------------	
	if (firstSector.y <= 0)
	{
		firstSector.y = 0;
	}
	else if (firstSector.y + SECTOR_HEIGHT + 1 >= m_pZone->GetHeight())
	{
		firstSector.y = m_pZone->GetHeight() - SECTOR_HEIGHT - 1;
	}

	//----------------------------------------------------------------------
	//
	//                         ImageObject ???
	//
	//----------------------------------------------------------------------	
	//POINT firstSector = PixelToMap(m_FirstZonePixel.x, m_FirstZonePixel.y);
	int sX1 = firstSector.x + SECTOR_SKIP_LEFT;
	int sY1 = firstSector.y + SECTOR_SKIP_UP;
	int sX2 = firstSector.x + SECTOR_WIDTH + 1;
	int sY2 = firstSector.y + SECTOR_HEIGHT + 1;

	//------------------------------------------------------
	// Zone?? ?????? ??? ??? Skip...
	//------------------------------------------------------
	if (sX1 < 0)
	{
		//tilePoint.x += -sX1 * TILE_X;
		sX1 = 0;
	}

	if (sX2 >= m_pZone->GetWidth())
	{
		sX2 = m_pZone->GetWidth() - 1;
	}

	if (sY1 < 0)
	{
		//tilePoint.y += -sY1 * TILE_Y;
		sY1 = 0;
	}

	if (sY2 >= m_pZone->GetHeight())
	{
		sY2 = m_pZone->GetHeight() - 1;
	}

	//------------------------------------------------------
	// ?????? ??? ImageObject?? ??? ?????.
	//------------------------------------------------------
	m_mapImageObject.clear();

	//------------------------------------------------------
	// ?? Sector?? ImageObject???
	//------------------------------------------------------
	int y, x, i;
	for (y = sY1; y <= sY2; y++)
	{
		for (x = sX1; x <= sX2; x++)
		{
			const MSector& sector = m_pZone->GetSector(x, y);

			// ImageObject?? ????.. ???~ ???????.
			// ??, ????? ??????? ??´?.
			if (sector.IsExistImageObject())
			{
				OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

				// Sector?? ??? ??? ImageObject???? ??????.
				for (i = 0; i < sector.GetImageObjectSize(); i++)
				{
					MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

					//----------------------------------------
					// ??? ?????? ü???? ??????,
					// ?? ImageObject????
					// ????? ViewSector?? ??? viewpoint??
					// ???? ??? ???. -_-;
					// ?????.. ?????? ??????.
					//----------------------------------------
					// ??? ?????? ??????.. ??????.
					//if (pImageObject->GetViewpoint()==y				// ?????? ??
					//	|| y==sY2-1									// ????
					//	|| pImageObject->GetViewpoint() < sY1)		// ù??
					{
						// ??? ????? ???????? ?????? ???.
						QWORD key = GetOutputImageObjectID(pImageObject);

						if (m_mapImageObject.find(key)
							== m_mapImageObject.end())
						{
							//----------------------------------------
							// Key?? = (Viewpoint << 32) | ID
							//----------------------------------------
							// Viewpoint?? ????? sort???.
							// Viewpoint?? ID?? ????? ?????? ?? ???.
							//----------------------------------------
							m_mapImageObject.insert(
								IMAGEOBJECT_OUTPUT_MAP::value_type(
									key,	// key??
									pImageObject
								)
							);
						}
					}

					iImageObject++;
				}
			}
		}
	}
}

//----------------------------------------------------------------------
// Update ImageObject
//----------------------------------------------------------------------
// m_FirstPoint?? ??? ????~ ??? ??????..
// parameter?? firstSector?? ?????? ??? ??????.
//
// ????? ???????? ImageObject?? m_mapImageObject???? ?????
// ??? ??Ÿ???? ImageObject?? m_mapImageObject?? ??????.
//----------------------------------------------------------------------
void
MTopView::UpdateImageObject(const POINT& newFirstSector)
{
	//#ifdef OUTPUT_DEBUG
		//	DEBUG_ADD("[TempDebug] Before Update ImageObject");
	//#endif	



	int x, y, i;

	//---------------------------------------------------------------
	// 
	//               ImageObject ????
	//
	//---------------------------------------------------------------
	// newFirstSector?? m_FirstSector(??? ?????? ???)?? ?????
	// ??? ???... check~~??????.
	//---------------------------------------------------------------
	// ????? ????? ????? ???
	//---------------------------------------------------------------
	if (newFirstSector.y != m_FirstSector.y)
	{
		int	sX1 = newFirstSector.x + SECTOR_SKIP_LEFT,
			sX2 = newFirstSector.x + SECTOR_WIDTH + 1,
			sX01 = m_FirstSector.x + SECTOR_SKIP_LEFT,
			sX02 = m_FirstSector.x + SECTOR_WIDTH + 1,
			eraseY1, eraseY2,		// ??????? ????? ??? ??
			lastY,					// ????? ?????? ??(????? ???? ???)
			newY1, newY2;			// ???? ??Ÿ???? ????? ??? ??

		// ü??? ??? ??°??
		bool bCheckLast = true;
		bool bCheckErase = true;
		bool bCheckNew = true;

		//-----------------------------------
		// Zone?????? ?????? ???
		//-----------------------------------
		if (sX2 >= m_pZone->GetWidth())
		{
			sX2 = m_pZone->GetWidth() - 1;
		}

		if (sX1 < 0)
		{
			sX1 = 0;
		}

		if (sX02 >= m_pZone->GetWidth())
		{
			sX02 = m_pZone->GetWidth() - 1;
		}

		if (sX01 < 0)
		{
			sX01 = 0;
		}

		//-----------------------------------
		// ???????? ????? ???
		//-----------------------------------
		if (newFirstSector.y - m_FirstSector.y < 0)
		{
			// ???????? ??????? ????? ????? ???????????.
			// --> ???? ????? ?????? ????? ??????,
			//     ???? ????? ?????? ????? ???? ????? ??????? ?????.

			// ??? ?? ??
			lastY = newFirstSector.y + SECTOR_HEIGHT + 1;	// ???? ????? ?????? ??

			// ?????? ?? ?? : lasyY+1 ~ eraseY
			eraseY1 = lastY + 1;
			eraseY2 = m_FirstSector.y + SECTOR_HEIGHT + 1;	// ???? ????? ?????? ??			

			// ???? ??Ÿ???? ??
			newY1 = newFirstSector.y + SECTOR_SKIP_UP;
			newY2 = m_FirstSector.y + SECTOR_SKIP_UP - 1;

			//------------------------------------------------------
			// Zone?? ?????? ??? ??? Skip...
			//------------------------------------------------------
			if (lastY >= m_pZone->GetHeight())
			{
				//lastY = m_pZone->GetHeight()-1;				
				// ?? ??? ????.
				bCheckLast = false;
			}

			if (eraseY1 >= m_pZone->GetHeight())
			{
				//eraseY1 = m_pZone->GetHeight()-1;
				// erase?? ??? ????. 
				bCheckErase = false;
			}
			else if (eraseY2 >= m_pZone->GetHeight())
			{
				eraseY2 = m_pZone->GetHeight() - 1;
			}

			if (newY2 < 0)
			{
				//newY2 = 0;
				// new?? ??? ????.
				bCheckNew = false;
			}
			else if (newY1 < 0)
			{
				newY1 = 0;
			}

		}
		//-----------------------------------
		// ????????? ????? ???
		//-----------------------------------
		else
		{
			// ??? ?? ??
			lastY = newFirstSector.y + SECTOR_SKIP_UP;	// ???? ????? ?????? ??

			// ??????? ?? : eraseY ~ lastY-1
			eraseY1 = m_FirstSector.y + SECTOR_SKIP_UP;	// ???? ????? ?????? ??
			eraseY2 = lastY - 1;

			// ???? ??Ÿ???? ??
			newY1 = m_FirstSector.y + SECTOR_HEIGHT + 1 + 1;
			newY2 = newFirstSector.y + SECTOR_HEIGHT + 1;

			//------------------------------------------------------
			// Zone?? ?????? ??? ??? Skip...
			//------------------------------------------------------				
			if (lastY < 0)
			{
				//lastY = 0;	
				bCheckLast = false;
			}

			if (eraseY2 < 0)
			{
				//eraseY2 = 0;	
				bCheckErase = false;
			}
			else if (eraseY1 < 0)
			{
				eraseY1 = 0;
			}

			if (newY1 >= m_pZone->GetHeight())
			{
				//newY1 = m_pZone->GetHeight()-1;				
				bCheckNew = false;
			}
			else if (newY2 >= m_pZone->GetHeight())
			{
				newY2 = m_pZone->GetHeight() - 1;
			}

		}


		//----------------------------------------------
		// erase1 ~ erase2?????? ??????(?) ???.
		//----------------------------------------------
		// ??? m_mapImageObject???? ?????.
		//----------------------------------------------
		if (bCheckErase)
		{
			for (y = eraseY1; y <= eraseY2; y++)
			{
				for (x = sX01; x <= sX02; x++)
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					// ImageObject?? ????.. ???~ ???????.
					// ??, ????? ??????? ??´?.
					if (sector.IsExistImageObject())
					{
						OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

						// Sector?? ??? ??? ImageObject???? ??????.
						for (int i = 0; i < sector.GetImageObjectSize(); i++)
						{
							MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

							//----------------------------------------
							// Key?? = (Viewpoint << 32) | ID
							//----------------------------------------
							// ??? ????? ???????? ?????? ????.
							IMAGEOBJECT_OUTPUT_MAP::iterator iDelete = m_mapImageObject.find(GetOutputImageObjectID(pImageObject));
							if (iDelete != m_mapImageObject.end())
							{
								m_mapImageObject.erase(iDelete);
							}

							iImageObject++;
						}
					}
				}
			}
		}

		//----------------------------------------------
		// lastY?? ??? ??????.
		//----------------------------------------------
		if (bCheckLast)
		{
			for (x = sX1; x <= sX2; x++)
			{
				const MSector& sector = m_pZone->GetSector(x, lastY);

				// ImageObject?? ??????..
				if (sector.IsExistImageObject())
				{
					OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

					// Sector?? ??? ??? ImageObject???? ??????.
					for (i = 0; i < sector.GetImageObjectSize(); i++)
					{
						MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

						// ??? ?????? ??????.. ??????.
						//if (pImageObject->GetViewpoint()==y				// ?????? ??
						//	|| y==sY2-1									// ????
						//	|| pImageObject->GetViewpoint() < sY1)		// ù??
						{
							QWORD key = GetOutputImageObjectID(pImageObject);
							// ??? ????? ???????? ?????? ???.
							if (m_mapImageObject.find(key)
								== m_mapImageObject.end())
							{
								//----------------------------------------
								// Key?? = (Viewpoint << 32) | ID
								//----------------------------------------
								// Viewpoint?? ????? sort???.
								// Viewpoint?? ID?? ????? ?????? ?? ???.
								//----------------------------------------
								m_mapImageObject.insert(
									IMAGEOBJECT_OUTPUT_MAP::value_type(
										key,	// key??
										pImageObject
									)
								);
							}
						}

						iImageObject++;
					}
				}
			}
		}

		//----------------------------------------------
		// ???? ??Ÿ?? ??? : newY1 ~ newY2
		//----------------------------------------------
		if (bCheckNew)
		{
			for (y = newY1; y <= newY2; y++)
			{
				for (x = sX1; x <= sX2; x++)
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					// ImageObject?? ??????..
					if (sector.IsExistImageObject())
					{
						OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

						// Sector?? ??? ??? ImageObject???? ??????.
						for (i = 0; i < sector.GetImageObjectSize(); i++)
						{
							MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

							// ??? ?????? ??????.. ??????.						
							//if (viewPoint==y				// ?????? ??
							//	|| viewPoint > newY2									// ????
							//	|| viewPoint < newY1)		// ù??
							{
								// ??? ????? ???????? ?????? ???.
								QWORD key = GetOutputImageObjectID(pImageObject);

								if (m_mapImageObject.find(key) == m_mapImageObject.end())
								{
									//----------------------------------------
									// Key?? = (Viewpoint << 32) | ID
									//----------------------------------------
									// Viewpoint?? ????? sort???.
									// Viewpoint?? ID?? ????? ?????? ?? ???.
									//----------------------------------------
									m_mapImageObject.insert(
										IMAGEOBJECT_OUTPUT_MAP::value_type(
											key,	// key??
											pImageObject
										)
									);
								}
							}

							iImageObject++;
						}
					}
				}
			}
		}

	}

	//---------------------------------------------------------------
	// ?¿?? ????? ????? ???
	//---------------------------------------------------------------
	if (newFirstSector.x != m_FirstSector.x)
	{
		int	sY1 = newFirstSector.y + SECTOR_SKIP_UP,
			sY2 = newFirstSector.y + SECTOR_HEIGHT + 1,
			sY01 = m_FirstSector.y + SECTOR_SKIP_UP,
			sY02 = m_FirstSector.y + SECTOR_HEIGHT + 1,
			eraseX1, eraseX2,		// ??????? ????? ??? ??
			lastX,					// ????? ?????? ??(????? ???? ???)
			newX1, newX2;			// ???? ??Ÿ???? ????? ??? ??

		// ü??? ??? ??°??
		bool bCheckLast = true;
		bool bCheckErase = true;
		bool bCheckNew = true;

		//-----------------------------------
		// Zone?????? ?????? ???
		//-----------------------------------
		if (sY2 >= m_pZone->GetHeight())
		{
			sY2 = m_pZone->GetHeight() - 1;
		}

		if (sY1 < 0)
		{
			sY1 = 0;
		}

		if (sY02 >= m_pZone->GetHeight())
		{
			sY02 = m_pZone->GetHeight() - 1;
		}

		if (sY01 < 0)
		{
			sY01 = 0;
		}

		//-----------------------------------
		// ???????? ????? ???
		//-----------------------------------
		if (newFirstSector.x - m_FirstSector.x < 0)
		{
			// ????????? ??????? ????? ????? ???????????.
			// --> ???? ????? ?????? ????? ??????,
			//     ???? ????? ?????? ????? ???? ????? ??????? ?????.

			// ??? ?? ??
			lastX = newFirstSector.x + SECTOR_WIDTH + 1;	// ???? ????? ?????? ??

			// ?????? ?? ?? : lasyX+1 ~ eraseX
			eraseX1 = lastX + 1;
			eraseX2 = m_FirstSector.x + SECTOR_WIDTH + 1;	// ???? ????? ?????? ??			

			// ???? ??Ÿ???? ??
			newX1 = newFirstSector.x + SECTOR_SKIP_LEFT;
			newX2 = m_FirstSector.x + SECTOR_SKIP_LEFT - 1;

			//------------------------------------------------------
			// Zone?? ?????? ??? ??? Skip...
			//------------------------------------------------------
			if (lastX >= m_pZone->GetWidth())
			{
				//lastX = m_pZone->GetWidth()-1;
				bCheckLast = false;
			}

			if (eraseX1 >= m_pZone->GetWidth())
			{
				//eraseX1 = m_pZone->GetWidth()-1;
				bCheckErase = false;
			}
			else if (eraseX2 >= m_pZone->GetWidth())
			{
				eraseX2 = m_pZone->GetWidth() - 1;
			}

			if (newX2 < 0)
			{
				//newX2 = 0;	
				bCheckNew = false;
			}
			else if (newX1 < 0)
			{
				newX1 = 0;
			}

		}
		//-----------------------------------
		// ?????????? ????? ???
		//-----------------------------------
		else
		{
			// ????             ???      ???
			// eraseX1~eraseX2~lastX ~~~~ newX1~newX2
			// ??? ?? ??
			lastX = newFirstSector.x + SECTOR_SKIP_LEFT;	// ???? ????? ?????? ??

			// ??????? ?? : eraseX ~ lastX-1
			eraseX1 = m_FirstSector.x + SECTOR_SKIP_LEFT;	// ???? ????? ?????? ??
			eraseX2 = lastX - 1;

			// ???? ??Ÿ???? ??
			newX1 = m_FirstSector.x + SECTOR_WIDTH + 1 + 1;
			newX2 = newFirstSector.x + SECTOR_WIDTH + 1;

			//------------------------------------------------------
			// Zone?? ?????? ??? ??? Skip...
			//------------------------------------------------------				
			if (lastX < 0)
			{
				//lastX = 0;
				bCheckLast = false;
			}

			if (eraseX2 < 0)
			{
				//eraseX2 = 0;
				bCheckErase = false;
			}
			else if (eraseX1 < 0)
			{
				eraseX1 = 0;
			}

			if (newX1 >= m_pZone->GetWidth())
			{
				//newX1 = m_pZone->GetWidth()-1;
				bCheckNew = false;
			}
			else if (newX2 >= m_pZone->GetWidth())
			{
				newX2 = m_pZone->GetWidth() - 1;
			}
		}


		//----------------------------------------------
		// erase1 ~ erase2?????? ??????(?) ???.
		//----------------------------------------------
		// ??? m_mapImageObject???? ?????.
		//----------------------------------------------
		if (bCheckErase)
		{
			for (x = eraseX1; x <= eraseX2; x++)
			{
				for (y = sY01; y <= sY02; y++)
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					// ImageObject?? ????.. ???~ ???????.
					// ??, ????? ??????? ??´?.
					if (sector.IsExistImageObject())
					{
						OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

						// Sector?? ??? ??? ImageObject???? ??????.
						for (i = 0; i < sector.GetImageObjectSize(); i++)
						{
							MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

							//----------------------------------------
							// Key?? = (Viewpoint << 32) | ID
							//----------------------------------------
							// ??? ????? ???????? ?????? ????.
							IMAGEOBJECT_OUTPUT_MAP::iterator iDelete = m_mapImageObject.find(GetOutputImageObjectID(pImageObject));
							if (iDelete != m_mapImageObject.end())
							{
								m_mapImageObject.erase(iDelete);
							}

							iImageObject++;
						}
					}
				}
			}
		}

		//----------------------------------------------
		// lastX?? ??? ??????.
		//----------------------------------------------
		if (bCheckLast)
		{
			for (y = sY1; y <= sY2; y++)
			{
				const MSector& sector = m_pZone->GetSector(lastX, y);

				// ImageObject?? ??????..
				if (sector.IsExistImageObject())
				{
					OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

					// Sector?? ??? ??? ImageObject???? ??????.
					for (i = 0; i < sector.GetImageObjectSize(); i++)
					{
						MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

						// ??? ?????? ??????.. ??????.
						//if (pImageObject->GetViewpoint()==y				// ?????? ??
							//|| y==sY2-1									// ????
							//|| pImageObject->GetViewpoint() < sY1)		// ù??
						{
							// ??? ????? ???????? ?????? ???.
							QWORD key = GetOutputImageObjectID(pImageObject);

							if (m_mapImageObject.find(key)
								== m_mapImageObject.end())
							{
								//----------------------------------------
								// Key?? = (Viewpoint << 32) | ID
								//----------------------------------------
								// Viewpoint?? ????? sort???.
								// Viewpoint?? ID?? ????? ?????? ?? ???.
								//----------------------------------------
								m_mapImageObject.insert(
									IMAGEOBJECT_OUTPUT_MAP::value_type(
										key,	// key??
										pImageObject
									)
								);
							}
						}

						iImageObject++;
					}
				}
			}
		}

		//----------------------------------------------
		// ???? ??Ÿ?? ??? : newX1 ~ newX2
		//----------------------------------------------
		if (bCheckNew)
		{
			for (x = newX1; x <= newX2; x++)
			{
				for (y = sY1; y <= sY2; y++)
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					// ImageObject?? ??????..
					if (sector.IsExistImageObject())
					{
						OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

						// Sector?? ??? ??? ImageObject???? ??????.
						for (i = 0; i < sector.GetImageObjectSize(); i++)
						{
							MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

							// ??? ?????? ??????.. ??????.
							//if (viewPoint<=y			// ???? ??
							//	|| viewPoint > sY2)		// ????
								//|| viewPoint < newX1)		// ù??
							{
								// ??? ????? ???????? ?????? ???.
								QWORD key = GetOutputImageObjectID(pImageObject);

								if (m_mapImageObject.find(key)
									== m_mapImageObject.end())
								{
									//----------------------------------------
									// Key?? = (Viewpoint << 32) | ID
									//----------------------------------------
									// Viewpoint?? ????? sort???.
									// Viewpoint?? ID?? ????? ?????? ?? ???.
									//----------------------------------------
									m_mapImageObject.insert(
										IMAGEOBJECT_OUTPUT_MAP::value_type(
											key,	// key??
											pImageObject
										)
									);
								}
							}

							iImageObject++;
						}
					}
				}
			}
		}
	}

	//#ifdef OUTPUT_DEBUG
		//	DEBUG_ADD("[TempDebug] After Determine ImageObject");
	//#endif	
}

//----------------------------------------------------------------------
// Is this corpse lying flat on the ground?
//----------------------------------------------------------------------
// A corpse sprite is a floor decal once its death animation has run out:
// the pooled blood is painted into the sprite itself, so it has to be
// drawn with the ground layer, before any actor. Otherwise a corpse one
// row in front paints its blood over the feet of whoever stands behind it.
//
// While the death animation is still playing the creature is upright and
// has real height, so it keeps its normal place in the row-sorted pass.
//----------------------------------------------------------------------
static bool
IsSettledCorpse(const MItem* pItem)
{
	if (pItem == NULL || pItem->GetItemClass() != ITEM_CLASS_CORPSE)
	{
		return false;
	}

	const MCreature* pCreature = ((const MCorpse*)pItem)->GetCreature();

	if (pCreature == NULL)
	{
		return false;
	}

	return pCreature->GetActionCount() >= pCreature->GetActionCountMax();
}

//----------------------------------------------------------------------
// Draw Zone
//----------------------------------------------------------------------
// Pixel ????? (Xp, Yp)?? 
//       ????? (0, 0)?? ?????? ??? ?????? ????? ??????.
//
// pSurface?? (X,Y)?? Zone?? (Xp,Yp)?????? ??????.
//----------------------------------------------------------------------
//
// ?????? ????? Tile?? ?????? ???? Tile?? ???????? ??,
// ??? Object?? ??????.
//
//----------------------------------------------------------------------
void
MTopView::DrawZone(int firstPointX, int firstPointY)
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawZone");
#endif

	//-------------------------------------------------
	// ItemName list?? ???????.
	//-------------------------------------------------
	ClearItemNameList();


	//-------------------------------------------------
	// ????? ?????? ?????°??
	//-------------------------------------------------
	BOOL bDrawCreatureMyName = FALSE;	// ?? ???
	//BOOL bDrawCreatureName	= FALSE;	// ?? ???

	// mouse?? ???õ? ?????..
	m_pSelectedCreature = NULL;

	// ????? ????? ???ó ???? ????
	m_DrawNameCreatureInfoVec.clear();

	//-------------------------------------------------
	//
	// Object?? ?????? ???? ????
	//
	//-------------------------------------------------
	// pointer variable
	//MCreature*		pCreature;
	MItem* pItem;

	// sprite point
	POINT	tilePoint, point;
	RECT	rect;

	int x;
	int y;
	int i;

	//-------------------------------------------------
	// Player?? ?????? ImageObject?? ???? ????
	//-------------------------------------------------
	//if (m_pZone->GetSector(g_pPlayer->GetX(), g_pPlayer->GetY()).IsImageObjectAndNotBlock())
	//{
		//m_BehindImageObjectID = m_pZone->GetSector(g_pPlayer->GetX(), g_pPlayer->GetY()).GetImageObject()->GetImageObjectID();
	//}
	//else
	//{
		//m_BehindImageObjectID = IMAGEOBJECTID_NULL;
	//}


	//-------------------------------------------------
	// ù Sector?? ??µ? ??? ????(smooth scroll?? ?????)
	// Player?? ????? ??????? ??? ?????.
	//-------------------------------------------------
	m_PlusPoint.x = g_pPlayer->GetSX() - firstPointX;
	m_PlusPoint.y = g_pPlayer->GetSY() - firstPointY;
	firstPointX -= g_pPlayer->GetSX();
	firstPointY -= g_pPlayer->GetSY();


	//---------------------------------------------------------------	
	// firstPoint?? ??µ? Zone?????? ù??° Sector
	//---------------------------------------------------------------	
	// Player?? ??????? ??? ????????? ??????
	// ????~~~~ ??~ ???? ??????.
	//---------------------------------------------------------------	
	POINT	firstSector;
	firstSector.x = g_pPlayer->GetX() + SECTOR_SKIP_PLAYER_LEFT;
	firstSector.y = g_pPlayer->GetY() + SECTOR_SKIP_PLAYER_UP;

	//---------------------------------------------------------------	
	// Zone?? ??? ???? ??,
	// ???? ??? ?? ????? ???..
	//---------------------------------------------------------------	
	// X ??? ?????..
	//---------------------------------------------------------------	
	if (firstSector.x <= 0)
	{
		if (firstSector.x == 0)
		{
			if (g_pPlayer->GetSX() < 0)
			{
				m_PlusPoint.x = 0;
				firstPointX = 0;
			}
		}
		else
		{
			m_PlusPoint.x = 0;
			firstPointX = 0;
		}

		firstSector.x = 0;
	}
	if (firstSector.x + SECTOR_WIDTH + 1 >= m_pZone->GetWidth())
	{
		if (firstSector.x + SECTOR_WIDTH + 1 == m_pZone->GetWidth())
		{
			if (g_pPlayer->GetSX() > 0)
			{
				m_PlusPoint.x = 0;
				firstPointX = 0;
			}
		}
		else
		{
			m_PlusPoint.x = 0;
			firstPointX = 0;
		}

		firstSector.x = m_pZone->GetWidth() - SECTOR_WIDTH - 1;
	}

	//---------------------------------------------------------------	
	// Y ??? ?????
	//---------------------------------------------------------------	
	if (firstSector.y <= 0)
	{
		if (firstSector.y == 0)
		{
			if (g_pPlayer->GetSY() < 0)
			{
				m_PlusPoint.y = 0;
				firstPointY = 0;
			}
		}
		else
		{
			m_PlusPoint.y = 0;
			firstPointY = 0;
		}

		firstSector.y = 0;
	}
	else if (firstSector.y + SECTOR_HEIGHT >= m_pZone->GetHeight())
	{
		if (firstSector.y + SECTOR_HEIGHT == m_pZone->GetHeight())
		{
			if (g_pPlayer->GetSY() > 0)
			{
				m_PlusPoint.y = 0;
				firstPointY = 0;
			}
		}
		else
		{
			m_PlusPoint.y = 0;
			firstPointY = 0;
		}

		firstSector.y = m_pZone->GetHeight() - SECTOR_HEIGHT;
	}

	//---------------------------------------------------------------
	// ???? ??Ÿ????? ????? ???????? ImageObject???
	//---------------------------------------------------------------
	// 2001.8.22

	__BEGIN_PROFILE("UpdateImageObject")

		UpdateImageObject(firstSector);

	__END_PROFILE("UpdateImageObject")


		//---------------------------------------------------------------
		// ?????? ????? ?????? ????..
		// ??? ???? ù ????? ??µ?? Sector?? ???
		//---------------------------------------------------------------
		m_FirstSector = firstSector;

	//---------------------------------------------------------------
	// ????? (0,0)?? ??Ÿ???? Zone?? Pixel???
	//---------------------------------------------------------------
	m_FirstZonePixel = MapToPixel(firstSector.x, firstSector.y);
	m_FirstZonePixel.x += m_PlusPoint.x;
	m_FirstZonePixel.y += m_PlusPoint.y;

#ifdef OUTPUT_DEBUG		
	if (g_pPlayer->GetX() < 0
		|| g_pPlayer->GetY() < 0
		|| g_pPlayer->GetX() >= g_pZone->GetWidth()
		|| g_pPlayer->GetY() >= g_pZone->GetHeight()
		|| m_FirstZonePixel.x > 12000 || m_FirstZonePixel.y > 6144
		|| m_FirstZonePixel.x < 0 || m_FirstZonePixel.y < 0)
	{
		if (g_pDebugMessage)
		{
			DEBUG_ADD_FORMAT("[Error]SX,SY-DrawZone:Player=(%d, %d), s(%d,%d)", g_pPlayer->GetX(), g_pPlayer->GetY(), g_pPlayer->GetSX(), g_pPlayer->GetSY());
		}
	}
#endif


	//---------------------------------------------------------------
	// Player?? ?????? ImageObject???? ó???? Filter?? ??? ???
	//---------------------------------------------------------------
	m_FilterPosition.x = g_pPlayer->GetPixelX() - m_FirstZonePixel.x - (m_ImageObjectFilter.GetWidth() >> 1) + TILE_X_HALF;
	m_FilterPosition.y = g_pPlayer->GetPixelY() - m_FirstZonePixel.y - (m_ImageObjectFilter.GetHeight() >> 1)
		- (g_pPlayer->IsFlyingCreature() ? 3 : 1) * TILE_Y;
	//filterPoint.x = 310;	// 410 - 100;
	//filterPoint.y = 116;	// 204 - 100;


	//short	sX,
	//		sY = (short)firstSector.y;

	// ????? Surface???? ???	
	tilePoint.y = firstPointY;


	//--------------------------------------------------
	// TileSurface?? ???? : m_TileSurfaceFirstPixelXY 
	//                      + (TILESURFACE_WIDTH+ TILESURFACE_HEIGHT)
	//
	// ???? ????? ????   : m_FirstZonePixel + (800,600)
	//--------------------------------------------------
	//
	// ???? ??? ?????? TileSurface?? ????? 
	// TileSurface?? ???? ???? ?????? ???.
	//
	//
	//--------------------------------------------------
	__BEGIN_PROFILE("ReuseTileSurface")

		RECT rectTileSurface;
	RECT rectScreen;
	RECT	rectReuse;

	// TileSurface?? Zone?????? ????
	rectTileSurface.left = m_TileSurfaceFirstZonePixelX;
	rectTileSurface.top = m_TileSurfaceFirstZonePixelY;
	rectTileSurface.right = m_TileSurfaceFirstZonePixelX + TILESURFACE_WIDTH;
	rectTileSurface.bottom = m_TileSurfaceFirstZonePixelY + TILESURFACE_HEIGHT;

	// ???? ????? ????
	rectScreen.left = m_FirstZonePixel.x;
	rectScreen.top = m_FirstZonePixel.y;
	rectScreen.right = m_FirstZonePixel.x + CLIPSURFACE_WIDTH;
	rectScreen.bottom = m_FirstZonePixel.y + CLIPSURFACE_HEIGHT;

	// ???? ??? ??¿? ????? TileSurface?? ?????? ?????.
	// ????? (0,0)?? ?? TileSurface?????? ù ??
	rectReuse.left = rectScreen.left - rectTileSurface.left;
	rectReuse.top = rectScreen.top - rectTileSurface.top;
	rectReuse.right = rectReuse.left + CLIPSURFACE_WIDTH;
	rectReuse.bottom = rectReuse.top + CLIPSURFACE_HEIGHT;

	//----------------------------------------------------------------	
	// rectReuse?? ?????? ????? ?????????? ?????? ???.
	//----------------------------------------------------------------
	/*
	// ????
	if (rectReuse.top != 0)
	{
		rect.left = 0;
		rect.right = SURFACE_WIDTH;
		rect.top = 0;
		rect.bottom = rectReuse.top;

		m_pSurface->FillRect(&rect, 0);
	}

	// ?????
	if (rectReuse.bottom != SURFACE_HEIGHT)
	{
		rect.left = 0;
		rect.right = SURFACE_WIDTH;
		rect.top = rectReuse.bottom;
		rect.bottom = SURFACE_HEIGHT;

		m_pSurface->FillRect(&rect, 0);
	}

	// ????
	if (rectReuse.left != 0)
	{
		rect.left = 0;
		rect.right = rectReuse.left;
		rect.top = rectReuse.top;
		rect.bottom = rectReuse.bottom;

		m_pSurface->FillRect(&rect, 0);
	}

	// ??????
	if (rectReuse.right != SURFACE_WIDTH)
	{
		rect.left = rectReuse.right;
		rect.right = SURFACE_WIDTH;
		rect.top = rectReuse.top;
		rect.bottom = rectReuse.bottom;

		m_pSurface->FillRect(&rect, 0);
	}
	*/



	//--------------------------------------------------
	// TileSurface?? ???? ?? ?? ????? ü????.
	//--------------------------------------------------
	// ?????? ??? ???	
	BYTE bLack = 0;

	//bLack |= (rectScreen.left <= rectTileSurface.left);				// 1
	//bLack |= (rectScreen.right >= rectTileSurface.right) << 1;		// 2
	//bLack |= (rectScreen.top <= rectTileSurface.top) << 2;			// 4
	//bLack |= (rectScreen.bottom >= rectTileSurface.bottom) << 3;	// 8
	int leftGap = rectScreen.left - rectTileSurface.left;
	int rightGap = rectScreen.right - rectTileSurface.right;
	int topGap = rectScreen.top - rectTileSurface.top;
	int bottomGap = rectScreen.bottom - rectTileSurface.bottom;

	if (leftGap <= 0) bLack |= FLAG_TILESURFACE_LACK_LEFT;
	if (rightGap >= 0) bLack |= FLAG_TILESURFACE_LACK_RIGHT;
	if (topGap <= 0) bLack |= FLAG_TILESURFACE_LACK_UP;
	if (bottomGap >= 0) bLack |= FLAG_TILESURFACE_LACK_DOWN;

	//DEBUG_ADD_FORMAT("[DRAW LACK]  (%d, %d)  %s %s %s %s", 
	//	g_pPlayer->GetX(), g_pPlayer->GetY(),
	//	((bLack & FLAG_TILESURFACE_LACK_LEFT)? "Left":" "),
	//	((bLack & FLAG_TILESURFACE_LACK_RIGHT)? "Right":" "),
	//	((bLack & FLAG_TILESURFACE_LACK_UP)? "Up":" "),
	//	((bLack & FLAG_TILESURFACE_LACK_DOWN)? "Down":" "));

	//---------------------------------------------------------------------
	//
	// ???? ????? TileSurface?? ????? ???
	//
	//---------------------------------------------------------------------
	if (bLack == 0)
	{
		//....
		//---------------------------------------------------------------
		// ???? ??Ÿ????? ????? ???????? ImageObject???
		//---------------------------------------------------------------
		// 2001.8.22
		//UpdateImageObject(firstSector);	

	}
	/*
	//---------------------------------------------------------------------
	//
	// ??? ???? ?????????? ???
	//
	//---------------------------------------------------------------------
	else if (abs(leftGap) >= (TILE_X<<2)
			|| abs(rightGap) >= (TILE_X<<2)
			|| abs(topGap) >= (TILE_Y<<2)
			|| abs(bottomGap) >= (TILE_Y<<2)
			)
	{
		DEBUG_ADD("[DrawZone] Too Far Move");

		m_mapImageObject.clear();
		m_mapCreature.clear();

		// Tile??ü?? ??? ??????.
		DrawTileSurface();

		DetermineImageObject();

		m_bFirstTileDraw = false;

		DEBUG_ADD("[DrawZone] Too Far Move - DrawTileSurface OK");
	}
	*/
	//---------------------------------------------------------------------
	//
	// ???? ????? TileSurface?? ?????? ??? ???
	//
	//---------------------------------------------------------------------
	else
	{
		//---------------------------------------------------------------
		// ???? ??Ÿ????? ????? ???????? ImageObject???
		//---------------------------------------------------------------
		// 2001.8.22
		//UpdateImageObject(firstSector);	

		//----------------------------------------
		// ?????????? ??????µ? ????? ????
		//----------------------------------------
		BYTE	changeCount = 0;	// 1 or 2
		int		sX1[2], sX2[2], sY1[2], sY2[2];
		POINT	firstTilePoint[2];


		//--------------------------
		// LEFT....
		//--------------------------
		if (bLack & FLAG_TILESURFACE_LACK_LEFT)
		{
			//--------------------------
			// LEFT + UP ????
			//--------------------------
			if (bLack & FLAG_TILESURFACE_LACK_UP)
			{
				//--------------------------------------------------
				// ???? Tile Surface????
				// ??? ????????? Tile Surface???? ????? ?? 
				// ??? ????? ??????.			
				//--------------------------------------------------
				point.x = TILESURFACE_OUTLINE_LEFT;
				point.y = TILESURFACE_OUTLINE_UP;
				rect.left = 0;
				rect.top = 0;
				rect.right = TILESURFACE_OUTLINE_RIGHT;
				rect.bottom = TILESURFACE_OUTLINE_DOWN;
				m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

				//--------------------------------------------------
				//
				// ?????? ????? ??????.
				//
				//--------------------------------------------------				

				changeCount = 2;

				//--------------------------------------------------
				// Left???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[0] = m_TileSurfaceFirstSectorX - TILESURFACE_SECTOR_EDGE;
				sY1[0] = m_TileSurfaceFirstSectorY - TILESURFACE_SECTOR_EDGE;
				sX2[0] = sX1[0] + TILESURFACE_SECTOR_EDGE;
				sY2[0] = sY1[0] + TILESURFACE_SECTOR_HEIGHT;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[0].x = 0;
				firstTilePoint[0].y = 0;

				//--------------------------------------------------
				// Up???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[1] = m_TileSurfaceFirstSectorX - TILESURFACE_SECTOR_EDGE;
				sY1[1] = m_TileSurfaceFirstSectorY - TILESURFACE_SECTOR_EDGE;
				sX2[1] = sX1[1] + TILESURFACE_SECTOR_WIDTH;
				sY2[1] = sY1[1] + TILESURFACE_SECTOR_EDGE;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[1].x = 0;
				firstTilePoint[1].y = 0;


				//--------------------------------------------------
				// ??¿? ????? TileSurface ???? ??? ????
				//--------------------------------------------------
				rectReuse.left += TILESURFACE_OUTLINE_LEFT;
				rectReuse.right += TILESURFACE_OUTLINE_LEFT;
				rectReuse.top += TILESURFACE_OUTLINE_UP;
				rectReuse.bottom += TILESURFACE_OUTLINE_UP;

				//--------------------------------------------------
				// ????? TileSurface?? ???? ??? ????
				//--------------------------------------------------
				m_TileSurfaceFirstSectorX -= TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelX -= TILESURFACE_OUTLINE_LEFT;
				m_TileSurfaceFirstSectorY -= TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelY -= TILESURFACE_OUTLINE_UP;
			}
			//--------------------------
			// LEFT + DOWN ????
			//--------------------------
			else if (bLack & FLAG_TILESURFACE_LACK_DOWN)
			{
				//--------------------------------------------------
				// ???? Tile Surface????
				// ??? ????????? Tile Surface???? ????? ?? 
				// ??? ????? ??????.			
				//--------------------------------------------------
				point.x = TILESURFACE_OUTLINE_LEFT;
				point.y = 0;
				rect.left = 0;
				rect.top = TILESURFACE_OUTLINE_UP;
				rect.right = TILESURFACE_OUTLINE_RIGHT;
				rect.bottom = TILESURFACE_HEIGHT;
				m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

				//--------------------------------------------------
				//
				// ?????? ????? ??????.
				//
				//--------------------------------------------------

				changeCount = 2;

				//--------------------------------------------------
				// Left???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[0] = m_TileSurfaceFirstSectorX - TILESURFACE_SECTOR_EDGE;
				sY1[0] = m_TileSurfaceFirstSectorY + TILESURFACE_SECTOR_EDGE;
				sX2[0] = sX1[0] + TILESURFACE_SECTOR_EDGE;
				sY2[0] = sY1[0] + TILESURFACE_SECTOR_HEIGHT;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[0].x = 0;
				firstTilePoint[0].y = 0;

				//--------------------------------------------------
				// Down???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[1] = m_TileSurfaceFirstSectorX - TILESURFACE_SECTOR_EDGE;
				sY1[1] = m_TileSurfaceFirstSectorY + TILESURFACE_SECTOR_HEIGHT;
				sX2[1] = sX1[1] + TILESURFACE_SECTOR_WIDTH;
				sY2[1] = sY1[1] + TILESURFACE_SECTOR_EDGE;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[1].x = 0;
				firstTilePoint[1].y = TILESURFACE_OUTLINE_DOWN;

				//--------------------------------------------------
				// ??¿? ????? TileSurface ???? ??? ????
				//--------------------------------------------------
				rectReuse.left += TILESURFACE_OUTLINE_LEFT;
				rectReuse.right += TILESURFACE_OUTLINE_LEFT;
				rectReuse.top -= TILESURFACE_OUTLINE_UP;
				rectReuse.bottom -= TILESURFACE_OUTLINE_UP;

				//--------------------------------------------------
				// ????? TileSurface?? ???? ??? ????
				//--------------------------------------------------
				m_TileSurfaceFirstSectorX -= TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelX -= TILESURFACE_OUTLINE_LEFT;
				m_TileSurfaceFirstSectorY += TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelY += TILESURFACE_OUTLINE_UP;
			}
			//--------------------------
			// LEFT ????
			//--------------------------
			else
			{
				//--------------------------------------------------
				// ???? Tile Surface????
				// ??? ????????? Tile Surface???? ????? ?? 
				// ??? ????? ??????.			
				//--------------------------------------------------
				point.x = TILESURFACE_OUTLINE_LEFT;
				point.y = 0;
				rect.left = 0;
				rect.top = 0;
				rect.right = TILESURFACE_OUTLINE_RIGHT;
				rect.bottom = TILESURFACE_HEIGHT;
				m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

				//--------------------------------------------------
				//
				// ?????? ????? ??????.
				//
				//--------------------------------------------------

				changeCount = 1;

				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[0] = m_TileSurfaceFirstSectorX - TILESURFACE_SECTOR_EDGE;
				sY1[0] = m_TileSurfaceFirstSectorY;
				sX2[0] = sX1[0] + TILESURFACE_SECTOR_EDGE;
				sY2[0] = sY1[0] + TILESURFACE_SECTOR_HEIGHT;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[0].x = 0;
				firstTilePoint[0].y = 0;

				//--------------------------------------------------
				// ??¿? ????? TileSurface ???? ??? ????
				//--------------------------------------------------
				rectReuse.left += TILESURFACE_OUTLINE_LEFT;
				rectReuse.right += TILESURFACE_OUTLINE_LEFT;

				//--------------------------------------------------
				// ????? TileSurface?? ???? ??? ????
				//--------------------------------------------------
				m_TileSurfaceFirstSectorX -= TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelX -= TILESURFACE_OUTLINE_LEFT;
			}
		}
		//--------------------------
		// RIGHT....
		//--------------------------
		else if (bLack & FLAG_TILESURFACE_LACK_RIGHT)
		{
			//--------------------------
			// RIGHT + UP ????
			//--------------------------
			if (bLack & FLAG_TILESURFACE_LACK_UP)
			{
				//--------------------------------------------------
				// ???? Tile Surface????
				// ??? ????????? Tile Surface???? ????? ?? 
				// ??? ????? ??????.			
				//--------------------------------------------------
				point.x = 0;
				point.y = TILESURFACE_OUTLINE_UP;
				rect.left = TILESURFACE_OUTLINE_LEFT;
				rect.top = 0;
				rect.right = TILESURFACE_WIDTH;
				rect.bottom = TILESURFACE_OUTLINE_DOWN;
				m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

				//--------------------------------------------------
				//
				// ?????? ????? ??????.
				//
				//--------------------------------------------------

				changeCount = 2;

				//--------------------------------------------------
				// Right???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[0] = m_TileSurfaceFirstSectorX + TILESURFACE_SECTOR_WIDTH;
				sY1[0] = m_TileSurfaceFirstSectorY - TILESURFACE_SECTOR_OUTLINE_UP;
				sX2[0] = sX1[0] + TILESURFACE_SECTOR_EDGE;
				sY2[0] = sY1[0] + TILESURFACE_SECTOR_HEIGHT;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[0].x = TILESURFACE_OUTLINE_RIGHT;
				firstTilePoint[0].y = 0;

				//--------------------------------------------------
				// UP???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[1] = m_TileSurfaceFirstSectorX + TILESURFACE_SECTOR_OUTLINE_LEFT;
				sY1[1] = m_TileSurfaceFirstSectorY - TILESURFACE_SECTOR_EDGE;
				sX2[1] = sX1[1] + TILESURFACE_SECTOR_WIDTH;
				sY2[1] = sY1[1] + TILESURFACE_SECTOR_EDGE;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[1].x = 0;
				firstTilePoint[1].y = 0;

				//--------------------------------------------------
				// ??¿? ????? TileSurface ???? ??? ????
				//--------------------------------------------------
				rectReuse.left -= TILESURFACE_OUTLINE_LEFT;
				rectReuse.right -= TILESURFACE_OUTLINE_LEFT;
				rectReuse.top += TILESURFACE_OUTLINE_UP;
				rectReuse.bottom += TILESURFACE_OUTLINE_UP;

				//--------------------------------------------------
				// ????? TileSurface?? ???? ??? ????
				//--------------------------------------------------
				m_TileSurfaceFirstSectorX += TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelX += TILESURFACE_OUTLINE_LEFT;
				m_TileSurfaceFirstSectorY -= TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelY -= TILESURFACE_OUTLINE_UP;
			}
			//--------------------------
			// RIGHT + DOWN ????
			//--------------------------
			else if (bLack & FLAG_TILESURFACE_LACK_DOWN)
			{
				//--------------------------------------------------
				// ???? Tile Surface????
				// ??? ????????? Tile Surface???? ????? ?? 
				// ??? ????? ??????.			
				//--------------------------------------------------
				point.x = 0;
				point.y = 0;
				rect.left = TILESURFACE_OUTLINE_LEFT;
				rect.top = TILESURFACE_OUTLINE_UP;
				rect.right = TILESURFACE_WIDTH;
				rect.bottom = TILESURFACE_HEIGHT;
				m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

				//--------------------------------------------------
				//
				// ?????? ????? ??????.
				//
				//--------------------------------------------------

				changeCount = 2;

				//--------------------------------------------------
				// Right???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[0] = m_TileSurfaceFirstSectorX + TILESURFACE_SECTOR_WIDTH;
				sY1[0] = m_TileSurfaceFirstSectorY + TILESURFACE_SECTOR_OUTLINE_UP;
				sX2[0] = sX1[0] + TILESURFACE_SECTOR_EDGE;
				sY2[0] = sY1[0] + TILESURFACE_SECTOR_HEIGHT;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[0].x = TILESURFACE_OUTLINE_RIGHT;
				firstTilePoint[0].y = 0;

				//--------------------------------------------------
				// Down???
				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[1] = m_TileSurfaceFirstSectorX + TILESURFACE_SECTOR_OUTLINE_LEFT;
				sY1[1] = m_TileSurfaceFirstSectorY + TILESURFACE_SECTOR_HEIGHT;
				sX2[1] = sX1[1] + TILESURFACE_SECTOR_WIDTH;
				sY2[1] = sY1[1] + TILESURFACE_SECTOR_EDGE;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[1].x = 0;
				firstTilePoint[1].y = TILESURFACE_OUTLINE_DOWN;

				//--------------------------------------------------
				// ??¿? ????? TileSurface ???? ??? ????
				//--------------------------------------------------
				rectReuse.left -= TILESURFACE_OUTLINE_LEFT;
				rectReuse.right -= TILESURFACE_OUTLINE_LEFT;
				rectReuse.top -= TILESURFACE_OUTLINE_UP;
				rectReuse.bottom -= TILESURFACE_OUTLINE_UP;

				//--------------------------------------------------
				// ????? TileSurface?? ???? ??? ????
				//--------------------------------------------------
				m_TileSurfaceFirstSectorX += TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelX += TILESURFACE_OUTLINE_LEFT;
				m_TileSurfaceFirstSectorY += TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelY += TILESURFACE_OUTLINE_UP;
			}
			//--------------------------
			// RIGHT ????
			//--------------------------
			else
			{
				//--------------------------------------------------
				// ???? Tile Surface????
				// ??? ????????? Tile Surface???? ????? ?? 
				// ??? ????? ??????.			
				//--------------------------------------------------
				point.x = 0;
				point.y = 0;
				rect.left = TILESURFACE_OUTLINE_LEFT;
				rect.top = 0;
				rect.right = TILESURFACE_WIDTH;
				rect.bottom = TILESURFACE_HEIGHT;
				m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

				//--------------------------------------------------
				//
				// ?????? ????? ??????.
				//
				//--------------------------------------------------

				changeCount = 1;

				//--------------------------------------------------
				// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
				//--------------------------------------------------			
				sX1[0] = m_TileSurfaceFirstSectorX + TILESURFACE_SECTOR_WIDTH;
				sY1[0] = m_TileSurfaceFirstSectorY;
				sX2[0] = sX1[0] + TILESURFACE_SECTOR_EDGE;
				sY2[0] = sY1[0] + TILESURFACE_SECTOR_HEIGHT;

				//--------------------------------------------------
				// ù??° ??? ???
				//--------------------------------------------------
				firstTilePoint[0].x = TILESURFACE_OUTLINE_RIGHT;
				firstTilePoint[0].y = 0;

				//--------------------------------------------------
				// ??¿? ????? TileSurface ???? ??? ????
				//--------------------------------------------------
				rectReuse.left -= TILESURFACE_OUTLINE_LEFT;
				rectReuse.right -= TILESURFACE_OUTLINE_LEFT;

				//--------------------------------------------------
				// ????? TileSurface?? ???? ??? ????
				//--------------------------------------------------
				m_TileSurfaceFirstSectorX += TILESURFACE_SECTOR_EDGE;
				m_TileSurfaceFirstZonePixelX += TILESURFACE_OUTLINE_LEFT;
			}
		}
		//--------------------------
		// UP ????
		//--------------------------
		else if (bLack & FLAG_TILESURFACE_LACK_UP)
		{
			//--------------------------------------------------
			// ???? Tile Surface????
			// ??? ????????? Tile Surface???? ????? ?? 
			// ??? ????? ??????.			
			//--------------------------------------------------
			point.x = 0;
			point.y = TILESURFACE_OUTLINE_UP;
			rect.left = 0;
			rect.top = 0;
			rect.right = TILESURFACE_WIDTH;
			rect.bottom = TILESURFACE_OUTLINE_DOWN;
			m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

			//--------------------------------------------------
			//
			// ?????? ????? ??????.
			//
			//--------------------------------------------------

			changeCount = 1;

			//--------------------------------------------------
			// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
			//--------------------------------------------------			
			sX1[0] = m_TileSurfaceFirstSectorX;
			sY1[0] = m_TileSurfaceFirstSectorY - TILESURFACE_SECTOR_EDGE;
			sX2[0] = sX1[0] + TILESURFACE_SECTOR_WIDTH;
			sY2[0] = sY1[0] + TILESURFACE_SECTOR_EDGE;

			//--------------------------------------------------
			// ù??° ??? ???
			//--------------------------------------------------
			firstTilePoint[0].x = 0;
			firstTilePoint[0].y = 0;

			//--------------------------------------------------
			// ??¿? ????? TileSurface ???? ??? ????
			//--------------------------------------------------
			rectReuse.top += TILESURFACE_OUTLINE_UP;
			rectReuse.bottom += TILESURFACE_OUTLINE_UP;

			//--------------------------------------------------
			// ????? TileSurface?? ???? ??? ????
			//--------------------------------------------------
			m_TileSurfaceFirstSectorY -= TILESURFACE_SECTOR_EDGE;
			m_TileSurfaceFirstZonePixelY -= TILESURFACE_OUTLINE_UP;
		}
		//--------------------------
		// DOWN ????
		//--------------------------
		else if (bLack & FLAG_TILESURFACE_LACK_DOWN)
		{
			//--------------------------------------------------
			// ???? Tile Surface????
			// ??? ????????? Tile Surface???? ????? ?? 
			// ??? ????? ??????.			
			//--------------------------------------------------
			point.x = 0;
			point.y = 0;
			rect.left = 0;
			rect.top = TILESURFACE_OUTLINE_UP;
			rect.right = TILESURFACE_WIDTH;
			rect.bottom = TILESURFACE_HEIGHT;
			m_pTileSurface->BltNoColorkey(&point, m_pTileSurface, &rect);

			//--------------------------------------------------
			//
			// ?????? ????? ??????.
			//
			//--------------------------------------------------

			changeCount = 1;

			//--------------------------------------------------
			// ????? sector (sX1,sY) ~ (sX2, sY2)???? ???
			//--------------------------------------------------			
			sX1[0] = m_TileSurfaceFirstSectorX;
			sY1[0] = m_TileSurfaceFirstSectorY + TILESURFACE_SECTOR_HEIGHT;
			sX2[0] = sX1[0] + TILESURFACE_SECTOR_WIDTH;
			sY2[0] = sY1[0] + TILESURFACE_SECTOR_EDGE;

			//--------------------------------------------------
			// ù??° ??? ???
			//--------------------------------------------------
			firstTilePoint[0].x = 0;
			firstTilePoint[0].y = TILESURFACE_OUTLINE_DOWN;

			//--------------------------------------------------
			// ??¿? ????? TileSurface ???? ??? ????
			//--------------------------------------------------
			rectReuse.top -= TILESURFACE_OUTLINE_UP;
			rectReuse.bottom -= TILESURFACE_OUTLINE_UP;

			//--------------------------------------------------
			// ????? TileSurface?? ???? ??? ????
			//--------------------------------------------------
			m_TileSurfaceFirstSectorY += TILESURFACE_SECTOR_EDGE;
			m_TileSurfaceFirstZonePixelY += TILESURFACE_OUTLINE_UP;
		}


		//------------------------------------------------------
		//
		//   ????????? Tile???? ???????..
		//
		//------------------------------------------------------
		int n;
		for (n = 0; n < changeCount; n++)
		{
			POINT tilePointTemp;

			//------------------------------------------------------
			// Zone?? ?????? ??? ??? Skip...
			//------------------------------------------------------
			if (sX1[n] < 0)
			{
				// Zone??? ????? ?????????? ????.
				rect.left = firstTilePoint[n].x;
				rect.top = 0;

				firstTilePoint[n].x += -sX1[n] * TILE_X;

				rect.right = firstTilePoint[n].x;
				rect.bottom = TILESURFACE_HEIGHT;

				m_pTileSurface->FillRect(&rect, 0);

				sX1[n] = 0;
			}

			if (sX2[n] > m_pZone->GetWidth())
			{
				sX2[n] = m_pZone->GetWidth();//-1;				

				// Zone??? ????? ?????????? ????.
				rect.left = firstTilePoint[n].x + (sX2[n] - sX1[n]) * TILE_X;
				rect.top = 0;
				rect.right = TILESURFACE_WIDTH;
				rect.bottom = TILESURFACE_HEIGHT;

				m_pTileSurface->FillRect(&rect, 0);
			}

			if (sY1[n] < 0)
			{
				// Zone??? ????? ?????????? ????.
				rect.left = firstTilePoint[n].x;
				rect.top = firstTilePoint[n].y;

				firstTilePoint[n].y += -sY1[n] * TILE_Y;

				rect.right = firstTilePoint[n].x + (sX2[n] - sX1[n]) * TILE_X;
				rect.bottom = firstTilePoint[n].y;
				m_pTileSurface->FillRect(&rect, 0);

				sY1[n] = 0;
			}

			if (sY2[n] > m_pZone->GetHeight())
			{
				// Zone??? ????? ?????????? ????.
				rect.left = firstTilePoint[n].x;
				rect.top = firstTilePoint[n].y;
				rect.right = firstTilePoint[n].x + (sX2[n] - sX1[n]) * TILE_X;
				rect.bottom = TILESURFACE_HEIGHT;
				m_pTileSurface->FillRect(&rect, 0);

				sY2[n] = m_pZone->GetHeight();//-1;
			}

			// ù??° ??			
			tilePointTemp.y = firstTilePoint[n].y;

			//---------------------------------------
			// LOCK
			//---------------------------------------
			if (!m_pTileSurface->Lock()) return;

			//char str[80];
			int endY = sY2[n];
			int endX = sX2[n];
			for (y = sY1[n]; y < endY; y++)
			{
				// ?? ???? ù??° Sector					
				tilePointTemp.x = firstTilePoint[n].x;

				for (x = sX1[n]; x < endX; x++)
				{
					//m_PreviousFogSpriteID[y][x] = SPRITEID_NULL;

					point = tilePointTemp;

					int spriteID = m_pZone->GetSector(x, y).GetSpriteID();

					if (spriteID == SPRITEID_NULL)
					{
#ifdef __DEBUG_OUTPUT__
						if (m_pZone->GetID() == 3001 && m_pZone->GetSector(x, y).IsBlockAny())
							m_pTileSurface->BltSprite(&point, &m_EtcSPK[1]);
						else
							m_pTileSurface->BltSprite(&point, &m_EtcSPK[SPRITEID_TILE_NULL]);
#else
						m_pTileSurface->BltSprite(&point, &m_EtcSPK[SPRITEID_TILE_NULL]);
#endif
					}
					else
					{
						CSprite& sprite = m_TileSPK[spriteID];

						//---------------------------------------
						// ID?? spriteID?? Tile?? Load???.
						//---------------------------------------
//						if (sprite.IsNotInit())
//						{
//							#ifdef	OUTPUT_DEBUG
//								if (g_pDebugMessage!=NULL)
//									sprintf(g_pDebugMessage->GetCurrent(), "[RunTimeLoading] Tile(%d,%d) - Draw : sprite=%d", x, y, spriteID);
//							#endif
//
//							m_TileSPKFile.seekg(m_TileSPKI[spriteID], ios::beg);
//							
//							//--------------------------------------------------
//							// Loading?? ?????? ???
//							//--------------------------------------------------
//							if (m_TileSPK[spriteID].LoadFromFile( m_TileSPKFile ))
//							{
//								#ifdef	OUTPUT_DEBUG
//									if (g_pDebugMessage!=NULL)
//									{
//										sprintf(g_pDebugMessage->GetCurrent(), "%s ...OK", g_pDebugMessage->GetCurrent());
//										g_pDebugMessage->Next();
//									}
//								#endif
//							}
//							//--------------------------------------------------
//							// ?????? ??? --> ??? Loading??? ??? ??????.				
//							//--------------------------------------------------
//							/*
//							// 2001.8.20 ???ó??
//							else
//							{
//								#ifdef	OUTPUT_DEBUG
//									if (g_pDebugMessage!=NULL)
//									{
//										sprintf(g_pDebugMessage->GetCurrent(), "%s ...Fail & Wait Loading", g_pDebugMessage->GetCurrent());
//										g_pDebugMessage->Next();
//									}
//								#endif
//
//		
//								// file thread ?????? ??????.
//								//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_HIGHEST);
//
//								MLoadingSPKWorkNode3* pNode = new MLoadingSPKWorkNode3(spriteID, m_TileSPKI[spriteID]);
//								pNode->SetSPK( &m_TileSPK, FILE_SPRITE_TILE );
//								pNode->SetType( 1 );
//								g_pLoadingThread->SetPriority( THREAD_PRIORITY_HIGHEST );
//								g_pLoadingThread->AddFirst( pNode );
//
//								// Thread???? Loading?? ?????????? ??????.
//								int waitCount = 0;
//								while (1)
//								{								
//									DEBUG_ADD_FORMAT( "Check Load id=%d", spriteID );
//
//									if (m_TileSPK[spriteID].IsInit())
//									{
//										DEBUG_ADD( "Is Init" );
//
//										break;
//									}
//									else
//									{
//										DEBUG_ADD( "Is Not Init" );
//									}
//
//									
//									//if (++waitCount == 1000)
//									//{
//									//	break;
//									//}											
//								}
//								//while (!m_TileSPK[spriteID].LoadFromFile( m_TileSPKFile ));
//
//								// file thread ?????? ?????.
//								//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_BELOW_NORMAL);	
//								g_pLoadingThread->SetPriority( THREAD_PRIORITY_LOWEST );
//
//							}
//							*/
//							
//							//if (sprite.IsInit())
//							//{
//							//	m_pTileSurface->BltSprite(&point, &sprite);
//							//}
//						}
						//else 
						//{
							// ???~~!!!!!!! ??? ??? ??´?~!!!
//							POINT pointTempTemp = point;
//							m_pTileSurface->BltSprite(&pointTempTemp, &m_EtcSPK[SPRITEID_TILE_NULL]);

						m_pTileSurface->BltSprite(&point, &sprite);
						//}
						//m_pTileSurface->BltSprite(&point, &m_SpritePack[ 3 ]);

						//sprintf(str, "(%d,%d)", x, y);			
						//m_pTileSurface->GDI_Text(point.x, point.y, str, 0);
					}


					// ???????? ??? ???
					tilePointTemp.x += TILE_X;
				}

				// ???? ??
				tilePointTemp.y += TILE_Y;
			}

			//---------------------------------------
			// UNLOCK
			//---------------------------------------
			m_pTileSurface->Unlock();
		}

		//		m_bFogChanged = true;
	}

	//----------------------------------------------------------------
	//
	// Tile Image to Current Surface
	//
	//----------------------------------------------------------------	
	point.x = 0;
	point.y = 0;


	////m_pSurface->BltDarkness(&point, m_pTileSurface, &rectReuse, DARK_VALUE);

	//----------------------------------------------------------------	
	//
	// [ TEST CODE ]  Perspective
	//
	//----------------------------------------------------------------	
	/*
#ifdef OUTPUT_DEBUG

	if (!g_pDXInput->KeyDown(DIK_LSHIFT))
	{
		m_pSurface->BltNoColorkey(&point, m_pTileSurface, &rectReuse);
	}
	else
	{
		if (CDirect3D::IsHAL())
		{

			// TileTexture ????
			CSpriteSurface* m_pTileTexture = new CSpriteSurface;
			m_pTileTexture->InitTextureSurface(256, 256);

			RECT destRect = { 0, 0, 256, 256 };

			m_pTileTexture->Blt(&destRect, m_pTileSurface, &rectReuse);

			CDirect3D::GetDevice()->SetTexture(0, m_pTileTexture->GetSurface());
			//CDirect3D::GetDevice()->SetTexture(0, NULL);

			static CD3DObject object;
			static bool first = true;

			if (first)
			{
				//--------------------------------------------------------
				// Light ????
				//--------------------------------------------------------
				D3DLIGHT7 light;
				ZeroMemory( &light, sizeof(D3DLIGHT7) );
				light.dltType        = D3DLIGHT_POINT;
				light.dcvDiffuse.r   = 1.0f;
				light.dcvDiffuse.g   = 1.0f;
				light.dcvDiffuse.b   = 1.0f;
				light.dcvSpecular    = light.dcvDiffuse;
				light.dvPosition.x   = light.dvDirection.x = 0.0f;
				light.dvPosition.y   = light.dvDirection.y = 8.5f;
				light.dvPosition.z   = light.dvDirection.z = -5.0f;
				light.dvAttenuation0 = 1.0f;
				light.dvRange        = D3DLIGHT_RANGE_MAX;

				CDirect3D::GetDevice()->SetLight( 0, &light );
				CDirect3D::GetDevice()->LightEnable( 0, TRUE );
				CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );


				// world matrix?? ?????..
				D3DMATRIX matWorld;
				CDirect3D::GetDevice()->GetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );
				CD3DMath::MultiplyRotateXMatrix(matWorld, 25*g_DEGTORAD);
				CDirect3D::GetDevice()->SetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );

				////object.CreatePlane(2.0f);
				object.CreatePlane(6.5f);
				object.Move(0, 0.5f, 0);


				D3DMATERIAL7 mtrl;
				mtrl.dcvDiffuse.r = mtrl.dcvAmbient.r = 1.0f;
				mtrl.dcvDiffuse.g = mtrl.dcvAmbient.g = 1.0f;
				mtrl.dcvDiffuse.b = mtrl.dcvAmbient.b = 1.0f;
				mtrl.dcvDiffuse.a = mtrl.dcvAmbient.a = 1.0f;

				//g_Background.Rotate( 90.0f*g_DEGTORAD, 0, 0 );
				mtrl.dcvEmissive.r = 0;
				mtrl.dcvEmissive.g = 0;
				mtrl.dcvEmissive.b = 0;
				object.SetMaterial( mtrl );

				first = false;
			}

			object.Draw();

			delete m_pTileTexture;
		}
	}

#else
*/


	bool bDrawBackGround = DrawEvent();

	__BEGIN_PROFILE("ReuseBltTileSurface")
		// 2004, 9, 3, sobeit add start - Ÿ?? ????? ????-_-;
	//	event = g_pEventManager->GetEventByFlag(EVENTFLAG_CLOUD_BACKGROUND);
		if (g_pPlayer->IsShowAdamCloud())
		{
			CDirectDrawSurface* pCloudSurface = g_pEventManager->GetEventBackground(EVENTBACKGROUNDID_CLOUD);
			if (pCloudSurface != NULL)
			{
				int CloudPos = g_CurrentFrame % SURFACE_WIDTH;
				POINT CloudPoint = { 0,0 };
				RECT CloudRect = { CloudPos, 0, SURFACE_WIDTH - 1, SURFACE_HEIGHT - 1 };
				if (CloudPos != (SURFACE_WIDTH - 1))
					m_pSurface->BltNoColorkey(&CloudPoint, pCloudSurface, &CloudRect);
				if (CloudPos != 0)
				{
					CloudPoint.x = (SURFACE_WIDTH - 1) - CloudPos;
					CloudRect.left = 0;
					CloudRect.right = CloudPos;
					m_pSurface->BltNoColorkey(&CloudPoint, pCloudSurface, &CloudRect);
				}
				if (bDrawBackGround)
					m_pSurface->Blt(&point, m_pTileSurface, &rectReuse);
			}
			else
				if (bDrawBackGround)
					m_pSurface->BltNoColorkey(&point, m_pTileSurface, &rectReuse);
		}
		else
			// 2004, 9, 3, sobeit add end - Ÿ?? ????? ????-_-;
			if (bDrawBackGround)
				m_pSurface->BltNoColorkey(&point, m_pTileSurface, &rectReuse);


	__END_PROFILE("ReuseBltTileSurface")

		__END_PROFILE("ReuseTileSurface")
		//#endif

		POINT tilePointTemp;
	int sX1, sX2, sY1, sY2;

	//------------------------------------------------------
	// Object?? ?????? ???? ??? ??? ????
	//------------------------------------------------------
	sX1 = firstSector.x + SECTOR_SKIP_LEFT;
	sY1 = firstSector.y + SECTOR_SKIP_UP;
	sX2 = firstSector.x + SECTOR_WIDTH;
	sY2 = firstSector.y + SECTOR_HEIGHT;

	// ????? Surface???? ???
	tilePoint.x = firstPointX + TILE_X * SECTOR_SKIP_LEFT;
	tilePoint.y = firstPointY + TILE_Y * SECTOR_SKIP_UP;

	//------------------------------------------------------
	// Zone?? ?????? ??? ??? Skip...
	//------------------------------------------------------
	if (sX1 < 0)
	{
		tilePoint.x += -sX1 * TILE_X;
		sX1 = 0;
	}

	if (sX2 >= m_pZone->GetWidth())
	{
		sX2 = m_pZone->GetWidth() - 1;
	}

	if (sY1 < 0)
	{
		tilePoint.y += -sY1 * TILE_Y;
		sY1 = 0;
	}

	if (sY2 >= m_pZone->GetHeight())
	{
		sY2 = m_pZone->GetHeight() - 1;
	}



	//------------------------------------------------------
	//
	//                  Object ???
	//
	//------------------------------------------------------

	//BYTE DarkBits;

	//---------------------------------------
	// LOCK
	//---------------------------------------
	if (!m_pSurface->Lock()) return;

	// 2004, 03, 24, sobeit start
		//------------------------------------------------------
		// Creature OutputMap?? ???????.
		//------------------------------------------------------
	__BEGIN_PROFILE("AddOutputCreature")

		AddOutputCreatureAll();

	__END_PROFILE("AddOutputCreature")

		BOOL bUnlockStatus = CDirect3D::IsHAL() && g_pUserOption->BlendingShadow;
	if (bUnlockStatus)
	{
		m_pSurface->Unlock();
	}


	CREATURE_OUTPUT_MAP::const_iterator iCreatureOutput = m_mapCreature.begin();

	//------------------------------------------------------
	// Creature - ghost draw.
	//------------------------------------------------------
	while (iCreatureOutput != m_mapCreature.end())
	{
		MCreature* const pCreature = (MCreature* const)((*iCreatureOutput).second);

		if (pCreature->GetCreatureType() == CREATURETYPE_GHOST)
		{
			// ??? ?????? sY1???? ???? ???..?? ???
			// ????? ????? ???? ????? ????? ?????								
			point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
			point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

			// ??? ???????.
			//int DarkBits = (m_pZone->GetSector(pCreature->GetX(),pCreature->GetY()).GetLight()==0)?m_DarkBits:0;

			DrawCreature(&point, pCreature);//, DarkBits);
		}

		iCreatureOutput++;
	}
	// 2004, 03, 24, sobeit end
		/*
		//------------------------------------------------------
		// Sprite ??? ????
		//------------------------------------------------------
				static int s_id = 0;
				static DWORD lastFrame = g_CurrentFrame;

				if (g_pDXInput->KeyDown(DIK_N) && g_CurrentFrame-lastFrame>2)
				{
					if (g_pDXInput->KeyDown(DIK_LSHIFT))
					{
						s_id -= 9;
					}

					if (--s_id < 0)
					{
						s_id = 0;
					}

					lastFrame = g_CurrentFrame;
				}

				if (g_pDXInput->KeyDown(DIK_M) && g_CurrentFrame-lastFrame>2)
				{
					if (g_pDXInput->KeyDown(DIK_LSHIFT))
					{
						s_id += 9;
					}

					if (++s_id >= m_CreatureSPK.GetSize())
					{
						s_id = m_CreatureSPK.GetSize()-1;
					}
					lastFrame = g_CurrentFrame;
				}

				POINT pointS = { 400, 20 };
				CIndexSprite::SetUsingColorSet( 10, 10 );
				char str[80];
				sprintf(str, "id = %d", s_id);

				m_pSurface->BltIndexSprite(&pointS, &m_CreatureSPK[s_id]);
	m_pSurface->Unlock();
				m_pSurface->GDI_Text(400, 2, str, 0xFFFFFF);
	if (!m_pSurface->Lock()) return;
		*/


		// Surface?? ?????? ????????.
		//SetSurfaceInfo(&m_SurfaceInfo, m_pSurface->GetDDSD());

		//------------------------------------------------------
		//
		//			??? ??????? ???
		//
		//------------------------------------------------------	
		//------------------------------------------------------
		// ??´???? ??? ImageObject Iterator
		//------------------------------------------------------
	IMAGEOBJECT_OUTPUT_MAP::const_iterator iImageObjectOutput0 = m_mapImageObject.begin();

	//------------------------------------------------------
	// ??? ?????? 0 ?? ??? ?????????
	// ImageObject???? ??????.
	// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (bDrawBackGround && iImageObjectOutput0 != m_mapImageObject.end())
	{
		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectOutput0).second);

		// ??? ?????? 0???? ???..?? ???
		if (pImageObject->GetViewpoint() == 0)
		{
			// ????? ????? ???? ????? ????? ?????								
			point.x = pImageObject->GetPixelX() - m_FirstZonePixel.x;
			point.y = pImageObject->GetPixelY() - m_FirstZonePixel.y;

			DrawImageObject(&point, pImageObject);
		}
		// ????.. Object ?????? ??µ?? ??????.
//		else
//		{
//			break;
//		}

		iImageObjectOutput0++;
	}


	//------------------------------------------------------
	//
	//			Ground Effect ???
	//
	//------------------------------------------------------	
	// player?? ????? ??? ??? [?????3]
	//------------------------------------------------------
	bool bPlayerInCasket = g_pPlayer->IsInCasket();

	if (bPlayerInCasket)
	{
		// [?????3]
		// ??? ??ü?? ???? ??????. Tile?? - -;;
		rect.left = 0;
		rect.right = CLIPSURFACE_WIDTH;
		rect.top = 0;
		rect.bottom = CLIPSURFACE_HEIGHT;

		DrawAlphaBox(&rect, 0, 0, 0, 20);
	}
	else
	{
		__BEGIN_PROFILE("DrawGroundEffect")

			DrawGroundEffect();

		__END_PROFILE("DrawGroundEffect")
	}


	//------------------------------------------------------
	//
	// ???õ? Sector?? ??????.
	//
	//------------------------------------------------------
	// ???õ? ????? ???.. ???õ? Object?? ???? ??...
	//if (g_pUserInformation->Invisible)
	{
	}
	//else


//	if (!g_pUserInformation->Invisible)
	{
		bool bPutSelectedSector = false;
		if (m_SelectSector.x != SECTORPOSITION_NULL
			&& m_SelectSector.y != SECTORPOSITION_NULL)
		{
			POINT selectedPoint = MapToScreen(m_SelectSector.x, m_SelectSector.y);

			// [ TEST CODE ]
			// 0,1,2,3,4,5
			// 2,2,3,3,4,4		
			const int MaxClickFrame = 12; // 6;
			const int frameID[MaxClickFrame] =
			{
				47 , 48 , 49 , 50 , 51, 52 , 53 , 54 , 55,  56 , 57 , 58
				//	4, 4, 5, 5, 6, 6
			};

			static int clickFrame = 0;

			if (g_pEventManager->GetEventByFlag(EVENTFLAG_NOT_DRAW_CREATURE) == NULL)
			{
				// ??? ??? ????
				CSprite* pSprite = &m_EtcSPK[frameID[clickFrame]];

				selectedPoint.x += TILE_X_HALF - (pSprite->GetWidth() >> 1);
				selectedPoint.y += TILE_Y_HALF - (pSprite->GetHeight() >> 1);


				m_pSurface->BltSprite(&selectedPoint, pSprite);
			}

			static DWORD lastTime = g_CurrentTime;
			if (g_CurrentTime - lastTime >= g_UpdateDelay)
			{
				if (++clickFrame == MaxClickFrame) clickFrame = 0;
				lastTime = g_CurrentTime;
			}


			// Player?? ???õ? ????? ??????..
			if (g_pPlayer->GetX() == m_SelectSector.x && g_pPlayer->GetY() == m_SelectSector.y)
			{
				m_SelectSector.x = SECTORPOSITION_NULL;
				m_SelectSector.y = SECTORPOSITION_NULL;
			}

			bPutSelectedSector = true;
		}
	}


	//------------------------------------------------------
	// 
	//       ImageObject?? ????? ???
	//
	//------------------------------------------------------
	//------------------------------------------------------
	// ??´???? ??? ImageObject Iterator
	//------------------------------------------------------
	__BEGIN_PROFILE("DrawImageObjectShadow")

		IMAGEOBJECT_OUTPUT_MAP::const_iterator iImageObjectShadowOutput = m_mapImageObject.begin();

	TYPE_SPRITEID	sprite;
	while (bDrawBackGround && iImageObjectShadowOutput != m_mapImageObject.end())
	{
		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectShadowOutput).second);

		// ????? ????? ???? ????? ????? ?????								
		point.x = pImageObject->GetPixelX() - m_FirstZonePixel.x;
		point.y = pImageObject->GetPixelY() - m_FirstZonePixel.y;

		if (pImageObject->GetSpriteID() == SPRITEID_NULL)
		{
			/*
			if (pImageObject->IsBltTypeNormal())
			{
				sprite = m_ImageObjectFPK[ pImageObject->GetFrameID() ][ pImageObject->GetFrame() ].GetSpriteID();
			}
			else
			{
				sprite = m_EffectAlphaFPK[ pImageObject->GetFrameID() ][ 0 ][ pImageObject->GetFrame() ].GetSpriteID();
			}
			*/
			iImageObjectShadowOutput++;
			continue;
		}
		else
		{
			sprite = pImageObject->GetSpriteID();
		}

		//point.y += m_ImageObjectSPK[sprite].GetHeight();
		//point.y -= 200;
		//m_pSurface->BltShadowSpriteDarkness(&point, &m_ImageObjectSSPK[ sprite ], 3-m_DarkBits);
		//m_pSurface->BltShadowSprite(&point, &m_ImageObjectSSPK[ 0 ]);

		iImageObjectShadowOutput++;
	}

	__END_PROFILE("DrawImageObjectShadow")

		//------------------------------------------------------
		//
		// ??? ???????? ????? ??? ImageObject???? ??????.
		//
		//------------------------------------------------------
		//------------------------------------------------------
		// ??´???? ??? ImageObject Iterator
		//------------------------------------------------------
		IMAGEOBJECT_OUTPUT_MAP::const_iterator iImageObjectOutput = m_mapImageObject.begin();

	//------------------------------------------------------
	// ??? ?????? sY1???? ???? ????? 
	// ImageObject???? ??????.
	// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (bDrawBackGround && iImageObjectOutput != m_mapImageObject.end())
	{
		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectOutput).second);

		// ??? ?????? sY1???? ???? ???..?? ???
		if (pImageObject->GetViewpoint() < sY1)
		{
			if (pImageObject->GetViewpoint() != 0)
			{
				// ????? ????? ???? ????? ????? ?????								
				point.x = pImageObject->GetPixelX() - m_FirstZonePixel.x;
				point.y = pImageObject->GetPixelY() - m_FirstZonePixel.y;

				DrawImageObject(&point, pImageObject);
			}
		}
		// ????.. Object ?????? ??µ?? ??????.
		else
		{
			break;
		}

		iImageObjectOutput++;
	}

	// ??????? ??????????? Loop?? ??? ???????? ????????? Frame?? ????? ?????
	if (g_bFrameChanged)
		MAnimationObject::NextLoopFrame();

	/*	- 2004, 4, 24 sobeit block - ghost ?????? ?????? ó?? ??
		//------------------------------------------------------
		// player?? ?? ??? ??? ??? ????.. [?????3]
		//------------------------------------------------------
		//------------------------------------------------------
		// Creature OutputMap?? ???????.
		//------------------------------------------------------
		__BEGIN_PROFILE("AddOutputCreature")

		AddOutputCreatureAll();

		__END_PROFILE("AddOutputCreature")

		//------------------------------------------------------
		//
		//			??? creature?? ?????? ??????.
		//
		//------------------------------------------------------
		// ??? creature?? shadow?? ??????.
		// (!) AddOutputCreatureAll()?? ?? ?????? ??????? ???.
		//
		// Lock?? ???¿??? ???????? ???????.
		//----------------------------------------------------
		// 3D????????? Unlock???? ?????.
		//----------------------------------------------------
		BOOL bUnlockStatus = CDirect3D::IsHAL() && g_pUserOption->BlendingShadow;
		if (bUnlockStatus)
		{
			m_pSurface->Unlock();
		}
	*/
	//------------------------------------------------------
	// ??´???? ??? Creature Iterator
	//------------------------------------------------------
	__BEGIN_PROFILE("DrawCreatureShadowAll")

		//CREATURE_OUTPUT_MAP::const_iterator iCreatureOutput = m_mapCreature.begin();
		iCreatureOutput = m_mapCreature.begin();

	//------------------------------------------------------
	// ????? ?????? ??? ??????.
	//------------------------------------------------------
	// Ÿ???? ü???? ?????? ???
	//------------------------------------------------------
	if (m_bTileSearchForCreature)
	{
		for (y = sY1; y <= sY2; y++)
		{
			for (x = sX1; x <= sX2; x++)
			{
				int darknessCount = g_pPlayer->GetDarknessCount();

				// darkness?? ??????? ????? ???. effect?? +2
				if (darknessCount < 0
					|| max(abs(g_pPlayer->GetX() - x), abs(g_pPlayer->GetY() - y)) <= darknessCount + 2)
					//g_pPlayer->ShowInDarkness(x, y))
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					if (sector.IsExistObject())
					{
						//------------------------------------------------------
						// Underground Creature
						//------------------------------------------------------
						MCreature* pCreature;

						/*
						pCreature = sector.GetUndergroundCreature();

						if (pCreature!=NULL)
						{
							// player???? darkness?? ??????? ????? ???
							if (pCreature==g_pPlayer
								|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY()))
							{
								// ????? ????? ???? ????? ????? ?????
								point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
								point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

								DrawCreatureShadow(&point, pCreature);//, DarkBits);
							}
						}
						*/

						//------------------------------------------------------
						// Ground Creature
						// Flying Creature
						//------------------------------------------------------
						int numCreature = sector.GetCreatureSize();

						if (numCreature > 0)
						{
							OBJECT_MAP::const_iterator iCreature = sector.GetCreatureIterator();

							for (int c = 0; c < numCreature; c++, iCreature++)
							{
								if (iCreature == sector.GetObjectEnd()
									|| iCreature->first > MSector::POSITION_FLYINGCREATURE_MAX)
									break;

								// underground?? ?????? ????.
								if (iCreature->first >= MSector::POSITION_GROUNDCREATURE)
								{
									pCreature = (MCreature*)iCreature->second;

									// player???? darkness?? ??????? ????? ???
									if (
										(pCreature == g_pPlayer
											|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY())) &&
										!pCreature->IsFakeCreature()
										)
									{
										// ????? ????? ???? ????? ????? ?????								
										point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
										point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

										DrawCreatureShadow(&point, pCreature);//, DarkBits);
									}
								}
							}
						}
					}
				}
			}
		}
		if (m_pZone->GetFakeCreatureNumber() > 0)
		{
			int numFakeCreature = m_pZone->GetFakeCreatureNumber();

			MZone::CREATURE_MAP::const_iterator itr = m_pZone->GetFakeCreatureBegin();

			for (int i = 0; i < numFakeCreature; i++, itr++)
			{
				if (itr->first < MSector::POSITION_GROUNDCREATURE ||
					itr->first > MSector::POSITION_FLYINGCREATURE_MAX)
					continue;

				MCreature* pCreature = itr->second;

				point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
				point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

				DrawCreatureShadow(&point, pCreature);
			}
		}

		// ????? ????? ???? ????? ????? ?????								
		point.x = g_pPlayer->GetPixelX() - m_FirstZonePixel.x;
		point.y = g_pPlayer->GetPixelY() - m_FirstZonePixel.y;

		DrawCreatureShadow(&point, g_pPlayer);//, DarkBits);		
	}
	//------------------------------------------------------
	// ????? OutputMap?? ?????? ???
	//------------------------------------------------------
	else
	{
		while (iCreatureOutput != m_mapCreature.end())
		{
			MCreature* const pCreature = (MCreature* const)((*iCreatureOutput).second);

			if (pCreature == g_pPlayer
				|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY()))
			{
				// ??? ?????? sY1???? ???? ???..?? ???
				// ????? ????? ???? ????? ????? ?????								
				point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
				point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

				// ??? ???????.
				//int DarkBits = (m_pZone->GetSector(pCreature->GetX(),pCreature->GetY()).GetLight()==0)?m_DarkBits:0;

				DrawCreatureShadow(&point, pCreature);//, DarkBits);
			}

			iCreatureOutput++;
		}
	}

	__END_PROFILE("DrawCreatureShadowAll")

		//----------------------------------------------------
		// ?????? Lock?? ???¸? ???????? ???.
		//----------------------------------------------------
		if (bUnlockStatus)
		{
			m_pSurface->Lock();
		}

	// Item ????? ??¿?
	//------------------------------------------------------
	// ????? Sector???? ??? ??????
	// Sector?? ??????? Object???? ??????.
	//------------------------------------------------------
	// ù??° ??	
	tilePointTemp.y = tilePoint.y;

	//----------------------------------------------------------------
	//
	// tile?? ?????? ?????? ?????? ???
	//
	//----------------------------------------------------------------
	if (m_bTileSearchForCreature)
	{
		for (y = sY1; y <= sY2; y++)
		{
			// ?? ???? ù??° Sector					
			tilePointTemp.x = tilePoint.x;

			for (x = sX1; x <= sX2; x++)
			{
				const MSector& sector = m_pZone->GetSector(x, y);

				//------------------------------------------------
				//
				//              Object ???
				//
				//------------------------------------------------

				if (sector.IsExistObject())
				{

					//----------------------------------------
					// Item?? ???
					//----------------------------------------					
					pItem = sector.GetItem();
					if (pItem != NULL
						&& g_pPlayer->ShowInDarkness(x, y))
					{
						//----------------------------------------
						// ??ü?? ???
						// Effect?? ?????? ???? ??????.
						//
						// 2002.1.23
						// ???? Effect?????? ????????..
						// ??? ??ü ???? ???? ????. 
						//----------------------------------------
						//if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)
						{
							//	pCorpseItem = pItem;
						}
						//else
						{
							point.x = tilePointTemp.x;
							point.y = tilePointTemp.y;


							// ??? ???????.
							//DarkBits = (sector.GetLight()==0)?m_DarkBits:0;

							// ????? Frame???????? cx,cy?? ??????? ???.									
							DrawItemShadow(&point, pItem);//, DarkBits);
						}
					}
				}


				//------------------------------------			
				// ???????? ??? ???
				//------------------------------------
				tilePointTemp.x += TILE_X;
			}


			// ???? ??
			tilePointTemp.y += TILE_Y;
		}
	}
	//----------------------------------------------------------------
	//
	// tile?? ??????? ??? OutputCreatureMap?? ?????? ?????? ?????? ???
	//
	//----------------------------------------------------------------
	else
	{
		for (y = sY1; y <= sY2; y++)
		{
			// ?? ??? ???? ???? ??? ?????? ?????? object?? ????.
			// AddOutputCreatureAll()???? m_bTileSearchForCreature?? false??
			// ????????... OutputCreatureMap?? ?????? ???.
			if (!bPlayerInCasket)	// [?????3]
			{
				// ?? ???? ù??° Sector					
				tilePointTemp.x = tilePoint.x;

				for (x = sX1; x <= sX2; x++)
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					//------------------------------------------------
					//
					//              Object ???
					//
					//------------------------------------------------
					//MItem* pCorpseItem = NULL;

					if (sector.IsExistObject())
					{
						//----------------------------------------
						// Item?? ???
						//----------------------------------------					
						pItem = sector.GetItem();
						if (pItem != NULL
							&& g_pPlayer->ShowInDarkness(x, y))
						{
							//----------------------------------------
							// ??ü?? ???
							// Effect?? ?????? ???? ??????.
							//
							// 2002.1.23
							// ???? Effect?????? ????????..
							// ??? ??ü ???? ???? ????. 
							//----------------------------------------
							//if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)
							{
								//	pCorpseItem = pItem;
							}
							//else
							{
								point.x = tilePointTemp.x;
								point.y = tilePointTemp.y;

								// ????? Frame???????? cx,cy?? ??????? ???.									
								DrawItemShadow(&point, pItem);//, DarkBits);
							}
						}
					}


					//------------------------------------			
					// ???????? ??? ???
					//------------------------------------
					tilePointTemp.x += TILE_X;
				}
			}	// bPlayerInCasket

			// ???? ??
			tilePointTemp.y += TILE_Y;
		}
	}


	//------------------------------------------------------
	//
	//        Settled corpses (ground layer)
	//
	//------------------------------------------------------
	// A corpse that has finished its death animation is a floor decal -
	// the blood pool is part of the sprite. Draw them all here, ahead of
	// every actor, so a character standing behind one is never painted
	// over by its blood. Corpses still mid-animation stay in the
	// row-sorted pass below.
	//------------------------------------------------------
	if (m_bTileSearchForCreature || !bPlayerInCasket)
	{
		tilePointTemp.y = tilePoint.y;

		for (y = sY1; y <= sY2; y++)
		{
			tilePointTemp.x = tilePoint.x;

			for (x = sX1; x <= sX2; x++)
			{
				const MSector& sector = m_pZone->GetSector(x, y);

				if (sector.IsExistObject())
				{
					pItem = sector.GetItem();

					if (IsSettledCorpse(pItem)
						&& g_pPlayer->ShowInDarkness(x, y))
					{
						point.x = tilePointTemp.x;
						point.y = tilePointTemp.y;

						DrawItem(&point, pItem);
					}
				}

				tilePointTemp.x += TILE_X;
			}

			tilePointTemp.y += TILE_Y;
		}
	}


	//------------------------------------------------------
	//
	//				???? sprite ???
	//
	//------------------------------------------------------
	//------------------------------------------------------
	// ??´???? ??? Creature Iterator
	//------------------------------------------------------
	iCreatureOutput = m_mapCreature.begin();

	//------------------------------------------------------
	// ??? ?????? sY1???? ???? ????? 
	// Creature???? ??????.
	// m_mapCreature???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (iCreatureOutput != m_mapCreature.end())
	{
		MCreature* const pCreature = (MCreature* const)((*iCreatureOutput).second);

		// 2004, 04, 24 sobeit add start -ghost ??? 
		if (pCreature->GetCreatureType() == CREATURETYPE_GHOST)
		{
			iCreatureOutput++;
			continue;
		}
		// 2004, 04, 24 sobeit add end -ghost ??? 

		// ??? ?????? sY1???? ???? ???..?? ???
		if (pCreature->GetY() < sY1)
		{
			// player???? darkness?? ??????? ????? ???
			if (pCreature == g_pPlayer
				|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY()))
			{
				// ????? ????? ???? ????? ????? ?????								
				point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
				point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

				// ??? ???????.
				//int DarkBits = (m_pZone->GetSector(pCreature->GetX(),pCreature->GetY()).GetLight()==0)?m_DarkBits:0;

				DrawCreature(&point, pCreature);//, DarkBits);
			}
		}
		// ????.. ?? ?? ????? ?????? ?????? ??µ?? ??????.
		else
		{
			break;
		}

		iCreatureOutput++;
	}

	//------------------------------------------------------
	// ????? Sector???? ??? ??????
	// Sector?? ??????? Object???? ??????.
	//------------------------------------------------------
	// ù??° ??	
	tilePointTemp.y = tilePoint.y;

	//----------------------------------------------------------------
	//
	// tile?? ?????? ?????? ?????? ???
	//
	//----------------------------------------------------------------
	if (m_bTileSearchForCreature)
	{
		for (y = sY1; y <= sY2; y++)
		{
			// ?? ???? ù??° Sector					
			tilePointTemp.x = tilePoint.x;

			for (x = sX1; x <= sX2; x++)
			{
				const MSector& sector = m_pZone->GetSector(x, y);

				//------------------------------------------------------
				// ??? ??? ???? ???
				//------------------------------------------------------
				if (g_pOperatorOption && g_pOperatorOption->bShowSkillRange)
				{
					if (g_pPlayer)
					{
						TYPE_ACTIONINFO specialActionInfo = g_pPlayer->GetSpecialActionInfo();

						if (specialActionInfo != ACTIONINFO_NULL)
						{
							int actionRange = 0;

							if ((*g_pActionInfoTable)[specialActionInfo].GetTarget() != FLAG_ACTIONINFO_TARGET_SELF)
							{
								actionRange = g_pPlayer->GetActionInfoRange(specialActionInfo);
							}

							TYPE_SECTORPOSITION playerX = g_pPlayer->GetX();
							TYPE_SECTORPOSITION playerY = g_pPlayer->GetY();

							int range = max(abs(playerX - x), abs(playerY - y));

							if (range <= actionRange)
							{
								int colorValue = max(255 - range * 20, 100);
								int colorValue16 = colorValue / 8;

								m_pSurface->Unlock();
								RECT rect2 = { tilePointTemp.x, tilePointTemp.y,
									tilePointTemp.x + TILE_X,
									tilePointTemp.y + TILE_Y };
								m_pSurface->DrawRect(&rect2,
									CDirectDraw::Color(0, colorValue16, 0));

								char szBuf[8];
								_itoa(range, szBuf, 10);

								m_pSurface->GDI_Text(
									tilePointTemp.x + 2,
									tilePointTemp.y + 2, szBuf,
									RGB(0, colorValue, 0));

								m_pSurface->Lock();
							}
						}
					}
				}

				//------------------------------------------------------
				// ???? block????? ????
				//------------------------------------------------------
#ifdef OUTPUT_DEBUG
				if (g_pDXInput->KeyDown(DIK_LCONTROL) && g_pDXInput->KeyDown(DIK_V))
				{
					if (sector.IsBlockServerGround())
					{
						m_pSurface->Unlock();
						RECT rect2 = { tilePointTemp.x, tilePointTemp.y,
							tilePointTemp.x + TILE_X,
							tilePointTemp.y + TILE_Y };
						m_pSurface->DrawRect(&rect2, CDirectDraw::Color(31, 20, 20));
						m_pSurface->Lock();
					}

					if (sector.IsExistAnyCreature())
					{
						m_pSurface->Unlock();
						RECT rect2 = { tilePointTemp.x + 2, tilePointTemp.y + 2,
							tilePointTemp.x + TILE_X - 2,
							tilePointTemp.y + TILE_Y - 2 };
						m_pSurface->DrawRect(&rect2, CDirectDraw::Color(20, 20, 31));
						m_pSurface->Lock();
					}
				}
#endif

				//------------------------------------------------------
				// ????? ImageObject?? ViewSector??? "X"????
				//------------------------------------------------------
#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
				if (g_pDXInput->KeyDown(DIK_A) &&
					(g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
				{
					if (sector.IsExistImageObject())
					{
						OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

						// Sector?? ??? ??? ImageObject???? ??????.
						for (i = 0; i < sector.GetImageObjectSize(); i++)
						{
							MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

							if (pImageObject->GetObjectType() == MObject::TYPE_IMAGEOBJECT
								&& pImageObject->GetImageObjectID() == g_ShowImageObjectID)
							{
								m_pSurface->Unlock();
								m_pSurface->GDI_Text(tilePointTemp.x,
									tilePointTemp.y,
									"X",
									0xFFFFFF);
								m_pSurface->Lock();
							}

							iImageObject++;
						}
					}
				}
#endif				

				//------------------------------------------------
				// ???? ?????? ??????.
				//------------------------------------------------
				//if (x==g_SelectSector.x && y==g_SelectSector.y)
				//{
				//	char str[80];
				//	sprintf(str, "%d", sector.GetLight());
				//	gC_font.PrintStringNoConvert(&m_SurfaceInfo, str, tilePointTemp.x+25,tilePointTemp.y+12, CDirectDraw::Color(28,28,28));				
				//}

				//------------------------------------------------
				//
				//              Object ???
				//
				//------------------------------------------------
				//MItem* pCorpseItem = NULL;

				if (sector.IsExistObject())
				{
					//----------------------------------------
					// test code : Object ??? ???
					//----------------------------------------
					//point = tilePointTemp;
					//m_pSurface->Copy(&point, m_pm_pSurface[0], 
					//				&m_SpriteSurfacePack[0][105]);
					//
					//----------------------------------------

					//----------------------------------------
					// Item?? ???
					//----------------------------------------					
					pItem = sector.GetItem();
					if (pItem != NULL
						&& g_pPlayer->ShowInDarkness(x, y))
					{
						//----------------------------------------
						// ??ü?? ???
						// Effect?? ?????? ???? ??????.
						//
						// 2002.1.23
						// ???? Effect?????? ????????..
						// ??? ??ü ???? ???? ????. 
						//----------------------------------------
						//if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)
						{
							//	pCorpseItem = pItem;
						}
						//else
						{
							point.x = tilePointTemp.x;
							point.y = tilePointTemp.y;

							/*
							// ????~~
							int size = g_CurrentFrame & 0x00000007;

							int x0 = point.x + TILE_X_HALF;
							int y0 = point.y + TILE_Y_HALF;
							int size4 = size << 4;
							int size2 = size << 3;
							RECT rect =
							{
								x0 - size4,
								y0 - size2,
								x0 + size4,
								y0 + size2
							};
							m_pSurface->ChangeBrightnessBit( &rect, g_CurrentFrame );
							*/

							// ??? ???????.
							//DarkBits = (sector.GetLight()==0)?m_DarkBits:0;

							// ????? Frame???????? cx,cy?? ??????? ???.									
							// settled corpses were already drawn with the ground layer
							if (!IsSettledCorpse(pItem))
							{
								DrawItem(&point, pItem);//, DarkBits);
							}
						}
					}
				}

				//------------------------------------------------
				//
				//              Effect ???
				//
				//------------------------------------------------
				//if (sector.IsExistEffect())
				//{				
				//	point = tilePointTemp;				
	//
	//				DrawEffect(&point, sector.GetEffectIterator(), sector.GetEffectSize());
	//			}

				//------------------------------------------------
				// ??ü
				//------------------------------------------------
				// ??!! ?????? ????? ??????.. ????.. ???? ????
				//------------------------------------------------
				/*
				if (pCorpseItem)
				{
					point.x = tilePointTemp.x;
					point.y = tilePointTemp.y;

					// ??? ???????.
					//DarkBits = (sector.GetLight()==0)?m_DarkBits:0;

					// ????? Frame???????? cx,cy?? ??????? ???.
					DrawItem(&point, pItem);//, DarkBits);
				}
				*/

				//------------------------------------			
				// ???????? ??? ???
				//------------------------------------
				tilePointTemp.x += TILE_X;
			}

			//-----------------------------------------------------
			// ?? ???? ?????????? 
			// ???????? ?? ???? ImageObject???? ??????.
			//-----------------------------------------------------	
			while (bDrawBackGround && iImageObjectOutput != m_mapImageObject.end())
			{
				MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectOutput).second);

				// ??? ?????? y?? ???? ??? ???
				if (pImageObject->GetViewpoint() <= y)
				{
					if (pImageObject->GetViewpoint() != 0)
					{
						// ????? ????? ???? ????? ????? ?????								
						point.x = pImageObject->GetPixelX() - m_FirstZonePixel.x;
						point.y = pImageObject->GetPixelY() - m_FirstZonePixel.y;

						DrawImageObject(&point, pImageObject);

						//char str[80];
						//sprintf(str, "%d", pImageObject->GetViewpoint());
						//m_pSurface->GDI_Text(point.x, point.y, str, 0xFFFFFF);
					}
				}
				// ????.. ??? ?????.
				else
				{
					break;
				}

				iImageObjectOutput++;
			}

			//------------------------------------------------------
			// ?? ???? ?????????? 
			// ???????? ?? ???? Creature???? ??????.
			//------------------------------------------------------
			while (iCreatureOutput != m_mapCreature.end())
			{
				MCreature* const pCreature = (MCreature* const)((*iCreatureOutput).second);

				// ??? ?????? sY1???? ???? ???..?? ???
				if (pCreature->GetY() <= y)
				{
					// 2004, 04, 24 sobeit add start -ghost ??? 
					if (pCreature->GetCreatureType() == CREATURETYPE_GHOST)
					{
						iCreatureOutput++;
						continue;
					}
					// 2004, 04, 24 sobeit add end -ghost ??? 

					// player???? darkness?? ??????? ????? ???
					if (pCreature == g_pPlayer
						|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY()))
					{
						// ????? ????? ???? ????? ????? ?????								
						point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
						point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

						// ??? ???????.
						//int DarkBits = (m_pZone->GetSector(pCreature->GetX(),pCreature->GetY()).GetLight()==0)?m_DarkBits:0;

						DrawCreature(&point, pCreature);//, DarkBits);
					}
				}
				// ????.. ?? ?? ????? ?????? ?????? ??µ?? ??????.
				else
				{
					break;
				}

				iCreatureOutput++;
			}

			//------------------------------------------------------
			// ??¼?????????.. ???? ?????? ü????.
			// Sector?? Effect ???
			//------------------------------------------------------
			// ???????? ??? ??µ??? ????...
			//------------------------------------------------------
			//------------------------------------------------------
			// Player?? Tile?? ?????? ??? ?????? ???? ü????.
			//------------------------------------------------------
			if (g_pPlayer->GetY() == y)
			{
				// ????? ????? ???? ????? ????? ?????								
				point.x = g_pPlayer->GetPixelX() - m_FirstZonePixel.x;
				point.y = g_pPlayer->GetPixelY() - m_FirstZonePixel.y;

				// ??? ???????.
				//int DarkBits = (m_pZone->GetSector(pCreature->GetX(),pCreature->GetY()).GetLight()==0)?m_DarkBits:0;

				DrawCreature(&point, g_pPlayer);//, DarkBits);
			}

			tilePointTemp.x = tilePoint.x;
			for (x = sX1; x <= sX2; x++)
			{
				int darknessCount = g_pPlayer->GetDarknessCount();

				// darkness?? ??????? ????? ???. effect?? +2
				if (darknessCount < 0
					|| max(abs(g_pPlayer->GetX() - x), abs(g_pPlayer->GetY() - y)) <= darknessCount + 2)
					//g_pPlayer->ShowInDarkness(x, y))
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					if (sector.IsExistObject())
					{
						//------------------------------------------------------
						// Underground Creature
						// Ground Creature
						// Flying Creature
						//------------------------------------------------------
						int numCreature = sector.GetCreatureSize();

						if (numCreature > 0)
						{
							OBJECT_MAP::const_iterator iCreature = sector.GetCreatureIterator();

							for (int c = 0; c < numCreature; c++, iCreature++)
							{
								if (iCreature == sector.GetObjectEnd()
									|| iCreature->first > MSector::POSITION_FLYINGCREATURE_MAX)
									break;

								MCreature* pCreature = (MCreature*)iCreature->second;

								// 2004, 04, 24 sobeit add start -ghost ??? 
								if (pCreature->GetCreatureType() == CREATURETYPE_GHOST)
									continue;
								// 2004, 04, 24 sobeit add end -ghost ??? 

								// player???? darkness?? ??????? ????? ???
								if (pCreature == g_pPlayer
									|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY()))
								{
									// ????? ????? ???? ????? ????? ?????								
									point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
									point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

									DrawCreature(&point, pCreature);//, DarkBits);
								}
							}
						}
					}

					//------------------------------------------------
					//
					//              Effect ???
					//
					//------------------------------------------------
					if (sector.IsExistEffect())
					{
						point = tilePointTemp;

						DrawEffect(&point, sector.GetEffectIterator(), sector.GetEffectSize());
					}
				}

				//------------------------------------			
				// ???????? ??? ???
				//------------------------------------
				tilePointTemp.x += TILE_X;
			}

			// ???? ??
			tilePointTemp.y += TILE_Y;
		}
	}
	//----------------------------------------------------------------
	//
	// tile?? ??????? ??? OutputCreatureMap?? ?????? ?????? ?????? ???
	//
	//----------------------------------------------------------------
	else
	{
		for (y = sY1; y <= sY2; y++)
		{
			// ?? ??? ???? ???? ??? ?????? ?????? object?? ????.
			// AddOutputCreatureAll()???? m_bTileSearchForCreature?? false??
			// ????????... OutputCreatureMap?? ?????? ???.
			if (!bPlayerInCasket)	// [?????3]
			{
				// ?? ???? ù??° Sector					
				tilePointTemp.x = tilePoint.x;

				for (x = sX1; x <= sX2; x++)
				{
					const MSector& sector = m_pZone->GetSector(x, y);

					//------------------------------------------------------
					// ??? ??? ???? ???
					//------------------------------------------------------
					if (g_pOperatorOption && g_pOperatorOption->bShowSkillRange)
					{
						if (g_pPlayer)
						{
							TYPE_ACTIONINFO specialActionInfo = g_pPlayer->GetSpecialActionInfo();

							if (specialActionInfo != ACTIONINFO_NULL)
							{
								int actionRange = 0;

								if ((*g_pActionInfoTable)[specialActionInfo].GetTarget() != FLAG_ACTIONINFO_TARGET_SELF)
								{
									actionRange = g_pPlayer->GetActionInfoRange(specialActionInfo);
								}

								TYPE_SECTORPOSITION playerX = g_pPlayer->GetX();
								TYPE_SECTORPOSITION playerY = g_pPlayer->GetY();

								int range = max(abs(playerX - x), abs(playerY - y));

								if (range <= actionRange)
								{
									int colorValue = max(255 - range * 20, 100);
									int colorValue16 = colorValue / 8;

									m_pSurface->Unlock();
									RECT rect2 = { tilePointTemp.x, tilePointTemp.y,
										tilePointTemp.x + TILE_X,
										tilePointTemp.y + TILE_Y };
									m_pSurface->DrawRect(&rect2,
										CDirectDraw::Color(0, colorValue16, 0));

									char szBuf[8];
									_itoa(range, szBuf, 10);

									m_pSurface->GDI_Text(
										tilePointTemp.x + 2,
										tilePointTemp.y + 2, szBuf,
										RGB(0, colorValue, 0));

									m_pSurface->Lock();
								}
							}
						}
					}

					//------------------------------------------------------
					// ???? block????? ????
					//------------------------------------------------------
#ifdef OUTPUT_DEBUG
					if (g_pDXInput->KeyDown(DIK_LCONTROL) && g_pDXInput->KeyDown(DIK_V))
					{
						if (sector.IsBlockServerGround())
						{
							m_pSurface->Unlock();
							RECT rect2 = { tilePointTemp.x, tilePointTemp.y,
								tilePointTemp.x + TILE_X,
								tilePointTemp.y + TILE_Y };
							m_pSurface->DrawRect(&rect2, CDirectDraw::Color(31, 20, 20));
							m_pSurface->Lock();
						}
					}
#endif

					//------------------------------------------------------
					// ????? ImageObject?? ViewSector??? "X"????
					//------------------------------------------------------
#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
					if (g_pDXInput->KeyDown(DIK_A) &&
						(g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
					{
						if (sector.IsExistImageObject())
						{
							OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

							// Sector?? ??? ??? ImageObject???? ??????.
							for (i = 0; i < sector.GetImageObjectSize(); i++)
							{
								MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);

								if (pImageObject->GetObjectType() == MObject::TYPE_IMAGEOBJECT
									&& pImageObject->GetImageObjectID() == g_ShowImageObjectID)
								{
									m_pSurface->Unlock();
									m_pSurface->GDI_Text(tilePointTemp.x,
										tilePointTemp.y,
										"X",
										0xFFFFFF);
									m_pSurface->Lock();
								}

								iImageObject++;
							}
						}
					}
#endif				

					//------------------------------------------------
					// ???? ?????? ??????.
					//------------------------------------------------
					//if (x==g_SelectSector.x && y==g_SelectSector.y)
					//{
					//	char str[80];
					//	sprintf(str, "%d", sector.GetLight());
					//	gC_font.PrintStringNoConvert(&m_SurfaceInfo, str, tilePointTemp.x+25,tilePointTemp.y+12, CDirectDraw::Color(28,28,28));				
					//}

					//------------------------------------------------
					//
					//              Object ???
					//
					//------------------------------------------------
					//MItem* pCorpseItem = NULL;

					if (sector.IsExistObject())
					{
						//----------------------------------------
						// test code : Object ??? ???
						//----------------------------------------
						//point = tilePointTemp;
						//m_pSurface->Copy(&point, m_pm_pSurface[0], 
						//				&m_SpriteSurfacePack[0][105]);
						//
						//----------------------------------------

						//----------------------------------------
						// Item?? ???
						//----------------------------------------					
						pItem = sector.GetItem();
						if (pItem != NULL
							&& g_pPlayer->ShowInDarkness(x, y))
						{
							//----------------------------------------
							// ??ü?? ???
							// Effect?? ?????? ???? ??????.
							//
							// 2002.1.23
							// ???? Effect?????? ????????..
							// ??? ??ü ???? ???? ????. 
							//----------------------------------------
							//if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)
							{
								//	pCorpseItem = pItem;
							}
							//else
							{
								point.x = tilePointTemp.x;
								point.y = tilePointTemp.y;

								/*
								// ????~~
								int size = g_CurrentFrame & 0x00000007;

								int x0 = point.x + TILE_X_HALF;
								int y0 = point.y + TILE_Y_HALF;
								int size4 = size << 4;
								int size2 = size << 3;
								RECT rect =
								{
									x0 - size4,
									y0 - size2,
									x0 + size4,
									y0 + size2
								};
								m_pSurface->ChangeBrightnessBit( &rect, g_CurrentFrame );
								*/

								// ??? ???????.
								//DarkBits = (sector.GetLight()==0)?m_DarkBits:0;

								// ????? Frame???????? cx,cy?? ??????? ???.									
								// settled corpses were already drawn with the ground layer
								if (!IsSettledCorpse(pItem))
								{
									DrawItem(&point, pItem);//, DarkBits);
								}
							}
						}
					}

					//------------------------------------------------
					//
					//              Effect ???
					//
					//------------------------------------------------
					//if (sector.IsExistEffect())
					//{				
					//	point = tilePointTemp;				
		//
		//				DrawEffect(&point, sector.GetEffectIterator(), sector.GetEffectSize());
		//			}

					//------------------------------------------------
					// ??ü
					//------------------------------------------------
					// ??!! ?????? ????? ??????.. ????.. ???? ????
					//------------------------------------------------
					/*
					if (pCorpseItem)
					{
						point.x = tilePointTemp.x;
						point.y = tilePointTemp.y;

						// ??? ???????.
						//DarkBits = (sector.GetLight()==0)?m_DarkBits:0;

						// ????? Frame???????? cx,cy?? ??????? ???.
						DrawItem(&point, pItem);//, DarkBits);
					}
					*/

					//------------------------------------			
					// ???????? ??? ???
					//------------------------------------
					tilePointTemp.x += TILE_X;
				}
			}	// bPlayerInCasket

			//-----------------------------------------------------
			// ?? ???? ?????????? 
			// ???????? ?? ???? ImageObject???? ??????.
			//-----------------------------------------------------	
			while (bDrawBackGround && iImageObjectOutput != m_mapImageObject.end())
			{
				MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectOutput).second);

				// ??? ?????? y?? ???? ??? ???
				if (pImageObject->GetViewpoint() <= y)
				{
					if (pImageObject->GetViewpoint() != 0)
					{
						// ????? ????? ???? ????? ????? ?????								
						point.x = pImageObject->GetPixelX() - m_FirstZonePixel.x;
						point.y = pImageObject->GetPixelY() - m_FirstZonePixel.y;

						DrawImageObject(&point, pImageObject);

						//char str[80];
						//sprintf(str, "%d", pImageObject->GetViewpoint());
						//m_pSurface->GDI_Text(point.x, point.y, str, 0xFFFFFF);
					}
				}
				// ????.. ??? ?????.
				else
				{
					break;
				}

				iImageObjectOutput++;
			}

			//------------------------------------------------------
			// ?? ???? ?????????? 
			// ???????? ?? ???? Creature???? ??????.
			//------------------------------------------------------
			while (iCreatureOutput != m_mapCreature.end())
			{
				MCreature* const pCreature = (MCreature* const)((*iCreatureOutput).second);

				// 2004, 04, 24 sobeit add start -ghost ??? 
				if (pCreature->GetCreatureType() == CREATURETYPE_GHOST)
				{
					iCreatureOutput++;
					continue;
				}
				// 2004, 04, 24 sobeit end start -ghost ??? 

				// ??? ?????? sY1???? ???? ???..?? ???
				if (pCreature->GetY() <= y)
				{
					// player???? darkness?? ??????? ????? ???
					if (pCreature == g_pPlayer
						|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY()))
					{
						// ????? ????? ???? ????? ????? ?????								
						point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
						point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

						// ??? ???????.
						//int DarkBits = (m_pZone->GetSector(pCreature->GetX(),pCreature->GetY()).GetLight()==0)?m_DarkBits:0;


						DrawCreature(&point, pCreature);//, DarkBits);

						// NPC?? ????? ?????????.
//						if(pCreature->IsNPC())
//							DrawNameCreatureVec.push_back(std::make_pair(pCreature, 0));
					}
				}
				// ????.. ?? ?? ????? ?????? ?????? ??µ?? ??????.
				else
				{
					break;
				}

				iCreatureOutput++;
			}

			//------------------------------------------------------
			// Sector?? Effect ???
			//------------------------------------------------------
			// ???????? ??? ??µ??? ????...
			//------------------------------------------------------
			if (!bPlayerInCasket)	// [?????3] ?? ??? ?????? Effect?? ?? ??´?.
			{
				tilePointTemp.x = tilePoint.x;
				for (x = sX1; x <= sX2; x++)
				{
					int darknessCount = g_pPlayer->GetDarknessCount();

					// darkness?? ??????? ????? ???. effect?? +2
					if (darknessCount < 0
						|| max(abs(g_pPlayer->GetX() - x), abs(g_pPlayer->GetY() - y)) <= darknessCount + 2)
						//g_pPlayer->ShowInDarkness(x, y))
					{
						const MSector& sector = m_pZone->GetSector(x, y);

						//------------------------------------------------
						//
						//              Effect ???
						//
						//------------------------------------------------
						if (sector.IsExistEffect())
						{
							point = tilePointTemp;

							DrawEffect(&point, sector.GetEffectIterator(), sector.GetEffectSize());
						}
					}

					//------------------------------------			
					// ???????? ??? ???
					//------------------------------------
					tilePointTemp.x += TILE_X;
				}
			}

			// ???? ??
			tilePointTemp.y += TILE_Y;
		}
	}


	//------------------------------------------------------
	// ??? ?????? sY2???? ? ????? 
	// Creature???? ??????.
	// m_mapCreature???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (iCreatureOutput != m_mapCreature.end())
	{
		MCreature* const pCreature = (MCreature* const)((*iCreatureOutput).second);

		// 2004, 04, 24 sobeit add start -ghost ??? 
		if (pCreature->GetCreatureType() == CREATURETYPE_GHOST)
		{
			iCreatureOutput++;
			continue;
		}
		// 2004, 04, 24 sobeit add end -ghost ??? 

		// player???? darkness?? ??????? ????? ???
		if (pCreature == g_pPlayer
			|| g_pPlayer->ShowInDarkness(pCreature->GetX(), pCreature->GetY()))
		{
			// ????? ????? ???? ????? ????? ?????								
			point.x = pCreature->GetPixelX() - m_FirstZonePixel.x;
			point.y = pCreature->GetPixelY() - m_FirstZonePixel.y;

			// ??? ???????.
			//int DarkBits = (m_pZone->GetSector(pCreature->GetX(),pCreature->GetY()).GetLight()==0)?m_DarkBits:0;

			DrawCreature(&point, pCreature);//, DarkBits);		
		}

		iCreatureOutput++;
	}


	//------------------------------------------------------
	// ??? ?????? sY2???? ? ????? 
	// ImageObject???? ??????.
	// m_mapImageObject???? Viewpoint?? key???? ??? Sort??? ???.
	//------------------------------------------------------
	while (bDrawBackGround && iImageObjectOutput != m_mapImageObject.end())
	{
		MImageObject* const pImageObject = (MImageObject* const)((*iImageObjectOutput).second);

		// ????? ????? ???? ????? ????? ?????								
		point.x = pImageObject->GetPixelX() - m_FirstZonePixel.x;
		point.y = pImageObject->GetPixelY() - m_FirstZonePixel.y;

		DrawImageObject(&point, pImageObject);

		iImageObjectOutput++;
	}

	//------------------------------------------------
	// mouse?????? ???? ??????? ??? ?? ?? ????????.
	//------------------------------------------------
	if (m_SOM.IsInit())
	{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("m_SOM.Clear()");
#endif
		//m_pSurface->BltSpriteOutlineOnly( &m_SOM,  m_SOMOutlineColor );

		// ????
		m_SOM.Clear();
	}

	//----------------------------------------------------------------
	// player?? ?? ??? ??? ??? ????.. [?????3]
	//----------------------------------------------------------------
	if (!bPlayerInCasket && bDrawBackGround)
	{
		//----------------------------------------------------------------
		//
		//          Weather - ???? ??? ???
		//
		//----------------------------------------------------------------

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("Start DrawWeather");
#endif

		__BEGIN_PROFILE("DrawWeather")

			if (g_pWeather->IsActive())
			{
				// ???? ??? ????..
				// ???? Player?? ????? ?????.
				//int gapX = ((g_pPlayer->GetX() - g_pWeather->GetStartX()) * TILE_X + g_pPlayer->GetSX()) % SURFACE_WIDTH;
				//int gapY = ((g_pPlayer->GetY() - g_pWeather->GetStartY()) * TILE_Y + g_pPlayer->GetSY()) % SURFACE_HEIGHT;
				int gapX = (m_FirstZonePixel.x - g_pWeather->GetStartX()) % SURFACE_WIDTH;
				int gapY = (m_FirstZonePixel.y - g_pWeather->GetStartY()) % SURFACE_HEIGHT;

				for (i = 0; i < g_pWeather->GetSize(); i++)
				{
					const MAP_EFFECT& MapEffect = (*g_pWeather)[i];

					// ?????? ???? ?????...
					if (MapEffect.IsActive())
					{
						point.x = MapEffect.GetX() - gapX;
						point.y = MapEffect.GetY() - gapY;

						if (point.x < 0) point.x += SURFACE_WIDTH;
						else if (point.x > SURFACE_WIDTH) point.x -= SURFACE_WIDTH;

						if (point.y < 0) point.y += SURFACE_HEIGHT;
						else if (point.y > SURFACE_HEIGHT) point.y -= SURFACE_HEIGHT;

						if (g_pWeather->GetWeatherType() == MWeather::WEATHER_SPOT)
						{
							CSpriteSurface::SetEffect(CSpriteSurface::EFFECT_SCREEN_ALPHA);
							CSpriteSurface::s_Value1 = MapEffect.GetCount() * 32 / MapEffect.GetMaxCount();
							m_pSurface->BltSpriteEffect(&point, &m_WeatherSPK[MapEffect.GetSpriteID()]);
						}
						//						m_pSurface->BltSpriteAlpha(&point, &m_WeatherSPK[ MapEffect.GetSpriteID() ], MapEffect.GetCount()*10/60);
						else
							m_pSurface->BltSprite(&point, &m_WeatherSPK[MapEffect.GetSpriteID()]);
						//m_pSurface->BltSpriteAlpha(&point, &m_WeatherSPK[ MapEffect.GetSpriteID() ], 24);

						/*
						#ifdef OUTPUT_DEBUG
							if (MapEffect.GetSpriteID()>20)
							{
								DEBUG_ADD_FORMAT("Weather[%d] : spriteID=%d", i, MapEffect.GetSpriteID());
							}
						#endif
						*/
					}
				}
			}


		/*
		//------------------------------------------------
		// ?????? ??? ???
		//------------------------------------------------
		const int ZoneID = g_pZone->GetID();

		if(ZoneID == 61 || ZoneID == 62 || ZoneID == 64 ||
		   ZoneID == 1006 || ZoneID == 1123 || ZoneID == 1603)
		{
			CSprite* pFogSprite = &m_WeatherFullScreenSPK[0];

			POINT CloudPoint = {

				-(m_FirstZonePixel.x % pFogSprite->GetWidth()) - pFogSprite->GetWidth(),
				-(m_FirstZonePixel.y % pFogSprite->GetHeight()),
			};

			POINT CurrentCloudPoint;

			const int speedv = 1;

//			const int speedh = 10;
//			const int pitchh = 3;
//			int offsety = (g_CurrentFrame % (pitchh * speedh)) / speedh;
//
//			if((g_CurrentFrame / (pitchh * speedh)) % 2)
//			{
//				offsety = 2 - offsety;
//			}

			CurrentCloudPoint.x = CloudPoint.x + (g_CurrentFrame % (pFogSprite->GetWidth() * speedv)) / speedv;

			int XSize = (SURFACE_WIDTH / pFogSprite->GetWidth()) + 3;
			int YSize = (SURFACE_HEIGHT / pFogSprite->GetHeight()) + 2;

			for(int ix = 0; ix < XSize; ++ix)
			{
				CurrentCloudPoint.y = CloudPoint.y;

				for(int iy = 0; iy < YSize; ++iy)
				{
					//m_pSurface->BltSpriteAlpha(&CurrentCloudPoint, pFogSprite, 8);
					m_pSurface->BltSpriteDarkness(&CurrentCloudPoint, pFogSprite, 16);
					CurrentCloudPoint.y += pFogSprite->GetHeight();
				}

				CurrentCloudPoint.x += pFogSprite->GetWidth();
			}
		}
		*/


		__END_PROFILE("DrawWeather")

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("End DrawWeather");
#endif

#ifdef __METROTECH_TEST__
		if (!g_bLight)
		{
#endif
			if ((g_pZone->GetID() != 1006 || !g_pPlayer->IsInSafeSector()) &&
				!g_pZone->IsSurvivalZone() &&	// ??????? ??????? ?????? ???. Very Hard -_-;;
				g_pZone->GetID() != 1311 &&			// ??????? ?????? ?????? ???.
				!g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST))
			{
				//----------------------------------------------------------------
				//
				// Draw LightFilter to Screen
				//
				//----------------------------------------------------------------
				__BEGIN_PROFILE("DrawLightFilter")

					//----------------------------------------------------------------
					// player?? light ??? ????
					//----------------------------------------------------------------
					int pX = g_pPlayer->GetPixelX() - m_FirstZonePixel.x + TILE_X_HALF;
				int pY = g_pPlayer->GetPixelY() - m_FirstZonePixel.y - TILE_Y;

				//----------------------------------------------------------------
				// 3D 
				//----------------------------------------------------------------
				// ?????? ??? ?þ? +3	
				//int addLight = (g_pPlayer->GetCreatureType()==CREATURETYPE_BAT ? 3 : 0);
	//			int addLight = (g_pPlayer->IsFlyingCreature() ? 3 : 0);
				g_pPlayer->CalculateLightSight();
				int playerLight = g_pPlayer->GetLightSight();// + g_pPlayer->GetItemLightSight() + addLight);

				if (g_pPlayer->IsInDarkness())
				{
					playerLight = min(playerLight, g_pPlayer->GetDarknessCount());
				}

				//	if(g_pPlayer->HasEffectStatus(EFFECTSTATUS_BLINDNESS))
				//		playerLight = 1;

				//	DEBUG_ADD_FORMAT("######## Sight  (%d)", playerLight); 
				if (CDirect3D::IsHAL())
				{
					AddLightFilter3D(pX,
						pY - (g_pPlayer->IsFlyingCreature() ? 72 : 0),	//g_pPlayer->GetZ(), 
						playerLight,
						false,	// screenPixel???			
						true);	// ?????? ????????? ??		

					// ??????? ???
					ADD_MOTORCYCLE_LIGHT_XY_3D(g_pPlayer, pX, pY, true);

					//---------------------------------------
					// UNLOCK
					//---------------------------------------
					m_pSurface->Unlock();

					//---------------------------------------
					// 3D ?þ?ó?? ???
					//---------------------------------------
					point.x = 0;
					point.y = 0;
					rect.left = 0;
					rect.top = 0;
					rect.right = SURFACE_WIDTH;
					rect.bottom = SURFACE_HEIGHT;
					g_pBack->BltNoColorkey(&point, g_pLast, &rect);

					g_pTopView->DrawLightBuffer3D();

					g_pLast->BltNoColorkey(&point, g_pBack, &rect);

					//				g_pLast->FillSurface(0x001F);

					//				DrawLightBuffer3D();

									// Surface?? ?????? ????????.
									//SetSurfaceInfo(&m_SurfaceInfo, m_pSurface->GetDDSD());
				}
				//----------------------------------------------------------------
				// 2D
				//----------------------------------------------------------------
				else
				{
					// ?????? ??? ?þ? +3		
					AddLightFilter2D(pX,
						pY - g_pPlayer->GetZ(),
						playerLight,
						false,	// screenPixel???			
						true);	// ?????? ????????? ??

					// ??????? ???
					ADD_MOTORCYCLE_LIGHT_XY_2D(g_pPlayer, pX, pY, true);

					DrawLightBuffer2D();

					//---------------------------------------
					// UNLOCK
					//---------------------------------------
					m_pSurface->Unlock();
				}

				__END_PROFILE("DrawLightFilter")
#ifdef __METROTECH_TEST__
			}
#endif
			}
		m_pSurface->Unlock();


		//----------------------------------------------------------------	
		// Unlock ???????..
		//----------------------------------------------------------------		

		//----------------------------------------------------------------
		// Mouse?? ???õ? ????????.. ???? ???? ???
		//----------------------------------------------------------------	

		if (IsRequestInfo())
			//			DrawCreatureMyName();
			bDrawCreatureMyName = DrawCreatureMyName();

		if (!m_DrawNameCreatureInfoVec.empty())
		{
			__BEGIN_PROFILE("DrawSelectedCreature")

				//			DrawCreatureName( m_pSelectedCreature );

				DRAWNAMECREATUREINFO_VEC::iterator pos = m_DrawNameCreatureInfoVec.begin();
			DRAWNAMECREATUREINFO_VEC::iterator endpos = m_DrawNameCreatureInfoVec.end();

			for (; pos != endpos; ++pos)
				DrawCreatureName(&*pos);

			__END_PROFILE("DrawSelectedCreature")
		}
		else
		{
			m_bDrawRequest = false;
		}


		//----------------------------------------------------------------
		// Item??????? ?????? ???.
		//----------------------------------------------------------------
		if (m_bDrawItemNameList && !g_pPlayer->IsInDarkness())
		{
			__BEGIN_PROFILE("DrawItemNameList")

				// unlock???¿??? ???..
				DrawItemNameList();

			__END_PROFILE("DrawItemNameList")
		}
	} // bPlayerInCasket
	else
	{
		// [?????3]
		m_pSurface->Unlock();
	}

	//----------------------------------------------------------------	
	// Lock
	//----------------------------------------------------------------	
	//m_pSurface->Lock();

	//----------------------------------------------------------------
	// ??? ?????? ????? ??????.
	//----------------------------------------------------------------
	// floating damage numbers join this frame's text before it is drawn
	DrawFloatingDamage();

	__BEGIN_PROFILE("DrawTextList")

		DrawTextList();

	__END_PROFILE("DrawTextList")

		//-------------------------------------------------
		// string ?????? ????????? ??? ????
		//-------------------------------------------------
		//*
	//	if(!g_pUserInformation->IsNetmarble) {
		if (!g_pUserInformation->IsNetmarble && !g_pUserInformation->IsTestServer)
		{
			DRAWNAMECREATUREINFO_VEC::iterator pos = m_DrawNameCreatureInfoVec.begin();
			DRAWNAMECREATUREINFO_VEC::iterator endpos = m_DrawNameCreatureInfoVec.end();

			for (; pos != endpos; ++pos)
				if (pos->bOverText)
					DrawCreatureNameOverText(&*pos);

			if (bDrawCreatureMyName)	// ?????
			{
				POINT myPoint = {
					g_pPlayer->GetPixelX() - m_FirstZonePixel.x,
					g_pPlayer->GetPixelY() - g_pPlayer->GetHeight() - m_FirstZonePixel.y
				};

				DrawNameCreatureInfo myDNCInfo(myPoint, g_pPlayer);

				DrawCreatureNameOverText(&myDNCInfo);
			}
		}
	/**/

	//-------------------------------------------------
	// ????? string?? ????? ?? ???????.
	//-------------------------------------------------
	ClearTextList();

	//-------------------------------------------------
	// guild mark ??? ????
	//-------------------------------------------------
//	#ifdef _DEBUG
//
//		if (g_pDXInput->KeyDown(DIK_LCONTROL) && g_pDXInput->KeyDown(DIK_G))
//		{		
//			int guildID = g_pPlayer->GetX()%10;
//
//			//-------------------------------------------------
//			// load??? ????? ????.
//			//-------------------------------------------------
//			CSprite* pSprite = g_pGuildMarkManager->GetGuildMark(guildID);
//
//			if (pSprite==NULL)
//			{		
//				//-------------------------------------------------
//				// file?? ????? ????.
//				//-------------------------------------------------
//				g_pGuildMarkManager->LoadGuildMark(guildID);
//
//				//-------------------------------------------------
//				// file???? load??????? ??? ü?
//				//-------------------------------------------------
//				pSprite = g_pGuildMarkManager->GetGuildMark(guildID);
//
//				//-------------------------------------------------
//				// file???? ???? ???..
//				// guildMark????????????? ???´?.
//				//-------------------------------------------------
//				if (pSprite==NULL)
//				{
//					// ??????.. ??÷? ?????? ???????.
//					char str[256];
//					sprintf(str, "Data\\Guild\\Guild%d.bmp", guildID);
////					g_pGuildMarkManager->CreateGuildMark( guildID, str );
//				}
//			}
//			else
//			{			
//				// ??????? ?? ?? ?????.
//				CSprite* pSpriteSmall = g_pGuildMarkManager->GetGuildMarkSmall(guildID);
//
//				m_pSurface->Lock();
//
//				POINT point = { 100, 100 };
//
//				m_pSurface->BltSprite(&point, pSprite);
//
//				if (pSpriteSmall!=NULL)
//				{
//					point.x = 150;
//					point.y = 100;
//					m_pSurface->BltSprite(&point, pSpriteSmall);
//				}
//
//				m_pSurface->Unlock();
//			}		
//		}
//	#endif


	//----------------------------------------------------------------
	// Light ???
	//----------------------------------------------------------------

	/*
	point.x = 210;
	point.y = 200;
	rect.left = 0;
	rect.top = 0;
	rect.right = 400;
	rect.bottom = 200;
	m_pSurface->CopyTransAlpha(&point, m_pBuffer, &rect, 8);
	*/


	//----------------------------------------------------------------
	// Object ???	
	//----------------------------------------------------------------
	// Y????? sort?? ??? ?????? ???.
	//----------------------------------------------------------------
	// list???? priority queue?? ?????°? ???? ???! sort???~~
	// ?????! list?? ?????? ?????? ???????... ???? ???? ?? ????!
	//----------------------------------------------------------------




	//----------------------------------------------------------------
	// Effect Test
	//----------------------------------------------------------------
	// sword
	/*
	static int swordFrameID = 5;
	static int swordD = 0;
	static int swordFrame = 0;

	point.x = 380;
	point.y = 260;;
	m_pSurface->BltAlphaSprite(&point, &m_EffectAlphaSPK[ m_EffectAlphaFPK[swordFrameID][swordD][swordFrame].GetSpriteID() ]);

	if (++swordFrame == 12) swordFrame = 0;

	// Aura
	static int auraFrameID = 3;
	static int auraD = 0;
	static int auraFrame = 0;

	point.x = 320;
	point.y = 220;;
	m_pSurface->BltAlphaSprite(&point, &m_EffectAlphaSPK[ m_EffectAlphaFPK[auraFrameID][auraD][auraFrame].GetSpriteID() ]);

	if (++auraFrame == 5) auraFrame = 0;
	*/

	//----------------------------------------------------------------
	//
	// TEST CODE  - SpriteOutlineManager Test
	//
	//----------------------------------------------------------------
	/*
	static	CSpriteOutlineManager	SOM;
	static int f = 0;

	if (f==0)
	{
		SOM.Add( 100,70, &m_ImageObjectSPK[38] );
		SOM.Add( 80,100, &m_ImageObjectSPK[38] );
		SOM.Add( 130,100, &m_ImageObjectSPK[39] );
		SOM.Add( 100,130, &m_ImageObjectSPK[39] );

		SOM.Generate();

		f=1;
	}

	m_pSurface->BltSpriteOutline( &SOM, 0xFFFF );
	*/

	/*
	// ????? Test
	if (g_pDXInput->KeyDown(DIK_SPACE))
	{
		point.x = g_x;
		point.y = g_y;

		if (CDirect3D::IsHAL())
		{
			sprite = 0;//rand()%m_pImageObjectShadowManager->GetMaxIndex();

			m_pSurface->Unlock();
			// EffectTexture ????
			//CDirect3D::GetDevice()->SetTexture(0, m_EffectTPK[sprite].GetSurface());
			CSpriteSurface* pSurface = m_pImageObjectShadowManager->GetTexture(sprite);
			CDirect3D::GetDevice()->SetTexture(0, pSurface->GetSurface());

			// ??? ????
			m_SpriteVertices[0].sx = point.x;
			m_SpriteVertices[0].sy = point.y;
			//m_SpriteVertices[1].sx = point.x,m_EffectTPK[sprite].GetWidth();
			m_SpriteVertices[1].sx = point.x + m_pImageObjectShadowManager->GetWidth(sprite);
			m_SpriteVertices[1].sy = point.y;
			m_SpriteVertices[2].sx = point.x;
			//m_SpriteVertices[2].sy = point.y,m_EffectTPK[sprite].GetHeight();
			m_SpriteVertices[2].sy = point.y + m_pImageObjectShadowManager->GetHeight(sprite);
			m_SpriteVertices[3].sx = m_SpriteVertices[1].sx;
			m_SpriteVertices[3].sy = m_SpriteVertices[2].sy;

			// ???
			CDirect3D::GetDevice()->DrawPrimitive(D3DPT_TRIANGLESTRIP,
									D3DFVF_TLVERTEX,
									m_SpriteVertices,
									4, 0);

			m_pSurface->Lock();
		}
		else
		{
			sprite = 0;//rand()%m_ImageObjectSSPK.GetSize();

			m_pSurface->BltShadowSpriteDarkness( &point, &m_ImageObjectSSPK[sprite], 1);
		}
	}
	*/

	//------------------------------------------------------
	// (???)
	// ??????? ?????? ??? ???? ??? ??? --> ??? ???	
	//
	//------------------------------------------------------
	/*
	if (!bPutSelectedSector && g_bLButtonDown)
	{
		POINT cursorPoint;
		GetCursorPos(&cursorPoint);
		// ?????, ???? Player??? ????? ???????? ???? ??????.
		point = GetSelectedSector(cursorPoint.x, cursorPoint.y);
		if (g_pPlayer->GetX()!=point.x || g_pPlayer->GetY()!=point.y)
		{
			// player?? ??????? ???????? ??? ??? ???
			int direction = g_pPlayer->GetDirection();

			// [ TEST CODE ]
			point.x = cursorPoint.x - 32;
			point.y = cursorPoint.y - 16;
			m_pSurface->BltSprite(&point, &m_EtcSPK[5,direction]);
		}
	}
	*/


	//---------------------------------------
	// UNLOCK
	//---------------------------------------
	//m_pSurface->Unlock();



	//---------------------------------------
	// Filter???? ????
	//---------------------------------------
	//rect.left = 410 - 100;
	//rect.right = 410 + 100;
	//rect.top = 270 - 100;
	//rect.bottom = 270 + 100;
	//m_pSurface->DrawRect(&rect, 0xFFFF);

	/*
	// Creature Sprite ???????
	static int s = 3;

	if (++s == 43) s=3;

	point.x = 500;
	point.y = 400;
	m_pSurface->BltSprite(&point, m_SpritePack.GetSprite( s ));
	*/

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawZone");
#endif
}

//----------------------------------------------------------------------
// Draw TileSurface
//----------------------------------------------------------------------
// - Player????? ??????? ???...
//   ???? Tile?? m_pTileSurface?? ?????.
//
// - ???? ????? Object???? ?????? ??¿? ????? ?? ??? ???????.
//----------------------------------------------------------------------
void
MTopView::DrawTileSurface()
{
	//-------------------------------------------------
	//
	// Object?? ?????? ???? ????
	//
	//-------------------------------------------------
	// sprite point
	POINT	tilePoint, point;
	RECT	rect;

	//---------------------------------------------------------------
	// Player?? ???? ??? ???? ù??° Sector????? ?????.
	// ????? (0,0)?? ??Ÿ???? Zone?? Pixel???
	//---------------------------------------------------------------
	m_FirstZonePixel = MapToPixel(g_pPlayer->GetX() + SECTOR_SKIP_PLAYER_LEFT,
		g_pPlayer->GetY() + SECTOR_SKIP_PLAYER_UP);
	m_FirstZonePixel.x += g_pPlayer->GetSX();
	m_FirstZonePixel.y += g_pPlayer->GetSY();

#ifdef OUTPUT_DEBUG
	if (g_pPlayer->GetX() < 0
		|| g_pPlayer->GetY() < 0
		|| g_pPlayer->GetX() >= g_pZone->GetWidth()
		|| g_pPlayer->GetY() >= g_pZone->GetHeight()
		|| m_FirstZonePixel.x > 12000 || m_FirstZonePixel.y > 6144
		|| m_FirstZonePixel.x < 0 || m_FirstZonePixel.y < 0)
	{
		if (g_pDebugMessage)
		{
			DEBUG_ADD_FORMAT("[Error]SX,SY-DrawTileSurface:Player=(%d, %d), s(%d,%d)", g_pPlayer->GetX(), g_pPlayer->GetY(), g_pPlayer->GetSX(), g_pPlayer->GetSY());
		}
	}
#endif

	//---------------------------------------------------------------	
	// ????? (0,0)?? ??µ?? Sector
	//---------------------------------------------------------------		
	POINT	firstSector;
	firstSector = PixelToMap(m_FirstZonePixel.x, m_FirstZonePixel.y);
	m_FirstSector = firstSector;

	//---------------------------------------------------------------
	// TileSurface (0,0)?? ??µ?? Sector???
	//---------------------------------------------------------------
	firstSector.x -= TILESURFACE_SECTOR_EDGE;
	firstSector.y -= TILESURFACE_SECTOR_EDGE;

	//---------------------------------------------------------------
	// TileSurface (0,0)?? Zone?????? pixel????? ???????.
	//---------------------------------------------------------------
	m_TileSurfaceFirstSectorX = firstSector.x;
	m_TileSurfaceFirstSectorY = firstSector.y;

	point = MapToPixel(m_TileSurfaceFirstSectorX, m_TileSurfaceFirstSectorY);
	m_TileSurfaceFirstZonePixelX = point.x;
	m_TileSurfaceFirstZonePixelY = point.y;


	//----------------------------------------------------------------------
	//
	//                         Tile ???
	//
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	// ?????? ???? Sector?? TileSurface?? (0,0)?? ?????? ??????.
	//----------------------------------------------------------------------
	int	sX1 = firstSector.x,
		sX2 = firstSector.x + TILESURFACE_SECTOR_WIDTH,		// sX1 ~ sX2
		sY1 = firstSector.y,
		sY2 = firstSector.y + TILESURFACE_SECTOR_HEIGHT;	// sY1 ~ sY2

	// ????? Surface???? ???	
	tilePoint.x = 0;
	tilePoint.y = 0;

	//------------------------------------------------------
	// Zone?? ?????? ??? ??? Skip...
	//------------------------------------------------------					
	if (sX1 < 0)
	{
		// Zone??? ????? ?????????? ????.
		rect.left = tilePoint.x;
		rect.top = 0;

		tilePoint.x += -sX1 * TILE_X;

		rect.right = tilePoint.x;
		rect.bottom = TILESURFACE_HEIGHT;

		m_pTileSurface->FillRect(&rect, 0);

		sX1 = 0;
	}

	if (sX2 > m_pZone->GetWidth())
	{
		sX2 = m_pZone->GetWidth();//-1;

		// Zone??? ????? ?????????? ????.
		rect.left = tilePoint.x + (sX2 - sX1) * TILE_X;
		rect.top = 0;
		rect.right = TILESURFACE_WIDTH;
		rect.bottom = TILESURFACE_HEIGHT;

		m_pTileSurface->FillRect(&rect, 0);
	}

	if (sY1 < 0)
	{
		// Zone??? ????? ?????????? ????.
		rect.left = tilePoint.x;
		rect.top = tilePoint.y;

		tilePoint.y += -sY1 * TILE_Y;

		rect.right = tilePoint.x + (sX2 - sX1) * TILE_X;
		rect.bottom = tilePoint.y;
		m_pTileSurface->FillRect(&rect, 0);

		sY1 = 0;
	}

	if (sY2 > m_pZone->GetHeight())
	{
		// Zone??? ????? ?????????? ????.
		rect.left = tilePoint.x;
		rect.top = tilePoint.y;
		rect.right = tilePoint.x + (sX2 - sX1) * TILE_X;
		rect.bottom = TILESURFACE_HEIGHT;
		m_pTileSurface->FillRect(&rect, 0);

		sY2 = m_pZone->GetHeight();//-1;
	}

	//------------------------------------------------------
	// ??? ?????????? ??ü?? ????.
	//------------------------------------------------------
	/*
	rect.left = 0;
	rect.top = 0;
	rect.right = TILESURFACE_WIDTH;
	rect.bottom = TILESURFACE_HEIGHT;

	m_pTileSurface->FillRect(&rect, 0);
	*/

	//------------------------------------------------------
	// ?? Sector???
	//------------------------------------------------------	
	// ù??° ??
	POINT tilePointTemp;
	tilePointTemp.y = tilePoint.y;

	int x;
	int y;

	DEBUG_ADD_FORMAT("[DrawTileSurface] (%d, %d) ~ (%d, %d)", sX1, sX2, sY1, sY2);

	//---------------------------------------
	// LOCK
	//---------------------------------------
	if (!m_pTileSurface->Lock()) return;

	//char str[80];
	for (y = sY1; y < sY2; y++)
	{
		// ?? ???? ù??° Sector					
		tilePointTemp.x = tilePoint.x;

		for (x = sX1; x < sX2; x++)
		{
			point = tilePointTemp;

			// (sX,sY) Sector?? SpriteID?? ??? ???
			//m_pTileSurface->Lock();			

			int spriteID = m_pZone->GetSector(x, y).GetSpriteID();

			if (spriteID == SPRITEID_NULL)
			{
#ifdef __DEBUG_OUTPUT__ 
				if (g_pZone->GetID() == 3001 && m_pZone->GetSector(x, y).IsBlockAny())
					m_pTileSurface->BltSprite(&point, &m_EtcSPK[1]);
				else
					m_pTileSurface->BltSprite(&point, &m_EtcSPK[SPRITEID_TILE_NULL]);
#else
				m_pTileSurface->BltSprite(&point, &m_EtcSPK[SPRITEID_TILE_NULL]);
#endif
			}
			else
			{
#ifdef OUTPUT_DEBUG
				//if (g_pDebugMessage)
				//	DEBUG_ADD_FORMAT("Draw Tile (%d, %d) id=%d", x, y, spriteID);
#endif

				CSprite& sprite = m_TileSPK[spriteID];

				//---------------------------------------
				// ID?? spriteID?? Tile?? Load???.
				//---------------------------------------
//				#ifdef	OUTPUT_DEBUG
//					char str[256];
//				#endif
//				if (sprite.IsNotInit())
//				{
//					#ifdef	OUTPUT_DEBUG
//						sprintf(str, "[RunTimeLoading] Tile(%d,%d) - DrawTileSurface : sprite=%d", x, y, spriteID);
//					#endif
//
//					m_TileSPKFile.seekg(m_TileSPKI[spriteID], ios::beg);
//					//--------------------------------------------------
//					// ?????? ???.. 
//					//--------------------------------------------------
//					if (m_TileSPK[spriteID].LoadFromFile( m_TileSPKFile ))
//					{
//						#ifdef	OUTPUT_DEBUG
//							strcat(str, "...OK");
//						#endif
//					}
//					//--------------------------------------------------
//					// ?????? ??? --> ??? Loading??? ??? ??????.				
//					//--------------------------------------------------
//					/*
//					// 2001.8.20 ???ó??
//					else
//					{
//						#ifdef	OUTPUT_DEBUG
//							strcat(str, "...Fail & Wait Loading");
//						#endif
//
//						// file thread ?????? ??????.
//						//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_HIGHEST);
//
//						// Thread???? Loading?? ?????????? ??????.
//						//while (m_TileSPK[spriteID].IsNotInit());
//						//while (!m_TileSPK[spriteID].LoadFromFile( m_TileSPKFile ));
//						MLoadingSPKWorkNode3* pNode = new MLoadingSPKWorkNode3(spriteID, m_TileSPKI[spriteID]);
//						pNode->SetSPK( &m_TileSPK, FILE_SPRITE_TILE );
//						pNode->SetType( 1 );
//						g_pLoadingThread->SetPriority( THREAD_PRIORITY_HIGHEST );
//						g_pLoadingThread->AddFirst( pNode );
//
//						while (1)
//						{
//							#ifdef	OUTPUT_DEBUG
//							//	DEBUG_ADD_FORMAT( "Check Load id=%d", spriteID );
//							#endif	
//
//							if (m_TileSPK[spriteID].IsInit())
//							{
//								#ifdef	OUTPUT_DEBUG
//								//	DEBUG_ADD( "Is Init" );
//								#endif	
//
//								break;
//							}
//							else
//							{
//								#ifdef	OUTPUT_DEBUG
//								//	DEBUG_ADD( "Is Not Init" );
//								#endif	
//							}
//						}
//
//						// file thread ?????? ?????.
//						//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_BELOW_NORMAL);					
//						g_pLoadingThread->SetPriority( THREAD_PRIORITY_LOWEST );
//					}
//					*/
//
//					DEBUG_ADD( str );
//				}

				// ???~~!!!!!!! ??? ??? ??´?~!!!
//				POINT pointTempTemp = point;
//				m_pTileSurface->BltSprite(&pointTempTemp, &m_EtcSPK[SPRITEID_TILE_NULL]);

				if (sprite.GetWidth() != 48 || sprite.GetHeight() != 24)
					int i = 0;

				m_pTileSurface->BltSprite(&point, &sprite);

#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
				if (m_pZone->GetSector(x, y).IsBlockGround())
				{
					if (g_pDXInput->KeyDown(DIK_T) && g_pDXInput->KeyDown(DIK_LCONTROL))
					{
						m_pTileSurface->BltSpriteColor(&point, &sprite, 0);
					}
				}
#endif				
			}

			//			m_PreviousFogSpriteID[y][x] = SPRITEID_NULL;//sector.GetFilterSpriteID();
						//m_pTileSurface->Unlock();		


						// ???????? ??? ???
			tilePointTemp.x += TILE_X;
		}

		// ???? ??
		tilePointTemp.y += TILE_Y;
	}

	//---------------------------------------
	// UNLOCK
	//---------------------------------------
	m_pTileSurface->Unlock();
}

//----------------------------------------------------------------------
// Draw PartyHP
//----------------------------------------------------------------------
void
MTopView::DrawPartyHP(POINT* pPoint, MCreature* pCreature)
{
	pPoint->x += 10;

	//-----------------------------------------------------
	// HP???
	//-----------------------------------------------------
	int currentHP = pCreature->GetHP();
	int maxHP = pCreature->GetMAX_HP();

	//-----------------------------------------------------
	// ????? ??? ??? HP?? ?? ????ó?? ???????.
	//-----------------------------------------------------
	if (pCreature->IsUndergroundCreature())
	{
		currentHP = 1;
		maxHP = 1;
	}

	if (currentHP > maxHP)
	{
		currentHP = maxHP;
	}

	//-----------------------------------------------------
	// ?????? ?????? pixel???
	//-----------------------------------------------------
	CSprite* pHPBackSprite = &m_EtcSPK[SPRITEID_PARTY_HP_BACKGROUND];
	CSprite* pHPSprite = &m_EtcSPK[SPRITEID_PARTY_HP];

	int maxPixels = pHPBackSprite->GetWidth();
	int currentPixels = (maxHP == 0) ? 0 : maxPixels * currentHP / maxHP;

	//-----------------------------------------------------
	// ???
	//-----------------------------------------------------
	POINT pointTemp;

	pointTemp.x = pPoint->x;
	pointTemp.y = pPoint->y;
	m_pSurface->BltSprite(&pointTemp, pHPBackSprite);

	pointTemp.x = pPoint->x;
	pointTemp.y = pPoint->y;

	int clipRight = pointTemp.x + currentPixels;

	if (clipRight > 0)
	{
		if (clipRight < m_pSurface->GetWidth())
		{
			int oldClipRight = m_pSurface->GetClipRight();
			m_pSurface->SetClipRight(clipRight);

			m_pSurface->BltSprite(&pointTemp, pHPSprite);

			m_pSurface->SetClipRight(oldClipRight);
		}
		else
		{
			m_pSurface->BltSprite(&pointTemp, pHPSprite);
		}
	}
}


//----------------------------------------------------------------------
// Draw InventoryEffect
//----------------------------------------------------------------------
void
MTopView::DrawInventoryEffect(POINT* pPoint)
{
	if (g_pInventoryEffectManager != NULL)
	{
		//------------------------------------------------
		// Unlock ???·? ?????. 
		//------------------------------------------------
		BOOL bLock = m_pSurface->IsLock();
		if (!bLock)
		{
			m_pSurface->Lock();
		}

		MScreenEffect::SetScreenBasis(pPoint->x, pPoint->y);

		g_pTopView->DrawEffect(pPoint, g_pInventoryEffectManager->GetEffects(), g_pInventoryEffectManager->GetSize());

		//------------------------------------------------
		// ???? lock ?? ???¸? ??? ???????.
		//------------------------------------------------
		if (!bLock)
		{
			m_pSurface->Unlock();
		}
	}
}


//----------------------------------------------------------------------
// Draw TitleEffect
//----------------------------------------------------------------------
void
MTopView::DrawTitleEffect(int iIndex)
{
#if __CONTENTS(__TITLE_UI_RENWEAL)
	std::vector<S_TITLE_UI_EFFECT>::iterator it = m_TitleUI_EFFECT_Info.begin();
	for (int i = 0; i < m_TitleUI_EFFECT_Info.size(); i++, it++)
	{
		if (iIndex != it->iIndex)
		{
			continue;
		}

		int _FrameID = (int)it->iFrameID;
		int _Direction = 0;

		if (it->iFrame > m_EffectAlphaFPK[_FrameID][_Direction].GetSize() - 1)
			it->iFrame = 0;

		int _Frame = it->iFrame;
		CEffectFrame& Frame = m_EffectAlphaFPK[_FrameID][_Direction][_Frame];
		int spriteID = Frame.GetSpriteID();

		POINT point = it->point;
		if (spriteID != SPRITEID_NULL)
		{
			point.x += Frame.GetCX();
			point.y += Frame.GetCY();

			DRAW_ALPHASPRITEPAL(&point, spriteID, m_EffectAlphaSPK, m_EffectAlphaPPK[_FrameID])//, m_EffectAlphaSPKI, m_EffectAlphaSPKFile)
		}

		if ((g_FrameCount % it->wDelay) == 0)
			it->iFrame++;
	}
#endif //__TITLE_UI_RENWEAL
}
#if __CONTENTS(__TITLE_UI_RENWEAL)
void	MTopView::AddTitleEffect(POINT* pPoint, WORD wEffectID, WORD wDelay, int iIndex)
{
	S_TITLE_UI_EFFECT	EF;

	EF.iFrameID = wEffectID;
	EF.iDirection = 0;
	EF.iFrame = 0;
	EF.point = *pPoint;
	EF.wDelay = wDelay;
	EF.iIndex = iIndex;
	m_TitleUI_EFFECT_Info.push_back(EF);
}


void	MTopView::DeleteTitleEffect()
{
	m_TitleUI_EFFECT_Info.clear();
	m_TitleUI_EFFECT_Info.resize(0);
}


void	MTopView::SetTitleEffectPos(POINT* pPoint, int iIndex)	//????? ??? ????
{
	std::vector<S_TITLE_UI_EFFECT>::iterator it = m_TitleUI_EFFECT_Info.begin();
	for (int i = 0; i < m_TitleUI_EFFECT_Info.size(); i++, it++)
	{
		if (iIndex != it->iIndex)
		{
			continue;
		}

		it->point = *pPoint;
	}
}
#endif //__TITLE_UI_RENWEAL


#if __CONTENTS(__BLITZ_COUPON_EVENT)
//----------------------------------------------------------------------
// Draw UIEffect
//----------------------------------------------------------------------
bool
MTopView::DrawUIEffect(POINT* pPoint, int _FrameID, int _Direction, int _Dly, bool _Loop)
{
	if (!_Loop && m_UIEffect_Frame + 1 >= m_EffectAlphaFPK[_FrameID][_Direction].GetSize())
	{
		return false;
	}

	if ((g_FrameCount % _Dly) == 0)
	{
		m_UIEffect_Frame++;
		if (m_UIEffect_Frame >= m_EffectAlphaFPK[_FrameID][_Direction].GetSize())
		{
			m_UIEffect_Frame = 0;
		}
	}

	if (m_EffectAlphaFPK[_FrameID][_Direction].GetSize() <= m_UIEffect_Frame)
	{
		m_UIEffect_Frame = 0;
	}

	CEffectFrame& Frame = m_EffectAlphaFPK[_FrameID][_Direction][m_UIEffect_Frame];
	int spriteID = Frame.GetSpriteID();
	POINT point = *pPoint;
	if (spriteID != SPRITEID_NULL)
	{
		point.x += Frame.GetCX();
		point.y += Frame.GetCY();

		DRAW_ALPHASPRITEPAL(&point, spriteID, m_EffectAlphaSPK, m_EffectAlphaPPK[_FrameID])
	}
	return true;
}
#endif //__BLITZ_COUPON_EVENT

// 2004, 11, 22, sobeit add start
void
MTopView::DrawBloodBibleEffect_InGear(POINT* pPoint)
{
#if __CONTENTS(__CONTRIBUTE_SYSTEM)
	int _FrameID = EFFECTSPRITETYPE_BLOODY_BIBLE_IN_GEAR_BLUE_EFFECT;
	int contributeGrade = countContributeGrade(g_char_slot_ingame.ContributePoint);

	if (contributeGrade == 1)
		_FrameID = EFFECTSPRITETYPE_BLOODY_BIBLE_IN_GEAR;
	else
		_FrameID -= (contributeGrade - 2);
	if (_FrameID > MAX_EFFECTSPRITETYPE_ALPHAEFFECT && _FrameID < 0)
		return;
#else
	int _FrameID = EFFECTSPRITETYPE_BLOODY_BIBLE_IN_GEAR;
#endif 

	int _Direction = 0;
	int _Frame = (g_FrameCount % 112) >> 3;


	CEffectFrame& Frame = m_EffectAlphaFPK[_FrameID][_Direction][_Frame];
	int spriteID = Frame.GetSpriteID();

	POINT point = *pPoint;
	if (spriteID != SPRITEID_NULL)
	{
		point.x += Frame.GetCX();
		point.y += Frame.GetCY();

		DRAW_ALPHASPRITEPAL(&point, spriteID, m_EffectAlphaSPK, m_EffectAlphaPPK[_FrameID])//, m_EffectAlphaSPKI, m_EffectAlphaSPKFile)
	}
}
#if __CONTENTS(__CONTRIBUTE_SYSTEM)
void
MTopView::DrawContrubuteGradeEffect(POINT* pPoint, MCreature* pCreature, int yPoint)
{
	int contributeGrade = countContributeGrade(pCreature->GetContributePoint());

	if (contributeGrade < 1 || contributeGrade > 7)
	{
		return;
	}
	int _FrameID = EFFECTSPRITETYPE_NAME_RED_EFFECT2 - ((contributeGrade - 1) * 2);
	int _Direction = 0;
	int _Frame = (g_FrameCount % 256) >> 2;

	//????? ???? ??????.. ?? ????? ??? ???? ???????? ???????. ????? ???? ?????? ?????°?...
	if (_FrameID > MAX_EFFECTSPRITETYPE_ALPHAEFFECT || _FrameID < 0 || _Frame < 0 || _Frame > 63)
		return;

	CEffectFrame& Frame = m_EffectAlphaFPK[_FrameID][_Direction][_Frame];
	int spriteID = Frame.GetSpriteID();
	POINT point = *pPoint;
	point.x += 2;
	point.y = yPoint;
	point.y -= 24;

	if (spriteID != SPRITEID_NULL)
	{
		point.x += Frame.GetCX();
		point.y += Frame.GetCY();
		//DRAW_ALPHASPRITEPAL(&point, spriteID, m_EffectAlphaSPK, m_EffectAlphaPPK[_FrameID])//, m_EffectAlphaSPKI, m_EffectAlphaSPKFile)
		//?? ???? ???? ??? ?? ??? ?????? ??? ???? ????? ?????? ????? ???? ?????? ???...
		CAlphaSpritePal* pSprite = &m_EffectAlphaSPK[spriteID];
		m_pSurface->BltAlphaSpritePal(pPoint, pSprite, m_EffectAlphaPPK[_FrameID]);
	}

	_FrameID--;
	_Frame = (g_FrameCount % 256) >> 2;
	if (_FrameID > MAX_EFFECTSPRITETYPE_ALPHAEFFECT || _FrameID < 0 || _Frame < 0 || _Frame > 63)
		return;
	CEffectFrame& Frame_Reverse = m_EffectAlphaFPK[_FrameID][_Direction][_Frame];
	spriteID = Frame_Reverse.GetSpriteID();
	point = *pPoint;
	point.x += 2;
	point.y = yPoint;
	point.y -= 24;

	if (spriteID != SPRITEID_NULL)
	{
		point.x += Frame.GetCX();
		point.y += Frame.GetCY();
		CAlphaSpritePal* pSprite = &m_EffectAlphaSPK[spriteID];
		m_pSurface->BltAlphaSpritePal(pPoint, pSprite, m_EffectAlphaPPK[_FrameID]);
	}
}
#endif //__CONTRIBUTE_SYSTEM
// 2004, 11, 22, sobeit add end
//----------------------------------------------------------------------
// Draw Chat String
//----------------------------------------------------------------------
int
MTopView::DrawChatString(POINT* pPoint, MCreature* pCreature, COLORREF color, BYTE flag)
{
	// ??????? ???????
	if (g_pOperatorOption->bHideSpeechBalloon)
		return 0;

	int y2 = pPoint->y;
	// ?? ??????? ???...
	//pPoint->x -= 50;

	// 2005, 1, 20, sobeit add start
	if (g_pUserOption->DoNotShowPersnalShopMsg && pCreature->CurPernalShop())
	{
		//str = "...";
		return y2;
	}
	// 2005, 1, 20, sobeit add end
	//---------------------------------------------------------
	//
	//			Font????
	//
	//---------------------------------------------------------	
	FONTID	font;
	//---------------------------------------------------------
	// Vampire?? Slayer?? ???? Font?? ????? ?????.
	//---------------------------------------------------------
	if (pCreature->IsVampire())
	{
		//gC_font.NotUseNoPublicEng();

		font = FONTID_VAMPIRE_CHAT;
	}
	else //if (pCreature->IsSlayer() || pCreature->IsNPC())	
	{
		font = FONTID_SLAYER_CHAT;
	}

	bool	bLargeFont = pCreature->HasEffectStatus(EFFECTSTATUS_LOUD);
	if (bLargeFont)
		font = FONTID_LARGE_CHAT;

	const int MinWidthChatBox = bLargeFont ? MIN_CHAT_LARGE_BOX_WIDTH : MIN_CHAT_BOX_WIDTH;
	const int ChatFontHeight = bLargeFont ? g_pClientConfig->FONT_HEIGHT + 5 : g_pClientConfig->FONT_HEIGHT;
	//---------------------------------------------------------
	//
	//		???? ?? string?? ????? ??´?.
	//
	//---------------------------------------------------------
	int maxLen = 0;
	int numString = g_pClientConfig->MAX_CHATSTRING;
	int maxWidth = 0;
	for (int i = g_pClientConfig->MAX_CHATSTRING_MINUS_1; i >= 0; i--)
	{
		const char* str = pCreature->GetChatString(i);

		//---------------------------------------------------------
		// ???? ??????.. ?????????.
		//---------------------------------------------------------
		if (str[0] != NULL)
		{
			int width = max(MinWidthChatBox, g_GetStringWidth(str, g_ClientPrintInfo[font]->hfont));

			if (width > maxWidth)
			{
				maxWidth = width;
			}
		}
		else
		{
			numString = g_pClientConfig->MAX_CHATSTRING_MINUS_1 - i;

			//---------------------------------------------------------
			// ?????? ???? ???	
			//---------------------------------------------------------
			if (numString == 0)
			{
				return y2;
			}

			break;
		}
	}

	//---------------------------------------------------------
	//
	//			??? ???? ????
	//
	//---------------------------------------------------------	
	int firstY = y2 - numString * ChatFontHeight;

	int x = pPoint->x;
	int y = firstY;



	//---------------------------------------------------------
	// ??? ??? ????? ????????..
	//---------------------------------------------------------
	if (x < 0) x = 0;

	if (m_SelectCreatureID == pCreature->GetID())
	{
		const int FontHeight = ChatFontHeight;

		if (pCreature->HasLevelName())
		{
			const int FontHeight2 = FontHeight << 1;

			if (y < FontHeight2)
			{
				firstY = FontHeight2;

				y2 = (numString + 2) * FontHeight;
				y = FontHeight2;
			}
		}
		else if (y < FontHeight)
		{
			firstY = FontHeight;

			y2 = (numString + 1) * FontHeight;
			y = FontHeight;
		}
	}
	else
	{
		if (y < 0)
		{
			firstY = 0;

			y2 = numString * ChatFontHeight;
			y = 0;
		}
	}

	int x2 = x + maxWidth;//gC_font.GetStringmaxWidth( str ),

	if (x2 >= CLIPSURFACE_WIDTH)
	{
		x -= x2 - CLIPSURFACE_WIDTH;
		x2 = CLIPSURFACE_WIDTH;
	}

	QWORD timeBase;

	if (pCreature == m_pSelectedCreature)
	{
		timeBase = ((QWORD)(g_CurrentFrame + 160) << 41);
	}
	else
	{
		if (pCreature->CurPernalShop() == 1)
			timeBase = ((QWORD)(g_CurrentFrame + pCreature->GetPersonalShopMsgTime()) << 41);
		else
			timeBase = ((QWORD)pCreature->GetChatTime() << 41)
			| ((QWORD)pCreature->GetID() << 9);
	}

	//---------------------------------------------------------
	//  ?? string ???...
	//---------------------------------------------------------
	int start = g_pClientConfig->MAX_CHATSTRING - numString;
	for (int i = start; i < g_pClientConfig->MAX_CHATSTRING; i++)
	{
		const char* str = pCreature->GetChatString(i);

		// ???? ??????E. ?????????.
		//if (str[0] != NULL)
		{
			if (pCreature->CurPernalShop() == 1)
				color = pCreature->GetPersnalShopColor();

			//m_pSurface->GDI_Text(pointTemp.x, pointTemp.y, str, 0xFFFF);
			//gC_font.PrintStringNoConvert(&m_SurfaceInfo, str, pPoint->x, pPoint->y, color);
			DRAWTEXT_NODE* pNode = new DRAWTEXT_NODE(
				x,
				y + 3,
				str,
				color,
				font,//,
				0//flag// | FLAG_DRAWTEXT_HANGUL_JOHAP
			);

			//---------------------------------------------------------
			// 33???? ????.. DWORD time???? ù bit?? ?????? ?? ???
			// Creature Object ID?? ???????? ?????? ???? ?????? 
			// ??¼??? i?? ?????? ??????? ??µ?? ?? ?????.
			//
			// ??????? < ???????? < ?????
			//---------------------------------------------------------
			QWORD time = timeBase + m_pqDrawText.size();
			pNode->SetTextTime(time);

			//---------------------------------------------------------
			// ?????? ???
			//---------------------------------------------------------
			if (i == start)
			{
				RECT rect = {
					x - 4,
					firstY - 2,
					x2 + 4,
					y2 + 2
				};

				pNode->SetBox(rect, m_ColorBlackHalf);

				//---------------------------------------------------------
				// ä?ÿ? ??? ??? ?????
				//---------------------------------------------------------
				if (g_pUserOption->DrawChatBoxOutline)
				{
					DWORD color_degree = pCreature->GetOriChatColor();

					DWORD shopColor = pCreature->GetPersnalShopColor();//
					BYTE r1, g1, b1;

					r1 = GetRValue(shopColor) >> 3;
					g1 = GetGValue(shopColor) >> 3;
					b1 = GetBValue(shopColor) >> 3;

					BYTE	r, b = (color_degree & 0xFF000000) >> 24;;

					b = min(max(b - 8, 0), 20);
					r = min(24, b + 4);

					if (pCreature->CurPernalShop() == 1)
						pNode->SetBoxOutline(CDirectDraw::Color(r1, g1, b1));
					else
						pNode->SetBoxOutline(CDirectDraw::Color(r, b, b));
				}
			}

			AddText(pNode);
		}
		//else
		//{	
			// y?? ??? return
		//	return firstY;
		//}

		// ????? ?????..
		y += ChatFontHeight;
	}

	return firstY;
}

//----------------------------------------------------------------------
// Draw Item : Item ??????	
//----------------------------------------------------------------------
// pSurface?? pPoint?? pItem?? ??????. 
//----------------------------------------------------------------------
void
MTopView::DrawItem(POINT* pPoint, MItem* pItem)
{
	if (g_pEventManager->GetEventByFlag(EVENTFLAG_NOT_DRAW_ITEM))
		return;

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawItem");
#endif

	__BEGIN_PROFILE("DrawItem")

		//------------------------------------------------
		// Slayer?? ??? Darkness ???? ?? ?? ????.
		//------------------------------------------------
		if (g_pZone->GetSector(pItem->GetX(), pItem->GetY()).HasDarkness() &&
			IsAffectFromDarkness() &&
			!(
				pItem->GetItemClass() == ITEM_CLASS_CORPSE &&
				((MCorpse*)pItem)->GetCreature() != NULL &&
				(
					((MCorpse*)pItem)->GetCreature()->GetCreatureType() >= 526 && ((MCorpse*)pItem)->GetCreature()->GetCreatureType() <= 549 ||
					((MCorpse*)pItem)->GetCreature()->GetCreatureType() >= 371 && ((MCorpse*)pItem)->GetCreature()->GetCreatureType() <= 376 ||
					((MCorpse*)pItem)->GetCreature()->GetCreatureType() >= 560 && ((MCorpse*)pItem)->GetCreature()->GetCreatureType() <= 563 ||
					((MCorpse*)pItem)->GetCreature()->GetCreatureType() == 670 ||
					((MCorpse*)pItem)->GetCreature()->GetCreatureType() == 672 ||
					((MCorpse*)pItem)->GetCreature()->GetCreatureType() == 673
					)
				)
			)
		{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("End DrawItem(D)");
#endif

			return;
		}

	//---------------------------------------------------------
	// ??ü?? ???? Creature?? ??????.
	//---------------------------------------------------------
	if (pItem->GetItemClass() == ITEM_CLASS_CORPSE)
	{
		MCorpse* pCorpse = (MCorpse*)pItem;

		MCreature* pCreature = pCorpse->GetCreature();

		POINT orPoint = *pPoint;

		if (pCreature != NULL)
		{
			//---------------------------------------------------------
			// PC Vampire?? ???? '????'?? ??????.
			// 
			// ?????????? ??? '????'?? ??????.
			//---------------------------------------------------------
//			if (//pCreature->GetCreatureType()==CREATURETYPE_VAMPIRE_MALE 
//				//|| pCreature->GetCreatureType()==CREATURETYPE_VAMPIRE_FEMALE)
//				(*g_pCreatureSpriteTable)[(*g_pCreatureTable)[pCreature->GetCreatureType()].SpriteType].IsPlayerVampireOnlySprite()
//				|| g_pUserInformation->GoreLevel == false)
//			{
//				CSprite* pSprite = &m_EtcSPK[ SPRITEID_VAMPIRE_DUST ];
//
//				POINT pointTemp;
//
//				// ??? ????
//				pointTemp.x = pPoint->x - 5;
//				pointTemp.y = pPoint->y;
//					
//
//				// ???õ? ???? ???
//				// Creature?? ?????? ??ó????? ???.. ????. - -;
///				if (m_SelectItemID == pItem->GetID())
//				{
//					m_SOMOutlineColor = m_ColorOutlineItem;
//					
//					m_SOM.Add( pointTemp.x, pointTemp.y, pSprite );
//					
//					m_SOM.Generate();
//
//					m_pSurface->BltSpriteOutline( &m_SOM, m_SOMOutlineColor );
//				}
//				else
//				{
//					m_pSurface->BltSprite(&pointTemp, pSprite);	
//				}
//			}
			//---------------------------------------------------------
			// ??? ????? ???? ???..
			//---------------------------------------------------------
//			else
			{
				// ???õ? ???? ???
				// Creature?? ?????? ??ó????? ???.. ????. - -;
				if (m_SelectItemID == pItem->GetID())
				{
					TYPE_OBJECTID temp = m_SelectCreatureID;
					m_SelectCreatureID = pCreature->GetID();

					//					if (
					//							(
					//							pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376	// ????
					//							|| pCreature->GetCreatureType() == 482	// ????????? ???
					//							)
					//						||
					//							(
					//							pCreature->GetActionCount() < pCreature->GetActionCountMax()
					//							// ??????? ????? ????????ü ????? ??????????
					//							|| g_pUserInformation->GoreLevel == false
					//							&& pCreature->GetActionCount()>=pCreature->GetActionCountMax()/2
					//							&& pCreature->IsDead()
					//							)
					//						)
					//					{
					//						POINT pointTemp = *pPoint;
					//						DrawCreatureShadow(&pointTemp, pCreature);
					//					}

					DrawCreature(pPoint, pCreature);

					m_SelectCreatureID = OBJECTID_NULL;
				}
				else
				{
					//					if (
					//							(
					//							pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376	// ????
					//							|| pCreature->GetCreatureType() == 482	// ????????? ???
					//							)
					//						||
					//							(
					//							pCreature->GetActionCount() < pCreature->GetActionCountMax()
					//							// ??????? ????? ????????ü ????? ??????????
					//							|| g_pUserInformation->GoreLevel == false
					//							&& pCreature->GetActionCount()>=pCreature->GetActionCountMax()/2
					//							&& pCreature->IsDead()
					//							)
					//						)
					//					{
					//						POINT pointTemp = *pPoint;
					//						DrawCreatureShadow(&pointTemp, pCreature);
					//					}

					DrawCreature(pPoint, pCreature);
				}
			}

			//------------------------------------------------
			// ???? ???????? ???? ??????
			// ?? Tile ???? ???????.
			//------------------------------------------------
			if (!(pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376 || pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563 || pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549 || pCreature->GetCreatureType() == 672 || pCreature->GetCreatureType() == 673))
			{
				if (pCreature->GetCreatureType() == 482 ||
					pCreature->GetCreatureType() == 650 ||
					pCreature->GetCreatureType() == 819)	// ????? select????
				{
					RECT rect;
					rect.left = orPoint.x;
					rect.top = orPoint.y;
					rect.right = orPoint.x;
					rect.bottom = orPoint.y;
					pCreature->SetScreenRect(&rect);
				}
				else
				{
					RECT rect;
					rect.left = orPoint.x;
					rect.top = orPoint.y;
					rect.right = orPoint.x + TILE_X;
					rect.bottom = orPoint.y + TILE_Y;
					pCreature->SetScreenRect(&rect);
				}
			}
		}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("End DrawItem");
#endif

		if (pCreature != NULL)
			pCreature->FadeChatString();

		return;
	}

	//---------------------------------------------------------
	// ???? ???????? ??? ?????..
	//---------------------------------------------------------
	// m_ItemDropFPK?? m_ItemDropSPK?? ???..
	//---------------------------------------------------------
	if (pItem->IsDropping())
	{
		int dropFrameID = pItem->GetDropFrameID();
		int frame = pItem->GetFrame();

		//#ifdef OUTPUT_DEBUG
		if (dropFrameID >= m_ItemDropFPK.GetSize())
		{
			DEBUG_ADD_FORMAT("[Error] DrawDropItem:item exceed Max : id=%d, itemFrame=%d", pItem->GetID(), dropFrameID);
			return;
		}
		else if (frame >= m_ItemDropFPK[dropFrameID].GetSize())
		{
			DEBUG_ADD_FORMAT("[Error] DrawDropItem:frame exceed Max : id=%d, itemFrame=%d, Frame=%d", pItem->GetID(), dropFrameID, frame);
			return;
		}
		//#endif

		//---------------------------------------------------------
		// ??? ?????????.. MZone::AddItem( ...) ????
		// if (bDropping) ???? ???????? ????? ???..
		//---------------------------------------------------------
// for?? ???? colse???? ??????????????afpk?????? ?????? ?????? ???????????  ???? ????? ???????.DJ 2006.12.19
/*
			const int numFrames = 6;
			int nFrame[numFrames] = {3, 0, 1, 2, 3, 0};

			for(TYPE_SPRITEID dropIndex = 0 ; dropIndex < m_ItemDropFPK.GetSize(); dropIndex++ )
			{

				for(TYPE_FRAMEID Index = 0; Index <  numFrames; Index++)
				{
					CFrame &dropFrameID = m_ItemDropFPK[ dropIndex ][ Index ];
					int sid = dropIndex*4 + nFrame[Index];
					int cx = TILE_X_HALF - (m_ItemDropISPK[sid].GetWidth()>>1);
					int cy = TILE_Y_HALF - (m_ItemDropISPK[sid].GetHeight()>>1);
					m_ItemDropFPK[ dropIndex ][ Index ].Set(sid, cx, cy);
				}
			}

			std::ofstream itemdropfilsave;
			std::ofstream itemdropfilsaveI;

			itemdropfilsave.open(g_pFileDef->getProperty("FILE_AFRAME_ITEMDROP").c_str(), ios::binary);
			itemdropfilsaveI.open(g_pFileDef->getProperty("FILE_AFRAMEINDEX_ITEMDROP").c_str(), ios::binary);
			m_ItemDropFPK.SaveToFile(itemdropfilsave,itemdropfilsaveI);

			itemdropfilsave.close();
			itemdropfilsaveI.close();
			/**/

		CFrame& Frame = m_ItemDropFPK[dropFrameID][frame];
		//CFrame &Frame = m_ItemTileFPK[ pItem->GetTileFrameID() ];

		POINT pointTemp;
		pointTemp.x = pPoint->x + Frame.GetCX();
		pointTemp.y = pPoint->y + Frame.GetCY() - pItem->GetDropHeight();

#ifdef OUTPUT_DEBUG
		if (Frame.GetSpriteID() >= m_ItemDropISPK.GetSize())
		{
			DEBUG_ADD_FORMAT("[Error] DrawDropItem:sprite exceed Max : id=%d, spriteid=%d", pItem->GetID(), Frame.GetSpriteID());
			return;
		}
#endif

		CIndexSprite* pSprite = &m_ItemDropISPK[Frame.GetSpriteID()];
		//CSprite* pSprite = &m_ItemTileSPK[ Frame.GetSpriteID() ];

		// ???
		int colorSet = pItem->GetItemOptionColorSet();

		//		if(pItem->IsQuestItem())
		//			colorSet = MItem::GetQuestItemColorset();
		//		else
		//		if(pItem->IsUniqueItem())
		//			colorSet = MItem::GetUniqueItemColorset();

		if (pItem->IsSpecialColorItem())
			colorSet = pItem->GetSpecialColorItemColorset();
		else if (colorSet >= OPTIONEXT_ITEM_COLOR)  // 2006.3.21
			colorSet = pItem->GetItemOptionExtColorSet(colorSet);

		CIndexSprite::SetUsingColorSet(colorSet, colorSet);

		m_pSurface->BltIndexSprite(&pointTemp, pSprite);
	}
	//---------------------------------------------------------
	// ???? ??? item
	//---------------------------------------------------------
	// m_ItemTileFPK?? m_ItemTileSPK?? ???
	//---------------------------------------------------------
	else
	{
		//#ifdef OUTPUT_DEBUG
			//	DEBUG_ADD_FORMAT("[TempDebug] Before Draw Item : %d", pItem->GetID());
		//#endif	

		int tileFrameID = pItem->GetTileFrameID();
		//#ifdef OUTPUT_DEBUG
		if (tileFrameID >= m_ItemTileFPK.GetSize())
		{
			DEBUG_ADD_FORMAT("[Error] DrawItem:frame exceed Max : id=%d, frame=%d", pItem->GetID(), tileFrameID);
			return;
		}
		//#endif

		CFrame& Frame = m_ItemTileFPK[tileFrameID];

		/*
		DEBUG_ADD_FORMAT("class=%d, type=%d, frame=%d, id=%d", pItem->GetItemClass(),
			pItem->GetItemType(),
			pItem->GetTileFrameID(),
			Frame.GetSpriteID());
		*/

		// ??? ????
		POINT pointTemp;
		pointTemp.x = pPoint->x + Frame.GetCX();
		pointTemp.y = pPoint->y + Frame.GetCY();

#ifdef OUTPUT_DEBUG
		if (Frame.GetSpriteID() >= m_ItemTileISPK.GetSize())
		{
			DEBUG_ADD_FORMAT("[Error] DrawItem:sprite exceed Max : id=%d, spriteid=%d", pItem->GetID(), Frame.GetSpriteID());
			return;
		}
#endif

		CIndexSprite* pSprite = &m_ItemTileISPK[Frame.GetSpriteID()];

		int colorSet = pItem->GetItemOptionColorSet();

		//		if(pItem->IsQuestItem())
		//			colorSet = MItem::GetQuestItemColorset();
		//		else
		//		if(pItem->IsUniqueItem())
		//			colorSet = MItem::GetUniqueItemColorset();

		if (pItem->IsSpecialColorItem())
			colorSet = pItem->GetSpecialColorItemColorset();
		else if (colorSet >= OPTIONEXT_ITEM_COLOR)  // 2006.3.21
			colorSet = pItem->GetItemOptionExtColorSet(colorSet);

		CIndexSprite::SetUsingColorSet(colorSet, colorSet);

		//---------------------------------------- 		
		// Item ???? ???? ???? ????
		// ???????? ???? ???? ??? ???????.
		//---------------------------------------- 	
		RECT rect;
		rect.left = max(pointTemp.x, pPoint->x);
		rect.top = max(pointTemp.y, pPoint->y);
		rect.right = min(pointTemp.x + pSprite->GetWidth(), pPoint->x + TILE_X);
		rect.bottom = min(pointTemp.y + pSprite->GetHeight(), pPoint->y + TILE_Y);

		pItem->SetScreenRect(&rect);

		// m_SpritePack???? ?????? sprite?? ??? ????????.

		//-------------------------------------------------------
		// Mouse?? ??????? ??? Item
		//-------------------------------------------------------
		if (m_SelectItemID == pItem->GetID())
		{
			m_SOMOutlineColor = m_ColorOutlineItem;

			m_SOM.Add(pointTemp.x, pointTemp.y, pSprite);

			m_SOM.Generate();

			//if (CDirect3D::IsHAL() || DarkBits==0)
			//{
			m_pSurface->BltSpriteOutline(&m_SOM, m_SOMOutlineColor);
			//}
			//else
			//{
				//m_pSurface->BltSpriteOutlineDarkness( &m_SOM, m_SOMOutlineColor, DarkBits );
			//}

			//-------------------------------------
			// mouse?? ??????? Item?? ???? ???
			//-------------------------------------
			/*
			const char* pItemName = pItem->GetName();

			// option?? ??????? ?????..
			COLORREF nameColor;
			if (pItem->GetItemOption()==0)
			{
				nameColor = m_ColorNameItem;
			}
			else
			{
				nameColor = m_ColorNameItemOption;
			}

			if (pItemName != NULL)
			{
				FONTID	font = FONTID_ITEM;

				int x = pointTemp.x + (pSprite->GetWidth()>>1);
				int y = pointTemp.y - g_pClientConfig->FONT_ITEM_HEIGHT;

				// ??? ??? ????? ????????..
				if (x<0) x=0;
				if (y<0) y=0;

				//--------------------------------------------------
				// GetStringWidth?????? unlock??????.
				//--------------------------------------------------
				m_pSurface->Unlock();
				int x2 = x + g_GetStringWidth(pItemName, g_ClientPrintInfo[font]->hfont);
				m_pSurface->Lock();

				int y2 = y + g_pClientConfig->FONT_ITEM_HEIGHT;

				if (x2 >= CLIPSURFACE_WIDTH)
				{
					x -= x2-CLIPSURFACE_WIDTH;
					x2 = CLIPSURFACE_WIDTH;
				}

				DRAWTEXT_NODE* pNode = new DRAWTEXT_NODE(
											x,
											y+1,
											pItemName,
											nameColor,
											font
				);


				// ?????? ???
				RECT rect = {
					x,
					y,
					x2,
					y2
				};

				pNode->SetBox( rect, m_ColorBlackHalf );

				AddText( pNode );
			}
			*/
			//gC_font.PrintStringNoConvert(&m_SurfaceInfo, 
			//							g_ItemTable[pItem->GetItemClass()][pItem->GetItemType()].GetName(), 
			//							pointTemp.x, pointTemp.y, 
			//							0xFFFF
			//);

		}
		else
		{
			//-------------------------------------------------------
			// ???? ???? Item???
			//-------------------------------------------------------
			//if (g_pPlayer->IsTraceItem() && g_pPlayer->GetTraceID()==pItem->GetID())
			{
				// RED
			//	m_pSurface->BltSpriteColor(&pointTemp, &m_ItemTileSPK[ Frame.GetSpriteID() ], 0);
			}
			//-------------------------------------------------------
			// ???? ???
			//-------------------------------------------------------
			//else 
			{
				//if (CDirect3D::IsHAL() || DarkBits==0)
				//{			

				//-------------------------------------------------------
				// ????? ???????..
				//-------------------------------------------------------
				if (pItem->GetItemClass() == ITEM_CLASS_MINE
					&& ((MMine*)pItem)->IsInstalled())
				{
					// ?????????? ??????.
					m_pSurface->BltIndexSpriteColor(&pointTemp, pSprite, 0);
				}
				//-------------------------------------------------------	
				// ??????? ??????
				//-------------------------------------------------------
				else
				{
#if __CONTENTS(__TUNING_ITEM)
					if (pItem->IsTuningItem())
					{
						m_pSurface->BltIndexSpriteColorSet(&pointTemp, pSprite, MItem::GetTuningItemColorSet());
					}
					else
#endif //__TUNING_ITEM
					{
						m_pSurface->BltIndexSprite(&pointTemp, pSprite);
					}
				}
				//}
				//else
				//{
					//m_pSurface->BltSpriteDarkness(&pointTemp, &m_ItemTileSPK[ Frame.GetSpriteID() ], DarkBits);
				//}
			}
		}

		// [ TEST CODE ]
		//-------------------------------------
		// mouse?? ??????? Item?? ???? ???
		//-------------------------------------
		/*
		DRAWTEXT_NODE* pNode = new DRAWTEXT_NODE(
									pointTemp.x + (m_ItemTileSPK[ Frame.GetSpriteID() ].GetWidth()>>1),
									pointTemp.y - 20,
									pItem->GetName(),
									0xFFFF
		);

		AddText( pNode );
		*/
	}

	//--------------------------------------------------------------------
	// ItemNameList?? ???
	//--------------------------------------------------------------------
	if (m_bDrawItemNameList)
	{
		DWORD distance = max(abs(g_pPlayer->GetX() - pItem->GetX()),
			abs(g_pPlayer->GetY() - pItem->GetY()));

		AddItemName(new DRAWITEMNAME_NODE(distance, pItem));
	}

	//#ifdef OUTPUT_DEBUG
		//	DEBUG_ADD("[TempDebug] After Draw Item");
	//#endif	

	__END_PROFILE("DrawItem")

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("End DrawItem");
#endif


}


//----------------------------------------------------------------------
// Draw ItemShadow : Item ????? ??????	
//----------------------------------------------------------------------
// pSurface?? pPoint?? pItem?? ??????.
//----------------------------------------------------------------------
void
MTopView::DrawItemShadow(POINT* pPoint, MItem* pItem)
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawItemShadow");
#endif

	__BEGIN_PROFILE("DrawItemShadow")

		//------------------------------------------------
		// Slayer?? ??? Darkness ???? ?? ?? ????.
		//------------------------------------------------
		if (g_pZone->GetSector(pItem->GetX(), pItem->GetY()).HasDarkness()
			&& IsAffectFromDarkness())
		{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
			DEBUG_ADD("End DrawItemShadow(D)");
#endif

			return;
		}

	//---------------------------------------------------------
	// ??ü?? ???? Creature?? ??????.
	//---------------------------------------------------------
	if (pItem->GetItemClass() == ITEM_CLASS_CORPSE)
	{
		MCorpse* pCorpse = (MCorpse*)pItem;

		MCreature* pCreature = pCorpse->GetCreature();

		if (pCreature != NULL)
		{
			//---------------------------------------------------------
			// ??? ????? ???? ???..
			//---------------------------------------------------------
			{
				// ???õ? ???? ???
				// Creature?? ?????? ??ó????? ???.. ????. - -;
				if (m_SelectItemID == pItem->GetID())
				{
					TYPE_OBJECTID temp = m_SelectCreatureID;
					m_SelectCreatureID = pCreature->GetID();

					if (
						(
							pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376	// ????
							|| pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563	// ????
							|| pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549	// ????
							|| pCreature->GetCreatureType() == 482	// ????????? ???
							|| pCreature->GetCreatureType() == 650	// ?????
							|| pCreature->GetCreatureType() == 819	// ?????? ?????
							|| pCreature->GetCreatureType() == 672
							|| pCreature->GetCreatureType() == 673
							|| pCreature->GetCreatureType() == 730 // ???? ?????1
							|| pCreature->GetCreatureType() == 731 // ???? ?????2
							|| pCreature->GetCreatureType() == 732 // ???? ?????3
							)
						||
						(
							pCreature->GetActionCount() < pCreature->GetActionCountMax()
							// ??????? ????? ????????ü ????? ??????????
							|| g_pUserInformation->GoreLevel == false
							&& pCreature->GetActionCount() >= pCreature->GetActionCountMax() / 2
							&& pCreature->IsDead()
							)
						)
					{
						POINT pointTemp = *pPoint;
						DrawCreatureShadow(&pointTemp, pCreature);
					}

					m_SelectCreatureID = OBJECTID_NULL;
				}
				else
				{
					if (
						(
							pCreature->GetCreatureType() >= 371 && pCreature->GetCreatureType() <= 376	// ????
							|| pCreature->GetCreatureType() >= 560 && pCreature->GetCreatureType() <= 563	// ????
							|| pCreature->GetCreatureType() >= 526 && pCreature->GetCreatureType() <= 549	// ????
							|| pCreature->GetCreatureType() == 482	// ????????? ???
							|| pCreature->GetCreatureType() == 650  // ?????
							|| pCreature->GetCreatureType() == 819	// ?????? ?????
							|| pCreature->GetCreatureType() == 672
							|| pCreature->GetCreatureType() == 673
							|| pCreature->GetCreatureType() == 730 // ???? ?????1
							|| pCreature->GetCreatureType() == 731 // ???? ?????2
							|| pCreature->GetCreatureType() == 732 // ???? ?????3
							)
						||
						(
							pCreature->GetActionCount() < pCreature->GetActionCountMax()
							// ??????? ????? ????????ü ????? ??????????
							|| g_pUserInformation->GoreLevel == false
							&& pCreature->GetActionCount() >= pCreature->GetActionCountMax() / 2
							&& pCreature->IsDead()
							)
						)
					{
						POINT pointTemp = *pPoint;
						DrawCreatureShadow(&pointTemp, pCreature);
					}
				}
			}

		}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("End DrawItemShadow");
#endif



		return;
	}


	__END_PROFILE("DrawItemShadow")

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("End DrawItemShadow");
#endif


}

//----------------------------------------------------------------------
// Draw ImageObject : ??? ??????	
//----------------------------------------------------------------------
// pSurface?? pPoint?? pImageObject?? ??????.
//----------------------------------------------------------------
void
MTopView::DrawImageObject(POINT* pPoint, MImageObject* pImageObject)
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawImageObject");
#endif

	__BEGIN_PROFILE("DrawImageObject")

#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
		if (g_pDXInput->KeyDown(DIK_T) && g_pDXInput->KeyDown(DIK_LCONTROL))
		{
			return;
		}
#endif

	//#ifdef OUTPUT_DEBUG
		//	DEBUG_ADD_FORMAT("[TempDebug] Before Draw ImageObject : %d", pImageObject->GetID());
	//#endif	

	// m_SpritePack???? ?????? sprite?? ??? ????????.
	// Player?? ?????? ????? ??? ?????? ó??

	//if (pImageObject->GetImageObjectID() == m_BehindImageObjectID)

	//m_pSurface->HLine(410,310, 1, 0xFFFF);
	// (410,310)?? Player?? ??? ???? ?????..???..
	// (g_x,g_y)?? Mouse????? ????????....

	//------------------------------------------------------
	// 1. ImageObject?? ?????? ??? ??????
	// 2. Viewpoint?? Player??????? ?????? ??°??
	// 3. Sprite?? Player?? ?????°??
	//------------------------------------------------------
	// 1,2,3?? ??? ?????? ???.. ?????? ó?????.
	//------------------------------------------------------
	bool bTrans;

	// player???
	//POINT m_FilterPosition;// = MapToPixel(g_pPlayer->GetX(), g_pPlayer->GetY());
	//m_FilterPosition.x -= m_FirstZonePixel.x-m_PlusPoint.x;
	//m_FilterPosition.y -= m_FirstZonePixel.y-m_PlusPoint.y;

	// ????? ????? ???? ????? ????? ?????								
	//m_FilterPosition.x = g_pPlayer->GetPixelX() - m_FirstZonePixel.x - filter.GetWidth();
	//m_FilterPosition.y = g_pPlayer->GetPixelY() - m_FirstZonePixel.y - filter.GetHeight();
	//m_FilterPosition.x = 310;	// 410 - 100;
	//m_FilterPosition.y = 116;	// 204 - 100;

	//------------------------------------------------------
	// ??ü?????? ???? ???? ???????.
	// ??, ImageObject?? ???(Zone?????? Sector???)?? 
	// ???? Tile???(Light?? 0?? ??? ???) DarkBits=0???? ???.
	//------------------------------------------------------
	//BYTE DarkBits;

	//------------------------------------------------------
	// [ TEST CODE ]
	//------------------------------------------------------
		// ???? ?????? code???.
		// MapEditor???? ?? ???????? ???.
	/*
	if (pImageObject->GetX()!=SECTORPOSITION_NULL && pImageObject->GetY()!=SECTORPOSITION_NULL
		&&
		m_pZone->GetSector(pImageObject->GetX(), pImageObject->GetY()).GetLight()!=0)
	{
		DarkBits = 0;
	}
	else
	{
		DarkBits = m_DarkBits;
	}

	// 3D ?????? ??? ???.. ?????? ???? ?????? ???? ??????.
	if (CDirect3D::IsHAL())
	{
		DarkBits = 0;
	}
	*/

	//----------------------------------------------------				
	// ????? ?????? ??????
	//----------------------------------------------------				
	BOOL bBlendingShadow = 1;//g_pUserOption->BlendingShadow;

	//------------------------------------------------------
	//
	//  ShadowObject ???
	//
	//------------------------------------------------------
	if (pImageObject->GetObjectType() == MObject::TYPE_SHADOWOBJECT)
	{
		int spriteID = pImageObject->GetSpriteID();

		if (spriteID != SPRITEID_NULL)
		{
			//			if (CDirect3D::IsHAL())
			//			{			
			//				if (bBlendingShadow)
			//				{
			//					DRAW_TEXTURE_SPRITE_LOCKED(pPoint->x, pPoint->y, 
			//												spriteID, m_pImageObjectShadowManager)
			//
			//
			//					/*
			//					#ifdef OUTPUT_DEBUG
			//						if (g_pDXInput->KeyDown(DIK_S) && 
			//							(g_pDXInput->KeyDown(DIK_RCONTROL)))
			//						{
			//							char str[128];
			//							sprintf(str, "iid=%d, view=%d", (int)pImageObject->GetImageObjectID(), (int)pImageObject->GetViewpoint());
			//							m_pSurface->GDI_Text(pPoint->x, pPoint->y, str, 0xFFFF);
			//						}
			//					#endif
			//					*/
			//				}
			//				else
			//				{
			//					m_pSurface->BltShadowSprite( pPoint, &(m_pImageObjectShadowManager->GetSprite(spriteID)) );
			//
			//					#ifdef OUTPUT_DEBUG
			//						if (g_pDXInput->KeyDown(DIK_S) && 
			//							(g_pDXInput->KeyDown(DIK_RCONTROL)))
			//						{
			//							m_pSurface->Unlock();
			//					
			//							char str[128];
			//							sprintf(str, "iid=%d, view=%d", (int)pImageObject->GetImageObjectID(), (int)pImageObject->GetViewpoint());
			//							m_pSurface->GDI_Text(pPoint->x, pPoint->y, str, 0xFFFF);
			//							
			//							m_pSurface->Lock();
			//						}
			//					#endif
			//				}
			//			}
			//			else
			{
				if (bBlendingShadow)
				{
					m_pSurface->BltShadowSpriteDarkness(pPoint, &m_ImageObjectSSPK[spriteID], 1);
				}
				else
				{
					m_pSurface->BltShadowSprite(pPoint, &m_ImageObjectSSPK[spriteID]);
				}

#ifdef OUTPUT_DEBUG
				if (g_pDXInput->KeyDown(DIK_S) &&
					(g_pDXInput->KeyDown(DIK_RCONTROL)))
				{
					m_pSurface->Unlock();

					char str[128];
					sprintf(str, "iid=%d, view=%d", (int)pImageObject->GetImageObjectID(), (int)pImageObject->GetViewpoint());
					m_pSurface->GDI_Text(pPoint->x, pPoint->y, str, 0xFFFF);

					m_pSurface->Lock();
				}
#endif
			}
		}
	}
	//------------------------------------------------------
	//
	//          ShadowAnimationObject ???
	//
	//------------------------------------------------------
	else if (pImageObject->GetObjectType() == MObject::TYPE_SHADOWANIMATIONOBJECT)
	{
	}
	//------------------------------------------------------
	//
	//          InteractionObject ???
	//
	//------------------------------------------------------
//	else if (pImageObject->GetObjectType()==MObject::TYPE_INTERACTIONOBJECT)
//	{
//		MInteractionObject* pInteractionObject = (MInteractionObject*)pImageObject;
//
//		TYPE_SPRITEID sprite;
//	
//		switch (pInteractionObject->GetBltType())
//		{
//			//--------------------------------
//			// Blt Normal
//			//--------------------------------
//			case BLT_NORMAL :
//			{
//				CFrame& Frame = m_InteractionObjectFPK[ pInteractionObject->GetFrameID() ][ pInteractionObject->GetFrame() ];
//				sprite = Frame.GetSpriteID();
//				
//				/*
//				#ifdef	OUTPUT_DEBUG
//					sprintf(g_pDebugMessage->GetCurrent(), 
//							"[%d] FrameID=%d, Frame=%d, sprite=%d, xy=(%d,%d)", 
//							pInteractionObject->GetID(), pInteractionObject->GetFrameID(), pInteractionObject->GetFrame(), sprite, pInteractionObject->GetX(), pInteractionObject->GetY()
//					);								
//					g_pDebugMessage->Next();					
//				#endif
//				*/
//
//				// ??? ????
//				pPoint->x += Frame.GetCX();
//				pPoint->y += Frame.GetCY();
//
//				CSprite* pSprite = &m_InteractionObjectSPK[ sprite ];		
//				
//				//---------------------------------------- 		
//				// InteractionObject?? ???? ???? ???? ????
//				//---------------------------------------- 	
//				RECT rect;
//				rect.left	= pPoint->x;
//				rect.top	= pPoint->y;
//				rect.right	= rect.left + pSprite->GetWidth();
//				rect.bottom = rect.top + pSprite->GetHeight();
//				pInteractionObject->SetScreenRect( &rect );
//
//
//				// ?????? check				
//				bTrans = pImageObject->IsTrans();
//
//				//--------------------------------
//				// ???õ? InteractionObject?? ???...
//				//--------------------------------
//				if (m_SelectInteractionObjectID==pInteractionObject->GetID())
//				{			
//					//if (m_ImageObjectSPK[ sprite ].IsInit())
//					{
//						m_SOMOutlineColor = m_ColorOutlineInteractionObject;						
//
//						m_SOM.Add( pPoint->x, pPoint->y, pSprite );
//						
//						m_SOM.Generate();
//
//						//if (DarkBits==0)
//						//{
//							m_pSurface->BltSpriteOutline( &m_SOM, m_SOMOutlineColor );
//						//}
//						//else
//						//{
//							//m_pSurface->BltSpriteOutlineDarkness( &m_SOM, m_SOMOutlineColor, DarkBits );
//						//}				
//					}
//				}
//				//--------------------------------
//				// ???? ó??
//				//--------------------------------
//				else if (bTrans)
//				{
//					// ??????
//					//m_pSurface->BltSpriteHalf(pPoint, &m_ImageObjectSPK[ sprite ]);
//					// ????? ???? filter ??? ????
//					CSprite::SetFilter(m_FilterPosition.x - pPoint->x,
//										m_FilterPosition.y - pPoint->y,
//										&m_ImageObjectFilter);
//
//					//if (m_ImageObjectSPK[ sprite ].IsIntersectFilter() 
//					//	|| DarkBits==0)
//					//{
//						m_pSurface->BltSpriteAlphaFilter(pPoint, pSprite);
//					//}
//					//else
//					//{
//					//	m_pSurface->BltSpriteAlphaFilterDarkness(pPoint, 
//														//&m_ImageObjectSPK[ sprite ], DarkBits);
//					//}					
//				}
//				//--------------------------------
//				// ??? ???
//				//--------------------------------
//				else
//				{
//					//if (m_ImageObjectSPK[ sprite ].IsInit())
//					{
//						///*
//						// Player?? ?þ??? ??? ??????
//						// ?þ? ??? ??? ImageObject?? ????? ???.
//						//CSprite& spriteRef = m_ImageObjectSPK[ sprite ];
//
//						//int sight = g_pPlayer->GetMaxLightSight();
//						//int pX = sight * TILE_X;
//						//int pY = sight * TILE_Y;
//						//int sX = 410-(pX>>1) - pPoint->x;
//						//int sY = 270-(pY>>1) - pPoint->y;			
//
//						//if (DarkBits==0 //|| m_ImageObjectSPK[ pImageObject->GetSpriteID() ].IsIntersectFilter())
//							// ??? ???????(s_X,s_Y)?? Sprite?? ????????? ???? ???
//						//	|| (sX < spriteRef.GetWidth() && sY < spriteRef.GetHeight()
//							// Filter?? ?????? Sprite ù?? ???? ? ???
//						//	 &&	sX,pX > 0 && sY,pY > 0))
//						//{
//							m_pSurface->BltSprite(pPoint, pSprite);
//						//}
//						//else
//						//{
//						//	m_pSurface->BltSpriteDarkness(pPoint, &m_ImageObjectSPK[ sprite ], DarkBits);
//						//}
//					}
//				}
//			}
//			break;
//
//			//--------------------------------
//			// Blt Channel 
//			//--------------------------------
//			case BLT_EFFECT :	
//				/*
//				//--------------------------------
//				// ???õ? InteractionObject?? ???...
//				//--------------------------------
//				if (m_SelectInteractionObjectID==pInteractionObject->GetID())
//				{
//					//DrawSpriteOutline(pPoint, &m_ImageObjectSPK[ sprite ], 0xFFFF);
//				}
//				else
//				{				
//					m_pSurface->BltAlphaSprite(pPoint, &m_EffectAlphaSPK[ sprite ]);
//				}
//				*/
//			break;
//		}
//
//		if (g_bFrameChanged)
//		{
//			pInteractionObject->NextFrame();
//		}
//	}	
	else
	{
		//------------------------------------------------------
		//
		//          ImageObject : Sprite ???
		//
		//------------------------------------------------------
		int spriteID = pImageObject->GetSpriteID();
		if (spriteID != SPRITEID_NULL)
		{
			//---------------------------------------
			// ID?? sprite?? ImageObject?? Load???.
			//---------------------------------------
//			#ifdef	OUTPUT_DEBUG
//				char str[256];
//			#endif
//
//			if (m_ImageObjectSPK[ spriteID ].IsNotInit())
//			{
//				#ifdef	OUTPUT_DEBUG
//					sprintf(str, "[RunTimeLoading] ImageObject : sprite=%d. fp=%ld. (%d x %d)", spriteID, m_ImageObjectSPKI[spriteID], m_ImageObjectSPK[spriteID].GetWidth(), m_ImageObjectSPK[spriteID].GetHeight());
//				#endif				
//
//				m_ImageObjectSPKFile.seekg(m_ImageObjectSPKI[spriteID], ios::beg);				
//				//--------------------------------------------------
//				// Loading?? ?????? ???
//				//--------------------------------------------------
//				if (m_ImageObjectSPK[ spriteID ].LoadFromFile( m_ImageObjectSPKFile ))
//				{
//					#ifdef	OUTPUT_DEBUG
//						sprintf(str, "%s ...OK (%d x %d)", str, m_ImageObjectSPK[spriteID].GetWidth(), m_ImageObjectSPK[spriteID].GetHeight());
//
//						DEBUG_ADD( str );
//					#endif
//				}
//				//--------------------------------------------------
//				// ?????? ??? --> ??? Loading??? ??? ??????.				
//				//--------------------------------------------------
//				/*
//				// 2001.8.20 ???ó??
//				else
//				{
//					#ifdef	OUTPUT_DEBUG
//						strcat(str, " ...Fail & Wait Loading");
//	
//						DEBUG_ADD( str );
//					#endif
//
//					// file thread ?????? ??????.
//					//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_HIGHEST);
//					MLoadingSPKWorkNode3* pNode = new MLoadingSPKWorkNode3(spriteID, m_ImageObjectSPKI[spriteID]);
//					pNode->SetSPK( &m_ImageObjectSPK, FILE_SPRITE_IMAGEOBJECT );
//					pNode->SetType( 1 );
//					g_pLoadingThread->SetPriority( THREAD_PRIORITY_HIGHEST );
//					g_pLoadingThread->AddFirst( pNode );
//
//					// Thread???? Loading?? ?????????? ??????.
//					//while (m_ImageObjectSPK[ spriteID ].IsNotInit());
//					while (1)
//					{
//						DEBUG_ADD_FORMAT( "Check Load id=%d", spriteID );
//
//						if (m_ImageObjectSPK[spriteID].IsInit())
//						{
//							DEBUG_ADD( "Is Init" );	
//						
//							break;
//						}
//						else
//						{
//							DEBUG_ADD( "Is Not Init" );
//						}
//					}
//					//while (!m_ImageObjectSPK[spriteID].LoadFromFile( m_ImageObjectSPKFile ));
//
//					// file thread ?????? ?????.
//					//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_BELOW_NORMAL);
//					g_pLoadingThread->SetPriority( THREAD_PRIORITY_LOWEST );
//
//				}
//				*/
//			}

			bTrans = pImageObject->IsTrans()
				&& pImageObject->GetViewpoint() >= g_pPlayer->GetY()
				// 2001.9.5 ???
				&& pImageObject->IsWallTransPosition(g_pPlayer->GetX(), g_pPlayer->GetY());


			//&& m_ImageObjectSPK[ spriteID ].IsColorPixel(
			//	410 - pPoint->x,
			//	310 - pPoint->y);

//--------------------------------
// ???? ó??
//--------------------------------
			if (bTrans)
			{
				///*
				//m_pSurface->BltSpriteHalf(pPoint, &m_ImageObjectSPK[ spriteID ]);


				// ????? ???? filter ??? ????
				CSprite::SetFilter(m_FilterPosition.x - pPoint->x,
					m_FilterPosition.y - pPoint->y,
					&m_ImageObjectFilter);

				if (m_ImageObjectSPK[spriteID].IsIntersectFilter())
				{
					//if (DarkBits==0)
					//{
#ifdef __3D_IMAGE_OBJECT__
					if (CDirect3D::IsHAL())
					{
						m_pSurface->Unlock();

						// Texture ????
						CSpriteSurface* pSurface = m_pImageObjectTextureManager->GetTexture(spriteID);

						if (pSurface != NULL)
						{
							CDirect3D::GetDevice()->SetTexture(0, pSurface->GetSurface());

							CDirect3D::GetDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
							CDirect3D::GetDevice()->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
							CDirect3D::GetDevice()->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);

							// ??? ????
							//						m_SpriteVertices[0].sx = pPoint->x;
							//						m_SpriteVertices[0].sy = pPoint->y;
							//						m_SpriteVertices[1].sx = pPoint->x,m_pImageObjectTextureManager->GetWidth(spriteID);
							//						m_SpriteVertices[1].sy = pPoint->y;
							//						m_SpriteVertices[2].sx = pPoint->x;
							//						m_SpriteVertices[2].sy = pPoint->y,m_pImageObjectTextureManager->GetHeight(spriteID);
							//						m_SpriteVertices[3].sx = m_SpriteVertices[1].sx;
							//						m_SpriteVertices[3].sy = m_SpriteVertices[2].sy;		

							CDirect3D::GetDevice()->BeginScene();

							RECT rect;

							rect.left = pPoint->x;
							rect.top = pPoint->y;
							rect.right = rect.left + m_pImageObjectTextureManager->GetWidth(spriteID);
							rect.bottom = rect.top + m_pImageObjectTextureManager->GetHeight(spriteID);

							m_TextureEffect.DrawEffect2D(&rect);

							CDirect3D::GetDevice()->EndScene();
							CDirect3D::GetDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
							CDirect3D::GetDevice()->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
							CDirect3D::GetDevice()->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
							m_pSurface->Lock();
						}
					}
#else
#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
					if (g_pDXInput->KeyDown(DIK_A) &&
						(g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
					{
						// CTRL+A?????? ???? ?? ??´?.
					}
					else
#endif
					{
						m_pSurface->BltSpriteAlphaFilter(pPoint,
							&m_ImageObjectSPK[spriteID]);
					}
#endif
					//}
					//else
					//{
					//	m_pSurface->BltSpriteAlphaFilterDarkness(pPoint, 
					//								&m_ImageObjectSPK[ spriteID ], DarkBits);
					//}	

					// Texture????? ?????? ???... 
					/*
					if (CDirect3D::IsHAL())
					{
						m_pSurface->Unlock();

						// Texture ????
						CSpriteSurface* pSurface = m_pImageObjectTextureManager->GetTexture(spriteID);
						CDirect3D::GetDevice()->SetTexture(0, pSurface->GetSurface());

						// ??? ????
						m_SpriteVertices[0].sx = pPoint->x;
						m_SpriteVertices[0].sy = pPoint->y;
						m_SpriteVertices[1].sx = pPoint->x,m_pImageObjectTextureManager->GetWidth(spriteID);
						m_SpriteVertices[1].sy = pPoint->y;
						m_SpriteVertices[2].sx = pPoint->x;
						m_SpriteVertices[2].sy = pPoint->y,m_pImageObjectTextureManager->GetHeight(spriteID);
						m_SpriteVertices[3].sx = m_SpriteVertices[1].sx;
						m_SpriteVertices[3].sy = m_SpriteVertices[2].sy;

						// ???
						CDirect3D::GetDevice()->DrawPrimitive(D3DPT_TRIANGLESTRIP,
												D3DFVF_TLVERTEX,
												m_SpriteVertices,
												4, 0);

						//CDirect3D::GetDevice()->EndScene();
						m_pSurface->Lock();
					}
					else
					{
						m_pSurface->BltSpriteAlphaFilter(pPoint,
													&m_ImageObjectSPK[ spriteID ]);
					}
					*/
				}
				else
				{
					//if (DarkBits==0)
					//{
#ifdef __3D_IMAGE_OBJECT__
					if (CDirect3D::IsHAL())
					{
						m_pSurface->Unlock();

						// Texture ????
						CSpriteSurface* pSurface = m_pImageObjectTextureManager->GetTexture(spriteID);

						if (pSurface != NULL)
						{
							CDirect3D::GetDevice()->SetTexture(0, pSurface->GetSurface());

							// ??? ????
							//						m_SpriteVertices[0].sx = pPoint->x;
							//						m_SpriteVertices[0].sy = pPoint->y;
							//						m_SpriteVertices[1].sx = pPoint->x,m_pImageObjectTextureManager->GetWidth(spriteID);
							//						m_SpriteVertices[1].sy = pPoint->y;
							//						m_SpriteVertices[2].sx = pPoint->x;
							//						m_SpriteVertices[2].sy = pPoint->y,m_pImageObjectTextureManager->GetHeight(spriteID);
							//						m_SpriteVertices[3].sx = m_SpriteVertices[1].sx;
							//						m_SpriteVertices[3].sy = m_SpriteVertices[2].sy;		

							CDirect3D::GetDevice()->BeginScene();

							RECT rect;

							rect.left = pPoint->x;
							rect.top = pPoint->y;
							rect.right = rect.left + m_pImageObjectTextureManager->GetWidth(spriteID);
							rect.bottom = rect.top + m_pImageObjectTextureManager->GetHeight(spriteID);

							m_TextureEffect.DrawEffect2D(&rect);

							CDirect3D::GetDevice()->EndScene();
							m_pSurface->Lock();
						}
					}
#else
					if (pImageObject->IsHalf())
						m_pSurface->BltSpriteHalf(pPoint, &m_ImageObjectSPK[spriteID]);
					else
						m_pSurface->BltSprite(pPoint, &m_ImageObjectSPK[spriteID]);
#endif
					//}
					//else 
					//{
						//m_pSurface->BltSpriteDarkness(pPoint, &m_ImageObjectSPK[ spriteID ], DarkBits);		
					//}
				}
				//*/

				//m_pSurface->BltSpriteHalf(pPoint, &m_ImageObjectSPK[ spriteID ]);
				//m_pSurface->BltSprite(pPoint, &m_ImageObjectOutlineSPK[ 0 ]);
			}
			//--------------------------------
			// ??? ???
			//--------------------------------
			else
			{
				//if (m_ImageObjectSPK[ spriteID ].IsInit())
				{
					///*
					// Player?? ?þ??? ??? ??????
					// ?þ? ??? ??? ImageObject?? ????? ???.
					//CSprite& spriteRef = m_ImageObjectSPK[ spriteID ];

					//int sight = g_pPlayer->GetMaxLightSight();
					//int pX = sight * TILE_X;
					//int pY = sight * TILE_Y;
					//int sX = 410-(pX>>1) - pPoint->x;
					//int sY = 270-(pY>>1) - pPoint->y;			

					//if (DarkBits==0 //|| m_ImageObjectSPK[ spriteID ].IsIntersectFilter())
						// ??? ???????(s_X,s_Y)?? Sprite?? ????????? ???? ???
					//	|| (sX < spriteRef.GetWidth() && sY < spriteRef.GetHeight()
						// Filter?? ?????? Sprite ù?? ???? ? ???
					//	 &&	sX,pX > 0 && sY,pY > 0))

					//{
#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
					if (g_pDXInput->KeyDown(DIK_A) &&
						(g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
					{
						// CTRL+A?????? ???? ?? ??´?.
					}
					else
#endif
					{
						if (pImageObject->IsHalf())
							m_pSurface->BltSpriteHalf(pPoint, &m_ImageObjectSPK[spriteID]);
						else
							m_pSurface->BltSprite(pPoint, &m_ImageObjectSPK[spriteID]);
						/*
						static int gray = 0;
						static int g = 1;

						CSpriteSurface::s_Value1 = gray;

						CSpriteSurface::SetEffect( CSpriteSurface::EFFECT_GRAY_SCALE_VARIOUS );

						m_pSurface->BltSpriteEffect(pPoint, &m_ImageObjectSPK[ spriteID ]);

						static DWORD lastFrame = g_CurrentFrame;

						if (g_CurrentFrame - lastFrame > 2)
						{
							gray += g;

							if (gray == 32 || gray == 0)
							{
								g = -g;
							}

							lastFrame = g_CurrentFrame;
						}
						*/
					}

					/*
					#ifdef OUTPUT_DEBUG
						if (g_pDXInput->KeyDown(DIK_RCONTROL))
						{
							static int value = 0;
							static int step = 0;

							if (value==0)
							{
								m_pSurface->BltSprite(pPoint, &m_ImageObjectSPK[ spriteID ]);
							}
							else if (value==64)
							{
								CSpriteSurface::SetEffect( CSpriteSurface::EFFECT_SIMPLE_OUTLINE );

								m_pSurface->BltSpriteEffect(pPoint, &m_ImageObjectSPK[ spriteID ]);
							}
							else
							{
								CSpriteSurface::s_Value1 = value;

								CSpriteSurface::SetEffect( CSpriteSurface::EFFECT_WIPE_OUT );

								m_pSurface->BltSpriteEffect(pPoint, &m_ImageObjectSPK[ spriteID ]);
							}

							if (g_pDXInput->KeyDown(DIK_RSHIFT))
							{
								if (value==0 || value==64)
								{
									if (step==0)
									{
										if (value==0)
											step = 1;
										else
											step = -1;
									}
									else step *= -1;
								}
							}
							else
							{
								value += step;

								if (value<0) value = 0;
								else if (value>64) value=64;
							}
						}
						else
					#endif
						{
							m_pSurface->BltSprite(pPoint, &m_ImageObjectSPK[ spriteID ]);
						}
						*/

						//}
						//else 
						//{
							//m_pSurface->BltSpriteDarkness(pPoint, &m_ImageObjectSPK[ spriteID ], DarkBits);		
						//}
				}
			}

			if (g_pUserInformation->attrOperator.GetValue() &&
				g_pOperatorOption->bShowImageObjectInfo)
			{
				char str[128];
				sprintf(str, "iid=%d, sid=%d, vp=%d", (int)pImageObject->GetImageObjectID(), (int)spriteID, (int)pImageObject->GetViewpoint());
				//??? ?? ?? ???? ?? ??? ó?? ?? by diesirace
				//m_pSurface->Unlock();
				m_pSurface->GDI_Text(pPoint->x, pPoint->y, str, 0xFFFF);
				//m_pSurface->Lock();
			}
		}


		//------------------------------------------------------------
		//
		//     AnimationObject?? ??? Frame?? ???????? ???.
		//
		//------------------------------------------------------------
		if (pImageObject->IsAnimation())
		{
			MAnimationObject* pAnimationObject = (MAnimationObject*)pImageObject;

			TYPE_SPRITEID sprite;

			POINT pointTemp = *pPoint;

			/*******************????? ??? ????******************/
			if (pAnimationObject->GetFrameID() < m_ImageObjectShadowFPK.GetSize()
				&& pAnimationObject->GetFrame() < m_ImageObjectShadowFPK[pAnimationObject->GetFrameID()].GetSize())
			{
				CFrame& FrameShadow = m_ImageObjectShadowFPK[pAnimationObject->GetFrameID()][pAnimationObject->GetFrame()];
				sprite = FrameShadow.GetSpriteID();

				if (sprite != SPRITEID_NULL)
				{
					pointTemp.x += (FrameShadow.GetCX());
					pointTemp.y += (FrameShadow.GetCY());
					if (bBlendingShadow)
					{
						m_pSurface->BltShadowSpriteDarkness(&pointTemp, &m_ImageObjectSSPK[sprite], 1);
					}
					else
					{
						m_pSurface->BltShadowSprite(&pointTemp, &m_ImageObjectSSPK[sprite]);
					}
				}
			}
			/*******************????? ??? ??******************/

			switch (pAnimationObject->GetBltType())
			{
				//--------------------------------
				// Blt Normal
				//--------------------------------
			case BLT_NORMAL:
			{
				int currentFrame = g_CurrentFrame % m_ImageObjectFPK[pAnimationObject->GetFrameID()].GetSize();

				CFrame& Frame = m_ImageObjectFPK[pAnimationObject->GetFrameID()][currentFrame];
				sprite = Frame.GetSpriteID();
				if (sprite >= m_ImageObjectSPK.GetSize())
					break;
				//---------------------------------------
				// ID?? sprite?? ImageObject?? Load???.
				//---------------------------------------
//					if (m_ImageObjectSPK[ sprite ].IsNotInit())
//					{
//						#ifdef	OUTPUT_DEBUG
//							if (g_pDebugMessage!=NULL)
//								sprintf(g_pDebugMessage->GetCurrent(), "[RunTimeLoading] AnimationObject : sprite=%d", sprite);
//						#endif
//
//						m_ImageObjectSPKFile.seekg(m_ImageObjectSPKI[sprite], ios::beg);						
//						
//						//--------------------------------------------------
//						// Loading?? ?????? ???
//						//--------------------------------------------------
//						if (m_ImageObjectSPK[ sprite ].LoadFromFile( m_ImageObjectSPKFile ))
//						{
//							#ifdef	OUTPUT_DEBUG
//								if (g_pDebugMessage!=NULL)
//								{
//									sprintf(g_pDebugMessage->GetCurrent(), "%s ...OK", g_pDebugMessage->GetCurrent());
//									g_pDebugMessage->Next();
//								}
//							#endif
//						}
//						//--------------------------------------------------
//						// ?????? ??? --> ??? Loading??? ??? ??????.				
//						//--------------------------------------------------
//						
////						// 2001.8.20 ???ó??
////						else
////						{
////							#ifdef	OUTPUT_DEBUG
////								if (g_pDebugMessage!=NULL)
////								{
////									sprintf(g_pDebugMessage->GetCurrent(), "%s ...Fail & Wait Loading", g_pDebugMessage->GetCurrent());
////									g_pDebugMessage->Next();
////								}
////							#endif
////
////							// file thread ?????? ??????.
////							//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_HIGHEST);
////							MLoadingSPKWorkNode3* pNode = new MLoadingSPKWorkNode3(sprite, m_ImageObjectSPKI[sprite]);
////							pNode->SetSPK( &m_ImageObjectSPK, FILE_SPRITE_IMAGEOBJECT );
////							pNode->SetType( 1 );
////							g_pLoadingThread->SetPriority( THREAD_PRIORITY_HIGHEST );
////							g_pLoadingThread->AddFirst( pNode );
////
////							// Thread???? Loading?? ?????????? ??????.
////							//while (m_ImageObjectSPK[sprite].IsNotInit());
////							while (1)
////							{
////								DEBUG_ADD_FORMAT( "Check Load id=%d", sprite );
////
////								if (m_ImageObjectSPK[sprite].IsInit())
////								{
////									DEBUG_ADD( "Is Init" );
////								
////									break;
////								}
////								else
////								{
////									DEBUG_ADD( "Is Not Init" );
////								}
////							}
////							//while (!m_ImageObjectSPK[ sprite ].LoadFromFile( m_ImageObjectSPKFile ));
////
////							// file thread ?????? ?????.
////							//SetThreadPriority(g_hFileThread, THREAD_PRIORITY_BELOW_NORMAL);
////							g_pLoadingThread->SetPriority( THREAD_PRIORITY_LOWEST );
////
////						}
//						
//					}
					// ??? ????
				pPoint->x += Frame.GetCX();
				pPoint->y += Frame.GetCY();

				// spriteID?? ???? ???? ?????? ??? check				
				if (spriteID == SPRITEID_NULL)
				{
					bTrans = true;
					//	bTrans = pImageObject->IsTrans();
								//&& pImageObject->GetViewpoint() >= g_pPlayer->GetY();
								//&& m_ImageObjectSPK[ sprite ].IsColorPixel(
								//	410 - pPoint->x,
								//	310 - pPoint->y);
				}

				//--------------------------------
				// ???? ó??
				//--------------------------------
				if (bTrans)
				{
					// ??????
					//m_pSurface->BltSpriteHalf(pPoint, &m_ImageObjectSPK[ sprite ]);
					// ????? ???? filter ??? ????
					CSprite::SetFilter(m_FilterPosition.x - pPoint->x,
						m_FilterPosition.y - pPoint->y,
						&m_ImageObjectFilter);

					//if (m_ImageObjectSPK[ sprite ].IsIntersectFilter()
					//	|| DarkBits==0)
					//{
					m_pSurface->BltSpriteAlphaFilter(pPoint,
						&m_ImageObjectSPK[sprite]);
					//}
					//else
					//{
						//m_pSurface->BltSpriteAlphaFilterDarkness(pPoint, 
														//&m_ImageObjectSPK[ sprite ], DarkBits);
					//}

				}
				//--------------------------------
				// ??? ???
				//--------------------------------
				else
				{
					///*
					// Player?? ?þ??? ??? ??????
					// ?þ? ??? ??? ImageObject?? ????? ???.
					//if (m_ImageObjectSPK[ sprite ].IsInit())
					{
						//CSprite& spriteRef = m_ImageObjectSPK[ sprite ];

						//int sight = g_pPlayer->GetMaxLightSight();
						//int pX = sight * TILE_X;
						//int pY = sight * TILE_Y;
						//int sX = 410-(pX>>1) - pPoint->x;
						//int sY = 270-(pY>>1) - pPoint->y;			

						//if (DarkBits==0 //|| m_ImageObjectSPK[ spriteID ].IsIntersectFilter())
							// ??? ???????(s_X,s_Y)?? Sprite?? ????????? ???? ???
						//	|| (sX < spriteRef.GetWidth() && sY < spriteRef.GetHeight()
							// Filter?? ?????? Sprite ù?? ???? ? ???
						//	 &&	sX,pX > 0 && sY,pY > 0))
						//{
						//if( pImageObject->IsHalf() )
						//	m_pSurface->BltSpriteHalf(pPoint, &m_ImageObjectSPK[ spriteID ]);
						//else
						m_pSurface->BltSprite(pPoint, &m_ImageObjectSPK[spriteID]);
						//}
						//else
						//{
							//m_pSurface->BltSpriteDarkness(pPoint, &m_ImageObjectSPK[ sprite ], DarkBits);
						//}
					}
				}
			}
			break;

			//--------------------------------
			// Blt Channel 
			//--------------------------------
			case BLT_EFFECT:
			{
				int fid = pAnimationObject->GetFrameID();
				if (fid < m_EffectAlphaFPK.GetSize())
				{
					CEffectFrame& Frame = m_EffectAlphaFPK[fid][0][pAnimationObject->GetFrame()];

					sprite = Frame.GetSpriteID();

					// ??? ????
					pPoint->x += Frame.GetCX();
					pPoint->y += Frame.GetCY();

					DRAW_ALPHASPRITEPAL(pPoint, sprite, m_EffectAlphaSPK, m_EffectAlphaPPK[fid])//, m_EffectAlphaSPKI, m_EffectAlphaSPKFile)					

						//-------------------------------------------------------
						// H/W ?????? ??? ??????...
						//-------------------------------------------------------
						if (CDirect3D::IsHAL())
						{
							//							#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
							//							CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, gtx_op );
							//							CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
							//							CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );	
							//							#endif
							//							
							//							DRAW_TEXTURE_SPRITEPAL_LOCKED(pPoint->x, pPoint->y, 
							//								sprite, m_pAlphaEffectTextureManager, fid )//m_EffectAlphaPPK[fid])
							//								
							//							#ifdef OUTPUT_DEBUG
							//								// ???????
							//							CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE);					
							//							CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
							//							CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   
							//							#endif


														//------------------------------------------------
														// LightFilter?? ??????.
														//------------------------------------------------
							AddLightFilter3D(pPoint->x + TILE_X_HALF,
								pPoint->y + TILE_Y_HALF,
								Frame.GetLight(),			// ???? ???
								false);		// false = screen???

						}
					//-------------------------------------------------------
					// 2D ???
					//-------------------------------------------------------
						else
						{
							//							DRAW_ALPHASPRITEPAL(pPoint, sprite, m_EffectAlphaSPK, m_EffectAlphaPPK[fid])//, m_EffectAlphaSPKI, m_EffectAlphaSPKFile)					

							AddLightFilter2D(pPoint->x + TILE_X_HALF,
								pPoint->y + TILE_Y_HALF,
								Frame.GetLight(),			// ???? ???
								false);		// false = screen???
						}
				}
				if (g_bFrameChanged)
				{
					pAnimationObject->NextFrame();
				}

			}
			break;

			//--------------------------------
			// Blt Channel 
			//--------------------------------
			case BLT_SCREEN:
			{
				int fid = pAnimationObject->GetFrameID();
				if (fid < m_EffectScreenFPK.GetSize())
				{
					CEffectFrame& Frame = m_EffectScreenFPK[fid][0][pAnimationObject->GetFrame()];
					sprite = Frame.GetSpriteID();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetSpriteID();

					if (sprite != SPRITEID_NULL)
					{
						// Frame ??? ????
						pPoint->x += Frame.GetCX();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCX();
						pPoint->y += Frame.GetCY();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCY();

						DRAW_NORMALSPRITEPAL_EFFECT(pPoint,
							sprite,
							m_EffectScreenSPK,
							//									m_EffectScreenSPKI, 
							//									m_EffectScreenSPKFile,
							m_EffectScreenPPK[fid],
							CSpriteSurface::EFFECT_SCREEN)
							//-------------------------------------------------------
							// H/W ?????? ??? ??????...
							//-------------------------------------------------------
							if (CDirect3D::IsHAL())
							{
								// screen test
//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_ONE );
//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE );									
//								
//								#ifdef OUTPUT_DEBUG
//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );									
//								#endif
//								
//								DRAW_TEXTURE_SPRITEPAL_LOCKED(pPoint->x, pPoint->y, 
//									sprite, m_pScreenEffectTextureManager, fid ) //m_EffectScreenPPK[fid])
//									
//									// ???????
//									CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   

								//------------------------------------------------
								// LightFilter?? ??????.
								//------------------------------------------------
								AddLightFilter3D(pPoint->x + TILE_X_HALF,
									pPoint->y + TILE_Y_HALF,
									Frame.GetLight(),			// ???? ???
									false);		// false = screen???

							}
						//-------------------------------------------------------
						// H/W ?????? ???? ??????...
						//-------------------------------------------------------
							else
							{
								//								DRAW_NORMALSPRITEPAL_EFFECT(pPoint, 
								//									sprite, 
								//									m_EffectScreenSPK, 
								////									m_EffectScreenSPKI, 
								////									m_EffectScreenSPKFile,
								//									m_EffectScreenPPK[fid],
								//									CSpriteSurface::EFFECT_SCREEN)

								AddLightFilter2D(pPoint->x + TILE_X_HALF,
									pPoint->y + TILE_Y_HALF,
									Frame.GetLight(),			// ???? ???
									false);		// false = screen???
							}
					}
					if (g_bFrameChanged)
					{
						pAnimationObject->NextFrame();
					}
				}


			}
			break;
			}

			//---------------------------------------
			// Test Code : Next Frame
			//---------------------------------------
			//if (rand()%5) 
//			if (g_bFrameChanged)
//			{
//				pAnimationObject->NextFrame();
//			}
		}

	}

	/*
	// ImageObject ID ???
	char str[80];
	sprintf(str, "%d", pImageObject->GetID());
	gC_font.PrintStringNoConvert(&m_SurfaceInfo,
								str,
								pPoint->x,
								pPoint->y,
								0xFFFFFF);
	*/

	//#ifdef OUTPUT_DEBUG
		//	DEBUG_ADD("[TempDebug] After Draw ImageObject");
	//#endif

	__END_PROFILE("DrawImageObject")

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
		DEBUG_ADD("End DrawImageObject");
#endif


}

//----------------------------------------------------------------
// SurfaceLock For AlphaEffect
//----------------------------------------------------------------
// 3D??? lock?? ?????? Unlock????...
// 3D???? lock?? ?? ?????? lock????..
//
// return???? ???? Lock????
//----------------------------------------------------------------
/*
BOOL
MTopView::SurfaceLockForAlphaEffect()
{
	BOOL bLock = m_pSurface->IsLock();

	if (CDirect3D::IsHAL())
	{
		if (bLock)
		{
			// 3D??? lock?? ?????? Unlock????...
			m_pSurface->Unlock();
		}
	}
	else
	{
		if (!bLock)
		{
			// 3D???? lock?? ?? ?????? lock????..
			m_pSurface->Lock();
		}
	}

	return bLock;
}
*/

//----------------------------------------------------------------
// SurfaceLock
//----------------------------------------------------------------
// return???? ?????? lock????
//----------------------------------------------------------------
/*
BOOL
MTopView::SurfaceLock()
{
	BOOL bLock = m_pSurface->IsLock();

	if (!bLock)
	{
		m_pSurface->Lock();
	}

	return bLock;
}
*/

//----------------------------------------------------------------
// SurfaceLock Restore
//----------------------------------------------------------------
// ?????? Lock???·? ???????.
//----------------------------------------------------------------
/*
void
MTopView::SurfaceLockRestore( BOOL bOldLock )
{
	if (bOldLock)
	{
		if (!m_pSurface->IsLock())
		{
			// ???? Lock????µ? Lock?? ????
			m_pSurface->Lock();
		}
	}
	else
	{
		if (m_pSurface->IsLock())
		{
			// ???? Lock?? ?????µ? Lock???..
			m_pSurface->Unlock();
		}
	}
}
*/

//----------------------------------------------------------------
// Effect ???
//----------------------------------------------------------------
void
MTopView::DrawEffect(POINT* pPoint, EFFECT_LIST::const_iterator iEffect, BYTE size)
{
	if (g_pEventManager->GetEventByFlag(EVENTFLAG_NOT_DRAW_EFFECT))
		return;

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawEffect");
#endif


	POINT point;

	for (int i = 0; i < size; i++)
	{
		MEffect* pEffect = *iEffect;

		if (pEffect->IsDelayFrame() == true)
		{
			iEffect++;
			continue;
		}

		//------------------------------------------------------------------
		// Tile?????? ??? ???
		//------------------------------------------------------------------
		//if (pEffect->GetEffectType()==MEffect::EFFECT_SECTOR)
		//{			
		//	point = *pPoint;
		//}
		//------------------------------------------------------------------
		// Screen ??? ???
		//------------------------------------------------------------------
		//else 
		if (pEffect->GetEffectType() == MEffect::EFFECT_SCREEN)
		{
			MScreenEffect* pScreenEffect = (MScreenEffect*)pEffect;
			//MScreenEffect::SetScreenBasis( pPoint->x, pPoint->y );	// ????? ????? ??µ?.. ????.
			point.x = pScreenEffect->GetScreenX();
			point.y = pScreenEffect->GetScreenY();
		}
		//------------------------------------------------------------------
		// Pixel?????? ??? ???
		//------------------------------------------------------------------
		else
		{
			//MMovingEffect* pMovingEffect = (MMovingEffect* const)pEffect;

			// Pixel ?????? ????? ????? ????? ??????.
			point.x = pEffect->GetPixelX() - m_FirstZonePixel.x;
			point.y = pEffect->GetPixelY() - pEffect->GetPixelZ() - m_FirstZonePixel.y;
		}

		//------------------------------------------------------------------
		// ???
		//------------------------------------------------------------------
		DrawEffect(&point, pEffect, pEffect->IsSelectable());

		// ???? Effect
		iEffect++;
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawEffect");
#endif


}


//----------------------------------------------------------------
// Draw GroundEffect
//----------------------------------------------------------------
// Tile?? ??? ??? ??? GroundEffect?? ???
//----------------------------------------------------------------
void
MTopView::DrawGroundEffect()
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawGroundEffect");
#endif

	POINT point;
	POINT pointTemp;

	int size = g_pZone->GetGroundEffectCount();
	MZone::EFFECT_MAP::const_iterator iEffect = g_pZone->GetGroundEffects();

	for (int i = 0; i < size; i++)
	{
		MEffect* pEffect = iEffect->second;

		if (pEffect->IsSkipDraw())
		{
			iEffect++;
			continue;
		}

		// ??????? ?? ??? ?? ????? ????
		if (g_pUserOption->UseTeenVersion == TRUE &&
			pEffect->GetFrameID() == EFFECTSPRITETYPE_HARPOON_BOMB_GROUND)
		{
			iEffect++;
			return;
		}

		// Pixel ?????? ????? ????? ????? ??????.
		// ?????
		point.x = pEffect->GetPixelX() - m_FirstZonePixel.x;
		point.y = pEffect->GetPixelY() - pEffect->GetPixelZ() - m_FirstZonePixel.y;

		pointTemp = point;

		//------------------------------------------------------------------
		// ???
		//------------------------------------------------------------------
		DrawEffect(&pointTemp, pEffect, pEffect->IsSelectable());

		//------------------------------------------------------------------
		// ???õ?? Effect?? ???
		//------------------------------------------------------------------
		if (pEffect->IsSelectable()
			&& m_SelectEffectID == pEffect->GetID())
		{
			MEffectTarget* pEffectTarget = pEffect->GetEffectTarget();

			if (pEffectTarget != NULL)
			{
				//------------------------------------------------------------------
				// Portal?? ???
				//------------------------------------------------------------------
				if (pEffectTarget->GetEffectTargetType() == MEffectTarget::EFFECT_TARGET_PORTAL
					&& g_pPlayer->IsVampire())
				{
					MPortalEffectTarget* pPortalEffectTarget = (MPortalEffectTarget*)pEffectTarget;

					char ownerName[20];
					strcpy(ownerName, pPortalEffectTarget->GetOwnerName());
					int zoneID = pPortalEffectTarget->GetZoneID();


					int zoneX = pPortalEffectTarget->GetZoneX();
					int zoneY = pPortalEffectTarget->GetZoneY();

					if (//( !g_pUserInformation->IsNetmarble || g_mapPremiumZone.find(zoneID) == g_mapPremiumZone.end() )
						/*&&*/ g_pSystemAvailableManager->ZoneFiltering(zoneID))
						gpC_mouse_pointer->SetCursorPortal(zoneID, zoneX, zoneY, ownerName);

				}
			}
		}
		// ???? Effect
		iEffect++;
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawGroundEffect");
#endif


}

//----------------------------------------------------------------
// Draw AttachEffect
//----------------------------------------------------------------
// Effect ???
// [???????? ????] - -;;
// type - 0 - ?????? ???
//        1 - fore ground?? ???
//        2 - back ground?? ???
//----------------------------------------------------------------
// ???..
// PairFrame?? AttachEffect?????? ??????.. 
// ActionFrame?? AttachEffect?????? ??????..
// NormalEffect???? ActionFrame?? ???????. ?????? ??????? ???? ?????? ??? --;; 
//----------------------------------------------------------------
void
MTopView::DrawAttachEffect(POINT* pPoint, ATTACHEFFECT_LIST::const_iterator iEffect, BYTE size, MCreature* pCreature, int type)
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("Start DrawAttachEffect");
#endif

	if (pCreature == NULL)
		return;
	POINT pointTemp;
	for (int i = 0; i < size; i++)
	{
		MEffect* pEffect = *iEffect;

		//----------------------------------------------------------------
		// Sprite?? ?????? Effect?? ??? ??????.
		// Attach?? ?????.. Attach????? Sprite?? ?????? ???
		//----------------------------------------------------------------		
		if (pEffect->GetEffectType() != MEffect::EFFECT_ATTACH ||
			pEffect->GetEffectType() == MEffect::EFFECT_ATTACH && ((MAttachEffect*)pEffect)->IsEffectSprite()


			)
		{
			int direction = pEffect->GetDirection();
			int frameID = pEffect->GetFrameID();
			int frame = pEffect->GetFrame();


			/////////////////////////////////////////////////////////////////////
			// 2005. 11. 14. by chyaya
			// ???ó????? ????? ??? ????? ????? ?? ?? ???? ??? ???? 
			// ?????? ??????? ???????

			// ???? ??????? ????? ?? ??? ?????? ??????? ???????.

			bool IsSomeCondition =
				(g_pPlayer->GetID() != pCreature->GetID() &&
					g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION)) ||
				//				pCreature->IsHallu() ||
				pCreature->GetCreatureType() == CREATURETYPE_VAMPIRE_GHOST;

			bool IsMasterEffect =
				(frameID >= EFFECTSPRITETYPE_GRAND_MASTER_ALPHA_VAMPIRE_100 &&
					frameID <= EFFECTSPRITETYPE_GRAND_MASTER_ALPHA_OUSTERS_150_NEW) ||
				(frameID >= EFFECTSPRITETYPE_MARKET_MASTER_VAMPIRE_100_FEAR &&
					frameID <= EFFECTSPRITETYPE_MARKET_MASTER_OUSTERS_150_ADVANCE_HOPE) ||
				(frameID >= EFFECTSPRITETYPE_MARKET_MASTER_VAMPIRE_100_COLOR5 &&
					frameID <= EFFECTSPRITETYPE_MARKET_MASTER_OUSTERS_ADVANCE_COLOR5) ||
				(frameID >= EFFECTSPRITETYPE_ADVANCEMENT_AURA_CRYSTAL_VAMPIRE &&
					frameID <= EFFECTSPRITETYPE_ADVANCEMENT_AURA_CRYSTAL_OUSTER) ||
				(frameID >= EFFECTSPRITETYPE_ADVANCEMENT_MASTER_VAMPIRE_BACK &&
					frameID <= EFFECTSPRITETYPE_ADVANCEMENT_MASTER_OUSTER);

			if (IsSomeCondition && IsMasterEffect)
			{
				iEffect++;
				continue;
			}

			//
			/////////////////////////////////////////////////////////////////////


			// 2004, 9, 14, sobeit add start - ????? ?????? ????? ???????
			int TempSecreenEffect = GET_EFFECTSPRITETYPE_SCREEN(frameID);

			if (pCreature->HasEffectStatus(EFFECTSTATUS_INSTALL_TURRET))
			{
				if (frameID != EFFECTSPRITETYPE_INSTALL_TURRET_FIRE1 &&
					frameID != EFFECTSPRITETYPE_INSTALL_TURRET_FIRE2 &&
					TempSecreenEffect != EFFECTSPRITETYPE_INSTALL_TURRET_BULLET)
				{
					iEffect++;
					continue;
				}
			}

			if (!IsMasterEffect &&
				g_pUserInformation->attrOperator.GetValue() &&
				g_pOperatorOption->bHideAttachEffect &&
				pCreature->IsPlayer() &&
				g_pPlayer->GetID() != pCreature->GetID())
			{
				iEffect++;
				continue;
			}

			// 2004, 9, 14, sobeit add end - ????? ?????? ????? ???????
//			if( pEffect->GetBltType() == BLT_EFFECT &&
//				frameID >= EFFECTSPRITETYPE_PROTECTION_FROM_BLOOD_ING &&
//				frameID <= EFFECTSPRITETYPE_PROTECTION_FROM_CURSE_ING			
//				)
//			{
//				// ??? ??????? ?? ??? ?? ??????? ?????? ???.
//				if( ! (
//					pCreature == g_pPlayer || pCreature->IsPlayerParty() )
//					)
//				{
//					iEffect++;					
//					continue;
//				}
//			}


			switch ((*iEffect)->GetBltType())
			{
				//--------------------------------------------------------
				// NORMAL
				//--------------------------------------------------------
			case BLT_NORMAL:
			{
				POINT pointTemp2 = *pPoint;

				AFFECT_ORBIT_EFFECT_POSITION(pEffect, pointTemp)

					DRAW_SPRITE_WITH_EFFECTFRAME(
						&pointTemp2,
						m_EffectNormalSPK,
						m_EffectNormalFPK,
						frameID, direction, frame)

					int est = GET_EFFECTSPRITETYPE_NORMAL(frameID);

				DRAW_SPRITE_WITH_EFFECTFRAME_PAIR(
					est,
					&pointTemp2,
					m_EffectNormalSPK,
					m_EffectNormalFPK,
					direction, frame)
			}
			break;

			//--------------------------------------------------------
			// EFFECT
			//--------------------------------------------------------
			case BLT_EFFECT:
			{
				int est = GET_EFFECTSPRITETYPE_EFFECT(frameID);
				// action?? ?´? effectFrameID?? ?????? ???.
				int aest = GET_ACTION_EFFECTSPRITETYPE(est);
				if (aest == 32)
				{
					int a = 0;
				}
				const bool bBack = (*g_pEffectSpriteTypeTable)[est].bPairFrameBack;
				int sest = GET_EFFECTSPRITETYPE_EFFECT(frameID);

				if (aest != FRAMEID_NULL)
				{
					// FRAMEID_NULL??? ü????. min( , ) -_-;
					int action = pCreature->GetAction();
					if (pCreature->IsSlayerCharacter())
					{
						action = SlayerActionSpecialization((MCreatureWear*)pCreature, action);
					}
					if (action == ACTION_ADVANCEMENT_SLAYER_MAGIC_ATTACK ||
						action == ACTION_ADVANCEMENT_OUSTERS_MAGIC_ATTACK ||
						action == ACTION_ADVANCEMENT_MAGIC_ATTACK)
						action = ACTION_MAGIC;

					if (aest == ACTIONEFFECTSPRITETYPE_EXTREME_MALE ||
						aest == ACTIONEFFECTSPRITETYPE_EXTREME_FEMALE)
					{
						if (GET_ACTION_EFFECTSPRITETYPE_FRAMEID(aest, action) == EFFECTSPRITETYPE_NULL ||
							pCreature->GetCreatureType() == CREATURETYPE_BAT ||
							pCreature->GetCreatureType() == CREATURETYPE_VAMPIRE_GHOST ||
							pCreature->GetCreatureType() == CREATURETYPE_WOLF ||
							pCreature->GetCreatureType() == CREATURETYPE_WER_WOLF ||
#if __CONTENTS(__FAST_TRANSFORTER)
							pCreature->GetCreatureType() == CREATURETYPE_FLITTERMOUSE ||
#endif //__FAST_TRANSFORTER
#if __CONTENTS(__SECOND_TRANSFORTER)
							pCreature->GetCreatureType() == CREATURETYPE_SHAPE_OF_DEMON ||
#endif //__SECOND_TRANSFORTER
							pCreature->IsAdvancementClass())
						{
							iEffect++;
							continue;
						}
					}

					int actionFrameID = GET_ACTION_EFFECTSPRITETYPE_FRAMEID(aest, action);

					direction = pCreature->GetDirection();

					if (actionFrameID != FRAMEID_NULL
						)//&& actionFrameID!=frameID)
					{
						frameID = actionFrameID;

						MAttachEffect* pAttachEffect = dynamic_cast<MAttachEffect*>(pEffect);
						const int actionCount = ((action == ACTION_MOVE || action == ACTION_SLAYER_MOTOR_MOVE) ? pCreature->GetMoveCount() : pCreature->GetActionCount());

						if (pAttachEffect->IsBasisCreature())
							frame = max(0, min(actionCount, pEffect->GetMaxFrame() - 1));
						else
							frame = pEffect->GetFrame();

						sest = GET_EFFECTSPRITETYPE_EFFECT(actionFrameID);
					}
				}

				// ?????? ?????? ??? ??????, ???? ????? ?????????Ÿ??? ???? ????????? ?????? ?????
				// ???? ????? ??????, ???????? ???? ?? ???? ?????. 
				// ????? ????? ???????, ?????? ??? ??????? ??? ?????? ?????? ?? ???? ????..

				bool	bLarSlash = est >= EFFECTSPRITETYPE_NEW_LAR_SLASH_MALE_FAST && est <= EFFECTSPRITETYPE_NEW_LAR_SLASH_FEMALE_SLOW;
				bool	bRediance = est >= EFFECTSPRITETYPE_NEW_REDIANCE_MALE_FAST && est <= EFFECTSPRITETYPE_NEW_REDIANCE_FEMALE_NORMAL_ATTACK_SLOW;
#if __CONTENTS(__SECOND_TRANSFORTER)
				bool	bUnicorn = (pCreature->GetAction() == ACTION_OUSTERS_UNICORN_STAND);
#endif //__SECOND_TRANSFORTER

				if (bRediance || bLarSlash
#if __CONTENTS(__SECOND_TRANSFORTER)
					|| bUnicorn
#endif //__SECOND_TRANSFORTER
					)
				{
					if (bRediance && (pCreature->GetAction() == ACTION_SLAYER_SWORD || pCreature->GetAction() == ACTION_SLAYER_SWORD_FAST || pCreature->GetAction() == ACTION_SLAYER_SWORD_SLOW) ||
						pCreature->GetAction() == ACTION_SLAYER_SWORD_2 || pCreature->GetAction() == ACTION_SLAYER_SWORD_2_SLOW || pCreature->GetAction() == ACTION_SLAYER_SWORD_2_FAST)
					{
						frame = max(0, min(pCreature->GetActionCount(), pEffect->GetMaxFrame() - 1));
					}
#if __CONTENTS(__SECOND_TRANSFORTER)
					else if (bUnicorn && (pCreature->GetAction() == ACTION_OUSTERS_UNICORN_STAND))
					{
						frame = max(0, min(pCreature->GetActionCount(), pEffect->GetMaxFrame() - 1));
					}
#endif //__SECOND_TRANSFORTER
					else
					{
						// ??? ??????? ?? ???? ?????.
						iEffect++;
						continue;
					}
				}

				EFFECTFRAME_ARRAY& EFA = m_EffectAlphaFPK[frameID][direction];

				frame = min(frame, EFA.GetSize() - 1);

				CEffectFrame& Frame = EFA[frame];

				bool bFrameBackground = Frame.IsBackground();
				AFFECT_ORBIT_EFFECT_BACKGROUND(pEffect, bFrameBackground);

				// ???? ??µ?°??? ü??? ?????.


				if (HAS_PAIR_EFFECTSPRITETYPE(sest) && bBack == true)
				{
					EFFECTSPRITETYPETABLE_INFO::FRAMEID_LIST& idList = (*g_pEffectSpriteTypeTable)[sest].PairFrameIDList;

					EFFECTSPRITETYPETABLE_INFO::FRAMEID_LIST::iterator iID = idList.begin();

					while (iID != idList.end())
					{
						int pairFrameID = *iID;	//(*g_pEffectSpriteTypeTable)[sest].PairFrameID;
						EFFECTFRAME_ARRAY& EA = m_EffectAlphaFPK[pairFrameID][direction];

						int tempFrame = frame % EA.GetSize();

						CEffectFrame& Frame = EA[tempFrame];

						bool bFrameBackground = Frame.IsBackground();
						AFFECT_ORBIT_EFFECT_BACKGROUND(pEffect, bFrameBackground);

						// ???? ??µ?°??? ü??? ?????.
						if (type == 0
							|| type == 1 && !bFrameBackground
							|| type == 2 && bFrameBackground)
						{
							int sprite = Frame.GetSpriteID();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetSpriteID();

							if (sprite != SPRITEID_NULL)
							{
								// Frame ??? ????
								pointTemp = *pPoint;
								pointTemp.x += Frame.GetCX();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCX();
								pointTemp.y += Frame.GetCY();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCY();

								AFFECT_ORBIT_EFFECT_POSITION(pEffect, pointTemp)

									DRAW_ALPHASPRITEPAL(&pointTemp,
										sprite,
										m_EffectAlphaSPK,
										m_EffectAlphaPPK[pairFrameID])
									//															m_EffectAlphaSPKI, 
									//															m_EffectAlphaSPKFile)
																		//-------------------------------------------------------
																		// H/W ?????? ??? ??????...
																		//-------------------------------------------------------
									if (CDirect3D::IsHAL())
									{
										//										#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
										//											CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, gtx_op );
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );	
										//										#endif
										//							
										//										DRAW_TEXTURE_SPRITEPAL_LOCKED(pointTemp.x, pointTemp.y, 
										//																	sprite, m_pAlphaEffectTextureManager, pairFrameID ) //m_EffectAlphaPPK[pairFrameID])
										//
										//										#ifdef OUTPUT_DEBUG
										//											// ???????
										//											CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE);					
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   
										//										#endif


																				//------------------------------------------------
																				// LightFilter?? ??????.
																				//------------------------------------------------
										AddLightFilter3D(pPoint->x + TILE_X_HALF,
											pPoint->y + TILE_Y_HALF,
											Frame.GetLight(),			// ???? ???
											false);		// false = screen???

									}
								//-------------------------------------------------------
								// H/W ?????? ???? ??????...
								//-------------------------------------------------------
									else
									{
										//										DRAW_ALPHASPRITEPAL(&pointTemp, 
										//															sprite, 
										//															m_EffectAlphaSPK, 
										//															m_EffectAlphaPPK[pairFrameID])
										////															m_EffectAlphaSPKI, 
										////															m_EffectAlphaSPKFile)

										AddLightFilter2D(pPoint->x + TILE_X_HALF,
											pPoint->y + TILE_Y_HALF,
											Frame.GetLight(),			// ???? ???
											false);		// false = screen???
									}
							}
						}

						iID++;
					}
				}

				if (type == 0
					|| type == 1 && !bFrameBackground
					|| type == 2 && bFrameBackground)
				{
					int sprite = Frame.GetSpriteID();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetSpriteID();

					if (sprite != SPRITEID_NULL)
					{
						// Frame ??? ????
						pointTemp = *pPoint;
						pointTemp.x += Frame.GetCX();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCX();
						pointTemp.y += Frame.GetCY();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCY();

						AFFECT_ORBIT_EFFECT_POSITION(pEffect, pointTemp)

							DRAW_ALPHASPRITEPAL(&pointTemp,
								sprite,
								m_EffectAlphaSPK,
								m_EffectAlphaPPK[frameID])
							//													m_EffectAlphaSPKI, 
							//													m_EffectAlphaSPKFile)
														//-------------------------------------------------------
														// H/W ?????? ??? ??????...
														//-------------------------------------------------------
							if (CDirect3D::IsHAL())
							{
								//								#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
								//									CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, gtx_op );
								//									CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
								//									CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );	
								//								#endif
								//					
								//								DRAW_TEXTURE_SPRITEPAL_LOCKED(pointTemp.x, pointTemp.y, 
								//															sprite, m_pAlphaEffectTextureManager, frameID ) //m_EffectAlphaPPK[frameID])
								//
								//								#ifdef OUTPUT_DEBUG
								//									// ???????
								//									CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE);					
								//									CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
								//									CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   
								//								#endif


																//------------------------------------------------
																// LightFilter?? ??????.
																//------------------------------------------------
								AddLightFilter3D(pPoint->x + TILE_X_HALF,
									pPoint->y + TILE_Y_HALF,
									Frame.GetLight(),			// ???? ???
									false);		// false = screen???

							}
						//-------------------------------------------------------
						// H/W ?????? ???? ??????...
						//-------------------------------------------------------
							else
							{
								DRAW_ALPHASPRITEPAL(&pointTemp,
									sprite,
									m_EffectAlphaSPK,
									m_EffectAlphaPPK[frameID])
									//													m_EffectAlphaSPKI, 
									//													m_EffectAlphaSPKFile)

									AddLightFilter2D(pPoint->x + TILE_X_HALF,
										pPoint->y + TILE_Y_HALF,
										Frame.GetLight(),			// ???? ???
										false);		// false = screen???
							}
					}
				}

				//--------------------------------------------------------
				// Pair Frame ???
				//--------------------------------------------------------
//					sest = GET_EFFECTSPRITETYPE_EFFECT( frameID );

				if (HAS_PAIR_EFFECTSPRITETYPE(sest) && bBack == false)
				{
					EFFECTSPRITETYPETABLE_INFO::FRAMEID_LIST& idList = (*g_pEffectSpriteTypeTable)[sest].PairFrameIDList;

					EFFECTSPRITETYPETABLE_INFO::FRAMEID_LIST::iterator iID = idList.begin();

					while (iID != idList.end())
					{
						int pairFrameID = *iID;	//(*g_pEffectSpriteTypeTable)[sest].PairFrameID;
						EFFECTFRAME_ARRAY& EA = m_EffectAlphaFPK[pairFrameID][direction];

						int tempFrame = frame % EA.GetSize();

						CEffectFrame& Frame = EA[tempFrame];

						bool bFrameBackground = Frame.IsBackground();
						AFFECT_ORBIT_EFFECT_BACKGROUND(pEffect, bFrameBackground);

						// ???? ??µ?°??? ü??? ?????.
						if (type == 0
							|| type == 1 && !bFrameBackground
							|| type == 2 && bFrameBackground)
						{
							int sprite = Frame.GetSpriteID();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetSpriteID();

							if (sprite != SPRITEID_NULL)
							{
								// Frame ??? ????
								pointTemp = *pPoint;
								pointTemp.x += Frame.GetCX();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCX();
								pointTemp.y += Frame.GetCY();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCY();

								AFFECT_ORBIT_EFFECT_POSITION(pEffect, pointTemp)

									DRAW_ALPHASPRITEPAL(&pointTemp,
										sprite,
										m_EffectAlphaSPK,
										m_EffectAlphaPPK[pairFrameID])
									//															m_EffectAlphaSPKI, 
									//															m_EffectAlphaSPKFile)
																		//-------------------------------------------------------
																		// H/W ?????? ??? ??????...
																		//-------------------------------------------------------
									if (CDirect3D::IsHAL())
									{
										//										#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
										//											CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, gtx_op );
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );	
										//										#endif
										//							
										//										DRAW_TEXTURE_SPRITEPAL_LOCKED(pointTemp.x, pointTemp.y, 
										//																	sprite, m_pAlphaEffectTextureManager, pairFrameID ) //m_EffectAlphaPPK[pairFrameID])
										//
										//										#ifdef OUTPUT_DEBUG
										//											// ???????
										//											CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE);					
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
										//											CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   
										//										#endif


																				//------------------------------------------------
																				// LightFilter?? ??????.
																				//------------------------------------------------
										AddLightFilter3D(pPoint->x + TILE_X_HALF,
											pPoint->y + TILE_Y_HALF,
											Frame.GetLight(),			// ???? ???
											false);		// false = screen???

									}
								//-------------------------------------------------------
								// H/W ?????? ???? ??????...
								//-------------------------------------------------------
									else
									{
										//										DRAW_ALPHASPRITEPAL(&pointTemp, 
										//															sprite, 
										//															m_EffectAlphaSPK, 
										//															m_EffectAlphaPPK[pairFrameID])
										////															m_EffectAlphaSPKI, 
										////															m_EffectAlphaSPKFile)

										AddLightFilter2D(pPoint->x + TILE_X_HALF,
											pPoint->y + TILE_Y_HALF,
											Frame.GetLight(),			// ???? ???
											false);		// false = screen???
									}
							}
						}

						iID++;
					}
				}
			}
			break;

			//--------------------------------------------------------
			// SCREEN
			//--------------------------------------------------------
			case BLT_SCREEN:
			{
				int est = GET_EFFECTSPRITETYPE_SCREEN(frameID);
				bool bBack = (*g_pEffectSpriteTypeTable)[est].bPairFrameBack;

				// action?? ?´? effectFrameID?? ?????? ???.
				int aest = GET_ACTION_EFFECTSPRITETYPE(est);

				if (aest != FRAMEID_NULL)
				{
					int action = pCreature->GetAction();
					if (action == ACTION_ADVANCEMENT_SLAYER_MAGIC_ATTACK ||
						action == ACTION_ADVANCEMENT_OUSTERS_MAGIC_ATTACK ||
						action == ACTION_ADVANCEMENT_MAGIC_ATTACK)
						action = ACTION_MAGIC;

					int actionFrameID = GET_ACTION_EFFECTSPRITETYPE_FRAMEID(aest, action);

					direction = pCreature->GetDirection();

					if (actionFrameID != FRAMEID_NULL
						&& actionFrameID != frameID)
					{
						frameID = actionFrameID;

						int actionCount = ((action == ACTION_MOVE || action == ACTION_SLAYER_MOTOR_MOVE) ? pCreature->GetMoveCount() : pCreature->GetActionCount());
						frame = max(0, min(actionCount, pEffect->GetMaxFrame() - 1));
					}
				}

				EFFECTFRAME_ARRAY& EFA = m_EffectScreenFPK[frameID][direction];
				frame = min(frame, EFA.GetSize() - 1);

				CEffectFrame& Frame = EFA[frame];

				bool bFrameBackground = Frame.IsBackground();
				AFFECT_ORBIT_EFFECT_BACKGROUND(pEffect, bFrameBackground);

				// ???? ??µ?°??? ü??? ?????.
				if (type == 0
					|| type == 1 && !bFrameBackground
					|| type == 2 && bFrameBackground)
				{
					int sprite = Frame.GetSpriteID();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetSpriteID();

					if (sprite != SPRITEID_NULL)
					{
						// Frame ??? ????
						pointTemp = *pPoint;
						pointTemp.x += Frame.GetCX();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCX();
						pointTemp.y += Frame.GetCY();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCY();

						AFFECT_ORBIT_EFFECT_POSITION(pEffect, pointTemp)

							DRAW_NORMALSPRITEPAL_EFFECT(&pointTemp,
								sprite,
								m_EffectScreenSPK,
								m_EffectScreenPPK[frameID],
								//													m_EffectScreenSPKI, 
								//													m_EffectScreenSPKFile,
								CSpriteSurface::EFFECT_SCREEN)
							//-------------------------------------------------------
							// H/W ?????? ??? ??????...
							//-------------------------------------------------------
							if (CDirect3D::IsHAL())
							{
								//								// screen test
								//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_ONE );
								//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE );									
								//
								//							#ifdef OUTPUT_DEBUG
								//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
								//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );									
								//							#endif
								//					
								//								DRAW_TEXTURE_SPRITEPAL_LOCKED(pointTemp.x, pointTemp.y, 
								//															sprite, m_pScreenEffectTextureManager, frameID ) // m_EffectScreenPPK[frameID])
								//
								//								// ???????
								//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
								//								CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   

																//------------------------------------------------
																// LightFilter?? ??????.
																//------------------------------------------------
								AddLightFilter3D(pPoint->x + TILE_X_HALF,
									pPoint->y + TILE_Y_HALF,
									Frame.GetLight(),			// ???? ???
									false);		// false = screen???

							}
						//-------------------------------------------------------
						// H/W ?????? ???? ??????...
						//-------------------------------------------------------
							else
							{
								DRAW_NORMALSPRITEPAL_EFFECT(&pointTemp,
									sprite,
									m_EffectScreenSPK,
									m_EffectScreenPPK[frameID],
									//													m_EffectScreenSPKI, 
									//													m_EffectScreenSPKFile,
									CSpriteSurface::EFFECT_SCREEN)

									AddLightFilter2D(pPoint->x + TILE_X_HALF,
										pPoint->y + TILE_Y_HALF,
										Frame.GetLight(),			// ???? ???
										false);		// false = screen???
							}
					}
				}

				//--------------------------------------------------------
				// Pair Frame ???
				//--------------------------------------------------------
				est = GET_EFFECTSPRITETYPE_SCREEN(frameID);

				if (HAS_PAIR_EFFECTSPRITETYPE(est))
				{
					EFFECTSPRITETYPETABLE_INFO::FRAMEID_LIST& idList = (*g_pEffectSpriteTypeTable)[est].PairFrameIDList;

					EFFECTSPRITETYPETABLE_INFO::FRAMEID_LIST::iterator iID = idList.begin();

					while (iID != idList.end())
					{
						int pairFrameID = *iID;//(*g_pEffectSpriteTypeTable)[*iID].PairFrameID;

						EFFECTFRAME_ARRAY& EA = m_EffectScreenFPK[pairFrameID][direction];

						int tempFrame = frame % EA.GetSize();

						CEffectFrame& Frame = EA[tempFrame];

						bool bFrameBackground = Frame.IsBackground();
						AFFECT_ORBIT_EFFECT_BACKGROUND(pEffect, bFrameBackground);

						// ???? ??µ?°??? ü??? ?????.
						if (type == 0
							|| type == 1 && !bFrameBackground
							|| type == 2 && bFrameBackground)
						{
							int sprite = Frame.GetSpriteID();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetSpriteID();

							if (sprite != SPRITEID_NULL)
							{
								// Frame ??? ????
								pointTemp = *pPoint;
								pointTemp.x += Frame.GetCX();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCX();
								pointTemp.y += Frame.GetCY();	//m_EffectAlphaFPK[(*iEffect)->GetFrameID()][direction][(*iEffect)->GetFrame()].GetCY();

								AFFECT_ORBIT_EFFECT_POSITION(pEffect, pointTemp)

									DRAW_NORMALSPRITEPAL_EFFECT(&pointTemp,
										sprite,
										m_EffectScreenSPK,
										m_EffectScreenPPK[pairFrameID],
										//															m_EffectScreenSPKI, 
										//															m_EffectScreenSPKFile,
										CSpriteSurface::EFFECT_SCREEN)
									//-------------------------------------------------------
									// H/W ?????? ??? ??????...
									//-------------------------------------------------------
									if (CDirect3D::IsHAL())
									{
										//										// screen test
										//										CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_ONE );
										//										CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE );									
										//
										//									#ifdef OUTPUT_DEBUG
										//										CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
										//										CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );									
										//									#endif
										//							
										//										DRAW_TEXTURE_SPRITEPAL_LOCKED(pointTemp.x, pointTemp.y, 
										//																	sprite, m_pScreenEffectTextureManager, pairFrameID )//m_EffectScreenPPK[pairFrameID])
										//
										//										// ???????
										//										CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
										//										CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   

																				//------------------------------------------------
																				// LightFilter?? ??????.
																				//------------------------------------------------
										AddLightFilter3D(pPoint->x + TILE_X_HALF,
											pPoint->y + TILE_Y_HALF,
											Frame.GetLight(),			// ???? ???
											false);		// false = screen???

									}
								//-------------------------------------------------------
								// H/W ?????? ???? ??????...
								//-------------------------------------------------------
									else
									{
										DRAW_NORMALSPRITEPAL_EFFECT(&pointTemp,
											sprite,
											m_EffectScreenSPK,
											m_EffectScreenPPK[pairFrameID],
											//															m_EffectScreenSPKI, 
											//															m_EffectScreenSPKFile,
											CSpriteSurface::EFFECT_SCREEN)

											AddLightFilter2D(pPoint->x + TILE_X_HALF,
												pPoint->y + TILE_Y_HALF,
												Frame.GetLight(),			// ???? ???
												false);		// false = screen???
									}
							}
						}

						iID++;
					}
				}
			}
			break;
			}
		}

		// ???? Effect
		iEffect++;
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD("End DrawAttachEffect");
#endif


}

//----------------------------------------------------------------
// (x,y)?? m_pZone?? Minimap?? ?????.
//----------------------------------------------------------------
// Test Function
//----------------------------------------------------------------
void
MTopView::DrawMinimap(int x, int y, BYTE scale)
{

	//------------------------------------------------
	// vampire?? ???? ??¾????.
	//------------------------------------------------
	//if (g_pPlayer->IsVampire())
	//{
	//	return;
	//}

	//------------------------------------------------
	// 3D ????
	//------------------------------------------------
//	if (CDirect3D::IsHAL())
//	{
//		if (m_pMinimapTexture!=NULL)
//		{
//			if (CDirect3D::GetDevice()->BeginScene()!=D3D_OK)
//			{
//				return;
//			}			
//	
//			// Texture ????
//			CDirect3D::GetDevice()->SetTexture(0, m_pMinimapTexture->GetSurface());
//
//			RECT rect = { x, 
//							y,
//							x + m_MinimapTextureWidth,
//							y + m_MinimapTextureHeight
//			};
//
//			m_TextureEffect.DrawEffect2D( &rect );		
//			
//			CDirect3D::GetDevice()->EndScene();		
//
//			CDirect3D::GetDevice()->SetTexture(0, NULL);
//
//
//			//------------------------------------------------
//			// Player????? ?????
//			//------------------------------------------------
////			if (!m_pSurface->Lock())
////				return;
////
////			POINT point = { x, y };
////
////			WORD*	pSurface	= (WORD*)m_pSurface->GetSurfacePointer();
////			long	lPitch		= m_pSurface->GetSurfacePitch();
////
////			// Player?? Minimap???? ???
////			point.x = x + g_pPlayer->GetX()*m_SectorToMinimapWidth;
////			point.y = y + g_pPlayer->GetY()*m_SectorToMinimapHeight;
////
////			// ?? ???
////			WORD*	pSurfaceTemp;	
////			pSurfaceTemp = (WORD*)((BYTE*)pSurface + lPitch*point.y + (point.x<<1));
////			*pSurfaceTemp++ = 0xFFFF;
////			*pSurfaceTemp = 0xFFFF;
////			pSurfaceTemp = (WORD*)((BYTE*)pSurface + lPitch*(point.y+1) + (point.x<<1));
////			*pSurfaceTemp++ = 0xFFFF;
////			*pSurfaceTemp = 0xFFFF;
////
////			m_pSurface->Unlock();
//		}
//	}
//	//------------------------------------------------
//	// 2D
//	//------------------------------------------------
//	else
//	{
//		if (m_pMinimapSPR!=NULL)
//		{
//			if (!m_pSurface->Lock())
//				return;
//
//			POINT point = { x, y };
//			m_pSurface->BltSpriteHalf(&point, m_pMinimapSPR);
//
//			//------------------------------------------------
//			// Player????? ?????
//			//------------------------------------------------	
////			WORD*	pSurface	= (WORD*)m_pSurface->GetSurfacePointer();
////			long	lPitch		= m_pSurface->GetSurfacePitch();
////
////			// Player?? Minimap???? ???
////			point.x = x + g_pPlayer->GetX()*m_SectorToMinimapWidth;
////			point.y = y + g_pPlayer->GetY()*m_SectorToMinimapHeight;
////
////			// ?? ???
////			WORD*	pSurfaceTemp;	
////			pSurfaceTemp = (WORD*)((BYTE*)pSurface + lPitch*point.y + (point.x<<1));
////			*pSurfaceTemp++ = 0xFFFF;
////			*pSurfaceTemp = 0xFFFF;
////			pSurfaceTemp = (WORD*)((BYTE*)pSurface + lPitch*(point.y+1) + (point.x<<1));
////			*pSurfaceTemp++ = 0xFFFF;
////			*pSurfaceTemp = 0xFFFF;
////
//			m_pSurface->Unlock();
//		}
//	}


	/*
	// ??? ???
	m_pSurface->Lock();

	POINT point = { x, y };

	// Minimap ???
	m_pSurface->BltSprite( &point, m_pMinimapSPR );

	int width = m_pMinimapSPR->GetWidth();
	int height = m_pMinimapSPR->GetHeight();

	WORD*	pSurface	= (WORD*)m_pSurface->GetSurfacePointer();
	long	lPitch		= m_pSurface->GetSurfacePitch();

	// Player?? Minimap???? ???
	point.x = x + g_pPlayer->GetX()*m_SectorToMinimapWidth;
	point.y = y + g_pPlayer->GetY()*m_SectorToMinimapHeight;

	// ?? ???
	WORD*	pSurfaceTemp;
	pSurfaceTemp = (WORD*)((BYTE*)pSurface + lPitch*point.y + (point.x<<1));
	*pSurfaceTemp++ = 0xFFFF;
	*pSurfaceTemp = 0xFFFF;
	pSurfaceTemp = (WORD*)((BYTE*)pSurface + lPitch*(point.y,1) + (point.x<<1));
	*pSurfaceTemp++ = 0xFFFF;
	*pSurfaceTemp = 0xFFFF;

	m_pSurface->Unlock();
	*/

	/*
	WORD	*lpSurface,
			*lpSurfaceTemp;

	WORD	pitch;

	WORD	color = 0;

	m_pSurface->LockW(lpSurface, pitch);

	lpSurface = (WORD*)((BYTE*)lpSurface + y*pitch + (x<<1));
	//lpSurface += (m_pZone->GetHeight()-1)<<scale;

	TYPE_SECTORPOSITION		pX = g_pPlayer->GetX(),
							pY = g_pPlayer->GetY();

	int i,j;

	// Draw Minimap
	for (i=0; i<m_pZone->GetHeight(); i++)
	{
		lpSurfaceTemp = lpSurface;

		for (j=0; j<m_pZone->GetWidth(); j++)
		{
			// player???
			if (pX==j && pY==i)
			{
				color = 0xFFFF;
			}
			else
			{
				// tile??????..

				//switch (m_pZone->GetSector(j,i).GetSpriteID())
				//{
				//	case 0 :	color = 0x07E0;	break;	// G
				//	case 1 :	color = 0xC2E5;	break;	// ?
				//	case 2 :	color = 0x001F;	break;	// B
				//	default :	color = 0x18E3;	break;	// ,black
				//}

				 const MSector &sector = m_pZone->GetSector(j,i);

				// ???
				if (sector.IsExistImageObject() && sector.IsBlockAny())
				{
					color = 0x07E0;	// G
				}
				// Creature
				else if (sector.IsExistAnyCreature())
				{
					color = 0xF000;	// R
				}
				// Item
				else if (sector.IsExistItem())
				{
					color = 0x001F;	// B
				}
				else
				{
					color = 0;
					//lpSurfaceTemp = (WORD*)((BYTE*)lpSurfaceTemp + pitch*scale + scale*4);
					//continue;
				}
			}

			*lpSurfaceTemp = color;

			// ???? ??
			lpSurfaceTemp = (WORD*)((BYTE*)lpSurfaceTemp + scale*4);
		}

		// ???? ??
		lpSurface = (WORD*)((BYTE*)lpSurface + pitch*scale);
	}

	m_pSurface->Unlock();
	*/
}

//----------------------------------------------------------------
// Draw Item Broken
//----------------------------------------------------------------
void
MTopView::DrawItemBroken(int x, int y)
{
	//	return;
		/*
			MAX_GEAR?? -10 ??? core zap+bloodbible ???? ??? ????..^^;
		*/
		// ????????? item???
	MPlayerGear* pGear = nullptr;
	int spriteID;
	int frameType;
	int toSomewhatBroken;
	int toAlmostBroken;

	//----------------------------------------------------------------
	// Slayer
	//----------------------------------------------------------------
	switch (g_pPlayer->GetRace())
	{
	case RACE_SLAYER:
	{
		if (g_pPlayer->IsMale())
		{
			frameType = 0;
			spriteID = SPRITEID_ITEM_BROKEN_SLAYER_MALE;
		}
		else
		{
			frameType = 1;
			spriteID = SPRITEID_ITEM_BROKEN_SLAYER_FEMALE;
		}

		pGear = g_pSlayerGear;
		//chyaya 2006.09.04 Cue Of Adam ???? ???? ??? begin
		toSomewhatBroken = MSlayerGear::MAX_GEAR_SLAYER + 2 - 17;
		//toSomewhatBroken = MSlayerGear::MAX_GEAR_SLAYER + 2-13;
		//toSomewhatBroken = MSlayerGear::MAX_GEAR_SLAYER + 2-12;
		//chyaya 2006.09.04 Cue Of Adam ???? ???? ??? end
		toAlmostBroken = toSomewhatBroken << 1;
	}
	break;

	case RACE_VAMPIRE:
	{
		if (g_pPlayer->IsMale())
		{
			frameType = 2;
			spriteID = SPRITEID_ITEM_BROKEN_VAMPIRE_MALE;
		}
		else
		{
			frameType = 3;
			spriteID = SPRITEID_ITEM_BROKEN_VAMPIRE_FEMALE;
		}

		pGear = g_pVampireGear;
		//chyaya 2006.09.04 Cue Of Adam ???? ???? ??? begin
		toSomewhatBroken = MVampireGear::MAX_GEAR_VAMPIRE - 17;
		//toSomewhatBroken = MVampireGear::MAX_GEAR_VAMPIRE-13;
		//toSomewhatBroken = MVampireGear::MAX_GEAR_VAMPIRE-12;
		//chyaya 2006.09.04 Cue Of Adam ???? ???? ??? end
		toAlmostBroken = toSomewhatBroken << 1;
	}
	break;

	case RACE_OUSTERS:
	{
		spriteID = SPRITEID_ITEM_BROKEN_OUSTERS;
		frameType = 4;
		pGear = g_pOustersGear;
		//chyaya 2006.09.04 Cue Of Adam ???? ???? ??? begin
		toSomewhatBroken = MOustersGear::MAX_GEAR_OUSTERS + 1 - 17;
		//toSomewhatBroken = MOustersGear::MAX_GEAR_OUSTERS + 1-13;
		//toSomewhatBroken = MOustersGear::MAX_GEAR_OUSTERS + 1-12;
		//chyaya 2006.09.04 Cue Of Adam ???? ???? ??? end
		toAlmostBroken = toSomewhatBroken << 1;
	}
	break;
	}

	//----------------------------------------------------------------
	// ????????? item?? ???? ??????.
	//----------------------------------------------------------------
	if (pGear->HasBrokenItem())
	{
		//---------------------------------------------
		// Lock
		//---------------------------------------------
		m_pSurface->Lock();

		//---------------------------------------------
		// ?? ??????(-_-;) ???
		//---------------------------------------------
		POINT pointBasis = { x + 15, y };
		POINT point = { pointBasis.x, pointBasis.y };

		m_pSurface->BltSprite(&point, &m_ItemBrokenSPK[spriteID]);


		int size = pGear->GetSize();

		//---------------------------------------------
		// Gear?? ??? slot?? ????? ü?
		//---------------------------------------------
		for (int i = 0; i < size - 4 - 6; i++)
		{
			const MItem* pItem = pGear->GetItem((BYTE)i);

			//---------------------------------------------
			// ???? ??????? ???????? ??????.
			//---------------------------------------------
			if (pItem != NULL)
			{
				MPlayerGear::ITEM_STATUS itemStatus = pGear->GetItemStatus(i);

				//---------------------------------------------
				// ?????? ???.. ?????? ???? ????? ?????.
				//---------------------------------------------
				int frameID;

				if (g_pPlayer->IsSlayer())
				{
					//---------------------------------------------
					// ??????.
					//---------------------------------------------
					if (i == MSlayerGear::GEAR_SLAYER_LEFTHAND)
					{
						// ??????? ???
						if (pItem->GetItemClass() == ITEM_CLASS_SHIELD)
						{
							frameID = i;
						}
						// ????? ????.. ??? ?????.
						else
						{
							continue;
						}
					}
					//---------------------------------------------
					// ???????? ???? ???? ???
					//---------------------------------------------
					else if (i == MSlayerGear::GEAR_SLAYER_RIGHTHAND)
					{
						// ???				
						if (pItem->IsGunItem())
						{
							//chyaya 2006.09.04 Item???? ??? begin
							frameID = MSlayerGear::MAX_GEAR_SLAYER + 1 - 17;
							//frameID = MSlayerGear::MAX_GEAR_SLAYER + 1 - 13;
							//frameID = MSlayerGear::MAX_GEAR_SLAYER + 1 - 12;
							//chyaya 2006.09.04 Item???? ??? end
						}
						// ??????
						else if (pItem->GetItemClass() == ITEM_CLASS_CROSS
							|| pItem->GetItemClass() == ITEM_CLASS_MACE)
						{
							//chyaya 2006.09.04 Item???? ??? begin
							frameID = MSlayerGear::MAX_GEAR_SLAYER - 15;
							//frameID = MSlayerGear::MAX_GEAR_SLAYER -13;
							//frameID = MSlayerGear::MAX_GEAR_SLAYER -12;
							//chyaya 2006.09.04 Item???? ??? end
						}
						// ????.. ???? ??
						else
						{
							frameID = i;
						}
					}
					else
					{
						frameID = i;
					}
				}
				else if (g_pPlayer->IsOusters())
				{
					if (i == MOustersGear::GEAR_OUSTERS_RIGHTHAND)
					{
						if (pItem->GetItemClass() == ITEM_CLASS_OUSTERS_WRISTLET)
							//chyaya 2006.09.04 Item???? ??? begin
							frameID = MOustersGear::MAX_GEAR_OUSTERS - 17;
						//frameID = MOustersGear::MAX_GEAR_OUSTERS -13;
						//frameID = MOustersGear::MAX_GEAR_OUSTERS -12;
						//chyaya 2006.09.04 Item???? ??? end
						else
							frameID = i;
					}
					else
					{
						frameID = i;
					}
				}
				else
				{
					frameID = i;
				}

				//---------------------------------------------
				// ????????? ????????? ???..
				//---------------------------------------------
				if (itemStatus == MPlayerGear::ITEM_STATUS_OK || pItem->GetItemClass() == ITEM_CLASS_COUPLE_RING ||
					pItem->GetItemClass() == ITEM_CLASS_VAMPIRE_COUPLE_RING || pItem->GetItemClass() == ITEM_CLASS_OUSTERS_HARMONIC_PENDENT)
				{
					CFrame& frame = m_ItemBrokenFPK[frameType][frameID];

					spriteID = frame.GetSpriteID();

					if (spriteID != SPRITEID_NULL)
					{
						point.x = pointBasis.x + frame.GetCX();
						point.y = pointBasis.y + frame.GetCY();

						m_pSurface->BltSprite(&point, &m_ItemBrokenSPK[spriteID]);
					}
				}
				//---------------------------------------------
				// ????? ???? ?????.. ????~??? ???..
				//---------------------------------------------
				else
				{
					int plusFrameID = ((itemStatus == MPlayerGear::ITEM_STATUS_SOMEWHAT_BROKEN) ? toSomewhatBroken : toAlmostBroken);

					CFrame& frame = m_ItemBrokenFPK[frameType][frameID + plusFrameID];

					spriteID = frame.GetSpriteID();

					if (spriteID != SPRITEID_NULL)
					{
						point.x = pointBasis.x + frame.GetCX();
						point.y = pointBasis.y + frame.GetCY();

						m_pSurface->BltSprite(&point, &m_ItemBrokenSPK[spriteID]);
					}
				}
			}
		}

		//---------------------------------------------
		// Unlock
		//---------------------------------------------
		m_pSurface->Unlock();
	}
}

//----------------------------------------------------------------
// Get MaxEffectFrame
//----------------------------------------------------------------
int
MTopView::GetMaxEffectFrame(BLT_TYPE bltType, TYPE_FRAMEID frameID) const
{
#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD_FORMAT("GetMaxEffectFrame(%d, %d)", bltType, frameID);
#endif

	int numFrame = 0;

	switch (bltType)
	{
	case BLT_SCREEN:
		numFrame = m_EffectScreenFPK[frameID][0].GetSize();
		break;

	case BLT_EFFECT:
		numFrame = m_EffectAlphaFPK[frameID][0].GetSize();
		break;

	case BLT_NORMAL:
		numFrame = m_EffectNormalFPK[frameID][0].GetSize();
		break;

	case BLT_SHADOW:
		numFrame = m_EffectShadowFPK[frameID][0].GetSize();
		break;
	}

#ifdef OUTPUT_DEBUG_DRAW_PROCESS
	DEBUG_ADD_FORMAT("numFrame = %d", numFrame);
#endif

	return numFrame;
}

//----------------------------------------------------------------
// Get EffectLight
//----------------------------------------------------------------
int
MTopView::GetEffectLight(BLT_TYPE bltType, TYPE_FRAMEID frameID, int dir, int frame) const
{
	switch (bltType)
	{
	case BLT_SCREEN:
		return m_EffectScreenFPK[frameID][dir][frame].GetLight();

	case BLT_EFFECT:
		return m_EffectAlphaFPK[frameID][dir][frame].GetLight();

	case BLT_NORMAL:
		return m_EffectNormalFPK[frameID][dir][frame].GetLight();

	case BLT_SHADOW:
		return m_EffectShadowFPK[frameID][dir][frame].GetLight();
	}

	return 0;
}

//----------------------------------------------------------------
// Get EffectSpriteType
//----------------------------------------------------------------
int
MTopView::GetEffectSpriteType(BLT_TYPE bltType, TYPE_FRAMEID frameID) const
{
	switch (bltType)
	{
	case BLT_SCREEN:
		return frameID + MAX_EFFECTSPRITETYPE_ALPHAEFFECT;

	case BLT_EFFECT:
		return frameID;

	case BLT_NORMAL:
		return frameID + MAX_EFFECTSPRITETYPE_SCREENEFFECT;

	case BLT_SHADOW:
		return frameID + MAX_EFFECTSPRITETYPE_NORMALEFFECT;
	}

	return 0;
}

//----------------------------------------------------------------
// EffectPalette
//----------------------------------------------------------------
// Flame Spike (Ousters book skill 556) uses Fire Piercing's fire
// sprites; its effects are drawn with a pink copy of the palette.
//----------------------------------------------------------------
static MPalette&
EffectPalette(MPalettePack& ppk, MEffect* pEffect)
{
	TYPE_FRAMEID frameID = pEffect->GetFrameID();

	if (pEffect->GetActionInfo() != SKILL_FLAME_SPIKE)
	{
		return ppk[frameID];
	}

	static std::map<TYPE_FRAMEID, MPalette*> s_PinkPalette;

	MPalette*& pPink = s_PinkPalette[frameID];

	if (pPink == NULL)
	{
		pPink = new MPalette;
		*pPink = ppk[frameID];

		for (int i = 0; i < pPink->GetSize(); i++)
		{
			WORD& color = (*pPink)[(BYTE)i];
			int r = CDirectDraw::Red(color);
			int g = CDirectDraw::Green(color);
			int b = CDirectDraw::Blue(color);

			color = CDirectDraw::Color((BYTE)r, (BYTE)(g * 11 / 20), (BYTE)min(31, max(b, g * 17 / 20)));
		}
	}

	return *pPink;
}

//----------------------------------------------------------------
// DrawEffect ( point, MEffect* )
//----------------------------------------------------------------
// m_pSurface->IsLock() ???¿??? ?????? ???????.
//----------------------------------------------------------------
void
MTopView::DrawEffect(POINT* pPoint, MEffect* pEffect, bool bSelectable)
{
	if (g_pEventManager->GetEventByFlag(EVENTFLAG_NOT_DRAW_EFFECT))
		return;

	POINT point = *pPoint;

	switch (pEffect->GetBltType())
	{
		//------------------------------------------------------------------
		//
		//						Normal Effect
		//
		//------------------------------------------------------------------
	case BLT_NORMAL:
	{
		CFrame& Frame = m_EffectNormalFPK[pEffect->GetFrameID()][pEffect->GetDirection()][pEffect->GetFrame()];
		int spriteID = Frame.GetSpriteID();

		if (spriteID != SPRITEID_NULL)	//< m_EffectNormalSPK.GetSize())
		{
			CSprite* pSprite = &m_EffectNormalSPK[spriteID];

			point.x += Frame.GetCX();
			point.y += Frame.GetCY();

			m_pSurface->BltSprite(&point, pSprite);

			//---------------------------------------- 		
			// ????? ???? ???? ???? ????
			//---------------------------------------- 	
			if (bSelectable)
			{
				RECT rect;
				rect.left = point.x;
				rect.top = point.y;
				rect.right = rect.left + pSprite->GetWidth();
				rect.bottom = rect.top + pSprite->GetHeight();

				pEffect->SetScreenRect(&rect);
			}
		}
		else
		{
			pEffect->ClearScreenRect();
		}
	}
	break;

	//------------------------------------------------------------------
	//
	//						Alpha Effect
	//
	//------------------------------------------------------------------
	case BLT_EFFECT:
	{
		CEffectFrame& Frame = m_EffectAlphaFPK[pEffect->GetFrameID()][pEffect->GetDirection()][pEffect->GetFrame()];
		int spriteID = Frame.GetSpriteID();

		if (spriteID != SPRITEID_NULL)
		{
			point.x += Frame.GetCX();
			point.y += Frame.GetCY();

			DRAW_ALPHASPRITEPAL(&point, spriteID, m_EffectAlphaSPK, EffectPalette(m_EffectAlphaPPK, pEffect))//, m_EffectAlphaSPKI, m_EffectAlphaSPKFile)

				//---------------------------------------- 		
				// ????? ???? ???? ???? ????
				//---------------------------------------- 	
				if (bSelectable)
				{
					CAlphaSpritePal* pSprite = &m_EffectAlphaSPK[spriteID];

					RECT rect;
					rect.left = point.x;
					rect.top = point.y;
					rect.right = rect.left + pSprite->GetWidth();
					rect.bottom = rect.top + pSprite->GetHeight();

					pEffect->SetScreenRect(&rect);
				}

			//-------------------------------------------------------
			// H/W ?????? ??? ??????...
			//-------------------------------------------------------
			if (CDirect3D::IsHAL())
			{
				//					#if defined(OUTPUT_DEBUG) && defined(_DEBUG)
				//						CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, gtx_op );
				//						CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
				//						CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );	
				//					#endif
				//					
				//					DRAW_TEXTURE_SPRITEPAL_LOCKED(point.x, point.y, spriteID, m_pAlphaEffectTextureManager, pEffect->GetFrameID() ) //m_EffectAlphaPPK[pEffect->GetFrameID()])
				//
				//					#ifdef OUTPUT_DEBUG
				//						// ???????
				//						CDirect3D::GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE);					
				//						CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
				//						CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   
				//					#endif

									//---------------------------------------- 		
									// ????? ???? ???? ???? ????
									//---------------------------------------- 	
				//					if (bSelectable)
				//					{
				//						RECT rect;
				//						rect.left	= point.x;
				//						rect.top	= point.y;
				//						rect.right	= rect.left + m_pAlphaEffectTextureManager->GetSpriteWidth(spriteID);
				//						rect.bottom = rect.top + m_pAlphaEffectTextureManager->GetSpriteHeight(spriteID);
				//
				//						pEffect->SetScreenRect( &rect );							
				//					}

									//------------------------------------------------
									// LightFilter?? ??????.
									//------------------------------------------------
				AddLightFilter3D(pPoint->x + TILE_X_HALF,
					pPoint->y + TILE_Y_HALF,
					Frame.GetLight(),			// ???? ???
					false);	// false = screen???					

			}
			//-------------------------------------------------------
			// H/W ?????? ???? ??????...
			//-------------------------------------------------------
			else
			{
				//					DRAW_ALPHASPRITEPAL(&point, spriteID, m_EffectAlphaSPK, m_EffectAlphaPPK[pEffect->GetFrameID()])//, m_EffectAlphaSPKI, m_EffectAlphaSPKFile)

				//					//---------------------------------------- 		
				//					// ????? ???? ???? ???? ????
				//					//---------------------------------------- 	
				//					if (bSelectable)
				//					{
				//						CAlphaSpritePal* pSprite = &m_EffectAlphaSPK[ spriteID ];
				//
				//						RECT rect;
				//						rect.left	= point.x;
				//						rect.top	= point.y;
				//						rect.right	= rect.left + pSprite->GetWidth();
				//						rect.bottom = rect.top + pSprite->GetHeight();
				//
				//						pEffect->SetScreenRect( &rect );
				//					}


				AddLightFilter2D(pPoint->x + TILE_X_HALF,
					pPoint->y + TILE_Y_HALF,
					Frame.GetLight(),			// ???? ???
					false);		// false = screen???
			}
		}
	}
	break;

	//------------------------------------------------------------------
	//
	//						Screen Effect
	//
	//------------------------------------------------------------------
	case BLT_SCREEN:
	{
		CEffectFrame& Frame = m_EffectScreenFPK[pEffect->GetFrameID()][pEffect->GetDirection()][pEffect->GetFrame()];
		int spriteID = Frame.GetSpriteID();

		if (spriteID != SPRITEID_NULL)
		{
			point.x += Frame.GetCX();
			point.y += Frame.GetCY();

			DRAW_NORMALSPRITEPAL_EFFECT(&point,
				spriteID,
				m_EffectScreenSPK,
				m_EffectScreenPPK[pEffect->GetFrameID()],
				//										m_EffectScreenSPKI, 
				//										m_EffectScreenSPKFile,
				CSpriteSurface::EFFECT_SCREEN)

				//---------------------------------------- 		
				// ????? ???? ???? ???? ????
				//---------------------------------------- 	
				if (bSelectable)
				{
					CSpritePal* pSprite = &m_EffectScreenSPK[spriteID];

					RECT rect;
					rect.left = point.x;
					rect.top = point.y;
					rect.right = rect.left + pSprite->GetWidth();
					rect.bottom = rect.top + pSprite->GetHeight();

					pEffect->SetScreenRect(&rect);
				}
			//-------------------------------------------------------
			// H/W ?????? ??? ??????...
			//-------------------------------------------------------
			if (CDirect3D::IsHAL())
			{
				// screen test
//					CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_ONE );
//					CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE );	
//
//				#ifdef OUTPUT_DEBUG
//					CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  gtx_src );
//					CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, gtx_dest );									
//				#endif
//					
//					DRAW_TEXTURE_SPRITEPAL_LOCKED(point.x, point.y, spriteID, m_pScreenEffectTextureManager, pEffect->GetFrameID() ) //m_EffectScreenPPK[pEffect->GetFrameID()])
//
//					// ???????
//					CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA );
//					CDirect3D::GetDevice()->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );   

					//---------------------------------------- 		
					// ????? ???? ???? ???? ????
					//---------------------------------------- 	
//					if (bSelectable)
//					{
//						RECT rect;
//						rect.left	= point.x;
//						rect.top	= point.y;
//						rect.right	= rect.left + m_pScreenEffectTextureManager->GetSpriteWidth(spriteID);
//						rect.bottom = rect.top + m_pScreenEffectTextureManager->GetSpriteHeight(spriteID);
//
//						pEffect->SetScreenRect( &rect );							
//					}

					//------------------------------------------------
					// LightFilter?? ??????.
					//------------------------------------------------
				AddLightFilter3D(pPoint->x + TILE_X_HALF,
					pPoint->y + TILE_Y_HALF,
					Frame.GetLight(),			// ???? ???
					false);	// false = screen???					

			}
			//-------------------------------------------------------
			// H/W ?????? ???? ??????...
			//-------------------------------------------------------
			else
			{
				//					DRAW_NORMALSPRITEPAL_EFFECT(&point, 
				//										spriteID, 
				//										m_EffectScreenSPK, 
				//										m_EffectScreenPPK[pEffect->GetFrameID()],
				////										m_EffectScreenSPKI, 
				////										m_EffectScreenSPKFile,
				//										CSpriteSurface::EFFECT_SCREEN)

				//					//---------------------------------------- 		
				//					// ????? ???? ???? ???? ????
				//					//---------------------------------------- 	
				//					if (bSelectable)
				//					{
				//						CSpritePal* pSprite = &m_EffectScreenSPK[ spriteID ];
				//
				//						RECT rect;
				//						rect.left	= point.x;
				//						rect.top	= point.y;
				//						rect.right	= rect.left + pSprite->GetWidth();
				//						rect.bottom = rect.top + pSprite->GetHeight();
				//
				//						pEffect->SetScreenRect( &rect );
				//					}


				AddLightFilter2D(pPoint->x + TILE_X_HALF,
					pPoint->y + TILE_Y_HALF,
					Frame.GetLight(),			// ???? ???
					false);		// false = screen???
			}

		}
	}
	break;

	//------------------------------------------------------------------
	//
	//						Shadow Effect
	//
	//------------------------------------------------------------------
	case BLT_SHADOW:
	{
		CEffectFrame& Frame = m_EffectShadowFPK[pEffect->GetFrameID()][pEffect->GetDirection()][pEffect->GetFrame()];
		int spriteID = Frame.GetSpriteID();

		if (spriteID != SPRITEID_NULL)
		{
			point.x += Frame.GetCX();
			point.y += Frame.GetCY();

			CShadowSprite* pSprite = &m_EffectShadowSPK[spriteID];

			if (pSprite->IsInit())
			{
				m_pSurface->BltShadowSpriteDarkness(&point, pSprite, 1);
			}
			//-------------------------------------------------------
			// H/W ?????? ??? ??????...
			//-------------------------------------------------------
			if (CDirect3D::IsHAL())
			{
				//					DRAW_TEXTURE_SPRITE_LOCKED(point.x, point.y, spriteID, m_pEffectShadowManager)		

									//------------------------------------------------
									// LightFilter?? ??????.
									//------------------------------------------------
				AddLightFilter3D(pPoint->x + TILE_X_HALF,
					pPoint->y + TILE_Y_HALF,
					Frame.GetLight(),			// ???? ???
					false);	// false = screen???					

			}
			//-------------------------------------------------------
			// H/W ?????? ???? ??????...
			//-------------------------------------------------------
			else
			{
				//					CShadowSprite* pSprite = &m_EffectShadowSPK[spriteID];
				//
				//					if (pSprite->IsInit())
				//					{
				//						m_pSurface->BltShadowSpriteDarkness(&point, pSprite, 1 );
				//						
				AddLightFilter2D(pPoint->x + TILE_X_HALF,
					pPoint->y + TILE_Y_HALF,
					Frame.GetLight(),			// ???? ???
					false);		// false = screen???
				//					}
			}
		}
	}
	break;
	}
}

//----------------------------------------------------------------------
// Get Random MonsterType In Zone
//----------------------------------------------------------------------
int
MTopView::GetRandomMonsterTypeInZone() const
{
	int numSprites = m_listLoadedMonsterSprite.GetSize();

	int spriteType;

	if (numSprites == 0)
	{
		// ?????? ???? ???.. 
		spriteType = 27;
	}
	else
	{
		int nth = rand() % numSprites;

		INT_ORDERED_LIST::DATA_LIST::const_iterator	iID = m_listLoadedMonsterSprite.GetIterator();

		for (int i = 0; i < nth; i++)
		{
			iID++;
		}

		spriteType = *iID;
	}

	// spriteID?? sprite?? ????? ????? ????? ???????.
	return g_pCreatureSpriteTypeMapper->GetRandomCreatureType(spriteType);
}

void
MTopView::DrawCreatureHPModify(POINT* point, MCreature* pCreature)
{
	//	return;

	if (!g_pPlayer->HasEffectStatus(EFFECTSTATUS_VIEW_HP) || pCreature->IsEmptyHPModifyList())
		return;

	MCreature::HPMODIFYLIST* pList = (MCreature::HPMODIFYLIST*)pCreature->GetHPModifyList();

	MCreature::HPMODIFYLIST::iterator itr = pList->begin();

	int py = point->y - (pList->size() - 1) * 15;

	g_FL2_GetDC();

	while (itr != pList->end())
	{
		char str[128];
		COLORREF color;

		const int modifyValue = itr->modify;
		if (itr->modify < 0)
		{
			sprintf(str, "%d", modifyValue);
			color = RGB(255, 150, 150);
		}
		else
		{
			sprintf(str, "+%d", modifyValue);
			//RGB(150, 255, 150);
		}

		g_PrintColorStrOut(point->x + TILE_X_HALF - g_GetStringWidth(str, gpC_base->m_chatting_pi.hfont) / 2, py, str, gpC_base->m_chatting_pi, color, RGB_BLACK);

		py += 15;

		if (GetTickCount() - itr->TickCount > g_pClientConfig->HPModifyListTime)
		{
			pList->pop_front();
			itr = pList->begin();
		}
		else
			itr++;
	}

	g_FL2_ReleaseDC();
}
// 2004, 08, 18, sobeit add start 
//----------------------------------------------------------------------
// DrawGuildMark
/*----------------------------------------------------------------------
	- ?????? ?? ????? ???? ??? ????? ???????.
	- ???? ????? ??????? ??? ?????.
	- g_pPlayer?? ?????? ???????? ???? ?????? ???????? ??? ??????? ???? ???
//----------------------------------------------------------------------*/
void
MTopView::DrawGuildMarkInSiegeWar(MCreature* pCreature, int YPos)
{
	// EFFECTSTATUS_GHOST ?? ????????? ???? ?????? ???? ??????
	if (pCreature == NULL) return;
	if (pCreature->HasEffectStatus(EFFECTSTATUS_GHOST))return;

	//-----------------------------------------------------
	// Hallu?? ????? ?????? ????? ??????
	//-----------------------------------------------------
	if (!g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION)
		&& pCreature->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR
		&& pCreature->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR
		&& pCreature->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR
		&& g_pPlayer->GetIsInSiegeWar()
		&& pCreature->GetIsInSiegeWar()
		)
	{
		//-----------------------------------------------------
		// Guild?? ??? ??? - Guild Mark ???
		//-----------------------------------------------------
		int guildID = pCreature->GetGuildNumber();

		if (guildID > 0)
		{
			//-------------------------------------------------
			// load??? ????? ????.
			//-------------------------------------------------
			CSprite* pSprite = g_pGuildMarkManager->GetGuildMarkSmall(guildID);

			if (pSprite != NULL)
			{
				POINT pointTemp = { pCreature->GetPixelX() - m_FirstZonePixel.x + 15, YPos - 23 };

				m_pSurface->BltSprite(&pointTemp, pSprite);

			}
			else
			{
				if (!g_pGuildMarkManager->HasGuildMark(guildID))
				{
					//-------------------------------------------------
					// file?? ????? ????.
					//-------------------------------------------------
					g_pGuildMarkManager->LoadGuildMark(guildID);

					//-------------------------------------------------
					// file???? load??????? ??? ü?
					//-------------------------------------------------
					pSprite = g_pGuildMarkManager->GetGuildMark(guildID);

					//-------------------------------------------------
					// file???? ???? ???..
					// guildMark????????????? ???´?.
					//-------------------------------------------------
					//if (pSprite==NULL) {}
				}
			}
		}
	}
}
// 2004, 08, 18, sobeit add end
//----------------------------------------------------------------------
// Draw CreatureName
//----------------------------------------------------------------------
// Unlock ???¿??? ??????? ??????.
//
// ???õ? ????? ????? ??????.
// ??????? HP, ????, ???.. ????? ?????? ??????.
// ??????(-_-;) RequestMode?? ???? icon?? ??????.
//----------------------------------------------------------------------
BOOL
MTopView::DrawCreatureName(POINT* pPoint, MCreature* pCreature)
{
	// object-tribe creatures (Mihnea storage / altar) carry no name or HP bar
	if (pCreature != NULL && pCreature->IsObject()) return FALSE;
	// EFFECTSTATUS_GHOST ?? ????????? ???? ?????? ???? ??????
	if (pCreature == NULL) return FALSE;
	if (pCreature->HasEffectStatus(EFFECTSTATUS_GHOST))return FALSE;

	int creatureType = pCreature->GetCreatureType();
	POINT point = *pPoint;

	// 2004, 08, 05 sobeit add start - ?????? ???????? ???????
	if (creatureType >= 726 &&
		creatureType <= 729)
		return FALSE;
	// 2004, 08, 05 sobeit add end - ???? 
	if (
		(g_pPlayer->HasEffectStatus(EFFECTSTATUS_YELLOW_POISON_TO_CREATURE) ||
			g_pPlayer->HasEffectStatus(EFFECTSTATUS_FLARE) ||
			g_pPlayer->HasEffectStatus(EFFECTSTATUS_BLINDNESS))
		&& pCreature != g_pPlayer
		)
	{
		// ?Ÿ??? ???? ??? ????? ???????.
		int sx, sy, ex, ey;

		sx = g_pPlayer->GetX() - 1;
		ex = g_pPlayer->GetX() + 1;

		sy = g_pPlayer->GetY() - 1;
		ey = g_pPlayer->GetY() + 1;

		if (pCreature->GetX() < sx || pCreature->GetX() > ex ||
			pCreature->GetY() < sy || pCreature->GetY() > ey)
			return FALSE;
	}

	//---------------------------------------------------------------
	// ?? ??? ??? ??? ????? ????. - -;
	//---------------------------------------------------------------
	if (pCreature->IsUndergroundCreature())
	{
		return FALSE;
	}

	int yPoint = DrawChatString(&point,
		pCreature,
		RGB_WHITE,	//CDirectDraw::Color(31,31,31),
		FLAG_DRAWTEXT_OUTLINE);

#if __CONTENTS(__CONTRIBUTE_SYSTEM)
	if (pCreature->IsPlayerOnly() && !pCreature->IsNPC() && pCreature->IsAlive())
	{
		//DrawContrubuteGradeEffect(&point, pCreature , yPoint);
		//m_pSurface->BltSpriteEffect(&point, &m_EffectAlphaSPK[EFFECTSPRITETYPE_NAME_WHITE_EFFECT1]);
	}

#endif //__CONTRIBUTE_SYSTEM

	bool bDrawChatString = (yPoint != point.y);

	point.y = yPoint;
	DrawCreatureHPModify(&point, pCreature);

#if __CONTENTS(__CONTRIBUTE_SYSTEM)
	DrawRequestModeMousePointer(&point, pCreature);
#else
	m_bDrawRequest = false;
	//------------------------------------------------
	// item ??????? ??? ???
	//------------------------------------------------
	if (IsRequestMode())
	{
		bool bRequest = false;

		if (pCreature->GetClassType() == MCreature::CLASS_FAKE)
		{
			MFakeCreature* pFakeCreature = (MFakeCreature*)pCreature;
			if (pFakeCreature->GetOwnerID() != OBJECTID_NULL)	// ?????
			{
				bRequest = true;
			}
		}
		//------------------------------------------------
		// ?????? ?????? ????.
		//------------------------------------------------			
		else if (// NPC?? ????
			!pCreature->IsNPC()
			// Player?? ???
			//&& pCreature->GetCreatureType()<=CREATURETYPE_VAMPIRE_FEMALE
			//&& (*g_pCreatureSpriteTable)[(*g_pCreatureTable)[pCreature->GetCreatureType()].SpriteType].IsPlayerOnlySprite()
			&& (*g_pCreatureSpriteTable)[(*g_pCreatureTable)[pCreature->GetCreatureType()].SpriteTypes[0]].IsPlayerOnlySprite()
			// ?? ??? ??? ???
			&& !pCreature->IsInCasket()
			// ???? ?????????? ??? ???? ??? ?? ??????...
			//&& !pCreature->IsUndergroundCreature()
			// ??????.. - -;
			&& pCreature->IsAlive()
			&& pCreature->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR
			&& pCreature->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR
			&& pCreature->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR
			)
		{
			bRequest = true;
		}

		if (bRequest == true)
		{
			POINT pointTemp;
			TYPE_SPRITEID	RequestSpriteID;

			//---------------------------------------------------------------
			// trade ??û??? ??????
			//---------------------------------------------------------------
			if (IsRequestTrade())
			{
				const int maxRequestFrames = SPRITEID_ITEM_TRADE_LAST - SPRITEID_ITEM_TRADE + 1;
				RequestSpriteID = SPRITEID_ITEM_TRADE + ((g_CurrentFrame >> 1) % (maxRequestFrames << 1));

				if (RequestSpriteID > SPRITEID_ITEM_TRADE_LAST)
				{
					RequestSpriteID = SPRITEID_ITEM_TRADE_LAST - (RequestSpriteID - SPRITEID_ITEM_TRADE_LAST);

					if (RequestSpriteID < SPRITEID_ITEM_TRADE)
					{
						RequestSpriteID = SPRITEID_ITEM_TRADE;
					}
				}
			}
			//---------------------------------------------------------------
			// party ??û??? ??????
			//---------------------------------------------------------------
			else if (IsRequestParty())
			{
				const int maxRequestFrames = SPRITEID_PARTY_REQUEST_LAST - SPRITEID_PARTY_REQUEST + 1;
				RequestSpriteID = SPRITEID_PARTY_REQUEST + ((g_CurrentFrame >> 1) % (maxRequestFrames + 4));

				if (RequestSpriteID > SPRITEID_PARTY_REQUEST_LAST)
				{
					RequestSpriteID = SPRITEID_PARTY_REQUEST_LAST;
				}
			}
			//---------------------------------------------------------------
			// Info ??û??? ??????
			//---------------------------------------------------------------
			else if (IsRequestInfo())
			{
				const int maxRequestFrames = SPRITEID_INFO_REQUEST_LAST - SPRITEID_INFO_REQUEST + 1;
				RequestSpriteID = SPRITEID_INFO_REQUEST + ((g_CurrentFrame >> 1) % (maxRequestFrames + 4));

				if (RequestSpriteID > SPRITEID_INFO_REQUEST_LAST)
				{
					RequestSpriteID = SPRITEID_INFO_REQUEST_LAST;
				}
			}

			pointTemp.x = g_x + 27 - m_EtcSPK[RequestSpriteID].GetWidth();
			pointTemp.y = g_y + 15 - m_EtcSPK[RequestSpriteID].GetHeight();

			m_pSurface->Lock();
			m_pSurface->BltSprite(&pointTemp, &m_EtcSPK[RequestSpriteID]);
			m_pSurface->Unlock();

			m_bDrawRequest = true;
		}
	}
#endif //__CONTRIBUTE_SYSTEM

	//------------------------------------------------
	// ????? ??? ???
	//------------------------------------------------
	//gC_font.PrintStringNoConvert(&m_SurfaceInfo, pCreature->GetName(), pointTemp.x, pointTemp.y, 0xFFFF);	
	bool bMasterWords = pCreature->GetCreatureType() == CREATURETYPE_SLAYER_OPERATOR
		|| pCreature->GetCreatureType() == CREATURETYPE_VAMPIRE_OPERATOR
		|| pCreature->GetCreatureType() == CREATURETYPE_OUSTERS_OPERATOR
		|| strncmp(pCreature->GetName(), (*g_pGameStringTable)[UI_STRING_MESSAGE_MASTER_NAME].GetString(), (*g_pGameStringTable)[UI_STRING_MESSAGE_MASTER_NAME].GetLength()) == 0;
	//						|| (strstr(pCreature->GetName(), "????")!=NULL);

	bool bHalluName = (g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION) ||
		!bMasterWords &&
		!g_pUserInformation->IsMaster &&
		(pCreature->GetClassType() == MCreature::CLASS_PLAYER ||
			pCreature->GetClassType() == MCreature::CLASS_CREATUREWEAR) &&
		g_pPlayer->GetRace() != pCreature->GetRace()) &&
		g_pZoneTable->Get(g_pZone->GetID())->ChatMaskByRace &&
		g_pPlayer->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR &&
		g_pPlayer->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR &&
		g_pPlayer->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR;						// ê ??????? ???? ????? 
	if (g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST))
		bHalluName = false;
#ifdef __METROTECH_TEST__
	if (g_bLight)
		bHalluName = false;
#endif
	const char* pName = (bHalluName ? pCreature->GetHalluName() : pCreature->GetName());

	COLORREF	color;
	FONTID		font;

	/*
	if (pCreature->IsVampire())
	{
		color	= m_ColorNameVampire;
		font	= FONTID_VAMPIRE_NAME;
	}
	else if (pCreature->IsSlayer())
	{
		color = m_ColorNameSlayer;
		font	= FONTID_SLAYER_NAME;
	}
	else if (pCreature->IsNPC())
	{
		color = m_ColorNameNPC;
		font	= FONTID_NPC_NAME;
	}
	*/

	//-----------------------------------------------------
	// NPC?? ???
	//-----------------------------------------------------
	if (pCreature->IsNPC())
	{
		// ?????? ???.. ?????? - -;;
		if (pCreature->GetCreatureType() == 217)
		{
			color = m_ColorNameAlignment[1];
			font = FONTID_VAMPIRE_NAME;
		}
		else
		{
			color = m_ColorNameNPC;
			font = FONTID_NPC_NAME;
		}
	}
	//-----------------------------------------------------
	// ?????? ???
	//-----------------------------------------------------
	else if (pCreature->GetCompetence() == 0)
	{
		color = CDirectDraw::Color(31, 23, 3);	// ????
		font = FONTID_NPC_NAME;
	}
	//-----------------------------------------------------
	// ?? ?????? ???
	//-----------------------------------------------------
	else if (!pCreature->IsPlayerOnly())
	{
		int monsterLevel = pCreature->GetLevel();

		if (monsterLevel < 0)
		{
			color = m_ColorNameAlignment[1];
		}
		else
		{
			int playerLevel = g_char_slot_ingame.level;

			if (g_pPlayer->IsSlayer())
			{
				int domain_level_Sword = (*g_pSkillManager)[SKILLDOMAIN_SWORD].GetDomainLevel();
				int domain_level_Blade = (*g_pSkillManager)[SKILLDOMAIN_BLADE].GetDomainLevel();
				int domain_level_Gun = (*g_pSkillManager)[SKILLDOMAIN_GUN].GetDomainLevel();
				int domain_level_Heal = (*g_pSkillManager)[SKILLDOMAIN_HEAL].GetDomainLevel();
				int domain_level_Enchant = (*g_pSkillManager)[SKILLDOMAIN_ENCHANT].GetDomainLevel();

				int domainLevel = 0;
				domainLevel = max(domain_level_Sword, domain_level_Blade);
				domainLevel = max(domainLevel, domain_level_Gun);
				domainLevel = max(domainLevel, domain_level_Heal);
				domainLevel = max(domainLevel, domain_level_Enchant);

				playerLevel = domainLevel;
			}

			playerLevel += g_char_slot_ingame.m_AdvancementLevel;

			int levelGap = monsterLevel - playerLevel;

			if (levelGap < -30)
			{
				color = RGB(217, 217, 217);	// ??? - ????
			}
			else if (levelGap < -11)
			{
				color = RGB(6, 216, 86);	// ????? - ?? ????
			}
			else if (levelGap < 10)
			{
				color = RGB(239, 228, 3);	// ????? - ????
			}
			else if (levelGap < 29)
			{
				color = RGB(255, 168, 41);	// ????? - ?? ?????
			}
			else
			{
				color = RGB(228, 28, 204);	// ????? - ?????
			}
		}

		font = FONTID_VAMPIRE_NAME;
	}
	//-----------------------------------------------------
	// Hallu?? ??? ???
	//-----------------------------------------------------
	else if (g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION))
	{
		color = m_ColorNameAlignment[1];
		font = FONTID_VAMPIRE_NAME;
	}
	//-----------------------------------------------------
	// Slayer?? ??? 
	//-----------------------------------------------------
	else if (g_pPlayer->IsSlayer())
	{
		//-----------------------------------------------------
		// ?????? ???? 
		//-----------------------------------------------------
		if (g_pPlayer->CanAttackTribe(pCreature)
			|| g_pJusticeAttackManager->HasCreature(pCreature->GetName()))
			//		if (g_pObjectSelector->CanAttack( pCreature ) )
		{
			color = m_ColorNameAlignment[1];
			font = FONTID_VAMPIRE_NAME;
		}
		//-----------------------------------------------------
		// ????.. ???? ???? ???
		//-----------------------------------------------------
		else
		{
			int alignment = pCreature->GetAlignment();

			//alignment = min( 4, max(0, alignment) );

			color = m_ColorNameAlignment[alignment];
			font = FONTID_SLAYER_NAME;
		}
	}
	else if (g_pPlayer->IsOusters())
	{
		if (g_pPlayer->CanAttackTribe(pCreature)
			|| g_pJusticeAttackManager->HasCreature(pCreature->GetName()))
		{
			color = m_ColorNameAlignment[1];
			font = FONTID_VAMPIRE_NAME;
		}
		//-----------------------------------------------------
		// ????.. ???? ???? ???
		//-----------------------------------------------------
		else
		{
			int alignment = pCreature->GetAlignment();

			color = m_ColorNameAlignment[alignment];
			font = FONTID_VAMPIRE_NAME;
		}
	}
	//-----------------------------------------------------
	// ????????? ???
	//-----------------------------------------------------
	else
	{
		//-----------------------------------------------------
		// ??? ????
		//-----------------------------------------------------
		if (g_pPlayer->CanAttackGuild(pCreature)
			|| g_pJusticeAttackManager->HasCreature(pCreature->GetName()))
			//		if(g_pObjectSelector->CanAttack( pCreature ) )
		{
			color = m_ColorNameAlignment[1];
			font = FONTID_VAMPIRE_NAME;
		}
		//-----------------------------------------------------
		// ????.. ???? ???? ???
		//-----------------------------------------------------
		else
		{
			int alignment = pCreature->GetAlignment();

			color = m_ColorNameAlignment[alignment];
			font = FONTID_VAMPIRE_NAME;
		}
	}


	if (pName != NULL)
	{
		const int FontHeight = g_pClientConfig->FONT_HEIGHT;
		const int FontHeight2 = FontHeight << 1;

		bool bDrawName = true;

		//-----------------------------------------------------
		// Level Name?? ??? ???
		//-----------------------------------------------------
		if (pCreature->HasLevelName())
		{
			const COLORREF levelNameColor = RGB(200, 250, 200);

			int yPoint_40 = yPoint - FontHeight2 - 3;

			if (bDrawChatString && yPoint_40 < FontHeight2)
				bDrawName = false;

			if (bDrawName)
			{
				DRAWTEXT_NODE* pLevelNameNode = new DRAWTEXT_NODE(
					pPoint->x + 1,
					yPoint_40 + 3,//2+1,
					pCreature->GetLevelName(),
					levelNameColor,
					font
				);

				AddText(pLevelNameNode);
			}
		}

		int yPoint_20 = yPoint - 23;	// g_pClientConfig->FONT_HEIGHT

		if (bDrawChatString && yPoint_20 < FontHeight)
			bDrawName = false;

		if (bDrawName)
		{

			//int MAX_HP_BAR = pCreature->GetHPBarWidth(); //g_pClientConfig->MAX_HP_BAR_PIXEL;
			int POSITION_HP_BAR = g_pClientConfig->POSITION_HP_BAR;

			int namePixel = g_GetStringWidth(pName, g_ClientPrintInfo[font]->hfont);
			int MAX_HP_BAR = max(100, namePixel + 20);

			int guildID = 0;
			//-----------------------------------------------------
			// ??? ??? ?? ??ü(???) ????? ???
			//-----------------------------------------------------
			int rectLeft = pPoint->x + POSITION_HP_BAR - 14;
			int rectRight = rectLeft + MAX_HP_BAR;//m_pointChatString.x+5 + namePixel;
			int rectTop = yPoint_20;
			int rectBottom = yPoint_20 + g_pClientConfig->FONT_HEIGHT;

			//-----------------------------------------------------
			// Hallu?? ????? ?????? ????? ??????
			//-----------------------------------------------------
			if (!g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION)
				&& pCreature->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR
				&& pCreature->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR
				&& pCreature->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR
				&& pCreature->GetCreatureType() != 672
				&& pCreature->GetCreatureType() != 673
				)
			{

				//-----------------------------------------------------
				// Guild?? ??? ??? - Guild Mark ???
				//-----------------------------------------------------
				guildID = pCreature->GetGuildNumber();
				if (g_pPlayer->GetRace() != pCreature->GetRace() && pCreature->IsPlayer() &&
					g_pZoneTable->Get(g_pZone->GetID())->ChatMaskByRace == true
					)
					guildID = 0;

				// grade???
				int gradeID = pCreature->GetGrade() - 1;	// 1~50 ???? ????????? -1??? 0~49?? ?????
				// ?÷???? ??? ???, ???? ????? ????
				if (!pCreature->IsPlayerOnly())
					gradeID = -1;

				// A monster carries its level on a badge left of the name plate, the
				// way the renewal art has it. Monsters ONLY: never a player, a town NPC, a
				// neutral (Competence 0) creature or a pet. The badge takes the spot
				// a clan mark would otherwise use.
				const bool bMonsterLevelBox =
					!pCreature->IsNPC()
					&& pCreature->GetCompetence() != 0
					&& !pCreature->IsPlayerOnly()
					&& pCreature->GetClassType() != MCreature::CLASS_FAKE
					&& pCreature->GetLevel() > 0;

				if (bMonsterLevelBox)
				{
					guildID = 0;

					// The badge art: sprite 0 is green, sprite 1 red, both 25x20.
					// Loaded on first use. Without the art there is no badge at all.
					static CSpritePack s_MonsterLevelSPK;
					static bool s_bMonsterLevelTried = false;
					if (!s_bMonsterLevelTried)
					{
						s_bMonsterLevelTried = true;
						if (!s_MonsterLevelSPK.LoadFromFile("data\\ui\\spk\\monsterlevel.spk"))
							s_MonsterLevelSPK.Release();
					}

					// No art, no badge. The plain drawn box that used to stand in for a
					// missing monsterlevel.spk looked poor, so it shows nothing instead.
					const int MONSTER_LEVEL_BADGE_GREEN = 0;
					const int MONSTER_LEVEL_BADGE_RED = 1;

					if (s_MonsterLevelSPK.GetSize() > MONSTER_LEVEL_BADGE_RED)
					{
						// Red: the monster is more than 5 levels above the player. Green: it
						// is within the player's range or below it. The player's level is
						// worked out exactly as the name colour below does it - a Slayer's
						// best domain level, otherwise the character level, plus advancement.
						int badgePlayerLevel = g_char_slot_ingame.level;

						if (g_pPlayer->IsSlayer())
						{
							badgePlayerLevel = max((*g_pSkillManager)[SKILLDOMAIN_SWORD].GetDomainLevel(),
												   (*g_pSkillManager)[SKILLDOMAIN_BLADE].GetDomainLevel());
							badgePlayerLevel = max(badgePlayerLevel, (*g_pSkillManager)[SKILLDOMAIN_GUN].GetDomainLevel());
							badgePlayerLevel = max(badgePlayerLevel, (*g_pSkillManager)[SKILLDOMAIN_HEAL].GetDomainLevel());
							badgePlayerLevel = max(badgePlayerLevel, (*g_pSkillManager)[SKILLDOMAIN_ENCHANT].GetDomainLevel());
						}

						badgePlayerLevel += g_char_slot_ingame.m_AdvancementLevel;

						const bool bBadgeRed = pCreature->GetLevel() > badgePlayerLevel + 5;
						CSprite* pBadge = &s_MonsterLevelSPK[bBadgeRed ? MONSTER_LEVEL_BADGE_RED : MONSTER_LEVEL_BADGE_GREEN];

						char szLevel[16];
						sprintf_s(szLevel, sizeof(szLevel), "%d", pCreature->GetLevel());

						const int levelPixel = g_GetStringWidth(szLevel, g_ClientPrintInfo[font]->hfont);
						const int levelBoxW = pBadge->GetWidth();
						const int levelBoxLeft = rectLeft - levelBoxW;

						POINT pointBadge = { levelBoxLeft,
											 rectTop + (((rectBottom - rectTop) - (int)pBadge->GetHeight()) >> 1) };

						m_pSurface->Lock();
						m_pSurface->BltSprite(&pointBadge, pBadge);
						m_pSurface->Unlock();

						// flush against the plate. The node keeps only a pointer,
						// so the digits need the heap copy
						const int levelX = levelBoxLeft + ((levelBoxW - levelPixel) >> 1);

						AddText(new DRAWTEXT_NODE_HEAP(levelX + 1, yPoint_20 + 4 + 1, szLevel, 0, font));
						AddText(new DRAWTEXT_NODE_HEAP(levelX, yPoint_20 + 4, szLevel, color, font));	// same colour as the name
					}
				}

				/*
				if (guildID>=0)
				{
				// run-time loading?? ?????? ??..
				if (guildID < m_GuildSPK.GetSize())
				{
				m_pSurface->Lock();

				  POINT pointTemp = { rectLeft-20, yPoint_20 };

					m_pSurface->BltSprite( &pointTemp, &m_GuildSPK[guildID] );

					  m_pSurface->Unlock();
					  }
					  }
			*/

			// CheckList
				if (guildID > 0)
				{
					//-------------------------------------------------
					// load??? ????? ????.
					//-------------------------------------------------
					CSprite* pSprite = g_pGuildMarkManager->GetGuildMarkSmall(guildID);

					if (pSprite != NULL)
					{
						m_pSurface->Lock();

						POINT pointTemp = { rectLeft - 20, yPoint_20 };

						m_pSurface->BltSprite(&pointTemp, pSprite);

						m_pSurface->Unlock();
					}
					else
					{
						if (!g_pGuildMarkManager->HasGuildMark(guildID))
						{
							//-------------------------------------------------
							// file?? ????? ????.
							//-------------------------------------------------
							g_pGuildMarkManager->LoadGuildMark(guildID);

							//-------------------------------------------------
							// file???? load??????? ??? ü?
							//-------------------------------------------------
							pSprite = g_pGuildMarkManager->GetGuildMark(guildID);

							//-------------------------------------------------
							// file???? ???? ???..
							// guildMark????????????? ???´?.
							//-------------------------------------------------
							//if (pSprite==NULL) {}
						}
					}
				}

				// ????? ??°?? ????? ???
				if (gradeID > -1 && gradeID <= GRADE_MARK_MAX)
				{
					CSprite* pSprite = g_pGuildMarkManager->GetGradeMarkSmall(gradeID, pCreature->GetRace());

					if (pSprite != NULL)
					{
						m_pSurface->Lock();

						POINT pointTemp = { rectRight, yPoint_20 };

						m_pSurface->BltSprite(&pointTemp, pSprite);

						m_pSurface->Unlock();
					}
				}

			}
			// ?????? ???? ??? ???
			if (pCreature->GetClassType() == MCreature::CLASS_FAKE)
			{
				MFakeCreature* pFakeCreature = (MFakeCreature*)pCreature;

				if (pFakeCreature->GetOwnerID() != OBJECTID_NULL)
				{
					MPetItem* pPetItem = pFakeCreature->GetPetItem();

					if (pPetItem != NULL)
					{
						int petLevel = pPetItem->GetNumber();
						if (petLevel > -1 && petLevel <= 50)
						{
							CSprite* pSprite = g_pGuildMarkManager->GetLevelMarkSmall(petLevel);

							if (pSprite != NULL)
							{
								m_pSurface->Lock();

								POINT pointTemp = { rectLeft - 20, yPoint_20 };

								m_pSurface->BltSprite(&pointTemp, pSprite);

								m_pSurface->Unlock();
							}
						}
					}
				}
			}

			//-----------------------------------------------------
			// HP???
			//-----------------------------------------------------
			int currentHP = max(0, int(pCreature->GetHP()));
			int maxHP = pCreature->GetMAX_HP();

#ifdef OUTPUT_DEBUG
			if (g_pDXInput->KeyDown(DIK_H) &&
				(g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
			{
				char str[128];
				sprintf(str, "HP=%d/%d", currentHP, maxHP);
				m_pSurface->GDI_Text(rectLeft, rectTop - 20, str, 0xFFFF);
			}
#endif

			//-----------------------------------------------------
			// ????? ??? ??? HP?? ?? ????ó?? ???????.
			//-----------------------------------------------------
			//if (pCreature->IsUndergroundCreature())
			//{
			//	currentHP = 1;
			//maxHP = 1;
			//}

			if (currentHP > maxHP)
			{
				currentHP = maxHP;
			}

			//-----------------------------------------------------
			// ?????? ?????? pixel???
			//-----------------------------------------------------
			int maxPixels = rectRight - rectLeft;
			int currentPixels = max(0, int((maxHP == 0) ? 0 : maxPixels * currentHP / maxHP));

			int nameX = //rectLeft+(MAX_HP_BAR>>1) - (namePixel>>1);
				rectLeft + ((MAX_HP_BAR - namePixel) >> 1);


			//-----------------------------------------------------
			// HP?? ?? ???? ??õ?? ???
			//-----------------------------------------------------
			if (currentPixels == maxPixels)
			{
				// ?????? ?? ??? ??????.. ??? ?? ????? ?????.. --
				DRAWTEXT_NODE* pNodeBase = new DRAWTEXT_NODE(
					nameX + 1,
					yPoint_20 + 4 + 1,
					NULL,//pName,
					0,
					font);

				AddText(pNodeBase);
			}
			//-----------------------------------------------------
			// HP ???? ??? ???
			//-----------------------------------------------------
			else
			{
				//-----------------------------------------------------
				//
				//			Max HP ????? Box?? ???
				//
				//-----------------------------------------------------
				DRAWTEXT_NODE* pNodeBase = new DRAWTEXT_NODE(
					nameX + 1,
					yPoint_20 + 4 + 1,
					NULL,//pName,
					0,
					font);

				//-----------------------------------------------------
				// HP ???? ???
				//-----------------------------------------------------
				RECT rect = { rectLeft + currentPixels,
					rectTop,
					rectRight,
					rectBottom
				};


				pNodeBase->SetBox(rect, m_ColorBlackHalf);
				AddText(pNodeBase);
			}

			//-----------------------------------------------------
			//
			//		???? HP????? Box?? ????? ??? ???
			//
			//-----------------------------------------------------
			// ???? HP????? Box?? ????? ??? ???(????)
			//-----------------------------------------------------	
			DRAWTEXT_NODE* pNode = new DRAWTEXT_NODE(
				nameX + 1,
				yPoint_20 + 4 + 1,
				pName,
				0,		//color,
				font);	//FLAG_DRAWTEXT_HANGUL_JOHAP);

			//-----------------------------------------------------
			// 0?? ???? box???
			//-----------------------------------------------------							
			if (currentPixels != 0)
			{
				//-----------------------------------------------------
				// ???? HP?? ???? ???
				//-----------------------------------------------------
				RECT rectHP = { rectLeft,
					rectTop,
					rectLeft + currentPixels,
					rectBottom
				};

				//-----------------------------------------------------
				// Current HP
				//-----------------------------------------------------				
				if (g_pUserOption->DrawTransHPBar)
				{
					pNode->SetBox(rectHP, m_ColorHPBar, true);
				}
				else
				{
					// ???????? HP bar?? ??????
					static WORD notTransHPBar
						= CDirectDraw::Color(g_pClientConfig->COLOR_HP_BAR_R >> 1,
							g_pClientConfig->COLOR_HP_BAR_G >> 1,
							g_pClientConfig->COLOR_HP_BAR_B >> 1);

					pNode->SetBox(rectHP, notTransHPBar, false);
				}
			}

			//-----------------------------------------------------
			// ?????? ????..
			//-----------------------------------------------------
			AddText(pNode);

			// 2004, 10, 28, sobeit add start -???? ? ????? ??? ????? ???.
			//		if(pCreature->IsAlive() && UI_IsMonsterKillQuest_Monster((*g_pCreatureTable)[pCreature->GetCreatureType()].SpriteTypes[0]))
			//		{
			//			WORD temp_color;
			//			temp_color = CIndexSprite::ColorSet[MItem::GetQuestItemColorset()][15];
			//			color  = RGB(CDirectDraw::Red(temp_color)<<3, CDirectDraw::Green(temp_color)<<3, CDirectDraw::Blue(temp_color)<<3);
			//		}
			// 2004, 10, 28, sobeit add end
			//-----------------------------------------------------
			// ??? ??? ???
			//-----------------------------------------------------
			DRAWTEXT_NODE* pNode2 = new DRAWTEXT_NODE(
				nameX,
				yPoint_20 + 4,
				pName,
				color,
				font);	//FLAG_DRAWTEXT_HANGUL_JOHAP);

			AddText(pNode2);
			// 2004, 6, 17, sobeit add start - about nick name - ???? ???? ????? ????? ?????..????.
			if (pCreature->GetNickNameType() != NicknameInfo::NICK_NONE)// && (/*g_pPlayer->GetRace() == pCreature->GetRace()|| */pCreature->IsFakeCreature()))
			{
				if (yPoint - 42 < 0) return FALSE; // ?? ???? ????..????
				BYTE bType = pCreature->GetNickNameType();
				const char* szNickName = pCreature->GetNickName().c_str();
				if (strlen(szNickName) > 0)
				{
					font = FONTID_USER_ID;

					namePixel = g_GetStringWidth(szNickName, g_ClientPrintInfo[font]->hfont);
					WORD bgColor = 0;

					switch (bType)
					{
					case NicknameInfo::NICK_BUILT_IN:
						color = RGB_YELLOW;
						bgColor = CDirectDraw::Color(255, 0, 0);

						break;
					case NicknameInfo::NICK_QUEST:
						color = RGB(150, 150, 150);
						bgColor = CDirectDraw::Color(0, 0, 255);
						break;
					case NicknameInfo::NICK_FORCED:
					case NicknameInfo::NICK_CUSTOM_FORCED:
						color = RGB_RED;
						break;
					case NicknameInfo::NICK_CUSTOM:
						color = RGB(50, 170, 230);
						//	bgColor = CDirectDraw::Color(255,0,0);
						break;
					case NicknameInfo::NICK_QUEST_2:
						color = RGB(0, 255, 128);
						bgColor = CDirectDraw::Color(0, 128, 255);
						break;
					default:
						color = RGB_YELLOW;
						break;
					}
					//	if(guildID >0) // ?????? ???????? ??????? hpbar+????? ???? ???
					//		nameX = rectLeft + ((MAX_HP_BAR - namePixel+20)>>1);
					//	else // ???? hpbar ???? ??´?
					nameX = rectLeft + ((MAX_HP_BAR - namePixel) >> 1);

					DRAWTEXT_NODE* pNode = new DRAWTEXT_NODE(
						nameX,
						yPoint - 38,
						szNickName,
						color,		//color,
						font);	//FLAG_DRAWTEXT_HANGUL_JOHAP);
					if (bgColor != 0)
					{
						//					RECT rectHP = {	nameX-2, 
						//									yPoint-42,
						//									nameX + namePixel+4,
						//									yPoint-23	};

						RECT rectHP = { rectLeft - 20,
							yPoint - 42,
							rectRight + 20 ,
							yPoint - 23 };

						if (bType == NicknameInfo::NICK_QUEST_2 || bType == NicknameInfo::NICK_QUEST_2)
						{
							rectHP.left -= 5;
							rectHP.right += 5;
						}

						if (g_pUserOption->DrawTransHPBar)
						{
							pNode->SetBox(rectHP, bgColor, true);
						}
						else
						{
							pNode->SetBox(rectHP, bgColor, false);
						}
					}
					AddText(pNode);
				}
			}
		}
		// 2004, 6, 17, sobeit add end - about nick name

	}

	return TRUE;
}
void//m_bDrawRequest?? ??? ??? ???? ???? false?? ????? ?????? ?????? ????? ??????? ??? ??? ???????...
MTopView::DrawRequestModeMousePointer(POINT* pPoint, MCreature* pCreature)
{
	POINT& point = *pPoint;
	m_bDrawRequest = false;

	if (IsRequestMode())
	{
		bool bRequest = false;

		if (pCreature->GetClassType() == MCreature::CLASS_FAKE)
		{
			MFakeCreature* pFakeCreature = (MFakeCreature*)pCreature;
			if (pFakeCreature->GetOwnerID() != OBJECTID_NULL)	// ?????
			{
				bRequest = true;
			}
		}
		//------------------------------------------------
		// ?????? ?????? ????.
		//------------------------------------------------			
		else if (// NPC?? ????
			!pCreature->IsNPC()
			// Player?? ???
			//&& pCreature->GetCreatureType()<=CREATURETYPE_VAMPIRE_FEMALE
			//&& (*g_pCreatureSpriteTable)[(*g_pCreatureTable)[pCreature->GetCreatureType()].SpriteType].IsPlayerOnlySprite()
			&& (*g_pCreatureSpriteTable)[(*g_pCreatureTable)[pCreature->GetCreatureType()].SpriteTypes[0]].IsPlayerOnlySprite()
			// ?? ??? ??? ???
			&& !pCreature->IsInCasket()
			// ???? ?????????? ??? ???? ??? ?? ??????...
			//&& !pCreature->IsUndergroundCreature()
			// ??????.. - -;
			&& pCreature->IsAlive()
			&& pCreature->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR
			&& pCreature->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR
			&& pCreature->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR
			)
		{
			bRequest = true;
		}

		if (bRequest == true)
		{
			POINT pointTemp;
			TYPE_SPRITEID	RequestSpriteID;

			//---------------------------------------------------------------
			// trade ??û??? ??????
			//---------------------------------------------------------------
			if (IsRequestTrade())
			{
				const int maxRequestFrames = SPRITEID_ITEM_TRADE_LAST - SPRITEID_ITEM_TRADE + 1;
				RequestSpriteID = SPRITEID_ITEM_TRADE + ((g_CurrentFrame >> 1) % (maxRequestFrames << 1));

				if (RequestSpriteID > SPRITEID_ITEM_TRADE_LAST)
				{
					RequestSpriteID = SPRITEID_ITEM_TRADE_LAST - (RequestSpriteID - SPRITEID_ITEM_TRADE_LAST);

					if (RequestSpriteID < SPRITEID_ITEM_TRADE)
					{
						RequestSpriteID = SPRITEID_ITEM_TRADE;
					}
				}
			}
			//---------------------------------------------------------------
			// party ??û??? ??????
			//---------------------------------------------------------------
			else if (IsRequestParty())
			{
				const int maxRequestFrames = SPRITEID_PARTY_REQUEST_LAST - SPRITEID_PARTY_REQUEST + 1;
				RequestSpriteID = SPRITEID_PARTY_REQUEST + ((g_CurrentFrame >> 1) % (maxRequestFrames + 4));

				if (RequestSpriteID > SPRITEID_PARTY_REQUEST_LAST)
				{
					RequestSpriteID = SPRITEID_PARTY_REQUEST_LAST;
				}
			}
			//---------------------------------------------------------------
			// Info ??û??? ??????
			//---------------------------------------------------------------
			else if (IsRequestInfo())
			{
				const int maxRequestFrames = SPRITEID_INFO_REQUEST_LAST - SPRITEID_INFO_REQUEST + 1;
				RequestSpriteID = SPRITEID_INFO_REQUEST + ((g_CurrentFrame >> 1) % (maxRequestFrames + 4));

				if (RequestSpriteID > SPRITEID_INFO_REQUEST_LAST)
				{
					RequestSpriteID = SPRITEID_INFO_REQUEST_LAST;
				}
			}

			pointTemp.x = g_x + 27 - m_EtcSPK[RequestSpriteID].GetWidth();
			pointTemp.y = g_y + 15 - m_EtcSPK[RequestSpriteID].GetHeight();

			m_pSurface->Lock();
			m_pSurface->BltSprite(&pointTemp, &m_EtcSPK[RequestSpriteID]);
			m_pSurface->Unlock();

			m_bDrawRequest = true;
		}
	}
}


// 2005. 12. 26 by chyaya
//----------------------------------------------------------------------
// DrawCreatureNameOverText
//----------------------------------------------------------------------
// DrawCreatureName???? ???,?????? ?????????
// ?????? ???? ??µ?? ??????
// ????? ???? ??µ??? ?? ??? ???? ??? ???? ???? ??´?.
// ?????? ??Ÿ?? ??? ???? -_-);;
//----------------------------------------------------------------------
void
MTopView::DrawCreatureNameOverText(POINT* pPoint, MCreature* pCreature)
{
	if (pCreature != NULL && pCreature->IsObject()) return;
	POINT point = *pPoint;
	int yPoint = DrawChatString(&point,
		pCreature,
		RGB_WHITE,	//CDirectDraw::Color(31,31,31),
		FLAG_DRAWTEXT_OUTLINE);
	int yPoint_20 = yPoint - 23;	// g_pClientConfig->FONT_HEIGHT

	bool bMasterWords = false;

	bool bHalluName = (g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION) ||
		!bMasterWords &&
		!g_pUserInformation->IsMaster &&
		(pCreature->GetClassType() == MCreature::CLASS_PLAYER ||
			pCreature->GetClassType() == MCreature::CLASS_CREATUREWEAR) &&
		g_pPlayer->GetRace() != pCreature->GetRace()) &&
		g_pZoneTable->Get(g_pZone->GetID())->ChatMaskByRace &&
		g_pPlayer->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR &&
		g_pPlayer->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR &&
		g_pPlayer->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR;						// ê ??????? ???? ????? 

	FONTID		font = FONTID_USER_ID;
	const char* pName = (bHalluName ? pCreature->GetHalluName() : pCreature->GetName());

	int namePixel = g_GetStringWidth(pName, g_ClientPrintInfo[font]->hfont);
	int POSITION_HP_BAR = g_pClientConfig->POSITION_HP_BAR;
	int MAX_HP_BAR = max(100, namePixel + 20);

	//-----------------------------------------------------
	// ??? ??? ?? ??ü(???) ????? ???
	//-----------------------------------------------------
	int rectLeft = pPoint->x + POSITION_HP_BAR - 14;
	int rectRight = rectLeft + MAX_HP_BAR;//m_pointChatString.x+5 + namePixel;
	int rectTop = yPoint_20;
	int rectBottom = yPoint_20 + g_pClientConfig->FONT_HEIGHT;


	if (!g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION) &&
		pCreature->GetCreatureType() != CREATURETYPE_VAMPIRE_OPERATOR &&
		pCreature->GetCreatureType() != CREATURETYPE_SLAYER_OPERATOR &&
		pCreature->GetCreatureType() != CREATURETYPE_OUSTERS_OPERATOR &&
		pCreature->GetCreatureType() != 672 &&
		pCreature->GetCreatureType() != 673
		)
	{
		const int ORIGIN_MARK_WIDTH = 11;
		const int ORIGIN_MARK_Y_FIX = 2;
		const int ORIGIN_MARK_X_FIX = 2;

		/*
		if(!pCreature->IsPlayer())
		{
			POINT	pointTemp;

			CSprite* pSprite = g_pGuildMarkManager->GetOriginMark(0);

			m_pSurface->Lock();

			pointTemp.x		= rectLeft + ORIGIN_MARK_X_FIX;						// ????
			pointTemp.y		= yPoint_20 + ORIGIN_MARK_Y_FIX;
			m_pSurface->BltSpriteColorSet(&pointTemp, pSprite, 80);

			pointTemp.x		= rectRight-ORIGIN_MARK_WIDTH-ORIGIN_MARK_X_FIX;	// ??????
			pointTemp.y		= yPoint_20 + ORIGIN_MARK_Y_FIX;
			m_pSurface->BltSpriteColorSet(&pointTemp, pSprite, 80);

			m_pSurface->Unlock();
		}
		*/


		//		if(pCreature->GetClassType() == MCreature::CLASS_PLAYER)	// ?÷?????? ?????
		if (pCreature->IsPlayer() && !pCreature->IsNPC() &&
			g_pPlayer->GetRace() == pCreature->GetRace())
		{

			//////////////////////////////////////////////////////////////////////////
			// 2005.12.23 chyaya
			// ??Ÿ?? ???
			// ???? ??????, ??????, ????????? ????? ?????? ??????.
			//

			int originmark = pCreature->GetOriginServerNum();

			// ??Ÿ???? ????????? ???
			if (originmark == ORIGIN_MOLDAVIA ||
				originmark == ORIGIN_WALAKIA ||
				originmark == ORIGIN_HISTRIA)
			{
				const int ORIGIN_MARK_SPR_IDX[ORIGIN_MAX] = {
					-1, 0, -1, 1, 2, -1,	// ??? ????????? ????? ???
				};

				CSprite* pSprite = g_pGuildMarkManager->GetOriginMark(
					ORIGIN_MARK_SPR_IDX[originmark]);

				if (pSprite != NULL)
				{
					POINT	pointTemp;

					m_pSurface->Lock();

					pointTemp.x = rectLeft + ORIGIN_MARK_X_FIX;						// ????
					pointTemp.y = yPoint_20 + ORIGIN_MARK_Y_FIX;
					m_pSurface->BltSpriteAlpha(&pointTemp, pSprite, 12);

					pointTemp.x = rectRight - ORIGIN_MARK_WIDTH - ORIGIN_MARK_X_FIX;	// ??????
					pointTemp.y = yPoint_20 + ORIGIN_MARK_Y_FIX;
					m_pSurface->BltSpriteAlpha(&pointTemp, pSprite, 12);

					m_pSurface->Unlock();
				}
			}

			//
			//////////////////////////////////////////////////////////////////////////
		}

	}


}


//----------------------------------------------------------------------
// DrawCreatureName
//----------------------------------------------------------------------
// DrawNameCreatureInfo?? ??????? ?????? DrawCreatureName()
//----------------------------------------------------------------------
BOOL MTopView::DrawCreatureName(DrawNameCreatureInfo* pDNCInfo)
{
	BOOL rtn = DrawCreatureName(&pDNCInfo->ptDrawPostion, pDNCInfo->pCreature);
	pDNCInfo->bOverText = rtn ? true : false;
	return rtn;
}



//----------------------------------------------------------------------
// DrawCreatureNameOverText
//----------------------------------------------------------------------
// DrawNameCreatureInfo?? ??????? ?????? DrawCreatureNameOverText()
//----------------------------------------------------------------------
void MTopView::DrawCreatureNameOverText(DrawNameCreatureInfo* pDNCInfo)
{
	DrawCreatureNameOverText(&pDNCInfo->ptDrawPostion, pDNCInfo->pCreature);
}


//----------------------------------------------------------------------
// GetChangeValueToDirection
//----------------------------------------------------------------------
// ???? ?????? ????? ???????.. - -;
//----------------------------------------------------------------------
POINT
MTopView::GetChangeValueToDirection(int direction)
{
	POINT pt = { 0, 0 };

	switch (direction)
	{
	case DIRECTION_LEFTDOWN: pt.x = -1;	pt.y = 1;	break;
	case DIRECTION_RIGHTUP: pt.x = 1;	pt.y = -1;	break;
	case DIRECTION_LEFTUP: pt.x = -1;	pt.y = -1;	break;
	case DIRECTION_RIGHTDOWN: pt.x = 1;	pt.y = 1;	break;
	case DIRECTION_LEFT: pt.x = -1;			break;
	case DIRECTION_DOWN: pt.y = 1;	break;
	case DIRECTION_UP: pt.y = -1;	break;
	case DIRECTION_RIGHT: pt.x = 1;			break;
	}

	return pt;
}


void
MTopView::DrawUndergroundCreature(POINT* pPoint, MCreature* pCreature)
{
	//----------------------------------------
	// ?????? burrow???  -_-;; 
	//----------------------------------------
	POINT pointTemp;
	RECT rect;

	CSprite* pSprite = &m_EtcSPK[SPRITEID_CREATURE_BURROW];

	//----------------------------------------
	// ??? ????
	//----------------------------------------
	int cx = 4;
	int cy = 4;
	pointTemp.x = pPoint->x + cx;
	pointTemp.y = pPoint->y + cy;

	//---------------------------------------- 		
	// ????? ???? ???? ???? ????
	//---------------------------------------- 	
	rect.left = pointTemp.x;
	rect.top = pointTemp.y;
	rect.right = rect.left + pSprite->GetWidth();
	rect.bottom = rect.top + pSprite->GetHeight();
	pCreature->SetScreenRect(&rect);

	//---------------------------------------- 	
	// ???ó ????? ??????? 
	//---------------------------------------- 	
	if (pCreature->IsShowNameCreature())	// ???? ?????? ??
	{
		AddDrawNameCreature(pPoint, pCreature);
	}
	else if (m_SelectCreatureID == pCreature->GetID())
	{
		// SpriteOutlineManager?? ???

		// ????? ??? ????? ?????? vampire???.
		if (g_pObjectSelector->CanAttack(pCreature))
		{
			m_SOMOutlineColor = m_ColorOutlineAttackPossible;
		}
		else
		{
			m_SOMOutlineColor = m_ColorOutlineAttackImpossible;
		}

		m_SOM.Add(pointTemp.x, pointTemp.y, pSprite);
		m_SOM.Generate();

		m_pSurface->BltSpriteOutline(&m_SOM, m_SOMOutlineColor);

		AddDrawNameCreature(pPoint, pCreature);
		m_pSelectedCreature = pCreature;
	}
	//----------------------------------------
	// ??? ???
	//----------------------------------------
	else
	{
		m_pSurface->BltSprite(&pointTemp, pSprite);
	}

	//----------------------------------------
	// ???? ???? Effect ???
	//----------------------------------------
	/*
	if (pCreature->IsExistAttachEffect())
	{
	pointTemp = *pPoint;
	//pointTemp.x += pCreature->GetSX();
	//pointTemp.y += pCreature->GetSY();

	  DrawAttachEffect(&pointTemp, pCreature->GetAttachEffectIterator(), pCreature->GetAttachEffectSize());
	  }
	*/
}

BOOL
MTopView::DrawCreatureMyName()
{
	// EFFECTSTATUS_GHOST ?? ????????? ???? ?????? ???? ??????
	if (g_pPlayer->HasEffectStatus(EFFECTSTATUS_GHOST)) return FALSE;

	const char* pCreatureName;
	pCreatureName = g_pUserInformation->CharacterID.GetString();
	bool bMasterWords = false;
	COLORREF	color;
	FONTID		font = FONTID_USER_ID;

	if (pCreatureName != NULL)
	{
		int alignment = g_char_slot_ingame.alignment;
		int POSITION_HP_BAR = g_pClientConfig->POSITION_HP_BAR;

		color = m_ColorNameAlignment[alignment];
		POINT TempPoint = { g_pPlayer->GetPixelX() - m_FirstZonePixel.x , g_pPlayer->GetPixelY() - g_pPlayer->GetHeight() - m_FirstZonePixel.y };
		const int firstY = DrawChatString(&TempPoint, g_pPlayer, g_pPlayer->GetChatColor());//CDirectDraw::Color(26,26,26));		

		int yPoint = firstY/*g_pPlayer->GetPixelY() - g_pPlayer->GetHeight() - m_FirstZonePixel.y*/;
		int rectLeft, rectRight, rectTop, rectBottom;
		int gradeID = 0;
		int guildID = 0;

		int yPoint_20 = yPoint - 23;	// g_pClientConfig->FONT_HEIGHT

		int namePixel = g_GetStringWidth(pCreatureName, g_ClientPrintInfo[font]->hfont);
		int MAX_HP_BAR = max(100, namePixel + 20);


		//-----------------------------------------------------
		// ??? ??? ?? ??ü(???) ????? ???
		//-----------------------------------------------------
		{
			rectLeft = g_pPlayer->GetPixelX() - m_FirstZonePixel.x + POSITION_HP_BAR - 14;
			rectRight = rectLeft + MAX_HP_BAR;//m_pointChatString.x+5 + namePixel;
			rectTop = yPoint_20;
			rectBottom = yPoint_20 + g_pClientConfig->FONT_HEIGHT;
		}
		//-----------------------------------------------------
		// Hallu?? ????? ?????? ????? ??????
		//-----------------------------------------------------
		if (!g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION))
		{
			//-----------------------------------------------------
			// Guild?? ??? ??? - Guild Mark ???
			//-----------------------------------------------------
			guildID = g_pPlayer->GetGuildNumber();

			gradeID = g_pPlayer->GetGrade() - 1;	// 1~50 ???? ????????? -1??? 0~49?? ?????
			if (!g_pPlayer->IsPlayerOnly())
				gradeID = -1;

			if (guildID > 0)
			{
				//-------------------------------------------------
				// load??? ????? ????.
				//-------------------------------------------------
				CSprite* pSprite = g_pGuildMarkManager->GetGuildMarkSmall(guildID);

				if (pSprite != NULL)
				{
					m_pSurface->Lock();

					POINT pointTemp = { rectLeft - 20, yPoint_20 };

					m_pSurface->BltSprite(&pointTemp, pSprite);

					m_pSurface->Unlock();
				}
				else
				{
					if (!g_pGuildMarkManager->HasGuildMark(guildID))
					{
						g_pGuildMarkManager->LoadGuildMark(guildID);
						pSprite = g_pGuildMarkManager->GetGuildMark(guildID);

					}
				}
			}

			// ????? ??°?? ????? ???
			if (gradeID > -1 && gradeID <= GRADE_MARK_MAX)
			{
				CSprite* pSprite = g_pGuildMarkManager->GetGradeMarkSmall(gradeID, g_pPlayer->GetRace());

				if (pSprite != NULL)
				{
					m_pSurface->Lock();

					POINT pointTemp = { rectRight, yPoint_20 };

					m_pSurface->BltSprite(&pointTemp, pSprite);

					m_pSurface->Unlock();
				}
			}

		}
		//-----------------------------------------------------
		// HP???
		//-----------------------------------------------------
		int currentHP = max(0, int(g_pPlayer->GetHP()));
		int maxHP = g_pPlayer->GetMAX_HP();

#ifdef OUTPUT_DEBUG
		if (g_pDXInput->KeyDown(DIK_H) &&
			(g_pDXInput->KeyDown(DIK_LCONTROL) || g_pDXInput->KeyDown(DIK_RCONTROL)))
		{
			char str[128];
			sprintf(str, "HP=%d/%d", currentHP, maxHP);
			m_pSurface->GDI_Text(rectLeft, rectTop - 20, str, 0xFFFF);
		}
#endif

		if (currentHP > maxHP)
		{
			currentHP = maxHP;
		}

		//-----------------------------------------------------
		// ?????? ?????? pixel???
		//-----------------------------------------------------
		int maxPixels = rectRight - rectLeft;
		int currentPixels = max(0, int((maxHP == 0) ? 0 : maxPixels * currentHP / maxHP));

		int nameX = //rectLeft+(MAX_HP_BAR>>1) - (namePixel>>1);
			rectLeft + ((MAX_HP_BAR - namePixel) >> 1);


		//-----------------------------------------------------
		// HP?? ?? ???? ??õ?? ???
		//-----------------------------------------------------
		if (currentPixels == maxPixels)
		{
			// ?????? ?? ??? ??????.. ??? ?? ????? ?????.. --
			DRAWTEXT_NODE* pNodeBase = new DRAWTEXT_NODE(
				nameX + 1,
				yPoint_20 + 4 + 1,
				NULL,//pName,
				0,
				font);

			AddText(pNodeBase);
		}
		//-----------------------------------------------------
		// HP ???? ??? ???
		//-----------------------------------------------------
		else
		{
			//-----------------------------------------------------
			//
			//			Max HP ????? Box?? ???
			//
			//-----------------------------------------------------
			DRAWTEXT_NODE* pNodeBase = new DRAWTEXT_NODE(
				nameX + 1,
				yPoint_20 + 4 + 1,
				NULL,//pName,
				0,
				font);

			//-----------------------------------------------------
			// HP ???? ???
			//-----------------------------------------------------
			RECT rect = { rectLeft + currentPixels,
							rectTop,
							rectRight,
							rectBottom
			};


			pNodeBase->SetBox(rect, m_ColorBlackHalf);
			AddText(pNodeBase);
		}

		//-----------------------------------------------------
		//
		//		???? HP????? Box?? ????? ??? ???
		//
		//-----------------------------------------------------
		// ???? HP????? Box?? ????? ??? ???(????)
		//-----------------------------------------------------			
		DRAWTEXT_NODE* pNode = new DRAWTEXT_NODE(
			nameX + 1,
			yPoint_20 + 4 + 1,
			pCreatureName,
			0,		//color,
			font);	//FLAG_DRAWTEXT_HANGUL_JOHAP);

		//-----------------------------------------------------
		// 0?? ???? box???
		//-----------------------------------------------------							
		if (currentPixels != 0)
		{
			//-----------------------------------------------------
			// ???? HP?? ???? ???
			//-----------------------------------------------------
			RECT rectHP = { rectLeft,
							rectTop,
							rectLeft + currentPixels,
							rectBottom
			};

			//-----------------------------------------------------
			// Current HP
			//-----------------------------------------------------				
			if (g_pUserOption->DrawTransHPBar)
			{
				pNode->SetBox(rectHP, m_ColorHPBar, true);
			}
			else
			{
				// ???????? HP bar?? ??????
				static WORD notTransHPBar
					= CDirectDraw::Color(g_pClientConfig->COLOR_HP_BAR_R >> 1,
						g_pClientConfig->COLOR_HP_BAR_G >> 1,
						g_pClientConfig->COLOR_HP_BAR_B >> 1);

				pNode->SetBox(rectHP, notTransHPBar, false);
			}
		}

		//-----------------------------------------------------
		// ?????? ????..
		//-----------------------------------------------------
		AddText(pNode);


		//-----------------------------------------------------
		// ??? ??? ???
		//-----------------------------------------------------
		DRAWTEXT_NODE* pNode2 = new DRAWTEXT_NODE(
			nameX,
			yPoint_20 + 4,
			pCreatureName,
			color,
			font);	//FLAG_DRAWTEXT_HANGUL_JOHAP);

		AddText(pNode2);
		// 2004, 6, 17, sobeit add start - about nick name
		if (g_pPlayer->GetNickNameType() != NicknameInfo::NICK_NONE)
		{
			BYTE bType = g_pPlayer->GetNickNameType();
			const char* szNickName = g_pPlayer->GetNickName().c_str();
			if (strlen(szNickName) > 0)
			{
				namePixel = g_GetStringWidth(szNickName, g_ClientPrintInfo[font]->hfont);
				WORD bgColor = 0;

				switch (bType)
				{
				case NicknameInfo::NICK_BUILT_IN:
					color = RGB_YELLOW;
					bgColor = CDirectDraw::Color(255, 0, 0);

					break;
				case NicknameInfo::NICK_QUEST:
					color = RGB(150, 150, 150);
					bgColor = CDirectDraw::Color(0, 0, 255);
					break;
				case NicknameInfo::NICK_FORCED:
				case NicknameInfo::NICK_CUSTOM_FORCED:
					color = RGB_RED;
					break;
				case NicknameInfo::NICK_CUSTOM:
					color = RGB(50, 170, 230);
					//	bgColor = CDirectDraw::Color(255,0,0);
					break;
				case NicknameInfo::NICK_QUEST_2:
					color = RGB(0, 255, 128);
					bgColor = CDirectDraw::Color(0, 128, 255);
					break;
				default:
					color = RGB_YELLOW;
					break;
				}
				//	if(guildID >0) // ?????? ???????? ??????? hpbar+????? ???? ???
				//		nameX = rectLeft + ((MAX_HP_BAR - namePixel+20)>>1);
				//	else // ???? hpbar ???? ??´?
				nameX = rectLeft + ((MAX_HP_BAR - namePixel) >> 1);

				DRAWTEXT_NODE* pNode = new DRAWTEXT_NODE(
					nameX,
					yPoint - 38,
					szNickName,
					color,		//color,
					font);	//FLAG_DRAWTEXT_HANGUL_JOHAP);
				if (bgColor != 0)
				{
					//					RECT rectHP = {	nameX-2, 
					//									yPoint-42,
					//									nameX + namePixel+4,
					//									yPoint-23	};

					RECT rectHP = { rectLeft - 20,
									yPoint - 42,
									rectRight + 20 ,
									yPoint - 23 };

					if (bType == NicknameInfo::NICK_QUEST_2 || bType == NicknameInfo::NICK_QUEST_2)
					{
						rectHP.left -= 5;
						rectHP.right += 5;
					}

					if (g_pUserOption->DrawTransHPBar)
					{
						pNode->SetBox(rectHP, bgColor, true);
					}
					else
					{
						pNode->SetBox(rectHP, bgColor, false);
					}
				}
				AddText(pNode);
			}

		}
		// 2004, 6, 17, sobeit add end - about nick name
	}

	return TRUE;
}


bool
MTopView::DrawEvent()
{
	bool bDrawBackGround = true;
	MEvent* AdvancementQuestEndingEvent = const_cast<MEvent*>(g_pEventManager->GetEvent(EVENTID_ADVANCEMENT_QUEST_ENDING));
	if (AdvancementQuestEndingEvent != NULL)
	{
		bDrawBackGround = ExcuteAdvancementQuestEnding((void*)AdvancementQuestEndingEvent);
	}
	else {
		MEvent* LostChildPrologueEvent = const_cast<MEvent*>(g_pEventManager->GetEvent(EVENTID_LOST_CHILD_PROLOGUE));

		if (LostChildPrologueEvent != NULL)
		{
			bDrawBackGround = ExcuteLostChildPrologue((void*)LostChildPrologueEvent);
		}
		else
		{
			bDrawBackGround = ExcuteOustersFinEvent();
		}
	}

	// 2005, 2, 18, sobeit add start - WebBrowser?? ?????? ?? ó??
	if (UI_IsRunning_WebBrowser())
	{
		//bDrawBackGround = false; // ????? ???? ??´?. UI?? ??´?..
		int TempValue = UI_GetMouseCursorInfo_WebBrowser();

		if (TempValue == 1) // ????? ?? ??? ?????? ????
		{
			UI_ShowWindowCursor(); // ??? ????????..
		}
		else if (TempValue == -1) // ????? ?? ??? ?????? ??????.
		{
			UI_HiddenWindowCursor(); // ??? ????????.
		}
		//		if(UI_IsMouseIn_WebBrowser())
		//		{
		//			
		//			CURSORINFO _CursorInfo;
		//			GetCursorInfo(&_CursorInfo);
		//			if(_CursorInfo.flags != CURSOR_SHOWING)
		//				ShowCursor( TRUE );
		//		}
		//		else
		//		{
		//			CURSORINFO _CursorInfo;
		//			GetCursorInfo(&_CursorInfo);
		//			if(_CursorInfo.flags == CURSOR_SHOWING)
		//				ShowCursor( FALSE );
		//		}

			// 2005, 2, 18, sobeit add end	
	}
	return bDrawBackGround;
}


bool
MTopView::ExcuteLostChildPrologue(void* pVoid)
{
	MEvent* AdvancementQuestEndingEvent = (MEvent*)pVoid;

	if (AdvancementQuestEndingEvent != NULL)
	{
		bool bFinEnd = false;

		int SpkIndex = AdvancementQuestEndingEvent->parameter4;
		if (m_AdvacementQuestEnding.GetSize() == 0)
		{
			//ivfstream	FinFile;
			//if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_ADVANCEMENT_QUEST").c_str(), FinFile))
			//	return false;
			//m_AdvacementQuestEnding.LoadFromFile(FinFile);
			//FinFile.close();

			if (!m_AdvacementQuestEnding.LoadFromFile(g_pFileDef->getProperty("FILE_SPRITE_ADVANCEMENT_QUEST").c_str()))
				return false;
		}
		const TYPE_SOUNDID soundID = SOUND_SLAYER_ENCHANT_B2;

		static BYTE fadeColor[3] = { 255, 255, 255 };
		static int fadeSpeed = 10;
		static int fadeDirect = 0;
		POINT pointZero = { 0, 0 };

		if (AdvancementQuestEndingEvent->parameter1 == 0)
		{
			fadeColor[0] = 255;
			fadeColor[1] = 255;
			fadeColor[2] = 255;
			fadeSpeed = 10;
			fadeDirect = 0;

			//AdvancementQuestEndingEvent->parameter1 = g_pPlayer->IsSlayer()?1000:2000;
			AdvancementQuestEndingEvent->parameter1 = 9998;
			AdvancementQuestEndingEvent->parameter2 = 0;
			AdvancementQuestEndingEvent->parameter3 = 1000;
			//				g_pPlayer->SetChatString((*g_pGameStringTable)[g_pPlayer->IsSlayer()?STRING_MESSAGE_RIPATY_SCRIPT_1:STRING_MESSAGE_AMATA_SCRIPT_1].GetString());
			//
			//				g_pMP3->Stop();
			//				g_pMP3->Open( "Data\\Music\\blood.wav" );


			if (g_DXSound.IsInit())
			{
				// 				if( g_pOGG != NULL )
				// 					g_pOGG->streamClose();
				// 				
				// 				if( g_pSoundBufferForOGG == NULL )
				// 					g_pSoundBufferForOGG = new CDirectSoundBuffer(g_hWnd, SOUND_STEREO, SOUND_44K, SOUND_16BIT);
				// 				
				// 				if( g_pOGG == NULL )
				// #ifdef _MT
				// 					g_pOGG = new COGGSTREAM(g_hWnd, g_pSoundBufferForOGG, 44100, 11025, 8800);
				// #else
				// 					g_pOGG = new COGGSTREAM(g_hWnd, g_pSoundBufferForOGG, 44100, 11025, 8800,1);
				// #endif
				// 				
				// 				if( g_oggfile != NULL)
				// 					fclose(g_oggfile );
				// 				g_oggfile = fopen("data\\music\\Silence_of_Battlefield.ogg","rb");
				// 				g_pOGG->streamLoad(g_oggfile, NULL);
				// 				g_pOGG->streamPlay(SOUND_PLAY_REPEAT);
				// 				int volume = (g_pUserOption->VolumeMusic - 15) * 250;
				// 				g_pOGG->streamVolume( max( -10000, min( -1, volume ) ) );

				BGMManager::Instance()->PlayMusic("data\\music\\Silence_of_Battlefield.ogg");
				int volume = (g_pUserOption->VolumeMusic - 15) * 250;
				BGMManager::Instance()->SetVolume(max(-10000, min(-1, volume)));
			}

			PlaySoundForce(soundID);
			AdvancementQuestEndingEvent->eventStartTickCount = GetTickCount();


			//			g_pBack->Lock();
			//			g_pBack->BltSprite( &pointZero, &m_AdvacementQuestEnding[SpkIndex] );
			//			g_pBack->Unlock();
		}
		//
		m_pSurface->Lock();
		if (AdvancementQuestEndingEvent->parameter3 > 32)
		{
			m_pSurface->BltSprite(&pointZero, &m_AdvacementQuestEnding[SpkIndex + 1]);
		}
		else if (AdvancementQuestEndingEvent->parameter3 == 0)
		{
			m_pSurface->BltSprite(&pointZero, &m_AdvacementQuestEnding[SpkIndex]);
		}
		else
		{
			m_pSurface->BltSprite(&pointZero, &m_AdvacementQuestEnding[SpkIndex]);
			m_pSurface->BltSpriteAlpha(&pointZero, &m_AdvacementQuestEnding[SpkIndex + 1], AdvancementQuestEndingEvent->parameter3);
		}
		m_pSurface->Unlock();
		if (AdvancementQuestEndingEvent->parameter3 > 0 && g_FrameCount & 0x01)
			AdvancementQuestEndingEvent->parameter3--;
		switch (AdvancementQuestEndingEvent->parameter1)
		{
		case 9998:
			if (m_pSurface->Lock())
			{
				int scroll_progress = -500 + (GetTickCount() - AdvancementQuestEndingEvent->eventStartTickCount) / 66;
				//				int scroll_progress = -500+(GetTickCount()-AdvancementQuestEndingEvent->eventStartTickCount)/11;

				const int scroll_x = SURFACE_WIDTH / 2 - m_AdvacementQuestEnding[0].GetWidth() / 2;
				const int scroll_y = (SURFACE_HEIGHT - 500) / 2;

				/*
				#ifdef CONVERT_1024_768
				#else
					const int scroll_x = 800/2-m_AdvacementQuestEnding[0].GetWidth()/2, scroll_y = 50;
				#endif
				*/

				int scroll = ((scroll_progress < 0) ? 0 : scroll_progress);
				int scroll2 = ((scroll_progress < 0) ? scroll_progress : scroll_progress);
				int scroll3 = ((scroll_progress < 0) ? 500 + scroll_progress : 500);

				Rect rect(0, scroll, m_AdvacementQuestEnding[0].GetWidth(), min(scroll3, m_AdvacementQuestEnding[0].GetHeight() - scroll));
				S_SURFACEINFO surface_info;
				SetSurfaceInfo(&surface_info, gpC_base->m_p_DDSurface_back->GetDDSD());

				int x = scroll_x, y = scroll_y - scroll2;
				RECT rt;
				rt.left = max(-x, rect.x);
				rt.top = max(-y, rect.y);
				rt.right = min(rect.x + rect.w, SURFACE_WIDTH - x);
				rt.bottom = min(rect.y + rect.h, SURFACE_HEIGHT - y);
				/*
				#ifdef CONVERT_1024_768
				#else
								rt.right = min(rect.x+rect.w, 800-x);
								rt.bottom = min(rect.y+rect.h, 600-y);
				#endif
				*/

				if (rt.bottom < rt.top)
				{
					AdvancementQuestEndingEvent->parameter1 = 9999;
				}

				if (rt.left < rt.right && rt.top < rt.bottom)
				{
					WORD* p_dest = (WORD*)surface_info.p_surface + x + rt.left;
					p_dest = (WORD*)((BYTE*)p_dest + (y + rt.top) * surface_info.pitch);

					//void BltClip(WORD *pDest, WORD pitch, RECT* pRect); // in CSprite.h
					m_AdvacementQuestEnding[0].BltClipWidth(p_dest, surface_info.pitch, &rt);
				}
				m_pSurface->Unlock();
			}
			break;

		case 9999:
			bFinEnd = true;
			break;
		}

		RECT	rect;
		rect.left = 0;
		rect.right = CLIPSURFACE_WIDTH;
		rect.top = 0;
		rect.bottom = CLIPSURFACE_HEIGHT;

		DrawAlphaBox(&rect, fadeColor[0], fadeColor[1], fadeColor[2], min(31, (GetTickCount() - AdvancementQuestEndingEvent->eventStartTickCount) / fadeSpeed) ^ fadeDirect);

		if (bFinEnd)
		{
			//				char szTemp[512];
			//				sprintf(szTemp, "%d", (GetTickCount()-AdvancementQuestEndingEvent->eventStartTickCount)/1000);
			//				UI_AddChatToHistory( szTemp, szTemp, 0, RGB(255, 255, 255) );
			UI_REQUEST_DIE_TIMER_RESET();
			g_pEventManager->RemoveEvent(EVENTID_LOST_CHILD_PROLOGUE);
			m_AdvacementQuestEnding.Release();
		}
		return false;
	}
	return true;
}



bool
MTopView::ExcuteAdvancementQuestEnding(void* pVoid)
{
	MEvent* AdvancementQuestEndingEvent = (MEvent*)pVoid;

	if (AdvancementQuestEndingEvent != NULL)
	{
		bool bFinEnd = false;

		int SpkIndex = AdvancementQuestEndingEvent->parameter4;
		if (m_AdvacementQuestEnding.GetSize() == 0)
		{
			// 			ivfstream	FinFile;
			// 			if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_ADVANCEMENT_QUEST").c_str(), FinFile))
			// 				return false;
			// 			m_AdvacementQuestEnding.LoadFromFile(FinFile);
			// 			FinFile.close();

			if (!m_AdvacementQuestEnding.LoadFromFile(g_pFileDef->getProperty("FILE_SPRITE_ADVANCEMENT_QUEST").c_str()))
				return false;
		}
		const TYPE_SOUNDID soundID = SOUND_SLAYER_ENCHANT_B2;

		static BYTE fadeColor[3] = { 255, 255, 255 };
		static int fadeSpeed = 10;
		static int fadeDirect = 0;
		POINT pointZero = { 0, 0 };

		if (AdvancementQuestEndingEvent->parameter1 == 0)
		{
			fadeColor[0] = 255;
			fadeColor[1] = 255;
			fadeColor[2] = 255;
			fadeSpeed = 10;
			fadeDirect = 0;

			//AdvancementQuestEndingEvent->parameter1 = g_pPlayer->IsSlayer()?1000:2000;
			AdvancementQuestEndingEvent->parameter1 = 9998;
			AdvancementQuestEndingEvent->parameter2 = 0;
			AdvancementQuestEndingEvent->parameter3 = 1000;
			//				g_pPlayer->SetChatString((*g_pGameStringTable)[g_pPlayer->IsSlayer()?STRING_MESSAGE_RIPATY_SCRIPT_1:STRING_MESSAGE_AMATA_SCRIPT_1].GetString());
			//
			//				g_pMP3->Stop();
			//				g_pMP3->Open( "Data\\Music\\blood.wav" );


			if (g_DXSound.IsInit())
			{
				// 				if( g_pOGG != NULL )
				// 					g_pOGG->streamClose();
				// 				
				// 				if( g_pSoundBufferForOGG == NULL )
				// 					g_pSoundBufferForOGG = new CDirectSoundBuffer(g_hWnd, SOUND_STEREO, SOUND_44K, SOUND_16BIT);
				// 				
				// 				if( g_pOGG == NULL )
				// #ifdef _MT
				// 					g_pOGG = new COGGSTREAM(g_hWnd, g_pSoundBufferForOGG, 44100, 11025, 8800);
				// #else
				// 					g_pOGG = new COGGSTREAM(g_hWnd, g_pSoundBufferForOGG, 44100, 11025, 8800,1);
				// #endif
				// 				
				// 				if( g_oggfile != NULL)
				// 					fclose(g_oggfile );
				// 				g_oggfile = fopen("data\\music\\Silence_of_Battlefield.ogg","rb");
				// 				g_pOGG->streamLoad(g_oggfile, NULL);
				// 				g_pOGG->streamPlay(SOUND_PLAY_REPEAT);
				// 				int volume = (g_pUserOption->VolumeMusic - 15) * 250;
				// 				g_pOGG->streamVolume( max( -10000, min( -1, volume ) ) );

				BGMManager::Instance()->PlayMusic("data\\music\\Silence_of_Battlefield.ogg");
				int volume = (g_pUserOption->VolumeMusic - 15) * 250;
				BGMManager::Instance()->SetVolume(max(-10000, min(-1, volume)));
			}
			PlaySoundForce(soundID);
			AdvancementQuestEndingEvent->eventStartTickCount = GetTickCount();


			//			g_pBack->Lock();
			//			g_pBack->BltSprite( &pointZero, &m_AdvacementQuestEnding[SpkIndex] );
			//			g_pBack->Unlock();
		}
		//
		m_pSurface->Lock();
		if (AdvancementQuestEndingEvent->parameter3 > 32)
		{
			m_pSurface->BltSprite(&pointZero, &m_AdvacementQuestEnding[SpkIndex + 1]);
		}
		else if (AdvancementQuestEndingEvent->parameter3 == 0)
		{
			m_pSurface->BltSprite(&pointZero, &m_AdvacementQuestEnding[SpkIndex]);
		}
		else
		{
			m_pSurface->BltSprite(&pointZero, &m_AdvacementQuestEnding[SpkIndex]);
			m_pSurface->BltSpriteAlpha(&pointZero, &m_AdvacementQuestEnding[SpkIndex + 1], AdvancementQuestEndingEvent->parameter3);
		}
		m_pSurface->Unlock();
		if (AdvancementQuestEndingEvent->parameter3 > 0 && g_FrameCount & 0x01)
			AdvancementQuestEndingEvent->parameter3--;
		switch (AdvancementQuestEndingEvent->parameter1)
		{
		case 9998:
			if (m_pSurface->Lock())
			{
				int scroll_progress = -500 + (GetTickCount() - AdvancementQuestEndingEvent->eventStartTickCount) / 66;
				//				int scroll_progress = -500+(GetTickCount()-AdvancementQuestEndingEvent->eventStartTickCount)/11;

				const int scroll_x = SURFACE_WIDTH / 2 - m_AdvacementQuestEnding[0].GetWidth() / 2;
				const int scroll_y = (SURFACE_HEIGHT - 500) / 2;
				/*
				#ifdef CONVERT_1024_768
				#else
								const int scroll_x = 800/2-m_AdvacementQuestEnding[0].GetWidth()/2, scroll_y = 50;
				#endif
				*/

				int scroll = ((scroll_progress < 0) ? 0 : scroll_progress);
				int scroll2 = ((scroll_progress < 0) ? scroll_progress : scroll_progress);
				int scroll3 = ((scroll_progress < 0) ? 500 + scroll_progress : 500);

				Rect rect(0, scroll,
					m_AdvacementQuestEnding[0].GetWidth(),
					min(scroll3, m_AdvacementQuestEnding[0].GetHeight() - scroll));

				S_SURFACEINFO surface_info;
				SetSurfaceInfo(&surface_info, gpC_base->m_p_DDSurface_back->GetDDSD());

				int x = scroll_x, y = scroll_y - scroll2;
				RECT rt;
				rt.left = max(-x, rect.x);
				rt.top = max(-y, rect.y);
				rt.right = min(rect.x + rect.w, SURFACE_WIDTH - x);
				rt.bottom = min(rect.y + rect.h, SURFACE_HEIGHT - y);

				if (rt.bottom < rt.top)
				{
					AdvancementQuestEndingEvent->parameter1 = 9999;
				}

				if (rt.left < rt.right && rt.top < rt.bottom)
				{
					WORD* p_dest = (WORD*)surface_info.p_surface + x + rt.left;
					p_dest = (WORD*)((BYTE*)p_dest + (y + rt.top) * surface_info.pitch);

					//void BltClip(WORD *pDest, WORD pitch, RECT* pRect); // in CSprite.h
					m_AdvacementQuestEnding[0].BltClipWidth(p_dest, surface_info.pitch, &rt);
				}
				m_pSurface->Unlock();
			}
			break;

		case 9999:
			bFinEnd = true;
			break;
		}

		RECT rect;
		rect.left = 0;
		rect.right = CLIPSURFACE_WIDTH;
		rect.top = 0;
		rect.bottom = CLIPSURFACE_HEIGHT;

		DrawAlphaBox(&rect, fadeColor[0], fadeColor[1], fadeColor[2], min(31, (GetTickCount() - AdvancementQuestEndingEvent->eventStartTickCount) / fadeSpeed) ^ fadeDirect);

		if (bFinEnd)
		{
			//				char szTemp[512];
			//				sprintf(szTemp, "%d", (GetTickCount()-AdvancementQuestEndingEvent->eventStartTickCount)/1000);
			//				UI_AddChatToHistory( szTemp, szTemp, 0, RGB(255, 255, 255) );
			UI_REQUEST_DIE_TIMER_RESET();
			g_pEventManager->RemoveEvent(EVENTID_ADVANCEMENT_QUEST_ENDING);
			m_AdvacementQuestEnding.Release();
		}
		return false;
	}
	return true;
}


bool
MTopView::ExcuteOustersFinEvent()
{
	bool bDrawBackGround = true;
	const MEvent* event = g_pEventManager->GetEventByFlag(EVENTFLAG_ONLY_EVENT_BACKGROUND);
	if (event != NULL)
	{
		if ((event->eventFlag & EVENTFLAG_ONLY_EVENT_BACKGROUND) == EVENTFLAG_NOT_DRAW_BACKGROUND)
		{
			m_pSurface->FillSurface(0);	//	????????
		}
		else if (event->parameter4 < EVENTBACKGROUNDID_MAX)
		{
			//			AssertEventBackground(event->parameter4);

			POINT p = { 0, 0 };

			RECT r = { 0, 0, SURFACE_WIDTH - 1, SURFACE_HEIGHT - 1 };
			/*
			#ifdef CONVERT_1024_768
			#else
						RECT r = {0, 0, 799, 599 };
			#endif
			*/
			//			if(!m_pSurface->Lock()) return;

			CDirectDrawSurface* pSurface = g_pEventManager->GetEventBackground((EVENTBACKGROUND_ID)event->parameter4);

			m_pSurface->BltNoColorkey(&p, pSurface, &r);

			//			m_pSurface->BltSprite(&p, g_pEventManager->GetEventBackground(event->parameter4));

			//			m_pSurface->Unlock();
		}

		if (event->eventFlag & EVENTFLAG_ONLY_EVENT_BACKGROUND)
			bDrawBackGround = false;
	}

	MEvent* OustersFinEvent = const_cast<MEvent*>(g_pEventManager->GetEvent(EVENTID_OUSTERS_FIN));

	if (OustersFinEvent != NULL)
	{
		bool bFinEnd = false;

		if (m_OustersFinSPK.GetSize() == 0)
		{
			// 			ivfstream	FinFile;
			// 			if (!FileOpenBinary(g_pFileDef->getProperty("FILE_SPRITE_OUSTERS_FIN").c_str(), FinFile))
			// 				return bDrawBackGround;
			// 			m_OustersFinSPK.LoadFromFile(FinFile);
			// 			FinFile.close();
			if (!m_OustersFinSPK.LoadFromFile(g_pFileDef->getProperty("FILE_SPRITE_OUSTERS_FIN").c_str()))
				return false;
		}

		const TYPE_SOUNDID soundID = SOUND_SLAYER_ENCHANT_B2;

		static BYTE fadeColor[3] = { 255, 255, 255 };
		static int fadeSpeed = 10;
		static int fadeDirect = 0;

		if (OustersFinEvent->parameter1 == 0)
		{
			fadeColor[0] = 255;
			fadeColor[1] = 255;
			fadeColor[2] = 255;
			fadeSpeed = 10;
			fadeDirect = 0;

			//OustersFinEvent->parameter1 = g_pPlayer->IsSlayer()?1000:2000;
			OustersFinEvent->parameter1 = 9998;
			OustersFinEvent->parameter2 = 0;
			//				g_pPlayer->SetChatString((*g_pGameStringTable)[g_pPlayer->IsSlayer()?STRING_MESSAGE_RIPATY_SCRIPT_1:STRING_MESSAGE_AMATA_SCRIPT_1].GetString());
			//
			//				g_pMP3->Stop();
			//				g_pMP3->Open( "Data\\Music\\blood.wav" );


			if (g_DXSound.IsInit())
			{
				// 				if( g_pOGG != NULL )
				// 					g_pOGG->streamClose();
				// 				
				// 				if( g_pSoundBufferForOGG == NULL )
				// 					g_pSoundBufferForOGG = new CDirectSoundBuffer(g_hWnd, SOUND_STEREO, SOUND_44K, SOUND_16BIT);
				// 				
				// 				if( g_pOGG == NULL )
				// #ifdef _MT
				// 					g_pOGG = new COGGSTREAM(g_hWnd, g_pSoundBufferForOGG, 44100, 11025, 8800);
				// #else
				// 					g_pOGG = new COGGSTREAM(g_hWnd, g_pSoundBufferForOGG, 44100, 11025, 8800,1);
				// #endif
				// 				
				// 				if( g_oggfile != NULL)
				// 					fclose(g_oggfile );
				// 				g_oggfile = fopen("data\\music\\chaos.ogg","rb");
				// 				g_pOGG->streamLoad(g_oggfile, NULL);
				// 				g_pOGG->streamPlay(SOUND_PLAY_REPEAT);
				// 				int volume = (g_pUserOption->VolumeMusic - 15) * 250;
				// 				g_pOGG->streamVolume( max( -10000, min( -1, volume ) ) );

				BGMManager::Instance()->PlayMusic("data\\music\\chaos.ogg");
				int volume = (g_pUserOption->VolumeMusic - 15) * 250;
				BGMManager::Instance()->SetVolume(max(-10000, min(-1, volume)));
			}
			PlaySoundForce(soundID);
			OustersFinEvent->eventStartTickCount = GetTickCount();
		}

		switch (OustersFinEvent->parameter1)
		{
		case 1000:
		{
			if ((GetTickCount() - OustersFinEvent->eventStartTickCount) / 4000 > 0)
			{
				OustersFinEvent->parameter2++;
				if (OustersFinEvent->parameter2 > 8)
				{
					g_pPlayer->ClearChatString();

					if (OustersFinEvent->parameter2 == 9)
					{
						g_pPlayer->SetDead();
						PlaySoundForce(g_pPlayer->IsFemale() ? SOUND_SLAYER_DIE_FEMALE : SOUND_SLAYER_DIE_MALE);
						fadeSpeed = 32 * 4;
						fadeColor[0] = 255;
						fadeColor[1] = 0;
						fadeColor[2] = 0;
					}

					if (OustersFinEvent->parameter2 > 9)
					{
						fadeColor[0] = 0;
						fadeColor[1] = 0;
						fadeColor[2] = 0;
						fadeDirect = 31;

						if (OustersFinEvent->parameter2 > 10)
						{
							OustersFinEvent->eventStartTickCount = GetTickCount();
							OustersFinEvent->parameter1 = 9998;
							OustersFinEvent->parameter4 = EVENTBACKGROUNDID_OUSTERS_SLAYER;
							g_pMP3->Play(false);
							fadeDirect = 0;
						}
					}
				}
				else
				{
					PlaySoundForce(soundID);
					g_pPlayer->SetChatString((*g_pGameStringTable)[STRING_MESSAGE_RIPATY_SCRIPT_1 + OustersFinEvent->parameter2].GetString());
				}

				if (OustersFinEvent->parameter2 < 11)
					OustersFinEvent->eventStartTickCount = GetTickCount();
			}
			if (m_pSurface->Lock())
			{
				const int char_plus_x = 10, char_plus_y = 24;

				POINT p[8] =
				{
					{ 400 - m_OustersFinSPK[3].GetWidth() / 2 + char_plus_x, 160 - m_OustersFinSPK[3].GetHeight() + char_plus_y},
					{ 300 - m_OustersFinSPK[4].GetWidth() / 2 + char_plus_x, 210 - m_OustersFinSPK[4].GetHeight() + char_plus_y},
					{ 240 - m_OustersFinSPK[5].GetWidth() / 2 + char_plus_x, 300 - m_OustersFinSPK[5].GetHeight() + char_plus_y},
					{ 300 - m_OustersFinSPK[6].GetWidth() / 2 + char_plus_x, 390 - m_OustersFinSPK[6].GetHeight() + char_plus_y},
					{ 400 - m_OustersFinSPK[7].GetWidth() / 2 + char_plus_x, 440 - m_OustersFinSPK[7].GetHeight() + char_plus_y},
					{ 500 - m_OustersFinSPK[8].GetWidth() / 2 + char_plus_x, 390 - m_OustersFinSPK[8].GetHeight() + char_plus_y},
					{ 560 - m_OustersFinSPK[9].GetWidth() / 2 + char_plus_x, 300 - m_OustersFinSPK[9].GetHeight() + char_plus_y},
					{ 500 - m_OustersFinSPK[10].GetWidth() / 2 + char_plus_x, 210 - m_OustersFinSPK[10].GetHeight() + char_plus_y},
				};

				m_pSurface->BltSprite(&p[0], &m_OustersFinSPK[3]);
				POINT strPoint = p[0];
				strPoint.x -= 15;
				strPoint.y -= 15;
				DrawChatString(&strPoint, g_pPlayer, RGB(255, 255, 255));

				if (OustersFinEvent->parameter2 > 0)
				{
					for (int i = 0; i < 7; i++)
					{
						m_pSurface->BltSprite(&p[i + 1], &m_OustersFinSPK[4 + i]);
					}
				}

				MCreature* pCreature = g_pPlayer;

				int action = pCreature->GetAction(), direction = 6, frame = pCreature->GetFrame();

				MCreatureWear* pCreatureWear = (MCreatureWear*)pCreature;

				// Creature?? Action?? ?´? add-on?? ??????.
				//action = pCreature->GetAction();

				WORD clothes;
				BYTE clothesType;

				for (int i = 0; i < ADDON_MAX; i++)
				{
					// Creature?? ???? ???? ????...
					// ???? ???????? ?????? ??? ?? ???.
					clothesType = MCreatureWear::s_AddonOrder[direction][i];

					// i??° ?????? ???? ??? ???? ????? ???.
					const MCreatureWear::ADDON_INFO& addonInfo = pCreatureWear->GetAddonInfo(clothesType);

					if (addonInfo.bAddon)
					{
						clothes = addonInfo.FrameID;

						CCreatureFramePack& addonFPK = pCreatureWear->IsMale() ? m_AddonManFPK : m_AddonWomanFPK;
						CIndexSpritePack& addonSPK = pCreatureWear->IsMale() ? m_AddonManSPK : m_AddonWomanSPK;

						// AddonWoman.cfpk is shorter than AddonMan.cfpk and TArray::operator[]
						// does no bounds check, so an out-of-range FrameID crashes. Skip it.
						if (clothes >= (int)addonFPK.GetSize()) continue;
						FRAME_ARRAY& FA = addonFPK[clothes][action][direction];

						// ??? ?????? ???
						if (FA.GetSize() > frame)
						{
							CFrame& Frame = FA[frame];
							int sprite = Frame.GetSpriteID();	//m_AddonFPK[clothes][action][direction][frame].GetSpriteID();
							int cx = Frame.GetCX();	//m_AddonFPK[clothes][action][direction][frame].GetCX();
							int cy = Frame.GetCY();	//m_AddonFPK[clothes][action][direction][frame].GetCY();


							CIndexSprite* pSprite = &addonSPK[sprite];

							// ????Sprite?? ???? ???? ???? ???
//									if (pSprite->IsNotInit())
//									{
//										LoadFromFileAddonSPK( clothes, action );
//									}


							POINT pointTemp;

							// ??? ????
							pointTemp.x = 384 + cx;
							pointTemp.y = 312 + cy;

							{
								int colorSet1 = addonInfo.ColorSet1, colorSet2 = addonInfo.ColorSet2;
								if (colorSet2 == UNIQUE_ITEM_COLOR || colorSet2 == QUEST_ITEM_COLOR)
								{
									// ????? ?????????
									//									if(colorSet2 == QUEST_ITEM_COLOR)
									//										colorSet2 = MItem::GetQuestItemColorset();
									//									else
									//										colorSet2 = MItem::GetUniqueItemColorset();										

									colorSet2 = MItem::GetSpecialColorItemColorset(colorSet2);
									if (addonInfo.ItemClass != ITEM_CLASS_COAT && addonInfo.ItemClass != ITEM_CLASS_TROUSER)
										colorSet1 = colorSet2;
								}

								CIndexSprite::SetUsingColorSet(colorSet1, colorSet2);

								// ???? ???
								if (pCreature->IsFade())
								{
									m_pSurface->BltIndexSpriteDarkness(&pointTemp, pSprite, 1);
								}
								else
								{
									m_pSurface->BltIndexSprite(&pointTemp, pSprite);
								}
							}

							// ???????? ???
							//CIndexSprite::SetUsingColorSet( addonInfo.ColorSet1, addonInfo.ColorSet2 );
							//m_pSurface->BltIndexSprite(&pointTemp, pSprite);

						}
					}

				}

				m_pSurface->Unlock();
			}
		}
		break;

		case 2000:
		{
			if ((GetTickCount() - OustersFinEvent->eventStartTickCount) / 4000 > 0)
			{
				OustersFinEvent->parameter2++;

				if (OustersFinEvent->parameter2 > 8)
				{
					g_pPlayer->ClearChatString();

					if (OustersFinEvent->parameter2 == 9)
					{
						g_pPlayer->SetDead();
						PlaySoundForce(g_pPlayer->IsFemale() ? SOUND_VAMPIRE_DIE_FEMALE : SOUND_VAMPIRE_DIE_MALE);
						fadeSpeed = 32 * 4;
						fadeColor[0] = 255;
						fadeColor[1] = 0;
						fadeColor[2] = 0;
					}

					if (OustersFinEvent->parameter2 > 9)
					{
						fadeColor[0] = 0;
						fadeColor[1] = 0;
						fadeColor[2] = 0;
						fadeDirect = 31;

						if (OustersFinEvent->parameter2 > 10)
						{
							OustersFinEvent->eventStartTickCount = GetTickCount();
							OustersFinEvent->parameter1 = 9998;
							OustersFinEvent->parameter4 = EVENTBACKGROUNDID_OUSTERS_VAMPIRE;
							g_pMP3->Play(false);
							fadeDirect = 0;
						}
					}
				}
				else
				{
					PlaySoundForce(soundID);
					g_pPlayer->SetChatString((*g_pGameStringTable)[STRING_MESSAGE_AMATA_SCRIPT_1 + OustersFinEvent->parameter2].GetString());
				}

				if (OustersFinEvent->parameter2 < 11)
					OustersFinEvent->eventStartTickCount = GetTickCount();
			}

			if (m_pSurface->Lock())
			{
				const int char_plus_x = 10, char_plus_y = 24;

				POINT p[8] =
				{
					{ 400 - m_OustersFinSPK[2].GetWidth() / 2 + char_plus_x, 160 - m_OustersFinSPK[2].GetHeight() + char_plus_y},
					{ 300 - m_OustersFinSPK[4].GetWidth() / 2 + char_plus_x, 210 - m_OustersFinSPK[4].GetHeight() + char_plus_y},
					{ 240 - m_OustersFinSPK[5].GetWidth() / 2 + char_plus_x, 300 - m_OustersFinSPK[5].GetHeight() + char_plus_y},
					{ 300 - m_OustersFinSPK[6].GetWidth() / 2 + char_plus_x, 390 - m_OustersFinSPK[6].GetHeight() + char_plus_y},
					{ 400 - m_OustersFinSPK[7].GetWidth() / 2 + char_plus_x, 440 - m_OustersFinSPK[7].GetHeight() + char_plus_y},
					{ 500 - m_OustersFinSPK[8].GetWidth() / 2 + char_plus_x, 390 - m_OustersFinSPK[8].GetHeight() + char_plus_y},
					{ 560 - m_OustersFinSPK[9].GetWidth() / 2 + char_plus_x, 300 - m_OustersFinSPK[9].GetHeight() + char_plus_y},
					{ 500 - m_OustersFinSPK[10].GetWidth() / 2 + char_plus_x, 210 - m_OustersFinSPK[10].GetHeight() + char_plus_y},
				};

				m_pSurface->BltSprite(&p[0], &m_OustersFinSPK[2]);
				POINT strPoint = p[0];
				strPoint.x -= 15;
				strPoint.y -= 15;
				DrawChatString(&strPoint, g_pPlayer, RGB(255, 255, 255));

				if (OustersFinEvent->parameter2 > 0)
				{
					for (int i = 0; i < 7; i++)
					{
						m_pSurface->BltSprite(&p[i + 1], &m_OustersFinSPK[4 + i]);
					}
				}

				int body, action, direction, frame, creature_type;

				MCreature* pCreature = g_pPlayer;


				body = pCreature->GetCreatureFrameID(0);
				action = pCreature->GetAction();
				direction = 6;//pCreature->GetDirection();
				frame = pCreature->GetFrame();//%m_CreatureFPK[body][action][direction].GetCount();
				creature_type = pCreature->GetCreatureType();

				// vampire
				FRAME_ARRAY& FA = m_CreatureFPK[body][action][direction];

				if (FA.GetSize() > frame)
				{
					CFrame& Frame = FA[frame];
					int sprite = Frame.GetSpriteID(),	//m_CreatureFPK[body][action][direction][frame].GetSpriteID(),
						cx = Frame.GetCX(),	//m_CreatureFPK[body][action][direction][frame].GetCX(),
						cy = Frame.GetCY();	//m_CreatureFPK[body][action][direction][frame].GetCY();

					CIndexSprite* pSprite = &GetCreatureSprite(sprite, body);

					//							if (pSprite->IsNotInit())
					//							{
					//								LoadFromFileCreatureActionSPK( body, action );
					//							}

					int colorSet1 = pCreature->GetBodyColor1();
					int colorSet2 = pCreature->GetBodyColor2();
					if (colorSet1 == QUEST_ITEM_COLOR || colorSet1 == UNIQUE_ITEM_COLOR)
						colorSet1 = MItem::GetSpecialColorItemColorset(colorSet1);

					if (colorSet2 == QUEST_ITEM_COLOR || colorSet2 == UNIQUE_ITEM_COLOR)
						colorSet2 = MItem::GetSpecialColorItemColorset(colorSet2);

					POINT pointTemp;

					// ??? ????
					pointTemp.x = 384 + cx;
					pointTemp.y = 312 + cy;

					CIndexSprite::SetUsingColorSet(colorSet1, colorSet2);
					m_pSurface->BltIndexSprite(&pointTemp, pSprite);
				}

				m_pSurface->Unlock();
			}
		}
		break;

		case 9998:
			if (m_pSurface->Lock())
			{
				int scroll_progress = -500 + (GetTickCount() - OustersFinEvent->eventStartTickCount) / 66;

				const int spriteID = 0;
				const int scroll_x = SURFACE_WIDTH / 2 - m_AdvacementQuestEnding[0].GetWidth() / 2;
				const int scroll_y = (SURFACE_HEIGHT - 500) / 2;
				/*
				#ifdef CONVERT_1024_768
				#else
								const int scroll_x = 800/2-m_OustersFinSPK[spriteID].GetWidth()/2, scroll_y = 50;
				#endif
				*/

				int scroll = ((scroll_progress < 0) ? 0 : scroll_progress);
				int scroll2 = ((scroll_progress < 0) ? scroll_progress : scroll_progress);
				int scroll3 = ((scroll_progress < 0) ? 500 + scroll_progress : 500);

				Rect rect(0, scroll, m_OustersFinSPK[spriteID].GetWidth(), min(scroll3, m_OustersFinSPK[spriteID].GetHeight() - scroll));
				S_SURFACEINFO surface_info;
				SetSurfaceInfo(&surface_info, gpC_base->m_p_DDSurface_back->GetDDSD());

				int x = scroll_x, y = scroll_y - scroll2;
				RECT rt;
				rt.left = max(-x, rect.x);
				rt.top = max(-y, rect.y);
				rt.right = min(rect.x + rect.w, SURFACE_WIDTH - x);
				rt.bottom = min(rect.y + rect.h, SURFACE_HEIGHT - y);

				if (rt.bottom < rt.top)
				{
					OustersFinEvent->parameter1 = 9999;
				}

				if (rt.left < rt.right && rt.top < rt.bottom)
				{
					WORD* p_dest = (WORD*)surface_info.p_surface + x + rt.left;
					p_dest = (WORD*)((BYTE*)p_dest + (y + rt.top) * surface_info.pitch);

					//void BltClip(WORD *pDest, WORD pitch, RECT* pRect); // in CSprite.h
					m_OustersFinSPK[spriteID].BltClipWidth(p_dest, surface_info.pitch, &rt);
				}

				Rect rect2(0, 0, m_OustersFinSPK[1].GetWidth(), min(scroll3 - (m_OustersFinSPK[spriteID].GetHeight() - scroll) - 50, m_OustersFinSPK[1].GetHeight()));

				x = scroll_x + m_OustersFinSPK[spriteID].GetWidth() / 2 - m_OustersFinSPK[1].GetWidth() / 2;
				y = scroll_y + max(200, (m_OustersFinSPK[spriteID].GetHeight() - scroll + 50));
				rt.left = max(-x, rect2.x);
				rt.top = max(-y, rect2.y);
				rt.right = min(rect2.x + rect2.w, SURFACE_WIDTH - x);
				rt.bottom = min(rect2.y + rect2.h, SURFACE_HEIGHT - y);
				if (rt.left < rt.right && rt.top < rt.bottom)
				{
					WORD* p_dest = (WORD*)surface_info.p_surface + x + rt.left;
					p_dest = (WORD*)((BYTE*)p_dest + (y + rt.top) * surface_info.pitch);

					//void BltClip(WORD *pDest, WORD pitch, rect2* prect2); // in CSprite.h
					m_OustersFinSPK[1].BltClipWidth(p_dest, surface_info.pitch, &rt);
				}

				m_pSurface->Unlock();
			}
			break;

		case 9999:
			bFinEnd = true;
			break;
		}

		RECT	rect;
		rect.left = 0;
		rect.right = CLIPSURFACE_WIDTH;
		rect.top = 0;
		rect.bottom = CLIPSURFACE_HEIGHT;

		DrawAlphaBox(&rect, fadeColor[0], fadeColor[1], fadeColor[2], min(31, (GetTickCount() - OustersFinEvent->eventStartTickCount) / fadeSpeed) ^ fadeDirect);

		if (bFinEnd)
		{
			//				char szTemp[512];
			//				sprintf(szTemp, "%d", (GetTickCount()-OustersFinEvent->eventStartTickCount)/1000);
			//				UI_AddChatToHistory( szTemp, szTemp, 0, RGB(255, 255, 255) );
			UI_REQUEST_DIE_TIMER_RESET();
			g_pEventManager->RemoveEvent(EVENTID_OUSTERS_FIN);
			m_OustersFinSPK.Release();
		}

	}
	return bDrawBackGround;
}


POINT MTopView::CalculateChatStringPoint(POINT* pPoint, MCreature* pCreature, bool SubFontHeight)
{
	POINT pointTemp;

	//---------------------------------------- 	
	// ??? ????? ??? ????
	//---------------------------------------- 	
	const int FontHeight = g_pClientConfig->FONT_HEIGHT;
	const int FontHeight2 = FontHeight << 1;

	pointTemp = *pPoint;
	if (pointTemp.x < 0) pointTemp.x = 0;
	if (pointTemp.x >= RESOLUTION_X) pointTemp.x = RESOLUTION_X - 1;

	//---------------------------------------- 	
	// Level Name ???? ????? ???
	//---------------------------------------- 	
	if (pCreature->HasLevelName())
	{
		if (SubFontHeight)
			pointTemp.y = pPoint->y - FontHeight2;

		if (pointTemp.y < FontHeight2)
			pointTemp.y = FontHeight2;
	}
	//---------------------------------------- 	
	// ??? ????? ???? ??
	//---------------------------------------- 	
	else
	{
		if (SubFontHeight)
			pointTemp.y = pPoint->y - FontHeight;

		if (pointTemp.y < FontHeight)
			pointTemp.y = FontHeight;
	}

	if (pointTemp.y >= RESOLUTION_Y) pointTemp.y = RESOLUTION_Y - 1;

	return pointTemp;
}


void MTopView::AddDrawNameCreature(POINT* pPoint, MCreature* pCreature, bool SubFontHeight)
{
	if (pCreature == NULL || pPoint == NULL || pCreature->GetName() == NULL) return;

	DrawNameCreatureInfo dncInfo;

	dncInfo.ptDrawPostion = CalculateChatStringPoint(pPoint, pCreature, SubFontHeight);
	dncInfo.pCreature = pCreature;
	dncInfo.bOverText = false;

	m_DrawNameCreatureInfoVec.push_back(dncInfo);
}


void MTopView::ClearDrawNameCreature()
{
	m_DrawNameCreatureInfoVec.clear();
}


bool MTopView::IsDrawNameCreature(MCreature* pCreature)
{
	DRAWNAMECREATUREINFO_VEC::iterator pos = m_DrawNameCreatureInfoVec.begin();
	DRAWNAMECREATUREINFO_VEC::iterator endpos = m_DrawNameCreatureInfoVec.end();

	for (; pos != endpos; ++pos)
		if (pos->pCreature == pCreature) return true;

	return false;
}



#if __CONTENTS(__2008_OCTOBER_MAKETITEM)
void MTopView::VampireComeFromPortalNonTurn(MCreature* pCreature)
{
	// Vampire ??? ???? ???? ?????? ???
	if (pCreature == NULL || !pCreature->IsVampire())
		return;

	int cutStart = 80;
	int cutInc = -5;

	int body = pCreature->GetCreatureFrameID(0);
	int action = ACTION_STAND;
	int direction = 2;

	WORD height = 1;

	// ???? Ÿ??? ?´? FA?? ã?????. ????
	const FRAME_ARRAY* pFA = NULL;

	pFA = &m_CreatureFPK[body][action][direction];


	const FRAME_ARRAY& FA = *pFA;

	if (FA.GetSize() != 0)
	{
		const CFrame& Frame = FA[0];
		int sprite = Frame.GetSpriteID();

		// ???? Ÿ??? ?´? Sprite?? ã?????. ??????
		const CIndexSprite* pSprite = NULL;

		pSprite = &GetCreatureSprite(sprite, body);


		if (pSprite != NULL)
		{
			height = pSprite->GetHeight();

			cutStart = (height + 15) - (height + 15) % 16;
			if (height == 0)
			{
				cutInc = 1;
			}
			else
			{
				cutInc = max(1, cutStart / 16);
			}
		}
	}

	pCreature->SetCutHeightFrom(cutStart, (height + 15) % 16, -cutInc, false);

	int x = pCreature->GetX();
	int y = pCreature->GetY();

	//------------------------------------------------------
	// ???? ????? ????? ????? ????? ???..
	//------------------------------------------------------

	TYPE_CREATURETYPE creatureType = pCreature->GetCreatureType();
	TYPE_ACTIONINFO summonActionResult = RESULT_MAGIC_BLOODY_TUNNEL_INTO;

	// ?????
	if (creatureType >= 693 && creatureType <= 697)
	{
		summonActionResult = RESULT_SUMMON_ULBERINE;
	}
	// ?????? ?????
	else if (creatureType == 765 || creatureType == 891)
	{
		summonActionResult = SKILL_CLIENT_GREAT_RUFFIAN_ALIVE;
	}
	// ???? ????
	else if (creatureType == 923)
	{
		summonActionResult = RESULT_SUMMON_LUPENG;
	}
	// ?????? ????
	else if (creatureType == 926)		//??????? ?????? ??????? ??? -- ????, ???? ????.
	{
		summonActionResult = SKILL_CLIENT_BEARBOX_SUMMON;
	}
	else if (creatureType == 927)		//?? ??????? ???
	{
		summonActionResult = RESULT_MAGIC_BLOODY_TUNNEL_INTO;
	}
#if __CONTENTS(__NEW_PET_FIRE_ELEMENTAL)
	else if (creatureType == 1043)		// ????? ???????
	{
		summonActionResult = SKILL_CLIENT_FIREELEMENTAL_SUMMON;
	}
#endif //__NEW_PET_FIRE_ELEMENTAL
	else if (creatureType == 1055)		// ???????
	{
		summonActionResult = SKILL_CLIENT_SUCCUBUS_SUMMON;
	}
#if __CONTENTS(__NEW_PET_INCUBUS)
	else if (creatureType == 1059)		// ???????
	{
		summonActionResult = SKILL_CLIENT_SUCCUBUS_SUMMON;
	}
#endif //__NEW_PET_INCUBUS
#if __CONTENTS(__PET_MASHIMARO)
	else if (creatureType == 1082)		// ???ø???
	{
		summonActionResult = SKILL_CLIENT_MASHIMARO_SUMMON;
	}
#endif //__PET_MASHIMARO
#if __CONTENTS(__PET_CATS) 
	else if (creatureType >= 1083 && creatureType <= 1085)		// ??????
	{
		summonActionResult = SKILL_CLIENT_CATS_SUMMON;
	}
#endif //__PET_CATS
#if __CONTENTS(__PET_CATS_UPGRADED)
	else if (creatureType >= 1089 && creatureType <= 1100)		// ??? ??????, ???? ?????? ????
	{
		summonActionResult = SKILL_CLIENT_CATS_SUMMON;
	}
#endif //__PET_CATS_UPGRADED
#if __CONTENTS(__UPGRADE_MONSTER_PET)
	else if (creatureType >= 1101 && creatureType <= 1112)		// ???,????..????
	{
		summonActionResult = SKILL_CLIENT_CATS_SUMMON;
	}
#endif //__UPGRADE_MONSTER_PET	

	// ??? ??? ????
	ExecuteActionInfoFromMainNode(
		summonActionResult,		// ??? ??? ???
		x, y, 0,
		DIRECTION_DOWN,			// ??? ????
		pCreature->GetID(),		// ????? ???? ????
		x, y, 0,
		0,						// ????? (????) ???? ?ð?		
		NULL,
		false);
}
#endif //__2008_OCTOBER_MAKETITEM


//------------------------------------------------------------------
// Come From Portal
//------------------------------------------------------------------
// ??? ????? ?????? ??? ???.. 
// Vampire, ACVampire ???? ?????
// MTopView?? ???
//------------------------------------------------------------------
void MTopView::VampireComeFromPortal(MCreature* pCreature)
{
	// Vampire ??? ???? ???? ?????? ???
	if (pCreature == NULL || !pCreature->IsVampire())
		return;

	int cutStart = 80;
	int cutInc = -5;
	WORD height = 1;

	int body = pCreature->GetCreatureFrameID(0);
	int action = ACTION_STAND;
	int direction = 2;

	// ???? Ÿ??? ?´? FA?? ã?????. ????
	const FRAME_ARRAY* pFA = NULL;

	if (pCreature->IsPlayerOnly() && !pCreature->IsOperator())
	{
		if (pCreature->IsAdvancementClass())
		{
			body = 0;
			if (pCreature->IsMale())	pFA = &m_AdvancementVampireManFPK[body][action][direction];
			else					pFA = &m_AdvancementVampireWomanFPK[body][action][direction];
		}
		else
		{
			pFA = &m_VampireFPK[body][action][direction];
		}
	}
	else
	{
		pFA = &m_CreatureFPK[body][action][direction];
	}

	const FRAME_ARRAY& FA = *pFA;

	if (FA.GetSize() != 0)
	{
		const CFrame& Frame = FA[0];
		int sprite = Frame.GetSpriteID();

		// ???? Ÿ??? ?´? Sprite?? ã?????. ??????
		const CIndexSprite* pSprite = NULL;

		if (pCreature->IsPlayerOnly() && !pCreature->IsOperator())
		{
			if (pCreature->IsAdvancementClass())
			{
				if (pCreature->IsMale())	pSprite = &m_AdvancementVampireManSPK[sprite];
				else					pSprite = &m_AdvancementVampireWomanSPK[sprite];
			}
			else
			{
				pSprite = &m_VampireSPK[sprite];
			}
		}
		else
		{
			pSprite = &GetCreatureSprite(sprite, body);;
		}


		if (pSprite != NULL)
		{
			height = pSprite->GetHeight();

			cutStart = (height + 15) - (height + 15) % 16;
			if (height == 0)
			{
				cutInc = 1;
			}
			else
			{
				cutInc = max(1, cutStart / 16);
			}
		}
	}

	pCreature->SetCutHeightFrom(cutStart, (height + 15) % 16, -cutInc);

	int x = pCreature->GetX();
	int y = pCreature->GetY();

	//------------------------------------------------------
	// ???? ????? ????? ????? ????? ???..
	//------------------------------------------------------

	TYPE_CREATURETYPE creatureType = pCreature->GetCreatureType();
	TYPE_ACTIONINFO summonActionResult = RESULT_MAGIC_BLOODY_TUNNEL_INTO;

	// ?????
	if (creatureType >= 693 && creatureType <= 697)
	{
		summonActionResult = RESULT_SUMMON_ULBERINE;
	}
	// ?????? ?????
	else if (creatureType == 765 || creatureType == 891)
	{
		summonActionResult = SKILL_CLIENT_GREAT_RUFFIAN_ALIVE;
	}
	// ???? ????
	else if (creatureType == 923)
	{
		summonActionResult = RESULT_SUMMON_LUPENG;
	}
	// ?????? ????
	else if (creatureType == 926)		//??????? ?????? ??????? ??? -- ????, ???? ????.
	{
#if __CONTENTS(__2008_OCTOBER_MAKETITEM)
		summonActionResult = SKILL_CLIENT_BEARBOX_SUMMON;
#endif //__2008_OCTOBER_MAKETITEM
	}
	else if (creatureType == 927)		//?? ??????? ???
	{
		summonActionResult = RESULT_MAGIC_BLOODY_TUNNEL_INTO;
	}
#if __CONTENTS(__NEW_PET_FIRE_ELEMENTAL)
	else if (creatureType == 1043)		// ????? ???????
	{
#if __CONTENTS(__2008_OCTOBER_MAKETITEM)		
		summonActionResult = SKILL_CLIENT_FIREELEMENTAL_SUMMON;
#endif //__2008_OCTOBER_MAKETITEM	
	}
#endif //__NEW_PET_FIRE_ELEMENTAL
#if __CONTENTS(__2008_OCTOBER_MAKETITEM)
	else if (creatureType == 1055)		// ???????
	{
		summonActionResult = SKILL_CLIENT_SUCCUBUS_SUMMON;
	}
#endif //__2008_OCTOBER_MAKETITEM
#if __CONTENTS(__NEW_PET_INCUBUS)
	else if (creatureType == 1059)		// ???????
	{
		summonActionResult = SKILL_CLIENT_SUCCUBUS_SUMMON;
	}
#endif //__NEW_PET_INCUBUS
#if __CONTENTS(__PET_MASHIMARO)
	else if (creatureType == 1082)		// ???ø???
	{
		summonActionResult = SKILL_CLIENT_MASHIMARO_SUMMON;
	}
#endif //__PET_MASHIMARO
#if __CONTENTS(__PET_CATS) 
	else if (creatureType >= 1083 && creatureType <= 1085)		// ??????
	{
		summonActionResult = SKILL_CLIENT_CATS_SUMMON;
	}
#endif //__PET_CATS
#if __CONTENTS(__PET_CATS_UPGRADED)
	else if (creatureType >= 1089 && creatureType <= 1100)		// ??? ??????, ???? ?????? ????
	{
		summonActionResult = SKILL_CLIENT_CATS_SUMMON;
	}
#endif //__PET_CATS_UPGRADED
#if __CONTENTS(__UPGRADE_MONSTER_PET)
	else if (creatureType >= 1101 && creatureType <= 1112)		// ???,????..????
	{
		summonActionResult = SKILL_CLIENT_CATS_SUMMON;
	}
#endif //__UPGRADE_MONSTER_PET	

	// ??? ??? ????
	ExecuteActionInfoFromMainNode(
		summonActionResult,		// ??? ??? ???
		x, y, 0,
		DIRECTION_DOWN,			// ??? ????
		pCreature->GetID(),		// ????? ???? ????
		x, y, 0,
		0,						// ????? (????) ???? ?ð?		
		NULL,
		false);
}

//------------------------------------------------------------------
// Move Into Portal
//------------------------------------------------------------------
// ??? ?????? ??????? ??? ???.. 
// Vampire, ACVampire ???? ?????
// MTopView?? ???
//------------------------------------------------------------------
void MTopView::VampireMoveIntoPortal(MCreature* pCreature)
{
	// Vampire ??? ???? ???? ?????? ???
	if (pCreature == NULL || !pCreature->IsVampire())
		return;

	int x = pCreature->GetX();
	int y = pCreature->GetY();
	MFakeCreature* pFakeCreature = g_pZone->NewFakeCreature(pCreature, x, y);

	pFakeCreature->SetFakeCreatureType(MFakeCreature::FAKE_CREATURE_TO_PORTAL);

	//------------------------------------------------------
	// Fake Creature?? Zone?? ???
	//------------------------------------------------------
	if (!g_pZone->AddFakeCreature(pFakeCreature))
	{
		delete pFakeCreature;
	}

	//------------------------------------------------------
	// ???? ????? ????? ????? ????? ???..
	//------------------------------------------------------
	ExecuteActionInfoFromMainNode(
		RESULT_MAGIC_BLOODY_TUNNEL_INTO,										// ??? ??? ???

		x, y, 0,
		DIRECTION_DOWN, // ??? ????

		pFakeCreature->GetID(),												// ????? ???? ????
		x, y, 0,

		0,													// ????? (????) ???? ?ð?		

		NULL,

		false);
}
CIndexSprite&
MTopView::GetCreatureSprite(int spriteIdx, int body)
{
	//int a = (*g_pCreatureSpriteTable)[body].FrameID;
	if (body < 287)
		return m_CreatureSPK[spriteIdx];
	//#if __CONTENTS(__ONIBLA)
	else
		return m_CreatureSPK2[spriteIdx];
	//#endif //__ONIBLA
}

CShadowSprite&
MTopView::GetCreatureShadowSprite(int spriteIdx, int body)
{
	//int a = (*g_pCreatureSpriteTable)[body].FrameID;
	if (body < 287)
		return m_CreatureSSPK[spriteIdx];
	//#if __CONTENTS(__ONIBLA)
	else
		return m_CreatureSSPK2[spriteIdx];
	//#endif //__ONIBLA
}
