SRC = ex3_for_sub_final_touches.c
# Tar command
TAR = tar
TARFLAGS = -cvf
# Name of the tarball file
TARNAME = itay_omer_maya.tgz

# Default target
default: all

all: clean ex3_for_sub_final_touches

ex3_for_sub_final_touches: $(SRC)
	gcc $(SRC) -libverbs -o server && ln -s server client 

clean:
	rm -rf ./client ./server

# Target to create a tarball of the source files
tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(SRC) Makefile README Results.pdf