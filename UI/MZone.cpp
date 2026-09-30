//----------------------------------------------------------------------
// MZone.cpp
//----------------------------------------------------------------------
#include "Client_PCH.h"
#pragma warning(disable:4786)

#include "UserOption.h"
#include <math.h>
#include <fstream>

#include "MZoneDef.h"
#include "MSector.h"
#include "MCreature.h"
#include "MFakeCreature.h"
#include "MPlayer.h"
#include "MImageObject.h"
#include "MShadowObject.h"
#include "MAnimationObject.h"
#include "MShadowAnimationObject.h"
#include "MInteractionObject.h"
#include "MPortal.h"
#include "MItem.h"
#include "MEffect.h"
#include "MZone.h"
#include "MTopView.h"
#include "MEffectGeneratorTable.h"
#include "MZoneTable.h"
#include "CMessageArray.h"
#include "DebugInfo.h"
#include "ServerInfo.h"
#include "Client.h"
#include "EffectSpriteTypeDef.h"
#include "SoundDef.h"
#include "MEffectSpriteTypeTable.h"
#include "MHelicopterManager.h"
#include "MParty.h"
#include "MTestDef.h"
#include "SkillDef.h"
#include "mintr.h"
#include "MNPC.h"
#include "MGuildType.h"
#include "MEffectStatusDef.h"

#include "MHelpDef.h"

//#include "VS_UI.h"
//#define	new			DEBUG_NEW
//#define	delete		DEBUG_DELETE

//----------------------------------------------------------------------
// Global
//----------------------------------------------------------------------
MZone*				g_pZone				= NULL;

BOOL g_bPlayPropeller = FALSE;

extern HWND					g_hWnd;
extern DWORD				g_ZoneRandomSoundTime;

extern void		SendPositionInfoToParty();
extern void		SendStatusInfoToParty();

#ifdef __METROTECH_TEST__
	extern int		g_iSpeed;
#endif

#ifdef	OUTPUT_DEBUG
	//extern CMessageArray		g_DebugMessage;
	#ifdef __METROTECH_TEST__
//		#define OUTPUT_DEBUG_UPDATE_ZONE
	#endif
//	#define OUTPUT_DEBUG_UPDATE_EFFECT
#endif

// ImageObject�� ���� ���� ���
//#ifdef __OUTPUT_IMAGEOBJECT__

#define	VISION_SECTOR_WIDTH_HALF				12
#define	VISION_SECTOR_HEIGHT_HALF				13

// 800x600 �ػ󵵶� �þ� ���ʹ� 1024x768�� �����ϴ�.
// ���������� �ػ��� ������ ���� ����
#define NEW_VISION_SECTOR_WIDTH_HALF			12
#define NEW_VISION_SECTOR_HEIGHT_HALF_UP		17
#define NEW_VISION_SECTOR_HEIGHT_HALF_DOWN		17
/*#ifdef CONVERT_1024_768
#else
	#define NEW_VISION_SECTOR_WIDTH_HALF			10
	#define NEW_VISION_SECTOR_HEIGHT_HALF_UP		14
	#define NEW_VISION_SECTOR_HEIGHT_HALF_DOWN		14
#endif*/

//----------------------------------------------------------------------
// Is RelicTable [��������]
//----------------------------------------------------------------------
bool
IsRelicTable( MItem* pItem )
{
	if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)
	{
		MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();
		
		return pCreature->GetCreatureType()>=371 && pCreature->GetCreatureType()<=376 ||
				pCreature->GetCreatureType()>=560 && pCreature->GetCreatureType()<=563 ||
				pCreature->GetCreatureType()>=526 && pCreature->GetCreatureType()<=549;
	}

	return false;
}

//----------------------------------------------------------------------
// define function ����
//----------------------------------------------------------------------
#define	CheckCreatureInDarkness( sector, x, y )						\
		{															\
			if (g_pPlayer->GetX()==x && g_pPlayer->GetY()==y)		\
			{														\
				g_pPlayer->CheckInDarkness();						\
			}														\
																	\
			int numCreature = sector.GetCreatureSize();				\
																	\
			if (numCreature > 0)									\
			{														\
				OBJECT_MAP::iterator iCreature = sector.GetCreatureIterator();	\
																		\
				for (int i=0; i<numCreature; i++, iCreature ++)			\
				{														\
					if (iCreature==sector.GetObjectEnd()					\
						|| iCreature->first > MSector::POSITION_FLYINGCREATURE_MAX)	\
						break;												\
																			\
					MCreature* pCreature = (MCreature*)iCreature->second;	\
																		\
					if (pCreature!=NULL)								\
					{													\
						pCreature->CheckInDarkness();					\
					}													\
				}														\
			}															\
																		\
			if (sector.IsExistItem())									\
			{															\
				MItem* pItem = sector.GetItem();						\
																		\
				if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)	\
				{														\
					MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();	\
																		\
					if (pCreature!=NULL)								\
					{													\
						pCreature->CheckInDarkness();					\
					}													\
				}														\
			}															\
		}
				
//----------------------------------------------------------------------
// RemoveSectorEffect [�����9]
//----------------------------------------------------------------------
#define RemoveSectorEffect( sX, sY, id )						\
		{														\
			MSector& sector = m_ppSector[sY][sX];				\
																\
			bool bSanctuary = sector.HasSanctuary();			\
			sector.RemoveEffect( id );							\
																\
			if (bSanctuary && !sector.HasSanctuary())			\
			{													\
				for (int i=-1; i<=1; i++)						\
				{												\
					for (int j=-1; j<=1; j++)					\
					{											\
						int x0 = sX + i;						\
						int y0 = sY + j;						\
																\
						if (i==0 && j==0						\
							|| x0<0 || x0>=m_Width				\
							|| y0<0 || y0>=m_Height)			\
						{										\
							continue;							\
						}										\
																\
						MSector& sector2 = m_ppSector[y0][x0];	\
						sector2.UnSetSanctuary();				\
					}											\
				}												\
			}													\
		}


//----------------------------------------------------------------------
//
// constructor/destructor
//
//----------------------------------------------------------------------

MZone::MZone(TYPE_SECTORPOSITION width, TYPE_SECTORPOSITION height)
{
	m_Width = 0;
	m_Height =0;

	m_ppSector	= NULL;
	//m_pPlayer	= NULL;

	Init(width, height);

	m_bZoneSoundLoaded	= false;

#if __CONTENTS(__RACE_OUSTERS)

	DEBUG_ADD("Load HornInfo");
	int zoneNum, portalNum;
	UI_PORTAL_FLAG portal;
	UI_PORTAL_LIST portalList;

	ivfstream hornInfoFile(g_pFileDef->getProperty("FILE_INFO_HORN").c_str(), std::ios_base::binary);
	hornInfoFile.read((char *)&zoneNum, sizeof(int));

	for(int zone = 0; zone < zoneNum; zone++)
	{
		hornInfoFile.read((char *)&portalNum, sizeof(int));

		for(int p = 0; p < portalNum; p++)
		{
			hornInfoFile.read((char *)&portal.zone_id, sizeof(int));
			hornInfoFile.read((char *)&portal.x, sizeof(int));
			hornInfoFile.read((char *)&portal.y, sizeof(int));
			hornInfoFile.read((char *)&portal.portal_x, sizeof(int));
			hornInfoFile.read((char *)&portal.portal_y, sizeof(int));
			portalList.push_back(portal);
		}
		m_horn.push_back(portalList);
		portalList.clear();
	}
	hornInfoFile.close();
	DEBUG_ADD("Load HornInfo OK");

#endif // __RACE_OUSTERS
}

MZone::~MZone()
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
void
MZone::Init(TYPE_SECTORPOSITION width, TYPE_SECTORPOSITION height)
{
	// 0 �� ������ return
	if (width==0 || height==0) 
		return;

	// memory����
	Release();

	m_Width	 = width;
	m_Height = height;

	m_ppSector = new MSector* [m_Height];

	int i;
	for (i=0; i<m_Height; i++)
	{
		m_ppSector[i] = new MSector [m_Width];
	}

	//for (i=0; i<m_Height; i++)
	//{
	//	for (int j=0; j<m_Width; j++)
	//	{
	//		m_ppSector[i][j].SetFilterSpriteID( 0 );
	//	}
	//}
}

//----------------------------------------------------------------------
// Release
//----------------------------------------------------------------------
void
MZone::Release()
{
	//---------------------------------
	// player ��ġ ����
	//---------------------------------
	//m_pPlayer = NULL;

	DEBUG_ADD( "Release ImageObject" );
	
	//---------------------------------
	// ImageObject ����
	//---------------------------------
	IMAGEOBJECT_MAP::iterator iImageObject = m_mapImageObject.begin();

	MImageObject *pImageObject = NULL;
	while (iImageObject != m_mapImageObject.end())
	{
		pImageObject = (*iImageObject).second;
		if (pImageObject!=NULL)
		{
			delete pImageObject;					
		}
		iImageObject++;
	}
	m_mapImageObject.clear();


	DEBUG_ADD( "Start Release Object" );
	
	//---------------------------------
	// object�� ����
	//---------------------------------
	ReleaseObject();

	//---------------------------------
	// Obstacle ����
	//---------------------------------
	/*
	OBSTACLE_LIST::iterator iObstacle = m_listObstacle.begin();

	MObstacle *pObstacle = NULL;
	while (iObstacle != m_listObstacle.end())
	{
		pObstacle = *iObstacle;
		if (pObstacle != NULL)
			delete pObstacle;
		iObstacle++;
	}
	m_listObstacle.clear();
	*/

	DEBUG_ADD_FORMAT( "MZone::Release() - %d x %d", m_Width, m_Height);

	//---------------------------------
	// sector ����
	//---------------------------------	
	if (m_ppSector!=NULL)
	{
		int i;
		for (i=0; i<m_Height; i++)
		{
			delete [] m_ppSector[i];			
		}

		delete [] m_ppSector;		
		
		m_ppSector=NULL;
		m_Width = 0;
		m_Height =0;
	}


	m_bZoneSoundLoaded = false;
	DEBUG_ADD("zone rel ok");
}

//----------------------------------------------------------------------
// Release Object
//----------------------------------------------------------------------
// instance object���� �����Ѵ�.
//----------------------------------------------------------------------
void
MZone::ReleaseObject()
{
	m_HelicopterManager.Release();

	//---------------------------------
	// map�� �ִ� Creature����
	//---------------------------------
	DEBUG_ADD_FORMAT("MZone::ReleaseCreature. size=%d", m_mapCreature.size());
	
	CREATURE_MAP::iterator iCreature;
	iCreature = m_mapCreature.begin();

	MCreature *pCreature = NULL;
	while (iCreature != m_mapCreature.end())
	{
		pCreature = (*iCreature).second;
		// player�� ������ �ʴ´�.
		if (pCreature!=NULL && pCreature->GetClassType()!=MCreature::CLASS_PLAYER)
		{
			DEBUG_ADD_FORMAT("RemoveCreature: [%s] id=%d, (%d, %d)", pCreature->GetName(), pCreature->GetID(), pCreature->GetX(), pCreature->GetY());
			
			/*
			if (g_pParty!=NULL && pCreature->IsPlayerParty())
			{
				PARTY_INFO* pInfo = g_pParty->GetMemberInfo( GetName() );

				if (pInfo!=NULL)
				{
					pInfo->bInSight = false;					
					pInfo->zoneID = 60002;
				}
			}
			*/
			
			// sector���� ����
			if (m_ppSector!=NULL)
			{
				int id = pCreature->GetID();
				int x = pCreature->GetX();
				int y = pCreature->GetY();
				int serverX = pCreature->GetServerX();
				int serverY = pCreature->GetServerY();

				UnSetServerBlock(pCreature->GetMoveType(), serverX, serverY);

				//if (x>=0 && y>=0 && x<m_Width && y<m_Height) 
				{
					//------------------------------------------------
					// sector���� ���Ž�Ų��.
					//------------------------------------------------
					if (!m_ppSector[y][x].RemoveCreature(id))
					{
						if (!m_ppSector[serverY][serverX].RemoveCreature(id))
						{	
							DEBUG_ADD_FORMAT("Can't RemoveCreatureWhenRelease! ID=%d client(%d,%d), server(%d,%d)", pCreature->GetID(), x,y,serverX,serverY);
						}
					}

					/*
					bool removed = m_ppSector[y][x].RemoveGroundCreature( id );
					if (!removed)
					{
						removed = m_ppSector[serverY][serverX].RemoveGroundCreature( id );

						if (!removed)
						{
							removed = m_ppSector[y][x].RemoveFlyingCreature( id );

							if (!removed)
							{
								removed = m_ppSector[serverY][serverX].RemoveFlyingCreature( id );

								if (!removed)
								{
									removed = m_ppSector[y][x].RemoveUndergroundCreature( id );

									if (!removed)
									{
										removed = m_ppSector[serverY][serverX].RemoveUndergroundCreature( id );

										DEBUG_ADD_FORMAT("[Error] Can't Remove Creature. ID=%d. xy=(%d, %d), sxy=(%d, %d)", id, x, y, serverX, serverY);
									}
								}
							}
						}
					}
					*/
				}
			}
		
			DEBUG_ADD("RemoveCreature: OK");
			
			delete pCreature;

			DEBUG_ADD("DeleteCreature: OK");
		}
		iCreature++;
	}
	m_mapCreature.clear();

	//---------------------------------
	// map�� �ִ� FakeCreature����
	//---------------------------------
	DEBUG_ADD_FORMAT("MZone::ReleaseFakeCreature. size=%d", m_mapFakeCreature.size());
	
	CREATURE_MAP::iterator iFakeCreature= m_mapFakeCreature.begin();

	while (iFakeCreature != m_mapFakeCreature.end())
	{
		pCreature = (*iFakeCreature).second;
		if (pCreature!=NULL)
		{
			delete pCreature;					
		}
		iFakeCreature++;
	}
	m_mapFakeCreature.clear();


	//---------------------------------
	// map�� �ִ� Item����
	//---------------------------------
	DEBUG_ADD_FORMAT("MZone::ReleaseItem. size=%d", m_mapItem.size());
	
	ITEM_MAP::iterator iItem = m_mapItem.begin();

	MItem *pItem = NULL;
	while (iItem != m_mapItem.end())
	{
		pItem = (*iItem).second;
		if (pItem!=NULL)
		{
			if (m_ppSector!=NULL)
			{
				// [��������]
				MSector& sector = m_ppSector[pItem->GetY()][pItem->GetX()];
				sector.RemoveItem();

				// Ȥ�� ������ �������.. ��ü ����..
				if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)
				{
					sector.UnSetBlockServerGround();

					// [��������]
					if (IsRelicTable(pItem))
					{
						sector.UnSetBlockGround();
						sector.UnSetBlockFlying();
					}
				}
			}

			delete pItem;					
		}
		iItem++;
	}
	m_mapItem.clear();

	//---------------------------------
	// Effect ����
	//---------------------------------
	DEBUG_ADD_FORMAT("MZone::ReleaseEffect. size=%d", m_mapEffect.size());
	
	EFFECT_MAP::iterator iEffect = m_mapEffect.begin();

	MEffect *pEffect = NULL;
	while (iEffect != m_mapEffect.end())
	{
		pEffect = iEffect->second;
		if (pEffect != NULL)
		{
			if (m_ppSector!=NULL)
			{
				// [�����9]
				RemoveSectorEffect( pEffect->GetX(), pEffect->GetY(), pEffect->GetID() );				
			}

			delete pEffect;			
		}
		iEffect++;
	}
	m_mapEffect.clear();

	//---------------------------------
	// Ground Effect ����
	//---------------------------------
	DEBUG_ADD_FORMAT("MZone::ReleaseGroundEffect. size=%d", m_mapGroundEffect.size());
	
	iEffect = m_mapGroundEffect.begin();

	while (iEffect != m_mapGroundEffect.end())
	{
		pEffect = iEffect->second;
		if (pEffect != NULL)
		{	
			delete pEffect;			
		}
		iEffect++;
	}
	m_mapGroundEffect.clear();


	//---------------------------------
	// Sound ����
	//---------------------------------
	DEBUG_ADD_FORMAT("MZone::ReleaseSound. size=%d", m_listSoundNode.size());
	
	SOUND_NODE_LIST::iterator iSound = m_listSoundNode.begin();

	while (iSound != m_listSoundNode.end())
	{
		SOUND_NODE*	pNode = *iSound;

		// �����.
		delete pNode;
		
		iSound++;			
	}

	m_listSoundNode.clear();

	// 2004, 11, 29, sobeit add start
	//-----------------------------------------------
	// wait effect list ����...
	//-----------------------------------------------
	DEBUG_ADD_FORMAT("MZone::ReleaseWaitEffect. size=%d", m_listWaitEffect.size());
	WAIT_EFFECT_LIST::iterator iWaitEffect = m_listWaitEffect.begin();

	while (iWaitEffect != m_listWaitEffect.end())
	{
		MEffect*	pNode = *iWaitEffect;

		delete pNode;

		iWaitEffect ++;
	}
	m_listWaitEffect.clear();
	// 2004, 11, 29, sobeit add end
	
	DEBUG_ADD("MZone::Release OK");
}

//----------------------------------------------------------------------
// Save To File SectorSound
//----------------------------------------------------------------------
// Width*Height
// SectorSoundInfo
// //MZoneSoundTable
//----------------------------------------------------------------------
bool		
MZone::SaveToFileSectorSound(std::ofstream& file)
{
	//-----------------------------------------------------------
	// ũ��
	//-----------------------------------------------------------
	file.write((const char*)&m_Width, SIZE_SECTORPOSITION);
	file.write((const char*)&m_Height, SIZE_SECTORPOSITION);

	//-----------------------------------------------------------
	// SectorSoundInfo
	//-----------------------------------------------------------
	for (int x=0; x<m_Width; x++)
	{
		for (int y=0; y<m_Height; y++)
		{
			const SECTORSOUND_LIST& listSound = m_ppSector[y][x].GetSectorSoundList();

			SECTORSOUND_LIST::const_iterator iSound = listSound.begin();

			int num = listSound.size();

			// �� Sector�� �ִ� sound�� ���� ����
			file.write((const char*)&num, 4);

			// �� sound����
			while (iSound != listSound.end())
			{
				const SECTORSOUND_INFO& info = *iSound;

				info.SaveToFile( file );

				iSound++;
			}			
		}
	}

	/*
	//-----------------------------------------------------------
	// MZoneSoundTable
	//-----------------------------------------------------------
	// ��.. ��������.. - -;
	// ��ư ���ǻ�(-_-;) ���� �־������. ����..
	//-----------------------------------------------------------
	bool bExistZoneTable = (g_pZoneTable!=NULL);

	// üũ��..
	file.write((const char*)&bExistZoneTable, 1);

	if (bExistZoneTable)
	{
		g_pZoneTable->SaveToFile( file );

		return true;
	}	

	return false;
	*/

	return true;
}

//----------------------------------------------------------------------
// Load From File SectorSound
//----------------------------------------------------------------------
// Width*Height�� ũ�� üũ�ϰ�.
// SectorSoundInfo�� Loading�ϰ�
// //MZoneSoundTable�� Loading�Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::LoadFromFileSectorSound(ivfstream& file)
{
	m_bZoneSoundLoaded = false;

	/*
	//---------------------------------------------------------------
	// Sector�� ���� ������ Loading�Ѵ�.
	//---------------------------------------------------------------
	// test code
	for (int x=0; x<m_Width; x++)
	{
		for (int y=0; y<m_Height; y++)
		{
			// ZoneSound 1���� 50,50���� �Ҹ����ٰ� ��� sector�� ���� �߰�
			// ��� ��� ���� �Ÿ��� �ȵǸ� �߰��� �ʿ�� ����.
			// �ϴ� �����Ƽ� �׽�Ʈ��.. - -;;
			//		DEBUG_ADD_FORMAT("AddSectorSound[%d][%d]", y, x);
			
		
			m_ppSector[y][x].AddSectorSound( 1, 30, 30 );		
		}
	}
	*/

	//-----------------------------------------------------------
	// ũ��
	//-----------------------------------------------------------
	//TYPE_SECTORPOSITION width, height;

	//file.read((char*)&width, SIZE_SECTORPOSITION);
	//file.read((char*)&height, SIZE_SECTORPOSITION);

	//if (m_Width!=width || m_Height!=height)
	{
		// Zoneũ��� SectorSound������ ũ�Ⱑ �ٸ� ���
		// �����̴� - -;
	//	return false;
	}

	//-----------------------------------------------------------
	// SectorSoundInfo
	//-----------------------------------------------------------
	//
	// �Ʒ��� ���� �����ϰ�..
	//
	//	 -         : �Ҹ��� ���� �ʴ� Sector
	//	 A,C,D     : 1���� SECTORSOUND_INFO�� �ִ� Sector (���� �Ҹ��� �ٸ�)
	//   B		   : 2���� SECTORSOUND_INFO�� �ִ� Sector
	//   E         : 3���� SECTORSOUND_INFO�� �ִ� Sector
	//   a~z       : SECTORSOUND_INFO
	//
	// ������ ���� ���ڸ�..
	//
	// ex) -----AAABBBAAC--DEEEEE---
	//
	//	   2						// �� �ٿ����� �ݺ�ȸ�� 2ȸ(-----AAABBBAAC, --DEEEEE )
	//                              // ������ ���κ��� �Ҹ����� sector�� �����ϸ� �ȴ�.
	//
	//      5						// ù��° �ݺ�. �Ҹ����� sector��(-----)
	//       4						// sound���� ( AAA, BBB, AA, C )
	//        {3 { 1 a }}			// 3���� sector( AAA )��  1���� info
	//        {3 { 2 b, b' }}		// 3���� sector( BBB )��  2���� info
	//        {2 { 1 a }}			// 2���� sector( AA )��  1���� info
	//        {1 { 1 c }}			// 1���� sector( C )��  1���� info
	//
	//      2                       // �ι�° �ݺ�. �Ҹ����� sector��(--)
	//       2						// sound���� ( D, EEEE )
	//        {1 { 1 d }}			// 1���� sector( D )��  1���� info
	//        {5 { 3 e, e', e" }}   // 5���� sector( EEEEE )�� 3���� info	
	//
	//-----------------------------------------------------------
	BYTE				num;
	SECTORSOUND_INFO	info;

	for (int y=0; y<m_Height; y++)
	{
		//-----------------------------------------------------------
		// �ݺ� ȸ�� : { 0�� ����, ���� ������ * { info * ���� } }
		//-----------------------------------------------------------
		file.read((char*)&num, 1);

		int x = 0;
		int numCount = num;
		
		//-----------------------------------------------------------
		// �ݺ�ȸ����ŭ..
		//-----------------------------------------------------------
		for (int i=0; i<numCount; i++)
		{
			//-----------------------------------------------------
			// �Ҹ� �ȳ��� sector��
			//-----------------------------------------------------
			file.read((char*)&num, 1);	
			x += num;	// ��ǥ skip

			//-----------------------------------------------------
			// �̹� �ݺ����� �Ҹ����� sector�鿡�� 
			// �������� ���� SECTORSOUND_INFO�� ������
			//-----------------------------------------------------
			file.read((char*)&num, 1);	
			int numSoundType = num;

			//-----------------------------------------------------
			// SECTORSOUND_INFO�� ��������ŭ �ݺ�
			//-----------------------------------------------------
			for (int j=0; j<numSoundType; j++)
			{
				//-----------------------------------------------------
				// ���� info�� ���� sector�� ��� �����ؼ� �ִ°�?
				//-----------------------------------------------------
				file.read((char*)&num, 1);		// assert( num > 0 );
				int numSector = num;

				//-----------------------------------------------------
				// sector�� SECTORSOUND_INFO�� ��� �ִ°�?
				//-----------------------------------------------------
				file.read((char*)&num, 1);		// assert( num > 0 );
				int numSound = num;

				//-----------------------------------------------------
				// sound�� �����鼭 sector�鿡 �����ؼ� info�� �߰��Ѵ�.
				//-----------------------------------------------------
				for (int n=0; n<numSound; n++)
				{
					//-----------------------------------------------------
					// �ϳ��� SECTORSOUND_INFO�� load
					//-----------------------------------------------------
					info.LoadFromFile( file );
					
					int xPlusNumSector = x + numSector;
					for (int k=x; k<xPlusNumSector; k++)
					{
						m_ppSector[y][k].AddSectorSound( info );
					}
				}

				x += numSector;
			}
		}
	}

	/*
	for (int x=0; x<m_Width; x++)
	{
		for (int y=0; y<m_Height; y++)
		{
			file.read((char*)&num, 1);

			#ifdef _DEBUG
				if (file.eof())
				{
					int a = 0;
				}
			#endif

			intNum = num;

			for (int n=0; n<intNum; n++)
			{
				info.LoadFromFile( file );

				#ifdef _DEBUG
					if (file.eof())
					{
						int a = 0;
					}
				#endif

				m_ppSector[y][x].AddSectorSound( info );			
			}
		}
	}
	*/
	

	//-----------------------------------------------------------
	// MZoneSoundTable
	//-----------------------------------------------------------
	// ��.. ��������.. - -;
	// ��ư ���ǻ�(-_-;) ���� �־������. ����..
	//-----------------------------------------------------------
	/*
	bool bExistZoneTable;

	// üũ��..
	file.read((char*)&bExistZoneTable, 1);

	if (bExistZoneTable)
	{
		g_pZoneTable->LoadFromFile( file );
		
		m_bZoneSoundLoaded = true;

		return true;
	}

	return false;
	*/

	m_bZoneSoundLoaded = true;
	return true;

}

