# program: FreeDOS FORMAT.EXE message set - version 0.92a and later
# language:Polish
# display codepage:CP852, (CR+LF)
# catalog codepage:CP852
# translators:Piotr Okrutny

# version (on help screen)
0,0:FreeDOS %6s - wersja %s\n
0,1:Autorzy: Brian E. Reifsnyder, Eric Auer i inni.\n
0,2:Copyright 1999 - 2024 na warunkach licencji GNU GPL w wersji 2 lub nowszej.\n\n
# 0.3 or 0.4 shown depending on if /Z:longhelp option used or not
0,3:Składnia (szczegóły i informacje dodatkowe w dokumentacji):\n\n
0,4:Składnia (więcej opcji w dokumentacji lub po użyciu /Z:longhelp):\n\n

# common
1,0:\n
# these two options for Yes and No are only 1 character and must be upper case
1,1:T
1,2:N
# user has to type full word matching this
1,3:TAK
1,4:NIE
# other possibly common bits
1,5:Błąd!\n
1,6:Błąd.\n
1,7:Przerywanie.\n

# short help screen (shown if compiled without LEGACY_HELP)
# Use as many messages as needed, 1 message per max 79 column line
# but if possible keep to less than 23 lines
# message numbers do not need to match up for messages for different languages
# Each message should end with a newline \n
# after every 23 messages user will be prompted to press a key to continue
2,0:FORMAT dysk: [/V[:etykieta]] [/Q] [/U] [/F:rozmiar] [/S] [/D]\n
2,1:FORMAT dysk: [/V[:etykieta]] [/Q] [/U] [/T:ścieżki /N:sektory] [/S] [/D]\n
#  /* the /4 option is a legacy shorthand for size selection: 360k in 1.2M drive */
#  /* (drive type detection and "double stepping" setting are automatic on ATs.) */
2,2:FORMAT dysk: [/V[:etykieta]] [/Q] [/U] [/4] [/S] [/D]\n
2,3:\n
2,4: /V:etykieta Określa etykietę woluminu; zapisuje też datę i godzinę.\n
2,5: /S         Uruchamia SYS: dysk będzie rozruchowy, z plikami systemowymi.\n
2,6: /D         Pokazuje szczegółowe dane diagnostyczne (do zgłaszania błędów).\n
2,7: /Q         Formatuje szybko. Bez /U można potem użyć UNFORMAT, a oznaczenia\n
2,8:            uszkodzonych klastrów zostają (przy /Q /U nie).\n
2,9: /U         Formatuje bezwarunkowo. Dyskietkę formatuje niskopoziomowo.\n
2,10: /F:rozmiar Określa rozmiar dyskietki do sformatowania. Typowe rozmiary:\n
2,11:            360, 720, 1200, 1440 lub 2880 (w kilobajtach). /F:0 pokazuje listę.\n
2,12: /4         Formatuje dyskietkę 360 kB w stacji 1,2 MB.\n
2,13: /T:ścieżki Określa liczbę ścieżek na dyskietce.\n
2,14: /N:sektory Określa liczbę sektorów na ścieżkę dyskietki.\n

