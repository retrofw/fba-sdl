#include "tiles_generic.h"
#include "sn76496.h"
#include "bitswap.h"
#include "z80.h"
#include "mc8123.h"

static unsigned char System1InputPort0[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char System1InputPort1[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char System1InputPort2[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char System1Dip[2]           = {0, 0};
static unsigned char System1Input[3]         = {0x00, 0x00, 0x00 };
static unsigned char System1Reset            = 0;

static unsigned char *Mem                    = NULL;
static unsigned char *MemEnd                 = NULL;
static unsigned char *RamStart               = NULL;
static unsigned char *RamEnd                 = NULL;
static unsigned char *System1Rom1            = NULL;
static unsigned char *System1Rom2            = NULL;
static unsigned char *System1PromRed         = NULL;
static unsigned char *System1PromGreen       = NULL;
static unsigned char *System1PromBlue        = NULL;
static unsigned char *System1Ram1            = NULL;
static unsigned char *System1Ram2            = NULL;
static unsigned char *System1SpriteRam       = NULL;
static unsigned char *System1PaletteRam      = NULL;
static unsigned char *System1BgRam           = NULL;
static unsigned char *System1VideoRam        = NULL;
static unsigned char *System1BgCollisionRam  = NULL;
static unsigned char *System1SprCollisionRam = NULL;
static unsigned char *System1deRam           = NULL;
static unsigned char *System1efRam           = NULL;
static unsigned char *System1f4Ram           = NULL;
static unsigned char *System1fcRam           = NULL;
static unsigned int  *System1Palette         = NULL;
static unsigned char *System1Tiles           = NULL;
static unsigned char *System1Sprites         = NULL;
static unsigned char *System1TempRom         = NULL;
static unsigned char *SpriteOnScreenMap      = NULL;
static unsigned char *System1Fetch1          = NULL;
static unsigned char *System1MC8123Key       = NULL;
static UINT32        *System1TilesPenUsage   = NULL;

static unsigned char  System1ScrollX[2];
static unsigned char  System1ScrollY;
static int            System1BgScrollX;
static int            System1BgScrollY;
static int            System1VideoMode;
static int            System1FlipScreen;
static int            System1SoundLatch;
static int            System1RomBank;
static int            NoboranbInp16Step;
static int            NoboranbInp17Step;
static int            NoboranbInp23Step;
static unsigned char  BlockgalDial1;
static unsigned char  BlockgalDial2;

static int System1SpriteRomSize;
static int System1NumTiles;
static int System1SpriteXOffset;
static int System1ColourProms = 0;
static int System1BankedRom = 0;

typedef void (*Decode)();
static Decode DecodeFunction;

typedef void (*MakeInputs)();
static MakeInputs MakeInputsFunction;

static int nCyclesDone[2], nCyclesTotal[2];
static int nCyclesSegment;

static Z80_Regs Z80_0;
static Z80_Regs Z80_1;
static void OpenCPU(int nCPU);
static void CloseCPU(int nCPU);

struct CPU_Config {
	Z80ReadIoHandler Z80In;
	Z80WriteIoHandler Z80Out;
	Z80ReadProgHandler Z80Read;
	Z80WriteProgHandler Z80Write;
	Z80ReadOpHandler Z80ReadOp;
	Z80ReadOpArgHandler Z80ReadOpArg;
};

static CPU_Config Z80_0_Config;
static CPU_Config Z80_1_Config;

/*==============================================================================================
Input Definitions
===============================================================================================*/

static struct BurnInputInfo BlockgalInputList[] = {
	{"Coin 1"            , BIT_DIGITAL  , System1InputPort2 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , System1InputPort2 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , System1InputPort2 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , System1InputPort2 + 5, "p2 start"  },

	{"P1 Left"           , BIT_DIGITAL  , System1InputPort0 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , System1InputPort0 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , System1InputPort2 + 6, "p1 fire 1" },

	{"P2 Left"           , BIT_DIGITAL  , System1InputPort0 + 2, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , System1InputPort0 + 3, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , System1InputPort2 + 7, "p2 fire 1" },

	{"Reset"             , BIT_DIGITAL  , &System1Reset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , System1InputPort2 + 3, "service"   },
	{"Dip 1"             , BIT_DIPSWITCH, System1Dip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, System1Dip + 1       , "dip"       },
};

STDINPUTINFO(Blockgal);

static struct BurnInputInfo FlickyInputList[] = {
	{"Coin 1"            , BIT_DIGITAL  , System1InputPort2 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , System1InputPort2 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , System1InputPort2 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , System1InputPort2 + 5, "p2 start"  },

	{"P1 Left"           , BIT_DIGITAL  , System1InputPort0 + 7, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , System1InputPort0 + 6, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , System1InputPort0 + 2, "p1 fire 1" },

	{"P2 Left"           , BIT_DIGITAL  , System1InputPort1 + 7, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , System1InputPort1 + 6, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , System1InputPort1 + 2, "p2 fire 1" },

	{"Reset"             , BIT_DIGITAL  , &System1Reset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , System1InputPort2 + 3, "service"   },
	{"Test"              , BIT_DIGITAL  , System1InputPort2 + 2, "diag"      },
	{"Dip 1"             , BIT_DIPSWITCH, System1Dip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, System1Dip + 1       , "dip"       },
};

STDINPUTINFO(Flicky);

static struct BurnInputInfo MyheroInputList[] = {
	{"Coin 1"            , BIT_DIGITAL  , System1InputPort2 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , System1InputPort2 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , System1InputPort2 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , System1InputPort2 + 5, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , System1InputPort0 + 5, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , System1InputPort0 + 4, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , System1InputPort0 + 7, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , System1InputPort0 + 6, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , System1InputPort0 + 1, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , System1InputPort0 + 2, "p1 fire 2" },

	{"P2 Up"             , BIT_DIGITAL  , System1InputPort1 + 5, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , System1InputPort1 + 4, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , System1InputPort1 + 7, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , System1InputPort1 + 6, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , System1InputPort1 + 1, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , System1InputPort1 + 2, "p2 fire 2" },

	{"Reset"             , BIT_DIGITAL  , &System1Reset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , System1InputPort2 + 3, "service"   },
	{"Test"              , BIT_DIGITAL  , System1InputPort2 + 2, "diag"      },
	{"Dip 1"             , BIT_DIPSWITCH, System1Dip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, System1Dip + 1       , "dip"       },
};

STDINPUTINFO(Myhero);

static struct BurnInputInfo SeganinjInputList[] = {
	{"Coin 1"            , BIT_DIGITAL  , System1InputPort2 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , System1InputPort2 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , System1InputPort2 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , System1InputPort2 + 5, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , System1InputPort0 + 5, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , System1InputPort0 + 4, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , System1InputPort0 + 7, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , System1InputPort0 + 6, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , System1InputPort0 + 0, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , System1InputPort0 + 1, "p1 fire 2" },
	{"P1 Fire 3"         , BIT_DIGITAL  , System1InputPort0 + 2, "p1 fire 3" },

	{"P2 Up"             , BIT_DIGITAL  , System1InputPort1 + 5, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , System1InputPort1 + 4, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , System1InputPort1 + 7, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , System1InputPort1 + 6, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , System1InputPort1 + 0, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , System1InputPort1 + 1, "p2 fire 2" },
	{"P2 Fire 3"         , BIT_DIGITAL  , System1InputPort1 + 2, "p2 fire 3" },

	{"Reset"             , BIT_DIGITAL  , &System1Reset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , System1InputPort2 + 3, "service"   },
	{"Test"              , BIT_DIGITAL  , System1InputPort2 + 2, "diag"      },
	{"Dip 1"             , BIT_DIPSWITCH, System1Dip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, System1Dip + 1       , "dip"       },
};

STDINPUTINFO(Seganinj);

static struct BurnInputInfo UpndownInputList[] = {
	{"Coin 1"            , BIT_DIGITAL  , System1InputPort2 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , System1InputPort2 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , System1InputPort2 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , System1InputPort2 + 5, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , System1InputPort0 + 5, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , System1InputPort0 + 4, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , System1InputPort0 + 7, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , System1InputPort0 + 6, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , System1InputPort0 + 2, "p1 fire 1" },

	{"P2 Up"             , BIT_DIGITAL  , System1InputPort1 + 5, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , System1InputPort1 + 4, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , System1InputPort1 + 7, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , System1InputPort1 + 6, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , System1InputPort1 + 2, "p2 fire 1" },

	{"Reset"             , BIT_DIGITAL  , &System1Reset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , System1InputPort2 + 3, "service"   },
	{"Test"              , BIT_DIGITAL  , System1InputPort2 + 2, "diag"      },
	{"Dip 1"             , BIT_DIPSWITCH, System1Dip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, System1Dip + 1       , "dip"       },
};

STDINPUTINFO(Upndown);

static struct BurnInputInfo WboyInputList[] = {
	{"Coin 1"            , BIT_DIGITAL  , System1InputPort2 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , System1InputPort2 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , System1InputPort2 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , System1InputPort2 + 5, "p2 start"  },

	{"P1 Left"           , BIT_DIGITAL  , System1InputPort0 + 7, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , System1InputPort0 + 6, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , System1InputPort0 + 1, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , System1InputPort0 + 2, "p1 fire 2" },

	{"P2 Left"           , BIT_DIGITAL  , System1InputPort1 + 7, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , System1InputPort1 + 6, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , System1InputPort1 + 1, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , System1InputPort1 + 2, "p2 fire 2" },

	{"Reset"             , BIT_DIGITAL  , &System1Reset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , System1InputPort2 + 3, "service"   },
	{"Test"              , BIT_DIGITAL  , System1InputPort2 + 2, "diag"      },
	{"Dip 1"             , BIT_DIPSWITCH, System1Dip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, System1Dip + 1       , "dip"       },
};

STDINPUTINFO(Wboy);

static struct BurnInputInfo WmatchInputList[] = {
	{"Coin 1"            , BIT_DIGITAL  , System1InputPort2 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , System1InputPort2 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , System1InputPort2 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , System1InputPort2 + 5, "p2 start"  },

	{"P1 Left Up"        , BIT_DIGITAL  , System1InputPort0 + 5, "p1 up"     },
	{"P1 Left Down"      , BIT_DIGITAL  , System1InputPort0 + 4, "p1 down"   },
	{"P1 Left Left"      , BIT_DIGITAL  , System1InputPort0 + 7, "p1 left"   },
	{"P1 Left Right"     , BIT_DIGITAL  , System1InputPort0 + 6, "p1 right"  },
	{"P1 Right Up"       , BIT_DIGITAL  , System1InputPort0 + 1, "p1 fire 1" },
	{"P1 Right Down"     , BIT_DIGITAL  , System1InputPort0 + 0, "p1 fire 2" },
	{"P1 Right Left"     , BIT_DIGITAL  , System1InputPort0 + 3, "p1 fire 3" },
	{"P1 Right Right"    , BIT_DIGITAL  , System1InputPort0 + 2, "p1 fire 4" },
	{"P1 Fire 1"         , BIT_DIGITAL  , System1InputPort2 + 6, "p1 fire 5" },

	{"P2 Left Up"        , BIT_DIGITAL  , System1InputPort1 + 5, "p2 up"     },
	{"P2 Left Down"      , BIT_DIGITAL  , System1InputPort1 + 4, "p2 down"   },
	{"P2 Left Left"      , BIT_DIGITAL  , System1InputPort1 + 7, "p2 left"   },
	{"P2 Left Right"     , BIT_DIGITAL  , System1InputPort1 + 6, "p2 right"  },
	{"P2 Right Up"       , BIT_DIGITAL  , System1InputPort1 + 1, "p2 fire 1" },
	{"P2 Right Down"     , BIT_DIGITAL  , System1InputPort1 + 0, "p2 fire 2" },
	{"P2 Right Left"     , BIT_DIGITAL  , System1InputPort1 + 3, "p2 fire 3" },
	{"P2 Right Right"    , BIT_DIGITAL  , System1InputPort1 + 2, "p2 fire 4" },
	{"P2 Fire 1"         , BIT_DIGITAL  , System1InputPort2 + 7, "p2 fire 5" },

	{"Reset"             , BIT_DIGITAL  , &System1Reset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , System1InputPort2 + 3, "service"   },
	{"Test"              , BIT_DIGITAL  , System1InputPort2 + 2, "diag"      },
	{"Dip 1"             , BIT_DIPSWITCH, System1Dip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, System1Dip + 1       , "dip"       },
};

STDINPUTINFO(Wmatch);

inline void System1ClearOpposites(unsigned char* nJoystickInputs)
{
	if ((*nJoystickInputs & 0x30) == 0x30) {
		*nJoystickInputs &= ~0x30;
	}
	if ((*nJoystickInputs & 0xc0) == 0xc0) {
		*nJoystickInputs &= ~0xc0;
	}
}

static inline void System1MakeInputs()
{
	// Reset Inputs
	System1Input[0] = System1Input[1] = System1Input[2] = 0x00;

	// Compile Digital Inputs
	for (int i = 0; i < 8; i++) {
		System1Input[0] |= (System1InputPort0[i] & 1) << i;
		System1Input[1] |= (System1InputPort1[i] & 1) << i;
		System1Input[2] |= (System1InputPort2[i] & 1) << i;
	}

	// Clear Opposites
	System1ClearOpposites(&System1Input[0]);
	System1ClearOpposites(&System1Input[1]);
}

static inline void BlockgalMakeInputs()
{
	System1Input[2] = 0x00;
	
	for (int i = 0; i < 8; i++) {
		System1Input[2] |= (System1InputPort2[i] & 1) << i;
	}
	
	if (System1InputPort0[0]) BlockgalDial1 += 0x04;
	if (System1InputPort0[1]) BlockgalDial1 -= 0x04;
	
	if (System1InputPort0[2]) BlockgalDial2 += 0x04;
	if (System1InputPort0[3]) BlockgalDial2 -= 0x04;;
}

#define SYSTEM1_COINAGE(dipval)								\
	{0   , 0xfe, 0   , 16   , "Coin A"                },				\
	{dipval, 0x01, 0x0f, 0x07, "4 Coins 1 Credit"       },				\
	{dipval, 0x01, 0x0f, 0x08, "3 Coins 1 Credit"       },				\
	{dipval, 0x01, 0x0f, 0x09, "2 Coins 1 Credit"       },				\
	{dipval, 0x01, 0x0f, 0x05, "2 Coins 1 Credit 4/2 5/3 6/4"},			\
	{dipval, 0x01, 0x0f, 0x04, "2 Coins 1 Credit 4/3"   },				\
	{dipval, 0x01, 0x0f, 0x0f, "1 Coin  1 Credit"       },				\
	{dipval, 0x01, 0x0f, 0x00, "1 Coin  1 Credit"       },				\
	{dipval, 0x01, 0x0f, 0x03, "1 Coin  1 Credit 5/6"   },				\
	{dipval, 0x01, 0x0f, 0x02, "1 Coin  1 Credit 4/5"   },				\
	{dipval, 0x01, 0x0f, 0x01, "1 Coin  1 Credit 2/3"   },				\
	{dipval, 0x01, 0x0f, 0x06, "2 Coins 3 Credits"      },				\
	{dipval, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"      },				\
	{dipval, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"      },				\
	{dipval, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"      },				\
	{dipval, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"      },				\
	{dipval, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"      },				\
											\
	{0   , 0xfe, 0   , 16   , "Coin B"                },				\
	{dipval, 0x01, 0xf0, 0x70, "4 Coins 1 Credit"       },				\
	{dipval, 0x01, 0xf0, 0x80, "3 Coins 1 Credit"       },				\
	{dipval, 0x01, 0xf0, 0x90, "2 Coins 1 Credit"       },				\
	{dipval, 0x01, 0xf0, 0x50, "2 Coins 1 Credit 4/2 5/3 6/4"},			\
	{dipval, 0x01, 0xf0, 0x40, "2 Coins 1 Credit 4/3"   },				\
	{dipval, 0x01, 0xf0, 0xf0, "1 Coin  1 Credit"       },				\
	{dipval, 0x01, 0xf0, 0x00, "1 Coin  1 Credit"       },				\
	{dipval, 0x01, 0xf0, 0x30, "1 Coin  1 Credit 5/6"   },				\
	{dipval, 0x01, 0xf0, 0x20, "1 Coin  1 Credit 4/5"   },				\
	{dipval, 0x01, 0xf0, 0x10, "1 Coin  1 Credit 2/3"   },				\
	{dipval, 0x01, 0xf0, 0x60, "2 Coins 3 Credits"      },				\
	{dipval, 0x01, 0xf0, 0xe0, "1 Coin  2 Credits"      },				\
	{dipval, 0x01, 0xf0, 0xd0, "1 Coin  3 Credits"      },				\
	{dipval, 0x01, 0xf0, 0xc0, "1 Coin  4 Credits"      },				\
	{dipval, 0x01, 0xf0, 0xb0, "1 Coin  5 Credits"      },				\
	{dipval, 0x01, 0xf0, 0xa0, "1 Coin  6 Credits"      },

static struct BurnDIPInfo BlockgalDIPList[]=
{
	// Default Values
	{0x0c, 0xff, 0xff, 0xd7, NULL                     },
	{0x0d, 0xff, 0xff, 0xff, NULL                     },

	// Dip 1
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x0c, 0x01, 0x01, 0x00, "Upright"                },
	{0x0c, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x0c, 0x01, 0x02, 0x00, "Off"                    },
	{0x0c, 0x01, 0x02, 0x02, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Lives"                  },
	{0x0c, 0x01, 0x08, 0x08, "2"                      },
	{0x0c, 0x01, 0x08, 0x00, "3"                      },
	
	{0   , 0xfe, 0   , 2   , "Bonus Life"             },
	{0x0c, 0x01, 0x10, 0x10, "10k 30k 60k 100k 150k"  },
	{0x0c, 0x01, 0x10, 0x00, "30k 50k 100k 200k 300k" },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x0c, 0x01, 0x20, 0x20, "Off"                    },
	{0x0c, 0x01, 0x20, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Service Mode"           },
	{0x0c, 0x01, 0x80, 0x80, "Off"                    },
	{0x0c, 0x01, 0x80, 0x00, "On"                     },
	
	// Dip 2
	SYSTEM1_COINAGE(0x0d)
};

STDDIPINFO(Blockgal);

static struct BurnDIPInfo BrainDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xfc, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x14, 0x01, 0x02, 0x02, "Off"                    },
	{0x14, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
};

STDDIPINFO(Brain);

static struct BurnDIPInfo FlickyDIPList[]=
{
	// Default Values
	{0x0d, 0xff, 0xff, 0xff, NULL                     },
	{0x0e, 0xff, 0xff, 0xfe, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x0d)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x0e, 0x01, 0x01, 0x00, "Upright"                },
	{0x0e, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x0e, 0x01, 0x0c, 0x0c, "2"                      },
	{0x0e, 0x01, 0x0c, 0x08, "3"                      },
	{0x0e, 0x01, 0x0c, 0x04, "4"                      },
	{0x0e, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x0e, 0x01, 0x30, 0x30, "30k  80k 160k"          },
	{0x0e, 0x01, 0x30, 0x20, "30k 100k 200k"          },
	{0x0e, 0x01, 0x30, 0x10, "40k 120k 240k"          },
	{0x0e, 0x01, 0x30, 0x00, "40k 140k 280k"          },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x0e, 0x01, 0x40, 0x40, "Easy"                   },
	{0x0e, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Flicky);

static struct BurnDIPInfo GardiaDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0x7c, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x14, 0x01, 0x02, 0x02, "Off"                    },
	{0x14, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x14, 0x01, 0x30, 0x30, " 5k 20k 30k"            },
	{0x14, 0x01, 0x30, 0x20, "10k 25k 50k"            },
	{0x14, 0x01, 0x30, 0x10, "15k 30k 60k"            },
	{0x14, 0x01, 0x30, 0x00, "None"                   },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x14, 0x01, 0x40, 0x40, "Easy"                   },
	{0x14, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Gardia);

static struct BurnDIPInfo MrvikingDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0x7c, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Maximum Credit"         },
	{0x14, 0x01, 0x02, 0x02, "9"                      },
	{0x14, 0x01, 0x02, 0x00, "99"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x14, 0x01, 0x30, 0x30, "10k, 30k then every 30k"},
	{0x14, 0x01, 0x30, 0x20, "20k, 40k then every 30k"},
	{0x14, 0x01, 0x30, 0x10, "30k, then every 30k"    },
	{0x14, 0x01, 0x30, 0x00, "40k, then every 30k"    },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x14, 0x01, 0x40, 0x40, "Easy"                   },
	{0x14, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Mrviking);

static struct BurnDIPInfo MrvikngjDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0x7c, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x14, 0x01, 0x30, 0x30, "10k, 30k then every 30k"},
	{0x14, 0x01, 0x30, 0x20, "20k, 40k then every 30k"},
	{0x14, 0x01, 0x30, 0x10, "30k, then every 30k"    },
	{0x14, 0x01, 0x30, 0x00, "40k, then every 30k"    },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x14, 0x01, 0x40, 0x40, "Easy"                   },
	{0x14, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Mrvikngj);

static struct BurnDIPInfo MyheroDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xfc, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x14, 0x01, 0x02, 0x02, "Off"                    },
	{0x14, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x14, 0x01, 0x30, 0x30, "30k"                    },
	{0x14, 0x01, 0x30, 0x20, "50k"                    },
	{0x14, 0x01, 0x30, 0x10, "70k"                    },
	{0x14, 0x01, 0x30, 0x00, "90k"                    },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x14, 0x01, 0x40, 0x40, "Easy"                   },
	{0x14, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Myhero);

