#pragma once
#include <vector>
#include <map>
#include <variant>
#include <string>
#include <nlohmann/json.hpp>

namespace globals
{
	enum EUIType :uint8_t
	{
		SLIDER = 0,
		CHEKCBOX = 1,
		COMBO = 2,
		INPUTEXT = 3
	};

	enum EDataType :uint8_t
	{
		NUMBER = 0,
		FLOAT = 1,
		TEXT = 2,
		BOOLEAN = 3,
		LIST =4
	};

	typedef struct _TUIOption
	{
		char title[256];
		EDataType type;
		EUIType ui_component; // 0滑块 1单选 2多选
		char value[8];
		union 
		{
			struct {
				char format[100];
				float min;
				float max;
			}slider;

			struct {
				std::vector<std::string>* str_list;
			}combo;

			struct{
				char data[256];
			}input;
		}body;
	}UIOption;



	namespace base
	{
		char game_server_root[MAX_PATH]{};

		bool switch_autoBackup = false;
		int mins_for_backup = 0;

		bool switch_restart = false;
		int mins_for_restart = 0;
	}

	namespace setting
	{
		nlohmann::ordered_json options;
	}

}
