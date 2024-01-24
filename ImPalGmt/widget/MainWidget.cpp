#include <vector>
#include <map>
#include <string>
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


void read_config(std::string config)
{
	std::map<std::string, std::string> keyValuePairs;

	std::istringstream iss(config);
	std::string line;
	while (std::getline(iss, line))
	{
		std::istringstream lineStream(line);
		std::string key, value;
		std::getline(lineStream, key, '=');
		std::getline(lineStream, value, '=');
		keyValuePairs[key] = value;
	}

	globals::setting::switch_难度 = keyValuePairs["Difficulty"];
	globals::setting::switch_白天流逝速度 = std::stof(keyValuePairs["DayTimeSpeedRate"]);
	globals::setting::switch_夜晚流逝速度 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_经验值倍率 = std::stof(keyValuePairs["ExpRate"]);
	globals::setting::switch_捕获概率倍率 = std::stof(keyValuePairs["PalCaptureRate"]);
	globals::setting::switch_帕鲁出现数量倍率 = std::stof(keyValuePairs["PalSpawnNumRate"]);
	globals::setting::switch_帕鲁攻击伤害倍率 = std::stof(keyValuePairs["PalDamageRateAttack"]);
	globals::setting::switch_帕鲁承受伤害倍率 = std::stof(keyValuePairs["PalDamageRateDefense"]);
	globals::setting::switch_帕鲁饱食度降低倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_帕鲁耐力降低倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_帕鲁生命值自然回复倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_帕鲁睡眠时生命值回复倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_玩家攻击伤害倍率 = std::stof(keyValuePairs["PlayerDamageRateAttack"]);
	globals::setting::switch_玩家承受伤害倍率 = std::stof(keyValuePairs["PlayerDamageRateDefense"]);
	globals::setting::switch_玩家饱食度降低倍率 = std::stof(keyValuePairs["PlayerStomachDecreaceRate"]);
	globals::setting::switch_玩家耐力降低倍率 = std::stof(keyValuePairs["PlayerStaminaDecreaceRate"]);
	globals::setting::switch_玩家睡眠事生命值回复倍率 = std::stof(keyValuePairs["PlayerAutoHpRegeneRateInSleep"]);
	globals::setting::switch_对建筑伤害倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_建筑物的劣化速度倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_世界内的掉落物上限 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_道具采集量倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_可采集物品生命值倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_可采集物品刷新间隔 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_道具掉落量倍率 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_巨大蛋孵化所需时间 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_是否会发生袭击时间 = keyValuePairs["Difficulty"] == "True"? true:false;
	globals::setting::switch_死亡惩罚 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_公会人数上线 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
	globals::setting::switch_可分派至据点工作的帕鲁上线 = std::stof(keyValuePairs["NightTimeSpeedRate"]);
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
		// 绘制背景
		auto ui = globals::setting::ui_ops .begin();
		draw::get_instance()->DrawFillRect(ui->title, blockStart / 2, 0, ImGui::GetContentRegionAvail().x - blockStart, blockHight, IM_COL32(0, 0, 0, 30), IM_COL32(8, 171, 210, 100));
		ImGui::SetCursorPosX(blockStart + winsize.x / 8.5);
		auto cursorY = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(cursorY - blockHight * 0.75);
		draw::get_instance()->ComboEx(ui->title, static_cast<int*>(ui->value), ui->combo.str_list);
		ImGui::SetCursorPosY(cursorY);
		ImGui::EndChild();
	}
	ImGui::Separator();
	if (ImGui::BeginChild("#blocks_option", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.9), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_HorizontalScrollbar))
	{
		for (auto ui = globals::setting::ui_ops.begin() + 1; ui != globals::setting::ui_ops.end(); ui++)
		{
			// 绘制背景
			draw::get_instance()->DrawFillRect(ui->title, blockStart, 0, ImGui::GetContentRegionAvail().x - blockStart * 2, blockHight, IM_COL32(0, 0, 0, 30), IM_COL32(8, 171, 210, 100));
			ImGui::SetCursorPosX(blockStart + winsize.x / 8.5);
			auto cursorY = ImGui::GetCursorPosY();
			if (ui->type == 0)
			{
				ImGui::SetCursorPosY(cursorY - blockHight * 0.85);
				draw::get_instance()->SliderFloatEx(ui->title, static_cast<float*>(ui->value), ui->slider.min, ui->slider.max);
			}
			else if (ui->type == 1)
			{
				ImGui::SetCursorPosY(cursorY - blockHight * 0.7);
				draw::get_instance()->CheckboxEx(ui->title, static_cast<bool*>(ui->value));
			}
			else if (ui->type == 2)
			{
				ImGui::SetCursorPosY(cursorY - blockHight * 0.75);
				draw::get_instance()->ComboEx(ui->title, static_cast<int*>(ui->value), ui->combo.str_list);
			}
			ImGui::SetCursorPosY(cursorY);
		}
		ImGui::EndChild();
	}

	if (ImGui::BeginChild("#blocks_end", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_NoBackground))
	{
		ImGui::SetCursorPos({ winsize.x / 2 - 95,10 });
		ImGui::Button(u8"\t\t\t\t\t确\t\t定\t\t\t\t\t");
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
