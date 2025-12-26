/* DDNet++ shop */

#include "shop.h"

#include "../gamecontext.h"

#include <generated/protocol.h>

CShopItem::CShopItem(
	const char *pName,
	const char *pPrice,
	int Level,
	const char *pDescription,
	const char *pOwnedUntil,
	CGameContext *pGameContext) :
	m_pGameContext(pGameContext),
	m_NeededLevel(Level)
{
	m_aTitle[0] = '\0';
	str_copy(m_aName, pName, sizeof(m_aName));
	str_copy(m_aPriceStr, pPrice, sizeof(m_aPriceStr));
	str_copy(m_aDescription, pDescription, sizeof(m_aDescription));
	str_copy(m_aOwnUntil, pOwnedUntil, sizeof(m_aOwnUntil));
	Title(); // trigger length check assert on server start
	m_Active = true;
}

CGameContext *CShopItem::GameServer()
{
	return m_pGameContext;
}

int CShopItem::Price(int ClientId)
{
	char aPrice[64] = {0};
	int i = 0;
	for(int k = 0; k < str_length(PriceStr(ClientId)); k++)
	{
		char c = PriceStr(ClientId)[k];
		if(c < '0' || c > '9')
			continue;
		aPrice[i++] = c;
		++c;
	}
	m_Price = atoi(aPrice);
	return m_Price;
}

const char *CShopItem::NeededLevelStr(int ClientId)
{
	str_format(m_aNeededLevelStr, sizeof(m_aNeededLevelStr), "%d", NeededLevel(ClientId));
	return m_aNeededLevelStr;
}

#define MAX_TITLE_LEN 36

const char *CShopItem::Title()
{
	// only compute title once because names do not change
	if(m_aTitle[0] != '\0')
		return m_aTitle;

	int NameLen = str_length(m_aName) * 2 + 4;
	dbg_assert(NameLen, "shop item name too long to generate title");
	int Padding = (MAX_TITLE_LEN - NameLen) / 2;
	mem_zero(m_aTitle, sizeof(m_aTitle));
	int i = 0;
	for(i = 0; i < Padding; i++)
		m_aTitle[i] = ' ';
	m_aTitle[i++] = '~';
	m_aTitle[i++] = ' ';
	for(int k = 0; k < str_length(m_aName); k++)
	{
		char c = m_aName[k];
		if(c == '\0')
			break;
		if(c == '_')
			m_aTitle[i++] = ' ';
		else
			m_aTitle[i++] = str_uppercase(c);
		m_aTitle[i++] = ' ';
	}
	m_aTitle[i++] = '~';
	return m_aTitle;
}

const char *CShopItem::OwnUntilLong()
{
	if(!str_comp(m_aOwnUntil, "dead"))
		return "You own this item until you die";
	if(!str_comp(m_aOwnUntil, "disconnect"))
		return "You own this item until\n"
		       "   you disconnect";
	if(!str_comp(m_aOwnUntil, "forever"))
		return "You own this item forever";
	return "";
}

bool CShopItem::CanBuy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	char aBuf[128];
	if(pPlayer->GetLevel() < NeededLevel(ClientId))
	{
		str_format(aBuf, sizeof(aBuf), "Для покупки тебе нужен уровень %d для '%s'", NeededLevel(ClientId), Name());
		GameServer()->SendChatTarget(ClientId, aBuf);
		return false;
	}
	if(pPlayer->m_Account.m_Money < Price(ClientId))
	{
		str_format(aBuf, sizeof(aBuf), "У тебя не достаточно денег! Нужно %s монет.", PriceStr(ClientId));
		GameServer()->SendChatTarget(ClientId, aBuf);
		return false;
	}
	return true;
}

bool CShopItem::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(!CanBuy(ClientId))
		return false;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "bought '%s'", Name());
	pPlayer->MoneyTransaction(-Price(ClientId), aBuf);
	str_format(aBuf, sizeof(aBuf), "You bought %s!", Name());
	GameServer()->SendChatTarget(ClientId, aBuf);
	return true;
}

IServer *CShop::Server()
{
	return m_pGameContext->Server();
}

