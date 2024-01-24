#include <map>
#include "draw.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <windows.h>


draw* draw::s_instance_ = NULL;

float CalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
{
	// FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
	const char* items_separated_by_zeros = (const char*)data;
	int items_count = 0;
	const char* p = items_separated_by_zeros;
	while (*p)
	{
		if (idx == items_count)
			break;
		p += strlen(p) + 1;
		items_count++;
	}
	if (!*p)
		return false;
	if (out_text)
		*out_text = p;
	return true;
}

bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}

bool Items_ArrayGetter2(void* data, int idx, const char** out_text)
{
	std::vector<std::string>* items = (std::vector<std::string>*)data;
	if (out_text)
		*out_text = (*items)[idx].c_str();
	return true;
}

draw* draw::get_instance()
{
	if (s_instance_ == NULL)
		s_instance_ = new draw();
	return s_instance_;
}


struct checkbox_animation {
	float animation;
};

struct slider_element {
	float opacity;
};
struct combo_element {
	float open_anim, arrow_anim;
};


bool draw::DrawFillRect(const char* label, float x, float y, float w, float h, uint32_t bg_color, uint32_t front_color, float round)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const ImRect rect(ImVec2{ window->DC.CursorPos.x+x,window->DC.CursorPos.y+y}, ImVec2(window->DC.CursorPos.x + x+w, window->DC.CursorPos.y +y+h));
	ImGui::ItemSize(rect, style.FramePadding.y);
	if (!ImGui::ItemAdd(rect, id))
		return false;

	

	window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color, round);

	bool hovered = ImGui::ItemHoverable(rect, id,0);
	if (hovered)
	{
		window->DrawList->AddRect(rect.Min, rect.Max, front_color, round, 0, 2);
	}
	return true;
}

bool draw::CheckboxEx(const char* label, bool* v)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);


	const float w = ImGui::CalcItemWidth();
	const float square_sz = 17;
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect frame_bb(ImVec2(pos.x + w - square_sz - 13, pos.y), ImVec2(window->DC.CursorPos.x + w, window->DC.CursorPos.y + square_sz - 1));
	auto tmp = ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f);
	const ImRect total_bb(pos,{ pos.x + tmp.x, pos.y + tmp.y});
	ImGui::ItemSize(total_bb, style.FramePadding.y);

	if (!ImGui::ItemAdd(total_bb, id))
	{
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return false;
	}
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		ImGui::MarkItemEdited(id);
	}

	static std::map <ImGuiID, checkbox_animation> anim;
	auto it_anim = anim.find(id);
	if (it_anim == anim.end())
	{
		anim.insert({ id, { 0.0f } });
		it_anim = anim.find(id);
	}

	it_anim->second.animation = ImLerp(it_anim->second.animation, *v ? 1.0f : 0.0f, 0.12f * (1.0f - ImGui::GetIO().DeltaTime));

	ImGui::RenderNavHighlight(total_bb, id);

	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, IM_COL32(15, 15, 16, 100), false, 9.0f);
	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImColor(147 / 255.0f, 190 / 255.0f, 66 / 255.0f, it_anim->second.animation), false, 9.0f);

	window->DrawList->AddCircleFilled(ImVec2(frame_bb.Min.x + 8 + 14 * it_anim->second.animation, frame_bb.Min.y + 8), 5.0f, ImColor(1.0f, 1.0f, 1.0f), 30);

	if (label_size.x > 0.0f)
		ImGui::RenderText({ total_bb.Min .x+10,total_bb.Min .y}, label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}