//----------------------------------------------------------------------
// Load From File
//----------------------------------------------------------------------
//   ���� size, ���� size
//   Zone ID
//   �Ӽ�
//   ����size * ����size ���� Sector����(TileSpriteID, Flag)
//   Obstacle��,  Obstacle�� * Obstacle ����
//   ImageObject��,  ImageObject�� * (ImageObject, ImageObjectSectorInfo)
//----------------------------------------------------------------------
bool		
MZone::LoadFromFile(ivfstream& file)
{
	//-------------------------------------------------
	// Header
	//-------------------------------------------------
	m_Info.LoadFromFile( file );
	

	//-------------------------------------------------
	//
	// 5�� 11�� version
	//
	//-------------------------------------------------
	if (m_Info.ZoneVersion==MAP_VERSION_2000_05_10)
	{
		//-------------------------------------------------
		// ZoneID 
		// �Ӽ�
		//-------------------------------------------------	
		file.read((char*)&m_fpTile, 4);				// Tile FP
		file.read((char*)&m_fpImageObject, 4);		// ImageObject FP

		//-------------------------------------------------
		// �̹� �ִ� �� ����
		//-------------------------------------------------
		Release();

		//-------------------------------------------------
		// Zone�� ���� Size, 
		//        ���� Size
		//-------------------------------------------------
		file.read((char*)&m_Width, 2);
		file.read((char*)&m_Height, 2);

		// �ƹ��͵� ���� ���
		if (m_Width==0 || m_Height==0)
			return false;

		//-------------------------------------------------
		// Zone�� �� Sector���� Load
		//-------------------------------------------------
		// memory���
		Init(m_Width, m_Height);

		///*
		int i,j;

		for (i=0; i<m_Height; i++)
		{
			for (j=0; j<m_Width; j++)
			{
				m_ppSector[i][j].LoadFromFile(file);			
			}
		}

		//*/
		/*
		struct MSector_st
		{
			TYPE_SPRITEID	spriteID;
			BYTE			property;
			BYTE			light;
		};

		MSector_st* tempSector = new MSector_st[m_Height*m_Width];

		file.read((char*)tempSector, sizeof(MSector_st)*m_Height*m_Width);

		int i,j;
		
		MSector_st* tempSectorPtr = tempSector;

		for (i=0; i<m_Height; i++)
		{
			for (j=0; j<m_Width; j++)
			{
				m_ppSector[i][j].Set( tempSectorPtr->spriteID, tempSectorPtr->property );
				// light�� �ǹ̾���.

				tempSectorPtr++;
			}
		}
		
		delete [] tempSector;
		*/

		//-------------------------------------------------
		// ���� ���� Filter ��� ����
		//-------------------------------------------------
		// �̺κ��� MapEditor����??
		/*
		for (i=0; i<m_Height; i++)
		{
			for (j=0; j<m_Width; j++)
			{
				if (m_ppSector[i][j].GetLight()!=0)	
				{
					SetLight(j,i, 0);
				}
			}
		}
		*/
		

		int size;

		//-------------------------------------------------
		// Portal�� �о���δ�.
		//-------------------------------------------------
		// 2001.7.11�� ����
		/*
		file.read((char *)&size, 4);

		// Client������ �ʿ���� �����̹Ƿ� �о���̱⸸ �Ѵ�.
		// [!] �ƿ� file position�� ����ؼ� �ǳʶ���� �ְڴ�.
		MPortal portal;
		for (i=0; i<size; i++)
		{
			portal.LoadFromFile( file );
		}
		*/

		//-------------------------------------------------
		// ImageObject ������ Load
		//-------------------------------------------------	
		file.read((char *)&size, 4);

		//-------------------------------------------------
		// Zone�� ImageObject���� Load
		//-------------------------------------------------
		MImageObject				*pImageObject = nullptr;
		IMAGEOBJECT_POSITION_LIST	ImageObjectPositionList;
		BYTE						ObjectType;
		for (i=0; i<size; i++)	
		{
			#ifdef __OUTPUT_IMAGEOBJECT__
				char str[1024];
			#endif

			// ImageObject memory�� ��� Load�Ѵ�.
			file.read((char*)&ObjectType, 1);

			switch (ObjectType)
			{
				case MObject::TYPE_IMAGEOBJECT :
					pImageObject = new MImageObject;
					
					#ifdef __OUTPUT_IMAGEOBJECT__	
						strcpy(str, "ImageObject : ");
					#endif
				break;

				case MObject::TYPE_SHADOWOBJECT :
					pImageObject = new MShadowObject;

					#ifdef __OUTPUT_IMAGEOBJECT__
						strcpy(str, "ShadowObject : ");
					#endif
				break;

				case MObject::TYPE_ANIMATIONOBJECT :
					pImageObject = new MAnimationObject;
					
					#ifdef __OUTPUT_IMAGEOBJECT__
						strcpy(str, "AnimationObject : ");
					#endif
				break;

				case MObject::TYPE_SHADOWANIMATIONOBJECT :
					pImageObject = new MShadowAnimationObject;
					
					#ifdef __OUTPUT_IMAGEOBJECT__
						strcpy(str, "ShadowAnimationObject : ");
					#endif
				break;

				case MObject::TYPE_INTERACTIONOBJECT :
					pImageObject = new MInteractionObject;
					
					#ifdef __OUTPUT_IMAGEOBJECT__
						strcpy(str, "InteractionObject : ");
					#endif
				break;
			}

			pImageObject->LoadFromFile(file);	
			
			#ifdef __OUTPUT_IMAGEOBJECT__
				sprintf(str, "%s[%d] vp=%d. ", str, pImageObject->GetImageObjectID(), pImageObject->GetViewpoint());
			#endif

			// ImageObject�� IMAGEOBJECT_MAP�� �߰��Ѵ�.
			AddImageObject(pImageObject);

			//-------------------------------------------------
			// ��� Load�� ImageObject�� �����ϴ� 
			// Sector���� ��ǥ�� Load�ؾ��Ѵ�.
			// (*) �� ������ �� �̻� �����ϰ� ���� �ʴ´�.
			//-------------------------------------------------		
			ImageObjectPositionList.LoadFromFile(file);		
			
			//-------------------------------------------------------
			// Load�� ImageObjectPositionList�� �� Position�� ���ؼ� 
			// Zone�� ImageObjectSector�� ǥ���ؾ� �Ѵ�.
			//-------------------------------------------------------
			IMAGEOBJECT_POSITION_LIST::POSITION_LIST::const_iterator 
				iImageObjectPosition = ImageObjectPositionList.GetIterator();


			/*
			// File�� SpriteID�� 61���� ���� ���� ���
			char str[256];
			char str2[80];
			if (pImageObject->GetSpriteID()==61)
			{
				sprintf(str, "IO[%d] Pixel=(%d, %d)  View=%d  Position=", pImageObject->GetID(), pImageObject->GetPixelX(), pImageObject->GetPixelY(), pImageObject->GetViewpoint());
			}
			*/

			// �� Sector�� ImageObjectǥ��
			for (int j=0; j<ImageObjectPositionList.GetSize(); j++)
			{
				#ifdef __OUTPUT_IMAGEOBJECT__
					sprintf(str, "%s(%d,%d) ", str, (int)(*iImageObjectPosition).X, (int)(*iImageObjectPosition).Y);
				#endif

				SetImageObjectSector((*iImageObjectPosition).X, (*iImageObjectPosition).Y, pImageObject->GetID());

				/*
				// File�� SpriteID�� 61���� ���� ���� ���
				if (pImageObject->GetSpriteID()==61)
				{
					sprintf(str2, "(%d, %d)  ", (*iImageObjectPosition).X, (*iImageObjectPosition).Y);
					strcat(str, str2);			
				}
				*/

				iImageObjectPosition++;			
			}			

			#ifdef __OUTPUT_IMAGEOBJECT__
				if (g_pDebugMessage!=NULL)
					g_pDebugMessage->AddToFile( str );
			#endif
			/*
			// File�� SpriteID�� 61���� ���� ���� ���
			if (pImageObject->GetSpriteID()==61)
			{
				strcpy(g_pDebugMessage->GetCurrent(), str);
				g_pDebugMessage->Next();
			}
			*/
		}					
	}
	//-----------------------------------------------------------------
	// �ٸ� ����??
	//-----------------------------------------------------------------
	else
	{
		return false;
	}



	return true;
}

//----------------------------------------------------------------------
// Set Player 
//
// Player�� Zone�� �߽ɿ��� ��µȴ�.
// Player�� Zone�� Sector�� ��ġ��Ų��.
//----------------------------------------------------------------------
void		
MZone::SetPlayer()//MPlayer* pPlayer)
{
	// Player Setting
	//m_pPlayer = pPlayer; 

	// Sector Setting
	//m_ppSector[m_pPlayer->GetY()][m_pPlayer->GetX()].AddGroundCreature(pPlayer);
	//m_ppSector[g_pPlayer->GetY()][g_pPlayer->GetX()].AddGroundCreature(&g_pPlayer->;

	// Player�� �þ� ���� Sector�� ���̰� �Ѵ�.
	//SetSight(g_pPlayer->GetX(), g_pPlayer->GetY(), g_pPlayer->GetLightSight());

	AddCreature( g_pPlayer );
	
	//---------------------------------------------------------------
	// �� ������ ��Ƽ���� �����ش�.
	//---------------------------------------------------------------
	SendPositionInfoToParty();
	SendStatusInfoToParty();
}

//----------------------------------------------------------------------
// Set Player 
//
// Player�� Zone�� �߽ɿ��� ��µȴ�.
// Player�� Zone�� Sector�� ��ġ��Ų��.
//----------------------------------------------------------------------
void		
MZone::RemovePlayer()//MPlayer* pPlayer)
{
	// Player Setting
	//m_pPlayer = pPlayer; 

	// Sector Setting
	//m_ppSector[m_pPlayer->GetY()][m_pPlayer->GetX()].AddGroundCreature(pPlayer);
	//m_ppSector[g_pPlayer->GetY()][g_pPlayer->GetX()].RemoveGroundCreature(&g_pPlayer->;

	// Player�� �þ� ���� Sector�� ���̰� �Ѵ�.
	//SetSight(g_pPlayer->GetX(), g_pPlayer->GetY(), g_pPlayer->GetLightSight());

	RemoveCreature( g_pPlayer->GetID() );	
}


//----------------------------------------------------------------------
// (x,y) sector�� �� �� �ִ°�?
//----------------------------------------------------------------------
// �� �� ���� ��츦 ��� check�Ͽ� 
// return false�� ����� �Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::CanMove(BYTE creatureType, TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	// Zone�� ������ ��� ���
	if (x<0 || x>=m_Width || y<0 || y>=m_Height)
		return false;

	const MSector& sector = m_ppSector[y][x];

	//------------------------------------------------------
	// [�����] Sanctuary �δ� �� �����δ�.
	//------------------------------------------------------
	if (sector.HasSanctuary()
		&& creatureType != MCreature::CREATURE_FAKE_NO_BLOCK
		&& creatureType != MCreature::CREATURE_FAKE_UNDERGROUND
		&& creatureType != MCreature::CREATURE_FAKE_GROUND
		&& creatureType != MCreature::CREATURE_FAKE_FLYING
		)
	{
		return false;
	}


	// �̹� �ٸ� Object�� �����ϴ� ���
	//if (m_ppSector[y][x].IsExistObject())
	{
		// objectType�� ���� �޸� check�� �ش�.
		switch (creatureType)
		{
			// UNDERGROUND CREATURE
			case MCreature::CREATURE_UNDERGROUND :
			case MCreature::CREATURE_FAKE_UNDERGROUND :
				return sector.CanStandUndergroundCreature();// || sector.IsPortal();

			// CREATURE
			case MCreature::CREATURE_GROUND :
			case MCreature::CREATURE_FAKE_GROUND :
				return sector.CanStandGroundCreature();// || sector.IsPortal();

			// FLYING CREATURE
			case MCreature::CREATURE_FLYING :
			case MCreature::CREATURE_FAKE_FLYING :
				return sector.CanStandFlyingCreature();// || sector.IsPortal();

			// fake�� ������ �����δ�.
			case MCreature::CREATURE_FAKE_NO_BLOCK :
				return true;
		}		
	}

	return true;
}

//----------------------------------------------------------------------
// Set ServerBlock
//----------------------------------------------------------------------
void		
MZone::SetServerBlock(BYTE creatureType, TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	// Zone�� ������ ��� ���
	if (x<0 || x>=m_Width || y<0 || y>=m_Height)
		return;

	// �̹� �ٸ� Object�� �����ϴ� ���
	//if (m_ppSector[y][x].IsExistObject())
	{
		// objectType�� ���� �޸� check�� �ش�.
		switch (creatureType)
		{
			// UNDERGROUND CREATURE
			case MCreature::CREATURE_UNDERGROUND :
				m_ppSector[y][x].SetBlockServerUnderground();
			break;

			// CREATURE
			case MCreature::CREATURE_GROUND :
				m_ppSector[y][x].SetBlockServerGround();
			break;

			// FLYING CREATURE
			case MCreature::CREATURE_FLYING :
				m_ppSector[y][x].SetBlockServerFlying();
			break;
		}		
	}
}

//----------------------------------------------------------------------
// UnSet ServerBlock
//----------------------------------------------------------------------
void		
MZone::UnSetServerBlock(BYTE creatureType, TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	// Zone�� ������ ��� ���
	if (x<0 || x>=m_Width || y<0 || y>=m_Height)
		return;

	// �̹� �ٸ� Object�� �����ϴ� ���
	//if (m_ppSector[y][x].IsExistObject())
	{
		// objectType�� ���� �޸� check�� �ش�.
		
		// 2002.3.29
		// block�� �ϴ� �� Ǭ��. ���� �̻��ؼ� - -;
		/*
		switch (creatureType)
		{
			// UNDERGROUND CREATURE
			case MCreature::CREATURE_UNDERGROUND :
				m_ppSector[y][x].UnSetBlockServerUnderground();
			break;

			// CREATURE
			case MCreature::CREATURE_GROUND :
				m_ppSector[y][x].UnSetBlockServerGround();
			break;

			// FLYING CREATURE
			case MCreature::CREATURE_FLYING :
				m_ppSector[y][x].UnSetBlockServerFlying();
			break;
		}
		*/
		MSector& sector = m_ppSector[y][x];
		sector.UnSetBlockServerUnderground();
		sector.UnSetBlockServerGround();
		sector.UnSetBlockServerFlying();
	}
}


//----------------------------------------------------------------------
// Move Ground Creature
//----------------------------------------------------------------------
// (xo,yo)�� �ִ� Creature�� (xn,yn)���� �ű��.
//----------------------------------------------------------------------
bool		
MZone::MoveGroundCreature(MCreature* pCreature, TYPE_SECTORPOSITION xo, TYPE_SECTORPOSITION yo, TYPE_SECTORPOSITION xn, TYPE_SECTORPOSITION yn)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (xn<0 || yn<0 || xn>=m_Width || yn>=m_Height) return false;

	// Player�� ��� --> �̵����� �ʴ´�.
	if (pCreature->GetClassType()==MCreature::CLASS_PLAYER
		// fake�� �̵��� �ǹ̰� ����.
		|| pCreature->IsFakeCreature())
	{
		return true;
	}

	// �̹� �̵����ִ� ���..
	//if (xo==xn && yo==yn)
	//	return false;

	
	//MCreature* pExistCreature = NULL;

	int id = pCreature->GetID();

	//------------------------------------------------
	// sector���� ���Ž�Ų��.
	//------------------------------------------------
	if (!m_ppSector[yo][xo].RemoveCreature(id))
	{
		int xs = pCreature->GetServerX();
		int ys = pCreature->GetServerY();

		if (!m_ppSector[ys][xs].RemoveCreature(id))
		{	
			DEBUG_ADD_FORMAT("Can't Remove! [Ground] ID=%d (%d,%d) --> (%d,%d)", pCreature->GetID(), xo,yo,xn,yn);			
		}
	}

	//------------------------------------------------
	// sector�� �߰���Ų��.
	//------------------------------------------------
	if (!m_ppSector[yn][xn].AddGroundCreature( pCreature ))
	{
		DEBUG_ADD_FORMAT("Can't Add GroundCreature! ID=%d (%d,%d)", pCreature->GetID(), xn,yn);			
	}

	
	/*
	if (!m_ppSector[yo][xo].RemoveGroundCreature(id, pExistCreature))
	{	
		if (!m_ppSector[yo][xo].RemoveFlyingCreature(id, pExistCreature))
		{
			if (!m_ppSector[yo][xo].RemoveUndergroundCreature(id, pExistCreature))
			{
				int xs = pCreature->GetServerX();
				int ys = pCreature->GetServerY();

				if (!m_ppSector[ys][xs].RemoveFlyingCreature(id, pExistCreature))
				{	
					if (!m_ppSector[ys][xs].RemoveGroundCreature(id, pExistCreature))
					{
						if (!m_ppSector[ys][xs].RemoveUndergroundCreature(id, pExistCreature))
						{

							int id = 0;
							if (pExistCreature!=NULL)
								id = pExistCreature->GetID();

							// remove ����
							//MessageBox(g_hWnd, "Already Removed![Flying]", NULL, MB_OK);
							DEBUG_ADD_FORMAT("Already Removed! [Ground] ID=%d (%d,%d) --> (%d,%d) ExistID = %d", pCreature->GetID(), xo,yo,xn,yn, id);
						}
					}
				}
			}
		}
	}	

	MCreature* pOldCreature = m_ppSector[yn][xn].GetGroundCreature();

	//------------------------------------------------
	// �ƹ��� ���� ���
	//------------------------------------------------
	if (pOldCreature == NULL)
	{
		// add								
		if (!m_ppSector[yn][xn].AddGroundCreature( pCreature ))
		{
			DEBUG_ADD_FORMAT("Can't Add GroundCreature! ID=%d (%d,%d)", pCreature->GetID(), xn,yn);			
		}
	}
	//------------------------------------------------
	// ���ο� �ڸ��� �̹� �ٸ� ĳ���Ͱ� �ִ� ���...
	//------------------------------------------------
	else
	{
		DEBUG_ADD_FORMAT("Already ExistCreature! [Ground] ID=%d (%d,%d) --> (%d,%d) ExistID = %d", pCreature->GetID(), xo,yo,xn,yn, pOldCreature->GetID());				
		
		// ���� �ִ� ĳ���Ͱ� Player�� ���
		//if (pOldCreature->GetID()==g_pPlayer->GetID())
		//{
			// �߰� ���� �ʴ´�.
			// �̵��ϱ� ������ �ǵ�����.
			// add								
		//	m_ppSector[xo][yo].AddGroundCreature( pCreature );

		//	return false;
		//}
		//else
		{
			//------------------------------------------------
			// ���� �ִ� ĳ���� ����..
			//------------------------------------------------						
			m_ppSector[yn][xn].RemoveGroundCreature( pOldCreature );

			#ifdef OUTPUT_DEBUG
				if (pOldCreature!=NULL)
					DEBUG_ADD_FORMAT("Remove Creature by New Creature! ID=%d (%d,%d)", pOldCreature->GetID(), xn,yn);
			#endif

			//------------------------------------------------
			// add								
			//------------------------------------------------
			m_ppSector[yn][xn].AddGroundCreature( pCreature );

			// Ȯ�ο�
			pCreature->SetX( xn );
			pCreature->SetY( yn );
			
			//------------------------------------------------
			// ���ŵ� ĳ���͸� ����ε� ��ġ�� �߰��Ѵ�.
			//------------------------------------------------
			if (pOldCreature!=NULL)
			{
				int x = pOldCreature->GetServerX();
				int y = pOldCreature->GetServerY();
				
				// ����ְ�.. ��ǥ�� �ٸ� ���..
				if (x!=xn || y!=yn)
				{
					if (pOldCreature->IsAlive())					
					{				
						pOldCreature->SetStop();
						pOldCreature->MovePosition(	x, y );
					}
					else
					{
						pOldCreature->MovePosition(	x, y );
					}
				}
			}
		}
	}		
	*/

	return true;
}

//----------------------------------------------------------------------
// Move Flying Creature
//----------------------------------------------------------------------
// (xo,yo)�� �ִ� FlyingCreature�� (xn,yn)���� �ű��.
//----------------------------------------------------------------------
bool		
MZone::MoveFlyingCreature(MCreature* pCreature, TYPE_SECTORPOSITION xo, TYPE_SECTORPOSITION yo, TYPE_SECTORPOSITION xn, TYPE_SECTORPOSITION yn)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (xn<0 || yn<0 || xn>=m_Width || yn>=m_Height) return false;

	// Player�� ��� --> �̵����� �ʴ´�.
	if (pCreature->GetClassType()==MCreature::CLASS_PLAYER
		// fake�� �̵��� �ǹ̰� ����.
		|| pCreature->IsFakeCreature())
	{
		return true;
	}

	// �̹� �̵����ִ� ���..
	//if (xo==xn && yo==yn)
	//	return false;

	//MCreature* pExistCreature = NULL;

	int id = pCreature->GetID();

	//------------------------------------------------
	// sector���� ���Ž�Ų��.
	//------------------------------------------------
	if (!m_ppSector[yo][xo].RemoveCreature(id))
	{
		int xs = pCreature->GetServerX();
		int ys = pCreature->GetServerY();

		if (!m_ppSector[ys][xs].RemoveCreature(id))
		{	
			DEBUG_ADD_FORMAT("Can't Remove! [Flying] ID=%d (%d,%d) --> (%d,%d)", pCreature->GetID(), xo,yo,xn,yn);			
		}
	}

	//------------------------------------------------
	// sector�� �߰���Ų��.
	//------------------------------------------------
	if (!m_ppSector[yn][xn].AddFlyingCreature( pCreature ))
	{
		DEBUG_ADD_FORMAT("Can't Add FlyingCreature! ID=%d (%d,%d)", pCreature->GetID(), xn,yn);			
	}

	/*
	// remove�� �� �� ���...
	if (!m_ppSector[yo][xo].RemoveFlyingCreature(id, pExistCreature))
	{	
		if (!m_ppSector[yo][xo].RemoveGroundCreature(id, pExistCreature))
		{
			if (!m_ppSector[yo][xo].RemoveUndergroundCreature(id, pExistCreature))
			{
				int xs = pCreature->GetServerX();
				int ys = pCreature->GetServerY();

				if (!m_ppSector[ys][xs].RemoveFlyingCreature(id, pExistCreature))
				{	
					if (!m_ppSector[ys][xs].RemoveGroundCreature(id, pExistCreature))
					{
						if (!m_ppSector[ys][xs].RemoveUndergroundCreature(id, pExistCreature))
						{

							int id = 0;
							if (pExistCreature!=NULL)
								id = pExistCreature->GetID();

							// remove ����
							//MessageBox(g_hWnd, "Already Removed![Flying]", NULL, MB_OK);
							DEBUG_ADD_FORMAT("Already Removed! [Flying] ID=%d (%d,%d) --> (%d,%d) ExistID = %d", pCreature->GetID(), xo,yo,xn,yn, id);
						}
					}
				}
			}
		}
	}

	// test
	MCreature* pOldCreature = m_ppSector[yn][xn].GetFlyingCreature();

	// �ƹ��� ���� ���
	if (pOldCreature == NULL)
	{
		// add								
		m_ppSector[yn][xn].AddFlyingCreature( pCreature );
	}
	// ���ο� �ڸ��� �̹� �ٸ� ĳ���Ͱ� �ִ� ���...
	else
	{
		DEBUG_ADD_FORMAT("Already ExistCreature! [Flying] ID=%d (%d,%d) --> (%d,%d) ExistID = %d", pCreature->GetID(), xo,yo,xn,yn, pOldCreature->GetID());				

		// ���� �ִ� ĳ���Ͱ� Player�� ���
		//if (pOldCreature->GetID()==g_pPlayer->GetID())
		//{
			// �߰� ���� �ʴ´�.
			// �̵��ϱ� ������ �ǵ�����.
			// add								
		//	m_ppSector[xo][yo].AddFlyingCreature( pCreature );

		//	return false;
		//}
		//else
		{
			// ���� �ִ� ĳ���� ����..
			m_ppSector[yn][xn].RemoveFlyingCreature( pOldCreature );

			#ifdef OUTPUT_DEBUG
				if (pOldCreature!=NULL)
					DEBUG_ADD_FORMAT("[Flying]Remove Creature by New Creature! ID=%d (%d,%d)", pOldCreature->GetID(), xn,yn);
			#endif

			// add								
			m_ppSector[yn][xn].AddFlyingCreature( pCreature );

			// Ȯ�ο�
			pCreature->SetX( xn );
			pCreature->SetY( yn );

			//------------------------------------------------
			// ���ŵ� ĳ���͸� ����ε� ��ġ�� �߰��Ѵ�.
			//------------------------------------------------
			if (pOldCreature!=NULL)
			{
				int x = pOldCreature->GetServerX();
				int y = pOldCreature->GetServerY();
				
				// ����ְ�.. ��ǥ�� �ٸ� ���..
				if (x!=xn || y!=yn)
				{
					if (pOldCreature->IsAlive())					
					{				
						pOldCreature->SetStop();
						pOldCreature->MovePosition(	x, y );
					}
					else
					{
						pOldCreature->MovePosition(	x, y );
					}
				}
			}
		}
	}		
	*/

	return true;
}

