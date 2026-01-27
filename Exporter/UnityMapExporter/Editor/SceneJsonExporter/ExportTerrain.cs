// Assets/Editor/TerrainHeightmapExporter.cs
// Unity Editor 전용: Terrain heightmap을 PNG 16-bit Grayscale + EXR로 export
// - PNG16: 직접 PNG 포맷(Grayscale, 16-bit) 생성 (Unity EncodeToPNG 의 16bit 보존 불확실성 회피)
// - EXR : Unity 내장 EncodeToEXR 사용 (권장)

#if UNITY_EDITOR
using UnityEditor;
using UnityEngine;
using System;
using System.IO;

public class TerrainHeightmapExporter : EditorWindow
{
    private Terrain _terrain;

    private bool _flipY = true;                 // 이미지 좌표계(top-left) 맞추기용 (보통 true 권장)
    private bool _exportPNG16 = true;
    private bool _exportEXR = true;

    private enum ExrStoreMode { Normalized01, WorldUnits } // EXR에 저장할 값 선택
    private ExrStoreMode _exrMode = ExrStoreMode.Normalized01;

    private string _fileBaseName = "Heightmap";
    private string _lastFolder = "Assets";

    [MenuItem("Tools/Terrain/Export Heightmap (PNG16, EXR)")]
    public static void Open()
    {
        var w = GetWindow<TerrainHeightmapExporter>("Terrain Height Export");
        w.minSize = new Vector2(520, 260);
        w.TryAutoPickTerrain();
    }

    private void TryAutoPickTerrain()
    {
        if (Selection.activeGameObject != null)
        {
            var t = Selection.activeGameObject.GetComponent<Terrain>();
            if (t != null) _terrain = t;
        }
        if (_terrain == null && Terrain.activeTerrain != null)
            _terrain = Terrain.activeTerrain;
    }

    private void OnGUI()
    {
        EditorGUILayout.LabelField("Terrain Heightmap Exporter", EditorStyles.boldLabel);
        EditorGUILayout.Space(6);

        _terrain = (Terrain)EditorGUILayout.ObjectField("Target Terrain", _terrain, typeof(Terrain), true);

        EditorGUILayout.Space(6);
        _fileBaseName = EditorGUILayout.TextField("File Base Name", _fileBaseName);
        _flipY = EditorGUILayout.ToggleLeft("Flip Y (Top-Left origin)", _flipY);

        EditorGUILayout.Space(6);
        _exportPNG16 = EditorGUILayout.ToggleLeft("Export PNG 16-bit Grayscale", _exportPNG16);
        _exportEXR = EditorGUILayout.ToggleLeft("Export EXR (R channel)", _exportEXR);

        using (new EditorGUI.DisabledScope(!_exportEXR))
        {
            _exrMode = (ExrStoreMode)EditorGUILayout.EnumPopup("EXR Store Mode", _exrMode);
            EditorGUILayout.HelpBox(
                "Normalized01: 0~1 그대로 저장\n" +
                "WorldUnits: (0~1) * Terrain Height(size.y) 를 저장 (DX에서 바로 월드높이로 쓰기 편함)",
                MessageType.Info);
        }

        EditorGUILayout.Space(10);

        using (new EditorGUI.DisabledScope(_terrain == null || (!_exportPNG16 && !_exportEXR)))
        {
            if (GUILayout.Button("Export...", GUILayout.Height(34)))
            {
                Export();
            }
        }

        EditorGUILayout.Space(6);
        EditorGUILayout.HelpBox(
            "주의: Heightmap 해상도는 TerrainData.heightmapResolution 기준이며 보통 (N x N)입니다.\n" +
            "PNG16은 R16_UNORM 같은 방식으로 DX12에서 바로 쓰기 좋습니다.",
            MessageType.None);
    }

    private void Export()
    {
        if (_terrain == null)
        {
            Debug.LogError("Terrain이 지정되지 않았습니다.");
            return;
        }

        var td = _terrain.terrainData;
        if (td == null)
        {
            Debug.LogError("TerrainData가 null 입니다.");
            return;
        }

        int res = td.heightmapResolution; // 보통 NxN
        // Unity GetHeights는 (height, width) = (res, res) 형태로 반환됨 (y, x 인덱스)
        float[,] heights = td.GetHeights(0, 0, res, res);

        // 저장 폴더 선택 (프로젝트 내부 권장)
        string folder = EditorUtility.OpenFolderPanel("Select Export Folder", _lastFolder, "");
        if (string.IsNullOrEmpty(folder))
            return;

        // 프로젝트 내부면 Assets 상대경로 갱신용
        _lastFolder = folder;

        // 파일명
        string baseName = string.IsNullOrWhiteSpace(_fileBaseName) ? "Heightmap" : _fileBaseName;

        try
        {
            if (_exportPNG16)
            {
                string pathPng = Path.Combine(folder, baseName + "_R16.png");
                SavePng16Grayscale(pathPng, heights, res, res, _flipY);
                Debug.Log($"PNG16 saved: {pathPng}");
            }

            if (_exportEXR)
            {
                string pathExr = Path.Combine(folder, baseName + "_R.exr");
                SaveExrR(pathExr, heights, td.size.y, res, res, _flipY, _exrMode);
                Debug.Log($"EXR saved: {pathExr}");
            }

            AssetDatabase.Refresh();
            EditorUtility.DisplayDialog("Export Complete", "Terrain height export finished.", "OK");
        }
        catch (Exception e)
        {
            Debug.LogException(e);
            EditorUtility.DisplayDialog("Export Failed", e.Message, "OK");
        }
    }

