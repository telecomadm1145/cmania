﻿#include "OpenFileDialog.h"
#include <filesystem>
#include "Animator.h"
#include <vector>
#include "LogOverlay.h"
#ifdef _WIN32
#include <format>
#include "File.h"
#endif
#ifdef __linux__
string formatTime(std::chrono::time_point<std::chrono::file_clock> tp){
	auto cftime=std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(tp));
	struct tm* timeinfo = localtime(&cftime);
	char buf[64];
	strftime(buf, 64, "%Y-%m-%d %H:%M:%S", timeinfo);
	return std::move(string(buf));
}
#endif
class OpenFileDialog : public Screen {
public:
	std::filesystem::path Path{};
	bool MustExists = false;
	bool AllowMulti = false;
	bool PickFolder = false;
	std::string Prompt{};
	std::filesystem::path ChoosenFile{};
	std::function<void()> OnDone{};
	std::function<void()> OnCancel{};
	double Offset = 0;
#ifdef __clang__
	class DurationRangeLimiter_0 {
	public:
		static inline auto Get(auto x) {
			return std::clamp(x, 800.0, 3500.0);
		}
	};
	using TransOut = Transition<
		EaseOut<CubicEasingFunction>,
		DurationRangeLimiter_0>;
#else
	using TransOut = Transition<
		EaseOut<CubicEasingFunction>,
		DurationRangeLimiter<800.0, 3500.0, LinearEasingDurationCalculator<1>>>;
#endif
	TransOut OffsetTrans{};
	using TransOut2 = Transition<
		EaseOut<CubicEasingFunction>,
		ConstantEasingDurationCalculator<50>>;
	TransOut2 Btn1Trans{};
	TransOut2 Btn2Trans{};
	bool btn1hover;
	bool btn2hover;
	struct Entry {
		std::u8string DisplayName;
		std::string LastModifiedTime;
		std::filesystem::path RealPath;
		bool Checked;
		bool IsPath;
	};
	bool dirty = true;
	std ::vector<Entry> Entries;
	Entry* selected_entry = 0;
	virtual void Render(GameBuffer& buf) {
		cx = buf.Width;
		cy = buf.Height;
		if (last_y < 0) {
			Offset = std::clamp(Offset, -(int)std::max((int)Entries.size(), buf.Height - 7) + buf.Height - 7.0, 0.0);
		}
		auto time = HpetClock();
		buf.DrawString(Prompt, 0, 0, { 255, 255, 255, 255 }, {});
		buf.DrawString(u8"当前目录: " + Path.u8string(), 0, 1, { 255, 255, 255, 255 }, {});
		buf.FillRect(0, 2, buf.Width, buf.Height - 2, { {}, { 10, 255, 255, 255 }, ' ' });
		buf.DrawString("文件名|修改日期", 0, 2, { 255, 255, 255, 255 }, {});
		buf.FillRect(33, 2, 38, 3, { {}, { 255, 20, 255, 40 }, ' ' });
		buf.DrawString("目录", 40, 2, { 255, 255, 255, 255 }, {});
		OffsetTrans.SetValue(time, Offset);
		int i = (int)OffsetTrans.GetCurrentValue(time) + 3;
		for (auto& ent : Entries) {
			if (i <= 2)
				goto c;
			if (i >= buf.Height - 2)
				break;
			{
				Color fg{ 255, 255, 255, 255 };
				if (ent.IsPath)
					fg = { 255, 20, 255, 40 };
				auto str = ent.DisplayName;
				if (str.size() > 30) {
					str.resize(30);
					str += u8"...";
				}
				if (&ent == selected_entry) {
					buf.FillRect(0, i, buf.Width, i + 1, { {}, { 125, 255, 255, 255 }, ' ' });
				}
				buf.DrawString(str, 0, i, fg, {});
				buf.DrawString(ent.LastModifiedTime, 45, i, { 255, 255, 255, 255 }, {});
			}
		c:
			i++;
		}
		if (selected_entry != 0) {
			buf.DrawString(u8"* " + selected_entry->RealPath.u8string(), 0, buf.Height - 2, { 255, 255, 255, 255 }, {});
		}
		else {
			std::string s = "总计";
			s += std::to_string(Entries.size());
			s += "个文件";
			buf.DrawString(s, 0, buf.Height - 2, { 255, 255, 255, 255 }, {});
		}
		Btn1Trans.SetValue(time, btn1hover ? 160 : 40);
		Btn2Trans.SetValue(time, btn2hover ? 160 : 40);
		buf.FillRect(buf.Width - 16, buf.Height - 2, buf.Width - 8, buf.Height, { {}, { (unsigned char)Btn1Trans.GetCurrentValue(time), 255, 255, 255 }, ' ' });
		buf.DrawString("取消", buf.Width - 14, buf.Height - 1, {}, {});
		buf.FillRect(buf.Width - 8, buf.Height - 2, buf.Width, buf.Height, { {}, { (unsigned char)Btn2Trans.GetCurrentValue(time), 255, 255, 255 }, ' ' });
		buf.DrawString("确定", buf.Width - 6, buf.Height - 1, {}, {});
	};
	void AddEntry(const std::filesystem::path& d) {
		Entry ent{};
		ent.DisplayName = d.filename().u8string();
#ifdef _WIN32
		ent.LastModifiedTime = std::format("{}", std::filesystem::last_write_time(d));
#else
		ent.LastModifiedTime = formatTime(std::filesystem::last_write_time(d));
#endif
		ent.IsPath = std::filesystem::is_directory(d);
		ent.RealPath = d;
		Entries.push_back(ent);
	}
	void AddEntry(const std::filesystem::path& d, std::u8string name) {
		Entry ent{};
		ent.DisplayName = name;
#ifdef _WIN32
		ent.LastModifiedTime = std::format("{}", std::filesystem::last_write_time(d));
#else
		ent.LastModifiedTime = formatTime(std::filesystem::last_write_time(d));
#endif
		ent.IsPath = std::filesystem::is_directory(d);
		ent.RealPath = d;
		Entries.push_back(ent);
	}
	void DoConfirm() {
		if (selected_entry == 0) {
			game->GetFeature<ILogger>().LogError("请选择一个文件或目录...");
			return;
		}
		if (OnDone) {
			if (PickFolder) {
				if (!selected_entry->IsPath) {
					game->GetFeature<ILogger>().LogError("请选择一个目录...");
					return;
				}
			}
			ChoosenFile = selected_entry->RealPath.string();
			OnDone();
		}
		parent->Back();
	}
	virtual void Tick(double fromRun) {
		if (dirty) {
			Entries.clear();
#ifdef _WIN32
			if (Path == "\\\\?\\DosDevices\\") {
				for (auto drive : GetAvaliableDrives()) {
					std::u8string d{};
					d += drive;
					d += ':';
					Entry ent{};
					ent.DisplayName = d;
					d += '\\';
					auto p = std::filesystem::path(d);
					ent.IsPath = true;
					try {
						ent.LastModifiedTime = std::format("{}", std::filesystem::last_write_time(p));
					}
					catch (...) {
						ent.LastModifiedTime = "N/A(拒绝访问)";
						ent.IsPath = false;
					}
					ent.RealPath = p;
					Entries.push_back(ent);
				}
				dirty = false;
				return;
			}
#endif
			try {
				AddEntry(Path, u8".");
#ifdef _WIN32
				if (Path == Path.parent_path()) {
					Entry ent{};
					ent.DisplayName = u8"..";
					ent.LastModifiedTime = "N/A";
					ent.IsPath = true;
					ent.RealPath = std::filesystem::path("\\\\?\\DosDevices\\");
					Entries.push_back(ent);
				}
				else {
#endif
					AddEntry(Path.parent_path(), u8"..");
#ifdef _WIN32
				}
#endif
				for (auto& d : std::filesystem::directory_iterator(Path)) {
					AddEntry(d);
				}
			}
			catch (...) {
				Entries.clear();
				Path = Path.parent_path();
				game->GetFeature<ILogger>().LogError("文件夹打开失败...返回");
				return;
			}
			dirty = false;
		}
	};
	virtual void Initalized(){};
	virtual void Key(KeyEventArgs kea) {
		if (kea.Pressed) {
			if (kea.Key == ConsoleKey::Escape || kea.Key == ConsoleKey::LeftArrow) {
				if (Path.parent_path() == Path) {
					parent->Back();
					if (OnCancel)
						OnCancel();
					return;
				}
				Offset = 0;
				Path = Path.parent_path();
				dirty = true;
				return;
			}
			else if (kea.Key == ConsoleKey::UpArrow || kea.Key == ConsoleKey::DownArrow) {
				if (selected_entry == 0) {
					selected_entry = &Entries[0];
				}
				else {
					bool up = kea.Key == ConsoleKey::UpArrow;
					int index = -1;
					for (int i = 0; i < (int)Entries.size(); i++) {
						if (&Entries[i] == selected_entry) {
							index = i;
							if (up) {
								index--;
								if (index == -1)
									index = Entries.size() - 1;
							}
							else {
								index++;
								if (index == Entries.size())
									index = 0;
							}
							break;
						}
					}
					selected_entry = &Entries[index];
				}
			}
			else if (kea.Key == ConsoleKey::RightArrow) {
				if (selected_entry != 0) {
					if (selected_entry->IsPath) {
						Offset = 0;
						Path = selected_entry->RealPath;
						dirty = true;
						selected_entry = 0;
					}
				}
			}
			else if (kea.Key == ConsoleKey::Enter) {
				DoConfirm();
			}
		}
	};
	virtual void Wheel(WheelEventArgs wea){};
	int last_x, last_y = -999;
	int cx = 0, cy = 0;
	double LastOffset = 0.0;
	virtual void Move(MoveEventArgs mea) {
		if (last_y >= 0) {
			Offset = LastOffset + (mea.Y - last_y) * 2;
		}
		else {
			// Button hover.
			//
			// buf.Width - 16, buf.Height - 2, buf.Width - 8, buf.Height
			btn1hover = btn2hover = false;
			if (mea.X >= cx - 16 && mea.Y >= cy - 2)
				if (mea.X <= cx - 8) // btn1
				{
					btn1hover = true;
				}
				else {
					btn2hover = true;
				}
		}
	};
	virtual void Activate(bool y) {
		if (y)
			if (Path.empty()) {
#ifdef _WIN32
				Path = getenv("USERPROFILE");
#else
				Path = getenv("HOME");
#endif
			}
	};
	virtual void Resize(){};
	virtual void MouseKey(MouseKeyEventArgs mkea) {
		if (mkea.Pressed) {
			last_x = mkea.X;
			last_y = mkea.Y;
			LastOffset = Offset;
		}
		else {
			if (mkea.X == last_x && mkea.Y == last_y) {
				if (mkea.X >= cx - 16 && mkea.Y >= cy - 2)
					if (mkea.X <= cx - 8) // btn1
					{
						parent->Back();
						if (OnCancel)
							OnCancel();
					}
					else {
						DoConfirm();
					}
				bool clicked = false;
				auto time = HpetClock();
				int i = (int)OffsetTrans.GetCurrentValue(time) + 3;
				for (auto& ent : Entries) {
					if (i <= 2) {
						i++;
						continue;
					}
					if (i >= cy - 2)
						break;
					if (mkea.Y == i) {
						if (selected_entry == 0) {
							selected_entry = &ent;
						}
						else {
							if (selected_entry == &ent) {
								if (ent.IsPath) {
									Offset = 0;
									Path = ent.RealPath;
									dirty = true;
									selected_entry = 0;
								}
							}
							else {
								selected_entry = &ent;
							}
						}
						clicked = true;
					}
					i++;
				}
				if (!clicked) {
					selected_entry = 0;
				}
			}
			last_y = -999;
		}
	};
	virtual void ProcessEvent(const char* evt, const void* evtargs){};
};

Screen* PickFile(std::string prompt, std::function<void(std::filesystem::path)> callback, std::function<void()> oncancel, bool pickfolder, std::filesystem::path def) {
	OpenFileDialog ofd{};
	ofd.Prompt = prompt;
	ofd.PickFolder = pickfolder;
	ofd.Path = def;
	OpenFileDialog* ptr;
	ptr = new OpenFileDialog{ ofd };
	ptr->OnDone = [ptr, callback]() {
		callback(ptr->ChoosenFile);
	};
	ptr->OnCancel = oncancel;
	return ptr;
}
