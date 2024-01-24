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
		EUIType type; // 0���� 1��ѡ 2��ѡ
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
		float switch_�ɷ������ݵ㹤������³���� = 0;

		std::vector<UIOption> ui_ops =
		{
			UIOption{ u8"�Ѷ�", EUIType::COMBO,&switch_�Ѷ�, {}, {{u8"����",u8"��ͨ",u8"����",u8"�Զ���"}}},
			UIOption{ u8"���������ٶ�", EUIType::SLIDER,&switch_���������ٶ�, {0.1f, 5.f}, {} },
			UIOption{ u8"ҹ�������ٶ�", EUIType::SLIDER,&switch_ҹ�������ٶ�, {0.1f, 5.f},  {} },
			UIOption{ u8"����ֵ����", EUIType::SLIDER,&switch_����ֵ����, {0.1f, 20.f}, {} },
			UIOption{ u8"������ʱ���", EUIType::SLIDER,&switch_������ʱ���, {0.5f,2.f},  {} },
			UIOption{ u8"��³������������ *�����³����������������Ϸ�����½�", EUIType::SLIDER,&switch_��³������������,  {0.5f,3.f}, {} },
			UIOption{ u8"��³�����˺�����", EUIType::SLIDER,&switch_��³�����˺�����, {0.1f, 5.f},  {} },
			UIOption{ u8"��³�����˺�����", EUIType::SLIDER,&switch_��³�����˺�����, {0.1f, 5.f},  {}},
			UIOption{ u8"��³��ʳ�Ƚ��ͱ���", EUIType::SLIDER,&switch_��³��ʳ�Ƚ��ͱ���, {0.1f, 5.f}, {} },
			UIOption{ u8"��³�������ͱ���", EUIType::SLIDER,&switch_��³�������ͱ���, {0.1f, 5.f},{} },
			UIOption{ u8"��³����ֵ��Ȼ�ظ�����", EUIType::SLIDER,&switch_��³����ֵ��Ȼ�ظ�����, {0.1f, 5.f}, {} },
			UIOption{ u8"��³˯��ʱ����ֵ�ظ����ʣ���³�ն��е�����ֵ�ظ����ʣ�", EUIType::SLIDER,&switch_��³˯��ʱ����ֵ�ظ�����, {0.1f, 5.f},{} },
			UIOption{ u8"��ҹ����˺�����", EUIType::SLIDER,&switch_��ҹ����˺�����, {0.1f, 5.f}, {} },
			UIOption{ u8"��ҳ����˺�����", EUIType::SLIDER,&switch_��ҳ����˺�����, {0.1f, 5.f},{} },
			UIOption{ u8"��ұ�ʳ�Ƚ��ͱ���", EUIType::SLIDER,&switch_��ұ�ʳ�Ƚ��ͱ���, {0.1f, 5.f},{} },
			UIOption{ u8"����������ͱ���", EUIType::SLIDER,&switch_����������ͱ���, {0.1f, 5.f}, {} },
			UIOption{ u8"���˯��������ֵ�ظ�����", EUIType::SLIDER,&switch_���˯��������ֵ�ظ�����, {0.1f, 5.f}, {} },
			UIOption{ u8"�Խ����˺�����", EUIType::SLIDER,&switch_�Խ����˺�����,  {0.5f,3.f}, {} },
			UIOption{ u8"��������ӻ��ٶȱ���", EUIType::SLIDER,&switch_��������ӻ��ٶȱ���, 0.f, 10.f,{} },
			UIOption{ u8"�����ڵĵ���������", EUIType::SLIDER,&switch_�����ڵĵ���������, 0.f, 5000.f,{} },
			UIOption{ u8"���߲ɼ�������", EUIType::SLIDER,&switch_���߲ɼ�������,  {0.5f,3.f}, {} },
			UIOption{ u8"�ɲɼ���Ʒ����ֵ����", EUIType::SLIDER,&switch_�ɲɼ���Ʒ����ֵ����,  {0.5f,3.f}, {} },
			UIOption{ u8"�ɲɼ���Ʒˢ�¼��", EUIType::SLIDER,&switch_�ɲɼ���Ʒˢ�¼��,  {0.5f,3.f}, {} },
			UIOption{ u8"���ߵ���������", EUIType::SLIDER,&switch_���ߵ���������,  {0.5f,3.f}, {} },
			UIOption{ u8"�޴󵰷�������ʱ�䣨Сʱ��*������Ҳ����Ӧ�ı����ʱ��", EUIType::SLIDER,&switch_�޴󵰷�������ʱ��, 0.f, 240.f,{} },
			UIOption{ u8"�Ƿ�ᷢ��Ϯ���¼���", EUIType::CHEKCBOX,&switch_�Ƿ�ᷢ��Ϯ��ʱ��, {}, {} },
			UIOption{ u8"�����ͷ�", EUIType::COMBO,&switch_�����ͷ�, {}, {{ u8"�������κζ���", u8"����װ������ĵ���", u8"�������еĵ���",u8"�������е����Լ���������³" }}},
			UIOption{ u8"������������", EUIType::SLIDER,&switch_������������, {1.f, 100.f} ,{} },
			UIOption{ u8"�ɷ������ݵ㹤������³����", EUIType::SLIDER,&switch_�ɷ������ݵ㹤������³����, {1.f, 20.f},  {} }

		};
	}

}
