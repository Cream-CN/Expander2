namespace Expander_CS.Backend.Notation

open System
open System.Collections.Generic
open Expander_CS.Backend.Core

/// 内部共享实现。仅用于 omega_y.hpp / omega-Y-magma.js 的移植。
module private OmegaYMagmaDetail =

    [<Literal>]
    let Infinity = System.Int32.MaxValue

    type Mountain = ResizeArray<ResizeArray<Entry>>

    /// 判定 seq 是否为极限哨兵 [Infinity]
    let isLimitSentinel (s: int[]) : bool =
        s.Length = 1 && s.[0] = Infinity

    // -----------------------------------------------------------------
    // 几何比较
    // -----------------------------------------------------------------

    /// 逐位比较 y：先比长度，再从高位到低位
    /// 返回 1(a>b) / -1(a<b) / 0(相等)
    let verticalCompare (a: int[]) (b: int[]) : int =
        if a.Length > b.Length then 1
        elif a.Length < b.Length then -1
        else
            let mutable i = a.Length - 1
            let mutable r = 0
            while i >= 0 && r = 0 do
                if a.[i] > b.[i] then r <- 1
                elif a.[i] < b.[i] then r <- -1
                i <- i - 1
            r

    let sameRow (e1: Entry) (e2: Entry) : bool =
        verticalCompare e1.Y e2.Y = 0

    /// vertical_increase(y, d)：清零低 d 位，第 d 位 +1
    let verticalIncrease (y: int[]) (d: int) : int[] =
        let mutable c = Array.copy y
        if d >= 0 then
            if d >= c.Length then Array.Resize(&c, d + 1)
            if c.[d] = 0 then c.[d] <- 1 else c.[d] <- c.[d] + 1
            let m = min d c.Length
            for i in 0 .. m - 1 do
                c.[i] <- 0
        c

    /// 从高到低找到第一位不同的维度；完全相同返回 -1
    let dimensionDifference (c1: int[]) (c2: int[]) : int =
        let mutable d = max c1.Length c2.Length
        let mutable r = -1
        while d > 0 && r = -1 do
            d <- d - 1
            let has1 = d < c1.Length
            let has2 = d < c2.Length
            if has1 <> has2 then r <- d
            elif has1 && has2 && c1.[d] <> c2.[d] then r <- d
        r

    // -----------------------------------------------------------------
    // 建山 / 转序列
    // -----------------------------------------------------------------

    /// from_sequence：从 int[] 构建初始 mountain
    let fromSequence (arena: EntryArena) (seq: int[]) : Mountain =
        let mountain = Mountain(seq.Length)
        for i in 0 .. seq.Length - 1 do
            let bottom = arena.Make(seq.[i], i, [| 1 |])
            let phantom = arena.Make(0, i, [||])
            bottom.RightLegDown <- phantom
            phantom.RightLegUp <- bottom
            if i > 0 then
                bottom.LeftLegDown <- mountain.[i - 1].[1]
                mountain.[i - 1].[1].LeftLegUp.Add(bottom)
            let col = ResizeArray<Entry>()
            col.Add(bottom)
            col.Add(phantom)
            mountain.Add(col)
        mountain

    /// to_sequence：取每列倒数第二个 Entry 的 value
    let toSequence (mountain: Mountain) : int[] =
        let out = Array.zeroCreate mountain.Count
        for i in 0 .. mountain.Count - 1 do
            let col = mountain.[i]
            if col.Count < 2 then out.[i] <- 0
            else out.[i] <- col.[col.Count - 2].Value
        out

    /// create_entry：由 parent、entry 在 entry 上方生成一层新 Entry
    let createEntry (arena: EntryArena) (parent: Entry) (entry: Entry) : Entry =
        let dd = dimensionDifference parent.Y entry.Y + 1
        let newentry =
            arena.Make(entry.Value - parent.Value, entry.X,
                       verticalIncrease entry.Y dd)
        newentry.RightLegDown <- entry
        entry.RightLegUp <- newentry
        newentry.LeftLegDown <- parent
        parent.LeftLegUp.Add(newentry)
        newentry

    /// draw_mountain：把每列填满，直到顶项 value = 1
    let drawMountain (arena: EntryArena) (mountain: Mountain) : Mountain =
        for column in mountain do
            let mutable loop = true
            while loop do
                if column.Count = 0 then loop <- false
                else
                    let entry = column.[0]
                    if isNull entry then loop <- false
                    elif entry.Value = 1 then loop <- false
                    else
                        let mutable parent = entry
                        let mutable innerLoop = true
                        while innerLoop do
                            let mutable up = parent.LeftLegDown
                            if isNull up then innerLoop <- false
                            else
                                while not (isNull up.RightLegUp) &&
                                      verticalCompare up.RightLegUp.Y parent.Y <= 0 do
                                    up <- up.RightLegUp
                                parent <- up
                                if parent.Value < entry.Value then
                                    innerLoop <- false
                        column.Insert(0, createEntry arena parent entry)
        mountain

    // -----------------------------------------------------------------
    // 二分查找 / 切片
    // -----------------------------------------------------------------

    let findLower (column: ResizeArray<Entry>) (y: int[]) : Entry =
        if column.Count = 0 then Unchecked.defaultof<Entry>
        else
            let mutable i1 = 0
            let mutable i2 = column.Count - 1
            while i1 < i2 do
                let i = (i1 + i2) / 2
                if verticalCompare column.[i].Y y < 0 then i2 <- i
                else i1 <- i + 1
            column.[i2]

    let findHigherEqual (column: ResizeArray<Entry>) (y: int[]) : Entry =
        if column.Count = 0 then Unchecked.defaultof<Entry>
        else
            let mutable i1 = 0
            let mutable i2 = column.Count - 1
            while i1 < i2 do
                let i = (i1 + i2 + 1) / 2
                if verticalCompare column.[i].Y y >= 0 then i1 <- i
                else i2 <- i - 1
            column.[i1]

    /// 返回 [lowequal, high) 区间内的项
    let yslice (column: ResizeArray<Entry>) (lowequal: int[]) (high: int[]) : ResizeArray<Entry> =
        let result = ResizeArray<Entry>()
        if column.Count > 0 then
            let mutable i1 = 0
            let mutable i2 = column.Count - 1
            while i1 < i2 do
                let i = (i1 + i2) / 2
                if verticalCompare column.[i].Y high < 0 then i2 <- i
                else i1 <- i + 1
            let start = i2
            i1 <- start
            i2 <- column.Count - 1
            while i1 < i2 do
                let i = (i1 + i2) / 2
                if verticalCompare column.[i].Y lowequal < 0 then i2 <- i
                else i1 <- i + 1
            for i in start .. i2 - 1 do
                result.Add(column.[i])
        result

    // -----------------------------------------------------------------
    // 收集器
    // -----------------------------------------------------------------

    /// collect_usual / collect_weak：medium magma 用的收集
    let rec collectWeak (working: Entry) (collection: ResizeArray<Entry>) : unit =
        for e in working.LeftLegUp do
            let child = e.RightLegDown
            if not (isNull child) then
                let alreadyIn =
                    collection |> Seq.exists (fun x -> obj.ReferenceEquals(x, child))
                if not alreadyIn && sameRow working child then
                    collection.Add(child)
                    collectWeak child collection

    /// collect1D / collect_strong：strong magma 用的收集
    let rec collectStrong (working: Entry) (collection: ResizeArray<Entry>) : unit =
        if not (isNull working) && not (isNull working.RightLegDown) then
            for child in working.RightLegDown.LeftLegUp do
                if not (isNull child) then
                    let alreadyIn =
                        collection |> Seq.exists (fun x -> obj.ReferenceEquals(x, child))
                    if not alreadyIn && sameRow working child then
                        collection.Add(child)
                        collectStrong child collection

    /// collect —— omega_y.hpp 的分发：1D 或 usual
    let collect (working: Entry) : ResizeArray<Entry> =
        let collection = ResizeArray<Entry>()
        if isNull working then collection
        elif verticalCompare working.Y [| 1 |] > 0
             && not (isNull working.RightLegDown)
             && dimensionDifference working.Y working.RightLegDown.Y = 0 then
            collectStrong working collection
            collection
        else
            collectWeak working collection
            collection

    // -----------------------------------------------------------------
    // 边填充 / 复制
    // -----------------------------------------------------------------

    let fillMagmaEdge (arena: EntryArena) (mountain: Mountain)
                      (sourceEntry: Entry) (leftlegEntry: Entry) : unit =
        let targetx = sourceEntry.X - sourceEntry.LeftLegDown.X + leftlegEntry.X
        let mutable d = dimensionDifference leftlegEntry.Y leftlegEntry.RightLegUp.Y
        while d >= 0 do
            let newentry =
                arena.Make(0, targetx, verticalIncrease leftlegEntry.Y d)
            newentry.LeftLegDown <- leftlegEntry
            leftlegEntry.LeftLegUp.Add(newentry)
            while mountain.Count <= targetx do
                mountain.Add(ResizeArray<Entry>())
            mountain.[targetx].Add(newentry)
            d <- d - 1

    let copySingleEdge (arena: EntryArena) (mountain: Mountain)
                       (sourceEntry: Entry) (xOffset: int) (BRx: int)
                       (targety: int[] option) : unit =
        let ty =
            match targety with
            | Some t -> Array.copy t
            | None -> Array.copy sourceEntry.Y
        let newX = sourceEntry.X + xOffset
        let newentry = arena.Make(0, newX, ty)
        if sourceEntry.Y.Length > 0 && not (isNull sourceEntry.LeftLegDown) then
            let leftlegEntry =
                if sourceEntry.LeftLegDown.X >= BRx then
                    let idx = sourceEntry.LeftLegDown.X + xOffset
                    while mountain.Count <= idx do
                        mountain.Add(ResizeArray<Entry>())
                    findLower mountain.[idx] newentry.Y
                else
                    sourceEntry.LeftLegDown
            newentry.LeftLegDown <- leftlegEntry
            if not (isNull leftlegEntry) then
                leftlegEntry.LeftLegUp.Add(newentry)
        while mountain.Count <= newX do
            mountain.Add(ResizeArray<Entry>())
        mountain.[newX].Add(newentry)

    // -----------------------------------------------------------------
    // 主骨架
    // -----------------------------------------------------------------

    type MagmaKind =
        | Medium
        | Strong

    /// magma_expand：medium / strong magma 的公共骨架
    let magmaExpand (seq: int[]) (FSterm: int) (kind: MagmaKind) : int[] =
        if seq.Length = 0 then [||]
        elif seq.[seq.Length - 1] = 1 then
            Array.sub seq 0 (seq.Length - 1)
        else
            let arena = new EntryArena()
            let mutable mountain = drawMountain arena (fromSequence arena seq)

            let child = mountain.[mountain.Count - 1]
            let br0 = child.[0].LeftLegDown
            let width = mountain.Count - 1 - br0.X

            let col0 = mountain.[br0.X]
            let mutable topIdx = 0
            for i in 0 .. col0.Count - 1 do
                if obj.ReferenceEquals(col0.[i], br0) then topIdx <- i
            let top = ResizeArray<Entry>()
            top.Add(child.[0])
            for i in topIdx .. col0.Count - 2 do
                top.Add(col0.[i])

            let s = Array.copy seq
            s.[s.Length - 1] <- s.[s.Length - 1] - 1
            mountain <- drawMountain arena (fromSequence arena s)

            let mutable BR = br0
            let col2 = mountain.[br0.X]
            let mutable found = false
            for e in col2 do
                if not found && sameRow e br0 then
                    BR <- e
                    found <- true

            let magmaEntries =
                Array.init (width + 1) (fun _ -> ResizeArray<Entry>())

            match kind with
            | Medium ->
                let mutable br1 = BR
                let mutable stop = false
                while not stop do
                    let collection = ResizeArray<Entry>()
                    collectWeak br1 collection
                    for entry in collection do
                        let dx = entry.X - BR.X
                        if dx >= 0 && dx < magmaEntries.Length then
                            magmaEntries.[dx].Add(entry)
                    if br1.Y.Length = 0 || isNull br1.RightLegDown then
                        stop <- true
                    else
                        br1 <- br1.RightLegDown
            | Strong ->
                let mutable br1 = BR
                let mutable stop = false
                while not stop do
                    if br1.Y.Length > 0 then
                        let collection = ResizeArray<Entry>()
                        collectStrong br1 collection
                        for entry in collection do
                            let dx = entry.X - BR.X
                            if dx >= 0 && dx < magmaEntries.Length then
                                magmaEntries.[dx].Add(entry)
                        if isNull br1.RightLegDown then
                            stop <- true
                        else
                            br1 <- br1.RightLegDown
                    else
                        let mutable dx1 = 0
                        while BR.X + 1 + dx1 < mountain.Count do
                            let col = mountain.[BR.X + 1 + dx1]
                            let dx = dx1 + 1
                            if dx < magmaEntries.Length && col.Count > 0 then
                                magmaEntries.[dx].Add(col.[col.Count - 1])
                            dx1 <- dx1 + 1
                        stop <- true

            for n in 1 .. FSterm do
                let refList = ResizeArray<Entry>()
                let lastCol = mountain.[mountain.Count - 1]
                for t in top do
                    let lo = findLower lastCol t.Y
                    if not (isNull lo) then refList.Add(lo)

                for dx in 1 .. width do
                    let targetIdx = BR.X + n * width + dx
                    while mountain.Count <= targetIdx do
                        mountain.Add(ResizeArray<Entry>())
                    mountain.[targetIdx] <- ResizeArray<Entry>()

                    for magmaEntry in magmaEntries.[dx] do
                        copySingleEdge arena mountain magmaEntry (n * width) BR.X None

                        let mutable sourceEntry = magmaEntry
                        let he = findHigherEqual refList magmaEntry.Y
                        if not (isNull he) then
                            let mutable targety = he.Y
                            let targety0 = targety

                            let mutable loop = true
                            while loop do
                                let inMagma =
                                    magmaEntries.[dx]
                                    |> Seq.exists (fun e ->
                                        obj.ReferenceEquals(e, sourceEntry.RightLegUp))
                                if sourceEntry.Value <= 1
                                   || inMagma
                                   || isNull sourceEntry.RightLegUp then
                                    loop <- false
                                else
                                    targety <-
                                        verticalIncrease targety
                                            (dimensionDifference sourceEntry.Y
                                                sourceEntry.RightLegUp.Y)
                                    sourceEntry <- sourceEntry.RightLegUp
                                    copySingleEdge arena mountain sourceEntry
                                        (n * width) BR.X (Some targety)

                            if magmaEntry.Y.Length > 0
                               && not (isNull magmaEntry.LeftLegDown) then
                                let leftlegx = magmaEntry.LeftLegDown.X + n * width
                                if leftlegx >= 0 && leftlegx < mountain.Count then
                                    let ys =
                                        yslice mountain.[leftlegx]
                                              magmaEntry.Y targety0
                                    for leftlegEntry in ys do
                                        fillMagmaEdge arena mountain
                                                      magmaEntry leftlegEntry

                    let targetCol = mountain.[targetIdx]
                    let sorted =
                        targetCol
                        |> Seq.sortWith (fun a b -> -(verticalCompare a.Y b.Y))
                        |> Seq.toArray
                    targetCol.Clear()
                    for e in sorted do targetCol.Add(e)

                    for i in 0 .. targetCol.Count - 2 do
                        targetCol.[i].RightLegDown <- targetCol.[i + 1]
                        targetCol.[i + 1].RightLegUp <- targetCol.[i]

                    if targetCol.Count > 0 then
                        targetCol.[0].Value <- 1
                        for i in 1 .. targetCol.Count - 2 do
                            let up = targetCol.[i].RightLegUp
                            if not (isNull up) && not (isNull up.LeftLegDown) then
                                targetCol.[i].Value <-
                                    up.Value + up.LeftLegDown.Value

            toSequence mountain

    /// omega_Y_limit：omega_y.hpp 的核心展开
    let omegaYLimit (seq: int[]) (FSterm: int) : int[] =
        if seq.Length = 0 then seq
        else
            let arena = new EntryArena()
            let mutable mountain = drawMountain arena (fromSequence arena seq)

            if mountain.Count = 0 then seq
            else
                let child = mountain.[mountain.Count - 1]
                if child.Count = 0 || isNull child.[0] then seq
                else
                    let br0 = child.[0].LeftLegDown
                    if isNull br0 then seq
                    elif br0.X < 0 || br0.X >= mountain.Count then seq
                    else
                        let width = mountain.Count - 1 - br0.X
                        if width <= 0 then seq
                        else
                            let brCol = mountain.[br0.X]
                            let mutable topIdx = -1
                            for i in 0 .. brCol.Count - 1 do
                                if topIdx < 0 && obj.ReferenceEquals(brCol.[i], br0) then
                                    topIdx <- i
                            if topIdx < 0 || topIdx + 1 >= brCol.Count then seq
                            else
                                let top = ResizeArray<Entry>()
                                top.Add(child.[0])
                                for i in topIdx .. brCol.Count - 2 do
                                    top.Add(brCol.[i])

                                let s = Array.copy seq
                                s.[s.Length - 1] <- s.[s.Length - 1] - 1
                                mountain <- drawMountain arena (fromSequence arena s)

                                if br0.X < 0 || br0.X >= mountain.Count then seq
                                else
                                    let col2 = mountain.[br0.X]
                                    let mutable found = false
                                    let mutable BR = br0
                                    for e in col2 do
                                        if not found && sameRow e br0 then
                                            BR <- e
                                            found <- true
                                    if not found then seq
                                    else
                                        let magmaEntries = ResizeArray<ResizeArray<Entry>>()
                                        for _ in 0 .. width do
                                            magmaEntries.Add(ResizeArray<Entry>())

                                        let mutable br1 = BR
                                        let mutable stop = false
                                        while not stop do
                                            let collection = collect br1
                                            for entry in collection do
                                                let dx = entry.X - BR.X
                                                if dx > 0 then
                                                    while magmaEntries.Count <= dx do
                                                        magmaEntries.Add(ResizeArray<Entry>())
                                                    magmaEntries.[dx].Add(entry)
                                            if br1.Y.Length = 0 || isNull br1.RightLegDown then
                                                stop <- true
                                            else
                                                br1 <- br1.RightLegDown

                                        for n in 1 .. FSterm do
                                            let refList = ResizeArray<Entry>()
                                            let lastCol = mountain.[mountain.Count - 1]
                                            for t in top do
                                                if not (isNull t) then
                                                    let lo = findLower lastCol t.Y
                                                    if not (isNull lo) then refList.Add(lo)

                                            if refList.Count > 0 then
                                                for dx in 1 .. width do
                                                    let colx = BR.X + n * width + dx
                                                    if colx >= 0 then
                                                        while mountain.Count <= colx do
                                                            mountain.Add(ResizeArray<Entry>())
                                                        mountain.[colx] <- ResizeArray<Entry>()

                                                        if dx < magmaEntries.Count then
                                                            for magmaEntry in magmaEntries.[dx] do
                                                                if not (isNull magmaEntry) then
                                                                    copySingleEdge arena mountain magmaEntry
                                                                        (n * width) BR.X None

                                                                    let he =
                                                                        findHigherEqual refList magmaEntry.Y
                                                                    if not (isNull he) then
                                                                        let mutable sourceEntry = magmaEntry
                                                                        let mutable targety = he.Y
                                                                        let targety0 = targety

                                                                        let mutable innerLoop = true
                                                                        while innerLoop do
                                                                            let inMagma =
                                                                                magmaEntries.[dx]
                                                                                |> Seq.exists (fun e ->
                                                                                    obj.ReferenceEquals(
                                                                                        e, sourceEntry.RightLegUp))
                                                                            if sourceEntry.Value <= 1
                                                                               || inMagma
                                                                               || isNull sourceEntry.RightLegUp then
                                                                                innerLoop <- false
                                                                            else
                                                                                targety <-
                                                                                    verticalIncrease targety
                                                                                        (dimensionDifference
                                                                                            sourceEntry.Y
                                                                                            sourceEntry.RightLegUp.Y)
                                                                                sourceEntry <- sourceEntry.RightLegUp
                                                                                copySingleEdge arena mountain
                                                                                    sourceEntry (n * width) BR.X
                                                                                    (Some targety)

                                                                        if magmaEntry.Y.Length > 0
                                                                           && not (isNull magmaEntry.LeftLegDown) then
                                                                            let leftlegx =
                                                                                magmaEntry.LeftLegDown.X + n * width
                                                                            if leftlegx >= 0
                                                                               && leftlegx < mountain.Count then
                                                                                let ys =
                                                                                    yslice mountain.[leftlegx]
                                                                                          magmaEntry.Y targety0
                                                                                for leftlegEntry in ys do
                                                                                    fillMagmaEdge arena mountain
                                                                                                  magmaEntry leftlegEntry

                                                            let targetCol = mountain.[colx]
                                                            let sorted =
                                                                targetCol
                                                                |> Seq.sortWith (fun a b ->
                                                                    -(verticalCompare a.Y b.Y))
                                                                |> Seq.toArray
                                                            targetCol.Clear()
                                                            for e in sorted do targetCol.Add(e)

                                                            for i in 0 .. targetCol.Count - 2 do
                                                                targetCol.[i].RightLegDown <- targetCol.[i + 1]
                                                                targetCol.[i + 1].RightLegUp <- targetCol.[i]

                                                            if targetCol.Count > 0 then
                                                                targetCol.[0].Value <- 1
                                                                for i in 1 .. targetCol.Count - 2 do
                                                                    let up = targetCol.[i].RightLegUp
                                                                    if not (isNull up)
                                                                       && not (isNull up.LeftLegDown) then
                                                                        targetCol.[i].Value <-
                                                                            up.Value + up.LeftLegDown.Value

                                        toSequence mountain


