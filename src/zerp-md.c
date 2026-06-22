/* First version to be written to a cart */

#include <genesis.h>
#include <timer.h>
#include <string.h>
#include "resources.h"

#define SND(f) PSG_setTone(0, 111861/f); PSG_setEnvelope(0, 0);

char mylog(u16 num) {
	char n = 0;
    if (num == 0) return -1;
    while (num >>= 1) n++;
    return n;
}

u16 drawnum(u32 val, u16 x, u16 y) {
	char str[12];
	intToStr(val, str, 1);
	VDP_drawText(str, x, y);
	return strlen(str);
}

u16 drawhex(u32 val, u16 x, u16 y) {
	char str[12];
	intToHex(val, str, 1);
	VDP_drawText(str, x, y);
	return strlen(str);
}

char mytoggle = 0;
u16 mygetch(void) {
    static u16 previousState = 0;
    u16 currentState = 0;
	u16 mytimer = 0;
    while (1) {
        currentState = JOY_readJoypad(JOY_1);
        u16 newPress = (currentState & (~previousState));
        previousState = currentState; 
        if (newPress != 0) { 
			mytoggle = 0;
			return mylog(newPress);
		}
		if (mytimer > 2) PSG_setEnvelope(0, 15);
		if (mytoggle && mytimer > 3 && currentState)
			return mylog(currentState);	
		if (mytimer > 16 && currentState){
			mytoggle = 1;
			return mylog(currentState);
		}
		if (mytimer > 3) mytoggle = 0;
        SYS_doVBlankProcess();
		mytimer++;
    }
}

void mydraw(const char *str, u16 x, u16 y) {
    u16 i = 0;
    while(str[i] != '\0') {
        u16 tile_to_draw = 16 + (str[i] < 'Z' ? str[i] - 'A' : str[i] - 'a' + 17); 
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, 0, 0, 0, tile_to_draw), x + i++, y);
    }
}

#define WIDTH 20
#define HEIGHT 10
#define BOMBS 3
#define MOBS 3
#define RIDEABLE_MOBS 1

unsigned char field[HEIGHT][WIDTH];
unsigned char stored_field[HEIGHT][WIDTH];
u32 seed;
char facing, air, super;
int stored_pos[2];
char style = 0;

static u32 my_seed;
void my_srand(u32 s){ my_seed = s; }
u32 my_rand(){
	my_seed = my_seed * 1664525 + 1013904223;
	return my_seed >> 16;
}

char alt_climb(const int p_x, const int p_y){
	if (facing && p_x < WIDTH - 1 && (field[p_y][p_x + 1] < 3 || super) && field[p_y + 1][p_x + 1]){
		return 3;
	}
	if (p_x > 1 && (field[p_y][p_x - 1] < 3 || super) && field[p_y + 1][p_x - 1]){
		return 2;
	}
	if (p_x < WIDTH - 1 && (field[p_y][p_x + 1] < 3 || super) && field[p_y + 1][p_x + 1]){
		return 3;
	}
	return 0;
}

int monsters[MOBS + 27][3];

void delmons(int y, int x){
	int i;
	for (i = 0; i < MOBS + 27; i++)
		if (monsters[i][0] == x && monsters[i][1] == y){
			monsters[i][0] = 0;
			monsters[i][1] = 0;
			break;
		}
}

void load_m(void){
	int k = 0, i, j;
	for (i = 0; i < MOBS + 27; i++){
		monsters[i][0] = 0;
		monsters[i][1] = 0;
		monsters[i][2] = 0;
	}
	for (i = 0; i < HEIGHT; i++)
		for (j = 1; j < WIDTH; j++)
			if (field[i][j] == 'q'){
				monsters[k][0] = j;
				monsters[k][1] = i;
				monsters[k][2] = my_rand() % 4;
				if (++k >= MOBS + 27) return;
			}
}

