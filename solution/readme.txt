Jakub Molinski 419502


PROCES ANALIZY:
Wykonywałem niemal wyłącznie analizę statyczną.
Na początek odpaliłem program pod gdb i próbowałem śledzić co się dzieje (ustawiając breakpoint na funkcjach z stdlib/allegro) ale było to na tyle skomplikowane, że zrezygnowałem.
Potem program odpalałem tylko po modyfikowaniu go w IDA (żeby sprawdzić czy nie popsuło się działanie dla przykładowych obrazków) i testując wyświetlanie obrazków
wyplutych przez mój konwerter.

Otworzyłem program w IDA, zaznaczając, żeby spróbował zresolvowac symbole linkowane dynamicznie - dzięki temu w kodzie widziałem gdzie wołane są funkcje z
stdlib i z allegro, co było ważnym punktem startowym do analizy.

Spróbowałem zdekompilować program - kilka funkcji się zdekompilowało, ale przy wszystkich dłuższych IDA krzyczała, że stack frame too big.
Następnie załadowałem program do ghidry i tam zdekompilowałem - udało się. W konfigu IDA spróbowałem zwiększyć maksymalny rozmiar funkcji w ustawieniach dekompilatora,
ale to w niczym nie pomogło.
Kontynuowałem dalej w ghidrze, próbowałem odszyfrować maina - co było dosyć proste. Dzięki temu, że widziałem nazwy funkcji z allegro, to łatwo było poprawić nazwy zmiennych,
aby pasowały do argumentów przyjmowanych przez te funkcje - do tego odrobina zmiany typów na bardziej typowe dla C. Przeszedłem do następnych funkcji, w których jakość
zdekompilowanego kodu była już dużo niższa. Ciężko się to analizowało, więc znów spróbowałem zdekompilować kod do c w IDA. Po dłuższym czasie szukania źródła problemu,
zauważyłem, że te funkcje mają w sobie takie linijki:
ADD RSP,0x192b00
Czyli coś podobnego do aloca(dużo) w c,
co powinno skótkować potem problemem z przepełnieniem stosu - nic takiego jednak nie obserwowałem w trakcie wykonania programu.
Przyglądając się dokładniej zauważyłem, że te operacje znajdują się w gałęziach, które nie powinny się wykonać w praktyce.
Zastąpiłem w kilku funkcjach te operacje NOPami, i wtedy udało się zdekompilować do c cały program w IDA.

Powtórzyłem dla maina to co zrobiłem w ghidrze, posprzątałem też zmienne.
Zacząłem czytać maina i na początku wczytywane były 2 wartości - liczba kolumn i liczba wierszy.
Przeanalizowałem przyjmowane przez funkcję argumenty i jej zdekompilowany kod. Po wyczyszczeniu kodu
(usunięcie niepotrzebnych kopii zmiennych, zmienienie niektórych nazw zmiennych itp.) to co robi funkcja stało się oczywiste:
przyjmuje jako argumenty:
- bufor 25-bajtowy, składający się z: 8 bajtów FILE*, 16 bajtów bufora, 1 bajtu oznaczającego błąd wczytywania
- liczbę bitów do wczytania z pliku
i zwraca liczbę równą wartości wczytanej z pliku (wczytuje tyle bitów ile wynosi parametr)

zmieniłem nazwę tej funkcji i kontynuowałem analizę.
W mainie została jeszcze jedna nieznana funkcja (sub_1B80) - po jej parametrach oczywiste było, że wczytuje ona dane z pliku i dekoduje je jako bitmapę, którą umie wyświetlić allegro.
Była jeszcze jedna funkcja, która bierze argumenty x, y, a, b, c i zmienia wartość piksela (x, y) z (r, g, b) na (r + a, g + b, b + c) (mod 256).
Skopiowałem zdekompilowany pseudokod z IDA do cliona i tam poczyściłem ten kod, przekształcajac go równoważnie. Nie było w tym już nic specjalnie interesującego, bez problemu udało się
odszyfrować wszystkie szczegóły formatu.

