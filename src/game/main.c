/*
 * Part of Astonia Client (c) Daniel Brockhaus. Please read license.txt.
 *
 * Startup And Command Line
 *
 * Contains the startup stuff and the parsing of the command line. Plus a
 * bunch of generic helper for memory allocation and error display.
 *
 */

#include <windows.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "../../src/astonia.h"
#include "../../src/game.h"
#include "../../src/game/_game.h"
#include "../../src/sdl.h"
#include "../../src/gui.h"
#include "../../src/client.h"
#include "../../src/modder.h"

int quit=0;
static int quickstart=0;
static int panic_reached=0;
static int xmemcheck_failed=0;
int largetext=0;
int vendor=1;
char user_keys[10]={'Q','W','E','A','S','D','Z','X','C','V'};

static char memcheck_failed_str[]={"TODO: memcheck failed"};  // TODO
static char panic_reached_str[]={"TODO: panic failure"}; // TODO

static FILE *errorfp;

// note, warn, fail, paranoia, addline

__declspec(dllexport) int note(const char *format,...) {
    va_list va;
    char buf[1024];


    va_start(va,format);
    vsprintf(buf,format,va);
    va_end(va);

    printf("NOTE: %s\n",buf);
    fflush(stdout);
#ifdef DEVELOPER
    addline("NOTE: %s\n",buf);
#endif

    return 0;
}

__declspec(dllexport) int warn(const char *format,...) {
    va_list va;
    char buf[1024];


    va_start(va,format);
    vsprintf(buf,format,va);
    va_end(va);

    printf("WARN: %s\n",buf);
    fflush(stdout);
    addline("WARN: %s\n",buf);

    return 0;
}

__declspec(dllexport) int fail(const char *format,...) {
    va_list va;
    char buf[1024];


    va_start(va,format);
    vsprintf(buf,format,va);
    va_end(va);

    fprintf(errorfp,"FAIL: %s\n",buf);
    fflush(errorfp);
    printf("FAIL: %s\n",buf);
    fflush(stdout);
    addline("FAIL: %s\n",buf);

    return -1;
}

__declspec(dllexport) void paranoia(const char *format,...) {
    va_list va;

    fprintf(errorfp,"PARANOIA EXIT in ");

    va_start(va,format);
    vfprintf(errorfp,format,va);
    va_end(va);

    fprintf(errorfp,"\n");
    fflush(errorfp);

    exit(-1);
}

static int _addlinesep=0;

void addlinesep(void) {
    _addlinesep=1;
}

__declspec(dllexport) void addline(const char *format,...) {
    va_list va;
    char buf[1024];

    if (_addlinesep) {
        _addlinesep=0;
        addline("-------------");
    }

    va_start(va,format);
    vsnprintf(buf,sizeof(buf)-1,format,va); buf[sizeof(buf)-1]=0;
    va_end(va);

    if (dd_text_init_done()) dd_add_text(buf);
}

// io

int rread(int fd,void *ptr,int size) {
    int n;

    while (size>0) {
        n=read(fd,ptr,size);
        if (n<0) return -1;
        if (n==0) return 1;
        size-=n;
        ptr=((unsigned char *)(ptr))+n;
    }
    return 0;
}

char* load_ascii_file(char *filename,int ID) {
    int fd,size;
    char *ptr;

    if ((fd=open(filename,O_RDONLY|O_BINARY))==-1) return NULL;
    if ((size=lseek(fd,0,SEEK_END))==-1) { close(fd); return NULL; }
    if (lseek(fd,0,SEEK_SET)==-1) { close(fd); return NULL; }
    ptr=xmalloc(size+1,ID);
    if (rread(fd,ptr,size)) { xfree(ptr); close(fd); return NULL; }
    ptr[size]=0;
    close(fd);

    return ptr;
}

// memory

//#define malloc_proc(size) GlobalAlloc(GPTR,size)
//#define realloc_proc(ptr,size) GlobalReAlloc(ptr,size,GMEM_MOVEABLE)
//#define free_proc(ptr) GlobalFree(ptr)

HANDLE myheap;
#define malloc_proc(size) myheapalloc(size)
#define realloc_proc(ptr,size) myheaprealloc(ptr,size)
#define free_proc(ptr) myheapfree(ptr)

int memused=0;
int memptrused=0;

int maxmemsize=0;
int maxmemptrs=0;
int memptrs[MAX_MEM];
int memsize[MAX_MEM];

struct memhead {
    int size;
    int ID;
};

