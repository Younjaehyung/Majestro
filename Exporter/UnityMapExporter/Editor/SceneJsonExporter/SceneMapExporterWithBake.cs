using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using UnityEditor;
using UnityEngine;
using Clrain.SceneToJson; // BakeSh.cs의 namespace

public class SceneMapExporterWithBakeAndFbx : EditorWindow
{
    [Serializable]
    public class SceneExport
    {
        public string sceneName;
        public string exportedAt;

        public string bakedTextureBaseDir;
        public string fbxBaseDir;

        public List<ObjectExport> objects = new();
    }

    [Serializable]
    public class ObjectExport
    {
        public string name;
        public string path;
        public bool activeInHierarchy;

        public TransformExport transform;

        public string meshName;
        public string meshAssetPath;
        public string meshFbxFile;

        public List<MaterialExport> materials = new();
    }

    [Serializable]
    public class TransformExport
    {
        public float[] worldPosition;
        public float[] worldRotationEuler; // [수정] 쿼터니언 -> 오일러(도)
        public float[] worldScale;

        public float[] localPosition;
        public float[] localRotationEuler; // [수정] 쿼터니언 -> 오일러(도)
        public float[] localScale;
    }

    [Serializable]
    public class MaterialExport
    {
        public int slot;
        public string materialName;
        public string materialAssetPath;
        public BakedExport baked;
    }

    [Serializable]
    public class BakedExport
    {
        public string albedoFile;
        public string normalFile;
        public string metallicFile;
        public string smoothnessFile;
        public string occlusionFile;
        public string emissionFile;
    }

    // =========================
    // UI 옵션
    // =========================
    private int _size = 1024;
    private bool _useSRGB = true;
    private bool _includeInactive = false;
    private bool _autoDetectProperties = true;

    private bool _exportFbxMeshes = true;
    private bool _exportFbxIncludeMaterial = true;

    private string _outputJsonFolder = "Assets/Exported";
    private string _outputTexFolder = "Assets/Exported/BakedPBR";
    private string _outputFbxFolder = "Assets/Exported/FBX";

    private readonly Dictionary<string, ShaderGraphBakeWindow.BakedPbrPaths> _bakeCache = new();
    private readonly Dictionary<int, string> _fbxMeshCache = new();

    [MenuItem("Tools/Scene Json Exporter/Export Scene (Bake PBR + Export FBX Mesh)")]
    public static void Open()
    {
        var w = GetWindow<SceneMapExporterWithBakeAndFbx>();
        w.titleContent = new GUIContent("Scene Export + Bake + FBX");
        w.minSize = new Vector2(540, 350);
        w.Show();
    }

    private void OnGUI()
    {
        EditorGUILayout.LabelField("씬 전체 Export: Transform + Mesh + (Bake PBR) + (Export FBX Mesh)", EditorStyles.boldLabel);
        EditorGUILayout.Space(6);

        _size = EditorGUILayout.IntPopup("Bake Size", _size,
            new[] { "512", "1024", "2048", "4096" },
            new[] { 512, 1024, 2048, 4096 });

        _useSRGB = EditorGUILayout.Toggle("sRGB Output(Base/Emission)", _useSRGB);
        _includeInactive = EditorGUILayout.Toggle("Include Inactive", _includeInactive);
        _autoDetectProperties = EditorGUILayout.Toggle("Auto Detect Properties", _autoDetectProperties);

        EditorGUILayout.Space(8);
        EditorGUILayout.LabelField("FBX Export", EditorStyles.boldLabel);
        _exportFbxMeshes = EditorGUILayout.Toggle("Export Mesh To FBX", _exportFbxMeshes);
        using (new EditorGUI.DisabledScope(!_exportFbxMeshes))
        {
            _exportFbxIncludeMaterial = EditorGUILayout.Toggle("Attach 1 Material When Exporting", _exportFbxIncludeMaterial);
        }

        EditorGUILayout.Space(8);
        EditorGUILayout.LabelField("Output Folders", EditorStyles.boldLabel);
        _outputJsonFolder = EditorGUILayout.TextField("JSON Folder", _outputJsonFolder);
        _outputTexFolder = EditorGUILayout.TextField("Texture Folder", _outputTexFolder);
        _outputFbxFolder = EditorGUILayout.TextField("FBX Folder", _outputFbxFolder);

        EditorGUILayout.Space(12);

        if (GUILayout.Button("EXPORT (JSON + Bake + FBX)", GUILayout.Height(40)))
        {
            ExportAll();
        }

        EditorGUILayout.HelpBox(
            "대상: MeshRenderer / SkinnedMeshRenderer\n" +
            "- JSON: Transform(월드/로컬), Mesh 이름, Material 슬롯별 베이크 텍스처 '파일명' 기록\n" +
            "- 회전값: Quaternion이 아니라 Euler(도)로 저장 (Inspector 표시값 기준)\n" +
            "- FBX: 같은 Mesh는 1번만 추출(캐시)하고 JSON에는 '파일명'만 기록\n\n" +
            "Unity FBX Exporter 패키지가 설치되어 있으면 자동으로 FBX 추출이 실행됩니다.",
            MessageType.Info);
    }