//----------------------------------------------------------------------
// Move Ungerground Creature
//----------------------------------------------------------------------
// (xo,yo)�� �ִ� UndergroundCreature�� (xn,yn)���� �ű��.
//----------------------------------------------------------------------
bool		
MZone::MoveUndergroundCreature(MCreature* pCreature, TYPE_SECTORPOSITION xo, TYPE_SECTORPOSITION yo, TYPE_SECTORPOSITION xn, TYPE_SECTORPOSITION yn)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (xn<0 || yn<0 || xn>=m_Width || yn>=m_Height) return false;

	// Player�� ��� --> �̵����� �ʴ´�.
	if (pCreature->GetClassType()==MCreature::CLASS_PLAYER
		// fake�� �̵��� �ǹ̰� ����.
		|| pCreature->IsFakeCreature())
	{
		return true;
	}

	// �̹� �̵����ִ� ���..
	//if (xo==xn && yo==yn)
	//	return false;

	
	//MCreature* pExistCreature = NULL;

	int id = pCreature->GetID();

	//------------------------------------------------
	// sector���� ���Ž�Ų��.
	//------------------------------------------------
	if (!m_ppSector[yo][xo].RemoveCreature(id))
	{
		int xs = pCreature->GetServerX();
		int ys = pCreature->GetServerY();

		if (!m_ppSector[ys][xs].RemoveCreature(id))
		{	
			DEBUG_ADD_FORMAT("Can't Remove! [Underground] ID=%d (%d,%d) --> (%d,%d)", pCreature->GetID(), xo,yo,xn,yn);			
		}
	}

	//------------------------------------------------
	// sector�� �߰���Ų��.
	//------------------------------------------------
	if (!m_ppSector[yn][xn].AddUndergroundCreature( pCreature ))
	{
		DEBUG_ADD_FORMAT("Can't Add UndergroundCreature! ID=%d (%d,%d)", pCreature->GetID(), xn,yn);			
	}

	/*
	// remove�� �� �� ���...
	if (!m_ppSector[yo][xo].RemoveUndergroundCreature(id, pExistCreature))
	{	
		if (!m_ppSector[yo][xo].RemoveGroundCreature(id, pExistCreature))
		{
			if (!m_ppSector[yo][xo].RemoveFlyingCreature(id, pExistCreature))
			{
				int xs = pCreature->GetServerX();
				int ys = pCreature->GetServerY();

				if (!m_ppSector[ys][xs].RemoveFlyingCreature(id, pExistCreature))
				{	
					if (!m_ppSector[ys][xs].RemoveGroundCreature(id, pExistCreature))
					{
						if (!m_ppSector[ys][xs].RemoveUndergroundCreature(id, pExistCreature))
						{

							int id = 0;
							if (pExistCreature!=NULL)
								id = pExistCreature->GetID();

							// remove ����
							//MessageBox(g_hWnd, "Already Removed![Flying]", NULL, MB_OK);
							DEBUG_ADD_FORMAT("Already Removed! [Under] ID=%d (%d,%d) --> (%d,%d) ExistID = %d", pCreature->GetID(), xo,yo,xn,yn, id);
						}
					}
				}
			}
		}
	}

	// test
	MCreature* pOldCreature = m_ppSector[yn][xn].GetUndergroundCreature();

	// �ƹ��� ���� ���
	if (pOldCreature == NULL)
	{
		// add								
		m_ppSector[yn][xn].AddUndergroundCreature( pCreature );
	}
	// ���ο� �ڸ��� �̹� �ٸ� ĳ���Ͱ� �ִ� ���...
	else
	{
		DEBUG_ADD_FORMAT("Already ExistCreature! [Underground] ID=%d (%d,%d) --> (%d,%d) ExistID = %d", pCreature->GetID(), xo,yo,xn,yn, pOldCreature->GetID());				

		// ���� �ִ� ĳ���Ͱ� Player�� ���
		//if (pOldCreature->GetID()==g_pPlayer->GetID())
		//{
			// �߰� ���� �ʴ´�.
			// �̵��ϱ� ������ �ǵ�����.
			// add								
		//	m_ppSector[xo][yo].AddUndergroundCreature( pCreature );

		//	return false;
		//}
		//else
		{
			//------------------------------------------------
			// ���� �ִ� ĳ���� ����..
			//------------------------------------------------						
			m_ppSector[yn][xn].RemoveUndergroundCreature( pOldCreature );

			#ifdef OUTPUT_DEBUG
				if (pOldCreature!=NULL)
					DEBUG_ADD_FORMAT("Remove Creature by New Creature! ID=%d (%d,%d)", pOldCreature->GetID(), xn,yn);
			#endif

			//------------------------------------------------
			// add								
			//------------------------------------------------
			m_ppSector[yn][xn].AddUndergroundCreature( pCreature );
			
			//------------------------------------------------
			// ���ŵ� ĳ���͸� ����ε� ��ġ�� �߰��Ѵ�.
			//------------------------------------------------
			if (pOldCreature!=NULL)
			{
				int x = pOldCreature->GetServerX();
				int y = pOldCreature->GetServerY();
				
				// ����ְ�.. ��ǥ�� �ٸ� ���..
				if (x!=xn || y!=yn)
				{
					if (pOldCreature->IsAlive())					
					{				
						pOldCreature->SetStop();
						pOldCreature->MovePosition(	x, y );
					}
					else
					{
						pOldCreature->MovePosition(	x, y );
					}
				}
			}
		}
	}	
	*/

	return true;
}

//----------------------------------------------------------------------
// Get CreatureID
//----------------------------------------------------------------------
// �̸����� ID�� ã�´�.
// flag : 1 npc����
//----------------------------------------------------------------------
TYPE_OBJECTID		
MZone::GetCreatureID(const char* pName, int flag) const
{
	CREATURE_MAP::const_iterator	iCreature = m_mapCreature.begin();

	MCreature* pCreature;

	//------------------------------------------------------
	// ��� Creature�� ���ؼ� Action
	//------------------------------------------------------
	while (iCreature != m_mapCreature.end())
	{
		pCreature = iCreature->second;

		// player�� �ƴ� ��쿡..
		if (pCreature->GetID()!=g_pPlayer->GetID()
			&& strcmp(pCreature->GetName(), pName)==0
			&& (flag == 0 || flag == 1 && !pCreature->IsNPC() )
			)
		{
			return pCreature->GetID();
		}

		iCreature++;
	}	

	return OBJECTID_NULL;
}

//----------------------------------------------------------------------
// Sector (x,y)�� �ִ� Creature�� ID�� �Ѱ��ش�.
//----------------------------------------------------------------------

TYPE_OBJECTID
MZone::GetCreatureID(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (x<0 || y<0 
		|| x>=m_Width || y>=m_Height) return OBJECTID_NULL;

	// ���� �ִ� sector�̸�	
	MCreature*	pCreature = m_ppSector[y][x].GetGroundCreature();

	// Creature�� ������
	if (pCreature != NULL)
	{			
		return pCreature->GetID();
	}

	return OBJECTID_NULL;

}

//----------------------------------------------------------------------
// Sector (x,y)�� �ִ� Creature�� �Ѱ��ش�.
//----------------------------------------------------------------------
MCreature*
MZone::GetCreatureBySector(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (x<0 || y<0 
		|| x>=m_Width || y>=m_Height) return NULL;

	// ���� �ִ� sector�̸�	
	MCreature*	pCreature = m_ppSector[y][x].GetGroundCreature();

	// Creature�� ������
	if (pCreature != NULL)
	{			
		return pCreature;
	}

	return NULL;

}
//----------------------------------------------------------------------
// Sector (x,y)�� �ִ� FlyingCreature�� ID�� �Ѱ��ش�.
//----------------------------------------------------------------------
TYPE_OBJECTID
MZone::GetFlyingCreatureID(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (x<0 || y<0 
		|| x>=m_Width || y>=m_Height) return OBJECTID_NULL;

	// ���� �ִ� sector�̸�	
	MCreature*	pCreature = (MCreature*)m_ppSector[y][x].GetFlyingCreature();

	// Creature�� ������
	if (pCreature != NULL)
	{			
		return pCreature->GetID();
	}

	return OBJECTID_NULL;

}


//----------------------------------------------------------------------
// Sector (x,y)�� �ִ� UndergroundCreature�� ID�� �Ѱ��ش�.
//----------------------------------------------------------------------
TYPE_OBJECTID
MZone::GetUndergroundCreatureID(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (x<0 || y<0 
		|| x>=m_Width || y>=m_Height) return OBJECTID_NULL;

	// ���� �ִ� sector�̸�	
	MCreature*	pCreature = (MCreature*)m_ppSector[y][x].GetUndergroundCreature();

	// Creature�� ������
	if (pCreature != NULL)
	{			
		return pCreature->GetID();
	}

	return OBJECTID_NULL;

}


//----------------------------------------------------------------------
// Update
//----------------------------------------------------------------------
void
MZone::Update()
{
	//------------------------------------------
	// �ٸ� Creature�� �����δ�.
	//------------------------------------------
	UpdateAllCreature();

	//------------------------------------------
	// fake Creature�� �����δ�.
	//------------------------------------------
	UpdateFakeCreature();

	//------------------------------------------
	// UpdateItem
	//------------------------------------------
	UpdateItem();

	#ifdef OUTPUT_DEBUG_UPDATE_ZONE
		DEBUG_ADD("MZone::before Effect");
	#endif

	//------------------------------------------
	// Effect�� �����δ�.
	//------------------------------------------
	UpdateWaitEffects();
	UpdateEffects();
	UpdateGroundEffects();
	//------------------------------------------
	// Helicopter
	//------------------------------------------
	m_HelicopterManager.Update();

	//------------------------------------------
	// Sound ó��
	//------------------------------------------
	UpdateSound();

	#ifdef OUTPUT_DEBUG_UPDATE_ZONE
		DEBUG_ADD("MZone::Update OK");
	#endif
}

//----------------------------------------------------------------------
// Move All Creature
//----------------------------------------------------------------------
// Zone�� ���� ��� Creature�� �ൿ�ϰ� �Ѵ�.
// Player�� ����
// Player�� �þ߸� ����� ���� ���Ѿ� �Ѵ�.
//----------------------------------------------------------------------
void
MZone::UpdateAllCreature()
{
	CREATURE_MAP::iterator	iCreature = m_mapCreature.begin();

	MCreature* pCreature;

	//	DEBUG_ADD_FORMAT("[UpdateAllCreature] %d Creature(s)", m_mapCreature.size());

	static DWORD nextResetTime = g_CurrentTime + g_pUserOption->persnalShopupdatetime;
	bool bResetPersonalShopValue = false;
	
	if( g_CurrentTime > nextResetTime )
	{
		nextResetTime = g_CurrentTime+ g_pUserOption->persnalShopupdatetime;
		bResetPersonalShopValue = true;
	}	
	
	//------------------------------------------------------
	// ��� Creature�� ���ؼ� Action
	//------------------------------------------------------
	while (iCreature != m_mapCreature.end())
	{
		pCreature = ((*iCreature).second);

		#ifdef OUTPUT_DEBUG_UPDATE_ZONE
				DEBUG_ADD_FORMAT("[UpdateCreature] id=%d, cType=%d, x=%d, y=%d, action=%d", 
									(int)pCreature->GetID(), (int)pCreature->GetCreatureType(),
									(int)pCreature->GetX(), (int)pCreature->GetY(), (int)pCreature->GetAction());
		#endif

		if( pCreature->CurPernalShop() == 1 && bResetPersonalShopValue )
			pCreature->SetPersonalShopMsgTime( rand() & 0x7F );
		
		//------------------------------------------------------
		//
		// Player�� ���
		//
		//------------------------------------------------------
		if (pCreature->GetClassType()==MCreature::CLASS_PLAYER)
		{
#ifdef __METROTECH_TEST__
//			for(int i = 0; i < abs(g_iSpeed)-1; i++)
//			{
//				g_pPlayer->Action();
////				g_pPlayer->UpdateAttachEffect();
//			}
#endif
			if(g_pPlayer->HasEffectStatus( EFFECTSTATUS_GHOST ) ||
				g_pPlayer->HasEffectStatus( EFFECTSTATUS_DRAGON_EYES )
#if __CONTENTS(__HERO_SYSTEM)
//				|| g_pPlayer->HasEffectStatus(EFFECTSTATUS_SLAYER_HERO) ||
//				g_pPlayer->HasEffectStatus(EFFECTSTATUS_VAMPIRE_HERO) ||
//				g_pPlayer->HasEffectStatus(EFFECTSTATUS_OUSTERS_HERO)
#endif //__HERO_SYSTEM
				)
				g_pPlayer->Action();
			
			g_pPlayer->Action();

			#ifdef OUTPUT_DEBUG_UPDATE_ZONE
				DEBUG_ADD_FORMAT("PlAok");
			#endif

			g_pPlayer->UpdateAttachEffect();

			#ifdef OUTPUT_DEBUG_UPDATE_ZONE
				DEBUG_ADD_FORMAT("PlEok");
			#endif			
		}
		//------------------------------------------------------
		//
		// �ٸ� Creature�� ���
		//
		//------------------------------------------------------
		else
		{
			int invisibleCount = pCreature->GetInvisibleCount();
			if(pCreature->HasEffectStatus( EFFECTSTATUS_DRAGON_EYES )
#if __CONTENTS(__HERO_SYSTEM)
				|| pCreature->HasEffectStatus( EFFECTSTATUS_SLAYER_HERO )
				|| pCreature->HasEffectStatus( EFFECTSTATUS_VAMPIRE_HERO )
				|| pCreature->HasEffectStatus( EFFECTSTATUS_OUSTERS_HERO )
				|| pCreature->HasEffectStatus( EFFECTSTATUS_LEVELWAR_SLAYER_HERO )
				|| pCreature->HasEffectStatus( EFFECTSTATUS_LEVELWAR_VAMPIRE_HERO )
				|| pCreature->HasEffectStatus( EFFECTSTATUS_LEVELWAR_OUSTERS_HERO )
#endif //__HERO_SYSTEM
				)
				pCreature->Action();
			pCreature->Action();
			pCreature->UpdateAttachEffect();

			//------------------------------------------------------
			// Creature�� ���� ���
			//------------------------------------------------------
			// Zone���� �����ϰ� MCorpse�� �ٲ۴�.
			//------------------------------------------------------
			/*
			if (pCreature->IsDead())
			{
				#ifdef CONNECT_SERVER
					// �״� ������ ���� ���
					// ��, AttachEffect�� ���� ���..
					if (pCreature->GetActionCount()==pCreature->GetActionCountMax()
						&& !pCreature->IsExistAttachEffect())
					{	
						// creature�� ��ü�� �ٲ۴�.
						AddCorpseFromCreature( pCreature );

						// map���� creature�� �����Ѵ�.
						CREATURE_MAP::iterator	iCreatureTemp = iCreature;
						iCreature++;

						m_mapCreature.erase( iCreatureTemp );

						continue;
					}
				#endif
			}
			*/

			//------------------------------------------------------
			// invisibleCount�� 64�� �� ĳ���ʹ� Zone���� �������� �Ѵ�.
			//------------------------------------------------------
			if (pCreature->GetInvisibleCount()==64
				&& invisibleCount!=64)
			{
				//-------------------------------------------------------
				// snipping mode�̸� ������ ��������(�ƹ��� �� �� �����Ƿ�)
				//
				// invisibility�̸� player�� slayer�̰�
				//					detect_hidden�� ���ٸ� ��������.
				//-------------------------------------------------------
				if (//pCreature->HasEffectStatus(EFFECTSTATUS_SNIPPING_MODE)|| 
					pCreature->HasEffectStatus(EFFECTSTATUS_INVISIBILITY)
						&& !g_pPlayer->IsVampire()
						&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_DETECT_INVISIBILITY)
						&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_OBSERVING_EYE)
 						&& !g_pPlayer->HasEffectStatus(EFFECTSTATUS_GNOME_WHISPER)
//						&& !g_pPlayer->HasEffectStatus( EFFECTSTATUS_GHOST )
						)
				{
					//-------------------------------------------------------
					// player�� �ƴ� ���
					//-------------------------------------------------------
					int id			= pCreature->GetID();
					int x			= pCreature->GetX();
					int y			= pCreature->GetY();
					int serverX		= pCreature->GetServerX();
					int serverY		= pCreature->GetServerY();

					//----------------------------------------
					// ��� ����
					//----------------------------------------
					if (pCreature->IsSlayer())
					{
						m_HelicopterManager.RemoveHelicopter( pCreature->GetID() );
					}				
					
					// ã�� ��� --> ����		
					bool removed = true;
					
					//------------------------------------------------
					// sector���� ���Ž�Ų��.
					//------------------------------------------------
					if (!m_ppSector[y][x].RemoveCreature(id))
					{
						if (!m_ppSector[serverY][serverX].RemoveCreature(id))
						{	
							DEBUG_ADD_FORMAT("Can't RemoveCreature! ID=%d client(%d,%d), server(%d,%d)", pCreature->GetID(), x,y,serverX,serverY);

							removed = false;
						}
					}

					UnSetServerBlock( pCreature->GetMoveType(), pCreature->GetServerX(), pCreature->GetServerY() );

					// ����
					delete pCreature;	

					// map���� creature�� �����Ѵ�.
					CREATURE_MAP::iterator	iCreatureTemp = iCreature;
					iCreature++;

					m_mapCreature.erase( iCreatureTemp );			


					continue;
				}
			}

		}

		iCreature++;
	}

#ifdef __METROTECH_TEST__
	extern int	g_iSpeed;
	if(g_iSpeed < 0)
	{
		CREATURE_MAP::iterator itr = m_mapCreature.begin();
		CREATURE_MAP::iterator endItr = m_mapCreature.end();
		
		while(itr != endItr)
		{
			MCreature *pCreature = itr->second;
			if(pCreature->IsPlayer() && pCreature->GetID() != g_pPlayer->GetID() && !pCreature->IsPlayerParty())
				break;
			
			itr++;
		}
		
		if(itr == endItr)
		{
			g_iSpeed = -4;
		}
		else
		{
			g_iSpeed = -1;
		}
	}
#endif
	
	#ifdef OUTPUT_DEBUG_UPDATE_ZONE
		DEBUG_ADD("UpdateAllCreature OK");
	#endif
}

//----------------------------------------------------------------------
// Create Corpse  From Creature
//----------------------------------------------------------------------
// ��ü(MCorpse)�� �����ϴµ�.. Creature�� �����Ѵ�..
//----------------------------------------------------------------------
/*
bool
MZone::AddCorpseFromCreature(MCreature* pCreature)
{
	DEBUG_ADD_FORMAT("Add Corpse From Creature id=%d", pCreature->GetID());

	int id = pCreature->GetID();
	int sX = pCreature->GetX();
	int sY = pCreature->GetY();

	//----------------------------------------
	// ä���� �����ش�.
	//----------------------------------------
	pCreature->ClearChatString();

	//----------------------------------------
	// Sector���� Creature ����
	//----------------------------------------
	// player�� �ƴ� ���..
	//----------------------------------------
	bool removed = false;
	if (id != g_pPlayer->GetID())
	{
		int serverX = pCreature->GetServerX();
		int serverY = pCreature->GetServerY();

		removed = m_ppSector[sY][sX].RemoveGroundCreature( id );
		if (!removed)
		{
			removed = m_ppSector[serverY][serverX].RemoveGroundCreature( id );

			if (!removed)
			{
				removed = m_ppSector[sY][sX].RemoveFlyingCreature( id );

				if (!removed)
				{
					removed = m_ppSector[serverY][serverX].RemoveFlyingCreature( id );

					if (!removed)
					{
						removed = m_ppSector[sY][sX].RemoveUndergroundCreature( id );

						if (!removed)
						{
							removed = m_ppSector[serverY][serverX].RemoveUndergroundCreature( id );

							DEBUG_ADD_FORMAT("[Error] Can't Remove Creature to corpse. ID=%d. xy=(%d, %d), sxy=(%d, %d)", id, sX, sY, serverX, serverY);
						}
					}
				}
			}
		}
	}

	// Creature�� ���Ű� �ȵ�����...
	#ifdef OUTPUT_DEBUG
		if (!removed)
		{			
			DEBUG_ADD_FORMAT("[Create Corpse] Can't Remove Creature from Sector: %d, (%d,%d)", id, sX, sY);
		}
	#endif
	
	//----------------------------------------
	//
	// ��ü�� �����ϰ� Creature�� �����Ѵ�.
	//
	//----------------------------------------
	// server���� ���� ��ġ
	int sSX = pCreature->GetServerX();
	int sSY = pCreature->GetServerY();

	MCorpse* pCorpse = (MCorpse*)MItem::NewItem( ITEM_CLASS_CORPSE );
	pCorpse->SetID( id );
	pCorpse->SetItemType( 0 );
	pCorpse->SetCreature( pCreature );
	pCorpse->SetPosition( sSX, sSY );

	// item ���� ����
	pCorpse->SetNumber( pCreature->GetItemCount() );


	//----------------------------------------	
	// �̹� �ٸ� Item�� �ִٸ�?? �����Ѵ�.
	//----------------------------------------
	if (m_ppSector[sSY][sSX].IsExistItem())
	{
		MItem* pOldItem = m_ppSector[sSY][sSX].GetItem();
		
		#ifdef OUTPUT_DEBUG
			if (pOldItem!=NULL)
			{
				DEBUG_ADD_FORMAT("Already Exist Item. Remove Old Item: %d, (%d,%d)", pOldItem->GetID(), sSX, sSY);
			}
		#endif

		bool bRemove = RemoveItem( pOldItem->GetID() );

		#ifdef OUTPUT_DEBUG
			if (!bRemove)
			{
					DEBUG_ADD_FORMAT("Can't Remove Old Item");
			}
		#endif
	}

	//----------------------------------------
	// Zone�� Item�߰�
	//----------------------------------------
	#ifdef OUTPUT_DEBUG
		if (!AddItem( pCorpse ))
		{
			// ��.. �Ұ����� ����� �� �� �ִ�. - -;
			DEBUG_ADD_FORMAT("[Error] Can't Add Corpse to Sector id=%d (%d,%d)", pCorpse->GetID(), sSX, sSY);
		
			delete pCorpse;

			return false;
		}

		return true;
	#else
		return AddItem( pCorpse );
	#endif
}
*/

//----------------------------------------------------------------------
// Create Corpse  From Creature
//----------------------------------------------------------------------
// ��ü(MCorpse)�� �����ϴµ�.. Creature�� �����Ѵ�..
//----------------------------------------------------------------------
bool
MZone::AddCorpseFromCreature(TYPE_OBJECTID id)
{
	DEBUG_ADD_FORMAT("Add Corpse From Creature id=%d", id);

	if (id==g_pPlayer->GetID())
	{
		DEBUG_ADD("No Player Corpse");
		return false;
	}

	CREATURE_MAP::iterator iCreature = m_mapCreature.find( id );

	if (iCreature==m_mapCreature.end())
	{
		DEBUG_ADD("No such creature");
		return false;
	}

	MCreature* pCreature = iCreature->second;

	UnSetServerBlock( pCreature->GetMoveType(), pCreature->GetServerX(), pCreature->GetServerY() );

	// Zone���� �����Ѵ�.
	m_mapCreature.erase( iCreature );	
	
	int sX = pCreature->GetX();
	int sY = pCreature->GetY();

	//----------------------------------------
	// ä���� �����ش�.
	//----------------------------------------
#if __CONTENTS(__TIPOJYU_CASTLE)			//ä�� ����
	switch(pCreature->GetCreatureType())
	{
	case 1045:
		break;

	case 723:
		break;
	}
#else
	if(pCreature->GetCreatureType() != 723) // ���� ���巹..^^: ���⼭ ����� �����鼭 �ϴ� ��簡 �ȳ���..
		pCreature->ClearChatString();
#endif //__TIPOJYU_CASTLE
	//----------------------------------------
	// Sector���� Creature ����
	//----------------------------------------
	// player�� �ƴ� ���..
	//----------------------------------------
	bool removed = false;
	if (id != g_pPlayer->GetID()
		&& !pCreature->IsFakeCreature())
	{
		//------------------------------------------------
		// sector���� ���Ž�Ų��.
		//------------------------------------------------
		removed = true;
		if (!m_ppSector[sY][sX].RemoveCreature(id))
		{
			int serverX = pCreature->GetServerX();
			int serverY = pCreature->GetServerY();

			if (!m_ppSector[serverY][serverX].RemoveCreature(id))
			{	
				removed = false;
				DEBUG_ADD_FORMAT("Can't RemoveCreatureToCorpse! ID=%d client(%d,%d), server(%d,%d)", pCreature->GetID(), sX,sY,serverX,serverY);
			}			
		}
		

		/*
		removed = m_ppSector[sY][sX].RemoveGroundCreature( id );
		if (!removed)
		{
			removed = m_ppSector[serverY][serverX].RemoveGroundCreature( id );

			if (!removed)
			{
				removed = m_ppSector[sY][sX].RemoveFlyingCreature( id );

				if (!removed)
				{
					removed = m_ppSector[serverY][serverX].RemoveFlyingCreature( id );

					if (!removed)
					{
						removed = m_ppSector[sY][sX].RemoveUndergroundCreature( id );

						if (!removed)
						{
							removed = m_ppSector[serverY][serverX].RemoveUndergroundCreature( id );

							DEBUG_ADD_FORMAT("[Error] Can't Remove Creature to corpse. ID=%d. xy=(%d, %d), sxy=(%d, %d)", id, sX, sY, serverX, serverY);
						}
					}
				}
			}
		}
		*/
	}

	// Creature�� ���Ű� �ȵ�����...
	#ifdef OUTPUT_DEBUG
		if (!removed)
		{			
			DEBUG_ADD_FORMAT("[Create Corpse] Can't Remove Creature from Sector: %d, (%d,%d)", id, sX, sY);
		}
	#endif
	
	//----------------------------------------
	//
	// ��ü�� �����ϰ� Creature�� �����Ѵ�.
	//
	//----------------------------------------
	// server���� ���� ��ġ
	int sSX = pCreature->GetServerX();
	int sSY = pCreature->GetServerY();

	MCorpse* pCorpse = (MCorpse*)MItem::NewItem( ITEM_CLASS_CORPSE );
	pCorpse->SetID( id );
	pCorpse->SetItemType( 0 );
	pCorpse->SetCreature( pCreature );
	pCorpse->SetPosition( sSX, sSY );

	// item ���� ����
	pCorpse->SetNumber( pCreature->GetItemCount() );


	//----------------------------------------	
	// �̹� �ٸ� Item�� �ִٸ�?? �����Ѵ�.
	//----------------------------------------
	if (m_ppSector[sSY][sSX].IsExistItem())
	{
		MItem* pOldItem = m_ppSector[sSY][sSX].GetItem();
		
		#ifdef OUTPUT_DEBUG
			if (pOldItem!=NULL)
			{
				DEBUG_ADD_FORMAT("Already Exist Item. Remove Old Item: %d, (%d,%d)", pOldItem->GetID(), sSX, sSY);
			}
		#endif

		bool bRemove = RemoveItem( pOldItem->GetID() );

		#ifdef OUTPUT_DEBUG
			if (!bRemove)
			{
					DEBUG_ADD_FORMAT("Can't Remove Old Item");
			}
		#endif
	}

	//----------------------------------------
	// Zone�� Item�߰�
	//----------------------------------------
	#ifdef OUTPUT_DEBUG
		if (!AddItem( pCorpse ))
		{
			// ��.. �Ұ����� ����� �� �� �ִ�. - -;
			DEBUG_ADD_FORMAT("[Error] Can't Add Corpse to Sector id=%d (%d,%d)", pCorpse->GetID(), sSX, sSY);
		
			delete pCorpse;

			return false;
		}

		return true;
	#else
		return AddItem( pCorpse );
	#endif

}


