//--------------------------------------------------------------------------
// MItemOptionTable.cpp
//--------------------------------------------------------------------------
#include "Client_PCH.h"
#include "MItemOptionTable.h"

//--------------------------------------------------------------------------
// Global
//--------------------------------------------------------------------------
ITEMOPTION_TABLE*		g_pItemOptionTable = NULL;

//char ITEMOPTION_INFO::ITEMOPTION_PARTENAME[MAX_PART][MAX_PARTNAME_LENGTH] =
//{
//	"STR",
//	"DEX",
//	"INT",
//	"HP",
//	"MP",
//	"HP Steal",
//	"MP Steal",
//	"HP Regeneration",
//	"MP Regeneration",
//	"ToHit",
//	"Defense",
//	"Damage",
//	"Protection",
//	"Durability",
//	"Poison Resistance",
//	"Acid Resistance",
//	"Curse Resistance",
//	"Blood Resistance",
//	"Vision",
//	"Attack Speed",
//	"Critical Hit",
//	"Luck",         // increase looting item type
//	"All Registance",      // increase all registance
//	"All Attributes",     // increase all attributes(str, dex, int)
//};
//
//char ITEMOPTION_INFO::ITEMOPTION_PARTNAME[MAX_PART][MAX_PARTNAME_LENGTH] =
//{
//	"STR",
//	"DEX",
//	"INT",
//	"HP",
//	"MP",
//	"HP흡수",
//	"MP흡수",
//	"HP재생력",
//	"MP재생력",
//	"명중률",
//	"디펜스",
//	"데미지",
//	"프로텍션",
//	"내구성",
//	"독 마법저항력",
//	"산 마법저항력",
//	"저주 마법저항력",
//	"블러드 마법저항력",
//	"시야",
//	"공격속도",
//	"크리티컬 히트",
//	"행운",
//	"모든 저항력",
//	"모든 능력치",
//};

//--------------------------------------------------------------------------
//
// constructor/destructor
//
//--------------------------------------------------------------------------
ITEMOPTION_INFO::ITEMOPTION_INFO()
{
}

ITEMOPTION_INFO::~ITEMOPTION_INFO()
{
}

//#include <stdio.h>

//--------------------------------------------------------------------------
//
// member functions
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Save To File
//--------------------------------------------------------------------------
//void			
//ITEMOPTION_INFO::SaveToFile(std::ofstream& file)
//{
//	//char str[80];
//	//sprintf(str, "E-%s", Name.GetString());
//	//EName = str;
//
//	EName.SaveToFile(file);							// ItemOption 이름	
//	Name.SaveToFile(file);							// ItemOption 이름
//	file.write((const char*)&Part, 4);				// ItemOption Part	
//	file.write((const char*)&PlusPoint, 4);			// 증가치
//	file.write((const char*)&PriceMultiplier, 4);				// 가치
//	//file.write((const char*)&PlusRequireAbility, 4);	// 필요 능력	
//	
//
//	file.write((const char*)&RequireSTR, 4);
//	file.write((const char*)&RequireDEX, 4);
//	file.write((const char*)&RequireINT, 4);		
//	file.write((const char*)&RequireSUM, 4);
//	file.write((const char*)&RequireLevel, 4);	
//
//	file.write((const char*)&ColorSet, 4);			// ColorSet번호
//	file.write((const char*)&UpgradeOptionType, 4);
//	file.write((const char*)&PreviousOptionType, 4);
//}

//--------------------------------------------------------------------------
// Load From File
//--------------------------------------------------------------------------
void			
ITEMOPTION_INFO::LoadFromFile(ivfstream& file)
{
	EName.LoadFromFile(file);							// ItemOption 이름	
	Name.LoadFromFile(file);							// ItemOption 이름
	file.read((char*)&Part, 4);				// ItemOption Part	
	file.read((char*)&PlusPoint, 4);			// 증가치
	file.read((char*)&PriceMultiplier, 4);				// 가치
	//file.read((char*)&PlusRequireAbility, 4);	// 필요 능력	
	
	file.read((char*)&RequireSTR, 4);
	file.read((char*)&RequireDEX, 4);
	file.read((char*)&RequireINT, 4);		
	file.read((char*)&RequireSUM, 4);
	file.read((char*)&RequireLevel, 4);

	
	file.read((char*)&ColorSet, 4);			// ColorSet번호
	file.read((char*)&UpgradeOptionType, 4);
	file.read((char*)&PreviousOptionType, 4);
}