# short help screen (shown if compiled _with_ LEGACY_HELP)
# Use as many messages as needed, 1 message per max 79 column line
# but if possible keep to less than 23 lines
# message numbers do not need to match up for messages for different languages
# Each message should end with a newline \n
# after every 23 messages user will be prompted to press a key to continue
20,0:FORMAT dysk: [/V[:etykieta]] [/Q] [/U] [/F:rozmiar] [/B | /S] [/D]\n
20,1:FORMAT dysk: [/V[:etykieta]] [/Q] [/U] [/T:ścieżki /N:sektory] [/B | /S] [/D]\n
20,2:FORMAT dysk: [/V[:etykieta]] [/Q] [/U] [/4] [/B | /S] [/D]\n
20,3:FORMAT dysk: [/Q] [/U] [/1] [/4] [/8] [/B | /S] [/D]\n
20,4:\n
20,5: /V:etykieta Określa etykietę woluminu; zapisuje też datę i godzinę.\n
20,6: /S         Uruchamia SYS: dysk będzie rozruchowy, z plikami systemowymi.\n
20,7: /B         Dla zgodności; dawniej rezerwował miejsce na pliki systemowe.\n
20,8: /D         Pokazuje szczegółowe dane diagnostyczne (do zgłaszania błędów).\n
20,9: /Q         Formatuje szybko. Bez /U można potem użyć UNFORMAT, a oznaczenia\n
20,10:            uszkodzonych klastrów zostają (przy /Q /U nie).\n
20,11: /U         Formatuje bezwarunkowo. Dyskietkę formatuje niskopoziomowo.\n
20,12: /F:rozmiar Określa rozmiar dyskietki do sformatowania. Typowe rozmiary:\n
20,13:            360, 720, 1200, 1440 lub 2880 (w kilobajtach). /F:0 pokazuje listę.\n
20,14: /4         Formatuje dyskietkę 360 kB w stacji 1,2 MB.\n
20,15: /T:ścieżki Określa liczbę ścieżek na dyskietce.\n
20,16: /N:sektory Określa liczbę sektorów na ścieżkę dyskietki.\n
20,17: /1         Formatuje jedną stronę dyskietki (160 kB / 180 kB).\n
20,18: /8         Formatuje dyskietkę 5,25\" z 8 sektorami/ścieżkę (160 kB / 320 kB).\n

# detailed help screen
# Use as many messages as needed, 1 message per max 79 column line
# message numbers do not need to match up for messages for different languages
# Each message should end with a newline \n
# after every 23 messages user will be prompted to press a key to continue
3,0:Ten FORMAT powstał dla projektu http://www.freedos.org/.\n
3,1:  Informacje o licencji GNU GPL: http://www.gnu.org/\n
3,2:Autor (1999-2003): Brian E. Reifsnyder <reifsnyderb@mindspring.com>\n
3,3:  Opiekun wersji 0.90 / 0.91 (2003-2006): Eric Auer <eric@coli.uni-sb.de>\n
3,4:Współtwórcy: Jan Verhoeven, John Price, James Clark, Tom Ehlert,\n
3,5:  Bart Oldeman, Jim Hall i inni. Nie zapominamy o wszystkich testerach!\n
3,6:\n
3,7:Przełączniki i dodatkowe funkcje:\n
3,8:/D (diagnostyka) i /Y (bez pytania o potwierdzenie) są zawsze dozwolone.\n
3,9:/B (miejsce na SYS) nic nie robi i nie łączy się z /S (SYS).\n
3,10:/V:etykieta nie dla dyskietek 160k/320k. Etykieta zapisuje datę formatowania.\n
3,11:\n
3,12:Rozmiar można podać tylko dla dyskietek. Możesz użyć\n
3,13:albo /F:rozmiar (w kilobajtach; rozmiar 0 wyświetla listę dozwolonych),\n
3,14:albo /T:ścieżki /N:sektory_na_ścieżkę,\n
3,15:albo dowolnego połączenia /1 (jedna strona, 160k/180k),\n
3,16:                      /8 (8 sektorów na ścieżkę, 160k/320k, DOS 1.x)\n
3,17:                  oraz /4 (dyskietka 160-360k w stacji 1200k)\n
3,18:\n
3,19:Aby nie pytać o potwierdzenie formatowania dysku twardego: /Z:seriously\n
3,20:Aby tylko zapisać dane do UNFORMAT (mirror) bez formatowania: /Z:mirror\n
3,21:Aby przywrócić dysk (UNFORMAT) z aktualnych danych mirror:   /Z:unformat\n
3,22:\n
3,23:Tryby dla DYSKIETEK: najpierw szybkie bezpieczne formatowanie, niskopoziomowe\n
3,24:  tylko w razie potrzeby. Bezpieczne zapisuje dane mirror do UNFORMAT.\n
3,25:Tryby dla DYSKÓW TWARDYCH: najpierw szybkie bezpieczne formatowanie, szybkie\n
3,26:  pełne tylko w razie potrzeby. Szybkie pełne tylko zeruje system plików.\n
3,27:Aby wymusić formatowanie niskopoziomowe (dyskietka) albo sprawdzić całą\n
3,28:  powierzchnię i wymazać całą zawartość (dysk twardy), użyj /U.\n
3,29:  FORMAT /Q /U to szybkie pełne formatowanie (bez niskiego poziomu i wymazania)\n
3,30:  FORMAT /Q to szybkie bezpieczne formatowanie (z danymi mirror, gdy się da)\n
3,31:    dane mirror zawsze nadpisują koniec obszaru danych!\n
3,32:  Bez /Q i bez /U FORMAT sam wybiera tryb (patrz wyżej).\n
3,33:\n
3,34:Obsługiwane typy FAT: FAT12, FAT16, FAT32, wszystkie z mirror / UNFORMAT.\n
3,35:Obsługiwane dyskietki: 160k 180k 320k 360k i 1200k w stacjach 5,25 cala\n
3,36:  oraz 720k i 1440k (2880k dotąd nietestowane) w stacjach 3,5 cala.\n
#3,?:DD drives are limited to 360k/720k respectively. 2880k is ED drives only.\n
3,37:Obsługiwane nadformaty: 400k 800k 1680k (i 3660k) z większą liczbą sektorów\n
3,38:  oraz 1494k (zamiast 1200k), 1743k (i 3486k) także z większą liczbą ścieżek.\n
3,39:  Więcej ścieżek działa nie we wszystkich stacjach - na własne ryzyko.\n
3,40:  Uwaga: starsze wersje DOS-u obsługują nadformaty tylko ze sterownikiem.\n
3,41:  720k w stacji 1440k wymaga dyskietki 720k. 360k formatuj w stacji 360k.\n
3,42:\n
3,43:Przy FAT32 przełącznik /A wymusza wyrównanie do 4k.\n

