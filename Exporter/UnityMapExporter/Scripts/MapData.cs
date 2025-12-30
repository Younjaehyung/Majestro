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
// Terrain 전체 정보
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

//////////////////////////////////////
// Prefab 오브젝트
//////////////////////////////////////
[Serializable]
public class MapObjectData
{
    public string prefabId;
    public Vector3 position;
    public Quaternion rotation;
    public Vector3 scale;
}

//////////////////////////////////////
// 최종 Export 구조
//////////////////////////////////////
[Serializable]
public class FinalMapJson
{
    public TerrainFullInfo terrain;
    public List<MapObjectData> objects = new List<MapObjectData>();
}
