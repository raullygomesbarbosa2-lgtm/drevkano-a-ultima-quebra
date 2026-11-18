#include <genesis.h>

/* DREVKANO: A ÚLTIMA QUEBRA — vertical slice de beat 'em up para Mega Drive.
 * Desenho em tilemap para economizar sprites e respeitar o orçamento de hardware.
 * Direcional: mover; A: soco; B: chute; C: especial; Start: iniciar/pausar.
 */
#define MAX_ENEMIES 3
#define SCREEN_W 320
#define FLOOR_Y 184
#define PLAYER_W 16
#define PLAYER_H 32
#define BTN_ATTACK (BUTTON_A | BUTTON_B | BUTTON_C)

typedef enum { ST_TITLE, ST_PLAY, ST_CLEAR, ST_OVER } GameState;
typedef struct {
    s16 x, y, hp, cooldown, stun, active;
    u16 kind;
} Enemy;

typedef struct {
    s16 x, y, vy, hp, score, lives, facing;
    s16 attack, attack_timer, guard, jump, invuln;
} Player;

static GameState state = ST_TITLE;
static Player p;
static s16 abs16(s16 v) { return (v < 0) ? -v : v; }
static Enemy enemies[MAX_ENEMIES];
static u16 frameCounter, buttons, previousButtons;
static s16 oldPlayerX, oldPlayerY;
static s16 oldEnemyX[MAX_ENEMIES], oldEnemyY[MAX_ENEMIES];
static u8 bgTiles[40 * 28];
static u16 buildingBackground;
static s16 oldFxX, oldFxY;
static u16 oldFx;

static const u16 pal0[16] = {
    RGB24_TO_VDPCOLOR(0x090c20), RGB24_TO_VDPCOLOR(0x10182d), RGB24_TO_VDPCOLOR(0x17233c), RGB24_TO_VDPCOLOR(0x26334e),
    RGB24_TO_VDPCOLOR(0xf2c34e), RGB24_TO_VDPCOLOR(0xd84e36), RGB24_TO_VDPCOLOR(0xf2e2c4), RGB24_TO_VDPCOLOR(0x55c9a0),
    RGB24_TO_VDPCOLOR(0x5997e8), RGB24_TO_VDPCOLOR(0xb956be), RGB24_TO_VDPCOLOR(0xe9f5fa), RGB24_TO_VDPCOLOR(0x7a4636),
    RGB24_TO_VDPCOLOR(0x292e3c), RGB24_TO_VDPCOLOR(0x566071), RGB24_TO_VDPCOLOR(0xf07a4f), RGB24_TO_VDPCOLOR(0x000000)
};
static const u16 pal1[16] = {
    RGB24_TO_VDPCOLOR(0x090c20), RGB24_TO_VDPCOLOR(0xf07a4f), RGB24_TO_VDPCOLOR(0x10182d), RGB24_TO_VDPCOLOR(0x26334e),
    RGB24_TO_VDPCOLOR(0xf2c34e), RGB24_TO_VDPCOLOR(0xd84e36), RGB24_TO_VDPCOLOR(0xf2e2c4), RGB24_TO_VDPCOLOR(0x55c9a0),
    RGB24_TO_VDPCOLOR(0x5997e8), RGB24_TO_VDPCOLOR(0xb956be), RGB24_TO_VDPCOLOR(0xe9f5fa), RGB24_TO_VDPCOLOR(0x7a4636),
    RGB24_TO_VDPCOLOR(0x292e3c), RGB24_TO_VDPCOLOR(0x566071), RGB24_TO_VDPCOLOR(0xf07a4f), RGB24_TO_VDPCOLOR(0x000000)
};