# main()
4,0:Niedozwolony znak w etykiecie woluminu: %c\n
4,1:Dozwolone są 0-9, A-Z, spacja, znaki specjalne / narodowe 128-255\n
4,2:oraz wszystkie z
# the following is printed for valid label characters from ordinal(' ')+1 through 127
4,3: %c
4,4:\nale bez znaków sterujących i żadnego z
# and now for enumerating illegal characters from ordinal(' ')+1 through 127
4,5: %c
# /F:size
4,6:Standardowe: 160, 180, 320, 360, 720, 1200, 1440, 2880.\n
4,7:Specjalne:   400, 800, 1680, 3360,    1494, 1743, 3486.\n
# /T tracks (cylinders)
4,8:Dobre: 40, 80. ???: 83.\n
# /N sectors per track
4,9:Standardowe: 8, 9, 15, 18, 36.\n
4,10:Specjalne:   10, 21, 42.\n
# other options
4,11:Nieznany przełącznik: /%c\n
4,12:Trzeba podać literę dysku.\n
4,13:Nie można użyć /T razem z /N.\n
4,14:Formatowanie zmieni rozmiar - wymuszam pełne formatowanie\n
4,15:Było: %d sekt./ścieżkę, głowice: %d. Będzie: %d sekt., głowice: %d\n
4,16:Nie można odnaleźć istniejącego formatu - wymuszam pełne formatowanie\n
4,17:Nie można odnaleźć istniejącego formatu - dane UNFORMAT nie zostaną zapisane.\n
4,18:Zapisywanie kopii sektorów systemowych na końcu dysku:\n
4,19:sektor rozruchowy, jedna tablica FAT, katalog główny. Przydatne dla UNFORMAT.\n
4,20:Nadpisywanie sektora rozruchowego, tablic FAT i katalogu głównego\n
4,21:danymi MIRROR/UNFORMAT zapisanymi wcześniej.\n
# message 4,22 should never be reached/shown
4,22:/Z:co???\n
4, 23: Pełne formatowanie (wymazywanie wszystkich danych)\n
4, 24: Szybkie formatowanie (tylko zerowanie systemu plików)\n
4, 25: Uwaga: usuwa oznaczenia uszkodzonych klastrów, jeśli są.\n
4, 26: Bezpieczne szybkie formatowanie (z próbą zapisu danych UNFORMAT)\n
# 4,27 through 4,32 display: <action> complete\n
4,27:Formatowanie
4,28:Szybkie formatowanie
4,29:Bezpieczne szybkie formatowanie
4,30:Mirror
4,31:UnFormat
4,32: - zakończone.\n
# 4,33 thru 4,35 are to ask user to repeat previous action
4,33:Sformatować
4,34:Przetworzyć
4,35: następną dyskietkę (t/n)?
# 4,36 thru 4,38 tell user we are repeating action
4,36:Przetwarzanie
4,37:Formatowanie
4,38:%s następnej dyskietki...\n