//----------------------------------------------------------------------
// Add Portal
//----------------------------------------------------------------------
// rect������ zoneID�� �̵��ϴ� portal�̴�.
//----------------------------------------------------------------------
void						
MZone::AddPortal(int type, int zoneID, const RECT& rect)
{
	int top = rect.top;
	int left = rect.left;
	int bottom = rect.bottom;
	int right = rect.right;

	if (top < 0) top = 0;
	if (left < 0) left = 0;
	if (bottom >= m_Height) bottom = m_Height-1;
	if (right >= m_Width) right = m_Width-1;

	if (top > bottom) { int temp=top; top=bottom; bottom=temp; }
	if (left > right) { int temp=left; left=right; right=left; }

	for (int i=top; i<=bottom; i++)
	{
		for (int j=left; j<=right; j++)
		{
			m_ppSector[i][j].AddPortal( type, zoneID );
		}
	}
}

//----------------------------------------------------------------------
// Keep Object In PlayerSight
//----------------------------------------------------------------------
// Zone�� ���� ��� Creature/Item�߿���
// Player�� �þ� �ȿ� �ִ� �͸� ����� 
// �������� ���Ž�Ų��.
// Server�� �þ� ������ ���߾�� �Ѵ�.
//----------------------------------------------------------------------
void
MZone::KeepObjectInSight(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y, BYTE sight)
{	
//	int sight15 = sight + (sight>>1);

	//------------------------------------------------------
	// Player�� �þ� ������ �����Ѵ�.
	//------------------------------------------------------
//	int minX = min( sight, VISION_SECTOR_WIDTH_HALF );
//	int minY = min( sight, VISION_SECTOR_HEIGHT_HALF );
//	int minY2 = min( sight, VISION_SECTOR_HEIGHT_HALF );//-2 );

	int minX = NEW_VISION_SECTOR_WIDTH_HALF;
	int minY = NEW_VISION_SECTOR_HEIGHT_HALF_UP;
	int minY2 = NEW_VISION_SECTOR_HEIGHT_HALF_DOWN;

	int sX1 = x-minX;
	int sY1 = y-minY;
	int sX2 = x+minX;
	int sY2 = y+minY2;

	//------------------------------------------------------
	// Zone�� ������ �ƴ� ��쿡 Skip...
	//------------------------------------------------------
	if (sX1 < 0) 
	{					
		sX1 = 0;	
	}

	if (sX2 >= m_Width)
	{
		sX2 = m_Width-1;
	}

	if (sY1 < 0)
	{
		sY1 = 0;	
	}

	if (sY2 >= m_Height)
	{
		sY2 = m_Height-1;
	}
	
	//------------------------------------------------------
	//
	//				��� Creature�� ���ؼ�....
	//
	//------------------------------------------------------
	CREATURE_MAP::iterator	iCreature = m_mapCreature.begin();

	MCreature* pCreature;

	while (iCreature != m_mapCreature.end())
	{
		pCreature = iCreature->second;

		//-----------------------------------------------------
		// Player�� �ƴ� ��쿡�� üũ�ϰ� �����Ѵ�.
		//-----------------------------------------------------
		if (pCreature->GetClassType()==MCreature::CLASS_PLAYER || pCreature->GetID() < 10000)	// Ŭ���̾�Ʈ ũ���Ĵ� ���� ���Ѵ�
		{	
			iCreature++;
		}		
		else
		{
			//-----------------------------------------------------
			// Creature�� Server��ǥ�� Player�� �þ߿� 
			// ���Ե��� ���� ��츸 ���Ž�Ų��.
			//-----------------------------------------------------			
			int cX = pCreature->GetServerX();
			int cY = pCreature->GetServerY();
//			int dist = abs(x - cX) + abs(y - cY);

			DEBUG_ADD_FORMAT("[KeepObjectInSight] %d : %d : %d , %d : %d : %d",
				sX1, cX, sX2, sY1, cY, sY2 );
			//-----------------------------------------------------			
			// �þ��� ���� �ȿ� ���ϴ� ���� �Ѿ��..
			//-----------------------------------------------------			
			if (cX>=sX1 && cX<=sX2 &&
				cY>=sY1 && cY<=(sY2 + pCreature->GetHeight() / TILE_Y)
//				&& dist <= sight15
				)
			{
				iCreature++;			
			}
			//-----------------------------------------------------			
			// �þ��� ���� �ȿ� ������ ������... ����~~
			//-----------------------------------------------------			
			else
			{
				// iTemp�� ����� �ȴ�.
				CREATURE_MAP::iterator iTemp = iCreature;

				// ���� ��..
				iCreature++;

				DEBUG_ADD_FORMAT("[Remove Creature by Sight] id=%d xy=(%d, %d)", pCreature->GetID(), cX, cY);						
				
				MCreature* pCreature = (*iTemp).second;
				
				//----------------------------------------
				// Sector���� ����
				//----------------------------------------
				// player�� �ƴ� ���..
				//----------------------------------------
				int id = pCreature->GetID();
				int x = pCreature->GetX();
				int y = pCreature->GetY();
				
				//------------------------------------------------
				// sector���� ���Ž�Ų��.
				//------------------------------------------------
				if (!m_ppSector[y][x].RemoveCreature(id))
				{
					if (!m_ppSector[cY][cX].RemoveCreature(id))
					{	
						DEBUG_ADD_FORMAT("Can't RemoveCreature! ID=%d client(%d,%d), server(%d,%d)", pCreature->GetID(), x,y,cX,cY);
					}
				}

				/*
				bool removed = m_ppSector[y][x].RemoveGroundCreature( id );
				if (!removed)
				{
					removed = m_ppSector[serverY][serverX].RemoveGroundCreature( id );

					if (!removed)
					{
						removed = m_ppSector[y][x].RemoveFlyingCreature( id );

						if (!removed)
						{
							removed = m_ppSector[serverY][serverX].RemoveFlyingCreature( id );

							if (!removed)
							{
								removed = m_ppSector[y][x].RemoveUndergroundCreature( id );

								if (!removed)
								{
									removed = m_ppSector[serverY][serverX].RemoveUndergroundCreature( id );

									DEBUG_ADD_FORMAT("[Error] Can't Remove Creature by sight. ID=%d. xy=(%d, %d), sxy=(%d, %d)", id, x, y, serverX, serverY);
								}
							}
						}
					}
				}
				*/

				//-------------------------------------------------------
				// ���� ���� �ɷ��� �ְ� player�� ��Ƽ�� ���
				//-------------------------------------------------------
				/*
				if (pCreature->IsPlayerParty()
					&& g_pParty!=NULL)
				{
					PARTY_INFO* pInfo = g_pParty->GetMemberInfo( pCreature->GetName() );
						
					if (pInfo!=NULL)
					{
						pInfo->bInSight = false;
						pInfo->zoneID = 60002;
					}
				}
				*/
				
				// memory���� ����
				UnSetServerBlock( pCreature->GetMoveType(), pCreature->GetServerX(), pCreature->GetServerY() );

				delete pCreature;				
				
				m_mapCreature.erase(iTemp);			
			}			
		}		
	}

	//------------------------------------------------------
	//
	//				��� Item�� ���ؼ�....
	//
	//------------------------------------------------------
	ITEM_MAP::iterator	iItem = m_mapItem.begin();

	MItem* pItem;

	while (iItem != m_mapItem.end())
	{
		pItem = iItem->second;
	
		//-----------------------------------------------------
		// Item�� ��ǥ�� Player�� �þ߿� 
		// ���Ե��� ���� ��츸 ���Ž�Ų��.
		//-----------------------------------------------------			
		int iX = pItem->GetX();
		int iY = pItem->GetY();
//		int dist = abs(x - iX) + abs(y - iY);

		//-----------------------------------------------------			
		// �þ��� ���� �ȿ� ���ϴ� ���� �Ѿ��..
		//-----------------------------------------------------			
		if (iX>=sX1 && iX<=sX2 &&
			iY>=sY1 && iY<=sY2 
//			&& dist <= sight15
			)
		{
			iItem++;			
		}
		//-----------------------------------------------------			
		// �þ��� ���� �ȿ� ������ ������... ����~~
		//-----------------------------------------------------			
		else
		{
			// iTemp�� ����� �ȴ�.
			ITEM_MAP::iterator iTemp = iItem;

			// ���� ��..
			iItem++;

			DEBUG_ADD_FORMAT("[Remove Item by Sight] id=%d xy=(%d, %d)", pItem->GetID(), iX, iY);					
				
			MItem* pItem = (*iTemp).second;

			// [��������]
			MSector& sector = m_ppSector[iY][iX];				
			sector.RemoveItem( pItem->GetID() );

			// [��������]
			if (IsRelicTable(pItem))
			{
				sector.UnSetBlockGround();
				sector.UnSetBlockFlying();
			}
				
			// memory���� ����
			//delete pItem;				
			SAFE_DELETE ( pItem );
				
			m_mapItem.erase(iTemp);	
		}					
	}
	
	//------------------------------------------------------
	// Effect �����
	//------------------------------------------------------
	EFFECT_MAP::iterator	iEffect = m_mapEffect.begin();

	MEffect* pEffect;

	while (iEffect != m_mapEffect.end())
	{
		pEffect = iEffect->second;
	
		//-----------------------------------------------------
		// Item�� ��ǥ�� Player�� �þ߿� 
		// ���Ե��� ���� ��츸 ���Ž�Ų��.
		//-----------------------------------------------------			
		int iX = pEffect->GetX();
		int iY = pEffect->GetY();
//		int dist = abs(x - iX) + abs(y - iY);

		//-----------------------------------------------------			
		// �þ��� ���� �ȿ� ���ϴ� ���� �Ѿ��..
		//-----------------------------------------------------			
		if (iX>=sX1 && iX<=sX2 &&
			iY>=sY1 && iY<=sY2 
//			&& dist <= sight15
			)
		{
			iEffect++;			
		}
		//-----------------------------------------------------			
		// �þ��� ���� �ȿ� ������ ������... ����~~
		//-----------------------------------------------------			
		else if (pEffect->GetEffectType()!=MEffect::EFFECT_CHASE &&
				(pEffect->GetFrameID() < EFFECTSPRITETYPE_MAP_BLACK_LARGE_SMOKE ||
				pEffect->GetFrameID() > EFFECTSPRITETYPE_MAP_BLACK_SMALL_SMOKE_3)&&
				pEffect->GetFrameID() != EFFECTSPRITETYPE_GDR_LAIR_POTAL)
		{
			// iTemp�� ����� �ȴ�.
			EFFECT_MAP::iterator iTemp = iEffect;

			// ���� ��..
			iEffect++;

			DEBUG_ADD_FORMAT("[Remove Effect by Sight] id=%d xy=(%d, %d)", pEffect->GetID(), iX, iY);					
				
			MEffect* pEffect = iTemp->second;

			// [�����9]
			RemoveSectorEffect( iX, iY, pEffect->GetID() );
				
			// memory���� ����
			delete pEffect;				
				
			m_mapEffect.erase(iTemp);	
		}	
		else
		{
			iEffect++;
		}
	}
}

//----------------------------------------------------------------------
// Zone�� Creature �߰� 
//----------------------------------------------------------------------
// �ܺο��� new����� �Ѵ�.
// �̹� �ִ��� Ȯ���� �ϰ� ������ �߰��ؾ� �Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::AddCreature(MCreature* pCreature)
{
	DEBUG_ADD_FORMAT("MZone::AddCreature id = %d", pCreature->GetID());

	if (pCreature->GetX()==SECTORPOSITION_NULL
		|| pCreature->GetY()==SECTORPOSITION_NULL)
	{
		DEBUG_ADD_FORMAT("[Error] Position Not Set");

		return false;
	}

	CREATURE_MAP::iterator	theIterator;

	theIterator = m_mapCreature.find(pCreature->GetID());
	
	// ���� ���� Creature�̸� �߰�	
	if (theIterator == m_mapCreature.end())
	{		
		// ������
		if (g_pParty!=NULL 
			&& pCreature->GetName()!=NULL
			&& g_pParty->HasMember( pCreature->GetName() )
			&& !pCreature->IsNPC() )
		{
			pCreature->SetPlayerParty();
				
			//-------------------------------------------------------
			// ���� ���� �ְ� player�� ��Ƽ�� ���
			//-------------------------------------------------------
			PARTY_INFO* pInfo = g_pParty->GetMemberInfo( pCreature->GetName() );
				
			if (pInfo!=NULL)
			{
				pInfo->bInSight = true;
				pInfo->HP = pCreature->GetHP();
				pInfo->MaxHP = pCreature->GetMAX_HP();
				pInfo->guildID = pCreature->GetGuildNumber();
			}			
		}

		//----------------------------------------
		// player�� Hallu�� ���
		//----------------------------------------
		if (g_pPlayer->HasEffectStatus(EFFECTSTATUS_HALLUCINATION))
		{
			pCreature->SetHalluCreature( g_pTopView->GetRandomMonsterTypeInZone() );
		}

		//----------------------------------------
		// �̵������ ����.. �ٽ� ����
		//----------------------------------------
		if (pCreature->IsUndergroundCreature())
		{
			// �׳� �����.. - -;
		}
		else
		{
			if ((*g_pCreatureTable)[pCreature->GetCreatureType()].IsFlyingCreature())
			{
				pCreature->SetFlyingCreature();
			}			
			else
			{
				pCreature->SetGroundCreature();
			}
		}

		bool bAdd;
		//----------------------------------------
		// Sector Setting
		//----------------------------------------
		// Player�� ���
		//----------------------------------------
		if (pCreature->GetClassType()==MCreature::CLASS_PLAYER)
		{
			//m_mapCreature.insert(CREATURE_MAP::value_type(pCreature->GetID(), pCreature));
			bAdd = true;
		}
		//----------------------------------------
		// Player�� �ƴ� ���
		//----------------------------------------
		else
		{			
			int x = pCreature->GetX();
			int y = pCreature->GetY();

			switch (pCreature->GetMoveType())
			{
				case MCreature::CREATURE_UNDERGROUND :
					bAdd = m_ppSector[y][x].AddUndergroundCreature(pCreature);

					//if (!bAdd)
					{
					//	m_ppSector[pCreature->GetY()][pCreature->GetX()].RemoveUndergroundCreature();
					//	bAdd = m_ppSector[pCreature->GetY()][pCreature->GetX()].AddUndergroundCreature(pCreature);
					}
				break;

				case MCreature::CREATURE_GROUND :
					bAdd = m_ppSector[y][x].AddGroundCreature(pCreature);		

					//if (!bAdd)
					{
					//	m_ppSector[pCreature->GetY()][pCreature->GetX()].RemoveGroundCreature();
					//	bAdd = m_ppSector[pCreature->GetY()][pCreature->GetX()].AddGroundCreature(pCreature);		
					}
				break;

				case MCreature::CREATURE_FLYING :
					bAdd = m_ppSector[y][x].AddFlyingCreature(pCreature);		

					//if (!bAdd)
					{
					//	m_ppSector[pCreature->GetY()][pCreature->GetX()].RemoveFlyingCreature();
					//	bAdd = m_ppSector[pCreature->GetY()][pCreature->GetX()].AddFlyingCreature(pCreature);		
					}
				break;
			}
		}

		if (bAdd)
		{
			m_mapCreature.insert(CREATURE_MAP::value_type(pCreature->GetID(), pCreature));

			pCreature->SetZone( this );
			
			return true;
		}		

		DEBUG_ADD_FORMAT("[Error] AddCreature Failed!");

		return false;
	}

	// �̹� �ִ� Creature�̸�,
	DEBUG_ADD_FORMAT("Add Failed - Already Exist in Zone");

	return false;
}

	

//----------------------------------------------------------------------
// Zone���� Creature ����
//----------------------------------------------------------------------
// ���ο��� delete���ش�.
//----------------------------------------------------------------------
bool
MZone::RemoveCreature(TYPE_OBJECTID id)
{
	DEBUG_ADD_FORMAT("Remove Creature. ID=%d", id);

	CREATURE_MAP::iterator	theIterator;

	// ID�� id�� Creature�� ã�´�.
	theIterator = m_mapCreature.find(id);
    
	// �׷� id�� ���� Creature�� ����.	
	if (theIterator == m_mapCreature.end())
		return false;


	MCreature* pCreature = (*theIterator).second;

	
	//-------------------------------------------------------
	// ���� ���� �ɷ��� �ְ� player�� ��Ƽ�� ���
	//-------------------------------------------------------
	/*
	if (pCreature->IsPlayerParty()
		&& g_pParty!=NULL)
	{
		PARTY_INFO* pInfo = g_pParty->GetMemberInfo( pCreature->GetName() );
			
		if (pInfo!=NULL)
		{
			pInfo->bInSight = false;
			pInfo->zoneID = 60002;
		}
	}
	*/
	
	//----------------------------------------
	// Sector���� ����
	//----------------------------------------
	// player�� ���
	//----------------------------------------
	if (pCreature==NULL || pCreature->GetClassType()==MCreature::CLASS_PLAYER)
	{
		m_mapCreature.erase(theIterator);
		
		return true;
	}
	
	//----------------------------------------
	// ��� ����
	//----------------------------------------
	if (pCreature->IsSlayer())
	{
		m_HelicopterManager.RemoveHelicopter( id );
	}

	//----------------------------------------
	// player�� �ƴ� ���
	//----------------------------------------	
	int x			= pCreature->GetX();
	int y			= pCreature->GetY();
	int serverX		= pCreature->GetServerX();
	int serverY		= pCreature->GetServerY();

	// move type�� ������� ID�� �� ã�ƺ���..
	/*
	switch (pCreature->GetMoveType())
	{
		case MCreature::CREATURE_GROUND :					
			removed = m_ppSector[y][x].RemoveGroundCreature( creatureID );
			if (!removed)
			{
				removed = m_ppSector[serverY][serverX].RemoveGroundCreature( creatureID );
			}
		break;

		case MCreature::CREATURE_FLYING :
			removed = m_ppSector[y][x].RemoveFlyingCreature( creatureID );
			if (!removed)
			{
				removed = m_ppSector[serverY][serverX].RemoveFlyingCreature( creatureID );
			}
		break;

		case MCreature::CREATURE_UNDERGROUND :
			removed = m_ppSector[y][x].RemoveUndergroundCreature( creatureID );
			if (!removed)
			{
				removed = m_ppSector[serverY][serverX].RemoveUndergroundCreature( creatureID );
			}
		break;
	}
	*/

	// ã�� ��� --> ����		
	bool removed = true;
	
	//------------------------------------------------
	// sector���� ���Ž�Ų��.
	//------------------------------------------------
	if (!m_ppSector[y][x].RemoveCreature(id))
	{
		if (!m_ppSector[serverY][serverX].RemoveCreature(id))
		{	
			DEBUG_ADD_FORMAT("Can't RemoveCreature! ID=%d client(%d,%d), server(%d,%d)", pCreature->GetID(), x,y,serverX,serverY);

			removed = false;
		}
	}

	/*
	removed = m_ppSector[y][x].RemoveGroundCreature( id );
	if (!removed)
	{
		removed = m_ppSector[serverY][serverX].RemoveGroundCreature( id );

		if (!removed)
		{
			removed = m_ppSector[y][x].RemoveFlyingCreature( id );

			if (!removed)
			{
				removed = m_ppSector[serverY][serverX].RemoveFlyingCreature( id );

				if (!removed)
				{
					removed = m_ppSector[y][x].RemoveUndergroundCreature( id );

					if (!removed)
					{
						removed = m_ppSector[serverY][serverX].RemoveUndergroundCreature( id );

						DEBUG_ADD_FORMAT("[Error] Can't Remove Creature. ID=%d. xy=(%d, %d), sxy=(%d, %d)", id, x, y, serverX, serverY);
					}
				}
			}
		}
	}
	*/

	// ������ǥ�� �׳� �����ش�. 2002.3.29
	UnSetServerBlock( pCreature->GetMoveType(), pCreature->GetServerX(), pCreature->GetServerY() );


	// memory���� ���� : Player�� �ƴ� ��쿡�� memory���� ����
	if (removed)
	{
		//UnSetServerBlock( pCreature->GetMoveType(), pCreature->GetServerX(), pCreature->GetServerY() );

		delete pCreature;

		m_mapCreature.erase(theIterator);
	}			

	return removed;
}


//----------------------------------------------------------------------
// Zone�� Creature �о����
//----------------------------------------------------------------------
// item������ ã�´�. -_-;
//----------------------------------------------------------------------
MCreature*	
MZone::GetCreature(TYPE_OBJECTID id)
{
	CREATURE_MAP::iterator	theIterator;

	// ID�� id�� Creature�� ã�´�.
	theIterator = m_mapCreature.find(id);

	// ���� ��� NULL�� return�Ѵ�.
	if (theIterator == m_mapCreature.end()) 
	{
		ITEM_MAP::iterator iItem = m_mapItem.find(id);

		if (iItem==m_mapItem.end())
		{
			return NULL;
		}

		MItem* pItem = iItem->second;

		if (pItem!=NULL
			&& pItem->GetItemClass()==ITEM_CLASS_CORPSE)
		{
			MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();

			if (pCreature!=NULL
				&& pCreature->GetActionCount() < pCreature->GetActionCountMax())
			{
				return pCreature;
			}
		}
		
		return NULL;
	}

	// ������ �� Creature�� return�Ѵ�.
	return (*theIterator).second;
}

//----------------------------------------------------------------------
// Zone�� Creature �о����
//----------------------------------------------------------------------
MCreature*	
MZone::GetCreatureOnly(TYPE_OBJECTID id)
{
	CREATURE_MAP::iterator	theIterator;

	// ID�� id�� Creature�� ã�´�.
	theIterator = m_mapCreature.find(id);

	// ���� ��� NULL�� return�Ѵ�.
	if (theIterator == m_mapCreature.end()) 
	{
		return NULL;
	}

	// ������ �� Creature�� return�Ѵ�.
	return theIterator->second;
}

//----------------------------------------------------------------------
// Zone�� ImageObject �߰� 
//----------------------------------------------------------------------
// �ܺο��� new����� �Ѵ�.
// �̹� �ִ��� Ȯ���� �ϰ� ������ �߰��ؾ� �Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::AddImageObject(MImageObject* pImageObject)
{
	if(pImageObject != NULL)
	{
		IMAGEOBJECT_MAP::iterator	theIterator;

		theIterator = m_mapImageObject.find(pImageObject->GetID());
		
		// ���� ���� ImageObject�̸� �߰�	
		if (theIterator == m_mapImageObject.end())
		{
			m_mapImageObject.insert(IMAGEOBJECT_MAP::value_type(pImageObject->GetID(), pImageObject));

			return true;
		}

		// �̹� �ִ� ImageObject�̸�,
	}
	return false;
}

	

//----------------------------------------------------------------------
// Zone���� ImageObject ����
//----------------------------------------------------------------------
// ���ο��� delete���ش�.
//----------------------------------------------------------------------
bool
MZone::RemoveImageObject(TYPE_OBJECTID	id)
{
	IMAGEOBJECT_MAP::iterator	theIterator;

	// ID�� id�� ImageObject�� ã�´�.
	theIterator = m_mapImageObject.find(id);
    
	// �׷� id�� ���� ImageObject�� ����.	
	if (theIterator == m_mapImageObject.end())
		return false;


	// ã�� ��� --> ����	
	MImageObject* pImageObject = (*theIterator).second;	


	//???????????????????????????????????????????????????????
	//
	// ImageObject�� �����ϴ� Sector�� �����ؾ����� �ʳ�??
	//
	//???????????????????????????????????????????????????????

	m_mapImageObject.erase(theIterator);

	// memory���� ����
	delete pImageObject;	

	return true;
}

//----------------------------------------------------------------------
// Zone�� ImageObject �о����
//----------------------------------------------------------------------
MImageObject*	
MZone::GetImageObject(TYPE_OBJECTID id)
{
	IMAGEOBJECT_MAP::iterator	theIterator;

	// ID�� id�� ImageObject�� ã�´�.
	theIterator = m_mapImageObject.find(id);

	// ���� ��� NULL�� return�Ѵ�.
	if (theIterator == m_mapImageObject.end()) 
		return NULL;

	// ������ �� ImageObject�� return�Ѵ�.
	return (*theIterator).second;
}

//----------------------------------------------------------------------
// SetImageObjectSector(sX,sY)
//----------------------------------------------------------------------
// Zone�� Sector�� ImageObject�� set�ϱ�
//----------------------------------------------------------------------
void		
MZone::SetImageObjectSector(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, TYPE_OBJECTID id)
{
	IMAGEOBJECT_MAP::iterator	theIterator;

	// ID�� id�� ImageObject�� ã�´�.
	theIterator = m_mapImageObject.find(id);

	// ���� ��� 
	if (theIterator == m_mapImageObject.end()) 
		return;
	
	// (sX,sY) Sector�� ImageObject�� set�Ѵ�.
	m_ppSector[sY][sX].AddImageObject((*theIterator).second);
}

