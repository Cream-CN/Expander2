using System;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Expander_CS.Backend.Common;
using Expander_CS.Backend.Notation;

namespace omegay
{
	public class Form1 : Form
	{
		// 控件
		private MenuStrip menuStrip;
		private ToolStripMenuItem menuFile, menuExit;
		private ToolStripMenuItem menuDefinition;
		private ToolStripMenuItem menuHelp, menuHelpItem, menuAbout, menuLegal;
		private Label lblSeq, lblTerm, lblNotation;
		private TextBox txtSeq, txtTerm;
		private ComboBox cmbNotation;
		private Button btnFS, btnFSalter;
		private RichTextBox rtbDef;

		// 记号名列表（占位，只用于 UI 展示）
		public sealed class NotationInfo
		{
			public string DisplayName { get; init; } = "";
			public string Definition { get; init; } = "";
			public Func<int[], int, int[]>? Expand { get; init; }
			public Func<string>? Suffix { get; init; }
			public Func<string, int, string>? ExpandText { get; init; }
		}

		private readonly NotationInfo[] _notations =
		{
			new NotationInfo
			{
				DisplayName = EmptyNotation.Name,
				Definition  = EmptyNotation.definition,
				Expand      = EmptyNotation.expand,
				Suffix      = EmptyNotation.suffix,
				ExpandText  = null,
			},
			new NotationInfo
			{
				DisplayName = PrSSNotation.Name,
				Definition  = PrSSNotation.definition,
				Expand      = PrSSNotation.expand,
				Suffix      = PrSSNotation.suffix,
				ExpandText  = null, // PrSS 不支持文本展开
			},
			new NotationInfo
			{
				DisplayName = EpsilonYNotation.Name,
				Definition  = EpsilonYNotation.definition,
				Expand      = EpsilonYNotation.expand,
				Suffix      = EpsilonYNotation.suffix,
				ExpandText  = null, // ε-Y 不支持文本展开
			},
			new NotationInfo
			{
				DisplayName = OmegaYNotation.Name,
				Definition  = OmegaYNotation.definition,
				Expand      = OmegaYNotation.expandAlter,
				Suffix      = OmegaYNotation.suffix,
				ExpandText  = null,
			},

			new NotationInfo
			{
				DisplayName = OmegaYMediumNotation.Name,
				Definition  = OmegaYMediumNotation.definition,
				Expand      = OmegaYMediumNotation.expand,
				Suffix      = OmegaYMediumNotation.suffix,
				ExpandText  = null,
			},
			new NotationInfo
			{
				DisplayName = OmegaYStrongNotation.Name,
				Definition  = OmegaYStrongNotation.definition,
				Expand      = OmegaYStrongNotation.expand,
				Suffix      = OmegaYStrongNotation.suffix,
				ExpandText  = null,
			},
		};

		public Form1()
		{
			BuildUi();
		}