bool draw::SliderFloatEx(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
	return SliderScalarEx(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool draw::SliderScalarEx(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = ImGui::CalcItemWidth();

	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	const ImRect total_bb({ window->DC.CursorPos.x + 10,window->DC.CursorPos.y },   ImVec2(window->DC.CursorPos.x+w, window->DC.CursorPos.y+ label_size.y + 16));
	const ImRect frame_bb(ImVec2(total_bb.Min.x, total_bb.Min.y+label_size.y + 13), total_bb.Max);

	const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
		return false;

	// Default format string when passing NULL
	if (format == NULL)
		format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

	const bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
	bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);
	if (!temp_input_is_active)
	{
		// Tabbing or CTRL-clicking on Slider turns it into an input box
		const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
		const bool clicked = hovered && ImGui::IsMouseClicked(0, id);
		const bool make_active = (input_requested_by_tabbing || clicked || g.NavActivateId == id);
		if (make_active && clicked)
			ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
		if (make_active && temp_input_allowed)
			if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
				temp_input_is_active = true;

		if (make_active && !temp_input_is_active)
		{
			ImGui::SetActiveID(id, window);
			ImGui::SetFocusID(id, window);
			ImGui::FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
		}
	}

	if (temp_input_is_active)
	{
		// Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
		const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
		return ImGui::TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
	}

	static std::map <ImGuiID, slider_element> anim;
	auto it_anim = anim.find(id);
	if (it_anim == anim.end())
	{
		anim.insert({ id, { 0.0f } });
		it_anim = anim.find(id);
	}

	it_anim->second.opacity = ImLerp(it_anim->second.opacity, ImGui::IsItemActive() ? 1.0f : 0.4f, 0.08f * (1.0f - ImGui::GetIO().DeltaTime));


	// Slider behavior
	ImRect grab_bb;
	const bool value_changed = ImGui::SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
	if (value_changed)
		ImGui::MarkItemEdited(id);

	char value_buf[64];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

	window->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, IM_COL32(103, 112, 120, 200), 5.0f);
	window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(grab_bb.Min.x, frame_bb.Max.y), ImColor(147, 190, 66), 5.0f); //»¬¿éÌõ
	window->DrawList->AddCircleFilled(ImVec2(grab_bb.Min.x - 1, grab_bb.Min.y-1 ), 4.0f, ImColor(1.0f, 1.0f, 1.0f), 30);

	ImGui::RenderText(total_bb.Min, label);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, it_anim->second.opacity)); {
		ImGui::RenderTextClipped(total_bb.Min, total_bb.Max, value_buf, value_buf_end, NULL, ImVec2(1.f, 0.f));
	} ImGui::PopStyleColor();


	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
	return value_changed;
}