int win_condition(void){
	int i, j;
	for (i = 0; i < HEIGHT; i++)
		for (j = 1; j < WIDTH; j++)
			if (field[i][j] == 1) return 0;
	return 1;
}

void store_field(void){
	int i, j;
	for (i = 0; i < HEIGHT; i++)
		for (j = 0; j < WIDTH; j++)
			stored_field[i][j] = field[i][j];
}

void load_field(void){
	int i, j;
	for (i = 0; i < HEIGHT; i++)
		for (j = 0; j < WIDTH; j++)
			field[i][j] = stored_field[i][j];
}

char *campaign[] = {
"0","0\n\n\
    @     #\n\
#######   #\n\
      #   !\n\
      #######!#####\n\
             !\n\
 !  !\n\
#######\n\
      #          !\n\
      #############",
"0\n\n\n\
 #####      ######\n\
 #   ########  ! #\n\
 # @        % ####\n\
 #####!####!###\n\
     ###  ###\n\n\n",
"1\n\n\n\
       #####\n\
       #   #\n\
       #   #\n\
       # @ #\n\
       #####\n\
   #           #\n\
   #!!!!!!!!!!!#\n\
   #!!!!!!!!!!!#",
"0\n\n\
              !\n\
\n\
       #\n\
     #        !\n\
\n\
    #    #   !\n\
      #  #     !\n\
         #\n\
     # @ #    !",
""
};

int open_level(int n){
	char *ptr = campaign[n];
	char start_bombs;
	int i, j;
	if (n > 4) return -1;
	start_bombs = *ptr - '0';
	ptr += 2;
	for (i = 0; i < HEIGHT; i++)
		for (j = 0; j < WIDTH; j++)
			field[i][j] = 0;
	for (i = 0; *ptr && i < HEIGHT; i++, ptr++)
		for(j = 1; *ptr && *ptr != '\n' && j < WIDTH; j++, ptr++)
			switch (*ptr){
				case ' ': field[i][j] =  0 ; break;
				case '!': field[i][j] =  1 ; break;
				case '#': field[i][j] =  3 ; break;
				case '%': field[i][j] = 'q'; break;
				case '@': 
					field[i][j] = '@';
					stored_pos[0] = j;
					stored_pos[1] = i;
					break;
				default:
					field[i][j] = 0;
			}
	load_m();
	return start_bombs;
}

void shuffle(int points[][2], int n){
	int i, j, temp0, temp1;
	for (i = n - 1; i > 0; i--){
		j = my_rand() % (i + 1);
		temp0 = points[i][0];
		temp1 = points[i][1];
		points[i][0] = points[j][0];
		points[i][1] = points[j][1];
		points[j][0] = temp0;
		points[j][1] = temp1;
	}
}

void make_field(void){
	int x, y, m = 0, i, flag, c, *p, *l;
	int points[WIDTH * HEIGHT][2];

	my_srand(seed);
	for (x = 0; x < WIDTH; x++){
		for (y = 0; y < HEIGHT; y++){
			points[x + y * WIDTH][0] = x;
			points[x + y * WIDTH][1] = y;
		}
	}
	for(p = (int*)field, l = p + sizeof(field)/sizeof(int); p < l; *p++ = 0);

	shuffle(points, WIDTH * HEIGHT);
	for (i = 0; i < WIDTH * HEIGHT; i++){
		int check[8][2];
		x = points[i][0] + 1;
		y = points[i][1];
		check[0][0] = x + 1;  check[0][1] = y + 1;
		check[1][0] = x - 1;  check[1][1] = y - 1;
		check[2][0] = x - 1;  check[2][1] = y + 1;
		check[3][0] = x + 1;  check[3][1] = y - 1;
		check[4][0] = x;      check[4][1] = y + 1;
		check[5][0] = x;      check[5][1] = y - 1;
		check[6][0] = x + 1;  check[6][1] = y;
		check[7][0] = x - 1;  check[7][1] = y;
		flag = 0;
		
		for (c = 0; c < 8; c++)
			if (check[c][0] >  0 && check[c][0] < WIDTH && 
				check[c][1] >= 0 && check[c][1] < HEIGHT)
				if (field[check[c][1]][check[c][0]])
					if (field[check[c][1]][check[c][0]] != 'q') flag++;
			
		if (flag == 0) field[y][x] = 3;
		if (flag == 1) field[y][x] = 3;
		if (flag == 2) field[y][x] = 1;
		if (flag > 2 && m < MOBS && (x + 1 < WIDTH / 2 || x - 1 > WIDTH / 2)){
			field[y][x] = 'q';
			m++;
		}
	}
	x = WIDTH / 2;
	y = HEIGHT - 1;
	while (field[y][x] && y > 0) y--;
	field[y][x] = '@';
	stored_pos[0] = x;
	stored_pos[1] = y;
	load_m();
}

