//#define _WITH_SKINNED_BONES_ANIMATION

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using UnityEditor;
using System.Text;

public class BinaryHierarchicalModelExtract : MonoBehaviour
{
    private List<string> m_pTextureNamesListForCounting = new List<string>();
    private List<string> m_pTextureNamesListForWriting = new List<string>();

    private BinaryWriter binaryWriter = null;

    bool isSkinning = false;

    bool FindTextureByName(List<string> pTextureNamesList, Texture texture)
    {
        if (texture)
        {
            string strTextureName = string.Copy(texture.name).Replace(" ", "_");
            for (int i = 0; i < pTextureNamesList.Count; i++)
            {
                if (pTextureNamesList.Contains(strTextureName)) return (true);
            }
            pTextureNamesList.Add(strTextureName);
            return (false);
        }
        else
        {
            return (true);
        }
    }

    void WriteObjectName(Object obj)
    {
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    void WriteObjectName(int i, Object obj)
    {
        binaryWriter.Write(i);
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    void WriteObjectName(string strHeader, Object obj)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    void WriteObjectName(string strHeader, int i, Object obj)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    void WriteObjectName(string strHeader, int i, int j, Object obj)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
        binaryWriter.Write(j);
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
    }

    void WriteObjectName(string strHeader, int i, Object obj, float f, int j, int k)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
        binaryWriter.Write((obj) ? string.Copy(obj.name).Replace(" ", "_") : "null");
        binaryWriter.Write(f);
        binaryWriter.Write(j);
        binaryWriter.Write(k);
    }

    void WriteString(string strToWrite)
    {
        binaryWriter.Write(strToWrite);
    }

