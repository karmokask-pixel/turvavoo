/**
 * @file    test_Sbr.c
 * @brief   Turvavöö meeldetuletuse (SBR) mooduli ühiktestid.
 *
 * Testkood ei ole tootekood, seega võib see kasutada <stdio.h>-d
 * (MISRA C:2012 reegel 21.6 kehtib mooduli, mitte testide kohta).
 *
 * Kolm osa:
 *  1. Nõuetepõhised testid tabelina (igal real nõude tunnus REQ-SBR-xxx),
 *     sh piirväärtused (ISO 26262-6, tabel 8).
 *  2. API testid: mis juhtub NULL_PTR argumentidega.
 *  3. Back-to-back test: iga võimalikku süüte/vöö/istme koodi ja iga
 *     kiiruse väärtust võrreldakse lihtsa referentsmudeliga.
 */
#include <stdio.h>      /* printf: tulemuste väljastamine */
#include "Sbr.h"        /* testitav moodul */
#include "Sbr_Cfg.h"    /* seadistus */

/* Ühe testjuhtumi kirjeldus: sisendid ja oodatav tulemus. */
typedef struct
{
    const char    *req;         /* nõude tunnus, nt "REQ-SBR-001"          */
    const char    *name;        /* testi lühinimi (väljastatakse ekraanile) */
    Sbr_InputType  in;          /* süüde, vöö, iste, speed_raw, age_ms      */
    boolean        exp_visual;  /* oodatav tuli                             */
    boolean        exp_audio;   /* oodatav heli                             */
    Sbr_StateType  exp_state;   /* oodatav olek                             */
    uint8          exp_faults;  /* oodatav vigade mask                      */
} TestCase;

/* Lühikesed nimed, et testitabel mahuks ära ja oleks loetav. */
#define OFF   SBR_IGN_OFF          /* süüde väljas   */
#define ON    SBR_IGN_ON           /* süüde sees     */
#define UNB   SBR_BELT_UNBUCKLED   /* vöö lahti      */
#define BUCK  SBR_BELT_BUCKLED     /* vöö kinni      */
#define EMPTY SBR_SEAT_EMPTY       /* iste tühi      */
#define OCC   SBR_SEAT_OCCUPIED    /* iste hõivatud  */

#define S_NONE  SBR_STATE_NO_WARNING     /* hoiatust pole */
#define S_VIS   SBR_STATE_VISUAL         /* ainult tuli   */
#define S_AUD   SBR_STATE_VISUAL_AUDIO   /* tuli + heli   */
#define S_DEG   SBR_STATE_DEGRADED       /* ohutu olek    */

/* Testitabel. Iga rida: nõue, nimi, { süüde, vöö, iste, kiirus, vanus },
 * oodatav tuli, oodatav heli, oodatav olek, oodatav vigade mask. */