# the following are long option keywords /Z:keyword
5,0:Nieprawidłowy tryb /Z - dozwolone: MIRROR, UNFORMAT, SERIOUSLY\n
# take a new mirror data snapshot
5,1:mirror
# revert to mirrored state, dangerous!
5,2:unformat
# User MEANS to format harddisk
5,3:seriously
# display LONG (detailed) version of help
5,4:longhelp

# Check_Remote_Subst()
6,0:Nie można formatować dysków sieciowych ani SUBST (kod %04x). Przerywanie.\n
6,1:Nieprawidłowy dysk! Przerywanie.\n
6,2:Nie można formatować dysków ASSIGN, JOIN ani SUBST. Przerywanie.\n

# Check_For_Format()
7,0:  Nie można odczytać sektora rozruchowego, dysk nie jest jeszcze sformatowany\n
7,1: Nieprawidłowy sektor rozruchowy - dysk nie jest jeszcze sformatowany (FAT)\n
7,2:Nieprawidłowy %sBPB (kod 0x%x). Dysk NIE jest jeszcze sformatowany.\n

# Write_System_Files()
8,0:\nOSTRZEŻENIE: brak SYS w ścieżce PATH - nie można zainstalować plików systemu!\n
8,1:\nUruchamianie SYS: %s %s\n
8,2:\nUruchamianie SYS w powłoce: %s\n
8,3:\nSYS zwrócił kod błędu (errorlevel) %d.\n
8,4:\nOSTRZEŻENIE: uruchomienie SYS nie powiodło się.\n

# Ask_User_To_Insert_Disk()
9,0: Włóż nową dyskietkę do stacji dysków %c:\n
9,1: Włóż nowy dysk,\n
9,2: Naciśnij ENTER, gdy właściwy dysk będzie w stacji...

# Confirm_Hard_Drive_Formatting()
10,0:UNFORMAT przywróci katalog główny i tablicę FAT do stanu\n
10,1:zapisanego wcześniej. To może poważnie namieszać!\n
10,2: OSTRZEŻENIE: WSZYSTKIE DANE NA %s\n
10,3:DYSKU NIEWYMIENIALNYM
10,4:DYSKIETCE
10,5: (%c:) ZOSTANĄ UTRACONE!
10,6: (%c:) MOGĄ ZOSTAĆ UTRACONE!
10,7: POTWIERDŹ!\n
# note: will be followed by " (YES/NO)? " from msg 1.3 & 1.4
10,8: Kontynuować formatowanie
10,9: Kontynuować UNFORMAT
# %s will be replaced with msg from 1.3
10,10:Aby potwierdzić, wpisz całe słowo %s.

# Ask_Label()
11,0:Wpisz etykietę woluminu (maks. 11 znaków):
11,1:Bez etykiety - na dysku nie będzie daty utworzenia.\n

# Critical_Error_Handler()
12,0:\n Błąd krytyczny przy dostępie do dysku (%s)\n
12,1:\n Program przerwany.\n

# Display_Drive_Statistics()
13,0:\n Numer seryjny woluminu: %04X-%04X\n
13,1:bajtów
13,2:kB
13,3:MB
13,4:Sektor nie ma 512 bajtów - statystyki będą błędne.\n
# %13s%c%s %s becomes <number><decimal separator>[0|5] [kbytes|bytes]
13,5 :%13s%c%s %s całkowitego miejsca na dysku (rozmiar dysku)\n
13,6 :%13s%c%s %s w uszkodzonych sektorach\n
13,7 :%13s%c%s %s w klastrach z uszkodzonymi sektorami\n
13,8 :%13s%c%s %s dostępnych na dysku (wolne klastry)\n
13,9 :%13s%c%s %s w każdej jednostce alokacji.\n
# second %s replaced with "  " for FAT32 or "" otherwise
13,10:%13s%s jednostek alokacji na dysku.\n
13,11:%13s%s jednostek alokacji oznaczonych jako uszkodzone\n
# FAT32, 2 spaces, see 13.10 & 13.11
13,12:
# FAT12/FAT16, no spaces, see 13.10 & 13.11
13,13:
13,14:GB

