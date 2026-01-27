using System.IO;
using UnityEditor;
using UnityEngine;

#if UNITY_EDITOR

namespace Clrain.SceneToJson
{
    public class ShaderGraphBakeWindow : EditorWindow
    {
        private const int DefaultSize = 1024;
        private Material material;
        private MeshRenderer meshRenderer;
        private int width = DefaultSize;
        private int height = DefaultSize;
        private bool useSRGB = true;
        private string outputName = "BakedShaderGraph";
        private string outputFolder = "Assets";
        private Color clearColor = new Color(0, 0, 0, 0);
        private string baseColorProperty = "_BaseMap";
        private string baseColorTintProperty = "_BaseColor";
        private string normalProperty = "_BumpMap";
        private string metallicProperty = "_MetallicGlossMap";
        private string metallicValueProperty = "_Metallic";
        private string smoothnessProperty = "_Smoothness";
        private string occlusionProperty = "_OcclusionMap";
        private string emissionProperty = "_EmissionMap";
        private string emissionColorProperty = "_EmissionColor";
        private bool exportSmoothness = true;
        private bool exportOcclusion = true;
        private bool exportEmission = true;

        [MenuItem("Tools/Scene Json Exporter/Shader Graph Bake")]
        public static void Open()
        {
            var window = GetWindow<ShaderGraphBakeWindow>();
            window.titleContent = new GUIContent("Shader Graph Bake");
            window.minSize = new Vector2(360f, 240f);
            window.Show();
        }

        [MenuItem("Assets/Scene Json Exporter/Bake Shader Graph To Texture", true)]
        private static bool ValidateQuickBake()
        {
            return Selection.activeObject is Material;
        }

        [MenuItem("Assets/Scene Json Exporter/Bake Shader Graph To Texture")]
        private static void QuickBake()
        {
            if (Selection.activeObject is not Material selectedMaterial)
            {
                EditorUtility.DisplayDialog("No Material", "Please select a Material asset.", "OK");
                return;
            }

            var outputPath = EditorUtility.SaveFilePanelInProject(
                "Save Baked Texture",
                selectedMaterial.name + "_Baked",
                "png",
                "Choose a location to save the baked texture.");
            if (string.IsNullOrEmpty(outputPath))
            {
                return;
            }

            BakeMaterialToTexture(selectedMaterial, DefaultSize, DefaultSize, true, outputPath);
        }

