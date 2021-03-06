/* This file is part of the BlizzLikeCore Project. See CREDITS and LICENSE files
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Garr
SD%Complete: 50
SDComment: Garr's enrage is missing
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    // Garr spells
    SPELL_ANTIMAGICPULSE        = 19492,
    SPELL_MAGMASHACKLES         = 19496,
    SPELL_ENRAGE                = 19516,                    // TODO Stacking enrage (stacks to 10 times)

    // Add spells
    SPELL_ERUPTION              = 19497,
    SPELL_MASSIVE_ERUPTION      = 20483,                    // TODO possible on death
    SPELL_IMMOLATE              = 20294,
    SPELL_SEPARATION_ANXIETY    = 23492,                    // Used if separated too far from Garr, 21095 use unknown.
};

struct BLIZZLIKE_DLL_DECL boss_garrAI : public ScriptedAI
{
    boss_garrAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiAntiMagicPulseTimer;
    uint32 m_uiMagmaShacklesTimer;

    void Reset() override
    {
        m_uiAntiMagicPulseTimer = 25000;
        m_uiMagmaShacklesTimer = 15000;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GARR, IN_PROGRESS);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GARR, DONE);
    }

    void JustReachedHome() override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GARR, FAIL);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // AntiMagicPulse_Timer
        if (m_uiAntiMagicPulseTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ANTIMAGICPULSE) == CAST_OK)
                m_uiAntiMagicPulseTimer = urand(10000, 15000);
        }
        else
            m_uiAntiMagicPulseTimer -= uiDiff;

        // MagmaShackles_Timer
        if (m_uiMagmaShacklesTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_MAGMASHACKLES) == CAST_OK)
                m_uiMagmaShacklesTimer = urand(8000, 12000);
        }
        else
            m_uiMagmaShacklesTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

struct BLIZZLIKE_DLL_DECL mob_fireswornAI : public ScriptedAI
{
    mob_fireswornAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiImmolateTimer;
    uint32 m_uiSeparationCheckTimer;

    void Reset() override
    {
        m_uiImmolateTimer = urand(4000, 8000);              // These times are probably wrong
        m_uiSeparationCheckTimer = 5000;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Immolate_Timer
        if (m_uiImmolateTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_IMMOLATE) == CAST_OK)
                    m_uiImmolateTimer = urand(5000, 10000);
            }
        }
        else m_uiImmolateTimer -= uiDiff;

        if (m_uiSeparationCheckTimer < uiDiff)
        {
            // Distance guesswork, but should be ok
            Creature* pGarr = m_pInstance->GetSingleCreatureFromStorage(NPC_GARR);
            if (pGarr && pGarr->isAlive() && !m_creature->IsWithinDist2d(pGarr->GetPositionX(), pGarr->GetPositionY(), 50.0f))
                DoCastSpellIfCan(m_creature, SPELL_SEPARATION_ANXIETY, CAST_TRIGGERED);

            m_uiSeparationCheckTimer = 5000;
        }
        else
            m_uiSeparationCheckTimer -= uiDiff;

        // Cast Erruption and let them die
        if (m_creature->GetHealthPercent() <= 10.0f)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_ERUPTION);
            m_creature->SetDeathState(JUST_DIED);
            m_creature->RemoveCorpse();
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_garr(Creature* pCreature)
{
    return new boss_garrAI(pCreature);
}

CreatureAI* GetAI_mob_firesworn(Creature* pCreature)
{
    return new mob_fireswornAI(pCreature);
}

void AddSC_boss_garr()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_garr";
    pNewScript->GetAI = &GetAI_boss_garr;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mob_firesworn";
    pNewScript->GetAI = &GetAI_mob_firesworn;
    pNewScript->RegisterSelf();
}
