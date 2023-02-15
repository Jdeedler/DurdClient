/*
 * Part of Astonia Client (c) Daniel Brockhaus. Please read license.txt.
 *
 * Mod Loading
 *
 * Loads and initializes the amod.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#include "../../src/astonia.h"
#include "../../src/modder.h"
#include "../../src/modder/_modder.h"
#include "../../src/game.h"
#include "../../src/game/_game.h"
#include "../../src/client.h"
#include "../../src/gui.h"

#define MAXMOD  6

struct mod {
    void (*_amod_init)(void);
    void (*_amod_exit)(void);
    void (*_amod_gamestart)(void);
    void (*_amod_frame)(void);
    void (*_amod_tick)(void);
    void (*_amod_mouse_move)(int x,int y);
    int (*_amod_mouse_click)(int x,int y,int what);
    void (*_amod_mouse_capture)(int onoff);
    void (*_amod_areachange)(void);
    int (*_amod_keydown)(int);
};

struct mod mod[MAXMOD]={{NULL}};

int (*_amod_display_skill_line)(int v,int base,int curr,int cn,char *buf)=NULL;

char *game_email_main="<no one>";
char *game_email_cash="<no one>";
char *game_url="<nowhere>";

int amod_init(void) {
	HMODULE dll_instance=NULL;
	void *tmp;
    char fname[80];

    for (int i=0; i<MAXMOD; i++) {
        sprintf(fname,"bin\\%cmod.dll",i+'a');
        dll_instance=LoadLibrary(fname);
        if (!dll_instance) continue;;

        // amod
        if ((tmp=GetProcAddress(dll_instance,"amod_init"))) mod[i]._amod_init=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_exit"))) mod[i]._amod_exit=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_gamestart"))) mod[i]._amod_gamestart=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_frame"))) mod[i]._amod_frame=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_tick"))) mod[i]._amod_tick=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_mouse_move"))) mod[i]._amod_mouse_move=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_mouse_click"))) mod[i]._amod_mouse_click=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_mouse_capture"))) mod[i]._amod_mouse_capture=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_areachange"))) mod[i]._amod_areachange=tmp;
        if ((tmp=GetProcAddress(dll_instance,"amod_keydown"))) mod[i]._amod_keydown=tmp;
        if (i!=0) continue; // only amod is allowed to override client stuff, the others can only add stuff

        if ((tmp=GetProcAddress(dll_instance,"amod_display_skill_line"))) _amod_display_skill_line=tmp;  // not really a variable, but...

        // client functions
        if ((tmp=GetProcAddress(dll_instance,"is_cut_sprite"))) is_cut_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"is_mov_sprite"))) is_mov_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"is_door_sprite"))) is_door_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"is_yadd_sprite"))) is_yadd_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"get_chr_height"))) get_chr_height=tmp;
        if ((tmp=GetProcAddress(dll_instance,"trans_asprite"))) trans_asprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"trans_charno"))) trans_charno=tmp;
        if ((tmp=GetProcAddress(dll_instance,"get_player_sprite"))) get_player_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"trans_csprite"))) trans_csprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"get_lay_sprite"))) get_lay_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"get_offset_sprite"))) get_offset_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"additional_sprite"))) additional_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"opt_sprite"))) opt_sprite=tmp;
        if ((tmp=GetProcAddress(dll_instance,"get_skltab_index"))) get_skltab_index=tmp;
        if ((tmp=GetProcAddress(dll_instance,"get_skltab_sep"))) get_skltab_sep=tmp;
        if ((tmp=GetProcAddress(dll_instance,"get_skltab_show"))) get_skltab_show=tmp;
        if ((tmp=GetProcAddress(dll_instance,"do_display_random"))) do_display_random=tmp;

        // client variables
        if ((tmp=GetProcAddress(dll_instance,"game_email_main"))) game_email_main=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_email_cash"))) game_email_cash=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_url"))) game_url=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_rankname"))) game_rankname=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_rankcount"))) game_rankcount=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_v_max"))) game_v_max=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_skill"))) game_skill=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_skilldesc"))) game_skilldesc=tmp;
        if ((tmp=GetProcAddress(dll_instance,"game_v_profbase"))) game_v_profbase=tmp;
    }

    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_init) mod[i]._amod_init();
    }

    return 1;
}

void amod_exit(void) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_exit) mod[i]._amod_exit();
    }
}

void amod_gamestart(void) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_gamestart) mod[i]._amod_gamestart();
    }
}

void amod_frame(void) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_frame) mod[i]._amod_frame();
    }
}

void amod_tick(void) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_tick) mod[i]._amod_tick();
    }
}

void amod_mouse_move(int x,int y) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_mouse_move) mod[i]._amod_mouse_move(x,y);
    }
}

int amod_mouse_click(int x,int y,int what) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_mouse_click) return mod[i]._amod_mouse_click(x,y,what);
    }
    return 0;
}

void amod_mouse_capture(int onoff) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_mouse_capture) mod[i]._amod_mouse_capture(onoff);
    }
}

int amod_display_skill_line(int v,int base,int curr,int cn,char *buf) {
    if (_amod_display_skill_line) return _amod_display_skill_line(v,base,curr,cn,buf);
    return 0;
}

void amod_areachange(void) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_areachange) mod[i]._amod_areachange();
    }
}

int amod_keydown(int key) {
    for (int i=0; i<MAXMOD; i++) {
        if (mod[i]._amod_areachange) return mod[i]._amod_keydown(key);
    }
    return 0;
}

