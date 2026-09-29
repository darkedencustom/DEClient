//-----------------------------------------------------------------------------
// ContentsFilter.h
//-----------------------------------------------------------------------------
// Filter contents using defines.
// Filtering at run time carries a hacking risk, so
// filtering is done at compile time.
//-----------------------------------------------------------------------------

#pragma once

#define __ON	(1)
#define __OFF	(0)

// The 2008.04.05 UI update is __OFF for several designs, but the in game menu
// artwork and its skin point list (interface.inf, dated 2008.04.04) ship with
// the four row layout in every build - option / logout / quit / cancel. With
// the update off, the menu still draws four rows while the code only wires
// three buttons and misreads the four point skin data as a packed two point
// format, so the quit row does nothing. This turns on just the quit entry and
// its supporting code, without enabling the rest of that UI update.
#define __GAMEMENU_QUITEXIT	__ON

// Blood Bible tokens come in a plain tier (type 0-11) and six clan tiers
// (type 12-83: Grun / Khan / Gabri / Phantom / Red / Ose, twelve bibles each).
// The clan tier display is gated behind __CONTRIBUTE_SYSTEM, which is __OFF for
// several designs because it also adds the contribution point field to
// PCSlayerInfo2 / PCOustersInfo2 / PCOustersInfo3 / ModifyInfo on the wire, and
// the server never writes that field (its __CONTRIBUTION_POINT_ON_WIRE__ is
// left undefined for exactly that reason, so the wire stays in agreement).
// String.inf still ships every clan tier string, and the server picks a tier
// from the player's contribution rank, so it can hand out any type above 11 -
// which this client could not render, printing uninitialized stack memory for
// those rows. This turns on just the clan tier text, which is display only and
// touches no packet, without enabling the contribution point wire change.
#define __BLOOD_BIBLE_TIERS	__ON

// Twelve bibles per tier, one plain tier plus six clan tiers, so the valid
// token types are 0-83. String.inf carries exactly 72 clan tier bonus strings
// (UI_STRING_MESSAGE_BLOOD_BIBLE_BONUS_GRUN_ARMEGA onwards) to match. The
// string table indexes unchecked, so callers must range check before use.
#define BLOOD_BIBLE_PER_TIER	12
#define BLOOD_BIBLE_TIER_COUNT	6
#define BLOOD_BIBLE_TYPE_MAX	(BLOOD_BIBLE_PER_TIER * (BLOOD_BIBLE_TIER_COUNT + 1))

//////////////////////////////////////////////////////////////////////////
// Korea

#ifdef __DESIGNED_KOREA
#define __DESIGN_NAME "Designed Korea"

#	define __ADVANCEMENT_CHARACTER			__ON
#	define __LANGUAGE_CHANGE				__ON

//#	define __POWER_JJANG_ID					__OFF
#	define __RACE_OUSTERS					__ON	// Taiwan first development. 2009.10.12
#	define __USER_GRADE						__ON
#	define __SMS_SERVICES					__OFF
#	define __RARE_SKILL						__ON

#	define __INTERNATIONAL_PREMIUM_SYSTEM	__OFF
#	define __NEW_SYSTEM_AVAILABILITIES		__OFF
#	define __ENCHANT_REQUIRE_LEVE			__OFF
#	define __ID_PASSWORD					__OFF

#	define __CHAR_DELETE					__OFF
#	define __KOREAN_UI						__ON
#	define __INTERNATIONAL_UI				__OFF
#	define __ENGLISH_STRFILTER				__OFF
#	define __CRYPT							__OFF
#	define __CONECT_LIMITER					__OFF
#	define __PERSNALSHOP_NAMING				__ON
#	define __HERO_SYSTEM					__ON
#	define __CONTRIBUTE_SYSTEM				__ON
#	define __LEVEL_WAR_RENEWAL				__ON
#	define __SERVER_SELECT_REVEWAL			__ON
#	define __MENEGROTH						__ON
#	define __GPS_ADD						__ON
#	define __2008_NEW_YEAR_EVENT			__ON
#	define __2008_FIRST_CHARGE_ITEM			__ON	// 2008 first paid item
#	define __DSG_EFFECT						__ON
#	define __GEAR_SWAP_CHANGE				__ON	//								live svr,	test svr,	dev svr
#	define __2008_WHITEDAY_LOVECANDY		__ON	// 2008 White Day item
#	define __080327_ORDINIS_VAPOR			__ON	// 2008 rank experience hourglass				test svr,	dev svr
#	define __PAYZONE_PASS_TICKET			__ON	// Paid zone entry ticket
#	define __QUEST_RENEWAL					__ON	//										test svr,	dev svr
#	define __EFFECT_RENEWAL					__ON	//								live svr,	test svr,	dev svr
#	define __SECOND_RARE_SKILL				__ON	//										test svr,	dev svr
#	define __080405_FIREST_UI_UPDATE		__ON	//										test svr,	dev svr
#	define __LIMITED_ITEM_UNISEX			__ON	
#	define __FAST_TRANSFORTER				__ON	
#	define __CLOVER_EVENT					__ON
#	define __QUEST_RENEWAL2					__ON	// __QUEST_RENEWAL was patched earlier; __QUEST_RENEWAL2 is still being revised.
#	define __GORE_BLOOD_BIBLE				__ON	// Item image by Blood Bible grade
#	define __AN_ADVANCEMENT_GUN_REFLAME		__ON	// Non-advancement Slayer muzzle flash effect (basic, Freezing Vane, Bullet of Light) coordinate fix

#	define __TIPOJYU_CASTLE					__ON
#	define __2008_JUNE_EVENT_EX_STONE		__ON	// Experience stone event (Friday the 13th)
#	define __NEW_ADVANCEMENT_ITEM			__ON	// New advancement item
#	define __TUNING_ITEM					__ON	// Tuning item
#	define __JAPAN_REVERSE_DEVELOP			__OFF	// Japan item reverse development
#	define __CHINESE_UI						__OFF	// Apply Chinese font enlargement and UI changes
#	define __JAPAN_UI						__OFF	// Japan UI
#	define __NEW_PET_FIRE_ELEMENTAL			__ON	// New market item (Fire Elemental)
#	define __SECOND_TRANSFORTER				__ON	// New transport, second batch
#	define __ADD_MAGICPROTECTION			__ON	// Magic defense added to accessory items (excludes core zap types and advancement items)
#	define __AUTO_ITEM_ROOTING				__OFF	// Item auto-looting code (close to test code)
#	define __LOCALIZING_LONGNAME			__OFF	// Allows long monster names when localizing for Japan
#	define __OX_EVENT						__ON	// OX event
#	define __ITEM_SHOW						__ON	// Item view

#	define __BLITZ_COUPON_EVENT				__ON	// Blitz server coupon event
#	define __TITLE_UI_RENWEAL				__ON	// Title UI renewal (2008.08 Blitz)
#	define __LOGIN_PACKET					__OFF	// Blocked by packet so the client can be downloaded, to prevent speed hacks (2008.09.08)
#	define __PET_FOOD_ITEM_ADD				__ON	// Max Fooder added (2008.09.08)
#	define __DYNAMIC_UI						__OFF	// 
#	define __PREMIUM_GIVE_ITEM_UI			__ON	// Premium granted item UI
#	define __2008_AUTUMN_LEAVES_EVENT		__ON	// Autumn leaves event			2008.10.1 patch
#	define __2008_OCTOBER_MAKETITEM			__ON	// October paid item
#	define __PREMIUM_CARD					__ON	// Premium card (split from __INTERNATIONAL_PREMIUM_SYSTEM)
#	define __EXTREME_FORCE_SCROLL			__ON	// Premium granted item "Extreme Force Scroll"
#	define __PREMIUM_LOTTER					__ON	// Premium granted item "Premium Lottery"
#	define __TIPOJYU_CASTLE_ITEM			__ON	// Filter added because item indices sometimes must be added while leaving Tipojyu contents in place; more like it may follow...
#	define __HALLOWEEN						__ON	// Japan Halloween event item; ought to go to the live server too.
#	define __SECOND_TRANSFORTER_ITEM		__ON	// Japan second fast transport - item only, split out as contents
#	define __GLOBAL_NPC						__ON	// NPC summon
#	define __MODIFY_FORCE_SCROLL			__ON	// Allow Odin Force Scroll and Tithuos Force Scroll to stack
#	define __MOVE_ZONE						__ON	// Map warp feature for developer and GM convenience
#	define __PREMIUM_CARD_D					__ON	// Premium Card D added
#	define __NEW_PET_INCUBUS				__ON	// Incubus pet added
#	define __EXPERT_PET_CHANGER				__ON	// Expert Pet Changer added
#	define __PET_VISION_AMPLE				__ON	// Pet item Vision Ampoule
#	define __CHECKTIME_CONTROLL				__ON	// Send time-check packets more often, for speed hack detection.
#	define __PREMIUM_CARD_C					__ON	// Premium Card C - the server had already reserved the empty item slots
#	define __TIME_STAMP						__ON	// Code added to socket output (login may fail because of it). ID: shootkj, date: 2008.12.18
#	define __CHRISTMAS_SOCKS_EVENT			__ON	// Event featuring Christmas stocking pieces
#	define __EVENTTREE_PILE					__ON	// Toggles whether event trees can stack; must be ON whenever the Blitz event is ON.