static const TestCase g_cases[] =
{
    /* --- REQ-SBR-001: hoiatus ainult siis, kui süüde sees + iste hõivatud + vöö lahti.
     *     Kõik 8 kombinatsiooni, kiirus 0. */
    { "REQ-SBR-001", "ign OFF, empty, unbuckled",        { OFF, UNB,  EMPTY, 0U, 0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-001", "ign OFF, empty, buckled",          { OFF, BUCK, EMPTY, 0U, 0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-001", "ign OFF, occupied, unbuckled",     { OFF, UNB,  OCC,   0U, 0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-001", "ign OFF, occupied, buckled",       { OFF, BUCK, OCC,   0U, 0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-001", "ign ON, empty, unbuckled",         { ON,  UNB,  EMPTY, 0U, 0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-001", "ign ON, empty, buckled",           { ON,  BUCK, EMPTY, 0U, 0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-001", "ign ON, occupied, unbuckled",      { ON,  UNB,  OCC,   0U, 0U }, TRUE,  FALSE, S_VIS,  SBR_FAULT_NONE },
    { "REQ-SBR-001", "ign ON, occupied, buckled",        { ON,  BUCK, OCC,   0U, 0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },

    /* --- REQ-SBR-002: tuli alati, heli üle 10 km/h. Piirväärtused 9,99 / 10,00 / 10,01 km/h. */
    { "REQ-SBR-002", "9.99 km/h -> visual only",         { ON,  UNB,  OCC,   999U,   0U }, TRUE,  FALSE, S_VIS,  SBR_FAULT_NONE },
    { "REQ-SBR-002", "10.00 km/h -> visual only",        { ON,  UNB,  OCC,   1000U,  0U }, TRUE,  FALSE, S_VIS,  SBR_FAULT_NONE },
    { "REQ-SBR-002", "10.01 km/h -> visual + audio",     { ON,  UNB,  OCC,   1001U,  0U }, TRUE,  TRUE,  S_AUD,  SBR_FAULT_NONE },
    { "REQ-SBR-002", "300.00 km/h (max) -> audio",       { ON,  UNB,  OCC,   30000U, 0U }, TRUE,  TRUE,  S_AUD,  SBR_FAULT_NONE },
    { "REQ-SBR-002", "50 km/h, buckled -> none",         { ON,  BUCK, OCC,   5000U,  0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-002", "50 km/h, seat empty -> none",      { ON,  UNB,  EMPTY, 5000U,  0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },
    { "REQ-SBR-002", "50 km/h, ign OFF -> none",         { OFF, UNB,  OCC,   5000U,  0U }, FALSE, FALSE, S_NONE, SBR_FAULT_NONE },

    /* --- REQ-SBR-003: sõnumi vanus. Piirväärtused 200 / 201 ms. */
    { "REQ-SBR-003", "age 200 ms -> still valid",        { ON,  UNB,  OCC,   5000U, 200U }, TRUE, TRUE,  S_AUD, SBR_FAULT_NONE },
    { "REQ-SBR-003", "age 201 ms -> safe state",         { ON,  UNB,  OCC,   5000U, 201U }, TRUE, FALSE, S_DEG, SBR_FAULT_TIMEOUT },
    { "REQ-SBR-003", "age max -> safe state",            { ON,  UNB,  OCC,   0U, 0xFFFFFFFFU }, TRUE, FALSE, S_DEG, SBR_FAULT_TIMEOUT },
    { "REQ-SBR-003", "stale + buckled -> safe state",    { ON,  BUCK, OCC,   0U, 201U }, TRUE, FALSE, S_DEG, SBR_FAULT_TIMEOUT },

    /* --- REQ-SBR-003: vigased signaalid. Iga signaal eraldi + mitu viga korraga. */
    { "REQ-SBR-003", "ignition = 2 (invalid)",           { 2U,    BUCK, OCC, 0U, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_IGNITION },
    { "REQ-SBR-003", "ignition = 0xFF (n/a)",            { 0xFFU, BUCK, OCC, 0U, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_IGNITION },
    { "REQ-SBR-003", "belt sensor FAULT",                { ON, SBR_BELT_FAULT, OCC, 0U, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_BELT },
    { "REQ-SBR-003", "belt = 0xFF (n/a)",                { ON, 0xFFU, OCC,   0U, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_BELT },
    { "REQ-SBR-003", "seat sensor FAULT",                { ON, BUCK, SBR_SEAT_FAULT, 0U, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_SEAT },
    { "REQ-SBR-003", "seat = 0xFF (n/a)",                { ON, BUCK, 0xFFU,  0U, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_SEAT },
    { "REQ-SBR-003", "speed 300.01 km/h (implausible)",  { ON, UNB,  OCC, 30001U,  0U }, TRUE, FALSE, S_DEG, SBR_FAULT_SPEED },
    { "REQ-SBR-003", "speed 0xFFFF (n/a)",               { ON, UNB,  OCC, 0xFFFFU, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_SPEED },
    { "REQ-SBR-003", "belt fault while ign OFF",         { OFF, SBR_BELT_FAULT, EMPTY, 0U, 0U }, TRUE, FALSE, S_DEG, SBR_FAULT_BELT },
    { "REQ-SBR-003", "several faults at once",           { ON, SBR_BELT_FAULT, OCC, 0xFFFFU, 500U }, TRUE, FALSE, S_DEG,
      (uint8)(SBR_FAULT_BELT | SBR_FAULT_SPEED | SBR_FAULT_TIMEOUT) },
};

static unsigned int g_run    = 0U;   /* tehtud kontrollide arv     */
static unsigned int g_failed = 0U;   /* ebaõnnestunud kontrollide arv */

/* Tagastab oleku nime tekstina (ekraanile väljastamiseks). */
static const char *StateName(Sbr_StateType s)
{
    const char *name;
    switch (s)
    {
        case SBR_STATE_NO_WARNING:   name = "NO_WARNING";   break;
        case SBR_STATE_VISUAL:       name = "VISUAL";       break;
        case SBR_STATE_VISUAL_AUDIO: name = "VISUAL_AUDIO"; break;
        case SBR_STATE_DEGRADED:     name = "DEGRADED";     break;
        default:                     name = "?";            break;
    }
    return name;
}

/* Loendab kontrolli ja väljastab teate, kui kontroll ebaõnnestus. */
static void Check(boolean ok, const char *req, const char *name)
{
    g_run++;
    if (!ok)
    {
        g_failed++;
        printf("  FAIL  [%s] %s\n", req, name);
    }
}

/* ---------------------------------------------------------------------- */
/* 1. Nõuetepõhised testid                                                */
/* ---------------------------------------------------------------------- */
static void RunTableTests(void)
{
    unsigned int i;
    /* Testjuhtumite arv = tabeli suurus / ühe rea suurus. */
    const unsigned int n = (unsigned int)(sizeof(g_cases) / sizeof(g_cases[0]));

    printf("1) Requirement-based tests\n");
    printf("   %-12s %-34s %-6s %-6s %-13s %s\n",
           "Req", "Case", "Visual", "Audio", "State", "Result");

    /* Käime tabeli läbi rida haaval. */
    for (i = 0U; i < n; i++)
    {
        const TestCase *tc = &g_cases[i];
        Sbr_OutputType  out;
        /* Kutsume testitavat funktsiooni selle rea sisenditega. */
        Std_ReturnType  ret = Sbr_Evaluate(&tc->in, &out);
        /* Test läbib ainult siis, kui KÕIK väljundid on oodatud. */
        boolean ok = (ret == E_OK) &&
                     (out.visual_on == tc->exp_visual) &&
                     (out.audio_on  == tc->exp_audio)  &&
                     (out.state     == tc->exp_state)  &&
                     (out.faults    == tc->exp_faults);

        /* Väljastame ühe tulemusrea. */
        printf("   %-12s %-34s %-6s %-6s %-13s %s\n",
               tc->req, tc->name,
               out.visual_on ? "ON" : "off",
               out.audio_on  ? "ON" : "off",
               StateName(out.state),
               ok ? "PASS" : "FAIL");
        Check(ok, tc->req, tc->name);
    }
}

/* ---------------------------------------------------------------------- */
/* 2. API testid: NULL_PTR argumendid                                     */
/* ---------------------------------------------------------------------- */
static void RunApiTests(void)
{
    const Sbr_InputType valid = { ON, UNB, OCC, 0U, 0U };   /* korrektne sisend */
    Sbr_OutputType out;
    Std_ReturnType ret;

    printf("\n2) API robustness tests\n");

    /* Täidame väljundi meelega valede väärtustega, et näha, kas funktsioon
     * kirjutab sinna ohutu oleku. */
    out.visual_on = FALSE;
    out.audio_on  = TRUE;
    out.state     = SBR_STATE_NO_WARNING;
    out.faults    = SBR_FAULT_NONE;

    /* Sisend puudub -> oodatakse E_NOT_OK ja ohutut olekut. */
    ret = Sbr_Evaluate(NULL_PTR, &out);
    Check((ret == E_NOT_OK) && (out.visual_on == TRUE) && (out.audio_on == FALSE) &&
          (out.state == SBR_STATE_DEGRADED) && (out.faults == SBR_FAULT_NULL_INPUT),
          "API", "NULL input -> E_NOT_OK + safe state");

    /* Väljund puudub -> oodatakse E_NOT_OK, programm ei tohi kokku joosta. */
    ret = Sbr_Evaluate(&valid, NULL_PTR);
    Check(ret == E_NOT_OK, "API", "NULL output -> E_NOT_OK, no crash");

    /* Mõlemad puuduvad -> E_NOT_OK. */
    ret = Sbr_Evaluate(NULL_PTR, NULL_PTR);
    Check(ret == E_NOT_OK, "API", "both NULL -> E_NOT_OK, no crash");

    /* Mõlemad olemas -> E_OK. */
    ret = Sbr_Evaluate(&valid, &out);
    Check(ret == E_OK, "API", "valid arguments -> E_OK");

    printf("   4 checks done\n");
}

/* ---------------------------------------------------------------------- */
/* 3. Back-to-back test: võrdlus sõltumatu referentsmudeliga              */
/* ---------------------------------------------------------------------- */

/* Referentsmudel, kirjutatud otse ülesande tekstist ja võimalikult lihtsalt.
 * Kui moodul ja referents annavad sama tulemuse, on loogika õige. */
static void Reference(const Sbr_InputType *in, boolean *visual, boolean *audio)
{
    boolean ign_ok   = (in->ignition_on <= 1U);       /* süüde 0 või 1      */
    boolean belt_ok  = (in->belt_state  <= 1U);       /* vöö 0 või 1        */
    boolean seat_ok  = (in->seat_state  <= 1U);       /* iste 0 või 1       */
    boolean speed_ok = (in->speed_raw   <= 30000U);   /* kuni 300 km/h      */
    boolean fresh    = (in->age_ms      <= 200U);     /* kuni 200 ms vana   */

    if (!(ign_ok && belt_ok && seat_ok && speed_ok && fresh))
    {
        *visual = TRUE;            /* reegel 3: viga -> tuli */
        *audio  = FALSE;
    }
    else if ((in->ignition_on == 1U) && (in->seat_state == 1U) && (in->belt_state == 0U))
    {
        *visual = TRUE;            /* reeglid 1 + 2: tuli, heli üle 10 km/h */
        *audio  = (in->speed_raw > 1000U);
    }
    else
    {
        *visual = FALSE;           /* hoiatust pole */
        *audio  = FALSE;
    }
}

static unsigned long g_b2b_run  = 0UL;   /* võrreldud kombinatsioonide arv */
static unsigned long g_b2b_fail = 0UL;   /* erinevuste arv                 */

/* Võrdleb ühe sisendikombinatsiooni korral moodulit ja referentsi. */
static void CompareOne(const Sbr_InputType *in)
{
    Sbr_OutputType out;
    boolean ref_visual;
    boolean ref_audio;

    (void)Sbr_Evaluate(in, &out);              /* mooduli tulemus     */
    Reference(in, &ref_visual, &ref_audio);    /* referentsi tulemus  */
    g_b2b_run++;

    /* Tulemused peavad ühtima. Lisaks: heli ei tohi kunagi olla ilma tuleta. */
    if ((out.visual_on != ref_visual) || (out.audio_on != ref_audio) ||
        (out.audio_on && !out.visual_on))
    {
        /* Väljastame ainult esimesed 5 erinevust, et ekraan ei täituks. */
        if (g_b2b_fail < 5UL)
        {
            printf("  FAIL  B2B ign=%u belt=%u seat=%u speed=%u age=%lu\n",
                   (unsigned)in->ignition_on, (unsigned)in->belt_state,
                   (unsigned)in->seat_state, (unsigned)in->speed_raw,
                   (unsigned long)in->age_ms);
        }
        g_b2b_fail++;
    }
}

static void RunBackToBack(void)
{
    /* Kiiruse ja vanuse väärtused, mis on piiride lähedal. */
    static const uint16 speeds[] = { 0U, 1000U, 1001U, 30000U, 30001U, 0xFFFFU };
    static const uint32 ages[]   = { 0U, 200U, 201U, 0xFFFFFFFFU };
    Sbr_InputType in;
    unsigned int ign;
    unsigned int belt;
    unsigned int seat;
    unsigned int s;
    unsigned int a;
    unsigned long speed;

    printf("\n3) Back-to-back test vs. reference model\n");

    /* a) Iga süüte/vöö/istme kood (igaüks 0...255), valitud kiirused ja vanused.
     *    256 x 256 x 256 x 10 = ~168 miljonit kombinatsiooni. */
    for (ign = 0U; ign <= 255U; ign++)
    {
        for (belt = 0U; belt <= 255U; belt++)
        {
            for (seat = 0U; seat <= 255U; seat++)
            {
                in.ignition_on = (uint8)ign;
                in.belt_state  = (uint8)belt;
                in.seat_state  = (uint8)seat;
                /* Kõik valitud kiirused, sõnum värske. */
                for (s = 0U; s < (sizeof(speeds) / sizeof(speeds[0])); s++)
                {
                    in.speed_raw = speeds[s];
                    in.age_ms    = 0U;
                    CompareOne(&in);
                }
                /* Kõik valitud vanused, kiirus 50 km/h. */
                in.speed_raw = 5000U;
                for (a = 0U; a < (sizeof(ages) / sizeof(ages[0])); a++)
                {
                    in.age_ms = ages[a];
                    CompareOne(&in);
                }
            }
        }
    }

    /* b) Iga kiiruse väärtus 0...65535 iga kehtiva süüte/vöö/istme
     *    kombinatsiooni ja iga valitud vanuse korral. */
    for (speed = 0UL; speed <= 0xFFFFUL; speed++)
    {
        for (ign = 0U; ign <= 1U; ign++)
        {
            for (belt = 0U; belt <= 1U; belt++)
            {
                for (seat = 0U; seat <= 1U; seat++)
                {
                    for (a = 0U; a < (sizeof(ages) / sizeof(ages[0])); a++)
                    {
                        in.ignition_on = (uint8)ign;
                        in.belt_state  = (uint8)belt;
                        in.seat_state  = (uint8)seat;
                        in.speed_raw   = (uint16)speed;
                        in.age_ms      = ages[a];
                        CompareOne(&in);
                    }
                }
            }
        }
    }

    printf("   %lu input combinations compared, %lu mismatches\n",
           g_b2b_run, g_b2b_fail);
    Check(g_b2b_fail == 0UL, "B2B", "implementation == reference model");
}

/* Testprogrammi algus: käivitab kõik kolm testiosa ja väljastab kokkuvõtte. */
int main(void)
{
    RunTableTests();
    RunApiTests();
    RunBackToBack();

    printf("\nSummary: %u checks, %u failed -> %s\n",
           g_run, g_failed, (g_failed == 0U) ? "ALL PASSED" : "FAILED");

    /* 0 = kõik testid läbisid (Makefile ja CI tunnevad selle ära). */
    return (g_failed == 0U) ? 0 : 1;
}
