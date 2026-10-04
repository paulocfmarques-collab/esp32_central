#include "LayoutDatabase.h"

#define C_GREEN      0x07E0
#define C_RED        0xF800
#define C_PURPLE     0x780F
#define C_BLUE       0x001F
#define C_ORANGE     0xFD20
#define C_MAGENTA    0xF81F
#define C_NAVY       0x000F
#define C_DARKCYAN   0x03EF
#define C_DARKGREY   0x7BEF
#define C_ROYALBLUE  0x435C
#define C_DARKGREEN  0x0320

Botao LayoutDatabase::botoesAcao[TOTAL_BOTOES] = {
    // ════════════════ MODO LOCAL (Índices 0 a 23) ════════════════
    {10,  95,  94, 26, "TIME",     "time",          C_DARKGREEN,    0}, 
    {113, 95,  94, 26, "DATE",     "date",          C_DARKGREEN,    0},
    {216, 95,  94, 26, "MAC",      "mac",           C_DARKGREEN,    0},
    {10,  130, 94, 26, "TEMP",     "temp",          C_DARKGREEN,    0},
    {113, 130, 94, 26, "CPU",      "cpu",           C_DARKGREEN,    0},
    {216, 130, 94, 26, "RAM",      "ram",           C_DARKGREEN,    0},

    {10,  95,  94, 26, "FLASH",    "flash",         C_DARKGREEN,    1}, 
    {113, 95,  94, 26, "SCAN",     "scan",          C_DARKGREEN,    1},
    {216, 95,  94, 26, "UPTIME",   "uptime",        C_DARKGREEN,    1},
    {10,  130, 94, 26, "MAC",      "mac",           C_DARKGREEN,    1},
    {113, 130, 94, 26, "NET",      "net_info",      C_DARKGREEN,    1},
    {216, 130, 94, 26, "RST WIFI", "reset_wifi",    C_DARKGREEN,    1},

    {10,  95,  94, 26, "INFO",      "info",         C_DARKGREEN,    2}, 
    {113, 95,  94, 26, "VERSION",   "version",      C_DARKGREEN,    2},
    {216, 95,  94, 26, "BUILD",     "build",        C_DARKGREEN,    2},
    {10,  130, 94, 26, "STATUS",    "status",       C_DARKGREEN,    2},
    {113, 130, 94, 26, "LAST CMD",  "lastcmd",      C_DARKGREEN,    2},
    {216, 130, 94, 26, "CMD COUNT", "cmdcount",     C_DARKGREEN,    2},

    {10,  95,  94, 26, "REASON",    "reason",       C_DARKGREEN,    3}, 
    {113, 95,  94, 26, "HELP",      "help",         C_DARKGREEN,    3}, 
    {216, 95,  94, 26, "PSRAM",     "psram",        C_DARKGREEN,    3},
    {10,  130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,     3},
    {113, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,     3},
    {216, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,     3},

    // ════════════════ MODO REMOTO (Índices 24 a 47) ════════════════
    {10,  95,  94, 26, "LED ON",   "led_on",        C_DARKGREEN,    0}, 
    {113, 95,  94, 26, "LED OFF",  "led_off",       C_DARKGREEN,    0},
    {216, 95,  94, 26, "BLINK 1s", "led_blink:1000",C_DARKGREEN,    0},
    {10,  130, 94, 26, "TEMP",     "temp",          C_DARKGREEN,    0},
    {113, 130, 94, 26, "CPU",      "cpu",           C_DARKGREEN,    0},
    {216, 130, 94, 26, "RAM",      "ram",           C_DARKGREEN,    0},

    {10,  95,  94, 26, "SD LIST",  "list",          C_DARKGREEN,    1}, 
    {113, 95,  94, 26, "SD READ",  "read:log.txt",  C_DARKGREEN,    1},
    {216, 95,  94, 26, "SD DEL",   "del:log.txt",   C_DARKGREEN,    1},
    {10,  130, 94, 26, "PISCA",    "led_pisca:5:200",C_DARKGREEN,   1},
    {113, 130, 94, 26, "UPTIME",   "uptime",        C_DARKGREEN,    1},
    {216, 130, 94, 26, "NET",      "net_info",      C_DARKGREEN,    1},

    {10,  95,  94, 26, "INFO",      "info",         C_DARKGREEN,    2}, 
    {113, 95,  94, 26, "VERSION",   "version",      C_DARKGREEN,    2},
    {216, 95,  94, 26, "BUILD",     "build",        C_DARKGREEN,    2},
    {10,  130, 94, 26, "STATUS",    "status",       C_DARKGREEN,    2},
    {113, 130, 94, 26, "FLASH",     "flash",        C_DARKGREEN,    2},
    {216, 130, 94, 26, "REASON",    "reason",       C_DARKGREEN,    2},

    {10,  95,  94, 26, "HELP",      "help",         C_DARKGREEN,    3}, 
    {113, 95,  94, 26, "DST ON",    "dst_on",       C_DARKGREY,     3},
    {216, 95,  94, 26, "DST OFF",   "dst_off",      C_DARKGREY,     3},
    {10,  130, 94, 26, "TIME",      "time",         C_DARKGREY,     3},
    {113, 130, 94, 26, "DATE",      "date",         C_DARKGREY,     3},
    {216, 130, 94, 26, "PSRAM",     "psram",        C_DARKGREY,     3}
};

bool LayoutDatabase::statusEscravos[5] = {true, false, false, false, false};