// TODO: removed unused memory areas
static char *memname[MAX_MEM]={
    "MEM_TOTA", //0
    "MEM_GLOB",
    "MEM_TEMP",
    "MEM_ELSE",
    "MEM_DL",
    "MEM_IC",   //5
    "MEM_SC",
    "MEM_VC",
    "MEM_PC",
    "MEM_GUI",
    "MEM_GAME", //10
    "MEM_TEMP11",
    "MEM_VPC",
    "MEM_VSC",
    "MEM_VLC",
    "MEM_SDL_BASE",
    "MEM_SDL_PIXEL",
    "MEM_SDL_PNG",
    "MEM_SDL_PIXEL2",
    "MEM_TEMP5",
    "MEM_TEMP6",
    "MEM_TEMP7",
    "MEM_TEMP8",
    "MEM_TEMP9",
    "MEM_TEMP10"
};

void* myheapalloc(int size) {
    void *ptr;

    memptrused++;
    ptr=HeapAlloc(myheap,HEAP_ZERO_MEMORY,size);
    memused+=HeapSize(myheap,0,ptr);

    return ptr;
}

void* myheaprealloc(void *ptr,int size) {
    memused-=HeapSize(myheap,0,ptr);
    ptr=HeapReAlloc(myheap,HEAP_ZERO_MEMORY,ptr,size);
    memused+=HeapSize(myheap,0,ptr);

    return ptr;
}

void myheapfree(void *ptr) {
    memptrused--;
    memused-=HeapSize(myheap,0,ptr);
    HeapFree(myheap,0,ptr);
}

void list_mem(void) {
    int i,flag=0;
    MEMORYSTATUS ms;
    extern long long mem_tex;

    note("--mem----------------------");
    for (i=1; i<MAX_MEM; i++) {
        if (memsize[i] || memptrs[i])  {
            flag=1;
            note("%s %.2fMB in %d ptrs",memname[i],memsize[i]/(1024.0*1024.0),memptrs[i]);
        }
    }
    if (flag) note("%s %.2fMB in %d ptrs",memname[0],memsize[0]/(1024.0*1024.0),memptrs[0]);
    note("%s %.2fMB in %d ptrs","MEM_MAX",maxmemsize/(1024.0*1024.0),maxmemptrs);
    note("---------------------------");
    note("Texture Cache: %.2fMB",mem_tex/(1024.0*1024.0));

    bzero(&ms,sizeof(ms));
    ms.dwLength=sizeof(ms);
    GlobalMemoryStatus(&ms);

    note("UsedMem=%.2fG of %.2fG",(memused+mem_tex)/1024.0/1024.0/1024.0,ms.dwTotalPhys/1024.0/1024.0/1024.0);

    i=HeapValidate(myheap,0,NULL);
    if (!i) note("validate says: %d",i);

}

static int memcheckset=0;
static unsigned char memcheck[256];

int xmemcheck(void *ptr) {
    struct memhead *mem;
    unsigned char *head,*tail,*rptr;

    if (!ptr) return 0;

    mem=(struct memhead *)(((unsigned char *)(ptr))-8-sizeof(memcheck));

    // ID check
    if (mem->ID>=MAX_MEM) { note("xmemcheck: ill mem id (%d)",mem->ID); xmemcheck_failed=1; return -1; }

    // border check
    head=((unsigned char *)(mem))+8;
    rptr=((unsigned char *)(mem))+8+sizeof(memcheck);
    tail=((unsigned char *)(mem))+8+sizeof(memcheck)+mem->size;

    if (memcmp(head,memcheck,sizeof(memcheck))) { fail("xmemcheck: ill head in %s (ptr=%p)",memname[mem->ID],rptr); xmemcheck_failed=1; return -1; }
    if (memcmp(tail,memcheck,sizeof(memcheck))) { fail("xmemcheck: ill tail in %s (ptr=%p)",memname[mem->ID],rptr); xmemcheck_failed=1; return -1; }

    return 0;
}

