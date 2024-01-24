#include <vector>
#include <string>
#include "MainWidget.h"
#include <stdint.h>
#include <windows.h>
#include <imgui_theme.h>
#include "../render/draw.h"

namespace GameOption
{
	typedef struct _TUIOption
	{
		char title[256];
		int type; // 0滑块 1单选 2多选
		void* value;
		float min;
		float max;
	}UIOption;

	int levels_choose = 0;
	int deathPunish_choose = 0;
	float 白天流逝速度 = 0;
	float 夜晚流逝速度 = 0;
	float 经验值倍率 = 0;
	float 捕获概率倍率 = 0;
	float 帕鲁出现数量倍率 = 0;
	float 帕鲁攻击伤害倍率 = 0;
	float 帕鲁承受伤害倍率 = 0;
	float 帕鲁饱食度降低倍率 = 0;
	float 帕鲁耐力降低倍率 = 0;
	float 帕鲁生命值自然回复倍率 = 0;
	float 帕鲁睡眠时生命值回复倍率 = 0;
	float 玩家攻击伤害倍率 = 0;
	float 玩家承受伤害倍率 = 0;
	float 玩家饱食度降低倍率 = 0;
	float 玩家耐力降低倍率 = 0;
	float 玩家睡眠事生命值回复倍率 = 0;
	float 对建筑伤害倍率 = 0;
	float 建筑物的劣化速度倍率 = 0;
	float 世界内的掉落物上限 = 0;
	float 道具采集量倍率 = 0;
	float 可采集物品生命值倍率 = 0;
	float 可采集物品刷新间隔 = 0;
	float 道具掉落量倍率 = 0;
	float 巨大蛋孵化所需时间 = 0;
	bool 是否会发生袭击时间 = true;
	float 死亡惩罚 = 0;
	float 公会人数上线 = 0;
	float 可分派至据点工作的帕鲁上线 = 0;


	std::vector<UIOption> ui_ops = {

		UIOption{ u8"白天流逝速度", 0,&白天流逝速度, 0.1f, 5.f },
		UIOption{ u8"夜晚流逝速度", 0,&夜晚流逝速度, 0.1f, 5.f },
		UIOption{ u8"经验值倍率", 0,&经验值倍率, 0.1f, 20.f },
		UIOption{ u8"捕获概率倍率", 0,&捕获概率倍率, 0.5f, 2.f },
		UIOption{ u8"帕鲁出现数量倍率 *提高帕鲁出现数量将导致游戏性能下降", 0,&帕鲁出现数量倍率, 0.5f, 3.f },
		UIOption{ u8"帕鲁攻击伤害倍率", 0,&帕鲁攻击伤害倍率, 0.1f, 5.f },
		UIOption{ u8"帕鲁承受伤害倍率", 0,&帕鲁承受伤害倍率, 0.1f, 5.f },
		UIOption{ u8"帕鲁饱食度降低倍率", 0,&帕鲁饱食度降低倍率, 0.1f, 5.f },
		UIOption{ u8"帕鲁耐力降低倍率", 0,&帕鲁耐力降低倍率, 0.1f, 5.f },
		UIOption{ u8"帕鲁生命值自然回复倍率", 0,&帕鲁生命值自然回复倍率, 0.1f, 5.f },
		UIOption{ u8"帕鲁睡眠时生命值回复倍率（帕鲁终端中的生命值回复倍率）", 0,&帕鲁睡眠时生命值回复倍率, 0.1f, 5.f },
		UIOption{ u8"玩家攻击伤害倍率", 0,&玩家攻击伤害倍率, 0.1f, 5.f },
		UIOption{ u8"玩家承受伤害倍率", 0,&玩家承受伤害倍率, 0.1f, 5.f },
		UIOption{ u8"玩家饱食度降低倍率", 0,&玩家饱食度降低倍率, 0.1f, 5.f },
		UIOption{ u8"玩家耐力降低倍率", 0,&玩家耐力降低倍率, 0.1f, 5.f },
		UIOption{ u8"玩家睡眠事生命值回复倍率", 0,&玩家睡眠事生命值回复倍率, 0.1f, 5.f },
		UIOption{ u8"对建筑伤害倍率", 0,&对建筑伤害倍率, 0.5f, 3.f },
		UIOption{ u8"建筑物的劣化速度倍率", 0,&建筑物的劣化速度倍率, 0.f, 10.f },
		UIOption{ u8"世界内的掉落物上限", 0,&世界内的掉落物上限, 0.f, 5000.f },
		UIOption{ u8"道具采集量倍率", 0,&道具采集量倍率, 0.5f, 3.f },
		UIOption{ u8"可采集物品生命值倍率", 0,&可采集物品生命值倍率, 0.5f, 3.f },
		UIOption{ u8"可采集物品刷新间隔", 0,&可采集物品刷新间隔, 0.5f, 3.f },
		UIOption{ u8"道具掉落量倍率", 0,&道具掉落量倍率, 0.5f, 3.f },
		UIOption{ u8"巨大蛋孵化所需时间（小时）*其他蛋也会响应改变孵化时间", 0,&巨大蛋孵化所需时间, 0.f, 240.f },
		UIOption{ u8"是否会发生袭击时间？", 1,&是否会发生袭击时间, 0.1f, 5.f },
		UIOption{ u8"死亡惩罚", 2,&死亡惩罚, 0.1f, 5.f },
		UIOption{ u8"公会人数上线", 0,&公会人数上线, 1.f, 100.f },
		UIOption{ u8"可分派至据点工作的帕鲁上线", 0,&可分派至据点工作的帕鲁上线, 1.f, 20.f }

	};
	
}

