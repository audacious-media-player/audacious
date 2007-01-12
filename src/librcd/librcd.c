#include <stdio.h>
#include <string.h>

#define _LIBRCD_C
#include "librcd.h"

#define NF_VALUE -2
#define max(a,b) ((a>b)?a:b)
#define min(a,b) ((a<b)?a:b)
#define bit(i) (1<<i)

typedef struct lng_stat2 {
  unsigned char a;
  unsigned char b;
  double rate;
  double srate;
  double erate;
} lng_stat2;

#include "russian_table.h"


static int end_symbol(char ch) {
    if (ch=='\r'||ch=='\n'||ch==0||ch==' '||ch=='\t'||ch==','||ch=='.'||ch=='!'||ch=='?'||ch==';'||ch=='-'||ch==':'||ch=='"'||ch=='\''||ch==')') return 1;
    return 0;
}

static int start_symbol(char ch) {
    if ((ch=='\t')||ch=='\r'||ch=='\n'||(ch==' ')||(ch=='(')||(ch=='"')||(ch=='\'')) return 1;
    return 0;
}    

typedef const struct lng_stat2 *lng_stat2_ptr;

static void bfind(const unsigned char *a, lng_stat2_ptr *w, lng_stat2_ptr *k, lng_stat2_ptr *al) {
  const struct lng_stat2 *winptr, *koiptr,*altptr;
  int ki,wi,ai,d,ws=0,ks=0,as=0;
    d=npow2>>1;
    wi=d;
    ki=d;
    ai=d;
    winptr=0;
    koiptr=0;
    altptr=0;
    do{
      d>>=1;
    
      if(!ws){
       if (wi>indexes2) wi-=d;
       else {
        winptr=enc_win+wi-1;
        if(a[0]==winptr->a){
          if(a[1]==winptr->b){
        	ws=1;
          }else if(a[1]<winptr->b){
            wi-=d;
          }else{ //b>win[wi].b
            wi+=d;
          }
        }else if(a[0]<winptr->a){
          wi-=d;
        }else{ //a>win[wi].a
          wi+=d;
        }
       }
      }
      if(!ks){
       if (ki>indexes2) ki-=d;
       else {
        koiptr=enc_koi+ki-1;
        if(a[0]==koiptr->a){
          if(a[1]==koiptr->b){
        	ks=1;
          }else if(a[1]<koiptr->b){
            ki-=d;
          }else{ //b>win[wi].b
            ki+=d;
          }
        }else if(a[0]<koiptr->a){
          ki-=d;
        }else{ //a>win[wi].a
          ki+=d;
        }
       }
      }
      if(!as){
       if (ai>indexes2) ai-=d;
       else {
        altptr=enc_alt+ai-1;
        if(a[0]==altptr->a){
          if(a[1]==altptr->b){
        	as=1;
          }else if(a[1]<altptr->b){
            ai-=d;
          }else{ //b>win[wi].b
            ai+=d;
          }
        }else if(a[0]<altptr->a){
          ai-=d;
        }else{ //a>win[wi].a
          ai+=d;
        }
       }
      }
    }while(d);
    if (ws) *w=winptr;
    else *w=NULL;
    if (ks) *k=koiptr;
    else *k=NULL;
    if (as) *al=altptr;
    else *al=NULL;
}

static double calculate(double s, double m, double e) {
    return s+m+e;
}