//--------------------------------------------------------------------------
// Load From File
//--------------------------------------------------------------------------
void			
ITEMOPTION_INFO::SaveToFile(std::ofstream& file)
{
}

//--------------------------------------------------------------------------
// Load From File
//--------------------------------------------------------------------------
void
ITEMOPTION_TABLE::LoadFromFile(ivfstream& file)
{
	int size;

	file.read((char *)&size, 4);
	for(int i = 0; i < size; i++)
	{
		ITEMOPTION_PARTENAME[i].LoadFromFile(file);
		ITEMOPTION_PARTNAME[i].LoadFromFile(file);
	}

	CTypeTable<ITEMOPTION_INFO>::LoadFromFile(file);
}

//--------------------------------------------------------------------------
// Blue Drop Shine / Edge ladder
//--------------------------------------------------------------------------
// Shine and Edge (EventStar 78/79) upgrade an option along the server's third
// ladder: the OptionInfo rows with UpgradeOptionType != 0 and UpgradeThirdRatio
// != 0 (OptionInfo::isUpgradeThirdPossible). itemoption.inf only carries the
// normal Blue Drop ladder, which stops at +5, so the client keeps this copy to
// know what Shine/Edge can enchant. Matches the server after DB migration 1.3.6;
// if that ladder changes, regenerate it from
//   SELECT OptionType, UpgradeOptionType FROM OptionInfo
//   WHERE UpgradeOptionType <> 0 AND UpgradeThirdRatio <> 0 ORDER BY OptionType;
//--------------------------------------------------------------------------
static const int s_ThirdUpgradeOption[][2] =
{
	{1,2}, {2,3}, {3,4}, {4,5}, {5,88}, {6,7}, {7,8}, {8,9}, {9,10}, {10,93}, {11,12}, {12,13},
	{13,14}, {14,15}, {15,98}, {16,17}, {17,18}, {18,19}, {19,20}, {20,103}, {21,22}, {22,23},
	{23,24}, {24,25}, {25,108}, {26,27}, {27,28}, {28,112}, {29,30}, {30,31}, {31,115}, {38,39},
	{39,40}, {40,41}, {41,42}, {42,124}, {43,44}, {44,45}, {45,46}, {46,47}, {47,129}, {48,49},
	{49,50}, {50,51}, {51,52}, {52,134}, {53,54}, {54,55}, {55,56}, {56,57}, {57,139}, {58,59},
	{59,60}, {60,61}, {61,62}, {62,144}, {63,64}, {64,65}, {65,149}, {66,67}, {67,68}, {68,152},
	{69,70}, {70,71}, {71,155}, {72,73}, {73,74}, {74,158}, {75,76}, {76,77}, {77,161}, {78,79},
	{79,80}, {80,81}, {81,82}, {82,164}, {83,84}, {84,85}, {85,86}, {86,87}, {87,169}, {88,89},
	{89,90}, {90,91}, {91,92}, {93,94}, {94,95}, {95,96}, {96,97}, {98,99}, {99,100}, {100,101},
	{101,102}, {103,104}, {104,105}, {105,106}, {106,107}, {108,109}, {109,110}, {110,111}, {111,216},
	{112,113}, {113,114}, {115,116}, {116,117}, {124,125}, {125,126}, {126,127}, {127,128}, {129,130},
	{130,131}, {131,132}, {132,133}, {134,135}, {135,136}, {136,137}, {137,138}, {139,140}, {140,141},
	{141,142}, {142,143}, {144,145}, {145,146}, {146,147}, {147,148}, {149,150}, {150,151}, {152,153},
	{153,154}, {155,156}, {156,157}, {158,159}, {159,160}, {161,162}, {162,163}, {164,165}, {165,166},
	{166,167}, {167,168}, {169,170}, {170,171}, {171,172}, {172,173}, {175,176}, {176,177}, {177,213},
	{178,179}, {179,180}, {180,181}, {181,182}, {182,208}, {183,184}, {184,185}, {185,205}, {205,206},
	{206,207}, {208,209}, {209,210}, {210,211}, {211,212}, {213,214}, {214,215}
};

int
ITEMOPTION_TABLE::GetThirdUpgradeOptionType(int option)
{
	for (size_t i = 0; i < sizeof(s_ThirdUpgradeOption) / sizeof(s_ThirdUpgradeOption[0]); i++)
	{
		if (s_ThirdUpgradeOption[i][0] == option)
			return s_ThirdUpgradeOption[i][1];
	}

	return 0;
}
