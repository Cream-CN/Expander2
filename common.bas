Attribute VB_Name = "common"
Option Explicit

Public Type Entry
    value         As Long
    x             As Long
    y             As Variant
    ykey          As Currency
    leftleg_up    As Variant
    rightleg_up   As Long
    rightleg_down As Long
    leftleg_down  As Long
End Type

Public Function WStringToUtf8(ByVal s As String) As String
    WStringToUtf8 = s
End Function

Public Function Utf8ToWString(ByVal s As String) As String
    Utf8ToWString = s
End Function

' ---- sequence -----------------------------------------------------

Public Function ParseSequence(ByVal s As String) As Long()
    If Len(s) = 0 Then
        Dim emptyArr() As Long
        ParseSequence = emptyArr
        Exit Function
    End If

    Dim parts() As String
    parts = Split(s, ",")

    Dim result() As Long
    ReDim result(0 To UBound(parts))

    Dim n As Long
    n = 0

    Dim i As Long
    For i = 0 To UBound(parts)
        Dim tok As String
        tok = Trim$(parts(i))
        If Len(tok) > 0 Then
            Dim v As Long
            On Error Resume Next
            v = CLng(tok)
            If Err.Number = 0 Then
                result(n) = v
                n = n + 1
            End If
            Err.Clear
            On Error GoTo 0
        End If
    Next i

    If n = 0 Then
        Dim emptyArr2() As Long
        ParseSequence = emptyArr2
        Exit Function
    End If

    ReDim Preserve result(0 To n - 1)
    ParseSequence = result
End Function

Public Function SeqToString(seq() As Long) As String
    Dim lo As Long, hi As Long
    lo = LBound(seq): hi = UBound(seq)
    If hi < lo Then
        SeqToString = "[]"
        Exit Function
    End If

    Dim r As String
    Dim i As Long
    For i = lo To hi
        If i > lo Then r = r & ","
        r = r & CStr(seq(i))
    Next i
    SeqToString = r
End Function

Public Type Entry
    value         As Long
    x             As Long
    y             As Variant
    ykey          As Currency
    leftleg_up    As Variant
    rightleg_up   As Long
    rightleg_down As Long
    leftleg_down  As Long
End Type

Public Function MakeYKey(ByVal y As Variant) As Currency
    Dim lo As Long, hi As Long
    On Error Resume Next
    lo = LBound(y): hi = UBound(y)
    On Error GoTo 0

    Dim sz As Long
    If hi < lo Then
        sz = 0
    Else
        sz = hi - lo + 1
    End If

    MakeYKey = CCur(sz And &HFF&) * 100000000000000#
End Function

Public Sub EntryRefreshKey(ByRef e As Entry)
    e.ykey = MakeYKey(e.y)
End Sub

Public Function MakeEntry(ByVal v As Long, ByVal x As Long, ByRef y As Variant) As Entry
    Dim e As Entry
    e.value = v
    e.x = x
    e.y = y
    EntryRefreshKey e
    MakeEntry = e
End Function

' ---- arena --------------------------------------------------------
' VB6 has no unique_ptr<vector>, so a module-level Collection holds
' all entries. Index is 1-based, matching Collection semantics.

Private m_arena As Collection
Private m_arenaReady As Boolean

Public Sub ArenaInit()
    Set m_arena = New Collection
    m_arenaReady = True
End Sub

Public Sub ArenaClear()
    If m_arenaReady Then Set m_arena = New Collection
End Sub

Public Function ArenaSize() As Long
    If m_arenaReady Then ArenaSize = m_arena.Count Else ArenaSize = 0
End Function

Public Function ArenaMake(ByVal v As Long, ByVal x As Long, ByRef y As Variant) As Long
    If Not m_arenaReady Then ArenaInit
    Dim e As Entry
    e = MakeEntry(v, x, y)
    m_arena.Add e
    ArenaMake = m_arena.Count
End Function

Public Function ArenaMakeEmpty() As Long
    If Not m_arenaReady Then ArenaInit
    Dim e As Entry
    m_arena.Add e
    ArenaMakeEmpty = m_arena.Count
End Function

Public Sub ArenaGet(ByVal idx As Long, ByRef outEntry As Entry)
    If m_arenaReady And idx >= 1 And idx <= m_arena.Count Then
        outEntry = m_arena(idx)
    End If
End Sub

' Value types cannot be replaced in place inside a Collection, so we
' rebuild the Collection with the new element at the same position.
Public Sub ArenaSet(ByVal idx As Long, ByRef e As Entry)
    If Not m_arenaReady Then Exit Sub
    If idx < 1 Or idx > m_arena.Count Then Exit Sub

    Dim tmp As Collection
    Dim i As Long

    Set tmp = New Collection
    For i = 1 To m_arena.Count
        If i = idx Then
            tmp.Add e
        Else
            tmp.Add m_arena(i)
        End If
    Next i
    Set m_arena = tmp
End Sub
