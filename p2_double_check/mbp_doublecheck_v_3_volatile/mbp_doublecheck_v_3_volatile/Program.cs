using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;

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

    //1: Błędny DCL
    public class Singleton1_Bledny
    {
        private static Data _instance;
        private static readonly object _lock = new object();
        public static void Reset() => _instance = null;

        public static Data GetInstance()
        {
            if (_instance == null)
            {
                lock (_lock)
                {
                    if (_instance == null)
                    {
                        _instance = new Data();
                    }
                }
            }
            return _instance;
        }
    }

    //2: Bez pierwszego IF'a
    public class Singleton2_Powolny
    {
        private static Data _instance;
        private static readonly object _lock = new object();
        public static void Reset() => _instance = null;

        public static Data GetInstance()
        {
            lock (_lock)
            {
                if (_instance == null)
                {
                    _instance = new Data();
                }
            }
            return _instance;
        }
    }

    //3: Z volatile
    
    public class Singleton3_Poprawny
    {
        private static volatile Data _instance;
        private static readonly object _lock = new object();
        public static void Reset() => _instance = null;

        public static Data GetInstance()
        {
            if (_instance == null)
            {
                lock (_lock)
                {
                    if (_instance == null)
                    {
                        _instance = new Data();
                    }
                }
            }
            return _instance;
        }
    }

    class Program
    {
        static void Main()
        {
            Console.WriteLine("Start testów DCL (od 1 do 32 wątków)...");
            long attempts = 10000;

            Console.WriteLine(" TEST 1: Błędny DCL");

            for (int watki = 1; watki <= 32; watki++)
            {
                bool error = false;

                for (long i = 0; i < attempts; i++)
                {
                    Singleton1_Bledny.Reset();
                    Barrier barrier = new Barrier(watki);
                    Task[] tasks = new Task[watki];

                    for (int t = 0; t < watki; t++)
                    {
                        tasks[t] = Task.Run(() =>
                        {
                            barrier.SignalAndWait();
                            Data data = Singleton1_Bledny.GetInstance();
                            try
                            {
                                if (data.Lista == null || data.Lista.Count < 1000) error = true;
                            }
                            catch { error = true; }
                        });
                    }
                    Task.WaitAll(tasks);
                    if (error) break;
                }

                Singleton1_Bledny.GetInstance();
                var opcje = new ParallelOptions { MaxDegreeOfParallelism = watki };
                Parallel.For(0, 100_000, opcje, i => Singleton1_Bledny.GetInstance());
                var sw = Stopwatch.StartNew();
                Parallel.For(0, 1000000, opcje, i => Singleton1_Bledny.GetInstance());
                sw.Stop();

                string status = error ? "BŁĄD!" : "Brak błędu";
                Console.WriteLine($"Wątki: {watki} | Przepustowość: {1.0 / sw.Elapsed.TotalSeconds:F4} MOPS");
            }

            Console.WriteLine(" TEST 2: Tylko Lock");

            for (int watki = 1; watki <= 32; watki++)
            {
                bool error = false;

                for (long i = 0; i < attempts; i++)
                {
                    Singleton2_Powolny.Reset();
                    Barrier barrier = new Barrier(watki);
                    Task[] tasks = new Task[watki];

                    for (int t = 0; t < watki; t++)
                    {
                        tasks[t] = Task.Run(() =>
                        {
                            barrier.SignalAndWait();
                            Data data = Singleton2_Powolny.GetInstance();
                            try
                            {
                                if (data.Lista == null || data.Lista.Count < 1000) error = true;
                            }
                            catch { error = true; }
                        });
                    }
                    Task.WaitAll(tasks);
                    if (error) break;
                }

                Singleton2_Powolny.GetInstance();
                var opcje = new ParallelOptions { MaxDegreeOfParallelism = watki };
                Parallel.For(0, 100_000, opcje, i => Singleton2_Powolny.GetInstance());
                var sw = Stopwatch.StartNew();
                Parallel.For(0, 1000000, opcje, i => Singleton2_Powolny.GetInstance());
                sw.Stop();

                string status = error ? "BŁĄD!" : "Brak błędu";
                Console.WriteLine($"Wątki: {watki} | Przepustowość: {1.0 / sw.Elapsed.TotalSeconds:F4} MOPS");
            }

            Console.WriteLine(" TEST 3: Z volatile");

            for (int watki = 1; watki <= 32; watki++)
            {
                bool error = false;

                for (long i = 0; i < attempts; i++)
                {
                    Singleton3_Poprawny.Reset();
                    Barrier barrier = new Barrier(watki);
                    Task[] tasks = new Task[watki];

                    for (int t = 0; t < watki; t++)
                    {
                        tasks[t] = Task.Run(() =>
                        {
                            barrier.SignalAndWait();
                            Data data = Singleton3_Poprawny.GetInstance();
                            try
                            {
                                if (data.Lista == null || data.Lista.Count < 1000) error = true;
                            }
                            catch { error = true; }
                        });
                    }
                    Task.WaitAll(tasks);
                    if (error) break;
                }

                Singleton3_Poprawny.GetInstance();
                var opcje = new ParallelOptions { MaxDegreeOfParallelism = watki };
                Parallel.For(0, 100_000, opcje, i => Singleton3_Poprawny.GetInstance());
                var sw = Stopwatch.StartNew();
                Parallel.For(0, 1000000, opcje, i => Singleton3_Poprawny.GetInstance());
                sw.Stop();

                string status = error ? "BŁĄD!" : "Brak błędu";
                Console.WriteLine($"Wątki: {watki} | Przepustowość: {1.0 / sw.Elapsed.TotalSeconds:F4} MOPS");
            }
        }
    }
}