void CShop::OnInit()
{
	m_vItems.push_back(new CShopItemShotgun(
		"shotgun",
		"1 000",
		5,
		"Дает тебе шотган.\n",
		"dead",
		m_pGameContext));
	m_vItems.push_back(new CShopItemGrenade(
		"grenade",
		"1 000",
		5,
		"Дает тебе ракетницу.\n",
		"dead",
		m_pGameContext));
	m_vItems.push_back(new CShopItemLaser(
		"laser",
		"1 000",
		5,
		"Дает тебе лазер.\n",
		"dead",
		m_pGameContext));
	m_vItems.push_back(new CShopItemNinja(
		"ninja",
		"1 000",
		5,
		"Дает тебе нидзю.\n",
		"dead",
		m_pGameContext));
}

bool CShopItemRainbow::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return false;
	if(!CShopItem::CanBuy(ClientId))
		return false;
	CShopItem::Buy(ClientId);
	pChr->m_Rainbow = true;
	return true;
}

bool CShopItemBloody::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return false;
	if(!CShopItem::Buy(ClientId))
		return false;
	pChr->m_Bloody = true;
	return true;
}

bool CShopItemChidraqul::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(!CShopItem::Buy(ClientId))
		return false;
	if(pPlayer->m_BoughtGame)
	{
		GameServer()->SendChatLoc(ClientId, "You already own this item.");
		return false;
	}
	pPlayer->m_BoughtGame = true;
	return true;
}

bool CShopItemShit::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_Shit++;
	return true;
}

bool CShopItemRoomKey::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_BoughtRoom)
	{
		GameServer()->SendChatTarget(ClientId, "You already own this item.");
		return false;
	}
	if(g_Config.m_SvRoomState == 0)
	{
		GameServer()->SendChatTarget(ClientId, "Room has been turned off by admin.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_BoughtRoom = true;
	return true;
}

const char *CShopItemRoomKey::PriceStr(int ClientId)
{
	str_copy(m_aPriceStr, g_Config.m_SvRoomPrice, sizeof(m_aPriceStr));
	return m_aPriceStr;
}

bool CShopItemPolice::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_PoliceRank > 2)
	{
		GameServer()->SendChatTarget(ClientId, "You already bought maximum police level.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_PoliceRank++;
	return true;
}

int CShopItemPolice::NeededLevel(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return 18;
	switch(pPlayer->m_Account.m_PoliceRank)
	{
	case 0:
		return 18;
	case 1:
		return 25;
	case 2:
		return 30;
	case 3:
		return 40;
	case 4:
		return 50;
	}
	return 18;
}

bool CShopItemTaser::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_TaserLevel > 6)
	{
		GameServer()->SendChatTarget(ClientId, "Taser already max level.");
		return false;
	}
	if(pPlayer->m_Account.m_PoliceRank < NeededPoliceRank(ClientId))
	{
		GameServer()->SendChatTarget(ClientId, "You don't have a weapon license.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_TaserLevel++;
	if(pPlayer->m_Account.m_TaserLevel == 1)
		GameServer()->SendChatTarget(ClientId, "Type '/taser help' for all cmds");
	else
		GameServer()->SendChatTarget(ClientId, "Taser has been upgraded.");
	return true;
}

const char *CShopItemTaser::PriceStr(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return 0;
	switch(pPlayer->m_Account.m_TaserLevel)
	{
	case 0:
		str_copy(m_aPriceStr, "50 000", sizeof(m_aPriceStr));
		break;
	case 1:
		str_copy(m_aPriceStr, "75 000", sizeof(m_aPriceStr));
		break;
	case 2:
		str_copy(m_aPriceStr, "100 000", sizeof(m_aPriceStr));
		break;
	case 3:
		str_copy(m_aPriceStr, "150 000", sizeof(m_aPriceStr));
		break;
	case 4:
		str_copy(m_aPriceStr, "200 000", sizeof(m_aPriceStr));
		break;
	case 5:
		str_copy(m_aPriceStr, "200 000", sizeof(m_aPriceStr));
		break;
	case 6:
		str_copy(m_aPriceStr, "200 000", sizeof(m_aPriceStr));
		break;
	default: // max level
		str_copy(m_aPriceStr, "max", sizeof(m_aPriceStr));
		break;
	}
	return m_aPriceStr;
}

int CShopItemTaser::NeededPoliceRank(int ClientId)
{
	return 3;
}

const char *CShopItemTaser::NeededLevelStr(int ClientId)
{
	str_format(m_aNeededPoliceRank, sizeof(m_aNeededPoliceRank), "Police[%d]", NeededPoliceRank(ClientId));
	return m_aNeededPoliceRank;
}

bool CShopItemPvpArenaTicket::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_PvpArenaTickets++;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "You bought a pvp_arena_ticket. You have %d tickets.", pPlayer->m_Account.m_PvpArenaTickets);
	GameServer()->SendChatTarget(ClientId, aBuf);
	return true;
}

