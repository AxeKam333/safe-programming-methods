using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace DclTest
{
    public class Data
    {
        public List<int> Lista;

        public Data()
        {
            Lista = new List<int>();
            for (int i = 0; i < 1000; i++) Lista.Add(i);
        }
    }

    public class Singleton
    {
        private static Data _instance;
        private static readonly object _lock = new object();
        public static void Reset() => _instance = null;

        public static Data GetInstance()
        {
            // usunięty 1. if
            lock (_lock)
            {
                if (_instance == null)
                {
                    _instance = new Data();
                }
                return _instance;
            }
        }
    }

    class Program
    {
        static void Main()
        {
            int threadCount = Environment.ProcessorCount;
            Console.WriteLine($"Start testu popr. wersji DCL na {threadCount} wątkach...");

            bool error = false;
            long attempts = 1000000;

            for (long i = 0; i < attempts; i++)
            {
                Singleton.Reset();
                Barrier barrier = new Barrier(threadCount);
                Task[] tasks = new Task[threadCount];

                for (int t = 0; t < threadCount; t++)
                {
                    tasks[t] = Task.Run(() =>
                    {
                        barrier.SignalAndWait();

                        Data data = Singleton.GetInstance();

                        try
                        {
                            if (data.Lista == null || data.Lista.Count < 1000)
                                error = true;
                        }
                        catch
                        {
                            error = true;
                        }
                    });
                }

                Task.WaitAll(tasks);

                if (error)
                {
                    Console.WriteLine($"\nBŁĄD Otrzymano niedokończony obiekt w próbie nr {i}!");
                    break;
                }
            }

            if (!error)
            {
                Console.WriteLine("\nWykonano 1 milion prób. Nie wywołano błędu.");
            }
        }
    }
}