    // =========================
    // EXR 저장 (Unity 내장)
    // =========================
    private static void SaveExrR(string path, float[,] heights, float terrainHeightWorld, int width, int height, bool flipY, ExrStoreMode mode)
    {
        // EXR은 채널이 필요하므로 RGBAHalf로 만들고 R에만 height를 넣는다.
        // linear = true 로 설정해 감마 영향을 배제
        var tex = new Texture2D(width, height, TextureFormat.RGBAHalf, false, true);

        var cols = new Color[width * height];

        for (int y = 0; y < height; ++y)
        {
            int sy = flipY ? (height - 1 - y) : y;

            for (int x = 0; x < width; ++x)
            {
                float h01 = heights[sy, x]; // 0..1
                float v = (mode == ExrStoreMode.WorldUnits) ? (h01 * terrainHeightWorld) : h01;
                cols[y * width + x] = new Color(v, 0f, 0f, 1f);
            }
        }

        tex.SetPixels(cols);
        tex.Apply(false, false);

        // ZIP 압축 사용. half 저장(기본)로 충분히 정밀도 유지됨.
        byte[] exr = tex.EncodeToEXR(Texture2D.EXRFlags.CompressZIP);
        File.WriteAllBytes(path, exr);

        UnityEngine.Object.DestroyImmediate(tex);
    }

    // =========================
    // PNG 16-bit Grayscale 저장 (직접 PNG 생성)
    // - ColorType: 0 (grayscale)
    // - BitDepth : 16
    // - Scanline filter: 0 (None)
    // - zlib: "무압축 Deflate 블록"으로 구성(용량은 다소 큼, 대신 Unity/런타임 의존성 최소)
    // =========================
    private static void SavePng16Grayscale(string path, float[,] heights, int width, int height, bool flipY)
    {
        // PNG는 16-bit 샘플이 "Big Endian"으로 저장됨
        // 각 scanline 시작에 filter byte(0)를 추가해야 함
        int bytesPerPixel = 2; // 16-bit grayscale
        int stride = 1 + width * bytesPerPixel;
        byte[] raw = new byte[stride * height];

        for (int y = 0; y < height; ++y)
        {
            int sy = flipY ? (height - 1 - y) : y;
            int rowStart = y * stride;
            raw[rowStart] = 0; // filter = 0(None)

            int p = rowStart + 1;
            for (int x = 0; x < width; ++x)
            {
                float h01 = Mathf.Clamp01(heights[sy, x]);

                // 0..1 -> 0..65535
                int v = Mathf.RoundToInt(h01 * 65535.0f);
                if (v < 0) v = 0;
                if (v > 65535) v = 65535;

                // Big Endian
                raw[p + 0] = (byte)((v >> 8) & 0xFF);
                raw[p + 1] = (byte)(v & 0xFF);
                p += 2;
            }
        }

        // PNG는 IDAT에 zlib 스트림을 넣어야 함.
        // 여기서는 zlib + "무압축 deflate 블록"을 직접 생성한다.
        byte[] zlib = ZlibNoCompression(raw);

        using (var fs = new FileStream(path, FileMode.Create, FileAccess.Write))
        using (var bw = new BinaryWriter(fs))
        {
            // PNG signature
            bw.Write(new byte[] { 137, 80, 78, 71, 13, 10, 26, 10 });

            // IHDR
            byte[] ihdr = BuildIHDR(width, height, bitDepth: 16, colorType: 0);
            WriteChunk(bw, "IHDR", ihdr);

            // IDAT
            WriteChunk(bw, "IDAT", zlib);

            // IEND
            WriteChunk(bw, "IEND", Array.Empty<byte>());
        }
    }

