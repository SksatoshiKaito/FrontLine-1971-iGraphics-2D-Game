#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include "iGraphics.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define PI      ((float)M_PI)
#define D2R(d)  ((d)*PI/180.f)
#define R2D(r)  ((r)*180.f/PI)
#define CLAMP(v,lo,hi2) ((v)<(lo)?(lo):(v)>(hi2)?(hi2):(v))

/* screen */
int SW = 900;
int SH = 650;
#define HUD_H 96

/* world */
#define TILE  56
#define ROWS  56
#define COLS  66
#define WW    (COLS*TILE)
#define WH    (ROWS*TILE)

/* zoom */
float gZoom = 1.0f;
#define ZOOM_MIN  0.45f
#define ZOOM_MAX  2.0f
#define ZOOM_STEP 0.15f

#define ETILE ((int)(TILE*gZoom))

/* tile IDs */
enum {
	T_GRASS = 0, T_WALL = 1, T_FLOOR = 2, T_DOOR = 4,
	T_ROAD = 6, T_WIRE = 7, T_GATE = 10, T_VEHICLE = 17,
	T_TENT_IMG = 21, T_BLD_IMG = 22, T_COMP = 23
};

/* minimap */
#define MM_R   90
#define MM_S   0.030f
#define MM_CX  (SW - MM_R - 8)
#define MM_CY  (SH - MM_R - 8)

int gs = 0; // 0=Menu, 1=Play, 2=Fail, 3=Win, 4=Pause, 5=Next Level, 6=Comic, 7=Settings, 8=Instructions, 9=Credits, 10=Ending
int hoverMenu = -1;
int imgPage = -1;

int initAudioDone = 0;
int imgComic = -1;
int gunFireTimer = 0;
int isGunPlaying = 0;

/* ── MUSIC TOGGLE ── */
int musicOn = 1;

/* comic slideshow */
int imgSlide[4] = { -1, -1, -1, -1 };
int comicSlide = 0;
int comicTimer = 0;
int comicFromMenu = 0;
int comicHoverSkip = 0;
#define COMIC_AUTO_TICKS 437
#define COMIC_TOTAL_SLIDES 4

/* ── SPRITE IMAGES ── */
int imgH = -1;
int imgSH[4] = { -1, -1, -1, -1 };
int imgOH[2] = { -1, -1 };
int imgE = -1;
int imgBu = -1;
int imgHome = -1, imgTabu = -1, imgFloor = -1;

/* ── ENDING SCREEN ── */
int imgEnding = -1;
int endingTimer = 0;
#define ENDING_TICKS 438   /* 7 saniy (438 x 16ms ≈ 7s) */

/* ═══════════════════════════════════════════════ MAP 56x66 ═══════════════════════════════════════════════ */
static int MAP[ROWS][COLS] = { { 0 } };

void buildMap() {
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) MAP[r][c] = T_GRASS;
	}

	for (int c = 4; c <= 61; c++) { MAP[4][c] = T_WIRE; MAP[46][c] = T_WIRE; }
	for (int r = 4; r <= 46; r++) { MAP[r][4] = T_WIRE; MAP[r][61] = T_WIRE; }
	for (int c = 5; c <= 60; c++) { MAP[5][c] = T_WALL; MAP[45][c] = T_WALL; }
	for (int r = 5; r <= 45; r++) { MAP[r][5] = T_WALL; MAP[r][60] = T_WALL; }

	for (int c = 30; c <= 35; c++) {
		MAP[45][c] = T_GATE;
		MAP[4][c] = T_ROAD;
		MAP[46][c] = T_ROAD;
		for (int r = 47; r < ROWS; r++) MAP[r][c] = T_ROAD;
	}

	for (int k = 0; k <= 6; k++) {
		int r = 10 + k * 4;
		for (int c = 6; c <= 12; c++) MAP[r][c] = T_WALL;
	}
	for (int r = 11; r <= 33; r++) {
		if ((r - 10) % 4 != 0) { for (int c = 6; c <= 12; c++) MAP[r][c] = T_FLOOR; }
	}

	for (int r = 14; r <= 38; r++) MAP[r][17] = T_WALL;
	for (int k = 0; k <= 6; k++) {
		int r = 14 + k * 4;
		for (int c = 18; c <= 24; c++) MAP[r][c] = T_WALL;
	}
	for (int r = 15; r <= 37; r++) {
		if ((r - 14) % 4 != 0) { for (int c = 18; c <= 24; c++) MAP[r][c] = T_FLOOR; }
	}

	for (int k = 0; k < 7; k++) {
		int r_start = 14 + k * 4;
		for (int r = r_start; r < r_start + 2; r++) {
			for (int c = 48; c < 51; c++) MAP[r][c] = T_TENT_IMG;
		}
	}

	for (int r = 6; r <= 14; r++) {
		for (int c = 26; c <= 40; c++) MAP[r][c] = T_BLD_IMG;
	}

	for (int r = 6; r <= 12; r++) {
		for (int c = 15; c <= 22; c++) MAP[r][c] = T_ROAD;
	}

	for (int r = 6; r <= 11; r++) {
		for (int c = 46; c <= 58; c++) {
			if (r == 6 || r == 11 || c == 46 || c == 58) MAP[r][c] = T_WALL;
			else MAP[r][c] = T_FLOOR;
		}
	}
	MAP[11][50] = T_DOOR; MAP[11][51] = T_DOOR;
	MAP[8][52] = T_COMP;  MAP[8][53] = T_COMP;
}

/* helpers */
static float rWY(int r){ return (float)((ROWS - 1 - r)*TILE); }
static float cWX(int c){ return (float)(c*TILE + TILE / 2); }
static float mWY(int r){ return rWY(r) + (float)(TILE / 2); }

void txD(double x, double y, const char*s, void*f){ char b[256]; strncpy(b, s, 255); b[255] = 0; iText(x, y, b, f); }
float flen(float a, float b){ return sqrtf(a*a + b*b); }
float fdst(float ax, float ay, float bx, float by){ return flen(ax - bx, ay - by); }

int tW(float wx, float wy){
	int c = (int)(wx / TILE), wr = (int)(wy / TILE);
	if (c<0 || c >= COLS || wr<0 || wr >= ROWS)return T_WALL;
	return MAP[ROWS - 1 - wr][c];
}
int solid(int t){ return t == T_WALL || t == T_GATE || t == T_WIRE || t == T_TENT_IMG || t == T_BLD_IMG || t == T_COMP; }
int canWlk(float wx, float wy, float r){
	float cx[4] = { wx - r, wx + r, wx - r, wx + r }, cy[4] = { wy - r, wy - r, wy + r, wy + r };
	for (int i = 0; i<4; i++){
		if (cx[i]<1 || cx[i] >= WW - 1 || cy[i]<1 || cy[i] >= WH - 1)return 0;
		int t = tW(cx[i], cy[i]);
		if (solid(t))return 0;
	}
	return 1;
}

float camX = 0, camY = 0;
float sX(float wx){ return (wx - camX)*gZoom; }
float sY(float wy){ return (wy - camY)*gZoom; }

#define CI 0
#define CM 1
#define CA 2
#define CS 3
#define CF 4
#define CG 5
#define CP 6

#define MAX_PATH_PTS 100
typedef struct {
	float x[MAX_PATH_PTS];
	float y[MAX_PATH_PTS];
	int count;
	int cur;
} PathData;
PathData subPath[7];
int  drawingPath = 0;
int  drawPathSub = -1;
int  pathDrawTick = 0;

/* data */
#define NM 1
#define NS 4
#define NA 0
#define NH (NM+NS+NA)

typedef struct{
	float x, y, a, rad, spd; int hp, mhp, alive, snk, inR, iv;
	int role; char nm[14]; float cr, cg, cb;
	int sub, cmd, cc, sc; float dx, dy; int st; float lx, ly;
}Hero;
Hero H[NH];
int act = 0;

#define NV 3
typedef struct{ float x, y, a, spd, w, h; int tp, cr2, active; char lbl[12]; float r, g, b; }Veh;
Veh V[NV];

#define NE 100
typedef struct{ float x, y, a, sx, sy, ex, ey, spd, vd, va, hp; int alive, me, al, at, tp, sc; }Ene;
Ene E[NE];

#define NO 0
typedef struct{ float x, y; int done; char lbl[24]; }Obj;
Obj OBJ[1];
int oDone = 0;

#define NBL 80
typedef struct{ float x, y, vx, vy, lf; int active, en, tp; }Bul;
Bul BL[NBL];

#define NBM 8
typedef struct{ float x, y; int active, t; }Bom;
Bom BM[NBM];

#define NP 160
typedef struct{ float x, y, vx, vy, r, g, b; int life; }Par;
Par PR[NP];

int selW = 0, cO = 0, cPh = 0, cSub = -1;
char LOG[220] = "JAI BANGLA! Level shuru hoyeche!";
int alLv = 0, alT = 0, cHP = 100, cDead = 0, tick = 0;
int autoTriggered = 0;
int bldgSpawnCount = 0, bldgSpawnTimer = 0;
struct { float x, y; int spawned; } tentsData[7];

#define ZB_X 8
#define ZB_Y (SH-HUD_H-44)

void spP(float x, float y, float r, float g, float b, int n){
	for (int i = 0; i<NP&&n>0; i++)if (PR[i].life <= 0){ PR[i].x = x; PR[i].y = y; PR[i].vx = ((float)(rand() % 200) - 100) / 30.f; PR[i].vy = ((float)(rand() % 200) - 100) / 30.f; PR[i].r = r; PR[i].g = g; PR[i].b = b; PR[i].life = 26; n--; }
}

static void playGunSound() {
	gunFireTimer = 65;
	if (!isGunPlaying) {
		isGunPlaying = 1;
	}
	mciSendStringA("seek gun to start", NULL, 0, NULL);
	mciSendStringA("play gun", NULL, 0, NULL);
}

void fBul(float ox, float oy, float tx, float ty, int en, int tp){
	float dx = tx - ox, dy = ty - oy, d = flen(dx, dy); if (d<1)return;
	float angle = atan2f(dy, dx);
	float off = en ? 20.f : 24.f;
	if (off > d*0.5f) off = d*0.5f;
	ox += cosf(angle) * off;
	oy += sinf(angle) * off;
	angle += ((float)(rand() % 100) - 50.f) * 0.005f;
	dx = cosf(angle);
	dy = sinf(angle);
	float sp = en ? 12.f : (tp == 2 ? 30.f : 24.f);
	float lf = en ? 20.f : (tp == 2 ? 30.f : 15.f);

	playGunSound();

	for (int i = 0; i<NBL; i++)if (!BL[i].active){
		BL[i].x = ox; BL[i].y = oy;
		BL[i].vx = dx * sp;
		BL[i].vy = dy * sp;
		BL[i].active = 1; BL[i].en = en; BL[i].tp = tp; BL[i].lf = lf;
		break;
	}
}

void mvH(int hi, float dx, float dy){
	Hero*u = &H[hi]; if (!u->alive)return;
	if (u->iv >= 0){
		Veh*v = &V[u->iv]; float nx = v->x + dx*v->spd, ny = v->y + dy*v->spd;
		if (canWlk(nx, ny, v->w / 2.f)){ v->x = nx; v->y = ny; u->x = nx; u->y = ny; }
		if (dx || dy)v->a = R2D(atan2f(dy, dx)); return;
	}
	float wf = 1.f;
	float spd = u->snk ? u->spd*0.42f : u->spd*wf;
	float nx = u->x + dx*spd, ny = u->y + dy*spd;
	if (dx || dy)u->a = R2D(atan2f(dy, dx));

	if (canWlk(nx, ny, u->rad)){ u->x = nx; u->y = ny; }
	else if (canWlk(nx, u->y, u->rad))u->x = nx;
	else if (canWlk(u->x, ny, u->rad))u->y = ny;
}

void togV(int hi){
	Hero*u = &H[hi];
	if (u->iv >= 0){
		V[u->iv].cr2 = -1; u->iv = -1; u->x += 18; u->y += 18;
		strcpy_s(LOG, sizeof(LOG), "Gari theke namlam!"); return;
	}
	int best = -1; float bd = 40.f;
	for (int i = 0; i<NV; i++){
		if (!V[i].active)continue;
		float d = fdst(u->x, u->y, V[i].x, V[i].y);
		if (d<bd && V[i].cr2<0){ bd = d; best = i; }
	}
	if (best >= 0){ u->iv = best; V[best].cr2 = hi; u->x = V[best].x; u->y = V[best].y; strcpy_s(LOG, sizeof(LOG), "Gari start!"); }
	else strcpy_s(LOG, sizeof(LOG), "Kachhe kono gari nei!");
}

void ulkGt(int hi){
	Hero*u = &H[hi]; int f = 0;
	for (int dr = -2; dr <= 2 && !f; dr++)for (int dc = -2; dc <= 2 && !f; dc++){
		float wx = u->x + dc*TILE, wy = u->y + dr*TILE; int c = (int)(wx / TILE), wr = (int)(wy / TILE);
		if (c<0 || c >= COLS || wr<0 || wr >= ROWS)continue; int mr = ROWS - 1 - wr; if (mr<0 || mr >= ROWS)continue;
		if (MAP[mr][c] == T_GATE){ MAP[mr][c] = T_DOOR; spP(wx, wy, 255, 200, 80, 10); strcpy_s(LOG, sizeof(LOG), "Gate katlo!"); f = 1; }
	}
	if (!f)strcpy_s(LOG, sizeof(LOG), "Kachhe kono gate nei.");
}
void plBom(int hi){
	if (H[hi].role != 3 && H[hi].role != 5 && H[hi].role != 0){ strcpy_s(LOG, sizeof(LOG), "Apni bomb dite parben na!"); return; }
	for (int i = 0; i<NBM; i++)if (!BM[i].active){ BM[i].x = H[hi].x; BM[i].y = H[hi].y; BM[i].t = 300; BM[i].active = 1; strcpy_s(LOG, sizeof(LOG), "Bomb lagano!"); return; }
	strcpy_s(LOG, sizeof(LOG), "Ar bomb nei!");
}
void detAll(){ int a = 0; for (int i = 0; i<NBM; i++)if (BM[i].active){ BM[i].t = 1; a = 1; }strcpy_s(LOG, sizeof(LOG), a ? "BOOM! Sab bomb fotao!" : "Kono bomb nei."); }

static void snav(int hi, float gx, float gy){
	Hero*u = &H[hi];
	float dx = gx - u->x, dy = gy - u->y, d = flen(dx, dy);
	if (d<14.f)return;
	dx /= d; dy /= d;
	float ba = atan2f(dy, dx);
	float an[5] = { 0, D2R(35.f), D2R(-35.f), D2R(70.f), D2R(-70.f) };
	float bx2 = 0, by2 = 0; int moved = 0;
	for (int k = 0; k<5; k++){
		float ca = ba + an[k];
		float nx = u->x + cosf(ca)*u->spd;
		float ny = u->y + sinf(ca)*u->spd;
		if (canWlk(nx, ny, u->rad)){ bx2 = nx; by2 = ny; moved = 1; break; }
	}
	if (moved){ u->a = R2D(atan2f(by2 - u->y, bx2 - u->x)); u->x = bx2; u->y = by2; }
	u->st++;
	if (u->st>50){
		if (flen(u->x - u->lx, u->y - u->ly)<8.f){
			float ra = ((float)(rand() % 628)) / 100.f;
			float ex2 = u->x + cosf(ra)*u->spd*3.f;
			float ey2 = u->y + sinf(ra)*u->spd*3.f;
			if (canWlk(ex2, ey2, u->rad)){ u->x = ex2; u->y = ey2; }
		}
		u->st = 0; u->lx = u->x; u->ly = u->y;
	}
	int ne = 0;
	for (int j = 0; j<NE; j++){
		if (!E[j].alive)continue;
		if (fdst(u->x, u->y, E[j].x, E[j].y)<200.f){ ne = 1; break; }
	}
	u->snk = ne;
}

int hasLOS(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1, dy = y2 - y1;
	float dist = flen(dx, dy);
	if (dist < 1) return 1;
	dx /= dist; dy /= dist;
	for (float r = 10.0f; r < dist; r += 8.0f) {
		float cx = x1 + dx * r;
		float cy = y1 + dy * r;
		int t = tW(cx, cy);
		if (solid(t)) return 0;
	}
	return 1;
}