bool CShopItemNinjaJetpack::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_NinjaJetpackBought)
	{
		GameServer()->SendChatTarget(ClientId, "You bought ninjajetpack. Turn it on using '/ninjajetpack'.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_NinjaJetpackBought = true;
	return true;
}

bool CShopItemSpawnShotgun::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpawnWeaponShotgun >= 5)
	{
		GameServer()->SendChatLoc(ClientId, "You already have maximum level for spawn shotgun.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_SpawnWeaponShotgun++;
	GameServer()->SetSpawnweapons(true, ClientId);
	return true;
}

bool CShopItemSpawnGrenade::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpawnWeaponGrenade >= 5)
	{
		GameServer()->SendChatLoc(ClientId, "You already have maximum level for spawn grenade.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_SpawnWeaponGrenade++;
	GameServer()->SetSpawnweapons(true, ClientId);
	return true;
}

bool CShopItemSpawnRifle::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpawnWeaponRifle >= 5)
	{
		GameServer()->SendChatLoc(ClientId, "You already have maximum level for spawn rifle.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_SpawnWeaponRifle++;
	GameServer()->SetSpawnweapons(true, ClientId);
	return true;
}

bool CShopItemSpookyGhost::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	if(pPlayer->m_Account.m_SpookyGhost >= 5)
	{
		GameServer()->SendChatLoc(ClientId, "You already have spooky ghost.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pPlayer->m_Account.m_SpookyGhost++;
	return true;
}

bool CShopItemWeapon::Buy(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
	{
		GameServer()->SendChatLoc(ClientId, "You have to be alive to buy this item.");
		return false;
	}
	if(!CShopItem::Buy(ClientId))
		return false;
	pChr->GiveWeapon(Weapon());
	return true;
}

CShop::CShop(CGameContext *pGameContext)
{
	m_pGameContext = pGameContext;
	mem_zero(m_MotdTick, sizeof(m_MotdTick));
	mem_zero(m_Page, sizeof(m_Page));
	mem_zero(m_PurchaseState, sizeof(m_PurchaseState));
	mem_zero(m_ChangePage, sizeof(m_ChangePage));
	mem_zero(m_InShop, sizeof(m_InShop));
	OnInit();
}

CShop::~CShop()
{
	for(auto &Item : m_vItems)
		delete Item;
}

CGameContext *CShop::GameServer()
{
	return m_pGameContext;
}

void CShop::ShowShopMotdCompressed(int ClientId)
{
	char aBuf[2048];
	str_format(aBuf,
		sizeof(aBuf),
		"***************************\n"
		"        ~  %s  ~      \n" // S H O P
		"***************************\n"
		"%s: '/buy (%s)'\n" // usage: /buy itemname
		"***************************\n"
		"%s | %s | %s | %s\n" // Item | Price | Level | Owned until
		"-------+------+--------+-------\n",
		GameServer()->Loc("МАГАЗИН", ClientId),
		GameServer()->Loc("usage", ClientId),
		GameServer()->Loc("Название", ClientId),
		GameServer()->Loc("Item", ClientId),
		GameServer()->Loc("Цена", ClientId),
		GameServer()->Loc("Лвл", ClientId),
		GameServer()->Loc("Owned until", ClientId));
	for(auto &Item : m_vItems)
	{
		if(!Item->IsActive())
			continue;

		char aItem[128];
		str_format(
			aItem,
			sizeof(aItem),
			"%s | %s | %s | %s\n",
			Item->Name(),
			Item->PriceStr(ClientId),
			Item->NeededLevelStr(ClientId),
			Item->OwnUntil());
		str_append(aBuf, aItem, sizeof(aBuf));
	}
	m_pGameContext->AbuseMotd(aBuf, ClientId);
}

void CShop::MotdTick(int ClientId)
{
	if(m_MotdTick[ClientId] < Server()->Tick())
	{
		m_Page[ClientId] = -1;
		m_PurchaseState[ClientId] = 0;
	}
}

void CShop::WillFireWeapon(int ClientId)
{
	if((m_Page[ClientId] != -1) && (m_PurchaseState[ClientId] == 1))
	{
		m_ChangePage[ClientId] = true;
	}
}

void CShop::FireWeapon(int Dir, int ClientId)
{
	if((m_ChangePage[ClientId]) && (m_Page[ClientId] != -1) && (m_PurchaseState[ClientId] == 1))
	{
		ShopWindow(Dir, ClientId);
		m_ChangePage[ClientId] = false;
	}
}

void CShop::LeaveShop(int ClientId)
{
	if(m_Page[ClientId] != -1)
		m_pGameContext->AbuseMotd("", ClientId);
	m_PurchaseState[ClientId] = 0;
	m_Page[ClientId] = -1;
	m_InShop[ClientId] = false;
}

void CShop::OnOpenScoreboard(int ClientId)
{
	m_MotdTick[ClientId] = 0;
}

void CShop::StartShop(int ClientId)
{
	if(!IsInShop(ClientId))
		return;
	if(m_PurchaseState[ClientId] == 2) // already in buy confirmation state
		return;
	if(m_Page[ClientId] != -1)
		return;

	ShopWindow(0, ClientId);
	m_PurchaseState[ClientId] = 1;
}

void CShop::ConfirmPurchase(int ClientId)
{
	if((m_Page[ClientId] == -1) || (m_Page[ClientId] == 0))
		return;

	char aBuf[256];
	str_format(aBuf,
		sizeof(aBuf),
		"***************************\n"
		"        ~  %s  ~      \n" // S H O P
		"***************************\n\n"
		"%s\n\n" // Are you sure you want to buy this item?
		"f3 - %s\n" // yes
		"f4 - %s\n\n" // no
		"***************************\n",
		GameServer()->Loc("МАГАЗИН", ClientId),
		GameServer()->Loc("Ты уверен, что хочешь купить этот товар?", ClientId),
		GameServer()->Loc("да", ClientId),
		GameServer()->Loc("нет", ClientId));

	m_pGameContext->AbuseMotd(aBuf, ClientId);
	m_PurchaseState[ClientId] = 2;
}

void CShop::PurchaseEnd(int ClientId, bool IsCancel)
{
	if(m_PurchaseState[ClientId] != 2) // nothing to end here
		return;

	char aResult[256];
	if(IsCancel)
	{
		char aBuf[256];
		str_copy(aResult, "You canceled the purchase.", sizeof(aResult));
		str_format(aBuf, sizeof(aBuf),
			"***************************\n"
			"        ~  %s  ~           \n" // S H O P
			"***************************\n\n"
			"%s\n\n"
			"***************************\n",
			GameServer()->Loc("МАГАЗИН", ClientId),
			aResult);

		m_pGameContext->AbuseMotd(aBuf, ClientId);
	}
	else
	{
		// TODO: performance go brrr make this take an id instead
		BuyItem(ClientId, GetItemNameById(m_Page[ClientId] - 1));
		ShopWindow(ClientId, 0);
	}

	m_PurchaseState[ClientId] = 1;
}

bool CShop::VoteNo(int ClientId)
{
	if(!IsInShop(ClientId))
		return false;

	if(m_PurchaseState[ClientId] == 2)
		PurchaseEnd(ClientId, true);
	else if(m_Page[ClientId] == -1)
		StartShop(ClientId);
	return true;
}

bool CShop::VoteYes(int ClientId)
{
	if(!IsInShop(ClientId))
		return false;

	if(m_PurchaseState[ClientId] == 1)
		ConfirmPurchase(ClientId);
	else if(m_PurchaseState[ClientId] == 2)
		PurchaseEnd(ClientId, false);
	return true;
}

const char *CShop::GetItemNameById(int ItemId)
{
	int Id = 0; // upper camel looks weird on id :D
	for(auto &Item : m_vItems)
		if(Item->IsActive())
			if(Id++ == ItemId)
				return Item->Name();
	return NULL;
}

int CShop::NumShopItems()
{
	int Total = 0;
	for(auto &Item : m_vItems)
		if(Item->IsActive())
			Total++;
	return Total;
}

void CShop::ShopWindow(int Dir, int ClientId)
{
	// motd is there for 10 sec
	m_MotdTick[ClientId] = Server()->Tick() + Server()->TickSpeed() * 10;

	// all active items plus the start page
	int MaxShopPages = NumShopItems();

	if(Dir == 0)
	{
		m_Page[ClientId] = 0;
	}
	else if(Dir == 1)
	{
		m_Page[ClientId]++;
		if(m_Page[ClientId] > MaxShopPages)
		{
			m_Page[ClientId] = 0;
		}
	}
	else if(Dir == -1)
	{
		m_Page[ClientId]--;
		if(m_Page[ClientId] < 0)
		{
			m_Page[ClientId] = MaxShopPages;
		}
	}

	char aMotd[2048];

	if(m_Page[ClientId] == 0)
	{
		str_format(aMotd,
			sizeof(aMotd),
			"***************************\n"
			"        ~  %s  ~           \n" // S H O P
			"***************************\n\n"
			"%s"
			"\n\n"
			"***************************\n"
			"%s", // If you want to buy an item press f3.
			GameServer()->Loc("S H O P", ClientId),
			GameServer()->Loc(
				"Добро пожаловать в магазин!'.\n\n"
				"Стреляя вправо, ты листаешь предметы вперед,\n"
				"и стреляя влево, ты возвращаешься на прошлый предмет назад.\n\n"
				"Хороших покупок!.",
				ClientId),
			GameServer()->Loc("Если ты хочешь купить какой-либо товар нажимай F3.", ClientId));
		m_pGameContext->AbuseMotd(aMotd, ClientId);
		return;
	}
	int ItemIndex = 0;
	for(auto &Item : m_vItems)
	{
		if(!Item->IsActive())
			continue;
		if(++ItemIndex != m_Page[ClientId])
			continue;

		str_format(
			aMotd,
			sizeof(aMotd),
			"***************************\n"
			"        ~  %s  ~           \n" // S H O P
			"***************************\n\n"
			"%s\n\n" // title
			"%s: %s\n" // Needed level
			"%s: %s\n" // Price
			"%s: %s\n\n" // Time
			"%s\n\n" // description
			"***************************\n"
			"%s\n\n\n" // If you want to buy an item press f3.
			"              ~ %d/%d ~              ",
			GameServer()->Loc("МАГАЗИН", ClientId),
			Item->Title(),
			GameServer()->Loc("Нужный лвл", ClientId),
			Item->NeededLevelStr(ClientId),
			GameServer()->Loc("Цена", ClientId),
			Item->PriceStr(ClientId),
			GameServer()->Loc("Время", ClientId),
			Item->OwnUntilLong(),
			Item->Description(),
			GameServer()->Loc("Если ты хочешь купить какой-либо товар нажимай F3.", ClientId),
			m_Page[ClientId], MaxShopPages);
	}
	m_pGameContext->AbuseMotd(aMotd, ClientId);
}

void CShop::BuyItem(int ClientId, const char *pItemName)
{
	if(!pItemName)
		return;

	if((g_Config.m_SvShopState == 1) && !IsInShop(ClientId))
	{
		GameServer()->SendChatLoc(ClientId, "Ты должен быть в магазине, чтобы купить какие-то товары.");
		return;
	}
	for(auto &Item : m_vItems)
	{
		if(!Item->IsActive())
			continue;
		if(str_comp(Item->Name(), pItemName))
			continue;

		Item->Buy(ClientId);
		return;
	}
	GameServer()->SendChatLoc(ClientId, "No such item '%s' see '/shop' for a full list.", pItemName);
}