// =====================================================================
// 对外接口 —— ω-Y 三个记号
// =====================================================================

module OmegaYNotation =

    [<Literal>]
    let Name = "ω-Y sequence"

    let definition : string = "ω-Y sequence (omega_y.hpp port)"

    /// 对序列执行 term 项展开；等价于原 C++ 的 FS 版本（丢弃结果末项）。
    /// 纯函数，不抛异常。
    let expand (seq: int[], term: int) : int[] =
        try
            if seq.Length = 0 then [||]
            elif seq.Length = 1 && seq.[0] = 0 then seq
            elif seq.[seq.Length - 1] = 1 then
                Array.sub seq 0 (seq.Length - 1)
            elif seq.Length = 1 then seq
            else
                let eff = term + seq.Length
                let full = OmegaYMagmaDetail.omegaYLimit seq eff
                if full.Length < 2 then full
                else Array.sub full 0 (full.Length - 1)
        with _ -> seq

    /// FSalter：保留末项，等价于原 C++ 的 expand_alter。
    let expandAlter (seq: int[], term: int) : int[] =
        try
            if seq.Length = 0 then [||]
            elif seq.[seq.Length - 1] = 1 then
                Array.sub seq 0 (seq.Length - 1)
            elif seq.Length = 1 then seq
            else
                let eff = term + seq.Length
                OmegaYMagmaDetail.omegaYLimit seq eff
        with _ -> seq

    let suffix () : string = ""


