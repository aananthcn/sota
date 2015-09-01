.DEFAULT_GOAL := client

# constants
IFLAGS = -I. \
	 -I../client \
	 -I../common \
	 -I../include

CFLAGS = -g ${IFLAGS} -O0
LFLAGS = -ljansson -lssl -lcrypto -lz -L../lib/arm -Wl,-rpath=../lib/arm

MKDIR  = mkdir -p

TARGET = ../sotaclient
ARCH   = arm
ARCHD  = ../obj/${ARCH}
PREFIX = arm-linux-gnueabi-
CC     = ${PREFIX}gcc


# objects
client_objs = tcpclient.o \
	      sotaclient.o

common_objs = unixcommon.o \
	      tcpcommon.o \
	      readline.o \
	      sotajson.o \
	      sotabdata.o \
	      metrics.o \
	      sotamulti.o


client_arm_objs = $(patsubst %.o,${ARCHD}/%.o,$(client_objs))
common_arm_objs = $(patsubst %.o,${ARCHD}/%.o,$(common_objs))


#rules
checkdir:
	$(MKDIR) ${ARCHD}


${ARCHD}/%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@


${ARCHD}/%.o: ../common/%.c
	$(CC) -c $(CFLAGS) $^ -o $@


tcpclient: ${client_arm_objs} ${common_arm_objs}
	$(CC) -o ${TARGET} $^ $(CFLAGS) $(LFLAGS)


clean:
	$(RM) ${client_arm_objs} ${common_arm_objs}
	$(RM) ${TARGET}


client: checkdir tcpclient