        private void OnGUI()
        {
            EditorGUILayout.LabelField("Shader Graph Bake", EditorStyles.boldLabel);
            EditorGUILayout.Space(4f);

            material = (Material)EditorGUILayout.ObjectField("Material", material, typeof(Material), false);
            meshRenderer = (MeshRenderer)EditorGUILayout.ObjectField("Mesh Renderer", meshRenderer, typeof(MeshRenderer), true);
            width = EditorGUILayout.IntField("Width", Mathf.Max(1, width));
            height = EditorGUILayout.IntField("Height", Mathf.Max(1, height));
            useSRGB = EditorGUILayout.Toggle("sRGB Output", useSRGB);
            outputName = EditorGUILayout.TextField("File Name", outputName);
            clearColor = EditorGUILayout.ColorField("Clear Color", clearColor);
            EditorGUILayout.LabelField("Material Map Properties", EditorStyles.boldLabel);
            baseColorProperty = EditorGUILayout.TextField("Base Color Map", baseColorProperty);
            baseColorTintProperty = EditorGUILayout.TextField("Base Color Tint", baseColorTintProperty);
            normalProperty = EditorGUILayout.TextField("Normal Map", normalProperty);
            metallicProperty = EditorGUILayout.TextField("Metallic Map", metallicProperty);
            metallicValueProperty = EditorGUILayout.TextField("Metallic Value", metallicValueProperty);
            smoothnessProperty = EditorGUILayout.TextField("Smoothness Value", smoothnessProperty);
            occlusionProperty = EditorGUILayout.TextField("Occlusion Map", occlusionProperty);
            emissionProperty = EditorGUILayout.TextField("Emission Map", emissionProperty);
            emissionColorProperty = EditorGUILayout.TextField("Emission Color", emissionColorProperty);
            exportSmoothness = EditorGUILayout.Toggle("Export Smoothness", exportSmoothness);
            exportOcclusion = EditorGUILayout.Toggle("Export Occlusion", exportOcclusion);
            exportEmission = EditorGUILayout.Toggle("Export Emission", exportEmission);

            if (GUILayout.Button("Auto Detect Properties"))
            {
                AutoDetectProperties();
            }

            using (new EditorGUILayout.HorizontalScope())
            {
                outputFolder = EditorGUILayout.TextField("Output Folder", outputFolder);
                if (GUILayout.Button("Select", GUILayout.Width(60f)))
                {
                    var folder = EditorUtility.OpenFolderPanel("Select Output Folder", "Assets", "");
                    if (!string.IsNullOrEmpty(folder))
                    {
                        if (folder.StartsWith(Application.dataPath))
                        {
                            outputFolder = "Assets" + folder.Substring(Application.dataPath.Length);
                        }
                        else
                        {
                            EditorUtility.DisplayDialog("Invalid Folder",
                                "Please select a folder inside the project Assets directory.", "OK");
                        }
                    }
                }
            }

            EditorGUILayout.Space(8f);

            using (new EditorGUI.DisabledScope(material == null))
            {
                if (GUILayout.Button("Bake Material To Texture", GUILayout.Height(32f)))
                {
                    if (string.IsNullOrWhiteSpace(outputFolder) || !AssetDatabase.IsValidFolder(outputFolder))
                    {
                        EditorUtility.DisplayDialog("Invalid Output Folder",
                            "Please enter a valid folder inside the Assets directory.", "OK");
                        return;
                    }

                    var outputPath = AssetDatabase.GenerateUniqueAssetPath(
                        Path.Combine(outputFolder, outputName + ".png"));
                    BakeMaterialToTexture(material, width, height, useSRGB, outputPath);
                }
            }

            using (new EditorGUI.DisabledScope(material == null))
            {
                if (GUILayout.Button("Bake Material Maps (Albedo/Normal/Metallic)", GUILayout.Height(32f)))
                {
                    if (string.IsNullOrWhiteSpace(outputFolder) || !AssetDatabase.IsValidFolder(outputFolder))
                    {
                        EditorUtility.DisplayDialog("Invalid Output Folder",
                            "Please enter a valid folder inside the Assets directory.", "OK");
                        return;
                    }

                    BakeMaterialMaps(material, width, height, useSRGB, outputFolder, outputName);
                }
            }

            using (new EditorGUI.DisabledScope(meshRenderer == null))
            {
                if (GUILayout.Button("Bake Mesh Renderer To Texture", GUILayout.Height(32f)))
                {
                    if (string.IsNullOrWhiteSpace(outputFolder) || !AssetDatabase.IsValidFolder(outputFolder))
                    {
                        EditorUtility.DisplayDialog("Invalid Output Folder",
                            "Please enter a valid folder inside the Assets directory.", "OK");
                        return;
                    }

                    var outputPath = AssetDatabase.GenerateUniqueAssetPath(
                        Path.Combine(outputFolder, outputName + ".png"));
                    BakeMeshRendererToTexture(meshRenderer, width, height, useSRGB, clearColor, outputPath);
                }
            }

            EditorGUILayout.HelpBox(
                "Material bake uses Graphics.Blit (no mesh/lighting data). Mesh Renderer bake renders the " +
                "actual mesh with its material and scene lighting into a RenderTexture before saving. " +
                "Material maps bake exports base color, normal, metallic, and optional smoothness/occlusion/emission outputs.",
                MessageType.Info);
        }

