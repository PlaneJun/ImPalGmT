#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include "MainWidget.h"
#include <time.h>
#include <stdint.h>
#include <imgui_theme.h>
#include "../utils/utils.h"
#include "../render/render.h"
#include "../render/draw.h"
#include "../rcon/rcon.h"
#include "../globals.hpp"
#include "../log/log.h"

#define WAIT_TIME_15_SEC 15
#define WAIT_TIME_60_SEC 60

rcon g_rcon_cli;
HANDLE g_recv_tid;
std::string g_console_logs{};
std::string g_server_run_time{};

// 读取本地游戏配置 & 游戏路径
void read_local_config()
{
	// 读取游戏服务器配置模板
	std::ifstream conf_file("./data/config.json");
	conf_file >> globals::setting::options;
	conf_file.close();

	// 读取游戏路径
	conf_file.open("./data/game.dat");
	conf_file >> globals::base::game_server_root;
	conf_file.close();

}

void save_server_to_json()
{
	std::ofstream conf_file("./data/config.json");
	conf_file << globals::setting::options.dump();
	conf_file.close();
}

bool generate_server_from_json(nlohmann::ordered_json datas)
{
	std::string conf_buffer = "[/Script/Pal.PalGameWorldSettings]\nOptionSettings=(";
	for (auto data : datas.items())
	{
		conf_buffer += data.key();
		conf_buffer += "=";
		globals::EDataType dtype = data.value()["data_type"].get<globals::EDataType>();
		auto obj_value = data.value()["value"];
		switch (dtype)
		{
			case globals::EDataType::NUMBER:
			{
				conf_buffer += std::to_string(obj_value.get<int>());
				break;
			}
			case globals::EDataType::FLOAT:
			{
				conf_buffer += std::to_string(obj_value.get<float>());
				break;
			}
			case globals::EDataType::BOOLEAN:
			{
				conf_buffer += obj_value.get<bool>() ? "True":"False";
				break;
			}
			case globals::EDataType::TEXT:
			{
				conf_buffer += "\""+obj_value.get<std::string>()+"\"";
				break;
			}
			case globals::EDataType::LIST:
			{
				int index = obj_value.get<int>();
				auto dict = data.value()["dict"].get<std::unordered_map<std::string,std::string>>();
				auto it = dict.begin();
				std::advance(it, index);
				if(it != dict.end())
					conf_buffer += it->first;
				else
					conf_buffer +="None";
				break;
			}
		}
		
		conf_buffer += ",";
	}
	// 去除最后一个逗号
	conf_buffer = conf_buffer.substr(0, conf_buffer.length() - 1);
	conf_buffer += ")";

	FILE* config = NULL;
	fopen_s(&config, "PalWorldSettings.ini", "w+");
	if (config == NULL)
		return false;
	fwrite(conf_buffer.c_str(),1, conf_buffer.length(),config);
	fclose(config);
	return true;
}

template <typename... Args>
std::string format_console_log(const char* format, Args... args)
{
	time_t  now = time(0);

	struct tm p;
	localtime_s(&p, &now);
	char buffer[2500] = { '\0' };
	sprintf_s(buffer, 2500, "[%04d-%02d-%02dT%02d:%02d:%02d] ", p.tm_year + 1900, p.tm_mon + 1, p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);
	sprintf_s(buffer + strlen(buffer), 2500 - strlen(buffer), format, args...);
	sprintf_s(buffer + strlen(buffer), 2500 - strlen(buffer), "\r\n");

	return buffer;
}

void RecvData_ThreadProc(rcon* rcon_inst)
{
	while (1)
	{
		try
		{
			int pkg_id;
			std::string d = rcon_inst->rcon_recv(pkg_id);
			if (!d.empty())
			{
				g_console_logs += format_console_log("%s", d.c_str());
			}
			
		}
		catch (std::exception e)
		{
			return;
		}
	}
}