		private void BuildUi()
		{
			SuspendLayout();

			Text = "ω-Y 展开器 (重构中)";
			ClientSize = new Size(384, 361);
			FormBorderStyle = FormBorderStyle.FixedSingle;
			MaximizeBox = false;
			StartPosition = FormStartPosition.CenterScreen;

			const int mh = 25;

			// ---------- 菜单栏 ----------
			menuStrip = new MenuStrip { Dock = DockStyle.Top };

			menuFile = new ToolStripMenuItem("文件(&F)");
			menuExit = new ToolStripMenuItem("退出(&X)");
			menuExit.Click += (s, e) => Application.Exit();
			menuFile.DropDownItems.Add(menuExit);

			menuDefinition = new ToolStripMenuItem("定义(&D)");
			{
				var menuDefPrSS = new ToolStripMenuItem("PrSS");
				menuDefPrSS.Click += MenuDefinitionPrSS_Click;
				menuDefinition.DropDownItems.Add(menuDefPrSS);

				var menuDefEpsilonY = new ToolStripMenuItem("ε-Y");
				menuDefEpsilonY.Click += MenuDefinitionEpsilonY_Click;
				menuDefinition.DropDownItems.Add(menuDefEpsilonY);

				var menuDefOmegaY = new ToolStripMenuItem("ω-Y");
				menuDefOmegaY.Click += MenuDefinitionOmegaY_Click;
				menuDefinition.DropDownItems.Add(menuDefOmegaY);

				var menuDefOmegaYMedium = new ToolStripMenuItem("ω-Y (medium magma)");
				menuDefOmegaYMedium.Click += MenuDefinitionOmegaYMedium_Click;
				menuDefinition.DropDownItems.Add(menuDefOmegaYMedium);

				var menuDefOmegaYStrong = new ToolStripMenuItem("ω-Y (strong magma)");
				menuDefOmegaYStrong.Click += MenuDefinitionOmegaYStrong_Click;
				menuDefinition.DropDownItems.Add(menuDefOmegaYStrong);
			}

			menuHelp = new ToolStripMenuItem("帮助(&H)");
			menuHelpItem = new ToolStripMenuItem("帮助(&H)...");
			menuHelpItem.Click += MenuHelpItem_Click;
			menuAbout = new ToolStripMenuItem("关于(&A)...");
			menuAbout.Click += MenuAbout_Click;
			menuLegal = new ToolStripMenuItem("法律声明(&L)...");
			menuLegal.Click += MenuLegal_Click;
			menuHelp.DropDownItems.Add(menuHelpItem);
			menuHelp.DropDownItems.Add(menuAbout);
			menuHelp.DropDownItems.Add(menuLegal);

			menuStrip.Items.Add(menuFile);
			menuStrip.Items.Add(menuDefinition);
			menuStrip.Items.Add(menuHelp);

			// ---------- 控件 ----------
			lblSeq = new Label
			{
				Text = "序列 (用逗号分隔):",
				Location = new Point(10, mh + 10),
				Size = new Size(150, 20),
			};
			txtSeq = new TextBox
			{
				Text = "1,2,3",
				Location = new Point(10, mh + 30),
				Size = new Size(200, 20),
			};

			lblTerm = new Label
			{
				Text = "项数:",
				Location = new Point(10, mh + 55),
				Size = new Size(50, 20),
			};
			txtTerm = new TextBox
			{
				Text = "1",
				Location = new Point(60, mh + 55),
				Size = new Size(80, 20),
			};

			lblNotation = new Label
			{
				Text = "记号:",
				Location = new Point(10, mh + 85),
				Size = new Size(80, 20),
			};
			cmbNotation = new ComboBox
			{
				DropDownStyle = ComboBoxStyle.DropDownList,
				Location = new Point(100, mh + 85),
				Size = new Size(220, 20),
			};
			foreach (var n in _notations) cmbNotation.Items.Add(n.DisplayName);
			if (cmbNotation.Items.Count > 0) cmbNotation.SelectedIndex = 0;

			btnFS = new Button
			{
				Text = "移除末项",
				Location = new Point(10, mh + 115),
				Size = new Size(120, 25),
			};
			btnFS.Click += BtnFS_Click;

			btnFSalter = new Button
			{
				Text = "保留末项",
				Location = new Point(140, mh + 115),
				Size = new Size(120, 25),
			};
			btnFSalter.Click += BtnFSalter_Click;

			rtbDef = new RichTextBox
			{
				Location = new Point(10, mh + 150),
				Size = new Size(360, 110),
				ReadOnly = true,
				ScrollBars = RichTextBoxScrollBars.Vertical,
				Font = new Font("Microsoft YaHei UI", 10f),
			};

			// ---------- 下拉框联动 definition ----------
			cmbNotation.SelectedIndexChanged += (s, e) =>
			{
				int idx = cmbNotation.SelectedIndex;
				if (idx >= 0 && idx < _notations.Length)
					rtbDef.Text = _notations[idx].Definition;
			};
			if (cmbNotation.SelectedIndex >= 0)
				rtbDef.Text = _notations[cmbNotation.SelectedIndex].Definition;

			// ---------- 加入窗体 ----------
			Controls.Add(rtbDef);
			Controls.Add(btnFSalter);
			Controls.Add(btnFS);
			Controls.Add(cmbNotation);
			Controls.Add(lblNotation);
			Controls.Add(txtTerm);
			Controls.Add(lblTerm);
			Controls.Add(txtSeq);
			Controls.Add(lblSeq);
			Controls.Add(menuStrip);

			MainMenuStrip = menuStrip;

			ResumeLayout(false);
			PerformLayout();
		}

		// ================= 按钮事件 =================
		private void BtnFS_Click(object sender, EventArgs e) => RunExpand(removeLast: true);
		private void BtnFSalter_Click(object sender, EventArgs e) => RunExpand(removeLast: false);

