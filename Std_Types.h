/**
 * @file    Std_Types.h
 * @brief   AUTOSAR-stiilis põhitüübid (lihtsustatud Std_Types.h ja
 *          Platform_Types.h), et moodul kompileeruks ka ilma AUTOSAR-i
 *          tarkvarapakita.
 *
 * Märkus: 'boolean' on C99 'bool', sest siis loeb MISRA C:2012 seda
 * tõeväärtustüübiks (reeglid 14.4 ja 10.1).
 */

/* Include guard: faili sisu loetakse sisse ainult üks kord, isegi kui
 * faili kaasatakse mitu korda (MISRA Dir 4.10). */
#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>   /* täpse laiusega täisarvud: uint8_t, uint16_t, uint32_t */
#include <stdbool.h>  /* C99 tõeväärtustüüp bool ning väärtused true ja false */

/* AUTOSAR-i nimed täpse laiusega tüüpidele. Tavalise 'int'-i suurus sõltub
 * protsessorist, need tüübid on alati sama laiusega (MISRA Dir 4.6). */
typedef uint8_t  uint8;    /* märgita täisarv, 8 bitti:  0 ... 255           */
typedef uint16_t uint16;   /* märgita täisarv, 16 bitti: 0 ... 65 535        */
typedef uint32_t uint32;   /* märgita täisarv, 32 bitti: 0 ... 4 294 967 295 */
typedef bool     boolean;  /* tõeväärtus: TRUE või FALSE                     */

/* Tõeväärtuste konstandid (defineeritakse ainult siis, kui neid veel pole). */
#ifndef TRUE
#define TRUE  (true)
#endif
#ifndef FALSE
#define FALSE (false)
#endif

/* AUTOSAR-i standardne funktsiooni tagastustüüp. */
typedef uint8 Std_ReturnType;
#define E_OK      ((Std_ReturnType)0x00U)   /* funktsioon õnnestus      */
#define E_NOT_OK  ((Std_ReturnType)0x01U)   /* funktsioon ei õnnestunud */

/* Tühi viit (AUTOSAR-i nimi C-keele NULL-i jaoks). */
#ifndef NULL_PTR
#define NULL_PTR  ((void *)0)
#endif

#endif /* STD_TYPES_H */
