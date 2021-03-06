.DEFAULT_GOAL := server 

# constants
IFLAGS = -I. \
	 -I../server \
	 -I../common \
	 -I../client

CFLAGS = -g ${IFLAGS}
LFLAGS = -ljansson -lmysqlclient -lssl -lcrypto

MKDIR  = mkdir -p

TARGET = ../sotaserver
ARCH   = intel
ARCHD  = ../obj/${ARCH}
#CC     = ${BUILD_CC}


# objects
server_objs = tcpserver.o \
	      sotaserver.o \
	      sotadb.o

common_objs = unixcommon.o \
	      tcpcommon.o \
	      readline.o \
	      sotajson.o \
	      sotabdata.o \
	      swreleases.o \
	      sotacache.o \
	      sotamulti.o


server_arm_objs = $(patsubst %.o,${ARCHD}/%.o,$(server_objs))
common_arm_objs = $(patsubst %.o,${ARCHD}/%.o,$(common_objs))


#rules
checkdir:
	$(MKDIR) ${ARCHD}


${ARCHD}/%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

${ARCHD}/%.o: ../common/%.c
	$(CC) -c $(CFLAGS) $^ -o $@

tcpserver: ${server_arm_objs} ${common_arm_objs}
	$(CC) -o ${TARGET} $^ $(CFLAGS) $(LFLAGS)


clean:
	$(RM) ${server_arm_objs} ${common_arm_objs}
	$(RM) ${TARGET}


server: checkdir tcpserver

