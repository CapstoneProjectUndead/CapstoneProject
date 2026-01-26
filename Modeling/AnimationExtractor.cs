using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.IO;

public class BinaryModelExporter : EditorWindow
{
    GameObject fbxAsset;
    BinaryWriter bw;

    private List<string> m_pTextureNamesListForCounting = new List<string>();
    private List<string> m_pTextureNamesListForWriting = new List<string>();
    Matrix4x4[] bindWorld;

    
    [MenuItem("Tools/DirectX Binary Exporter")]
    static void Init()
    {
        GetWindow<BinaryModelExporter>("DX Binary Exporter");
    }

    void OnGUI()
    {
        GUILayout.Label("FBX Asset 선택", EditorStyles.boldLabel);
        fbxAsset = EditorGUILayout.ObjectField("FBX Asset", fbxAsset, typeof(GameObject), false) as GameObject;

        if (fbxAsset == null)
        {
            EditorGUILayout.HelpBox("Project 창에서 FBX 파일을 선택하세요.", MessageType.Info);
            return;
        }

        if (GUILayout.Button("Export Binary"))
            ExportBinary();
    }

    void ExportBinary()
    {
        string assetPath = AssetDatabase.GetAssetPath(fbxAsset);
        string savePath = EditorUtility.SaveFilePanel("Export Binary", "", fbxAsset.name + ".bin", "bin");
        if (string.IsNullOrEmpty(savePath))
            return;

        bw = new BinaryWriter(File.Open(savePath, FileMode.Create));

        Transform root = fbxAsset.transform;
        SkinnedMeshRenderer smr = fbxAsset.GetComponentInChildren<SkinnedMeshRenderer>();

        if (!smr)
        {
            Debug.LogError("SkinnedMeshRenderer를 찾을 수 없습니다.");
            return;
        }

        // 1) 프레임 트리
        WriteFrameHierarchy(root);

        // 2) 메쉬 + 스키닝
        WriteMeshAndSkinning(smr);

        CacheBindPoseWorld(smr.bones);

        // 3) 애니메이션
        WriteAnimationData(assetPath, root, smr.bones);

        bw.Close();
        Debug.Log("Export 완료: " + savePath);
    }

    // ============================================================
    //  FRAME HIERARCHY
    // ============================================================

    void WriteFrameHierarchy(Transform current)
    {
        WriteString("<Frame>:");
        WriteString(current.name.Replace(" ", "_"));

        WriteTransform("<Transform>:", current);
        WriteLocalMatrix("<TransformMatrix>:", current);

        WriteInteger("<Children>:", current.childCount);

        for (int i = 0; i < current.childCount; i++)
            WriteFrameHierarchy(current.GetChild(i));

        WriteString("</Frame>");
    }

    // ============================================================
    //  MESH + SKINNING
    // ============================================================
    void DebugPrintBones(Transform[] bones)
    {
        Debug.Log("=== Unity Bone Order ===");
        for (int i = 0; i < bones.Length; i++)
        {
            Debug.Log($"{i}: {bones[i].name}");
        }
    }
    void WriteMeshAndSkinning(SkinnedMeshRenderer smr)
    {
        Mesh mesh = smr.sharedMesh;

        // Mesh Info
        WriteMeshInfo(mesh);

        // Materials
        WriteMaterials(smr.sharedMaterials);

        // Skinning
        WriteSkinningData(smr);
        //DebugPrintBones(smr.bones);
    }

    void WriteMaterials(Material[] materials)
    {
        WriteInteger("<Materials>:", materials.Length);
        for (int i = 0; i < materials.Length; i++)
        {
            WriteInteger("<Material>:", i);

            if (materials[i].HasProperty("_Color"))
                WriteColor("<AlbedoColor>:", materials[i].GetColor("_Color"));

            if (materials[i].HasProperty("_EmissionColor"))
                WriteColor("<EmissiveColor>:", materials[i].GetColor("_EmissionColor"));

            if (materials[i].HasProperty("_SpecColor"))
                WriteColor("<SpecularColor>:", materials[i].GetColor("_SpecColor"));

            if (materials[i].HasProperty("_Glossiness"))
                WriteFloat("<Glossiness>:", materials[i].GetFloat("_Glossiness"));

            if (materials[i].HasProperty("_Metallic"))
                WriteFloat("<Metallic>:", materials[i].GetFloat("_Metallic"));

            if (materials[i].HasProperty("_MainTex"))
                WriteTextureName("<AlbedoMap>:", materials[i].GetTexture("_MainTex"));

            if (materials[i].HasProperty("_BumpMap"))
                WriteTextureName("<NormalMap>:", materials[i].GetTexture("_BumpMap"));
        }
        WriteString("</Materials>");
    }

