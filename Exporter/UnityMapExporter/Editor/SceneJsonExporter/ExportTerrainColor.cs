#if UNITY_EDITOR
using UnityEditor;
using UnityEngine;
using System;
using System.IO;
using System.Collections.Generic;

public class TerrainPBRExporter : EditorWindow
{
    private Terrain _terrain;
    private bool _flipY = true;
    private bool _copyTextures = true;
    private string _baseName = "Terrain";

    [MenuItem("Tools/Terrain/Export PBR Terrain Set (ControlMaps + Layer PBR)")]
    public static void Open()
    {
        var w = GetWindow<TerrainPBRExporter>("Terrain PBR Export");
        w.minSize = new Vector2(620, 260);

        if (Selection.activeGameObject)
        {
            var t = Selection.activeGameObject.GetComponent<Terrain>();
            if (t) w._terrain = t;
        }
        if (w._terrain == null && Terrain.activeTerrain != null)
            w._terrain = Terrain.activeTerrain;
    }

    private void OnGUI()
    {
        EditorGUILayout.LabelField("Export: ControlMaps + TerrainLayer PBR Textures + JSON", EditorStyles.boldLabel);
        EditorGUILayout.Space(6);

        _terrain = (Terrain)EditorGUILayout.ObjectField("Target Terrain", _terrain, typeof(Terrain), true);
        _baseName = EditorGUILayout.TextField("Base Name", _baseName);
        _flipY = EditorGUILayout.ToggleLeft("Flip Y (Top-Left origin)", _flipY);
        _copyTextures = EditorGUILayout.ToggleLeft("Copy layer textures to export folder", _copyTextures);

        EditorGUILayout.Space(10);
        using (new EditorGUI.DisabledScope(_terrain == null))
        {
            if (GUILayout.Button("Export...", GUILayout.Height(34)))
                Export();
        }

        EditorGUILayout.HelpBox(
            "URP/HDRP TerrainLit MaskMap 채널: R=Metallic, G=AO, B=Height, A=Smoothness.\n" +
            "ControlMap은 4레이어를 RGBA 한 장에 담아 여러 장으로 저장합니다.",
            MessageType.None);
    }

    [Serializable] public class LayerPBRMeta
    {
        public int layerIndex;
        public string layerName;

        public string albedoFile;   // diffuseTexture
        public string normalFile;   // normalMapTexture (없을 수 있음)
        public string maskFile;     // maskMapTexture (없을 수 있음)

        public Vector2 tileSizeWS;
        public Vector2 tileOffsetWS;

        public int controlMapIndex;     // layerIndex/4
        public int controlChannel;      // layerIndex%4 (0=R,1=G,2=B,3=A)

        // 텍스처가 없을 때를 대비한 스칼라(URP/HDRP에서 레이어에 존재)
        public float metallic;
        public float smoothness;
    }

    [Serializable] public class TerrainPBRMeta
    {
        public string name;

        public int alphamapWidth;
        public int alphamapHeight;
        public int layerCount;
        public int controlMapCount;

        public Vector3 terrainSize; // TerrainData.size
        public List<LayerPBRMeta> layers = new List<LayerPBRMeta>();
    }

