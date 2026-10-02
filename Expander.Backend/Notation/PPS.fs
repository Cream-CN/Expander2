namespace Expander_CS.Backend.Notation

module internal PpsDetail =

    type PPS4Variant =
        | PPS4
        | WPPS4
        | TPPS4
        | EWPPS4

    let inline isSuccessor (seq: int[]) =
        seq.Length > 0 && seq.[seq.Length - 1] = 0

    let inline dropLast (seq: int[]) =
        if seq.Length = 0 then [||]
        else Array.sub seq 0 (seq.Length - 1)

    let inline copySeq (seq: int[]) = Array.copy seq

    let expandPps1 (seq: int[], n: int) : int[] =
        if seq.Length = 0 then
            [||]
        elif isSuccessor seq then
            dropLast seq
        else
            let y = seq.Length
            let x = seq.[y - 1]
            if x <= 0 || x > y then
                copySeq seq
            else
                let badRootIndex = x - 1
                let b = seq.[badRootIndex]
                let L = y - x
                if L <= 0 then
                    copySeq seq
                else
                    let res = ResizeArray<int>(seq.[0 .. y - 2])
                    let mutable weak = false
                    let mutable i = badRootIndex + 1
                    while i < y - 1 && not weak do
                        if seq.[i] = b then weak <- true
                        i <- i + 1

                    let newLast = if weak then b else seq.[y - 1] - 1
                    res.Add(newLast)

                    let mutable targetLen = y + n * L - 1
                    if targetLen < y then targetLen <- y

                    let mutable i = x + 1
                    while i <= targetLen - L do
                        let mutable skip = false
                        let valI =
                            if i < y then
                                seq.[i - 1]
                            elif i = y then
                                res.[y - 1]
                            else
                                if i - 1 < res.Count then
                                    res.[i - 1]
                                else
                                    skip <- true
                                    0
                        if not skip then
                            let newVal = if valI >= x then valI + L else valI
                            let targetIndex = i + L
                            if targetIndex <= targetLen then
                                if targetIndex - 1 >= res.Count then
                                    while res.Count < targetIndex do
                                        res.Add(0)
                                res.[targetIndex - 1] <- newVal
                        i <- i + 1

                    res.ToArray()

    let expandPps4Variant (seq: int[], n: int, variant: PPS4Variant) : int[] =
        if seq.Length = 0 then
            [||]
        elif isSuccessor seq then
            dropLast seq
        else
            let y = seq.Length
            let x = seq.[y - 1]
            if x <= 0 || x > y then
                copySeq seq
            else
                let badRootIndex = x - 1
                let b = seq.[badRootIndex]
                let L = y - x
                if L <= 0 then
                    copySeq seq
                else
                    let res = ResizeArray<int>(seq.[0 .. y - 2])
                    let mutable weak = false
                    let mutable i = badRootIndex + 1
                    while i < y - 1 && not weak do
                        if seq.[i] = b then weak <- true
                        i <- i + 1

                    let mutable newLast = b
                    let mutable strongExpand = false

                    if not weak then
                        let mutable foundCol = -1
                        match variant with
                        | EWPPS4 ->
                            let mutable candidate = x - 2
                            let mutable stop = false
                            while candidate >= b && foundCol = -1 && not stop do
                                if candidate >= 0 && candidate < y then
                                    let v = seq.[candidate]
                                    if v = b then
                                        foundCol <- candidate + 1
                                    elif v < b then
                                        stop <- true
                                if not stop then
                                    candidate <- candidate - 1
                        | WPPS4 ->
                            let mutable candidate = x - 2
                            while candidate >= b && foundCol = -1 do
                                if candidate >= 0 && candidate < y then
                                    if seq.[candidate] = b then
                                        foundCol <- candidate + 1
                                candidate <- candidate - 1
                        | _ ->
                            let mutable candidate = x - 2
                            while candidate >= b && foundCol = -1 do
                                if candidate >= 0 && candidate < y then
                                    if seq.[candidate] <= b then
                                        foundCol <- candidate + 1
                                candidate <- candidate - 1

                        if foundCol <> -1 then
                            newLast <- foundCol
                            strongExpand <- (variant = TPPS4)
                        else
                            newLast <- b

                    res.Add(newLast)

                    if strongExpand then
                        let totalLen = y + n * L
                        let out = ResizeArray<int>(totalLen)
                        for i in 0 .. y - 2 do
                            out.Add(seq.[i])
                        out.Add(newLast)

                        let mutable position = y + 1
                        while position <= totalLen do
                            let isLastCopy = position > y && ((position - y) % L = 0)
                            if isLastCopy then
                                let copyNumber = (position - y) / L
                                out.Add(newLast + copyNumber * L)
                            else
                                let sourcePosition = position - L
                                let sourceValue = out.[sourcePosition - 1]
                                out.Add(if sourceValue >= x then sourceValue + L else sourceValue)
                            position <- position + 1
                        out.ToArray()
                    else
                        let mutable targetLen = y + n * L - 1
                        if targetLen < y then targetLen <- y

                        let mutable i = x + 1
                        while i <= targetLen - L do
                            let mutable skip = false
                            let valI =
                                if i < y then
                                    seq.[i - 1]
                                elif i = y then
                                    res.[y - 1]
                                else
                                    if i - 1 < res.Count then
                                        res.[i - 1]
                                    else
                                        skip <- true
                                        0
                            if not skip then
                                let newVal = if valI >= x then valI + L else valI
                                let targetIndex = i + L
                                if targetIndex <= targetLen then
                                    if targetIndex - 1 >= res.Count then
                                        while res.Count < targetIndex do
                                            res.Add(0)
                                    res.[targetIndex - 1] <- newVal
                            i <- i + 1
                        res.ToArray()

    let expandSecondPps4 (seq: int[], count: int) : int[] =
        if seq.Length = 0 then
            [||]
        else
            let y = seq.Length
            let x = seq.[y - 1]
            if x <= 0 then
                dropLast seq
            elif x > y then
                copySeq seq
            else
                let b = seq.[x - 1]
                let L = y - x
                if L <= 0 then
                    copySeq seq
                else
                    let mutable value = b
                    let mutable strongExpand = false
                    let mutable foundLessOrEqual = false

                    let mutable column = y - 1
                    while column >= x + 1 && not foundLessOrEqual do
                        if seq.[column - 1] <= b then
                            foundLessOrEqual <- true
                        column <- column - 1

                    if not foundLessOrEqual then
                        let mutable foundColumn = -1
                        let strongStart = b + 1
                        let strongEnd = x - 1
                        if strongStart <= strongEnd then
                            let mutable candidate = strongEnd
                            while candidate >= strongStart && foundColumn = -1 do
                                if seq.[candidate - 1] = b then
                                    foundColumn <- candidate
                                candidate <- candidate - 1
                        if foundColumn <> -1 then
                            value <- foundColumn
                            strongExpand <- true

                    let totalLength = y + count * L - 1
                    if totalLength <= 0 then
                        [||]
                    else
                        let result = Array.zeroCreate<int> totalLength
                        for i in 0 .. x - 1 do
                            result.[i] <- seq.[i]
                        for i in x .. y - 2 do
                            result.[i] <- seq.[i]
                        result.[y - 1] <- value

                        for i in x .. y - 1 do
                            let baseValue = if i = y - 1 then value else seq.[i]
                            let shifts = if i = y - 1 then count - 1 else count
                            let mutable copy = 1
                            while copy <= shifts do
                                let position = i + copy * L
                                if position < totalLength then
                                    if (i = y - 1 && strongExpand) || baseValue >= x then
                                        result.[position] <- baseValue + copy * L
                                    else
                                        result.[position] <- baseValue
                                copy <- copy + 1
                        result

    let expand2Pps4 (seq: int[], n: int) : int[] =
        if seq.Length = 0 then
            [||]
        else
            let y = seq.Length
            let x = seq.[y - 1]
            if n = 0 then
                dropLast seq
            elif x <= 0 then
                dropLast seq
            elif x > y then
                copySeq seq
            elif y = 2 && seq.[0] = 0 && seq.[1] = 2 then
                Array.init (n + 1) id
            else
                let b = seq.[x - 1]
                let L = y - x
                if L <= 0 then
                    copySeq seq
                else
                    let mutable equalCount = 0
                    for col in x + 1 .. y - 1 do
                        if seq.[col - 1] = b then
                            equalCount <- equalCount + 1

                    let v =
                        if equalCount >= 2 then
                            b
                        else
                            let mutable foundCol = -1
                            let strongStart = b + 1
                            let strongEnd = x - 1
                            if strongStart <= strongEnd then
                                let mutable col = strongEnd
                                while col >= strongStart && foundCol = -1 do
                                    if seq.[col - 1] <= b then
                                        foundCol <- col
                                    col <- col - 1
                            if foundCol <> -1 then foundCol else b

                    let totalLen = y + n * L - 1
                    if totalLen <= 0 then
                        [||]
                    else
                        let res = Array.zeroCreate<int> totalLen
                        for i in 0 .. x - 1 do
                            res.[i] <- seq.[i]
                        for i in x .. y - 2 do
                            res.[i] <- seq.[i]
                        res.[y - 1] <- v

                        for i in x .. y - 1 do
                            let baseVal = if i = y - 1 then v else seq.[i]
                            let ge = baseVal >= x
                            let maxK = if i = y - 1 then n - 1 else n
                            let mutable k = 1
                            while k <= maxK do
                                let pos = i + k * L
                                if pos < totalLen then
                                    res.[pos] <- if ge then baseVal + k * L else baseVal
                                k <- k + 1
                        res


