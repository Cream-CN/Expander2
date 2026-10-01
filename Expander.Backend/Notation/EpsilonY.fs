//ε-Y 由 go men 设计
namespace Expander_CS.Backend.Notation

open System
open System.Collections.Generic

module private EpsilonYDetail =

    [<AllowNullLiteral>]
    type Entry() =
        member val Value : int = 0 with get, set
        member val Row : int array = [||] with get, set
        member val Cloumn : int = -1 with get, set
        member val Idx : int = 0 with get, set
        member val No : int = 0 with get, set
        member val Id : int = 0 with get, set
        member val Parent : Entry = null with get, set
        member val Head : Entry = null with get, set
        member val Foot : Entry = null with get, set
        member val Ref : Entry = null with get, set

    type Arena() =
        let data = ResizeArray<Entry>()

        member _.Make() =
            let e = Entry()
            data.Add(e)
            e

        member _.Make(value: int,
                      row: int array,
                      cloumn: int,
                      idx: int,
                      no: int,
                      id: int,
                      parent: Entry,
                      head: Entry,
                      foot: Entry,
                      refEntry: Entry) =
            let e = Entry()
            e.Value <- value
            e.Row <- row
            e.Cloumn <- cloumn
            e.Idx <- idx
            e.No <- no
            e.Id <- id
            e.Parent <- parent
            e.Head <- head
            e.Foot <- foot
            e.Ref <- refEntry
            data.Add(e)
            e

        member this.MakeRoot() =
            this.Make(0, [| 1 |], -1, 0, 0, 0, null, null, null, null)

    let rec rowGenerator (n: int) : int array =
        Array.init n (fun i -> if i = 0 then 1 else 0)

    and compareRow (r1: int array) (r2: int array) : int =
        let mutable i = 0
        let mutable result = 0
        let mutable finished = false

        while not finished && i < r1.Length && i < r2.Length do
            if r1.[i] > r2.[i] then
                result <- 1
                finished <- true
            elif r1.[i] < r2.[i] then
                result <- -1
                finished <- true
            else
                i <- i + 1

        if not finished then
            if r1.Length > i then result <- 1
            elif r2.Length > i then result <- -1
            else result <- 0

        result

    and compareDimension (r1: int array) (r2: int array) : int =
        let a = if r1.Length <= 1 then 1 else r1.[1]
        let b = if r2.Length <= 1 then 1 else r2.[1]
        a - b

    and rowAddition (r1: int array) (r2: int array) : int array =
        let mutable i = 0
        let mutable j = 0
        let mutable result = [||]
        let mutable finished = false

        while not finished do
            if i >= r1.Length then
                result <- Array.append (Array.sub r1 0 i) (Array.sub r2 j (r2.Length - j))
                finished <- true
            elif j >= r2.Length then
                result <- r1
                finished <- true
            elif r1.[i] < r2.[j] then
                result <- Array.append (Array.sub r1 0 i) (Array.sub r2 j (r2.Length - j))
                finished <- true
            elif r1.[i] = r2.[j] && j + 1 < r2.Length && r2.[j + 1] > r2.[j] then
                i <- i + 1
                j <- j + 1
            else
                i <- i + 1

        result

    and rowDifference (r1: int array) (r2: int array) : int array =
        let mutable i = 0
        let mutable j = 0
        let mutable result = [||]
        let mutable finished = false

        while not finished do
            if i >= r1.Length then
                result <- [||]
                finished <- true
            elif j >= r2.Length || r1.[i] > r2.[j] then
                let row = Array.sub r1 i (r1.Length - i)

                if row.Length = 0 then
                    result <- row
                else
                    let rowList = ResizeArray<int>(row)
                    let mutable k = i - 1

                    while k >= 0 do
                        if k < r1.Length && r1.[k] < rowList.[0] then
                            rowList.Insert(0, r1.[k])
                        k <- k - 1

                    result <- rowList.ToArray()

                finished <- true
            else
                i <- i + 1
                j <- j + 1

        result

    and divide (d: int array) : int array array =
        let dd =
            if d.Length > 1 then Array.sub d 1 (d.Length - 1)
            else [||]

        let ret = ResizeArray<int array>()

        for i in 0 .. dd.Length - 2 do
            let mutable loop = true

            while loop do
                let old = dd.[i]
                dd.[i] <- dd.[i] - 1

                if old > 0 then
                    ret.Add(rowGenerator (dd.Length - i))
                else
                    loop <- false

        if dd.Length > 0 && dd.[dd.Length - 1] <> 0 then
            ret.Add([| dd.[dd.Length - 1] |])

        ret.ToArray()

    and merge (d: int array array) : int array =
        if d.Length = 0 then
            [||]
        else
            let ret = rowGenerator d.[0].Length
            ret.[0] <- ret.[0] - 1

            for i in 0 .. d.Length - 2 do
                ret.[ret.Length - d.[i].Length] <- ret.[ret.Length - d.[i].Length] + 1

            ret.[ret.Length - d.[d.Length - 1].Length] <-
                ret.[ret.Length - d.[d.Length - 1].Length] + d.[d.Length - 1].[0]

            Array.append [| 0 |] ret

    and proc (d: int array) : int array =
        if d.Length > 2 && d.[0] = 0 && d.[1] > 0 then
            let div = divide d

            if div.Length > 1 then
                [| div.[0].Length - 1 |]
            elif div.Length = 1 && div.[0].Length > 2 then
                [| div.[0].Length - 2 |]
            else
                d
        else
            d

    and isDimensionLimited (it: Entry) (d: int array) (arena: Arena) : bool =
        let pd = proc d
        let foot = getFootRow it d arena

        if pd.Length = 1 && pd.[0] <> 0 && foot.Length > 1 && foot.[1] > pd.[0] then
            true
        elif d.Length = 2 && d.[0] = 0 && d.[1] > 0 && foot.Length > d.[1] then
            true
        else
            false

    and rowStandardization (r: int array) (d: int array) (arena: Arena) : int array =
        if r.Length <= 2 || r.[r.Length - 1] <= r.[r.Length - 2] then
            r
        else
            let s = Array.sub r 0 (r.Length - 1)
            s.[s.Length - 1] <- s.[s.Length - 1] + 1

            let seq = toSequence s arena
            let rr = expand_impl seq 2 d false arena

            if rr.Length > r.Length - 1 && rr.[r.Length - 1] <= r.[r.Length - 1] - 1 then
                rowStandardization s d arena
            else
                r

    and getFootRow (it: Entry) (d: int array) (arena: Arena) : int array =
        if isNull it.Parent then
            [| 1 |]
        elif compareRow it.Row it.Parent.Row = 0
             || d.Length = 0
             || (d.Length = 1 && d.[0] = 0)
             || (d.Length = 2 && d.[0] = 0)
             || (d.Length = 3 && d.[0] = 0 && d.[1] = 1 && d.[2] = 0) then
            Array.append it.Row [| 1 |]
        else
            let row = ResizeArray<int>()
            row.Add(1)

            let mutable i = 0
            let mutable brk = false

            while not brk do
                i <- i + 1

                if i < it.Row.Length then
                    row.Add(it.Row.[i])
                else
                    row.Add(0)

                if it.Parent.Row.Length <= i || it.Parent.Row.[i] < it.Row.[i] then
                    brk <- true

            row.[row.Count - 1] <- row.[row.Count - 1] + 1
            rowStandardization (row.ToArray()) [| 0 |] arena

    and setElementRefrence (m: ResizeArray<ResizeArray<Entry>>) : unit =
        for col in m do
            for e in col do
                if not (e.Row.Length <= 1 && e.Value <= 1) && not (isNull e.Parent) then
                    if compareRow e.Row e.Parent.Row = 0 then
                        e.Ref <- e.Parent
                    elif not (isNull e.Head)
                         && not (isNull e.Head.Parent)
                         && not (isNull e.Head.Parent.Foot)
                         && compareRow e.Head.Parent.Foot.Row e.Row <= 0 then
                        e.Ref <- e.Head.Parent.Foot

                        while not (isNull e.Ref) && compareRow e.Ref.Row e.Row = 0 do
                            e.Ref <- e.Ref.Ref
                    elif not (isNull e.Head) then
                        e.Ref <- e.Head.Parent

    and setElementNo (m: ResizeArray<ResizeArray<Entry>>) (b: Entry) : unit =
        let mutable id = 0

        for i in 0 .. m.Count - 1 do
            for j in 0 .. m.[i].Count - 1 do
                let e = m.[i].[j]

                if i = b.Cloumn then
                    e.No <- j + 1
                elif i < b.Cloumn || (e.Value <= 1 && j = 0) then
                    e.No <- 0
                elif not (isNull e.Ref) then
                    e.No <- e.Ref.No

                if i > b.Cloumn then
                    e.Id <- id
                    id <- id + 1

                if i = b.Cloumn || i = m.Count - 1 then
                    e.Id <- e.No

    and getReferenceChain (it: Entry) : Entry list =
        let c = ResizeArray<Entry>()
        let mutable cur = it
        let mutable brk = false

        while not brk do
            c.Insert(0, cur)

            if cur.Value <= 1 && cur.Row.Length = 1 then
                brk <- true
            elif isNull cur.Ref then
                brk <- true
            else
                cur <- cur.Ref

        c |> Seq.toList

    and toSequence (s: int array) (arena: Arena) : Entry list =
        let seq = ResizeArray<Entry>()

        for i in 0 .. s.Length - 1 do
            if s.[i] <= 1 then
                let root = arena.MakeRoot()
                seq.Add(arena.Make(s.[i], [||], i, 0, 0, 0, root, null, null, null))
            else
                let mutable found = false
                let mutable j = i - 1

                while not found && j >= 0 do
                    if s.[j] < s.[i] then
                        seq.Add(arena.Make(s.[i], [||], i, 0, 0, 0, seq.[j], null, null, null))
                        found <- true
                    j <- j - 1

                if not found then
                    let root = arena.MakeRoot()
                    seq.Add(arena.Make(s.[i], [||], i, 0, 0, 0, root, null, null, null))

        seq |> Seq.toList

    and drawMountain (s: Entry list) (d: int array) (arena: Arena) : ResizeArray<ResizeArray<Entry>> =
        let m = ResizeArray<ResizeArray<Entry>>()

        for e in s do
            let parent =
                if isNull e.Parent || e.Parent.Cloumn < 0 then
                    arena.MakeRoot()
                else
                    m.[e.Parent.Cloumn].[0]

            let first = arena.Make(e.Value, [| 1 |], e.Cloumn, 0, 0, 0, parent, null, null, null)
            m.Add(ResizeArray<Entry>([ first ]))

        for i in 0 .. m.Count - 1 do
            let mutable it = m.[i].[0]
            let mutable continueLoop = it.Value > 1

            while continueLoop do
                if isDimensionLimited it d arena then
                    continueLoop <- false
                else
                    let footRow = getFootRow it d arena

                    let foot =
                        arena.Make(it.Value - it.Parent.Value,
                                   footRow,
                                   i,
                                   it.Idx + 1,
                                   0,
                                   0,
                                   null,
                                   it,
                                   null,
                                   null)

                    m.[i].Add(foot)

                    let mutable p = it.Parent

                    if not (isNull p)
                       && not (isNull p.Foot)
                       && compareRow p.Foot.Row foot.Row <= 0 then
                        p <- p.Foot

                    while not (isNull p) && p.Value >= foot.Value do
                        p <- p.Parent

                    foot.Parent <- p
                    it <- foot
                    continueLoop <- it.Value > 1

        m

    and getOds (m: ResizeArray<ResizeArray<Entry>>) (arena: Arena) : Entry list =
        let o = ResizeArray<Entry>()

        for col in m do
            let last = col.[col.Count - 1]

            let parent =
                if last.Value <= 1 then
                    arena.MakeRoot()
                elif not (isNull last.Parent) && last.Parent.Cloumn >= 0 then
                    o.[last.Parent.Cloumn]
                else
                    arena.MakeRoot()

            o.Add(arena.Make(last.Value, [| 1 |], col.[0].Cloumn, 0, 0, 0, parent, null, null, null))

        o |> Seq.toList

    and getMds (m: ResizeArray<ResizeArray<Entry>>) (arena: Arena) : Entry list =
        let lastCol = m.[m.Count - 1]
        let last = lastCol.[lastCol.Count - 1]
        let chain = getReferenceChain last
        let seq = ResizeArray<int>()

        for e in chain do
            seq.Add(if e.Row.Length <= 1 then 1 else e.Row.[1])

        toSequence (seq.ToArray()) arena

    and getBootIndex (s: Entry list) (d: int array) (arena: Arena) : int * int =
        let m = drawMountain s d arena
        setElementRefrence m

        let lastCol = m.[m.Count - 1]
        let mutable t = lastCol.[lastCol.Count - 1]

        if t.Value = 1 && not (isNull t.Head) then
            t <- t.Head

        let b = t.Parent

        if not (isNull b) && t.Value - b.Value > 1 && (proc d).Length = 1 then
            let o = getOds m arena

            let new_d =
                if d.Length = 1 || (divide d).Length = 1 then
                    d
                else
                    let div = divide d
                    let sub = Array.sub div 1 (div.Length - 1)
                    merge sub

            let boot = getBootIndex o new_d arena
            let bootFirst = fst boot
            let bootSecond = m.[bootFirst].Count - 1
            (bootFirst, bootSecond)
        elif not (isNull b) && compareDimension b.Row t.Row < 0 then
            let ch = getReferenceChain t

            let dd =
                let baseDd = [| 0; 1 |]
                if d.Length > 2 && d.[0] = 0 && d.[1] = 0 then
                    Array.sub d 2 (d.Length - 2)
                elif d.Length = 3 && d.[0] = 1 && d.[1] = 0 && d.[2] = 1 then
                    d
                else
                    baseDd

            let mds = getMds m arena
            let boot = getBootIndex mds dd arena
            let bootFirst = fst boot
            let c = ch.[bootFirst].Cloumn
            let idx = m.[c].[m.[c].Count - 1].Idx
            (c, idx)
        else
            let col = if not (isNull b) then b.Cloumn else -1
            let idx = if not (isNull b) then b.Idx else 0
            (col, idx)

    and copyElement (m: ResizeArray<ResizeArray<Entry>>)
                    (b: Entry)
                    (t: Entry)
                    (it: Entry)
                    (op: ResizeArray<Entry>)
                    (i: int)
                    (d: int array)
                    (f: bool)
                    (arena: Arena) : unit =
        if it.Value <= 1 && it.Row.Length > 1 then
            it.Parent <- it.Ref

            while not (isNull it.Parent) && it.Parent.Value > 1 do
                it.Parent <- it.Parent.Ref

        let min_row =
            if it.Row.Length > 1 then
                getFootRow op.[op.Count - 1] d arena
            else
                [| 1 |]

        let mutable max_row = it.Row
        let mutable r : Entry = null

        let c =
            if f then
                it.Ref.Cloumn + (t.Cloumn - b.Cloumn) * (i + 1)
            else
                t.Cloumn + i

        r <- m.[c].[0]

        while not (isNull r.Foot) && r.Foot.Id <= it.Ref.Id do
            r <- r.Foot

        let fr =
            if isNull it.Foot then
                getFootRow it d arena
            else
                it.Foot.Row

        if compareRow (rowDifference it.Row it.Ref.Row) [| 1; 2 |] < 0 then
            max_row <- rowAddition r.Row (rowDifference it.Row it.Ref.Row)
        else
            let seq = toSequence (rowDifference fr it.Ref.Row) arena
            let expanded = expand_impl seq (it.Row.Length + i + 1) [| 0 |] false arena
            max_row <- rowAddition r.Row (rowDifference expanded it.Ref.Row)

        if it.Cloumn = t.Cloumn && it.Idx = t.Idx
           && compareRow (rowDifference it.Row it.Ref.Row) [| 1; 2 |] >= 0 then
            let p = t.Parent
            t.Parent <- b

            let seq = toSequence (rowDifference (getFootRow t d arena) t.Ref.Row) arena
            let expanded = expand_impl seq (it.Row.Length + i + 1) [| 0 |] false arena
            max_row <- rowAddition r.Row (rowDifference expanded t.Ref.Row)

            t.Parent <- p

        if d.Length = 0
           || (d.Length = 1 && d.[0] = 0)
           || (d.Length = 2 && d.[0] = 0)
           || (d.Length = 3 && d.[0] = 0 && d.[1] = 1 && d.[2] = 0) then
            max_row <- it.Row

        let mutable row = min_row

        while compareRow row max_row <= 0 do
            let e =
                arena.Make(it.Value,
                           row,
                           m.Count - 1,
                           op.Count,
                           it.No,
                           it.Id,
                           null,
                           null,
                           null,
                           null)

            op.Add(e)

            if op.Count > 1 then
                op.[op.Count - 1].Head <- op.[op.Count - 2]
                op.[op.Count - 2].Foot <- op.[op.Count - 1]

            let pc =
                if f then
                    if it.Parent.Cloumn >= b.Cloumn then
                        m.Count - 1 + it.Parent.Cloumn - it.Cloumn
                    else
                        it.Parent.Cloumn
                else
                    if it.Parent.Cloumn >= b.Cloumn then
                        m.Count - 2
                    else
                        it.Parent.Cloumn

            let mutable p =
                if pc >= 0 then
                    m.[pc].[m.[pc].Count - 1]
                else
                    arena.MakeRoot()

            while not (isNull p) && compareRow p.Row row > 0 do
                p <- p.Head

            op.[op.Count - 1].Parent <- p
            row <- getFootRow op.[op.Count - 1] d arena

    and copyCloumn (m: ResizeArray<ResizeArray<Entry>>)
                   (b: Entry)
                   (t: Entry)
                   (c: int)
                   (i: int)
                   (ex: int array)
                   (d: int array)
                   (f: bool)
                   (arena: Arena) : unit =
        let mutable it = m.[c].[0]
        m.Add(ResizeArray<Entry>())
        let new_col = m.[m.Count - 1]

        let mutable brk = false

        while not brk do
            copyElement m b t it new_col i d f arena

            if not (isNull it.Foot) then
                it <- it.Foot
            else
                brk <- true

        let new_idx = m.Count - 1
        let last = new_col.[new_col.Count - 1]
        last.Value <- (if new_idx < ex.Length then ex.[new_idx] else it.Value)

        let mutable cur = new_col.[new_col.Count - 1]

        while not (isNull cur.Head) do
            cur.Head.Value <- cur.Value + cur.Head.Parent.Value
            cur <- cur.Head

    and expand_impl (s: Entry list)
                    (n: int)
                    (d: int array)
                    (f: bool)
                    (arena: Arena) : int array =
        if s.IsEmpty then
            [||]
        else
            let last = List.last s

            if last.Value <= 1 then
                let result = ResizeArray<int>()

                for i in 0 .. s.Length - 2 do
                    result.Add(s.[i].Value)

                result.ToArray()
            else
                let m = drawMountain s d arena
                let lastCol = m.[m.Count - 1]
                let mutable t = lastCol.[lastCol.Count - 1]
                let mutable ex = [||]

                if t.Value = 1 && not (isNull t.Head) then
                    t <- t.Head

                let b = t.Parent

                setElementRefrence m
                setElementNo m b

                if not (isNull b) && t.Value - b.Value > 1 && (proc d).Length = 1 then
                    let o = getOds m arena

                    let new_d =
                        if d.Length = 1 || (divide d).Length = 1 then
                            d
                        else
                            let div = divide d
                            let sub = Array.sub div 1 (div.Length - 1)
                            merge sub

                    ex <- expand_impl o n new_d f arena
                elif not (isNull t.Foot) then
                    m.[m.Count - 1].RemoveAt(m.[m.Count - 1].Count - 1)
                    t.Foot <- null

                    if not (isNull t.Parent) then
                        t.Parent <- t.Parent.Parent

                for e in m.[m.Count - 1] do
                    e.Value <- e.Value - 1

                for i in b.No .. m.[b.Cloumn].Count - 1 do
                    let mutable idx = t.Idx
                    let src = m.[b.Cloumn].[i]
                    idx <- idx + 1

                    let e =
                        arena.Make(src.Value,
                                   src.Row,
                                   t.Cloumn,
                                   idx,
                                   src.No,
                                   src.No,
                                   src.Parent,
                                   m.[t.Cloumn].[m.[t.Cloumn].Count - 1],
                                   null,
                                   src)

                    m.[t.Cloumn].Add(e)
                    m.[t.Cloumn].[m.[t.Cloumn].Count - 2].Foot <- e

                for i in 0 .. n - 1 do
                    if f then
                        for j in b.Cloumn + 1 .. t.Cloumn do
                            copyCloumn m b t j i ex d true arena
                    else
                        copyCloumn m b t t.Cloumn i ex d false arena

                let result = ResizeArray<int>()

                for col in m do
                    result.Add(col.[0].Value)

                result.ToArray()

    and expandEntry (seq: int array) (term: int) : int array =
        if term < 1 || seq.Length = 0 then
            seq
        else
            let arena = Arena()
            let s = toSequence seq arena
            let d = [| 1 |]
            expand_impl s term d false arena


module EpsilonYNotation =

    [<Literal>]
    let Name = "\u03B5-Y" // ε-Y

    let definition : string =
        "ε-Y 记号（即 1-Y，维度序列 {1}）"

    let suffix () : string =
        ""

    let expand (seq: int[], term: int) : int[] =
        EpsilonYDetail.expandEntry seq term