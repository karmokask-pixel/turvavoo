/**
 * @file    Sbr.c
 * @brief   Turvavöö meeldetuletuse (SBR) loogika.
 *
 * Järgitud reeglid (lühidalt):
 *  - MISRA C:2012: täpse laiusega tüübid, U-sufiks märgita konstantidel,
 *    looksulud igal if/else harul, iga if-else-if ahel lõpeb else-ga,
 *    igal switch-il on default, igal funktsioonil üks väljumispunkt,
 *    pole dünaamilist mälu, rekursiooni, ujukomaarve ega stdio-d.
 *  - AUTOSAR: mooduli eesliide (Sbr_), Std_ReturnType, NULL_PTR kontroll,
 *    seadistus eraldi failis Sbr_Cfg.h.
 *  - ISO 26262: määratud ohutu olek, sisendite vahemiku ja vanuse kontroll,
 *    nõuete tunnused (REQ-SBR-xxx) jälgitavuseks testideni.
 */
#include "Sbr.h"       /* mooduli liides: tüübid ja sisendite koodid   */
#include "Sbr_Cfg.h"   /* seadistus: lävi, ajalimiit, kiiruse piirid   */

/* ------------------------------------------------------------------------ */
/* Sisemiste funktsioonide prototüübid.                                     */
/* static = nähtav ainult selles failis (MISRA reeglid 8.2 ja 8.8).          */
/* ------------------------------------------------------------------------ */
static uint8 Sbr_CheckSignals(const Sbr_InputType *input);
static Sbr_StateType Sbr_SelectState(const Sbr_InputType *input, uint8 faults);
static void Sbr_WriteOutput(Sbr_StateType state, uint8 faults,
                            Sbr_OutputType *output);

/* ------------------------------------------------------------------------ */
/* Sisemised funktsioonid                                                   */
/* ------------------------------------------------------------------------ */

/**
 * @brief  Kontrollib kõiki sisendeid: sõnumi vanust ja lubatud väärtusi.
 * @param[in] input  sisendsignaalid (ei tohi olla NULL_PTR)
 * @return Vigade bitimask. SBR_FAULT_NONE tähendab, et kõik on korras.
 *
 * REQ-SBR-003
 */
static uint8 Sbr_CheckSignals(const Sbr_InputType *input)
{
    /* Alguses vigu pole. Muutuja saab väärtuse kohe (MISRA reegel 9.1). */
    uint8 faults = SBR_FAULT_NONE;

    /* Sõnum vanem kui 200 ms -> andmeid ei saa usaldada.
     * Täpselt 200 ms on veel lubatud, sest reegel ütleb "vanem kui". */
    if (input->age_ms > SBR_MSG_TIMEOUT_MS)
    {
        faults |= SBR_FAULT_TIMEOUT;   /* lisab biti, teisi bitte ei muuda */
    }

    /* Süüde: lubatud on ainult 0 ja 1, kõik muu (2...255) on viga. */
    switch (input->ignition_on)
    {
        case SBR_IGN_OFF:
        case SBR_IGN_ON:
            break;                     /* kehtiv väärtus, viga pole */

        default:
            faults |= SBR_FAULT_IGNITION;
            break;
    }

    /* Turvavöö: lubatud on 0 ja 1. Anduri viga (2) ja tundmatu kood
     * on mõlemad vead. */
    switch (input->belt_state)
    {
        case SBR_BELT_UNBUCKLED:
        case SBR_BELT_BUCKLED:
            break;                     /* kehtiv väärtus, viga pole */

        case SBR_BELT_FAULT:
        default:
            faults |= SBR_FAULT_BELT;
            break;
    }

    /* Iste: lubatud on 0 ja 1. Anduri viga (2) ja tundmatu kood
     * on mõlemad vead. */
    switch (input->seat_state)
    {
        case SBR_SEAT_EMPTY:
        case SBR_SEAT_OCCUPIED:
            break;                     /* kehtiv väärtus, viga pole */

        case SBR_SEAT_FAULT:
        default:
            faults |= SBR_FAULT_SEAT;
            break;
    }

    /* Kiirus: viga, kui väärtus puudub (0xFFFF) või on üle 300 km/h,
     * mis on füüsikaliselt ebausutav. */
    if ((input->speed_raw == SBR_SPEED_RAW_NOT_AVAILABLE) ||
        (input->speed_raw > SBR_SPEED_RAW_MAX_VALID))
    {
        faults |= SBR_FAULT_SPEED;
    }

    return faults;   /* üks väljumispunkt (MISRA reegel 15.5) */
}

/**
 * @brief  Valib hoiatuse oleku sisendite ja vigade maski põhjal.
 * @param[in] input   sisendsignaalid (ei tohi olla NULL_PTR)
 * @param[in] faults  funktsiooni Sbr_CheckSignals() tulemus
 * @return Valitud olek.
 *
 * Järjekord: 1) vead -> ohutu olek, 2) hoiatuse tingimused, 3) hoiatust pole.
 */