#	define __HERO_SYSTEM_ITEM				__ON	
#	define __CONTRIBUTE_SYSTEM_ITEM			__ON	
#	define __LEVEL_WAR_RENEWAL_ITEM			__ON	
#	define __MENEGROTH_ITEM					__ON
#	define __LC_SELECT_WORLD_ERROR			__ON	// Packet sent when server selection fails; added for the overseas version
#	define __FUNCTION_PETFOOD				__ON	// Functional pet food
#	define __PCITEMINFO						__ON	// Consolidates the previously scattered pieces so item info goes through PCItemInfo

#	define __REPAIRSYSTEM_RENEWAL			__ON	// Make repair price server-dependent.
#	define __DCGLOBAL_NPC					__ON	// Discounter NPC summon item
#	define __GAMEOPTION_ITEMBAY_MESSAGEBOX	__ON	// Game option for the "sell this item?" confirmation box
#	define __WORLDCHATING					__ON	// Cross-world chat system
#	define __FUNCTION_PETFOOD_SMART			__ON	// Smart Fooder
#	define __VISION_AMPLE					__ON	// Vision Ampoule
#	define __THIRD_ENCHANT_2				__ON	// Third Enchant 2
#	define __XTRAP							__OFF	// Add/remove XTrap; the lib files have to be moved too, of course
#	define __NPROTECT						__OFF	// Add/remove NProtect	2021-08-13 off by kim
#	define __PREMIUM_CARD_ADAY				__ON	// Premium Card A Day - new per-day premium card - 2009.04.28
#	define __PREMIUM_LOTTER_A				__ON	// Premium Lottery A - new lottery card - 2009.04.28
#	define __PREMIUM_LOTTER_B				__ON	// Premium Lottery B - new lottery card - 2009.04.29
#	define __ADVANCEMENT_NEW_UNIQUE_ITEM	__ON	// Advancement unique item - 2009.05.14
#	define __HIDDEN_BOSS_SYSTEM				__ON	// Hidden boss spawn system
#	define __CHANGE_PET_INFO_WINDOW_IMAGE	__ON	// Pet info window image change - 2009.05.21
#	define __PET_MIXINGFORGE				__ON	// Pet combiner and combine canceller
#	define __PCITEMINFO2					__ON	// PCItemInfo merge, second pass; finish it quickly and fold into the first
#	define __ATTACK_SPEED					__ON	// Make attack speed server-dependent
#	define __ONIBLA							__ON	// Onibla development
#	define __FRIEND_ADDITION				__ON	// Friend list contents - 2009.06.12
#	define __ONIBLA_ITEM					__ON	// Onibla contents item - 2009.06.29
#	define __AUTO_ATTACT					__OFF	// Restrict to auto-attack monsters only.
#	define __IMI_INTERFACE					__OFF	// IMI interface
#	define __DARKEDEN_MARKET				__ON   // Turn off the DarkEden market
#	define __CREDIT							__ON	// Turn off credits
#	define __15_PVP_INHIBITION				__OFF	// Block under-15 users from the PvP server (for the age-12 rating review)
#	define __SUMMER_VACTATION_COOL_EVENT	__ON	// Summer vacation special - Cool Summer event - 2009.07.27
#	define __IMI_LOW_LEVEL_ITEM				__ON	// Low-grade items for IMI - 2009.08.03
#	define __IMI_SENDGLASS					__ON	// Hourglass for IMI
#	define __PET_MASHIMARO					__ON	// Mashimaro
#	define __TREASURE_BOX					__ON	// Treasure box
#	define __PET_CATS						__ON	// New Japan market item, 3 cats - 2009.08.14
#	define __MASTER_EFFECTS					__ON	// Time-limited advancement/non-advancement effect items - 2009.08.24
#	define __IMI_NEW_ENCHANT_ITEM			__ON	// New enchant item for IMI - 2009.08.25
#	define __MOONLIGHT_FESTIVAL_EVENT		__ON	// Moonlight Festival event - 2009.09.15
#	define __TREBLE_PET_SANDGLASS			__ON	// Triple-bonus pet experience hourglass - 2009.09.21
#	define __IMI_MIXNGFORGE					__ON	// Mixing Forge for IMI - 2009.09.21
#	define __NONE_ADVANCEMENT_SANDGLASS		__ON	// General-purpose non-advancement experience hourglass - 2009.09.22
#	define __PET_CATS_UPGRADED				__ON	// Japan market upgrade, 12 cats - 2009.09.21
#	define __UPGRADE_MONSTER_PET			__ON	// Upgraded monster pet - 2009.10.09
#	define __20091028_ADD_ITEM				__ON	// New items added - 2009.10.28
#	define __TAIWAN_INTERFACE				__OFF	// Taiwan interface
#	define __CRIMSON_EVENT					__ON	// Crimson event added - 2009.11.09
#	define __20091116_ADD_RARE_SKILL		__ON	// Rare skill added - 2009.11.16
#	define __20091119_ADD_ENCHANT_ITEM		__ON	// New enchant item - 2009.11.19

#	define __NEW_SKIN_181		__OFF	// New set and Ousters appearance - 2021.08.14
#	define __OPTION_NAME_PRINT		__ON	// Remove item option suffix - 2021.08.15
#endif	// DESIGNED_KOREA

//////////////////////////////////////////////////////////////////////////
// Internation

#ifdef __DESIGNED_INTERNATION
#define __DESIGN_NAME "Designed Internation"

#define __ADVANCEMENT_CHARACTER			__OFF
#define __LANGUAGE_CHANGE				__OFF

//#	define __POWER_JJANG_ID					__OFF
#define __RACE_OUSTERS						__ON
#define __USER_GRADE						__OFF
#define __SMS_SERVICES						__OFF
#define __RARE_SKILL						__ON
#define __INTERNATIONAL_PREMIUM_SYSTEM		__ON

#define __NEW_SYSTEM_AVAILABILITIES			__ON
#define __ENCHANT_REQUIRE_LEVE				__ON
#define __ID_PASSWORD						__ON

#define __CHAR_DELETE						__ON
#define __KOREAN_UI							__OFF
#define __INTERNATIONAL_UI					__ON
#define __ENGLISH_STRFILTER					__ON
#define __CRYPT								__ON
#define __CONECT_LIMITER					__ON
#define __PERSNALSHOP_NAMING				__OFF

