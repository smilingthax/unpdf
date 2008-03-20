#ifndef _TABLES_H
#define _TABLES_H

// also fix opcode-encoding table!
#define EOL -2
#define FILL -3
#define EOL0 0 // not for decoding
#define EOL1 -1

#define OP_P    -5
#define OP_H    -6
#define OP_VR3  -7
#define OP_VR2  -8
#define OP_VR1  -9
#define OP_V   -10
#define OP_VL1 -11
#define OP_VL2 -12
#define OP_VL3 -13
#define OP_EXT -14

#define MAX_OP 15

// 6bit decoding Table for white huffmann codes
#define DECODE_COLORHUFF_BITS 6
unsigned short whitehufftable[1280]={
   320,  1152,   896,0x600d,   128,   832,   192,0x6001,0x600c,   448,  1024,   256,   704,    64,0x500a,0x500a,
0x500b,0x500b,   576,   384,   960,  1088,   512,0x60c0,0x6680,   640,  1216,   768,0x4002,0x4002,0x4002,0x4002,
0x4003,0x4003,0x4003,0x4003,0x5080,0x5080,0x5008,0x5008,0x5009,0x5009,0x6010,0x6011,0x4004,0x4004,0x4004,0x4004,
0x4005,0x4005,0x4005,0x4005,0x600e,0x600f,0x5040,0x5040,0x4006,0x4006,0x4006,0x4006,0x4007,0x4007,0x4007,0x4007,
0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,0x803f,
0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,0x8140,
0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,0x8180,
0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,
0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,0x7014,
0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,0x8021,
0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,0x8022,
0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,
0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,0x7013,
0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,0x801f,
0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,
0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,0x802b,
0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,0x802c,
0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,
0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,0x7015,
0xcffd,0xcffe,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1, // FILL, EOL
0xb700,0xb700,0xc7c0,0xc800,0xc840,0xc880,0xc8c0,0xc900,0xb740,0xb740,0xb780,0xb780,0xc940,0xc980,0xc9c0,0xca00,
0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,0x801d,
0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,0x801e,
0x95c0,0x95c0,0x95c0,0x95c0,0x95c0,0x95c0,0x95c0,0x95c0,0x9600,0x9600,0x9600,0x9600,0x9600,0x9600,0x9600,0x9600,
0x9640,0x9640,0x9640,0x9640,0x9640,0x9640,0x9640,0x9640,0x96c0,0x96c0,0x96c0,0x96c0,0x96c0,0x96c0,0x96c0,0x96c0,
0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,
0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,0x7012,
0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,0x8035,
0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,0x8036,
0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,
0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,0x701a,
0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,0x8037,
0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,0x8038,
0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,0x8039,
0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,0x803a,
0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,
0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,0x701b,
0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,0x803b,
0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,0x803c,
0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,0x81c0,
0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,0x8200,
0x92c0,0x92c0,0x92c0,0x92c0,0x92c0,0x92c0,0x92c0,0x92c0,0x9300,0x9300,0x9300,0x9300,0x9300,0x9300,0x9300,0x9300,
0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,0x8280,
0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,
0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,0x701c,
0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,0x803d,
0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,0x803e,
0x94c0,0x94c0,0x94c0,0x94c0,0x94c0,0x94c0,0x94c0,0x94c0,0x9500,0x9500,0x9500,0x9500,0x9500,0x9500,0x9500,0x9500,
0x9540,0x9540,0x9540,0x9540,0x9540,0x9540,0x9540,0x9540,0x9580,0x9580,0x9580,0x9580,0x9580,0x9580,0x9580,0x9580,
0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,
0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,0x7100,
0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,0x8023,
0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,0x8024,
0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,0x8025,
0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,0x8026,
0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,
0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,0x7017,
0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,0x802f,
0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,0x8030,
0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,
0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,0x7018,
0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,0x8031,
0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,0x8032,
0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,0x8027,
0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,0x8028,
0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,0x8029,
0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,0x802a,
0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,0x8033,
0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,0x8034,
0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,
0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,0x7019,
0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,0x802d,
0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,0x802e,
0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,
0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,0x7016,
0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,0x8240,
0x9340,0x9340,0x9340,0x9340,0x9340,0x9340,0x9340,0x9340,0x9380,0x9380,0x9380,0x9380,0x9380,0x9380,0x9380,0x9380,
0x93c0,0x93c0,0x93c0,0x93c0,0x93c0,0x93c0,0x93c0,0x93c0,0x9400,0x9400,0x9400,0x9400,0x9400,0x9400,0x9400,0x9400,
0x9440,0x9440,0x9440,0x9440,0x9440,0x9440,0x9440,0x9440,0x9480,0x9480,0x9480,0x9480,0x9480,0x9480,0x9480,0x9480};