# misc messages
14,0:\n Nieprawidłowe połączenie opcji... przeczytaj pomoc. Przerywanie.\n
14,1:-- naciśnij ENTER, aby zobaczyć następną stronę, albo ESC, aby przerwać --
14,2:\nPrzerwano na żądanie użytkownika.\n
14,3:Niedozwolona wartość parametru - %s%s\n
14,4:Ukończono %3d%%.
14,5:Brak wymaganej wartości parametru - %s:wartość\n

# Create_File_System()
15,0:Flagi FAT32 0x%04x zmienione na 0.\n
15,1:Wersja FAT32 0x%04x wymuszona na 0.\n
15,2:Zapasowy sektor rozruchowy / FS Info\x20 
15,3:wymuszony.\n
15,4:- domyślne położenie nieprawidłowe.\n
15,5: Używane będą sektory 6 i 1.\n
15,6:wyłączony (brak miejsca).\n

# Get_FS_Info()
16,0:Rozmiar katalogu głównego zaokrąglony do %u pozycji (wielokrotność %u).\n
16,1:Sektor nie ma 512 bajtów. Przerywanie!\n
16,2:Nieprawidłowa liczba tablic FAT. Przerywanie!\n

# Write_Boot_Sectors()
17,0:Brak zapasowego sektora rozruchowego.\n
17,1:Nieprawidłowe położenie zapasowego sektora rozruchowego: %u??\n
17,2:Brak sektora FS Info.\n
17,3:Nieprawidłowe położenie sektora FS Info: %u??\n

# Write_FAT_Tables()
18,0:Nieprawidłowa liczba tablic FAT. Przerywanie!\n
18,1:OSTRZEŻENIE: każda FAT ma %lu sektorów, > 16MB-64k, niezgodne z Win9x!\n
18,2: Przygotowywanie obszaru FAT...\n
18,3: Optymalny początkowy rozmiar katalogu głównego: %lu klastrów.\n
18,4:Katalog główny obcięty do 125 klastrów!\n

# drivio.c
19,0: [Błąd %d]\n
19,1: Nieprawidłowy dysk! Przerywanie.\n
19,2: Zignorowano błąd blokady logicznej FreeDOS 0x%x.\n
19,3: Nie można zablokować dysku logicznego (błąd 0x%x)! Przerywanie.\n
19,4: Zignorowano błąd blokady fizycznej FreeDOS 0x%x.\n
19,5: Nie można zablokować fizycznej stacji dyskietek (błąd 0x%x)!?\n
# maybe a better error message? Drive_IO() Invalid request for 0 sectors?
19,6:Drive_IO(x,y,0)?\n
# last %s for 19.7 & 19.8 replaced with READ or WRITE from 19.9 or 19.10
19,7:* uszkodzone sektory: %ld (kod 0x%x) przy %s *\n
19,8:Drive_IO(%s %ld, liczba %d ) [%s] [dysk %c%c]\n
19,9:ZAPISIE
19,10:ODCZYCIE
19,11:\nNie można odczytać flag dostępu (błąd %02x). Przerywanie.\n
19,12:\nNie można włączyć flag dostępu (błąd %02x). Przerywanie.\n

# see above for set 20

