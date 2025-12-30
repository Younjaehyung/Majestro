#if UNITY_EDITOR
using UnityEditor;
using UnityEngine;
using System.IO;

public class MapExporterWindow : EditorWindow
{
    private string rootName = "MapRoot";
    private string fileName = "map01.json";

    [MenuItem("Tools/Map Exporter")]
    public static void Open()
    {
        GetWindow<MapExporterWindow>("Map Exporter");
    }

    private void OnGUI()
    {
        GUILayout.Label("Map Export Settings", EditorStyles.boldLabel);

        rootName = EditorGUILayout.TextField("Map Root Name", rootName);
        fileName = EditorGUILayout.TextField("File Name", fileName);

        GUILayout.Space(10);

        if (GUILayout.Button("Export Map to JSON", GUILayout.Height(30)))
        {
            Export();
        }
    }

    private void Export()
    {
        FinalMapJson data = MapScanner.BuildFinal(rootName);
        if (data == null)
        {
            Debug.LogError("Export 실패 (MapRoot 또는 Terrain 없음)");
            return;
        }

        string json = JsonUtility.ToJson(data, true);

        string dir = Path.Combine(Application.dataPath, "../ExportedMaps");
        if (!Directory.Exists(dir))
            Directory.CreateDirectory(dir);

        string path = Path.Combine(dir, fileName);
        File.WriteAllText(path, json);

        Debug.Log("Export 완료: " + path);
        EditorUtility.RevealInFinder(path);
    }
}
#endif
