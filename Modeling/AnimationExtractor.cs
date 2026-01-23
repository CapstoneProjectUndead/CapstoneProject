using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.IO;

public class BinaryModelExporter : EditorWindow
{
    GameObject fbxAsset;

    [MenuItem("Tools/Binary Model Exporter")]
    static void Init()
    {
        GetWindow<BinaryModelExporter>("Binary Exporter");
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
        {
            ExportBinary();
        }
    }

    void ExportBinary()
    {
        string assetPath = AssetDatabase.GetAssetPath(fbxAsset);
        string savePath = EditorUtility.SaveFilePanel("Export Binary", "", fbxAsset.name + ".bin", "bin");

        if (string.IsNullOrEmpty(savePath))
            return;

        using (BinaryWriter bw = new BinaryWriter(File.Open(savePath, FileMode.Create)))
        {
            WriteModel(bw, fbxAsset, assetPath);
        }

        Debug.Log("Export 완료: " + savePath);
    }

    // ============================================================
    //  MODEL EXPORT
    // ============================================================

    void WriteModel(BinaryWriter bw, GameObject fbxRoot, string assetPath)
    {
        // Animations (FBX Asset에서 직접 가져옴)
        WriteAnimationClips(bw, assetPath);
    }

    // ============================================================
    //  ANIMATION EXPORT (정답)
    // ============================================================

    void WriteAnimationClips(BinaryWriter bw, string assetPath)
    {
        Object[] allAssets = AssetDatabase.LoadAllAssetsAtPath(assetPath);

        List<AnimationClip> clips = new List<AnimationClip>();
        foreach (var a in allAssets)
        {
            if (a is AnimationClip clip)
                clips.Add(clip);
        }

        bw.Write("<AnimationClipCount>:");
        bw.Write(clips.Count);

        foreach (var clip in clips)
            WriteAnimationClip(bw, clip);
    }

    void WriteAnimationClip(BinaryWriter bw, AnimationClip clip)
    {
        bw.Write("<AnimationClip>:");
        bw.Write(clip.name);

        bw.Write("<ClipLength>:");
        bw.Write(clip.length);

        var bindings = AnimationUtility.GetCurveBindings(clip);

        bw.Write("<CurveCount>:");
        bw.Write(bindings.Length);

        foreach (var b in bindings)
        {
            bw.Write("<CurvePath>:");
            bw.Write(b.path);

            bw.Write("<CurveProperty>:");
            bw.Write(b.propertyName);

            AnimationCurve curve = AnimationUtility.GetEditorCurve(clip, b);

            bw.Write("<KeyCount>:");
            bw.Write(curve.keys.Length);

            foreach (var k in curve.keys)
            {
                bw.Write("<KeyTime>:");
                bw.Write(k.time);

                bw.Write("<KeyValue>:");
                bw.Write(k.value);
            }
        }

        bw.Write("</AnimationClip>");
    }

    // ============================================================
    //  UTILS
    // ============================================================

    void WriteVector(BinaryWriter bw, Vector3 v)
    {
        bw.Write(v.x); bw.Write(v.y); bw.Write(v.z);
    }

    void WriteVector(BinaryWriter bw, Vector4 v)
    {
        bw.Write(v.x); bw.Write(v.y); bw.Write(v.z); bw.Write(v.w);
    }

    void WriteVector(BinaryWriter bw, Quaternion q)
    {
        bw.Write(q.x); bw.Write(q.y); bw.Write(q.z); bw.Write(q.w);
    }

    void WriteMatrix(BinaryWriter bw, Matrix4x4 m)
    {
        for (int i = 0; i < 16; i++)
            bw.Write(m[i]);
    }
}