void TimeCall_ThreadProc()
{
	static uint32_t s_step = 0;
	while (true)
	{
		uint32_t wait = 0;
		// 定时备份
		if (globals::base::switch_autoBackup)
		{
			wait = globals::base::mins_for_backup * WAIT_TIME_60_SEC;
			if (!(s_step % wait))
			{
				std::string bak_path = std::string(globals::base::game_server_root) + "\\Pal\\Backup\\Backup_"+utils::get_local_time();
				std::string save_path = std::string(globals::base::game_server_root) + "\\Pal\\Saved";
				utils::copy_dir(bak_path, save_path);
				MSG_LOG("backup at %p !", bak_path.c_str());
			}
		}

		// 定时重启
		if (globals::base::switch_restart)
		{
			wait = globals::base::mins_for_restart * WAIT_TIME_60_SEC;
			if (!(s_step % wait))
			{
				std::string bak_path = std::string(globals::base::game_server_root) + "\\Pal\\Backup\\Backup_" + utils::get_local_time();
				std::string save_path = std::string(globals::base::game_server_root) + "\\Pal\\Saved";
				utils::copy_dir(bak_path, save_path);
				utils::kill_process_by_name(L"PalServer.exe");
				utils::create_process_by_filename(std::string(globals::base::game_server_root).append("\\PalServer.exe").c_str());
				MSG_LOG("restart at %s!", utils::get_local_time().c_str());
			}
		}

		// 更新服务器运行时间
		if (!(s_step % WAIT_TIME_15_SEC))
		{
			g_server_run_time = utils::get_process_runtime_by_pid(utils::get_pid_by_name(L"PalServer.exe"));
		}

		Sleep(1000);
		s_step++;
	}
}