        private static void BakeMaterialToTexture(
            Material bakeMaterial,
            int bakeWidth,
            int bakeHeight,
            bool bakeSRGB,
            string outputPath)
        {
            if (bakeMaterial == null)
            {
                EditorUtility.DisplayDialog("No Material", "Please assign a material to bake.", "OK");
                return;
            }

            if (string.IsNullOrWhiteSpace(outputPath))
            {
                EditorUtility.DisplayDialog("Invalid Output Path",
                    "Please provide a valid file path for the baked texture.", "OK");
                return;
            }

            var colorSpace = bakeSRGB ? RenderTextureReadWrite.sRGB : RenderTextureReadWrite.Linear;
            var rt = RenderTexture.GetTemporary(bakeWidth, bakeHeight, 0, RenderTextureFormat.ARGB32, colorSpace);
            var prevRt = RenderTexture.active;

            try
            {
                Graphics.Blit(null, rt, bakeMaterial);
                RenderTexture.active = rt;

                var tex = new Texture2D(bakeWidth, bakeHeight, TextureFormat.RGBA32, false, !bakeSRGB);
                tex.ReadPixels(new Rect(0, 0, bakeWidth, bakeHeight), 0, 0);
                tex.Apply();

                var pngData = tex.EncodeToPNG();
                File.WriteAllBytes(outputPath, pngData);

                AssetDatabase.ImportAsset(outputPath);
                Selection.activeObject = AssetDatabase.LoadAssetAtPath<Texture2D>(outputPath);

                DestroyImmediate(tex);
            }
            finally
            {
                RenderTexture.active = prevRt;
                RenderTexture.ReleaseTemporary(rt);
            }

            AssetDatabase.SaveAssets();
            EditorUtility.DisplayDialog("Bake Complete", "Texture saved successfully.", "OK");
        }

        private static void BakeMeshRendererToTexture(
            MeshRenderer renderer,
            int bakeWidth,
            int bakeHeight,
            bool bakeSRGB,
            Color clear,
            string outputPath)
        {
            if (renderer == null)
            {
                EditorUtility.DisplayDialog("No Renderer", "Please assign a MeshRenderer to bake.", "OK");
                return;
            }

            if (string.IsNullOrWhiteSpace(outputPath))
            {
                EditorUtility.DisplayDialog("Invalid Output Path",
                    "Please provide a valid file path for the baked texture.", "OK");
                return;
            }

            var bounds = renderer.bounds;
            var center = bounds.center;
            var radius = bounds.extents.magnitude;
            var tempCamera = new GameObject("ShaderGraphBakeCamera").AddComponent<Camera>();
            tempCamera.enabled = false;
            tempCamera.orthographic = false;
            tempCamera.clearFlags = CameraClearFlags.SolidColor;
            tempCamera.backgroundColor = clear;
            tempCamera.nearClipPlane = 0.01f;
            tempCamera.farClipPlane = Mathf.Max(1f, radius * 4f);
            tempCamera.cullingMask = 1 << renderer.gameObject.layer;
            tempCamera.transform.position = center + new Vector3(0f, 0f, -radius * 2.5f);
            tempCamera.transform.LookAt(center);

            var colorSpace = bakeSRGB ? RenderTextureReadWrite.sRGB : RenderTextureReadWrite.Linear;
            var rt = RenderTexture.GetTemporary(bakeWidth, bakeHeight, 24, RenderTextureFormat.ARGB32, colorSpace);
            var prevRt = RenderTexture.active;

            try
            {
                tempCamera.targetTexture = rt;
                tempCamera.Render();
                RenderTexture.active = rt;

                var tex = new Texture2D(bakeWidth, bakeHeight, TextureFormat.RGBA32, false, !bakeSRGB);
                tex.ReadPixels(new Rect(0, 0, bakeWidth, bakeHeight), 0, 0);
                tex.Apply();

                var pngData = tex.EncodeToPNG();
                File.WriteAllBytes(outputPath, pngData);

                AssetDatabase.ImportAsset(outputPath);
                Selection.activeObject = AssetDatabase.LoadAssetAtPath<Texture2D>(outputPath);

                DestroyImmediate(tex);
            }
            finally
            {
                RenderTexture.active = prevRt;
                RenderTexture.ReleaseTemporary(rt);
                DestroyImmediate(tempCamera.gameObject);
            }

            AssetDatabase.SaveAssets();
            EditorUtility.DisplayDialog("Bake Complete", "Texture saved successfully.", "OK");
        }