void undrawat(int x, int y){
	if (field[y][x]) return;
	VDP_drawText("  ", x * 2 - 1, y * 2 + 4);
	VDP_drawText("  ", x * 2 - 1, y * 2 + 5);
}

void drawmob(int x, int y){
	mydraw("ab", x * 2 - 1, y * 2 + 4);
	mydraw("cd", x * 2 - 1, y * 2 + 5);
}

void drawat(int x, int y){
	if (air)
		if (facing) mydraw(super ? "lm" : "LM", x * 2 - 1, y * 2 + 4);
		else mydraw(super ? "jk" : "JK", x * 2 - 1, y * 2 + 4);
	else mydraw(super ? "fg": "FG", x * 2 - 1, y * 2 + 4);
	if (air)
		if (facing) mydraw(super ? "ho" : "HO", x * 2 - 1, y * 2 + 5);
		else mydraw(super ? "ni" : "NI", x * 2 - 1, y * 2 + 5);
	else mydraw(super ? "hi" : "HI", x * 2 - 1, y * 2 + 5);
}

void draw_field(char tutorial){
	int i, j;
	char F;
	if (tutorial) VDP_drawText("A - charge  B - retry  C - wait", 4, 1);
	else mydraw("ABCABCABCABCABCABCABCABCABCABCABCABCAB", 1, 3);
	for (i = 0; i < HEIGHT; i++){
		for (j = 1; j < WIDTH; j++){
			F = field[i][j];
			if (F == '@') VDP_drawText("  ", j * 2 - 1, i * 2 + 4);
			else if (F == 'q') mydraw("ab", j * 2 - 1, i * 2 + 4);
			else if (F == 1) mydraw(style ? "EE" : "ee", j * 2 - 1, i * 2 + 4);
			else if (F == 3) mydraw(style ? "DD" : "PQ", j * 2 - 1, i * 2 + 4);
			else VDP_drawText("  ", j * 2 - 1, i * 2 + 4);
		}
		for (j = 1; j < WIDTH; j++){
			F = field[i][j];
			if (F == '@') VDP_drawText("  ", j * 2 - 1, i * 2 + 5);
			else if (F == 'q') mydraw("cd", j * 2 - 1, i * 2 + 5);
			else if (F == 1) mydraw(style ? "EE" : "ee", j * 2 - 1, i * 2 + 5);
			else if (F == 3) mydraw(style ? "DD" : "pq", j * 2 - 1, i * 2 + 5);
			else VDP_drawText("  ", j * 2 - 1, i * 2 + 5);
		}
	}
	if (tutorial) VDP_drawText("D-PAD - move  START - menu", 7, 26);
	else mydraw("ABCABCABCABCABCABCABCABCABCABCABCABCAB", 1, 24);
}

/* TODO: main */