void Table_Base()
{
	static bool connect = false;
	if (ImGui::BeginChild("#server_console", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.7), false))
	{
		ImGui::TextUnformatted(g_console_logs.c_str());
		ImGui::EndChild();
	}

	ImGui::BeginDisabled(!connect);
	static char input_cmd[256]{};
	ImGui::InputTextWithHint("##rcon_command", "/Command", input_cmd, sizeof(input_cmd));
	ImGui::SameLine();
	if (ImGui::Button(u8"\t发\t送\t"))
	{
		g_rcon_cli.rcon_send(rcon::ESEND_TYPE::EDATA, input_cmd);
		ZeroMemory(input_cmd, sizeof(input_cmd));
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"\t清\t空\t"))
	{
		g_console_logs.clear();
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"\t保\t存\t"))
	{
		std::string save_path = "./log.txt";
		HANDLE hFile = CreateFileA(save_path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (GetLastError() != NULL)
		{
			MessageBoxA(NULL, "save fail", NULL, NULL);
		}
		else
		{
			std::string str_gbk = utils::utf2gbk(g_console_logs.c_str()).c_str();
			WriteFile(hFile, str_gbk.c_str(), str_gbk.length(), NULL, NULL);
			CloseHandle(hFile);
		}
	}
	ImGui::EndDisabled(); ImGui::SameLine();
	static const char* btn_connect[] = { u8"\t连\t接\t" ,u8"\t断\t开\t" };
	if (ImGui::Button(btn_connect[connect]))
	{
		if (!connect)
		{
			ImGui::OpenPopup("connect to remote rcon");
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		}
		else
		{
			// 清理
			TerminateThread(g_recv_tid, 0);
			g_rcon_cli.close();
			connect = false;
		}
		
	}
	ImGui::Separator();
	if (ImGui::BeginChild("#server_function", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y*0.85), false, ImGuiWindowFlags_NoBackground))
	{
		ImGui::Checkbox(u8"自动备份", &globals::base::switch_autoBackup);
		ImGui::SameLine();
		ImGui::Checkbox(u8"定时重启", &globals::base::switch_restart);
		ImGui::EndChild();
	}
	ImGui::Separator();
	if (ImGui::BeginChild("#server_status", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_NoBackground))
	{
		ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.3);
		ImGui::Text((u8"服务器存在时间："+g_server_run_time).c_str());
		ImGui::EndChild();
	}
	if (ImGui::BeginPopupModal("connect to remote rcon", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(u8"连接信息:");
		ImGui::Separator();
		static char ip_port_buf[100]="127.0.0.1:25575";
		static char pwd_buf[100]{};
		ImGui::Text(u8"IP/端口:");
		ImGui::SameLine();
		ImGui::InputTextWithHint("##ip_rcon", ip_port_buf, ip_port_buf, sizeof(ip_port_buf));
		ImGui::Text(u8"密	 码:");
		ImGui::SameLine();
		ImGui::InputText("##rcon_pwd", pwd_buf, sizeof(pwd_buf));
		if (ImGui::Button(u8"确认", ImVec2(120, 0)))
		{
			std::string tmp(ip_port_buf);
			int index = tmp.find(":");
			if (index != std::string::npos)
			{
				std::string ip = tmp.substr(0, index);
				int port = atoi(tmp.substr(index+1).c_str());
				g_rcon_cli.init_socket(ip.c_str(), port);
				if (!g_rcon_cli.auth(pwd_buf))
				{
					MessageBoxA(NULL, "rcon auth fail", NULL, NULL);
				}
				else
				{
					g_console_logs += format_console_log("init rcon server done!");
					g_recv_tid = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)RecvData_ThreadProc, &g_rcon_cli, NULL, NULL);
					connect = true;
				}
			}
			else
			{
				MessageBoxA(NULL, "invaild input", NULL, NULL);
			}

			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(u8"取消", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	//修正数据
	if (globals::base::mins_for_backup <= 0)
		globals::base::mins_for_backup = 1;
	if (globals::base::mins_for_restart <= 0)
		globals::base::mins_for_restart = 1;

}

void Table_Setting(ImVec2 winsize)
{
	int blockStart = 50, blockHight = 55;
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
	if (ImGui::BeginChild("#blocks_begin", ImVec2(ImGui::GetContentRegionAvail().x, 60), false, ImGuiWindowFlags_NoBackground))
	{
		auto ui = globals::setting::options.begin();
		std::string title = (*ui)["name"].get<std::string>();
		draw::get_instance()->DrawFillRect(title.c_str(), blockStart / 2, 0, ImGui::GetContentRegionAvail().x - blockStart, blockHight, IM_COL32(0, 0, 0, 30), IM_COL32(8, 171, 210, 100));
		float cursorY = ImGui::GetCursorPosY();
		ImGui::SetCursorPos({ float(blockStart + winsize.x / 8.5),float(cursorY - blockHight * 0.75) });
		int value = (*ui)["value"].get<int>();

		std::vector<std::string> datas{};
		for (auto d : (*ui)["dict"])
			datas.push_back(d.get<std::string>());

		draw::get_instance()->ComboEx(title.c_str(), &value, datas);
		(*ui)["value"] = value;
		ImGui::SetCursorPosY(cursorY);
		ImGui::EndChild();
	}
	ImGui::Separator();
	if (ImGui::BeginChild("#blocks_option", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.9), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_HorizontalScrollbar))
	{
		for (auto ui = globals::setting::options.begin() ; ui != globals::setting::options.end(); ui++)
		{
			// 跳过第一个,因为上边绘制过了
			if(ui == globals::setting::options.begin())
				continue;

			std::string title = (*ui)["name"].get<std::string>();
			globals::EDataType data_type = (*ui)["data_type"].get<globals::EDataType>();
			globals::EUIType ui_type = (*ui)["ui_type"].get<globals::EUIType>();

			draw::get_instance()->DrawFillRect(title.c_str(), blockStart, 0, ImGui::GetContentRegionAvail().x - blockStart * 2, blockHight, IM_COL32(0, 0, 0, 30), IM_COL32(8, 171, 210, 100));
			ImGui::SetCursorPosX(blockStart + winsize.x / 8.5);
			auto cursorY = ImGui::GetCursorPosY();
			if (ui_type == globals::EUIType::SLIDER)
			{
				float value{};
				char format[20]{};
				if (data_type == globals::EDataType::NUMBER)
				{
					strcpy_s(format, "%.f");
					value = (*ui)["value"].get<int>();
				}
				else if (data_type == globals::EDataType::FLOAT)
				{
					strcpy_s(format, "%.1f");
					value = (*ui)["value"].get<float>();
				}
				ImGui::SetCursorPosY(cursorY - blockHight * 0.85);
				draw::get_instance()->SliderFloatEx(title.c_str(), &value, (*ui)["min"].get<float>(), (*ui)["max"].get<float>(), format);
				if (data_type == globals::EDataType::NUMBER)
				{
					(*ui)["value"] = (int)value;
				}
				else if (data_type == globals::EDataType::FLOAT)
				{
					(*ui)["value"] = value;
				}
				
			}
			else if (ui_type == globals::EUIType::CHEKCBOX)
			{
				bool value = (*ui)["value"].get<bool>();

				ImGui::SetCursorPosY(cursorY - blockHight * 0.7);
				draw::get_instance()->CheckboxEx(title.c_str(), &value);
				(*ui)["value"] = value;
			}
			else if (ui_type == globals::EUIType::COMBO)
			{
				int value = (*ui)["value"].get<int>();

				std::vector<std::string> datas{};
				for (auto d : (*ui)["dict"])
					datas.push_back(d.get<std::string>());

				ImGui::SetCursorPosY(cursorY - blockHight * 0.75);
				draw::get_instance()->ComboEx(title.c_str(), &value, datas);

				(*ui)["value"] = value;
			}
			else if (ui_type == globals::EUIType::INPUTEXT)
			{
				char buffer[256]{};
				if (data_type == globals::EDataType::NUMBER)
				{
					_itoa_s((*ui)["value"].get<int>(), buffer,10);
				}
				else if (data_type == globals::EDataType::TEXT)
				{
					strcpy_s(buffer, (*ui)["value"].get<std::string>().c_str());
				}
				ImGui::SetCursorPosY(cursorY - blockHight * 0.75);
				draw::get_instance()->InputTextEx(title.c_str(), buffer, sizeof(buffer));
				if (data_type == globals::EDataType::NUMBER)
				{
					(*ui)["value"] = atoi(buffer);
				}
				else if (data_type == globals::EDataType::TEXT)
				{
					(*ui)["value"] = buffer;
				}
				
			}
			ImGui::SetCursorPosY(cursorY);
		}
		ImGui::EndChild();
	}

	if (ImGui::BeginChild("#blocks_end", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_NoBackground))
	{
		ImGui::SetCursorPos({ winsize.x / 2 - 95,10 });
		if (ImGui::Button(u8"\t\t\t\t\t确\t\t定\t\t\t\t\t"))
		{
			// 保存到本地json
			save_server_to_json();
			// 生成配置
			generate_server_from_json(globals::setting::options);
			// 打开生成的文件
			system("cmd.exe /c notepad PalWorldSettings.ini");
		}
		ImGui::EndChild();
	}
}