#define __HERO_SYSTEM						__ON	//-- ON when building the info files, OFF for the executable
#define __CONTRIBUTE_SYSTEM					__OFF	// server VersionType.h has __CONTRIBUTION_POINT_ON_WIRE__ commented out, so it never writes m_ContributePoint
#define __LEVEL_WAR_RENEWAL					__ON	//-- ON when building the info files, OFF for the executable
#define __SERVER_SELECT_REVEWAL				__OFF
#define __MENEGROTH							__ON
#define __GPS_ADD							__OFF
#define __2008_NEW_YEAR_EVENT				__OFF
#define __2008_FIRST_CHARGE_ITEM			__OFF	// 2008 first paid item
#define __DSG_EFFECT						__ON
#define __GEAR_SWAP_CHANGE					__ON	// Dual equip window (gear set I/II); DEServer wire fields restored to match 2026-09-17
#define __2008_WHITEDAY_LOVECANDY			__OFF	// 2008 White Day item
#define __080327_ORDINIS_VAPOR				__ON	// 2008 rank experience hourglass
#define __PAYZONE_PASS_TICKET				__OFF
#define __QUEST_RENEWAL						__OFF	// Quest renewal
#define __EFFECT_RENEWAL					__OFF
#define __SECOND_RARE_SKILL					__ON
#define __080405_FIREST_UI_UPDATE			__OFF	// wide flag (95 blocks/16 files) gating the accelerator + hotkey UI; __GAMEMENU_QUITEXIT covers the one part we need
#define __LIMITED_ITEM_UNISEX				__OFF
#define __FAST_TRANSFORTER					__ON
#define __CLOVER_EVENT						__OFF
#define __QUEST_RENEWAL2					__OFF
#define __GORE_BLOOD_BIBLE					__OFF	// Item image by Blood Bible grade
#define __AN_ADVANCEMENT_GUN_REFLAME		__OFF	// Non-advancement Slayer muzzle flash effect (basic, Freezing Vane, Bullet of Light) coordinate fix
#define __TIPOJYU_CASTLE					__ON
#define __HERO_CONTRINBUTE_LEVEL			__OFF	// Menegroth mixes contribution, hero and level war effects, so it is ON when building the Info files and OFF for the executable.
#define __2008_JUNE_EVENT_EX_STONE			__OFF	// Experience stone event (Friday the 13th)
#define __CREATURE_INDEX_BLANK_SUCCESS		__ON	// Fixes the empty-index error when two people edit creature contents at once.
#define __NEW_ADVANCEMENT_ITEM				__OFF	// New advancement item
#define __TUNING_ITEM						__ON	// Tuning item
#define __JAPAN_REVERSE_DEVELOP				__ON	// Japan item reverse development
#define __CHINESE_UI						__OFF	// Apply Chinese font enlargement and UI changes
#define __JAPAN_UI							__OFF	// Japan UI
#define __NEW_PET_FIRE_ELEMENTAL			__ON	// New market item (Fire Elemental)
#define __SECOND_TRANSFORTER				__ON	// New transport, second batch
#define __ADD_MAGICPROTECTION				__ON	// Magic defense added to accessory items (excludes core zap types and advancement items)
#define __LOCALIZING_LONGNAME				__ON 	// Allows long monster names when localizing for Japan
#define __OX_EVENT							__OFF	// OX event
#define __ITEM_SHOW							__ON	// Item view
#define __BLITZ_COUPON_EVENT				__OFF	// Blitz server coupon event
#define __TITLE_UI_RENWEAL					__ON 	// Title UI renewal (2008.08 Blitz)
#define __LOGIN_PACKET						__OFF	// Blocked by packet so the client can be downloaded, to prevent speed hacks (2008.09.08)
#define __PET_FOOD_ITEM_ADD					__OFF	// Max Fooder added (2008.09.08)
#define __DYNAMIC_UI						__OFF
#define __PREMIUM_GIVE_ITEM_UI				__OFF	// Premium granted item UI
#define __2008_AUTUMN_LEAVES_EVENT			__ON	// Autumn leaves event
#define __2008_OCTOBER_MAKETITEM			__ON	// October paid item
#define __PREMIUM_CARD						__ON	// Premium card (split from __INTERNATIONAL_PREMIUM_SYSTEM)
#define __EXTREME_FORCE_SCROLL				__OFF	// Premium granted item "Extreme Force Scroll"
#define __PREMIUM_LOTTER					__OFF	// Premium granted item "Premium Lottery"
#define __TIPOJYU_CASTLE_ITEM				__ON	// Filter added because item indices sometimes must be added while leaving Tipojyu contents in place; more like it may follow...
#define __HALLOWEEN							__OFF	// Japan Halloween event item; ought to go to the live server too.
#define __SECOND_TRANSFORTER_ITEM			__ON	// Japan second fast transport - item only, split out as contents
#define __GLOBAL_NPC						__ON	// NPC summon
#define __MODIFY_FORCE_SCROLL				__OFF	// Allow Odin Force Scroll and Tithuos Force Scroll to stack
#define __MOVE_ZONE							__OFF	// Map warp feature for developer and GM convenience
#define __PREMIUM_CARD_D					__OFF	// Premium Card D added
#define __NEW_PET_INCUBUS					__ON	// Incubus pet added
#define __EXPERT_PET_CHANGER				__OFF	// Expert Pet Changer added
#define __PET_VISION_AMPLE					__OFF	// Pet item Vision Ampoule
#define __CHECKTIME_CONTROLL				__OFF	// Send time-check packets more often, for speed hack detection.
#define __PREMIUM_CARD_C					__ON	// Premium Card C - the server had already reserved the empty item slots
#define __TIME_STAMP						__OFF	// Code added to socket output (login may fail because of it). ID: shootkj, date: 2008.12.18
#define __CHRISTMAS_SOCKS_EVENT				__OFF	// Event featuring Christmas stocking pieces
#define __EVENTTREE_PILE					__OFF	// Toggles whether event trees can stack; must be ON whenever the Blitz event is ON.

#define __HERO_SYSTEM_ITEM					__ON	
#define __CONTRIBUTE_SYSTEM_ITEM			__OFF	
#define __LEVEL_WAR_RENEWAL_ITEM			__ON	
#define __MENEGROTH_ITEM					__ON
#define __LC_SELECT_WORLD_ERROR				__OFF	// Packet sent when server selection fails; added for the overseas version
#define __FUNCTION_PETFOOD					__OFF	// Functional pet food
#define __PCITEMINFO						__OFF	// Consolidates the previously scattered pieces so item info goes through PCItemInfo
#define __REPAIRSYSTEM_RENEWAL				__ON	// Make repair price server-dependent.
#define __DCGLOBAL_NPC						__OFF	// Discounter NPC summon item
#define __GAMEOPTION_ITEMBAY_MESSAGEBOX		__OFF	// Game option for the "sell this item?" confirmation box
#define __WORLDCHATING						__OFF	// Cross-world chat system
#define __FUNCTION_PETFOOD_SMART			__OFF	// Smart Fooder
#define __VISION_AMPLE						__OFF	// Vision Ampoule
#define __THIRD_ENCHANT_2					__OFF	// Third Enchant 2
#define __XTRAP								__OFF	// Add/remove XTrap; the lib files have to be moved too, of course
#define __NPROTECT							__OFF	// Add/remove NProtect
#define __PREMIUM_CARD_ADAY					__OFF	// Premium Card A Day - new per-day premium card - 2009.04.28
#define __PREMIUM_LOTTER_A					__OFF	// Premium Lottery A - new lottery card - 2009.04.28
#define __PREMIUM_LOTTER_B					__OFF	// Premium Lottery B - new lottery card - 2009.04.29
#define __ADVANCEMENT_NEW_UNIQUE_ITEM		__OFF	// Advancement unique item - 2009.05.14
#define __HIDDEN_BOSS_SYSTEM				__ON	// Hidden boss spawn system
#define __CHANGE_PET_INFO_WINDOW_IMAGE		__OFF	// Pet info window image change - 2009.05.21
#define __PET_MIXINGFORGE					__OFF	// Pet combiner and combine canceller
#define __PCITEMINFO2						__ON    // PCItemInfo merge, second pass; finish it quickly and fold into the first
#define __ATTACK_SPEED						__ON	// Make attack speed server-dependent
#define __ONIBLA							__ON	// Onibla development
#define __FRIEND_ADDITION					__OFF	// Friend list contents - 2009.06.12
#define __ONIBLA_ITEM						__ON	// Onibla contents item - 2009.06.29  ** flipped ON to match item.inf layout **
#define __AUTO_ATTACT						__OFF	// Restrict to auto-attack monsters only.
#define __IMI_INTERFACE						__OFF	// IMI interface
#define __DARKEDEN_MARKET					__OFF   // Turn off the DarkEden market
#define __CREDIT							__OFF	// Turn off credits
#define __15_PVP_INHIBITION					__OFF	// Block under-15 users from the PvP server (for the age-12 rating review)
#define __SUMMER_VACTATION_COOL_EVENT		__OFF	// Summer vacation special - Cool Summer event - 2009.07.27
#define __IMI_LOW_LEVEL_ITEM				__OFF	// Low-grade items for IMI - 2009.08.03
#define __IMI_SENDGLASS						__OFF	// Hourglass for IMI
#define __PET_MASHIMARO						__ON	// Mashimaro
#define __TREASURE_BOX						__OFF	// Treasure box
#define __PET_CATS							__ON	// New Japan market item, 3 cats - 2009.08.14
#define __MASTER_EFFECTS					__ON	// Time-limited advancement/non-advancement effect items - 2009.08.24
#define __IMI_NEW_ENCHANT_ITEM				__OFF	// New enchant item for IMI - 2009.08.25
#define __MOONLIGHT_FESTIVAL_EVENT			__ON	// Moonlight Festival event - 2009.09.15
#define __TREBLE_PET_SANDGLASS				__ON	// Triple-bonus pet experience hourglass - 2009.09.21
#define __IMI_MIXNGFORGE					__ON	// Mixing Forge for IMI - 2009.09.21
#define __NONE_ADVANCEMENT_SANDGLASS		__ON	// General-purpose non-advancement experience hourglass - 2009.09.22
#define __PET_CATS_UPGRADED					__ON	// Japan market upgrade, 12 cats - 2009.09.21
#define __UPGRADE_MONSTER_PET				__ON	// Upgraded monster pet - 2009.10.09
#define __20091028_ADD_ITEM					__ON	// New items added - 2009.10.28
#define __TAIWAN_INTERFACE					__OFF	// Taiwan interface
#define __CRIMSON_EVENT						__ON	// Crimson event added - 2009.11.09
#define __20091116_ADD_RARE_SKILL			__ON	// Rare skill added - 2009.11.16
#define __20091119_ADD_ENCHANT_ITEM			__ON	// New enchant item - 2009.11.19

#define __NEW_SKIN_181		__OFF	// New set and Ousters appearance - 2021.08.14
#endif	// DESIGNED_INTERNATION



//////////////////////////////////////////////////////////////////////////
// China

#ifdef __DESIGNED_CHINA
#define __DESIGN_NAME "Designed China"

#	define __ADVANCEMENT_CHARACTER			__ON
#	define __LANGUAGE_CHANGE				__ON

//#	define __POWER_JJANG_ID					__OFF
#	define __RACE_OUSTERS					__ON	// Taiwan first development. 2009.10.12
#	define __USER_GRADE						__OFF
#	define __SMS_SERVICES					__ON
#	define __RARE_SKILL						__ON
#	define __INTERNATIONAL_PREMIUM_SYSTEM	__OFF

#	define __NEW_SYSTEM_AVAILABILITIES		__OFF
#	define __ENCHANT_REQUIRE_LEVE			__OFF
#	define __ID_PASSWORD					__OFF

#	define __CHAR_DELETE					__ON
#	define __INTERNATIONAL_UI				__OFF
#	define __KOREAN_UI						__OFF
#	define __ENGLISH_STRFILTER				__OFF
#	define __CRYPT							__OFF
#	define __CONECT_LIMITER					__OFF
#	define __PERSNALSHOP_NAMING				__ON