static int is_win_charset2(const unsigned char *txt, int len){
  const struct lng_stat2 *winptr, *koiptr,*altptr;
  double winstep,koistep,altstep,winestep,koiestep,altestep,winsstep,koisstep,altsstep;
  double winstat=0,koistat=0,altstat=0,winestat=0,koiestat=0,altestat=0,winsstat=0,koisstat=0,altsstat=0;
  long j;
  
#ifdef _AUTO_DEBUG
  fprintf(stderr,"Word: %s\n",txt);
#endif
  for(j=0;j<len-1;j++){
    //skip bottom half of table
    if(txt[j]<128 || txt[j+1]<128) continue;
#ifdef _AUTO_DEBUG
    fprintf(stderr,"Pair: %c%c",txt[j],txt[j+1]);
#endif
    bfind(txt+j,&winptr,&koiptr,&altptr);

    if ((j==0)||(start_symbol(txt[j-1]))) {
	if (winptr) winsstep=winptr->srate;
	else winsstep=NF_VALUE;
	if (koiptr) koisstep=koiptr->srate;
	else koisstep=NF_VALUE;
	if (altptr) altsstep=altptr->srate;
	else altsstep=NF_VALUE;
	winestep=0;
	koiestep=0;
	altestep=0;
	winstep=0;
	koistep=0;
	altstep=0;
#ifdef _AUTO_DEBUG
	fprintf(stderr,", Win %lf, Koi %lf, Alt: %lf\n",winsstep,koisstep,altsstep);
#endif
    } else if ((j==len-2)||(end_symbol(txt[j+2]))) {
	if (winptr) winestep=winptr->erate;
	else winestep=NF_VALUE;
	if (koiptr) koiestep=koiptr->erate;
	else koiestep=NF_VALUE;
	if (altptr) altestep=altptr->erate;
	else altestep=NF_VALUE;
	winsstep=0;
	koisstep=0;
	altsstep=0;
	winstep=0;
	koistep=0;
	altstep=0;
#ifdef _AUTO_DEBUG
	fprintf(stderr,", Win %lf, Koi %lf, Alt %lf\n",winestep,koiestep,altestep);
#endif
    } else {
	if (winptr) winstep=winptr->rate;
	else winstep=NF_VALUE;
	if (koiptr) koistep=koiptr->rate;
	else koistep=NF_VALUE;
	if (altptr) altstep=altptr->rate;
	else altstep=NF_VALUE;
	winsstep=0;
	winestep=0;
	koisstep=0;
	koiestep=0;
	altsstep=0;
	altestep=0;
#ifdef _AUTO_DEBUG
	fprintf(stderr,", Win %lf, Koi %lf, Alt %lf\n",winstep,koistep,altstep);
#endif
    }
    
    winstat+=winstep;
    koistat+=koistep;
    altstat+=altstep;
    winsstat+=winsstep;
    koisstat+=koisstep;
    altsstat+=altsstep;
    winestat+=winestep;
    koiestat+=koiestep;
    altestat+=altestep;
  }
  
#ifdef _AUTO_DEBUG
  fprintf(stderr,"Start.  Win: %lf, Koi: %lf, Alt: %lf\n",winsstat,koisstat,altsstat);
  fprintf(stderr,"Middle. Win: %lf, Koi: %lf, Alt: %lf\n",winstat,koistat,altstat);
  fprintf(stderr,"End.    Win: %lf, Koi: %lf, Alt: %lf\n",winestat,koiestat,altestat);
  fprintf(stderr,"Final.  Win: %lf, Koi: %lf, Alt: %lf\n",calculate(winsstat,winstat,winestat),calculate(koisstat,koistat,koiestat),calculate(altsstat,altstat,altestat));
#endif
  if ((calculate(altsstat,altstat,altestat)>calculate(koisstat,koistat,koiestat))&&(calculate(altsstat,altstat,altestat)>calculate(winsstat,winstat,winestat))) return 3;
  if (calculate(koisstat,koistat,koiestat)>calculate(winsstat,winstat,winestat)) return 1;
  return 0;
}


static int check_utf8(const unsigned char *buf, int len) {
    long i,j;
    int bytes=0,rflag=0;
    unsigned char tmp;
    int res=0;

    for (i=0;i<len;i++) {
	if (buf[i]<128) continue;
	
	if (bytes>0) {
	    if ((buf[i]&0xC0)==0x80) {
		if (rflag) {
		    tmp=buf[i]&0x3F;
		    // Russian is 0x410-0x44F
		    if ((rflag==1)&&(tmp>=0x10)) res++;
		    else if ((rflag==2)&&(tmp<=0x0F)) res++;
		}
		bytes--;
	    } else {
		res--;
		bytes=1-bytes;
		rflag=0;
	    }
	} else {
	    for (j=6;j>=0;j--)
		if ((buf[i]&bit(j))==0) break;
	    
	    if ((j==0)||(j==6)) {
		if ((j==6)&&(bytes<0)) bytes++;
		else res--;
		continue;
	    }
	    bytes=6-j;
	    if (bytes==1) {
		// Cyrrilic D0-D3, Russian - D0-D1
		if (buf[i]==0xD0) rflag=1;
		else if (buf[i]==0xD1) rflag=2;
	    }
	}
	    
	if ((buf[i]==0xD0)||(buf[i]==0xD1)) {
	    if (i+1==len) break;
	    	    
	}
    }
    return res;
}



rcd_russian_charset rcdGetRussianCharset(const char *buf,int len) {
    long l;

    l = len?len:strlen(buf);
    if (check_utf8((const unsigned char *)buf,l)>1) return RUSSIAN_CHARSET_UTF8;
    return is_win_charset2((const unsigned char *)buf,l);
}

/* Compatibility */
rcd_russian_charset get_russian_charset(const char *buf,int len) {
    return rcdGetRussianCharset(buf, len);
}