    void WriteMeshInfo(Mesh mesh)
    {
        WriteString("<Mesh>:");
        WriteInteger(mesh.vertexCount);

        WriteBoundingBox("<Bounds>:", mesh.bounds);

        if (mesh.vertices.Length > 0) WriteVectors("<Positions>:", mesh.vertices);
        if (mesh.colors.Length > 0) WriteColors("<Colors>:", mesh.colors);
        if (mesh.uv.Length > 0) WriteTextureCoords("<TextureCoords0>:", mesh.uv);
        if (mesh.normals.Length > 0) WriteVectors("<Normals>:", mesh.normals);

        WriteInteger("<SubMeshes>:", mesh.subMeshCount);
        for (int i = 0; i < mesh.subMeshCount; i++)
        {
            int[] indices = mesh.GetTriangles(i);
            WriteIntegers("<SubMesh>:", i, indices);
        }

        WriteString("</Mesh>");
    }

    void WriteSkinningData(SkinnedMeshRenderer smr)
    {
        Mesh mesh = smr.sharedMesh;
        Transform[] bones = smr.bones;

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

        for (int i = 0; i < weights.Length; i++)
        {
            BoneWeight bwgt = weights[i];
            int[] idx = { bwgt.boneIndex0, bwgt.boneIndex1, bwgt.boneIndex2, bwgt.boneIndex3 };
            float[] w = { bwgt.weight0, bwgt.weight1, bwgt.weight2, bwgt.weight3 };

            WriteIntegers("<BoneIndex>:", idx);
            WriteFloats("<BoneWeight>:", w);
        }
    }

    // ============================================================
    //  ANIMATION EXPORT
    // ============================================================
    void CacheBindPoseWorld(Transform[] bones)
    {
        bindWorld = new Matrix4x4[bones.Length];
        for (int i = 0; i < bones.Length; i++)
            bindWorld[i] = bones[i].localToWorldMatrix;
    }
    void WriteAnimationData(string assetPath, Transform root, Transform[] bones)
    {
        Object[] allAssets = AssetDatabase.LoadAllAssetsAtPath(assetPath);

        List<AnimationClip> clips = new List<AnimationClip>();
        foreach (var a in allAssets)
        {
            if (a is AnimationClip clip && !clip.name.StartsWith("__preview__"))
                clips.Add(clip);
        }

        WriteInteger("<AnimationClipCount>:", clips.Count);

        foreach (var clip in clips)
            WriteAnimationClip(clip, root, bones);
    }

    void WriteAnimationClip(AnimationClip clip, Transform root, Transform[] bones)
    {
        WriteString("<AnimationClip>:", clip.name);
        WriteFloat("<ClipLength>:", clip.length);

        var keyTimes = CollectKeyTimes(clip);
        WriteInteger("<KeyframeCount>:", keyTimes.Count);

        foreach (float t in keyTimes)
        {
            WriteKeyframe(clip, t, root, bones, bindWorld);
        }

        WriteString("</AnimationClip>");
    }

    void WriteKeyframe(AnimationClip clip, float time, Transform root, Transform[] bones, Matrix4x4[] bindWorld)
    {
        clip.SampleAnimation(root.gameObject, time);

        WriteString("<Keyframe>:");
        WriteFloat("<Time>:", time);

        for (int i = 0; i < bones.Length; i++)
        {
            Transform bone = bones[i];

            Matrix4x4 animatedWorld = bone.localToWorldMatrix;

            int parentIndex = System.Array.IndexOf(bones, bone.parent);
            Matrix4x4 parentBindWorld =
                parentIndex < 0 ? Matrix4x4.identity : bindWorld[parentIndex];

            Matrix4x4 animatedLocal;

            if (parentIndex < 0)
            {
                animatedLocal = animatedWorld;
            }
            else
            {
                Matrix4x4 parentAnimatedWorld = bones[parentIndex].localToWorldMatrix;
                animatedLocal = parentAnimatedWorld.inverse * animatedWorld;
            }

            Vector3 pos;
            Quaternion rot;
            Vector3 scale;
            DecomposeMatrix(animatedLocal, out pos, out rot, out scale);

            WriteInteger("<Bone>:", i);
            WriteVector("<T>:", pos);
            WriteVector("<R>:", rot);
            WriteVector("<S>:", scale);
        }

        WriteString("</Keyframe>");
    }

    List<float> CollectKeyTimes(AnimationClip clip)
    {
        var bindings = AnimationUtility.GetCurveBindings(clip);
        HashSet<float> times = new HashSet<float>();

        foreach (var b in bindings)
        {
            AnimationCurve curve = AnimationUtility.GetEditorCurve(clip, b);
            foreach (var k in curve.keys)
                times.Add(k.time);
        }

        List<float> list = new List<float>(times);
        list.Sort();
        return list;
    }

