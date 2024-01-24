#pragma once
#include <vector>
#include <string>

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
		BOOLEAN = 3
	};

	typedef struct _TUIOption
	{
		char title[256];
		EUIType type; // 0滑块 1单选 2多选
		void* value;
		struct {
			float min;
			float max;
		}slider;
		struct {
			std::vector<std::string> str_list;
		}combo;

	}UIOption;



	namespace base
	{
		bool switch_autoBackup = false;
		int mins_for_backup = 0;

		bool switch_restart = false;
		int mins_for_restart = 0;
	}

	namespace setting
	{
		int switch_Difficulty = 0;
		float switch_DayTimeSpeedRate = 0;
		float switch_NightTimeSpeedRate = 0;
		float switch_ExpRate = 0;
		float switch_PalCaptureRate = 0;
		float switch_PalSpawnNumRate = 0;
		float switch_PalDamageRateAttack = 0;
		float switch_PalDamageRateDefense = 0;
		float switch_PlayerDamageRateAttack = 0;
		float switch_PlayerDamageRateDefense = 0;
		float switch_PlayerStomachDecreaceRate = 0;
		float switch_PlayerStaminaDecreaceRate = 0;
		float switch_PlayerAutoHPRegeneRate = 0;
		float switch_PlayerAutoHpRegeneRateInSleep = 0;
		float switch_PalStomachDecreaceRate = 0;
		float switch_PalStaminaDecreaceRate = 0;
		float switch_PalAutoHPRegeneRate = 0;
		float switch_PalAutoHpRegeneRateInSleep = 0;
		float switch_BuildObjectDamageRate = 0;
		float switch_BuildObjectDeteriorationDamageRate = 0;
		float switch_CollectionDropRate = 0;
		float switch_CollectionObjectHpRate = 0;
		float switch_CollectionObjectRespawnSpeedRate = 0;
		float switch_EnemyDropItemRate = 0;
		float switch_DeathPenalty = 0;
		bool switch_GuildPlayerMaxNum = true;
		float switch_PalEggDefaultHatchingTime = 0;
		float switch_ServerPlayerMaxNum = 0;
		float switch_可分派至据点工作的帕鲁上线 = 0;

		std::vector<UIOption> ui_ops =
		{
			UIOption{ u8"难度", EUIType::COMBO,&switch_难度, {}, {{u8"休闲",u8"普通",u8"困难",u8"自定义"}}},
			UIOption{ u8"白天流逝速度", EUIType::SLIDER,&switch_白天流逝速度, {0.1f, 5.f}, {} },
			UIOption{ u8"夜晚流逝速度", EUIType::SLIDER,&switch_夜晚流逝速度, {0.1f, 5.f},  {} },
			UIOption{ u8"经验值倍率", EUIType::SLIDER,&switch_经验值倍率, {0.1f, 20.f}, {} },
			UIOption{ u8"捕获概率倍率", EUIType::SLIDER,&switch_捕获概率倍率, {0.5f,2.f},  {} },
			UIOption{ u8"帕鲁出现数量倍率 *提高帕鲁出现数量将导致游戏性能下降", EUIType::SLIDER,&switch_帕鲁出现数量倍率,  {0.5f,3.f}, {} },
			UIOption{ u8"帕鲁攻击伤害倍率", EUIType::SLIDER,&switch_帕鲁攻击伤害倍率, {0.1f, 5.f},  {} },
			UIOption{ u8"帕鲁承受伤害倍率", EUIType::SLIDER,&switch_帕鲁承受伤害倍率, {0.1f, 5.f},  {}},
			UIOption{ u8"帕鲁饱食度降低倍率", EUIType::SLIDER,&switch_帕鲁饱食度降低倍率, {0.1f, 5.f}, {} },
			UIOption{ u8"帕鲁耐力降低倍率", EUIType::SLIDER,&switch_帕鲁耐力降低倍率, {0.1f, 5.f},{} },
			UIOption{ u8"帕鲁生命值自然回复倍率", EUIType::SLIDER,&switch_帕鲁生命值自然回复倍率, {0.1f, 5.f}, {} },
			UIOption{ u8"帕鲁睡眠时生命值回复倍率（帕鲁终端中的生命值回复倍率）", EUIType::SLIDER,&switch_帕鲁睡眠时生命值回复倍率, {0.1f, 5.f},{} },
			UIOption{ u8"玩家攻击伤害倍率", EUIType::SLIDER,&switch_玩家攻击伤害倍率, {0.1f, 5.f}, {} },
			UIOption{ u8"玩家承受伤害倍率", EUIType::SLIDER,&switch_玩家承受伤害倍率, {0.1f, 5.f},{} },
			UIOption{ u8"玩家饱食度降低倍率", EUIType::SLIDER,&switch_玩家饱食度降低倍率, {0.1f, 5.f},{} },
			UIOption{ u8"玩家耐力降低倍率", EUIType::SLIDER,&switch_玩家耐力降低倍率, {0.1f, 5.f}, {} },
			UIOption{ u8"玩家睡眠事生命值回复倍率", EUIType::SLIDER,&switch_玩家睡眠事生命值回复倍率, {0.1f, 5.f}, {} },
			UIOption{ u8"对建筑伤害倍率", EUIType::SLIDER,&switch_对建筑伤害倍率,  {0.5f,3.f}, {} },
			UIOption{ u8"建筑物的劣化速度倍率", EUIType::SLIDER,&switch_建筑物的劣化速度倍率, 0.f, 10.f,{} },
			UIOption{ u8"世界内的掉落物上限", EUIType::SLIDER,&switch_世界内的掉落物上限, 0.f, 5000.f,{} },
			UIOption{ u8"道具采集量倍率", EUIType::SLIDER,&switch_道具采集量倍率,  {0.5f,3.f}, {} },
			UIOption{ u8"可采集物品生命值倍率", EUIType::SLIDER,&switch_可采集物品生命值倍率,  {0.5f,3.f}, {} },
			UIOption{ u8"可采集物品刷新间隔", EUIType::SLIDER,&switch_可采集物品刷新间隔,  {0.5f,3.f}, {} },
			UIOption{ u8"道具掉落量倍率", EUIType::SLIDER,&switch_道具掉落量倍率,  {0.5f,3.f}, {} },
			UIOption{ u8"巨大蛋孵化所需时间（小时）*其他蛋也会响应改变孵化时间", EUIType::SLIDER,&switch_巨大蛋孵化所需时间, 0.f, 240.f,{} },
			UIOption{ u8"是否会发生袭击事件？", EUIType::CHEKCBOX,&switch_是否会发生袭击时间, {}, {} },
			UIOption{ u8"死亡惩罚", EUIType::COMBO,&switch_死亡惩罚, {}, {{ u8"不掉落任何东西", u8"掉落装备以外的道具", u8"掉落所有的道具",u8"掉落所有道具以及队伍内帕鲁" }}},
			UIOption{ u8"公会人数上线", EUIType::SLIDER,&switch_公会人数上线, {1.f, 100.f} ,{} },
			UIOption{ u8"可分派至据点工作的帕鲁上线", EUIType::SLIDER,&switch_可分派至据点工作的帕鲁上线, {1.f, 20.f},  {} }

		};
	}

}