# hdisk.c
21,0:Domyślny BPB *FAT1x*: %u sektorów/FAT, katalog główny %u. Wymuszony FAT32.\n
21,1:Nieprawidłowy dysk! Przerywanie.\n
21,2:Brak dostępu! Problem z blokadą (LOCK)? Przerywanie.\n
21,3:Błąd GENIOCTL/0860: %02x. Próba FAT32.\n
21,4:Błąd GENIOCTL/4860: %02x. Jądro bez FAT32?\n
21,5:Rozmiar FAT (FAT1x): %u sektorów?\n
21,6:Okno DOS w WinNT/XP/2k. Formatowanie niemożliwe.\n
21,7:Poprawiono położenie katalogu głównego FAT32 w domyślnym BPB na 2.\n
# GDP = Get_Device_Parameters()
21,8:Autokorekta GDP: musi być FAT12! Rozmiar FAT1x: %u\n
21,9:Autokorekta GDP: musi być FAT16! Rozmiar FAT1x: %u\n
21,10:Autokorekta GDP: to rzeczywiście FAT32! Rozmiar FAT: %u\n
21,11:Autokorekta GDP: usuwam FAT (%u) / katalog główny (%u) FAT1x z dysku FAT32.\n
21,12:Autokorekta GDP: to nie może być FAT1x.\n
21,13:Autokorekta GDP: to nie może być FAT1x, brak tablicy FAT1x\n
21,14:Autokorekta GDP: to nie może być FAT1x, brak katalogu głównego FAT1x.\n
21,15:Błąd odczytu domyślnego BPB przez GDP: %02x.\n
21,16: Wyrównanie dla nieparzystej liczby tablic FAT32.\n
21,17:Wolumin ma zerowy rozmiar!? Przerywanie.\n
21,18:%d bajtów na sektor, a nie 512!? Przerywanie.\n
21,19:Nie 1 ani 2 tablice FAT, tylko %hu!? Przerywanie.\n
21,20:OSTRZEŻENIE: klastry większe niż 64k. To bardzo niezgodne!\n
# 21.21 & 21.22 go together
21,21:OSTRZEŻENIE: klastry większe niż 32k. Nie zadziała w Win9x ani MS-DOS!\n
21,22:  WinME, WinNT/2k/XP/2003 i FreeDOS poradzą sobie z tym.\n
21,23:BŁĄD: rozmiar klastra to nie 0,5/1/2/4/8/16/32/64k ani 128k, tylko %d.%dk!\n
21,24: Prawie sformatowano dysk FAT32 jako FAT1x, uff...\n
# 21.25 & 21.26 go together
21.25:OSTRZEŻENIE: rozmiar FAT32 przekroczy (16 MB - 64 kB)!\n
21,26:  Win9x nie użyje tego dysku. Inne systemy zużyją więcej RAM/CPU.\n
21,27: Prawie sformatowano dysk FAT1x jako FAT32, uff...\n
# 21.28 or 21.29 followed by "FAT12.\n" or "FAT16.\n"
# with %lu %s size, e.g. 999 kbytes  -- see 13.1 and similar
21,28: Rozmiar dysku: %lu %s,\x20 
21,29: Uwaga: niestandardowy rozmiar sektora,\x20 