		private void RunExpand(bool removeLast)
		{
			string seqText = txtSeq.Text;
			string termText = txtTerm.Text.Trim();

			if (string.IsNullOrEmpty(seqText))
			{
				MessageBox.Show(this, "序列不能为空", "错误",
					MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}
			if (!int.TryParse(termText, out int term))
			{
				MessageBox.Show(this, "项数必须是整数", "错误",
					MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}
			if (term < 1)
			{
				MessageBox.Show(this, "项数必须为正整数", "错误",
					MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}

			int[] seq = Sequence.parseSequence(seqText);
			if (seq.Length == 0)
			{
				MessageBox.Show(this, "序列中没有可解析的整数", "错误",
					MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}

			int sel = cmbNotation.SelectedIndex;
			if (sel < 0 || sel >= _notations.Length) sel = 0;
			var info = _notations[sel];

			int[] result = info.Expand!(seq, term);

			if (removeLast && result.Length > 1)
				Array.Resize(ref result, result.Length - 1);

			string body = Sequence.seqToString(result);
			string tail = info.Suffix?.Invoke() ?? "";
			string final = body + tail;

			MessageBox.Show(this, final, "展开结果",
				MessageBoxButtons.OK, MessageBoxIcon.Information);
		}

		// ================= 菜单事件 =================
		private void MenuDefinitionPrSS_Click(object? sender, EventArgs e)
		{
			ShowDefinitionDialog("PrSS 定义", PrSSNotation.definition);
		}

		private void MenuDefinitionEpsilonY_Click(object? sender, EventArgs e)
		{
			ShowDefinitionDialog("ε-Y 定义", EpsilonYNotation.definition);
		}

		private void MenuDefinitionOmegaY_Click(object? sender, EventArgs e)
		{
			ShowDefinitionDialog("ω-Y 定义", OmegaYNotation.definition);
		}

		private void MenuDefinitionOmegaYMedium_Click(object? sender, EventArgs e)
		{
			ShowDefinitionDialog("ω-Y (medium magma) 定义",
				OmegaYMediumNotation.definition);
		}

		private void MenuDefinitionOmegaYStrong_Click(object? sender, EventArgs e)
		{
			ShowDefinitionDialog("ω-Y (strong magma) 定义",
				OmegaYStrongNotation.definition);
		}

		private void ShowDefinitionDialog(string title, string text)
		{
			using var dlg = new Form
			{
				Text = title,
				ClientSize = new Size(420, 260),
				FormBorderStyle = FormBorderStyle.FixedDialog,
				MaximizeBox = false,
				MinimizeBox = false,
				StartPosition = FormStartPosition.CenterParent,
				ShowInTaskbar = false,
			};

			var rtb = new RichTextBox
			{
				Location = new Point(10, 10),
				Size = new Size(400, 200),
				ReadOnly = true,
				ScrollBars = RichTextBoxScrollBars.Vertical,
				Font = new Font("Microsoft YaHei UI", 10f),
				Text = text,
			};

			var btnOk = new Button
			{
				Text = "确定",
				DialogResult = DialogResult.OK,
				Location = new Point(330, 220),
				Size = new Size(80, 25),
			};

			dlg.Controls.Add(rtb);
			dlg.Controls.Add(btnOk);
			dlg.AcceptButton = btnOk;
			dlg.CancelButton = btnOk;

			dlg.ShowDialog(this);
		}

		private void MenuHelpItem_Click(object sender, EventArgs e)
		{
			MessageBox.Show(this,
				"代码署名\n" +
				"Hypcos 部分记号的代码修改自notation-explorer\n" +
				"SmileLee-lyx 部分记号的代码修改自 NER\n" +
				"曹知秋 记号提供给AI的定义使用《大数理论》的原文\n" +
				"MrSS的定义来自 AAA滚木批发 (QQ3682911373)\n" +
				"ε-Y的代码修改自Go men的代码\n" +
				"ω-Y 三个记号移植自 omega_y.hpp 与 omega-Y-magma.js\n" +
				"请注意 代码系利用人工智能技术生成，我（和所有贡献者）不保证展开结果正确",
				"帮助", MessageBoxButtons.OK, MessageBoxIcon.Information);
		}

		private void MenuAbout_Click(object sender, EventArgs e)
		{
			MessageBox.Show(this,
				"ω-Y 展开器\n\nBy Cream-CN\n",
				"关于", MessageBoxButtons.OK, MessageBoxIcon.Information);
		}

		private void MenuLegal_Click(object sender, EventArgs e)
		{
			MessageBox.Show(this,
				"Unlicense 授权\n\n" +
				"This is free and unencumbered software released into the public domain.\n\n" +
				"Anyone is free to copy, modify, publish, use, compile, sell, or\n" +
				"distribute this software, either in source code form or as a compiled\n" +
				"binary, for any purpose, commercial or non-commercial, and by any\n" +
				"means.\n\n" +
				"In jurisdictions that recognize copyright laws, the author or authors\n" +
				"of this software dedicate any and all copyright interest in the\n" +
				"software to the public domain. We make this dedication for the benefit\n" +
				"of the public at large and to the detriment of our heirs and\n" +
				"successors. We intend this dedication to be an overt act of\n" +
				"relinquishment in perpetuity of all present and future rights to this\n" +
				"software under copyright law.\n\n" +
				"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n" +
				"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n" +
				"MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n" +
				"IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n" +
				"OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n" +
				"ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n" +
				"OTHER DEALINGS IN THE SOFTWARE.\n\n" +
				"For more information, please refer to <https://unlicense.org/>\n",
				"法律声明", MessageBoxButtons.OK, MessageBoxIcon.Information);
		}
	}
}