void* xmalloc(int size,int ID) {
    struct memhead *mem;
    unsigned char *head,*tail,*rptr;

    if (!memcheckset) {
        note("initialized memcheck");
        for (memcheckset=0; memcheckset<sizeof(memcheck); memcheckset++) memcheck[memcheckset]=rrand(256);
        sprintf(memcheck,"!MEMCKECK MIGHT FAIL!");
        myheap=HeapCreate(0,0,0);
    }

    if (!size) return NULL;

    mem=malloc_proc(8+sizeof(memcheck)+size+sizeof(memcheck));
    if (!mem) { fail("OUT OF MEMORY !!!"); return NULL; }

    if (ID>=MAX_MEM) { fail("xmalloc: ill mem id"); return NULL; }

    mem->ID=ID;
    mem->size=size;
    memsize[mem->ID]+=mem->size;
    memptrs[mem->ID]+=1;
    memsize[0]+=mem->size;
    memptrs[0]+=1;

    if (memsize[0]>maxmemsize) maxmemsize=memsize[0];
    if (memptrs[0]>maxmemptrs) maxmemptrs=memptrs[0];

    head=((unsigned char *)(mem))+8;
    rptr=((unsigned char *)(mem))+8+sizeof(memcheck);
    tail=((unsigned char *)(mem))+8+sizeof(memcheck)+mem->size;

    // set memcheck
    memcpy(head,memcheck,sizeof(memcheck));
    memcpy(tail,memcheck,sizeof(memcheck));

    xmemcheck(rptr);

    return rptr;
}

void* xcalloc(int size,int ID) {
    void *ptr;

    ptr=xmalloc(size,ID);
    if (ptr) bzero(ptr,size);
    return ptr;
}

char* xstrdup(const char *src,int ID) {
    int size;
    char *dst;

    size=strlen(src)+1;

    dst=xmalloc(size,ID);
    if (!dst) return NULL;

    memcpy(dst,src,size);

    return dst;
}


void xfree(void *ptr) {
    struct memhead *mem;

    if (!ptr) return;
    if (xmemcheck(ptr)) return;

    // get mem
    mem=(struct memhead *)(((unsigned char *)(ptr))-8-sizeof(memcheck));

    // free
    memsize[mem->ID]-=mem->size;
    memptrs[mem->ID]-=1;
    memsize[0]-=mem->size;
    memptrs[0]-=1;

    free_proc(mem);
}

void xinfo(void *ptr) {
    struct memhead *mem;

    if (!ptr) { printf("NULL"); return; }
    if (xmemcheck(ptr)) { printf("ILL"); return; }

    // get mem
    mem=(struct memhead *)(((unsigned char *)(ptr))-8-sizeof(memcheck));

    printf("%d bytes",mem->size);
}

void* xrealloc(void *ptr,int size,int ID) {
    struct memhead *mem;
    unsigned char *head,*tail,*rptr;

    if (!ptr) return xmalloc(size,ID);
    if (!size) { xfree(ptr); return NULL; }
    if (xmemcheck(ptr)) return NULL;

    mem=(struct memhead *)(((unsigned char *)(ptr))-8-sizeof(memcheck));

    // realloc
    memsize[mem->ID]-=mem->size;
    memptrs[mem->ID]-=1;
    memsize[0]-=mem->size;
    memptrs[0]-=1;

    mem=realloc_proc(mem,8+sizeof(memcheck)+size+sizeof(memcheck));
    if (!mem) { fail("xrealloc: OUT OF MEMORY !!!"); return NULL; }

    mem->ID=ID;
    mem->size=size;
    memsize[mem->ID]+=mem->size;
    memptrs[mem->ID]+=1;
    memsize[0]+=mem->size;
    memptrs[0]+=1;

    if (memsize[0]>maxmemsize) maxmemsize=memsize[0];
    if (memptrs[0]>maxmemptrs) maxmemptrs=memptrs[0];

    head=((unsigned char *)(mem))+8;
    rptr=((unsigned char *)(mem))+8+sizeof(memcheck);
    tail=((unsigned char *)(mem))+8+sizeof(memcheck)+mem->size;

    // set memcheck
    memcpy(head,memcheck,sizeof(memcheck));
    memcpy(tail,memcheck,sizeof(memcheck));

    return rptr;
}

void* xrecalloc(void *ptr,int size,int ID) {
    struct memhead *mem;
    unsigned char *head,*tail,*rptr;

    if (!ptr) return xcalloc(size,ID);
    if (!size) { xfree(ptr); return NULL; }
    if (xmemcheck(ptr)) return NULL;

    mem=(struct memhead *)(((unsigned char *)(ptr))-8-sizeof(memcheck));

    // realloc
    memsize[mem->ID]-=mem->size;
    memptrs[mem->ID]-=1;
    memsize[0]-=mem->size;
    memptrs[0]-=1;

    mem=realloc_proc(mem,8+sizeof(memcheck)+size+sizeof(memcheck));
    if (!mem) { fail("xrecalloc: OUT OF MEMORY !!!"); return NULL; }

    if (size-mem->size>0) {
        bzero(((unsigned char *)(mem))+8+sizeof(memcheck)+mem->size,size-mem->size);
    }

    mem->ID=ID;
    mem->size=size;
    memsize[mem->ID]+=mem->size;
    memptrs[mem->ID]+=1;
    memsize[0]+=mem->size;
    memptrs[0]+=1;

    if (memsize[0]>maxmemsize) maxmemsize=memsize[0];
    if (memptrs[0]>maxmemptrs) maxmemptrs=memptrs[0];

    head=((unsigned char *)(mem))+8;
    rptr=((unsigned char *)(mem))+8+sizeof(memcheck);
    tail=((unsigned char *)(mem))+8+sizeof(memcheck)+mem->size;

    // set memcheck
    memcpy(head,memcheck,sizeof(memcheck));
    memcpy(tail,memcheck,sizeof(memcheck));

    return rptr;
}