        private void BakeMaterialMaps(
            Material bakeMaterial,
            int bakeWidth,
            int bakeHeight,
            bool bakeSRGB,
            string outputDir,
            string filePrefix)
        {
            if (bakeMaterial == null)
            {
                EditorUtility.DisplayDialog("No Material", "Please assign a material to bake.", "OK");
                return;
            }

            if (string.IsNullOrWhiteSpace(outputDir))
            {
                EditorUtility.DisplayDialog("Invalid Output Folder",
                    "Please enter a valid folder inside the Assets directory.", "OK");
                return;
            }

            var albedoPath = AssetDatabase.GenerateUniqueAssetPath(
                Path.Combine(outputDir, filePrefix + "_Albedo.png"));
            ExportBaseColorMap(bakeMaterial, albedoPath, bakeWidth, bakeHeight, bakeSRGB);

            ExportMaterialTextureOrFallback(
                bakeMaterial,
                normalProperty,
                Path.Combine(outputDir, filePrefix + "_Normal.png"),
                bakeWidth,
                bakeHeight,
                new Color(0.5f, 0.5f, 1f, 1f),
                false);

            ExportMaterialTextureOrFallback(
                bakeMaterial,
                metallicProperty,
                Path.Combine(outputDir, filePrefix + "_Metallic.png"),
                bakeWidth,
                bakeHeight,
                GetMetallicFallback(bakeMaterial),
                false);

            if (exportSmoothness)
            {
                ExportSmoothnessMap(
                    bakeMaterial,
                    Path.Combine(outputDir, filePrefix + "_Smoothness.png"),
                    bakeWidth,
                    bakeHeight);
            }

            if (exportOcclusion)
            {
                ExportMaterialTextureOrFallback(
                    bakeMaterial,
                    occlusionProperty,
                    Path.Combine(outputDir, filePrefix + "_Occlusion.png"),
                    bakeWidth,
                    bakeHeight,
                    Color.white,
                    false);
            }

            if (exportEmission)
            {
                ExportEmissionMap(
                    bakeMaterial,
                    Path.Combine(outputDir, filePrefix + "_Emission.png"),
                    bakeWidth,
                    bakeHeight,
                    true);
            }

            AssetDatabase.SaveAssets();
            EditorUtility.DisplayDialog("Bake Complete", "Material maps saved successfully.", "OK");
        }

        private void ExportBaseColorMap(
            Material bakeMaterial,
            string outputPath,
            int bakeWidth,
            int bakeHeight,
            bool useSrgb)
        {
            var texture = bakeMaterial != null && bakeMaterial.HasProperty(baseColorProperty)
                ? bakeMaterial.GetTexture(baseColorProperty) as Texture2D
                : null;
            var tint = bakeMaterial != null && bakeMaterial.HasProperty(baseColorTintProperty)
                ? bakeMaterial.GetColor(baseColorTintProperty)
                : Color.white;

            if (texture != null)
            {
                SaveTextureCopy(texture, outputPath, tint, useSrgb);
                return;
            }

            BakeMaterialToTexture(bakeMaterial, bakeWidth, bakeHeight, useSrgb, outputPath);
        }

        private static void ExportMaterialTextureOrFallback(
            Material bakeMaterial,
            string propertyName,
            string outputPath,
            int bakeWidth,
            int bakeHeight,
            Color fallbackColor,
            bool useSrgb)
        {
            var resolved = ResolveProperty(bakeMaterial, propertyName);
            var texture = resolved != null
                ? bakeMaterial.GetTexture(resolved) as Texture2D
                : null;

            if (texture != null)
            {
                SaveTextureCopy(texture, outputPath, Color.white, useSrgb);
                return;
            }

            var tex = new Texture2D(bakeWidth, bakeHeight, TextureFormat.RGBA32, false, !useSrgb);
            var pixels = tex.GetPixels();
            for (var i = 0; i < pixels.Length; i++)
            {
                pixels[i] = fallbackColor;
            }

            tex.SetPixels(pixels);
            tex.Apply();

            var pngData = tex.EncodeToPNG();
            File.WriteAllBytes(outputPath, pngData);
            AssetDatabase.ImportAsset(outputPath);
            DestroyImmediate(tex);
        }

