# Instrukcja uruchomienia

Aplikacja CLI implementująca własny protokół kolejkowy z komunikacją zabezpieczoną TLS pomiędzy serwerem a kilkoma typami klientów (kiosk, app, admin).

---

## Wymagania wstępne

Upewnij się, że następujące narzędzia są zainstalowane w systemie:

- `git`
- `cmake` (≥ 3.28.3)
- `gcc` lub `clang` (wymagana obsługa standardu C11)
- `openssl` (biblioteki + narzędzie CLI)
- `make`

Na Debian/Ubuntu:

```bash
sudo apt install git cmake gcc libssl-dev make
```

---

## Klonowanie repozytorium

```bash
git clone https://github.com/jakubjachxwicz/pus_queue_protocol.git
cd pus_queue_protocol
```

---

## Konfiguracja certyfikatów TLS

Serwer wymaga certyfikatów TLS przed uruchomieniem. Należy wygenerować Urząd Certyfikacji (CA) oraz certyfikat serwera. Wszystkie poniższe polecenia należy wykonać z katalogu `server/`.

```bash
cd server
```

### 1. Utwórz Urząd Certyfikacji (CA)

```bash
bash create_ca.sh
```

Skrypt generuje:
- `ca.key` - klucz prywatny CA
- `ca.crt` - certyfikat CA

### 2. Utwórz certyfikat serwera

```bash
bash create_cert.sh
```

Skrypt generuje:
- `server.key` - klucz prywatny serwera
- `server.csr` - żądanie podpisania certyfikatu
- `server.crt` - certyfikat serwera podpisany przez CA

> **Uwaga:** Skrypty muszą być uruchamiane w podanej kolejności. `create_cert.sh` wymaga plików `ca.key` i `ca.crt` wygenerowanych przez `create_ca.sh`.

---

## Budowanie projektu

Projekt składa się z dwóch niezależnych celów CMake: serwera i klientów (kiosk, app, admin)
### Budowanie serwera

```bash
cd server
mkdir -p build && cd build
cmake ..
make
```

Skompilowany plik wykonywalny znajdzie się pod ścieżką `server/build/pus_queue_protocol`.

### Budowanie klientów

```bash
cd clients
mkdir -p build && cd build
cmake ..
make
```

W katalogu `clients/build/` powstaną trzy pliki wykonywalne: `kiosk_client`, `app_client`, `admin_client`.

---

## Kopiowanie certyfikatów do katalogów build

Serwer i klienty wczytują pliki certyfikatów po nazwie w czasie działania programu, dlatego wymagane pliki muszą znajdować się w katalogu roboczym każdego pliku wykonywalnego (czyli w odpowiednim katalogu `build/`).

Wykonaj poniższe polecenia z głównego katalogu repozytorium:

**Serwer** - wymaga podpisanego certyfikatu i klucza prywatnego:

```bash
cp server/server.crt server/server.key server/build/
```

**Klienty** - wymagają certyfikatu CA do weryfikacji tożsamości serwera:

```bash
cp server/ca.crt clients/build/
```

> **Uwaga:** `ca.key` służy wyłącznie do podpisywania certyfikatów i nie powinien być kopiowany do katalogów build ani rozpowszechniany.

---

## Uruchamianie

### Uruchomienie serwera

Z katalogu `server/build/`:

```bash
./pus_queue_protocol
```

Serwer musi być uruchomiony przed połączeniem jakiegokolwiek klienta.

### Uruchomienie klienta

Z katalogu `clients/build/` uruchom wybrany klient:

```bash
./kiosk_client
```

```bash
./app_client
```

```bash
./admin_client
```

---

## Struktura projektu

```
pus_queue_protocol/
├── server/               # Kod źródłowy serwera i skrypty TLS
│   ├── main.c
│   ├── CMakeLists.txt
│   ├── create_ca.sh      # Generuje klucz i certyfikat CA
│   ├── create_cert.sh    # Generuje klucz i certyfikat serwera
│   ├── message_handlers/ # Handlery wiadomości protokołu
│   ├── queue/            # Implementacja kolejki
│   ├── registry.c
│   ├── notify.c / .h
│   └── types.h
└── clients/              # Kod źródłowy klientów
    ├── CMakeLists.txt
    ├── admin_main.c
    ├── app_main.c
    ├── kiosk_main.c
    ├── common/           # Wspólne narzędzia klientów
    └── handlers/         # Handlery protokołu po stronie klienta
```