# floppy.c
22,0:Format_Floppy_Cylinder( head=%d cylinder=%d ) sectors=%d [int 13.5]\n
22,1:Sprawdzam tylko pierwsze %d sektorów na ścieżce\n
22,2:\nFormatowanie nie powiodło się, błąd w pierwszych 7 sektorach!\n
22,3:Sektor %4ld CHS=[%2d:%d:%2d] uszkodzony\n
22,4:Numer dysku twardego! Przerywanie.\n
22,5:Formatowanie na %ldk (Cyl=%ld Głow=%ld Sek=%2ld)\n
22,6:Nieznany typ nośnika dla formatu %ldk\n
22,7:Nieznany typ nośnika dla formatu %ldk (Cyl=%ld Głow=%ld Sek=%2ld)\n
22,8:Stacja typu 0x%x (int 13.8) traktowana jako 1440k.\n
22,9:Domyślny format stacji: %ldk (Cyl=%ld Głow=%ld Sek=%2ld)\n
22,10:Rozmiar %ldk nieokreślony!??\n
22,11:%ldk w stacji %dk? Za dużo. Przerywanie.\n
22,12:NADFORMAT: %ldk w stacji %dk. Powodzenia!\n
22,13:To stacja 720k: /1, /4 ani /8 nie są możliwe.\n
22,14:Minimalny rozmiar dla tego typu stacji to 720k\n
22,15:To stacja 3,5 cala: /1, /4 ani /8 nie są możliwe.\n
22,16:Minimalny rozmiar dla tego typu stacji to 720k.\n
22,17:To stacja 3,5 cala: format 1200k niemożliwy.\n
22,18:Nie udało się ustawić trybu stacji (klasa rozmiaru %d), błąd %02x hex\n
22,19:Brak dyskietki w stacji!\n
22,20:Kontynuuję mimo to.\n
22,21:Brak dyskietki w stacji (przekroczony czas)!\n
22,22:Nośnik %ldk  (%d x %ld x %2d)  nieobsługiwany przez tę stację!?\n
22,23:Błąd ustawiania geometrii (int 13.18): %02x.\x20 
22,24:Rezygnuję.\n
22,25:Zignorowano.\n
22,26:DDPT jest w ROM - możliwe tylko standardowe rozmiary.\n
22,27:SEKTORY NA ŚCIEŻKĘ zostają na %d, oczekiwano %d. Kontynuuję mimo to.\n
22,28:Nie można zmienić DDPT: DDPT w ROM\n
22,29:Nieznana długość przerwy dla %d sekt./cyl. Powodzenia z wartością BIOS!\n
22,30:TWEAK: sektory na cylinder w DDPT ustawione na %d\n
22,31:TWEAK: %d sektorów na cylinder, długość przerwy formatu %d!\n
22,32:Reset kontrolera stacji dyskietek nie powiódł się (kod %x) - DDPT odrzucona?\n
22,33:-- naciśnij ENTER, aby sformatować dysk (ESC, aby przerwać) --\n
22,34:[podwójny krok]\x20 
22,35:[skonfigurowany typ %hu]\x20 
22,36: 500 kbps (HD 1xx0k)\n
22,37: 300 kbps (DD  360k)\n
22,38: 250 kbps (DD  720k)\n
22,39:1000 kbps (ED 2880k)\n

# savefs.c
23,0:Nie można zapisać sektora MIRROR MAP %lu - UNFORMAT nie zadziała!\n
23,1:Sektor nie ma 512 bajtów. Nie można zapisać danych UNFORMAT.\n
23,2:Nie 1 ani 2 kopie FAT. Nie można zapisać danych UNFORMAT.\n
23,3:OSTRZEŻENIE: liczba sektorów zarezerwowanych / rozruchowych to %u, nie 1.\n
23,4:OSTRZEŻENIE: sektorów zarezerwowanych jest %u, po formatowaniu będzie %u.\n
23,5: Musi być FAT32, nie %s!\n
23,6: Dane UNFORMAT NIE zostaną zapisane, lista uszkodzonych klastrów przepadnie.\n
23,7:OSTRZEŻENIE: FAT32 z dodatkowym katalogiem głównym w stylu FAT1x???\n
23,8: Musi być FAT32, nie FAT1x!\n
23,9: Katalog główny FAT32, ale tablica FAT1x!\n
23,10: Rozmiar %s, a miało być FAT32!\n
23,11: Nieprawdopodobny katalog główny, FAT lub rozmiar dysku! Zły sektor rozruchowy?\n
23,12: Duża FAT przy małej ilości danych? Zły sektor rozruchowy?\n
23,13:Katalog główny NIE jest w 1. klastrze, NIE zostanie zapisany!\n
23,14: Dysk wygląda na niesformatowany, dane UNFORMAT NIE zostały zapisane.\n
23,15:Inna liczba sektorów zarezerwowanych: ZNALEZIONO %lu / ZAPLANOWANO %u.\n
23,16:Inna liczba tablic FAT: JEST %lu / PLAN %hu\n
23,17:Inny rozmiar klastra: JEST %lu / PLAN %hu (sektorów)\n
#  FOUND # FAT?? / PLANNED # FAT??
23,18:Inny rozmiar FAT: ZNALEZIONO %lu %s / ZAPLANOWANO %lu %s\n
23,19:Dane MIRROR nadpisałyby używane klastry. Przerywanie.
23,20:Bezpieczne formatowanie: trzeba zniszczyć %lu używanych sektorów danych!\n
23,21:Błąd zapisu wskaźnika mapy mirror - UNFORMAT nie zadziała!\n
23,22:Pomijam zapis kopii danych UNFORMAT / mirror.\n
23,23:Błąd odczytu w sektorze %lu - dane UNFORMAT uszkodzone\n
23,24:Błąd zapisu w sektorze %lu - dane UNFORMAT uszkodzone\n
23,25: Mapa mirror ma %lu bajtów,\x20 
23,26: %lu sektorów skopiowanych.\n
23,27:Brak danych MIRROR / UNDELETE: nieprawidłowa sygnatura.\n
23,28:\n Koniec mapy mirror. UNFORMAT zakończony.\n
23,29: Kopiowanie sektora rozruchowego do kopii zapasowej.\n
23,30: Unieważnianie danych sektora FS Info.\n
23,31:*** Nie można skopiować sektora zapasowego %lu do sektora %lu ***\n
23,32:Sektor rozruchowy zostałby nadpisany bzdurami: 0 albo > 7 tablic FAT.\n
23,33:*** Nie można przywrócić sektora %lu ***\n
23,34:*** Nie można skopiować sektora FAT %lu do sektora %lu ***\n
23,35:OSTRZEŻENIE: zażądano %hu kopii FAT, używam %hu.\n
23,36:OSTRZEŻENIE: UNFORMAT zmieni dysk FAT1x na FAT32.\n
23,37:OSTRZEŻENIE: UNFORMAT zmieni dysk FAT32 na FAT1x!?\n
23,38:\n Dane sektora rozruchowego: %hu kopii FAT (przes. %lu), %lu sektorów na FAT\n
23,39: Zapisywanie danych UNFORMAT...\n