// 6bit decoding Table for black huffmann codes
unsigned short blackhufftable[960]={
    64,   128,   512,   192,0x6009,0x6008,0x5007,0x5007,0x4006,0x4006,0x4006,0x4006,0x4005,0x4005,0x4005,0x4005,
0x3001,0x3001,0x3001,0x3001,0x3001,0x3001,0x3001,0x3001,0x3004,0x3004,0x3004,0x3004,0x3004,0x3004,0x3004,0x3004,
0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,0x2003,
0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,
0xcffd,0xcffe,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1, // FILL, EOL
0xb700,0xb700,0xc7c0,0xc800,0xc840,0xc880,0xc8c0,0xc900,0xb740,0xb740,0xb780,0xb780,0xc940,0xc980,0xc9c0,0xca00,
0xa012,0xa012,0xa012,0xa012,0xc034,   768,   640,0xc037,0xc038,   384,   448,0xc03b,0xc03c,   576,0xb018,0xb018,
0xb019,0xb019,   256,0xc140,0xc180,0xc1c0,   320,0xc035,0xc036,   832,   704,   896,0xa040,0xa040,0xa040,0xa040,
0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,0x800d,
0xb017,0xb017,0xc032,0xc033,0xc02c,0xc02d,0xc02e,0xc02f,0xc039,0xc03a,0xc03d,0xc100,0xa010,0xa010,0xa010,0xa010,
0xa011,0xa011,0xa011,0xa011,0xc030,0xc031,0xc03e,0xc03f,0xc01e,0xc01f,0xc020,0xc021,0xc028,0xc029,0xb016,0xb016,
0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,0x800e,
0x900f,0x900f,0x900f,0x900f,0x900f,0x900f,0x900f,0x900f,0xc080,0xc0c0,0xc01a,0xc01b,0xc01c,0xc01d,0xb013,0xb013,
0xb014,0xb014,0xc022,0xc023,0xc024,0xc025,0xc026,0xc027,0xb015,0xb015,0xc02a,0xc02b,0xa000,0xa000,0xa000,0xa000,
0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,
0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,0x700c,
0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,
0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,0xd680,
0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,
0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,0xd6c0,
0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,
0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,0xd200,
0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,
0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,0xd240,
0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,
0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,0xd500,
0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,
0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,0xd540,
0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,
0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,0xd580,
0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,
0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,0xd5c0,
0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,
0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,0x700a,
0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,
0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,0x700b,
0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,
0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,0xd600,
0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,
0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,0xd640,
0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,
0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,0xd300,
0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,
0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,0xd340,
0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,
0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,0xd400,
0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,
0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,0xd440,
0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,
0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,0xd280,
0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,
0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,0xd2c0,
0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,
0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,0xd380,
0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,
0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,0xd3c0,
0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,
0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,0xd480,
0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,
0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0,0xd4c0};

// 4bit decoding Table for Opcodes
#define DECODE_OPCODE_BITS 4
unsigned short opcodetable[48]={ // OP: 0x(width)+1+OP_C
    16,0x4ffb,0x3ffa,0x3ffa,0x3ff5,0x3ff5,0x3ff7,0x3ff7, // OP_P, OP_H, OP_H, OP_VL1, OP_VL1, OP_VR1, OP_VR1
0x1ff6,0x1ff6,0x1ff6,0x1ff6,0x1ff6,0x1ff6,0x1ff6,0x1ff6, // OP_V
    32,    -1,0x7ff2,0x7ff2,0x7ff3,0x7ff3,0x7ff9,0x7ff9, // OP_EXT, OP_EXT, OP_VL3, OP_VL3, OP_VL3, OP_VR3
0x6ff4,0x6ff4,0x6ff4,0x6ff4,0x6ff8,0x6ff8,0x6ff8,0x6ff8, // OP_VL2, OP_VR2
    -1,0xcffe,0xbffc,    -1,    -1,    -1,    -1,    -1, // EOL, SEOL
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1};

