.DEFAULT_GOAL := server 

# constants
IFLAGS = -I. \
	 -I../server \
	 -I../common

CFLAGS = -g ${IFLAGS}
LFLAGS = -ljansson -lmysqlclient

MKDIR  = mkdir -p

TARGET = ../sotaserver
ARCH   = intel
ARCHD  = ../obj/${ARCH}
PREFIX = 
CC     = ${PREFIX}gcc


# objects
server_objs = tcpserver.o \
	      sotaserver.o \
	      sotadb.o

common_objs = unixcommon.o \
	      tcpcommon.o \
	      readline.o \
	      sotajson.o


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