#	define __HERO_SYSTEM					__ON
#	define __CONTRIBUTE_SYSTEM				__ON
#	define __LEVEL_WAR_RENEWAL				__ON
#	define __SERVER_SELECT_REVEWAL			__ON
#	define __MENEGROTH						__ON
#	define __GPS_ADD						__ON
#	define __2008_NEW_YEAR_EVENT			__ON
#	define __2008_FIRST_CHARGE_ITEM			__ON	//2008 first paid item
#	define __DSG_EFFECT						__ON
#	define __GEAR_SWAP_CHANGE				__ON	//Dual equip window
#	define __2008_WHITEDAY_LOVECANDY		__ON	//2008 White Day item
#	define __080327_ORDINIS_VAPOR			__ON	//2008 rank experience hourglass
#	define __PAYZONE_PASS_TICKET			__ON
#	define __QUEST_RENEWAL					__ON	//Quest renewal
#	define __EFFECT_RENEWAL					__ON
#	define __SECOND_RARE_SKILL				__ON
#	define __080405_FIREST_UI_UPDATE		__ON
#	define __LIMITED_ITEM_UNISEX			__ON
#	define __FAST_TRANSFORTER				__ON
#	define __CLOVER_EVENT					__ON
#	define __QUEST_RENEWAL2					__ON
#	define __GORE_BLOOD_BIBLE				__OFF	// Item image by Blood Bible grade
#	define __AN_ADVANCEMENT_GUN_REFLAME		__OFF	// Non-advancement Slayer muzzle flash effect (basic, Freezing Vane, Bullet of Light) coordinate fix

#	define __TIPOJYU_CASTLE					__OFF
#	define __2008_JUNE_EVENT_EX_STONE		__OFF	// Experience stone event (Friday the 13th)
#	define __NEW_ADVANCEMENT_ITEM			__OFF	// New advancement item
#	define __TUNING_ITEM					__OFF	// Tuning item
#	define __JAPAN_REVERSE_DEVELOP			__OFF	// Japan item reverse development
#	define __CHINESE_UI						__ON	// Apply Chinese font enlargement and UI changes
#	define __JAPAN_UI						__OFF	//Japan UI
#	define __NEW_PET_FIRE_ELEMENTAL			__OFF	// New market item (Fire Elemental)
#	define __SECOND_TRANSFORTER				__OFF	// New transport, second batch
#	define __ADD_MAGICPROTECTION			__OFF	// Magic defense added to accessory items (excludes core zap types and advancement items)
#	define __LOCALIZING_LONGNAME			__OFF	// Allows long monster names when localizing for Japan
#	define __OX_EVENT						__OFF	//OX event
#	define __ITEM_SHOW						__OFF	// Item view
#	define __BLITZ_COUPON_EVENT				__OFF	// Blitz server coupon event
#	define __TITLE_UI_RENWEAL				__OFF	// Title UI renewal (2008.08 Blitz)
#	define __LOGIN_PACKET					__OFF	// Blocked by packet so the client can be downloaded, to prevent speed hacks (2008.09.08)
#	define __PET_FOOD_ITEM_ADD				__OFF	// Max Fooder added (2008.09.08)
#	define __DYNAMIC_UI						__OFF
#	define __PREMIUM_GIVE_ITEM_UI			__OFF	// Premium granted item UI
#	define __2008_AUTUMN_LEAVES_EVENT		__ON	// Autumn leaves event
#	define __2008_OCTOBER_MAKETITEM			__OFF	// October paid item
#	define __PREMIUM_CARD					__OFF	// Premium card (split from __INTERNATIONAL_PREMIUM_SYSTEM)
#	define __EXTREME_FORCE_SCROLL			__OFF	// Premium granted item "Extreme Force Scroll"
#	define __PREMIUM_LOTTER					__OFF	// Premium granted item "Premium Lottery"
#	define __TIPOJYU_CASTLE_ITEM			__OFF	// Filter added because item indices sometimes must be added while leaving Tipojyu contents in place; more like it may follow...
#	define __HALLOWEEN						__OFF	// Japan Halloween event item; ought to go to the live server too.
#	define __SECOND_TRANSFORTER_ITEM		__OFF	// Japan second fast transport - item only, split out as contents
#	define __GLOBAL_NPC						__ON	// NPC summon
#	define __MODIFY_FORCE_SCROLL			__OFF	// Allow Odin Force Scroll and Tithuos Force Scroll to stack
#	define __MOVE_ZONE						__OFF	// Map warp feature for developer and GM convenience
#	define __PREMIUM_CARD_D					__OFF	// Premium Card D added
#	define __NEW_PET_INCUBUS				__OFF	// Incubus pet added
#	define __EXPERT_PET_CHANGER				__OFF	// Expert Pet Changer added
#	define __PET_VISION_AMPLE				__OFF	// Pet item Vision Ampoule
#	define __CHECKTIME_CONTROLL				__OFF	// Send time-check packets more often, for speed hack detection.
#	define __PREMIUM_CARD_C					__ON	// Premium Card C - the server had already reserved the empty item slots
#	define __TIME_STAMP						__OFF	// Code added to socket output (login may fail because of it). ID: shootkj, date: 2008.12.18
#	define __CHRISTMAS_SOCKS_EVENT			__OFF	// Event featuring Christmas stocking pieces
#	define __EVENTTREE_PILE					__OFF	// Toggles whether event trees can stack; must be ON whenever the Blitz event is ON.

#	define __HERO_SYSTEM_ITEM				__OFF	
#	define __CONTRIBUTE_SYSTEM_ITEM			__OFF	
#	define __LEVEL_WAR_RENEWAL_ITEM			__OFF	
#	define __MENEGROTH_ITEM					__OFF
#	define __LC_SELECT_WORLD_ERROR			__OFF	// Packet sent when server selection fails; added for the overseas version
#	define __FUNCTION_PETFOOD				__OFF	// Functional pet food
#	define __PCITEMINFO						__OFF	// Consolidates the previously scattered pieces so item info goes through PCItemInfo
#	define __REPAIRSYSTEM_RENEWAL			__OFF	// Make repair price server-dependent.
#	define __DCGLOBAL_NPC					__OFF	// Discounter NPC summon item
#	define __GAMEOPTION_ITEMBAY_MESSAGEBOX	__OFF	// Game option for the "sell this item?" confirmation box
#	define __WORLDCHATING					__OFF	// Cross-world chat system
#	define __FUNCTION_PETFOOD_SMART			__OFF	// Smart Fooder
#	define __VISION_AMPLE					__OFF	// Vision Ampoule
#	define __THIRD_ENCHANT_2				__OFF	// Third Enchant 2
#	define __XTRAP							__OFF	// Add/remove XTrap; the lib files have to be moved too, of course
#	define __NPROTECT						__OFF	// Add/remove NProtect
#	define __PREMIUM_CARD_ADAY				__OFF	// Premium Card A Day - new per-day premium card - 2009.04.28
#	define __PREMIUM_LOTTER_A				__OFF	// Premium Lottery A - new lottery card - 2009.04.28
#	define __PREMIUM_LOTTER_B				__OFF	// Premium Lottery B - new lottery card - 2009.04.29
#	define __ADVANCEMENT_NEW_UNIQUE_ITEM	__OFF	// Advancement unique item - 2009.05.14
#	define __HIDDEN_BOSS_SYSTEM				__ON	// Hidden boss spawn system
#	define __CHANGE_PET_INFO_WINDOW_IMAGE	__OFF	// Pet info window image change - 2009.05.21
#	define __PET_MIXINGFORGE				__OFF	// Pet combiner and combine canceller
#	define __PCITEMINFO2					__OFF   // PCItemInfo merge, second pass; finish it quickly and fold into the first
#	define __ATTACK_SPEED					__OFF	// Make attack speed server-dependent
#	define __ONIBLA							__OFF	// Onibla development
#	define __FRIEND_ADDITION				__OFF	// Friend list contents - 2009.06.12
#	define __ONIBLA_ITEM					__OFF	// Onibla contents item - 2009.06.29
#	define __AUTO_ATTACT					__OFF	// Restrict to auto-attack monsters only.
#	define __IMI_INTERFACE					__OFF	// IMI interface
#	define __DARKEDEN_MARKET				__OFF   // Turn off the DarkEden market
#	define __CREDIT							__OFF	// Turn off credits
#	define __15_PVP_INHIBITION				__OFF	// Block under-15 users from the PvP server (for the age-12 rating review)
#	define __SUMMER_VACTATION_COOL_EVENT	__OFF	// Summer vacation special - Cool Summer event - 2009.07.27
#	define __IMI_LOW_LEVEL_ITEM				__OFF	// Low-grade items for IMI - 2009.08.03
#	define __IMI_SENDGLASS					__OFF	// Hourglass for IMI
#	define __PET_MASHIMARO					__OFF	// Mashimaro
#	define __TREASURE_BOX					__OFF	// Treasure box
#	define __PET_CATS						__OFF	// New Japan market item, 3 cats - 2009.08.14
#	define __MASTER_EFFECTS					__OFF	// Time-limited advancement/non-advancement effect items - 2009.08.24
#	define __IMI_NEW_ENCHANT_ITEM			__OFF	// New enchant item for IMI - 2009.08.25
#	define __MOONLIGHT_FESTIVAL_EVENT		__ON	// Moonlight Festival event - 2009.09.15
#	define __TREBLE_PET_SANDGLASS			__ON	// Triple-bonus pet experience hourglass - 2009.09.21
#	define __IMI_MIXNGFORGE					__ON	// Mixing Forge for IMI - 2009.09.21
#	define __NONE_ADVANCEMENT_SANDGLASS		__ON	// General-purpose non-advancement experience hourglass - 2009.09.22
#	define __PET_CATS_UPGRADED				__ON	// Japan market upgrade, 12 cats - 2009.09.21
#	define __UPGRADE_MONSTER_PET			__ON	// Upgraded monster pet - 2009.10.09
#	define __20091028_ADD_ITEM				__ON	// New items added - 2009.10.28
#	define __TAIWAN_INTERFACE				__OFF	// Taiwan interface
#	define __CRIMSON_EVENT					__ON	// Crimson event added - 2009.11.09
#	define __20091116_ADD_RARE_SKILL		__ON	// Rare skill added - 2009.11.16
#	define __20091119_ADD_ENCHANT_ITEM		__ON	// New enchant item - 2009.11.19

