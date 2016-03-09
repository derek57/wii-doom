// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:  none
//        Implements special effects:
//        Texture animation, height or lighting changes
//         according to adjacent sectors, respective
//         utility functions, etc.
//
//-----------------------------------------------------------------------------


#ifndef __P_SPEC__
#define __P_SPEC__


#define FLOOR_SOLID                0
#define FLOOR_WATER                1
#define FLOOR_LAVA                 2
#define FLOOR_NUKAGE               3
#define FLOOR_BLOOD                4
#define FLOOR_SLIME                5

#define GLOWSPEED                  8
#define STROBEBRIGHT               5
#define FASTDARK                   15
#define SLOWDARK                   35
#define PLATWAIT                   3
#define PLATSPEED                  FRACUNIT
#define MAXPLATS                   30*256        // CHANGED FOR HIRES
#define VDOORSPEED                 FRACUNIT*2
#define VDOORWAIT                  150
#define CEILSPEED                  FRACUNIT
#define CEILWAIT                   150
#define MAXCEILINGS                30
#define FLOORSPEED                 FRACUNIT

 // max # of wall switches in a level
#define MAXSWITCHES                100

 // 4 players, 4 buttons each at once, max.
#define MAXBUTTONS                 32

 // 1 second, in ticks. 
#define BUTTONTIME                 35             

//      Define values for map objects
#define MO_TELEPORTMAN             14

#define FRICTION_MASK              0x100

#define ELEVATORSPEED              (FRACUNIT * 4)

// jff 02/04/98 Define masks, shifts, for fields in 
// generalized linedef types
#define GenFloorBase               0x6000
#define GenCeilingBase             0x4000
#define GenDoorBase                0x3c00
#define GenLockedBase              0x3800
#define GenLiftBase                0x3400
#define GenStairsBase              0x3000
#define GenCrusherBase             0x2F80

#define TriggerType                0x0007
#define TriggerTypeShift           0

// define masks and shifts for the floor type fields
#define FloorCrush                 0x1000
#define FloorChange                0x0c00
#define FloorTarget                0x0380
#define FloorDirection             0x0040
#define FloorModel                 0x0020
#define FloorSpeed                 0x0018

#define FloorCrushShift            12
#define FloorChangeShift           10
#define FloorTargetShift           7
#define FloorDirectionShift        6
#define FloorModelShift            5
#define FloorSpeedShift            3

// define masks and shifts for the ceiling type fields
#define CeilingCrush               0x1000
#define CeilingChange              0x0c00
#define CeilingTarget              0x0380
#define CeilingDirection           0x0040
#define CeilingModel               0x0020
#define CeilingSpeed               0x0018

#define CeilingCrushShift          12
#define CeilingChangeShift         10
#define CeilingTargetShift         7
#define CeilingDirectionShift      6
#define CeilingModelShift          5
#define CeilingSpeedShift          3

// define masks and shifts for the lift type fields
#define LiftTarget                 0x0300
#define LiftDelay                  0x00c0
#define LiftMonster                0x0020
#define LiftSpeed                  0x0018

#define LiftTargetShift            8
#define LiftDelayShift             6
#define LiftMonsterShift           5
#define LiftSpeedShift             3

// define masks and shifts for the stairs type fields
#define StairIgnore                0x0200
#define StairDirection             0x0100
#define StairStep                  0x00c0
#define StairMonster               0x0020
#define StairSpeed                 0x0018

#define StairIgnoreShift           9
#define StairDirectionShift        8
#define StairStepShift             6
#define StairMonsterShift          5
#define StairSpeedShift            3

// define masks and shifts for the crusher type fields
#define CrusherSilent              0x0040
#define CrusherMonster             0x0020
#define CrusherSpeed               0x0018

#define CrusherSilentShift         6
#define CrusherMonsterShift        5
#define CrusherSpeedShift          3

// define masks and shifts for the door type fields
#define DoorDelay                  0x0300
#define DoorMonster                0x0080
#define DoorKind                   0x0060
#define DoorSpeed                  0x0018

