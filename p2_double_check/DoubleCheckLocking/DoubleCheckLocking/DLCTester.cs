using System;
using System.Runtime.Serialization;
using System.Threading;

public class DCLTester
{
    private class Foo
    {
        public int[] Data;

        public Foo()
        {
            Thread.SpinWait(50);
            Data = new int[6] { 1, 2, 3, 4, 5, 67 };
        }
    }

    private static Foo FooObj;
    private static volatile bool errorOccurred;
    private static Barrier syncBarrier;
    private static readonly object _lock = new object();
    private static void ResetState()
    {
        FooObj = null;
        errorOccurred = false;
    }

    // 1. Klasyczny wzorzec DCL
    public static void DCL()
    {
        syncBarrier.SignalAndWait();
        if (FooObj == null)
        {
            lock (_lock)
            {
                if (FooObj == null)
                {
                    FooObj = new Foo();
                    return;
                }
            }
        }
        ValidateRead();
    }

    // 2. Bezpieczna inicjalizacja z pełną blokadą (Single-Checked Locking)
    public static void SCL()
    {
        syncBarrier.SignalAndWait();
        lock (_lock)
        {
            if (FooObj == null)
            {
                FooObj = new Foo();
                return;
            }
        }
        ValidateRead();
    }

    // 3. Wariant DCL z wymuszoną rearanżacją instrukcji
    public static void DCLSimulatedReordering()
    {
        syncBarrier.SignalAndWait();
        if (FooObj == null)
        {
            lock (_lock)
            {
                if (FooObj == null)
                {
                    // Publikacja wskaźnika przed inicjalizacją danych (symulacja out-of-order execution)
                    FooObj = (Foo)FormatterServices.GetUninitializedObject(typeof(Foo));
                    Thread.SpinWait(50);
                    FooObj.Data = new int[6] { 1, 2, 3, 4, 5, 67 };
                    return;
                }
            }
        }
        ValidateRead();
    }

    private static void ValidateRead()
    {
        Foo temp = FooObj;
        if (temp != null)
        {
            try
            {
                int length = temp.Data.Length;
            }
            catch (NullReferenceException)
            {
                errorOccurred = true;
            }
        }
    }

    public static void Main()
    {
        int[] numOfThreads = new int[] { 2, 4, 8, 12, 16, 24, 32 };
        int trials = 1000;

        ThreadStart[] testMethods = new ThreadStart[] { SCL, DCL, DCLSimulatedReordering };
        string[] testNames = new string[] {"SCL", "DCL", "DCL z rearanżacją instrukcji" };

        Console.WriteLine($"Liczba prób: {trials}");

        for (int m = 0; m < testMethods.Length; m++)
        //for (int m = 2; m < testMethods.Length; m++)
        {
            Console.WriteLine($"\nAlgorytm: {testNames[m]}");
            Console.WriteLine("Licz. wątków; Błędy DCL");

            foreach (int threadsCount in numOfThreads)
            {
                int errorCount = 0;

                for (int i = 0; i < trials; i++)
                {
                    ResetState();

                    syncBarrier = new Barrier(threadsCount);
                    Thread[] threads = new Thread[threadsCount];

                    for (int j = 0; j < threadsCount; j++)
                    {
                        threads[j] = new Thread(testMethods[m]);
                        threads[j].Start();
                    }

                    for (int j = 0; j < threadsCount; j++)
                    {
                        threads[j].Join();
                    }

                    if (errorOccurred)
                    {
                        errorCount++;
                    }

                    syncBarrier.Dispose();
                }

                Console.WriteLine($"{threadsCount};{errorCount}");
            }
        }
    }
}
