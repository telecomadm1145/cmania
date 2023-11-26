#pragma once
#include <fstream>
#include "Binary.h"
#define SETTING_FILE_NAME "profile.bin"

class Profile {
public:
	template<class T>
	class ProfileEntry : public DataObject {
		T value;
		// Í¨¹ý DataObject ¼Ì³Ð
		void Write(std::ostream& stm) override {

		}
		void Read(std::istream& stm) override {

		}
	};
	std::unordered_map<std::string, std::unique_ptr<DataObject>> Entries;
	void Load()
	{

	}
	void Save() 
	{

	}
};