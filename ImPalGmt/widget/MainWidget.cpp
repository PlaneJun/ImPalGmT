#include <vector>
#include <map>
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


#define WAIT_TIME_60_SEC 60

rcon g_rcon_cli;
std::string g_console_logs{};


void read_json(std::string path)
{
	std::ifstream conf_file(path);
	conf_file >> globals::setting::options;
}

void save_json(std::string path)
{
	std::ofstream conf_file(path);
	conf_file << globals::setting::options.dump();
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
		if (globals::base::switch_autoBackup)
		{
			wait = globals::base::mins_for_backup * WAIT_TIME_60_SEC;
			if (!(s_step % wait))
			{
				utils::copy_dir("./Pal/Backup/Backup_" + utils::get_local_time() , "./Pal/Saved");
				printf("backup at %d min!\n", globals::base::mins_for_backup);
			}
		}

		if (globals::base::switch_restart)
		{
			wait = globals::base::mins_for_restart * WAIT_TIME_60_SEC;
			if (!(s_step % wait))
			{
				utils::copy_dir("./Pal/Backup/Backup_" + utils::get_local_time(), "./Pal/Saved");
				utils::kill_process_by_name(L"PalServer.exe");
				utils::create_process_by_filename("./PalServer.exe");
				printf("restart at %d min!\n", globals::base::mins_for_restart);
			}
		}
		Sleep(1000);
		s_step++;
	}
}

void Table_Base()
{
	if (ImGui::BeginChild("#server_console", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.7), false))
	{
		ImGui::TextUnformatted(g_console_logs.c_str());
		ImGui::EndChild();
	}
	static char input_cmd[256]{};
	ImGui::InputTextWithHint("##recon_command", "/Command", input_cmd, sizeof(input_cmd));
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
	ImGui::Separator();
	if (ImGui::BeginChild("#server_status", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_NoBackground))
	{
		ImGui::Checkbox(u8"自动备份", &globals::base::switch_autoBackup);
		ImGui::SameLine();
		ImGui::Checkbox(u8"定时重启", &globals::base::switch_restart);
		ImGui::EndChild();
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
				(*ui)["value"] = value;
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
				strcpy_s(buffer, (*ui)["value"].get<std::string>().c_str());
				ImGui::SetCursorPosY(cursorY - blockHight * 0.75);
				draw::get_instance()->InputTextEx(title.c_str(), buffer, sizeof(buffer));
				(*ui)["value"] = buffer;
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
			save_json("./data/config.json");
		}
		ImGui::EndChild();
	}

}

void Table_Option()
{
	ImGui::InputInt(u8"自动备份时间间隔",&globals::base::mins_for_backup);
	ImGui::InputInt(u8"自动重启时间间隔", &globals::base::mins_for_restart);
}

void MainWidget::OnPaint()
{
	static bool s_init = false;
	if (!s_init)
	{
		g_rcon_cli.init_socket("111.67.201.70", 25575);
		if (!g_rcon_cli.auth("PlaneJun_666"))
		{
			MessageBoxA(NULL, "rcon auth fail", NULL, NULL);
			return;
		}
		g_console_logs += format_console_log("init rcon server done!");
		CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)RecvData_ThreadProc, &g_rcon_cli, NULL, NULL));
		CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)TimeCall_ThreadProc, NULL, NULL, NULL));
		read_json("./data/config.json");
		s_init = true;
	}

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = { 2.f,2.f };

	//static ImGuiTheme::ImGuiTweakedTheme theme;
	//ImGuiTheme::ShowThemeTweakGui(&theme);
	//ImGuiTheme::ApplyTweakedTheme(theme);

    if (ImGui::Begin("main", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        ImGui::SetWindowPos("main", { 0,0 });
        ImGui::SetWindowSize("main", size_);
        
        ImGuiTheme::ApplyTheme(ImGuiTheme::ImGuiTheme_MaterialFlat);

		if (ImGui::BeginTabBar("main_tabs_page", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem(u8"基本"))
			{
				Table_Base();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"配置"))
			{
				Table_Setting(GetWindowSize());
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
