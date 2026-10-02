using System;
using Expander_CS.Backend.Common;
using Expander_CS.Backend.Notation;

namespace omegay
{
	/// <summary>
	/// UI 侧使用的记号适配项，用来抹平各记号类成员的命名差异。
	/// </summary>
	public sealed class NotationInfo
	{
		public string DisplayName { get; init; } = "";
		public string Definition { get; init; } = "";
		public Func<int[], int, int[]>? Expand { get; init; }
		public Func<string>? Suffix { get; init; }
		public Func<string, int, string>? ExpandText { get; init; }
	}

	/// <summary>
	/// 所有可用记号的注册表。新增记号只需在这里加一项，Form1 不用改。
	/// </summary>
	public static class NotationRegistry
	{
		public static readonly NotationInfo[] All =
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
				ExpandText  = null,
			},
			new NotationInfo
			{
				DisplayName = EpsilonYNotation.Name,
				Definition  = EpsilonYNotation.definition,
				Expand      = EpsilonYNotation.expand,
				Suffix      = EpsilonYNotation.suffix,
				ExpandText  = null,
			},
			new NotationInfo
			{
				DisplayName = Mrss121Notation.Name,
				Definition  = "MrSS1.2.1 定义\n\n" +
							  "\n",
				Expand      = Mrss121Notation.Expand,
				Suffix      = Mrss121Notation.Suffix,
				ExpandText  = Mrss121Notation.ExpandString,
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
	}
}