void MainWidget::OnPaint()
{
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

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"配置"))
			{
				float blockStart=50,blockHight = 60;
				ImVec2 winsize = GetWindowSize();
				//ImGui::SetCursorPosX(blockStart);
				//if (ImGui::BeginChild("#难度", ImVec2(ImGui::GetContentRegionAvail().x - blockStart, blockHight), false, ImGuiWindowFlags_HorizontalScrollbar))
				//{
				//	ImGui::SetCursorPos({ winsize.x / 6 ,blockHight / 3 });
				//	static const char* levels_items[] = { u8"休闲", u8"普通", u8"困难",u8"自定义" };
					//ImGui::Combo(u8"难度", &GameOption::levels_choose, levels_items, IM_ARRAYSIZE(levels_items));
				//	ImGui::EndChild();
				//}
				float aaa{};
				if (ImGui::BeginChild("#setting_block", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_HorizontalScrollbar))
				{
					
					

					for (int i = 0; i < GameOption::ui_ops.size(); i++)
					{
						ImGui::SetCursorPosX(blockStart);
						draw::get_instance()->draw_filled_rect(std::to_string(i).c_str(), ImGui::GetContentRegionAvail().x - blockStart, blockHight, IM_COL32(0, 0, 0, 30));
						if (GameOption::ui_ops[i].type == 0)
						{
							ImGui::SetCursorPos({ blockStart + winsize.x / 8, ImGui::GetCursorPosY()+10 });
							ImGui::SliderFloat(GameOption::ui_ops[i].title, &aaa, 1, 100);
						}
						else if (GameOption::ui_ops[i].type == 1)
						{
							//ImGui::SetCursorPosY(blockHight / 3);
							//ImGui::Checkbox(GameOption::ui_ops[i].title, (bool*)GameOption::ui_ops[i].value);
						}
						else if (GameOption::ui_ops[i].type == 2)
						{
							//ImGui::SetCursorPosY(blockHight / 3);
							//static const char* deathPunish_items[] = { u8"不掉落任何东西", u8"掉落装备以外的道具", u8"掉落所有道具",u8"掉落所有道具及队伍内帕鲁" };
							//ImGui::Combo(GameOption::ui_ops[i].title, &GameOption::deathPunish_choose, deathPunish_items, IM_ARRAYSIZE(deathPunish_items));
						}
					}
					
					ImGui::EndChild();
				}

				for (int i=0;i< GameOption::ui_ops.size();i++)
				{
					//ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 30));
					//ImGui::SetCursorPosX(blockStart+winsize.x / 6);
					//if (GameOption::ui_ops[i].type == 0)
					//{
					//	//ImGui::SetCursorPosY(blockHight / 4);
						//ImGui::SliderFloat(GameOption::ui_ops[i].title, (float*)GameOption::ui_ops[i].value, GameOption::ui_ops[i].min, GameOption::ui_ops[i].max);
					//}
					//else if (GameOption::ui_ops[i].type == 1)
					//{
					//	//ImGui::SetCursorPosY(blockHight / 3);
					//	ImGui::Checkbox(GameOption::ui_ops[i].title, (bool*)GameOption::ui_ops[i].value);
					//}
					//else if (GameOption::ui_ops[i].type == 2)
					//{
					//	//ImGui::SetCursorPosY(blockHight / 3);
					//	static const char* deathPunish_items[] = { u8"不掉落任何东西", u8"掉落装备以外的道具", u8"掉落所有道具",u8"掉落所有道具及队伍内帕鲁" };
					//	ImGui::Combo(GameOption::ui_ops[i].title, &GameOption::deathPunish_choose, deathPunish_items, IM_ARRAYSIZE(deathPunish_items));
					//}
					////ImGui::PopStyleColor();
					//ImGui::Text("\n\n");
					//ImGui::NextColumn();
					//ImGui::PushID(i);

					//RECT 


					//ImGui::PopID();

				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"关于&赞助"))
			{
			
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
