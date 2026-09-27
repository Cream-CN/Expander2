namespace Expander_CS.Backend.Common

open System

module Sequence =

    /// 将逗号分隔的字符串解析为 int[]。
    /// - 半角逗号 ',' 为分隔符
    /// - 去除每段中的所有空白字符
    /// - 空段（连续/首尾逗号）忽略
    /// - 非空段用 Int32.TryParse 转换，失败静默跳过
    /// - 永抛异常；全无效返回 [||]
    let parseSequence (text: string) : int[] =
        if String.IsNullOrEmpty(text) then
            [||]
        else
            text.Split(',')
            |> Array.choose (fun s ->
                let cleaned =
                    s |> String.filter (fun c -> not (Char.IsWhiteSpace c))
                match Int32.TryParse(cleaned) with
                | true, v -> Some v
                | _ -> None)

    /// 将 int[] 序列化为以半角逗号连接的字符串。
    /// null 或空数组返回 "[]"。
    let seqToString (seq: int[]) : string =
        if isNull seq || seq.Length = 0 then
            "[]"
        else
            seq
            |> Array.map string
            |> String.concat ","