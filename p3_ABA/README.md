# Wykres Wydajności LL/SC STACK

Ten skrypt Pythona czyta dane z pliku `wyniki/wyniki_telefon.txt` i generuje wykres wydajności LL/SC STACK w zależności od liczby wątków.

## Wymagania

- Python 3.x
- matplotlib (zainstaluj za pomocą `pip install -r requirements.txt`)

## Uruchomienie

Uruchom skrypt:

```bash
python plot_results.py
```

Wykres zostanie zapisany jako `performance_plot.png`.

## Opis danych

Dane pochodzą z testów wydajności LL/SC na ARM z różną liczbą wątków.