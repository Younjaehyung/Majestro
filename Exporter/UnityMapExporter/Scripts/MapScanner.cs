#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;
using System.Collections.Generic;

// [추가] Unity 버전별 GlobalObjectId API 시그니처 차이를 흡수하기 위한 Reflection
using System;
using System.Reflection;

public static class MapScanner
{
    //////////////////////////////////////////////////////
    // 1. 2D/3D 배열을 1D로 변환 (JsonUtility 제한 해결)
    //    ※ Terrain 쪽은 "그대로" 두라고 하셨으므로 기존 주석/동작 유지
    //////////////////////////////////////////////////////
    private static FloatArray1D Flatten2D(float[,] src)
    {
        int w = src.GetLength(1);
        int h = src.GetLength(0);

        FloatArray1D result = new FloatArray1D();
        result.width = w;
        result.height = h;

        // 기존 코드 그대로: data는 비워둔 상태(주석 처리 유지)
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
    // 2. Terrain 세부 정보 추출 (기존 그대로)
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
        //info.alphamaps = Flatten3D(alpha3D);

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

            // 기존 코드 그대로: data는 비워둔 상태(주석 처리 유지)
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
    // [수정 핵심] GlobalObjectId 문자열 얻기 (CS1501 해결)
    //
    // 문제 원인:
    // - Unity 버전에 따라 GlobalObjectId.GetGlobalObjectIdSlow 시그니처가 달라
    //   (Object, out GlobalObjectId) 오버로드가 없는 버전에서 CS1501 발생
    //
    // 해결:
    // - Reflection으로 현재 Unity에 존재하는 시그니처를 찾아 호출
    //   1) GetGlobalObjectIdSlow(Object) -> GlobalObjectId
    //   2) GetGlobalObjectIdSlow(int)    -> GlobalObjectId
    //   3) (구버전) GetGlobalObjectIdSlow(Object, out GlobalObjectId)
    //////////////////////////////////////////////////////
    private static string GetGlobalIdString(UnityEngine.Object obj)
    {
        if (obj == null)
            return string.Empty;

        Type t = typeof(GlobalObjectId);

        // 1) GetGlobalObjectIdSlow(Object) : GlobalObjectId
        MethodInfo mObj = t.GetMethod(
            "GetGlobalObjectIdSlow",
            BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static,
            null,
            new[] { typeof(UnityEngine.Object) },
            null);

        if (mObj != null && mObj.ReturnType == typeof(GlobalObjectId))
        {
            GlobalObjectId gid = (GlobalObjectId)mObj.Invoke(null, new object[] { obj });
            return gid.ToString();
        }

        // 2) GetGlobalObjectIdSlow(int) : GlobalObjectId
        MethodInfo mInt = t.GetMethod(
            "GetGlobalObjectIdSlow",
            BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static,
            null,
            new[] { typeof(int) },
            null);

        if (mInt != null && mInt.ReturnType == typeof(GlobalObjectId))
        {
            GlobalObjectId gid = (GlobalObjectId)mInt.Invoke(null, new object[] { obj.GetInstanceID() });
            return gid.ToString();
        }

        // 3) (구버전) GetGlobalObjectIdSlow(Object, out GlobalObjectId)
        MethodInfo mOut = t.GetMethod(
            "GetGlobalObjectIdSlow",
            BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static,
            null,
            new[] { typeof(UnityEngine.Object), typeof(GlobalObjectId).MakeByRefType() },
            null);

        if (mOut != null)
        {
            object[] args = new object[] { obj, default(GlobalObjectId) };
            mOut.Invoke(null, args);
            return ((GlobalObjectId)args[1]).ToString();
        }

        // 현재 Unity에서 해당 API를 찾지 못한 경우
        return string.Empty;
    }

    //////////////////////////////////////////////////////
    // 3. MapRoot 기반 오브젝트 스캔
    //
    // [수정] 기존 반환 타입: List<MapObjectData>
    //        변경 반환 타입: List<ExportedGameObject>
    //
    // 요구 포맷:
    // "type": "GameObject",
    // "guid": "GlobalObjectId_V1-...",
    // "parent": { "type":"GameObject","value":"..." },
    // "childs": [ { "type":"GameObject","value":"..." } ],
    // "components": [ { "type":"Transform","value":"..." } ]
    //////////////////////////////////////////////////////
    public static List<ExportedGameObject> ScanObjects(string rootName = "MapRoot")
    {
        GameObject rootGO = GameObject.Find(rootName);
        if (rootGO == null)
        {
            Debug.LogError($"MapScanner: '{rootName}' GameObject를 찾을 수 없습니다.");
            return null;
        }

        List<ExportedGameObject> list = new List<ExportedGameObject>();

        // [수정] true로 주면 비활성 오브젝트까지 포함되어
        // isActiveSelf / isActiveInHierarchy를 의미 있게 기록 가능
        Transform[] all = rootGO.GetComponentsInChildren<Transform>(true);

        foreach (Transform t in all)
        {
            // 요구사항: 오브젝트만. MapRoot 자체는 제외
            if (t == rootGO.transform)
                continue;

            GameObject go = t.gameObject;

            ExportedGameObject obj = new ExportedGameObject();
            obj.type = "GameObject";
            obj.guid = GetGlobalIdString(go);
            obj.name = go.name;

            obj.isActiveSelf = go.activeSelf;
            obj.isActiveInHierarchy = go.activeInHierarchy;
            obj.isStatic = go.isStatic;

            obj.layer = go.layer;
            obj.tag = go.tag;
            obj.hideFlags = go.hideFlags.ToString();
            obj.isPersistent = EditorUtility.IsPersistent(go);

            // parent
            // [설계] MapRoot는 export에서 제외하므로,
            // MapRoot 직계 자식은 parent=null로 두는 것이 downstream 처리에 안전함
            if (t.parent != null && t.parent != rootGO.transform)
            {
                obj.parent = new ObjectRef
                {
                    type = "GameObject",
                    value = GetGlobalIdString(t.parent.gameObject)
                };
            }
            else
            {
                obj.parent = null;
            }

            // childs
            obj.childs = new List<ObjectRef>();
            for (int i = 0; i < t.childCount; i++)
            {
                Transform c = t.GetChild(i);
                obj.childs.Add(new ObjectRef
                {
                    type = "GameObject",
                    value = GetGlobalIdString(c.gameObject)
                });
            }

            // components
            // 요구 예시에 Transform만 있어서 기본으로 Transform만 넣음.
            // 필요하면 MeshRenderer/Collider/Animator 등도 동일 패턴으로 확장 가능.
            obj.components = new List<ComponentRef>();

            obj.components.Add(new ComponentRef
            {
                type = "Transform",
                value = GetGlobalIdString(go.transform)
            });

            list.Add(obj);
        }

        return list;
    }

    //////////////////////////////////////////////////////
    // 4. Terrain + Objects 통합 JSON 생산
    //////////////////////////////////////////////////////
    public static FinalMapJson BuildFinal(string rootName = "MapRoot")
    {
        FinalMapJson final = new FinalMapJson();

        final.terrain = ExportTerrainFull();

        // [수정] objects가 이제 ExportedGameObject 리스트로 들어감
        final.objects = ScanObjects(rootName);

        return final;
    }
}
#endif
