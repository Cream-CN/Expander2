namespace Expander_CS.Backend.Notation

module PrSSNotation =
    [<Literal>]
    let Name = "PrSS"

    let definition : string =
        "PrSS（Primitive Sequence System，原始序列系统）：以逗号分隔的非负整数序列。展开 term 项时：若序列为空返回空；若末项为 0，删除末项；否则令 k 为末项下标，寻找最大的 i<k 使 seq[i] < seq[k]。若不存在，则末项减 1 并追加 term 个 0；若存在，令 ai=seq[i]，取块 seq[i+1..k-1]，删除末项，然后重复 term 次追加 ai 与该块。"

    let expand (seq: int[], term: int) : int[] =
        if isNull seq then
            [||]
        elif seq.Length = 0 then
            [||]
        elif term <= 0 then
            Array.copy seq
        else
            let result = ResizeArray<int>(seq)
            let k = result.Count - 1

            if result.[k] = 0 then
                result.RemoveAt(k)
                result.ToArray()
            else
                let mutable i = -1
                let mutable j = k - 1

                while j >= 0 && i = -1 do
                    if result.[j] < result.[k] then
                        i <- j
                    j <- j - 1

                if i = -1 then
                    let ak = result.[k]
                    result.RemoveAt(k)
                    result.Add(ak - 1)

                    for _ in 1..term do
                        result.Add(0)

                    result.ToArray()
                else
                    let ai = result.[i]
                    let block = result.GetRange(i + 1, k - i - 1).ToArray()

                    result.RemoveAt(k)

                    for _ in 1..term do
                        result.Add(ai)
                        result.AddRange(block)

                    result.ToArray()

    let suffix () : string = " [PrSS]"