#	define __NEW_SKIN_181		__OFF	// New set and Ousters appearance - 2021.08.14
#endif	// __DESIGNED_CHINA



//////////////////////////////////////////////////////////////////////////
// Japan

#ifdef __DESIGNED_JAPAN
#define __DESIGN_NAME "Designed Japan"

#	define __ADVANCEMENT_CHARACTER			__ON
#	define __LANGUAGE_CHANGE				__ON

//#	define __POWER_JJANG_ID					__OFF
#	define __RACE_OUSTERS					__ON	// Taiwan first development. 2009.10.12
#	define __USER_GRADE						__OFF
#	define __SMS_SERVICES					__OFF
#	define __RARE_SKILL						__ON
#	define __INTERNATIONAL_PREMIUM_SYSTEM	__OFF

#	define __NEW_SYSTEM_AVAILABILITIES		__OFF
#	define __ENCHANT_REQUIRE_LEVE			__OFF
#	define __ID_PASSWORD					__OFF

#	define __CHAR_DELETE					__ON
#	define __INTERNATIONAL_UI				__OFF
#	define __KOREAN_UI						__OFF
#	define __ENGLISH_STRFILTER				__OFF
#	define __CRYPT							__OFF
#	define __CONECT_LIMITER					__OFF
#	define __PERSNALSHOP_NAMING				__ON

#	define __HERO_SYSTEM					__ON
#	define __CONTRIBUTE_SYSTEM				__ON
#	define __LEVEL_WAR_RENEWAL				__ON
#	define __SERVER_SELECT_REVEWAL			__ON
#	define __MENEGROTH						__ON
#	define __GPS_ADD						__ON
#	define __2008_NEW_YEAR_EVENT			__ON
#	define __2008_FIRST_CHARGE_ITEM			__ON	//2008 first paid item
#	define __DSG_EFFECT						__ON
#	define __GEAR_SWAP_CHANGE				__ON	//Dual equip window
#	define __2008_WHITEDAY_LOVECANDY		__ON	//2008 White Day item
#	define __080327_ORDINIS_VAPOR			__ON	//2008 rank experience hourglass
#	define __PAYZONE_PASS_TICKET			__ON
#	define __QUEST_RENEWAL					__ON	//Quest renewal
#	define __EFFECT_RENEWAL					__ON
#	define __SECOND_RARE_SKILL				__ON
#	define __080405_FIREST_UI_UPDATE		__ON
#	define __LIMITED_ITEM_UNISEX			__ON
#	define __FAST_TRANSFORTER				__ON
#	define __CLOVER_EVENT					__ON
#	define __QUEST_RENEWAL2					__ON
#	define __GORE_BLOOD_BIBLE				__ON	// Item image by Blood Bible grade
#	define __AN_ADVANCEMENT_GUN_REFLAME		__ON	// Non-advancement Slayer muzzle flash effect (basic, Freezing Vane, Bullet of Light) coordinate fix

#	define __TIPOJYU_CASTLE					__ON
#	define __2008_JUNE_EVENT_EX_STONE		__ON	// Experience stone event (Friday the 13th)
#	define __NEW_ADVANCEMENT_ITEM			__ON	// New advancement item
#	define __TUNING_ITEM					__ON	// Tuning item
#	define __JAPAN_REVERSE_DEVELOP			__ON	// Japan item reverse development
#	define __CHINESE_UI						__OFF	// Apply Chinese font enlargement and UI changes
#	define __JAPAN_UI						__ON	// Japan UI
#	define __NEW_PET_FIRE_ELEMENTAL			__ON	// New market item (Fire Elemental)
#	define __SECOND_TRANSFORTER				__ON	// New transport, second batch
#	define __ADD_MAGICPROTECTION			__ON	// Magic defense added to accessory items (excludes core zap types and advancement items)

#	define __LOCALIZING_LONGNAME			__ON	// Allows long monster names when localizing for Japan
#	define __OX_EVENT						__ON	// OX event
#	define __ITEM_SHOW						__ON	// Item view
#	define __BLITZ_COUPON_EVENT				__ON	// Blitz server coupon event
#	define __TITLE_UI_RENWEAL				__ON	// Title UI renewal (2008.08 Blitz)
#	define __LOGIN_PACKET					__OFF	// Blocked by packet so the client can be downloaded, to prevent speed hacks (2008.09.08)
#	define __PET_FOOD_ITEM_ADD				__ON	// Max Fooder added (2008.09.08)
#	define __DYNAMIC_UI						__OFF
#	define __PREMIUM_GIVE_ITEM_UI			__ON	// Premium granted item UI
#	define __2008_AUTUMN_LEAVES_EVENT		__ON	// Autumn leaves event
#	define __2008_OCTOBER_MAKETITEM			__ON	// October paid item
#	define __PREMIUM_CARD					__ON	// Premium card (split from __INTERNATIONAL_PREMIUM_SYSTEM)
#	define __EXTREME_FORCE_SCROLL			__ON	// Premium granted item "Extreme Force Scroll"
#	define __PREMIUM_LOTTER					__ON	// Premium granted item "Premium Lottery"
#	define __TIPOJYU_CASTLE_ITEM			__ON	// Filter added because item indices sometimes must be added while leaving Tipojyu contents in place; more like it may follow...
#	define __HALLOWEEN						__ON	// Japan Halloween event item; ought to go to the live server too.
#	define __SECOND_TRANSFORTER_ITEM		__ON	// Japan second fast transport - item only, split out as contents
#	define __GLOBAL_NPC						__ON	// NPC summon
#	define __MODIFY_FORCE_SCROLL			__ON	// Allow Odin Force Scroll and Tithuos Force Scroll to stack
#	define __MOVE_ZONE						__ON	// Map warp feature for developer and GM convenience
#	define __PREMIUM_CARD_D					__ON	// Premium Card D added
#	define __NEW_PET_INCUBUS				__ON	// Incubus pet added
#	define __EXPERT_PET_CHANGER				__ON	// Expert Pet Changer added
#	define __PET_VISION_AMPLE				__ON	// Pet item Vision Ampoule
#	define __CHECKTIME_CONTROLL				__ON	// Send time-check packets more often, for speed hack detection.
#	define __PREMIUM_CARD_C					__ON	// Premium Card C - the server had already reserved the empty item slots
#	define __TIME_STAMP						__ON	// Code added to socket output (login may fail because of it). ID: shootkj, date: 2008.12.18
#	define __CHRISTMAS_SOCKS_EVENT			__ON	// Event featuring Christmas stocking pieces
#	define __EVENTTREE_PILE					__ON	// Toggles whether event trees can stack; must be ON whenever the Blitz event is ON.