        private static void SaveTextureCopy(Texture2D source, string outputPath)
        {
            SaveTextureCopy(source, outputPath, Color.white, true);
        }

        private static void SaveTextureCopy(Texture2D source, string outputPath, Color tint, bool useSrgb)
        {
            if (source == null)
            {
                return;
            }

            var readable = source.isReadable ? source : DuplicateReadableTexture(source);
            if (readable == null)
            {
                EditorUtility.DisplayDialog("Texture Not Readable",
                    $"Unable to read texture {source.name}. Please enable Read/Write.", "OK");
                return;
            }

            var tinted = ApplyTint(readable, tint, useSrgb);
            var pngData = tinted.EncodeToPNG();
            File.WriteAllBytes(outputPath, pngData);
            AssetDatabase.ImportAsset(outputPath);
            if (tinted != readable)
            {
                DestroyImmediate(tinted);
            }

            if (readable != source && readable != tinted)
            {
                DestroyImmediate(readable);
            }
        }

        private static Texture2D DuplicateReadableTexture(Texture2D source)
        {
            var rt = RenderTexture.GetTemporary(source.width, source.height, 0, RenderTextureFormat.ARGB32);
            var prevRt = RenderTexture.active;
            Graphics.Blit(source, rt);
            RenderTexture.active = rt;

            var readable = new Texture2D(source.width, source.height, TextureFormat.RGBA32, false, false);
            readable.ReadPixels(new Rect(0, 0, source.width, source.height), 0, 0);
            readable.Apply();

            RenderTexture.active = prevRt;
            RenderTexture.ReleaseTemporary(rt);
            return readable;
        }

        private static Texture2D ApplyTint(Texture2D source, Color tint, bool useSrgb)
        {
            if (tint == Color.white)
            {
                return source;
            }

            var tex = new Texture2D(source.width, source.height, TextureFormat.RGBA32, false, !useSrgb);
            var pixels = source.GetPixels();
            for (var i = 0; i < pixels.Length; i++)
            {
                pixels[i] *= tint;
            }

            tex.SetPixels(pixels);
            tex.Apply();
            return tex;
        }

        private Color GetMetallicFallback(Material bakeMaterial)
        {
            var resolved = ResolveProperty(bakeMaterial, metallicValueProperty);
            if (bakeMaterial != null && resolved != null)
            {
                var value = bakeMaterial.GetFloat(resolved);
                return new Color(value, value, value, 1f);
            }

            return Color.black;
        }

        private void ExportSmoothnessMap(
            Material bakeMaterial,
            string outputPath,
            int bakeWidth,
            int bakeHeight)
        {
            var resolved = ResolveProperty(bakeMaterial, smoothnessProperty);
            if (bakeMaterial != null && resolved != null)
            {
                var value = bakeMaterial.GetFloat(resolved);
                ExportSolidColorTexture(outputPath, bakeWidth, bakeHeight, new Color(value, value, value, 1f), false);
                return;
            }

            ExportSolidColorTexture(outputPath, bakeWidth, bakeHeight, Color.black, false);
        }

        private void ExportEmissionMap(
            Material bakeMaterial,
            string outputPath,
            int bakeWidth,
            int bakeHeight,
            bool useSrgb)
        {
            var resolvedTexture = ResolveProperty(bakeMaterial, emissionProperty);
            var texture = resolvedTexture != null
                ? bakeMaterial.GetTexture(resolvedTexture) as Texture2D
                : null;
            var resolvedColor = ResolveProperty(bakeMaterial, emissionColorProperty);
            var color = resolvedColor != null
                ? bakeMaterial.GetColor(resolvedColor)
                : Color.black;

            if (texture != null)
            {
                SaveTextureCopy(texture, outputPath, color, useSrgb);
                return;
            }

            ExportSolidColorTexture(outputPath, bakeWidth, bakeHeight, color, useSrgb);
        }