//----------------------------------------------------------------------
// UnSetImageObjectSector(sX,sY)
//----------------------------------------------------------------------
// Zone�� Sector�� ImageObject�� UnSet�ϱ�
//----------------------------------------------------------------------
void		
MZone::UnSetImageObjectSector(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, TYPE_OBJECTID id)
{
	//-------------------------------------------------------
	// (sX,sY) Sector�� ImageObject�� set�Ѵ�.
	//-------------------------------------------------------
	m_ppSector[sY][sX].RemoveImageObject(id);
}

//----------------------------------------------------------------------
// Zone�� Obstacle �߰�
//----------------------------------------------------------------------
// �ܺο��� new�� ������Ѵ�.
// Sector�� �߰��� �� ������ m_listObstacle�� �߰��Ѵ�.
//----------------------------------------------------------------------
/*
bool		
MZone::AddObstacle(MObstacle* pObstacle)
{	
	if (!m_ppSector[pObstacle->GetY()][pObstacle->GetX()].AddObstacle(pObstacle))
	{
		// ����� �߰��� ���� ���� ���
		return false;
	}

	// Sector�� �߰��� �Ǿ����ϱ� list���� �߰��Ѵ�.
	// ���߿� memory���� ����� ������ �ϱ� ���ؼ��̴�.
	m_listObstacle.push_front(pObstacle);

	return true;
}
*/

//----------------------------------------------------------------------
// Zone���� Obstacle ����		
//----------------------------------------------------------------------
// Sector(sX,sY)�� �ִ� Obstacle�� �����Ѵ�.
// map������ �����ؾ� �Ѵ�.
//----------------------------------------------------------------------
// ���� �����߿��� ������ �� �Լ��� ȣ��� ������ ����.
// Obstacle�� ������ �繰�̱� �����̴�.
//----------------------------------------------------------------------
//bool		
//MZone::RemoveObstacle(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY)
//{
	/*
	MObstacle* pObstacle;

	// ����� ���ŵ� ���
	if (m_ppSector[sY][sX].RemoveObstacle(pObstacle))
	{
		// pObstacle�� ���ŵ� MObstacle�� ����ִ�.
		// ID�� ���� Obstacle�� mapObstacle���� �����Ѵ�.
		OBSTACLE_MAP::iterator	iObstacle = m_mapObstacle.find(pObstacle->GetID());
    
		// �׷� id�� ���� ImageObjectPositionList�� ����.	
		if (iObstacle == m_mapObstacle.end())
			return false;

		// map���� ����
		m_mapObstacle.erase( iObstacle );

		// memory���� ����
		delete pObstacle;
		
		return true;
	}

	// ����� ���ŵ��� ���� ��� (���ϱ�?) - ���?
	*/
	//return false;
//}


//----------------------------------------------------------------------
// Change Frame Obstacle
//----------------------------------------------------------------------
// �ӽ� �Լ�  :  ��� Obstacle�� Frame�� ��ȭ��Ų��.
//----------------------------------------------------------------------
/*
void
MZone::ChangeFrameObstacle()
{
	OBSTACLE_LIST::iterator iObstacle = m_listObstacle.begin();

	MObstacle* pObstacle;
	while (iObstacle != m_listObstacle.end())
	{
		pObstacle = *iObstacle;

		pObstacle->NextFrame();

		iObstacle++;
	}
	
}
*/

//----------------------------------------------------------------------
// Sector (x,y)�� �ִ� Item�� ID�� �Ѱ��ش�.
//----------------------------------------------------------------------
TYPE_OBJECTID
MZone::GetItemID(TYPE_SECTORPOSITION x, TYPE_SECTORPOSITION y)
{
	//-------------------------------------------------------
	// Zone�� ���� ���̸� check ���Ѵ�.
	//-------------------------------------------------------
	if (//x<0 || y<0 || 
		x>=m_Width || y>=m_Height) return OBJECTID_NULL;

	// ���� �ִ� sector�̸�
	MItem*	pItem = m_ppSector[y][x].GetItem();

	// Item�� ������
	if (pItem != NULL)
	{			
		return pItem->GetID();
	}

	return OBJECTID_NULL;

	
}

//----------------------------------------------------------------------
// Zone�� Item �߰� 
//----------------------------------------------------------------------
// �ܺο��� new����� �Ѵ�.
// �̹� �ִ��� Ȯ���� �ϰ� ������ �߰��ؾ� �Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::AddItem(MItem* pItem, BOOL bDropping)
{
	
	if( pItem->GetItemClass() == ITEM_CLASS_COMMON_QUEST_ITEM )
		int num = 0;

	//--------------------------------------------------------	
	// ����߸��� Frame�� �ִ� ���...
	//--------------------------------------------------------	
	if (pItem->GetItemClass()!=ITEM_CLASS_MOTORCYCLE)	// ������̴� �� ��������. �Ҹ��� �� ����.
	{
		if (bDropping 
			&& pItem->GetDropFrameID()!=FRAMEID_NULL)				// �������� �׸��� ���� ���
		{
			pItem->SetDropping();
		}
		else
		{
			//--------------------------------------------------------	
			// �ٷ� �������� ���� sound���
			//--------------------------------------------------------	
			//PlaySound( pItem->GetTileSoundID(),
			//					false,
			//					g_pPlayer->GetX(), g_pPlayer->GetY());
		}
	}

	//--------------------------------------------------------	
	// zone�� item�߰�
	//--------------------------------------------------------	
	ITEM_MAP::iterator	theIterator;

	theIterator = m_mapItem.find(pItem->GetID());
	
	// ���� ���� Item�̸� �߰�	
	if (theIterator == m_mapItem.end())
	{
		// Sector Setting [��������]
		MSector& sector = m_ppSector[pItem->GetY()][pItem->GetX()];

		if (sector.AddItem(pItem))
		{		
			m_mapItem.insert(ITEM_MAP::value_type(pItem->GetID(), pItem));

			// [��������] - �������ε� ������ block��Ų��.
			if (IsRelicTable(pItem))
			{
				sector.SetBlockGround();
				sector.SetBlockFlying();
			}

			bool bSpecialItem = false;

			// �ƴ��� ���� ���� ����Ʈ ����
			if(pItem->GetItemClass() == ITEM_CLASS_BLOOD_BIBLE)
			{
				bSpecialItem = true;
				ExecuteActionInfoFromMainNode(
					ARMEGA_TILE+pItem->GetItemType(),		// �� ��ü�� RESULT_ACTIONINFO�̴�.
					
					pItem->GetX(), pItem->GetY(), 0,
					DIRECTION_DOWN,
					
					OBJECTID_NULL,
					pItem->GetX(), pItem->GetY(), 0,
					
					0xFFFF,	
					
					NULL
					);
			}

			// �ƴ��� ���� ���� ����Ʈ ����
			if(pItem->GetItemClass() == ITEM_CLASS_CASTLE_SYMBOL)
			{
				int TempEffetType[6] = {	AMBER_OF_GUARD_TILE,
											AMBER_OF_AVENGER_TILE,
											AMBER_OF_IMMORTAL_TILE,
											AMBER_OF_CURSE_TILE,
											AMBER_OF_LIFE_TILE,
											AMBER_OF_LIGHT_TILE,
										};
				int TempItemType = pItem->GetItemType();

				if(TempItemType >5 ) TempItemType = 0;
				bSpecialItem = true;

				ExecuteActionInfoFromMainNode(
					TempEffetType[TempItemType],		// �� ��ü�� RESULT_ACTIONINFO�̴�.
					
					pItem->GetX(), pItem->GetY(), 0,
					DIRECTION_DOWN,
					
					OBJECTID_NULL,
					pItem->GetX(), pItem->GetY(), 0,
					
					0xFFFF,	
					
					NULL
					);
			}

			if(bSpecialItem)
			{
				ExecuteActionInfoFromMainNode(
					DROP_BLOOD_BIBLE,		// �� ��ü�� RESULT_ACTIONINFO�̴�.
					
					pItem->GetX(), pItem->GetY(), 0,
					DIRECTION_DOWN,
					
					OBJECTID_NULL,
					pItem->GetX(), pItem->GetY(), 0,
					
					0xFFFF,	
					
					NULL
					);
			}
			
			// 2004, 10, 25, sobeit add start - �巹�� ���� ����Ʈ ����
			if(pItem->GetItemClass() == ITEM_CLASS_WAR_ITEM)
			{
				int warItemIndex = pItem->GetItemType();
				if(0 == warItemIndex) // �巹�� ���� �� ��
				{
					ExecuteActionInfoFromMainNode(
						SKILL_CLIENT_DRAGON_EYES,		// �� ��ü�� RESULT_ACTIONINFO�̴�.
						
						pItem->GetX(), pItem->GetY(), 0,
						DIRECTION_DOWN,
						
						OBJECTID_NULL,
						pItem->GetX(), pItem->GetY(), 0,
						
						0xFFFF,	
						
						NULL
						);
				}
#if __CONTENTS(__HERO_SYSTEM)
				else if(warItemIndex > 0 && warItemIndex < 7)
				{
					ExecuteActionInfoFromMainNode(
						(SKILL_CLIENT_SLAYER_HERO + ((warItemIndex-1)%3)),		// �� ��ü�� RESULT_ACTIONINFO�̴�.
						
						pItem->GetX(), pItem->GetY(), 0,
						DIRECTION_DOWN,
						
						OBJECTID_NULL,
						pItem->GetX(), pItem->GetY(), 0,
						
						0xFFFF,	
						
						NULL
						);
				}
#endif //__HERO_SYSTEM
			}
			// 2004, 10, 25, sobeit add end - �巹�� ���� ����Ʈ ����

			return true;
		}

		DEBUG_ADD_FORMAT("[Error] Already Exist Item id=%d, xy(%d,%d), cl=%d, ty=%d",
								pItem->GetID(), 
								pItem->GetX(), pItem->GetY(), 
								(int)pItem->GetItemClass(), 
								(int)pItem->GetItemType());
		

		return false;
	}

	// �̹� �ִ� Item�̸�,
	return false;
}
	

//----------------------------------------------------------------------
// Zone���� Item ����
//----------------------------------------------------------------------
// ���ο��� delete���ش�.
//----------------------------------------------------------------------
bool
MZone::RemoveItem(TYPE_OBJECTID id)
{
	DEBUG_ADD_FORMAT("Remove Item. ID=%d", id);
	
	ITEM_MAP::iterator	theIterator;

	ITEM_MAP::iterator tempItr = m_mapItem.begin();
	TYPE_OBJECTID idd;
	while( tempItr != m_mapItem.end() )
	{
		 idd = (*tempItr).first;
		tempItr++;
	}

	// ID�� id�� Item�� ã�´�.
	theIterator = m_mapItem.find(id);
    
	// �׷� id�� ���� Item�� ����.	
	if (theIterator == m_mapItem.end())
		return false;


	// ã�� ��� --> ����	

	// Sector���� ����
	MItem* pItem = NULL;
	// [��������]

	int sector_y = (*theIterator).second->GetY(), sector_x = (*theIterator).second->GetX();
	MSector& sector = m_ppSector[sector_y][sector_x];
	bool removed = sector.RemoveItem(pItem);

	if (removed && pItem!=NULL)
	{
		// [��������]
		if (IsRelicTable(pItem))
		{
			sector.UnSetBlockGround();
			sector.UnSetBlockFlying();
		}
		bool bSpecialItem = false;
		
		// �ƴ��� ���� ���� ����Ʈ ����
		if(pItem->GetItemClass() == ITEM_CLASS_BLOOD_BIBLE)
		{
			bSpecialItem = true;
						
			if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)(EFFECTSPRITETYPE_ARMEGA_TILE + pItem->GetItemType()) ) )
			{
				DEBUG_ADD("[Error] Can't Remove Effect From Tile (ARMEGA_TILE HardCoding)");
			}
		}
		
		// 2004, 10, 25, sobeit add start - �巹�� ���� ����Ʈ ����
		if(pItem->GetItemClass() == ITEM_CLASS_WAR_ITEM)
		{
			int warItemIndex = pItem->GetItemType();
			if(0 == warItemIndex)
			{
				if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)(EFFECTSPRITETYPE_DRAGON_EYE)) )
				{
					DEBUG_ADD("[Error] Can't Remove Effect From Tile (ITEM_CLASS_WAR_ITEM)");
				}
			}
#if __CONTENTS(__HERO_SYSTEM)
			else if(warItemIndex > 0 && warItemIndex < 7)
			{
				if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)(EFFECTSPRITETYPE_SLAYER_HERO + ((warItemIndex-1)%3)) ))
				{
					DEBUG_ADD("[Error] Can't Remove Effect From Tile (ITEM_CLASS_WAR_ITEM)");
				}
			}
#endif //__HERO_SYSTEM

		}
		// 2004, 10, 25, sobeit add end
		// �ƴ��� ���� ���� ����Ʈ ����
		if(pItem->GetItemClass() == ITEM_CLASS_CASTLE_SYMBOL)
		{
			bSpecialItem = true;

			int TempEffetType[6] = {	EFFECTSPRITETYPE_AMBER_OF_GUARD_TILE,
										EFFECTSPRITETYPE_AMBER_OF_AVENGER_TILE,
										EFFECTSPRITETYPE_AMBER_OF_IMMORTAL_TILE,
										EFFECTSPRITETYPE_AMBER_OF_CURSE_TILE,
										EFFECTSPRITETYPE_AMBER_OF_LIFE,
										EFFECTSPRITETYPE_AMBER_OF_LIGHT,
										};
				int TempItemType = pItem->GetItemType();
				if(TempItemType >5 ) TempItemType = 0;
			if (!g_pZone->RemoveTileEffect( sector_x,sector_y,  (TYPE_EFFECTSPRITETYPE)(TempEffetType[TempItemType]) ) )
			{
				DEBUG_ADD("[Error] Can't Remove Effect From Tile (AMBER_OF_GUARD_TILE HardCoding)");
			}
		}
		
		if(bSpecialItem)
		{
			if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)EFFECTSPRITETYPE_DROP_BLOOD_BIBLE) )
			{
				DEBUG_ADD("[Error] Can't Remove Effect From Tile (DropBloodBible HardCoding)");
			}
		}

		m_mapItem.erase(theIterator);

		//------------------------------------------------------
		// Item Check Buffer�� �ִ� ��� ������� �Ѵ�. 2001.8.27
		//------------------------------------------------------
		MItem* pCheckItem = g_pPlayer->GetItemCheckBuffer();

		if (pCheckItem!=NULL)
		{
			DEBUG_ADD_FORMAT("Remove Item. ID=%d", id);

			if (pItem->GetID()==pCheckItem->GetID())
			{
				//---------------------------------------------
				// item check buffer�� �����ش�.
				//---------------------------------------------
				g_pPlayer->ClearItemCheckBuffer();
			}
		}
		else
		{
			DEBUG_ADD("Check Item is NULL");
		}

		// memory���� ����
		//delete pItem;	
		SAFE_DELETE ( pItem );
	}

	return removed;
}

//----------------------------------------------------------------------
// Zone���� Item ����
//----------------------------------------------------------------------
// zone���� ���Ÿ� �ϰ� delete���� �ʴ´�.
//----------------------------------------------------------------------
bool
MZone::PickupItem(TYPE_OBJECTID id)
{
	ITEM_MAP::iterator	theIterator;

	// ID�� id�� Item�� ã�´�.
	theIterator = m_mapItem.find(id);
    
	// �׷� id�� ���� Item�� ����.	
	if (theIterator == m_mapItem.end())
		return false;


	// ã�� ��� --> ����	

	// Sector���� ����
	MItem* pItem = NULL;
	int sector_y = (*theIterator).second->GetY(), sector_x = (*theIterator).second->GetX();
	bool removed = m_ppSector[sector_y][sector_x].RemoveItem(pItem);

	bool bSpecialItem = false;

	if(pItem != NULL && removed)
	{
		// �ƴ��� ���� ���� ����Ʈ ����
		if(pItem->GetItemClass() == ITEM_CLASS_BLOOD_BIBLE)
		{
			bSpecialItem = true;
						
			if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)(EFFECTSPRITETYPE_ARMEGA_TILE + pItem->GetItemType() )) )
			{
				DEBUG_ADD("[Error] Can't Remove Effect From Tile (ARMEGA_TILE HardCoding)");
			}
		}
		
		// 2004, 10, 25, sobeit add start - �巹�� ���� ����Ʈ ����
		if(pItem->GetItemClass() == ITEM_CLASS_WAR_ITEM)
		{
			int warItemIndex = pItem->GetItemType();

			if(0 == warItemIndex)
			{
				if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)(EFFECTSPRITETYPE_DRAGON_EYE)) )
				{
					DEBUG_ADD("[Error] Can't Remove Effect From Tile (ITEM_CLASS_WAR_ITEM)");
				}
			}
#if __CONTENTS(__HERO_SYSTEM)
			else if(warItemIndex > 0 && warItemIndex < 7)
			{
				if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)(EFFECTSPRITETYPE_SLAYER_HERO + ((warItemIndex-1)%3)) ))
				{
					DEBUG_ADD("[Error] Can't Remove Effect From Tile (ITEM_CLASS_WAR_ITEM)");
				}
			}
#endif //__HERO_SYSTEM
		}
		// 2004, 10, 25, sobeit add end
		
		// �ƴ��� ���� ���� ����Ʈ ����
		if(pItem->GetItemClass() == ITEM_CLASS_CASTLE_SYMBOL)
		{
			bSpecialItem = true;

			int TempEffetType[6] = {	EFFECTSPRITETYPE_AMBER_OF_GUARD_TILE,
									EFFECTSPRITETYPE_AMBER_OF_AVENGER_TILE,
									EFFECTSPRITETYPE_AMBER_OF_IMMORTAL_TILE,
									EFFECTSPRITETYPE_AMBER_OF_CURSE_TILE,
									EFFECTSPRITETYPE_AMBER_OF_LIFE,
									EFFECTSPRITETYPE_AMBER_OF_LIGHT,
									};
			int TempItemType = pItem->GetItemType();
			if(TempItemType >5 ) TempItemType = 0;
			if (!g_pZone->RemoveTileEffect( sector_x,sector_y,  (TYPE_EFFECTSPRITETYPE)(TempEffetType[TempItemType] )) )
			{
				DEBUG_ADD("[Error] Can't Remove Effect From Tile (AMBER_OF_GUARD_TILE HardCoding)");
			}
		}
		
		if(bSpecialItem)
		{
			if (!g_pZone->RemoveTileEffect( sector_x,sector_y,   (TYPE_EFFECTSPRITETYPE)EFFECTSPRITETYPE_DROP_BLOOD_BIBLE) )
			{
				DEBUG_ADD("[Error] Can't Remove Effect From Tile (DropBloodBible HardCoding)");
			}
		}
	}


	if (removed)
	{
		m_mapItem.erase(theIterator);
	}

	return removed;
}

//----------------------------------------------------------------------
// Zone�� Item �о����
//----------------------------------------------------------------------
MItem*	
MZone::GetItem(TYPE_OBJECTID id)
{
	ITEM_MAP::iterator	theIterator;

	// ID�� id�� Item�� ã�´�.
	theIterator = m_mapItem.find(id);

	// ���� ��� NULL�� return�Ѵ�.
	if (theIterator == m_mapItem.end()) 
		return NULL;

	// ������ �� Item�� return�Ѵ�.
	return (*theIterator).second;
}

//----------------------------------------------------------------------
// Zone�� Effect �߰�
//----------------------------------------------------------------------
// m_mapEffect�� �߰��Ѵ�.
// Sector�� listEffect�� �߰��Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::AddEffect(MEffect* pNewEffect, DWORD dwWaitCount)
{	
	DEBUG_ADD("AddEffect");

	if(dwWaitCount)
	{
		m_listWaitEffect.push_back( pNewEffect );
		return true;
	}

	int x = pNewEffect->GetX();
	int y = pNewEffect->GetY();


	// Effect�� �߰��� �� ���� ���
	if(CanAddEffect(pNewEffect) == false)
	{
		delete pNewEffect;
		return false;
	}
	

	//---------------------------------------------------------
	// Zone�� ��踦 �Ѿ�� ���..
	//---------------------------------------------------------
	if (x<0 || x>=m_Width
		|| y<0 || y>=m_Height)
	{
		DEBUG_ADD("OuterZone");

		// chase effect �� �Ѿ�� ������.
		if (pNewEffect->GetEffectType()==MEffect::EFFECT_CHASE)
		{
			TYPE_OBJECTID id = pNewEffect->GetID();

			// ���� ��츸 �߰��Ѵ�.
			if (m_mapEffect.find( id )==m_mapEffect.end())
			{
				// list�� �߰�
				m_mapEffect[id] = pNewEffect;				

				DEBUG_ADD("ChaseEffect");

				return true;
			}
		}

		DEBUG_ADD("deleteEff");

		delete pNewEffect;
		
		return false;
	}

	MSector &sector = m_ppSector[y][x];

	//---------------------------------------------------------
	// ������ Effect�� ��..
	//
	// Sector�� ������ Effect�� ��츸...
	//---------------------------------------------------------
	BOOL AddOK = TRUE;

	if (pNewEffect->GetEffectType()==MEffect::EFFECT_SECTOR && !pNewEffect->IsMulti())
	{
		DEBUG_ADD("getSector");

		
		if (sector.IsExistEffect())
		{
			EFFECT_LIST::const_iterator iEffect = sector.GetEffectIterator();
			
			int num = sector.GetEffectSize();

			DEBUG_ADD_FORMAT("secEffNum=%d", num);

			for (int i=0; i<num; i++)
			{
				MEffect* pEffect = *iEffect;

				//---------------------------------------------------------
				// �׸��� ���ٸ� ���� Effect�� ����Ѵ�.
				// �ð��� �� ���� ��´�.
				//---------------------------------------------------------
				// ���� type�� ã�´�.
				if (pEffect->GetEffectType()==MEffect::EFFECT_SECTOR)
				{
					int fid1 = pNewEffect->GetFrameID();
					int fid2 = pEffect->GetFrameID();

					int est1 = g_pTopView->GetEffectSpriteType( (BLT_TYPE)pNewEffect->GetBltType(), fid1 );
					int est2 = g_pTopView->GetEffectSpriteType( (BLT_TYPE)pEffect->GetBltType(), fid2 );
					
//					// 2004, 10, 19, sobeit add start ���� 140 ��ų ����
//					if(est1 == EFFECTSPRITETYPE_SPIT_STREAM ||
//						est1 == EFFECTSPRITETYPE_GREAT_RUFFIAN_2_AXE_THROW ||
//						est1 == EFFECTSPRITETYPE_GREAT_RUFFIAN_2_AXE_THROW_SHADOW ||
//						est1 == EFFECTSPRITETYPE_GREAT_RUFFIAN_1_AXE_GROUND ||
//						est1 == EFFECTSPRITETYPE_GREAT_RUFFIAN_1_AXE_WAVE
//						)
//					{
//						AddOK = TRUE; // �� ��ų�� ���� �׸��� ��Ÿ�Ͽ� �ΰ� �̻� �ٸ���ġ�� ��Ÿ�� ���� �ִ�..-_-
//					}
//					else
//					// 2004, 10, 19, sobeit add end
					if (fid1 == fid2)
					{
						// �߰��ҷ��� ���� �� �ʰ� ���� ��쿡�� �ð��� Ȯ���Ѵ�.
						if (pNewEffect->GetEndFrame() > pEffect->GetEndFrame() &&
							est1 != EFFECTSPRITETYPE_DRAGON_FIRE_CRACKER) 
						{
							int count = pNewEffect->GetEndFrame() - g_CurrentFrame;
							
							pEffect->SetCount( count );
						}

						// ������ �ִٸ� �߰�����..
						AddOK = FALSE;
						
						if( est1 ==	EFFECTSPRITETYPE_DRAGON_FIRE_CRACKER )
							AddOK = TRUE;

						break;
					}
					// �Ǵ� Darkness�� ���... ����~~ -_-;;
					else
					{					
						if (
							( est1>=EFFECTSPRITETYPE_DARKNESS_1_1
							&& est1<=EFFECTSPRITETYPE_DARKNESS_3_5
							&& est2>=EFFECTSPRITETYPE_DARKNESS_1_1
							&& est2<=EFFECTSPRITETYPE_DARKNESS_3_5

							&& (est1>=EFFECTSPRITETYPE_DARKNESS_2_1
								&& est1<=EFFECTSPRITETYPE_DARKNESS_2_5
								&& est2>=EFFECTSPRITETYPE_DARKNESS_2_1
								&& est2<=EFFECTSPRITETYPE_DARKNESS_2_5
								|| est1>=EFFECTSPRITETYPE_DARKNESS_1_1
								&& est1<=EFFECTSPRITETYPE_DARKNESS_1_5
								&& est2>=EFFECTSPRITETYPE_DARKNESS_1_1
								&& est2<=EFFECTSPRITETYPE_DARKNESS_1_5
								|| est1>=EFFECTSPRITETYPE_DARKNESS_3_1
								&& est1<=EFFECTSPRITETYPE_DARKNESS_3_5
								&& est2>=EFFECTSPRITETYPE_DARKNESS_3_1
								&& est2<=EFFECTSPRITETYPE_DARKNESS_3_5) 
								)||

							(	est1>=EFFECTSPRITETYPE_GRAY_DARKNESS_1_1
							&& est1<=EFFECTSPRITETYPE_GRAY_DARKNESS_3_5
							&& est2>=EFFECTSPRITETYPE_GRAY_DARKNESS_1_1
							&& est2<=EFFECTSPRITETYPE_GRAY_DARKNESS_3_5

							&& (est1>=EFFECTSPRITETYPE_GRAY_DARKNESS_2_1
								&& est1<=EFFECTSPRITETYPE_GRAY_DARKNESS_2_5
								&& est2>=EFFECTSPRITETYPE_GRAY_DARKNESS_2_1
								&& est2<=EFFECTSPRITETYPE_GRAY_DARKNESS_2_5
								|| est1>=EFFECTSPRITETYPE_GRAY_DARKNESS_1_1
								&& est1<=EFFECTSPRITETYPE_GRAY_DARKNESS_1_5
								&& est2>=EFFECTSPRITETYPE_GRAY_DARKNESS_1_1
								&& est2<=EFFECTSPRITETYPE_GRAY_DARKNESS_1_5
								|| est1>=EFFECTSPRITETYPE_GRAY_DARKNESS_3_1
								&& est1<=EFFECTSPRITETYPE_GRAY_DARKNESS_3_5
								&& est2>=EFFECTSPRITETYPE_GRAY_DARKNESS_3_1
								&& est2<=EFFECTSPRITETYPE_GRAY_DARKNESS_3_5)
								)								
							)
						{
							// �߰��ҷ��� ���� �� �ʰ� ���� ��쿡�� �ð��� Ȯ���Ѵ�.
							if (pNewEffect->GetEndFrame() > pEffect->GetEndFrame()) 
							{
								int count = pNewEffect->GetEndFrame() - g_CurrentFrame;
								
								pEffect->SetCount( count );
							}

							// ������ �ִٸ� �߰�����..
							AddOK = FALSE;

							// �ݺ� darkness�� ��쿡��..
							if (est1>=EFFECTSPRITETYPE_DARKNESS_2_1 &&
								est1<=EFFECTSPRITETYPE_DARKNESS_2_5 ||
								est1>=EFFECTSPRITETYPE_GRAY_DARKNESS_2_1 &&
								est1<=EFFECTSPRITETYPE_GRAY_DARKNESS_2_5)
							{
								sector.SetDarkness();
							}

						
							CheckCreatureInDarkness( sector, x, y );

							break;
						}	// [�����]
							// sanctuary �ִ� ���			
						else if (est1>=EFFECTSPRITETYPE_SANCTUARY_1
								&& est1<=EFFECTSPRITETYPE_SANCTUARY_3)
						{
							// ��ó Ÿ�ϵ鿡�� üũ�Ѵ�. [�����9]
							for (int i=-1; i<=1; i++)
							{							
								for (int j=-1; j<=1; j++)
								{	
									int x0 = x + i;
									int y0 = y + j;

									if (x0<0 || x0>=m_Width
										|| y0<0 || y0>=m_Height)
									{
										continue;
									}

									MSector& sector2 = m_ppSector[y0][x0];
									sector2.SetSanctuary();
								}
							}
						}

					}
				}

				iEffect++;
			}
		}
	}
			
	//---------------------------------------------------------
	// �߰��ص� �Ǵ� ���
	//---------------------------------------------------------
	if (AddOK)
	{
		DEBUG_ADD("addOK");

		TYPE_OBJECTID id = pNewEffect->GetID();

		// ���� ��츸 �߰��Ѵ�.
		if (m_mapEffect.find( id )==m_mapEffect.end())
		{
			// list�� �߰�
			m_mapEffect[id] = pNewEffect;

			// sector�� �߰�
			MSector& sector = m_ppSector[y][x];

			DEBUG_ADD("secAddEff");
			sector.AddEffect( pNewEffect );

			DEBUG_ADD("chkDark");
			CheckCreatureInDarkness( sector, x, y );
			DEBUG_ADD("chkDarkOK");

			return true;
		}
	}

	// �þ� ����
//	if (pEffect->GetBltType()==BLT_EFFECT)
//	{
//		SetLight(pEffect->GetX(), pEffect->GetY(), pEffect->GetLight());		
//	}

	DEBUG_ADD("delEff");
	delete pNewEffect;

	DEBUG_ADD("reF");

	return false;
}