#define DoorDelayShift             8
#define DoorMonsterShift           7
#define DoorKindShift              5
#define DoorSpeedShift             3

// define masks and shifts for the locked door type fields
#define LockedNKeys                0x0200
#define LockedKey                  0x01c0
#define LockedKind                 0x0020
#define LockedSpeed                0x0018

#define LockedNKeysShift           9
#define LockedKeyShift             6
#define LockedKindShift            5
#define LockedSpeedShift           3

#define DAMAGE_MASK                0x60
#define DAMAGE_SHIFT               5
#define SECRET_MASK                0x80
#define SECRET_SHIFT               7
#define FRICTION_MASK              0x100
#define FRICTION_SHIFT             8
#define PUSH_MASK                  0x200
#define PUSH_SHIFT                 9


// define names for the TriggerType field of the general linedefs
typedef enum
{
    WalkOnce,
    WalkMany,
    SwitchOnce,
    SwitchMany,
    GunOnce,
    GunMany,
    PushOnce,
    PushMany
} triggertype_e;

// define names for the Speed field of the general linedefs
typedef enum
{
    SpeedSlow,
    SpeedNormal,
    SpeedFast,
    SpeedTurbo
} motionspeed_e;

// define names for the Target field of the general floor
typedef enum
{
    FtoHnF,
    FtoLnF,
    FtoNnF,
    FtoLnC,
    FtoC,
    FbyST,
    Fby24,
    Fby32
} floortarget_e;

// define names for the Changer Type field of the general floor
typedef enum
{
    FNoChg,
    FChgZero,
    FChgTxt,
    FChgTyp
} floorchange_e;

// define names for the Change Model field of the general floor
typedef enum
{
    FTriggerModel,
    FNumericModel
} floormodel_t;

// define names for the Target field of the general ceiling
typedef enum
{
    CtoHnC,
    CtoLnC,
    CtoNnC,
    CtoHnF,
    CtoF,
    CbyST,
    Cby24,
    Cby32
} ceilingtarget_e;

// define names for the Changer Type field of the general ceiling
typedef enum
{
    CNoChg,
    CChgZero,
    CChgTxt,
    CChgTyp
} ceilingchange_e;

// define names for the Change Model field of the general ceiling
typedef enum
{
    CTriggerModel,
    CNumericModel
} ceilingmodel_t;

// define names for the Target field of the general lift
typedef enum
{
    F2LnF,
    F2NnF,
    F2LnC,
    LnF2HnF
} lifttarget_e;

// define names for the door Kind field of the general ceiling
typedef enum
{
    OdCDoor,
    ODoor,
    CdODoor,
    CDoor
} doorkind_e;

// define names for the locked door Kind field of the general ceiling
typedef enum
{
    AnyKey,
    RCard,
    BCard,
    YCard,
    RSkull,
    BSkull,
    YSkull,
    AllKeys
} keykind_e;

//jff 2/23/98 identify the special classes that can share sectors
typedef enum
{
    floor_special,
    ceiling_special,
    lighting_special
} special_e;

typedef enum
{
    elevateUp,
    elevateDown,
    elevateCurrent
} elevator_e;

typedef struct
{
    thinker_t   thinker;
    elevator_e  type;
    sector_t    *sector;
    int         direction;
    fixed_t     floordestheight;
    fixed_t     ceilingdestheight;
    fixed_t     speed;
} elevator_t;

//jff 3/15/98 pure texture/type change for better generalized support
typedef enum
{
    trigChangeOnly,
    numChangeOnly
} change_e;

//
// P_LIGHTS
//
typedef struct
{
    thinker_t    thinker;
    sector_t*    sector;
    int          count;
    int          maxlight;
    int          minlight;
    
} fireflicker_t;

typedef struct
{
    thinker_t    thinker;
    sector_t*    sector;
    int          count;
    int          maxlight;
    int          minlight;
    int          maxtime;
    int          mintime;
    
} lightflash_t;

typedef struct
{
    thinker_t    thinker;
    sector_t*    sector;
    int          count;
    int          minlight;
    int          maxlight;
    int          darktime;
    int          brighttime;
    
} strobe_t;

