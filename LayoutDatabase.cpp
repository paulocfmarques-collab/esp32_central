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
    {10,  130, 94, 26, "DESLIGAR",  "desligar",     C_DARKGREEN,    3},
    {113, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,     3},
    {216, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,     3},

    {10,  95, 94, 26, "SD STATUS",  "sd_status",    C_DARKGREEN,    4},
    {113,  95, 94, 26, "SD LISTA",  "sd_list",      C_DARKGREEN,    4},
    {216,  95, 94, 26, "SD LOG",    "sd_log",       C_DARKGREEN,    4},
    {10,  130, 94, 26, "SD TESTE",  "sd_test",      C_DARKGREEN,    4},
    {113,  130, 94, 26, "LIMPA LOG","sd_clear_log", C_DARKGREEN,    4},
    {216, 130, 94, 26, "[ VAGO ]",  "vago",         C_DARKGREY,     4},

    // ════════════════ MODO REMOTO (comandos comuns a todos os ESPs) ════════════════
    {10, 95, 94, 26, "LED ON", "led_on",            C_DARKGREEN,    0},
    {113, 95, 94, 26, "LED OFF", "led_off",         C_DARKGREEN,    0},
    {216, 95, 94, 26, "BLINK 1s", "led_blink:1000", C_DARKGREEN,    0},
    {10, 130, 94, 26, "TEMP", "temp",               C_DARKGREEN,    0},
    {113, 130, 94, 26, "CPU", "cpu",                C_DARKGREEN,    0},
    {216, 130, 94, 26, "RAM", "ram",                C_DARKGREEN,    0},

    {10, 95, 94, 26, "UPTIME", "uptime",            C_DARKGREEN,    1},
    {113, 95, 94, 26, "HEAP", "heap",               C_DARKGREEN,    1},
    {216, 95, 94, 26, "FLASH", "flash",             C_DARKGREEN,    1},
    {10, 130, 94, 26, "NET", "net_info",            C_DARKGREEN,    1},
    {113, 130, 94, 26, "MAC", "mac",                C_DARKGREEN,    1},
    {216, 130, 94, 26, "RSSI", "rssi",              C_DARKGREEN,    1},

    {10, 95, 94, 26, "INFO", "info",                C_DARKGREEN,    2},
    {113, 95, 94, 26, "STATUS", "status",           C_DARKGREEN,    2},
    {216, 95, 94, 26, "VERSION", "version",         C_DARKGREEN,    2},
    {10, 130, 94, 26, "BUILD", "build",             C_DARKGREEN,    2},
    {113, 130, 94, 26, "REASON", "reason",          C_DARKGREEN,    2},
    {216, 130, 94, 26, "HELP", "help",              C_DARKGREEN,    2},

    {10, 95, 94, 26, "TIME", "time",                C_DARKGREEN,    3},
    {113, 95, 94, 26, "DATE", "date",               C_DARKGREEN,    3},
    {216, 95, 94, 26, "ALIVE", "alive",             C_DARKGREEN,    3},
    {10, 130, 94, 26, "IP", "ip",                   C_DARKGREEN,    3},
    {113, 130, 94, 26, "REBOOT", "reboot",          C_DARKGREEN,    3},
    {216, 130, 94, 26, "RST WIFI", "reset_wifi",    C_DARKGREEN,    3},

    {10, 95, 94, 26, "SD STATUS", "sd_status",      C_DARKGREEN,    4},
    {113, 95, 94, 26, "SD LISTA", "sd_list",        C_DARKGREEN,    4},
    {216, 95, 94, 26, "SD LOG", "sd_log",           C_DARKGREEN,    4},
    {10, 130, 94, 26, "SD TESTE", "sd_test",        C_DARKGREEN,    4},
    {113, 130, 94, 26, "LIMPA LOG", "sd_clear_log", C_DARKGREEN,    4},
    {216, 130, 94, 26, "BREATH", "led_breath",      C_DARKGREEN,    4},

};

bool LayoutDatabase::statusEscravos[5] = {true, false, false, false, false};