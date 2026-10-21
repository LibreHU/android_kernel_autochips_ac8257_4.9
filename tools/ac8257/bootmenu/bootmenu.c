/* bootmenu - menu de demarrage UJC201/AC8257 : TWRP / Android / Fastboot, demarrage auto d'Android.
 *
 * Installe comme /init dans le ramdisk du boot.img (noyau patche skip_initramfs -> want_initramfs).
 *  - affiche l'interface (ui_data.h, genere par gen_ui.py) sur fb0 : ecran logique 1280x720,
 *    tourne de 90 deg vers la dalle 720x1280 comme TWRP (x_disp = 719 - y, y_disp = x) ;
 *  - tactile mtk-tpd (coordonnees paysage ~1024x600, X inverse) + touches du bandeau HOME / BACK ;
 *  - TWRP     : reboot(RESTART2, "recovery")   (meme chemin que adb reboot recovery)
 *    Fastboot : reboot(RESTART2, "bootloader")
 *    Android  : monte system (PARTNAME=system) en lecture seule, bascule la racine dessus
 *               et execute son /init (methode magiskinit pour les appareils system-as-root "legacy").
 *  - jamais de FBIOBLANK (le pilote Jancar coupe la dalle) ; "0" dans fb0/blank reveille le tactile.
 *  - toute erreur d'affichage -> demarrage direct d'Android ; system introuvable -> TWRP.
 *
 * Modes :  /init                          (PID 1, boot normal)
 *          bootmenu --test                (dans TWRP : affiche et reagit, Android = quitter)
 *          bootmenu --dump out.raw P M B  (qemu : ecrit la trame dalle 736x1280 RGBA, sans materiel)
 * Autonome (pas de libc) : syscalls aarch64 directs. */
#include "ui_data.h"
typedef unsigned long u64; typedef long s64; typedef unsigned short u16; typedef short s16;
typedef int s32; typedef unsigned int u32; typedef unsigned char u8;