Załączam moją pseudo-reimplementację viewera w C, która powstała w tym procesie (miejscami mogą nie zgadzać się typy, nazwy, wywołania z allegro - nie kompilowałem tego kodu).

OPIS FORMATU:
format pliku:
jest to plik binarny, składający się z liczb różnej długości (w bitach) specyfikowanej przez format, bez paddingu (poza dopełnieniem ostatniego bajtu dowolnymi wartościami, do 8 bitow).

Pseudo-gramatyka tego formatu:

V(N) := ciąg N bitów, interpretowany jako liczba całkowita bez znaku

header-value := V(24)
columns-number := header-value
rows-number := header-value
header := columns-number rows-number

cmd-code := V(3)
cmd-2-body := empty
cmd-7-body := V(1) V(1) V(1)
cmd-1-body := V(3) V(3) V(3)
cmd-6-body := V(8) V(8) V(8)
cmd-0-body := V(4)
cmd-4-body := V(10)
cmd-5-body := V(3) V(3)
cmd-3-body := V(2) V(2) V(2) V(8)

statement := cmd-code (cmd-0-body | cmd-1-body | cmd-2-body | cmd-3-body | cmd-4-body | cmd-5-body | cmd-6-body | cmd-7-body)
padding := bity potrzebne do dopełnienia ostatniego bajtu
file := header statement* padding

Czyli:
plik składa się z nagłówka (liczba kolumn, liczba wierszy - liczby 24-bitowe)
po czym następuje ciąg komend postaci cmd-code i ciało komendy

bitmapa zaczyna jako cała biała, a potem komendy zmieniają w niej wartości
utrzymywany jest stan - 3 liczby uint8_t r, g, b (początkowo ich wartość to 0), oraz liczby row, column (początkowa wartość 0, 0) oznaczające aktualną pozycję kursora

opisy poszczególnych komend:
(Czytelna implementacja tych wszystkich komend znajduje sie w pliku viewer-reimplementation.c)

0, 4) argument: liczba o_ile_przesunąć
przesuwa kursor o o_ile_przesunąć pozycji w lewo, zawijając do następnego wiersza jeżeli kursor dojdzie do prawej granicy bitmapy
1, 6, 7) argumenty: liczby diff_r, diff_g, diff_b
zmienia wartość stanu wartości r, g, b, tak, że
r, g, b = (r + diff_r), (g + diff_g), (b + diff_b) (mod 256)
2) argumenty: brak
aktualizuje wartość piksela pod kursorem w taki sposób, że jeżeli piksel = (x, y, z)
to nowa wartość pod kursorem to
(x + r, y + g, z + b) (mod 256)
po czym przesuwa kursor na następną pozycję
3) argumenty: diff_r, diff_g, diff_b, ile_pikseli
wylicza
delta_r, delta_g, delta_b = diff_r - 2, diff_g - 2, diff_b - 2

a następnie realizuje operację (łatwiej opisać pseudokodem)
poczatkowe_rgb = (r, g, b)
zapisz_oryginalny_kursor()
for _ in range(ile_pikseli):
    wykonaj-cmd-2
    r, g, b = r + delta_r, g + delta_g, b + delta_b (underflow i overflow jak na uint8_t)
przywroc_oryginalny_kursor()
r, g, b = poczatkowe_rgb
5) argumenty: column_power_diff row_power_diff
wyliczane jest:
(row i column to wartości kursora)
other_row = row + 2^(row_power_diff + 1)
other_column = column + 2^(column_power_diff + 1)
a następnie wszystkie piksele w prostokącie którego lewy górny punkt to
(column, row)
a prawy dolny punkt to
(other_column, other_row)
ustawiane są na wartość (r, g, b)

KONWERTER:
plik img_convert
nie korzysta z komendy 5 - wypisywania pomalowanych prostokątów
plik mnt_mini.png po skonwertowaniu ma około 1.16 MB
załączam także plik requirements.txt (do parsowania png jest używana biblioteka pypng)