#define Q_MAX 4000
int bfsPath(float sx, float sy, float ex, float ey, PathData* outPath) {
	int sc = (int)(sx / TILE), sr = ROWS - 1 - (int)(sy / TILE);
	int ec = (int)(ex / TILE), er = ROWS - 1 - (int)(ey / TILE);
	if (sc == ec && sr == er) return 0;
	if (sc<0 || sc >= COLS || sr<0 || sr >= ROWS) return 0;
	if (ec<0 || ec >= COLS || er<0 || er >= ROWS) return 0;

	int dist[ROWS][COLS];
	int par[ROWS][COLS];
	for (int i = 0; i<ROWS; i++) for (int j = 0; j<COLS; j++) dist[i][j] = -1;

	int q[Q_MAX]; int qh = 0, qt = 0;
	q[qt++] = sr*COLS + sc;
	dist[sr][sc] = 0;

	int dr[] = { -1, 1, 0, 0 };
	int dc[] = { 0, 0, -1, 1 };

	while (qh<qt) {
		int u = q[qh++];
		int r = u / COLS, c = u%COLS;
		if (r == er && c == ec) break;

		for (int i = 0; i<4; i++) {
			int nr = r + dr[i], nc = c + dc[i];
			if (nr >= 0 && nr<ROWS&&nc >= 0 && nc<COLS) {
				if (dist[nr][nc] == -1) {
					int t = MAP[nr][nc];
					if (t == T_WALL || t == T_WIRE || t == T_BLD_IMG || t == T_COMP || t == T_TENT_IMG) continue;
					dist[nr][nc] = dist[r][c] + 1;
					par[nr][nc] = u;
					q[qt++] = nr*COLS + nc;
				}
			}
		}
	}

	if (dist[er][ec] == -1) return 0;

	int cur = er*COLS + ec;
	int pathTmp[120]; int pc = 0;
	while (cur != sr*COLS + sc) {
		if (pc >= 119) break;
		pathTmp[pc++] = cur;
		cur = par[cur / COLS][cur%COLS];
	}
	outPath->count = 0;
	for (int i = pc - 1; i >= 0; i -= 2) {
		int r = pathTmp[i] / COLS, c = pathTmp[i] % COLS;
		outPath->x[outPath->count] = cWX(c);
		outPath->y[outPath->count] = mWY(r);
		outPath->count++;
		if (outPath->count >= MAX_PATH_PTS - 1) break;
	}
	outPath->cur = 0;
	return 1;
}

void updS(int hi) {
	Hero* u = &H[hi];
	if (!u->alive) return;

	if (u->cc > 0) u->cc--;
	if (u->sc > 0) u->sc--;
	int si = hi - NM;

	if (u->cmd == CF) {
		float fx = H[act].x + (si - 1.5f)*70.f;
		float fy = H[act].y - 80.f;
		snav(hi, fx, fy);
		int bi = -1; float bd = 500.f;
		for (int j = 0; j<NE; j++) {
			if (E[j].alive && fdst(u->x, u->y, E[j].x, E[j].y) < bd && hasLOS(u->x, u->y, E[j].x, E[j].y)) {
				bd = fdst(u->x, u->y, E[j].x, E[j].y); bi = j;
			}
		}
		if (bi >= 0 && u->sc == 0) {
			u->a = R2D(atan2f(E[bi].y - u->y, E[bi].x - u->x));
			fBul(u->x, u->y, E[bi].x, E[bi].y, 0, 0);
			u->sc = 8;
		}
		if (u->hp < u->mhp * 0.40f && u->hp > 0) {
			if (bi >= 0) {
				float ang = atan2f(u->y - E[bi].y, u->x - E[bi].x);
				float nx = u->x + cosf(ang) * u->spd;
				float ny = u->y + sinf(ang) * u->spd;
				if (canWlk(nx, ny, u->rad)){ u->x = nx; u->y = ny; }
			}
		}
		return;
	}

	if (u->hp < u->mhp * 0.40f && u->hp > 0) {
		int nearestE = -1; float nD = 9000;
		for (int i = 0; i < NE; i++) {
			if (E[i].alive) {
				float d = fdst(u->x, u->y, E[i].x, E[i].y);
				if (d < nD) { nD = d; nearestE = i; }
			}
		}
		if (nearestE >= 0 && nD < 280.0f) {
			float ang = atan2f(u->y - E[nearestE].y, u->x - E[nearestE].x);
			float nx = u->x + cosf(ang) * u->spd;
			float ny = u->y + sinf(ang) * u->spd;
			if (canWlk(nx, ny, u->rad)){ u->x = nx; u->y = ny; u->a = R2D(ang); }
			if (u->sc == 0 && hasLOS(u->x, u->y, E[nearestE].x, E[nearestE].y)) {
				fBul(u->x, u->y, E[nearestE].x, E[nearestE].y, 0, u->role == 2 ? 2 : 0);
				u->sc = 10;
			}
			return;
		}
	}

	int targetE = -1;
	float bestD = 500.0f;

	for (int i = 0; i < NE; i++) {
		if (E[i].alive) {
			float d = fdst(u->x, u->y, E[i].x, E[i].y);
			if (d < bestD) {
				if (hasLOS(u->x, u->y, E[i].x, E[i].y) || u->cmd == CI || u->cmd == CG) {
					bestD = d; targetE = i;
				}
			}
		}
	}

	if (targetE >= 0) {
		if (hasLOS(u->x, u->y, E[targetE].x, E[targetE].y)) {
			u->a = R2D(atan2f(E[targetE].y - u->y, E[targetE].x - u->x));
			if (u->cmd == CI || u->cmd == CG) {
				if (bestD > 60.0f) snav(hi, E[targetE].x, E[targetE].y);
			}
			if (u->sc == 0) {
				fBul(u->x, u->y, E[targetE].x, E[targetE].y, 0, u->role == 2 ? 2 : 0);
				u->sc = 8;
			}
		}
		else {
			if (u->cmd == CI || u->cmd == CG) {
				if (tick % 20 == 0) bfsPath(u->x, u->y, E[targetE].x, E[targetE].y, &subPath[si]);
				u->cmd = CP;
			}
		}
	}

	if (u->cmd == CM) {
		snav(hi, u->dx, u->dy);
		if (fdst(u->x, u->y, u->dx, u->dy) < 18.f) u->cmd = CI;
	}
	else if (u->cmd == CP) {
		PathData* pd = &subPath[si];
		if (pd->count > 0 && pd->cur < pd->count) {
			float gx = pd->x[pd->cur], gy = pd->y[pd->cur];
			if (fdst(u->x, u->y, gx, gy) < 20.f) pd->cur++;
			else snav(hi, gx, gy);
		}
		else {
			u->cmd = CI;
		}
	}
}

int spawnEnemy(float x, float y, int alert) {
	for (int i = 0; i<NE; i++) {
		if (!E[i].alive) {
			E[i].x = x; E[i].y = y;
			E[i].sx = x - 20 + rand() % 40; E[i].sy = y - 20 + rand() % 40;
			E[i].ex = x - 30 + rand() % 60; E[i].ey = y - 30 + rand() % 60;
			E[i].spd = 2.0f;
			E[i].vd = 170.f; E[i].va = 60.f; E[i].hp = 50;
			E[i].alive = 1; E[i].me = 1; E[i].al = alert;
			E[i].at = alert ? 200 : 0; E[i].tp = 0; E[i].sc = 0;
			return i;
		}
	}
	return -1;
}

void resetGame(){
	buildMap();
	imgH = iLoadImage("h1.png");
	imgSH[0] = iLoadImage("sh1.png");
	imgSH[1] = iLoadImage("sh2.png");
	imgSH[2] = iLoadImage("sh3.png");
	imgSH[3] = iLoadImage("sh4.png");
	imgBu = -1;
	imgOH[0] = -1;
	imgOH[1] = -1;
	imgE = iLoadImage("e.png");
	imgHome = iLoadImage("home.jpg");
	imgTabu = iLoadImage("tabu.jpg");
	imgFloor = -1;

	tick = 0; alLv = 0; alT = 0; cHP = 100; cDead = 0; oDone = 0; cO = 0; cPh = 0; cSub = -1; selW = 0; act = 0; gZoom = 1.0f;
	drawingPath = 0; drawPathSub = -1;
	autoTriggered = 0; bldgSpawnCount = 0; bldgSpawnTimer = 0;

	/* ending state reset */
	imgEnding = -1;
	endingTimer = 0;

	for (int i = 0; i<NS + NA; i++){ subPath[i].count = 0; subPath[i].cur = 0; }
	for (int k = 0; k<7; k++) { tentsData[k].x = cWX(52); tentsData[k].y = mWY(14 + k * 4); tentsData[k].spawned = 0; }

	struct{ float x; int role; const char*nm; float r, g, b; float spd; int mhp; }
	MD[NM] = { { cWX(2), 0, "BIPLOB", 28, 182, 50, 8.5f, 150 } };
	for (int i = 0; i<NM; i++){
		H[i].x = MD[i].x; H[i].y = mWY(48); H[i].a = 90; H[i].rad = 17; H[i].spd = MD[i].spd;
		H[i].hp = H[i].mhp = MD[i].mhp; H[i].alive = 1; H[i].snk = (MD[i].role == 2) ? 1 : 0;
		H[i].inR = 0; H[i].iv = -1; H[i].role = MD[i].role; strcpy_s(H[i].nm, sizeof(H[i].nm), MD[i].nm);
		H[i].cr = MD[i].r; H[i].cg = MD[i].g; H[i].cb = MD[i].b; H[i].sub = 0; H[i].cmd = CI;
		H[i].cc = 0; H[i].sc = 0; H[i].st = 0; H[i].lx = H[i].x; H[i].ly = H[i].y;
	}

	struct{ float x; int role; const char*nm; float r, g, b; float spd; int mhp; }
	SD[NS] = { { cWX(1), 4, "JALAL", 80, 202, 80, 8.5f, 100 }, { cWX(3), 4, "KAMAL", 80, 202, 80, 8.5f, 100 },
	{ cWX(1), 4, "HARUN", 202, 80, 80, 8.5f, 100 }, { cWX(3), 4, "RATAN", 202, 202, 80, 8.5f, 100 } };
	for (int i = 0; i<NS; i++){
		int hi = NM + i; H[hi].x = SD[i].x; H[hi].y = mWY(49); H[hi].a = 90; H[hi].rad = 16; H[hi].spd = SD[i].spd;
		H[hi].hp = H[hi].mhp = SD[i].mhp; H[hi].alive = 1; H[hi].snk = 0;
		H[hi].iv = -1; strcpy_s(H[hi].nm, sizeof(H[hi].nm), SD[i].nm); H[hi].cr = SD[i].r; H[hi].cg = SD[i].g; H[hi].cb = SD[i].b;
		H[hi].sub = 1; H[hi].cmd = CF; H[hi].dx = H[hi].x; H[hi].dy = H[hi].y; H[hi].cc = 0; H[hi].sc = 0;
	}

	Veh vt[NV] = {
		{ cWX(17), mWY(8), 0, 5.5f, 44, 26, 1, -1, 1, "JEEP", 96, 96, 46 },
		{ cWX(19), mWY(10), 0, 4.f, 52, 28, 2, -1, 1, "TRUCK", 66, 50, 30 },
		{ cWX(21), mWY(12), 0, 4.5f, 44, 26, 1, -1, 1, "JEEP2", 96, 96, 46 }
	};
	for (int i = 0; i<NV; i++)V[i] = vt[i];

	for (int i = 0; i<NE; i++) E[i].alive = 0;

	struct{ int mr; float sx, ex; float spd, vd, va, hp; int tp; } ED[15] = {
		{ 44, cWX(10), cWX(55), 2.5f, 170, 55, 50, 0 }, { 43, cWX(30), cWX(35), 2.0f, 170, 55, 50, 0 },
		{ 25, cWX(26), cWX(45), 2.5f, 180, 60, 50, 0 }, { 20, cWX(26), cWX(45), 2.5f, 160, 55, 50, 0 },
		{ 30, cWX(26), cWX(45), 2.5f, 180, 60, 50, 0 }, { 18, cWX(42), cWX(46), 2.2f, 170, 50, 50, 0 },
		{ 35, cWX(42), cWX(46), 2.2f, 170, 50, 50, 0 }, { 17, cWX(25), cWX(40), 2.5f, 190, 60, 80, 1 },
		{ 18, cWX(25), cWX(40), 2.2f, 180, 60, 80, 1 }, { 15, cWX(14), cWX(16), 1.8f, 150, 50, 50, 0 },
		{ 25, cWX(14), cWX(16), 1.8f, 150, 50, 50, 0 }, { 35, cWX(14), cWX(16), 1.8f, 150, 50, 50, 0 },
		{ 12, cWX(42), cWX(45), 1.8f, 150, 50, 100, 2 }, { 13, cWX(48), cWX(55), 2.0f, 160, 50, 100, 2 },
		{ 8, cWX(20), cWX(22), 3.0f, 200, 60, 50, 0 }
	};
	for (int i = 0; i<15; i++) {
		float wy2 = mWY(ED[i].mr); E[i].x = ED[i].sx; E[i].y = wy2; E[i].sx = ED[i].sx; E[i].sy = wy2;
		E[i].ex = ED[i].ex; E[i].ey = wy2; E[i].spd = ED[i].spd; E[i].vd = ED[i].vd; E[i].va = ED[i].va;
		E[i].hp = ED[i].hp; E[i].alive = 1; E[i].me = 1; E[i].al = 0; E[i].at = 0; E[i].tp = ED[i].tp;
		E[i].a = 0; E[i].sc = 0;
	}

	int rmR[12] = { 12, 16, 20, 24, 28, 32, 16, 20, 24, 28, 32, 36 };
	int rmC[12] = { 9, 9, 9, 9, 9, 9, 21, 21, 21, 21, 21, 21 };
	for (int k = 0; k<12; k++) {
		for (int i = 0; i<3; i++) {
			spawnEnemy(cWX(rmC[k]) + (rand() % 30) - 15, mWY(rmR[k]) + (rand() % 30) - 15, 0);
		}
	}

	oDone = 0;

	for (int i = 0; i<NBL; i++)BL[i].active = 0; for (int i = 0; i<NBM; i++)BM[i].active = 0; for (int i = 0; i<NP; i++)PR[i].life = 0;
	gs = 1; strcpy_s(LOG, sizeof(LOG), "Jai Bangla! Pak sena nishchihna koro!");
}

void drawPaths(){
	for (int si = 0; si<NS + NA; si++){
		PathData*pd = &subPath[si];
		if (pd->count<2)continue;
		int hi = NM + si;
		if (!H[hi].alive)continue;
		if (H[hi].cmd != CP) continue;
		iSetColor((int)H[hi].cr, (int)H[hi].cg, (int)H[hi].cb);
		for (int k = pd->cur; k<pd->count - 1; k++){
			float x1 = sX(pd->x[k]), y1 = sY(pd->y[k]);
			float x2 = sX(pd->x[k + 1]), y2 = sY(pd->y[k + 1]);
			iLine(x1, y1, x2, y2);
			iFilledCircle((int)x1, (int)y1, 3);
		}
		iFilledCircle((int)sX(pd->x[pd->count - 1]), (int)sY(pd->y[pd->count - 1]), 3);
		if (H[hi].cmd == CP && pd->cur<pd->count){
			iSetColor(255, 255, 0);
			iCircle((int)sX(pd->x[pd->cur]), (int)sY(pd->y[pd->cur]), (int)(8 * gZoom));
		}
	}

	if (drawingPath && drawPathSub >= 0){
		PathData*pd = &subPath[drawPathSub];
		iSetColor(0, 255, 128);
		for (int k = 0; k<pd->count - 1; k++) iLine(sX(pd->x[k]), sY(pd->y[k]), sX(pd->x[k + 1]), sY(pd->y[k + 1]));
		if (pd->count>0) iFilledCircle((int)sX(pd->x[0]), (int)sY(pd->y[0]), 5);
		if (pd->count>0){
			int hi = NM + drawPathSub;
			char buf[32]; sprintf_s(buf, sizeof(buf), "PATH: %s", H[hi].nm);
			txD(sX(pd->x[0]) + 6, sY(pd->y[0]) + 6, buf, GLUT_BITMAP_HELVETICA_10);
		}
	}
}

