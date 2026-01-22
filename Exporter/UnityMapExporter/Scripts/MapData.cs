using System;
using System.Collections.Generic;
using UnityEngine;

//////////////////////////////////////
// Heightmap / Alphamap 직렬화 용
//////////////////////////////////////
[Serializable]
public class FloatArray1D
{
    public float[] data;
    public int width;
    public int height;
}

[Serializable]
public class FloatArray3D
{
    public float[] data;
    public int width;
    public int height;
    public int depth;
}

//////////////////////////////////////
// Tree Info
//////////////////////////////////////
[Serializable]
public class TreeJson
{
    public Vector3 position;
    public float heightScale;
    public float widthScale;
    public int prototypeId;
}

//////////////////////////////////////
// Detail Layer (잔디/부쉬)
//////////////////////////////////////
[Serializable]
public class DetailLayerJson
{
    public int[] data;  // int[,] flatten
    public int width;
    public int height;
}

//////////////////////////////////////
// Terrain 전체 정보 (기존 그대로)
//////////////////////////////////////
[Serializable]
public class TerrainFullInfo
{
    public string name;
    public Vector3 position;
    public Vector3 size;

    public int heightmapResolution;
    public FloatArray1D heights;

    public int alphamapWidth;
    public int alphamapHeight;
    public FloatArray3D alphamaps;

    public List<DetailLayerJson> detailLayers;
    public List<TreeJson> trees;
}

//////////////////////////////////////////////////////////////
// [추가] 요청하신 포맷을 위한 참조 타입
// - parent/childs/components가 아래 형태로 나가도록:
//   { "type": "GameObject", "value": "GlobalObjectId_V1-..." }
//////////////////////////////////////////////////////////////
[Serializable]
public class ObjectRef
{
    public string type;
    public string value;
}

[Serializable]
public class ComponentRef
{
    public string type;
    public string value;
}

//////////////////////////////////////////////////////////////
// [추가] 오브젝트 export 데이터 (요청 포맷)
// - 기존 MapObjectData(prefabId/pos/rot/scale) 대신
//   GameObject 메타 + parent/childs + components 구조로 export
//////////////////////////////////////////////////////////////
[Serializable]
public class ExportedGameObject
{
    public string type; // "GameObject"
    public string guid; // "GlobalObjectId_V1-..."
    public string name;

    public bool isActiveSelf;
    public bool isActiveInHierarchy;
    public bool isStatic;

    public int layer;
    public string tag;
    public string hideFlags;    // "None" 등 문자열
    public bool isPersistent;

    public ObjectRef parent;             // null 가능
    public List<ObjectRef> childs = new List<ObjectRef>();
    public List<ComponentRef> components = new List<ComponentRef>();
}

//////////////////////////////////////
// 최종 Export 구조
//////////////////////////////////////
[Serializable]
public class FinalMapJson
{
    public TerrainFullInfo terrain;

    // [수정] 기존 List<MapObjectData> objects -> List<ExportedGameObject> objects
    //        "오브젝트만" 요청 포맷으로 바꾸기 위함
    public List<ExportedGameObject> objects = new List<ExportedGameObject>();
}
