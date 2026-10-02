VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Omega-Y Expander (VB6)"
   ClientHeight    =   6060
   ClientLeft      =   120
   ClientTop       =   450
   ClientWidth     =   6480
   ScaleHeight     =   6060
   ScaleWidth      =   6480
   StartUpPosition =   2  'CenterScreen
   Begin VB.ComboBox ComboNotation 
      Height          =   315
      Left            =   1680
      Style           =   2  'Dropdown List
      TabIndex        =   2
      Top             =   1440
      Width           =   3615
   End
   Begin VB.CommandButton BtnFSalter 
      Caption         =   "Keep Last"
      Height          =   405
      Left            =   2400
      TabIndex        =   4
      Top             =   1920
      Width           =   1695
   End
   Begin VB.CommandButton BtnFS 
      Caption         =   "Drop Last"
      Height          =   405
      Left            =   240
      TabIndex        =   3
      Top             =   1920
      Width           =   1695
   End
   Begin VB.TextBox EditTerm 
      Height          =   315
      Left            =   960
      TabIndex        =   1
      Text            =   "1"
      Top             =   960
      Width           =   975
   End
   Begin VB.TextBox EditSeq 
      Height          =   315
      Left            =   240
      TabIndex        =   0
      Text            =   "1,2,3"
      Top             =   480
      Width           =   5055
   End
   Begin VB.TextBox RichDef 
      Height          =   3015
      Left            =   240
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   8
      Top             =   2520
      Width           =   6015
   End
   Begin VB.Label LblSeq 
      Caption         =   "Sequence (comma separated):"
      Height          =   255
      Left            =   240
      TabIndex        =   5
      Top             =   240
      Width           =   2415
   End
   Begin VB.Label LblTerm 
      Caption         =   "Term:"
      Height          =   255
      Left            =   240
      TabIndex        =   6
      Top             =   960
      Width           =   735
   End
   Begin VB.Label LblNotation 
      Caption         =   "Notation:"
      Height          =   255
      Left            =   240
      TabIndex        =   7
      Top             =   1440
      Width           =   735
   End
   Begin VB.Menu MnuFile 
      Caption         =   "&File"
      Begin VB.Menu MnuExit 
         Caption         =   "E&xit"
      End
   End
   Begin VB.Menu MnuDef 
      Caption         =   "&Definitions"
      Begin VB.Menu MnuDefItem 
         Caption         =   ""
         Index           =   0
         Visible         =   0   'False
      End
   End
   Begin VB.Menu MnuHelp 
      Caption         =   "&Help"
      Begin VB.Menu MnuHelpCmd 
         Caption         =   "&Help..."
      End
      Begin VB.Menu MnuAbout 
         Caption         =   "&About..."
      End
      Begin VB.Menu MnuLegal 
         Caption         =   "&Legal..."
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

' ===================================================================
'  Form1.frm -- Main window
' ===================================================================

Private Sub Form_Load()
    Dim i As Long, n As Long

    ComboNotation.Left = 240

    n = NotationCount()
    For i = 0 To n - 1
        ComboNotation.AddItem NotationName(i)
    Next i
    If n > 0 Then ComboNotation.ListIndex = 0

    For i = 0 To n - 1
        Dim lbl As String
        lbl = NotationName(i) & " definition (&" & Chr$(Asc("A") + (i Mod 26)) & ")"
        If i > 0 Then Load MnuDefItem(i)
        MnuDefItem(i).Caption = lbl
        MnuDefItem(i).Visible = True
    Next i

    RichDef.Font.Name = "Microsoft YaHei UI"
    RichDef.Font.Size = 10
    RichDef.Locked = True

    ShowDefinition 0
End Sub

Private Sub Form_Resize()
    On Error Resume Next
    Dim w As Long, h As Long
    w = ScaleWidth
    h = ScaleHeight
    If w < 1000 Or h < 1000 Then Exit Sub

    EditSeq.Width = w - 480
    ComboNotation.Width = w - 480
    RichDef.Width = w - 480
    RichDef.Height = h - RichDef.Top - 240