//----------------------------------------------------------------------
// Can Add Effect
//----------------------------------------------------------------------
bool
MZone::CanAddEffect(MEffect* pNewEffect)
{
	int x = pNewEffect->GetX();
	int y = pNewEffect->GetY();

	const TYPE_FRAMEID frameID = pNewEffect->GetFrameID();

	bool bDarkNess =	frameID >= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_DARKNESS_1_1].FrameID		&&
						frameID <= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_DARKNESS_3_5].FrameID		||
						frameID >= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_GRAY_DARKNESS_1_1].FrameID &&
						frameID <= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_GRAY_DARKNESS_3_5].FrameID;

	bool bAcidSwamp =	(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_ACID_SWAP_1].FrameID <= frameID &&
						(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_ACID_SWAP_3].FrameID >= frameID;

	bool bProminence =	(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_PROMINENCE_ING].FrameID <= frameID		&&
						(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_PROMINENCE_START].FrameID >= frameID	||
						(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_PROMINENCE2_ING].FrameID <= frameID	&&
						(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_PROMINENCE3_START].FrameID >= frameID;

	bool bBloodWall =	(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_BLOODY_WALL_1].FrameID <= frameID	&&
						(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_BLOODY_WALL_3].FrameID >= frameID;

	bool bHellFire =	(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_HELLFIRE_END].FrameID <= frameID	&&
						(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_HELLFIRE_BODY_GROUND].FrameID >= frameID;

	bool bFuryOfGnome =	(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_FURY_OF_GNOME_GROUND_START].FrameID <= frameID	&&
						(*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_FURY_OF_GNOME_STONE_LOOP].FrameID >= frameID;

	
	if(bDarkNess)
	{
		const MSector& sector= m_ppSector[y][x];
		if(sector.HasDarknessForbidden() == true )
		{
			return false;
		}
	}
	// ��� �ֺ����� ��ũ�Ͻ��� ���� �ʴ´�.
	if( bDarkNess )
	{
		if( !( x < 0 || x >= GetWidth() || y < 0 || y >=GetHeight() ) )
		{
			const MSector& sector= m_ppSector[y][x];
			const MItem* pItem = sector.GetItem();
			
			// ��밡 �ִ°��� ��ũ�Ͻ��� ��������.
			if( pItem != NULL && pItem->GetItemClass() == ITEM_CLASS_CORPSE &&
				((MCorpse*)pItem)->GetCreature() != NULL &&
				((MCorpse*)pItem)->GetCreature()->GetCreatureType() == 670 )
			{
				return false;
			}
		}		
	}

	// � ����Ʈ�� �ɷ��ִ� ��� �� ���� �ɸ��� �ʴ� ����Ʈ�� �ִ�.
	if( bDarkNess || bAcidSwamp || bProminence || bBloodWall || bHellFire || bFuryOfGnome)
	{
		bool bCantUseMercyGround				= bDarkNess || bAcidSwamp || bProminence;
		TYPE_FRAMEID frameIDMercyGroundBegin	= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_MERCY_GROUND_1].FrameID;
		TYPE_FRAMEID frameIDMercyGroundEnd		= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_MERCY_GROUND_9].FrameID;

		bool bCantUseRegenTower					= bDarkNess || bAcidSwamp || bProminence;
		TYPE_FRAMEID frameIDRegenTower			= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_REGEN_TOWER_GROUND].FrameID;

		bool bCantUseHeavenGround				= bDarkNess || bAcidSwamp || bProminence || bBloodWall || bHellFire || bFuryOfGnome;
		TYPE_FRAMEID frameIDHeavenGroundBegin	= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_HEAVEN_GROUND_REPEAT].FrameID;
		TYPE_FRAMEID frameIDHeavenGroundEnd		= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_HEAVEN_GROUND_START].FrameID;

		bool bCantUseSummonClay					= bDarkNess || bAcidSwamp || bBloodWall;
		TYPE_FRAMEID frameIDSummonClayBegin		= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_SUMMON_CLAY_START].FrameID;
		TYPE_FRAMEID frameIDSummonClayEnd		= (*g_pEffectSpriteTypeTable)[EFFECTSPRITETYPE_SUMMON_CLAY_END].FrameID;

		EFFECT_MAP::iterator iGroundEffect = m_mapGroundEffect.begin();
		
		for(;iGroundEffect != m_mapGroundEffect.end(); ++iGroundEffect)
		{
			MEffect* pEffect = iGroundEffect->second;
			TYPE_FRAMEID SearchedFrameID = pEffect->GetFrameID();
			
			const int effectX = pEffect->GetX();
			const int effectY = pEffect->GetY();

			// Mercy Ground �� ��� ������ ��ũ�Ͻ� ���� �ʴ´�.
			if(bCantUseMercyGround							&&
			   SearchedFrameID >= frameIDMercyGroundBegin	&&
			   SearchedFrameID <= frameIDMercyGroundEnd)
			{
				int fix_y = (SearchedFrameID - frameIDMercyGroundBegin) / 3 - 1;
				int fix_x = (SearchedFrameID - frameIDMercyGroundBegin) % 3 - 1;

				if(effectX + fix_x == x && effectY + fix_y == y)
				{
					return false;
				}
			}
			else if(bCantUseRegenTower						&&
					x == effectX && y == effectY			&&
					SearchedFrameID == frameIDRegenTower)
			{
				return false;
			}
			else if(bCantUseHeavenGround							&&
					SearchedFrameID >= frameIDHeavenGroundBegin		&&
					SearchedFrameID <= frameIDHeavenGroundEnd)
			{
				if(x <= effectX + 2 && x >= effectX - 2 &&
				   y <= effectY + 2 && y >= effectY - 2)
				{
					return false;
				}
			}
			else if(bCantUseSummonClay							&&
					SearchedFrameID >= frameIDSummonClayBegin	&&
					SearchedFrameID <= frameIDSummonClayEnd)
			{
				if(x <= effectX + 2 && x >= effectX - 2 &&
				   y <= effectY + 2 && y >= effectY - 2)
				{
					return false;
				}
			}
		}
	}

	return true;
}

//----------------------------------------------------------------------
// Get Effect
//----------------------------------------------------------------------
MEffect*	
MZone::GetEffect(TYPE_OBJECTID id) const
{
	EFFECT_MAP::const_iterator iEffect = m_mapEffect.find( id );

	if (iEffect==m_mapEffect.end())
	{
		return NULL;
	}

	return iEffect->second;
}

//----------------------------------------------------------------------
// Remove Effect ( id )
//----------------------------------------------------------------------
// effect�� id�� ���� �ǹ̰� ������..
// �����ζ� �������ϴ� ��찡 �־��.. - -;
//----------------------------------------------------------------------
bool		
MZone::RemoveEffect(TYPE_OBJECTID id)
{
	EFFECT_MAP::iterator iEffect = m_mapEffect.find( id );

	if (iEffect==m_mapEffect.end())
	{
		return false;
	}

	MEffect* pEffect = iEffect->second;

	int x = pEffect->GetX();
	int y = pEffect->GetY();

	// ��踦 ���� �ʴ� ���
	if (x>=0 && x<m_Width
		&& y>=0 && y<m_Height)
	{
		MSector& sector = m_ppSector[y][x];
		
		// [�����9]
		RemoveSectorEffect( x, y, pEffect->GetID() );

		CheckCreatureInDarkness( sector, x, y );
	}

	delete pEffect;

	m_mapEffect.erase( iEffect );

	return true;
}

//----------------------------------------------------------------------
// Remove Tile Effect
//----------------------------------------------------------------------
// tile�� �ִ� effect�� �����.
//----------------------------------------------------------------------
bool		
MZone::RemoveTileEffect(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, int effectStatus, int serverID)
{
	if (effectStatus >= g_pEffectStatusTable->GetSize())
	{
		DEBUG_ADD_FORMAT("[Error]EffectStatus is NULL or Over: %d", effectStatus);

		return false;
	}

	TYPE_EFFECTSPRITETYPE type = (*g_pEffectStatusTable)[effectStatus].EffectSpriteType;
	
	//----------------------------------------------------------------------
	// EffectStatus�� ���� Ư���Ѱ� üũ
	//----------------------------------------------------------------------
	switch (effectStatus)
	{
		case EFFECTSTATUS_TRYING_POSITION :
		case EFFECTSTATUS_TRAP_INSTALLED :
		{
			bool bDelete = false;
			
			EFFECT_MAP::iterator iGroundEffect = m_mapGroundEffect.begin();
			
			while (iGroundEffect != m_mapGroundEffect.end())
			{
				MEffect* pEffect = iGroundEffect->second;
				
				MEffectTarget* pEffectTarget = pEffect->GetEffectTarget();
				TYPE_FRAMEID	frameID	= (*g_pEffectSpriteTypeTable)[type].FrameID;

				if( pEffect->GetFrameID() == frameID &&
					pEffect->GetX() == sX &&
					pEffect->GetY() == sY )
				{
					delete pEffect;
					m_mapGroundEffect.erase( iGroundEffect );
					break;
				}
				
				iGroundEffect++;
			}
			
			DEBUG_ADD("EffectSpriteType of EffectStatus is NULL");
			
			return bDelete;
		}
		break;

		//----------------------------------------------------------------
		// vampire ��Ż�� m_mapGroundEffect�� �߰��Ǿ� �ִ�..
		//----------------------------------------------------------------		
		case EFFECTSTATUS_VAMPIRE_PORTAL :
		{
			bool bDelete = false;

			EFFECT_MAP::iterator iGroundEffect = m_mapGroundEffect.begin();

			while (iGroundEffect != m_mapGroundEffect.end())
			{
				MEffect* pEffect = iGroundEffect->second;

				MEffectTarget* pEffectTarget = pEffect->GetEffectTarget();

				if (pEffectTarget!=NULL)
				{
					//------------------------------------------------------------------
					// Portal�� ���
					//------------------------------------------------------------------
					if (pEffectTarget->GetEffectTargetType()==MEffectTarget::EFFECT_TARGET_PORTAL)
					{
						MPortalEffectTarget* pPortalEffectTarget = (MPortalEffectTarget*)pEffectTarget;

						int portalServerID = pPortalEffectTarget->GetServerID();
						
						if (serverID==portalServerID)
						{
							delete pEffect;

							m_mapGroundEffect.erase( iGroundEffect );

							break;
						}
					}
				}

				iGroundEffect++;
			}

			DEBUG_ADD("EffectSpriteType of EffectStatus is NULL");

			return bDelete;
		}
		break;	// - -;
		
		case EFFECTSTATUS_GROUND_ELEMENTAL_AURA:
			if( g_pZone != NULL )
			{
				const_cast<MSector &>(g_pZone->GetSector(sX, sY)).UnSetGroundElemental();
			}
			break;
		
		case EFFECTSTATUS_DARKNESS_FORBIDDEN:
			if( g_pZone != NULL )
			{
				const_cast<MSector &>(g_pZone->GetSector(sX, sY)).UnSetDarknessForbidden();
			}
			break;
			
		case EFFECTSTATUS_FURY_OF_GNOME:
			if( g_pZone != NULL )
			{
				const_cast<MSector &>(g_pZone->GetSector(sX, sY)).UnSetFuryOfGnome();
			}
			break;	
		case EFFECTSTATUS_DRAGON_TORNADO:
		case EFFECTSTATUS_DRAGON_TORNADO_CHILD:
		case EFFECTSTATUS_DRAGON_HURRICANE:
		case EFFECTSTATUS_DRAGON_HURRICANE_CHILD:
		case EFFECTSTATUS_DRAGON_HURRICANE_2:
		case EFFECTSTATUS_DRAGON_HURRICANE_2_SMALL:
			{
				return RemoveFakeCreature(serverID);
			}
		case EFFECTSTATUS_DELETE_TILE:
			if( g_pZone != NULL )
			{
				const_cast<MSector &>(g_pZone->GetSector(sX, sY)).UnSetDisableTileImage();
				if(g_pTopView != NULL)
					g_pTopView->SetFirstDraw();
			}
			return true;
		case EFFECTSTATUS_MERCY_GROUND:
			{
				typedef std::map<TYPE_FRAMEID, TYPE_EFFECTSPRITETYPE> FRAMEIDSPRITETYPE_MAP;

				FRAMEIDSPRITETYPE_MAP MercyGroundFrameIDSet;

				for(int i = 0; i < 9; ++i)
				{
					TYPE_EFFECTSPRITETYPE		effectSpriteType	= EFFECTSPRITETYPE_MERCY_GROUND_1 + i;
					TYPE_FRAMEID				frameID = (*g_pEffectSpriteTypeTable)[effectSpriteType].FrameID;
					MercyGroundFrameIDSet.insert(std::make_pair(frameID,effectSpriteType));
				}

				TYPE_SECTORPOSITION sLeft	= sX - 1;
				TYPE_SECTORPOSITION sRight	= sX + 1;
				TYPE_SECTORPOSITION sTop	= sY - 1;
				TYPE_SECTORPOSITION sBottom	= sY + 1;

				EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();
				
				while(iEffect != m_mapGroundEffect.end())
				{
					MEffect* pEffect = iEffect->second;
					
					TYPE_FRAMEID frameID = pEffect->GetFrameID();

					int effectX = pEffect->GetX();
					int effectY = pEffect->GetY();

					if(effectX >= sLeft	&&	effectX <= sRight	&&
					   effectY >= sTop	&&  effectY <= sBottom)
					{
						FRAMEIDSPRITETYPE_MAP::iterator iter = MercyGroundFrameIDSet.find(frameID);
					
						if(iter == MercyGroundFrameIDSet.end()) {
							++iEffect;
							continue;
						}

						TYPE_EFFECTSPRITETYPE		effectSpriteType = iter->second;
						int idx = effectSpriteType - EFFECTSPRITETYPE_MERCY_GROUND_1;
						int ix = effectX + idx % 3 - 1;
						int iy = effectY + idx / 3 - 1;

						if(ix == sX && iy == sY)
						{
							delete pEffect;
							EFFECT_MAP::iterator iBackup = iEffect++;
							m_mapGroundEffect.erase(iBackup);
							continue;
						}
					}

					++iEffect;
				}
			}
			return true;
		case EFFECTSTATUS_PROMINENCE:
		case EFFECTSTATUS_PROMINENCE_2:
		case EFFECTSTATUS_PROMINENCE_3:
			{
				int LevelOfEffectSprite[3] = {
					EFFECTSPRITETYPE_PROMINENCE_ING, EFFECTSPRITETYPE_PROMINENCE2_ING, EFFECTSPRITETYPE_PROMINENCE3_ING
				};

				for(int j = 0; j < 3; ++j)
				{
					int effectSpriteID = LevelOfEffectSprite[j];
					for(int i = 0; i < 3; ++i)
						RemoveTileEffect(sX, sY, TYPE_EFFECTSPRITETYPE(effectSpriteID + i));
				}
			}
			return true;
	}

	bool rtn = false;

	// EffectSpriteType�� ����� �����Ѵ�.
	if(type != EFFECTSPRITETYPE_NULL) {
		rtn |= RemoveTileEffect(sX, sY, type, serverID);
	}
 
	// ActionInfo�� ����� �����Ѵ�.
	TYPE_ACTIONINFO actionInfoIdx = (*g_pEffectStatusTable)[effectStatus].ActionInfo;
	if(actionInfoIdx != ACTIONINFO_NULL) {
		rtn |= RemoveTileEffectByActionInfo(sX, sY, actionInfoIdx, serverID);
		rtn |= RemoveGroundEffectByActionInfo(sX, sY, actionInfoIdx);
	}
	
	return rtn;
}


//----------------------------------------------------------------------
// Remove Tile Effect
//----------------------------------------------------------------------
// tile�� �ִ� effect�� �����.
//----------------------------------------------------------------------
bool		
MZone::RemoveTileEffectByActionInfo(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, TYPE_ACTIONINFO actionInfoIdx, int serverID)
{
	MActionInfo&	actionInfo	= (*g_pActionInfoTable)[actionInfoIdx];
	int				size		= actionInfo.GetSize();
	bool			rtn			= false;

	for(int idx = 0; idx < size; ++idx) {
		TYPE_EFFECTSPRITETYPE	effectSpriteType	= actionInfo[idx].EffectSpriteType;
		TYPE_EFFECTGENERATORID	effectGeneratorId	= actionInfo[idx].EffectGeneratorID;

		// Effect Generator���� ����� ����� �ٸ���.
		// ����� ���� Effect�� Effect Generator�� ���ٸ�
		// �׶� �׶� �߰��Ѵ� -��-

		switch(effectSpriteType)
		{
		case EFFECTSPRITETYPE_BLOODY_WALL_1:
			{
				for(int i = 0; i < 3; ++i) {
					TYPE_EFFECTSPRITETYPE type = effectSpriteType+i;
					rtn |= RemoveTileEffect(sX, sY, type, serverID);
				}
			}
			break;
		default:
			rtn |= RemoveTileEffect(sX, sY, effectSpriteType, serverID);
			break;
		}
	}
	
	return rtn;
}

/*
//----------------------------------------------------------------------
// Remove Mercy Ground Tile Effect
//----------------------------------------------------------------------
// tile�� �ִ� Mercy Ground effect�� �����.
//----------------------------------------------------------------------
bool		
MZone::RemoveMercyGroundTileEffect(TYPE_SECTORPOSITION sLeft, TYPE_SECTORPOSITION sTop, TYPE_SECTORPOSITION sRight, TYPE_SECTORPOSITION sBottom)
{
	bool	rtn			= false;

	// �ϴ� �߰�Ÿ���� �˻��ؼ� �����ڸ� Ÿ�ϵ��� �����.
	// �߰�Ÿ���� �˻��� �ʿ��ϱ� ������ �ϴ� ������ �ʴ´�.
	for(int iy = sTop; iy <= sBottom; ++iy)
		for(int ix = sLeft; ix <= sRight; ++ix)
		{
			bool	findCenter	= false;
			int		findPos		= -1;
			int		centerX		= -1;
			int		centerY		= -1;
			
			for(int i = 0; i < 9; ++i)
			{
				if(i == 4) continue;	// �߰� Ÿ���� ���߿� �����.

				centerX = ix + i%3-1;
				centerY = iy + i/3-1;
				
				EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();
				
				for(; iEffect != m_mapGroundEffect.end(); ++iEffect)
				{
					MEffect* pEffect = iEffect->second;
					
					if(pEffect->GetX() != centerX || pEffect->GetY() != centerY) continue;
					
					TYPE_EFFECTSPRITETYPE		effectSpriteType	= EFFECTSPRITETYPE_MERCY_GROUND_5;
					TYPE_FRAMEID				frameID = (*g_pEffectSpriteTypeTable)[effectSpriteType].FrameID;
					
					if( pEffect->GetFrameID() != frameID ) continue;
					
					findCenter	= true;
					findPos		= 8-i;
					break;
				}
				
				if(findCenter)
				{
					EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();
					
					for(; iEffect != m_mapGroundEffect.end(); ++iEffect)
					{
						MEffect* pEffect = iEffect->second;
						
						if(pEffect->GetX() != centerX || pEffect->GetY() != centerY) continue;
						
						TYPE_EFFECTSPRITETYPE		effectSpriteType	= EFFECTSPRITETYPE_MERCY_GROUND_1 + findPos;
						TYPE_FRAMEID				frameID = (*g_pEffectSpriteTypeTable)[effectSpriteType].FrameID;
						
						if( pEffect->GetFrameID() != frameID ) continue;
						
						delete pEffect;
						rtn = true;
						m_mapGroundEffect.erase(iEffect);
						break;
					}
				}
			}
		}


	// ���� �߰� Ÿ�ϸ� �����.
	for(iy = sTop; iy <= sBottom; ++iy)
		for(int ix = sLeft; ix <= sRight; ++ix)
		{
			EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();

			for(; iEffect != m_mapGroundEffect.end(); ++iEffect)
			{
				MEffect* pEffect = iEffect->second;
				
				if(pEffect->GetX() != ix || pEffect->GetY() != iy) continue;
				
				TYPE_EFFECTSPRITETYPE		effectSpriteType	= EFFECTSPRITETYPE_MERCY_GROUND_5;
				TYPE_FRAMEID				frameID = (*g_pEffectSpriteTypeTable)[effectSpriteType].FrameID;
				
				if( pEffect->GetFrameID() != frameID ) continue;
				
				delete pEffect;
				rtn = true;
				m_mapGroundEffect.erase(iEffect);
				break;
			}
		}

	return rtn;
}
*/


