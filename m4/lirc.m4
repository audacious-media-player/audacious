## lirc.m4 (Macros for autoconf)
##
## (C) 1999 Christoph Bartelmus (lirc@bartelmus.de)
## 

#######################################################################
##
##   Check for LIRC
##
#######################################################################

dnl AC_PATH_LIRC([MINIMUM-VERSION])
dnl Check for LIRC and define LIRCD
dnl
AC_DEFUN(AC_PATH_LIRC,
[
  min_lirc_version=ifelse([$1], ,0.5.5,$1)
  AC_MSG_CHECKING(for LIRC>=$min_lirc_version)

  AC_CACHE_VAL(ac_cv_have_lirc,[
    no_lirc=no
    lirc_version=none
    lirc_cross_compiling=no

    if test ! -S /dev/lircd; then
      LIRCD=/dev/null
      no_lirc=yes
    else
      LIRCD=/dev/lircd

      rm -f conf.lirc
      ac_save_cflags="${CFLAGS}"
      CFLAGS="$CFLAGS -DLIRCD=\"$LIRCD\""
      AC_TRY_RUN(
[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#define PACKET_SIZE 256
/* three seconds */
#define TIMEOUT 3

int timeout=0;

void sigalrm(int sig)
{
	timeout=1;
}

const char *read_string(int fd)
{
	static char buffer[PACKET_SIZE+1]="";
	char *end;
	static int ptr=0;
	ssize_t ret;
		
	if(ptr>0)
	{
		memmove(buffer,buffer+ptr,strlen(buffer+ptr)+1);
		ptr=strlen(buffer);
		end=strchr(buffer,'\n');
	}
	else
	{
		end=NULL;
	}
	alarm(TIMEOUT);
	while(end==NULL)
	{
		if(PACKET_SIZE<=ptr)
		{
			ptr=0;
			return(NULL);
		}
		ret=read(fd,buffer+ptr,PACKET_SIZE-ptr);

		if(ret<=0 || timeout)
		{
			if(!timeout)
			{
				alarm(0);
			}
			ptr=0;
			return(NULL);
		}
		buffer[ptr+ret]=0;
		ptr=strlen(buffer);
		end=strchr(buffer,'\n');
	}
	alarm(0);timeout=0;

	end[0]=0;
	ptr=strlen(buffer)+1;
	return(buffer);
}

enum packet_state
{
	P_BEGIN,
	P_MESSAGE,
	P_STATUS,
	P_DATA,
	P_N,
	P_DATA_N,
	P_END
};

char *get_version(int fd,const char *packet)
{
	int done,todo;
	const char *string,*data;
	char *endptr;
	enum packet_state state;
	int status,n;
	unsigned long data_n;
	unsigned int major,minor,micro;
	static char version[100];

	todo=strlen(packet);
	data=packet;
	while(todo>0)
	{
		done=write(fd,(void *) data,todo);
		if(done<0)
		{
			return(NULL);
		}
		data+=done;
		todo-=done;
	}

	/* get response */
	status=0;
	state=P_BEGIN;
	n=0;
	while(1)
	{
		string=read_string(fd);
		if(string==NULL) return(NULL);
		switch(state)
		{
		case P_BEGIN:
			if(strcasecmp(string,"BEGIN")!=0)
			{
				continue;
			}
			state=P_MESSAGE;
			break;
		case P_MESSAGE:
			if(strncasecmp(string,packet,strlen(string))!=0 ||
			   strlen(string)+1!=strlen(packet))
			{
				state=P_BEGIN;
				continue;
			}
			state=P_STATUS;
			break;
		case P_STATUS:
			if(strcasecmp(string,"SUCCESS")==0)
			{
				status=0;
			}
			else if(strcasecmp(string,"END")==0)
			{
				status=0;
				return(NULL);
			}
			else if(strcasecmp(string,"ERROR")==0)
			{
				status=-1;
			}
			else
			{
				goto bad_packet;
			}
			state=P_DATA;
			break;
		case P_DATA:
			if(strcasecmp(string,"END")==0)
			{
				return(NULL);
			}
			else if(strcasecmp(string,"DATA")==0)
			{
				state=P_N;
				break;
			}
			goto bad_packet;
		case P_N:
			errno=0;
			data_n=strtoul(string,&endptr,0);
			if(!*string || *endptr)
			{
				goto bad_packet;
			}
			if(data_n==0)
			{
				state=P_END;
			}
			else
			{
				state=P_DATA_N;
			}
			break;
		case P_DATA_N:
			if(data_n==1 && status==0)
			{
				if(sscanf(string,"%u.%u.%u",
					  &major,&minor,&micro)==3)
				{
					sprintf(version,"%u.%u.%u",
						major,minor,micro);
				}
				else
				{
					goto bad_packet;
				}
			}
			n++;
			if(n==data_n) state=P_END;
			break;
		case P_END:
			if(strcasecmp(string,"END")==0)
			{
				return(version);
			}
			goto bad_packet;
			break;
		}
	}
 bad_packet:
	return(NULL);
}

int main(int argc,char **argv)
{
	char *version,*min_version;
	unsigned int major,minor,micro,min_major,min_minor,min_micro;
	struct sockaddr_un addr;
	FILE *result;
	int fd;
	struct sigaction act;

	result=fopen("conf.lirc","w");
	if(result==NULL) exit(EXIT_FAILURE);
	
	act.sa_handler=sigalrm;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;           /* we need EINTR */
	sigaction(SIGALRM,&act,NULL);

	addr.sun_family=AF_UNIX;
	strcpy(addr.sun_path,LIRCD);
	fd=socket(AF_UNIX,SOCK_STREAM,0);
	if(fd==-1)
	{
		fprintf(result,"unknown");
		fclose(result);
		close(fd);
		exit(EXIT_FAILURE);
	};
	if(connect(fd,(struct sockaddr *)&addr,sizeof(addr))==-1)
	{
		fprintf(result,"unknown");
		fclose(result);
		close(fd);
		exit(EXIT_FAILURE);
	};

	version=get_version(fd,"VERSION\n");
	if(version==NULL)
	{
		fprintf(result,"<0.5.5");
		fclose(result);
		close(fd);
		exit(EXIT_FAILURE);
	}
	fprintf(result,"%s",version);
	fclose(result);
	close(fd);

	/* HP/UX 9 (%@#!) writes to sscanf strings */

	min_version=strdup("$min_lirc_version");
	if(min_version==NULL) exit(EXIT_FAILURE);

	if(sscanf(version,"%u.%u.%u",&major,&minor,&micro)!=3 ||
	   sscanf(min_version,"%u.%u.%u",&min_major,&min_minor,&min_micro)!=3)
	{
		exit(EXIT_FAILURE);
	}
	if(major<min_major ||
	   (major==min_major && minor<min_minor) ||
	   (major==min_major && minor==min_minor && micro<min_micro))
	{
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
],
        if test -f conf.lirc; then
          lirc_version=`cat conf.lirc`

##        lirc_major_version=`cat lirc.conf | \
##          sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
##        lirc_minor_version=`cat lirc.conf | \
##          sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
##        lirc_micro_version=`cat lirc.conf | \
##          sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
        else
          no_lirc=yes
        fi,
        no_lirc=yes
        if test -f conf.lirc; then
          lirc_version=`cat conf.lirc`
        else
          lirc_version=none
        fi
        ,lirc_cross_compiling=yes
      )
## AC_TRY_RUN()      
      rm -f conf.lirc
      CFLAGS="$ac_save_CFLAGS"
    fi
    ac_cv_have_lirc="no_lirc=${no_lirc} lirc_version=\"${lirc_version}\" \
                     lirc_cross_compiling=${lirc_cross_compiling} \
                     LIRCD=${LIRCD}"
  ])
## AC_CACHE_VAL

  eval "$ac_cv_have_lirc"

  if test x${no_lirc} = xyes; then
    if test x${lirc_version} = xunknown; then
      AC_MSG_RESULT([missing (lircd not running ?)])
    elif test x${lirc_version} != xnone; then
      AC_MSG_RESULT([missing (found lirc-${lirc_version})])
    else
      AC_MSG_RESULT(missing)
    fi
  else
    if test x${lirc_cross_compiling} = xyes; then
      AC_MSG_RESULT(found)
    else
      AC_MSG_RESULT([found lirc-${lirc_version}])
    fi
  fi

  AC_DEFINE_UNQUOTED(LIRCD,"${LIRCD}")
]
)
