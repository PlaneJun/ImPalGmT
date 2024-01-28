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
		int type; // 0���� 1��ѡ 2��ѡ
		void* value;
		float min;
		float max;
	}UIOption;

	int levels_choose = 0;
	int deathPunish_choose = 0;
	float ���������ٶ� = 0;
	float ҹ�������ٶ� = 0;
	float ����ֵ���� = 0;
	float ������ʱ��� = 0;
	float ��³������������ = 0;
	float ��³�����˺����� = 0;
	float ��³�����˺����� = 0;
	float ��³��ʳ�Ƚ��ͱ��� = 0;
	float ��³�������ͱ��� = 0;
	float ��³����ֵ��Ȼ�ظ����� = 0;
	float ��³˯��ʱ����ֵ�ظ����� = 0;
	float ��ҹ����˺����� = 0;
	float ��ҳ����˺����� = 0;
	float ��ұ�ʳ�Ƚ��ͱ��� = 0;
	float ����������ͱ��� = 0;
	float ���˯��������ֵ�ظ����� = 0;
	float �Խ����˺����� = 0;
	float ��������ӻ��ٶȱ��� = 0;
	float �����ڵĵ��������� = 0;
	float ���߲ɼ������� = 0;
	float �ɲɼ���Ʒ����ֵ���� = 0;
	float �ɲɼ���Ʒˢ�¼�� = 0;
	float ���ߵ��������� = 0;
	float �޴󵰷�������ʱ�� = 0;
	bool �Ƿ�ᷢ��Ϯ��ʱ�� = true;
	float �����ͷ� = 0;
	float ������������ = 0;
	float �ɷ������ݵ㹤������³���� = 0;


	std::vector<UIOption> ui_ops = {

		UIOption{ u8"���������ٶ�", 0,&���������ٶ�, 0.1f, 5.f },
		UIOption{ u8"ҹ�������ٶ�", 0,&ҹ�������ٶ�, 0.1f, 5.f },
		UIOption{ u8"����ֵ����", 0,&����ֵ����, 0.1f, 20.f },
		UIOption{ u8"������ʱ���", 0,&������ʱ���, 0.5f, 2.f },
		UIOption{ u8"��³������������ *�����³����������������Ϸ�����½�", 0,&��³������������, 0.5f, 3.f },
		UIOption{ u8"��³�����˺�����", 0,&��³�����˺�����, 0.1f, 5.f },
		UIOption{ u8"��³�����˺�����", 0,&��³�����˺�����, 0.1f, 5.f },
		UIOption{ u8"��³��ʳ�Ƚ��ͱ���", 0,&��³��ʳ�Ƚ��ͱ���, 0.1f, 5.f },
		UIOption{ u8"��³�������ͱ���", 0,&��³�������ͱ���, 0.1f, 5.f },
		UIOption{ u8"��³����ֵ��Ȼ�ظ�����", 0,&��³����ֵ��Ȼ�ظ�����, 0.1f, 5.f },
		UIOption{ u8"��³˯��ʱ����ֵ�ظ����ʣ���³�ն��е�����ֵ�ظ����ʣ�", 0,&��³˯��ʱ����ֵ�ظ�����, 0.1f, 5.f },
		UIOption{ u8"��ҹ����˺�����", 0,&��ҹ����˺�����, 0.1f, 5.f },
		UIOption{ u8"��ҳ����˺�����", 0,&��ҳ����˺�����, 0.1f, 5.f },
		UIOption{ u8"��ұ�ʳ�Ƚ��ͱ���", 0,&��ұ�ʳ�Ƚ��ͱ���, 0.1f, 5.f },
		UIOption{ u8"����������ͱ���", 0,&����������ͱ���, 0.1f, 5.f },
		UIOption{ u8"���˯��������ֵ�ظ�����", 0,&���˯��������ֵ�ظ�����, 0.1f, 5.f },
		UIOption{ u8"�Խ����˺�����", 0,&�Խ����˺�����, 0.5f, 3.f },
		UIOption{ u8"��������ӻ��ٶȱ���", 0,&��������ӻ��ٶȱ���, 0.f, 10.f },
		UIOption{ u8"�����ڵĵ���������", 0,&�����ڵĵ���������, 0.f, 5000.f },
		UIOption{ u8"���߲ɼ�������", 0,&���߲ɼ�������, 0.5f, 3.f },
		UIOption{ u8"�ɲɼ���Ʒ����ֵ����", 0,&�ɲɼ���Ʒ����ֵ����, 0.5f, 3.f },
		UIOption{ u8"�ɲɼ���Ʒˢ�¼��", 0,&�ɲɼ���Ʒˢ�¼��, 0.5f, 3.f },
		UIOption{ u8"���ߵ���������", 0,&���ߵ���������, 0.5f, 3.f },
		UIOption{ u8"�޴󵰷�������ʱ�䣨Сʱ��*������Ҳ����Ӧ�ı����ʱ��", 0,&�޴󵰷�������ʱ��, 0.f, 240.f },
		UIOption{ u8"�Ƿ�ᷢ��Ϯ��ʱ�䣿", 1,&�Ƿ�ᷢ��Ϯ��ʱ��, 0.1f, 5.f },
		UIOption{ u8"�����ͷ�", 2,&�����ͷ�, 0.1f, 5.f },
		UIOption{ u8"������������", 0,&������������, 1.f, 100.f },
		UIOption{ u8"�ɷ������ݵ㹤������³����", 0,&�ɷ������ݵ㹤������³����, 1.f, 20.f }

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
			if (ImGui::BeginTabItem(u8"����"))
			{

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"����"))
			{
				float blockStart=50,blockHight = 60;
				ImVec2 winsize = GetWindowSize();
				//ImGui::SetCursorPosX(blockStart);
				//if (ImGui::BeginChild("#�Ѷ�", ImVec2(ImGui::GetContentRegionAvail().x - blockStart, blockHight), false, ImGuiWindowFlags_HorizontalScrollbar))
				//{
				//	ImGui::SetCursorPos({ winsize.x / 6 ,blockHight / 3 });
				//	static const char* levels_items[] = { u8"����", u8"��ͨ", u8"����",u8"�Զ���" };
					//ImGui::Combo(u8"�Ѷ�", &GameOption::levels_choose, levels_items, IM_ARRAYSIZE(levels_items));
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
							//static const char* deathPunish_items[] = { u8"�������κζ���", u8"����װ������ĵ���", u8"�������е���",u8"�������е��߼���������³" };
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
					//	static const char* deathPunish_items[] = { u8"�������κζ���", u8"����װ������ĵ���", u8"�������е���",u8"�������е��߼���������³" };
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
			if (ImGui::BeginTabItem(u8"����&����"))
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