    private void ExportAll()
    {
        EnsureFolder(_outputJsonFolder);
        EnsureFolder(_outputTexFolder);
        if (_exportFbxMeshes)
            EnsureFolder(_outputFbxFolder);

        var scene = UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene();
        if (!scene.IsValid())
        {
            Debug.LogError("유효한 씬이 아닙니다.");
            return;
        }

        _bakeCache.Clear();
        _fbxMeshCache.Clear();

        var export = new SceneExport
        {
            sceneName = scene.name,
            exportedAt = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),
            bakedTextureBaseDir = _outputTexFolder.Replace("\\", "/"),
            fbxBaseDir = _outputFbxFolder.Replace("\\", "/"),
            objects = new List<ObjectExport>()
        };

        var renderers = Resources.FindObjectsOfTypeAll<Renderer>();
        int total = renderers.Length;
        int processed = 0;

        try
        {
            foreach (var r in renderers)
            {
                processed++;
                if (r == null) continue;

                EditorUtility.DisplayProgressBar("Exporting Scene...", $"Processing Renderer {processed}/{total}", (float)processed / total);

                if (!r.gameObject.scene.IsValid()) continue;
                if (!r.gameObject.scene.Equals(scene)) continue;

                if (!_includeInactive && !r.gameObject.activeInHierarchy)
                    continue;

                Mesh mesh = null;
                if (r is MeshRenderer)
                {
                    var mf = r.GetComponent<MeshFilter>();
                    if (mf == null || mf.sharedMesh == null) continue;
                    mesh = mf.sharedMesh;
                }
                else if (r is SkinnedMeshRenderer smr)
                {
                    if (smr.sharedMesh == null) continue;
                    mesh = smr.sharedMesh;
                }
                else
                {
                    continue;
                }

                string meshAssetPath = AssetDatabase.GetAssetPath(mesh);

                // FBX 메시 추출(캐시)
                string meshFbxUnityPath = null;
                if (_exportFbxMeshes)
                {
                    meshFbxUnityPath = ExportMeshToFbxCached(mesh, r.sharedMaterials, _outputFbxFolder);
                }

                var obj = new ObjectExport
                {
                    name = r.gameObject.name,
                    path = GetHierarchyPath(r.transform),
                    activeInHierarchy = r.gameObject.activeInHierarchy,
                    transform = ExportTransformEuler(r.transform), // [수정] Euler 추출 함수 사용

                    meshName = mesh.name,
                    meshAssetPath = string.IsNullOrEmpty(meshAssetPath) ? null : meshAssetPath,
                    meshFbxFile = FileNameOnly(meshFbxUnityPath),

                    materials = new List<MaterialExport>()
                };

                var mats = r.sharedMaterials;
                for (int i = 0; i < mats.Length; i++)
                {
                    var m = mats[i];
                    if (m == null) continue;

                    var matAssetPath = AssetDatabase.GetAssetPath(m);

                    string prefix = $"{Sanitize(mesh.name)}_{Sanitize(m.name)}_{i}";

                    string cacheKey = $"{m.GetInstanceID()}::{prefix}";
                    if (!_bakeCache.TryGetValue(cacheKey, out var baked))
                    {
                        baked = ShaderGraphBakeWindow.BakeMaterialMapsForExporter(
                            m,
                            _size,
                            _size,
                            _useSRGB,
                            _outputTexFolder,
                            prefix,
                            new ShaderGraphBakeWindow.BakeOptions
                            {
                                AutoDetectProperties = _autoDetectProperties,
                                ExportSmoothness = true,
                                ExportOcclusion = true,
                                ExportEmission = true,
                                UseUniquePath = true
                            });

                        _bakeCache[cacheKey] = baked;
                    }

                    obj.materials.Add(new MaterialExport
                    {
                        slot = i,
                        materialName = m.name,
                        materialAssetPath = string.IsNullOrEmpty(matAssetPath) ? null : matAssetPath,
                        baked = new BakedExport
                        {
                            albedoFile = FileNameOnly(baked.Albedo),
                            normalFile = FileNameOnly(baked.Normal),
                            metallicFile = FileNameOnly(baked.Metallic),
                            smoothnessFile = FileNameOnly(baked.Smoothness),
                            occlusionFile = FileNameOnly(baked.Occlusion),
                            emissionFile = FileNameOnly(baked.Emission)
                        }
                    });
                }

                export.objects.Add(obj);
            }
        }
        finally
        {
            EditorUtility.ClearProgressBar();
        }