module PPSNotation =
    [<Literal>]
    let Name = "PPS"
    let definition : string = "PPS"
    let expand (seq: int[], term: int) : int[] = PpsDetail.expandPps1(seq, term)
    let suffix () : string = ""

module PPS4Notation =
    [<Literal>]
    let Name = "PPS4"
    let definition : string = "PPS4"
    let expand (seq: int[], term: int) : int[] =
        PpsDetail.expandPps4Variant(seq, term, PpsDetail.PPS4Variant.PPS4)
    let suffix () : string = ""

module WPPS4Notation =
    [<Literal>]
    let Name = "Weak PPS4"
    let definition : string = "Weak PPS4"
    let expand (seq: int[], term: int) : int[] =
        PpsDetail.expandPps4Variant(seq, term, PpsDetail.PPS4Variant.WPPS4)
    let suffix () : string = ""

module TPPS4Notation =
    [<Literal>]
    let Name = "Third PPS4"
    let definition : string = "Third PPS4"
    let expand (seq: int[], term: int) : int[] =
        PpsDetail.expandPps4Variant(seq, term, PpsDetail.PPS4Variant.TPPS4)
    let suffix () : string = ""

module EWPPS4Notation =
    [<Literal>]
    let Name = "Extremely Weak PPS4"
    let definition : string = "Extremely Weak PPS4"
    let expand (seq: int[], term: int) : int[] =
        PpsDetail.expandPps4Variant(seq, term, PpsDetail.PPS4Variant.EWPPS4)
    let suffix () : string = ""

module SecondPPS4Notation =
    [<Literal>]
    let Name = "Second PPS4"
    let definition : string = "Second PPS4"
    let expand (seq: int[], term: int) : int[] =
        PpsDetail.expandSecondPps4(seq, term)
    let suffix () : string = ""

module PPS2Notation =
    [<Literal>]
    let Name = "2-pps4"
    let definition : string = "2-pps4"
    let expand (seq: int[], term: int) : int[] =
        PpsDetail.expand2Pps4(seq, term)
    let suffix () : string = ""