module OmegaYMediumNotation =

    [<Literal>]
    let Name = "ω-Y (medium magma)"

    let definition : string = "ω-Y sequence with medium magma"

    /// 对序列 seq 执行 term 项展开。纯函数，不抛异常。
    let expand (seq: int[], term: int) : int[] =
        try
            if seq.Length = 0 then [||]
            elif OmegaYMagmaDetail.isLimitSentinel seq then [| 1; 1 + term |]
            elif seq.[seq.Length - 1] = 1 then
                Array.sub seq 0 (seq.Length - 1)
            else
                OmegaYMagmaDetail.magmaExpand seq term OmegaYMagmaDetail.Medium
        with _ -> [||]

    let suffix () : string = ""


module OmegaYStrongNotation =

    [<Literal>]
    let Name = "ω-Y (strong magma)"

    let definition : string = "ω-Y sequence with strong magma"

    /// 对序列 seq 执行 term 项展开。纯函数，不抛异常。
    let expand (seq: int[], term: int) : int[] =
        try
            if seq.Length = 0 then [||]
            elif OmegaYMagmaDetail.isLimitSentinel seq then [| 1; 1 + term |]
            elif seq.[seq.Length - 1] = 1 then
                Array.sub seq 0 (seq.Length - 1)
            else
                OmegaYMagmaDetail.magmaExpand seq term OmegaYMagmaDetail.Strong
        with _ -> [||]

    let suffix () : string = ""