void drawMap(){
	int etile = (int)(TILE*gZoom);
	if (etile<1)etile = 1;

	for (int mr = 0; mr<ROWS; mr++){
		float wy2 = sY(rWY(mr));
		if (wy2<-(float)(etile * 15) || wy2>(float)SH)continue;
		for (int c = 0; c<COLS; c++){
			float wx2 = sX((float)(c*TILE));
			if (wx2<-(float)(etile * 15) || wx2>(float)SW)continue;
			int t = MAP[mr][c];

			if (t == T_BLD_IMG) {
				if (mr == 14 && c == 26) {
					if (imgHome >= 0) iShowImage((int)wx2, (int)wy2, 15 * etile, 9 * etile, imgHome);
					else { iSetColor(100, 90, 80); iFilledRectangle(wx2, wy2, 15 * etile, 9 * etile); }
				}
				else if (imgHome < 0) {
					iSetColor(100, 90, 80); iFilledRectangle(wx2, wy2, etile, etile);
				}
				continue;
			}
			else if (t == T_TENT_IMG) {
				int isBL = 0;
				for (int k = 0; k<7; k++) if (c == 48 && mr == (15 + k * 4)) isBL = 1;
				if (isBL) {
					if (imgTabu >= 0) iShowImage((int)wx2, (int)wy2, 3 * etile, 2 * etile, imgTabu);
					else { iSetColor(64, 80, 48); iFilledRectangle(wx2, wy2, 3 * etile, 2 * etile); }
				}
				else if (imgTabu < 0) {
					iSetColor(64, 80, 48); iFilledRectangle(wx2, wy2, etile, etile);
				}
				continue;
			}
			else if (t == T_FLOOR) {
				// Procedural only
			}

			switch (t){
			case T_GRASS:  iSetColor(34, 64, 20); break;
			case T_WALL:   iSetColor(140, 60, 40); break;
			case T_FLOOR:  iSetColor(105, 88, 62); break;
			case T_DOOR:   iSetColor(98, 72, 38); break;
			case T_ROAD:   iSetColor(92, 76, 54); break;
			case T_WIRE:   iSetColor(55, 44, 27); break;
			case T_GATE:   iSetColor(124, 50, 16); break;
			case T_COMP:   iSetColor(40, 40, 40); break;
			default:       iSetColor(22, 22, 22);
			}
			iFilledRectangle(wx2, wy2, (float)etile, (float)etile);

			if (gZoom>0.55f){
				if (t == T_GRASS){
					iSetColor(50, 95, 30); iLine(wx2 + 4 * gZoom, wy2 + 4 * gZoom, wx2 + 3 * gZoom, wy2 + 9 * gZoom);
					iSetColor(40, 85, 20); iLine(wx2 + 6 * gZoom, wy2 + 5 * gZoom, wx2 + 7 * gZoom, wy2 + 10 * gZoom);
					iSetColor(50, 95, 30); iLine(wx2 + 9 * gZoom, wy2 + 3 * gZoom, wx2 + 10 * gZoom, wy2 + 8 * gZoom);
					if ((mr * 11 + c * 5) % 2 == 0) {
						iSetColor(45, 90, 25); iLine(wx2 + etile*0.5f, wy2 + 4 * gZoom, wx2 + etile*0.5f - 2 * gZoom, wy2 + 9 * gZoom);
						iSetColor(45, 90, 25); iLine(wx2 + etile*0.5f + 2 * gZoom, wy2 + 4 * gZoom, wx2 + etile*0.5f + 3 * gZoom, wy2 + 8 * gZoom);
						iSetColor(40, 85, 22); iLine(wx2 + etile*0.7f, wy2 + 3 * gZoom, wx2 + etile*0.7f + 1 * gZoom, wy2 + 7 * gZoom);
					}
					if ((mr * 13 + c * 7) % 3 != 0) {
						iSetColor(55, 100, 35); iLine(wx2 + etile*0.8f, wy2 + 6 * gZoom, wx2 + etile*0.8f + 2 * gZoom, wy2 + 12 * gZoom);
						iSetColor(40, 80, 20); iLine(wx2 + etile*0.7f, wy2 + 7 * gZoom, wx2 + etile*0.6f, wy2 + 11 * gZoom);
						iSetColor(48, 92, 28); iLine(wx2 + etile*0.2f, wy2 + 5 * gZoom, wx2 + etile*0.2f - 1 * gZoom, wy2 + 10 * gZoom);
					}
					if ((mr * 17 + c * 11) % 5 != 0) {
						iSetColor(52, 98, 28); iLine(wx2 + etile*0.3f, wy2 + 2 * gZoom, wx2 + etile*0.4f, wy2 + 8 * gZoom);
						iSetColor(45, 88, 23); iLine(wx2 + etile*0.4f, wy2 + 2 * gZoom, wx2 + etile*0.5f, wy2 + 7 * gZoom);
						iSetColor(40, 82, 18); iLine(wx2 + etile*0.6f, wy2 + 8 * gZoom, wx2 + etile*0.55f, wy2 + 13 * gZoom);
					}
				}
				if (t == T_FLOOR){
					iSetColor(105, 88, 62); iRectangle(wx2 + 1, wy2 + 1, etile - 2, etile - 2);
					iSetColor(75, 58, 42);
					float ef = (float)etile;
					for (int k = 1; k < 4; k++) iLine(wx2 + 1, (int)(wy2 + k * ef / 4), wx2 + ef - 1, (int)(wy2 + k * ef / 4));
					for (int k = 0; k < 4; k++) {
						float yy1 = wy2 + k * ef / 4;
						float yy2 = wy2 + (k + 1) * ef / 4;
						float xoff = (k % 2 == 0) ? (ef / 3) : (ef / 6);
						for (float xx = wx2 + xoff; xx < wx2 + ef; xx += ef / 2) {
							iLine(xx, yy1, xx, yy2);
						}
					}
				}
				if (t == T_WALL){
					iSetColor(140, 60, 40); iRectangle(wx2 + 1, wy2 + 1, etile - 2, etile - 2);
					iSetColor(110, 40, 20); float etF = (float)etile;
					for (int k = 1; k<4; k++) iLine(wx2 + 2, (int)(wy2 + k*etF / 4), wx2 + etF - 2, (int)(wy2 + k*etF / 4));
					for (int k = 0; k<4; k++) {
						float rowY = wy2 + k*etF / 4.0f;
						float endY = wy2 + (k + 1)*etF / 4.0f;
						for (int bx = 0; bx < 4; bx++) {
							float off = (k % 2 == 0) ? 0 : (etF / 6.0f);
							float lineX = wx2 + off + bx*etF / 3.0f;
							if (lineX > wx2 && lineX < wx2 + etF) iLine(lineX, rowY, lineX, endY);
						}
					}
				}
				if (t == T_ROAD){
					iSetColor(106, 89, 63); iRectangle(wx2 + 1, wy2 + 1, etile - 2, etile - 2);
					iSetColor(76, 64, 45);
					iLine(wx2 + etile / 3, wy2 + 1, wx2 + etile / 3, wy2 + etile - 1);
					iLine(wx2 + 2 * etile / 3, wy2 + 1, wx2 + 2 * etile / 3, wy2 + etile - 1);
				}
				if (t == T_WIRE){
					iSetColor(55, 44, 27); iFilledRectangle(wx2 + 1, wy2 + etile / 2 - 3 * gZoom, etile - 2, 6 * gZoom);
					iSetColor(140, 120, 46); iLine(wx2, wy2 + etile / 2, wx2 + etile, wy2 + etile / 2);
					for (int k = 0; k<4; k++){ int kx = (int)(wx2 + k*etile / 3 + etile / 6); iFilledCircle(kx, (int)(wy2 + etile / 2), (int)(2 * gZoom)); }
				}
				if (t == T_GATE){
					iSetColor(152, 82, 20); iFilledRectangle(wx2 + 2, wy2 + 2, etile - 4, etile - 4);
					iSetColor(205, 130, 36); for (int k = 1; k<4; k++)iLine((int)(wx2 + k*etile / 4), wy2 + 3, (int)(wx2 + k*etile / 4), wy2 + etile - 3);
					iSetColor(240, 188, 52); iFilledCircle((int)(wx2 + etile / 2), (int)(wy2 + etile / 2), (int)(6 * gZoom));
				}
				if (t == T_DOOR){
					iSetColor(75, 52, 23); iRectangle(wx2 + 2, wy2 + 2, etile - 4, etile - 4);
					iSetColor(112, 82, 40);
					iLine(wx2 + etile / 2 - 4 * gZoom, wy2 + 4, wx2 + etile / 2 - 4 * gZoom, wy2 + etile - 4);
					iLine(wx2 + etile / 2 + 4 * gZoom, wy2 + 4, wx2 + etile / 2 + 4 * gZoom, wy2 + etile - 4);
				}
				if (t == T_COMP){
					iSetColor(80, 80, 80); iFilledRectangle(wx2 + 2 * gZoom, wy2 + 2 * gZoom, etile - 4 * gZoom, etile - 4 * gZoom);
					iSetColor(50, 50, 50); iFilledRectangle(wx2 + 4 * gZoom, wy2 + 4 * gZoom, etile - 8 * gZoom, etile - 8 * gZoom);
					iSetColor(200, 200, 200); txD(wx2 + 4 * gZoom, wy2 + etile / 2 - 4, "BUNKER", GLUT_BITMAP_HELVETICA_10);
				}
			}
		}
	}

	float fx = sX((float)(5 * TILE)), fy = sY(rWY(45)), fw = sX((float)(60 * TILE)) - sX((float)(5 * TILE)), fh = sY(rWY(5)) - sY(rWY(45));
	if (fh<0)fh = -fh;
	if (!cDead){
		iSetColor(100, 26, 26); iRectangle(fx, fy, fw, fh);
		iSetColor(120, 36, 36); iRectangle(fx + 2, fy + 2, fw - 4, fh - 4);
		char lb[36]; sprintf_s(lb, sizeof(lb), "PAKISTANI CAMP  HP:%d%%", cHP);
		iSetColor(160, 55, 55); txD(fx + fw / 2 - 68, fy + fh + 3, lb, GLUT_BITMAP_HELVETICA_10);
	}
	else {
		iSetColor(88, 46, 0); txD(fx + fw / 2 - 100, fy + fh / 2, "** CAMP DHWANGS! **", GLUT_BITMAP_HELVETICA_18);
	}
}

void drawVeh(int i){
	Veh*v = &V[i]; if (!v->active)return;
	float wx2 = sX(v->x), wy2 = sY(v->y);
	if (wx2<-80 || wx2>SW + 80 || wy2<-80 || wy2>SH + 80)return;
	float hw = v->w / 2.f*gZoom, hh = v->h / 2.f*gZoom;

	iSetColor(v->r, v->g, v->b); iFilledRectangle(wx2 - hw, wy2 - hh, hw * 2, hh * 2);
	iSetColor(0, 0, 0); iRectangle(wx2 - hw, wy2 - hh, hw * 2, hh * 2);
	iSetColor(13, 13, 13);
	iFilledRectangle(wx2 - hw - 3 * gZoom, wy2 - hh + 2 * gZoom, 5 * gZoom, 8 * gZoom);
	iFilledRectangle(wx2 + hw - 2 * gZoom, wy2 - hh + 2 * gZoom, 5 * gZoom, 8 * gZoom);
	iFilledRectangle(wx2 - hw - 3 * gZoom, wy2 + hh - 10 * gZoom, 5 * gZoom, 8 * gZoom);
	iFilledRectangle(wx2 + hw - 2 * gZoom, wy2 + hh - 10 * gZoom, 5 * gZoom, 8 * gZoom);

	if (gZoom>0.6f){ iSetColor(252, 252, 172); txD(wx2 - 16, wy2 - 5, v->lbl, GLUT_BITMAP_HELVETICA_10); }
	if (v->cr2 >= 0){ iSetColor(0, 252, 192); txD(wx2 - 4, wy2 + 14 * gZoom, "[*]", GLUT_BITMAP_HELVETICA_10); }
}

void drawHero(int hi){
	Hero*u = &H[hi]; if (!u->alive)return; if (u->iv >= 0)return;
	float wx2 = sX(u->x), wy2 = sY(u->y);
	if (wx2<-30 || wx2>SW + 30 || wy2<-30 || wy2>SH + 30)return;
	int zr = (int)(u->rad*gZoom); if (zr<3)zr = 3;

	iSetColor(0, 0, 0); iFilledCircle((int)(wx2 + 2), (int)(wy2 - 2), zr);

	if (u->snk){ iSetColor(28, 92, 28); iCircle((int)wx2, (int)wy2, zr + (int)(14 * gZoom)); }

	int imgSize = (int)(zr * 4.8f);
	if (imgSize < 10) imgSize = 10;
	int imgX = (int)(wx2 - imgSize / 2);
	int imgY = (int)(wy2 - imgSize / 2);

	int dImg = -1;
	if (hi == 0) dImg = imgH;
	else if (hi > 0 && hi <= 4) dImg = imgSH[hi - 1];

	if (dImg >= 0){
		glPushMatrix();
		glTranslatef(wx2, wy2, 0);
		glRotatef(u->a - 90.0f, 0, 0, 1);
		iShowImage(-imgSize / 2, -imgSize / 2, imgSize, imgSize, dImg);
		glPopMatrix();
	}
	else {
		iSetColor(u->cr, u->cg, u->cb); iFilledCircle((int)wx2, (int)wy2, zr);
		iSetColor(0, 0, 0); iCircle((int)wx2, (int)wy2, zr);
		float ang = D2R(u->a); float gunLen = 26.f*gZoom;
		iSetColor(165, 165, 165); iLine(wx2, wy2, wx2 + cosf(ang)*gunLen, wy2 + sinf(ang)*gunLen);
	}

	if (hi < NM + NS) {
		iSetColor(16, 16, 16); iFilledRectangle(wx2 - 13 * gZoom, wy2 + zr + 3, 26 * gZoom, 4 * gZoom);
		float hf = (float)u->hp / u->mhp;
		iSetColor(hf>0.5f ? 20 : 208, hf>0.5f ? 174 : 40, 20);
		iFilledRectangle(wx2 - 13 * gZoom, wy2 + zr + 3, 26 * gZoom*hf, 4 * gZoom);
	}
}

void drawEne(int i){
	Ene*e = &E[i]; if (!e->alive)return;
	float wx2 = sX(e->x), wy2 = sY(e->y);
	if (wx2<-280 || wx2>SW + 280 || wy2<-280 || wy2>SH + 280)return;
	float ba = D2R(e->a);

	int r2 = (int)(((e->tp == 1) ? 25 : (e->tp == 2 ? 27 : 21))*gZoom);
	if (r2<3)r2 = 3;
	iSetColor(0, 0, 0); iFilledCircle((int)(wx2 + 2), (int)(wy2 - 2), r2);

	int imgSize = (int)(r2 * 3.3f);
	if (imgE >= 0) {
		glPushMatrix();
		glTranslatef(wx2, wy2, 0);
		glRotatef(e->a - 90.0f, 0, 0, 1);
		iShowImage(-imgSize / 2, -imgSize / 2, imgSize, imgSize, imgE);
		glPopMatrix();
	}
	else {
		if (e->tp == 1)iSetColor(188, 148, 0); else if (e->tp == 2)iSetColor(134, 95, 46); else iSetColor(168, 138, 50);
		iFilledCircle((int)wx2, (int)wy2, r2);
		iSetColor(0, 0, 0); iCircle((int)wx2, (int)wy2, r2);
		iSetColor(255, 255, 255); iLine(wx2, wy2, wx2 + cosf(ba) * 22 * gZoom, wy2 + sinf(ba) * 22 * gZoom);
	}

	iSetColor(16, 16, 16); iFilledRectangle(wx2 - 10 * gZoom, wy2 + r2 + 2, 20 * gZoom, 3 * gZoom);
	float mh = (float)(e->tp == 1 ? 80 : e->tp == 2 ? 100 : 50);
	iSetColor(195, 26, 26); iFilledRectangle(wx2 - 10 * gZoom, wy2 + r2 + 2, e->hp * 20 * gZoom / mh, 3 * gZoom);
}


void drawBoms(){
	for (int i = 0; i<NBM; i++){
		if (!BM[i].active)continue;
		float wx2 = sX(BM[i].x), wy2 = sY(BM[i].y);
		iSetColor(250, 90, 0); iFilledCircle((int)wx2, (int)wy2, (int)(7 * gZoom));
		iSetColor(250, 210, 0); iCircle((int)wx2, (int)wy2, (int)(7 * gZoom));
		char ct[4]; sprintf_s(ct, sizeof(ct), "%d", (int)(BM[i].t / 60) + 1);
		iSetColor(250, 250, 0); txD(wx2 - 4, wy2 + 12 * gZoom, ct, GLUT_BITMAP_HELVETICA_10);
	}
}
void drawBuls(){
	for (int i = 0; i<NBL; i++){
		if (!BL[i].active)continue;
		float wx2 = sX(BL[i].x), wy2 = sY(BL[i].y);
		float ba = atan2f(BL[i].vy, BL[i].vx);
		float len = BL[i].tp == 2 ? 16.f * gZoom : 10.f * gZoom;
		float tw = BL[i].tp == 2 ? 2.8f * gZoom : 2.0f * gZoom;
		double px[4], py[4];
		px[0] = wx2 + cosf(ba)*len;       py[0] = wy2 + sinf(ba)*len;
		px[1] = wx2 + cosf(ba + PI / 2)*tw;   py[1] = wy2 + sinf(ba + PI / 2)*tw;
		px[2] = wx2 - cosf(ba)*len;       py[2] = wy2 - sinf(ba)*len;
		px[3] = wx2 + cosf(ba - PI / 2)*tw;   py[3] = wy2 + sinf(ba - PI / 2)*tw;

		if (BL[i].en) {
			iSetColor(255, 120, 0); iFilledPolygon(px, py, 4);
			iSetColor(200, 30, 0); iPolygon(px, py, 4);
		}
		else if (BL[i].tp == 2) {
			iSetColor(170, 250, 170); iFilledPolygon(px, py, 4);
			iSetColor(50, 200, 50); iPolygon(px, py, 4);
		}
		else {
			iSetColor(255, 230, 100); iFilledPolygon(px, py, 4);
			iSetColor(220, 120, 0); iPolygon(px, py, 4);
		}
	}
}
void drawPars(){
	for (int i = 0; i<NP; i++){
		if (PR[i].life <= 0)continue;
		float a = (float)PR[i].life / 26.f;
		iSetColor(PR[i].r*a, PR[i].g*a, PR[i].b*a);
		iFilledCircle((int)sX(PR[i].x), (int)sY(PR[i].y), (int)(3 * gZoom));
	}
}

