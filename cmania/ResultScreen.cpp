#include "ResultScreen.h"
#include "ScreenController.h"
#include "BackgroundComponent.h"

class ResultScreen : public Screen {
private:
	BackgroundComponent bg{0.2};
	Record rec;

public:
	ResultScreen(Record rec, std::string bg_path) : rec(rec) {
		//bg.LoadBackground(bg_path);
	}
	virtual void Render(GameBuffer& buf) override {
		//bg.Render(buf);
		auto line = std::string();
		line.append(rec.PlayerName);
		line.append(" played ");
		line.append(rec.BeatmapTitle);
		line.append("[");
		line.append(rec.BeatmapVersion);
		line.append("]");
		buf.DrawString(line, 0, 0, { 255, 255, 255, 255 }, {});
		{
			int i = 0;
			int j = 0;
			for (auto& res : rec.ResultCounter) {
				auto name = GetHitResultName(res.first);
				auto clr = GetHitResultColor(res.first);
				clr.Alpha = 180;
				buf.DrawString(name, j * 40, i * 2 + 5, clr, {});
				auto val = std::to_string(res.second);
				clr.Alpha = 255;
				buf.DrawString(val, j * 40 + 6, i * 2 + 5, clr, {});
				j++;
				if (j == 2) {
					i++;
					j = 0;
				}
			}
		}
	};
	virtual void Tick(double fromRun){};
	virtual void Initalized(){};
	virtual void Key(KeyEventArgs kea) {
		if (kea.Pressed)
			parent->Back();
	};
	virtual void Wheel(WheelEventArgs wea){};
	virtual void Move(MoveEventArgs mea){};
	virtual void Activate(bool){};
	virtual void Resize(){};
	virtual void MouseKey(MouseKeyEventArgs mkea){};
	virtual void ProcessEvent(const char* evt, const void* evtargs){};
};

Screen* MakeResultScreen(Record rec, const std::string& bg_path) {
	return new ResultScreen(rec, bg_path);
}