        var json = JsonUtility.ToJson(export, prettyPrint: true);
        string jsonPath = AssetDatabase.GenerateUniqueAssetPath(
            Path.Combine(_outputJsonFolder, $"{scene.name}_MapExport.json"));

        File.WriteAllText(jsonPath, json);

        AssetDatabase.ImportAsset(jsonPath);
        AssetDatabase.Refresh();

        EditorUtility.DisplayDialog(
            "Export Complete",
            $"JSON: {jsonPath}\nTextures: {_outputTexFolder}\nFBX: {(_exportFbxMeshes ? _outputFbxFolder : "(disabled)")}\nObjects: {export.objects.Count}",
            "OK");

        Debug.Log($"Export 완료\nJSON: {jsonPath}\nTextures: {_outputTexFolder}\nFBX: {(_exportFbxMeshes ? _outputFbxFolder : "(disabled)")}\nObjects: {export.objects.Count}");
    }

    // =========================
    // Transform Export (Euler)
    // =========================
    private static TransformExport ExportTransformEuler(Transform t)
    {
        var pW = t.position;
        var rW = t.eulerAngles;      // [수정] Quaternion -> Euler (Inspector 표시와 동일 계열)
        var sW = t.lossyScale;

        var pL = t.localPosition;
        var rL = t.localEulerAngles; // [수정] Quaternion -> Euler
        var sL = t.localScale;

        return new TransformExport
        {
            worldPosition = new[] { pW.x, pW.y, pW.z },
            worldRotationEuler = new[] { rW.x, rW.y, rW.z }, // [수정]
            worldScale = new[] { sW.x, sW.y, sW.z },

            localPosition = new[] { pL.x, pL.y, pL.z },
            localRotationEuler = new[] { rL.x, rL.y, rL.z }, // [수정]
            localScale = new[] { sL.x, sL.y, sL.z }
        };
    }

    // =========================
    // FBX Export (Mesh 단위 캐시)
    // =========================
    private string ExportMeshToFbxCached(Mesh mesh, Material[] materials, string outFolder)
    {
        if (mesh == null) return null;

        int id = mesh.GetInstanceID();
        if (_fbxMeshCache.TryGetValue(id, out var cachedPath))
            return cachedPath;

        string baseName = Sanitize(mesh.name);
        string fbxPath = AssetDatabase.GenerateUniqueAssetPath(Path.Combine(outFolder, $"{baseName}.fbx"));

        bool ok = FbxExporterUtil.TryExportMeshAsFbx(mesh,
            (_exportFbxIncludeMaterial && materials != null && materials.Length > 0) ? materials[0] : null,
            fbxPath);

        if (!ok)
        {
            Debug.LogWarning($"FBX Export 실패(또는 FBX Exporter 패키지 없음): mesh={mesh.name}");
            _fbxMeshCache[id] = null;
            return null;
        }

        _fbxMeshCache[id] = fbxPath;
        return fbxPath;
    }

    // =========================
    // Path 유틸 (파일명만)
    // =========================
    private static string FileNameOnly(string path)
    {
        if (string.IsNullOrEmpty(path)) return null;
        path = path.Replace("\\", "/");
        int slash = path.LastIndexOf('/');
        return (slash >= 0) ? path.Substring(slash + 1) : Path.GetFileName(path);
    }

    // =========================
    // Hierarchy/Folder 유틸
    // =========================
    private static string GetHierarchyPath(Transform t)
    {
        var stack = new Stack<string>();
        while (t != null)
        {
            stack.Push(t.name);
            t = t.parent;
        }
        return string.Join("/", stack);
    }

    private static void EnsureFolder(string folder)
    {
        if (AssetDatabase.IsValidFolder(folder))
            return;

        var parts = folder.Split('/');
        if (parts.Length == 0 || parts[0] != "Assets")
            throw new Exception("폴더는 Assets로 시작해야 합니다. 예: Assets/Exported");

        string cur = "Assets";
        for (int i = 1; i < parts.Length; i++)
        {
            string next = $"{cur}/{parts[i]}";
            if (!AssetDatabase.IsValidFolder(next))
                AssetDatabase.CreateFolder(cur, parts[i]);
            cur = next;
        }
    }

    private static string Sanitize(string s)
    {
        foreach (char c in Path.GetInvalidFileNameChars())
            s = s.Replace(c, '_');
        return s;
    }

    // =========================
    // Unity FBX Exporter 호출 유틸(리플렉션)
    // =========================
    private static class FbxExporterUtil
    {
        private static Type _modelExporterType;
        private static MethodInfo _exportObjectMethod;

        private static bool Ensure()
        {
            if (_exportObjectMethod != null) return true;

            foreach (var asm in AppDomain.CurrentDomain.GetAssemblies())
            {
                var t = asm.GetType("UnityEditor.Formats.Fbx.Exporter.ModelExporter");
                if (t == null) continue;
                _modelExporterType = t;
                break;
            }

            if (_modelExporterType == null)
                return false;

            _exportObjectMethod = _modelExporterType.GetMethod(
                "ExportObject",
                BindingFlags.Public | BindingFlags.Static,
                binder: null,
                types: new[] { typeof(string), typeof(UnityEngine.Object) },
                modifiers: null);

            return _exportObjectMethod != null;
        }

        public static bool TryExportMeshAsFbx(Mesh mesh, Material matOrNull, string unityAssetPathFbx)
        {
            if (!Ensure())
                return false;

            var go = new GameObject($"__FBXEXPORT__{mesh.name}");
            try
            {
                var mf = go.AddComponent<MeshFilter>();
                mf.sharedMesh = mesh;

                var mr = go.AddComponent<MeshRenderer>();
                if (matOrNull != null)
                    mr.sharedMaterial = matOrNull;

                _exportObjectMethod.Invoke(null, new object[] { unityAssetPathFbx, go });

                AssetDatabase.ImportAsset(unityAssetPathFbx);
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"FBX Export Exception: {e}");
                return false;
            }
            finally
            {
                DestroyImmediate(go);
            }
        }
    }
}
