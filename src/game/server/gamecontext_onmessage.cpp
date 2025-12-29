// gamecontext scoped OnMessage ddnet++ methods

#include "gamecontext.h"

#include <engine/shared/config.h>

#include <game/server/teams.h>

#include <cstring>

bool CGameContext::AbortTeamChange(int ClientId, CPlayer *pPlayer)
{
	if(IsMinigame(ClientId))
	{
		SendChatTarget(ClientId, "[МИНИ-ИГРЫ] Нельзя менять команду во время мини-игры или находясь в тюрьме.");
		return true;
	}

	if(m_apPlayers[ClientId]->m_SpawnBlocks > 3)
	{
		SendChatTarget(ClientId, "[SPAWNBLOCK] Нельзя сменить команду: слишком много спавнблоков. Попробуйте позже.");
		return true;
	}

	if(m_apPlayers[ClientId]->m_IsBlockWaving)
	{
		SendChatTarget(ClientId, "[BlockWave] Нельзя менять команду во время blockwave. Используйте '/blockwave leave'");
		return true;
	}

	//zCatch survival LMS ChillerDragon Instagib grenade rifle
	if(g_Config.m_SvInstagibMode == 2 || g_Config.m_SvInstagibMode == 4) //gLMS iLMS
	{
		SendChatTarget(ClientId, "Нельзя присоединиться к уже идущему survival-раунду. Дождитесь окончания.");
		return true;
	}

	if(pPlayer->m_GangsterBagMoney)
	{
		SendChatTarget(ClientId, "Перед выходом/наблюдением очистите gangsterbag, иначе потеряете деньги.");
		SendChatTarget(ClientId, "или очистите сами командой '/gangsterbag clear'");
		return true;
	}
	return false;
}

bool CGameContext::AbortKill(int ClientId, CPlayer *pPlayer, CCharacter *pChr)
{
	if(m_InstaGrenadeRoundEndTickTicker && m_apPlayers[ClientId]->m_IsInstaArena_gdm)
		return true; //yy evil silent return
	if(m_InstaRifleRoundEndTickTicker && m_apPlayers[ClientId]->m_IsInstaArena_idm)
		return true; //yy evil silent return

	for(auto &Minigame : m_vMinigames)
		if(!Minigame->AllowSelfKill(ClientId))
			return true;

	if(m_apPlayers[ClientId]->m_IsBlockWaving && !pPlayer->m_IsBlockWaveWaiting)
	{
		SendChatTarget(ClientId, "[BlockWave] Нельзя самоубиться во время blockwave. Используйте '/blockwave leave'.");
		return true;
	}
	if(m_apPlayers[ClientId]->m_SpawnBlocks > 3 && g_Config.m_SvSpawnBlockProtection == 2)
	{
		SendChatTarget(ClientId, "[SPAWNBLOCK] Нельзя самоубиться: слишком много спавнблоков. Попробуйте позже.");
		return true;
	}
	if(!g_Config.m_SvAllowBombSelfkill && GetPlayerChar(ClientId) && GetPlayerChar(ClientId)->m_IsBombing)
	{
		SendChatTarget(ClientId, "[BOMB] Защита от самоубийства активна. Используйте '/bomb leave' чтобы выйти и вернуть деньги. Любой другой способ выхода приведёт к потере денег.");
		return true;
	}
	if(m_apPlayers[ClientId]->m_IsSurvivaling)
	{
		if(g_Config.m_SvSurvivalKillProtection == 2) //full on
		{
			SendChatTarget(ClientId, "[SURVIVAL] Защита от убийства. Сначала '/survival leave'.");
			return true;
		}
		else if(g_Config.m_SvSurvivalKillProtection == 1 && !m_apPlayers[ClientId]->m_IsSurvivalLobby) //allowed in lobby
		{
			SendChatTarget(ClientId, "[SURVIVAL] Защита от убийства. Сначала '/survival leave'.");
			return true;
		}
		//else == off
	}
	if(m_apPlayers[ClientId]->m_IsInstaArena_fng && pChr->m_FreezeTime)
	{
		SendChatTarget(ClientId, "[INSTA] Нельзя самоубиться в FNG, пока вы заморожены.");
		return true;
	}
	return false;
}
