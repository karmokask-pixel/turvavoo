/**
 * @file    Sbr_Cfg.h
 * @brief   Turvavöö meeldetuletuse (SBR) seadistus.
 *
 * Kõik seadistatavad arvud on siin failis. Nii pole loogikas endas
 * "maagilisi arve" (MISRA) ja seadistus on koodist eraldi (AUTOSAR).
 * Teise auto jaoks piisab ainult selle faili muutmisest.
 */
#ifndef SBR_CFG_H
#define SBR_CFG_H

#include "Std_Types.h"   /* uint16, uint32 */

/* speed_raw skaala: 1 bitt = 0,01 km/h.
 * Arvutame ainult täisarvudega, ujukomaarve ei kasutata. */

/** REQ-SBR-002: heli lisandub ainult üle 10 km/h (10,00 km/h = 1000). */
#define SBR_SPEED_AUDIO_THRESHOLD_RAW  ((uint16)1000U)

/** Suurim usutav kiirus: 300,00 km/h. Suurem väärtus loetakse veaks. */
#define SBR_SPEED_RAW_MAX_VALID        ((uint16)30000U)

/** Väärtus, mille saatja paneb sõnumisse, kui kiirust pole teada. */
#define SBR_SPEED_RAW_NOT_AVAILABLE    ((uint16)0xFFFFU)

/** REQ-SBR-003: sellest vanem sõnum [ms] loetakse veaks. */
#define SBR_MSG_TIMEOUT_MS             ((uint32)200U)

#endif /* SBR_CFG_H */
