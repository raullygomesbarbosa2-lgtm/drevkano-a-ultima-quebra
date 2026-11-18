#include <genesis.h>

/* Boot ROM metadata for DREVKANO; SGDK emits the header/checksum at link time. */
__attribute__((externally_visible))
const ROMHeader rom_header = {
    "SEGA MEGA DRIVE ",
    "(C)DREVKANO 2026",
    "DREVKANO: A ULTIMA QUEBRA                      ",
    "DREVKANO: THE LAST BREAK                       ",
    "GM 00000001-00",
    0x0000,
    "J               ",
    0x00000000,
    0x000FFFFF,
    0x00FF0000,
    0x00FFFFFF,
    "  ",
    0x0000,
    0x00000000,
    0x00000000,
    "            ",
    "Jogo original em desenvolvimento SGDK 2026 ",
    "JUE             "
};