void drawPathCursor(int mx, int my){
	if (drawingPath && drawPathSub >= 0){
		iSetColor(0, 200, 100); iCircle(mx, my, 8);
		iSetColor(255, 255, 0); txD(mx + 10, my + 4, "DRAWING PATH...", GLUT_BITMAP_HELVETICA_10);
	}
}

void drawMM(){
	int cx = MM_CX, cy = MM_CY, R = MM_R;
	iSetColor(4, 4, 4); iFilledCircle(cx, cy, R + 4);
	iSetColor(18, 28, 12); iFilledCircle(cx, cy, R);
	for (int mr = 0; mr<ROWS; mr++){
		for (int c = 0; c<COLS; c++){
			int t = MAP[mr][c];
			float wx = c*TILE + TILE / 2.f;
			float wy = (ROWS - 1 - mr)*TILE + TILE / 2.f;
			float dx2 = (wx - H[act].x)*MM_S;
			float dy2 = (wy - H[act].y)*MM_S;
			if (dx2*dx2 + dy2*dy2>(float)((R - 3)*(R - 3)))continue;
			int mx = cx + (int)dx2;
			int my = cy + (int)dy2;
			switch (t){
			case T_GRASS:  iSetColor(18, 42, 10); break;
			case T_WALL:   iSetColor(72, 60, 42); break;
			case T_FLOOR:  iSetColor(82, 69, 48); break;
			case T_ROAD:   iSetColor(96, 82, 60); break;
			case T_GATE:   iSetColor(200, 80, 20); break;
			case T_WIRE:   iSetColor(80, 66, 32); break;
			case T_TENT_IMG: iSetColor(55, 72, 42); break;
			default:       iSetColor(20, 20, 20); break;
			}
			int ps = (int)(TILE*MM_S) + 1; if (ps<1)ps = 1;
			iFilledRectangle((float)mx, (float)my, (float)ps, (float)ps);
		}
	}
	for (int i = 0; i<NE; i++){
		if (!E[i].alive)continue;
		float dx2 = (E[i].x - H[act].x)*MM_S, dy2 = (E[i].y - H[act].y)*MM_S;
		if (dx2*dx2 + dy2*dy2>(float)((R - 5)*(R - 5)))continue;
		iSetColor(E[i].al ? 255 : 188, 26, 26);
		iFilledCircle(cx + (int)dx2, cy + (int)dy2, 2);
	}
	for (int i = 0; i<NO; i++){
		if (OBJ[i].done)continue;
		float dx2 = (OBJ[i].x - H[act].x)*MM_S, dy2 = (OBJ[i].y - H[act].y)*MM_S;
		if (dx2*dx2 + dy2*dy2>(float)((R - 5)*(R - 5)))continue;
		iSetColor(238, 202, 0); iFilledCircle(cx + (int)dx2, cy + (int)dy2, 3);
	}
	for (int h = 0; h<NH; h++){
		if (!H[h].alive)continue;
		float dx2 = (H[h].x - H[act].x)*MM_S, dy2 = (H[h].y - H[act].y)*MM_S;
		if (dx2*dx2 + dy2*dy2>(float)((R - 4)*(R - 4)))continue;
		iSetColor(H[h].cr, H[h].cg, H[h].cb);
		iFilledCircle(cx + (int)dx2, cy + (int)dy2, h<NM ? 4 : 3);
	}
	iSetColor(255, 255, 0); iFilledCircle(cx, cy, 5); iSetColor(255, 255, 255); iCircle(cx, cy, 5);
	float ang = D2R(H[act].a);
	iSetColor(255, 255, 255); iLine(cx, cy, cx + (int)(cosf(ang) * 9), cy + (int)(sinf(ang) * 9));
	iSetColor(58, 88, 44); iCircle(cx, cy, R);
}

void drawZoomButtons(){
	iSetColor(6, 12, 4); iFilledRectangle(ZB_X - 2, ZB_Y - 2, 62, 44);
	iSetColor(34, 58, 20); iRectangle(ZB_X - 2, ZB_Y - 2, 62, 44);
	int zIn = (gZoom<ZOOM_MAX);
	iSetColor(zIn ? 22 : 12, zIn ? 45 : 22, zIn ? 14 : 10); iFilledRectangle(ZB_X, ZB_Y + 22, 26, 20);
	iSetColor(zIn ? 66 : 30, zIn ? 110 : 50, zIn ? 44 : 24); iRectangle(ZB_X, ZB_Y + 22, 26, 20);
	iSetColor(zIn ? 168 : 80, zIn ? 228 : 110, zIn ? 108 : 60); txD(ZB_X + 9, ZB_Y + 28, "+", GLUT_BITMAP_HELVETICA_18);
	int zOut = (gZoom>ZOOM_MIN);
	iSetColor(zOut ? 22 : 12, zOut ? 45 : 22, zOut ? 14 : 10); iFilledRectangle(ZB_X, ZB_Y, 26, 20);
	iSetColor(zOut ? 66 : 30, zOut ? 110 : 50, zOut ? 44 : 24); iRectangle(ZB_X, ZB_Y, 26, 20);
	iSetColor(zOut ? 168 : 80, zOut ? 228 : 110, zOut ? 108 : 60); txD(ZB_X + 10, ZB_Y + 6, "-", GLUT_BITMAP_HELVETICA_18);
}

static int curMX = 0, curMY = 0;

void drawHUD(){
	iSetColor(6, 9, 4); iFilledRectangle(0, 0, (float)SW, (float)HUD_H);
	iSetColor(24, 48, 16); iLine(0, HUD_H, (float)SW, (float)HUD_H);
	for (int h = 0; h<NM; h++){
		float bx = 2.f + h*106.f;
		iSetColor((h == act) ? 16 : 7, (h == act) ? 36 : 11, (h == act) ? 10 : 6); iFilledRectangle(bx, 2, 103, HUD_H - 4);
		iSetColor((h == act) ? 168 : 32, (h == act) ? 188 : 50, (h == act) ? 0 : 18); iRectangle(bx, 2, 103, HUD_H - 4);
		iSetColor(H[h].cr, H[h].cg, H[h].cb); iFilledCircle((int)(bx + 11), (int)(HUD_H / 2), 9);
		iSetColor(198, 198, 172); txD(bx + 24, HUD_H - 14, H[h].nm, GLUT_BITMAP_HELVETICA_10);
		iSetColor(18, 18, 14); iFilledRectangle(bx + 24, HUD_H - 36, 72, 8);
		float hf = (float)H[h].hp / H[h].mhp; iSetColor(hf>0.5f ? 18 : 205, hf>0.5f ? 168 : 38, 18); iFilledRectangle(bx + 24, HUD_H - 36, 72 * hf, 8);
		iSetColor(50, 50, 40); iRectangle(bx + 24, HUD_H - 36, 72, 8);
		if (!H[h].alive){ iSetColor(192, 24, 24); txD(bx + 28, HUD_H / 2 - 6, "SHAHID", GLUT_BITMAP_HELVETICA_10); }
	}
	for (int i = 0; i<NS; i++){
		int hi = NM + i; float bx = 250.f + i*64.f; int isSel = (cSub == i);
		iSetColor(isSel ? 28 : 5, isSel ? 28 : 7, isSel ? 6 : 4); iFilledRectangle(bx, 2, 62, HUD_H - 4);
		iSetColor(isSel ? 0 : 36, isSel ? 220 : 36, isSel ? 128 : 28); iRectangle(bx, 2, 62, HUD_H - 4);
		iSetColor(188, 188, 165); txD(bx + 20, HUD_H - 13, H[hi].nm, GLUT_BITMAP_HELVETICA_10);
		iSetColor(18, 18, 14); iFilledRectangle(bx + 2, HUD_H - 34, 58, 5);
		float hf = (float)H[hi].hp / H[hi].mhp; iSetColor(hf>0.5f ? 18 : 175, hf>0.5f ? 155 : 36, 18); iFilledRectangle(bx + 2, HUD_H - 34, 58 * hf, 5);
		if (!H[hi].alive){ iSetColor(148, 22, 22); txD(bx + 6, HUD_H / 2 - 5, "SHAHID", GLUT_BITMAP_HELVETICA_10); }
	}
	for (int i = 0; i<NA; i++){
		int hi = NM + NS + i; float bx = 250.f + (NS + i)*64.f;
		iSetColor(10, 10, 10); iFilledRectangle(bx, 2, 62, HUD_H - 4);
		iSetColor(180, 0, 180); iRectangle(bx, 2, 62, HUD_H - 4);
		iSetColor(188, 188, 165); txD(bx + 20, HUD_H - 13, H[hi].nm, GLUT_BITMAP_HELVETICA_10);
		txD(bx + 14, HUD_H - 34, autoTriggered ? "ATTACK" : "IDLE", GLUT_BITMAP_HELVETICA_10);
		if (!H[hi].alive){ iSetColor(148, 22, 22); txD(bx + 6, HUD_H / 2 - 5, "SHAHID", GLUT_BITMAP_HELVETICA_10); }
	}

	if (alLv == 0){ iSetColor(18, 182, 18); txD(130, 10, "NIRAPAD", GLUT_BITMAP_HELVETICA_12); }
	else if (alLv == 1){ iSetColor(228, 182, 0); txD(130, 10, "SANDEH!", GLUT_BITMAP_HELVETICA_12); }
	else{ iSetColor(228, 22, 22); txD(130, 10, "ALARM!", GLUT_BITMAP_HELVETICA_12); }

	iSetColor(66, 66, 56);
	txD(SW - 250, HUD_H - 8, "WASD:Chalo LClick:Guli RClick:Gari Z/V:Zoom", GLUT_BITMAP_HELVETICA_10);
	txD(SW - 250, HUD_H - 18, "C+1-7: Sub sel   RMB drag=path", GLUT_BITMAP_HELVETICA_10);
	txD(SW - 250, HUD_H - 28, "P:Bomb N:Luko", GLUT_BITMAP_HELVETICA_10);
	txD(SW - 250, HUD_H - 38, "X:Detonate K:Gate F:New Level", GLUT_BITMAP_HELVETICA_10);

	iSetColor(3, 3, 3); iFilledRectangle(0, HUD_H, (float)SW, 14);
	iSetColor(80, 172, 96); txD(4, HUD_H + 2, LOG, GLUT_BITMAP_HELVETICA_10);
}

void drawCmdMenu(){
	iSetColor(2, 5, 1); iFilledRectangle(142, 142, 616, 328); iSetColor(34, 84, 24); iRectangle(142, 142, 616, 328);
	iSetColor(44, 198, 44); txD(240, 460, "MUKTIJUDDHO COMMAND", GLUT_BITMAP_HELVETICA_18);
	if (cPh == 0){
		iSetColor(178, 178, 158); txD(162, 434, "1-7 chap: muktijodha select koro:", GLUT_BITMAP_HELVETICA_12);
		iSetColor(118, 118, 102); txD(162, 168, "ESC ba C = bondho", GLUT_BITMAP_HELVETICA_10);
	}
	else {
		int hi = NM + cSub; char buf[52]; sprintf_s(buf, sizeof(buf), "Command: %s", H[hi].nm); iSetColor(232, 205, 66); txD(162, 434, buf, GLUT_BITMAP_HELVETICA_12);
		iSetColor(178, 178, 158);
		txD(162, 412, "M = MOVE   A = ATTACK   S = STOP", GLUT_BITMAP_HELVETICA_12);
		txD(162, 390, "F = FOLLOW   G = GUARD", GLUT_BITMAP_HELVETICA_12);
	}
}