static u16 tileId(u16 color) { return TILE_USER_INDEX + color; }
static void putTile(u16 x, u16 y, u16 tile, u16 pal) {
    if (buildingBackground && x < 40 && y < 28) bgTiles[y * 40 + x] = (u8)tile;
    VDP_setTileMapXY(VDP_BG_A, TILE_ATTR_FULL(pal, TRUE, FALSE, FALSE, tileId(tile)), x, y);
}
static void restoreBackground(u16 x, u16 y, u16 w, u16 h) {
    u16 xx, yy;
    for (yy = y; yy < y + h && yy < 28; yy++)
        for (xx = x; xx < x + w && xx < 40; xx++)
            putTile(xx, yy, bgTiles[yy * 40 + xx], PAL0);
}
static void paintRect(u16 x, u16 y, u16 w, u16 h, u16 tile, u16 pal) {
    u16 xx, yy;
    for (yy = 0; yy < h; yy++) for (xx = 0; xx < w; xx++) putTile(x + xx, y + yy, tile, pal);
}
static void tileSolid(u16 index, u8 color) {
    u32 data[8]; u16 i;
    u32 px = ((u32)color << 28) | ((u32)color << 24) | ((u32)color << 20) | ((u32)color << 16) |
             ((u32)color << 12) | ((u32)color << 8) | ((u32)color << 4) | color;
    for (i = 0; i < 8; i++) data[i] = px;
    VDP_loadTileData(data, tileId(index), 1, DMA);
}
static void setupTiles(void) {
    u16 i;
    for (i = 1; i <= 15; i++) tileSolid(i, (u8)i);
}
static void drawSkyline(void) {
    u16 x, h, bx;
    buildingBackground = 1;
    VDP_clearPlane(VDP_BG_A, TRUE);
    paintRect(0, 0, 40, 28, 1, PAL0);
    /* Fundo noturno e blocos de prédios com janelas em baixa contagem de tiles. */
    for (x = 0; x < 40; x += 4) {
        h = 5 + ((x * 7 + 3) % 8);
        bx = (x + 1) % 40;
        paintRect(bx, 14 - h, 3, h, 3, PAL0);
        paintRect(bx, 14 - h, 3, 1, 13, PAL0);
        if ((x % 8) == 0) paintRect(bx + 1, 15 - h, 1, h - 1, 4, PAL0);
    }
    /* Calçada, rua e faixa molhada de luz. */
    paintRect(0, 15, 40, 1, 12, PAL0);
    paintRect(0, 16, 40, 3, 2, PAL0);
    paintRect(0, 19, 40, 9, 1, PAL0);
    for (x = 1; x < 40; x += 8) paintRect(x, 22, 3, 1, 4, PAL0);
    /* Poças/neon e caixa de energia como obstáculos visuais na rua. */
    paintRect(9, 18, 3, 1, 9, PAL0);
    paintRect(25, 17, 2, 2, 11, PAL0);
    buildingBackground = 0;
}
static void drawActor(s16 x, s16 y, u16 body, u16 trim, u16 facing, u16 anim) {
    u16 tx = (u16)(x / 8), ty = (u16)(y / 8);
    /* Silhueta 2x4 em tiles; pequenas mudanças indicam passada/ataque. */
    paintRect(tx, ty, 2, 1, trim, PAL1);
    paintRect(tx, ty + 1, 2, 2, body, PAL1);
    paintRect(tx, ty + 3, 2, 1, 12, PAL0);
    if (facing) {
        putTile(tx + 1, ty, 10, PAL1);
        if (anim & 1) putTile(tx + 1, ty + 2, trim, PAL1);
    } else {
        putTile(tx, ty, 10, PAL1);
        if (anim & 1) putTile(tx, ty + 2, trim, PAL1);
    }
}
static void drawHud(void) {
    char s[24];
    VDP_drawText("DREVKANO  |  A ULTIMA QUEBRA", 1, 1);
    VDP_drawText("RUA DO CAIS - 01", 1, 2);
    sprintf(s, "VIDA %02d", p.hp); VDP_drawText(s, 27, 1);
    sprintf(s, "PONTOS %05d", p.score); VDP_drawText(s, 27, 2);
    VDP_drawText("A SOCO  B CHUTE  C ESPECIAL", 1, 27);
}
static void soundHit(u16 special) {
    PSG_setEnvelope(0, 0);
    PSG_setTone(0, special ? 260 : 460);
    PSG_setEnvelope(0, 12);
}
static void initGame(void) {
    u16 i;
    p.x = 54; p.y = FLOOR_Y; p.vy = 0; p.hp = 100; p.score = 0; p.lives = 3;
    p.facing = 1; p.attack = p.attack_timer = p.guard = p.jump = p.invuln = 0;
    frameCounter = 0; oldPlayerX = p.x; oldPlayerY = p.y;
    for (i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].x = (s16)(158 + i * 70); enemies[i].y = FLOOR_Y;
        enemies[i].hp = (i == 2) ? 3 : 2; enemies[i].cooldown = 30 + i * 13;
        enemies[i].stun = 0; enemies[i].active = 1; enemies[i].kind = i % 3;
        oldEnemyX[i] = enemies[i].x; oldEnemyY[i] = enemies[i].y;
    }
    drawSkyline();
}
static void startAttack(u16 type) {
    if (p.attack_timer == 0) {
        p.attack = type; p.attack_timer = (type == 3) ? 22 : 13;
        if (type == 3) { p.hp = (p.hp > 12) ? p.hp - 12 : p.hp; soundHit(1); }
        else soundHit(0);
    }
}
static void updatePlayer(void) {
    s16 dx = 0; u16 i, range, alive = 0;
    if (buttons & BUTTON_LEFT) { dx--; p.facing = 0; }
    if (buttons & BUTTON_RIGHT) { dx++; p.facing = 1; }
    if ((buttons & BUTTON_DOWN) && p.y < FLOOR_Y + 12) p.y += 1;
    if (p.guard) { /* guard reduz mobilidade; segurando C sem ataque */ dx = 0; }
    p.x += dx * ((buttons & BUTTON_A) ? 2 : 1);
    if (p.x < 16) p.x = 16;
    if (p.x > 292) p.x = 292;
    if ((buttons & BUTTON_C) && !(previousButtons & BUTTON_C)) {
        if ((buttons & BUTTON_A) && (buttons & BUTTON_B)) startAttack(3);
        else p.guard = 1;
    }
    if (!(buttons & BUTTON_C)) p.guard = 0;
    if ((buttons & BUTTON_A) && !(previousButtons & BUTTON_A)) startAttack(1);
    if ((buttons & BUTTON_B) && !(previousButtons & BUTTON_B)) startAttack(2);
    if ((buttons & BUTTON_UP) && !(previousButtons & BUTTON_UP) && !p.jump) { p.jump = 1; p.vy = -5; }
    if (p.jump) { p.y += p.vy; p.vy++; if (p.y >= FLOOR_Y) { p.y = FLOOR_Y; p.vy = 0; p.jump = 0; } }
    if (p.attack_timer) p.attack_timer--;
    if (p.invuln) p.invuln--;
    /* Combate com alcance e profundidade, sem sobreposição impossível de rua. */
    if (p.attack_timer > 4 && p.attack) {
        range = (p.attack == 3) ? 58 : (p.attack == 2 ? 42 : 30);
        for (i = 0; i < MAX_ENEMIES; i++) if (enemies[i].active && !enemies[i].stun) {
            s16 ex = enemies[i].x - p.x;
            if (abs16(ex) <= range && abs16(enemies[i].y - p.y) <= 18 && ((ex >= 0) == (p.facing != 0))) {
                enemies[i].hp--; enemies[i].stun = 24; enemies[i].x += p.facing ? 9 : -9;
                p.score += (p.attack == 3) ? 250 : 100;
                if (enemies[i].hp <= 0) { enemies[i].active = 0; p.score += 250; }
                p.attack_timer = 4;
            }
        }
    }
    for (i = 0; i < MAX_ENEMIES; i++) if (enemies[i].active) alive++;
    if (!alive) state = ST_CLEAR;
}
static void updateEnemies(void) {
    u16 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i]; s16 d;
        if (!e->active) continue;
        if (e->stun) { e->stun--; continue; }
        d = p.x - e->x;
        if (abs16(d) > 25) e->x += (d > 0) ? 1 : -1;
        if (abs16(e->y - p.y) > 10) e->y += (p.y > e->y) ? 1 : -1;
        if (e->cooldown) e->cooldown--;
        else if (abs16(d) < 24 && abs16(e->y - p.y) < 12) {
            e->cooldown = 48 + (i * 9);
            if (!p.invuln && !p.guard && !p.jump) {
                p.hp -= (e->kind == 2) ? 9 : 6; p.invuln = 28;
                if (p.hp <= 0) { p.hp = 0; state = ST_OVER; }
            }
        }
    }
}
static void renderGame(void) {
    u16 i;
    /* Restore previous actor/effect footprints to the original background tiles. */
    restoreBackground((u16)(oldPlayerX / 8), (u16)((oldPlayerY - 24) / 8), 2, 4);
    for (i = 0; i < MAX_ENEMIES; i++)
        restoreBackground((u16)(oldEnemyX[i] / 8), (u16)((oldEnemyY[i] - 24) / 8), 2, 4);
    if (oldFx) restoreBackground((u16)oldFxX, (u16)oldFxY, 2, 1);
    oldFx = 0;
    for (i = 0; i < MAX_ENEMIES; i++) if (enemies[i].active)
        drawActor(enemies[i].x, enemies[i].y - 24, 8 + enemies[i].kind, 5 + enemies[i].kind, (p.x > enemies[i].x), frameCounter >> 4);
    if (!p.invuln || (frameCounter & 2)) drawActor(p.x, p.y - 24, 5, 4, p.facing, frameCounter >> 3);
    oldPlayerX = p.x; oldPlayerY = p.y;
    for (i = 0; i < MAX_ENEMIES; i++) { oldEnemyX[i] = enemies[i].x; oldEnemyY[i] = enemies[i].y; }
    if (p.attack_timer > 4) {
        s16 fx = p.x + (p.facing ? 22 : -14);
        oldFxX = (s16)(fx / 8); oldFxY = (s16)((p.y - 16) / 8); oldFx = 1;
        paintRect((u16)oldFxX, (u16)oldFxY, (p.attack == 3) ? 2 : 1, 1, (p.attack == 3) ? 9 : 4, PAL0);
    }
    drawHud();
}
static void drawTitle(void) {
    drawSkyline();
    VDP_drawText("DREVKANO", 14, 7);
    VDP_drawText("A ULTIMA QUEBRA", 10, 9);
    VDP_drawText("UM BEAT 'EM UP ORIGINAL", 8, 14);
    VDP_drawText("DIRECIONAL: ANDAR / ESQUIVAR", 5, 18);
    VDP_drawText("A: SOCO   B: CHUTE", 9, 20);
    VDP_drawText("C: DEFESA   A+B+C: ESPECIAL", 5, 22);
    VDP_drawText("APERTE START", 12, 25);
}
int main(void) {
    JOY_init(); VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 32, TRUE); VDP_setPalette(PAL0, pal0); VDP_setPalette(PAL1, pal1);
    setupTiles(); state = ST_TITLE; drawTitle();
    { static GameState drawnEnd = ST_TITLE;
    while (1) {
        previousButtons = buttons; buttons = JOY_readJoypad(JOY_1); frameCounter++;
        if (state == ST_TITLE) {
            if (buttons & BUTTON_START) { initGame(); state = ST_PLAY; }
        } else if (state == ST_PLAY) {
            updatePlayer(); updateEnemies(); renderGame();
        } else {
            if (drawnEnd != state) {
                drawSkyline();
            if (state == ST_CLEAR) {
                VDP_drawText("FASE 1 CONCLUIDA!", 10, 10);
                VDP_drawText("A RUA DO CAIS ESTA LIVRE", 7, 13);
                VDP_drawText("START: JOGAR DE NOVO", 9, 19);
            } else {
                VDP_drawText("VOCE CAIU...", 12, 10);
                VDP_drawText("START: TENTAR DE NOVO", 9, 19);
            }
                drawnEnd = state;
            }
            if (buttons & BUTTON_START) { initGame(); state = ST_PLAY; }
        }
        VDP_waitVSync();
    }
    }
    return 0;
}
