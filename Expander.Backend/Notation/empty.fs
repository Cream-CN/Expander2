namespace Expander_CS.Backend.Notation

module EmptyNotation =

    [<Literal>]
    let Name = "空记号"
    let definition : string =
        "原样返回输入"
    let expand (seq: int[], term: int) : int[] =
        if isNull seq then [||] else seq
    let suffix () : string = ""
    let expandText (text: string, term: int) : string = null