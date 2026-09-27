namespace Expander_CS.Backend.Notation

module EmptyNotation =

    [<Literal>]
    let Name = "空记号"

    /// 定义文本；当前为占位，后续版本会替换。
    let definition : string =
        "空记号：对任意输入序列原样返回，不做任何数学展开。"

    /// 原样返回 seq，忽略 term。
    /// - 纯函数，不修改入参
    /// - seq 为 null 时返回 [||]
    /// - 不抛异常
    let expand (seq: int[], term: int) : int[] =
        if isNull seq then [||] else seq

    /// 无附加后缀。
    let suffix () : string = ""

    /// 不支持文本展开；C# 注册表中对应项留 null 即可。
    let expandText (text: string, term: int) : string = null