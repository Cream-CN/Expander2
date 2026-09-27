namespace Expander_CS.Backend.Core

type EntryArena() =

    let entries = ResizeArray<Entry>()

    /// 创建一个默认 Entry。
    member _.Make() : Entry =
        let e = Entry()
        entries.Add(e)
        e

    /// 用给定参数创建一个 Entry（会计算 YKey）。
    member _.Make(v: int, x: int, y: int[]) : Entry =
        let e = Entry(v, x, y)
        entries.Add(e)
        e

    /// 语义等价于 Make()，明确表示"空 Entry"。
    member _.MakeEmpty() : Entry =
        let e = Entry()
        entries.Add(e)
        e

    /// 释放 arena 中所有 Entry；之后之前返回的引用全部失效。
    member _.Clear() : unit =
        entries.Clear()

    /// 当前 arena 中 Entry 的数量。
    member _.Size : int = entries.Count