//----------------------------------------------------------------------
// Remove Tile Effect
//----------------------------------------------------------------------
// tile�� �ִ� effect�� �����.
//----------------------------------------------------------------------
bool		
MZone::RemoveTileEffect(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, TYPE_EFFECTSPRITETYPE type, int serverID)
{
	if (type==EFFECTSPRITETYPE_NULL)
	{
		DEBUG_ADD("[Error]EffectSpriteType of EffectStatus is NULL");

		return false;
	}

	TYPE_FRAMEID	frameID = (*g_pEffectSpriteTypeTable)[type].FrameID;

	if (frameID==FRAMEID_NULL)
	{
		DEBUG_ADD("[Error]FrameID of EffectSpriteType is NULL");

		return false;
	}

	MSector& sector = m_ppSector[sY][sX];

	//--------------------------------------------------
	// effect�� �ִ� sector���� Ȯ��
	//--------------------------------------------------
	if (sector.IsExistEffect())
	{
		int num = sector.GetEffectSize();

		bool isRemoveDarknessEffect = (frameID>=EFFECTSPRITETYPE_DARKNESS_1_1
								&& frameID<=EFFECTSPRITETYPE_DARKNESS_3_5) ||
								(frameID>=EFFECTSPRITETYPE_GRAY_DARKNESS_1_1 
								&& frameID <= EFFECTSPRITETYPE_GRAY_DARKNESS_3_5 
//								||	frameID >= EFFECTSPRITETYPE_MAP_BLACK_LARGE_SMOKE &&
//								frameID <= EFFECTSPRITETYPE_MAP_BLACK_SMALL_SMOKE_3
								);

		EFFECT_LIST::const_iterator iEffect = sector.GetEffectIterator();

		for (int i=0; i<num; i++)
		{
			MEffect* pEffect = *iEffect;

			//--------------------------------------------------
			// sector�� �����ϴ� effect�� üũ�ؼ� �����.
			//--------------------------------------------------
			if (pEffect->GetEffectType() == MEffect::EFFECT_SECTOR)
			{
				bool isExistDarknessEffect = ((pEffect->GetFrameID()>=EFFECTSPRITETYPE_DARKNESS_1_1
											&& pEffect->GetFrameID()<=EFFECTSPRITETYPE_DARKNESS_3_5) ||
											pEffect->GetFrameID() >= EFFECTSPRITETYPE_GRAY_DARKNESS_1_1 &&
											pEffect->GetFrameID() <= EFFECTSPRITETYPE_GRAY_DARKNESS_3_5 
//											||	pEffect->GetFrameID() <= EFFECTSPRITETYPE_MAP_BLACK_LARGE_SMOKE &&
//											pEffect->GetFrameID() >= EFFECTSPRITETYPE_MAP_BLACK_SMALL_SMOKE_3;
											);
				//--------------------------------------------------
				// ���� effect type�� ���
				//--------------------------------------------------
				if (pEffect->GetFrameID() == frameID
					// Darkness�� ���... �ϵ��ڵ�.. - -;;
					|| isRemoveDarknessEffect
					&& isExistDarknessEffect					
					)
				{
					// sector���� ���� [�����9]
					RemoveSectorEffect( sX, sY, pEffect->GetID() );

					
					//--------------------------------------------------
					// zone�� list������ üũ�ؼ� �����.
					//--------------------------------------------------
					// delete
					EFFECT_MAP::iterator iZoneEffect = m_mapEffect.begin();

					BOOL found = FALSE;
					while (iZoneEffect != m_mapEffect.end())
					{
						MEffect* pZoneEffect = iZoneEffect->second;

						// ���� Effect�̸�..
						if (pZoneEffect->GetID()==pEffect->GetID())
						{
							m_mapEffect.erase( iZoneEffect );

							found = TRUE;

							break;
						}

						iZoneEffect++;
					}

					if (!found)
					{
						EFFECT_MAP::iterator iGroundEffect = m_mapGroundEffect.begin();

						BOOL found = FALSE;
						while (iGroundEffect != m_mapGroundEffect.end())
						{
							MEffect* pGroundEffect = iGroundEffect->second;

							// ���� Effect�̸�..
							if (pGroundEffect->GetID()==pEffect->GetID())
							{
								m_mapGroundEffect.erase( iGroundEffect );

								found = TRUE;

								break;
							}

							iGroundEffect++;
						}
					}

					// ��ũ�Ͻ��� ��� �� Ÿ�Ͽ� �ִ� ĳ���͸� �����ش�.
					if (isRemoveDarknessEffect)
					{
						CheckCreatureInDarkness( sector, sX, sY );
					}

					delete pEffect;
					
					return true;
				}
			}

			iEffect++;
		}
	}

	DEBUG_ADD("EffectSpriteType of EffectStatus is NULL");
	
	return false;
}


//----------------------------------------------------------------------
// Change Frame Effect
//----------------------------------------------------------------------
// �ӽ� �Լ�  :  ��� Effect�� Frame�� ��ȭ��Ų��.
//----------------------------------------------------------------------
void
MZone::UpdateEffects()
{
	EFFECT_MAP::iterator iEffect = m_mapEffect.begin();

	EFFECT_MAP::iterator iTemp;
	
	MEffect* pEffect;
	int count = m_mapEffect.size();


	int		sX, sY;		// ���� ���� ��ǥ
	int		nX, nY;		// ���� ���� ��ǥ
	int		light;
	int		id;
	
//	bool bChangeSight;

	for (int i=0; i<count; i++)	
	{
		pEffect = iEffect->second;

		if(pEffect->IsDelayFrame() == true)
		{
			iEffect++;
			continue;
		}

		// ���� ��ǥ ���
		id		= pEffect->GetID();
		sX		= pEffect->GetX();
		sY		= pEffect->GetY();
		light	= pEffect->GetLight();

		// �þ� �ٲ�� �ϴ°�?
//		bChangeSight = pEffect->GetBltType()==BLT_EFFECT;

		BOOL sXYinZone = (sX>=0 && sX<m_Width && sY>=0 && sY<m_Height);
		
		//---------------------------------------
		//
		// ����� ������ ���
		//
		//---------------------------------------
		if (pEffect->Update())
		{
			nX = pEffect->GetX();
			nY = pEffect->GetY();

			BOOL nXYinZone = (nX>=0 && nX<m_Width && nY>=0 && nY<m_Height);

			//---------------------------------------
			// chase effect�� �ƴϰ�..
			// Zone�� ��踦 �Ѿ ���.. --> �����Ѵ�.
			//---------------------------------------
			if (pEffect->GetEffectType()!=MEffect::EFFECT_CHASE
//				&& (pEffect->GetFrameID() <= EFFECTSPRITETYPE_MAP_BLACK_LARGE_SMOKE ||
//					pEffect->GetFrameID() >= EFFECTSPRITETYPE_MAP_BLACK_SMALL_SMOKE_3)
				&& !nXYinZone)
			{
				// �þ� ����
//				if (bChangeSight)
//				{
//					UnSetLight(sX, sY, light);
//				}

				if (sXYinZone)
				{
					// sector���� ���� [�����9]
					RemoveSectorEffect( sX, sY, id );
				}
										
				#ifdef OUTPUT_DEBUG_UPDATE_EFFECT
					DEBUG_ADD("delete pEffect0");
					// memory����							
					delete pEffect;			
					DEBUG_ADD("delete OK");
				#else
					delete pEffect;
				#endif
				
				iTemp = iEffect;
				iEffect++;

				// list���� �����Ѵ�.
				m_mapEffect.erase(iTemp);	

				continue;
			}

			//-----------------------------------------------------------
			// Effect�� ������ ���� ó���� ���ش�.
			//-----------------------------------------------------------
			// Sector�� �����Ǿ� �ִ� Effect
			//-----------------------------------------------------------
			if (pEffect->GetEffectType() == MEffect::EFFECT_SECTOR)
			{
				// ������ Effect�̰�, ���� ũ�Ⱑ �޶�������..
//				if (bChangeSight && light!=pEffect->GetLight())
//				{
//					UnSetLight(sX, sY, light);
//					SetLight(sX, sY, pEffect->GetLight());
//				}				
			}
			//-----------------------------------------------------------
			// �����̴� Effect
			//-----------------------------------------------------------
			else
			{
				// ���� Effect�� ��� --> Sector��ǥ�� ���� Sector�� üũ~�ؾ��Ѵ�.
				// ���� ��ǥ�� ���Ͽ� 
				//case MEffect::EFFECT_LINEAR :
				//case MEffect::EFFECT_PARABOLA :
				//case MEffect::EFFECT_GUIDANCE :
				//case MEffect::EFFECT_ATTACH :

				//------------------------------------------------------
				// ��ǥ�� �޶�������..
				//------------------------------------------------------
				if (sX!=nX || sY!=nY)
				{
					//------------------------------------------------------
					// Block�Ǵ� Sector�� ���� ��� �����Ѵ�.
					//------------------------------------------------------
					//
					// Block�� ������ ���� �ٸ��� ������??
					//
					// ���߸� ...
					if (0)
						// �������� block�� üũ���� �ʱ� ������
						// block�� �����Ѵ�.
						//
						//pEffect->GetEffectType()!=MEffect::EFFECT_CHASE
						//&& nXYinZone && m_ppSector[nY][nX].IsBlockFlying())						
					{
						// �þ� ����
//						if (bChangeSight)
//						{
//							UnSetLight(sX, sY, light);
//						}

						// sector���� ����	
						if (sXYinZone)
						{
							// [�����9]
							RemoveSectorEffect(sX, sY, id);
						}
													
						// memory����							
						#ifdef OUTPUT_DEBUG_UPDATE_EFFECT
							DEBUG_ADD("delete pEffect1");
							// memory����							
							delete pEffect;			
							DEBUG_ADD("delete OK");
						#else
							delete pEffect;
						#endif
						
						iTemp = iEffect;
						iEffect++;

						// list���� �����Ѵ�.
						m_mapEffect.erase(iTemp);	

						continue;
					}

					// �þ� �ٽ� ����
//					if (bChangeSight)
//					{
//						UnSetLight(sX, sY, light);
//						SetLight(pEffect->GetX(), pEffect->GetY(), pEffect->GetLight());
//					}

					// ���� ��ǥ���� ���� 
					if (sXYinZone)
					{
						// [�����9]
						RemoveSectorEffect(sX, sY, id);
					}

					//���ο� ��ǥ�� �߰��Ѵ�.
					if (nXYinZone)
					{
						m_ppSector[nY][nX].AddEffect( pEffect );
					}
				}		
				//------------------------------------------------------
				// ���� ũ��(�þ�)�� �ٲ������..
				//------------------------------------------------------
				else if (light != pEffect->GetLight())
				{
					// �þ� �ٽ� ����
//					if (bChangeSight)
//					{
//						UnSetLight(sX, sY, light);
//						SetLight(sX, sY, pEffect->GetLight());
//					}
				}
			}

			//-----------------------------------------------
			//
			// �� Effect�� ������ ���� LinkCount�� ���ؼ�
			// ���� ����Ǵ� Effect�� ������ �����ؾ� �Ѵ�.
			//
			// ����Frame�� EndLinkFrame�� �Ѿ ���
			//
			//-----------------------------------------------
			if (g_CurrentFrame >= pEffect->GetEndLinkFrame()
				&& pEffect->GetLinkSize() != 0)
			{
				// GenerateNext���� 
				// pEffect�� EffectTarget�� NULL�� ������ֱ� ������
				// ���⼭ ���� �ʿ� ����.
				#ifdef OUTPUT_DEBUG_UPDATE_EFFECT
					DEBUG_ADD("GenerateNext1");
					g_pEffectGeneratorTable->GenerateNext( pEffect );
					DEBUG_ADD("GenerateNext1 OK");
				#else
					g_pEffectGeneratorTable->GenerateNext( pEffect );
				#endif

				// pEffect�� ������ �����ؾ� �ϹǷ� ����� �ȵȴ�.
			}
			
			
			iEffect++;
		}
		//---------------------------------------
		//
		// �� ������ ��� = �����ؾ� �ϴ� ��� 
		//
		//---------------------------------------
		else
		{
			//-----------------------------------------------
			//
			//     Effect ����
			//
			//-----------------------------------------------
			// �þ� ����			
			//if (bChangeSight)
			//{
				//UnSetLight(sX, sY, light);
			//}			
			//-----------------------------------------------
			//
			// ���� ����Ǵ� Effect�� ������ �����ؾ� �Ѵ�.
			//
			//-----------------------------------------------
			if (pEffect->GetLinkSize() != 0)
			{
				#ifdef OUTPUT_DEBUG_UPDATE_EFFECT
					DEBUG_ADD("GenerateNext2");
					g_pEffectGeneratorTable->GenerateNext( pEffect );
					DEBUG_ADD("GenerateNext2 OK");
				#else
					g_pEffectGeneratorTable->GenerateNext( pEffect );
				#endif
			}		
			
			// sector���� ����	
			if (sXYinZone)
			{
				MSector& sector = m_ppSector[sY][sX];
				
				// [�����9]
				RemoveSectorEffect( sX, sY, id );

				CheckCreatureInDarkness( sector, sX, sY );				
			}
							
			#ifdef OUTPUT_DEBUG_UPDATE_EFFECT
				DEBUG_ADD("delete pEffect2");
				// memory����							
				delete pEffect;			
				DEBUG_ADD("delete OK");
			#else
				delete pEffect;
			#endif

			iTemp = iEffect;
			iEffect++;

			// list���� �����Ѵ�.
			m_mapEffect.erase(iTemp);	
		}		
	}	
}



//----------------------------------------------------------------------
// Zone�� Effect �߰�
//----------------------------------------------------------------------
// m_mapGroundEffect�� �߰��Ѵ�.
// GroundEffect�� pixel��ǥ���� �����Ƿ� 
// Sector��ǥ�� üũ�� �ʿ䰡 ����.
//----------------------------------------------------------------------
bool		
MZone::AddGroundEffect(MEffect* pEffect)
{	
	TYPE_OBJECTID id = pEffect->GetID();
	BOOL	AddOK = TRUE;
	
	if( m_mapGroundEffect.empty() )
		AddOK = TRUE;
	else
	{
		EFFECT_MAP::iterator Itr = m_mapGroundEffect.begin();
		EFFECT_MAP::iterator endItr = m_mapGroundEffect.end();
		
		int NewEffectX = pEffect->GetX();
		int NewEffectY = pEffect->GetY();
		
		while( Itr != endItr )
		{
			MEffect *pOldEffect = Itr->second;			
			
			if(NewEffectX == pOldEffect->GetX() && NewEffectY == pOldEffect->GetY())
			{
				int est1 = g_pTopView->GetEffectSpriteType( (BLT_TYPE) pEffect->GetBltType(), pEffect->GetFrameID() );
				int est2 = g_pTopView->GetEffectSpriteType( (BLT_TYPE) pOldEffect->GetBltType(), pOldEffect->GetFrameID() );

				if( est1 == EFFECTSPRITETYPE_SUMMON_GROUND_ELEMENTAL_END && est2 == EFFECTSPRITETYPE_SUMMON_GROUND_ELEMENTAL_ING)
				{
					pOldEffect->SetCount( 0 );
				}

				if( est1 == EFFECTSPRITETYPE_MAGIC_ELUSION_ING && est2 == EFFECTSPRITETYPE_MAGIC_ELUSION_ING ||
					est1 >= EFFECTSPRITETYPE_MERCY_GROUND_1 && est1 <= EFFECTSPRITETYPE_MERCY_GROUND_9 &&
					est1 == est2)
				{
					if( pEffect->GetEndFrame() > pOldEffect->GetEndFrame() )
					{
						int count = pEffect->GetEndFrame() - g_CurrentFrame;
						pOldEffect->SetCount( count );
					}
					AddOK = FALSE;
				}
				else if ( est2 == EFFECTSPRITETYPE_REGEN_TOWER_GROUND && 
					(est1 == EFFECTSPRITETYPE_ICE_FIELD_2 ||
					est1 == EFFECTSPRITETYPE_ICE_FIELD_1 ||
					est1 == EFFECTSPRITETYPE_ICE_FIELD_3
					)
					)
				{
					AddOK = FALSE;
				}
			}
			if(AddOK == FALSE)
				break;			
			Itr++;
		}
	}
	if(AddOK == TRUE)
	{
		m_mapGroundEffect[id] = pEffect;
	}		
		/*
		//---------------------------------------------------------
		// ���ð����� Effect�� sector��ǥ�� �ٲ��� �ʴ´ٰ� ����.
		//---------------------------------------------------------
		if (pEffect->IsSelectable())
		{	
			int x = pEffect->GetX();
			int y = pEffect->GetY();

			//---------------------------------------------------------
			// Zone�� ��踦 ���� �ʴ� ��츸 Sector�� �߰��Ѵ�.
			//---------------------------------------------------------
			if (x<0 || x>=m_Width
				|| y<0 || y>=m_Height)
			{		
			}
			else
			{
				m_ppSector[y][x].AddEffect( pEffect );
			}
		}
		*/

	return true;
//	return false;
}

//----------------------------------------------------------------------
// Get GroundEffect
//----------------------------------------------------------------------
// A ground effect drawn with this frame within radius tiles of (sX,sY)? Dracula Castle hides the 2F portal
// tooltip while the Mihnea door (EST 2282, a server tile effect) stands on the stairs.
bool
MZone::IsGroundEffectNear(int sX, int sY, TYPE_FRAMEID frameID, int radius) const
{
	for (EFFECT_MAP::const_iterator i = m_mapGroundEffect.begin(); i != m_mapGroundEffect.end(); ++i)
	{
		MEffect* pEffect = i->second;
		if (pEffect == NULL || pEffect->GetFrameID() != frameID) continue;
		int dx = pEffect->GetX() - sX, dy = pEffect->GetY() - sY;
		if (dx <= radius && -dx <= radius && dy <= radius && -dy <= radius)
			return true;
	}
	// generic tile effects (GCAddEffectToTile -> action node) sit in the sectors' own lists
	for (int y = sY - radius; y <= sY + radius; y++)
	{
		for (int x = sX - radius; x <= sX + radius; x++)
		{
			if (x < 0 || y < 0 || x >= GetWidth() || y >= GetHeight()) continue;
			const MSector& sector = GetSector(x, y);
			EFFECT_LIST::const_iterator it = sector.GetEffectIterator();
			for (int n = sector.GetEffectSize(); n > 0; --n, ++it)
				if (*it != NULL && (*it)->GetFrameID() == frameID) return true;
		}
	}
	return false;
}

MEffect*	
MZone::GetGroundEffect(TYPE_OBJECTID id) const
{
	EFFECT_MAP::const_iterator iEffect = m_mapGroundEffect.find( id );

	if (iEffect==m_mapGroundEffect.end())
	{
		return NULL;
	}

	return iEffect->second;
}

//----------------------------------------------------------------------
// Remove GroundEffect ( id )
//----------------------------------------------------------------------
// effect�� id�� ���� �ǹ̰� ������..
// �����ζ� �������ϴ� ��찡 �־��.. - -;
//----------------------------------------------------------------------
bool		
MZone::RemoveGroundEffect(TYPE_OBJECTID id)
{
	EFFECT_MAP::iterator iEffect = m_mapGroundEffect.find( id );

	if (iEffect==m_mapGroundEffect.end())
	{
		return false;
	}

	MEffect* pEffect = iEffect->second;

	/*
	//---------------------------------------------------------
	// ���ð����� Effect�� sector��ǥ�� �ٲ��� �ʴ´ٰ� ����.
	//---------------------------------------------------------
	if (pEffect->IsSelectable())
	{
		int x = pEffect->GetX();
		int y = pEffect->GetY();

		//---------------------------------------------------------
		// Zone�� ��踦 ���� �ʴ� ��츸 Sector�� �߰��Ѵ�.
		//---------------------------------------------------------
		if (x<0 || x>=m_Width
			|| y<0 || y>=m_Height)
		{		
		}
		else
		{
			m_ppSector[y][x].RemoveEffect( pEffect->GetID() );
		}
	}
	*/

	delete pEffect;

	m_mapGroundEffect.erase( iEffect );

	return true;
}


//----------------------------------------------------------------------
// Remove GroundEffect ( id )
//----------------------------------------------------------------------
// frameID�� GroundEffect�� �����Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::RemoveGroundEffectByActionInfo(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, TYPE_ACTIONINFO actionInfoIdx)
{
	MActionInfo&	actionInfo	= (*g_pActionInfoTable)[actionInfoIdx];
	int				size		= actionInfo.GetSize();

	if(size == 0) return false;

	bool rtn = false;

	EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();

	while(iEffect != m_mapGroundEffect.end())
	{
		bool bNext = false;

		MEffect* pEffect = iEffect->second;

		if(pEffect->GetX() != sX || pEffect->GetY() != sY) {
			++iEffect;
			continue;
		}
		
		for(int i = 0; i < size; ++i)
		{
			TYPE_EFFECTSPRITETYPE		effectSpriteType	= actionInfo[i].EffectSpriteType;
			TYPE_FRAMEID				frameID = (*g_pEffectSpriteTypeTable)[effectSpriteType].FrameID;
			EFFECTSPRITETYPETABLE_INFO&	effectSpriteTypeTable = (*g_pEffectSpriteTypeTable)[effectSpriteType];

			if( pEffect->GetFrameID() != frameID ) continue;

			delete pEffect;

			EFFECT_MAP::iterator iEffectBackup	= iEffect++;
			m_mapGroundEffect.erase( iEffectBackup );
			rtn		= true;
			bNext	= true;
			break;
		}
	
		if(!bNext) ++iEffect;
	}


	return rtn;
}


//----------------------------------------------------------------------
// Update GroundEffects
//----------------------------------------------------------------------
// �ӽ� �Լ�  :  ��� GroundEffect�� Frame�� ��ȭ��Ų��.
//----------------------------------------------------------------------
void
MZone::UpdateGroundEffects()
{
	EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();

	EFFECT_MAP::iterator iTemp;
	
	MEffect* pEffect;
	int count = m_mapGroundEffect.size();


	int		light;
	int		id;
	
	for (int i=0; i<count; i++)	
	{
		pEffect = iEffect->second;

		// ���� ��ǥ ���
		id		= pEffect->GetID();
		light	= pEffect->GetLight();

		//---------------------------------------
		//
		// ����� ������ ���
		//
		//---------------------------------------
		if (pEffect->Update())
		{
			//-----------------------------------------------
			//
			// �� Effect�� ������ ���� LinkCount�� ���ؼ�
			// ���� ����Ǵ� Effect�� ������ �����ؾ� �Ѵ�.
			//
			// ����Frame�� EndLinkFrame�� �Ѿ ���
			//
			//-----------------------------------------------
			if (g_CurrentFrame >= pEffect->GetEndLinkFrame()
				&& pEffect->GetLinkSize() != 0)
			{
				// GenerateNext���� 
				// pEffect�� EffectTarget�� NULL�� ������ֱ� ������
				// ���⼭ ���� �ʿ� ����.
				g_pEffectGeneratorTable->GenerateNext( pEffect );

				// pEffect�� ������ �����ؾ� �ϹǷ� ����� �ȵȴ�.
			}

			// ����..	
			iEffect++;
		}
		//---------------------------------------
		//
		// �� ������ ��� = �����ؾ� �ϴ� ��� 
		//
		//---------------------------------------
		else
		{
			//-----------------------------------------------
			//
			// ���� ����Ǵ� Effect�� ������ �����ؾ� �Ѵ�.
			//
			//-----------------------------------------------
			if (pEffect->GetLinkSize() != 0)
			{
				g_pEffectGeneratorTable->GenerateNext( pEffect );
			}				
			
			//---------------------------------------------------------
			// ���ð����� Effect�� sector��ǥ�� �ٲ��� �ʴ´ٰ� ����.
			//---------------------------------------------------------
			/*
			if (pEffect->IsSelectable())
			{
				int x = pEffect->GetX();
				int y = pEffect->GetY();

				//---------------------------------------------------------
				// Zone�� ��踦 ���� �ʴ� ��츸 Sector���� ����
				//---------------------------------------------------------
				if (x<0 || x>=m_Width
					|| y<0 || y>=m_Height)
				{		
				}
				else
				{
					m_ppSector[y][x].RemoveEffect( pEffect->GetID() );
				}
			}
			*/
												
			// memory����							
			delete pEffect;			

			iTemp = iEffect;
			iEffect++;

			// list���� �����Ѵ�.
			m_mapGroundEffect.erase(iTemp);	
		}		
	}	
}

//----------------------------------------------------------------------
// Get Near SpriteSet
//----------------------------------------------------------------------
// (sX,sY) ��ó�� Tile�� ImageObject�� SpriteSet�� �˻��ؼ� �Ѱ��ش�.
//----------------------------------------------------------------------
void						
MZone::GetNearSpriteSet(CSpriteSetManager& TileSSM, CSpriteSetManager& ImageObjectSSM, TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY) const
{
	//----------------------------------------------------------------------
	//
	//                         ImageObject �˻�
	//
	//----------------------------------------------------------------------	
	POINT firstSector;
	firstSector.x = sX + SECTOR_SKIP_PLAYER_LEFT*3;
	firstSector.y = sY + SECTOR_SKIP_PLAYER_UP*3;
	int sX1 = firstSector.x + SECTOR_SKIP_LEFT*3;
	int sY1 = firstSector.y + SECTOR_SKIP_UP*3;
	int sX2 = firstSector.x + SECTOR_WIDTH*3;
	int sY2 = firstSector.y + SECTOR_HEIGHT*3;
	
	//------------------------------------------------------
	// Zone�� ������ �ƴ� ��쿡 Skip...
	//------------------------------------------------------
	if (sX1 < 0) 
	{					
		//tilePoint.x += -sX1 * TILE_X;
		sX1 = 0;	
	}

	if (sX2 >= m_Width)
	{
		sX2 = m_Width-1;
	}

	if (sY1 < 0)
	{
		//tilePoint.y += -sY1 * TILE_Y;
		sY1 = 0;	
	}

	if (sY2 >= m_Height)
	{
		sY2 = m_Height-1;
	}
		
	DEBUG_ADD_FORMAT("GetNearSpriteSet: (%d, %d) ~ (%d, %d)", sX1, sY1, sX2, sY2);

	//------------------------------------------------------
	// �� Sector�� ImageObject�˻�
	//------------------------------------------------------
	int y,x,i;

	int maxTile = g_pTopView->m_TileSPK.GetSize();

	for (y=sY1; y<=sY2; y++)
	{				
		for (x=sX1; x<=sX2; x++)
		{				
			const MSector& sector = m_ppSector[y][x];

			//--------------------------------------------
			// Tile SpriteID�� �����Ѵ�.
			//--------------------------------------------
			int spriteID = sector.GetSpriteID();

			if (spriteID < maxTile)
			{
				TileSSM.Add( spriteID );
			}

			//--------------------------------------------
			// ImageObject�� �ִٸ�.. 
			//--------------------------------------------
			if (sector.IsExistImageObject())
			{
				OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();

				//--------------------------------------------				
				// Sector�� �ִ� ��� ImageObject���� �˻��Ѵ�.
				// �� ImageObject�� SpriteID�� ��� �߰��Ѵ�.
				//--------------------------------------------
				for (i=0; i<sector.GetImageObjectSize(); i++)
				{
					MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);
					
					if (pImageObject->GetSpriteID() != SPRITEID_NULL)
					{
						ImageObjectSSM.Add( pImageObject->GetSpriteID() );
					}
					
					// Frame�� �ִ� ���... Frame�� SpriteID�� �˻� 
					if (pImageObject->IsAnimation())
					{
						DEBUG_ADD_FORMAT("AnimationObject: (%d, %d)",x, y);

						int fid = ((MAnimationObject*)pImageObject)->GetFrameID();

						if(fid < g_pTopView->m_ImageObjectFPK.GetSize())
						{
							FRAME_ARRAY* pFrameArray = &(g_pTopView->m_ImageObjectFPK[ fid ]);
							
							// �� Frame���� SpriteID�� �˻��Ѵ�.
							for (int j=0; j<pFrameArray->GetSize(); j++)
							{
								if ((*pFrameArray)[j].GetSpriteID() != SPRITEID_NULL)
								{
									ImageObjectSSM.Add( (*pFrameArray)[j].GetSpriteID() );
								}
							}
						}
					}
					
					
					iImageObject++;
				}
			}
		}
	}
}


