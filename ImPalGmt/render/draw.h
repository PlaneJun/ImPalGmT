#include <string>
#include <vector>
#include <imgui_internal.h>



class draw
{
public:
	static draw* get_instance();

	bool DrawFillRect(const char* label,float x,float y,float w,float h, uint32_t bg_color,uint32_t front_color,float round=0.f);

	bool CheckboxEx(const char* label, bool* v);

	bool SliderFloatEx(const char* label, float* v, float v_min, float v_max, const char* format="%.3f", ImGuiSliderFlags flags=0);
	bool SliderScalarEx(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = 0, ImGuiSliderFlags flags = 0);

	bool BeginComboEx(const char* label, const char* preview_value, ImGuiComboFlags flags=0);
	bool ComboEx(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items=-1);
	bool ComboEx(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items=-1);
	bool ComboEx(const char* label, int* current_item, std::vector<std::string> items, int height_in_items = -1);
	bool ComboEx(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items=-1);


	bool InputTextEx(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags=0, ImGuiInputTextCallback callback=0, void* user_data=0);
	bool InputTextExEx(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = 0, void* callback_user_data = 0);

	void ProgressBarEx(const char* label,float curt,float max, const ImVec2& size_arg=ImVec2(-FLT_MIN, 0));
private:
	static draw* s_instance_;
};



