Attribute VB_Name = "Module2"
Attribute VB_Name = "expand"
Option Explicit

Public Sub Main()
    Form1.Show
End Sub

Public Function NotationCount() As Long
    NotationCount = 1
End Function

Public Function NotationName(ByVal idx As Long) As String
    Select Case idx
        Case 0: NotationName = "Empty Notation"
        Case Else: NotationName = ""
    End Select
End Function

Public Function NotationId(ByVal idx As Long) As String
    Select Case idx
        Case 0: NotationId = "empty"
        Case Else: NotationId = ""
    End Select
End Function

Public Function NotationDefinition(ByVal idx As Long) As String
    Select Case idx
        Case 0
            NotationDefinition = "empty" & vbCrLf & vbCrLf & _
                                 "Empty"
        Case Else
            NotationDefinition = ""
    End Select
End Function

Public Function NotationExpand(ByVal idx As Long, _
                               ByVal seq As String, _
                               ByVal term As Long) As String
    Select Case idx
        Case 0: NotationExpand = EmptyNotation_Expand(seq, term)
        Case Else: NotationExpand = ""
    End Select
End Function

Public Function NotationSuffix(ByVal idx As Long) As String
    Select Case idx
        Case 0: NotationSuffix = EmptyNotation_Suffix()
        Case Else: NotationSuffix = ""
    End Select
End Function