int main(u16 hard){
	char tutorial = 0;
	int p_x, p_y;
	u16 p_i = '?';
	u16 prpr_i = '?';
	u32 static_seed = 0;
	char sector_order = 0;
	int score, maxscore;
	int level, turns, pr_turn;
	int bombs, bms_lv;
	int scr_lv, retries;
	int mptr = 1;
	int i, j, dodraw = 1;
	int p_yt = 0, p_xt = 0;
	char keystr[9];
	PAL_setColor((PAL2 * 16) + 15, RGB24_TO_VDPCOLOR(0xFFFF48));
	PAL_setPalette(PAL1, custom_font.palette->data, DMA);
	VDP_loadTileSet(custom_font.tileset, 16, DMA);
	title:
	mptr = 1;
	VDP_clearPlane(BG_A, 1);
	VDP_drawText("______ _______  ______  _____ ", 5, 6);
	VDP_drawText("  ___/ |______ |_____/ |_____]", 5, 7);
	VDP_drawText(" /     |       |   \\   |     ", 5, 8);
	VDP_drawText("/_____ |______ |    \\_ |     ", 5, 9);
	while (1){
		if (mptr == 0) VDP_setTextPalette(PAL2);
		VDP_drawText(mptr == 0 ? "> PLAY TUTORIAL <" : "  PLAY TUTORIAL  ", 11, 13);
		VDP_setTextPalette(PAL0);
		if (mptr == 1) VDP_setTextPalette(PAL2);
		VDP_drawText(mptr == 1 ? ">  RANDOM MODE  <" : "   RANDOM MODE   ", 11, 15);
		VDP_setTextPalette(PAL0);
		if (mptr == 2) VDP_setTextPalette(PAL2);
		VDP_drawText(mptr == 2 ? ">  FIXED ORDER  <" : "   FIXED ORDER   ", 11, 17);
		VDP_setTextPalette(PAL0);
		p_i = mygetch();
		if (p_i == 7 || p_i == 6 || p_i == 5 || p_i == 4) break;
		if (p_i == 1 || p_i == 3) mptr = (mptr + 2) % 3;
		mptr = (mptr + 2) % 3;
		SND(456)
	}
	switch (mptr){
	case 0:
		tutorial = 1;
		sector_order = 0;
	break;
	case 1:
		tutorial = 0;
		sector_order = 0;
	break;
	case 2:
		tutorial = 0;
		sector_order = 1;
		VDP_clearPlane(BG_A, 1);
		for (i = 0; i < 6; i++) keystr[i] = '0';
		keystr[6] = 0;
		VDP_drawText("Enter starting sector ID", 8, 12);
		VDP_drawText("-", 17, 14);
		VDP_drawText(keystr, 17, 15);
		VDP_drawText("-", 17, 16);
		static_seed = 0;
		i = 0; j = 0;
		while (1){
			p_i = mygetch();
			VDP_drawText(" ", 17 + i, 14);
			VDP_drawText(" ", 17 + i, 16);
			if (p_i == 7 || p_i == 6 || p_i == 5 || p_i == 4) break;
			if (p_i == 2) { i = (i + 5) % 6; j = keystr[i] - '0'; }
			if (p_i == 3) { i = (i + 1) % 6; j = keystr[i] - '0'; }
			if (p_i == 1) j = (j + 9) % 10;
			if (p_i == 0) j = (j + 1) % 10;
			keystr[i] = j + '0';
			VDP_drawText("-", 17 + i, 14);
			VDP_drawText("-", 17 + i, 16);
			VDP_drawText(keystr, 17, 15);
		}
		for (i = 0, j = 1; i < 6; i++, j *=10)
			static_seed += (keystr[5 - i] - '0') * j;
	break;
	}
	p_i = '?';
	VDP_clearPlane(BG_A, 1);
	here:
	my_srand(getSubTick());
	if (sector_order) seed = static_seed;
	else seed = my_rand();
	facing = 1;		air = 0;
	level = 1;		turns = 0;		super = 0;
	score = 0;		maxscore = 0;	bombs = BOMBS;
	if (tutorial) bombs = open_level(1);
	else make_field();
	if (bombs < 0) { make_field(); tutorial = 0; bombs = BOMBS; }
	bms_lv = bombs;
	scr_lv = 0;
	retries = 0;
	store_field();
	p_x = stored_pos[0];
	p_y = stored_pos[1];
    while (1){
		int retry = 0;
		int skip_ai = 0;
		i = 1;
		j = 2 + tutorial;
		VDP_drawText("Lv: ", i, j);
		i += 4;
		i += drawnum(level, i, j);
		if (tutorial){
			VDP_drawText(" | Tutorial", i, j);
			i += 11;
		}
		else {
			VDP_drawText(" | Sector: ", i, j);
			i += 11;
			i += drawnum(seed, i, j);
		}
		VDP_drawText(" | Charges: ", i, j);
		i += 12;
		i += drawnum(bombs, i, j);
		VDP_drawText("     ", i , j);
		if (dodraw) draw_field(tutorial);
		else undrawat(p_xt, p_yt);
		drawat(p_x, p_y);
		i = 1;
		j = 25 - tutorial;
		VDP_drawText("Turn: ", i, j);
		i += 6;
		i += drawnum(turns, i, j);
		VDP_drawText(" | Score: ", i, j);
		i += 10;
		i += drawnum(score, i, j);
		VDP_drawText(" | Top: ", i, j);
		i += 8;
		i += drawnum(maxscore, i, j);
		VDP_drawText("            ", i , j);
		
		dodraw = 1;
		prpr_i = p_i;
		p_i = mygetch();
		undrawat(p_x, p_y);
		p_xt = p_x;
		p_yt = p_y;
		pr_turn = turns;
		if (p_i == 0 && p_y > 0){
			dodraw = 0;
			SND(456)
			if (field[p_y - 1][p_x] == 'q'){
				if (!super) retry++;
				else {
					delmons(p_y - 1, p_x);
					score += 9;
				}
			}
			if (field[p_y - 1][p_x] < 3 || super){
				char climb;
				turns++;
				if (field[p_y - 1][p_x] >= 3) super = 0;
				field[p_y][p_x] = 0;
				if (field[p_y - 1][p_x]) {score++; SND(560)}
				p_y--;
				field[p_y][p_x] = 0;
				undrawat(p_x, p_y);
				field[p_y][p_x] = '@';
			
				climb = alt_climb(p_x, p_y);
				if (climb && air) p_i = climb;
			}
		}
		switch (p_i){
			case 5: SND(456) dodraw = 0; break;
			case 0: dodraw = 0; break;
			case 11: style = !style; skip_ai++; break;
			case 7: 
				goto title;
			case 4: {
				if (prpr_i == 4) goto here;
				retry++;
				break;
			}
			case 6: {
				dodraw = 0;
				if (bombs && !super){
					turns++;
					SND(999)
					super = 1;
					bombs--;
				}
				break;
			}
			case 2: {
				dodraw = 0;
				SND(456)
				facing = 0;
				if (p_x <= 1) break;
				if (field[p_y][p_x - 1] == 'q'){
					if (!super) retry++;
					else {
						delmons(p_y, p_x - 1);
						score += 9;
					}
				}
				if (field[p_y][p_x - 1] < 3  || super){
					turns++;
					if (field[p_y][p_x - 1] >= 3) super = 0;
					if (field[p_y][p_x - 1]) {score++; SND(560)}
					field[p_y][p_x] = 0;
					p_x--;
					field[p_y][p_x] = '@';
				} else {
					if (p_y <= 0) break;
					if (field[p_y - 1][p_x - 1] < 3 && 
						field[p_y - 1][p_x] < 3){
							turns += 2;
							if (field[p_y - 1][p_x]) {score++; SND(560)}
							if (field[p_y - 1][p_x - 1]) {score++; SND(560)}
							field[p_y][p_x] = 0;
							field[p_y - 1][p_x] = 0;
							undrawat(p_x, p_y - 1);
							p_y--;
							p_x--;
							field[p_y][p_x] = '@';
						}
				}
				break;
			}
			case 3: {
				dodraw = 0;
				SND(456)
				facing = 1;
				if (p_x + 1 >= WIDTH) break;
				if (field[p_y][p_x + 1] == 'q'){
					if (!super) retry++;
					else {
						delmons(p_y, p_x + 1);
						score += 9;
					}
				}
				if (field[p_y][p_x + 1] < 3  || super){
					turns++;
					if (field[p_y][p_x + 1] >= 3) super = 0;
					if (field[p_y][p_x + 1]) {score++; SND(560)}
					field[p_y][p_x] = 0;
					p_x++;
					field[p_y][p_x] = '@';
				} else {
					if (p_y <= 0) break;
					if (field[p_y - 1][p_x + 1] < 3 && 
						field[p_y - 1][p_x] < 3){
							turns += 2;
							if (field[p_y - 1][p_x]) {score++; SND(560)}
							if (field[p_y - 1][p_x + 1]) {score++; SND(560)}
							field[p_y][p_x] = 0;
							field[p_y - 1][p_x] = 0;
							undrawat(p_x, p_y - 1);
							p_y--;
							p_x++;
							field[p_y][p_x] = '@';
						}
				}
				break;
			}
			case 1: {
				dodraw = 0;
				SND(456)
				if (p_y + 1 >= HEIGHT) break;
				if (field[p_y + 1][p_x] == 'q'){
					if (!super) retry++;
					else {
						delmons(p_y + 1, p_x);
						score += 9;
					}
				}
				if (field[p_y + 1][p_x] < 3 || super){
					turns++;
					if (field[p_y + 1][p_x] >= 3) super = 0;
					field[p_y][p_x] = 0;
					if (field[p_y + 1][p_x]) {score++; SND(560)}
					p_y++;
					field[p_y][p_x] = '@';
				}
				break;
			}
			default: {
				dodraw = 0;
				p_i = '?';
				skip_ai++;
				break;
			}
		}
		if (pr_turn == turns && p_i < 4) 
			PSG_setEnvelope(0, 4);
		if (score > maxscore) maxscore = score;
		
		for (i = 0; i < MOBS + 27 && !skip_ai; i++){
			int k[4], *monst = monsters[i];
			int dir_x, dir_y;
			if (!monst[0]) continue;
			k[0] = monst[2];
			k[3] = (monst[2] + 2) % 4;
			if (my_rand() % 2){
				k[1] = (monst[2] + 1) % 4;
				k[2] = (monst[2] + 3) % 4;
			} else {
				k[1] = (monst[2] + 3) % 4;
				k[2] = (monst[2] + 1) % 4;
			}
			for (j = 0; j < 4; j++){
				monst[2] = k[j];
				dir_y = monst[1];
				dir_x = monst[0];
				if (!dir_x) break;
				switch (k[j]){
					case 0: dir_x++; break;
					case 1: dir_y++; break;
					case 2: dir_x--; break;
					case 3: dir_y--; break;
				}
				if (dir_x > 0 && dir_x < WIDTH && dir_y >= 0 && dir_y < HEIGHT){
					char cell = field[dir_y][dir_x];
					if (!cell || (j == 3 && cell != 'q' && cell != '@')){
						field[monst[1]][monst[0]] = 0;
						undrawat(monst[0], monst[1]);
						monst[1] = dir_y;
						monst[0] = dir_x;
						field[dir_y][dir_x] = 'q';
					}
					else if (cell == '@') retry++;
					else continue;
				}
				else continue;
				break;
			}
			drawmob(monst[0], monst[1]);
		}
		if (win_condition()){
			if (bombs == bms_lv && bms_lv)
				score += 30;
			mytoggle = 0;
			dodraw = 1;
			super = 0;
			VDP_clearPlane(BG_A, 1);
			/*
			i = 12;
			if (level == 4 && tutorial){
				VDP_setTextPalette(PAL2);
				VDP_drawText("!!! TUTORIAL COMPLETED !!!", 6, 6);
				VDP_setTextPalette(PAL0);
			}
			VDP_drawText("Level", i, 8 + tutorial);
			i += 6;
			i += drawnum(level, i, 8 + tutorial);
			VDP_drawText(" clear!", i, 8 + tutorial);
			if (!tutorial){
				VDP_drawText("Sector:", 9, 10);
				drawnum(seed, 26, 10);
			}
			VDP_drawText("Turns:", 9, 11); drawnum(turns, 26, 11);
			VDP_drawText("Score gain:", 9, 12); drawnum(score - scr_lv, 26, 12);
			VDP_drawText("Charges used:", 9, 13); drawnum(bms_lv - bombs, 26, 13);
			VDP_drawText("Retries:", 9, 14); drawnum(retries, 26, 14);
			VDP_drawText("Press a button...", 11, 16);*/
			i = 12;
			if (level == 4 && tutorial){
				VDP_setTextPalette(PAL2);
				VDP_drawText("!!! TUTORIAL COMPLETED !!!", 6, 6);
				VDP_setTextPalette(PAL0);
			}
			VDP_drawText("Level", i, 8);
			i += 6;
			i += drawnum(level, i, 8);
			VDP_drawText(" clear!", i, 8);
			VDP_drawText("Sector:", 8, 10);
			if (tutorial) VDP_drawText("Tutorial", 22, 10);
			else drawnum(seed, 22, 10);
			VDP_drawText("Turns:", 8, 11); drawnum(turns, 22, 11);
			VDP_drawText("Score gain:", 8, 12); drawnum(score - scr_lv, 22, 12);
			VDP_drawText("Charges used:", 8, 13); drawnum(bms_lv - bombs, 22, 13);
			VDP_drawText("Retries:", 8, 14); drawnum(retries, 22, 14);
			VDP_drawText("Press any button...", 10, 16);
			mygetch();
			level++;
			turns = 0;
			bombs = BOMBS;
			my_srand(my_rand() + getSubTick());
			if (sector_order) seed++;
			else seed = my_rand();
			if (tutorial) bombs = open_level(level);
			else {make_field(); tutorial = 0;}
			if (bombs < 0) {
				tutorial = 0; 
				bombs = BOMBS;
				goto title;
			}
			bms_lv = bombs;
			scr_lv = score;
			retries = 0;
			store_field();
			p_x = stored_pos[0];
			p_y = stored_pos[1];
			VDP_clearPlane(BG_A, 1);
		}
		while (air && (p_y + 1 < HEIGHT) && !skip_ai){
			if (field[p_y + 1][p_x] == 'q') retry++;
			if (field[p_y + 1][p_x]) break;
			undrawat(p_xt, p_yt);
			drawat(p_x, p_y);
			p_yt = p_y;
			p_xt = p_x;
			SYS_doVBlankProcess();
			field[p_y++][p_x] = 0;
			field[p_y][p_x] = '@';
		}
		if (retry){
			VDP_clearPlane(BG_A, 1);
			bombs = bms_lv;
			mytoggle = 0;
			retries++;
			super = 0;
			turns = 0;
			load_field();
			load_m();
			p_x = stored_pos[0];
			p_y = stored_pos[1];
			dodraw = 1;
			score = score / 2;
			retry = 0;
			SND(300)
		}
		if (!field[p_y + 1][p_x] && p_y + 1 < HEIGHT) air++;
		else air = 0;
		if (field[p_y + 1][p_x] == 'q' && p_y + 1 < HEIGHT && !RIDEABLE_MOBS) air++;
    }
}