typedef struct
{
    thinker_t    thinker;
    sector_t*    sector;
    int          minlight;
    int          maxlight;
    int          direction;

} glow_t;

//
// P_DOORS
//
typedef enum
{
    doorNormal,
    doorClose30ThenOpen,
    doorClose,
    doorOpen,
    doorRaiseIn5Mins,
    doorBlazeRaise,
    doorBlazeOpen,
    doorBlazeClose,

    //jff 02/05/98 add generalized door types
    genRaise,
    genBlazeRaise,
    genOpen,
    genBlazeOpen,
    genClose,
    genBlazeClose,
    genCdO,
    genBlazeCdO

} vldoor_e;

typedef struct
{
    thinker_t    thinker;
    vldoor_e     type;
    sector_t*    sector;
    fixed_t      topheight;
    fixed_t      speed;

    // 1 = up, 0 = waiting at top, -1 = down
    int          direction;
    
    // tics to wait at the top
    int          topwait;
    // (keep in case a door going down is reset)
    // when it reaches 0, start going down
    int          topcountdown;
    
    //jff 1/31/98 keep track of line door is triggered by
    line_t       *line;

    //killough 10/98: sector tag for gradual lighting effects
    int          lighttag;

} vldoor_t;

//
// P_FLOOR
//
typedef enum
{
    // lower floor to highest surrounding floor
    lowerFloor,
    
    // lower floor to lowest surrounding floor
    lowerFloorToLowest,
    
    // lower floor to highest surrounding floor VERY FAST
    turboLower,
    
    // raise floor to lowest surrounding CEILING
    raiseFloor,
    
    // raise floor to next highest surrounding floor
    raiseFloorToNearest,

    //jff 02/03/98 lower floor to next lowest neighbor
    lowerFloorToNearest,

    //jff 02/03/98 lower floor 24 absolute
    lowerFloor24,

    //jff 02/03/98 lower floor 32 absolute
    lowerFloor32Turbo,

    // raise floor to shortest height texture around it
    raiseToTexture,
    
    // lower floor to lowest surrounding floor
    //  and change floorpic
    lowerAndChange,
  
    raiseFloor24,

    //jff 02/03/98 raise floor 32 absolute
    raiseFloor32Turbo,

    raiseFloor24AndChange,
    raiseFloorCrush,

     // raise to next highest floor, turbo-speed
    raiseFloorTurbo,       
    donutRaise,
    raiseFloor512,
    
    //jff 02/04/98  add types for generalized floor mover
    genFloor,
    genFloorChg,
    genFloorChg0,
    genFloorChgT,

    buildStair,
    genBuildStair

} floor_e;

typedef enum
{
    build8,        // slowly build by 8
    turbo16        // quickly build by 16
    
} stair_e;

typedef struct
{
    thinker_t    thinker;
    floor_e      type;
    dboolean     crush;
    sector_t*    sector;
    int          direction;
    int          newspecial;
    short        texture;
    fixed_t      floordestheight;
    fixed_t      speed;
    dboolean     stopsound;

    //jff 3/14/98 add to fix bug in change transfers
    int          oldspecial;

} floormove_t;

typedef enum
{
    ok,
    crushed,
    pastdest
    
} result_e;

//
// P_CEILNG
//
typedef enum
{
    lowerToFloor,
    raiseToHighest,
    lowerToLowest,
    lowerToMaxFloor,
    lowerAndCrush,
    crushAndRaise,
    fastCrushAndRaise,
    silentCrushAndRaise,

    //jff 02/04/98 add types for generalized ceiling mover
    genCeiling,
    genCeilingChg,
    genCeilingChg0,
    genCeilingChgT,

    //jff 02/05/98 add types for generalized ceiling mover
    genCrusher,
    genSilentCrusher

} ceiling_e;