End Sub

Private Sub ComboNotation_Click()
    ShowDefinition ComboNotation.ListIndex
End Sub

Private Sub BtnFS_Click()
    DoExpand True
End Sub

Private Sub BtnFSalter_Click()
    DoExpand False
End Sub

Private Sub MnuExit_Click()
    Unload Me
End Sub

Private Sub MnuDefItem_Click(Index As Integer)
    ShowDefinitionPopup Index
End Sub

Private Sub MnuHelpCmd_Click()
    MsgBox "Credits" & vbCrLf & _
           "Some Hypcos notation code was adapted from notation-explorer." & vbCrLf & _
           "Some SmileLee-lyx notation code was adapted from NER." & vbCrLf & _
           "The definitions of Cao Zhiqiu's notations were taken from the original text of the book on large numbers." & vbCrLf & _
           "The MrSS definition comes from AAA (QQ 3682911373)." & vbCrLf & _
           "The epsilon-Y code was adapted from Go men's code." & vbCrLf & _
           "Note: this code was generated with the help of AI. The author does not guarantee that the expansion results are correct.", _
           vbInformation, "Help"
End Sub

Private Sub MnuAbout_Click()
    MsgBox "Omega-Y Expander" & vbCrLf & vbCrLf & "By Cream-CN", _
           vbInformation, "About"
End Sub

Private Sub MnuLegal_Click()
    MsgBox "Unlicense" & vbCrLf & vbCrLf & _
           "This is free and unencumbered software released into the public domain." & _
           vbCrLf & vbCrLf & _
           "For more information, please refer to <https://unlicense.org/>", _
           vbInformation, "Legal"
End Sub

Private Sub ShowDefinition(ByVal idx As Long)
    If idx < 0 Or idx >= NotationCount() Then Exit Sub
    RichDef.Text = "[" & NotationName(idx) & "]" & vbCrLf & vbCrLf & _
                   NotationDefinition(idx)
    RichDef.SelStart = 0
End Sub

Private Sub ShowDefinitionPopup(ByVal idx As Long)
    ' Requires Form2 (with a TextBox named TxtDef). Comment out until then.
    Form1.Caption = "Definition - " & NotationName(idx)
    Form1.TxtDef.Text = NotationDefinition(idx)
    Form1.Show vbModal
End Sub

Private Sub DoExpand(ByVal removeLast As Boolean)
    Dim s As String
    s = Trim$(EditSeq.Text)
    If Len(s) = 0 Then
        MsgBox "Sequence must not be empty.", vbCritical, "Error"
        Exit Sub
    End If

    Dim term As Long
    On Error Resume Next
    term = CLng(Trim$(EditTerm.Text))
    If Err.Number <> 0 Then
        Err.Clear
        MsgBox "Term must be an integer.", vbCritical, "Error"
        Exit Sub
    End If
    On Error GoTo 0
    If term < 1 Then
        MsgBox "Term must be a positive integer.", vbCritical, "Error"
        Exit Sub
    End If

    Dim idx As Long
    idx = ComboNotation.ListIndex
    If idx < 0 Or idx >= NotationCount() Then idx = 0

    Dim result As String
    result = NotationExpand(idx, s, term)

    If removeLast Then
        Dim arr() As Long
        arr = ParseSequence(result)
        If UBound(arr) >= LBound(arr) Then
            If UBound(arr) >= 1 Then
                Dim shorter() As Long
                ReDim shorter(0 To UBound(arr) - 1)
                Dim i As Long
                For i = 0 To UBound(shorter)
                    shorter(i) = arr(i)
                Next i
                result = SeqToString(shorter)
            End If
        End If
    End If

    result = result & NotationSuffix(idx)
    MsgBox result, vbInformation, "Expansion Result"
End Sub