    private static byte[] BuildIHDR(int w, int h, byte bitDepth, byte colorType)
    {
        // IHDR: width(4) height(4) bitDepth(1) colorType(1) compression(1) filter(1) interlace(1)
        byte[] d = new byte[13];
        WriteBE32(d, 0, (uint)w);
        WriteBE32(d, 4, (uint)h);
        d[8] = bitDepth;
        d[9] = colorType;
        d[10] = 0; // compression method(0)
        d[11] = 0; // filter method(0)
        d[12] = 0; // interlace(0)
        return d;
    }

    private static void WriteChunk(BinaryWriter bw, string type4, byte[] data)
    {
        // length (big-endian)
        WriteBE32(bw, (uint)data.Length);

        // type
        byte[] typeBytes = System.Text.Encoding.ASCII.GetBytes(type4);
        bw.Write(typeBytes);

        // data
        bw.Write(data);

        // crc (type + data)
        uint crc = Crc32(typeBytes, data);
        WriteBE32(bw, crc);
    }

    // =========================
    // zlib(무압축) 생성: [zlib header][deflate blocks][adler32]
    // =========================
    private static byte[] ZlibNoCompression(byte[] input)
    {
        // zlib header: CMF/FLG
        // 0x78 0x01 => deflate + 32K window, fastest/no compression
        // (정확한 FLG 조합 중 하나로 널리 사용)
        using (var ms = new MemoryStream())
        {
            ms.WriteByte(0x78);
            ms.WriteByte(0x01);

            int offset = 0;
            int remaining = input.Length;

            while (remaining > 0)
            {
                int blockLen = Math.Min(65535, remaining);
                bool final = (remaining - blockLen) == 0;

                // Deflate uncompressed block header:
                // BFINAL(1bit) + BTYPE(2bit=00) => 1byte로 정렬해서 작성
                // uncompressed 블록은 다음 바이트 경계부터 LEN/NLEN이 온다.
                byte bfinal = (byte)(final ? 1 : 0);
                ms.WriteByte(bfinal); // 0000000(BTYPE=00) + BFINAL

                // LEN, NLEN (little-endian)
                ushort len = (ushort)blockLen;
                ushort nlen = (ushort)~len;
                ms.WriteByte((byte)(len & 0xFF));
                ms.WriteByte((byte)((len >> 8) & 0xFF));
                ms.WriteByte((byte)(nlen & 0xFF));
                ms.WriteByte((byte)((nlen >> 8) & 0xFF));

                // block data
                ms.Write(input, offset, blockLen);

                offset += blockLen;
                remaining -= blockLen;
            }

            uint adler = Adler32(input);
            // adler32 big-endian
            ms.WriteByte((byte)((adler >> 24) & 0xFF));
            ms.WriteByte((byte)((adler >> 16) & 0xFF));
            ms.WriteByte((byte)((adler >> 8) & 0xFF));
            ms.WriteByte((byte)(adler & 0xFF));

            return ms.ToArray();
        }
    }

    private static uint Adler32(byte[] data)
    {
        const uint MOD = 65521;
        uint a = 1, b = 0;
        for (int i = 0; i < data.Length; ++i)
        {
            a = (a + data[i]) % MOD;
            b = (b + a) % MOD;
        }
        return (b << 16) | a;
    }

    // =========================
    // CRC32 (PNG chunk CRC)
    // =========================
    private static readonly uint[] _crcTable = BuildCrcTable();

    private static uint[] BuildCrcTable()
    {
        uint[] table = new uint[256];
        for (uint i = 0; i < 256; i++)
        {
            uint c = i;
            for (int k = 0; k < 8; k++)
                c = ((c & 1) != 0) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        return table;
    }

    private static uint Crc32(byte[] type, byte[] data)
    {
        uint c = 0xFFFFFFFFu;
        for (int i = 0; i < type.Length; i++)
            c = _crcTable[(c ^ type[i]) & 0xFF] ^ (c >> 8);
        for (int i = 0; i < data.Length; i++)
            c = _crcTable[(c ^ data[i]) & 0xFF] ^ (c >> 8);
        return c ^ 0xFFFFFFFFu;
    }

    // =========================
    // Big-endian write helpers
    // =========================
    private static void WriteBE32(byte[] dst, int offset, uint v)
    {
        dst[offset + 0] = (byte)((v >> 24) & 0xFF);
        dst[offset + 1] = (byte)((v >> 16) & 0xFF);
        dst[offset + 2] = (byte)((v >> 8) & 0xFF);
        dst[offset + 3] = (byte)(v & 0xFF);
    }

    private static void WriteBE32(BinaryWriter bw, uint v)
    {
        bw.Write((byte)((v >> 24) & 0xFF));
        bw.Write((byte)((v >> 16) & 0xFF));
        bw.Write((byte)((v >> 8) & 0xFF));
        bw.Write((byte)(v & 0xFF));
    }
}
#endif