typedef struct
{
    thinker_t                   thinker;
    ceiling_e                   type;
    sector_t*                   sector;
    fixed_t                     bottomheight;
    fixed_t                     topheight;
    fixed_t                     speed;
    dboolean                     crush;

    // 1 = up, 0 = waiting, -1 = down
    int                         direction;

    // ID
    int                         tag;                   
    int                         olddirection;
    
    // jff 2/22/98 copied from killough's plats
    struct ceilinglist_s        *list;

    //jff 02/04/98 add these to support ceiling changers
    int                         newspecial;
    short                       texture;
    fixed_t                     oldspeed;

    //jff 3/14/98 add to fix bug in change transfers
    int                         oldspecial;

} ceiling_t;

typedef struct ceilinglist_s
{
    ceiling_t                   *ceiling;
    struct ceilinglist_s        *next, **prev;

} ceilinglist_t;

//
// P_SWITCH
//
typedef struct
{
    char         name1[9];
    char         name2[9];
    short        episode;
    
} switchlist_t;

typedef enum
{
    at_top,
    at_middle,
    at_bottom

} bwhere_e;

typedef struct
{
    line_t*      line;
    bwhere_e     where;
    int          btexture;
    int          btimer;
    degenmobj_t  *soundorg;

} button_t;

//
// P_PLATS
//
typedef enum
{
    up,
    down,
    waiting,
    in_stasis

} plat_e;

typedef enum
{
    perpetualRaise,
    downWaitUpStay,
    raiseAndChange,
    raiseToNearestAndChange,
    blazeDWUS,
    genLift,            // jff added to support generalized Plat types
    genPerpetual,
    toggleUpDn

} plattype_e;

typedef struct
{
    thinker_t    thinker;
    sector_t*    sector;
    fixed_t      speed;
    fixed_t      low;
    fixed_t      high;
    int          wait;
    int          count;
    plat_e       status;
    plat_e       oldstatus;
    dboolean      crush;
    int          tag;
    plattype_e   type;
    
    struct platlist_s  *list;   // killough
} plat_t;

// New limit-free plat structure -- killough
typedef struct platlist_s
{
    plat_t             *plat;
    struct platlist_s  *next, **prev;
} platlist_t;

// killough 3/7/98: Add generalized scroll effects

typedef struct
{
    thinker_t thinker;          // Thinker structure for scrolling
    fixed_t dx, dy;             // (dx,dy) scroll speeds
    int affectee;               // Number of affected sidedef, sector, tag, or whatever
    int control;                // Control sector (-1 if none) used to control scrolling
    fixed_t last_height;        // Last known height of control sector
    fixed_t vdx, vdy;           // Accumulated velocity if accelerative
    int accel;                  // Whether it's accelerative
    enum
    {
        sc_side,
        sc_floor,
        sc_ceiling,
        sc_carry,
        sc_carry_ceiling,       // killough 4/11/98: carry objects hanging on ceilings
    } type;                     // Type of scroll effect
} scroll_t;

// phares 3/20/98: added new model of Pushers for push/pull effects

typedef struct
{
    thinker_t   thinker;        // Thinker structure for Pusher
    enum
    {
        p_push,
        p_pull,
        p_wind,
        p_current 
    } type;
    mobj_t      *source;        // Point source if point pusher
    int         x_mag;          // X Strength
    int         y_mag;          // Y Strength
    int         magnitude;      // Vector strength for point pusher
    int         radius;         // Effective radius for point pusher
    int         x;              // X of point source if point pusher
    int         y;              // Y of point source if point pusher
    int         affectee;       // Number of affected sector
} pusher_t;

// phares 3/14/98
//
// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.
//
typedef struct msecnode_s
{
    sector_t            *m_sector;      // a sector containing this object
    struct mobj_s       *m_thing;       // this object
    struct msecnode_s   *m_tprev;       // prev msecnode_t for this thing
    struct msecnode_s   *m_tnext;       // next msecnode_t for this thing
    struct msecnode_s   *m_sprev;       // prev msecnode_t for this sector
    struct msecnode_s   *m_snext;       // next msecnode_t for this sector
    dboolean            visited;        // killough 4/4/98, 4/7/98: used in search algorithms
} msecnode_t;


button_t   buttonlist[MAXBUTTONS]; 


