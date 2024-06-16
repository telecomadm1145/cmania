#include "AboutScreen.h"
class AboutScreen : public Screen {
	virtual void Render(GameBuffer& buf) {
		buf.DrawString(
			"Cmania\n"
			"在控制台终端上的音游\n"
			"\n"
			"由 telecomadm1145 开发\n"
			"原始游戏: osu (by ppy)\n"
			"游戏代码使用 MIT 许可证分发\n"
			"您可以任意下载 复制 修改 再分发此游戏\n"
			"祝您游戏开心 UwU\n"
			"[Esc] 返回\n", 0, 0, {}, {});
	}
	virtual void Key(KeyEventArgs kea) {
		if (kea.Pressed) {
			if (kea.Key == ConsoleKey::Escape) {
				parent->Back();
				return;
			}
		}
	}
};

Screen* MakeAboutScreen() {
	return new AboutScreen();
}
