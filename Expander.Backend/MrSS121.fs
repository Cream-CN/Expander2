// ============================================================
// 文件：MrSS121.fs
// 命名空间：Expander_CS.Backend.Notation
// 对应：MrSS121.hpp
// 依赖：MrSSShare.fs（必须排在本文件之前编译）
// ============================================================

namespace Expander_CS.Backend.Notation

open System
open Expander_CS.Backend.Common

[<RequireQualifiedAccess>]
module Mrss121Notation =

    [<Literal>]
    let Name = "MrSS1.2.1"

    let rec private startsWith (a: Expr) (b: Expr) : bool =
        if a.IsAtom && b.IsAtom then
            a.Value = b.Value
        elif not a.IsAtom && b.IsAtom then
            if a.Children.Count = 0 then false
            else startsWith a.Children.[0] b
        elif a.IsAtom && not b.IsAtom then
            false
        else
            if a.Children.Count < b.Children.Count then
                false
            else
                let mutable ok = true
                let mutable i = 0
                while ok && i < b.Children.Count do
                    if not (startsWith a.Children.[i] b.Children.[i]) then
                        ok <- false
                    i <- i + 1
                ok

    let private findBadRoot (seq: ResizeArray<Expr>) : int option =
        if seq.Count < 2 then
            None
        else
            let last = seq.[seq.Count - 1]
            let mutable result = None
            let mutable i = seq.Count - 2

            while result.IsNone && i >= 0 do
                let cand = seq.[i]
                if cand.IsAtom && last.IsAtom then
                    if cand.Value < last.Value then
                        result <- Some i
                elif startsWith last cand then
                    result <- Some i
                i <- i - 1

            result

    let private difference (last: Expr) (root: Expr) =
        Mrss.lastValue last - Mrss.lastValue root - 1

    let private badPart (seq: ResizeArray<Expr>) (root: int) : ResizeArray<Expr> =
        let result = ResizeArray<Expr>()
        for i = root to seq.Count - 2 do
            result.Add(seq.[i])
        result

    let private replaceLast
        (seq: ResizeArray<Expr>)
        (bad: ResizeArray<Expr>)
        (diff: int)
        (term: int) : ResizeArray<Expr> =

        let result = ResizeArray<Expr>()
        for i = 0 to seq.Count - 2 do
            result.Add(seq.[i])

        for i = 1 to term do
            for e in bad do
                result.Add(Mrss.addValue e (diff * i))

        if bad.Count > 0 && result.Count > 0 then
            let rootVal = Mrss.lastValue bad.[0]
            let mutable extra = 0

            while Mrss.lastValue result.[result.Count - 1] <= rootVal && extra < term do
                for e in bad do
                    result.Add(Mrss.addValue e (diff * (term + extra + 1)))
                extra <- extra + 1

        result

    let private expandY1 (seq: ResizeArray<Expr>) (term: int) : ResizeArray<Expr> =
        if seq.Count = 0 then
            ResizeArray<Expr>()
        elif Mrss.isOne seq.[seq.Count - 1] then
            let r = ResizeArray<Expr>(seq)
            r.RemoveAt(r.Count - 1)
            r
        else
            match findBadRoot seq with
            | None ->
                let r = ResizeArray<Expr>(seq)
                r.RemoveAt(r.Count - 1)
                r
            | Some root ->
                let diff = difference seq.[seq.Count - 1] seq.[root]
                let bad = badPart seq root
                replaceLast seq bad diff term

    let private expandHigh (seq: ResizeArray<Expr>) (term: int) : ResizeArray<Expr> =
        expandY1 seq term

    let private expandExpr (seq: ResizeArray<Expr>) (term: int) : ResizeArray<Expr> =
        if seq.Count = 0 then
            ResizeArray<Expr>()
        elif Mrss.isOne seq.[seq.Count - 1] then
            let r = ResizeArray<Expr>(seq)
            r.RemoveAt(r.Count - 1)
            r
        else
            let order = Mrss.expressionOrder seq
            if order <= 1 then
                expandY1 seq term
            else
                expandHigh seq term

    [<CompiledName("Expand")>]
    let expand (seq: int[]) (term: int) : int[] =
        let t = if term < 1 then 1 else term
        let exprSeq = Mrss.fromIntSequence seq
        let result = expandExpr exprSeq t

        match Mrss.toIntSequence result with
        | Some ints -> ints
        | None ->
            if isNull seq then [||]
            else Array.copy seq

    [<CompiledName("Suffix")>]
    let suffix () = ""

    [<CompiledName("ExpandString")>]
    let expandString (seq: string) (term: int) : string =
        let t = if term < 1 then 1 else term
        let parsed = Parser.Parse(seq)

        if isNull parsed then
            "[]"
        else
            let result = expandExpr parsed t
            Parser.ToString(result)

    [<CompiledName("Parse")>]
    let parse (seq: string) : ResizeArray<Expr> =
        Parser.Parse(seq)

    [<CompiledName("ToString")>]
    let toString (seq: ResizeArray<Expr>) : string =
        Parser.ToString(seq)