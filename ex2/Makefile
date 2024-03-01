SRC = bw_template.c
# Tar command
TAR = tar
TARFLAGS = -cvf
# Name of the tarball file
TARNAME = 321616401.tgz

# Default target
default: all

all: clean bw_template

bw_template: $(SRC)
	gcc $(SRC) -libverbs -o server && ln -s server client

clean:
	rm -rf ./client ./server

# Target to create a tarball of the source files
tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(SRC) Makefile README Results.pdf