static Sbr_StateType Sbr_SelectState(const Sbr_InputType *input, uint8 faults)
{
    Sbr_StateType state;   /* saab väärtuse igas allolevas harus */

    if (faults != SBR_FAULT_NONE)
    {
        /* REQ-SBR-003: vigane või aegunud signaal -> tuli (ohutu olek).
         * Heli jääb välja, sest vigaste andmetega ei saa kinnitada,
         * et vöö on tegelikult lahti. */
        state = SBR_STATE_DEGRADED;
    }
    else if ((input->ignition_on == SBR_IGN_ON) &&
             (input->seat_state == SBR_SEAT_OCCUPIED) &&
             (input->belt_state == SBR_BELT_UNBUCKLED))
    {
        /* REQ-SBR-001: süüde sees JA iste hõivatud JA vöö lahti
         * -> hoiatus on vajalik. */
        if (input->speed_raw > SBR_SPEED_AUDIO_THRESHOLD_RAW)
        {
            /* REQ-SBR-002: üle 10 km/h -> tuli + heli. */
            state = SBR_STATE_VISUAL_AUDIO;
        }
        else
        {
            /* REQ-SBR-002: alati vähemalt tuli (ka täpselt 10,00 km/h). */
            state = SBR_STATE_VISUAL;
        }
    }
    else
    {
        /* REQ-SBR-001: süüde väljas, iste tühi või vöö kinni
         * -> hoiatust pole vaja. */
        state = SBR_STATE_NO_WARNING;
    }

    return state;   /* üks väljumispunkt (MISRA reegel 15.5) */
}

/**
 * @brief  Teisendab oleku väljundkäskudeks: tuli, heli, olek ja vead.
 * @param[in]  state   valitud olek
 * @param[in]  faults  vigade bitimask
 * @param[out] output  väljund (ei tohi olla NULL_PTR)
 *
 * Tundmatu oleku väärtus satub default-harusse, mis kirjutab ohutu oleku
 * (kaitsev programmeerimine, MISRA reegel 16.4).
 */
static void Sbr_WriteOutput(Sbr_StateType state, uint8 faults,
                            Sbr_OutputType *output)
{
    switch (state)
    {
        case SBR_STATE_NO_WARNING:
            /* Hoiatust pole: tuli ja heli väljas. */
            output->visual_on = FALSE;
            output->audio_on  = FALSE;
            output->state     = SBR_STATE_NO_WARNING;
            break;

        case SBR_STATE_VISUAL:
            /* Ainult tuli. */
            output->visual_on = TRUE;
            output->audio_on  = FALSE;
            output->state     = SBR_STATE_VISUAL;
            break;

        case SBR_STATE_VISUAL_AUDIO:
            /* Tuli + heli. */
            output->visual_on = TRUE;
            output->audio_on  = TRUE;
            output->state     = SBR_STATE_VISUAL_AUDIO;
            break;

        case SBR_STATE_DEGRADED:
        default:
            /* Ohutu olek: tuli SEES, heli VÄLJAS. Siia jõuab ka iga
             * tundmatu väärtus (nt kui muutuja on mälus rikutud). */
            output->visual_on = TRUE;
            output->audio_on  = FALSE;
            output->state     = SBR_STATE_DEGRADED;
            break;
    }

    /* Vigade mask läheb alati väljundisse, et diagnostika näeks põhjust. */
    output->faults = faults;
}

/* ------------------------------------------------------------------------ */
/* Avalik funktsioon                                                        */
/* ------------------------------------------------------------------------ */

/* Kirjeldus on failis Sbr.h. */
Std_ReturnType Sbr_Evaluate(const Sbr_InputType *input, Sbr_OutputType *output)
{
    /* Algväärtused on ohutud: kui midagi läheb valesti, jääb ohutu olek. */
    Std_ReturnType ret    = E_NOT_OK;
    Sbr_StateType  state  = SBR_STATE_DEGRADED;    /* ohutu olek       */
    uint8          faults = SBR_FAULT_NULL_INPUT;  /* "sisend puudub"  */

    /* Sisendeid hinnatakse ainult siis, kui sisendi viit on olemas. */
    if (input != NULL_PTR)
    {
        faults = Sbr_CheckSignals(input);          /* 1. vigade tuvastus */
        state  = Sbr_SelectState(input, faults);   /* 2. hoiatuse valik  */
        ret    = E_OK;
    }

    /* Väljundisse kirjutatakse ainult siis, kui väljundi viit on olemas.
     * NULL_PTR-i kaudu kirjutamine kõrvaldaks programmi. */
    if (output != NULL_PTR)
    {
        Sbr_WriteOutput(state, faults, output);    /* 3. väljundi kirjutamine */
    }
    else
    {
        ret = E_NOT_OK;   /* kutsuja saab teada, et väljund puudus */
    }

    return ret;   /* E_OK ainult siis, kui mõlemad viidad olid korras */
}
