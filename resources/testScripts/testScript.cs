using System;

namespace Tests
{
public class CSharpTesting
{
    public float MyPublicFloatVar = 5.0f;

    public void PrintFloatVar()
    {
        Console.WriteLine("MyPublicFloatVar = {0:F}", MyPublicFloatVar);
    }

    private void IncrementFloatVar(float value)
    {
        MyPublicFloatVar += value;
    }
}

public class UWU {}
public class OWO {}
public class OVO {}
}