void drawMenuButton(float x, float y, float w, float h, const char* text, int isHover) {
	if (isHover) {
		iSetColor(180, 50, 50); iFilledRectangle(x, y, w, h);
		iSetColor(220, 80, 80); iLine(x, y + h, x + w, y + h); iLine(x, y, x, y + h);
		iSetColor(80, 20, 20); iLine(x, y, x + w, y); iLine(x + w, y, x + w, y + h);
		iRectangle(x - 1, y - 1, w + 2, h + 2);
	}
	else {
		iSetColor(140, 35, 35); iFilledRectangle(x, y, w, h);
		iSetColor(180, 60, 60); iLine(x, y + h, x + w, y + h); iLine(x, y, x, y + h);
		iSetColor(60, 10, 10); iLine(x, y, x + w, y); iLine(x + w, y, x + w, y + h);
		iRectangle(x - 1, y - 1, w + 2, h + 2);
	}

	float tx = x + (w - strlen(text) * 16) / 2.0f + 4;
	float ty = y + (h - 20) / 2.0f;

	iSetColor(20, 40, 20);
	txD(tx + 2, ty - 2, text, GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(100, 160, 100);
	txD(tx, ty, text, GLUT_BITMAP_TIMES_ROMAN_24);
}

/* ══════════════════════════════════════════════════════════════
LEVEL 2 - BUNKER SHOOTER
══════════════════════════════════════════════════════════════ */

int B_USE_IMAGES = 1;

#define B_STATE_PLAYING 1
#define B_STATE_GAMEOVER 2
#define B_STATE_WON 3
#define B_STATE_CUTSCENE 4

int b_gameState = B_STATE_PLAYING;

int B_imgPlayerIdle = -1, B_imgPlayerCover = -1;
int B_imgPlayerIdleBack = -1;
int B_imgPlayerWalk[4] = { -1, -1, -1, -1 };
int B_imgPlayerWalkBack[4] = { -1, -1, -1, -1 };
int B_imgEnemyBody[4] = { -1, -1, -1, -1 };
int B_imgBarrel = -1;
int B_imgCutscene[3] = { -1, -1, -1 };
int B_cutsceneFrames = 3;
int B_currentCutsceneFrame = 0;

float B_END_DOOR_Y = 4000.0f;
int B_imagesLoaded = 0;

struct B_Player {
	float x, y;
	float lastX, lastY;
	int hp;
	int inCover;
	int isDead;
	int walkFrame;
	int walkTimer;
	float coverTimer;
	int facingBack;
	int movingTimer;
} B_player;

struct B_Enemy {
	float x, y;
	int hp;
	int active;
	int isDead;
	float shootTimer;
	int walkFrame;
} B_enemies[100];

struct B_Obstacle {
	float x, y;
	int active;
} B_obstacles[100];

struct B_EnemyBullet {
	float x, y;
	float vx, vy;
	int active;
} B_enemyBullets[200];

struct B_PlayerBullet {
	float x, y;
	float vx, vy;
	int active;
	int targetEnemy;
	int damagePayload;
} B_pBullets[100];

int B_currentWave = 1;
int B_waveActive = 0;
int B_enemiesRemaining = 0;

int B_aimX = 450;
int B_aimY = 325;

void B_worldToScreen(float wx, float wy, float *sx, float *sy) {
	float rx = wx - B_player.x;
	float ry = wy - B_player.y;
	*sx = (SW / 2.0f) + (rx * 1.5f + ry * 1.5f);
	*sy = (SH / 4.0f) + (-rx * 0.75f + ry * 0.75f);
}

typedef struct {
	int type;
	float sx, sy;
	float wy;
	void* ref;
} B_Drawable;

int B_compareDrawable(const void* a, const void* b) {
	B_Drawable* da = (B_Drawable*)a;
	B_Drawable* db = (B_Drawable*)b;
	if (da->sy < db->sy) return 1;
	if (da->sy > db->sy) return -1;
	return 0;
}

void B_loadAllImages() {
	if (B_imagesLoaded) return;
	B_imagesLoaded = 1;
	B_imgPlayerIdle = iLoadImage("images/player_walk1.png");
	B_imgPlayerIdleBack = iLoadImage("images/player_back1.png");
	B_imgPlayerWalk[0] = iLoadImage("images/player_walk2.png");
	B_imgPlayerWalk[1] = iLoadImage("images/player_walk1.png");
	B_imgPlayerWalk[2] = iLoadImage("images/player_walk3.png");
	B_imgPlayerWalk[3] = iLoadImage("images/player_walk2.png");
	B_imgPlayerWalkBack[0] = iLoadImage("images/player_back1.png");
	B_imgPlayerWalkBack[1] = iLoadImage("images/player_back2.png");
	B_imgPlayerWalkBack[2] = iLoadImage("images/player_back3.png");
	B_imgPlayerWalkBack[3] = iLoadImage("images/player_back2.png");
	B_imgPlayerCover = iLoadImage("images/player_cover.png");
	B_imgEnemyBody[0] = iLoadImage("images/enemy_body2.png");
	B_imgEnemyBody[1] = iLoadImage("images/enemy_body1.png");
	B_imgEnemyBody[2] = iLoadImage("images/enemy_body2.png");
	B_imgEnemyBody[3] = iLoadImage("images/enemy_body3.png");
	B_imgBarrel = iLoadImage("images/obj.png");
	B_imgCutscene[0] = iLoadImage("images/cutscene1.png");
	B_imgCutscene[1] = iLoadImage("images/cutscene2.png");
	B_imgCutscene[2] = iLoadImage("images/cutscene3.png");
}

void B_initGame() {
	B_player.x = 0;
	B_player.y = 0;
	B_player.lastX = 0;
	B_player.lastY = 0;
	B_player.walkFrame = -1;
	B_player.walkTimer = 0;
	B_player.hp = 100;
	B_player.isDead = 0;
	B_player.inCover = 0;
	B_player.coverTimer = 0.0f;
	B_player.facingBack = 0;
	B_player.movingTimer = 0;

	B_currentWave = 1;
	B_waveActive = 0;
	B_enemiesRemaining = 0;
	b_gameState = B_STATE_PLAYING;

	for (int i = 0; i<100; i++) B_enemies[i].active = 0;
	for (int i = 0; i<200; i++) B_enemyBullets[i].active = 0;
	for (int i = 0; i<100; i++) B_pBullets[i].active = 0;

	float obsY = 200.0f;
	for (int i = 0; i < 50; i++) {
		B_obstacles[i].active = 1;
		obsY += 150.0f + (rand() % 200);
		B_obstacles[i].y = obsY;
		B_obstacles[i].x = -120.0f + (rand() % 240);
		if (i < 49 && rand() % 5 == 0) {
			i++;
			B_obstacles[i].active = 1;
			B_obstacles[i].y = obsY + (rand() % 40 - 20);
			B_obstacles[i].x = B_obstacles[i - 1].x + (rand() % 50 - 25);
		}
	}
}

void B_checkCover() {
	B_player.inCover = 0;
	for (int i = 0; i<50; i++) {
		if (!B_obstacles[i].active) continue;
		float dx = B_player.x - B_obstacles[i].x;
		float dy = B_obstacles[i].y - B_player.y;
		if (fabs(dx) < 40.0f && dy > 0.0f && dy < 60.0f) {
			B_player.inCover = 1;
			break;
		}
	}
}

void B_spawnEnemyBullet(float ex, float ey) {
	playGunSound();
	for (int i = 0; i<200; i++) {
		if (!B_enemyBullets[i].active) {
			B_enemyBullets[i].active = 1;
			B_enemyBullets[i].x = ex;
			B_enemyBullets[i].y = ey;
			float dx = B_player.x - ex;
			float dy = B_player.y - ey;
			float len = sqrtf(dx*dx + dy*dy);
			if (len < 1) len = 1;
			B_enemyBullets[i].vx = (dx / len) * 7.0f;
			B_enemyBullets[i].vy = (dy / len) * 7.0f;
			break;
		}
	}
}

void B_updateGameState() {
	if (b_gameState != B_STATE_PLAYING) return;

	if (B_player.x == B_player.lastX && B_player.y == B_player.lastY) {
		if (B_player.movingTimer > 0) B_player.movingTimer--;
		else B_player.walkFrame = -1;
	}
	else {
		B_player.movingTimer = 10;
	}
	B_player.lastX = B_player.x;
	B_player.lastY = B_player.y;

	B_checkCover();

	if (B_player.inCover) {
		B_player.coverTimer += 0.016f;
	}
	else {
		B_player.coverTimer = 0.0f;
	}

	float nextWaveTriggerY = B_currentWave * 800.0f;

	if (!B_waveActive && B_player.y > nextWaveTriggerY - 100.0f && B_player.y < B_END_DOOR_Y - 500) {
		B_waveActive = 1;
		int spawnCount = B_currentWave * 2 + 1;
		for (int i = 0; i<spawnCount; i++) {
			for (int j = 0; j<100; j++) {
				if (!B_enemies[j].active) {
					B_enemies[j].active = 1;
					B_enemies[j].isDead = 0;
					B_enemies[j].hp = 100;
					B_enemies[j].x = (float)((rand() % 200) - 100);
					B_enemies[j].y = nextWaveTriggerY + 400.0f + (rand() % 200);
					B_enemies[j].shootTimer = 1.0f + (rand() % 100) / 50.0f;
					B_enemiesRemaining++;
					break;
				}
			}
		}
	}

	if (B_waveActive && B_enemiesRemaining <= 0) {
		B_waveActive = 0;
		B_currentWave++;
	}

	for (int i = 0; i<100; i++) {
		if (!B_enemies[i].active || B_enemies[i].isDead) continue;

		if (B_player.y >= B_enemies[i].y - 80.0f) {
			B_player.hp = 0;
			B_player.isDead = 1;
			b_gameState = B_STATE_GAMEOVER;
		}

		float targetX = B_player.x + ((i % 5) - 2) * 45.0f;
		float distY = B_enemies[i].y - B_player.y;
		float stopDist = (B_player.coverTimer > 5.0f) ? 0.0f : 200.0f;

		if (distY > stopDist || fabs(B_enemies[i].x - targetX) > 5.0f) {
			if (distY > stopDist) B_enemies[i].y -= 1.5f;
			if (B_enemies[i].x > targetX + 5.0f) B_enemies[i].x -= 0.5f;
			else if (B_enemies[i].x < targetX - 5.0f) B_enemies[i].x += 0.5f;
			B_enemies[i].walkFrame = ((int)fabs(B_enemies[i].y) / 10) % 4;
		}
		else {
			B_enemies[i].walkFrame = -1;
		}

		B_enemies[i].shootTimer -= 0.016f;
		if (B_enemies[i].shootTimer <= 0) {
			B_spawnEnemyBullet(B_enemies[i].x, B_enemies[i].y);
			B_enemies[i].shootTimer = 2.0f + (rand() % 100) / 50.0f;
		}
	}

	for (int i = 0; i<200; i++) {
		if (!B_enemyBullets[i].active) continue;
		B_enemyBullets[i].x += B_enemyBullets[i].vx;
		B_enemyBullets[i].y += B_enemyBullets[i].vy;
		float dx = B_enemyBullets[i].x - B_player.x;
		float dy = B_enemyBullets[i].y - B_player.y;
		float dist = sqrtf(dx*dx + dy*dy);
		if (dist < 20.0f) {
			if (B_player.inCover && B_enemyBullets[i].vy < 0) {
				B_enemyBullets[i].active = 0;
			}
			else {
				B_player.hp -= 10;
				B_enemyBullets[i].active = 0;
				if (B_player.hp <= 0) {
					B_player.isDead = 1;
					b_gameState = B_STATE_GAMEOVER;
				}
			}
		}
		if (B_enemyBullets[i].y < B_player.y - 400.0f) {
			B_enemyBullets[i].active = 0;
		}
	}

	for (int i = 0; i<100; i++) {
		if (!B_pBullets[i].active) continue;
		B_pBullets[i].x += B_pBullets[i].vx;
		B_pBullets[i].y += B_pBullets[i].vy;
		if (B_pBullets[i].y > B_player.y + 1600.0f || B_pBullets[i].y < B_player.y - 400.0f) {
			B_pBullets[i].active = 0;
			continue;
		}
		if (B_pBullets[i].targetEnemy != -1) {
			int e = B_pBullets[i].targetEnemy;
			if (B_enemies[e].active && !B_enemies[e].isDead) {
				if (B_pBullets[i].y >= B_enemies[e].y - 30.0f) {
					B_enemies[e].hp -= B_pBullets[i].damagePayload;
					B_pBullets[i].active = 0;
					if (B_enemies[e].hp <= 0) {
						B_enemies[e].isDead = 1;
						B_enemiesRemaining--;
					}
				}
			}
			else {
				B_pBullets[i].targetEnemy = -1;
			}
		}
	}

	if (B_player.y >= B_END_DOOR_Y) {
		b_gameState = B_STATE_CUTSCENE;
		B_currentCutsceneFrame = 0;
	}
}

void B_drawCrosshair() {
	iSetColor(0, 255, 0);
	iCircle(B_aimX, B_aimY, 7);
	iLine(B_aimX - 12, B_aimY, B_aimX - 4, B_aimY);
	iLine(B_aimX + 12, B_aimY, B_aimX + 4, B_aimY);
	iLine(B_aimX, B_aimY - 12, B_aimX, B_aimY - 4);
	iLine(B_aimX, B_aimY + 12, B_aimX, B_aimY + 4);
	iFilledCircle(B_aimX, B_aimY, 2);
}

void B_drawGame() {
	B_Drawable drawables[500];
	int dCount = 0;

	float psx, psy;
	B_worldToScreen(B_player.x, B_player.y, &psx, &psy);
	drawables[dCount].type = 4; drawables[dCount].sy = psy; drawables[dCount].sx = psx; dCount++;

	for (int i = 0; i<50; i++) {
		if (B_obstacles[i].active) {
			float sx2, sy2;
			B_worldToScreen(B_obstacles[i].x, B_obstacles[i].y, &sx2, &sy2);
			drawables[dCount].type = 2; drawables[dCount].sy = sy2; drawables[dCount].sx = sx2; drawables[dCount].ref = &B_obstacles[i]; dCount++;
		}
	}

	for (int i = 0; i<100; i++) {
		if (B_enemies[i].active) {
			float sx2, sy2;
			B_worldToScreen(B_enemies[i].x, B_enemies[i].y, &sx2, &sy2);
			drawables[dCount].type = 3; drawables[dCount].sy = sy2; drawables[dCount].sx = sx2; drawables[dCount].ref = &B_enemies[i]; dCount++;
		}
	}

	for (int i = 0; i<200; i++) {
		if (B_enemyBullets[i].active) {
			float sx2, sy2;
			B_worldToScreen(B_enemyBullets[i].x, B_enemyBullets[i].y, &sx2, &sy2);
			drawables[dCount].type = 5; drawables[dCount].sy = sy2; drawables[dCount].sx = sx2; drawables[dCount].ref = &B_enemyBullets[i]; dCount++;
		}
	}

	float wallStartWy = ((int)B_player.y / 150) * 150 - 400.0f;
	for (float wy2 = wallStartWy; wy2 < B_player.y + 1600.0f; wy2 += 150.0f) {
		float sx2, sy2;
		B_worldToScreen(-250.0f, wy2, &sx2, &sy2);
		drawables[dCount].type = 7; drawables[dCount].sy = sy2; drawables[dCount].sx = sx2; drawables[dCount].wy = wy2; dCount++;
	}

	for (int i = 0; i<100; i++) {
		if (B_pBullets[i].active) {
			float sx2, sy2;
			B_worldToScreen(B_pBullets[i].x, B_pBullets[i].y, &sx2, &sy2);
			drawables[dCount].type = 9; drawables[dCount].sy = sy2; drawables[dCount].sx = sx2; drawables[dCount].ref = &B_pBullets[i]; dCount++;
		}
	}

	qsort(drawables, dCount, sizeof(B_Drawable), B_compareDrawable);

	for (int i = 0; i<dCount; i++) {
		B_Drawable d = drawables[i];
		if (d.sx < -200 || d.sx > SW + 200 || d.sy < -200 || d.sy > SH + 200) continue;

		switch (d.type) {
		case 2:
			if (!B_USE_IMAGES || B_imgBarrel < 0) {
				iSetColor(139, 69, 19);
				iFilledRectangle(d.sx - 35, d.sy, 70, 80);
			}
			else {
				iShowImage((int)(d.sx - 35), (int)d.sy, 70, 80, B_imgBarrel);
			}
			break;

		case 3: {
					B_Enemy* e = (B_Enemy*)d.ref;
					if (!e->isDead) {
						if (!B_USE_IMAGES || B_imgEnemyBody[0] < 0) {
							iSetColor(200, 50, 50); iFilledRectangle(d.sx - 75, d.sy, 150, 220);
						}
						else {
							int eFrame = e->walkFrame == -1 ? 0 : e->walkFrame % 4;
							iShowImage((int)(d.sx - 75), (int)d.sy, 150, 220, B_imgEnemyBody[eFrame]);
						}
						iSetColor(255, 0, 0); iFilledRectangle(d.sx - 75, d.sy - 15, 150, 6);
						iSetColor(0, 255, 0); iFilledRectangle(d.sx - 75, d.sy - 15, (e->hp / 100.0f) * 150, 6);
					}
					else {
						iSetColor(150, 20, 20);
						float bwx = e->x, bwy = e->y;
						float s1x, s1y, s2x, s2y, s3x, s3y, s4x, s4y, s5x, s5y, s6x, s6y;
						B_worldToScreen(bwx - 45.0f, bwy - 25.0f, &s1x, &s1y);
						B_worldToScreen(bwx + 30.0f, bwy - 50.0f, &s2x, &s2y);
						B_worldToScreen(bwx + 55.0f, bwy + 15.0f, &s3x, &s3y);
						B_worldToScreen(bwx + 10.0f, bwy + 60.0f, &s4x, &s4y);
						B_worldToScreen(bwx - 50.0f, bwy + 40.0f, &s5x, &s5y);
						B_worldToScreen(bwx - 25.0f, bwy + 5.0f, &s6x, &s6y);
						double bPx[6] = { s1x, s2x, s3x, s4x, s5x, s6x };
						double bPy[6] = { s1y, s2y, s3y, s4y, s5y, s6y };
						iFilledPolygon(bPx, bPy, 6);
					}
					break;
		}

		case 4:
			if (B_player.inCover) {
				if (!B_USE_IMAGES || B_imgPlayerCover < 0) {
					iSetColor(0, 150, 255);
					iFilledRectangle(d.sx - 50, d.sy, 100, 100);
				}
				else {
					iShowImage((int)(d.sx - 75), (int)d.sy, 150, 150, B_imgPlayerCover);
				}
			}
			else {
				if (!B_USE_IMAGES || B_imgPlayerIdle < 0) {
					iSetColor(0, 150, 255);
					iFilledRectangle(d.sx - 75, d.sy, 150, 220);
				}
				else {
					float bImgW = 150.0f;
					float bImgH = 220.0f;
					float bImgX = d.sx - (bImgW / 2.0f);
					float bImgY = d.sy;
					if (B_player.walkFrame == -1) {
						if (B_player.facingBack) {
							iShowImage((int)bImgX, (int)bImgY, (int)bImgW, (int)bImgH, B_imgPlayerIdleBack);
						}
						else {
							iShowImage((int)bImgX, (int)bImgY, (int)bImgW, (int)bImgH, B_imgPlayerIdle);
						}
					}
					else {
						if (B_player.facingBack) {
							iShowImage((int)bImgX, (int)bImgY, (int)bImgW, (int)bImgH, B_imgPlayerWalkBack[B_player.walkFrame % 4]);
						}
						else {
							iShowImage((int)bImgX, (int)bImgY, (int)bImgW, (int)bImgH, B_imgPlayerWalk[B_player.walkFrame % 4]);
						}
					}
				}
			}
			break;

		case 5: {
					B_EnemyBullet* eb = (B_EnemyBullet*)d.ref;
					iSetColor(255, 255, 0);
					float tsx, tsy;
					B_worldToScreen(eb->x - eb->vx * 4.0f, eb->y - eb->vy * 4.0f, &tsx, &tsy);
					iLine(d.sx, d.sy, tsx, tsy);
					iLine(d.sx + 1, d.sy + 1, tsx + 1, tsy + 1);
					iLine(d.sx - 1, d.sy - 1, tsx - 1, tsy - 1);
					break;
		}

		case 7: {
					float bl_sx, bl_sy, br_sx, br_sy;
					B_worldToScreen(-250.0f, d.wy, &bl_sx, &bl_sy);
					B_worldToScreen(-250.0f, d.wy + 150.0f, &br_sx, &br_sy);
					double wpx[4] = { bl_sx, br_sx, br_sx, bl_sx };
					double wpy[4] = { bl_sy, br_sy, br_sy + 200.0f, bl_sy + 200.0f };
					iSetColor(120, 125, 120); iFilledPolygon(wpx, wpy, 4);
					iSetColor(85, 90, 85);
					iLine((bl_sx + br_sx) / 2.0f, (bl_sy + br_sy) / 2.0f, (bl_sx + br_sx) / 2.0f, (bl_sy + br_sy) / 2.0f + 200.0f);
					iSetColor(70, 65, 60);
					double dirtPx[4] = { bl_sx, br_sx, br_sx, bl_sx };
					double dirtPy[4] = { bl_sy, br_sy, br_sy + 15.0f, bl_sy + 15.0f };
					iFilledPolygon(dirtPx, dirtPy, 4);
					double tPx[4] = { bl_sx, br_sx, br_sx - 60.0f, bl_sx - 60.0f };
					double tPy[4] = { bl_sy + 200.0f, br_sy + 200.0f, br_sy + 230.0f, bl_sy + 230.0f };
					iSetColor(145, 150, 145); iFilledPolygon(tPx, tPy, 4);
					iSetColor(90, 95, 90); iPolygon(tPx, tPy, 4);

					if (d.wy > B_END_DOOR_Y - 80.0f && d.wy < B_END_DOOR_Y + 80.0f) {
						float dl_sx, dl_sy, dr_sx, dr_sy;
						B_worldToScreen(-250.0f, d.wy + 40.0f, &dl_sx, &dl_sy);
						B_worldToScreen(-250.0f, d.wy + 110.0f, &dr_sx, &dr_sy);
						double drPx[4] = { dl_sx, dr_sx, dr_sx, dl_sx };
						double drPy[4] = { dl_sy, dr_sy, dr_sy + 150.0f, dl_sy + 150.0f };
						iSetColor(90, 70, 60); iFilledPolygon(drPx, drPy, 4);
						iSetColor(50, 40, 30); iPolygon(drPx, drPy, 4);
						double inDrPx[4] = { dl_sx + 5.0f, dr_sx - 5.0f, dr_sx - 5.0f, dl_sx + 5.0f };
						double inDrPy[4] = { dl_sy + 5.0f, dr_sy + 5.0f, dr_sy + 145.0f, dl_sy + 145.0f };
						iSetColor(100, 80, 70); iFilledPolygon(inDrPx, inDrPy, 4);
						float mid_sx = (dl_sx + dr_sx) / 2.0f;
						float mid_sy = (dl_sy + dr_sy) / 2.0f + 95.0f;
						iSetColor(60, 50, 40); iFilledCircle(mid_sx, mid_sy, 22.0);
						iSetColor(150, 190, 210); iFilledCircle(mid_sx, mid_sy, 16.0);
						float wheel_sy = (dl_sy + dr_sy) / 2.0f + 40.0f;
						iSetColor(40, 40, 40); iFilledCircle(mid_sx, wheel_sy, 14.0);
						iSetColor(90, 80, 70); iFilledCircle(mid_sx, wheel_sy, 10.0);
						iSetColor(50, 50, 50); iFilledCircle(mid_sx, wheel_sy, 4.0);
					}
					else {
						float mid_sx = (bl_sx + br_sx) / 2.0f;
						float mid_sy = (bl_sy + br_sy) / 2.0f + 140.0f;
						iSetColor(40, 40, 40); iFilledCircle(mid_sx, mid_sy, 12.0);
						iSetColor(255, 255, 150); iFilledCircle(mid_sx, mid_sy, 8.0);
						iSetColor(255, 255, 255); iFilledCircle(mid_sx, mid_sy, 4.0);
					}
					break;
		}

		case 9: {
					B_PlayerBullet* pb = (B_PlayerBullet*)d.ref;
					iSetColor(0, 255, 255);
					float tsx, tsy;
					B_worldToScreen(pb->x - pb->vx * 1.5f, pb->y - pb->vy * 1.5f, &tsx, &tsy);
					iLine(d.sx, d.sy, tsx, tsy);
					iLine(d.sx + 1, d.sy + 1, tsx + 1, tsy + 1);
					iLine(d.sx - 1, d.sy - 1, tsx - 1, tsy - 1);
					break;
		}
		}
	}
}

void B_drawFull() {
	if (b_gameState == B_STATE_PLAYING) {
		iSetColor(20, 20, 20); iFilledRectangle(0, 0, (float)SW, (float)SH);

		float startWy = ((int)B_player.y / 100) * 100 - 400.0f;
		for (float wy2 = startWy + 1400.0f; wy2 >= startWy; wy2 -= 100.0f) {
			for (float wx2 = -200.0f; wx2 <= 200.0f; wx2 += 100.0f) {
				float tsx1, tsy1, tsx2, tsy2, tsx3, tsy3, tsx4, tsy4;
				B_worldToScreen(wx2 - 50.0f, wy2 - 50.0f, &tsx1, &tsy1);
				B_worldToScreen(wx2 + 50.0f, wy2 - 50.0f, &tsx2, &tsy2);
				B_worldToScreen(wx2 + 50.0f, wy2 + 50.0f, &tsx3, &tsy3);
				B_worldToScreen(wx2 - 50.0f, wy2 + 50.0f, &tsx4, &tsy4);
				iSetColor(45, 50, 50);
				double fpx[4] = { tsx1, tsx2, tsx3, tsx4 }; double fpy[4] = { tsy1, tsy2, tsy3, tsy4 };
				iFilledPolygon(fpx, fpy, 4);
				iSetColor(25, 25, 25);
				iPolygon(fpx, fpy, 4);
				if (((int)(fabs(wx2) + fabs(wy2)) / 100) % 3 == 0) {
					iSetColor(35, 40, 40);
					iLine(tsx1 + 20, tsy1 + 10, tsx2 - 10, tsy2 + 5);
				}
			}
		}

		B_drawGame();

		char uiText[100];
		iSetColor(255, 0, 0);
		sprintf(uiText, "HEALTH: %d", B_player.hp);
		iText(30, SH - 40, uiText, GLUT_BITMAP_TIMES_ROMAN_24);

		iSetColor(0, 255, 255);
		if (B_waveActive) {
			sprintf(uiText, "WAVE %d ACTIVE! Enemies Left: %d", B_currentWave, B_enemiesRemaining);
		}
		else {
			sprintf(uiText, "WAVE %d CLEARED. Advance ->", B_currentWave - 1);
			if (B_currentWave == 1) sprintf(uiText, "ADVANCE FORWARD ->");
		}
		iText(30, SH - 80, uiText, GLUT_BITMAP_TIMES_ROMAN_24);

		B_drawCrosshair();
	}
	else if (b_gameState == B_STATE_GAMEOVER) {
		iSetColor(100, 0, 0); iFilledRectangle(0, 0, (float)SW, (float)SH);
		iSetColor(255, 255, 255);
		iText(SW / 2 - 80, SH / 2, "YOU DIED IN THE BUNKER", GLUT_BITMAP_TIMES_ROMAN_24);
		iText(SW / 2 - 80, SH / 2 - 40, "Click to Retry", GLUT_BITMAP_HELVETICA_18);
	}
	else if (b_gameState == B_STATE_CUTSCENE) {
		if (B_USE_IMAGES && B_imgCutscene[B_currentCutsceneFrame] >= 0) {
			iShowImage(0, 0, SW, SH, B_imgCutscene[B_currentCutsceneFrame]);
		}
		else {
			iSetColor(0, 0, 0); iFilledRectangle(0, 0, (float)SW, (float)SH);
			iSetColor(255, 255, 255);
			char slideText[50];
			sprintf(slideText, "Story Slide %d / %d", B_currentCutsceneFrame + 1, B_cutsceneFrames);
			iText(SW / 2 - 80, SH / 2, slideText, GLUT_BITMAP_TIMES_ROMAN_24);
		}
		iSetColor(255, 255, 0);
		iText(SW / 2 - 60, 50, "Click to Continue...", GLUT_BITMAP_HELVETICA_18);
	}
	else if (b_gameState == B_STATE_WON) {
		iSetColor(0, 16, 0); iFilledRectangle(0, 0, (float)SW, (float)SH);
		iSetColor(0, 232, 66);
		iText(SW / 2 - 100, SH / 2, "BUNKER MISSION SAFAL!", GLUT_BITMAP_TIMES_ROMAN_24);
	}
}

void B_iMouse(int button, int state, int mx, int my) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (b_gameState == B_STATE_GAMEOVER) {
			B_initGame();
			b_gameState = B_STATE_PLAYING;
		}
		else if (b_gameState == B_STATE_PLAYING) {
			int selectedEnemy = -1;
			int hitDamage = 0;
			float targetX = B_player.x;
			float targetY = B_player.y + 600.0f;

			for (int i = 0; i < 100; i++) {
				if (!B_enemies[i].active || B_enemies[i].isDead) continue;
				float sx2, sy2;
				B_worldToScreen(B_enemies[i].x, B_enemies[i].y, &sx2, &sy2);
				if (mx >= sx2 - 75 && mx <= sx2 + 75 && my >= sy2 && my <= sy2 + 220) {
					selectedEnemy = i;
					targetX = B_enemies[i].x;
					targetY = B_enemies[i].y;
					if (my >= sy2 + 175) {
						hitDamage = 100;
					}
					else {
						hitDamage = 34;
					}
					break;
				}
			}

			for (int i = 0; i<100; i++) {
				if (!B_pBullets[i].active) {
					playGunSound();
					float dx = targetX - B_player.x;
					float dy = targetY - B_player.y;
					float len = sqrtf(dx*dx + dy*dy);
					if (len == 0) len = 1;
					B_pBullets[i].active = 1;
					float frontOffset = 200.0f;
					float sideOffset = -105.0f;
					B_pBullets[i].x = B_player.x + (dx / len) * frontOffset;
					B_pBullets[i].y = B_player.y + (dy / len) * frontOffset;
					B_pBullets[i].x += (dy / len) * sideOffset;
					B_pBullets[i].y -= (dx / len) * sideOffset;
					B_pBullets[i].targetEnemy = selectedEnemy;
					B_pBullets[i].damagePayload = hitDamage;
					B_pBullets[i].vx = (dx / len) * 35.0f;
					B_pBullets[i].vy = (dy / len) * 35.0f;
					break;
				}
			}
		}
		else if (b_gameState == B_STATE_CUTSCENE) {
			B_currentCutsceneFrame++;
			if (B_currentCutsceneFrame >= B_cutsceneFrames) {
				b_gameState = B_STATE_WON;
			}
		}
	}
}