static s64 sys6(s64 n,s64 a,s64 b,s64 c,s64 d,s64 e,s64 f){
 register s64 x8 asm("x8")=n,x0 asm("x0")=a,x1 asm("x1")=b,x2 asm("x2")=c,x3 asm("x3")=d,x4 asm("x4")=e,x5 asm("x5")=f;
 asm volatile("svc 0":"+r"(x0):"r"(x8),"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5):"memory");return x0;}
#define sys(n,a,b,c,d) sys6(n,a,b,c,d,0,0)
/* le compilateur peut generer des appels memset/memcpy : versions minimales */
void*memset(void*d,int c,unsigned long n){volatile unsigned char*p=d;while(n--)*p++=(unsigned char)c;return d;}
void*memcpy(void*d,const void*s,unsigned long n){volatile unsigned char*p=d;const unsigned char*q=s;while(n--)*p++=*q++;return d;}
enum{SYS_dup3=24,SYS_ioctl=29,SYS_mknodat=33,SYS_mkdirat=34,SYS_umount2=39,SYS_mount=40,SYS_chdir=49,
 SYS_chroot=51,SYS_openat=56,SYS_close=57,SYS_read=63,SYS_write=64,SYS_ppoll=73,SYS_sync=81,
 SYS_exit=93,SYS_nanosleep=101,SYS_clock_gettime=113,SYS_reboot=142,SYS_getpid=172,SYS_munmap=215,
 SYS_mmap=222,SYS_execve=221};
#define AT_FDCWD -100
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_NONBLOCK 04000
#define O_CLOEXEC 02000000

/* ------------------------------------------------------------------ utilitaires */
static int op(const char*p,int f){return (int)sys(SYS_openat,AT_FDCWD,(s64)p,f,0644);}
static void cl(int fd){if(fd>=0)sys(SYS_close,fd,0,0,0);}
static void msleep(int ms){s64 ts[2]={ms/1000,(ms%1000)*1000000L};sys(SYS_nanosleep,(s64)ts,0,0,0);}
static s64 now_ms(void){s64 ts[2];sys(SYS_clock_gettime,1/*MONOTONIC*/,(s64)ts,0,0);return ts[0]*1000+ts[1]/1000000;}
static int slen(const char*s){int n=0;while(s[n])n++;return n;}
static int streq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static int starts(const char*a,const char*p){while(*p)if(*a++!=*p++)return 0;return 1;}
static int atoi_(const char*s){int n=0,g=1;if(*s=='-'){g=-1;s++;}while(*s>='0'&&*s<='9')n=n*10+(*s++-'0');return n*g;}
static char*pstr(char*o,const char*s){while(*s)*o++=*s++;*o=0;return o;}
static char*pnum(char*o,s64 v){char t[24];int k=0;if(v<0){*o++='-';v=-v;}do{t[k++]='0'+v%10;v/=10;}while(v);while(k)*o++=t[--k];*o=0;return o;}
static int rdfile(const char*p,char*b,int n){int fd=op(p,O_RDONLY);if(fd<0)return -1;s64 r=sys(SYS_read,fd,(s64)b,n-1,0);cl(fd);if(r<0)r=0;b[r]=0;
 while(r>0&&(b[r-1]=='\n'||b[r-1]==' '))b[--r]=0;return (int)r;}
static void wrfile(const char*p,const char*s){int fd=op(p,O_WRONLY);if(fd>=0){sys(SYS_write,fd,(s64)s,slen(s),0);cl(fd);}}

static int kmsg=-1,testmode=0;
static void klog(const char*a,const char*b){char m[256],*o=pstr(m,"<6>bootmenu: ");o=pstr(o,a);if(b)o=pstr(o,b);*o++='\n';*o=0;
 if(kmsg>=0)sys(SYS_write,kmsg,(s64)m,o-m,0);if(testmode)sys(SYS_write,1,(s64)m+3,o-m-3,0);}
static void klogn(const char*a,s64 v){char t[24];pnum(t,v);klog(a,t);}

static u32 mkdev(u32 ma,u32 mi){return (mi&0xff)|(ma<<8)|((mi&~0xffu)<<12);}
/* "MAJ:MIN" (fichier dev de sysfs) -> noeud */
static int mknode(const char*sysdev,const char*node,u32 type){char b[32];if(rdfile(sysdev,b,sizeof b)<=0)return -1;
 int ma=atoi_(b),i=0;while(b[i]&&b[i]!=':')i++;if(!b[i])return -1;int mi=atoi_(b+i+1);
 s64 r=sys(SYS_mknodat,AT_FDCWD,(s64)node,type|0600,mkdev(ma,mi));return r==0||r==-17/*EEXIST*/?0:-1;}

/* ------------------------------------------------------------------ affichage */
static u32 lb[UI_W*UI_H];                 /* ecran logique 0x00RRGGBB */
static u8*fbm;static s64 fbsz;static int fbfd=-1;
static u32 var[40];                        /* struct fb_var_screeninfo (160 octets) */
static u32 stride,xres,yres,bpp,page,npages;
#define V_XRES 0
#define V_YRES 1
#define V_YRES_VIRT 3
#define V_YOFF 5
#define V_BPP 6
#define V_ROFF 8
#define V_GOFF 11
#define V_BOFF 14
#define V_TOFF 17
#define V_TLEN 18

static void blit(int id){const struct spr*s=&ui_spr[id];const u32*r=ui_rle+s->off;u32 n=s->n,x=0,y=0;
 u32*row=lb+s->y*UI_W+s->x;
 for(u32 i=0;i<n;i++){u32 c=r[i]&0xffffff,k=(r[i]>>24)+1;
  while(k--){row[x]=c;if(++x==(u32)s->w){x=0;y++;row+=UI_W;}}}}
static void fillrect(int x,int y,int w,int h,u32 c){for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)lb[j*UI_W+i]=c;}

static u32 px_r,px_g,px_b,px_a;          /* decalages des canaux dans le pixel 32 bits */
static void px_setup(void){px_r=var[V_ROFF];px_g=var[V_GOFF];px_b=var[V_BOFF];
 if(var[V_TLEN])px_a=var[V_TOFF];else{u32 used=(1u<<px_r)|(1u<<px_g)|(1u<<px_b);px_a=32;
  for(u32 o=0;o<32;o+=8)if(!(used&(1u<<o))){px_a=o;break;}}}
/* ecran logique (x,y) -> dalle (719-y, x) ; ecrit dans la page 'pg' */
static void present_to(u8*base){
 if(bpp==32){for(u32 x=0;x<UI_W&&x<yres;x++){u32*d=(u32*)(base+(u64)x*stride);
   for(u32 y=0;y<UI_H&&y<xres;y++){u32 c=lb[y*UI_W+x];u32 v=((c>>16)&0xff)<<px_r|((c>>8)&0xff)<<px_g|(c&0xff)<<px_b;
    if(px_a<32)v|=0xffu<<px_a;d[xres-1-y]=v;}}}
 else {for(u32 x=0;x<UI_W&&x<yres;x++){u16*d=(u16*)(base+(u64)x*stride);  /* RGB565 */
   for(u32 y=0;y<UI_H&&y<xres;y++){u32 c=lb[y*UI_W+x];d[xres-1-y]=(u16)(((c>>19)&0x1f)<<11|((c>>10)&0x3f)<<5|((c>>3)&0x1f));}}}}
static void present(void){
 if(!fbm)return;
 u32 pg=npages>=2?(page+1)%npages:0;
 present_to(fbm+(u64)pg*yres*stride);
 var[V_YOFF]=pg*yres;
 if(sys(SYS_ioctl,fbfd,0x4601/*FBIOPUT_VSCREENINFO*/,(s64)var,0)<0)sys(SYS_ioctl,fbfd,0x4606/*FBIOPAN_DISPLAY*/,(s64)var,0);
 page=pg;}

static int fb_open(void){
 const char*c[]={"/dev/graphics/fb0","/dev/fb0",0};
 for(int i=0;c[i]&&fbfd<0;i++)fbfd=op(c[i],O_RDWR|O_CLOEXEC);
 if(fbfd<0){const char*n=testmode?"/tmp/bm_fb0":"/dev/fb0";
  for(int t=0;t<60&&mknode("/sys/class/graphics/fb0/dev",n,0020000)<0;t++)msleep(50);
  fbfd=op(n,O_RDWR|O_CLOEXEC);}
 if(fbfd<0){klog("fb0 introuvable",0);return -1;}
 u8 fix[80];
 if(sys(SYS_ioctl,fbfd,0x4600,(s64)var,0)<0||sys(SYS_ioctl,fbfd,0x4602,(s64)fix,0)<0){klog("ioctl fb echec",0);return -1;}
 stride=*(u32*)(fix+48);xres=var[V_XRES];yres=var[V_YRES];bpp=var[V_BPP];
 npages=yres?var[V_YRES_VIRT]/yres:1;if(npages>3)npages=3;
 page=yres?var[V_YOFF]/yres:0;if(page>=npages)page=0;
 if((bpp!=32&&bpp!=16)||!stride||xres<UI_H||yres<UI_W){klog("format fb non gere",0);return -1;}
 fbsz=(s64)stride*yres*(npages?npages:1);
 s64 m=sys6(SYS_mmap,0,fbsz,3/*RW*/,1/*SHARED*/,fbfd,0);
 if(m<0&&m>-4096){klog("mmap fb echec",0);return -1;}
 fbm=(u8*)m;px_setup();
 klogn("fb ok, pages=",npages);return 0;}

/* ------------------------------------------------------------------ tactile */
struct ev{s64 sec,usec;u16 type,code;s32 value;};
#define IOC(d,t,n,s) (((d)<<30)|((s)<<16)|((t)<<8)|(n))
static int tp_open(void){
 char p[64],nm[64];
 for(int t=0;t<25;t++){
  for(int i=0;i<16;i++){
   char*o=pstr(p,"/sys/class/input/event");o=pnum(o,i);pstr(o,"/device/name");
   if(rdfile(p,nm,sizeof nm)<=0||!streq(nm,"mtk-tpd"))continue;
   char d[40];o=pstr(d,"/dev/input/event");pnum(o,i);
   int fd=op(d,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
   if(fd<0){o=pstr(p,"/sys/class/input/event");o=pnum(o,i);pstr(o,"/dev");
    char n[40];o=pstr(n,testmode?"/tmp/bm_ev":"/dev/input/event");pnum(o,i);
    if(mknode(p,n,0020000)==0)fd=op(n,O_RDONLY|O_NONBLOCK|O_CLOEXEC);}
   if(fd>=0){klog("tactile : ",d);return fd;}}
  msleep(40);}
 klog("mtk-tpd introuvable (demarrage auto seulement)",0);return -1;}

/* ------------------------------------------------------------------ actions */
enum{A_NONE=-1,A_TWRP=0,A_ANDROID=1,A_FASTBOOT=2};
static void do_reboot(const char*arg){
 klog("reboot ",arg);sys(SYS_sync,0,0,0,0);msleep(100);
 sys(SYS_reboot,0xfee1dead,672274793,0xA1B2C3D4/*RESTART2*/,(s64)arg);
 sys(SYS_reboot,0xfee1dead,672274793,0x01234567/*RESTART*/,0);
 for(;;)msleep(1000);}

static char**g_argv,**g_envp;
static int find_system(char*node){
 char p[64],b[512];
 for(int t=0;t<100;t++){
  for(int i=1;i<=96;i++){
   char*o=pstr(p,"/sys/class/block/mmcblk0p");o=pnum(o,i);pstr(o,"/uevent");
   if(rdfile(p,b,sizeof b)<=0)continue;
   int ma=-1,mi=-1,ok=0;char*l=b;
   while(*l){if(starts(l,"MAJOR="))ma=atoi_(l+6);else if(starts(l,"MINOR="))mi=atoi_(l+6);
    else if(starts(l,"PARTNAME=system\n")||(starts(l,"PARTNAME=system")&&l[15]==0))ok=1;
    while(*l&&*l!='\n')l++;if(*l)l++;}
   if(ok&&ma>=0&&mi>=0){pstr(node,"/dev/block_system");
    s64 r=sys(SYS_mknodat,AT_FDCWD,(s64)node,0060000|0600,mkdev(ma,mi));
    klogn("system = mmcblk0p",i);return r==0||r==-17?0:-1;}}
  msleep(50);}
 return -1;}

static void boot_android(void){
 if(testmode){klog("test : Android choisi (pas de bascule hors PID 1)",0);sys(SYS_exit,0,0,0,0);}
 char node[40];
 if(find_system(node)<0){klog("partition system introuvable",0);return;}
 sys(SYS_mkdirat,AT_FDCWD,(s64)"/system_root",0755,0);
 /* bloc en lecture seule avant montage (comme fs_mgr_set_blk_ro) : pas d'ecriture de journal */
 {int one=1,fd=op(node,O_RDONLY|O_CLOEXEC);if(fd>=0){sys(SYS_ioctl,fd,0x125d/*BLKROSET*/,(s64)&one,0);cl(fd);}}
 s64 r=sys6(SYS_mount,(s64)node,(s64)"/system_root",(s64)"ext4",1/*MS_RDONLY*/,0,0);
 if(r<0){ /* journal a rejouer : on fait comme le noyau stock (montage ro sur bloc rw) */
  klogn("montage ro strict echec, reessai ",r);
  int zero=0,fd=op(node,O_RDONLY|O_CLOEXEC);if(fd>=0){sys(SYS_ioctl,fd,0x125d,(s64)&zero,0);cl(fd);}
  r=sys6(SYS_mount,(s64)node,(s64)"/system_root",(s64)"ext4",1,0,0);}
 if(r<0){klogn("montage system echec ",r);return;}
 if(fbm)sys(SYS_munmap,(s64)fbm,fbsz,0,0);
 cl(fbfd);
 klog("bascule vers /init de system",0);
 cl(kmsg);kmsg=-1;
 sys(SYS_umount2,(s64)"/dev",2/*MNT_DETACH*/,0,0);
 sys(SYS_umount2,(s64)"/sys",2,0,0);
 sys(SYS_umount2,(s64)"/proc",2,0,0);
 sys(SYS_chdir,(s64)"/system_root",0,0,0);
 sys6(SYS_mount,(s64)".",(s64)"/",0,8192/*MS_MOVE*/,0,0);
 sys(SYS_chroot,(s64)".",0,0,0);
 sys(SYS_chdir,(s64)"/",0,0,0);
 char*av[]={"/init",0};
 sys(SYS_execve,(s64)"/init",(s64)(g_argv&&g_argv[0]?g_argv:av),(s64)g_envp,0);
 /* echec : on ne peut plus rien afficher proprement -> recovery */
 do_reboot("recovery");}

/* ------------------------------------------------------------------ interface */
static int pressed=-1,msg=0,paused=0,dirty=1;static s64 t0,tlast;
static void draw(s64 now){
 blit(SPR_BASE);
 if(pressed>=0)blit(SPR_CARD0_P+pressed);
 if(!paused&&msg<AUTOBOOT_S){s64 e=now-t0;if(e<0)e=0;s64 w=(s64)PROG_W*e/(AUTOBOOT_S*1000);if(w>PROG_W)w=PROG_W;
  fillrect(PROG_X,PROG_Y,(int)w,PROG_H,COL_ACC);}
 else if(msg==MSG_BOOTING)fillrect(PROG_X,PROG_Y,PROG_W,PROG_H,COL_ACC);
 blit(SPR_MSG0+msg);
 present();}

static int card_at(int x,int y){if(y<CARD_Y||y>=CARD_Y+CARD_H)return -1;
 for(int i=0;i<3;i++)if(x>=card_x[i]&&x<card_x[i]+CARD_W)return i;return -1;}

static void choose(int a){
 pressed=a;msg=a==A_TWRP?MSG_TWRP:a==A_FASTBOOT?MSG_FASTBOOT:MSG_BOOTING;draw(now_ms());msleep(a==A_ANDROID?50:350);
 if(a==A_TWRP)do_reboot("recovery");
 if(a==A_FASTBOOT)do_reboot("bootloader");
 boot_android();
 /* system introuvable ou non montable : TWRP */
 pressed=A_TWRP;msg=MSG_NOSYS;draw(now_ms());msleep(2500);do_reboot("recovery");}

static void run(void){
 int tp=-1,have_fb=fb_open()==0;
 if(!have_fb){klog("pas d'affichage : Android direct",0);boot_android();do_reboot("recovery");}
 t0=tlast=now_ms();draw(t0);
 tp=tp_open();
 /* reveil tactile (notification ecran allume, sans extinction) : apres ouverture de mtk-tpd */
 wrfile("/sys/class/graphics/fb0/blank","0");
 t0=tlast=now_ms();
 const int KEYX=1030,RX=1024,RY=600;
 struct ev e[64];s32 rx=0,ry=0;int contact=0,down=0,hit=-1,keyheld=0,woke=0,seen=0;
 for(;;){
  s64 now=now_ms();
  if(!woke&&!seen&&now-t0>1500){woke=1;wrfile("/sys/class/graphics/fb0/blank","0");}  /* 2e reveil, comme touchfix */
  if(!paused){s64 rem=AUTOBOOT_S*1000-(now-t0);if(rem<=0)choose(A_ANDROID);
   int m=AUTOBOOT_S-(int)((rem+999)/1000);if(m<0)m=0;msg=m;}
  else if(now-tlast>60000)choose(A_ANDROID);            /* oubli : Android au bout d'une minute */
  if(!paused||dirty){draw(now);dirty=0;}
  if(tp<0){msleep(40);continue;}
  struct{s32 fd;s16 ev,rev;}pf={tp,1/*POLLIN*/,0};s64 ts[2]={0,40000000};
  if(sys6(SYS_ppoll,(s64)&pf,1,(s64)ts,0,0,0)<=0)continue;
  s64 n=sys(SYS_read,tp,(s64)e,sizeof e,0);if(n<=0)continue;seen=1;
  for(int i=0;i<(int)(n/sizeof(struct ev));i++){struct ev*p=&e[i];
   if(p->type==3&&(p->code==0x35||p->code==0)){rx=p->value;contact=1;continue;}
   if(p->type==3&&(p->code==0x36||p->code==1)){ry=p->value;continue;}
   if(!(p->type==0&&p->code==0))continue;           /* fin de trame : SYN_REPORT */
   tlast=now_ms();
   if(contact){
    if(rx>KEYX){ /* bandeau : HOME (Y 148-230) = Android, BACK (230-313) = TWRP */
     if(!down&&!keyheld){keyheld=1;paused=1;if(ry>=148&&ry<230)choose(A_ANDROID);if(ry>=230&&ry<313)choose(A_TWRP);}}
    else{int lx=(RX-(rx<0?0:rx>RX?RX:rx))*UI_W/RX,ly=(ry<0?0:ry>RY?RY:ry)*UI_H/RY;int c=card_at(lx,ly);
     if(!down){down=1;hit=c;paused=1;}
     pressed=(hit>=0&&c==hit)?hit:-1;}
   }else{
    if(down&&pressed>=0)choose(pressed);
    down=0;hit=-1;pressed=-1;keyheld=0;}
   if(paused)msg=MSG_PAUSED;
   dirty=1;contact=0;}}
}

/* ------------------------------------------------------------------ --dump (tests qemu) */
static void dump(const char*out,int pr,int m,int prog){
 static u8 fake[736*1280*4];fbm=fake;xres=720;yres=1280;stride=736*4;bpp=32;npages=1;
 var[V_ROFF]=0;var[V_GOFF]=8;var[V_BOFF]=16;var[V_TOFF]=24;var[V_TLEN]=8;px_setup();
 pressed=pr;msg=m;paused=(m==MSG_PAUSED);t0=0;
 blit(SPR_BASE);if(pressed>=0)blit(SPR_CARD0_P+pressed);
 if(prog>0)fillrect(PROG_X,PROG_Y,PROG_W*prog/100,PROG_H,COL_ACC);
 blit(SPR_MSG0+msg);present_to(fbm);
 int fd=op(out,O_WRONLY|O_CREAT|O_TRUNC);sys(SYS_write,fd,(s64)fake,sizeof fake,0);cl(fd);}

void start_c(s64*sp){
 int argc=(int)sp[0];char**argv=(char**)(sp+1);char**envp=argv+argc+1;
 g_argv=argv;g_envp=envp;
 if(argc>=6&&streq(argv[1],"--dump")){dump(argv[2],atoi_(argv[3]),atoi_(argv[4]),atoi_(argv[5]));sys(SYS_exit,0,0,0,0);}
 if(argc>=2&&streq(argv[1],"--test"))testmode=1;
 if(!testmode&&sys(SYS_getpid,0,0,0,0)!=1){const char m[]="bootmenu: --test (TWRP) ou /init (PID 1)\n";sys(SYS_write,2,(s64)m,sizeof m-1,0);sys(SYS_exit,1,0,0,0);}
 if(!testmode){
  sys(SYS_mkdirat,AT_FDCWD,(s64)"/proc",0755,0);sys(SYS_mkdirat,AT_FDCWD,(s64)"/sys",0755,0);sys(SYS_mkdirat,AT_FDCWD,(s64)"/dev",0755,0);
  sys6(SYS_mount,(s64)"proc",(s64)"/proc",(s64)"proc",0,0,0);
  sys6(SYS_mount,(s64)"sysfs",(s64)"/sys",(s64)"sysfs",0,0,0);
  sys6(SYS_mount,(s64)"tmpfs",(s64)"/dev",(s64)"tmpfs",2/*NOSUID*/,(s64)"mode=0755",0);
  sys(SYS_mknodat,AT_FDCWD,(s64)"/dev/kmsg",0020000|0600,mkdev(1,11));
  sys(SYS_mkdirat,AT_FDCWD,(s64)"/dev/input",0755,0);
  kmsg=op("/dev/kmsg",O_WRONLY|O_CLOEXEC);}
 else kmsg=op("/dev/kmsg",O_WRONLY|O_CLOEXEC);
 klog("start",testmode?" (test)":0);
 run();}
__attribute__((naked)) void _start(void){asm volatile("mov x0, sp\n bl start_c\n mov x8,#93\n svc 0");}