static struct BurnDIPInfo NoboranbDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0x2f, NULL                     },
	{0x14, 0xff, 0xff, 0xff, NULL                     },

	// Dip 1
	{0   , 0xfe, 0   , 4   , "Coin A"                 },
	{0x13, 0x01, 0x03, 0x00, "2 Coins 1 Credit"       },
	{0x13, 0x01, 0x03, 0x03, "1 Coin  1 Credit"       },
	{0x13, 0x01, 0x03, 0x02, "1 Coin  2 Credits"      },
	{0x13, 0x01, 0x03, 0x01, "1 Coin  3 Credits"      },
	
	{0   , 0xfe, 0   , 4   , "Coin B"                 },
	{0x13, 0x01, 0x0c, 0x00, "2 Coins 1 Credit"       },
	{0x13, 0x01, 0x0c, 0x0c, "1 Coin  1 Credit"       },
	{0x13, 0x01, 0x0c, 0x08, "1 Coin  2 Credits"      },
	{0x13, 0x01, 0x0c, 0x04, "1 Coin  3 Credits"      },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x13, 0x01, 0x30, 0x20, "Easy"                   },
	{0x13, 0x01, 0x30, 0x30, "Medium"                 },
	{0x13, 0x01, 0x30, 0x10, "Hard"                   },
	{0x13, 0x01, 0x30, 0x00, "Hardest"                },
	
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x13, 0x01, 0x40, 0x00, "Upright"                },
	{0x13, 0x01, 0x40, 0x40, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x13, 0x01, 0x80, 0x00, "Off"                    },
	{0x13, 0x01, 0x80, 0x80, "On"                     },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x03, 0x02, "2"                      },
	{0x14, 0x01, 0x03, 0x03, "3"                      },
	{0x14, 0x01, 0x03, 0x01, "5"                      },
	{0x14, 0x01, 0x03, 0x00, "99"                     },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x14, 0x01, 0x0c, 0x08, "40k,  80k, 120k, 160k"  },
	{0x14, 0x01, 0x0c, 0x0c, "50k, 100k, 150k"        },
	{0x14, 0x01, 0x0c, 0x04, "60k, 120k, 180k"        },
	{0x14, 0x01, 0x0c, 0x00, "None"                   },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x14, 0x01, 0x10, 0x00, "Off"                    },
	{0x14, 0x01, 0x10, 0x10, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Free Play"              },
	{0x14, 0x01, 0x20, 0x20, "Off"                    },
	{0x14, 0x01, 0x20, 0x00, "On"                     },
};

STDDIPINFO(Noboranb);

static struct BurnDIPInfo Pitfall2DIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xdc, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x14, 0x01, 0x02, 0x02, "Off"                    },
	{0x14, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 2   , "Bonus Life"             },
	{0x14, 0x01, 0x10, 0x10, "20k 50k"                },
	{0x14, 0x01, 0x10, 0x00, "30k 70k"                },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x14, 0x01, 0x20, 0x20, "Off"                    },
	{0x14, 0x01, 0x20, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Time"                   },
	{0x14, 0x01, 0x40, 0x00, "2 minutes"              },
	{0x14, 0x01, 0x40, 0x40, "3 minutes"              },
};

STDDIPINFO(Pitfall2);

static struct BurnDIPInfo PitfalluDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xde, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x06, 0x06, "3"                      },
	{0x14, 0x01, 0x06, 0x04, "4"                      },
	{0x14, 0x01, 0x06, 0x02, "5"                      },
	{0x14, 0x01, 0x06, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 4   , "Starting Stage"         },
	{0x14, 0x01, 0x18, 0x18, "1"                      },
	{0x14, 0x01, 0x18, 0x10, "2"                      },
	{0x14, 0x01, 0x18, 0x08, "3"                      },
	{0x14, 0x01, 0x18, 0x00, "4"                      },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x14, 0x01, 0x20, 0x20, "Off"                    },
	{0x14, 0x01, 0x20, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Time"                   },
	{0x14, 0x01, 0x40, 0x00, "2 minutes"              },
	{0x14, 0x01, 0x40, 0x40, "3 minutes"              },
};

STDDIPINFO(Pitfallu);

static struct BurnDIPInfo RegulusDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0x7e, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x14, 0x01, 0x40, 0x40, "Easy"                   },
	{0x14, 0x01, 0x40, 0x00, "Hard"                   },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x14, 0x01, 0x80, 0x80, "Off"                    },
	{0x14, 0x01, 0x80, 0x00, "On"                     },
};

STDDIPINFO(Regulus);

static struct BurnDIPInfo RegulusoDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xfe, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x08, "4"                      },
	{0x14, 0x01, 0x0c, 0x04, "5"                      },
	{0x14, 0x01, 0x0c, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x14, 0x01, 0x40, 0x40, "Easy"                   },
	{0x14, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Reguluso);

static struct BurnDIPInfo SeganinjDIPList[]=
{
	// Default Values
	{0x15, 0xff, 0xff, 0xff, NULL                     },
	{0x16, 0xff, 0xff, 0xdc, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x15)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x16, 0x01, 0x01, 0x00, "Upright"                },
	{0x16, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x16, 0x01, 0x02, 0x02, "Off"                    },
	{0x16, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x16, 0x01, 0x0c, 0x08, "2"                      },
	{0x16, 0x01, 0x0c, 0x0c, "3"                      },
	{0x16, 0x01, 0x0c, 0x04, "4"                      },
	{0x16, 0x01, 0x0c, 0x00, "240"                    },
	
	{0   , 0xfe, 0   , 2   , "Bonus Life"             },
	{0x16, 0x01, 0x10, 0x10, "20k  70k 120k 170k"     },
	{0x16, 0x01, 0x10, 0x00, "50k 100k 150k 200k"     },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x16, 0x01, 0x20, 0x20, "Off"                    },
	{0x16, 0x01, 0x20, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x16, 0x01, 0x40, 0x40, "Easy"                   },
	{0x16, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Seganinj);

static struct BurnDIPInfo StarjackDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xf6, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x06, 0x06, "3"                      },
	{0x14, 0x01, 0x06, 0x04, "4"                      },
	{0x14, 0x01, 0x06, 0x02, "5"                      },
	{0x14, 0x01, 0x06, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 8   , "Bonus Life"             },
	{0x14, 0x01, 0x38, 0x38, "Every 20k"              },
	{0x14, 0x01, 0x38, 0x28, "Every 30k"              },
	{0x14, 0x01, 0x38, 0x18, "Every 40k"              },
	{0x14, 0x01, 0x38, 0x08, "Every 50k"              },
	{0x14, 0x01, 0x38, 0x30, "20k, then every 30k"    },
	{0x14, 0x01, 0x38, 0x20, "30k, then every 40k"    },
	{0x14, 0x01, 0x38, 0x10, "40k, then every 50k"    },
	{0x14, 0x01, 0x38, 0x00, "50k, then every 60k"    },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x14, 0x01, 0xc0, 0xc0, "Easy"                   },
	{0x14, 0x01, 0xc0, 0x80, "Medium"                 },
	{0x14, 0x01, 0xc0, 0x40, "Hard"                   },
	{0x14, 0x01, 0xc0, 0x00, "Hardest"                },
};

STDDIPINFO(Starjack);

static struct BurnDIPInfo StarjacsDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xfe, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x06, 0x06, "3"                      },
	{0x14, 0x01, 0x06, 0x04, "4"                      },
	{0x14, 0x01, 0x06, 0x02, "5"                      },
	{0x14, 0x01, 0x06, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 2   , "Ship"                   },
	{0x14, 0x01, 0x08, 0x08, "Single"                 },
	{0x14, 0x01, 0x08, 0x00, "Multi"                  },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x14, 0x01, 0x30, 0x30, "30k, then every 40k"    },
	{0x14, 0x01, 0x30, 0x20, "40k, then every 50k"    },
	{0x14, 0x01, 0x30, 0x10, "50k, then every 60k"    },
	{0x14, 0x01, 0x30, 0x00, "60k, then every 70k"    },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x14, 0x01, 0xc0, 0xc0, "Easy"                   },
	{0x14, 0x01, 0xc0, 0x80, "Medium"                 },
	{0x14, 0x01, 0xc0, 0x40, "Hard"                   },
	{0x14, 0x01, 0xc0, 0x00, "Hardest"                },
};

STDDIPINFO(Starjacs);

static struct BurnDIPInfo SwatDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xfe, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x06, 0x06, "3"                      },
	{0x14, 0x01, 0x06, 0x04, "4"                      },
	{0x14, 0x01, 0x06, 0x02, "5"                      },
	{0x14, 0x01, 0x06, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 8   , "Bonus Life"             },
	{0x14, 0x01, 0x38, 0x38, "30k"                    },
	{0x14, 0x01, 0x38, 0x30, "40k"                    },
	{0x14, 0x01, 0x38, 0x28, "50k"                    },
	{0x14, 0x01, 0x38, 0x20, "60k"                    },
	{0x14, 0x01, 0x38, 0x18, "70k"                    },
	{0x14, 0x01, 0x38, 0x10, "80k"                    },
	{0x14, 0x01, 0x38, 0x08, "90k"                    },
	{0x14, 0x01, 0x38, 0x00, "None"                   },
};

STDDIPINFO(Swat);

static struct BurnDIPInfo TeddybbDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0xfe, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x13)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x01, 0x00, "Upright"                },
	{0x14, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x14, 0x01, 0x02, 0x00, "Off"                    },
	{0x14, 0x01, 0x02, 0x02, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x0c, 0x08, "2"                      },
	{0x14, 0x01, 0x0c, 0x0c, "3"                      },
	{0x14, 0x01, 0x0c, 0x04, "4"                      },
	{0x14, 0x01, 0x0c, 0x00, "252"                    },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x14, 0x01, 0x30, 0x30, "100k 400k"              },
	{0x14, 0x01, 0x30, 0x20, "200k 600k"              },
	{0x14, 0x01, 0x30, 0x10, "400k 800k"              },
	{0x14, 0x01, 0x30, 0x00, "600k"                   },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x14, 0x01, 0x40, 0x40, "Easy"                   },
	{0x14, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Teddybb);

static struct BurnDIPInfo UpndownDIPList[]=
{
	// Default Values
	{0x11, 0xff, 0xff, 0xff, NULL                     },
	{0x12, 0xff, 0xff, 0xfe, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x11)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x12, 0x01, 0x01, 0x00, "Upright"                },
	{0x12, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x12, 0x01, 0x06, 0x06, "3"                      },
	{0x12, 0x01, 0x06, 0x04, "4"                      },
	{0x12, 0x01, 0x06, 0x02, "5"                      },
	{0x12, 0x01, 0x06, 0x00, "Infinite"               },
	
	{0   , 0xfe, 0   , 8   , "Bonus Life"             },
	{0x12, 0x01, 0x38, 0x38, "10k"                    },
	{0x12, 0x01, 0x38, 0x30, "20k"                    },
	{0x12, 0x01, 0x38, 0x28, "30k"                    },
	{0x12, 0x01, 0x38, 0x20, "40k"                    },
	{0x12, 0x01, 0x38, 0x18, "50k"                    },
	{0x12, 0x01, 0x38, 0x10, "60k"                    },
	{0x12, 0x01, 0x38, 0x08, "70k"                    },
	{0x12, 0x01, 0x38, 0x00, "None"                   },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x12, 0x01, 0xc0, 0xc0, "Easy"                   },
	{0x12, 0x01, 0xc0, 0x80, "Medium"                 },
	{0x12, 0x01, 0xc0, 0x40, "Hard"                   },
	{0x12, 0x01, 0xc0, 0x00, "Hardest"                },
};

STDDIPINFO(Upndown);

static struct BurnDIPInfo WboyDIPList[]=
{
	// Default Values
	{0x0f, 0xff, 0xff, 0xff, NULL                     },
	{0x10, 0xff, 0xff, 0xec, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x0f)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x10, 0x01, 0x01, 0x00, "Upright"                },
	{0x10, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x10, 0x01, 0x02, 0x02, "Off"                    },
	{0x10, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x10, 0x01, 0x0c, 0x0c, "3"                      },
	{0x10, 0x01, 0x0c, 0x08, "4"                      },
	{0x10, 0x01, 0x0c, 0x04, "5"                      },
	{0x10, 0x01, 0x0c, 0x00, "Freeplay"               },
	
	{0   , 0xfe, 0   , 2   , "Bonus Life"             },
	{0x10, 0x01, 0x10, 0x10, "30k 100k 170k 240k"     },
	{0x10, 0x01, 0x10, 0x00, "30k 120k 210k 300k"     },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x10, 0x01, 0x20, 0x00, "Off"                    },
	{0x10, 0x01, 0x20, 0x20, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x10, 0x01, 0x40, 0x40, "Easy"                   },
	{0x10, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Wboy);

static struct BurnDIPInfo Wboy3DIPList[]=
{
	// Default Values
	{0x0f, 0xff, 0xff, 0xff, NULL                     },
	{0x10, 0xff, 0xff, 0xec, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x0f)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x10, 0x01, 0x01, 0x00, "Upright"                },
	{0x10, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x10, 0x01, 0x02, 0x02, "Off"                    },
	{0x10, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x10, 0x01, 0x0c, 0x0c, "1"                      },
	{0x10, 0x01, 0x0c, 0x08, "2"                      },
	{0x10, 0x01, 0x0c, 0x04, "3"                      },
	{0x10, 0x01, 0x0c, 0x00, "Freeplay"               },
	
	{0   , 0xfe, 0   , 2   , "Bonus Life"             },
	{0x10, 0x01, 0x10, 0x10, "30k 100k 170k 240k"     },
	{0x10, 0x01, 0x10, 0x00, "30k 120k 210k 300k"     },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x10, 0x01, 0x20, 0x00, "Off"                    },
	{0x10, 0x01, 0x20, 0x20, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x10, 0x01, 0x40, 0x40, "Easy"                   },
	{0x10, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Wboy3);

static struct BurnDIPInfo WboyuDIPList[]=
{
	// Default Values
	{0x0f, 0xff, 0xff, 0xbe, NULL                     },
	{0x10, 0xff, 0xff, 0xff, NULL                     },

	// Dip 1
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x0f, 0x01, 0x01, 0x00, "Upright"                },
	{0x0f, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x0f, 0x01, 0x06, 0x00, "2"                      },
	{0x0f, 0x01, 0x06, 0x06, "3"                      },
	{0x0f, 0x01, 0x06, 0x04, "4"                      },
	{0x0f, 0x01, 0x06, 0x02, "5"                      },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x0f, 0x01, 0x40, 0x40, "Off"                    },
	{0x0f, 0x01, 0x40, 0x00, "On"                     },
	
	// Dip 2
	{0   , 0xfe, 0   , 8   , "Coinage"                },
	{0x10, 0x01, 0x07, 0x04, "4 Coins 1 Credit"       },
	{0x10, 0x01, 0x07, 0x05, "3 Coins 1 Credit"       },
	{0x10, 0x01, 0x07, 0x00, "4 Coins 2 Credits"      },
	{0x10, 0x01, 0x07, 0x06, "2 Coins 1 Credit"       },
	{0x10, 0x01, 0x07, 0x01, "3 Coins 2 Credits"      },
	{0x10, 0x01, 0x07, 0x02, "2 Coins 1 Credits"      },
	{0x10, 0x01, 0x07, 0x07, "1 Coin  1 Credit"       },
	{0x10, 0x01, 0x07, 0x03, "1 Coin  2 Credits"      },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x10, 0x01, 0x10, 0x00, "Off"                    },
	{0x10, 0x01, 0x10, 0x10, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Mode"                   },
	{0x10, 0x01, 0xc0, 0xc0, "Normal Game"            },
	{0x10, 0x01, 0xc0, 0x80, "Free Play"              },
	{0x10, 0x01, 0xc0, 0x40, "Test Mode"              },
	{0x10, 0x01, 0xc0, 0x00, "Endless Game"           },
};

STDDIPINFO(Wboyu);

static struct BurnDIPInfo WbdeluxeDIPList[]=
{
	// Default Values
	{0x0f, 0xff, 0xff, 0xff, NULL                     },
	{0x10, 0xff, 0xff, 0x7c, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x0f)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x10, 0x01, 0x01, 0x00, "Upright"                },
	{0x10, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x10, 0x01, 0x02, 0x02, "Off"                    },
	{0x10, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x10, 0x01, 0x0c, 0x0c, "3"                      },
	{0x10, 0x01, 0x0c, 0x08, "4"                      },
	{0x10, 0x01, 0x0c, 0x04, "5"                      },
	{0x10, 0x01, 0x0c, 0x00, "Freeplay"               },
	
	{0   , 0xfe, 0   , 2   , "Bonus Life"             },
	{0x10, 0x01, 0x10, 0x10, "30k 100k 170k 240k"     },
	{0x10, 0x01, 0x10, 0x00, "30k 120k 210k 300k"     },
	
	{0   , 0xfe, 0   , 2   , "Allow Continue"         },
	{0x10, 0x01, 0x20, 0x00, "Off"                    },
	{0x10, 0x01, 0x20, 0x20, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x10, 0x01, 0x40, 0x40, "Easy"                   },
	{0x10, 0x01, 0x40, 0x00, "Hard"                   },
	
	{0   , 0xfe, 0   , 2   , "Energy Consumption"     },
	{0x10, 0x01, 0x80, 0x00, "Slow"                   },
	{0x10, 0x01, 0x80, 0x80, "Fast"                   },
};

STDDIPINFO(Wbdeluxe);

static struct BurnDIPInfo WmatchDIPList[]=
{
	// Default Values
	{0x19, 0xff, 0xff, 0xff, NULL                     },
	{0x1a, 0xff, 0xff, 0xfc, NULL                     },

	// Dip 1
	SYSTEM1_COINAGE(0x19)
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x1a, 0x01, 0x01, 0x00, "Upright"                },
	{0x1a, 0x01, 0x01, 0x01, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x1a, 0x01, 0x02, 0x02, "Off"                    },
	{0x1a, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Time"                   },
	{0x1a, 0x01, 0x0c, 0x0c, "Normal"                 },
	{0x1a, 0x01, 0x0c, 0x08, "Fast"                   },
	{0x1a, 0x01, 0x0c, 0x04, "Faster"                 },
	{0x1a, 0x01, 0x0c, 0x00, "Fastest"                },
	
	{0   , 0xfe, 0   , 2   , "Bonus Life"             },
	{0x1a, 0x01, 0x10, 0x10, "20k 50k"                },
	{0x1a, 0x01, 0x10, 0x00, "30k 70k"                },
	
	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x1a, 0x01, 0x40, 0x40, "Easy"                   },
	{0x1a, 0x01, 0x40, 0x00, "Hard"                   },
};

STDDIPINFO(Wmatch);

#undef SYSTEM1_COINAGE

/*==============================================================================================
ROM Descriptions
===============================================================================================*/

static struct BurnRomInfo BlockgalRomDesc[] = {
	{ "bg.116",            0x004000, 0xa99b231a, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "bg.109",            0x004000, 0xa6b573d5, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	
	{ "bg.120",            0x002000, 0xd848faff, BRF_ESS | BRF_PRG }, //  2	Z80 #2 Program Code
	
	{ "bg.62",             0x002000, 0x7e3ea4eb, BRF_GRA },		  //  3 Tiles
	{ "bg.61",             0x002000, 0x4dd3d39d, BRF_GRA },		  //  4 Tiles
	{ "bg.64",             0x002000, 0x17368663, BRF_GRA },		  //  5 Tiles
	{ "bg.63",             0x002000, 0x0c8bc404, BRF_GRA },		  //  6 Tiles
	{ "bg.66",             0x002000, 0x2b7dc4fa, BRF_GRA },		  //  7 Tiles
	{ "bg.65",             0x002000, 0xed121306, BRF_GRA },		  //  8 Tiles
	
	{ "bg.117",            0x004000, 0xe99cc920, BRF_GRA },		  //  9 Sprites
	{ "bg.04",             0x004000, 0x213057f8, BRF_GRA },		  //  10 Sprites
	{ "bg.110",            0x004000, 0x064c812c, BRF_GRA },		  //  11 Sprites
	{ "bg.05",             0x004000, 0x02e0b040, BRF_GRA },		  //  12 Sprites

	{ "pr5317.76",         0x000100, 0x648350b8, BRF_OPT },		  //  13 Timing PROM
	
	{ "317-0029.key",      0x002000, 0x350d7f93, BRF_ESS | BRF_PRG }, //  14 MC8123 Key
};

STD_ROM_PICK(Blockgal);
STD_ROM_FN(Blockgal);

static struct BurnRomInfo BrainRomDesc[] = {
	{ "brain.1",           0x008000, 0x2d2aec31, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "brain.2",           0x008000, 0x810a8ab5, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "brain.3",           0x008000, 0x9a225634, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "brain.120",         0x008000, 0xc7e50278, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "brain.62",          0x004000, 0x7dce2302, BRF_GRA },		  //  4 Tiles
	{ "brain.64",          0x004000, 0x7ce03fd3, BRF_GRA },		  //  5 Tiles
	{ "brain.66",          0x004000, 0xea54323f, BRF_GRA },		  //  6 Tiles
	
	{ "brain.117",         0x008000, 0x92ff71a4, BRF_GRA },		  //  7 Sprites
	{ "brain.110",         0x008000, 0xa1b847ec, BRF_GRA },		  //  8 Sprites
	{ "brain.4",           0x008000, 0xfd2ea53b, BRF_GRA },		  //  9 Sprites

	{ "bprom.3",           0x000100, 0x8eee0f72, BRF_OPT },		  //  10 Red PROM
	{ "bprom.2",           0x000100, 0x3e7babd7, BRF_OPT },		  //  11 Green PROM
	{ "bprom.1",           0x000100, 0x371c44a6, BRF_OPT },		  //  12 Blue PROM
	{ "pr5317.76",         0x000100, 0x648350b8, BRF_OPT },		  //  13 Timing PROM
};

STD_ROM_PICK(Brain);
STD_ROM_FN(Brain);

static struct BurnRomInfo FlickyRomDesc[] = {
	{ "epr5978a.116",      0x004000, 0x296f1492, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr5979a.109",      0x004000, 0x64b03ef9, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	
	{ "epr-5869.120",      0x002000, 0x6d220d4e, BRF_ESS | BRF_PRG }, //  2	Z80 #2 Program Code
	