bool draw::BeginComboEx(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
	g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together
	const float size =ImGui::CalcItemWidth();


	const ImRect rect(window->DC.CursorPos,  ImVec2(window->DC.CursorPos.x+size, window->DC.CursorPos.y+25));
	const ImVec2 arrow_size = ImGui::CalcTextSize("A",NULL,true);

	const ImRect clickable(ImVec2(window->DC.CursorPos.x+ size/1.7, window->DC.CursorPos.y), ImVec2(window->DC.CursorPos.x + size, window->DC.CursorPos.y + 25));

	ImGui::ItemSize(rect, style.FramePadding.y);
	if (!ImGui::ItemAdd(clickable, id, &rect))
		return false;

	// Open on click
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(clickable, id, &hovered, &held);
	const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
	bool popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
	if (pressed && !popup_open)
	{
		ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	static std::map <ImGuiID, combo_element> anim;
	auto it_anim = anim.find(id);
	if (it_anim == anim.end())
	{
		anim.insert({ id, { 0.0f, 0.0f } });
		it_anim = anim.find(id);
	}

	it_anim->second.open_anim = ImLerp(it_anim->second.open_anim, popup_open ? 1.0f : 0.0f, 0.04f * (1.0f - ImGui::GetIO().DeltaTime));
	it_anim->second.arrow_anim = ImLerp(it_anim->second.arrow_anim, popup_open ? 0.3f : 0.0f, 0.05f * (1.0f - ImGui::GetIO().DeltaTime));

	window->DrawList->AddRectFilled(clickable.Min, clickable.Max, ImColor(34, 34, 36), 3.0f);
	window->DrawList->AddRect(clickable.Min, clickable.Max, ImColor(1.0f, 1.0f, 1.0f, 0.03f), 3.0f);

	window->DrawList->AddText({ rect.Min .x+10,rect.Min .y+5}, ImGui::GetColorU32(ImGuiCol_Text), label);

	ImGui::RenderTextClipped( ImVec2(clickable.Min.x+14, clickable.Min.y+5),  ImVec2(clickable.Max.x-24, clickable.Max.y+47), preview_value, NULL, NULL, ImVec2(0.0f, 0.0f));


	window->DrawList->AddText(ImVec2(clickable.Max.x - 20, (clickable.Min.y + clickable.Max.y) / 2 - arrow_size.y / 2), ImColor(1.0f, 1.0f, 1.0f, 0.2f + it_anim->second.arrow_anim), "A");


	if (!popup_open)
		return false;

	g.NextWindowData.Flags = backup_next_window_data_flags;
	if (!ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None))
	{
		g.NextWindowData.ClearFlags();
		return false;
	}

	// Set popup size
	float w = clickable.GetWidth();
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
	{
		g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
	}
	else
	{
		if ((flags & ImGuiComboFlags_HeightMask_) == 0)
			flags |= ImGuiComboFlags_HeightRegular;
		IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_)); // Only one
		int popup_max_height_in_items = -1;
		if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
		else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
		else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
		ImGui::SetNextWindowSizeConstraints(ImVec2(w, 0.0f), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items) * it_anim->second.open_anim));
	}

	// This is essentially a specialized version of BeginPopupEx()
	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

	// Set position given a custom constraint (peak into expected window size so we can position it)
	// FIXME: This might be easier to express with an hypothetical SetNextWindowPosConstraints() function?
	// FIXME: This might be moved to Begin() or at least around the same spot where Tooltips and other Popups are calling FindBestWindowPosForPopupEx()?
	if (ImGuiWindow* popup_window = ImGui::FindWindowByName(name))
		if (popup_window->WasActive)
		{
			// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
			ImVec2 size_expected = ImGui::CalcWindowNextAutoFitSize(popup_window);
			popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
			ImRect r_outer = ImGui::GetPopupAllowedExtentRect(popup_window);
			ImVec2 pos = ImGui::FindBestWindowPosForPopupEx(clickable.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, clickable, ImGuiPopupPositionPolicy_ComboBox);
			ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y));
		}

	// We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 5)); // Horizontally align ourselves with the framed text
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 3.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.03f));
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(ImColor(34, 34, 36)));
	bool ret = ImGui::Begin(name, NULL, window_flags | ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);
	if (!ret)
	{
		ImGui::EndPopup();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}
	return true;
}

bool draw::ComboEx(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!BeginComboEx(label, preview_value, ImGuiComboFlags_None))
		return false;

	// Display items
	// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{
		ImGui::PushID(i);
		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(data, i, &item_text))
			item_text = "*Unknown item*";
		if (ImGui::Selectable(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			ImGui::SetItemDefaultFocus();
		ImGui::PopID();
	}

	ImGui::EndCombo();

	if (value_changed)
		ImGui::MarkItemEdited(g.LastItemData.ID);

	return value_changed;
}

bool draw::ComboEx(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
	const bool value_changed = ComboEx(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
	return value_changed;
}

bool draw::ComboEx(const char* label, int* current_item, std::vector<std::string> items, int height_in_items )
{
	const bool value_changed = ComboEx(label, current_item, Items_ArrayGetter2, (void*)&items, items.size(), height_in_items);
	return value_changed;
}

bool draw::ComboEx(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
	int items_count = 0;
	const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
	while (*p)
	{
		p += strlen(p) + 1;
		items_count++;
	}
	bool value_changed = ComboEx(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
	return value_changed;
}