        private static void ExportSolidColorTexture(
            string outputPath,
            int bakeWidth,
            int bakeHeight,
            Color color,
            bool useSrgb)
        {
            var tex = new Texture2D(bakeWidth, bakeHeight, TextureFormat.RGBA32, false, !useSrgb);
            var pixels = tex.GetPixels();
            for (var i = 0; i < pixels.Length; i++)
            {
                pixels[i] = color;
            }

            tex.SetPixels(pixels);
            tex.Apply();

            var pngData = tex.EncodeToPNG();
            File.WriteAllBytes(outputPath, pngData);
            AssetDatabase.ImportAsset(outputPath);
            DestroyImmediate(tex);
        }

        private static string ResolveProperty(Material bakeMaterial, string primary)
        {
            if (bakeMaterial == null || string.IsNullOrWhiteSpace(primary))
            {
                return null;
            }

            return bakeMaterial.HasProperty(primary) ? primary : null;
        }

        private void AutoDetectProperties()
        {
            baseColorProperty = DetectProperty(baseColorProperty, "_BaseMap", "_MainTex", "_BaseColorMap");
            baseColorTintProperty = DetectProperty(baseColorTintProperty, "_BaseColor", "_Color");
            normalProperty = DetectProperty(normalProperty, "_BumpMap", "_NormalMap");
            metallicProperty = DetectProperty(metallicProperty, "_MetallicGlossMap", "_MetallicMap");
            metallicValueProperty = DetectProperty(metallicValueProperty, "_Metallic");
            smoothnessProperty = DetectProperty(smoothnessProperty, "_Smoothness", "_Glossiness");
            occlusionProperty = DetectProperty(occlusionProperty, "_OcclusionMap");
            emissionProperty = DetectProperty(emissionProperty, "_EmissionMap");
            emissionColorProperty = DetectProperty(emissionColorProperty, "_EmissionColor");
        }

        private string DetectProperty(string current, params string[] candidates)
        {
            if (material == null)
            {
                return current;
            }

            foreach (var candidate in candidates)
            {
                if (material.HasProperty(candidate))
                {
                    return candidate;
                }
            }

            return current;
        }

        // ======================================================================
        // [추가] Exporter(배치 작업)에서 호출하기 위한 Public API (UI/팝업 없이 동작)
        // - BakeSh.cs의 기존 인스턴스 메서드/필드를 그대로 재사용한다.
        // - static 메서드에서 임시 인스턴스를 생성해 CS0120 문제를 해결한다.
        // ======================================================================
        public struct BakedPbrPaths
        {
            public string Albedo;
            public string Normal;
            public string Metallic;
            public string Smoothness;
            public string Occlusion;
            public string Emission;
        }

        public struct BakeOptions
        {
            public bool AutoDetectProperties; // URP/Standard 프로퍼티 자동 감지
            public bool ExportSmoothness;
            public bool ExportOcclusion;
            public bool ExportEmission;
            public bool UseUniquePath;        // 파일 충돌 시 유니크 경로 생성
        }

