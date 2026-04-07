using System;
using System.Runtime.Serialization;
using System.Threading;
using static System.Runtime.InteropServices.JavaScript.JSType;
class Foo2
{
    public int[] Data;

    public Foo2()
    {
        Data = new int[6] { 0,0,0,0,0,0 };
    }
}
public class UninitializedExample
{
    // bez volatile
    private static Foo2 FooObj;
    public int Data;

    public static void Writer()
    {
        if (FooObj == null)
        {
            lock (typeof(Foo2))
            {
                if (FooObj == null)
                {
                    //FooObj = new Foo();

                    // Jak potencjalnie kompilator JIT lub procesor ma prawo zmienić kolejność operacji:

                    // Krok 1: Alokacja pamięci
                    // Krok 3: Przypisanie referencji (przed inicjalizacją).

                    FooObj = FormatterServices.GetUninitializedObject(typeof(Foo2)) as Foo2;
                    // Metoda FormatterServices.GetUninitializedObject alokuje pamięć dla obiektu na stercie i inicjuje wszystkie jej bajty zerami, ale całkowicie pomija wywołanie konstruktora.

                    Thread.Sleep(50);

                    // Krok 2: Wywołanie konstruktora
                    FooObj.Data = new int[6] { 1, 2, 3, 4, 5, 67 };

                }
            }
        }
    }

    public static void _Main()
    {
        Thread writer = new Thread(Writer);
        Thread reader = new Thread(() =>
        {
            Thread.Sleep(10);
            if (FooObj != null)
            {
                // Referencja obiektu istnieje, ale jego stan wewnętrzny nie został zainicjalizowany.
                Console.WriteLine($"Odczytana wartość: {FooObj.Data[0]} (Oczekiwana: 1)");
            }
        });

        writer.Start();
        reader.Start();
        writer.Join();
        reader.Join();
    }
}
