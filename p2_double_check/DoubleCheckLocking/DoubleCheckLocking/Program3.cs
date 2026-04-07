using System;
using System.Threading;
using System.Threading.Tasks;

public class DclSimulation
{
    private class Foo
    {
        public int[] Data;

        public Foo()
        {
            Thread.SpinWait(50); 
            Data = new int[6] { 1,2,3,4,5,67 };
        }
    }

    private class Trial
    {
        private Foo _instance;
        private readonly object _lock = new object();

        public Foo GetState()
        {
            if (_instance == null)
            {
                lock (_lock)
                {
                    if (_instance == null)
                    {
                        _instance = new Foo();
                    }
                }
            }
            return _instance;
        }
    }

    public static void Run(int iterations)
    {
        int safeCount = 0;
        int corruptedCount = 0;

        for (int i = 0; i < iterations; i++)
        {
            var trial = new Trial();
            var startSignal = new ManualResetEventSlim(false);

            int localSafe = 0;
            int localCorrupt = 0;

            Action worker = () =>
            {
                startSignal.Wait(); // Synchronizacja precyzyjnego startu
                var state = trial.GetState();

                // Analiza spójności: referencja istnieje, ale konstruktor nie zakończył pracy
                if (state.Data[0] != 1)
                    Interlocked.Increment(ref localCorrupt);
                else
                    Interlocked.Increment(ref localSafe);
            };

            var t1 = Task.Run(worker);
            var t2 = Task.Run(worker);

            startSignal.Set(); // Równoczesne zwolnienie wątków
            Task.WaitAll(t1, t2);

            safeCount += localSafe;
            corruptedCount += localCorrupt;
        }

        Console.WriteLine($"--- Wyniki symulacji ({iterations} prób) ---");
        Console.WriteLine($"Bezpieczne odczyty: {safeCount}");
        Console.WriteLine($"Odczyt niezainicjowanej pamięci (usterka DCL): {corruptedCount}");
    }
}