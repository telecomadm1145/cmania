export module Game;
import <vector>;
import <string>;
import <typeinfo>;
import <functional>;
import <exception>;
import <map>;
import <any>;

export class Game
{
public:
	class Component
	{
	protected:
		class Game* parent;
	public:
		void Init(Game& game_inst)
		{
			parent = &game_inst;
		}
		virtual void ProcessEvent(const char* evt, const void* evtargs) = 0;
	};
private:
	std::vector<Component*> records;
public:
	template<typename T>
	Game& Use()
	{
		auto com = new T();
		com->Init(*this);
		records.emplace_back(com);
		return *this;
	}
	void Raise(const char* evt)
	{
		for (auto comp : records)
		{
			comp->ProcessEvent(evt,0);
		}
	}
	template<typename EventArgs>
	void Raise(const char* evt, const EventArgs& evtargs)
	{
		for (auto comp : records)
		{
			comp->ProcessEvent(evt, &evtargs);
		}
	}
};
export using GameComponent = Game::Component;