void B_iKeyboard(unsigned char key) {
	if (b_gameState == B_STATE_PLAYING && !B_player.isDead) {
		B_player.walkTimer++;
		if (B_player.walkTimer > 2) {
			if (B_player.walkFrame == -1) B_player.walkFrame = 0;
			else B_player.walkFrame = (B_player.walkFrame + 1) % 4;
			B_player.walkTimer = 0;
		}
		if (key == 'w' || key == 'W') {
			B_player.y += 20.0f;
			B_player.facingBack = 0;
		}
		else if (key == 's' || key == 'S') {
			B_player.y -= 20.0f;
			B_player.facingBack = 1;
		}
		else if (key == 'a' || key == 'A') {
			B_player.x -= 40.0f;
		}
		else if (key == 'd' || key == 'D') {
			B_player.x += 40.0f;
		}
		if (B_player.x < -150.0f) B_player.x = -150.0f;
		if (B_player.x > 150.0f) B_player.x = 150.0f;
	}
}

void B_iMouseMove(int mx, int my) {
	B_aimX = mx;
	B_aimY = my;
}

void B_iPassiveMouseMove(int mx, int my) {
	B_aimX = mx;
	B_aimY = my;
}

/* ══════════════════════════════════════════════════════════════
END OF LEVEL 2 CODE
══════════════════════════════════════════════════════════════ */


static void drawDivider(float x, float y, float w) {
	iSetColor(180, 140, 40); iLine(x, y, x + w * 0.15f, y);
	iSetColor(220, 180, 60); iLine(x + w*0.15f, y, x + w*0.85f, y);
	iSetColor(180, 140, 40); iLine(x + w*0.85f, y, x + w, y);
	iSetColor(255, 210, 80); iFilledCircle((int)(x + w / 2), (int)y, 3);
}

static void drawCredits() {
	iSetColor(4, 8, 4);   iFilledRectangle(0, 0, (float)SW, (float)SH);
	iSetColor(8, 14, 6);  iFilledRectangle(40, 30, SW - 80, SH - 60);
	iSetColor(14, 26, 10); iFilledRectangle(50, 40, SW - 100, SH - 80);

	iSetColor(160, 120, 30);
	iLine(50, 40, 50, 90);   iLine(50, 40, 100, 40);
	iLine(SW - 50, 40, SW - 50, 90); iLine(SW - 50, 40, SW - 100, 40);
	iLine(50, SH - 40, 50, SH - 90); iLine(50, SH - 40, 100, SH - 40);
	iLine(SW - 50, SH - 40, SW - 50, SH - 90); iLine(SW - 50, SH - 40, SW - 100, SH - 40);

	iSetColor(16, 8, 4);
	iFilledRectangle(SW / 2 - 300, SH - 100, 600, 46);
	iSetColor(60, 36, 10);
	iRectangle(SW / 2 - 300, SH - 100, 600, 46);

	iSetColor(20, 80, 20);
	txD(SW / 2 - 248, SH - 88, "MUKTIJODDHA: SHADHINOTA - 1971", GLUT_BITMAP_TIMES_ROMAN_24);
	iSetColor(80, 200, 80);
	txD(SW / 2 - 250, SH - 86, "MUKTIJODDHA: SHADHINOTA - 1971", GLUT_BITMAP_TIMES_ROMAN_24);

	drawDivider(SW / 2 - 220, SH - 112, 440);

	iSetColor(220, 180, 50);
	txD(SW / 2 - 58, SH - 150, "C R E D I T S", GLUT_BITMAP_TIMES_ROMAN_24);
	drawDivider(SW / 2 - 180, SH - 164, 360);

	iSetColor(140, 140, 120);
	txD(SW / 2 - 96, SH - 200, "DEVELOPED BY", GLUT_BITMAP_HELVETICA_18);
	drawDivider(SW / 2 - 260, SH - 214, 520);

	typedef struct { const char* name; const char* id; const char* role; int r, g, b; } CreditEntry;
	CreditEntry team[3] = {
		{ "Mohammad Waizur Rahman", "00724205101121", "Lead Developer & Game Design", 28, 180, 220 },
		{ "Hojjaifa Shikder", "00724205101115", "AI & Combat Systems", 80, 210, 80 },
		{ "Talha Ibne Zahid", "00724205101100", "Map Design & Graphics", 220, 160, 50 }
	};

	float cardW = 240, cardH = 110;
	float totalW = 3 * cardW + 2 * 20;
	float startX = SW / 2.0f - totalW / 2.0f;
	float cardY = SH - 370;

	for (int i = 0; i < 3; i++) {
		float cx = startX + i * (cardW + 20);

		iSetColor(0, 0, 0);
		iFilledRectangle(cx + 4, cardY - 4, cardW, cardH);

		iSetColor(10, 18, 8);
		iFilledRectangle(cx, cardY, cardW, cardH);

		iSetColor(team[i].r / 3, team[i].g / 3, team[i].b / 3);
		iRectangle(cx, cardY, cardW, cardH);
		iSetColor(team[i].r, team[i].g, team[i].b);
		iRectangle(cx + 1, cardY + 1, cardW - 2, cardH - 2);

		iSetColor(team[i].r / 2, team[i].g / 2, team[i].b / 2);
		iFilledRectangle(cx + 2, cardY + cardH - 14, cardW - 4, 12);

		char badge[4]; sprintf_s(badge, sizeof(badge), "#%d", i + 1);
		iSetColor(team[i].r, team[i].g, team[i].b);
		iFilledCircle((int)(cx + 18), (int)(cardY + cardH - 8), 10);
		iSetColor(0, 0, 0);
		txD(cx + 13, cardY + cardH - 13, badge, GLUT_BITMAP_HELVETICA_10);

		iSetColor(team[i].r, team[i].g, team[i].b);
		txD(cx + 10, cardY + 84, team[i].name, GLUT_BITMAP_HELVETICA_12);

		iSetColor(170, 170, 150);
		txD(cx + 10, cardY + 66, team[i].id, GLUT_BITMAP_HELVETICA_12);

		iSetColor(team[i].r / 2, team[i].g / 2, team[i].b / 2);
		iLine(cx + 8, cardY + 58, cx + cardW - 8, cardY + 58);

		iSetColor(190, 190, 165);
		txD(cx + 10, cardY + 42, team[i].role, GLUT_BITMAP_HELVETICA_10);

		iSetColor(100, 150, 100);
		txD(cx + 10, cardY + 22, "CSE, BUET", GLUT_BITMAP_HELVETICA_10);
	}

	drawDivider(SW / 2 - 260, cardY - 18, 520);

	iSetColor(80, 80, 60);
	txD(SW / 2 - 170, cardY - 40, "Computer Science & Engineering, BUET", GLUT_BITMAP_HELVETICA_12);

	iSetColor(0, 100, 50);
	iFilledRectangle(SW / 2 - 140, cardY - 62, 280, 12);
	iSetColor(200, 40, 40);
	iFilledCircle(SW / 2, (int)(cardY - 56), 16);
	iSetColor(0, 120, 60);
	iCircle(SW / 2, (int)(cardY - 56), 16);

	iSetColor(220, 200, 60);
	txD(SW / 2 - 64, cardY - 95, "JAI BANGLA!", GLUT_BITMAP_HELVETICA_18);

	drawMenuButton(SW / 2 - 120, 56, 240, 54, "BACK", hoverMenu == 4);
}

