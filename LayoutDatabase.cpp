#include "LayoutDatabase.h"

#define C_GREEN   0x07E0
#define C_RED     0xF800
#define C_PURPLE  0x780F
#define C_BLUE    0x001F
#define C_ORANGE  0xFD20
#define C_MAGENTA 0xF81F
#define C_NAVY    0x000F
#define C_DARKCYAN 0x03EF
#define C_DARKGREY 0x7BEF

Botao LayoutDatabase::botoesAcao[TOTAL_BOTOES] = {
    // ════════════════ MODO LOCAL (Índices 0 a 23) ════════════════
    {10,  95,  94, 26, "LED ON",   "led_on",        C_GREEN,   0}, 
    {113, 95,  94, 26, "LED OFF",  "led_off",       C_RED,     0},
    {216, 95,  94, 26, "BLINK 1s", "led_blink:1000",C_PURPLE,  0},
    {10,  130, 94, 26, "TEMP",     "temp",          C_BLUE,    0},
    {113, 130, 94, 26, "CPU",      "cpu",           C_ORANGE,  0},
    {216, 130, 94, 26, "RAM",      "ram",           0x51D0,    0},

    {10,  95,  94, 26, "FLASH",    "flash",         0x91a4,    1}, 
    {113, 95,  94, 26, "SCAN",     "scan",          0x7BEF,    1},
    {216, 95,  94, 26, "UPTIME",   "uptime",        0x03E0,    1},
    {10,  130, 94, 26, "MAC",      "mac",           0xB1DF,    1},
    {113, 130, 94, 26, "NET",      "net_info",      0x05FF,    1},
    {216, 130, 94, 26, "RST WIFI", "reset_wifi",    0xA000,    1},

    {10,  95,  94, 26, "INFO",      "info",         C_MAGENTA, 2}, 
    {113, 95,  94, 26, "VERSION",   "version",      C_NAVY,    2},
    {216, 95,  94, 26, "BUILD",     "build",        0x441F,    2},
    {10,  130, 94, 26, "STATUS",    "status",       0x134F,    2},
    {113, 130, 94, 26, "LAST CMD",  "lastcmd",      C_DARKCYAN,2},
    {216, 130, 94, 26, "CMD COUNT", "cmdcount",     0x73A0,    2},

    {10,  95,  94, 26, "REASON",    "reason",       0xD420,    3}, 
    {113, 95,  94, 26, "HELP",      "help",         C_BLUE,    3}, // Adicionado HELP aqui
    {216, 95,  94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3},
    {10,  130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3},
    {113, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3},
    {216, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3},

    // ════════════════ MODO REMOTO (Índices 24 a 47) ════════════════
    {10,  95,  94, 26, "LED ON",   "led_on",        C_GREEN,   0}, 
    {113, 95,  94, 26, "LED OFF",  "led_off",       C_RED,     0},
    {216, 95,  94, 26, "BLINK 1s", "led_blink:1000",C_PURPLE,  0},
    {10,  130, 94, 26, "TEMP",     "temp",          C_BLUE,    0},
    {113, 130, 94, 26, "CPU",      "cpu",           C_ORANGE,  0},
    {216, 130, 94, 26, "RAM",      "ram",           0x51D0,    0},

    {10,  95,  94, 26, "SD LIST",  "list",          C_BLUE,    1}, 
    {113, 95,  94, 26, "SD READ",  "read:log.txt",  C_NAVY,    1},
    {216, 95,  94, 26, "SD DEL",   "del:log.txt",   0x91a4,    1},
    {10,  130, 94, 26, "PISCA",    "led_pisca:5:200",C_ORANGE, 1},
    {113, 130, 94, 26, "UPTIME",   "uptime",        0x03E0,    1},
    {216, 130, 94, 26, "NET",      "net_info",      0x7800,    1},

    {10,  95,  94, 26, "INFO",      "info",         C_MAGENTA, 2}, 
    {113, 95,  94, 26, "VERSION",   "version",      C_NAVY,    2},
    {216, 95,  94, 26, "BUILD",     "build",        0x441F,    2},
    {10,  130, 94, 26, "STATUS",    "status",       0x134F,    2},
    {113, 130, 94, 26, "FLASH",     "flash",        C_DARKCYAN,2},
    {216, 130, 94, 26, "REASON",    "reason",       0x73A0,    2},

    {10,  95,  94, 26, "HELP",      "help",         0xD420,    3}, 
    {113, 95,  94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,    3},
    {216, 95,  94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3},
    {10,  130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3},
    {113, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3},
    {216, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,3}
};

bool LayoutDatabase::statusEscravos[5] = {true, false, false, false, false};