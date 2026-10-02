// ============================================================
// 文件：MrSSShare.fs
// 命名空间：Expander_CS.Backend.Common
// 对应：mrssshare.hpp
// ============================================================

namespace Expander_CS.Backend.Common

open System
open System.Text

[<AllowNullLiteral>]
type Expr(atom: bool, value: int, children: ResizeArray<Expr>) =
    member val Atom = atom with get, set
    member val Value = value with get, set
    member val Children = children with get

    new() = Expr(true, 0, ResizeArray<Expr>())
    new(v: int) = Expr(true, v, ResizeArray<Expr>())
    new(children: Expr seq) = Expr(false, 0, ResizeArray<Expr>(children))

    member this.IsAtom = this.Atom
    member this.IsExpr = not this.Atom

[<RequireQualifiedAccess>]
module Mrss =

    let isOne (e: Expr) =
        e.IsAtom && e.Value = 1

    let rec elementOrder (e: Expr) =
        if e.IsAtom then
            if e.Value = 1 then 0 else 1
        else
            let mutable maxChild = -1
            for c in e.Children do
                maxChild <- max maxChild (elementOrder c)
            maxChild + 1

    let expressionOrder (seq: ResizeArray<Expr>) =
        let mutable maxOrder = -1
        for e in seq do
            maxOrder <- max maxOrder (elementOrder e)
        maxOrder

    let rec firstValue (e: Expr) =
        if e.IsAtom then
            e.Value
        elif e.Children.Count = 0 then
            0
        else
            firstValue e.Children.[0]

    let rec lastValue (e: Expr) =
        if e.IsAtom then
            e.Value
        elif e.Children.Count = 0 then
            0
        else
            lastValue e.Children.[e.Children.Count - 1]

    let rec addValue (e: Expr) (delta: int) : Expr =
        if e.IsAtom then
            Expr(e.Value + delta)
        else
            let r = Expr(e.Atom, e.Value, ResizeArray<Expr>(e.Children))
            if r.Children.Count > 0 then
                let idx = r.Children.Count - 1
                r.Children.[idx] <- addValue r.Children.[idx] delta
            r

    let fromIntSequence (seq: int[]) : ResizeArray<Expr> =
        let out = ResizeArray<Expr>()
        if not (isNull seq) then
            for v in seq do
                out.Add(Expr(v))
        out

    let toIntSequence (seq: ResizeArray<Expr>) : int[] option =
        let out = ResizeArray<int>()
        let mutable ok = true
        if not (isNull seq) then
            for e in seq do
                if not e.IsAtom then
                    ok <- false
                else
                    out.Add(e.Value)
        if ok then Some(out.ToArray()) else None

type Parser private (s: string) =
    let mutable pos = 0

    member private this.Eof = pos >= s.Length

    member private this.Peek() =
        if this.Eof then '\000' else s.[pos]

    member private this.Get() =
        if this.Eof then
            '\000'
        else
            let c = s.[pos]
            pos <- pos + 1
            c

    member private this.SkipWs() =
        while not this.Eof && Char.IsWhiteSpace(this.Peek()) do
            pos <- pos + 1

    member private this.ParseInt() : int option =
        this.SkipWs()
        if this.Eof || not (Char.IsDigit(this.Peek())) then
            None
        else
            let mutable value = 0
            while not this.Eof && Char.IsDigit(this.Peek()) do
                value <- value * 10 + (int (this.Get()) - int '0')
            Some value

    member private this.ParseElement() : Expr option =
        this.SkipWs()
        if this.Eof then
            None
        elif this.Peek() = '(' then
            this.Get() |> ignore
            match this.ParseSequenceUntil(')') with
            | None -> None
            | Some seq ->
                this.SkipWs()
                if this.Eof || this.Get() <> ')' then
                    None
                else
                    Some(Expr(seq))
        else
            match this.ParseInt() with
            | Some v -> Some(Expr(v))
            | None -> None

    member private this.ParseSequenceUntil(endChar: char) : ResizeArray<Expr> option =
        let result = ResizeArray<Expr>()
        this.SkipWs()

        if not this.Eof && this.Peek() = endChar then
            Some result
        else
            let rec loop () =
                match this.ParseElement() with
                | None -> None
                | Some e ->
                    result.Add(e)
                    this.SkipWs()

                    if this.Eof then
                        Some result
                    elif this.Peek() = endChar then
                        Some result
                    elif this.Get() <> ',' then
                        None
                    else
                        this.SkipWs()
                        if not this.Eof && this.Peek() = endChar then
                            Some result
                        else
                            loop ()

            loop()

    static member private TryParse(str: string) : ResizeArray<Expr> option =
        if isNull str then
            None
        else
            let p = Parser(str)
            match p.ParseSequenceUntil('\000') with
            | Some seq ->
                p.SkipWs()
                if p.Eof then Some seq else None
            | None -> None

    static member Parse(str: string) : ResizeArray<Expr> =
        match Parser.TryParse(str) with
        | Some seq -> seq
        | None -> null

    static member ToString(seq: ResizeArray<Expr>) : string =
        if isNull seq || seq.Count = 0 then
            "[]"
        else
            let sb = StringBuilder()
            for i = 0 to seq.Count - 1 do
                if i <> 0 then sb.Append(',') |> ignore
                sb.Append(Parser.ToString(seq.[i])) |> ignore
            sb.ToString()

    static member ToString(e: Expr) : string =
        if e.IsAtom then
            string e.Value
        else
            let sb = StringBuilder()
            sb.Append('(') |> ignore
            for i = 0 to e.Children.Count - 1 do
                if i <> 0 then sb.Append(',') |> ignore
                sb.Append(Parser.ToString(e.Children.[i])) |> ignore
            sb.Append(')') |> ignore
            sb.ToString()