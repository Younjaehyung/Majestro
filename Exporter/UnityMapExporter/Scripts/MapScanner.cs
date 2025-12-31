#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;
using System.Collections.Generic;

public static class MapScanner
{
    //////////////////////////////////////////////////////
    // 1. 2D/3D 배열을 1D로 변환 (JsonUtility 제한 해결)
    //////////////////////////////////////////////////////
    private static FloatArray1D Flatten2D(float[,] src)
    {
        int w = src.GetLength(1);
        int h = src.GetLength(0);

        FloatArray1D result = new FloatArray1D();
        result.width = w;
        result.height = h;

        //result.data = new float[w * h];

        //int idx = 0;
        //for (int y = 0; y < h; y++)
        //{
        //    for (int x = 0; x < w; x++)
        //    {
        //        result.data[idx++] = src[y, x];
        //    }
        //}

        return result;
    }

    private static FloatArray3D Flatten3D(float[,,] src)
    {
        int h = src.GetLength(0);
        int w = src.GetLength(1);
        int d = src.GetLength(2);

        FloatArray3D result = new FloatArray3D();
        result.width = w;
        result.height = h;
        result.depth = d;
        result.data = new float[w * h * d];

        int idx = 0;
        for (int z = 0; z < d; z++)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    result.data[idx++] = src[y, x, z];

        return result;
    }

    //////////////////////////////////////////////////////
    // 2. Terrain 세부 정보 추출
    //////////////////////////////////////////////////////
    public static TerrainFullInfo ExportTerrainFull()
    {
        Terrain terrain = GameObject.FindObjectOfType<Terrain>();
        if (terrain == null)
        {
            Debug.LogWarning("Terrain을 찾지 못했습니다.");
            return null;
        }

        TerrainData td = terrain.terrainData;

        TerrainFullInfo info = new TerrainFullInfo();
        info.name = terrain.name;
        info.position = terrain.transform.position;
        info.size = td.size;

        // Heightmap
        int hm = td.heightmapResolution;
        info.heightmapResolution = hm;
        float[,] height2D = td.GetHeights(0, 0, hm, hm);
        info.heights = Flatten2D(height2D);

        // Alphamap (Splatmap)
        int aw = td.alphamapWidth;
        int ah = td.alphamapHeight;
        float[,,] alpha3D = td.GetAlphamaps(0, 0, aw, ah);
        info.alphamapWidth = aw;
        info.alphamapHeight = ah;
        info.alphamaps = Flatten3D(alpha3D);

        // Detail layers
        info.detailLayers = new List<DetailLayerJson>();
        int detailLayerCount = td.detailPrototypes.Length;

        for (int layer = 0; layer < detailLayerCount; layer++)
        {
            int dw = td.detailWidth;
            int dh = td.detailHeight;
            int[,] detail = td.GetDetailLayer(0, 0, dw, dh, layer);

            DetailLayerJson dlj = new DetailLayerJson();
            dlj.width = dw;
            dlj.height = dh;
            //dlj.data = new int[dw * dh];

            //int idx = 0;
            //for (int y = 0; y < dh; y++)
            //    for (int x = 0; x < dw; x++)
            //        dlj.data[idx++] = detail[y, x];

            info.detailLayers.Add(dlj);
        }

        // Trees
        info.trees = new List<TreeJson>();
        foreach (var t in td.treeInstances)
        {
            info.trees.Add(new TreeJson
            {
                position = new Vector3(t.position.x, t.position.y, t.position.z),
                heightScale = t.heightScale,
                widthScale = t.widthScale,
                prototypeId = t.prototypeIndex
            });
        }

        return info;
    }

    //////////////////////////////////////////////////////
    // 3. MapRoot 기반 Prefab 오브젝트 스캔
    //////////////////////////////////////////////////////
    public static List<MapObjectData> ScanObjects(string rootName = "MapRoot")
    {
        GameObject rootGO = GameObject.Find(rootName);
        if (rootGO == null)
        {
            Debug.LogError($"MapScanner: '{rootName}' GameObject를 찾을 수 없습니다.");
            return null;
        }

        List<MapObjectData> list = new List<MapObjectData>();
        Transform[] all = rootGO.GetComponentsInChildren<Transform>();

        foreach (Transform t in all)
        {
            if (t == rootGO.transform)
                continue;

            GameObject go = t.gameObject;
            GameObject prefab = PrefabUtility.GetCorrespondingObjectFromSource(go);
            string id = prefab != null ? prefab.name : go.name;

            list.Add(new MapObjectData
            {
                prefabId = id,
                position = t.position,
                rotation = t.rotation,
                scale = t.localScale
            });
        }

        return list;
    }

    //////////////////////////////////////////////////////
    // 4. Terrain + Prefab 오브젝트 통합 JSON 생산
    //////////////////////////////////////////////////////
    public static FinalMapJson BuildFinal(string rootName = "MapRoot")
    {
        FinalMapJson final = new FinalMapJson();

        final.terrain = ExportTerrainFull();
        final.objects = ScanObjects(rootName);

        return final;
    }
}
#endif