// Encoding
typedef struct {
  char len;
  unsigned short bits;
} ENCHUFF;

// whitehuff: 0..63,64,128,192,...,2560
ENCHUFF whitehuff[104]={
{ 8,0x3500},{ 6,0x1c00},{ 4,0x7000},{ 4,0x8000},{ 4,0xb000},{ 4,0xc000},{ 4,0xe000},{ 4,0xf000},
{ 5,0x9800},{ 5,0xa000},{ 5,0x3800},{ 5,0x4000},{ 6,0x2000},{ 6,0x0c00},{ 6,0xd000},{ 6,0xd400},
{ 6,0xa800},{ 6,0xac00},{ 7,0x4e00},{ 7,0x1800},{ 7,0x1000},{ 7,0x2e00},{ 7,0x0600},{ 7,0x0800},
{ 7,0x5000},{ 7,0x5600},{ 7,0x2600},{ 7,0x4800},{ 7,0x3000},{ 8,0x0200},{ 8,0x0300},{ 8,0x1a00},
{ 8,0x1b00},{ 8,0x1200},{ 8,0x1300},{ 8,0x1400},{ 8,0x1500},{ 8,0x1600},{ 8,0x1700},{ 8,0x2800},
{ 8,0x2900},{ 8,0x2a00},{ 8,0x2b00},{ 8,0x2c00},{ 8,0x2d00},{ 8,0x0400},{ 8,0x0500},{ 8,0x0a00},
{ 8,0x0b00},{ 8,0x5200},{ 8,0x5300},{ 8,0x5400},{ 8,0x5500},{ 8,0x2400},{ 8,0x2500},{ 8,0x5800},
{ 8,0x5900},{ 8,0x5a00},{ 8,0x5b00},{ 8,0x4a00},{ 8,0x4b00},{ 8,0x3200},{ 8,0x3300},{ 8,0x3400},

{ 5,0xd800},{ 5,0x9000},{ 6,0x5c00},{ 7,0x6e00},{ 8,0x3600},{ 8,0x3700},{ 8,0x6400},{ 8,0x6500},
{ 8,0x6800},{ 8,0x6700},{ 9,0x6600},{ 9,0x6680},{ 9,0x6900},{ 9,0x6980},{ 9,0x6a00},{ 9,0x6a80},
{ 9,0x6b00},{ 9,0x6b80},{ 9,0x6c00},{ 9,0x6c80},{ 9,0x6d00},{ 9,0x6d80},{ 9,0x4c00},{ 9,0x4c80},
{ 9,0x4d00},{ 6,0x6000},{ 9,0x4d80},{11,0x0100},{11,0x0180},{11,0x01a0},{12,0x0120},{12,0x0130},
{12,0x0140},{12,0x0150},{12,0x0160},{12,0x0170},{12,0x01c0},{12,0x01d0},{12,0x01e0},{12,0x01f0}};

