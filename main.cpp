#include <mod/amlmod.h>
#include <mod/logger.h>
#include <isautils.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #include "AArchASMHelper/Thumbv7_ASMHelper.h"
    #define BYVER(__for32, __for64) (__for32)
    using namespace ThumbV7;
#else
    #include "GTASA_STRUCTS_210.h"
    #include "AArchASMHelper/ARMv8_ASMHelper.h"
    #define BYVER(__for32, __for64) (__for64)
    using namespace ARMv8;
#endif
#define sizeofA(__aVar)  ((int)(sizeof(__aVar)/sizeof(__aVar[0])))

MYMOD(net.usergrinch.rusjj.helifix, HeliFixSA, 1.1, Grinch_ & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.2)
    ADD_DEPENDENCY_VER(net.rusjj.gtasa.utils, 1.6)
END_DEPLIST()

ISAUtils* sautils = NULL;
uintptr_t pGTASA;
void* hGTASA;

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Variables
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
#define DARKTIME_START 19
CHeli **Helis;
uint8_t *ms_nGameClockHours;
uint32_t *NumberOfSearchLights;

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Functions
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
inline void SetupHeliCrash(CHeli*& heli)
{
    if(!heli || !heli->vehicleFlags.bCanBeDamaged || heli->m_nCreateBy != PERMANENT_VEHICLE) return;
    CPed *pDriver = heli->m_pDriver;
    if(!pDriver || !pDriver->IsAlive())
    {
        heli->m_AutoPilot.Mission = MISSION_HELI_CRASH_AND_BURN;
        heli->m_fHealth = 200.0f;
        if(pDriver) sautils->MarkEntityAsNotNeeded(pDriver);

        CPed *pPassenger = heli->m_pPassenger[0];
        if(pPassenger) sautils->MarkEntityAsNotNeeded(pPassenger);

        heli = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Hooks
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
DECL_HOOK(CHeli*, GenerateHeli, CPed* pTargetPed, bool bNewsHeli)
{
    CHeli* self = GenerateHeli(pTargetPed, bNewsHeli);
    if(self->m_nModelIndex == 497)
    {
        sautils->LoadModelId(280);
        self->m_pDriver = sautils->CreatePed(PED_TYPE_COP, 280, self);
        self->m_pPassenger[0] = sautils->CreatePed(PED_TYPE_COP, 280, self, 0);
        sautils->MarkModelAsNotNeeded(280);
    }
    else
    {
        sautils->LoadModelId(61);
        self->m_pDriver = sautils->CreatePed(PED_TYPE_CIVMALE, 61, self);
        if(self->m_nModelIndex == 488)
        {
            sautils->LoadModelId(23);
            self->m_pPassenger[0] = sautils->CreatePed(PED_TYPE_CIVMALE, 23, self, 0);
            sautils->MarkModelAsNotNeeded(23);
        }
        sautils->MarkModelAsNotNeeded(61);
    }
    return self;
}
DECL_HOOKv(UpdateHelis)
{
    if(*NumberOfSearchLights > 0)
    {
        int hour = *ms_nGameClockHours; 
        if(hour > 6 && hour < DARKTIME_START) *NumberOfSearchLights = 0;
    }

    UpdateHelis();

    SetupHeliCrash(Helis[0]);
    SetupHeliCrash(Helis[1]);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////      Main
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
extern "C" void OnAllModsLoaded()
{
    logger->SetTag("HeliFix");

    sautils = (ISAUtils*)GetInterface("SAUtils");
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");

    SET_TO(Helis, aml->GetSym(hGTASA, "_ZN5CHeli6pHelisE"));
    SET_TO(ms_nGameClockHours, aml->GetSym(hGTASA, "_ZN6CClock18ms_nGameClockHoursE"));
    SET_TO(NumberOfSearchLights, aml->GetSym(hGTASA, "_ZN5CHeli20NumberOfSearchLightsE"));

    aml->PlaceB(pGTASA + BYVER(0x5735B6 + 0x1, 0x695D30), pGTASA + BYVER(0x5735C4 + 0x1, 0x695D44));
    HOOKPLT(UpdateHelis, pGTASA + BYVER(0x66EBEC, 0x83E370));
    #ifdef AML32
    HOOKPLT(GenerateHeli, pGTASA + 0x675120);
    aml->Write16(pGTASA + 0x57284A, MOV2Bits::Create(DARKTIME_START, 0));
    #else
    HOOK(GenerateHeli, aml->GetSym(hGTASA, "_ZN5CHeli12GenerateHeliEP4CPedb"));
    aml->Write32(pGTASA + 0x694FB4, MOVBits::Create(DARKTIME_START, 0, false));
    #endif
}