    void DecomposeMatrix(Matrix4x4 m, out Vector3 pos, out Quaternion rot, out Vector3 scale)
    {
        pos = m.GetColumn(3);

        Vector3 x = m.GetColumn(0);
        Vector3 y = m.GetColumn(1);
        Vector3 z = m.GetColumn(2);

        scale = new Vector3(x.magnitude, y.magnitude, z.magnitude);

        if (scale.x != 0) x /= scale.x;
        if (scale.y != 0) y /= scale.y;
        if (scale.z != 0) z /= scale.z;

        rot = Quaternion.LookRotation(z, y);
    }

    // WriteXX
    void WriteTextureName(string strHeader, Texture texture)
    {
        bw.Write(strHeader);
        if (texture)
        {
            if (FindTextureByName(m_pTextureNamesListForWriting, texture))
            {
                bw.Write("@" + string.Copy(texture.name).Replace(" ", "_"));
            }
            else
            {
                bw.Write(string.Copy(texture.name).Replace(" ", "_"));
            }
        }
        else
        {
            bw.Write("null");
        }
    }

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

    void WriteString(string header, string value) { bw.Write(header); bw.Write(value); }
    void WriteString(string value) { bw.Write(value); }
    void WriteInteger(string header, int v) { bw.Write(header); bw.Write(v); }
    void WriteInteger(int v) { bw.Write(v); }
    void WriteFloat(string header, float f) { bw.Write(header); bw.Write(f); }
    void WriteFloat(float f) { bw.Write(f); }

    void WriteVector(string header, Vector3 v) { bw.Write(header); WriteVector(v); }
    void WriteVector(Vector3 v) { bw.Write(v.x); bw.Write(v.y); bw.Write(v.z); }

    void WriteVector(string header, Quaternion q) { bw.Write(header); WriteVector(q); }
    void WriteVector(Quaternion q) { bw.Write(q.x); bw.Write(q.y); bw.Write(q.z); bw.Write(q.w); }

    void WriteColor(string header, Color c) { bw.Write(header); WriteColor(c); }
    void WriteColor(Color c) { bw.Write(c.r); bw.Write(c.g); bw.Write(c.b); bw.Write(c.a); }

    void WriteTextureCoords(string header, Vector2[] uv)
    {
        bw.Write(header);
        bw.Write(uv.Length);
        foreach (var v in uv) { bw.Write(v.x); bw.Write(1 - v.y); }
    }

    void WriteVectors(string header, Vector3[] v)
    {
        bw.Write(header);
        bw.Write(v.Length);
        foreach (var p in v) WriteVector(p);
    }

    void WriteColors(string header, Color[] c)
    {
        bw.Write(header);
        bw.Write(c.Length);
        foreach (var p in c) WriteColor(p);
    }

    void WriteIntegers(string header, int i, int[] arr)
    {
        bw.Write(header);
        bw.Write(i);
        bw.Write(arr.Length);
        foreach (var v in arr) bw.Write(v);
    }

    void WriteFloats(string header, float[] arr)
    {
        bw.Write(header);
        bw.Write(arr.Length);
        foreach (var v in arr) bw.Write(v);
    }

    void WriteBoundingBox(string header, Bounds b)
    {
        bw.Write(header);
        WriteVector(b.center);
        WriteVector(b.extents);
    }

    void WriteMatrix(Matrix4x4 m)
    {
        bw.Write(m.m00); bw.Write(m.m10); bw.Write(m.m20); bw.Write(m.m30);
        bw.Write(m.m01); bw.Write(m.m11); bw.Write(m.m21); bw.Write(m.m31);
        bw.Write(m.m02); bw.Write(m.m12); bw.Write(m.m22); bw.Write(m.m32);
        bw.Write(m.m03); bw.Write(m.m13); bw.Write(m.m23); bw.Write(m.m33);
    }

    void WriteMatrixes(string header, Matrix4x4[] arr)
    {
        bw.Write(header);
        bw.Write(arr.Length);
        foreach (var m in arr) WriteMatrix(m);
    }

    void WriteLocalMatrix(string header, Transform t)
    {
        bw.Write(header);
        Matrix4x4 m = Matrix4x4.identity;
        m.SetTRS(t.localPosition, t.localRotation, t.localScale);
        WriteMatrix(m);
    }

    void WriteTransform(string header, Transform t)
    {
        bw.Write(header);
        WriteVector(t.localPosition);
        WriteVector(t.localEulerAngles);
        WriteVector(t.localScale);
        WriteVector(t.localRotation);
    }

    void WriteIntegers(string header, int[] arr)
    {
        bw.Write(header);
        bw.Write(arr.Length);
        foreach (var v in arr)
            bw.Write(v);
    }
}