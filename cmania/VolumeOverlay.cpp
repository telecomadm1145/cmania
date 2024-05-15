#include "VolumeOverlay.h"
#include "GameBuffer.h"
#include "Hpet.h"
#include "BassAudioManager.h"
#include "ScreenController.h"

class VolumeOverlay : public Overlay {
	double volume = 100;
	int w = 0, h = 0;
	virtual void Render(GameBuffer& buf) {
		w = buf.Width;
		h = buf.Height;
		auto vol = std::to_string(int(volume));
		buf.FillRect(0, 0, buf.Width, buf.Height, { {}, { 150, 20, 20, 20 } });
		buf.DrawString(vol, buf.Width - 5, buf.Height - 5, {}, {});
		buf.DrawString("ÒôÁ¿", buf.Width - 5, buf.Height - 6, {}, {});
	};
	virtual void Key(KeyEventArgs kea) {
		if (kea.Key == ConsoleKey::UpArrow && kea.Pressed) {
			volume += 1;
			GetBassAudioManager()->SetMasterVolume(volume / 100);
		}
		if (kea.Key == ConsoleKey::DownArrow && kea.Pressed) {
			volume -= 1;
			GetBassAudioManager()->SetMasterVolume(volume / 100);
		}
	};
	virtual void MouseKey(MouseKeyEventArgs mkea){};
	virtual bool HitTest(int x, int y) {
		return false;
	};
	virtual void Tick(double fromRun){
	};
};

Overlay* MakeVolumeOverlay() {
	return new VolumeOverlay();
}