	{ "epr-5868.62",       0x002000, 0x7402256b, BRF_GRA },		  //  3 Tiles
	{ "epr-5867.61",       0x002000, 0x2f5ce930, BRF_GRA },		  //  4 Tiles
	{ "epr-5866.64",       0x002000, 0x967f1d9a, BRF_GRA },		  //  5 Tiles
	{ "epr-5865.63",       0x002000, 0x03d9a34c, BRF_GRA },		  //  6 Tiles
	{ "epr-5864.66",       0x002000, 0xe659f358, BRF_GRA },		  //  7 Tiles
	{ "epr-5863.65",       0x002000, 0xa496ca15, BRF_GRA },		  //  8 Tiles
	
	{ "epr-5855.117",      0x004000, 0xb5f894a1, BRF_GRA },		  //  9 Sprites
	{ "epr-5856.110",      0x004000, 0x266af78f, BRF_GRA },		  //  10 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  11 Timing PROM
};

STD_ROM_PICK(Flicky);
STD_ROM_FN(Flicky);

static struct BurnRomInfo Flicks1RomDesc[] = {
	{ "ic129",             0x002000, 0x7011275c, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "ic130",             0x002000, 0xe7ed012d, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "ic131",             0x002000, 0xc5e98cd1, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "ic132",             0x002000, 0x0e5122c2, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	
	{ "epr-5869.120",      0x002000, 0x6d220d4e, BRF_ESS | BRF_PRG }, //  4	Z80 #2 Program Code
	
	{ "epr-5868.62",       0x002000, 0x7402256b, BRF_GRA },		  //  5 Tiles
	{ "epr-5867.61",       0x002000, 0x2f5ce930, BRF_GRA },		  //  6 Tiles
	{ "epr-5866.64",       0x002000, 0x967f1d9a, BRF_GRA },		  //  7 Tiles
	{ "epr-5865.63",       0x002000, 0x03d9a34c, BRF_GRA },		  //  8 Tiles
	{ "epr-5864.66",       0x002000, 0xe659f358, BRF_GRA },		  //  9 Tiles
	{ "epr-5863.65",       0x002000, 0xa496ca15, BRF_GRA },		  //  10 Tiles
	
	{ "epr-5855.117",      0x004000, 0xb5f894a1, BRF_GRA },		  //  11 Sprites
	{ "epr-5856.110",      0x004000, 0x266af78f, BRF_GRA },		  //  12 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  13 Timing PROM
};

STD_ROM_PICK(Flicks1);
STD_ROM_FN(Flicks1);

static struct BurnRomInfo Flicks2RomDesc[] = {
	{ "epr-6621.bin",      0x004000, 0xb21ff546, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-6622.bin",      0x004000, 0x133a8bf1, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	
	{ "epr-5869.120",      0x002000, 0x6d220d4e, BRF_ESS | BRF_PRG }, //  2	Z80 #2 Program Code
	
	{ "epr-5868.62",       0x002000, 0x7402256b, BRF_GRA },		  //  3 Tiles
	{ "epr-5867.61",       0x002000, 0x2f5ce930, BRF_GRA },		  //  4 Tiles
	{ "epr-5866.64",       0x002000, 0x967f1d9a, BRF_GRA },		  //  5 Tiles
	{ "epr-5865.63",       0x002000, 0x03d9a34c, BRF_GRA },		  //  6 Tiles
	{ "epr-5864.66",       0x002000, 0xe659f358, BRF_GRA },		  //  7 Tiles
	{ "epr-5863.65",       0x002000, 0xa496ca15, BRF_GRA },		  //  8 Tiles
	
	{ "epr-5855.117",      0x004000, 0xb5f894a1, BRF_GRA },		  //  9 Sprites
	{ "epr-5856.110",      0x004000, 0x266af78f, BRF_GRA },		  //  10 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  11 Timing PROM
};

STD_ROM_PICK(Flicks2);
STD_ROM_FN(Flicks2);

static struct BurnRomInfo FlickyoRomDesc[] = {
	{ "epr-5857.bin",      0x002000, 0xa65ac88e, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr5858a.bin",      0x002000, 0x18b412f4, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5859.bin",      0x002000, 0xa5558d7e, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5860.bin",      0x002000, 0x1b35fef1, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	
	{ "epr-5869.120",      0x002000, 0x6d220d4e, BRF_ESS | BRF_PRG }, //  4	Z80 #2 Program Code
	
	{ "epr-5868.62",       0x002000, 0x7402256b, BRF_GRA },		  //  5 Tiles
	{ "epr-5867.61",       0x002000, 0x2f5ce930, BRF_GRA },		  //  6 Tiles
	{ "epr-5866.64",       0x002000, 0x967f1d9a, BRF_GRA },		  //  7 Tiles
	{ "epr-5865.63",       0x002000, 0x03d9a34c, BRF_GRA },		  //  8 Tiles
	{ "epr-5864.66",       0x002000, 0xe659f358, BRF_GRA },		  //  9 Tiles
	{ "epr-5863.65",       0x002000, 0xa496ca15, BRF_GRA },		  //  10 Tiles
	
	{ "epr-5855.117",      0x004000, 0xb5f894a1, BRF_GRA },		  //  11 Sprites
	{ "epr-5856.110",      0x004000, 0x266af78f, BRF_GRA },		  //  12 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  13 Timing PROM
};

STD_ROM_PICK(Flickyo);
STD_ROM_FN(Flickyo);

static struct BurnRomInfo GardiaRomDesc[] = {
	{ "epr10255.1",        0x008000, 0x89282a6b, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr10254.2",        0x008000, 0x2826b6d8, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr10253.3",        0x008000, 0x7911260f, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr10243.120",      0x004000, 0x87220660, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr10249.61",       0x004000, 0x4e0ad0f2, BRF_GRA },		  //  4 Tiles
	{ "epr10248.64",       0x004000, 0x3515d124, BRF_GRA },		  //  5 Tiles
	{ "epr10247.66",       0x004000, 0x541e1555, BRF_GRA },		  //  6 Tiles
	
	{ "epr10234.117",      0x008000, 0x8a6aed33, BRF_GRA },		  //  7 Sprites
	{ "epr10233.110",      0x008000, 0xc52784d3, BRF_GRA },		  //  8 Sprites
	{ "epr10236.04",       0x008000, 0xb35ab227, BRF_GRA },		  //  9 Sprites
	{ "epr10235.5",        0x008000, 0x006a3151, BRF_GRA },		  //  10 Sprites

