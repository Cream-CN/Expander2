//Entry
namespace Expander_CS.Backend.Core

open System.Collections.Generic

[<AllowNullLiteral>]
type Entry() =

    let mutable value = 0
    let mutable x = 0
    let mutable y : int[] = [||]
    let mutable ykey : uint64 = 0UL

    let leftLegUp = ResizeArray<Entry>()
    let mutable rightLegUp : Entry = null
    let mutable rightLegDown : Entry = null
    let mutable leftLegDown : Entry = null

    member _.Value
        with get () = value
        and set v = value <- v

    member _.X
        with get () = x
        and set v = x <- v

    ///NOTICE 直接赋值不会自动刷新 YKey，需手动调用 RefreshKey()。
    member _.Y
        with get () = y
        and set v = y <- v

    member _.YKey = ykey

    /// F# 侧为 ResizeArray<Entry>，C# 侧看到 System.Collections.Generic.List<Entry>。
    member _.LeftLegUp = leftLegUp

    member _.RightLegUp
        with get () = rightLegUp
        and set v = rightLegUp <- v

    member _.RightLegDown
        with get () = rightLegDown
        and set v = rightLegDown <- v

    member _.LeftLegDown
        with get () = leftLegDown
        and set v = leftLegDown <- v

    /// 根据当前 Y 重新计算 YKey；不抛异常。
    member this.RefreshKey() =
        ykey <- Entry.MakeYKey(this.Y)

    /// 带参构造：设置 V / X / Y 后立即计算 YKey。
    new(v: int, x: int, y: int[]) as this =
        Entry() then
            this.Value <- v
            this.X <- x
            this.Y <- y
            this.RefreshKey()

    /// 与原 C++ 一致的 ykey 计算：
    /// - 最高字节：y.Length 的低 8 位
    /// - 前 7 个元素：第 i 个取低 16 位，左移 (i*8) 位后按位或
    /// - 存在碰撞可能，长度 256 的倍数与 0 碰撞
    ///准备重构
    static member MakeYKey(y: int[]) : uint64 =
        if isNull y then
            0UL
        else
            let len = y.Length
            let mutable key = (uint64 (len &&& 0xFF)) <<< 56
            let n = min len 7
            for i in 0 .. n - 1 do
                let low16 = uint64 (y.[i] &&& 0xFFFF)
                key <- key ||| (low16 <<< (i * 8))
            key