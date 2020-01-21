PROGRAM?=
SOURCES=$(PROGRAM).c #evp_cript.c

SRCDIR=	applications/$(PROGRAM)
OUTDIR=	out

OBJECTS= $(addprefix $(SRCDIR)/, $(SOURCES))
PROGRAMPATH= $(addprefix $(OUTDIR)/, $(PROGRAM))

CC= gcc # -s -march=native -flto -mtune=native -Os -Ofast -ffunction-sections -fdata-sections 		# strip, optimize for performance and then (mainly) for size. After that place all functions and data to separate sections
WARNINGS= -W -Wall -ansi -Wextra -pedantic -Wstrict-overflow=5 -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes # turn on all possible warnings
LINKER= -lpaho-mqtt3c -lm -lpthread -lcrypto # -flto -Wl,-Map=$(PROGRAM).map,--cref,--gc-section -Wl,--build-id=none		# and with linker delete unneeded ones
DEBUG= -ggdb

export SOURCES
export DEBUG
export COMPILER
export WARNINGS

all: $(PROGRAMPATH)

$(PROGRAMPATH): 
	mkdir -p $(OUTDIR)
	$(CC) $(OBJECTS) -o $@ $(DEBUG) $(LINKER)

clean:
	rm -rf out/$(PROGRAM)*
