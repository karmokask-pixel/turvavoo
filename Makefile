# Kompilaator (vaikimisi gcc; clang-iga: make CC=clang test)
CC      ?= gcc

# Ranged kompilaatori hoiatused; -Werror muudab iga hoiatuse veaks.
CFLAGS  := -std=c99 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion \
           -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
           -Wswitch-default -Wundef -Werror

# Need sihtmärgid ei ole failinimed.
.PHONY: all test coverage misra clean

# "make" ilma argumendita kompileerib testprogrammi.
all: test_Sbr

# Testprogramm kompileeritakse uuesti, kui mõni neist failidest muutub.
test_Sbr: Sbr.c test_Sbr.c Sbr.h Sbr_Cfg.h Std_Types.h
	$(CC) $(CFLAGS) -o $@ Sbr.c test_Sbr.c

# Kompileerib ja käivitab testid.
test: test_Sbr
	./test_Sbr

# Mõõdab, kui suur osa Sbr.c ridadest ja harudest testidega läbiti (gcov).
coverage:
	$(CC) -std=c99 -O0 --coverage -o test_Sbr_cov Sbr.c test_Sbr.c
	./test_Sbr_cov > /dev/null
	gcov -b -c -o . test_Sbr_cov-Sbr.gcno | head -5

# MISRA C:2012 kontroll cppchecki MISRA lisamooduliga (ainult moodul, mitte testid).
misra:
	cppcheck --addon=misra --enable=style,warning,performance,portability \
	         --std=c99 --error-exitcode=1 -q Sbr.c && echo "MISRA: no findings"

# Kustutab kompileeritud ja katvuse failid.
clean:
	rm -f test_Sbr test_Sbr_cov *.gcda *.gcno *.gcov