static void drawSettings() {
	iSetColor(4, 8, 4);   iFilledRectangle(0, 0, (float)SW, (float)SH);
	iSetColor(8, 14, 6);  iFilledRectangle(40, 30, SW - 80, SH - 60);
	iSetColor(14, 26, 10); iFilledRectangle(50, 40, SW - 100, SH - 80);

	iSetColor(160, 120, 30);
	iLine(50, 40, 50, 90);   iLine(50, 40, 100, 40);
	iLine(SW - 50, 40, SW - 50, 90); iLine(SW - 50, 40, SW - 100, 40);
	iLine(50, SH - 40, 50, SH - 90); iLine(50, SH - 40, 100, SH - 40);
	iLine(SW - 50, SH - 40, SW - 50, SH - 90); iLine(SW - 50, SH - 40, SW - 100, SH - 40);

	iSetColor(60, 36, 10); iFilledRectangle(SW / 2 - 160, SH - 100, 320, 46);
	iSetColor(60, 36, 10); iRectangle(SW / 2 - 160, SH - 100, 320, 46);
	iSetColor(20, 80, 20);
	txD(SW / 2 - 78, SH - 88, "SETTINGS", GLUT_BITMAP_TIMES_ROMAN_24);
	iSetColor(80, 200, 80);
	txD(SW / 2 - 80, SH - 86, "SETTINGS", GLUT_BITMAP_TIMES_ROMAN_24);

	drawDivider(SW / 2 - 200, SH - 112, 400);

	float rowY = SH - 240;
	float rowX = SW / 2 - 260;

	iSetColor(10, 18, 8);  iFilledRectangle(rowX, rowY, 520, 70);
	iSetColor(34, 60, 24); iRectangle(rowX, rowY, 520, 70);

	iSetColor(200, 180, 50);
	iFilledCircle((int)(rowX + 34), (int)(rowY + 28), 10);
	iLine(rowX + 44, rowY + 28, rowX + 44, rowY + 56);
	iLine(rowX + 44, rowY + 56, rowX + 60, rowY + 52);
	iLine(rowX + 60, rowY + 52, rowX + 60, rowY + 60);
	iFilledCircle((int)(rowX + 60), (int)(rowY + 60), 8);

	iSetColor(210, 210, 180);
	txD(rowX + 80, rowY + 44, "BACKGROUND MUSIC", GLUT_BITMAP_HELVETICA_18);
	iSetColor(130, 130, 110);
	txD(rowX + 80, rowY + 24, "Toggle in-game music on or off", GLUT_BITMAP_HELVETICA_12);

	float tbX = rowX + 390, tbY = rowY + 18, tbW = 100, tbH = 34;
	int mHov = (hoverMenu == 7);
	if (musicOn) {
		iSetColor(mHov ? 30 : 18, mHov ? 160 : 130, mHov ? 30 : 20);
		iFilledRectangle(tbX, tbY, tbW, tbH);
		iSetColor(60, 220, 60);
		iRectangle(tbX, tbY, tbW, tbH);
		iSetColor(50, 200, 50);
		iFilledCircle((int)(tbX + tbW - 14), (int)(tbY + tbH / 2), 12);
		iSetColor(255, 255, 255);
		txD(tbX + 10, tbY + 10, "ON", GLUT_BITMAP_HELVETICA_18);
	}
	else {
		iSetColor(mHov ? 100 : 70, mHov ? 20 : 14, 14);
		iFilledRectangle(tbX, tbY, tbW, tbH);
		iSetColor(160, 40, 40);
		iRectangle(tbX, tbY, tbW, tbH);
		iSetColor(160, 50, 50);
		iFilledCircle((int)(tbX + 14), (int)(tbY + tbH / 2), 12);
		iSetColor(200, 200, 200);
		txD(tbX + 32, tbY + 10, "OFF", GLUT_BITMAP_HELVETICA_18);
	}

	if (musicOn) {
		iSetColor(60, 180, 60);
		txD(rowX + 80, rowY + 6, "Music is currently: ENABLED", GLUT_BITMAP_HELVETICA_10);
	}
	else {
		iSetColor(180, 60, 60);
		txD(rowX + 80, rowY + 6, "Music is currently: MUTED", GLUT_BITMAP_HELVETICA_10);
	}

	drawDivider(SW / 2 - 200, rowY - 16, 400);

	iSetColor(70, 70, 55);
	txD(SW / 2 - 200, rowY - 44, "More settings coming in future updates...", GLUT_BITMAP_HELVETICA_12);

	drawMenuButton(SW / 2 - 120, 56, 240, 54, "BACK", hoverMenu == 4);
}


