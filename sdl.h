/*
 * Part of Astonia Client (c) Daniel Brockhaus. Please read license.txt.
 */

#define GFXPATH "../gfx/"

#define MAXSPRITE 250000

int sdl_init(int width,int height,char *title);
int sdl_clear(void);
int sdl_render(void);

int sdl_ic_load(int sprite);
void sdl_blit(int stx,int sx,int sy,int clipsx,int clipsy,int clipex,int clipey,int x_offset,int y_offset);
int sdl_tx_load(int sprite,int sink,int freeze,int grid,int scale,int cr,int cg,int cb,int light,int sat,int c1,int c2,int c3,int shine,int ml,int ll,int rl,int ul,int dl);