// Temporary holder for thing_sectorlist threads
extern msecnode_t      *sector_list;    // phares 3/16/98

// Maintain a freelist of msecnode_t's to reduce memory allocs and frees.
extern msecnode_t      *headsecnode;

//
// End-level timer (-TIMER option)
//
extern dboolean   levelTimer;
extern dboolean   *isliquid;
extern dboolean   *isteleport;

extern int        levelTimeCount;

extern platlist_t *activeplats; // killough

extern ceilinglist_t* activeceilings;


//
// P_TELEPT
//
int         EV_Teleport(line_t* line, int side, mobj_t* thing);
int         EV_BuildStairs(line_t *line, stair_e type);
int         EV_DoFloor(line_t *line, floor_e floortype);
int         twoSided(int sector, int line);
int         P_FindSectorFromLineTag(const line_t *line, int start);
int         P_FindMinSurroundingLight(sector_t* sector, int max);
int         EV_StartLightStrobing(line_t *line);
int         EV_TurnTagLightsOff(line_t* line);
int         EV_LightTurnOn(line_t* line, int bright);
int         EV_DoPlat(line_t* line, plattype_e type, int amount);
int         EV_DoDoor(line_t *line, vldoor_e type);
int         EV_DoLockedDoor(line_t *line, vldoor_e type, mobj_t *thing);
int         EV_DoCeiling(line_t *line, ceiling_e type);

mobj_t      *P_GetPushThing(int);

//
// SPECIAL
//
dboolean    EV_DoDonut(line_t* line);
dboolean    EV_SilentTeleport(line_t *line, int side, mobj_t *thing);
dboolean    EV_SilentLineTeleport(line_t *line, int side, mobj_t *thing, dboolean reverse);
dboolean    P_CanUnlockGenDoor(line_t *line, player_t *player);
dboolean    P_SectorActive(special_e t, sector_t *sec);
dboolean    EV_DoChange(line_t *line, change_e changetype);
dboolean    EV_DoElevator(line_t *line, elevator_e elevtype);
dboolean    EV_StopPlat(line_t* line);
dboolean    P_CheckLineSide(mobj_t *actor, fixed_t x, fixed_t y);
dboolean    P_SectorHasLightSpecial(sector_t *sec); 
dboolean    P_CheckTag(line_t *line);
dboolean    EV_CeilingCrushStop(line_t* line);
dboolean    P_ActivateInStasisCeiling(line_t* line);

// when needed
dboolean    P_UseSpecialLine(mobj_t* thing, line_t* line, int side);
dboolean    EV_DoGenFloor(line_t *line);
dboolean    EV_DoGenCeiling(line_t *line);
dboolean    EV_DoGenLift(line_t *line);
dboolean    EV_DoGenStairs(line_t *line);
dboolean    EV_DoGenCrusher(line_t *line);
dboolean    EV_DoGenDoor(line_t *line);
dboolean    EV_DoGenLockedDoor(line_t *line);

// at game start
void        P_InitPicAnims (void);

// at map load
void        P_SpawnSpecials (void);
void        P_SetLiquids(void); 

