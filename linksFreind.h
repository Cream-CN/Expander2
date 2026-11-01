#pragma once


#include <string>

namespace expander_links {
    struct showsOfFriLnks {
        std::string showName;
        std::string Link;
    };
    inline const showsOfFriLnks kFriendLinks[] = {
        { "Hypcos NE", "https://github.com/hypcos/notation-explorer" },
        { "ne - weritten", "https://github.com/smilelee-lyx/ne-weritten" },
        { "《大数理论》",      "https://github.com/zhiqiucao/googology" },
        { "basmat",         "https://github.com/kyodaisuu/basmat" },
        { "X-Y Expander",       "https://gomen520.github.io" },
        { "Free Software Foundtion" , "https://www.fsf.org/"}

    };
    inline constexpr int kFriendLinkCount =
        static_cast<int>(sizeof(kFriendLinks) / sizeof(kFriendLinks[0]));
} // namespace expander_links
