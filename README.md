W mailu otrzymaliście program potrafiący wyświetlać obrazki w nieznanym formacie. Waszym celem będzie zrozumienie tego formatu na podstawie załączonego czytnika oraz trzech przykładowych obrazków, a następnie stworzenie programu potrafiącego konwertować do niego dowolne obrazki.

Informacje o programie

Jest to Linuxowa binarka na architekturę x64, która wyświetla w oknie obraz .img podany w pierwszym argumencie.

Do uruchomienia wymagana jest biblioteka Allegro5 (https://liballeg.org/download.html). Na Ubuntu można ją zainstalować przez:

sudo add-apt-repository ppa:allegro/5.2
sudo apt-get install liballegro5.2 liballegro5-dev
Wskazówki

Dokumentacja Allegro5 znajdziecie pod https://liballeg.org/a5docs/trunk/
Nie musicie czytać całego kodu linia po linii. Zalecanym przez nas podejściem jest połączenie eksperymentowania z plikiem wejściowym, czytania kodu w interesujących miejscach i podgląd stanu procesu w debuggerze.
Uwaga: Nie wszystkie możliwości formatu musiały zostać pokryte przez załączone przykłady!

Cel

Dokładny opis rozwiązania oraz procesu analizy. Powinno z niego krok po kroku być widać w jaki sposób analizowaliście format oraz program, które funkcje co w nim robiły, itd.
Konwerter, który potrafi przekonwertować dowolny obraz w formacie PNG na format używany przez program z zadania. Powinien być używalny w następujący sposób: `./img_convert <input>.png <output>.img`. Oczywiście może składać się z większej liczby plików, chodzi nam tylko o to, żebyśmy mogli łatwo testować Wasze rozwiązania, do czego potrzebujemy wspólnego interfejsu. Jeśli używacie Pythona i niestandardowych modułów, to prosimy o dodanie odpowiedniego requirements.txt.

Oceniane będą m.in.:
Poprawne i dokładne zrozumienie oraz opisanie formatu pliku obsługiwanego przez program z zadania.
Działający konwerter (patrz wyżej).
Do maksymalnej ilości punktów konwerter powinien być możliwie optymalny (dla mnt_mini.png wynikowy plik powinien mieć około 1.1MB, a czas działania mieścić się w minucie).
Samodzielność rozwiązania.
Rozwiązania częściowe też są akceptowane, za proporcjonalną ilość punktów.