    void WriteString(string strHeader, string strToWrite)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(strToWrite);
    }

    void WriteString(string strToWrite, int i)
    {
        binaryWriter.Write(strToWrite);
        binaryWriter.Write(i);
    }

    void WriteString(string strToWrite, int i, float f)
    {
        binaryWriter.Write(strToWrite);
        binaryWriter.Write(i);
        binaryWriter.Write(f);
    }

    void WriteTextureName(string strHeader, Texture texture)
    {
        binaryWriter.Write(strHeader);
        if (texture)
        {
            if (FindTextureByName(m_pTextureNamesListForWriting, texture))
            {
                binaryWriter.Write("@" + string.Copy(texture.name).Replace(" ", "_"));
            }
            else
            {
                binaryWriter.Write(string.Copy(texture.name).Replace(" ", "_"));
            }
        }
        else
        {
            binaryWriter.Write("null");
        }
    }

    void WriteBool(bool i)
    {
        binaryWriter.Write(i);
    }

    void WriteInteger(int i)
    {
        binaryWriter.Write(i);
    }

    void WriteInteger(string strHeader, int i)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(i);
    }

    void WriteFloat(string strHeader, float f)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(f);
    }

    void WriteVector(Vector2 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
    }

    void WriteVector(string strHeader, Vector2 v)
    {
        binaryWriter.Write(strHeader);
        WriteVector(v);
    }

    void WriteVector(Vector3 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
        binaryWriter.Write(v.z);
    }

    void WriteVector(string strHeader, Vector3 v)
    {
        binaryWriter.Write(strHeader);
        WriteVector(v);
    }

    void WriteVector(Vector4 v)
    {
        binaryWriter.Write(v.x);
        binaryWriter.Write(v.y);
        binaryWriter.Write(v.z);
        binaryWriter.Write(v.w);
    }

    void WriteVector(string strHeader, Vector4 v)
    {
        binaryWriter.Write(strHeader);
        WriteVector(v);
    }

    void WriteVector(Quaternion q)
    {
        binaryWriter.Write(q.x);
        binaryWriter.Write(q.y);
        binaryWriter.Write(q.z);
        binaryWriter.Write(q.w);
    }

    void WriteVector(string strHeader, Quaternion q)
    {
        binaryWriter.Write(strHeader);
        WriteVector(q);
    }

    void WriteColor(Color c)
    {
        binaryWriter.Write(c.r);
        binaryWriter.Write(c.g);
        binaryWriter.Write(c.b);
        binaryWriter.Write(c.a);
    }

    void WriteColor(string strHeader, Color c)
    {
        binaryWriter.Write(strHeader);
        WriteColor(c);
    }

    void WriteTextureCoord(Vector2 uv)
    {
        binaryWriter.Write(uv.x);
        binaryWriter.Write(1.0f - uv.y);
    }

    void WriteVectors(string strHeader, Vector2[] vectors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(vectors.Length);
        if (vectors.Length > 0) foreach (Vector2 v in vectors) WriteVector(v);
    }

    void WriteVectors(string strHeader, Vector3[] vectors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(vectors.Length);
        if (vectors.Length > 0) foreach (Vector3 v in vectors) WriteVector(v);
    }

    void WriteVectors(string strHeader, Vector4[] vectors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(vectors.Length);
        if (vectors.Length > 0) foreach (Vector4 v in vectors) WriteVector(v);
    }

    void WriteColors(string strHeader, Color[] colors)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(colors.Length);
        if (colors.Length > 0) foreach (Color c in colors) WriteColor(c);
    }

    void WriteTextureCoords(string strHeader, Vector2[] uvs)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(uvs.Length);
        if (uvs.Length > 0) foreach (Vector2 uv in uvs) WriteTextureCoord(uv);
    }

    void WriteFloats(string header, float[] arr)
    {
        binaryWriter.Write(header);
        binaryWriter.Write(arr.Length);
        foreach (var v in arr) binaryWriter.Write(v);
    }

    void WriteIntegers(int[] pIntegers)
    {
        binaryWriter.Write(pIntegers.Length);
        foreach (int i in pIntegers) binaryWriter.Write(i);
    }

    void WriteIntegers(string strHeader, int[] pIntegers)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(pIntegers.Length);
        if (pIntegers.Length > 0) foreach (int i in pIntegers) binaryWriter.Write(i);
    }

    void WriteIntegers(string strHeader, int n, int[] pIntegers)
    {
        binaryWriter.Write(strHeader);
        binaryWriter.Write(n);
        binaryWriter.Write(pIntegers.Length);
        if (pIntegers.Length > 0) foreach (int i in pIntegers) binaryWriter.Write(i);
    }

    void WriteBoundingBox(string strHeader, Bounds bounds)
    {
        binaryWriter.Write(strHeader);
        WriteVector(bounds.center);
        WriteVector(bounds.extents);
    }

    void WriteMatrix(Matrix4x4 matrix)
    {
        binaryWriter.Write(matrix.m00);
        binaryWriter.Write(matrix.m10);
        binaryWriter.Write(matrix.m20);
        binaryWriter.Write(matrix.m30);
        binaryWriter.Write(matrix.m01);
        binaryWriter.Write(matrix.m11);
        binaryWriter.Write(matrix.m21);
        binaryWriter.Write(matrix.m31);
        binaryWriter.Write(matrix.m02);
        binaryWriter.Write(matrix.m12);
        binaryWriter.Write(matrix.m22);
        binaryWriter.Write(matrix.m32);
        binaryWriter.Write(matrix.m03);
        binaryWriter.Write(matrix.m13);
        binaryWriter.Write(matrix.m23);
        binaryWriter.Write(matrix.m33);
    }

    void WriteMatrix(Vector3 position, Quaternion rotation, Vector3 scale)
    {
        Matrix4x4 matrix = Matrix4x4.identity;
        matrix.SetTRS(position, rotation, scale);
        WriteMatrix(matrix);
    }

    void WriteTransform(string strHeader, Transform current)
    {
        binaryWriter.Write(strHeader);
        WriteVector(current.localPosition);
        WriteVector(current.localEulerAngles);
        WriteVector(current.localScale);
        WriteVector(current.localRotation);
    }

    void WriteLocalMatrix(string strHeader, Transform current)
    {
        binaryWriter.Write(strHeader);
        Matrix4x4 matrix = Matrix4x4.identity;
        matrix.SetTRS(current.localPosition, current.localRotation, current.localScale);
        WriteMatrix(matrix);
    }

    void WriteWorldMatrix(string strHeader, Transform current)
    {
        binaryWriter.Write(strHeader);
        Matrix4x4 matrix = Matrix4x4.identity;
        matrix.SetTRS(current.position, current.rotation, current.lossyScale);
        WriteMatrix(matrix);
    }

    void WriteMatrixes(string strHeader, Matrix4x4[] matrixes)
    {
        WriteString(strHeader, matrixes.Length);
        if (matrixes.Length > 0)
        {
            foreach (Matrix4x4 matrix in matrixes) WriteMatrix(matrix);
        }
    }

    int GetTexturesCount(Material[] materials)
    {
        int nTextures = 0;
        for (int i = 0; i < materials.Length; i++)
        {
            if (materials[i].HasProperty("_MainTex"))
            {
                if (!FindTextureByName(m_pTextureNamesListForCounting, materials[i].GetTexture("_MainTex"))) nTextures++;
            }
            if (materials[i].HasProperty("_SpecGlossMap"))
            {
                if (!FindTextureByName(m_pTextureNamesListForCounting, materials[i].GetTexture("_SpecGlossMap"))) nTextures++;
            }
            if (materials[i].HasProperty("_MetallicGlossMap"))
            {
                if (!FindTextureByName(m_pTextureNamesListForCounting, materials[i].GetTexture("_MetallicGlossMap"))) nTextures++;
            }
            if (materials[i].HasProperty("_BumpMap"))
            {
                if (!FindTextureByName(m_pTextureNamesListForCounting, materials[i].GetTexture("_BumpMap"))) nTextures++;
            }
            if (materials[i].HasProperty("_EmissionMap"))
            {
                if (!FindTextureByName(m_pTextureNamesListForCounting, materials[i].GetTexture("_EmissionMap"))) nTextures++;
            }
            if (materials[i].HasProperty("_DetailAlbedoMap"))
            {
                if (!FindTextureByName(m_pTextureNamesListForCounting, materials[i].GetTexture("_DetailAlbedoMap"))) nTextures++;
            }
            if (materials[i].HasProperty("_DetailNormalMap"))
            {
                if (!FindTextureByName(m_pTextureNamesListForCounting, materials[i].GetTexture("_DetailNormalMap"))) nTextures++;
            }
        }
        return (nTextures);
    }

    int GetTexturesCount(Transform current)
    {
        int nTextures = 0;
        MeshRenderer meshRenderer = current.gameObject.GetComponent<MeshRenderer>();
        if (meshRenderer) nTextures = GetTexturesCount(meshRenderer.materials);

        for (int k = 0; k < current.childCount; k++) nTextures += GetTexturesCount(current.GetChild(k));

        return (nTextures);
    }

    bool TryGetMesh(Transform t, out Mesh mesh, out Renderer renderer)
    {
        mesh = null;
        renderer = null;

        // SkinnedMesh 우선
        SkinnedMeshRenderer skinned =
            t.GetComponent<SkinnedMeshRenderer>();

        if (skinned && skinned.sharedMesh)
        {
            mesh = skinned.sharedMesh;
            renderer = skinned;
            return true;
        }

        // 일반 Mesh
        MeshFilter mf = t.GetComponent<MeshFilter>();
        MeshRenderer mr = t.GetComponent<MeshRenderer>();

        if (mf && mr && mf.sharedMesh)
        {
            mesh = mf.sharedMesh;
            renderer = mr;
            return true;
        }

        return false;
    }

    void WriteMeshInfo(Mesh mesh, Renderer renderer)
    {
        WriteString("<Mesh>:");

        // --- Header ---
        int materialCount = renderer.sharedMaterials.Length;

        WriteInteger(mesh.vertexCount);
        WriteInteger(mesh.subMeshCount);
        WriteInteger(materialCount);
        WriteBool(isSkinning);

        // --- Geometry ---
        WriteBoundingBox("<Bounds>:", mesh.bounds);
        if ((mesh.vertices != null) && (mesh.vertices.Length > 0)) WriteVectors("<Positions>:", mesh.vertices);
        if ((mesh.colors != null) && (mesh.colors.Length > 0)) WriteColors("<Colors>:", mesh.colors);
        if ((mesh.uv != null) && (mesh.uv.Length > 0)) WriteTextureCoords("<TextureCoords0>:", mesh.uv);
        if ((mesh.uv2 != null) && (mesh.uv2.Length > 0)) WriteTextureCoords("<TextureCoords1>:", mesh.uv2);
        if ((mesh.normals != null) && (mesh.normals.Length > 0)) WriteVectors("<Normals>:", mesh.normals);

        // --- SubMeshes ---
        WriteInteger("<SubMeshes>:", mesh.subMeshCount);
        if (mesh.subMeshCount > 0)
        {
            for (int i = 0; i < mesh.subMeshCount; i++)
            {
                int[] subindicies = mesh.GetTriangles(i);
                WriteIntegers("<SubMesh>:", i, subindicies);
            }
        }

        // --- Materials ---
        WriteMaterials(renderer.sharedMaterials);

        // --- Skinning ---
        if (isSkinning)
        {
            BoneWeight[] weights = mesh.boneWeights;
            WriteInteger("<BoneWeightCount>:", weights.Length);

            for (int i = 0; i < weights.Length; i++)
            {
                BoneWeight bwgt = weights[i];
                int[] idx = { bwgt.boneIndex0, bwgt.boneIndex1, bwgt.boneIndex2, bwgt.boneIndex3 };
                float[] w = { bwgt.weight0, bwgt.weight1, bwgt.weight2, bwgt.weight3 };

                WriteIntegers("<BoneIndex>:", idx);
                WriteFloats("<BoneWeight>:", w);
            }
        }

        WriteString("</Mesh>");
    }

    void WriteMaterials(Material[] materials)
    {
        WriteInteger("<Materials>:", materials.Length);
        for (int i = 0; i < materials.Length; i++)
        {
            WriteInteger("<Material>:", i);

            if (materials[i].HasProperty("_Color"))
            {
                Color albedo = materials[i].GetColor("_Color");
                WriteColor("<AlbedoColor>:", albedo);
            }
            if (materials[i].HasProperty("_EmissionColor"))
            {
                Color emission = materials[i].GetColor("_EmissionColor");
                WriteColor("<EmissiveColor>:", emission);
            }
            if (materials[i].HasProperty("_SpecColor"))
            {
                Color specular = materials[i].GetColor("_SpecColor");
                WriteColor("<SpecularColor>:", specular);
            }
            if (materials[i].HasProperty("_Glossiness"))
            {
                WriteFloat("<Glossiness>:", materials[i].GetFloat("_Glossiness"));
            }
            if (materials[i].HasProperty("_Smoothness"))
            {
                WriteFloat("<Smoothness>:", materials[i].GetFloat("_Smoothness"));
            }
            if (materials[i].HasProperty("_Metallic"))
            {
                WriteFloat("<Metallic>:", materials[i].GetFloat("_Metallic"));
            }
            if (materials[i].HasProperty("_SpecularHighlights"))
            {
                WriteFloat("<SpecularHighlight>:", materials[i].GetFloat("_SpecularHighlights"));
            }
            if (materials[i].HasProperty("_GlossyReflections"))
            {
                WriteFloat("<GlossyReflection>:", materials[i].GetFloat("_GlossyReflections"));
            }

            if (materials[i].HasProperty("_MainTex"))
            {
                Texture mainAlbedoMap = materials[i].GetTexture("_MainTex");
                WriteTextureName("<AlbedoMap>:", mainAlbedoMap);
            }
            if (materials[i].HasProperty("_SpecGlossMap"))
            {
                Texture specularcMap = materials[i].GetTexture("_SpecGlossMap");
                WriteTextureName("<SpecularMap>:", specularcMap);
            }
            if (materials[i].HasProperty("_MetallicGlossMap"))
            {
                Texture metallicMap = materials[i].GetTexture("_MetallicGlossMap");
                WriteTextureName("<MetallicMap>:", metallicMap);
            }
            if (materials[i].HasProperty("_BumpMap"))
            {
                Texture bumpMap = materials[i].GetTexture("_BumpMap");
                WriteTextureName("<NormalMap>:", bumpMap);
            }
            if (materials[i].HasProperty("_EmissionMap"))
            {
                Texture emissionMap = materials[i].GetTexture("_EmissionMap");
                WriteTextureName("<EmissionMap>:", emissionMap);
            }
            if (materials[i].HasProperty("_DetailAlbedoMap"))
            {
                Texture detailAlbedoMap = materials[i].GetTexture("_DetailAlbedoMap");
                WriteTextureName("<DetailAlbedoMap>:", detailAlbedoMap);
            }
            if (materials[i].HasProperty("_DetailNormalMap"))
            {
                Texture detailNormalMap = materials[i].GetTexture("_DetailNormalMap");
                WriteTextureName("<DetailNormalMap>:", detailNormalMap);
            }
        }
        WriteString("</Materials>");
    }

    void WriteSkeleton(SkinnedMeshRenderer smr)
    {
        Mesh mesh = smr.sharedMesh;
        Transform[] bones = smr.bones;

        WriteString("<Skeleton>:");
        WriteInteger("<BoneCount>:", bones.Length);

        for (int i = 0; i < bones.Length; i++)
        {
            WriteString("<BoneName>:", bones[i].name);
            WriteLocalMatrix("<BoneLocalMatrix>:", bones[i]);
        }

        for (int i = 0; i < bones.Length; i++)
        {
            int parentIndex = System.Array.IndexOf(bones, bones[i].parent);
            WriteInteger("<ParentIndex>:", parentIndex);
        }

        WriteMatrixes("<BindPoses>:", mesh.bindposes);

        BoneWeight[] weights = mesh.boneWeights;
        WriteInteger("<BoneWeightCount>:", weights.Length);

        WriteString("</Skeleton>");
    }

    void WriteFrameHierarchyInfo(Transform t)
    {
        WriteString("<Frame>:");
        WriteString(t.name);
        WriteString("<Transform>:");
        WriteMatrix(t.localToWorldMatrix);

        if (TryGetMesh(t, out Mesh mesh, out Renderer renderer))
        {
            WriteMeshInfo(mesh, renderer);
        }

        WriteInteger("<Children>:", t.childCount);
        for (int i = 0; i < t.childCount; i++)
            WriteFrameHierarchyInfo(t.GetChild(i));

        WriteString("</Frame>");
    }

    void Start()
    {
        binaryWriter = new BinaryWriter(File.Open(string.Copy(gameObject.name).Replace(" ", "_") + ".bin", FileMode.Create));

        // 스키닝
        SkinnedMeshRenderer smr = GetComponentInChildren<SkinnedMeshRenderer>();
        isSkinning = smr != null;

        WriteBool(isSkinning);

        if (isSkinning)
        {
            WriteSkeleton(smr);
        }

        // Frame
        WriteFrameHierarchyInfo(transform);

        binaryWriter.Flush();
        binaryWriter.Close();

        print("Model Binary Write Completed");
    }
}