#	define __HERO_SYSTEM_ITEM				__ON	
#	define __CONTRIBUTE_SYSTEM_ITEM			__ON	
#	define __LEVEL_WAR_RENEWAL_ITEM			__ON	
#	define __MENEGROTH_ITEM					__ON
#	define __LC_SELECT_WORLD_ERROR			__ON	// Packet sent when server selection fails; added for the overseas version
#	define __FUNCTION_PETFOOD				__ON	// Functional pet food
#	define __PCITEMINFO						__ON	// Consolidates the previously scattered pieces so item info goes through PCItemInfo
#	define __REPAIRSYSTEM_RENEWAL			__ON	// Make repair price server-dependent.
#	define __DCGLOBAL_NPC					__ON	// Discounter NPC summon item
#	define __GAMEOPTION_ITEMBAY_MESSAGEBOX	__ON	// Game option for the "sell this item?" confirmation box
#	define __WORLDCHATING					__OFF	// Cross-world chat system
#	define __FUNCTION_PETFOOD_SMART			__ON	// Smart Fooder
#	define __VISION_AMPLE					__ON	// Vision Ampoule
#	define __THIRD_ENCHANT_2				__ON	// Third Enchant 2
#	define __XTRAP							__ON	// Add/remove XTrap; the lib files have to be moved too, of course
#	define __NPROTECT						__OFF	// Add/remove NProtect
#	define __PREMIUM_CARD_ADAY				__ON	// Premium Card A Day - new per-day premium card - 2009.04.28
#	define __PREMIUM_LOTTER_A				__ON	// Premium Lottery A - new lottery card - 2009.04.28
#	define __PREMIUM_LOTTER_B				__ON	// Premium Lottery B - new lottery card - 2009.04.29
#	define __ADVANCEMENT_NEW_UNIQUE_ITEM	__ON	// Advancement unique item - 2009.05.14
#	define __HIDDEN_BOSS_SYSTEM				__ON	// Hidden boss spawn system
#	define __CHANGE_PET_INFO_WINDOW_IMAGE	__ON	// Pet info window image change - 2009.05.21
#	define __PET_MIXINGFORGE				__ON	// Pet combiner and combine canceller
#	define __PCITEMINFO2					__ON   // PCItemInfo merge, second pass; finish it quickly and fold into the first
#	define __ATTACK_SPEED					__ON	// Make attack speed server-dependent
#	define __ONIBLA							__ON	// Onibla development
#	define __FRIEND_ADDITION				__OFF	// Friend list contents - 2009.06.12
#	define __ONIBLA_ITEM					__ON	// Onibla contents item - 2009.06.29
#	define __AUTO_ATTACT					__OFF	// Restrict to auto-attack monsters only.
#	define __IMI_INTERFACE					__OFF	// IMI interface
#	define __DARKEDEN_MARKET				__ON   // Turn off the DarkEden market
#	define __CREDIT							__ON	// Turn off credits
#	define __15_PVP_INHIBITION				__OFF	// Block under-15 users from the PvP server (for the age-12 rating review)
#	define __SUMMER_VACTATION_COOL_EVENT	__ON	// Summer vacation special - Cool Summer event - 2009.07.27
#	define __IMI_LOW_LEVEL_ITEM				__ON	// Low-grade items for IMI - 2009.08.03
#	define __IMI_SENDGLASS					__ON	// Hourglass for IMI
#	define __PET_MASHIMARO					__ON	// Mashimaro
#	define __TREASURE_BOX					__ON	// Treasure box
#	define __PET_CATS						__ON	// New Japan market item, 3 cats - 2009.08.14
#	define __MASTER_EFFECTS					__ON	// Time-limited advancement/non-advancement effect items - 2009.08.24
#	define __IMI_NEW_ENCHANT_ITEM			__ON	// New enchant item for IMI - 2009.08.25
#	define __MOONLIGHT_FESTIVAL_EVENT		__ON	// Moonlight Festival event - 2009.09.15
#	define __TREBLE_PET_SANDGLASS			__ON	// Triple-bonus pet experience hourglass - 2009.09.21
#	define __IMI_MIXNGFORGE					__ON	// Mixing Forge for IMI - 2009.09.21
#	define __NONE_ADVANCEMENT_SANDGLASS		__ON	// General-purpose non-advancement experience hourglass - 2009.09.22
#	define __PET_CATS_UPGRADED				__ON	// Japan market upgrade, 12 cats - 2009.09.21
#	define __UPGRADE_MONSTER_PET			__ON	// Upgraded monster pet - 2009.10.09
#	define __20091028_ADD_ITEM				__ON	// New items added - 2009.10.28
#	define __TAIWAN_INTERFACE				__OFF	// Taiwan interface
#	define __CRIMSON_EVENT					__ON	// Crimson event added - 2009.11.09
#	define __20091116_ADD_RARE_SKILL		__ON	// Rare skill added - 2009.11.16
#	define __20091119_ADD_ENCHANT_ITEM		__ON	// New enchant item - 2009.11.19

#	define __NEW_SKIN_181		__OFF	// New set and Ousters appearance - 2021.08.14
#endif	// __DESIGNED_JAPAN


#ifdef __DESIGNED_ITEMMANIA
#define __DESIGN_NAME "Designed IMI"

#	define __ADVANCEMENT_CHARACTER			__ON
#	define __LANGUAGE_CHANGE				__ON

//#	define __POWER_JJANG_ID					__OFF
#	define __RACE_OUSTERS					__ON	// Taiwan first development. 2009.10.12
#	define __USER_GRADE						__ON
#	define __SMS_SERVICES					__OFF
#	define __RARE_SKILL						__ON

#	define __INTERNATIONAL_PREMIUM_SYSTEM	__OFF
#	define __NEW_SYSTEM_AVAILABILITIES		__OFF
#	define __ENCHANT_REQUIRE_LEVE			__OFF
#	define __ID_PASSWORD					__OFF

#	define __CHAR_DELETE					__ON
#	define __KOREAN_UI						__ON
#	define __INTERNATIONAL_UI				__OFF
#	define __ENGLISH_STRFILTER				__OFF
#	define __CRYPT							__OFF
#	define __CONECT_LIMITER					__OFF
#	define __PERSNALSHOP_NAMING				__ON
#	define __HERO_SYSTEM					__ON
#	define __CONTRIBUTE_SYSTEM				__ON
#	define __LEVEL_WAR_RENEWAL				__ON
#	define __SERVER_SELECT_REVEWAL			__ON
#	define __MENEGROTH						__ON
#	define __GPS_ADD						__ON
#	define __2008_NEW_YEAR_EVENT			__ON
#	define __2008_FIRST_CHARGE_ITEM			__ON	// 2008 first paid item
#	define __DSG_EFFECT						__ON
#	define __GEAR_SWAP_CHANGE				__ON	//								live svr,	test svr,	dev svr
#	define __2008_WHITEDAY_LOVECANDY		__ON	// 2008 White Day item
#	define __080327_ORDINIS_VAPOR			__ON	// 2008 rank experience hourglass				test svr,	dev svr
#	define __PAYZONE_PASS_TICKET			__ON	// Paid zone entry ticket
#	define __QUEST_RENEWAL					__ON	//										test svr,	dev svr
#	define __EFFECT_RENEWAL					__ON	//								live svr,	test svr,	dev svr
#	define __SECOND_RARE_SKILL				__ON	//										test svr,	dev svr
#	define __080405_FIREST_UI_UPDATE		__ON	//										test svr,	dev svr
#	define __LIMITED_ITEM_UNISEX			__ON	
#	define __FAST_TRANSFORTER				__ON	
#	define __CLOVER_EVENT					__ON
#	define __QUEST_RENEWAL2					__ON	// __QUEST_RENEWAL was patched earlier; __QUEST_RENEWAL2 is still being revised.
#	define __GORE_BLOOD_BIBLE				__ON	// Item image by Blood Bible grade
#	define __AN_ADVANCEMENT_GUN_REFLAME		__ON	// Non-advancement Slayer muzzle flash effect (basic, Freezing Vane, Bullet of Light) coordinate fix

#	define __TIPOJYU_CASTLE					__ON
#	define __2008_JUNE_EVENT_EX_STONE		__ON	// Experience stone event (Friday the 13th)
#	define __NEW_ADVANCEMENT_ITEM			__ON	// New advancement item
#	define __TUNING_ITEM					__ON	// Tuning item
#	define __JAPAN_REVERSE_DEVELOP			__OFF	// Japan item reverse development
#	define __CHINESE_UI						__OFF	// Apply Chinese font enlargement and UI changes
#	define __JAPAN_UI						__OFF	// Japan UI
#	define __NEW_PET_FIRE_ELEMENTAL			__ON	// New market item (Fire Elemental)
#	define __SECOND_TRANSFORTER				__ON	// New transport, second batch
#	define __ADD_MAGICPROTECTION			__ON	// Magic defense added to accessory items (excludes core zap types and advancement items)
#	define __AUTO_ITEM_ROOTING				__OFF	// Item auto-looting code (close to test code)
#	define __LOCALIZING_LONGNAME			__OFF	// Allows long monster names when localizing for Japan
#	define __OX_EVENT						__ON	// OX event
#	define __ITEM_SHOW						__ON	// Item view

#	define __BLITZ_COUPON_EVENT				__ON	// Blitz server coupon event
#	define __TITLE_UI_RENWEAL				__ON	// Title UI renewal (2008.08 Blitz)
#	define __LOGIN_PACKET					__OFF	// Blocked by packet so the client can be downloaded, to prevent speed hacks (2008.09.08)
#	define __PET_FOOD_ITEM_ADD				__ON	// Max Fooder added (2008.09.08)
#	define __DYNAMIC_UI						__OFF	// 
#	define __PREMIUM_GIVE_ITEM_UI			__ON	// Premium granted item UI
#	define __2008_AUTUMN_LEAVES_EVENT		__ON	// Autumn leaves event			2008.10.1 patch
#	define __2008_OCTOBER_MAKETITEM			__ON	// October paid item
#	define __PREMIUM_CARD					__ON	// Premium card (split from __INTERNATIONAL_PREMIUM_SYSTEM)
#	define __EXTREME_FORCE_SCROLL			__ON	// Premium granted item "Extreme Force Scroll"
#	define __PREMIUM_LOTTER					__ON	// Premium granted item "Premium Lottery"
#	define __TIPOJYU_CASTLE_ITEM			__ON	// Filter added because item indices sometimes must be added while leaving Tipojyu contents in place; more like it may follow...
#	define __HALLOWEEN						__ON	// Japan Halloween event item; ought to go to the live server too.
#	define __SECOND_TRANSFORTER_ITEM		__ON	// Japan second fast transport - item only, split out as contents
#	define __GLOBAL_NPC						__ON	// NPC summon
#	define __MODIFY_FORCE_SCROLL			__ON	// Allow Odin Force Scroll and Tithuos Force Scroll to stack
#	define __MOVE_ZONE						__ON	// Map warp feature for developer and GM convenience
#	define __PREMIUM_CARD_D					__ON	// Premium Card D added
#	define __NEW_PET_INCUBUS				__ON	// Incubus pet added
#	define __EXPERT_PET_CHANGER				__ON	// Expert Pet Changer added
#	define __PET_VISION_AMPLE				__ON	// Pet item Vision Ampoule
#	define __CHECKTIME_CONTROLL				__ON	// Send time-check packets more often, for speed hack detection.
#	define __PREMIUM_CARD_C					__ON	// Premium Card C - the server had already reserved the empty item slots
#	define __TIME_STAMP						__ON	// Code added to socket output (login may fail because of it). ID: shootkj, date: 2008.12.18
#	define __CHRISTMAS_SOCKS_EVENT			__ON	// Event featuring Christmas stocking pieces
#	define __EVENTTREE_PILE					__ON	// Toggles whether event trees can stack; must be ON whenever the Blitz event is ON.