// rrandom

void rrandomize(void) {
    srand(time(NULL));
}

int rrand(int range) {
    int r;

    r=rand();
    return (range*r/(RAND_MAX+1));
}

// wsa network

int net_init(void) {
    WSADATA wsadata;

    if (WSAStartup(0x0002,&wsadata)) return -1;
    return 0;
}

int net_exit(void) {
    WSACleanup();
    return 0;
}

// parsing command line

void display_usage(void) {
    char *buf,*txt;

    txt=buf=malloc(1024*4);
    buf+=sprintf(buf,"The Astonia Client can only be started from the command line or with a specially created shortcut.\n\n");
    buf+=sprintf(buf,"Usage: moac -u playername -p password -d url [-w width]\n ... -h height] [-l largetextenable] [-s soundenable]\n");
    buf+=sprintf(buf," ... [-m threads] [-o options] [-f fullscreen] [-c cachesize]\n ... [-k framespersecond] -x [contextmenuenable]\n\n");
    buf+=sprintf(buf,"url being, for example, \"server.astonia.com\" or \"192.168.77.132\" (without the quotes).\n");
    buf+=sprintf(buf,"width and height are the desired window size. If this matches the desktop size the client will start in windowed borderless pseudo-fullscreen mode.\n");
    buf+=sprintf(buf,"fullscreen, largetextenable and soundenable can be either 0 or 1, for off or on.\n");
    buf+=sprintf(buf,"threads is the number of background threads the game should use. Use 0 to disable. Default is 4.\n");
    buf+=sprintf(buf,"options is a bitfield. Bit 0 (value of 1) enables the Dark GUI by Tegra.\n");
    buf+=sprintf(buf,"cachesize is the size of the texture cache. Default is 5000. Very low numbers will crash!\n");
    buf+=sprintf(buf,"framespersecond will set the display rate in frames per second.\n");
    buf+=sprintf(buf,"contextmenuenable is another bitfield. Bit 0 enabled the context menu, bit 1 the new keybindings (default: 3)\n");

    MessageBox(NULL,txt,"Usage",MB_APPLMODAL|MB_OK|MB_ICONEXCLAMATION);

    printf("%s",txt);

    free(txt);
}

char server_url[256];
int want_width=0,want_height=0;

int parse_cmd(char *s) {
    int n;
    char *end;

    while (isspace(*s)) s++;

    while (*s) {
        if (*s=='-') {
            s++;
            if (tolower(*s)=='u') {         // -u <username>
                s++;
                while (isspace(*s)) s++;
                n=0; while (n<40 && *s && !isspace(*s)) username[n++]=*s++;
                username[n]=0;
                quickstart=1;
            } else if (tolower(*s)=='p') {  // -p <password>
                s++;
                while (isspace(*s)) s++;
                n=0; while (n<16 && *s && !isspace(*s)) password[n++]=*s++;
                password[n]=0;
            } else if (tolower(*s)=='d') { // -d <server url>
                s++;
                while (isspace(*s)) s++;
                n=0; while (n<250 && *s && !isspace(*s)) server_url[n++]=*s++;
            } else if (tolower(*s)=='h') {    // -h <horizontal_resolution>
                s++;
                while (isspace(*s)) s++;
                want_height=strtol(s,&end,10);
                s=end;
                if (*s=='p') s++;
            } else if (tolower(*s)=='w') {    // -w <vertical_resolution>
                s++;
                while (isspace(*s)) s++;
                want_width=strtol(s,&end,10);
                s=end;
            } else if (tolower(*s)=='l') { // -l Large Text
                s++;
                while (isspace(*s)) s++;
                largetext=strtol(s,&end,10);
                s=end;
            } else if (tolower(*s)=='s') { // -s Sound
                s++;
                while (isspace(*s)) s++;
                enable_sound=strtol(s,&end,10);
                s=end;
            } else if (tolower(*s)=='m') { // -m Multi-Threaded
                s++;
                while (isspace(*s)) s++;
                sdl_multi=strtol(s,&end,10);
                s=end;
            } else if (tolower(*s)=='o') { // -o option
                s++;
                while (isspace(*s)) s++;
                sprite_options=strtoull(s,&end,10);
                s=end;
            } else if (tolower(*s)=='f') { // -f fullscreen
                s++;
                while (isspace(*s)) s++;
                sdl_fullscreen=strtol(s,&end,10);
                s=end;
            } else if (tolower(*s)=='c') { // -c cachesize
                s++;
                while (isspace(*s)) s++;
                sdl_cache_size=strtol(s,&end,10);
                s=end;
            } else if (tolower(*s)=='k') { // -k frames per second
                s++;
                while (isspace(*s)) s++;
                frames_per_second=strtol(s,&end,10);
                s=end;
            } else if (tolower(*s)=='x') { // -x context menu (and other newbie-friendly changes)
                s++;
                while (isspace(*s)) s++;
                context_enabled=strtol(s,&end,10);
                s=end;
            } else { display_usage(); return -1; }
        } else { display_usage(); return -2; }
        while (isspace(*s)) s++;
    }
    return 0;
}