// every tic
void        P_UpdateSpecials (void);
void        P_ResurrectPlayer(player_t *player);
void        P_ShootSpecialLine(mobj_t* thing, line_t* line);
void        P_CrossSpecialLine(line_t *line, int side, mobj_t *thing);
void        P_PlayerInSpecialSector (player_t* player);
void        P_SpawnFireFlicker (sector_t* sector);
void        T_LightFlash (lightflash_t* flash);
void        P_SpawnLightFlash (sector_t* sector);
void        T_StrobeFlash (strobe_t* flash);
void        P_SpawnStrobeFlash(sector_t* sector, int fastOrSlow, int inSync);
void        T_Glow(glow_t* g);
void        P_SpawnGlowingLight(sector_t* sector);
void        P_ChangeSwitchTexture(line_t* line, int useAgain);
void        P_InitSwitchList(void);
void        T_PlatRaise(plat_t* plat);
void        P_AddActivePlat(plat_t* plat);
void        P_RemoveActivePlat(plat_t* plat);
void        P_ActivateInStasis(int tag);
void        EV_VerticalDoor(line_t* line, mobj_t* thing);
void        T_VerticalDoor (vldoor_t* door);
void        P_SpawnDoorCloseIn30 (sector_t* sec);
void        P_SpawnDoorRaiseIn5Mins(sector_t* sec);
void        T_MoveCeiling (ceiling_t* ceiling);
void        P_AddActiveCeiling(ceiling_t* c);
void        P_RemoveActiveCeiling(ceiling_t* c);
void        P_RemoveAllActiveCeilings(void);
void        P_RemoveAllActivePlats(void);
void        T_MoveFloor( floormove_t* floor);
void        T_MoveElevator(elevator_t *elevator);
void        T_Scroll(scroll_t *s);
void        T_FireFlicker(fireflicker_t *flick);
void        P_StartButton(line_t *line, bwhere_e w, int texture, int time);
void        P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y);
void        P_FreeSecNodeList(void);
void        P_DelSeclist(msecnode_t *node);
void        T_Pusher(pusher_t *);      // phares 3/20/98: Push thinker

sector_t*   getSector(int currentSector, int line, int side);
sector_t*   getNextSector(line_t* line, sector_t* sec);
sector_t*   P_FindModelFloorSector(fixed_t floordestheight, int secnum); //jff 02/04/98
sector_t*   P_FindModelCeilingSector(fixed_t ceildestheight, int secnum); //jff 02/04/98 

side_t*     getSide(int currentSector, int line, int side);

fixed_t     P_FindLowestFloorSurrounding(sector_t* sec);
fixed_t     P_FindHighestFloorSurrounding(sector_t* sec);
fixed_t     P_FindNextHighestFloor(sector_t* sec, int currentheight);
fixed_t     P_FindNextLowestFloor(sector_t *sec, int currentheight);
fixed_t     P_FindLowestCeilingSurrounding(sector_t* sec);
fixed_t     P_FindHighestCeilingSurrounding(sector_t* sec);
fixed_t     P_FindNextLowestCeiling(sector_t *sec, int currentheight); // jff 2/04/98
fixed_t     P_FindNextHighestCeiling(sector_t *sec, int currentheight); // jff 2/04/98
fixed_t     P_FindShortestTextureAround(int secnum); // jff 2/04/98
fixed_t     P_FindShortestUpperAround(int secnum); // jff 2/04/98

result_e    T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, dboolean crush, int floorOrCeiling, int direction);

void        P_LoadTerrainTypeDefs(void);
void        P_InitTerrainTypes(void);
int         P_GetTerrainTypeForPoint(fixed_t x, fixed_t y, int position);
dboolean    P_IsSecret(const sector_t *sec);
dboolean    P_WasSecret(const sector_t *sec);


#if 0 // UNUSED
//
//      Sliding doors...
//
typedef enum
{
    sd_opening,
    sd_waiting,
    sd_closing

} sd_e;



typedef enum
{
    sdt_openOnly,
    sdt_closeOnly,
    sdt_openAndClose

} sdt_e;




typedef struct
{
    thinker_t    thinker;
    sdt_e        type;
    line_t*      line;
    int          frame;
    int          whichDoorIndex;
    int          timer;
    sector_t*    frontsector;
    sector_t*    backsector;
    sd_e         status;

} slidedoor_t;



typedef struct
{
    char         frontFrame1[9];
    char         frontFrame2[9];
    char         frontFrame3[9];
    char         frontFrame4[9];
    char         backFrame1[9];
    char         backFrame2[9];
    char         backFrame3[9];
    char         backFrame4[9];
    
} slidename_t;



typedef struct
{
    int          frontFrames[4];
    int          backFrames[4];

} slideframe_t;



// how many frames of animation
#define SNUMFRAMES               4
#define SDOORWAIT                35*3
#define SWAITTICS                4

// how many diff. types of anims
#define MAXSLIDEDOORS        5                            

void P_InitSlidingDoorFrames(void);

void
EV_SlidingDoor
( line_t*        line,
  mobj_t*        thing );
#endif


#endif