#	define __HERO_SYSTEM_ITEM				__ON	
#	define __CONTRIBUTE_SYSTEM_ITEM			__ON	
#	define __LEVEL_WAR_RENEWAL_ITEM			__ON	
#	define __MENEGROTH_ITEM					__ON
#	define __LC_SELECT_WORLD_ERROR			__ON	// Packet sent when server selection fails; added for the overseas version
#	define __FUNCTION_PETFOOD				__ON	// Functional pet food
#	define __PCITEMINFO						__ON	// Consolidates the previously scattered pieces so item info goes through PCItemInfo

#	define __REPAIRSYSTEM_RENEWAL			__ON	// Make repair price server-dependent.
#	define __DCGLOBAL_NPC					__ON	// Discounter NPC summon item
#	define __GAMEOPTION_ITEMBAY_MESSAGEBOX	__ON	// Game option for the "sell this item?" confirmation box
#	define __WORLDCHATING					__ON	// Cross-world chat system
#	define __FUNCTION_PETFOOD_SMART			__ON	// Smart Fooder
#	define __VISION_AMPLE					__ON	// Vision Ampoule
#	define __THIRD_ENCHANT_2				__ON	// Third Enchant 2
#	define __XTRAP							__OFF	// Add/remove XTrap; the lib files have to be moved too, of course
#	define __NPROTECT						__ON	// Add/remove NProtect
#	define __PREMIUM_CARD_ADAY				__ON	// Premium Card A Day - new per-day premium card - 2009.04.28
#	define __PREMIUM_LOTTER_A				__ON	// Premium Lottery A - new lottery card - 2009.04.28
#	define __PREMIUM_LOTTER_B				__ON	// Premium Lottery B - new lottery card - 2009.04.29
#	define __ADVANCEMENT_NEW_UNIQUE_ITEM	__ON	// Advancement unique item - 2009.05.14
#	define __HIDDEN_BOSS_SYSTEM				__ON	// Hidden boss spawn system
#	define __CHANGE_PET_INFO_WINDOW_IMAGE	__ON	// Pet info window image change - 2009.05.21
#	define __PET_MIXINGFORGE				__ON	// Pet combiner and combine canceller
#	define __PCITEMINFO2					__ON	// PCItemInfo merge, second pass; finish it quickly and fold into the first
#	define __ATTACK_SPEED					__ON	// Make attack speed server-dependent
#	define __ONIBLA							__ON	// Onibla development
#	define __FRIEND_ADDITION				__ON	// Friend list contents - 2009.06.12
#	define __ONIBLA_ITEM					__ON	// Onibla contents item - 2009.06.29
#	define __AUTO_ATTACT					__ON	// Restrict to auto-attack monsters only.
#	define __IMI_INTERFACE					__ON	// IMI interface
#	define __DARKEDEN_MARKET				__OFF   // Turn off the DarkEden market
#	define __CREDIT							__OFF	// Turn off credits
#	define __15_PVP_INHIBITION				__OFF	// Block under-15 users from the PvP server (for the age-12 rating review)
#	define __SUMMER_VACTATION_COOL_EVENT	__ON	// Summer vacation special - Cool Summer event - 2009.07.27
#	define __IMI_LOW_LEVEL_ITEM				__ON	// Low-grade items for IMI - 2009.08.03
#	define __IMI_SENDGLASS					__ON	// Hourglass for IMI
#	define __PET_MASHIMARO					__ON	// Mashimaro
#	define __TREASURE_BOX					__ON	// Treasure box
#	define __PET_CATS						__ON	// New Japan market item, 3 cats - 2009.08.14
#	define __MASTER_EFFECTS					__ON	// Time-limited advancement/non-advancement effect items - 2009.08.24
#	define __IMI_NEW_ENCHANT_ITEM			__ON	// New enchant item for IMI - 2009.08.25
#	define __MOONLIGHT_FESTIVAL_EVENT		__ON	// Moonlight Festival event - 2009.09.15
#	define __TREBLE_PET_SANDGLASS			__ON	// Triple-bonus pet experience hourglass - 2009.09.21
#	define __IMI_MIXNGFORGE					__ON	// Mixing Forge for IMI - 2009.09.21
#	define __NONE_ADVANCEMENT_SANDGLASS		__ON	// General-purpose non-advancement experience hourglass - 2009.09.22
#	define __PET_CATS_UPGRADED				__ON	// Japan market upgrade, 12 cats - 2009.09.21
#	define __UPGRADE_MONSTER_PET			__ON	// Upgraded monster pet - 2009.10.09
#	define __20091028_ADD_ITEM				__ON	// New items added - 2009.10.28
#	define __TAIWAN_INTERFACE				__OFF	// Taiwan interface
#	define __CRIMSON_EVENT					__ON	// Crimson event added - 2009.11.09
#	define __20091116_ADD_RARE_SKILL		__ON	// Rare skill added - 2009.11.16
#	define __20091119_ADD_ENCHANT_ITEM		__ON	// New enchant item - 2009.11.19

#	define __NEW_SKIN_181		__OFF	// New set and Ousters appearance - 2021.08.14
#endif	// __DESIGNED_ITEMMANIA



//////////////////////////////////////////////////////////////////////////
// Taiwan

#ifdef __DESIGNED_TAIWAN
#define __DESIGN_NAME "Designed Taiwan"

#	define __ADVANCEMENT_CHARACTER			__ON
#	define __LANGUAGE_CHANGE				__ON

//#	define __POWER_JJANG_ID					__OFF
#	define __RACE_OUSTERS					__OFF	// Taiwan first development. 2009.10.12
#	define __USER_GRADE						__OFF
#	define __SMS_SERVICES					__OFF
#	define __RARE_SKILL						__ON

#	define __INTERNATIONAL_PREMIUM_SYSTEM	__OFF
#	define __NEW_SYSTEM_AVAILABILITIES		__OFF
#	define __ENCHANT_REQUIRE_LEVE			__OFF
#	define __ID_PASSWORD					__OFF

#	define __CHAR_DELETE					__ON
#	define __KOREAN_UI						__OFF
#	define __INTERNATIONAL_UI				__OFF
#	define __ENGLISH_STRFILTER				__OFF
#	define __CRYPT							__OFF
#	define __CONECT_LIMITER					__OFF
#	define __PERSNALSHOP_NAMING				__ON
#	define __HERO_SYSTEM					__ON
#	define __CONTRIBUTE_SYSTEM				__ON
#	define __LEVEL_WAR_RENEWAL				__ON
#	define __SERVER_SELECT_REVEWAL			__ON
#	define __MENEGROTH						__ON
#	define __GPS_ADD						__ON
#	define __2008_NEW_YEAR_EVENT			__ON
#	define __2008_FIRST_CHARGE_ITEM			__ON	// 2008 first paid item
#	define __DSG_EFFECT						__ON
#	define __GEAR_SWAP_CHANGE				__ON	//								live svr,	test svr,	dev svr
#	define __2008_WHITEDAY_LOVECANDY		__ON	// 2008 White Day item
#	define __080327_ORDINIS_VAPOR			__ON	// 2008 rank experience hourglass				test svr,	dev svr
#	define __PAYZONE_PASS_TICKET			__ON	// Paid zone entry ticket
#	define __QUEST_RENEWAL					__ON	//										test svr,	dev svr
#	define __EFFECT_RENEWAL					__ON	//								live svr,	test svr,	dev svr
#	define __SECOND_RARE_SKILL				__ON	//										test svr,	dev svr
#	define __080405_FIREST_UI_UPDATE		__ON	//										test svr,	dev svr
#	define __LIMITED_ITEM_UNISEX			__ON	
#	define __FAST_TRANSFORTER				__ON	
#	define __CLOVER_EVENT					__ON
#	define __QUEST_RENEWAL2					__ON	// __QUEST_RENEWAL was patched earlier; __QUEST_RENEWAL2 is still being revised.
#	define __GORE_BLOOD_BIBLE				__ON	// Item image by Blood Bible grade
#	define __AN_ADVANCEMENT_GUN_REFLAME		__ON	// Non-advancement Slayer muzzle flash effect (basic, Freezing Vane, Bullet of Light) coordinate fix

