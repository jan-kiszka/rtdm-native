APPS = canconfig cansend canrecv rtdmtest rtcan_rtt irqloop irqbench \
       rtdmtest-task

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I../../include

all: $(APPS)

$(APPS): %: %.c
	$(CC) $(CFLAGS) -o $@ $< -pthread -lrt

clean:
	rm $(APPS)