void iDraw(){
	int ww = glutGet(GLUT_WINDOW_WIDTH);
	int wh = glutGet(GLUT_WINDOW_HEIGHT);
	if (ww > 0 && wh > 0 && (ww != SW || wh != SH)) {
		SW = ww;
		SH = wh;
		glViewport(0, 0, SW, SH);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, SW, 0.0, SH, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	iClear();
	if (!initAudioDone) {
		initAudioDone = 1;
		imgComic = iLoadImage("co.jpg");
		imgPage = iLoadImage("page.jpg");
		imgSlide[0] = iLoadImage("s1.jpg");
		imgSlide[1] = iLoadImage("s2.jpg");
		imgSlide[2] = iLoadImage("s3.jpg");
		imgSlide[3] = iLoadImage("s4.jpg");
		mciSendStringA("open \"gun.wav\" alias gun", NULL, 0, NULL);
		if (musicOn) PlaySoundA("com.wav", NULL, SND_LOOP | SND_ASYNC);
	}

	if (gs == 0){
		if (imgPage >= 0) iShowImage(0, 0, SW, SH, imgPage);
		else {
			iSetColor(6, 12, 3); iFilledRectangle(0, 0, (float)SW, (float)SH);
			iSetColor(34, 188, 50); txD(SW / 2 - 200, SH - 100, "MUKTIJODDHA: SHADHINOTA - 1971", GLUT_BITMAP_TIMES_ROMAN_24);
		}

		float bx = SW - 300;
		float bw = 240, bh = 55;
		drawMenuButton(bx, 480, bw, bh, "START", hoverMenu == 0);
		drawMenuButton(bx, 410, bw, bh, "STORY", hoverMenu == 5);
		drawMenuButton(bx, 340, bw, bh, "SETTINGS", hoverMenu == 1);
		drawMenuButton(bx, 270, bw, bh, "INSTRUCTION", hoverMenu == 2);
		drawMenuButton(bx, 200, bw, bh, "CREDITS", hoverMenu == 3);
		drawMenuButton(bx, 130, bw, bh, "EXIT", hoverMenu == 6);
		return;
	}

	if (gs == 7){ drawSettings(); return; }

	if (gs == 8){
		iSetColor(0, 0, 0); iFilledRectangle(0, 0, SW, SH);
		iSetColor(255, 255, 255); txD(SW / 2 - 100, SH - 100, "INSTRUCTION", GLUT_BITMAP_TIMES_ROMAN_24);
		txD(SW / 2 - 250, SH / 2 + 50, "WASD: Move, C+1-7: Sub select, RMB drag=path", GLUT_BITMAP_HELVETICA_18);
		txD(SW / 2 - 250, SH / 2 + 10, "Click ANYWHERE to shoot. C: Command Menu.", GLUT_BITMAP_HELVETICA_18);
		txD(SW / 2 - 250, SH / 2 - 30, "LAKSHO: Sab camp dhwangs koro!", GLUT_BITMAP_HELVETICA_18);
		drawMenuButton(SW / 2 - 120, 100, 240, 60, "BACK", hoverMenu == 4);
		return;
	}

	if (gs == 9){ drawCredits(); return; }

	/* ══════════════════════════
	gs == 10  →  ENDING SCREEN
	e.jpg full screen, 7 saniy
	══════════════════════════ */
	if (gs == 10) {
		if (imgEnding < 0) imgEnding = iLoadImage("e.jpg");
		if (imgEnding >= 0) {
			iShowImage(0, 0, SW, SH, imgEnding);
		}
		else {
			/* fallback: e.jpg na thakle simple screen */
			iSetColor(0, 16, 0); iFilledRectangle(0, 0, (float)SW, (float)SH);
			iSetColor(0, 232, 66);
			txD(SW / 2 - 200, SH / 2 + 20, "JAI BANGLA! SHADHINOTA ARJON HOYECHE!", GLUT_BITMAP_TIMES_ROMAN_24);
			iSetColor(0, 180, 50);
			txD(SW / 2 - 130, SH / 2 - 30, "Sab Pak sena dhwangs hoyeche!", GLUT_BITMAP_HELVETICA_18);
		}
		/* progress bar niche */
		float prog = (float)endingTimer / ENDING_TICKS;
		if (prog > 1.0f) prog = 1.0f;
		iSetColor(20, 20, 20);   iFilledRectangle(SW / 2 - 150, 18, 300, 10);
		iSetColor(40, 180, 40);  iFilledRectangle(SW / 2 - 150, 18, 300 * prog, 10);
		iSetColor(60, 220, 60);  iRectangle(SW / 2 - 150, 18, 300, 10);
		return;
	}

	if (gs == 6) {
		int si = comicSlide;
		if (si < 0) si = 0; if (si > COMIC_TOTAL_SLIDES - 1) si = COMIC_TOTAL_SLIDES - 1;
		if (imgSlide[si] >= 0) {
			iShowImage(0, 0, SW, SH, imgSlide[si]);
		}
		else {
			iSetColor(0, 0, 0); iFilledRectangle(0, 0, (float)SW, (float)SH);
			iSetColor(255, 255, 255);
			char slideBuf[40]; sprintf_s(slideBuf, sizeof(slideBuf), "SLIDE %d / %d", si + 1, COMIC_TOTAL_SLIDES);
			txD(SW / 2 - 40, SH / 2, slideBuf, GLUT_BITMAP_HELVETICA_18);
		}
		for (int k = 0; k < COMIC_TOTAL_SLIDES; k++) {
			if (k == si) { iSetColor(255, 220, 50); iFilledCircle(SW / 2 - (COMIC_TOTAL_SLIDES - 1) * 10 + k * 20, 30, 6); }
			else { iSetColor(120, 120, 100); iFilledCircle(SW / 2 - (COMIC_TOTAL_SLIDES - 1) * 10 + k * 20, 30, 5); }
		}
		float prog = (float)comicTimer / COMIC_AUTO_TICKS;
		if (prog > 1.0f) prog = 1.0f;
		iSetColor(40, 40, 40); iFilledRectangle(SW / 2 - 150, 8, 300, 8);
		iSetColor(200, 180, 50); iFilledRectangle(SW / 2 - 150, 8, 300 * prog, 8);
		float skipX = SW - 180, skipY = SH - 60, skipW = 140, skipH = 40;
		if (comicHoverSkip) {
			iSetColor(220, 80, 40); iFilledRectangle(skipX, skipY, skipW, skipH);
			iSetColor(255, 120, 60); iRectangle(skipX, skipY, skipW, skipH);
		}
		else {
			iSetColor(160, 50, 30); iFilledRectangle(skipX, skipY, skipW, skipH);
			iSetColor(200, 80, 50); iRectangle(skipX, skipY, skipW, skipH);
		}
		iSetColor(255, 255, 220);
		if (si < COMIC_TOTAL_SLIDES - 1) txD(skipX + 30, skipY + 12, "SKIP >>>", GLUT_BITMAP_HELVETICA_18);
		else txD(skipX + 20, skipY + 12, "START >>>", GLUT_BITMAP_HELVETICA_18);
		return;
	}

	if (gs == 4){ iSetColor(0, 0, 0); iFilledRectangle(0, 0, (float)SW, (float)SH); iSetColor(232, 232, 0); txD(348, 392, "BIRATI", GLUT_BITMAP_TIMES_ROMAN_24); txD(288, 352, "ESC chap: chalu koro", GLUT_BITMAP_HELVETICA_18); return; }
	if (gs == 2){ iSetColor(16, 0, 0); iFilledRectangle(0, 0, (float)SW, (float)SH); iSetColor(232, 34, 34); txD(254, 422, "MISSION BYARTHO", GLUT_BITMAP_TIMES_ROMAN_24); return; }
	if (gs == 3){ iSetColor(0, 16, 0); iFilledRectangle(0, 0, (float)SW, (float)SH); iSetColor(0, 232, 66); txD(172, 422, "MISSION SAFAL!", GLUT_BITMAP_TIMES_ROMAN_24); return; }
	if (gs == 5){ B_drawFull(); return; }

	drawMap();
	drawPaths();
	drawBoms(); drawBuls(); drawPars();
	for (int i = 0; i<NV; i++)drawVeh(i);
	for (int i = 0; i<NE; i++)drawEne(i);
	for (int h = NH - 1; h >= 0; h--)drawHero(h);

	drawHUD();
	drawMM();
	drawZoomButtons();
	if (cO)drawCmdMenu();
	drawPathCursor(curMX, curMY);
}

void updatePhysics(){
	/* ── GUN TIMER ── */
	if (gunFireTimer > 0) {
		gunFireTimer--;
		if (gunFireTimer == 0 && isGunPlaying) {
			isGunPlaying = 0;
			mciSendStringA("stop gun", NULL, 0, NULL);
			mciSendStringA("seek gun to start", NULL, 0, NULL);
		}
	}

	/* Comic slideshow auto-advance */
	if (gs == 6) {
		comicTimer++;
		if (comicTimer >= COMIC_AUTO_TICKS) {
			comicTimer = 0;
			comicSlide++;
			if (comicSlide >= COMIC_TOTAL_SLIDES) {
				comicSlide = 0;
				if (comicFromMenu) { gs = 0; }
				else {
					PlaySoundA(NULL, 0, 0);
					if (musicOn) PlaySoundA("na.wav", NULL, SND_LOOP | SND_ASYNC);
					resetGame();
				}
			}
		}
		return;
	}

	/* ══════════════════════════════════════
	gs == 10  →  ENDING SCREEN COUNTDOWN
	438 tick (~7s) por exit(0)
	══════════════════════════════════════ */
	if (gs == 10) {
		endingTimer++;
		if (endingTimer >= ENDING_TICKS) {
			PlaySoundA(NULL, 0, 0);   /* sob sound bondho */
			exit(0);
		}
		return;
	}

	if (gs == 5) { B_updateGameState(); return; }
	if (gs != 1)return; tick++;
	if (tick % 60 == 0){
		for (int h = 0; h < NM; h++){
			if (H[h].alive && H[h].hp < H[h].mhp) H[h].hp += 3;
		}
	}

	float viewW = (float)SW / gZoom;
	float viewH = (float)(SH - HUD_H) / gZoom;
	float tcx = H[act].x - viewW / 2.f;
	float tcy = H[act].y - viewH / 2.f;
	camX += (tcx - camX)*0.13f; camY += (tcy - camY)*0.13f;
	float maxCamX = WW - viewW; if (maxCamX < 0) maxCamX = 0;
	if (camX > maxCamX) camX = maxCamX;
	if (camX < 0) camX = 0;

	float maxCamY = WH - viewH; if (maxCamY < 0) maxCamY = 0;
	if (camY > maxCamY) camY = maxCamY;
	if (camY < 0) camY = 0;

	for (int h = NM; h<NH; h++)updS(h);

	for (int k = 0; k<7; k++) {
		if (!tentsData[k].spawned) {
			for (int h = 0; h<NH; h++) {
				if (H[h].alive && fdst(H[h].x, H[h].y, tentsData[k].x, tentsData[k].y) < 200.f) {
					tentsData[k].spawned = 1;
					spawnEnemy(tentsData[k].x, tentsData[k].y, 1);
					break;
				}
			}
		}
	}

	if (bldgSpawnCount < 7) {
		bldgSpawnTimer++;
		if (bldgSpawnTimer > 300) {
			bldgSpawnTimer = 0;
			spawnEnemy(cWX(33), mWY(19), 1);
			bldgSpawnCount++;
		}
	}

	for (int i = 0; i<NBL; i++){
		if (!BL[i].active)continue; BL[i].x += BL[i].vx; BL[i].y += BL[i].vy; BL[i].lf--;
		if (BL[i].lf <= 0){ BL[i].active = 0; continue; }
		int bt = tW(BL[i].x, BL[i].y);
		if (solid(bt)){ spP(BL[i].x, BL[i].y, 250, 195, 95, 4); BL[i].active = 0; continue; }
		if (BL[i].en){
			for (int h = 0; h<NH; h++){
				if (!H[h].alive || H[h].iv >= 0)continue;
				if (fdst(BL[i].x, BL[i].y, H[h].x, H[h].y)<15){ H[h].hp -= 14; spP(H[h].x, H[h].y, 250, 145, 0, 5); if (H[h].hp <= 0){ H[h].alive = 0; } BL[i].active = 0; break; }
			}
		}
		else {
			for (int j = 0; j<NE; j++){
				if (!E[j].alive)continue;
				if (fdst(BL[i].x, BL[i].y, E[j].x, E[j].y)<18){
					E[j].hp -= 26.f; spP(E[j].x, E[j].y, 250, 46, 46, 6);
					if (!E[j].al) { E[j].al = 1; E[j].at = 262; }
					int aw = 0;
					for (int ww = 0; ww<NE; ww++) {
						if (ww != j && E[ww].alive && !E[ww].al && fdst(E[j].x, E[j].y, E[ww].x, E[ww].y)<500.f) {
							E[ww].al = 1; E[ww].at = 400; aw++; if (aw >= 15) break;
						}
					}
					if (E[j].hp <= 0){ E[j].alive = 0; }
					BL[i].active = 0; break;
				}
			}
		}
	}
	for (int i = 0; i<NO; i++){
		if (OBJ[i].done)continue;
		for (int h = 0; h<NH; h++){
			if (!H[h].alive)continue;
			if (fdst(H[h].x, H[h].y, OBJ[i].x, OBJ[i].y)<28){ OBJ[i].done = 1; oDone++; spP(OBJ[i].x, OBJ[i].y, 250, 210, 0, 14); }
		}
	}
	for (int i = 0; i<NP; i++){ if (PR[i].life <= 0)continue; PR[i].x += PR[i].vx; PR[i].y += PR[i].vy; PR[i].vx *= 0.87f; PR[i].vy *= 0.87f; PR[i].life--; }
	if (alT>0)alT--; if (alT == 0 && alLv>0)alLv--;

	for (int i = 0; i < NE; i++){
		Ene* e = &E[i];
		if (!e->alive) continue;
		if (e->at > 0) e->at--;
		if (e->at == 0) e->al = 0;
		if (e->sc > 0) e->sc--;

		int targetH = -1;
		float hBestD = 320.0f;

		for (int h = 0; h < NH; h++){
			if (H[h].alive && H[h].iv < 0){
				float d = fdst(e->x, e->y, H[h].x, H[h].y);
				float visibilityRange = H[h].snk ? hBestD * 0.4f : hBestD;
				if (d < visibilityRange && hasLOS(e->x, e->y, H[h].x, H[h].y)){
					hBestD = d; targetH = h;
				}
			}
		}

		if (targetH >= 0){
			e->al = 1;
			for (int aw2 = 0; aw2 < NE; aw2++) {
				if (aw2 != i && E[aw2].alive && !E[aw2].al && fdst(e->x, e->y, E[aw2].x, E[aw2].y) < 450.f) {
					E[aw2].al = 1; E[aw2].at = 400;
				}
			}
			float tx = H[targetH].x, ty = H[targetH].y;
			e->a = R2D(atan2f(ty - e->y, tx - e->x));

			if (hBestD > 70.0f){
				float ang = atan2f(ty - e->y, tx - e->x);
				float nx = e->x + cosf(ang) * e->spd * 1.3f;
				float ny = e->y + sinf(ang) * e->spd * 1.3f;
				if (canWlk(nx, ny, 10.0f)){ e->x = nx; e->y = ny; }
			}

			if (e->sc == 0){
				fBul(e->x, e->y, tx, ty, 1, 0);
				e->sc = 40; e->at = 400; alLv = 2; alT = 422;
			}
		}
		else {
			if (e->al) {
				int bh = -1; float bhd = 600.f;
				for (int h = 0; h < NH; h++) {
					if (H[h].alive) { float dd = fdst(e->x, e->y, H[h].x, H[h].y); if (dd < bhd) { bhd = dd; bh = h; } }
				}
				if (bh >= 0) {
					float ang2 = atan2f(H[bh].y - e->y, H[bh].x - e->x);
					float nx2 = e->x + cosf(ang2) * e->spd * 1.5f;
					float ny2 = e->y + sinf(ang2) * e->spd * 1.5f;
					if (canWlk(nx2, ny2, 10.f)) { e->x = nx2; e->y = ny2; }
					e->a = R2D(ang2);
				}
			}
			else {
				float ptx = e->me ? e->ex : e->sx, pty = e->me ? e->ey : e->sy;
				float pdx = ptx - e->x, pdy = pty - e->y, pd = flen(pdx, pdy);
				if (pd < 5) {
					for (int tr = 0; tr<10; tr++) {
						float nx = e->x + (rand() % 160 - 80);
						float ny = e->y + (rand() % 160 - 80);
						if (canWlk(nx, ny, 10.f)) {
							if (e->me) { e->ex = nx; e->ey = ny; }
							else { e->sx = nx; e->sy = ny; }
							break;
						}
					}
					e->me = !e->me;
				}
				else {
					e->x += (pdx / pd) * (e->spd * 0.4f);
					e->y += (pdy / pd) * (e->spd * 0.4f);
					e->a = R2D(atan2f(pdy, pdx));
				}
			}
		}
	}

	/* Hero sob mare gele FAIL */
	int any = 0; for (int h = 0; h<NM; h++)if (H[h].alive)any = 1;
	if (!any){ gs = 2; return; }

	/* ══════════════════════════════════════════════
	Sob enemy mare gele → ENDING SCREEN (gs=10)
	Prothom 300 tick (5s) spawn hote dewa hoy,
	take ignore kora hoy.
	══════════════════════════════════════════════ */
	if (tick > 300) {
		int anyEnemy = 0;
		for (int i = 0; i < NE; i++) {
			if (E[i].alive) { anyEnemy = 1; break; }
		}
		if (!anyEnemy) {
			PlaySoundA(NULL, 0, 0);
			if (musicOn) PlaySoundA("win.wav", NULL, SND_ASYNC);  /* optional: jodi win.wav thake */
			if (imgEnding < 0) imgEnding = iLoadImage("e.jpg");
			endingTimer = 0;
			gs = 10;
		}
	}
}

void iKeyboard(unsigned char key){
	if (gs == 5) {
		if (key == 27) { gs = 0; return; }
		B_iKeyboard(key);
		return;
	}
	if (key == '\r'){ if (gs == 0) { gs = 6; comicSlide = 0; comicTimer = 0; comicFromMenu = 0; } else if (gs != 1)resetGame(); return; }
	if (key == 27){
		if (gs == 6) {
			if (comicFromMenu) { gs = 0; return; }
			PlaySoundA(NULL, 0, 0);
			if (musicOn) PlaySoundA("na.wav", NULL, SND_LOOP | SND_ASYNC);
			resetGame();
			return;
		}
		if (cO){ cO = 0; cPh = 0; return; }
		if (cSub >= 0) { cSub = -1; return; }
		if (gs == 1)gs = 4; else if (gs == 4)gs = 1; else if (gs == 7 || gs == 8 || gs == 9) gs = 0; else if (gs == 0) exit(0); return;
	}
	if (gs == 1){
		if (key == 'z' || key == 'Z'){ gZoom += ZOOM_STEP; if (gZoom>ZOOM_MAX)gZoom = ZOOM_MAX; return; }
		if (key == 'v' || key == 'V'){ gZoom -= ZOOM_STEP; if (gZoom<ZOOM_MIN)gZoom = ZOOM_MIN; return; }
	}
	if (gs != 1)return;

	if (key == 'f' || key == 'F') {
		float d = fdst(H[act].x, H[act].y, cWX(52), mWY(8));
		if (d < 70.f) {
			PlaySoundA(NULL, 0, 0);
			if (musicOn) PlaySoundA("music.wav", NULL, SND_LOOP | SND_ASYNC);
			gs = 5;
			B_loadAllImages();
			B_initGame();
			b_gameState = B_STATE_PLAYING;
		}
	}

	if (cO){
		if (cPh == 0){
			if (key >= '1'&&key <= '7'){
				cSub = key - '1'; cPh = 1;
			}
		}
		else {
			int hi = NM + cSub;
			if (key == 'm' || key == 'M'){ H[hi].cmd = CM; H[hi].dx = H[act].x + (cSub - 3)*26.f; H[hi].dy = H[act].y + 40.f; }
			else if (key == 'a' || key == 'A') H[hi].cmd = CA;
			else if (key == 's' || key == 'S') H[hi].cmd = CS;
			else if (key == 'f' || key == 'F') H[hi].cmd = CF;
			else if (key == 'g' || key == 'G') H[hi].cmd = CG;
			cO = 0; cPh = 0;
		}
		return;
	}
	if (key == 'w' || key == 'W'){ mvH(act, 0, 1); return; }
	if (key == 's' || key == 'S')mvH(act, 0, -1);
	if (key == 'a' || key == 'A')mvH(act, -1, 0);
	if (key == 'd' || key == 'D')mvH(act, 1, 0);
	if (key == 'k' || key == 'K')ulkGt(act);
	if (key == 'c' || key == 'C'){
		if (cO) { cO = 0; cPh = 0; cSub = -1; }
		else { cO = 1; cPh = 0; strcpy_s(LOG, sizeof(LOG), "Command: 1-7 chap."); }
	}
}

void iSpecialKeyboard(unsigned char key){
	if (gs != 1 || cO)return;
	if (key == GLUT_KEY_UP)mvH(act, 0, 1);
	if (key == GLUT_KEY_DOWN)mvH(act, 0, -1);
	if (key == GLUT_KEY_LEFT)mvH(act, -1, 0);
	if (key == GLUT_KEY_RIGHT)mvH(act, 1, 0);
}

void iMouse(int button, int state, int mx, int my){
	my = my + (SH - 650);
	if (gs == 5) { B_iMouse(button, state, mx, my); return; }

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (gs == 0) {
			if (hoverMenu == 0) { gs = 6; comicSlide = 0; comicTimer = 0; comicFromMenu = 0; }
			else if (hoverMenu == 5) { gs = 6; comicSlide = 0; comicTimer = 0; comicFromMenu = 1; }
			else if (hoverMenu == 1) gs = 7;
			else if (hoverMenu == 2) gs = 8;
			else if (hoverMenu == 3) gs = 9;
			else if (hoverMenu == 6) exit(0);
			return;
		}
		else if (gs == 6) {
			float skipX = SW - 180, skipY = SH - 60, skipW = 140, skipH = 40;
			if (mx >= skipX && mx <= skipX + skipW && my >= skipY && my <= skipY + skipH) {
				comicSlide++; comicTimer = 0;
				if (comicSlide >= COMIC_TOTAL_SLIDES) {
					comicSlide = 0;
					if (comicFromMenu) { gs = 0; }
					else {
						PlaySoundA(NULL, 0, 0);
						if (musicOn) PlaySoundA("na.wav", NULL, SND_LOOP | SND_ASYNC);
						resetGame();
					}
				}
			}
			return;
		}
		else if (gs == 7) {
			if (hoverMenu == 4) { gs = 0; return; }
			if (hoverMenu == 7) {
				musicOn = !musicOn;
				if (musicOn) {
					PlaySoundA("com.wav", NULL, SND_LOOP | SND_ASYNC);
				}
				else {
					PlaySoundA(NULL, 0, 0);
				}
				return;
			}
			return;
		}
		else if (gs == 8 || gs == 9) {
			if (hoverMenu == 4) gs = 0;
			return;
		}
	}

	if (gs != 1)return;
	if (cO&&state == GLUT_DOWN){ cO = 0; cPh = 0; return; }

	if (button == GLUT_LEFT_BUTTON&&state == GLUT_DOWN){
		if (mx >= ZB_X&&mx <= ZB_X + 26 && my >= ZB_Y + 22 && my <= ZB_Y + 42){ gZoom += ZOOM_STEP; if (gZoom>ZOOM_MAX)gZoom = ZOOM_MAX; return; }
		if (mx >= ZB_X&&mx <= ZB_X + 26 && my >= ZB_Y&&my <= ZB_Y + 20){ gZoom -= ZOOM_STEP; if (gZoom<ZOOM_MIN)gZoom = ZOOM_MIN; return; }
	}

	if (my <= HUD_H){
		if (button == GLUT_LEFT_BUTTON&&state == GLUT_DOWN){
			for (int i = 0; i<NS + NA; i++){
				if (mx >= (int)(250.f + i*64.f) && mx <= (int)(250.f + i*64.f + 62)){
					cSub = i; cO = 0; cPh = 0; return;
				}
			}
		}
		return;
	}

	if (button == GLUT_RIGHT_BUTTON){
		if (state == GLUT_DOWN){
			if (cSub >= 0){
				drawingPath = 1; drawPathSub = cSub;
				subPath[cSub].count = 1; subPath[cSub].cur = 0;
				subPath[cSub].x[0] = (float)mx / gZoom + camX;
				subPath[cSub].y[0] = (float)my / gZoom + camY;
			}
			else togV(act);
		}
		if (state == GLUT_UP){
			if (drawingPath&&drawPathSub >= 0){
				int hi = NM + drawPathSub;
				if (subPath[drawPathSub].count >= 2){
					H[hi].cmd = CP; subPath[drawPathSub].cur = 0;
				}
				drawingPath = 0; drawPathSub = -1;
			}
		}
	}

	if (button == GLUT_LEFT_BUTTON&&state == GLUT_DOWN){
		float wx = (float)mx / gZoom + camX;
		float wy = (float)my / gZoom + camY;
		fBul(H[act].x, H[act].y, wx, wy, 0, H[act].role == 2 ? 2 : selW);
		H[act].a = R2D(atan2f(wy - H[act].y, wx - H[act].x));
	}
}

void iPassiveMouseMove(int mx, int my){
	my = my + (SH - 650);
	if (gs == 5) { B_iPassiveMouseMove(mx, my); return; }
	hoverMenu = -1;
	if (gs == 0) {
		float bx = SW - 300; float bw = 240, bh = 55;
		if (mx >= bx && mx <= bx + bw) {
			if (my >= 480 && my <= 480 + bh) hoverMenu = 0;
			else if (my >= 410 && my <= 410 + bh) hoverMenu = 5;
			else if (my >= 340 && my <= 340 + bh) hoverMenu = 1;
			else if (my >= 270 && my <= 270 + bh) hoverMenu = 2;
			else if (my >= 200 && my <= 200 + bh) hoverMenu = 3;
			else if (my >= 130 && my <= 130 + bh) hoverMenu = 6;
		}
	}
	else if (gs == 6) {
		comicHoverSkip = 0;
		float skipX = SW - 180, skipY = SH - 60, skipW = 140, skipH = 40;
		if (mx >= skipX && mx <= skipX + skipW && my >= skipY && my <= skipY + skipH) comicHoverSkip = 1;
	}
	else if (gs == 7) {
		float bbx = SW / 2 - 120; float bbw = 240, bbh = 54;
		if (mx >= bbx && mx <= bbx + bbw && my >= 56 && my <= 56 + bbh) { hoverMenu = 4; }
		float tbX = (SW / 2 - 260) + 390, tbY = (SH - 240) + 18, tbW = 100, tbH = 34;
		if (mx >= tbX && mx <= tbX + tbW && my >= tbY && my <= tbY + tbH) { hoverMenu = 7; }
	}
	else if (gs == 8 || gs == 9) {
		float bx2 = SW / 2 - 120; float bw2 = 240, bh2 = 60;
		if (mx >= bx2 && mx <= bx2 + bw2 && my >= 100 && my <= 100 + bh2) hoverMenu = 4;
	}

	curMX = mx; curMY = my; if (gs != 1)return;
	Hero*u = &H[act]; if (u->iv >= 0)return;
	float wx = (float)mx / gZoom + camX, wy = (float)my / gZoom + camY;
	u->a = R2D(atan2f(wy - u->y, wx - u->x));
}

void iMouseMove(int mx, int my){
	my = my + (SH - 650);
	if (gs == 5) { B_iMouseMove(mx, my); return; }
	curMX = mx; curMY = my; if (gs != 1)return;
	if (drawingPath&&drawPathSub >= 0 && my>HUD_H){
		PathData*pd = &subPath[drawPathSub];
		if (pd->count<MAX_PATH_PTS){
			float wx = (float)mx / gZoom + camX, wy = (float)my / gZoom + camY;
			if (fdst(wx, wy, pd->x[pd->count - 1], pd->y[pd->count - 1]) >= TILE*0.6f){
				pd->x[pd->count] = wx; pd->y[pd->count] = wy; pd->count++;
			}
		}
	}
}

int main(){
	srand((unsigned)time(NULL));

	SW = 900;
	SH = 650;

	iInitialize(SW, SH, "Muktijoddha: Shadhinota - 1971");
	glutFullScreen();
	iSetTimer(16, updatePhysics);
	iStart();
	return 0;
}