#	define __TIPOJYU_CASTLE					__ON
#	define __2008_JUNE_EVENT_EX_STONE		__ON	// Experience stone event (Friday the 13th)
#	define __NEW_ADVANCEMENT_ITEM			__ON	// New advancement item
#	define __TUNING_ITEM					__ON	// Tuning item
#	define __JAPAN_REVERSE_DEVELOP			__OFF	// Japan item reverse development
#	define __CHINESE_UI						__ON	// Apply Chinese font enlargement and UI changes
#	define __JAPAN_UI						__OFF	// Japan UI
#	define __NEW_PET_FIRE_ELEMENTAL			__ON	// New market item (Fire Elemental)
#	define __SECOND_TRANSFORTER				__ON	// New transport, second batch
#	define __ADD_MAGICPROTECTION			__ON	// Magic defense added to accessory items (excludes core zap types and advancement items)
#	define __AUTO_ITEM_ROOTING				__OFF	// Item auto-looting code (close to test code)
#	define __LOCALIZING_LONGNAME			__OFF	// Allows long monster names when localizing for Japan
#	define __OX_EVENT						__ON	// OX event
#	define __ITEM_SHOW						__ON	// Item view

#	define __BLITZ_COUPON_EVENT				__ON	// Blitz server coupon event
#	define __TITLE_UI_RENWEAL				__ON	// Title UI renewal (2008.08 Blitz)
#	define __LOGIN_PACKET					__OFF	// Blocked by packet so the client can be downloaded, to prevent speed hacks (2008.09.08)
#	define __PET_FOOD_ITEM_ADD				__ON	// Max Fooder added (2008.09.08)
#	define __DYNAMIC_UI						__OFF	// 
#	define __PREMIUM_GIVE_ITEM_UI			__ON	// Premium granted item UI
#	define __2008_AUTUMN_LEAVES_EVENT		__ON	// Autumn leaves event			2008.10.1 patch
#	define __2008_OCTOBER_MAKETITEM			__ON	// October paid item
#	define __PREMIUM_CARD					__ON	// Premium card (split from __INTERNATIONAL_PREMIUM_SYSTEM)
#	define __EXTREME_FORCE_SCROLL			__ON	// Premium granted item "Extreme Force Scroll"
#	define __PREMIUM_LOTTER					__ON	// Premium granted item "Premium Lottery"
#	define __TIPOJYU_CASTLE_ITEM			__ON	// Filter added because item indices sometimes must be added while leaving Tipojyu contents in place; more like it may follow...
#	define __HALLOWEEN						__ON	// Japan Halloween event item; ought to go to the live server too.
#	define __SECOND_TRANSFORTER_ITEM		__ON	// Japan second fast transport - item only, split out as contents
#	define __GLOBAL_NPC						__ON	// NPC summon
#	define __MODIFY_FORCE_SCROLL			__ON	// Allow Odin Force Scroll and Tithuos Force Scroll to stack
#	define __MOVE_ZONE						__ON	// Map warp feature for developer and GM convenience
#	define __PREMIUM_CARD_D					__ON	// Premium Card D added
#	define __NEW_PET_INCUBUS				__ON	// Incubus pet added
#	define __EXPERT_PET_CHANGER				__ON	// Expert Pet Changer added
#	define __PET_VISION_AMPLE				__ON	// Pet item Vision Ampoule
#	define __CHECKTIME_CONTROLL				__ON	// Send time-check packets more often, for speed hack detection.
#	define __PREMIUM_CARD_C					__ON	// Premium Card C - the server had already reserved the empty item slots
#	define __TIME_STAMP						__ON	// Code added to socket output (login may fail because of it). ID: shootkj, date: 2008.12.18
#	define __CHRISTMAS_SOCKS_EVENT			__ON	// Event featuring Christmas stocking pieces
#	define __EVENTTREE_PILE					__ON	// Toggles whether event trees can stack; must be ON whenever the Blitz event is ON.

#	define __HERO_SYSTEM_ITEM				__ON	
#	define __CONTRIBUTE_SYSTEM_ITEM			__ON	
#	define __LEVEL_WAR_RENEWAL_ITEM			__ON	
#	define __MENEGROTH_ITEM					__ON
#	define __LC_SELECT_WORLD_ERROR			__ON	// Packet sent when server selection fails; added for the overseas version
#	define __FUNCTION_PETFOOD				__ON	// Functional pet food
#	define __PCITEMINFO						__ON	// Consolidates the previously scattered pieces so item info goes through PCItemInfo

#	define __REPAIRSYSTEM_RENEWAL			__ON	// Make repair price server-dependent.
#	define __DCGLOBAL_NPC					__ON	// Discounter NPC summon item
#	define __GAMEOPTION_ITEMBAY_MESSAGEBOX	__ON	// Game option for the "sell this item?" confirmation box
#	define __WORLDCHATING					__ON	// Cross-world chat system
#	define __FUNCTION_PETFOOD_SMART			__ON	// Smart Fooder
#	define __VISION_AMPLE					__ON	// Vision Ampoule
#	define __THIRD_ENCHANT_2				__ON	// Third Enchant 2
#	define __XTRAP							__OFF	// Add/remove XTrap; the lib files have to be moved too, of course
#	define __NPROTECT						__ON	// Add/remove NProtect
#	define __PREMIUM_CARD_ADAY				__ON	// Premium Card A Day - new per-day premium card - 2009.04.28
#	define __PREMIUM_LOTTER_A				__ON	// Premium Lottery A - new lottery card - 2009.04.28
#	define __PREMIUM_LOTTER_B				__ON	// Premium Lottery B - new lottery card - 2009.04.29
#	define __ADVANCEMENT_NEW_UNIQUE_ITEM	__ON	// Advancement unique item - 2009.05.14
#	define __HIDDEN_BOSS_SYSTEM				__ON	// Hidden boss spawn system
#	define __CHANGE_PET_INFO_WINDOW_IMAGE	__ON	// Pet info window image change - 2009.05.21
#	define __PET_MIXINGFORGE				__ON	// Pet combiner and combine canceller
#	define __PCITEMINFO2					__ON	// PCItemInfo merge, second pass; finish it quickly and fold into the first
#	define __ATTACK_SPEED					__ON	// Make attack speed server-dependent
#	define __ONIBLA							__ON	// Onibla development
#	define __FRIEND_ADDITION				__ON	// Friend list contents - 2009.06.12
#	define __ONIBLA_ITEM					__ON	// Onibla contents item - 2009.06.29
#	define __AUTO_ATTACT					__OFF	// Restrict to auto-attack monsters only.
#	define __IMI_INTERFACE					__OFF	// IMI interface
#	define __DARKEDEN_MARKET				__ON   // Turn off the DarkEden market
#	define __CREDIT							__ON	// Turn off credits
#	define __15_PVP_INHIBITION				__OFF	// Block under-15 users from the PvP server (for the age-12 rating review)
#	define __SUMMER_VACTATION_COOL_EVENT	__ON	// Summer vacation special - Cool Summer event - 2009.07.27
#	define __IMI_LOW_LEVEL_ITEM				__ON	// Low-grade items for IMI - 2009.08.03
#	define __IMI_SENDGLASS					__ON	// Hourglass for IMI
#	define __PET_MASHIMARO					__ON	// Mashimaro
#	define __TREASURE_BOX					__ON	// Treasure box
#	define __PET_CATS						__ON	// New Japan market item, 3 cats - 2009.08.14
#	define __MASTER_EFFECTS					__ON	// Time-limited advancement/non-advancement effect items - 2009.08.24
#	define __IMI_NEW_ENCHANT_ITEM			__ON	// New enchant item for IMI - 2009.08.25
#	define __MOONLIGHT_FESTIVAL_EVENT		__ON	// Moonlight Festival event - 2009.09.15
#	define __TREBLE_PET_SANDGLASS			__ON	// Triple-bonus pet experience hourglass - 2009.09.21
#	define __IMI_MIXNGFORGE					__ON	// Mixing Forge for IMI - 2009.09.21
#	define __NONE_ADVANCEMENT_SANDGLASS		__ON	// General-purpose non-advancement experience hourglass - 2009.09.22
#	define __PET_CATS_UPGRADED				__ON	// Japan market upgrade, 12 cats - 2009.09.21
#	define __20091028_ADD_ITEM				__ON	// New items added - 2009.10.28
#	define __TAIWAN_INTERFACE				__ON	// Taiwan interface
#	define __CRIMSON_EVENT					__ON	// Crimson event added - 2009.11.09
#	define __20091116_ADD_RARE_SKILL		__ON	// Rare skill added - 2009.11.16
#	define __20091119_ADD_ENCHANT_ITEM		__ON	// New enchant item - 2009.11.19

#	define __NEW_SKIN_181		__OFF	// New set and Ousters appearance - 2021.08.14
#endif	// __DESIGNED_TAIWAN




//////////////////////////////////////////////////////////////////////////
// Macro

// If a macro is not defined, treat it as __ON.
#define __CONTENTS(f)	((f) == __ON)


/*#ifdef __DESIGN_NAME
#	pragma message ("Darkeden project \"" __DESIGN_NAME "\"")
#else
#	pragma message ("Darkeden design macro undefined")
#endif/*/
#undef __DESIGN_NAME