void Table_Option()
{
	ImGui::SeparatorText(u8"程序设置");
	ImGui::Text(u8"自动备份时间间隔（分钟）:");
	ImGui::SameLine();
	ImGui::InputInt("##auto_bakup_time", &globals::base::mins_for_backup);
	ImGui::Text(u8"自动重启时间间隔（分钟）:");
	ImGui::SameLine();
	ImGui::InputInt("##auto_restart_time", &globals::base::mins_for_restart);
	ImGui::Text(u8"游戏服务器根路径:");
	ImGui::SameLine();
	ImGui::InputText(u8"##set_game_path", globals::base::game_server_root,sizeof(globals::base::game_server_root));
	ImGui::SameLine();
	if (ImGui::Button(u8"设置/更新 游戏路径"))
	{
		if (std::filesystem::exists(std::string(globals::base::game_server_root).append("\\PalServer.exe")))
		{
			std::ofstream game_path("./data/game.dat");
			game_path << globals::base::game_server_root;
			game_path.close();
			MessageBoxA(NULL, "更新路径成功", "ok", NULL);
		}
		else {
			MessageBoxA(NULL, "选择路径错误,请选择类似下列路径:\nE:\\Game\\steamcmd\\steamapps\\common\\PalServer", "Error", NULL);
		}
	}
}

void MainWidget::OnPaint()
{
	static bool s_init = false;
	if (!s_init)
	{
		CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)TimeCall_ThreadProc, NULL, NULL, NULL));
		read_local_config();
		s_init = true;
	}

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = { 2.f,2.f };
	{
		//static ImGuiTheme::ImGuiTweakedTheme theme;
		//ImGuiTheme::ShowThemeTweakGui(&theme);
		//ImGuiTheme::ApplyTweakedTheme(theme);
	}

    if (ImGui::Begin("main", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        ImGui::SetWindowPos("main", { 0,0 });
        ImGui::SetWindowSize("main", size_);
        ImGuiTheme::ApplyTheme(ImGuiTheme::ImGuiTheme_MaterialFlat);

		// 检查路径是否设置
		if (strlen(globals::base::game_server_root) <= 0 || !std::filesystem::exists(std::string(globals::base::game_server_root) + "\\PalServer.exe"))
		{
			ImGui::OpenPopup(u8"选择游戏服务器路径");
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal(u8"选择游戏服务器路径", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(u8"输入游戏路径");
			ImGui::Separator();
			static char buf[100]{};
			ImGui::InputText("##GamePath", globals::base::game_server_root, 100);
			if (ImGui::Button(u8"确认"))
			{
				if (std::filesystem::exists(std::string(globals::base::game_server_root).append("\\PalServer.exe")))
				{
					std::ofstream game_path("./data/game.dat");
					game_path << globals::base::game_server_root;
					game_path.close();
					ImGui::CloseCurrentPopup();
				}
				else {
					MessageBoxA(NULL, "选择路径错误,请选择类似下列路径:\nE:\\Game\\steamcmd\\steamapps\\common\\PalServer", "Error", NULL);
				}
			}
			ImGui::EndPopup();
		}
		
		// 绘制主页面
		if (ImGui::BeginTabBar("main_tabs_page", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem(u8"基本"))
			{
				Table_Base();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"服务器配置"))
			{
				Table_Setting(GetWindowSize());
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"存档管理"))
			{
				
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"设置"))
			{
				Table_Option();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		
        ImGui::End();
    }
}
 
void MainWidget::SetWindowSize(float w, float h)
{
    size_.x = w;
    size_.y = h;
}