        /// <summary>
        /// [추가] 씬 익스포터가 호출하는 엔트리 포인트.
        /// EditorWindow를 띄우지 않고도 BakeSh의 인스턴스 로직을 재사용할 수 있게 한다.
        /// </summary>
        public static BakedPbrPaths BakeMaterialMapsForExporter(
            Material bakeMaterial,
            int bakeWidth,
            int bakeHeight,
            bool bakeSRGB,
            string outputDir,
            string filePrefix,
            BakeOptions opt)
        {
            if (bakeMaterial == null)
            {
                Debug.LogError("BakeMaterialMapsForExporter: bakeMaterial is null");
                return default;
            }
            if (string.IsNullOrWhiteSpace(outputDir))
            {
                Debug.LogError("BakeMaterialMapsForExporter: outputDir is null/empty");
                return default;
            }
            if (!AssetDatabase.IsValidFolder(outputDir))
            {
                Debug.LogError($"BakeMaterialMapsForExporter: outputDir is not a valid Unity folder: {outputDir}");
                return default;
            }

            // [핵심 수정] static 컨텍스트에서 인스턴스 멤버 접근을 위해 임시 인스턴스 생성
            var inst = CreateInstance<ShaderGraphBakeWindow>();
            try
            {
                // 인스턴스 필드 세팅(자동감지 필요)
                inst.material = bakeMaterial;
                inst.width = bakeWidth;
                inst.height = bakeHeight;
                inst.useSRGB = bakeSRGB;
                inst.outputFolder = outputDir;
                inst.outputName = filePrefix;

                inst.exportSmoothness = opt.ExportSmoothness;
                inst.exportOcclusion = opt.ExportOcclusion;
                inst.exportEmission = opt.ExportEmission;

                if (opt.AutoDetectProperties)
                    inst.AutoDetectProperties();

                // [추가] 팝업 없이 동작하는 베이크 실행
                return inst.BakeMaterialMaps_NoDialog(
                    bakeMaterial,
                    bakeWidth,
                    bakeHeight,
                    bakeSRGB,
                    outputDir,
                    filePrefix,
                    opt.UseUniquePath);
            }
            finally
            {
                DestroyImmediate(inst);
            }
        }

        /// <summary>
        /// [추가] 기존 BakeMaterialMaps의 배치용 버전(팝업/Selection 변경 없음)
        /// </summary>
        private BakedPbrPaths BakeMaterialMaps_NoDialog(
            Material bakeMaterial,
            int bakeWidth,
            int bakeHeight,
            bool bakeSRGB,
            string outputDir,
            string filePrefix,
            bool useUniquePath)
        {
            var result = new BakedPbrPaths();

            string MakePath(string suffix)
            {
                var raw = Path.Combine(outputDir, filePrefix + suffix);
                return useUniquePath ? AssetDatabase.GenerateUniqueAssetPath(raw) : raw;
            }

            // BaseColor(Albedo)
            result.Albedo = MakePath("_Albedo.png");
            ExportBaseColorMap(bakeMaterial, result.Albedo, bakeWidth, bakeHeight, bakeSRGB);

            // Normal
            result.Normal = MakePath("_Normal.png");
            ExportMaterialTextureOrFallback(
                bakeMaterial,
                normalProperty, // 인스턴스 필드
                result.Normal,
                bakeWidth,
                bakeHeight,
                new Color(0.5f, 0.5f, 1f, 1f),
                false);

            // Metallic
            result.Metallic = MakePath("_Metallic.png");
            ExportMaterialTextureOrFallback(
                bakeMaterial,
                metallicProperty, // 인스턴스 필드
                result.Metallic,
                bakeWidth,
                bakeHeight,
                GetMetallicFallback(bakeMaterial), // 인스턴스 메서드
                false);

            // Smoothness(원본 BakeSh 구현이 Smoothness 기반이므로 그대로 출력)
            if (exportSmoothness)
            {
                result.Smoothness = MakePath("_Smoothness.png");
                ExportSmoothnessMap(bakeMaterial, result.Smoothness, bakeWidth, bakeHeight);
            }

            // Occlusion
            if (exportOcclusion)
            {
                result.Occlusion = MakePath("_Occlusion.png");
                ExportMaterialTextureOrFallback(
                    bakeMaterial,
                    occlusionProperty, // 인스턴스 필드
                    result.Occlusion,
                    bakeWidth,
                    bakeHeight,
                    Color.white,
                    false);
            }

            // Emission
            if (exportEmission)
            {
                result.Emission = MakePath("_Emission.png");
                ExportEmissionMap(bakeMaterial, result.Emission, bakeWidth, bakeHeight, true);
            }

            AssetDatabase.SaveAssets();
            return result;
        }

    }
}



#endif
