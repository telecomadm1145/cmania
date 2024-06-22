#include "AboutScreen.h"
class AboutScreen : public Screen {
	virtual void Render(GameBuffer& buf) {
#ifdef _WIN32
		buf.DrawString(
			"Cmania\n"
			"�ڿ���̨�ն��ϵ�����\n"
			"\n"
			"�� telecomadm1145 ����\n"
			"ԭʼ��Ϸ: osu (by ppy)\n"
			"��Ϸ����ʹ�� MIT ����֤�ַ�\n"
			"�������������� ���� �޸� �ٷַ�����Ϸ\n"
			"ף����Ϸ���� UwU\n"
			"[Esc] ����\n", 0, 0, {}, {});
#endif
#ifdef __linux__
		buf.DrawString(
			"Cmania\n"
			"\n"
			"\n"
			"来自telecomadm1145 \n"
			"linux 移植 by tinyfly\n"
			"玩法及元素版权: osu (by ppy)\n"
			"分发版权: MIT\n"
			"\n"
			"Have fun UwU\n"
			"[Esc] 来退出\n", 0, 0, {}, {});
#endif
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