	{ "bprom.3",           0x000100, 0x8eee0f72, BRF_OPT },		  //  11 Red PROM
	{ "bprom.2",           0x000100, 0x3e7babd7, BRF_OPT },		  //  12 Green PROM
	{ "bprom.1",           0x000100, 0x371c44a6, BRF_OPT },		  //  13 Blue PROM
	{ "pr5317.4",          0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Gardia);
STD_ROM_FN(Gardia);

static struct BurnRomInfo MrvikingRomDesc[] = {
	{ "epr-5873.129",      0x002000, 0x14d21624, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-5874.130",      0x002000, 0x6df7de87, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5875.131",      0x002000, 0xac226100, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5876.132",      0x002000, 0xe77db1dc, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5755.133",      0x002000, 0xedd62ae1, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5756.134",      0x002000, 0x11974040, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5763.3",        0x002000, 0xd712280d, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5762.82",       0x002000, 0x4a91d08a, BRF_GRA },		  //  7 Tiles
	{ "epr-5761.65",       0x002000, 0xf7d61b65, BRF_GRA },		  //  8 Tiles
	{ "epr-5760.81",       0x002000, 0x95045820, BRF_GRA },		  //  9 Tiles
	{ "epr-5759.64",       0x002000, 0x5f9bae4e, BRF_GRA },		  //  10 Tiles
	{ "epr-5758.80",       0x002000, 0x808ee706, BRF_GRA },		  //  11 Tiles
	{ "epr-5757.63",       0x002000, 0x480f7074, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5749.86",       0x004000, 0xe24682cd, BRF_GRA },		  //  13 Sprites
	{ "epr-5750.93",       0x004000, 0x6564d1ad, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Mrviking);
STD_ROM_FN(Mrviking);

static struct BurnRomInfo MrvikngjRomDesc[] = {
	{ "epr-5751.129",      0x002000, 0xae97a4c5, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-5752.130",      0x002000, 0xd48e6726, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5753.131",      0x002000, 0x28c60887, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5754.132",      0x002000, 0x1f47ed02, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5755.133",      0x002000, 0xedd62ae1, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5756.134",      0x002000, 0x11974040, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5763.3",        0x002000, 0xd712280d, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5762.82",       0x002000, 0x4a91d08a, BRF_GRA },		  //  7 Tiles
	{ "epr-5761.65",       0x002000, 0xf7d61b65, BRF_GRA },		  //  8 Tiles
	{ "epr-5760.81",       0x002000, 0x95045820, BRF_GRA },		  //  9 Tiles
	{ "epr-5759.64",       0x002000, 0x5f9bae4e, BRF_GRA },		  //  10 Tiles
	{ "epr-5758.80",       0x002000, 0x808ee706, BRF_GRA },		  //  11 Tiles
	{ "epr-5757.63",       0x002000, 0x480f7074, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5749.86",       0x004000, 0xe24682cd, BRF_GRA },		  //  13 Sprites
	{ "epr-5750.93",       0x004000, 0x6564d1ad, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Mrvikngj);
STD_ROM_FN(Mrvikngj);

static struct BurnRomInfo MyheroRomDesc[] = {
	{ "epr6963b.116",      0x004000, 0x4daf89d4, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr6964a.109",      0x004000, 0xc26188e5, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-6927.96",       0x004000, 0x3cbbaf64, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-69xx.120",      0x002000, 0x0039e1e9, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-6966.62",       0x002000, 0x157f0401, BRF_GRA },		  //  4 Tiles
	{ "epr-6961.61",       0x002000, 0xbe53ce47, BRF_GRA },		  //  5 Tiles
	{ "epr-6960.64",       0x002000, 0xbd381baa, BRF_GRA },		  //  6 Tiles
	{ "epr-6959.63",       0x002000, 0xbc04e79a, BRF_GRA },		  //  7 Tiles
	{ "epr-6958.66",       0x002000, 0x714f2c26, BRF_GRA },		  //  8 Tiles
	{ "epr-6957.65",       0x002000, 0x80920112, BRF_GRA },		  //  9 Tiles
	
	{ "epr-6921.117",      0x004000, 0xf19e05a1, BRF_GRA },		  //  10 Sprites
	{ "epr-6923.04",       0x004000, 0x7988adc3, BRF_GRA },		  //  11 Sprites
	{ "epr-6922.110",      0x004000, 0x37f77a78, BRF_GRA },		  //  12 Sprites
	{ "epr-6924.05",       0x004000, 0x42bdc8f6, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Myhero);
STD_ROM_FN(Myhero);

static struct BurnRomInfo NoboranbRomDesc[] = {
	{ "nobo-t.bin",        0x008000, 0x176fd168, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "nobo-r.bin",        0x008000, 0xd61cf3c9, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "nobo-s.bin",        0x008000, 0xb0e7697f, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "nobo-m.bin",        0x004000, 0x415adf76, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "nobo-k.bin",        0x008000, 0x446fbcdd, BRF_GRA },		  //  4 Tiles
	{ "nobo-j.bin",        0x008000, 0xf12df039, BRF_GRA },		  //  5 Tiles	
	{ "nobo-l.bin",        0x008000, 0x35f396df, BRF_GRA },		  //  6 Tiles
	
	{ "nobo-q.bin",        0x008000, 0x2442b86d, BRF_GRA },		  //  7 Sprites
	{ "nobo-o.bin",        0x008000, 0xe33743a6, BRF_GRA },		  //  8 Sprites
	{ "nobo-p.bin",        0x008000, 0x7fbba01d, BRF_GRA },		  //  9 Sprites
	{ "nobo-n.bin",        0x008000, 0x85e7a29f, BRF_GRA },		  //  10 Sprites

	{ "nobo_pr.16d",       0x000100, 0x95010ac2, BRF_OPT },		  //  11 Red PROM
	{ "nobo_pr.15d",       0x000100, 0xc55aac0c, BRF_OPT },		  //  12 Green PROM
	{ "nobo_pr.14d",       0x000100, 0xde394cee, BRF_OPT },		  //  13 Blue PROM
	{ "nobo_pr.13a",       0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Noboranb);
STD_ROM_FN(Noboranb);

static struct BurnRomInfo Pitfall2RomDesc[] = {
	{ "epr6456a.116",      0x004000, 0xbcc8406b, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr6457a.109",      0x004000, 0xa016fd2a, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr6458a.96",       0x004000, 0x5c30b3e8, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-6462.120",      0x002000, 0x86bb9185, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr6474a.62",       0x002000, 0x9f1711b9, BRF_GRA },		  //  4 Tiles
	{ "epr6473a.61",       0x002000, 0x8e53b8dd, BRF_GRA },		  //  5 Tiles
	{ "epr6472a.64",       0x002000, 0xe0f34a11, BRF_GRA },		  //  6 Tiles
	{ "epr6471a.63",       0x002000, 0xd5bc805c, BRF_GRA },		  //  7 Tiles
	{ "epr6470a.66",       0x002000, 0x1439729f, BRF_GRA },		  //  8 Tiles
	{ "epr6469a.65",       0x002000, 0xe4ac6921, BRF_GRA },		  //  9 Tiles
	
	{ "epr6454a.117",      0x004000, 0xa5d96780, BRF_GRA },		  //  10 Sprites
	{ "epr-6455.05",       0x004000, 0x32ee64a1, BRF_GRA },		  //  11 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  12 Timing PROM
};

STD_ROM_PICK(Pitfall2);
STD_ROM_FN(Pitfall2);

static struct BurnRomInfo PitfalluRomDesc[] = {
	{ "epr-6623.116",      0x004000, 0xbcb47ed6, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr6624a.109",      0x004000, 0x6e8b09c1, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-6625.96",       0x004000, 0xdc5484ba, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-6462.120",      0x002000, 0x86bb9185, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr6474a.62",       0x002000, 0x9f1711b9, BRF_GRA },		  //  4 Tiles
	{ "epr6473a.61",       0x002000, 0x8e53b8dd, BRF_GRA },		  //  5 Tiles
	{ "epr6472a.64",       0x002000, 0xe0f34a11, BRF_GRA },		  //  6 Tiles
	{ "epr6471a.63",       0x002000, 0xd5bc805c, BRF_GRA },		  //  7 Tiles
	{ "epr6470a.66",       0x002000, 0x1439729f, BRF_GRA },		  //  8 Tiles
	{ "epr6469a.65",       0x002000, 0xe4ac6921, BRF_GRA },		  //  9 Tiles
	
	{ "epr6454a.117",      0x004000, 0xa5d96780, BRF_GRA },		  //  10 Sprites
	{ "epr-6455.05",       0x004000, 0x32ee64a1, BRF_GRA },		  //  11 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  12 Timing PROM
};

STD_ROM_PICK(Pitfallu);
STD_ROM_FN(Pitfallu);

static struct BurnRomInfo RegulusRomDesc[] = {
	{ "epr-5640a.129",     0x002000, 0xdafb1528, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-5641a.130",     0x002000, 0x0fcc850e, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5642a.131",     0x002000, 0x4feffa17, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5643a.132",     0x002000, 0xb8ac7eb4, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5644.133",      0x002000, 0xffd05b7d, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5645a.134",     0x002000, 0x6b4bf77c, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5652.3",        0x002000, 0x74edcb98, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5651.82",       0x002000, 0xf07f3e82, BRF_GRA },		  //  7 Tiles
	{ "epr-5650.65",       0x002000, 0x84c1baa2, BRF_GRA },		  //  8 Tiles
	{ "epr-5649.81",       0x002000, 0x6774c895, BRF_GRA },		  //  9 Tiles
	{ "epr-5648.64",       0x002000, 0x0c69e92a, BRF_GRA },		  //  10 Tiles
	{ "epr-5647.80",       0x002000, 0x9330f7b5, BRF_GRA },		  //  11 Tiles
	{ "epr-5646.63",       0x002000, 0x4dfacbbc, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5638.86",       0x004000, 0x617363dd, BRF_GRA },		  //  13 Sprites
	{ "epr-5639.93",       0x004000, 0xa4ec5131, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Regulus);
STD_ROM_FN(Regulus);

static struct BurnRomInfo RegulusoRomDesc[] = {
	{ "epr-5640.129",      0x002000, 0x8324d0d4, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-5641.130",      0x002000, 0x0a09f5c7, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5642.131",      0x002000, 0xff27b2f6, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5643.132",      0x002000, 0x0d867df0, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5644.133",      0x002000, 0xffd05b7d, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5645.134",      0x002000, 0x57a2b4b4, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5652.3",        0x002000, 0x74edcb98, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5651.82",       0x002000, 0xf07f3e82, BRF_GRA },		  //  7 Tiles
	{ "epr-5650.65",       0x002000, 0x84c1baa2, BRF_GRA },		  //  8 Tiles
	{ "epr-5649.81",       0x002000, 0x6774c895, BRF_GRA },		  //  9 Tiles
	{ "epr-5648.64",       0x002000, 0x0c69e92a, BRF_GRA },		  //  10 Tiles
	{ "epr-5647.80",       0x002000, 0x9330f7b5, BRF_GRA },		  //  11 Tiles
	{ "epr-5646.63",       0x002000, 0x4dfacbbc, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5638.86",       0x004000, 0x617363dd, BRF_GRA },		  //  13 Sprites
	{ "epr-5639.93",       0x004000, 0xa4ec5131, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Reguluso);
STD_ROM_FN(Reguluso);

static struct BurnRomInfo RegulusuRomDesc[] = {
	{ "epr-5950.129",      0x002000, 0x3b047b67, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-5951.130",      0x002000, 0xd66453ab, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5952.131",      0x002000, 0xf3d0158a, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5953.132",      0x002000, 0xa9ad4f44, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5644.133",      0x002000, 0xffd05b7d, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5955.134",      0x002000, 0x65ddb2a3, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5652.3",        0x002000, 0x74edcb98, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5651.82",       0x002000, 0xf07f3e82, BRF_GRA },		  //  7 Tiles
	{ "epr-5650.65",       0x002000, 0x84c1baa2, BRF_GRA },		  //  8 Tiles
	{ "epr-5649.81",       0x002000, 0x6774c895, BRF_GRA },		  //  9 Tiles
	{ "epr-5648.64",       0x002000, 0x0c69e92a, BRF_GRA },		  //  10 Tiles
	{ "epr-5647.80",       0x002000, 0x9330f7b5, BRF_GRA },		  //  11 Tiles
	{ "epr-5646.63",       0x002000, 0x4dfacbbc, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5638.86",       0x004000, 0x617363dd, BRF_GRA },		  //  13 Sprites
	{ "epr-5639.93",       0x004000, 0xa4ec5131, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Regulusu);
STD_ROM_FN(Regulusu);

static struct BurnRomInfo SeganinjRomDesc[] = {
	{ "epr-.116",          0x004000, 0xa5d0c9d0, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-.109",          0x004000, 0xb9e6775c, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-6552.96",       0x004000, 0xf2eeb0d8, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-6559.120",      0x002000, 0x5a1570ee, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-6558.62",       0x002000, 0x2af9eaeb, BRF_GRA },		  //  4 Tiles
	{ "epr-6592.61",       0x002000, 0x7804db86, BRF_GRA },		  //  5 Tiles
	{ "epr-6556.64",       0x002000, 0x79fd26f7, BRF_GRA },		  //  6 Tiles
	{ "epr-6590.63",       0x002000, 0xbf858cad, BRF_GRA },		  //  7 Tiles
	{ "epr-6554.66",       0x002000, 0x5ac9d205, BRF_GRA },		  //  8 Tiles
	{ "epr-6588.65",       0x002000, 0xdc931dbb, BRF_GRA },		  //  9 Tiles
	
	{ "epr-6546.117",      0x004000, 0xa4785692, BRF_GRA },		  //  10 Sprites
	{ "epr-6548.04",       0x004000, 0xbdf278c1, BRF_GRA },		  //  11 Sprites
	{ "epr-6547.110",      0x004000, 0x34451b08, BRF_GRA },		  //  12 Sprites
	{ "epr-6549.05",       0x004000, 0xd2057668, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Seganinj);
STD_ROM_FN(Seganinj);

static struct BurnRomInfo SeganinuRomDesc[] = {
	{ "epr-7149.116",      0x004000, 0xcd9fade7, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-7150.109",      0x004000, 0xc36351e2, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-6552.96",       0x004000, 0xf2eeb0d8, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-6559.120",      0x002000, 0x5a1570ee, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-6558.62",       0x002000, 0x2af9eaeb, BRF_GRA },		  //  4 Tiles
	{ "epr-6592.61",       0x002000, 0x7804db86, BRF_GRA },		  //  5 Tiles
	{ "epr-6556.64",       0x002000, 0x79fd26f7, BRF_GRA },		  //  6 Tiles
	{ "epr-6590.63",       0x002000, 0xbf858cad, BRF_GRA },		  //  7 Tiles
	{ "epr-6554.66",       0x002000, 0x5ac9d205, BRF_GRA },		  //  8 Tiles
	{ "epr-6588.65",       0x002000, 0xdc931dbb, BRF_GRA },		  //  9 Tiles
	
	{ "epr-6546.117",      0x004000, 0xa4785692, BRF_GRA },		  //  10 Sprites
	{ "epr-6548.04",       0x004000, 0xbdf278c1, BRF_GRA },		  //  11 Sprites
	{ "epr-6547.110",      0x004000, 0x34451b08, BRF_GRA },		  //  12 Sprites
	{ "epr-6549.05",       0x004000, 0xd2057668, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Seganinu);
STD_ROM_FN(Seganinu);

static struct BurnRomInfo NprincsuRomDesc[] = {
	{ "epr-6573.129",      0x002000, 0xd2919c7d, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-6574.130",      0x002000, 0x5a132833, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-6575.131",      0x002000, 0xa94b0bd4, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-6576.132",      0x002000, 0x27d3bbdb, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-6577.133",      0x002000, 0x73616e03, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-6578.134",      0x002000, 0xab68499f, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-6559.120",      0x002000, 0x5a1570ee, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-6558.62",       0x002000, 0x2af9eaeb, BRF_GRA },		  //  7 Tiles
	{ "epr-6557.61",       0x002000, 0x6eb131d0, BRF_GRA },		  //  8 Tiles
	{ "epr-6556.64",       0x002000, 0x79fd26f7, BRF_GRA },		  //  9 Tiles
	{ "epr-6555.63",       0x002000, 0x7f669aac, BRF_GRA },		  //  10 Tiles
	{ "epr-6554.66",       0x002000, 0x5ac9d205, BRF_GRA },		  //  11 Tiles
	{ "epr-6553.65",       0x002000, 0xeb82a8fe, BRF_GRA },		  //  12 Tiles
	
	{ "epr-6546.117",      0x004000, 0xa4785692, BRF_GRA },		  //  13 Sprites
	{ "epr-6548.04",       0x004000, 0xbdf278c1, BRF_GRA },		  //  14 Sprites
	{ "epr-6547.110",      0x004000, 0x34451b08, BRF_GRA },		  //  15 Sprites
	{ "epr-6549.05",       0x004000, 0xd2057668, BRF_GRA },		  //  16 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  17 Timing PROM
};

STD_ROM_PICK(Nprincsu);
STD_ROM_FN(Nprincsu);

static struct BurnRomInfo StarjackRomDesc[] = {
	{ "epr5320b.129",      0x002000, 0x7ab72ecd, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr5321a.130",      0x002000, 0x38b99050, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr5322a.131",      0x002000, 0x103a595b, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5323.132",      0x002000, 0x46af0d58, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5324.133",      0x002000, 0x1e89efe2, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5325.134",      0x002000, 0xd6e379a1, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5332.3",        0x002000, 0x7a72ab3d, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5331.82",       0x002000, 0x251d898f, BRF_GRA },		  //  7 Tiles
	{ "epr-5330.65",       0x002000, 0xeb048745, BRF_GRA },		  //  8 Tiles
	{ "epr-5329.81",       0x002000, 0x3e8bcaed, BRF_GRA },		  //  9 Tiles
	{ "epr-5328.64",       0x002000, 0x9ed7849f, BRF_GRA },		  //  10 Tiles
	{ "epr-5327.80",       0x002000, 0x79e92cb1, BRF_GRA },		  //  11 Tiles
	{ "epr-5326.63",       0x002000, 0xba7e2b47, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5318.86",       0x004000, 0x6f2e1fd3, BRF_GRA },		  //  13 Sprites
	{ "epr-5319.93",       0x004000, 0xebee4999, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Starjack);
STD_ROM_FN(Starjack);

static struct BurnRomInfo StarjacsRomDesc[] = {
	{ "a1_ic29.129",       0x002000, 0x59a22a1f, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "a1_ic30.130",       0x002000, 0x7f4597dc, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "a1_ic31.131",       0x002000, 0x6074c046, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "a1_ic32.132",       0x002000, 0x1c48a3fa, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "a1_ic33.133",       0x002000, 0x7598bd51, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "a1_ic34.134",       0x002000, 0xf66fa604, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5332.3",        0x002000, 0x7a72ab3d, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5331.82",       0x002000, 0x251d898f, BRF_GRA },		  //  7 Tiles
	{ "a1_ic65.65",        0x002000, 0x0ab1893c, BRF_GRA },		  //  8 Tiles
	{ "epr-5329.81",       0x002000, 0x3e8bcaed, BRF_GRA },		  //  9 Tiles
	{ "a1_ic64.64",        0x002000, 0x7f628ae6, BRF_GRA },		  //  10 Tiles
	{ "epr-5327.80",       0x002000, 0x79e92cb1, BRF_GRA },		  //  11 Tiles
	{ "a1_ic63.63",        0x002000, 0x5bcb253e, BRF_GRA },		  //  12 Tiles
	
	{ "a1_ic86.86",        0x004000, 0x6f2e1fd3, BRF_GRA },		  //  13 Sprites
	{ "a1_ic93.93",        0x004000, 0x07987244, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Starjacs);
STD_ROM_FN(Starjacs);

static struct BurnRomInfo SwatRomDesc[] = {
	{ "epr5807b.129",      0x002000, 0x93db9c9f, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-5808.130",      0x002000, 0x67116665, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5809.131",      0x002000, 0xfd792fc9, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5810.132",      0x002000, 0xdc2b279d, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5811.133",      0x002000, 0x093e3ab1, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5812.134",      0x002000, 0x5bfd692f, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5819.3",        0x002000, 0xf6afd0fd, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5818.82",       0x002000, 0xb22033d9, BRF_GRA },		  //  7 Tiles
	{ "epr-5817.65",       0x002000, 0xfd942797, BRF_GRA },		  //  8 Tiles
	{ "epr-5816.81",       0x002000, 0x4384376d, BRF_GRA },		  //  9 Tiles
	{ "epr-5815.64",       0x002000, 0x16ad046c, BRF_GRA },		  //  10 Tiles
	{ "epr-5814.80",       0x002000, 0xbe721c99, BRF_GRA },		  //  11 Tiles
	{ "epr-5813.63",       0x002000, 0x0d42c27e, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5805.86",       0x004000, 0x5a732865, BRF_GRA },		  //  13 Sprites
	{ "epr-5806.93",       0x004000, 0x26ac258c, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Swat);
STD_ROM_FN(Swat);

static struct BurnRomInfo TeddybbRomDesc[] = {
	{ "epr-6768.116",      0x004000, 0x5939817e, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-6769.109",      0x004000, 0x14a98ddd, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-6770.96",       0x004000, 0x67b0c7c2, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr6748x.120",      0x002000, 0xc2a1b89d, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-6747.62",       0x002000, 0xa0e5aca7, BRF_GRA },		  //  4 Tiles
	{ "epr-6746.61",       0x002000, 0xcdb77e51, BRF_GRA },		  //  5 Tiles
	{ "epr-6745.64",       0x002000, 0x0cab75c3, BRF_GRA },		  //  6 Tiles
	{ "epr-6744.63",       0x002000, 0x0ef8d2cd, BRF_GRA },		  //  7 Tiles
	{ "epr-6743.66",       0x002000, 0xc33062b5, BRF_GRA },		  //  8 Tiles
	{ "epr-6742.65",       0x002000, 0xc457e8c5, BRF_GRA },		  //  9 Tiles
	
	{ "epr-6735.117",      0x004000, 0x1be35a97, BRF_GRA },		  //  10 Sprites
	{ "epr-6737.04",       0x004000, 0x6b53aa7a, BRF_GRA },		  //  11 Sprites
	{ "epr-6736.110",      0x004000, 0x565c25d0, BRF_GRA },		  //  12 Sprites
	{ "epr-6738.05",       0x004000, 0xe116285f, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Teddybb);
STD_ROM_FN(Teddybb);

static struct BurnRomInfo TeddybboRomDesc[] = {
	{ "epr-6739.116",      0x004000, 0x81a37e69, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-6740.109",      0x004000, 0x715388a9, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-6741.96",       0x004000, 0xe5a74f5f, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-6748.120",      0x002000, 0x9325a1cf, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-6747.62",       0x002000, 0xa0e5aca7, BRF_GRA },		  //  4 Tiles
	{ "epr-6746.61",       0x002000, 0xcdb77e51, BRF_GRA },		  //  5 Tiles
	{ "epr-6745.64",       0x002000, 0x0cab75c3, BRF_GRA },		  //  6 Tiles
	{ "epr-6744.63",       0x002000, 0x0ef8d2cd, BRF_GRA },		  //  7 Tiles
	{ "epr-6743.66",       0x002000, 0xc33062b5, BRF_GRA },		  //  8 Tiles
	{ "epr-6742.65",       0x002000, 0xc457e8c5, BRF_GRA },		  //  9 Tiles
	
	{ "epr-6735.117",      0x004000, 0x1be35a97, BRF_GRA },		  //  10 Sprites
	{ "epr-6737.04",       0x004000, 0x6b53aa7a, BRF_GRA },		  //  11 Sprites
	{ "epr-6736.110",      0x004000, 0x565c25d0, BRF_GRA },		  //  12 Sprites
	{ "epr-6738.05",       0x004000, 0xe116285f, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Teddybbo);
STD_ROM_FN(Teddybbo);

static struct BurnRomInfo UpndownRomDesc[] = {
	{ "epr5516a.129",      0x002000, 0x038c82da, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr5517a.130",      0x002000, 0x6930e1de, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5518.131",      0x002000, 0x2a370c99, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5519.132",      0x002000, 0x9d664a58, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5520.133",      0x002000, 0x208dfbdf, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5521.134",      0x002000, 0xe7b8d87a, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5535.3",        0x002000, 0xcf4e4c45, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5527.82",       0x002000, 0xb2d616f1, BRF_GRA },		  //  7 Tiles
	{ "epr-5526.65",       0x002000, 0x8a8b33c2, BRF_GRA },		  //  8 Tiles
	{ "epr-5525.81",       0x002000, 0xe749c5ef, BRF_GRA },		  //  9 Tiles
	{ "epr-5524.64",       0x002000, 0x8b886952, BRF_GRA },		  //  10 Tiles
	{ "epr-5523.80",       0x002000, 0xdede35d9, BRF_GRA },		  //  11 Tiles
	{ "epr-5522.63",       0x002000, 0x5e6d9dff, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5514.86",       0x004000, 0xfcc0a88b, BRF_GRA },		  //  13 Sprites
	{ "epr-5515.93",       0x004000, 0x60908838, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Upndown);
STD_ROM_FN(Upndown);

static struct BurnRomInfo UpndownuRomDesc[] = {
	{ "epr-5679.129",      0x002000, 0xc4f2f9c2, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-5680.130",      0x002000, 0x837f021c, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-5681.131",      0x002000, 0xe1c7ff7e, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-5682.132",      0x002000, 0x4a5edc1e, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-5520.133",      0x002000, 0x208dfbdf, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-5684.133",      0x002000, 0x32fa95da, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-5528.3",        0x002000, 0x00cd44ab, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-5527.82",       0x002000, 0xb2d616f1, BRF_GRA },		  //  7 Tiles
	{ "epr-5526.65",       0x002000, 0x8a8b33c2, BRF_GRA },		  //  8 Tiles
	{ "epr-5525.81",       0x002000, 0xe749c5ef, BRF_GRA },		  //  9 Tiles
	{ "epr-5524.64",       0x002000, 0x8b886952, BRF_GRA },		  //  10 Tiles
	{ "epr-5523.80",       0x002000, 0xdede35d9, BRF_GRA },		  //  11 Tiles
	{ "epr-5522.63",       0x002000, 0x5e6d9dff, BRF_GRA },		  //  12 Tiles
	
	{ "epr-5514.86",       0x004000, 0xfcc0a88b, BRF_GRA },		  //  13 Sprites
	{ "epr-5515.93",       0x004000, 0x60908838, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Upndownu);
STD_ROM_FN(Upndownu);

static struct BurnRomInfo WboyRomDesc[] = {
	{ "epr-7489.116",      0x004000, 0x130f4b70, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-7490.109",      0x004000, 0x9e656733, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-7491.96",       0x004000, 0x1f7d0efe, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-7498.120",      0x002000, 0x78ae1e7b, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-7497.62",       0x002000, 0x08d609ca, BRF_GRA },		  //  4 Tiles
	{ "epr-7496.61",       0x002000, 0x6f61fdf1, BRF_GRA },		  //  5 Tiles
	{ "epr-7495.64",       0x002000, 0x6a0d2c2d, BRF_GRA },		  //  6 Tiles
	{ "epr-7494.63",       0x002000, 0xa8e281c7, BRF_GRA },		  //  7 Tiles
	{ "epr-7493.66",       0x002000, 0x89305df4, BRF_GRA },		  //  8 Tiles
	{ "epr-7492.65",       0x002000, 0x60f806b1, BRF_GRA },		  //  9 Tiles
	
	{ "epr-7485.117",      0x004000, 0xc2891722, BRF_GRA },		  //  10 Sprites
	{ "epr-7487.04",       0x004000, 0x2d3a421b, BRF_GRA },		  //  11 Sprites
	{ "epr-7486.110",      0x004000, 0x8d622c50, BRF_GRA },		  //  12 Sprites
	{ "epr-7488.05",       0x004000, 0x007c2f1b, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Wboy);
STD_ROM_FN(Wboy);

static struct BurnRomInfo WboyoRomDesc[] = {
	{ "epr-.116",          0x004000, 0x51d27534, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-.109",          0x004000, 0xe29d1cd1, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-7491.96",       0x004000, 0x1f7d0efe, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-7498.120",      0x002000, 0x78ae1e7b, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-7497.62",       0x002000, 0x08d609ca, BRF_GRA },		  //  4 Tiles
	{ "epr-7496.61",       0x002000, 0x6f61fdf1, BRF_GRA },		  //  5 Tiles
	{ "epr-7495.64",       0x002000, 0x6a0d2c2d, BRF_GRA },		  //  6 Tiles
	{ "epr-7494.63",       0x002000, 0xa8e281c7, BRF_GRA },		  //  7 Tiles
	{ "epr-7493.66",       0x002000, 0x89305df4, BRF_GRA },		  //  8 Tiles
	{ "epr-7492.65",       0x002000, 0x60f806b1, BRF_GRA },		  //  9 Tiles
	
	{ "epr-7485.117",      0x004000, 0xc2891722, BRF_GRA },		  //  10 Sprites
	{ "epr-7487.04",       0x004000, 0x2d3a421b, BRF_GRA },		  //  11 Sprites
	{ "epr-7486.110",      0x004000, 0x8d622c50, BRF_GRA },		  //  12 Sprites
	{ "epr-7488.05",       0x004000, 0x007c2f1b, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Wboyo);
STD_ROM_FN(Wboyo);

static struct BurnRomInfo Wboy2RomDesc[] = {
	{ "epr-7587.129",      0x002000, 0x1bbb7354, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "epr-7588.130",      0x002000, 0x21007413, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "epr-7589.131",      0x002000, 0x44b30433, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "epr-7590.132",      0x002000, 0xbb525a0b, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-7591.133",      0x002000, 0x8379aa23, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-7592.134",      0x002000, 0xc767a5d7, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr-7498.120",      0x002000, 0x78ae1e7b, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-7497.62",       0x002000, 0x08d609ca, BRF_GRA },		  //  7 Tiles
	{ "epr-7496.61",       0x002000, 0x6f61fdf1, BRF_GRA },		  //  8 Tiles
	{ "epr-7495.64",       0x002000, 0x6a0d2c2d, BRF_GRA },		  //  9 Tiles
	{ "epr-7494.63",       0x002000, 0xa8e281c7, BRF_GRA },		  //  10 Tiles
	{ "epr-7493.66",       0x002000, 0x89305df4, BRF_GRA },		  //  11 Tiles
	{ "epr-7492.65",       0x002000, 0x60f806b1, BRF_GRA },		  //  12 Tiles
	
	{ "epr-7485.117",      0x004000, 0xc2891722, BRF_GRA },		  //  13 Sprites
	{ "epr-7487.04",       0x004000, 0x2d3a421b, BRF_GRA },		  //  14 Sprites
	{ "epr-7486.110",      0x004000, 0x8d622c50, BRF_GRA },		  //  15 Sprites
	{ "epr-7488.05",       0x004000, 0x007c2f1b, BRF_GRA },		  //  16 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  17 Timing PROM
};

STD_ROM_PICK(Wboy2);
STD_ROM_FN(Wboy2);

static struct BurnRomInfo Wboy2uRomDesc[] = {
	{ "ic129_02.bin",      0x002000, 0x32c4b709, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "ic130_03.bin",      0x002000, 0x56463ede, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "ic131_04.bin",      0x002000, 0x775ed392, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "ic132_05.bin",      0x002000, 0x7b922708, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "epr-7591.133",      0x002000, 0x8379aa23, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "epr-7592.134",      0x002000, 0xc767a5d7, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr7498a.3",        0x002000, 0xc198205c, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-7497.62",       0x002000, 0x08d609ca, BRF_GRA },		  //  7 Tiles
	{ "epr-7496.61",       0x002000, 0x6f61fdf1, BRF_GRA },		  //  8 Tiles
	{ "epr-7495.64",       0x002000, 0x6a0d2c2d, BRF_GRA },		  //  9 Tiles
	{ "epr-7494.63",       0x002000, 0xa8e281c7, BRF_GRA },		  //  10 Tiles
	{ "epr-7493.66",       0x002000, 0x89305df4, BRF_GRA },		  //  11 Tiles
	{ "epr-7492.65",       0x002000, 0x60f806b1, BRF_GRA },		  //  12 Tiles
	
	{ "epr-7485.117",      0x004000, 0xc2891722, BRF_GRA },		  //  13 Sprites
	{ "epr-7487.04",       0x004000, 0x2d3a421b, BRF_GRA },		  //  14 Sprites
	{ "epr-7486.110",      0x004000, 0x8d622c50, BRF_GRA },		  //  15 Sprites
	{ "epr-7488.05",       0x004000, 0x007c2f1b, BRF_GRA },		  //  16 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  17 Timing PROM
};

STD_ROM_PICK(Wboy2u);
STD_ROM_FN(Wboy2u);

static struct BurnRomInfo Wboy3RomDesc[] = {
	{ "wb_1",              0x004000, 0xbd6fef49, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "wb_2",              0x004000, 0x4081b624, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "wb_3",              0x004000, 0xc48a0e36, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-7498.120",      0x002000, 0x78ae1e7b, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-7497.62",       0x002000, 0x08d609ca, BRF_GRA },		  //  4 Tiles
	{ "epr-7496.61",       0x002000, 0x6f61fdf1, BRF_GRA },		  //  5 Tiles
	{ "epr-7495.64",       0x002000, 0x6a0d2c2d, BRF_GRA },		  //  6 Tiles
	{ "epr-7494.63",       0x002000, 0xa8e281c7, BRF_GRA },		  //  7 Tiles
	{ "epr-7493.66",       0x002000, 0x89305df4, BRF_GRA },		  //  8 Tiles
	{ "epr-7492.65",       0x002000, 0x60f806b1, BRF_GRA },		  //  9 Tiles
	
	{ "epr-7485.117",      0x004000, 0xc2891722, BRF_GRA },		  //  10 Sprites
	{ "epr-7487.04",       0x004000, 0x2d3a421b, BRF_GRA },		  //  11 Sprites
	{ "epr-7486.110",      0x004000, 0x8d622c50, BRF_GRA },		  //  12 Sprites
	{ "epr-7488.05",       0x004000, 0x007c2f1b, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Wboy3);
STD_ROM_FN(Wboy3);

static struct BurnRomInfo Wboy4RomDesc[] = {
	{ "ic2.bin",           0x008000, 0x48b2c006, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "ic3.bin",           0x008000, 0x466cae31, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	
	{ "7583.126",          0x008000, 0x99334b3c, BRF_ESS | BRF_PRG }, //  2	Z80 #2 Program Code
	
	{ "epr7610.ic62",      0x004000, 0x1685d26a, BRF_GRA },		  //  3 Tiles
	{ "epr7609.ic64",      0x004000, 0x87ecba53, BRF_GRA },		  //  4 Tiles
	{ "epr7608.ic66",      0x004000, 0xe812b3ec, BRF_GRA },		  //  5 Tiles
	
	{ "7578.87",           0x008000, 0x6ff1637f, BRF_GRA },		  //  6 Sprites
	{ "7577.86",           0x008000, 0x58b3705e, BRF_GRA },		  //  7 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  8 Timing PROM
};

STD_ROM_PICK(Wboy4);
STD_ROM_FN(Wboy4);

static struct BurnRomInfo WboyuRomDesc[] = {
	{ "ic116_89.bin",      0x004000, 0x73d8cef0, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "ic109_90.bin",      0x004000, 0x29546828, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "ic096_91.bin",      0x004000, 0xc7145c2a, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	
	{ "epr-7498.120",      0x002000, 0x78ae1e7b, BRF_ESS | BRF_PRG }, //  3	Z80 #2 Program Code
	
	{ "epr-7497.62",       0x002000, 0x08d609ca, BRF_GRA },		  //  4 Tiles
	{ "epr-7496.61",       0x002000, 0x6f61fdf1, BRF_GRA },		  //  5 Tiles
	{ "epr-7495.64",       0x002000, 0x6a0d2c2d, BRF_GRA },		  //  6 Tiles
	{ "epr-7494.63",       0x002000, 0xa8e281c7, BRF_GRA },		  //  7 Tiles
	{ "epr-7493.66",       0x002000, 0x89305df4, BRF_GRA },		  //  8 Tiles
	{ "epr-7492.65",       0x002000, 0x60f806b1, BRF_GRA },		  //  9 Tiles
	
	{ "ic117_85.bin",      0x004000, 0x1ee96ae8, BRF_GRA },		  //  10 Sprites
	{ "ic004_87.bin",      0x004000, 0x119735bb, BRF_GRA },		  //  11 Sprites
	{ "ic110_86.bin",      0x004000, 0x26d0fac4, BRF_GRA },		  //  12 Sprites
	{ "ic005_88.bin",      0x004000, 0x2602e519, BRF_GRA },		  //  13 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  14 Timing PROM
};

STD_ROM_PICK(Wboyu);
STD_ROM_FN(Wboyu);

static struct BurnRomInfo WbdeluxeRomDesc[] = {
	{ "wbd1.bin",          0x002000, 0xa1bedbd7, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "ic130_03.bin",      0x002000, 0x56463ede, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "wbd3.bin",          0x002000, 0x6fcdbd4c, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "ic132_05.bin",      0x002000, 0x7b922708, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "wbd5.bin",          0x002000, 0xf6b02902, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "wbd6.bin",          0x002000, 0x43df21fe, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "epr7498a.3",        0x002000, 0xc198205c, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "epr-7497.62",       0x002000, 0x08d609ca, BRF_GRA },		  //  7 Tiles
	{ "epr-7496.61",       0x002000, 0x6f61fdf1, BRF_GRA },		  //  8 Tiles
	{ "epr-7495.64",       0x002000, 0x6a0d2c2d, BRF_GRA },		  //  9 Tiles
	{ "epr-7494.63",       0x002000, 0xa8e281c7, BRF_GRA },		  //  10 Tiles
	{ "epr-7493.66",       0x002000, 0x89305df4, BRF_GRA },		  //  11 Tiles
	{ "epr-7492.65",       0x002000, 0x60f806b1, BRF_GRA },		  //  12 Tiles
	
	{ "epr-7485.117",      0x004000, 0xc2891722, BRF_GRA },		  //  13 Sprites
	{ "epr-7487.04",       0x004000, 0x2d3a421b, BRF_GRA },		  //  14 Sprites
	{ "epr-7486.110",      0x004000, 0x8d622c50, BRF_GRA },		  //  15 Sprites
	{ "epr-7488.05",       0x004000, 0x007c2f1b, BRF_GRA },		  //  16 Sprites

	{ "pr-5317.76",        0x000100, 0x648350b8, BRF_OPT },		  //  17 Timing PROM
};

STD_ROM_PICK(Wbdeluxe);
STD_ROM_FN(Wbdeluxe);

static struct BurnRomInfo WmatchRomDesc[] = {
	{ "wm.129",            0x002000, 0xb6db4442, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "wm.130",            0x002000, 0x59a0a7a0, BRF_ESS | BRF_PRG }, //  1	Z80 #1 Program Code
	{ "wm.131",            0x002000, 0x4cb3856a, BRF_ESS | BRF_PRG }, //  2	Z80 #1 Program Code
	{ "wm.132",            0x002000, 0xe2e44b29, BRF_ESS | BRF_PRG }, //  3	Z80 #1 Program Code
	{ "wm.133",            0x002000, 0x43a36445, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	{ "wm.134",            0x002000, 0x5624794c, BRF_ESS | BRF_PRG }, //  5	Z80 #1 Program Code
	
	{ "wm.3",              0x002000, 0x50d2afb7, BRF_ESS | BRF_PRG }, //  6	Z80 #2 Program Code
	
	{ "wm.82",             0x002000, 0x540f0bf3, BRF_GRA },		  //  7 Tiles
	{ "wm.65",             0x002000, 0x92c1e39e, BRF_GRA },		  //  8 Tiles
	{ "wm.81",             0x002000, 0x6a01ff2a, BRF_GRA },		  //  9 Tiles
	{ "wm.64",             0x002000, 0xaae6449b, BRF_GRA },		  //  10 Tiles
	{ "wm.80",             0x002000, 0xfc3f0bd4, BRF_GRA },		  //  11 Tiles
	{ "wm.63",             0x002000, 0xc2ce9b93, BRF_GRA },		  //  12 Tiles
	
	{ "wm.86",             0x004000, 0x238ae0e5, BRF_GRA },		  //  13 Sprites
	{ "wm.93",             0x004000, 0xa2f19170, BRF_GRA },		  //  14 Sprites

	{ "pr-5317.106",       0x000100, 0x648350b8, BRF_OPT },		  //  15 Timing PROM
};

STD_ROM_PICK(Wmatch);
STD_ROM_FN(Wmatch);

/*==============================================================================================
Decode Functions
===============================================================================================*/

static void sega_decode(const UINT8 convtable[32][4])
{
	int A;

	int length = 0x10000;
	int cryptlen = 0x8000;
	UINT8 *rom = System1Rom1;
	UINT8 *decrypted = System1Fetch1;
	
	for (A = 0x0000;A < cryptlen;A++)
	{
		int xorval = 0;

		UINT8 src = rom[A];

		/* pick the translation table from bits 0, 4, 8 and 12 of the address */
		int row = (A & 1) + (((A >> 4) & 1) << 1) + (((A >> 8) & 1) << 2) + (((A >> 12) & 1) << 3);

		/* pick the offset in the table from bits 3 and 5 of the source data */
		int col = ((src >> 3) & 1) + (((src >> 5) & 1) << 1);
		/* the bottom half of the translation table is the mirror image of the top */
		if (src & 0x80)
		{
			col = 3 - col;
			xorval = 0xa8;
		}

		/* decode the opcodes */
		decrypted[A] = (src & ~0xa8) | (convtable[2*row][col] ^ xorval);

		/* decode the data */
		rom[A] = (src & ~0xa8) | (convtable[2*row+1][col] ^ xorval);

		if (convtable[2*row][col] == 0xff)	/* table incomplete! (for development) */
			decrypted[A] = 0xee;
		if (convtable[2*row+1][col] == 0xff)	/* table incomplete! (for development) */
			rom[A] = 0xee;
	}

	/* this is a kludge to catch anyone who has code that crosses the encrypted/ */
	/* decrypted boundary. ssanchan does it */
	if (length > 0x8000)
	{
		int bytes = 0x4000;
		memcpy(&decrypted[0x8000], &rom[0x8000], bytes);
	}
}

static void flicky_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x08,0x88,0x00,0x80 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...0...0...0 */
		{ 0x80,0x00,0xa0,0x20 }, { 0x88,0x80,0x08,0x00 },	/* ...0...0...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x28,0x08,0x20,0x00 },	/* ...0...0...1...0 */
		{ 0x28,0x08,0x20,0x00 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...0...1...1 */
		{ 0x08,0x88,0x00,0x80 }, { 0x80,0x00,0xa0,0x20 },	/* ...0...1...0...0 */
		{ 0x80,0x00,0xa0,0x20 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...0...1 */
		{ 0x28,0x08,0x20,0x00 }, { 0x28,0x08,0x20,0x00 },	/* ...0...1...1...0 */
		{ 0x28,0x08,0x20,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...1...1 */
		{ 0x08,0x88,0x00,0x80 }, { 0xa8,0x88,0x28,0x08 },	/* ...1...0...0...0 */
		{ 0xa8,0x88,0x28,0x08 }, { 0x80,0x00,0xa0,0x20 },	/* ...1...0...0...1 */
		{ 0x28,0x08,0x20,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...1...0...1...0 */
		{ 0xa8,0x88,0x28,0x08 }, { 0x88,0x80,0x08,0x00 },	/* ...1...0...1...1 */
		{ 0x08,0x88,0x00,0x80 }, { 0x80,0x00,0xa0,0x20 },	/* ...1...1...0...0 */
		{ 0xa8,0x88,0x28,0x08 }, { 0x80,0x00,0xa0,0x20 },	/* ...1...1...0...1 */
		{ 0x28,0x08,0x20,0x00 }, { 0x28,0x08,0x20,0x00 },	/* ...1...1...1...0 */
		{ 0x08,0x88,0x00,0x80 }, { 0x88,0x80,0x08,0x00 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

static void hvymetal_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x88,0xa8,0x80,0xa0 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...0...0...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0x80,0x08,0x00 },	/* ...0...0...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x88,0xa8,0x80,0xa0 },	/* ...0...0...1...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0x80,0x08,0x00 },	/* ...0...0...1...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...0...0 */
		{ 0x88,0x80,0x08,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...1...0 */
		{ 0x88,0x80,0x08,0x00 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...1...1...1 */
		{ 0xa0,0x20,0xa8,0x28 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...0...0...0 */
		{ 0xa0,0x20,0xa8,0x28 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...0...0...1 */
		{ 0xa0,0x20,0xa8,0x28 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...0...1...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...0...1...1 */
		{ 0x28,0xa8,0x08,0x88 }, { 0xa0,0x20,0xa8,0x28 },	/* ...1...1...0...0 */
		{ 0xa0,0x20,0xa8,0x28 }, { 0x28,0xa8,0x08,0x88 },	/* ...1...1...0...1 */
		{ 0x28,0xa8,0x08,0x88 }, { 0xa0,0x20,0xa8,0x28 },	/* ...1...1...1...0 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x28,0xa8,0x08,0x88 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

void mrviking_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x28,0xa8,0x08,0x88 }, { 0x88,0x80,0x08,0x00 },	/* ...0...0...0...0 */
		{ 0x88,0x08,0x80,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...0...0...0...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x28,0xa8,0x08,0x88 },	/* ...0...0...1...0 */
		{ 0x88,0x08,0x80,0x00 }, { 0x88,0x08,0x80,0x00 },	/* ...0...0...1...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...0...0 */
		{ 0x88,0x80,0x08,0x00 }, { 0x28,0xa8,0x08,0x88 },	/* ...0...1...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...1...1...0 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...1...1...1 */
		{ 0x88,0x80,0x08,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...1...0...0...0 */
		{ 0x88,0x08,0x80,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...1...0...0...1 */
		{ 0xa0,0x80,0x20,0x00 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...0...1...0 */
		{ 0xa0,0x80,0x20,0x00 }, { 0x88,0x08,0x80,0x00 },	/* ...1...0...1...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0xa0,0x80,0x20,0x00 },	/* ...1...1...0...0 */
		{ 0xa0,0x80,0x20,0x00 }, { 0xa0,0x80,0x20,0x00 },	/* ...1...1...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...1...1...0 */
		{ 0xa0,0x80,0x20,0x00 }, { 0xa0,0x80,0xa8,0x88 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

static void nprinces_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x08,0x88,0x00,0x80 }, { 0xa0,0x20,0x80,0x00 },	/* ...0...0...0...0 */
		{ 0xa8,0xa0,0x28,0x20 }, { 0x88,0xa8,0x80,0xa0 },	/* ...0...0...0...1 */
		{ 0x88,0x80,0x08,0x00 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...0...1...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...0...1...1 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0xa0,0x20,0x80,0x00 },	/* ...0...1...0...0 */
		{ 0xa8,0xa0,0x28,0x20 }, { 0xa8,0xa0,0x28,0x20 },	/* ...0...1...0...1 */
		{ 0x88,0x80,0x08,0x00 }, { 0x88,0xa8,0x80,0xa0 },	/* ...0...1...1...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0xa8,0x80,0xa0 },	/* ...0...1...1...1 */
		{ 0xa0,0x20,0x80,0x00 }, { 0xa0,0x20,0x80,0x00 },	/* ...1...0...0...0 */
		{ 0x08,0x88,0x00,0x80 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...0...0...1 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0x80,0x08,0x00 },	/* ...1...0...1...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...0...1...1 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...1...0...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...1...0...1 */
		{ 0x88,0x80,0x08,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...1...1...1...0 */
		{ 0x08,0x88,0x00,0x80 }, { 0x28,0x08,0xa8,0x88 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

static void pitfall2_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0xa0,0x80,0xa8,0x88 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...0...0...0 */
		{ 0x08,0x88,0x28,0xa8 }, { 0x28,0xa8,0x20,0xa0 },	/* ...0...0...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...0...1...0 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0xa8,0x20,0x28 },	/* ...0...0...1...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x20,0x00,0xa0,0x80 },	/* ...0...1...0...0 */
		{ 0x28,0xa8,0x20,0xa0 }, { 0x20,0x00,0xa0,0x80 },	/* ...0...1...0...1 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0xa8,0x20,0x28 },	/* ...0...1...1...0 */
		{ 0x28,0xa8,0x20,0xa0 }, { 0xa0,0xa8,0x20,0x28 },	/* ...0...1...1...1 */
		{ 0x20,0x00,0xa0,0x80 }, { 0x80,0x88,0xa0,0xa8 },	/* ...1...0...0...0 */
		{ 0x80,0x88,0xa0,0xa8 }, { 0x80,0x88,0xa0,0xa8 },	/* ...1...0...0...1 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0x80,0xa8,0x88 },	/* ...1...0...1...0 */
		{ 0x80,0x88,0xa0,0xa8 }, { 0x28,0xa8,0x20,0xa0 },	/* ...1...0...1...1 */
		{ 0x20,0x00,0xa0,0x80 }, { 0x80,0x88,0xa0,0xa8 },	/* ...1...1...0...0 */
		{ 0x80,0x88,0xa0,0xa8 }, { 0x20,0x00,0xa0,0x80 },	/* ...1...1...0...1 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0x80,0xa8,0x88 },	/* ...1...1...1...0 */
		{ 0x80,0x88,0xa0,0xa8 }, { 0x28,0xa8,0x20,0xa0 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

static void regulus_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x28,0x08,0xa8,0x88 }, { 0x88,0x80,0x08,0x00 },	/* ...0...0...0...0 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x28,0xa8,0x08,0x88 },	/* ...0...0...0...1 */
		{ 0x88,0x80,0x08,0x00 }, { 0x88,0x08,0x80,0x00 },	/* ...0...0...1...0 */
		{ 0x88,0x08,0x80,0x00 }, { 0x28,0xa8,0x08,0x88 },	/* ...0...0...1...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...0...0 */
		{ 0x88,0x80,0x08,0x00 }, { 0x88,0x80,0x08,0x00 },	/* ...0...1...0...1 */
		{ 0x88,0x08,0x80,0x00 }, { 0x88,0x08,0x80,0x00 },	/* ...0...1...1...0 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...1...1...1 */
		{ 0x80,0xa0,0x00,0x20 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...0...0...0 */
		{ 0x28,0xa8,0x08,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...0...0...1 */
		{ 0x80,0xa0,0x00,0x20 }, { 0x80,0xa0,0x00,0x20 },	/* ...1...0...1...0 */
		{ 0x28,0xa8,0x08,0x88 }, { 0x80,0xa0,0x00,0x20 },	/* ...1...0...1...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...1...0...0 */
		{ 0x80,0xa0,0x00,0x20 }, { 0xa0,0x80,0xa8,0x88 },	/* ...1...1...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x80,0xa0,0x00,0x20 },	/* ...1...1...1...0 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0xa0,0x80,0xa8,0x88 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

static void seganinj_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0x08,0x80,0x00 },	/* ...0...0...0...0 */
		{ 0x28,0xa8,0x08,0x88 }, { 0xa0,0xa8,0x80,0x88 },	/* ...0...0...0...1 */
		{ 0xa8,0xa0,0x28,0x20 }, { 0xa8,0xa0,0x28,0x20 },	/* ...0...0...1...0 */
		{ 0x28,0xa8,0x08,0x88 }, { 0xa0,0xa8,0x80,0x88 },	/* ...0...0...1...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...1...0...0 */
		{ 0x28,0xa8,0x08,0x88 }, { 0x88,0x08,0x80,0x00 },	/* ...0...1...0...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...1...1...0 */
		{ 0x28,0xa8,0x08,0x88 }, { 0xa8,0xa0,0x28,0x20 },	/* ...0...1...1...1 */
		{ 0x88,0x08,0x80,0x00 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...0...0...0 */
		{ 0xa0,0xa8,0x80,0x88 }, { 0x28,0xa8,0x08,0x88 },	/* ...1...0...0...1 */
		{ 0xa8,0xa0,0x28,0x20 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...0...1...0 */
		{ 0xa8,0xa0,0x28,0x20 }, { 0x28,0xa8,0x08,0x88 },	/* ...1...0...1...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...1...0...0 */
		{ 0x28,0x08,0xa8,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...1...1...0...1 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...1...1...0 */
		{ 0xa8,0xa0,0x28,0x20 }, { 0x28,0x08,0xa8,0x88 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

void swat_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x88,0x08,0x80,0x00 }, { 0xa0,0xa8,0x80,0x88 },	/* ...0...0...0...0 */
		{ 0x88,0x08,0x80,0x00 }, { 0x88,0xa8,0x80,0xa0 },	/* ...0...0...0...1 */
		{ 0xa0,0x80,0x20,0x00 }, { 0x88,0x08,0x80,0x00 },	/* ...0...0...1...0 */
		{ 0xa0,0xa8,0x80,0x88 }, { 0x88,0x08,0x80,0x00 },	/* ...0...0...1...1 */
		{ 0x28,0x20,0xa8,0xa0 }, { 0xa0,0xa8,0x80,0x88 },	/* ...0...1...0...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x28,0x20,0xa8,0xa0 },	/* ...0...1...0...1 */
		{ 0xa0,0x80,0x20,0x00 }, { 0xa0,0xa8,0x80,0x88 },	/* ...0...1...1...0 */
		{ 0x28,0x20,0xa8,0xa0 }, { 0xa0,0xa8,0x80,0x88 },	/* ...0...1...1...1 */
		{ 0xa0,0x80,0x20,0x00 }, { 0xa0,0x80,0x20,0x00 },	/* ...1...0...0...0 */
		{ 0xa0,0x20,0x80,0x00 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...0...0...1 */
		{ 0xa0,0x20,0x80,0x00 }, { 0xa0,0x20,0x80,0x00 },	/* ...1...0...1...0 */
		{ 0xa0,0x20,0x80,0x00 }, { 0xa0,0x20,0x80,0x00 },	/* ...1...0...1...1 */
		{ 0xa0,0x80,0x20,0x00 }, { 0xa0,0x80,0x20,0x00 },	/* ...1...1...0...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x28,0x20,0xa8,0xa0 },	/* ...1...1...0...1 */
		{ 0xa0,0xa8,0x80,0x88 }, { 0xa0,0x80,0x20,0x00 },	/* ...1...1...1...0 */
		{ 0x28,0x20,0xa8,0xa0 }, { 0xa0,0xa8,0x80,0x88 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

static void teddybb_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x20,0x28,0x00,0x08 }, { 0x80,0x00,0xa0,0x20 },	/* ...0...0...0...0 */
		{ 0x20,0x28,0x00,0x08 }, { 0xa0,0xa8,0x20,0x28 },	/* ...0...0...0...1 */
		{ 0x28,0x08,0xa8,0x88 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...0...1...0 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0x80,0xa8,0x88 },	/* ...0...0...1...1 */
		{ 0x20,0x28,0x00,0x08 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...1...0...0 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0xa8,0x20,0x28 },	/* ...0...1...0...1 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...1...1...0 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0x28,0x08,0xa8,0x88 },	/* ...0...1...1...1 */
		{ 0x80,0x00,0xa0,0x20 }, { 0x80,0x00,0xa0,0x20 },	/* ...1...0...0...0 */
		{ 0xa0,0x20,0xa8,0x28 }, { 0xa0,0xa8,0x20,0x28 },	/* ...1...0...0...1 */
		{ 0xa0,0x20,0xa8,0x28 }, { 0xa0,0x80,0xa8,0x88 },	/* ...1...0...1...0 */
		{ 0xa0,0x80,0xa8,0x88 }, { 0xa0,0x80,0xa8,0x88 },	/* ...1...0...1...1 */
		{ 0x80,0x00,0xa0,0x20 }, { 0x20,0x28,0x00,0x08 },	/* ...1...1...0...0 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0x20,0xa8,0x28 },	/* ...1...1...0...1 */
		{ 0x80,0x00,0xa0,0x20 }, { 0xa0,0x80,0xa8,0x88 },	/* ...1...1...1...0 */
		{ 0xa0,0xa8,0x20,0x28 }, { 0xa0,0x20,0xa8,0x28 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

void wmatch_decode(void)
{
	static const UINT8 convtable[32][4] =
	{
		/*       opcode                   data                     address      */
		/*  A    B    C    D         A    B    C    D                           */
		{ 0x88,0xa8,0x80,0xa0 }, { 0xa0,0x80,0x20,0x00 },	/* ...0...0...0...0 */
		{ 0x08,0x88,0x00,0x80 }, { 0x88,0xa8,0x80,0xa0 },	/* ...0...0...0...1 */
		{ 0x20,0x00,0xa0,0x80 }, { 0x20,0x28,0xa0,0xa8 },	/* ...0...0...1...0 */
		{ 0x20,0x28,0xa0,0xa8 }, { 0xa0,0x80,0x20,0x00 },	/* ...0...0...1...1 */
		{ 0xa8,0x28,0x88,0x08 }, { 0xa8,0x28,0x88,0x08 },	/* ...0...1...0...0 */
		{ 0x08,0x88,0x00,0x80 }, { 0xa8,0x28,0x88,0x08 },	/* ...0...1...0...1 */
		{ 0xa8,0x28,0x88,0x08 }, { 0x20,0x28,0xa0,0xa8 },	/* ...0...1...1...0 */
		{ 0xa8,0x28,0x88,0x08 }, { 0xa8,0x28,0x88,0x08 },	/* ...0...1...1...1 */
		{ 0x20,0x28,0xa0,0xa8 }, { 0x88,0xa8,0x80,0xa0 },	/* ...1...0...0...0 */
		{ 0x88,0xa8,0x80,0xa0 }, { 0x20,0x28,0xa0,0xa8 },	/* ...1...0...0...1 */
		{ 0x20,0x28,0xa0,0xa8 }, { 0xa0,0x80,0x20,0x00 },	/* ...1...0...1...0 */
		{ 0x20,0x28,0xa0,0xa8 }, { 0x20,0x28,0xa0,0xa8 },	/* ...1...0...1...1 */
		{ 0x20,0x00,0xa0,0x80 }, { 0x20,0x28,0xa0,0xa8 },	/* ...1...1...0...0 */
		{ 0xa8,0x28,0x88,0x08 }, { 0xa0,0x80,0x20,0x00 },	/* ...1...1...0...1 */
		{ 0x20,0x28,0xa0,0xa8 }, { 0x20,0x28,0xa0,0xa8 },	/* ...1...1...1...0 */
		{ 0xa8,0x28,0x88,0x08 }, { 0xa8,0x28,0x88,0x08 }	/* ...1...1...1...1 */
	};

	sega_decode(convtable);
}

static void sega_decode_2(const UINT8 opcode_xor[64],const int opcode_swap_select[64],
		const UINT8 data_xor[64],const int data_swap_select[64])
{
	int A;
	static const UINT8 swaptable[24][4] =
	{
		{ 6,4,2,0 }, { 4,6,2,0 }, { 2,4,6,0 }, { 0,4,2,6 },
		{ 6,2,4,0 }, { 6,0,2,4 }, { 6,4,0,2 }, { 2,6,4,0 },
		{ 4,2,6,0 }, { 4,6,0,2 }, { 6,0,4,2 }, { 0,6,4,2 },
		{ 4,0,6,2 }, { 0,4,6,2 }, { 6,2,0,4 }, { 2,6,0,4 },
		{ 0,6,2,4 }, { 2,0,6,4 }, { 0,2,6,4 }, { 4,2,0,6 },
		{ 2,4,0,6 }, { 4,0,2,6 }, { 2,0,4,6 }, { 0,2,4,6 },
	};


	UINT8 *rom = System1Rom1;
	UINT8 *decrypted = System1Fetch1;

	for (A = 0x0000;A < 0x8000;A++)
	{
		int row;
		UINT8 src;
		const UINT8 *tbl;


		src = rom[A];

		/* pick the translation table from bits 0, 3, 6, 9, 12 and 14 of the address */
		row = (A & 1) + (((A >> 3) & 1) << 1) + (((A >> 6) & 1) << 2)
				+ (((A >> 9) & 1) << 3) + (((A >> 12) & 1) << 4) + (((A >> 14) & 1) << 5);

		/* decode the opcodes */
		tbl = swaptable[opcode_swap_select[row]];
		decrypted[A] = BITSWAP08(src,7,tbl[0],5,tbl[1],3,tbl[2],1,tbl[3]) ^ opcode_xor[row];

		/* decode the data */
		tbl = swaptable[data_swap_select[row]];
		rom[A] = BITSWAP08(src,7,tbl[0],5,tbl[1],3,tbl[2],1,tbl[3]) ^ data_xor[row];
	}
}

static void astrofl_decode(void)
{
	static const UINT8 opcode_xor[64] =
	{
		0x04,0x51,0x40,0x01,0x55,0x44,0x05,0x50,0x41,0x00,0x54,0x45,
		0x04,0x51,0x40,0x01,0x55,0x44,0x05,0x50,0x41,0x00,0x54,0x45,
		0x04,0x51,0x40,0x01,0x55,0x44,0x05,0x50,
		0x04,0x51,0x40,0x01,0x55,0x44,0x05,0x50,0x41,0x00,0x54,0x45,
		0x04,0x51,0x40,0x01,0x55,0x44,0x05,0x50,0x41,0x00,0x54,0x45,
		0x04,0x51,0x40,0x01,0x55,0x44,0x05,0x50,
	};

	static const UINT8 data_xor[64] =
	{
		0x54,0x15,0x44,0x51,0x10,0x41,0x55,0x14,0x45,0x50,0x11,0x40,
		0x54,0x15,0x44,0x51,0x10,0x41,0x55,0x14,0x45,0x50,0x11,0x40,
		0x54,0x15,0x44,0x51,0x10,0x41,0x55,0x14,
		0x54,0x15,0x44,0x51,0x10,0x41,0x55,0x14,0x45,0x50,0x11,0x40,
		0x54,0x15,0x44,0x51,0x10,0x41,0x55,0x14,0x45,0x50,0x11,0x40,
		0x54,0x15,0x44,0x51,0x10,0x41,0x55,0x14,
	};

	static const int opcode_swap_select[64] =
	{
		0,0,1,1,1,2,2,3,3,4,4,4,5,5,6,6,
		6,7,7,8,8,9,9,9,10,10,11,11,11,12,12,13,

		8,8,9,9,9,10,10,11,11,12,12,12,13,13,14,14,
		14,15,15,16,16,17,17,17,18,18,19,19,19,20,20,21,
	};

	static const int data_swap_select[64] =
	{
		0,0,1,1,2,2,2,3,3,4,4,5,5,5,6,6,
		7,7,7,8,8,9,9,10,10,10,11,11,12,12,12,13,

		8,8,9,9,10,10,10,11,11,12,12,13,13,13,14,14,
		15,15,15,16,16,17,17,18,18,18,19,19,20,20,20,21,
	};

	sega_decode_2(opcode_xor,opcode_swap_select,data_xor,data_swap_select);
}

static void fdwarrio_decode(void)
{
	static const UINT8 opcode_xor[64] =
	{
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
		0x40,0x50,0x44,0x54,0x41,0x51,0x45,0x55,
	};

	static const UINT8 data_xor[64] =
	{
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
		0x10,0x04,0x14,0x01,0x11,0x05,0x15,0x00,
	};

	static const int opcode_swap_select[64] =
	{
		4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,
		10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,
	};

	static const int data_swap_select[64] =
	{
		  4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,
		10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,
		12,
	};

	sega_decode_2(opcode_xor,opcode_swap_select,data_xor,data_swap_select);
}

static void wboy2_decode(void)
{
	static const UINT8 opcode_xor[64] =
	{
		0x00,0x45,0x11,0x01,0x44,0x10,0x55,0x05,0x41,0x14,0x04,0x40,0x15,0x51,
		0x01,0x44,0x10,0x00,0x45,0x11,0x54,0x04,0x40,0x15,0x05,0x41,0x14,0x50,
		0x00,0x45,0x11,0x01,
		0x00,0x45,0x11,0x01,0x44,0x10,0x55,0x05,0x41,0x14,0x04,0x40,0x15,0x51,
		0x01,0x44,0x10,0x00,0x45,0x11,0x54,0x04,0x40,0x15,0x05,0x41,0x14,0x50,
		0x00,0x45,0x11,0x01,
	};

	static const UINT8 data_xor[64] =
	{
		0x55,0x05,0x41,0x14,0x50,0x00,0x15,0x51,0x01,0x44,0x10,0x55,0x05,0x11,
		0x54,0x04,0x40,0x15,0x51,0x01,0x14,0x50,0x00,0x45,0x11,0x54,0x04,0x10,
		0x55,0x05,0x41,0x14,
		0x55,0x05,0x41,0x14,0x50,0x00,0x15,0x51,0x01,0x44,0x10,0x55,0x05,0x11,
		0x54,0x04,0x40,0x15,0x51,0x01,0x14,0x50,0x00,0x45,0x11,0x54,0x04,0x10,
		0x55,0x05,0x41,0x14,
	};

	static const int opcode_swap_select[64] =
	{
		2,
		5,1,5,1,5,
		0,4,0,4,0,4,
		7,3,7,3,7,3,
		6,2,6,2,6,
		1,5,1,5,1,5,
		0,4,0,

		10,
		13,9,13,9,13,
		8,12,8,12,8,12,
		15,11,15,11,15,11,
		14,10,14,10,14,
		9,13,9,13,9,13,
		8,12,8,
	};

	static const int data_swap_select[64] =
	{
		3,7,3,7,3,7,
		2,6,2,6,2,
		5,1,5,1,5,1,
		4,0,4,0,4,
		8,
		3,7,3,7,3,
		6,2,6,2,

		11,15,11,15,11,15,
		10,14,10,14,10,
		13,9,13,9,13,9,
		12,8,12,8,12,
		16,
		11,15,11,15,11,
		14,10,14,10,
	};

	sega_decode_2(opcode_xor,opcode_swap_select,data_xor,data_swap_select);
}

static void sega_decode_317(int order, int opcode_shift, int data_shift)
{
	static const UINT8 xor1_317[1+64] =
	{
		0x54,
		0x14,0x15,0x41,0x14,0x50,0x55,0x05,0x41,0x01,0x10,0x51,0x05,0x11,0x05,0x14,0x55,
		0x41,0x05,0x04,0x41,0x14,0x10,0x45,0x50,0x00,0x45,0x00,0x00,0x00,0x45,0x00,0x00,
		0x54,0x04,0x15,0x10,0x04,0x05,0x11,0x44,0x04,0x01,0x05,0x00,0x44,0x15,0x40,0x45,
		0x10,0x15,0x51,0x50,0x00,0x15,0x51,0x44,0x15,0x04,0x44,0x44,0x50,0x10,0x04,0x04,
	};

	static const UINT8 xor2_317[2+64] =
	{
		0x04,
		0x44,
		0x15,0x51,0x41,0x10,0x15,0x54,0x04,0x51,0x05,0x55,0x05,0x54,0x45,0x04,0x10,0x01,
		0x51,0x55,0x45,0x55,0x45,0x04,0x55,0x40,0x11,0x15,0x01,0x40,0x01,0x11,0x45,0x44,
		0x40,0x05,0x15,0x15,0x01,0x50,0x00,0x44,0x04,0x50,0x51,0x45,0x50,0x54,0x41,0x40,
		0x14,0x40,0x50,0x45,0x10,0x05,0x50,0x01,0x40,0x01,0x50,0x50,0x50,0x44,0x40,0x10,
	};

	static const int swap1_317[1+64] =
	{
		 7,
		 1,11,23,17,23, 0,15,19,
		20,12,10, 0,18,18, 5,20,
		13, 0,18,14, 5, 6,10,21,
		 1,11, 9, 3,21, 4, 1,17,
		 5, 7,16,13,19,23,20, 2,
		10,23,23,15,10,12, 0,22,
		14, 6,15,11,17,15,21, 0,
		 6, 1, 1,18, 5,15,15,20,
	};

	static const int swap2_317[2+64] =
	{
		 7,
		12,
		18, 8,21, 0,22,21,13,21,
		20,13,20,14, 6, 3, 5,20,
		 8,20, 4, 8,17,22, 0, 0,
		 6,17,17, 9, 0,16,13,21,
		 3, 2,18, 6,11, 3, 3,18,
		18,19, 3, 0, 5, 0,11, 8,
		 8, 1, 7, 2,10, 8,10, 2,
		 1, 3,12,16, 0,17,10, 1,
	};

	if (order)
		sega_decode_2( xor2_317+opcode_shift, swap2_317+opcode_shift, xor1_317+data_shift, swap1_317+data_shift );
	else
		sega_decode_2( xor1_317+opcode_shift, swap1_317+opcode_shift, xor2_317+data_shift, swap2_317+data_shift );
}

static void gardia_decode()
{
	sega_decode_317( 1, 1, 1 );
}
#define BIT(x,n) (((x)>>(n))&1)

#undef BIT
static void blockgal_decode()
{
	mc8123_decrypt_rom(0, 0, System1Rom1, System1Fetch1, System1MC8123Key);
}

/*==============================================================================================
Allocate Memory
===============================================================================================*/

static int MemIndex()
{
	unsigned char *Next; Next = Mem;

	System1Rom1            = Next; Next += 0x020000;
	System1Fetch1          = Next; Next += 0x010000;
	System1Rom2            = Next; Next += 0x008000;
	System1PromRed         = Next; Next += 0x000100;
	System1PromGreen       = Next; Next += 0x000100;
	System1PromBlue        = Next; Next += 0x000100;

	RamStart = Next;

	System1Ram1            = Next; Next += 0x0020fd;
	System1Ram2            = Next; Next += 0x000800;
	System1SpriteRam       = Next; Next += 0x000200;
	System1PaletteRam      = Next; Next += 0x000600;
	System1BgRam           = Next; Next += 0x000800;
	System1VideoRam        = Next; Next += 0x000700;
	System1BgCollisionRam  = Next; Next += 0x000400;
	System1SprCollisionRam = Next; Next += 0x000400;
	System1deRam           = Next; Next += 0x000200;
	System1efRam           = Next; Next += 0x000100;
	System1f4Ram           = Next; Next += 0x000400;
	System1fcRam           = Next; Next += 0x000400;
	SpriteOnScreenMap      = Next; Next += (256 * 256);
	
	RamEnd = Next;

	System1Sprites         = Next; Next += System1SpriteRomSize;
	System1Tiles           = Next; Next += (System1NumTiles * 8 * 8);
	System1TilesPenUsage   = (UINT32*)Next; Next += System1NumTiles * sizeof(UINT32);
	System1Palette         = (unsigned int*)Next; Next += 0x000600 * sizeof(unsigned int);
	MemEnd = Next;

	return 0;
}

/*==============================================================================================
Reset Functions
===============================================================================================*/

static int System1DoReset()
{
	OpenCPU(0);
	Z80Reset();
	CloseCPU(0);
	
	OpenCPU(1);
	Z80Reset();
	CloseCPU(1);
	
	System1ScrollX[0] = System1ScrollX[1] = System1ScrollY = 0;
	System1BgScrollX = 0;
	System1BgScrollY = 0;
	System1VideoMode = 0;
	System1FlipScreen = 0;
	System1SoundLatch = 0;
	System1RomBank = 0;
	NoboranbInp16Step = 0;
	NoboranbInp17Step = 0;
	NoboranbInp23Step = 0;
	BlockgalDial1 = 0;
	BlockgalDial2 = 0;
	
	return 0;
}

/*==============================================================================================
Memory Handlers
===============================================================================================*/

unsigned char __fastcall System1Z801PortRead(unsigned int a)
{
	a &= 0xff;
	
	switch (a) {
		case 0x00: {
			return 0xff - System1Input[0];
		}
	
		case 0x04: {
			return 0xff - System1Input[1];
		}
		
		case 0x08: {
			return 0xff - System1Input[2];
		}
		
		case 0x0c: {
			return System1Dip[0];
		}
		
		case 0x0d: {
			return System1Dip[1];
		}
	
		case 0x10: {
			return System1Dip[1];
		}
		
		case 0x11: {
			return System1Dip[0];
		}
		
		case 0x15: {
			return System1VideoMode;
		}
	
		case 0x19: {
			return System1VideoMode;
		}
	}	

	bprintf(PRINT_NORMAL, _T("IO Read %x\n"), a);
	return 0;
}

unsigned char __fastcall BlockgalZ801PortRead(unsigned int a)
{
	a &= 0xff;
	
	switch (a) {
		case 0x00: {
			return BlockgalDial1;
		}
	
		case 0x04: {
			return BlockgalDial2;
		}
		
		case 0x08: {
			return 0xff - System1Input[2];
		}
		
		case 0x0c: {
			return System1Dip[0];
		}
		
		case 0x0d: {
			return System1Dip[1];
		}
	
		case 0x10: {
			return System1Dip[1];
		}
		
		case 0x11: {
			return System1Dip[0];
		}
		
		case 0x15: {
			return System1VideoMode;
		}
	
		case 0x19: {
			return System1VideoMode;
		}
	}	

	bprintf(PRINT_NORMAL, _T("IO Read %x\n"), a);
	return 0;
}

unsigned char __fastcall NoboranbZ801PortRead(unsigned int a)
{
	a &= 0xff;
	
	switch (a) {
		case 0x00: {
			return 0xff - System1Input[0];
		}
		
		case 0x04: {
			return 0xff - System1Input[1];
		}
		
		case 0x08: {
			return 0xff - System1Input[2];
		}
		
		case 0x0c: {
			return System1Dip[0];
		}
		
		case 0x0d: {
			return System1Dip[1];
		}
		
		case 0x15: {
			return System1VideoMode;
		}
	
		case 0x16: {
			return NoboranbInp16Step;
		}
		
		case 0x1c: {
			return 0x80;
		}
		
		case 0x22: {
			return NoboranbInp17Step;
		}
		
		case 0x23: {
			return NoboranbInp23Step;
		}
	}	

	bprintf(PRINT_NORMAL, _T("IO Read %x\n"), a);
	return 0;
}

int FireNmi = 0;

void __fastcall System1Z801PortWrite(unsigned int a, unsigned char d)
{
	a &= 0xff;
	
	switch (a) {
		case 0x14:
		case 0x18: {
			System1SoundLatch = d;
			
			CloseCPU(0);
			OpenCPU(1);
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 1);
			Z80Execute(0);
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 0);
			Z80Execute(0);
			CloseCPU(1);
			OpenCPU(0);
			return;
		}
		
		case 0x15:		
		case 0x19: {
			System1VideoMode = d;
			System1FlipScreen = d & 0x80;
			return;
		}
	}
	
	bprintf(PRINT_NORMAL, _T("IO Write %x, %x\n"), a, d);
}

void __fastcall BrainZ801PortWrite(unsigned int a, unsigned char d)
{
	a &= 0xff;
	
	switch (a) {
		case 0x14:
		case 0x18: {
			System1SoundLatch = d;
			
			CloseCPU(0);
			OpenCPU(1);
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 1);
			Z80Execute(0);
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 0);
			Z80Execute(0);
			CloseCPU(1);
			OpenCPU(0);
			return;
		}
		
		case 0x15:		
		case 0x19: {
			System1VideoMode = d;
			System1FlipScreen = d & 0x80;
			
			System1RomBank = ((d & 0x04) >> 2) + ((d & 0x40) >> 5);
			return;
		}
	}
	
	bprintf(PRINT_NORMAL, _T("IO Write %x, %x\n"), a, d);
}

void __fastcall NoboranbZ801PortWrite(unsigned int a, unsigned char d)
{
	a &= 0xff;
	
	switch (a) {
		case 0x14:
		case 0x18: {
			System1SoundLatch = d;
			
			CloseCPU(0);
			OpenCPU(1);
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 1);
			Z80Execute(0);
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 0);
			Z80Execute(0);
			CloseCPU(1);
			OpenCPU(0);
			return;
		}
		
		case 0x15: {
			System1VideoMode = d;
			System1FlipScreen = d & 0x80;

			System1RomBank = ((d & 0x04) >> 2) + ((d & 0x40) >> 5);
			return;
		}
		
		case 0x16: {
			NoboranbInp16Step = d;
			return;
		}
		
		case 0x17: {
			NoboranbInp17Step = d;
			return;
		}
		
		case 0x24: {
			NoboranbInp23Step = d;
			return;
		}
	}
	
	bprintf(PRINT_NORMAL, _T("IO Write %x, %x\n"), a, d);
}

unsigned char __fastcall System1Z801ProgRead(unsigned int a)
{
	if (a <= 0x7fff) return System1Rom1[a];
	int BankAddress = (System1RomBank) ? (System1RomBank * 0x4000) + 0x10000 - 0x8000 + a : a;
	if (a >= 0x8000 && a <= 0xbfff) return System1Rom1[BankAddress];
	
	if (a >= 0xc000 && a <= 0xcfff) return System1Ram1[a - 0xc000];
	if (a >= 0xd000 && a <= 0xd1ff) return System1SpriteRam[a - 0xd000];
	if (a >= 0xd200 && a <= 0xd7ff) return System1Ram1[a - 0xd200 + 0x1000];
	if (a >= 0xd800 && a <= 0xddff) return System1PaletteRam[a - 0xd800];
	if (a >= 0xde00 && a <= 0xdfff) return System1deRam[a - 0xde00];
	if (a >= 0xe000 && a <= 0xe7ff) return System1BgRam[a - 0xe000];
	if (a >= 0xe800 && a <= 0xeeff) return System1VideoRam[a - 0xe800];
	if (a >= 0xef00 && a <= 0xefff) return System1efRam[a - 0xef00];
	if (a >= 0xf000 && a <= 0xf3ff) return System1BgCollisionRam[a - 0xf000];
	if (a >= 0xf400 && a <= 0xf7ff) return System1f4Ram[a - 0xf400];
	if (a >= 0xf800 && a <= 0xfbff) return System1SprCollisionRam[a - 0xf800];
	if (a >= 0xfc00 && a <= 0xffff) return System1fcRam[a - 0xfc00];
	
	bprintf(PRINT_NORMAL, _T("Prog Read %x\n"), a);
	return 0;
}

unsigned char __fastcall NoboranbZ801ProgRead(unsigned int a)
{
	if (a <= 0x7fff) return System1Rom1[a];
	int BankAddress = (System1RomBank) ? (System1RomBank * 0x4000) + 0x10000 - 0x8000 + a : a;
	if (a >= 0x8000 && a <= 0xbfff) return System1Rom1[BankAddress];
	
	if (a >= 0xc000 && a <= 0xc3ff) return System1BgCollisionRam[a - 0xc000];
	if (a >= 0xc400 && a <= 0xc7ff) return System1f4Ram[a - 0xc400];
	if (a >= 0xc800 && a <= 0xcbff) return System1SprCollisionRam[a - 0xc800];
	if (a >= 0xcc00 && a <= 0xcfff) return System1fcRam[a - 0xcc00];
	if (a >= 0xd000 && a <= 0xd1ff) return System1SpriteRam[a - 0xd000];
	if (a >= 0xd200 && a <= 0xd7ff) return System1Ram1[a - 0xd200 + 0x1000];
	if (a >= 0xd800 && a <= 0xddff) return System1PaletteRam[a - 0xd800];
	if (a >= 0xde00 && a <= 0xdfff) return System1deRam[a - 0xde00];
	if (a >= 0xe000 && a <= 0xe7ff) return System1BgRam[a - 0xe000];
	if (a >= 0xe800 && a <= 0xeeff) return System1VideoRam[a - 0xe800];
	if (a >= 0xef00 && a <= 0xefff) return System1efRam[a - 0xef00];
	if (a >= 0xf000 && a <= 0xffff) return System1Ram1[a - 0xf000];
	
	bprintf(PRINT_NORMAL, _T("Prog Read %x\n"), a);
	return 0;
}

void __fastcall System1Z801ProgWrite(unsigned int a, unsigned char d)
{
	if (a >= 0xc000 && a <= 0xcfff) {
		System1Ram1[a - 0xc000] = d;
		return;
	}
	
	if (a >= 0xd000 && a <= 0xd1ff) {
		System1SpriteRam[a - 0xd000] = d;
		return;
	}
	
	if (a >= 0xd200 && a <= 0xd7ff) {
		System1Ram1[a - 0xd200 + 0x1000] = d;
		return;
	}
	
	if (a >= 0xd800 && a <= 0xddff) {
		System1PaletteRam[a - 0xd800] = d;
		return;
	}
	
	if (a >= 0xde00 && a <= 0xdfff) {
		System1deRam[a - 0xde00] = d;
		return;
	}
	
	if (a >= 0xe000 && a <= 0xe7ff) {
		System1BgRam[a - 0xe000] = d;
		return;
	}
	
	if (a >= 0xe800 && a <= 0xeeff) {
		System1VideoRam[a - 0xe800] = d;
		return;
	}
	
	if (a >= 0xf000 && a <= 0xf3ff) {
		System1BgCollisionRam[a - 0xf000] = 0x7e;
		return;
	}
	
	if (a >= 0xf400 && a <= 0xf7ff) {
		System1f4Ram[a - 0xf400] = d;
		return;
	}
	
	if (a >= 0xf800 && a <= 0xfbff) {
		System1SprCollisionRam[a - 0xf800] = 0x7e;
		return;
	}
	
	if (a >= 0xfc00 && a <= 0xffff) {
		System1fcRam[a - 0xfc00] = d;
		return;
	}
	
	switch (a) {
		case 0xefbd: {
			System1ScrollY = d;
			break;
		}
		
		case 0xeffc: {
			System1ScrollX[0] = d;
			break;
		}
		
		case 0xeffd: {
			System1ScrollX[1] = d;
			break;
		}
	}
	
	if (a >= 0xef00 && a <= 0xefff) {
		System1efRam[a - 0xef00] = d;
		return;
	}
	
	bprintf(PRINT_NORMAL, _T("Prog Write %x, %x\n"), a, d);
}

void __fastcall NoboranbZ801ProgWrite(unsigned int a, unsigned char d)
{
	if (a >= 0xc000 && a <= 0xc3ff) {
		System1BgCollisionRam[a - 0xc000] = 0x7e;
		return;
	}
	
	if (a >= 0xc400 && a <= 0xc7ff) {
		System1f4Ram[a - 0xc400] = d;
		return;
	}
	
	if (a >= 0xc800 && a <= 0xcbff) {
		System1SprCollisionRam[a - 0xc800] = 0x7e;
		return;
	}
	
	if (a >= 0xcc00 && a <= 0xcfff) {
		System1fcRam[a - 0xcc00] = d;
		return;
	}	
	
	if (a >= 0xd000 && a <= 0xd1ff) {
		System1SpriteRam[a - 0xd000] = d;
		return;
	}
	
	if (a >= 0xd200 && a <= 0xd7ff) {
		System1Ram1[a - 0xd200 + 0x1000] = d;
		return;
	}
	
	if (a >= 0xd800 && a <= 0xddff) {
		System1PaletteRam[a - 0xd800] = d;
		return;
	}
	
	if (a >= 0xde00 && a <= 0xdfff) {
		System1deRam[a - 0xde00] = d;
		return;
	}
	
	if (a >= 0xe000 && a <= 0xe7ff) {
		System1BgRam[a - 0xe000] = d;
		return;
	}
	
	if (a >= 0xe800 && a <= 0xeeff) {
		System1VideoRam[a - 0xe800] = d;
		return;
	}
	
	if (a >= 0xf000 && a <= 0xffff) {
		System1Ram1[a - 0xf000] = d;
		return;
	}
	
	switch (a) {
		case 0xefbd: {
			System1ScrollY = d;
			break;
		}
		
		case 0xeffc: {
			System1ScrollX[0] = d;
			break;
		}
		
		case 0xeffd: {
			System1ScrollX[1] = d;
			break;
		}
	}
	
	if (a >= 0xef00 && a <= 0xefff) {
		System1efRam[a - 0xef00] = d;
		return;
	}
	
	bprintf(PRINT_NORMAL, _T("Prog Write %x, %x\n"), a, d);
}

unsigned char __fastcall System1Z801OpRead(unsigned int a)
{
	if (a <= 0x7fff) return System1Fetch1[a];
	int BankAddress = (System1RomBank) ? (System1RomBank * 0x4000) + 0x10000 - 0x8000 + a : a;
	if (a >= 0x8000 && a <= 0xbfff) return System1Fetch1[BankAddress];
	
	if (a >= 0xc000 && a <= 0xcfff) return System1Ram1[a - 0xc000];
	if (a >= 0xd000 && a <= 0xd1ff) return System1SpriteRam[a - 0xd000];
	if (a >= 0xd200 && a <= 0xd7ff) return System1Ram1[a - 0xd200 + 0x1000];
	if (a >= 0xd800 && a <= 0xddff) return System1PaletteRam[a - 0xd800];
	if (a >= 0xde00 && a <= 0xdfff) return System1deRam[a - 0xde00];
	if (a >= 0xe000 && a <= 0xe7ff) return System1BgRam[a - 0xe000];
	if (a >= 0xe800 && a <= 0xeeff) return System1VideoRam[a - 0xe800];
	if (a >= 0xef00 && a <= 0xefff) return System1efRam[a - 0xef00];
	if (a >= 0xf000 && a <= 0xf3ff) return System1BgCollisionRam[a - 0xf000];
	if (a >= 0xf400 && a <= 0xf7ff) return System1f4Ram[a - 0xf400];
	if (a >= 0xf800 && a <= 0xfbff) return System1SprCollisionRam[a - 0xf800];
	if (a >= 0xfc00 && a <= 0xffff) return System1fcRam[a - 0xfc00];
	
	bprintf(PRINT_NORMAL, _T("Op Read %x\n"), a);
	return 0;
}

unsigned char __fastcall System1Z801OpReadNotEnc(unsigned int a)
{
	if (a <= 0x7fff) return System1Rom1[a];
	int BankAddress = (System1RomBank) ? (System1RomBank * 0x4000) + 0x10000 - 0x8000 + a : a;
	if (a >= 0x8000 && a <= 0xbfff) return System1Rom1[BankAddress];
	
	if (a >= 0xc000 && a <= 0xcfff) return System1Ram1[a - 0xc000];
	if (a >= 0xd000 && a <= 0xd1ff) return System1SpriteRam[a - 0xd000];
	if (a >= 0xd200 && a <= 0xd7ff) return System1Ram1[a - 0xd200 + 0x1000];
	if (a >= 0xd800 && a <= 0xddff) return System1PaletteRam[a - 0xd800];
	if (a >= 0xde00 && a <= 0xdfff) return System1deRam[a - 0xde00];
	if (a >= 0xe000 && a <= 0xe7ff) return System1BgRam[a - 0xe000];
	if (a >= 0xe800 && a <= 0xeeff) return System1VideoRam[a - 0xe800];
	if (a >= 0xef00 && a <= 0xefff) return System1efRam[a - 0xef00];
	if (a >= 0xf000 && a <= 0xf3ff) return System1BgCollisionRam[a - 0xf000];
	if (a >= 0xf400 && a <= 0xf7ff) return System1f4Ram[a - 0xf400];
	if (a >= 0xf800 && a <= 0xfbff) return System1SprCollisionRam[a - 0xf800];
	if (a >= 0xfc00 && a <= 0xffff) return System1fcRam[a - 0xfc00];
	
	bprintf(PRINT_NORMAL, _T("Op Read %x\n"), a);
	return 0;
}

unsigned char __fastcall System1Z801OpArgRead(unsigned int a)
{
	if (a <= 0x7fff) return System1Rom1[a];
	int BankAddress = (System1RomBank) ? (System1RomBank * 0x4000) + 0x10000 - 0x8000 + a: a;
	if (a >= 0x8000 && a <= 0xbfff) return System1Rom1[BankAddress];
	
	if (a >= 0xc000 && a <= 0xcfff) return System1Ram1[a - 0xc000];
	if (a >= 0xd000 && a <= 0xd1ff) return System1SpriteRam[a - 0xd000];
	if (a >= 0xd200 && a <= 0xd7ff) return System1Ram1[a - 0xd200 + 0x1000];
	if (a >= 0xd800 && a <= 0xddff) return System1PaletteRam[a - 0xd800];
	if (a >= 0xde00 && a <= 0xdfff) return System1deRam[a - 0xde00];
	if (a >= 0xe000 && a <= 0xe7ff) return System1BgRam[a - 0xe000];
	if (a >= 0xe800 && a <= 0xeeff) return System1VideoRam[a - 0xe800];
	if (a >= 0xef00 && a <= 0xefff) return System1efRam[a - 0xef00];
	if (a >= 0xf000 && a <= 0xf3ff) return System1BgCollisionRam[a - 0xf000];
	if (a >= 0xf400 && a <= 0xf7ff) return System1f4Ram[a - 0xf400];
	if (a >= 0xf800 && a <= 0xfbff) return System1SprCollisionRam[a - 0xf800];
	if (a >= 0xfc00 && a <= 0xffff) return System1fcRam[a - 0xfc00];
	
	bprintf(PRINT_NORMAL, _T("Op Arg Read %x\n"), a);
	return 0;
}

unsigned char __fastcall System1Z802PortRead(unsigned int a)
{
	a &= 0xff;
	
	switch (a) {

	}	

	bprintf(PRINT_NORMAL, _T("Z80 2 IO Read %x\n"), a);
	return 0;
}

void __fastcall System1Z802PortWrite(unsigned int a, unsigned char d)
{
	a &= 0xff;
	
	switch (a) {
	}
	
	bprintf(PRINT_NORMAL, _T("Z80 2 IO Write %x, %x\n"), a, d);
}

unsigned char __fastcall System1Z802ProgRead(unsigned int a)
{
	if (a <= 0x7fff) return System1Rom2[a];
	
	if (a >= 0x8000 && a <= 0x87ff) return System1Ram2[a - 0x8000];
	
	switch (a) {
		case 0xe000: {
			return System1SoundLatch;
		}
	}
	
	bprintf(PRINT_NORMAL, _T("Z80 2 Prog Read %x\n"), a);
	return 0;
}

void __fastcall System1Z802ProgWrite(unsigned int a, unsigned char d)
{
	if (a < 0x8000) return;

	if (a >= 0x8000 && a <= 0x87ff) {
		System1Ram2[a - 0x8000] = d;
		return;
	}
	
	switch (a) {
		case 0xa000:
		case 0xa001:
		case 0xa002:
		case 0xa003: {
			SN76496Write(0, d);
			return;
		}
		
		case 0xc000:
		case 0xc001:
		case 0xc002:
		case 0xc003: {
			SN76496Write(1, d);
			return;
		}
	}
	
	bprintf(PRINT_NORMAL, _T("Z80 2 Prog Write %x, %x\n"), a, d);
}

unsigned char __fastcall System1Z802OpRead(unsigned int a)
{
	if (a <= 0x7fff) return System1Rom2[a];
	if (a >= 0x8000 && a <= 0x87ff) return System1Ram2[a - 0x8000];
	
	bprintf(PRINT_NORMAL, _T("Z80 2 Op Read %x\n"), a);
	return 0;
}

unsigned char __fastcall System1Z802OpArgRead(unsigned int a)
{
	if (a <= 0x7fff) return System1Rom2[a];
	if (a >= 0x8000 && a <= 0x87ff) return System1Ram2[a - 0x8000];
	
	bprintf(PRINT_NORMAL, _T("Z80 2 Op Arg Read %x\n"), a);
	return 0;
}

static void OpenCPU(int nCPU)
{
	switch (nCPU) {
		case 0: {
			Z80SetContext(&Z80_0);
			
			Z80SetIOReadHandler(Z80_0_Config.Z80In);
			Z80SetIOWriteHandler(Z80_0_Config.Z80Out);
			Z80SetProgramReadHandler(Z80_0_Config.Z80Read);
			Z80SetProgramWriteHandler(Z80_0_Config.Z80Write);
			Z80SetCPUOpArgReadHandler(Z80_0_Config.Z80ReadOpArg);
			Z80SetCPUOpReadHandler(Z80_0_Config.Z80ReadOp);
			return;
		}
		
		case 1: {
			Z80SetContext(&Z80_1);
			
			Z80SetIOReadHandler(Z80_1_Config.Z80In);
			Z80SetIOWriteHandler(Z80_1_Config.Z80Out);
			Z80SetProgramReadHandler(Z80_1_Config.Z80Read);
			Z80SetProgramWriteHandler(Z80_1_Config.Z80Write);
			Z80SetCPUOpArgReadHandler(Z80_1_Config.Z80ReadOpArg);
			Z80SetCPUOpReadHandler(Z80_1_Config.Z80ReadOp);
			return;
		}
	}
}

static void CloseCPU(int nCPU)
{
	switch (nCPU) {
		case 0: {
			Z80GetContext(&Z80_0);
			return;
		}
		
		case 1: {
			Z80GetContext(&Z80_1);
			return;
		}
	}
}

/*==============================================================================================
Driver Inits
===============================================================================================*/

static void CalcPenUsage()
{
	int i, x, y;
	UINT32 Usage;
	unsigned char *dp = NULL;
	
	for (i = 0; i < System1NumTiles; i++) {
		dp = System1Tiles + (i * 64);
		Usage = 0;
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				Usage |= 1 << dp[x];					
			}
			
			dp += 8;
		}
		
		System1TilesPenUsage[i] = Usage;
	}
}

static int TilePlaneOffsets[3]  = { 0, 0x20000, 0x40000 };
static int NoboranbTilePlaneOffsets[3]  = { 0, 0x40000, 0x80000 };
static int TileXOffsets[8]      = { 0, 1, 2, 3, 4, 5, 6, 7 };
static int TileYOffsets[8]      = { 0, 8, 16, 24, 32, 40, 48, 56 };

static int System1Init(int nZ80Rom1Num, int nZ80Rom1Size, int nZ80Rom2Num, int nZ80Rom2Size, int nTileRomNum, int nTileRomSize, int nSpriteRomNum, int nSpriteRomSize, bool bReset)
{
	int nRet = 0, nLen, i, RomOffset;
	
	System1NumTiles = (nTileRomNum * nTileRomSize) / 24;
	System1SpriteRomSize = nSpriteRomNum * nSpriteRomSize;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MemIndex();

	System1TempRom = (unsigned char*)malloc(0x18000);
		
	// Load Z80 #1 Program roms
	RomOffset = 0;
	for (i = 0; i < nZ80Rom1Num; i++) {
		nRet = BurnLoadRom(System1Rom1 + (i * nZ80Rom1Size), i + RomOffset, 1); if (nRet != 0) return 1;
	}
	
	if (System1BankedRom) {
		memcpy(System1TempRom, System1Rom1, 0x18000);
		memset(System1Rom1, 0, 0x18000);
		memcpy(System1Rom1 + 0x00000, System1TempRom + 0x00000, 0x8000);
		memcpy(System1Rom1 + 0x10000, System1TempRom + 0x08000, 0x8000);
		memcpy(System1Rom1 + 0x08000, System1TempRom + 0x08000, 0x8000);
		memcpy(System1Rom1 + 0x18000, System1TempRom + 0x10000, 0x8000);
	}
	
	if (DecodeFunction) DecodeFunction();
	
	// Load Z80 #2 Program roms
	RomOffset += nZ80Rom1Num;
	for (i = 0; i < nZ80Rom2Num; i++) {
		nRet = BurnLoadRom(System1Rom2 + (i * nZ80Rom2Size), i + RomOffset, 1); if (nRet != 0) return 1;
	}
	
	// Load and decode tiles
	memset(System1TempRom, 0, 0x18000);
	RomOffset += nZ80Rom2Num;
	for (i = 0; i < nTileRomNum; i++) {
		nRet = BurnLoadRom(System1TempRom + (i * nTileRomSize), i + RomOffset, 1);
	}
	if (System1NumTiles > 0x800) {
		GfxDecode(System1NumTiles, 3, 8, 8, NoboranbTilePlaneOffsets, TileXOffsets, TileYOffsets, 0x40, System1TempRom, System1Tiles);
	} else {
		GfxDecode(System1NumTiles, 3, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x40, System1TempRom, System1Tiles);
	}
	CalcPenUsage();
	free(System1TempRom);
	
	// Load Sprite roms
	RomOffset += nTileRomNum;
	for (i = 0; i < nSpriteRomNum; i++) {
		nRet = BurnLoadRom(System1Sprites + (i * nSpriteRomSize), i + RomOffset, 1);
	}
	
	// Load Colour proms
	if (System1ColourProms) {
		RomOffset += nSpriteRomNum;
		nRet = BurnLoadRom(System1PromRed, 0 + RomOffset, 1);
		nRet = BurnLoadRom(System1PromGreen, 1 + RomOffset, 1);
		nRet = BurnLoadRom(System1PromBlue, 2 + RomOffset, 1);
	}
	
	// Setup the Z80 emulation
	Z80Init();
	
	Z80_0_Config.Z80In = System1Z801PortRead;
	Z80_0_Config.Z80Out = System1Z801PortWrite;
	Z80_0_Config.Z80Read = System1Z801ProgRead;
	Z80_0_Config.Z80Write = System1Z801ProgWrite;
	Z80_0_Config.Z80ReadOpArg = System1Z801OpArgRead;
	if (DecodeFunction) {
		Z80_0_Config.Z80ReadOp = System1Z801OpRead;
	} else {
		Z80_0_Config.Z80ReadOp = System1Z801OpReadNotEnc;
	}
	
	Z80_1_Config.Z80In = System1Z802PortRead;
	Z80_1_Config.Z80Out = System1Z802PortWrite;
	Z80_1_Config.Z80Read = System1Z802ProgRead;
	Z80_1_Config.Z80Write = System1Z802ProgWrite;
	Z80_1_Config.Z80ReadOpArg = System1Z802OpArgRead;
	Z80_1_Config.Z80ReadOp = System1Z802OpRead;
	
	OpenCPU(0);
	CloseCPU(0);
	
	memset(SpriteOnScreenMap, 255, 256 * 256);
	
	System1SpriteXOffset = 1;
	
	nCyclesTotal[0] = 4000000 / 60;
	nCyclesTotal[1] = 4000000 / 60;

	SN76489AInit(0, 2000000, 0);
	SN76489AInit(1, 4000000, 1);
	
	GenericTilesInit();
	
	MakeInputsFunction = System1MakeInputs;
	
	// Reset the driver
	if (bReset) System1DoReset();
	
	return 0;
}

static int BlockgalInit()
{
	int nRet;
	
	System1MC8123Key = (unsigned char*)malloc(0x2000);
	BurnLoadRom(System1MC8123Key, 14, 1);

	DecodeFunction = blockgal_decode;
	
	nRet = System1Init(2, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
	free(System1MC8123Key);
	
	Z80_0_Config.Z80In = BlockgalZ801PortRead;
	MakeInputsFunction = BlockgalMakeInputs;

	return nRet;
}

static int BrainInit()
{
	int nRet;
	
	System1ColourProms = 1;
	System1BankedRom = 1;
	
	nRet = System1Init(3, 0x8000, 1, 0x8000, 3, 0x4000, 3, 0x8000, 0);
	
	Z80_0_Config.Z80Out = BrainZ801PortWrite;
	
	System1DoReset();
	
	return nRet;
}

static int FlickyInit()
{
	DecodeFunction = flicky_decode;
	
	return System1Init(2, 0x4000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int Flicks1Init()
{
	DecodeFunction = flicky_decode;
	
	return System1Init(4, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int Flicks2Init()
{
	return System1Init(2, 0x4000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int GardiaInit()
{
	int nRet;
	
	System1ColourProms = 1;
	System1BankedRom = 1;
	
	DecodeFunction = gardia_decode;
	
	nRet = System1Init(3, 0x8000, 1, 0x4000, 3, 0x4000, 4, 0x8000, 0);
	
	Z80_0_Config.Z80Out = BrainZ801PortWrite;
		
	System1DoReset();
	
	return nRet;
}

static int MrvikingInit()
{
	DecodeFunction = mrviking_decode;
	
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int MyheroInit()
{
	return System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int NoboranbInit()
{
	int nRet;
	
	System1ColourProms = 1;
	System1BankedRom = 1;
	
	nRet = System1Init(3, 0x8000, 1, 0x4000, 3, 0x8000, 4, 0x8000, 0);
	nCyclesTotal[0] = 8000000 / 60;
	System1Rom2[0x02f9] = 0x28;
	
	Z80_0_Config.Z80In = NoboranbZ801PortRead;
	Z80_0_Config.Z80Out = NoboranbZ801PortWrite;
	Z80_0_Config.Z80Read = NoboranbZ801ProgRead;
	Z80_0_Config.Z80Write = NoboranbZ801ProgWrite;
	Z80_0_Config.Z80ReadOpArg = NoboranbZ801ProgRead;
	Z80_0_Config.Z80ReadOp = NoboranbZ801ProgRead;
	
	System1DoReset();
	
	return nRet;
}

static int Pitfall2Init()
{
	int nRet;
	
	DecodeFunction = pitfall2_decode;
	
	nRet = System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
	nCyclesTotal[0] = 3600000 / 60;
	
	return nRet;
}

static int PitfalluInit()
{
	int nRet;
	
	nRet = System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
	nCyclesTotal[0] = 3600000 / 60;
	
	return nRet;
}

static int RegulusInit()
{
	DecodeFunction = regulus_decode;
	
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int RegulusuInit()
{
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int SeganinjInit()
{
	DecodeFunction = seganinj_decode;

	return System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int SeganinuInit()
{
	return System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int NprincsuInit()
{
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int StarjackInit()
{
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int SwatInit()
{
	DecodeFunction = swat_decode;
	
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int TeddybbInit()
{
	DecodeFunction = teddybb_decode;

	return System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int UpndownInit()
{
	DecodeFunction = nprinces_decode;
	
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int UpndownuInit()
{
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int WboyInit()
{
	DecodeFunction = astrofl_decode;
	
	return System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int WboyoInit()
{
	DecodeFunction = hvymetal_decode;
	
	return System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int Wboy2Init()
{
	DecodeFunction = wboy2_decode;
	
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int Wboy2uInit()
{
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int Wboy4Init()
{
	DecodeFunction = fdwarrio_decode;
	
	return System1Init(2, 0x8000, 1, 0x8000, 3, 0x4000, 2, 0x8000, 1);
}

static int WboyuInit()
{
	return System1Init(3, 0x4000, 1, 0x2000, 6, 0x2000, 4, 0x4000, 1);
}

static int WmatchInit()
{
	DecodeFunction = wmatch_decode;
	
	return System1Init(6, 0x2000, 1, 0x2000, 6, 0x2000, 2, 0x4000, 1);
}

static int System1Exit()
{
	Z80Exit();
	
	SN76496Exit();

	GenericTilesExit();
	
	free(Mem);
	Mem = NULL;
	
	System1SoundLatch = 0;
	System1ScrollX[0] = System1ScrollX[1] = System1ScrollY = 0;
	System1BgScrollX = 0;
	System1BgScrollY = 0;
	System1VideoMode = 0;
	System1FlipScreen = 0;
	System1RomBank = 0;
	NoboranbInp16Step = 0;
	NoboranbInp17Step = 0;
	NoboranbInp23Step = 0;
	BlockgalDial1 = 0;
	BlockgalDial2 = 0;
	
	System1SpriteRomSize = 0;
	System1NumTiles = 0;
	System1SpriteXOffset = 0;
	System1ColourProms = 0;
	System1BankedRom = 0;
	
	DecodeFunction = NULL;
	MakeInputsFunction = NULL;
	
	return 0;
}

/*==============================================================================================
Graphics Rendering
===============================================================================================*/

static void DrawPixel(int x, int y, int SpriteNum, int Colour)
{
	int xr, yr, SpriteOnScreen, dx, dy;
	
	dx = x;
	dy = y;
	if (nScreenWidth == 240) dx -= 8;
	
	if (x < 0 || x > 255  || y < 0 || y > 255) return;
	
	if (SpriteOnScreenMap[(y * 256) + x] != 255) {
		SpriteOnScreen = SpriteOnScreenMap[(y * 256) + x];
		System1SprCollisionRam[SpriteOnScreen + (32 * SpriteNum)] = 0xff;
	}
	
	SpriteOnScreenMap[(y * 256) + x] = SpriteNum;
	
	if (dx >= 0 && dx < nScreenWidth && dy >= 0 && dy < nScreenHeight) {
		unsigned short *pPixel = pTransDraw + (dy * nScreenWidth);
		pPixel[dx] = Colour;
	}
	
	xr = ((x - System1BgScrollX) & 0xff) / 8;
	yr = ((y - System1BgScrollY) & 0xff) / 8;
	
	if (System1BgRam[2 * (32 * yr + xr) + 1] & 0x10) {
		System1BgCollisionRam[0x20 + SpriteNum] = 0xff;
	}
}

static void DrawSprite(int Num)
{
	int Src, Bank, Height, sy, Row;
	INT16 Skip;
	unsigned char *SpriteBase;
	unsigned int *SpritePalette;
	
	SpriteBase = System1SpriteRam + (0x10 * Num);
	Src = (SpriteBase[7] << 8) | SpriteBase[6];
	Bank = 0x8000 * (((SpriteBase[3] & 0x80) >> 7) + ((SpriteBase[3] & 0x40) >> 5));
	Bank &= (System1SpriteRomSize - 1);
	Skip = (SpriteBase[5] << 8) | SpriteBase[4];
	
	Height = SpriteBase[1] - SpriteBase[0];
	SpritePalette = System1Palette + (0x10 * Num);
	
	sy = SpriteBase[0] + 1;
	
	for (Row = 0; Row < Height; Row++) {
		int x, y, Src2;
		
		Src = Src2 = Src + Skip;
		x = ((SpriteBase[3] & 0x01) << 8) + SpriteBase[2] + System1SpriteXOffset;
		y = sy + Row;
		
		x /= 2;
		
		while(1) {
			int Colour1, Colour2, Data;
			
			Data = System1Sprites[Bank + (Src2 & 0x7fff)];
			
			if (Src & 0x8000) {
				Src2--;
				Colour1 = Data & 0x0f;
				Colour2 = Data >> 4;
			} else {
				Src2++;
				Colour1 = Data >> 4;
				Colour2 = Data & 0x0f;
			}
			
			if (Colour1 == 0x0f) break;
			if (Colour1) DrawPixel(x, y, Num, Colour1 + (0x10 * Num));
			x++;
			
			if (Colour2 == 0x0f) break;
			if (Colour2) DrawPixel(x, y, Num, Colour2 + (0x10 * Num));
			x++;
		}
	}
}

static void System1DrawSprites()
{
	int i, SpriteBottomY, SpriteTopY;
	unsigned char *SpriteBase;
	
	memset(SpriteOnScreenMap, 255, 256 * 256);
	
	for (i = 0; i < 32; i++) {
		SpriteBase = System1SpriteRam + (0x10 * i);
		SpriteTopY = SpriteBase[0];
		SpriteBottomY = SpriteBase[1];
		if (SpriteBottomY && (SpriteBottomY - SpriteTopY > 0)) {
			DrawSprite(i);
		}
	}
}

static void System1DrawBgLayer(int PriorityDraw)
{
	int Offs, sx, sy;
	
	System1BgScrollX = ((System1ScrollX[0] >> 1) + ((System1ScrollX[1] & 1) << 7) + 14) & 0xff;
	System1BgScrollY = (-System1ScrollY & 0xff);
	
	if (PriorityDraw == -1) {
		for (Offs = 0; Offs < 0x800; Offs += 2) {
			int Code, Colour;
		
			Code = (System1BgRam[Offs + 1] << 8) | System1BgRam[Offs + 0];
			Code = ((Code >> 4) & 0x800) | (Code & 0x7ff);
			Colour = ((Code >> 5) & 0x3f);
			
			sx = (Offs >> 1) % 32;
			sy = (Offs >> 1) / 32;
			
			sx = 8 * sx + System1BgScrollX;
			sy = 8 * sy + System1BgScrollY;
			
			if (nScreenWidth == 240) sx -= 8;
			
			Code &= (System1NumTiles - 1);
			
			Render8x8Tile_Clip(pTransDraw, Code, sx      , sy      , Colour, 3, 512 * 2, System1Tiles);
			Render8x8Tile_Clip(pTransDraw, Code, sx - 256, sy      , Colour, 3, 512 * 2, System1Tiles);
			Render8x8Tile_Clip(pTransDraw, Code, sx      , sy - 256, Colour, 3, 512 * 2, System1Tiles);
			Render8x8Tile_Clip(pTransDraw, Code, sx - 256, sy - 256, Colour, 3, 512 * 2, System1Tiles);
		}
	} else {
		PriorityDraw <<= 3;
		
		for (Offs = 0; Offs < 0x800; Offs += 2) {
			if ((System1BgRam[Offs + 1] & 0x08) == PriorityDraw) {
				int Code, Colour;
			
				Code = (System1BgRam[Offs + 1] << 8) | System1BgRam[Offs + 0];
				Code = ((Code >> 4) & 0x800) | (Code & 0x7ff);
				Colour = ((Code >> 5) & 0x3f);
								
				int ColourOffs = 0x40;
				if (Colour >= 0x10 && Colour <= 0x1f) ColourOffs += 0x10;
				if (Colour >= 0x20 && Colour <= 0x2f) ColourOffs += 0x20;
				if (Colour >= 0x30 && Colour <= 0x3f) ColourOffs += 0x30;
		
				sx = (Offs >> 1) % 32;
				sy = (Offs >> 1) / 32;
			
				sx = 8 * sx + System1BgScrollX;
				sy = 8 * sy + System1BgScrollY;
				
				if (nScreenWidth == 240) sx -= 8;
				
				Code &= (System1NumTiles - 1);
				
				Render8x8Tile_Mask_Clip(pTransDraw, Code, sx      , sy      , Colour, 3, 0, 512 * 2, System1Tiles);
				Render8x8Tile_Mask_Clip(pTransDraw, Code, sx - 256, sy      , Colour, 3, 0, 512 * 2, System1Tiles);
				Render8x8Tile_Mask_Clip(pTransDraw, Code, sx      , sy - 256, Colour, 3, 0, 512 * 2, System1Tiles);
				Render8x8Tile_Mask_Clip(pTransDraw, Code, sx - 256, sy - 256, Colour, 3, 0, 512 * 2, System1Tiles);
			}
		}
	}
}

static void System1DrawFgLayer(int PriorityDraw)
{
	int Offs, sx, sy;
	
	PriorityDraw <<= 3;
	
	for (Offs = 0; Offs < 0x700; Offs += 2) {
		int Code, Colour;
		
		if ((System1VideoRam[Offs + 1] & 0x08) == PriorityDraw) {
			Code = (System1VideoRam[Offs + 1] << 8) | System1VideoRam[Offs + 0];
			Code = ((Code >> 4) & 0x800) | (Code & 0x7ff);
			Colour = (Code >> 5) & 0x3f;
		
			sx = (Offs >> 1) % 32;
			sy = (Offs >> 1) / 32;
			
			sx *= 8;
			sy *= 8;
			
			if (nScreenWidth == 240) sx -= 8;
		
			Code %= System1NumTiles;
			Code &= (System1NumTiles - 1);
			
			if (System1TilesPenUsage[Code] & ~1) {
				Render8x8Tile_Mask_Clip(pTransDraw, Code, sx, sy, Colour, 3, 0, 512, System1Tiles);
			}
		}
	}
}

static inline unsigned char pal2bit(unsigned char bits)
{
	bits &= 3;
	return (bits << 6) | (bits << 4) | (bits << 2) | bits;
}

static inline unsigned char pal3bit(unsigned char bits)
{
	bits &= 7;
	return (bits << 5) | (bits << 2) | (bits >> 1);
}

inline static unsigned int CalcCol(unsigned short nColour)
{
	int r, g, b;

	r = pal3bit(nColour >> 0);
	g = pal3bit(nColour >> 3);
	b = pal2bit(nColour >> 6);

	return BurnHighCol(r, g, b, 0);
}

static int System1CalcPalette()
{
	if (System1ColourProms) {
		int i;
		for (i = 0; i < 0x600; i++) {
			int bit0, bit1, bit2, bit3, r, g, b, val;
		
			val = System1PromRed[System1PaletteRam[i]];
			bit0 = (val >> 0) & 0x01;
			bit1 = (val >> 1) & 0x01;
			bit2 = (val >> 2) & 0x01;
			bit3 = (val >> 3) & 0x01;
			r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		
			val = System1PromGreen[System1PaletteRam[i]];
			bit0 = (val >> 0) & 0x01;
			bit1 = (val >> 1) & 0x01;
			bit2 = (val >> 2) & 0x01;
			bit3 = (val >> 3) & 0x01;
			g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		
			val = System1PromBlue[System1PaletteRam[i]];
			bit0 = (val >> 0) & 0x01;
			bit1 = (val >> 1) & 0x01;
			bit2 = (val >> 2) & 0x01;
			bit3 = (val >> 3) & 0x01;
			b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	
			System1Palette[i] = BurnHighCol(r, g, b, 0);
		}
	} else {
		for (int i = 0; i < 0x600; i++) {
			System1Palette[i] = CalcCol(System1PaletteRam[i]);
		}
	}

	return 0;
}

static void System1Render()
{
	BurnTransferClear();
	System1CalcPalette();
	System1DrawBgLayer(-1);
	System1DrawFgLayer(0);
	System1DrawBgLayer(0);
	System1DrawSprites();
	System1DrawBgLayer(1);
	System1DrawFgLayer(1);
	if (System1VideoMode & 0x010) BurnTransferClear();
	BurnTransferCopy(System1Palette);
}


/*==============================================================================================
Frame functions
===============================================================================================*/

int System1Frame()
{
	int nInterleave = 10;
	int nSoundBufferPos = 0;
		
	if (System1Reset) System1DoReset();

	MakeInputsFunction();
	
	nCyclesDone[0] = nCyclesDone[1] = 0;
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;
		
		// Run Z80 #1
		nCurrentCPU = 0;
		OpenCPU(nCurrentCPU);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = Z80Execute(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		if (i == 9) {
			Z80SetIrqLine(0, 1);
			Z80Execute(0);
			Z80SetIrqLine(0, 0);
			Z80Execute(0);
		}
		CloseCPU(nCurrentCPU);
		
		nCurrentCPU = 1;
		OpenCPU(nCurrentCPU);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = Z80Execute(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		if (i == 2 || i == 4 || i == 6 || i == 8) {
			Z80SetIrqLine(0, 1);
			Z80Execute(0);
			Z80SetIrqLine(0, 0);
			Z80Execute(0);
		}
		CloseCPU(nCurrentCPU);
		
		int nSegmentLength = nBurnSoundLen / nInterleave;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
		SN76496Update(0, pSoundBuf, nSegmentLength);
		SN76496Update(1, pSoundBuf, nSegmentLength);
		nSoundBufferPos += nSegmentLength;
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			SN76496Update(0, pSoundBuf, nSegmentLength);
			SN76496Update(1, pSoundBuf, nSegmentLength);
		}
	}
	
	if (pBurnDraw) System1Render();
	
	return 0;
}

/*==============================================================================================
Scan Driver
===============================================================================================*/

static int System1Scan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin != NULL) {
		*pnMin = 0x029675;
	}
	
	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = RamStart;
		ba.nLen	  = RamEnd-RamStart;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
//		ZetScan(nAction);			// Scan Z80
		SN76496Scan(nAction, pnMin);

		// Scan critical driver variables
		SCAN_VAR(nCyclesDone);
		SCAN_VAR(nCyclesSegment);
		SCAN_VAR(System1Dip);
		SCAN_VAR(System1Input);
		SCAN_VAR(System1ScrollX);
		SCAN_VAR(System1ScrollY);
		SCAN_VAR(System1BgScrollX);
		SCAN_VAR(System1BgScrollY);
		SCAN_VAR(System1VideoMode);
		SCAN_VAR(System1FlipScreen);
		SCAN_VAR(System1SoundLatch);
		SCAN_VAR(System1RomBank);
		SCAN_VAR(NoboranbInp16Step);
		SCAN_VAR(NoboranbInp17Step);
		SCAN_VAR(NoboranbInp23Step);
	}
	
	if (nAction & ACB_WRITE) {
//		ZetOpen(0);
//		ZetMapArea(0x8000, 0xbfff, 0, System1Rom1 + 0x10000 + (System1RomBank * 0x4000));
//		ZetMapArea(0x8000, 0xbfff, 2, System1Rom1 + 0x10000 + (System1RomBank * 0x4000));
//		ZetClose();
	}
	
	return 0;
}

/*==============================================================================================
Driver defs
===============================================================================================*/

struct BurnDriver BurnDrvBlockgal = {
	"blockgal", NULL, NULL, "1987",
	"Block Gal (MC-8123B, 317-0029)\0", NULL, "Sega / Vic Tokai", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, BlockgalRomInfo, BlockgalRomName, BlockgalInputInfo, BlockgalDIPInfo,
	BlockgalInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 256, 3, 4
};

struct BurnDriver BurnDrvBrain = {
	"brain", NULL, NULL, "1986",
	"Brain\0", NULL, "Coreland / Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, BrainRomInfo, BrainRomName, MyheroInputInfo, BrainDIPInfo,
	BrainInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvFlicky = {
	"flicky", NULL, NULL, "1984",
	"Flicky (128k Version, System 2, 315-5051)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, FlickyRomInfo, FlickyRomName, FlickyInputInfo, FlickyDIPInfo,
	FlickyInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvFlicks1 = {
	"flicks1", "flicky", NULL, "1984",
	"Flicky (64k Version, System 1, 315-5051, set 2)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, Flicks1RomInfo, Flicks1RomName, FlickyInputInfo, FlickyDIPInfo,
	Flicks1Init, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvFlicks2 = {
	"flicks2", "flicky", NULL, "1984",
	"Flicky (128k Version, System 2, not encrypted)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, Flicks2RomInfo, Flicks2RomName, FlickyInputInfo, FlickyDIPInfo,
	Flicks2Init, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvFlickyo = {
	"flickyo", "flicky", NULL, "1984",
	"Flicky (64k Version, System 1, 315-5051, set 1)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, FlickyoRomInfo, FlickyoRomName, FlickyInputInfo, FlickyDIPInfo,
	Flicks1Init, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvGardia = {
	"gardia", NULL, NULL, "1986",
	"Gardia (317-0006)\0", NULL, "Sega / Coreland", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, GardiaRomInfo, GardiaRomName, MyheroInputInfo, GardiaDIPInfo,
	GardiaInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 256, 3, 4
};

struct BurnDriver BurnDrvMrviking = {
	"mrviking", NULL, NULL, "1984",
	"Mister Viking (315-5041)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, MrvikingRomInfo, MrvikingRomName, MyheroInputInfo, MrvikingDIPInfo,
	MrvikingInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvMrvikngj = {
	"mrvikngj", "mrviking", NULL, "1984",
	"Mister Viking (315-5041, Japan)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, MrvikngjRomInfo, MrvikngjRomName, MyheroInputInfo, MrvikingDIPInfo,
	MrvikingInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvMyhero = {
	"myhero", NULL, NULL, "1985",
	"My Hero (US, not encrypted)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, MyheroRomInfo, MyheroRomName, MyheroInputInfo, MyheroDIPInfo,
	MyheroInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvNoboranb = {
	"noboranb", NULL, NULL, "1986",
	"Noboranka (Japan)\0", NULL, "bootleg", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, NoboranbRomInfo, NoboranbRomName, MyheroInputInfo, NoboranbDIPInfo,
	NoboranbInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvPitfall2 = {
	"pitfall2", NULL, NULL, "1985",
	"Pitfall II (315-5093)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, Pitfall2RomInfo, Pitfall2RomName, MyheroInputInfo, Pitfall2DIPInfo,
	Pitfall2Init, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvPitfallu = {
	"pitfallu", "pitfall2", NULL, "1985",
	"Pitfall II (not encrypted)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, PitfalluRomInfo, PitfalluRomName, MyheroInputInfo, PitfalluDIPInfo,
	PitfalluInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvRegulus = {
	"regulus", NULL, NULL, "1983",
	"Regulus (315-5033, rev. A)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, RegulusRomInfo, RegulusRomName, MyheroInputInfo, RegulusDIPInfo,
	RegulusInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvReguluso = {
	"reguluso", "regulus", NULL, "1983",
	"Regulus (315-5033)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, RegulusoRomInfo, RegulusoRomName, MyheroInputInfo, RegulusoDIPInfo,
	RegulusInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvRegulusu = {
	"regulusu", "regulus", NULL, "1983",
	"Regulus (not encrypted)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, RegulusuRomInfo, RegulusuRomName, MyheroInputInfo, RegulusDIPInfo,
	RegulusuInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvSeganinj = {
	"seganinj", NULL, NULL, "1985",
	"Sega Ninja (315-5102)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, SeganinjRomInfo, SeganinjRomName, SeganinjInputInfo, SeganinjDIPInfo,
	SeganinjInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvSeganinu = {
	"seganinu", "seganinj", NULL, "1985",
	"Sega Ninja (not encrypted)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, SeganinuRomInfo, SeganinuRomName, SeganinjInputInfo, SeganinjDIPInfo,
	SeganinuInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvNprincsu = {
	"nprincsu", "seganinj", NULL, "1985",
	"Ninja Princess (64k Ver. not encrypted)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, NprincsuRomInfo, NprincsuRomName, SeganinjInputInfo, SeganinjDIPInfo,
	NprincsuInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvStarjack = {
	"starjack", NULL, NULL, "1983",
	"Star Jacker (Sega)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, StarjackRomInfo, StarjackRomName, MyheroInputInfo, StarjackDIPInfo,
	StarjackInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvStarjacs = {
	"starjacs", "starjack", NULL, "1983",
	"Star Jacker (Stern)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, StarjacsRomInfo, StarjacsRomName, MyheroInputInfo, StarjacsDIPInfo,
	StarjackInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};

struct BurnDriver BurnDrvSwat = {
	"swat", NULL, NULL, "1984",
	"SWAT (315-5048)\0", NULL, "Coreland / Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, SwatRomInfo, SwatRomName, MyheroInputInfo, SwatDIPInfo,
	SwatInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 256, 3, 4
};

struct BurnDriver BurnDrvTeddybb = {
	"teddybb", NULL, NULL, "1985",
	"TeddyBoy Blues (315-5115, New Ver.)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, TeddybbRomInfo, TeddybbRomName, MyheroInputInfo, TeddybbDIPInfo,
	TeddybbInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvTeddybbo = {
	"teddybbo", "teddybb", NULL, "1985",
	"TeddyBoy Blues (315-5115, Old Ver.)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, TeddybboRomInfo, TeddybboRomName, MyheroInputInfo, TeddybbDIPInfo,
	TeddybbInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvUpndown = {
	"upndown", NULL, NULL, "1983",
	"Up'n Down (315-5030)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, UpndownRomInfo, UpndownRomName, UpndownInputInfo, UpndownDIPInfo,
	UpndownInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 256, 3, 4
};

struct BurnDriver BurnDrvUpndownu = {
	"upndownu", "upndown", NULL, "1983",
	"Up'n Down (not encrypted)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, UpndownuRomInfo, UpndownuRomName, UpndownInputInfo, UpndownDIPInfo,
	UpndownuInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 256, 3, 4
};

struct BurnDriver BurnDrvWboy = {
	"wboy", NULL, NULL, "1986",
	"Wonder Boy (set 1, 315-5177)\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, WboyRomInfo, WboyRomName, WboyInputInfo, WboyDIPInfo,
	WboyInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWboyo = {
	"wboyo", "wboy", NULL, "1986",
	"Wonder Boy (set 1, 315-5135)\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, WboyoRomInfo, WboyoRomName, WboyInputInfo, WboyDIPInfo,
	WboyoInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWboy2 = {
	"wboy2", "wboy", NULL, "1986",
	"Wonder Boy (set 2, 315-5178)\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, Wboy2RomInfo, Wboy2RomName, WboyInputInfo, WboyDIPInfo,
	Wboy2Init, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWboy2u = {
	"wboy2u", "wboy", NULL, "1986",
	"Wonder Boy (set 2, not encrypted)\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, Wboy2uRomInfo, Wboy2uRomName, WboyInputInfo, WboyDIPInfo,
	Wboy2uInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWboy3 = {
	"wboy3", "wboy", NULL, "1986",
	"Wonder Boy (set 3, 315-5135)\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, Wboy3RomInfo, Wboy3RomName, WboyInputInfo, Wboy3DIPInfo,
	WboyoInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWboy4 = {
	"wboy4", "wboy", NULL, "1986",
	"Wonder Boy (set 4, 315-5162)\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, Wboy4RomInfo, Wboy4RomName, WboyInputInfo, WboyDIPInfo,
	Wboy4Init, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWboyu = {
	"wboyu", "wboy", NULL, "1986",
	"Wonder Boy (not encrypted)\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, WboyuRomInfo, WboyuRomName, WboyInputInfo, WboyuDIPInfo,
	WboyuInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWbdeluxe = {
	"wbdeluxe", "wboy", NULL, "1986",
	"Wonder Boy Deluxe\0", NULL, "Sega (Escape License)", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, WbdeluxeRomInfo, WbdeluxeRomName, WboyInputInfo, WbdeluxeDIPInfo,
	Wboy2uInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 256, 224, 4, 3
};

struct BurnDriver BurnDrvWmatch = {
	"wmatch", NULL, NULL, "1984",
	"Water Match (315-5064)\0", NULL, "Sega", "System 1",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_SEGA_SYSTEM1,
	NULL, WmatchRomInfo, WmatchRomName, WmatchInputInfo, WmatchDIPInfo,
	WmatchInit, System1Exit, System1Frame, NULL, System1Scan,
	NULL, 224, 240, 3, 4
};
