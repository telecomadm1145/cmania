// cmania.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

import Game;
import Win32ConsoleComponent;
import TickSource;
import <thread>;
import BufferController;
import FpsOverlay;
import ScreenController;
import MainMenuScreen;
import BeatmapManagementService;
import AudioService;

int main()
{
    Game game;
    game.Use<Win32ConsoleComponent>()
        .Use<TickSource>()
        .Use<BufferController>()
        .Use<ScreenController>()
        .Use<BeatmapManagementService>()
        .Use<FpsOverlay>()
        .Use<AudioService>();
    game.Raise("start");
    auto scr = MainMenuScreen();
    game.Raise("navigate", scr);
    while(true)
        std::this_thread::sleep_for(std::chrono::milliseconds(0x7fffffff));
}