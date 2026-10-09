using System.Diagnostics;

namespace REALM.UI;

internal static class Program
{
    private static readonly string RepoRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));

    private static void Main()
    {
        var nativeExe = Path.Combine(RepoRoot, "REALM.Native", "build", "GeoSandboxNative.exe");

        if (!File.Exists(nativeExe))
        {
            Console.WriteLine("Native engine not built yet.");
            Console.WriteLine("Build it with:");
            Console.WriteLine("g++ -std=c++17 -O2 -I src src/App/main.cpp src/Engine/VoxelGrid.cpp src/Renderer/OpenGLRenderer.cpp -o build\\GeoSandboxNative.exe -lopengl32 -lglu32 -lgdi32 -luser32");
            Console.WriteLine("Press any key to exit...");
            Console.ReadKey(true);
            return;
        }

        Console.WriteLine("Launching native terrain mesh...");
        var startInfo = new ProcessStartInfo
        {
            FileName = nativeExe,
            WorkingDirectory = Path.GetDirectoryName(nativeExe)!,
            UseShellExecute = true
        };

        Process.Start(startInfo);
    }
}
