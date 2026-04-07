using System;
using System.Runtime.Serialization;
using System.Threading;



public class UninitializedMany
{
    private class Foo
    {
        public int[] Data;
    }

    private static Foo FooObj;
    public static void Writer()
    {
        if (FooObj == null)
        {
            lock (typeof(UninitializedMany))
            {
                if (FooObj == null)
                {
                    FooObj = (Foo)FormatterServices.GetUninitializedObject(typeof(Foo));
                    // Symulacja czasu pomiędzy alokacją a inicjalizacją
                    Thread.SpinWait(50);
                    FooObj.Data = new int[5];
                }
            }
        }
    }

    public static void _Main()
    {
        int trials = 1000;
        int errorCount = 0;
        int successCount = 0;

        for (int i = 0; i < trials; i++)
        {
            // Resetowanie stanu przed każdą próbą
            FooObj = null;
            bool errorOccurred = false;

            Thread writer = new Thread(Writer);

            Thread reader = new Thread(() =>
            {
                // Próba wstrzelenia się w okno podatności
                Thread.SpinWait(50);

                if (FooObj != null)
                {
                    try
                    {
                        // Próba odczytu stanu wewnętrznego
                        int length = FooObj.Data.Length;
                    }
                    catch (NullReferenceException)
                    {
                        // Jeśli obiekt istnieje, ale tablica to null – mamy błąd DCL
                        errorOccurred = true;
                    }
                }
            });

            //Console.WriteLine($"Próba {i + 1}");
            writer.Start();
            reader.Start();
            writer.Join();
            reader.Join();

            if (errorOccurred)
            {
                errorCount++;
            }
            else
            {
                successCount++;
            }
        }

        Console.WriteLine($"Liczba prób: {trials}");
        Console.WriteLine($"Błędy DCL (odczyt surowego obiektu): {errorCount}");
        Console.WriteLine($"Sukcesy (poprawny odczyt lub ominięcie): {successCount}");
    }
}