/**
 * @file    Sbr.h
 * @brief   Turvavöö meeldetuletuse (SBR = Seat Belt Reminder) liides.
 *
 * Nõuded (ülesandest):
 *  REQ-SBR-001  Hoiatus on vajalik ainult siis, kui süüde on sees, iste on
 *               hõivatud ja turvavöö ei ole kinnitatud.
 *  REQ-SBR-002  Alati vähemalt visuaalne hoiatus. Kui kiirus on üle
 *               10 km/h, lisandub heli.
 *  REQ-SBR-003  Kui mõni signaal on vigane või sõnum vanem kui 200 ms,
 *               peab visuaalne hoiatus töötama (ohutu olek).
 *
 * Ohutus (ISO 26262):
 *  - Ohutu olek: visuaalne hoiatus SEES, heli VÄLJAS.
 *  - Iga sisendi väärtust kontrollitakse; tundmatu kood on viga.
 *  - Funktsioon ei mäleta midagi eelmisest korrast: samad sisendid
 *    annavad alati sama tulemuse (deterministlik).
 *  - Dünaamilist mälu, rekursiooni ega ujukomaarve ei kasutata.
 */
#ifndef SBR_H
#define SBR_H

#include "Std_Types.h"   /* uint8, uint16, uint32, boolean, Std_ReturnType */

/* ------------------------------------------------------------------------ */
/* Sisendite koodid (toorväärtused, nagu need tulevad sõiduki siinilt)      */
/* ------------------------------------------------------------------------ */

/* ignition_on: süüte olek */
#define SBR_IGN_OFF            ((uint8)0U)   /* süüde väljas */
#define SBR_IGN_ON             ((uint8)1U)   /* süüde sees   */
/* iga muu väärtus = vigane või puuduv signaal */

/* belt_state: turvavöö lukk */
#define SBR_BELT_UNBUCKLED     ((uint8)0U)   /* vöö kinnitamata    */
#define SBR_BELT_BUCKLED       ((uint8)1U)   /* vöö kinnitatud     */
#define SBR_BELT_FAULT         ((uint8)2U)   /* andur teatab veast */
/* iga muu väärtus = vigane või puuduv signaal */

/* seat_state: istme andur */
#define SBR_SEAT_EMPTY         ((uint8)0U)   /* iste tühi          */
#define SBR_SEAT_OCCUPIED      ((uint8)1U)   /* iste hõivatud      */
#define SBR_SEAT_FAULT         ((uint8)2U)   /* andur teatab veast */
/* iga muu väärtus = vigane või puuduv signaal */

/* ------------------------------------------------------------------------ */
/* Vigade bitid (bitimask diagnostika jaoks)                                */
/* Igal veal on oma bitt; mitu viga korraga = bitid kokku (nt 0x02 | 0x10). */
/* ------------------------------------------------------------------------ */
#define SBR_FAULT_NONE         ((uint8)0x00U)   /* vigu pole                */
#define SBR_FAULT_IGNITION     ((uint8)0x01U)   /* süüte signaal vigane     */
#define SBR_FAULT_BELT         ((uint8)0x02U)   /* vöö signaal vigane       */
#define SBR_FAULT_SEAT         ((uint8)0x04U)   /* istme signaal vigane     */
#define SBR_FAULT_SPEED        ((uint8)0x08U)   /* kiirus puudub või vale   */
#define SBR_FAULT_TIMEOUT      ((uint8)0x10U)   /* sõnum vanem kui 200 ms   */
#define SBR_FAULT_NULL_INPUT   ((uint8)0x20U)   /* sisendi viit on NULL_PTR */

/* ------------------------------------------------------------------------ */
/* Tüübid                                                                   */
/* ------------------------------------------------------------------------ */

/** Ühe hindamise koondtulemus. */
typedef enum
{
    SBR_STATE_NO_WARNING   = 0,  /* hoiatust pole vaja                 */
    SBR_STATE_VISUAL       = 1,  /* ainult visuaalne hoiatus (tuli)    */
    SBR_STATE_VISUAL_AUDIO = 2,  /* tuli + helisignaal                 */
    SBR_STATE_DEGRADED     = 3   /* signaali viga -> ohutu olek (tuli) */
} Sbr_StateType;

/** Sisendsignaalid (väljade nimed täpselt nagu ülesandes). */
typedef struct
{
    uint8  ignition_on;  /* süüde: SBR_IGN_xxx                          */
    uint8  belt_state;   /* turvavöö: SBR_BELT_xxx                      */
    uint8  seat_state;   /* iste: SBR_SEAT_xxx                          */
    uint16 speed_raw;    /* kiirus, 1 bitt = 0,01 km/h; 0xFFFF = puudub */
    uint32 age_ms;       /* aeg sõnumi saabumisest [ms]                 */
} Sbr_InputType;

/** Väljundkäsud näidikule ja helisignaalile. */
typedef struct
{
    boolean       visual_on;   /* märgutuli või teade näidikul: TRUE = sees */
    boolean       audio_on;    /* helisignaal: TRUE = sees                  */
    Sbr_StateType state;       /* koondolek                                 */
    uint8         faults;      /* vigade bitimask (SBR_FAULT_xxx)           */
} Sbr_OutputType;

/* ------------------------------------------------------------------------ */
/* Avalik funktsioon (API)                                                  */
/* ------------------------------------------------------------------------ */

/**
 * @brief  Arvutab ühe tsükli turvavöö hoiatuse.
 *
 * Kutsu seda tsükliliselt (nt iga 10 ms järel) värskete signaalidega.
 *
 * @param[in]  input   Sisendsignaalid. Kui NULL_PTR, kirjutatakse ohutu olek.
 * @param[out] output  Hoiatuse käsud. Ei tohi olla NULL_PTR.
 *
 * @return E_OK      sisendid hinnati
 * @return E_NOT_OK  mõni argument oli NULL_PTR (kui output on olemas,
 *                   kirjutati sinna ohutu olek)
 */
Std_ReturnType Sbr_Evaluate(const Sbr_InputType *input, Sbr_OutputType *output);

#endif /* SBR_H */
