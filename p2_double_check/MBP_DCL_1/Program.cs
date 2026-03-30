using System;
using System.Threading.Tasks;

namespace DclBadExample
{
    // Obiekt, który chcemy "leniwie" zainicjować
    public class KosztownyObiekt
    {
        public List<int> MojaLista;
        public KosztownyObiekt()
        {
            MojaLista = new List<int>();

            // 2. Wypełnianie listy (kolejne dziesiątki instrukcji dla procesora)
            for (int i = 0; i < 1000; i++)
            {
                MojaLista.Add(i);
            }
        }
    }

    public class BlednyDclSingleton
    {
        // KLUCZ PROBLEMU: Zmienna bez słowa kluczowego 'volatile'
        private static KosztownyObiekt _instance;
        private static readonly object _lockHandle = new object();


        public static KosztownyObiekt GetInstance()
        {
            if (_instance == null)
            {
                lock (_lockHandle)
                {
                    if (_instance == null)
                    {
                        // BOMBA ZEGAROWA (Rearanżacja instrukcji):
                        // Kompilator/Procesor może najpierw zadeklarować w pamięci, że _instance nie jest null,
                        // a dopiero w następnym ułamku sekundy wejść do konstruktora i przypisać Dane = 42.
                        _instance = new KosztownyObiekt();
                    }
                }
            }
            return _instance; // Inny wątek może tu zwrócić obiekt, w którym Dane wciąż wynosi 0
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Testowanie błędnego DCL (Slajd 28)...");

            // Odpalamy 100 wątków jednocześnie, by spróbować wywołać błąd
            Parallel.For(0, 100, i =>
            {
                var instancja = BlednyDclSingleton.GetInstance();

                // Jeśli instrukcje zostały zreorganizowane, program może tu wypisać 0 zamiast 42,
                // albo w skrajnych przypadkach rzucić NullReferenceException w trakcie dostępu do .Dane
                if (instancja.MojaLista[999] != 999)
                {
                    Console.WriteLine($"[BŁĄD WĄTKU] Otrzymano niepełny obiekt! Dane = {instancja.MojaLista}");
                }
            });

            Console.WriteLine("Test zakończony.");
            Console.ReadLine();
        }
    }
}