# savefs.c unable to save filesystem properties error message
# Use as many messages as needed, 1 message per max 79 column line
# message numbers do not need to match up for messages for different languages
# Each message should end with a newline \n
# after every 23 messages user will be prompted to press a key to continue
24,0:Zmienią się właściwości systemu plików - nie można zachować\n
24,1:starej (być może pustej) listy uszkodzonych klastrów. Aby ją uaktualnić,\n
24,2:użyj programu do sprawdzania powierzchni albo FORMAT /U.\n

# recordbc.c
25,0:Dysk nie nadaje się do użytku: uszkodzony sektor systemowy. Przerwano.\n
25,1:\nUszkodzony sektor rozruchowy.\n
25,2:\nUszkodzony sektor FAT.\n
25,3:\nUszkodzony sektor katalogu głównego.\n
25,4:\nUszkodzony sektor rozruchowy.\n
25,5:\nUszkodzony sektor FAT.\n
25,6:\nUszkodzony sektor katalogu głównego.\n
25,7:\nUszkodzony sektor zarezerwowany (rozruchowy, info lub ich kopia...).\n
25,8:\nUszkodzony sektor FAT.\n
25,9:*** Uszkodzony sektor %lu, klaster %lu oznaczony w FAT jako uszkodzony! ***\n
25,10:OSTRZEŻENIE: trzeba oznaczyć UŻYWANY klaster %lu jako uszkodzony!\n
25,11:*** Znaleziono %u uszkodzonych sektorów. Oznaczono %lu uszkodz. klastrów. ***\n

# bcread.c (bad clusters)
26,0:\n *** Za dużo uszkodzonych klastrów! Po formatowaniu sprawdź powierzchnię! ***\n
26,1:BadClustPreserve przerwane: sektor nie ma 512 bajtów!\n
26,2: Przeszukiwanie sektorów FAT od %lu do %lu...\n
26,3: Przeszukiwanie istniejącej FAT...\n
26,4:\n Statystyka klastrów: %lu używanych, %lu uszkodzonych, %lu pozycji, ost. %lu.\n
26,5:\n Statystyka klastrów: %lu używanych, %lu uszkodzonych, %lu pozycji, ost. %lu.\n
26,6: Nie można przetworzyć istniejącej FAT12 - za duża!\n
26,7:\n Statystyka klastrów: %lu używanych, %lu uszkodzonych, %lu pozycji, ost. %lu.\n