    private void Export()
    {
        var td = _terrain.terrainData;
        if (td == null) { Debug.LogError("TerrainData null"); return; }

        var layers = td.terrainLayers;
        if (layers == null || layers.Length == 0) { Debug.LogError("TerrainLayer 없음"); return; }

        int aw = td.alphamapWidth;
        int ah = td.alphamapHeight;
        int layerCount = Mathf.Min(td.alphamapLayers, layers.Length);

        float[,,] alpha = td.GetAlphamaps(0, 0, aw, ah); // [y,x,layer] :contentReference[oaicite:2]{index=2}

        string folder = EditorUtility.OpenFolderPanel("Select Export Folder", Application.dataPath, "");
        if (string.IsNullOrEmpty(folder)) return;

        string baseName = string.IsNullOrWhiteSpace(_baseName) ? "Terrain" : _baseName;

        // --- 1) ControlMaps 저장 ---
        int controlMapCount = (layerCount + 3) / 4;

        for (int cm = 0; cm < controlMapCount; ++cm)
        {
            // [수정/핵심] ControlMap은 linear 데이터이므로 linear Texture2D 생성
            var tex = new Texture2D(aw, ah, TextureFormat.RGBA32, false, true);
            var pixels = new Color32[aw * ah];

            for (int y = 0; y < ah; ++y)
            {
                int sy = _flipY ? (ah - 1 - y) : y;
                for (int x = 0; x < aw; ++x)
                {
                    float r = GetAlpha(alpha, sy, x, cm * 4 + 0, layerCount);
                    float g = GetAlpha(alpha, sy, x, cm * 4 + 1, layerCount);
                    float b = GetAlpha(alpha, sy, x, cm * 4 + 2, layerCount);
                    float a = GetAlpha(alpha, sy, x, cm * 4 + 3, layerCount);

                    pixels[y * aw + x] = new Color32(
                        (byte)Mathf.Clamp(Mathf.RoundToInt(r * 255f), 0, 255),
                        (byte)Mathf.Clamp(Mathf.RoundToInt(g * 255f), 0, 255),
                        (byte)Mathf.Clamp(Mathf.RoundToInt(b * 255f), 0, 255),
                        (byte)Mathf.Clamp(Mathf.RoundToInt(a * 255f), 0, 255));
                }
            }

            tex.SetPixels32(pixels);
            tex.Apply(false, false);

            string outPath = Path.Combine(folder, $"{baseName}_ControlMap_{cm}.png");
            File.WriteAllBytes(outPath, tex.EncodeToPNG());
            DestroyImmediate(tex);
        }

        // --- 2) Layer PBR Meta + (옵션) 텍스처 파일 복사 ---
        var meta = new TerrainPBRMeta
        {
            name = _terrain.name,
            alphamapWidth = aw,
            alphamapHeight = ah,
            layerCount = layerCount,
            controlMapCount = controlMapCount,
            terrainSize = td.size
        };

        string projectRoot = Directory.GetParent(Application.dataPath).FullName;

        for (int i = 0; i < layerCount; ++i)
        {
            var tl = layers[i];

            string albedoFile = GetAndOptionallyCopy(tl.diffuseTexture, projectRoot, folder, _copyTextures);
            string normalFile = GetAndOptionallyCopy(tl.normalMapTexture, projectRoot, folder, _copyTextures);
            string maskFile   = GetAndOptionallyCopy(tl.maskMapTexture, projectRoot, folder, _copyTextures);

            var lm = new LayerPBRMeta
            {
                layerIndex = i,
                layerName = tl.name,

                albedoFile = albedoFile,
                normalFile = normalFile,
                maskFile = maskFile,

                tileSizeWS = tl.tileSize,
                tileOffsetWS = tl.tileOffset,

                controlMapIndex = i / 4,
                controlChannel = i % 4,

                metallic = tl.metallic,
                smoothness = tl.smoothness
            };

            meta.layers.Add(lm);
        }

        string jsonPath = Path.Combine(folder, $"{baseName}_TerrainPBR.json");
        File.WriteAllText(jsonPath, JsonUtility.ToJson(meta, true));

        AssetDatabase.Refresh();
        EditorUtility.DisplayDialog("Export Complete", "PBR terrain set exported.", "OK");
    }

    private static float GetAlpha(float[,,] a, int y, int x, int layer, int layerCount)
        => (layer < 0 || layer >= layerCount) ? 0f : Mathf.Clamp01(a[y, x, layer]);

    private static string GetAndOptionallyCopy(Texture tex, string projectRoot, string outFolder, bool copy)
    {
        if (tex == null) return "";

        string assetPath = AssetDatabase.GetAssetPath(tex); // "Assets/.."
        if (string.IsNullOrEmpty(assetPath)) return "";

        string file = Path.GetFileName(assetPath);

        if (copy)
        {
            string srcAbs = Path.Combine(projectRoot, assetPath.Replace("/", Path.DirectorySeparatorChar.ToString()));
            string dstAbs = Path.Combine(outFolder, file);
            File.Copy(srcAbs, dstAbs, true);
        }
        return file;
    }
}
#endif