//----------------------------------------------------------------------
// Get Near Order All SpriteSet
//----------------------------------------------------------------------
// (sX,sY) ���� ������ ���� ������� Tile�� ImageObject�� SpriteSet�� �˻��ؼ� �Ѱ��ش�.
//----------------------------------------------------------------------
void						
MZone::GetNearOrderAllSpriteSet(CSpriteSetManager& TileSSM, CSpriteSetManager& ImageObjectSSM,
								TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY,
								int NearIgnoreCX, int NearIgnoreCY) const
{
	//----------------------------------------------------------------------
	//
	//                         ImageObject �˻�
	//
	//----------------------------------------------------------------------	
	POINT direction[4] = {
		{0, -1}, {1, 0}, {0, 1}, {-1, 0},
	};
	
	int maxTile			= g_pTopView->m_TileSPK.GetSize();
	int maxImageObject	= g_pTopView->m_ImageObjectSPK.GetSize();

	int maxLength		= max(m_Width, m_Height);

	int findX		= sX;
	int findY		= sY;
	int findDir	= 0;
	int findLength = 1;
	bool bIncreaseLength	= false;

	while(findLength < maxLength)
	{
		for(int i = 0; i < findLength; ++i)
		{
			//------------------------------------------------------
			// Zone�� ������ �ƴ� ��쿡 Skip...
			//------------------------------------------------------
			if (findX >= 0 && findX < m_Width &&
				findY >= 0 && findY < m_Height &&
				(abs(findX - sX) >= NearIgnoreCX ||
				 abs(findY - sY) >= NearIgnoreCY))
			{
#ifdef OUTPUT_DEBUG
				char szBuf[256];
				sprintf(szBuf, "GetNearOrderAllSpriteSet : (%d, %d)\n", findX, findY);
				OutputDebugString(szBuf);
#endif	
				
				const MSector& sector = m_ppSector[findY][findX];
				
				//--------------------------------------------
				// Tile SpriteID�� �����Ѵ�.
				//--------------------------------------------
				int spriteID = sector.GetSpriteID();
				
				if (spriteID < maxTile)
				{
					TileSSM.Add( spriteID );
				}
				
				//--------------------------------------------
				// ImageObject�� �ִٸ�.. 
				//--------------------------------------------
				if (sector.IsExistImageObject())
				{
					OBJECT_MAP::const_iterator iImageObject = sector.GetImageObjectIterator();
					
					//--------------------------------------------				
					// Sector�� �ִ� ��� ImageObject���� �˻��Ѵ�.
					// �� ImageObject�� SpriteID�� ��� �߰��Ѵ�.
					//--------------------------------------------
					for (int j = 0; j < sector.GetImageObjectSize(); j++)
					{
						MImageObject* const pImageObject = (MImageObject* const)((*iImageObject).second);
						
						if (pImageObject->GetSpriteID() != SPRITEID_NULL)
						{
							ImageObjectSSM.Add( pImageObject->GetSpriteID() );
						}
						
						// Frame�� �ִ� ���... Frame�� SpriteID�� �˻� 
						if (pImageObject->IsAnimation())
						{
							//						DEBUG_ADD_FORMAT("AnimationObject: (%d, %d)",x, y);
							
							int fid = ((MAnimationObject*)pImageObject)->GetFrameID();
							
							if(fid < g_pTopView->m_ImageObjectFPK.GetSize())
							{
								FRAME_ARRAY* pFrameArray = &(g_pTopView->m_ImageObjectFPK[ fid ]);
								
								// �� Frame���� SpriteID�� �˻��Ѵ�.
								for (int k = 0; k < pFrameArray->GetSize(); k++)
								{
									if ((*pFrameArray)[k].GetSpriteID() != SPRITEID_NULL)
									{
										ImageObjectSSM.Add( (*pFrameArray)[k].GetSpriteID() );
									}
								}
							}
						}
						
						iImageObject++;
					}
					
				} // if (sector.IsExistImageObject())

			}

			findX += direction[findDir].x;
			findY += direction[findDir].y;

		} // for(register int i = 0; i < findLength; ++i)

		if(bIncreaseLength) ++findLength;
		bIncreaseLength = !bIncreaseLength;

		if(++findDir == 4) findDir = 0;

	} // while(findLength < maxLength)
}


//----------------------------------------------------------------------
// Add Sound
//----------------------------------------------------------------------
// ��� ������ ����� Sound�� �����صд�.
//----------------------------------------------------------------------
void
MZone::AddSound(SOUND_NODE* pNode)
{
	m_listSoundNode.push_back( pNode );
}

//----------------------------------------------------------------------
// Update Sound
//----------------------------------------------------------------------
// ��� ������ ����� Sound�� üũ�ؼ� �Ҹ��� ����.
//----------------------------------------------------------------------
void
MZone::UpdateSound()
{
	SOUND_NODE_LIST::iterator iSound = m_listSoundNode.begin();

	while (iSound != m_listSoundNode.end())
	{
		SOUND_NODE*	pNode = *iSound;

		//-------------------------------------------------
		// �Ҹ��� ����� �ð��� ��������.. PlaySound
		//-------------------------------------------------
		if (pNode->GetPlayTime() < g_CurrentTime)
		{
			// �ѹ��� �Ҹ��� ����.
			PlaySound( pNode->GetSoundID(), false, pNode->GetX(), pNode->GetY() );

			// �����.
			delete pNode;

			// ��� ����ص״ٰ� �����.
			SOUND_NODE_LIST::iterator iTemp = iSound;
			iSound++;
			m_listSoundNode.erase( iTemp );
		}
		//-------------------------------------------------
		// �ƴ� ���.. ���� �Ҹ� üũ..
		//-------------------------------------------------
		else
		{
			iSound++;
		}	
	}

	//----------------------------------------------------------------
	// Zone���� random���� ���� �Ҹ�
	//----------------------------------------------------------------
	//----------------------------------------------------------------
	// ����� �Ҹ�..
	// �ϵ��ڵ�.. ������.. ���߿� ���ߵȴ�.
	//----------------------------------------------------------------
	int zoneID = (g_bZonePlayerInLarge ? g_nZoneLarge : g_nZoneSmall);
			
	if (zoneID==2106 || zoneID==2004 || zoneID==2014 || zoneID==2024)
	{
		if (!g_bPlayPropeller)// && g_CurrentTime > g_ZoneRandomSoundTime)
		{
			int x, y;
			x = g_pPlayer->GetX();// + ((rand()%2)? 1 : -1) * (rand()%7 + 4);
			y = g_pPlayer->GetY();// + ((rand()%2)? 1 : -1) * (rand()%6 + 2);

			PlaySound( SOUND_WORLD_PROPELLER, true, x, y ); /* true, x, y */
			g_bPlayPropeller = TRUE;

			//g_ZoneRandomSoundTime = 0x0FFFFFFF;		// -_-;;
		}
	}
	else
	{
		if (g_bPlayPropeller)
		{
			StopSound( SOUND_WORLD_PROPELLER );

			g_bPlayPropeller = FALSE;
		}

		if (g_CurrentTime > g_ZoneRandomSoundTime)
		{
			ZONETABLE_INFO* pZoneInfo = g_pZoneTable->Get( zoneID );

			if (pZoneInfo!=NULL)
			{
				int soundID = pZoneInfo->GetRandomSoundID();			

				int x, y;

				x = g_pPlayer->GetX() + ((rand()%2)? 1 : -1) * (rand()%15 + 13);
				y = g_pPlayer->GetY() + ((rand()%2)? 1 : -1) * (rand()%12 + 10);

				PlaySound( soundID, false, x, y );
			}
			
			// 10~30�� �Ŀ� �ٽ� �Ҹ� ����
			g_ZoneRandomSoundTime = g_CurrentTime + ((rand()%10)+6)*1000;			
		}
	}
}

//----------------------------------------------------------------------
// Update Item
//----------------------------------------------------------------------
// Item�� ��ü�� ���.. - -;
//----------------------------------------------------------------------
void
MZone::UpdateItem()
{
	ITEM_MAP::iterator iItem = m_mapItem.begin();

	while (iItem!= m_mapItem.end())
	{
		MItem*	pItem = (*iItem).second;

		//----------------------------------------------------------------------
		// ��ü�� ���.. ��ü�� �پ��ִ� Effect�� ó�����ش�.
		//----------------------------------------------------------------------
		if (pItem->GetItemClass()==ITEM_CLASS_CORPSE)
		{
			MCorpse* pCorpse = (MCorpse*)pItem;

			MCreature* pCreature = pCorpse->GetCreature();

			if (pCreature!=NULL)
			{
				if (pCreature->GetNextAction()!=ACTION_STAND
					|| pCreature->GetActionCount()<pCreature->GetActionCountMax()
					|| pCreature->GetCreatureType() == 672 )
				{
					pCreature->Action();
				}
				pCreature->UpdateAttachEffect();
			}
		}

		//----------------------------------------------------------------------
		// �������� �ִ� Item�� ���..
		//----------------------------------------------------------------------
		else if (pItem->IsDropping())
		{
			pItem->NextDropFrame();

			//--------------------------------------------------------
			// �� ������ ���
			//--------------------------------------------------------
			if (!pItem->IsDropping())
			{
				PlaySound( pItem->GetTileSoundID(),
							false,
							g_pPlayer->GetX(), g_pPlayer->GetY());
			}
		}
		
		iItem++;		
	}

}

//------------------------------------------------------------------
// New FakeCreature
//------------------------------------------------------------------
// (x,y)�� pCreature�� FakeCreature�� �����Ѵ�.
//------------------------------------------------------------------
MFakeCreature*
MZone::NewFakeCreature(int creatureType, int x, int y, int dir)
{
	//------------------------------------------------------
	// Fake Creature ����
	//------------------------------------------------------
	MFakeCreature* pFakeCreature = new MFakeCreature;
	
	pFakeCreature->SetZone( this );
	
#ifdef OUTPUT_DEBUG
	char name[20];
	sprintf(name, "Fake%d", pFakeCreature->GetID());
	pFakeCreature->SetName( name );
#endif
	
	//pFakeCreature->SetID( 0 );  �ڵ� �߱� �ȴ�.
//	pFakeCreature->SetSameBody( pCreature );
	
//	if (pCreature->IsWear())
//	{
//		pFakeCreature->SetSameWear( (MCreatureWear*)pCreature );
//	}	

	pFakeCreature->SetCreatureType(creatureType);
	pFakeCreature->SetPosition( x, y );
	pFakeCreature->SetServerPosition( x, y );
	
	pFakeCreature->SetDirection( dir );
	pFakeCreature->SetCurrentDirection( dir );
	pFakeCreature->SetAction( ACTION_STAND );
	
	return pFakeCreature;
}

//------------------------------------------------------------------
// New FakeCreature
//------------------------------------------------------------------
// (x,y)�� pCreature�� FakeCreature�� �����Ѵ�.
//------------------------------------------------------------------
MFakeCreature*
MZone::NewFakeCreature(MCreature* pCreature, int x, int y)
{
	//------------------------------------------------------
	// Fake Creature ����
	//------------------------------------------------------
	MFakeCreature* pFakeCreature = new MFakeCreature;

	pFakeCreature->SetZone( this );
	
	#ifdef OUTPUT_DEBUG
		char name[20];
		sprintf(name, "Fake%d", pFakeCreature->GetID());
		pFakeCreature->SetName( name );
	#endif

	//pFakeCreature->SetID( 0 );  �ڵ� �߱� �ȴ�.
	pFakeCreature->SetSameBody( pCreature );

	if (pCreature->IsWear())
	{
		pFakeCreature->SetSameWear( (MCreatureWear*)pCreature );
	}	

	if (pCreature->IsAdvancementClass())
	{
		pFakeCreature->SetStatus(MODIFY_ADVANCEMENT_CLASS_LEVEL, pCreature->GetAdvancementClassLevel());
	}
	
	pFakeCreature->SetPosition( x, y );
	pFakeCreature->SetServerPosition( x, y );

	int dir = pCreature->GetDirection();

	pFakeCreature->SetDirection( dir );
	pFakeCreature->SetCurrentDirection( dir );
	pFakeCreature->SetAction( ACTION_STAND );
	pFakeCreature->SetWeaponSpeed( pCreature->GetWeaponSpeed() );
	pFakeCreature->SetBasicActionInfo( pCreature->GetBasicActionInfo() );
	pFakeCreature->SetSpecialActionInfo( pCreature->GetSpecialActionInfo() );

	return pFakeCreature;
}


//----------------------------------------------------------------------
// Zone�� FakeCreature �߰� 
//----------------------------------------------------------------------
// �ܺο��� new����� �Ѵ�.
// �̹� �ִ��� Ȯ���� �ϰ� ������ �߰��ؾ� �Ѵ�.
//----------------------------------------------------------------------
bool		
MZone::AddFakeCreature(MCreature* pCreature)
{
	DEBUG_ADD_FORMAT("MZone::AddFakeCreature id = %d", pCreature->GetID());

	CREATURE_MAP::iterator	theIterator;

	theIterator = m_mapFakeCreature.find(pCreature->GetID());
	
	// ���� ���� Creature�̸� �߰�	
	if (theIterator == m_mapFakeCreature.end())
	{
		m_mapFakeCreature.insert(CREATURE_MAP::value_type(pCreature->GetID(), pCreature));
		
		return true;
	}		

	// �̹� �ִ� Creature�̸�,
	DEBUG_ADD_FORMAT("Add Failed - Already Exist in Zone");

	return false;
}

	

//----------------------------------------------------------------------
// Zone���� FakeCreature ����
//----------------------------------------------------------------------
// ���ο��� delete���ش�.
//----------------------------------------------------------------------
bool
MZone::RemoveFakeCreature(TYPE_OBJECTID id)
{
	CREATURE_MAP::iterator	theIterator;

	// ID�� id�� Creature�� ã�´�.
	theIterator = m_mapFakeCreature.find(id);
    
	// �׷� id�� ���� Creature�� ����.	
	if (theIterator == m_mapFakeCreature.end())
		return false;


	MFakeCreature* pFakeCreature = (MFakeCreature *)theIterator->second;

	if(pFakeCreature != NULL)
	{
		// ���ΰ��
		if(pFakeCreature->GetOwnerID() != OBJECTID_NULL)
		{
			MCreature *pCreature = GetCreature(pFakeCreature->GetOwnerID());

			if(pCreature != NULL)
			{
				if(pCreature->GetPetID() == pFakeCreature->GetID())
					pCreature->SetPetID(OBJECTID_NULL);
				else if(pCreature->GetElementalID() == pFakeCreature->GetID())
					pCreature->SetElementalID(OBJECTID_NULL);
			}
		}

		delete pFakeCreature;
	}

	m_mapFakeCreature.erase(theIterator);

	return true;
}


//----------------------------------------------------------------------
// Zone�� Creature �о����
//----------------------------------------------------------------------
MCreature*	
MZone::GetFakeCreature(TYPE_OBJECTID id)
{
	CREATURE_MAP::iterator	theIterator;

	// ID�� id�� Creature�� ã�´�.
	theIterator = m_mapFakeCreature.find(id);

	// ���� ��� NULL�� return�Ѵ�.
	if (theIterator == m_mapFakeCreature.end()) 
	{
		return NULL;
	}

	// ������ �� Creature�� return�Ѵ�.
	return (*theIterator).second;
}

//----------------------------------------------------------------------
// Update FakeCreature
//----------------------------------------------------------------------
void		
MZone::UpdateFakeCreature()
{
	CREATURE_MAP::iterator	iCreature = m_mapFakeCreature.begin();

	MCreature* pCreature;
	
	//	DEBUG_ADD_FORMAT("[UpdateAllCreature] %d Creature(s)", m_mapCreature.size());
	
	//------------------------------------------------------
	// ��� Creature�� ���ؼ� Action
	//------------------------------------------------------
	while (iCreature != m_mapFakeCreature.end())
	{
		pCreature = iCreature->second;

		//------------------------------------------------------
		// MFakeCreature�� ��츸..
		//------------------------------------------------------
		if (pCreature->GetClassType()==MCreature::CLASS_FAKE)
		{
			MFakeCreature* pFakeCreature = (MFakeCreature*)pCreature;

			pFakeCreature->Action();
			pFakeCreature->UpdateAttachEffect();

			//------------------------------------------------------
			// FakeCreature�� ������� ���
			//------------------------------------------------------
			if (pFakeCreature->IsFakeEnd())
			{	
				CREATURE_MAP::iterator	iCreatureTemp = iCreature;
				iCreature++;

				m_mapFakeCreature.erase( iCreatureTemp );

				delete pFakeCreature;

				continue;
			}
		}

		iCreature++;
	}
}

//----------------------------------------------------------------------
// Add Helicopter
//----------------------------------------------------------------------
bool
MZone::AddHelicopter(TYPE_OBJECTID creatureID, int x0, int y0, int ExtendType)
{
	MHelicopter* pHelicopter = new MHelicopter;

	if (!pHelicopter->SetChaseCreature( creatureID, x0, y0, ExtendType )
		|| !m_HelicopterManager.AddHelicopter( pHelicopter ))
	{
		DEBUG_ADD("Can't Add Helicopter");

		delete pHelicopter;

		return false;
	}

	return true;
}

//----------------------------------------------------------------------
// Get Helicopter
//----------------------------------------------------------------------
MHelicopter*	
MZone::GetHelicopter(TYPE_OBJECTID creatureID) const
{
	return m_HelicopterManager.GetHelicopter(creatureID);
}

//----------------------------------------------------------------------
// Remove Helicopter
//----------------------------------------------------------------------
void		
MZone::RemoveHelicopter(TYPE_OBJECTID creatureID)
{
	m_HelicopterManager.RemoveHelicopter( creatureID );
}

//----------------------------------------------------------------------
// Change To HalluCreature
//----------------------------------------------------------------------
void		
MZone::ChangeToHalluCreature()
{
	CREATURE_MAP::iterator	iCreature = m_mapCreature.begin();

	MCreature* pCreature;

	DEBUG_ADD("Zone::ChangeToHalluCreature");
	
	//------------------------------------------------------
	// ��� Creature�� ���ؼ� Action
	//------------------------------------------------------
	while (iCreature != m_mapCreature.end())
	{
		pCreature = iCreature->second;

		// player�� �ƴ� ��쿡...
		if (pCreature!=g_pPlayer)
		{
			pCreature->SetHalluCreature( g_pTopView->GetRandomMonsterTypeInZone() );
		}

		iCreature ++;
	}

	DEBUG_ADD("Zone::ChangeToHalluCreature OK");

}

//----------------------------------------------------------------------
// Remove HalluCreature
//----------------------------------------------------------------------
void		
MZone::RemoveHalluCreature()
{
	CREATURE_MAP::iterator	iCreature = m_mapCreature.begin();

	MCreature* pCreature;

	DEBUG_ADD("Zone::RemoveHalluCreature");
	
	//------------------------------------------------------
	// ��� Creature�� ���ؼ� Action
	//------------------------------------------------------
	while (iCreature != m_mapCreature.end())
	{
		pCreature = iCreature->second;

		// player�� �ƴ� ��쿡...
		if (pCreature!=g_pPlayer)
		{
			pCreature->UnSetHalluCreature();
		}

		iCreature ++;
	}

	DEBUG_ADD("Zone::RemoveHalluCreature OK");
/*
	ITEM_MAP::iterator	iItem = m_mapItem.begin();

	MItem *pItem;

	while (iItem != m_mapItem.end())
	{
		pItem = iItem->second;

		if (pItem != NULL && pItem->GetItemClass()==ITEM_CLASS_CORPSE)
		{
			MCorpse* pCorpse = (MCorpse*)pItem;
			
			MCreature* pCreature = pCorpse->GetCreature();
			
			if (pCreature!=NULL && pCreature != g_pPlayer)
			{
				pCreature->UnSetHalluCreature();
			}
		}

		iItem++;
	}

	DEBUG_ADD("Zone::RemoveHalluCorpse OK");
*/
}

//----------------------------------------------------------------------
// Set Safe Sector
//----------------------------------------------------------------------
void						
MZone::SetSafeSector( const RECT& rect, BYTE fSafe )
{
	int top = rect.top;
	int left = rect.left;
	int bottom = rect.bottom;
	int right = rect.right;

	if (top < 0) top = 0;
	if (left < 0) left = 0;
	if (bottom >= m_Height) bottom = m_Height-1;
	if (right >= m_Width) right = m_Width-1;

	if (top > bottom) { int temp=top; top=bottom; bottom=temp; }
	if (left > right) { int temp=left; left=right; right=left; }

	for (int i=top; i<=bottom; i++)
	{
		for (int j=left; j<=right; j++)
		{
			m_ppSector[i][j].SetSafe( fSafe );
		}
	}
}

MItem *
MZone::GetCorpseKilledByMe(int limitItemCount)
{
	ITEM_MAP::iterator itr = m_mapItem.begin();
	ITEM_MAP::iterator endItr = m_mapItem.end();

	while(itr != endItr)
	{
		MItem* pItem = itr->second;

		if (pItem!=NULL
			&& pItem->GetItemClass()==ITEM_CLASS_CORPSE && pItem->GetNumber() > limitItemCount)
		{
			MCreature* pCreature = ((MCorpse*)pItem)->GetCreature();

			if (pCreature!=NULL
				&& pCreature->GetDrainCreatureID() == g_pPlayer->GetID())
			{
				return pItem;
			}
		}

		itr++;
	}

	return NULL;
}

BYTE			
MZone::GetPKType()
{
	if(g_pZoneTable)
	{
		ZONETABLE_INFO* pZoneInfo = g_pZoneTable->Get( GetID() );
		if(NULL != pZoneInfo)
			return pZoneInfo->PKType;
	}

	return 0;
}


// 2004, 8, 30, sobeit add start - sweep vice test
//----------------------------------------------------------------------
// ChangeSwapViceType
//----------------------------------------------------------------------
// tile�� �ִ� SwapViceType�� ���� �Ѵ�.
//----------------------------------------------------------------------
void		
MZone::ChangeSwapViceType(TYPE_SECTORPOSITION sX, TYPE_SECTORPOSITION sY, TYPE_EFFECTSPRITETYPE type, WORD wDelay)
{
	EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();

	MEffect *pEffect = NULL;
	while (iEffect != m_mapGroundEffect.end())
	{
		pEffect = iEffect->second;
		if (pEffect != NULL)
		{
			if( pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_1 ||
					pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_2 ||
					pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_3 )
				{
					pEffect->SetFrameID(type, pEffect->GetMaxFrame());
					pEffect->SetDelayFrame(wDelay);
					return;
			}
		}
		iEffect++;
	}

//	
//
//	MSector& sector = m_ppSector[sY][sX];
//
//	//--------------------------------------------------
//	// effect�� �ִ� sector���� Ȯ��
//	//--------------------------------------------------
//	if (sector.IsExistEffect())
//	{
//		int num = sector.GetEffectSize();
//
//		EFFECT_LIST::const_iterator iEffect = sector.GetEffectIterator();
//
//		for (int i=0; i<num; i++)
//		{
//			MEffect* pEffect = *iEffect;
//
//			//--------------------------------------------------
//			// sector�� �����ϴ� effect�� üũ�ؼ� �����.
//			//--------------------------------------------------
//			if (pEffect->GetEffectType() == MEffect::EFFECT_SECTOR)
//			{
//				if( pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_1 ||
//					pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_2 ||
//					pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_3 )
//				{
//					pEffect->SetFrameID(type, pEffect->GetMaxFrame());
//					pEffect->SetDelayFrame(wDelay);
//					return;
//				}
//			}
//
//			iEffect++;
//		}
//	}
	return ;
}

void		
MZone::RemoveSwapViceType()
{
	EFFECT_MAP::iterator iEffect = m_mapGroundEffect.begin();

	MEffect *pEffect = NULL;
	while (iEffect != m_mapGroundEffect.end())
	{
		pEffect = iEffect->second;
		if (pEffect != NULL)
		{
			if( pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_1 ||
				pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_2 ||
				pEffect->GetFrameID() == EFFECTSPRITETYPE_SWEEP_VICE_PRECASTING_3 )
			{
				// list���� ����
				EFFECT_MAP::iterator iEffectBackup = iEffect++;
				m_mapGroundEffect.erase( iEffectBackup );
				
				// �޸� ����
				delete pEffect;

				continue;
			}
		}
		iEffect++;
	}

}
// 2004, 8, 30, sobeit add end - sweep vice test

// 2004, 8, 31, sobeit add start
void
MZone::UpdateWaitEffects()
{
	WAIT_EFFECT_LIST::iterator iWaitEffect = m_listWaitEffect.begin();

	while (iWaitEffect != m_listWaitEffect.end())
	{
		MEffect*	pNode = *iWaitEffect;
		if( !pNode->IsWaitFrame())
		{
			AddEffect( pNode );
			iWaitEffect = m_listWaitEffect.erase(iWaitEffect);
		}
		else
			iWaitEffect ++;
	}
}
// 2004, 8, 31, sobeit add end

bool MZone::IsSurvivalZone() const
{
	return GetID() == 8001;
}