// blackhuff: 0..63,64,128,192,...,2560
ENCHUFF blackhuff[104]={
{10,0x0dc0},{ 3,0x4000},{ 2,0xc000},{ 2,0x8000},{ 3,0x6000},{ 4,0x3000},{ 4,0x2000},{ 5,0x1800},
{ 6,0x1400},{ 6,0x1000},{ 7,0x0800},{ 7,0x0a00},{ 7,0x0e00},{ 8,0x0400},{ 8,0x0700},{ 9,0x0c00},
{10,0x05c0},{10,0x0600},{10,0x0200},{11,0x0ce0},{11,0x0d00},{11,0x0d80},{11,0x06e0},{11,0x0500},
{11,0x02e0},{11,0x0300},{12,0x0ca0},{12,0x0cb0},{12,0x0cc0},{12,0x0cd0},{12,0x0680},{12,0x0690},
{12,0x06a0},{12,0x06b0},{12,0x0d20},{12,0x0d30},{12,0x0d40},{12,0x0d50},{12,0x0d60},{12,0x0d70},
{12,0x06c0},{12,0x06d0},{12,0x0da0},{12,0x0db0},{12,0x0540},{12,0x0550},{12,0x0560},{12,0x0570},
{12,0x0640},{12,0x0650},{12,0x0520},{12,0x0530},{12,0x0240},{12,0x0370},{12,0x0380},{12,0x0270},
{12,0x0280},{12,0x0580},{12,0x0590},{12,0x02b0},{12,0x02c0},{12,0x05a0},{12,0x0660},{12,0x0670},

{10,0x03c0},{12,0x0c80},{12,0x0c90},{12,0x05b0},{12,0x0330},{12,0x0340},{12,0x0350},{13,0x0360},
{13,0x0368},{13,0x0250},{13,0x0258},{13,0x0260},{13,0x0268},{13,0x0390},{13,0x0398},{13,0x03a0},
{13,0x03a8},{13,0x03b0},{13,0x03b8},{13,0x0290},{13,0x0298},{13,0x02a0},{13,0x02a8},{13,0x02d0},
{13,0x02d8},{13,0x0320},{13,0x0328},{11,0x0100},{11,0x0180},{11,0x01a0},{12,0x0120},{12,0x0130},
{12,0x0140},{12,0x0150},{12,0x0160},{12,0x0170},{12,0x01c0},{12,0x01d0},{12,0x01e0},{12,0x01f0}};

// Opcodes with code OP_? at position -OP_? !
ENCHUFF opcode[MAX_OP]={
{13,0x0010}, // EOL0, encode only
{13,0x0018}, // EOL1, encode only
{12,0x0010}, // EOL
{12,0x0000}, // "FILL" just for completeness...
{-1,-1},
{ 4,0x1000}, // OP_P
{ 3,0x2000}, // OP_H
{ 7,0x0600}, // OP_VR3
{ 6,0x0c00}, // OP_VR2
{ 3,0x6000}, // OP_VR1
{ 1,0x8000}, // OP_V
{ 3,0x4000}, // OP_VL1
{ 6,0x0800}, // OP_VL2
{ 7,0x0400}, // OP_VL3
{ 7,0x0200}  // OP_EXT
};

// Table for RLE-encoding
int rlecode[128]={
0x00000008,0x00000017,0x00000116,0x00000026,0x00000215,0x00001115,0x00000125,0x00000035,
0x00000314,0x00001214,0x00011114,0x00002114,0x00000224,0x00001124,0x00000134,0x00000044,
0x00000413,0x00001313,0x00011213,0x00002213,0x00021113,0x00111113,0x00012113,0x00003113,
0x00000323,0x00001223,0x00011123,0x00002123,0x00000233,0x00001133,0x00000143,0x00000053,
0x00000512,0x00001412,0x00011312,0x00002312,0x00021212,0x00111212,0x00012212,0x00003212,
0x00031112,0x00121112,0x01111112,0x00211112,0x00022112,0x00112112,0x00013112,0x00004112,
0x00000422,0x00001322,0x00011222,0x00002222,0x00021122,0x00111122,0x00012122,0x00003122,
0x00000332,0x00001232,0x00011132,0x00002132,0x00000242,0x00001142,0x00000152,0x00000062,
0x00000611,0x00001511,0x00011411,0x00002411,0x00021311,0x00111311,0x00012311,0x00003311,
0x00031211,0x00121211,0x01111211,0x00211211,0x00022211,0x00112211,0x00013211,0x00004211,
0x00041111,0x00131111,0x01121111,0x00221111,0x02111111,0x11111111,0x01211111,0x00311111,
0x00032111,0x00122111,0x01112111,0x00212111,0x00023111,0x00113111,0x00014111,0x00005111,
0x00000521,0x00001421,0x00011321,0x00002321,0x00021221,0x00111221,0x00012221,0x00003221,
0x00031121,0x00121121,0x01111121,0x00211121,0x00022121,0x00112121,0x00013121,0x00004121,
0x00000431,0x00001331,0x00011231,0x00002231,0x00021131,0x00111131,0x00012131,0x00003131,
0x00000341,0x00001241,0x00011141,0x00002141,0x00000251,0x00001151,0x00000161,0x00000071};
#endif