void save_options(void) {
    int handle;

    handle=open("bin/data/moac.dat",O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0666);
    if (handle==-1) return;

    write(handle,&user_keys,sizeof(user_keys));
    close(handle);
}

void load_options(void) {
    int handle;

    handle=open("bin/data/moac.dat",O_RDONLY|O_BINARY);
    if (handle==-1) return;

    read(handle,&user_keys,sizeof(user_keys));
    close(handle);
}

// convert command line from unix style to windows style
void convert_cmd_line(char *d,int argc,char *args[],int maxsize) {
    int n;
    char *s;

    maxsize-=2;

    for (n=1; n<argc && maxsize>0; n++) {
        for (s=args[n]; *s && maxsize>0; *d++=*s++) maxsize--;
        *d++=' '; maxsize--;
    }
    *d=0;
}

// main
int main(int argc,char *args[]) {
    int ret;
    char buf[80],buffer[1024];
    struct hostent *he;

    errorfp=fopen("bin/data/moac.log","a");
    if (!errorfp) errorfp=stderr;

    amod_init();

    load_options();

    convert_cmd_line(buffer,argc,args,1000);
    if ((ret=parse_cmd(buffer))!=0) return -1;

    // set some stuff
    if (!*username || !*password || !*server_url) {
        display_usage();
        return 0;
    }

    SetProcessDPIAware(); // I hate Windows very much.

    // next init (only once)
    if (net_init()==-1) {
        MessageBox(NULL,"Can't Initialize Windows Networking Libraries.","Error",MB_APPLMODAL|MB_OK|MB_ICONSTOP);
        return -1;
    }

    if (isdigit(server_url[0])) {
        target_server=ntohl(inet_addr(server_url));
    } else {
        he=gethostbyname(server_url);
        if (he) target_server=ntohl(*(unsigned long *)(*he->h_addr_list));
        else {
            fail("Could not resolve server %s.",server_url);
            return -2;
        }
    }

    note("Using login server at %u.%u.%u.%u",(target_server>>24)&255,(target_server>>16)&255,(target_server>>8)&255,(target_server>>0)&255);

    // init random
    rrandomize();

    if (!want_height) {
        if (want_width) want_height=want_width*9/16;
    }
    if (!want_width) {
        if (!want_height) want_width=want_height*16/9;
    }

    sprintf(buf,"Astonia 3 v%d.%d.%d",(VERSION>>16)&255,(VERSION>>8)&255,(VERSION)&255);
    if (!sdl_init(want_width,want_height,buf)) {
        dd_exit();
        net_exit();
        return -1;
    }

    dd_init();
    init_sound();

    if (largetext) {
        namesize=0;
        dd_set_textfont(1);
    }

    main_init();
    update_user_keys();

    main_loop();

    amod_exit();
    main_exit();
    sound_exit();
    dd_exit();
    sdl_exit();

    list_mem();

    if (panic_reached) MessageBox(NULL,panic_reached_str,"recursion panic",MB_APPLMODAL|MB_OK|MB_ICONSTOP);
    if (xmemcheck_failed) MessageBox(NULL,memcheck_failed_str,"memory panic",MB_APPLMODAL|MB_OK|MB_ICONSTOP);

    net_exit();
    return 0;
}
