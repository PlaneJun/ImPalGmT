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

int InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end)
{
	int line_count = 0;
	const char* s = text_begin;
	while (char c = *s++) // We are only matching for \n so we can ignore UTF-8 decoding
		if (c == '\n')
			line_count++;
	s--;
	if (s[0] != '\n' && s[0] != '\r')
		line_count++;
	*out_text_end = s;
	return line_count;
}

ImVec2 InputTextCalcTextSizeW(ImGuiContext* ctx, const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining=0, ImVec2* out_offset=0, bool stop_on_new_line=false)
{
	ImGuiContext& g = *ctx;
	ImFont* font = g.Font;
	const float line_height = g.FontSize;
	const float scale = line_height / font->FontSize;

	ImVec2 text_size = ImVec2(0, 0);
	float line_width = 0.0f;

	const ImWchar* s = text_begin;
	while (s < text_end)
	{
		unsigned int c = (unsigned int)(*s++);
		if (c == '\n')
		{
			text_size.x = ImMax(text_size.x, line_width);
			text_size.y += line_height;
			line_width = 0.0f;
			if (stop_on_new_line)
				break;
			continue;
		}
		if (c == '\r')
			continue;

		const float char_width = font->GetCharAdvance((ImWchar)c) * scale;
		line_width += char_width;
	}

	if (text_size.x < line_width)
		text_size.x = line_width;

	if (out_offset)
		*out_offset = ImVec2(line_width, text_size.y + line_height);  // offset allow for the possibility of sitting after a trailing \n

	if (line_width > 0 || text_size.y == 0.0f)                        // whereas size.y will ignore the trailing \n
		text_size.y += line_height;

	if (remaining)
		*remaining = s;

	return text_size;
}

namespace ImStb
{

	static int     STB_TEXTEDIT_STRINGLEN(const ImGuiInputTextState* obj) { return obj->CurLenW; }
	static ImWchar STB_TEXTEDIT_GETCHAR(const ImGuiInputTextState* obj, int idx) { IM_ASSERT(idx <= obj->CurLenW); return obj->TextW[idx]; }
	static float   STB_TEXTEDIT_GETWIDTH(ImGuiInputTextState* obj, int line_start_idx, int char_idx) { ImWchar c = obj->TextW[line_start_idx + char_idx]; if (c == '\n') return STB_TEXTEDIT_GETWIDTH_NEWLINE; ImGuiContext& g = *obj->Ctx; return g.Font->GetCharAdvance(c) * (g.FontSize / g.Font->FontSize); }
	static int     STB_TEXTEDIT_KEYTOTEXT(int key) { return key >= 0x200000 ? 0 : key; }
	static ImWchar STB_TEXTEDIT_NEWLINE = '\n';
	static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, ImGuiInputTextState* obj, int line_start_idx)
	{
		const ImWchar* text = obj->TextW.Data;
		const ImWchar* text_remaining = NULL;
		const ImVec2 size = InputTextCalcTextSizeW(obj->Ctx, text + line_start_idx, text + obj->CurLenW, &text_remaining, NULL, true);
		r->x0 = 0.0f;
		r->x1 = size.x;
		r->baseline_y_delta = size.y;
		r->ymin = 0.0f;
		r->ymax = size.y;
		r->num_chars = (int)(text_remaining - (text + line_start_idx));
	}

	static bool is_separator(unsigned int c)
	{
		return c == ',' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == '|' || c == '\n' || c == '\r' || c == '.' || c == '!';
	}

	static int is_word_boundary_from_right(ImGuiInputTextState* obj, int idx)
	{
		// When ImGuiInputTextFlags_Password is set, we don't want actions such as CTRL+Arrow to leak the fact that underlying data are blanks or separators.
		if ((obj->Flags & ImGuiInputTextFlags_Password) || idx <= 0)
			return 0;

		bool prev_white = ImCharIsBlankW(obj->TextW[idx - 1]);
		bool prev_separ = is_separator(obj->TextW[idx - 1]);
		bool curr_white = ImCharIsBlankW(obj->TextW[idx]);
		bool curr_separ = is_separator(obj->TextW[idx]);
		return ((prev_white || prev_separ) && !(curr_separ || curr_white)) || (curr_separ && !prev_separ);
	}
	static int is_word_boundary_from_left(ImGuiInputTextState* obj, int idx)
	{
		if ((obj->Flags & ImGuiInputTextFlags_Password) || idx <= 0)
			return 0;

		bool prev_white = ImCharIsBlankW(obj->TextW[idx]);
		bool prev_separ = is_separator(obj->TextW[idx]);
		bool curr_white = ImCharIsBlankW(obj->TextW[idx - 1]);
		bool curr_separ = is_separator(obj->TextW[idx - 1]);
		return ((prev_white) && !(curr_separ || curr_white)) || (curr_separ && !prev_separ);
	}
	static int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(ImGuiInputTextState* obj, int idx) { idx--; while (idx >= 0 && !is_word_boundary_from_right(obj, idx)) idx--; return idx < 0 ? 0 : idx; }
	static int  STB_TEXTEDIT_MOVEWORDRIGHT_MAC(ImGuiInputTextState* obj, int idx) { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_left(obj, idx)) idx++; return idx > len ? len : idx; }
	static int  STB_TEXTEDIT_MOVEWORDRIGHT_WIN(ImGuiInputTextState* obj, int idx) { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_right(obj, idx)) idx++; return idx > len ? len : idx; }
	static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(ImGuiInputTextState* obj, int idx) { ImGuiContext& g = *obj->Ctx; if (g.IO.ConfigMacOSXBehaviors) return STB_TEXTEDIT_MOVEWORDRIGHT_MAC(obj, idx); else return STB_TEXTEDIT_MOVEWORDRIGHT_WIN(obj, idx); }
#define STB_TEXTEDIT_MOVEWORDLEFT   STB_TEXTEDIT_MOVEWORDLEFT_IMPL  // They need to be #define for stb_textedit.h
#define STB_TEXTEDIT_MOVEWORDRIGHT  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL

	static void STB_TEXTEDIT_DELETECHARS(ImGuiInputTextState* obj, int pos, int n)
	{
		ImWchar* dst = obj->TextW.Data + pos;

		// We maintain our buffer length in both UTF-8 and wchar formats
		obj->Edited = true;
		obj->CurLenA -= ImTextCountUtf8BytesFromStr(dst, dst + n);
		obj->CurLenW -= n;

		// Offset remaining text (FIXME-OPT: Use memmove)
		const ImWchar* src = obj->TextW.Data + pos + n;
		while (ImWchar c = *src++)
			*dst++ = c;
		*dst = '\0';
	}

	static bool STB_TEXTEDIT_INSERTCHARS(ImGuiInputTextState* obj, int pos, const ImWchar* new_text, int new_text_len)
	{
		const bool is_resizable = (obj->Flags & ImGuiInputTextFlags_CallbackResize) != 0;
		const int text_len = obj->CurLenW;
		IM_ASSERT(pos <= text_len);

		const int new_text_len_utf8 = ImTextCountUtf8BytesFromStr(new_text, new_text + new_text_len);
		if (!is_resizable && (new_text_len_utf8 + obj->CurLenA + 1 > obj->BufCapacityA))
			return false;

		// Grow internal buffer if needed
		if (new_text_len + text_len + 1 > obj->TextW.Size)
		{
			if (!is_resizable)
				return false;
			IM_ASSERT(text_len < obj->TextW.Size);
			obj->TextW.resize(text_len + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1);
		}

		ImWchar* text = obj->TextW.Data;
		if (pos != text_len)
			memmove(text + pos + new_text_len, text + pos, (size_t)(text_len - pos) * sizeof(ImWchar));
		memcpy(text + pos, new_text, (size_t)new_text_len * sizeof(ImWchar));

		obj->Edited = true;
		obj->CurLenW += new_text_len;
		obj->CurLenA += new_text_len_utf8;
		obj->TextW[obj->CurLenW] = '\0';

		return true;
	}

	// We don't use an enum so we can build even with conflicting symbols (if another user of stb_textedit.h leak their STB_TEXTEDIT_K_* symbols)
#define STB_TEXTEDIT_K_LEFT         0x200000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x200001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x200002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x200003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x200004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x200005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x200006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x200007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x200008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x200009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x20000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x20000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x20000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x20000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_PGUP         0x20000E // keyboard input to move cursor up a page
#define STB_TEXTEDIT_K_PGDOWN       0x20000F // keyboard input to move cursor down a page
#define STB_TEXTEDIT_K_SHIFT        0x400000

#define STB_TEXTEDIT_IMPLEMENTATION
#define STB_TEXTEDIT_memmove memmove
#include "imstb_textedit.h"

// stb_textedit internally allows for a single undo record to do addition and deletion, but somehow, calling
// the stb_textedit_paste() function creates two separate records, so we perform it manually. (FIXME: Report to nothings/stb?)
	static void stb_textedit_replace(ImGuiInputTextState* str, STB_TexteditState* state, const STB_TEXTEDIT_CHARTYPE* text, int text_len)
	{
		stb_text_makeundo_replace(str, state, 0, str->CurLenW, text_len);
		ImStb::STB_TEXTEDIT_DELETECHARS(str, 0, str->CurLenW);
		state->cursor = state->select_start = state->select_end = 0;
		if (text_len <= 0)
			return;
		if (ImStb::STB_TEXTEDIT_INSERTCHARS(str, 0, text, text_len))
		{
			state->cursor = state->select_start = state->select_end = text_len;
			state->has_preferred_x = 0;
			return;
		}
		IM_ASSERT(0); // Failed to insert character, normally shouldn't happen because of how we currently use stb_textedit_replace()
	}

} // namespace ImStb

void InputTextReconcileUndoStateAfterUserCallback(ImGuiInputTextState* state, const char* new_buf_a, int new_length_a)
{
	ImGuiContext& g = *GImGui;
	const ImWchar* old_buf = state->TextW.Data;
	const int old_length = state->CurLenW;
	const int new_length = ImTextCountCharsFromUtf8(new_buf_a, new_buf_a + new_length_a);
	g.TempBuffer.reserve_discard((new_length + 1) * sizeof(ImWchar));
	ImWchar* new_buf = (ImWchar*)(void*)g.TempBuffer.Data;
	ImTextStrFromUtf8(new_buf, new_length + 1, new_buf_a, new_buf_a + new_length_a);

	const int shorter_length = ImMin(old_length, new_length);
	int first_diff;
	for (first_diff = 0; first_diff < shorter_length; first_diff++)
		if (old_buf[first_diff] != new_buf[first_diff])
			break;
	if (first_diff == old_length && first_diff == new_length)
		return;

	int old_last_diff = old_length - 1;
	int new_last_diff = new_length - 1;
	for (; old_last_diff >= first_diff && new_last_diff >= first_diff; old_last_diff--, new_last_diff--)
		if (old_buf[old_last_diff] != new_buf[new_last_diff])
			break;

	const int insert_len = new_last_diff - first_diff + 1;
	const int delete_len = old_last_diff - first_diff + 1;
	if (insert_len > 0 || delete_len > 0)
		if (STB_TEXTEDIT_CHARTYPE* p = stb_text_createundo(&state->Stb.undostate, first_diff, delete_len, insert_len))
			for (int i = 0; i < delete_len; i++)
				p[i] = ImStb::STB_TEXTEDIT_GETCHAR(state, first_diff + i);
}

void InputTextDeactivateHook(ImGuiID id)
{
	ImGuiContext& g = *GImGui;
	ImGuiInputTextState* state = &g.InputTextState;
	if (id == 0 || state->ID != id)
		return;
	g.InputTextDeactivatedState.ID = state->ID;
	if (state->Flags & ImGuiInputTextFlags_ReadOnly)
	{
		g.InputTextDeactivatedState.TextA.resize(0); // In theory this data won't be used, but clear to be neat.
	}
	else
	{
		IM_ASSERT(state->TextA.Data != 0);
		g.InputTextDeactivatedState.TextA.resize(state->CurLenA + 1);
		memcpy(g.InputTextDeactivatedState.TextA.Data, state->TextA.Data, state->CurLenA + 1);
	}
}

bool InputTextFilterCharacter(ImGuiContext* ctx, unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data, ImGuiInputSource input_source)
{
	IM_ASSERT(input_source == ImGuiInputSource_Keyboard || input_source == ImGuiInputSource_Clipboard);
	unsigned int c = *p_char;

	// Filter non-printable (NB: isprint is unreliable! see #2467)
	bool apply_named_filters = true;
	if (c < 0x20)
	{
		bool pass = false;
		pass |= (c == '\n' && (flags & ImGuiInputTextFlags_Multiline)); // Note that an Enter KEY will emit \r and be ignored (we poll for KEY in InputText() code)
		pass |= (c == '\t' && (flags & ImGuiInputTextFlags_AllowTabInput));
		if (!pass)
			return false;
		apply_named_filters = false; // Override named filters below so newline and tabs can still be inserted.
	}

	if (input_source != ImGuiInputSource_Clipboard)
	{
		// We ignore Ascii representation of delete (emitted from Backspace on OSX, see #2578, #2817)
		if (c == 127)
			return false;

		// Filter private Unicode range. GLFW on OSX seems to send private characters for special keys like arrow keys (FIXME)
		if (c >= 0xE000 && c <= 0xF8FF)
			return false;
	}

	// Filter Unicode ranges we are not handling in this build
	if (c > IM_UNICODE_CODEPOINT_MAX)
		return false;

	// Generic named filters
	if (apply_named_filters && (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsScientific)))
	{
		// The libc allows overriding locale, with e.g. 'setlocale(LC_NUMERIC, "de_DE.UTF-8");' which affect the output/input of printf/scanf to use e.g. ',' instead of '.'.
		// The standard mandate that programs starts in the "C" locale where the decimal point is '.'.
		// We don't really intend to provide widespread support for it, but out of empathy for people stuck with using odd API, we support the bare minimum aka overriding the decimal point.
		// Change the default decimal_point with:
		//   ImGui::GetIO()->PlatformLocaleDecimalPoint = *localeconv()->decimal_point;
		// Users of non-default decimal point (in particular ',') may be affected by word-selection logic (is_word_boundary_from_right/is_word_boundary_from_left) functions.
		ImGuiContext& g = *ctx;
		const unsigned c_decimal_point = (unsigned int)g.IO.PlatformLocaleDecimalPoint;
		if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific))
			if (c == '.' || c == ',')
				c = c_decimal_point;

		// Full-width -> half-width conversion for numeric fields (https://en.wikipedia.org/wiki/Halfwidth_and_Fullwidth_Forms_(Unicode_block)
		// While this is mostly convenient, this has the side-effect for uninformed users accidentally inputting full-width characters that they may
		// scratch their head as to why it works in numerical fields vs in generic text fields it would require support in the font.
		if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific | ImGuiInputTextFlags_CharsHexadecimal))
			if (c >= 0xFF01 && c <= 0xFF5E)
				c = c - 0xFF01 + 0x21;

		// Allow 0-9 . - + * /
		if (flags & ImGuiInputTextFlags_CharsDecimal)
			if (!(c >= '0' && c <= '9') && (c != c_decimal_point) && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
				return false;

		// Allow 0-9 . - + * / e E
		if (flags & ImGuiInputTextFlags_CharsScientific)
			if (!(c >= '0' && c <= '9') && (c != c_decimal_point) && (c != '-') && (c != '+') && (c != '*') && (c != '/') && (c != 'e') && (c != 'E'))
				return false;

		// Allow 0-9 a-F A-F
		if (flags & ImGuiInputTextFlags_CharsHexadecimal)
			if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
				return false;

		// Turn a-z into A-Z
		if (flags & ImGuiInputTextFlags_CharsUppercase)
			if (c >= 'a' && c <= 'z')
				c += (unsigned int)('A' - 'a');

		if (flags & ImGuiInputTextFlags_CharsNoBlank)
			if (ImCharIsBlankW(c))
				return false;

		*p_char = c;
	}

	// Custom callback filter
	if (flags & ImGuiInputTextFlags_CallbackCharFilter)
	{
		ImGuiContext& g = *GImGui;
		ImGuiInputTextCallbackData callback_data;
		callback_data.Ctx = &g;
		callback_data.EventFlag = ImGuiInputTextFlags_CallbackCharFilter;
		callback_data.EventChar = (ImWchar)c;
		callback_data.Flags = flags;
		callback_data.UserData = user_data;
		if (callback(&callback_data) != 0)
			return false;
		*p_char = callback_data.EventChar;
		if (!callback_data.EventChar)
			return false;
	}

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

bool draw::InputTextEx(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
	return InputTextExEx(label, NULL, buf, (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
}

bool draw::InputTextExEx(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	IM_ASSERT(buf != NULL && buf_size >= 0);
	IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
	IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackCompletion) && (flags & ImGuiInputTextFlags_AllowTabInput))); // Can't use both together (they both use tab key)

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	const bool RENDER_SELECTION_WHEN_INACTIVE = false;
	const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;
	const bool is_readonly = (flags & ImGuiInputTextFlags_ReadOnly) != 0;
	const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
	const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
	const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
	if (is_resizable)
		IM_ASSERT(callback != NULL); // Must provide a callback if you set the ImGuiInputTextFlags_CallbackResize flag!

	if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope (including the scrollbar)
		ImGui::BeginGroup();
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	const ImVec2 frame_size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(), (is_multiline ? g.FontSize * 8.0f : label_size.y) + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
	const ImVec2 total_size = ImVec2(frame_size.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_size.y);

	const ImRect frame_bb({ window->DC.CursorPos.x + frame_size.x / 1.6f,window->DC.CursorPos.y }, { window->DC.CursorPos.x + frame_size.x ,window->DC.CursorPos.y + frame_size.y });

	const ImRect total_bb(window->DC.CursorPos, { window->DC.CursorPos.x + total_size.x ,window->DC.CursorPos.y + total_size .y});

	ImGuiWindow* draw_window = window;
	ImVec2 inner_size = frame_size;
	ImGuiItemStatusFlags item_status_flags = 0;
	ImGuiLastItemData item_data_backup;
	if (is_multiline)
	{
		ImVec2 backup_pos = window->DC.CursorPos;
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
		{
			ImGui::EndGroup();
			return false;
		}
		item_status_flags = g.LastItemData.StatusFlags;
		item_data_backup = g.LastItemData;
		window->DC.CursorPos = backup_pos;

		// We reproduce the contents of BeginChildFrame() in order to provide 'label' so our window internal data are easier to read/debug.
		// FIXME-NAV: Pressing NavActivate will trigger general child activation right before triggering our own below. Harmless but bizarre.
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Ensure no clip rect so mouse hover can reach FramePadding edges
		bool child_visible = ImGui::BeginChildEx(label, id, frame_bb.GetSize(), true, ImGuiWindowFlags_NoMove);
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();
		if (!child_visible)
		{
			ImGui::EndChild();
			ImGui::EndGroup();
			return false;
		}
		draw_window = g.CurrentWindow; // Child window
		draw_window->DC.NavLayersActiveMaskNext |= (1 << draw_window->DC.NavLayerCurrent); // This is to ensure that EndChild() will display a navigation highlight so we can "enter" into it.
		draw_window->DC.CursorPos.x += style.FramePadding.x;
		draw_window->DC.CursorPos.y += style.FramePadding.y;
		inner_size.x -= draw_window->ScrollbarSizes.x;
	}
	else
	{
		// Support for internal ImGuiInputTextFlags_MergedItem flag, which could be redesigned as an ItemFlags if needed (with test performed in ItemAdd)
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!(flags & ImGuiInputTextFlags_MergedItem))
			if (!ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
				return false;
		item_status_flags = g.LastItemData.StatusFlags;
	}
	const bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
	if (hovered)
		g.MouseCursor = ImGuiMouseCursor_TextInput;

	// We are only allowed to access the state if we are already the active widget.
	ImGuiInputTextState* state = ImGui::GetInputTextState(id);

	const bool input_requested_by_tabbing = (item_status_flags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
	const bool input_requested_by_nav = (g.ActiveId != id) && ((g.NavActivateId == id) && ((g.NavActivateFlags & ImGuiActivateFlags_PreferInput) || (g.NavInputSource == ImGuiInputSource_Keyboard)));

	const bool user_clicked = hovered && io.MouseClicked[0];
	const bool user_scroll_finish = is_multiline && state != NULL && g.ActiveId == 0 && g.ActiveIdPreviousFrame == ImGui::GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
	const bool user_scroll_active = is_multiline && state != NULL && g.ActiveId == ImGui::GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
	bool clear_active_id = false;
	bool select_all = false;

	float scroll_y = is_multiline ? draw_window->Scroll.y : FLT_MAX;

	const bool init_changed_specs = (state != NULL && state->Stb.single_line != !is_multiline); // state != NULL means its our state.
	const bool init_make_active = (user_clicked || user_scroll_finish || input_requested_by_nav || input_requested_by_tabbing);
	const bool init_state = (init_make_active || user_scroll_active);
	if ((init_state && g.ActiveId != id) || init_changed_specs)
	{
		// Access state even if we don't own it yet.
		state = &g.InputTextState;
		state->CursorAnimReset();

		// Backup state of deactivating item so they'll have a chance to do a write to output buffer on the same frame they report IsItemDeactivatedAfterEdit (#4714)
		InputTextDeactivateHook(state->ID);

		// Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
		// From the moment we focused we are ignoring the content of 'buf' (unless we are in read-only mode)
		const int buf_len = (int)strlen(buf);
		state->InitialTextA.resize(buf_len + 1);    // UTF-8. we use +1 to make sure that .Data is always pointing to at least an empty string.
		memcpy(state->InitialTextA.Data, buf, buf_len + 1);

		// Preserve cursor position and undo/redo stack if we come back to same widget
		// FIXME: Since we reworked this on 2022/06, may want to differenciate recycle_cursor vs recycle_undostate?
		bool recycle_state = (state->ID == id && !init_changed_specs);
		if (recycle_state && (state->CurLenA != buf_len || (state->TextAIsValid && strncmp(state->TextA.Data, buf, buf_len) != 0)))
			recycle_state = false;

		// Start edition
		const char* buf_end = NULL;
		state->ID = id;
		state->TextW.resize(buf_size + 1);          // wchar count <= UTF-8 count. we use +1 to make sure that .Data is always pointing to at least an empty string.
		state->TextA.resize(0);
		state->TextAIsValid = false;                // TextA is not valid yet (we will display buf until then)
		state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, buf_size, buf, NULL, &buf_end);
		state->CurLenA = (int)(buf_end - buf);      // We can't get the result from ImStrncpy() above because it is not UTF-8 aware. Here we'll cut off malformed UTF-8.

		if (recycle_state)
		{
			// Recycle existing cursor/selection/undo stack but clamp position
			// Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
			state->CursorClamp();
		}
		else
		{
			state->ScrollX = 0.0f;
			stb_textedit_initialize_state(&state->Stb, !is_multiline);
		}

		if (!is_multiline)
		{
			if (flags & ImGuiInputTextFlags_AutoSelectAll)
				select_all = true;
			if (input_requested_by_nav && (!recycle_state || !(g.NavActivateFlags & ImGuiActivateFlags_TryToPreserveState)))
				select_all = true;
			if (input_requested_by_tabbing || (user_clicked && io.KeyCtrl))
				select_all = true;
		}

		if (flags & ImGuiInputTextFlags_AlwaysOverwrite)
			state->Stb.insert_mode = 1; // stb field name is indeed incorrect (see #2863)
	}

	const bool is_osx = io.ConfigMacOSXBehaviors;
	if (g.ActiveId != id && init_make_active)
	{
		IM_ASSERT(state && state->ID == id);
		ImGui::SetActiveID(id, window);
		ImGui::SetFocusID(id, window);
		ImGui::FocusWindow(window);
	}
	if (g.ActiveId == id)
	{
		// Declare some inputs, the other are registered and polled via Shortcut() routing system.
		if (user_clicked)
			ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
		g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
		if (is_multiline || (flags & ImGuiInputTextFlags_CallbackHistory))
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
		ImGui::SetKeyOwner(ImGuiKey_Home, id);
		ImGui::SetKeyOwner(ImGuiKey_End, id);
		if (is_multiline)
		{
			ImGui::SetKeyOwner(ImGuiKey_PageUp, id);
			ImGui::SetKeyOwner(ImGuiKey_PageDown, id);
		}
		if (is_osx)
			ImGui::SetKeyOwner(ImGuiMod_Alt, id);
		if (flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_AllowTabInput)) // Disable keyboard tabbing out as we will use the \t character.
			ImGui::SetShortcutRouting(ImGuiKey_Tab, id);
	}

	// We have an edge case if ActiveId was set through another widget (e.g. widget being swapped), clear id immediately (don't wait until the end of the function)
	if (g.ActiveId == id && state == NULL)
		ImGui::ClearActiveID();

	// Release focus when we click outside
	if (g.ActiveId == id && io.MouseClicked[0] && !init_state && !init_make_active) //-V560
		clear_active_id = true;

	// Lock the decision of whether we are going to take the path displaying the cursor or selection
	bool render_cursor = (g.ActiveId == id) || (state && user_scroll_active);
	bool render_selection = state && (state->HasSelection() || select_all) && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
	bool value_changed = false;
	bool validated = false;

	// When read-only we always use the live data passed to the function
	// FIXME-OPT: Because our selection/cursor code currently needs the wide text we need to convert it when active, which is not ideal :(
	if (is_readonly && state != NULL && (render_cursor || render_selection))
	{
		const char* buf_end = NULL;
		state->TextW.resize(buf_size + 1);
		state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, buf, NULL, &buf_end);
		state->CurLenA = (int)(buf_end - buf);
		state->CursorClamp();
		render_selection &= state->HasSelection();
	}

	// Select the buffer to render.
	const bool buf_display_from_state = (render_cursor || render_selection || g.ActiveId == id) && !is_readonly && state && state->TextAIsValid;
	const bool is_displaying_hint = (hint != NULL && (buf_display_from_state ? state->TextA.Data : buf)[0] == 0);

	// Password pushes a temporary font with only a fallback glyph
	if (is_password && !is_displaying_hint)
	{
		const ImFontGlyph* glyph = g.Font->FindGlyph('*');
		ImFont* password_font = &g.InputTextPasswordFont;
		password_font->FontSize = g.Font->FontSize;
		password_font->Scale = g.Font->Scale;
		password_font->Ascent = g.Font->Ascent;
		password_font->Descent = g.Font->Descent;
		password_font->ContainerAtlas = g.Font->ContainerAtlas;
		password_font->FallbackGlyph = glyph;
		password_font->FallbackAdvanceX = glyph->AdvanceX;
		IM_ASSERT(password_font->Glyphs.empty() && password_font->IndexAdvanceX.empty() && password_font->IndexLookup.empty());
		ImGui::PushFont(password_font);
	}

	// Process mouse inputs and character inputs
	int backup_current_text_length = 0;
	if (g.ActiveId == id)
	{
		IM_ASSERT(state != NULL);
		backup_current_text_length = state->CurLenA;
		state->Edited = false;
		state->BufCapacityA = buf_size;
		state->Flags = flags;

		// Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
		// Down the line we should have a cleaner library-wide concept of Selected vs Active.
		g.ActiveIdAllowOverlap = !io.MouseDown[0];

		// Edit in progress
		const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + state->ScrollX;
		const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y) : (g.FontSize * 0.5f));

		if (select_all)
		{
			state->SelectAll();
			state->SelectedAllMouseLock = true;
		}
		else if (hovered && io.MouseClickedCount[0] >= 2 && !io.KeyShift)
		{
			stb_textedit_click(state, &state->Stb, mouse_x, mouse_y);
			const int multiclick_count = (io.MouseClickedCount[0] - 2);
			if ((multiclick_count % 2) == 0)
			{
				// Double-click: Select word
				// We always use the "Mac" word advance for double-click select vs CTRL+Right which use the platform dependent variant:
				// FIXME: There are likely many ways to improve this behavior, but there's no "right" behavior (depends on use-case, software, OS)
				const bool is_bol = (state->Stb.cursor == 0) || ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb.cursor - 1) == '\n';
				if (STB_TEXT_HAS_SELECTION(&state->Stb) || !is_bol)
					state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
				//state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
				if (!STB_TEXT_HAS_SELECTION(&state->Stb))
					ImStb::stb_textedit_prep_selection_at_cursor(&state->Stb);
				state->Stb.cursor = ImStb::STB_TEXTEDIT_MOVEWORDRIGHT_MAC(state, state->Stb.cursor);
				state->Stb.select_end = state->Stb.cursor;
				ImStb::stb_textedit_clamp(state, &state->Stb);
			}
			else
			{
				// Triple-click: Select line
				const bool is_eol = ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb.cursor) == '\n';
				state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART);
				state->OnKeyPressed(STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT);
				state->OnKeyPressed(STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT);
				if (!is_eol && is_multiline)
				{
					ImSwap(state->Stb.select_start, state->Stb.select_end);
					state->Stb.cursor = state->Stb.select_end;
				}
				state->CursorFollow = false;
			}
			state->CursorAnimReset();
		}
		else if (io.MouseClicked[0] && !state->SelectedAllMouseLock)
		{
			if (hovered)
			{
				if (io.KeyShift)
					stb_textedit_drag(state, &state->Stb, mouse_x, mouse_y);
				else
					stb_textedit_click(state, &state->Stb, mouse_x, mouse_y);
				state->CursorAnimReset();
			}
		}
		else if (io.MouseDown[0] && !state->SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
		{
			stb_textedit_drag(state, &state->Stb, mouse_x, mouse_y);
			state->CursorAnimReset();
			state->CursorFollow = true;
		}
		if (state->SelectedAllMouseLock && !io.MouseDown[0])
			state->SelectedAllMouseLock = false;

		// We expect backends to emit a Tab key but some also emit a Tab character which we ignore (#2467, #1336)
		// (For Tab and Enter: Win32/SFML/Allegro are sending both keys and chars, GLFW and SDL are only sending keys. For Space they all send all threes)
		if ((flags & ImGuiInputTextFlags_AllowTabInput) && ImGui::Shortcut(ImGuiKey_Tab, id) && !is_readonly)
		{
			unsigned int c = '\t'; // Insert TAB
			if (InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data, ImGuiInputSource_Keyboard))
				state->OnKeyPressed((int)c);
		}

		// Process regular text input (before we check for Return because using some IME will effectively send a Return?)
		// We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
		const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeySuper);
		if (io.InputQueueCharacters.Size > 0)
		{
			if (!ignore_char_inputs && !is_readonly && !input_requested_by_nav)
				for (int n = 0; n < io.InputQueueCharacters.Size; n++)
				{
					// Insert character if they pass filtering
					unsigned int c = (unsigned int)io.InputQueueCharacters[n];
					if (c == '\t') // Skip Tab, see above.
						continue;
					if (InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data, ImGuiInputSource_Keyboard))
						state->OnKeyPressed((int)c);
				}

			// Consume characters
			io.InputQueueCharacters.resize(0);
		}
	}

	// Process other shortcuts/key-presses
	bool revert_edit = false;
	if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
	{
		IM_ASSERT(state != NULL);

		const int row_count_per_page = ImMax((int)((inner_size.y - style.FramePadding.y) / g.FontSize), 1);
		state->Stb.row_count_per_page = row_count_per_page;

		const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
		const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
		const bool is_startend_key_down = is_osx && io.KeySuper && !io.KeyCtrl && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End

		// Using Shortcut() with ImGuiInputFlags_RouteFocused (default policy) to allow routing operations for other code (e.g. calling window trying to use CTRL+A and CTRL+B: formet would be handled by InputText)
		// Otherwise we could simply assume that we own the keys as we are active.
		const ImGuiInputFlags f_repeat = ImGuiInputFlags_Repeat;
		const bool is_cut = (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_X, id, f_repeat) || ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_Delete, id, f_repeat)) && !is_readonly && !is_password && (!is_multiline || state->HasSelection());
		const bool is_copy = (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_C, id) || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Insert, id)) && !is_password && (!is_multiline || state->HasSelection());
		const bool is_paste = (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_V, id, f_repeat) || ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_Insert, id, f_repeat)) && !is_readonly;
		const bool is_undo = (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_Z, id, f_repeat)) && !is_readonly && is_undoable;
		const bool is_redo = (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_Y, id, f_repeat) || (is_osx && ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiMod_Shift | ImGuiKey_Z, id, f_repeat))) && !is_readonly && is_undoable;
		const bool is_select_all = ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_A, id);

		// We allow validate/cancel with Nav source (gamepad) to makes it easier to undo an accidental NavInput press with no keyboard wired, but otherwise it isn't very useful.
		const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
		const bool is_enter_pressed = ImGui::IsKeyPressed(ImGuiKey_Enter, true) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, true);
		const bool is_gamepad_validate = nav_gamepad_active && (ImGui::IsKeyPressed(ImGuiKey_NavGamepadActivate, false) || ImGui::IsKeyPressed(ImGuiKey_NavGamepadInput, false));
		const bool is_cancel = ImGui::Shortcut(ImGuiKey_Escape, id, f_repeat) || (nav_gamepad_active && ImGui::Shortcut(ImGuiKey_NavGamepadCancel, id, f_repeat));

		// FIXME: Should use more Shortcut() and reduce IsKeyPressed()+SetKeyOwner(), but requires modifiers combination to be taken account of.
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); }
		else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); }
		else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && is_multiline) { if (io.KeyCtrl) ImGui::SetScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); }
		else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && is_multiline) { if (io.KeyCtrl) ImGui::SetScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, ImGui::GetScrollMaxY())); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); }
		else if (ImGui::IsKeyPressed(ImGuiKey_PageUp) && is_multiline) { state->OnKeyPressed(STB_TEXTEDIT_K_PGUP | k_mask); scroll_y -= row_count_per_page * g.FontSize; }
		else if (ImGui::IsKeyPressed(ImGuiKey_PageDown) && is_multiline) { state->OnKeyPressed(STB_TEXTEDIT_K_PGDOWN | k_mask); scroll_y += row_count_per_page * g.FontSize; }
		else if (ImGui::IsKeyPressed(ImGuiKey_Home)) { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
		else if (ImGui::IsKeyPressed(ImGuiKey_End)) { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
		else if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !is_readonly && !is_cut)
		{
			if (!state->HasSelection())
			{
				// OSX doesn't seem to have Super+Delete to delete until end-of-line, so we don't emulate that (as opposed to Super+Backspace)
				if (is_wordmove_key_down)
					state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
			}
			state->OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && !is_readonly)
		{
			if (!state->HasSelection())
			{
				if (is_wordmove_key_down)
					state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
				else if (is_osx && io.KeySuper && !io.KeyAlt && !io.KeyCtrl)
					state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
			}
			state->OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
		}
		else if (is_enter_pressed || is_gamepad_validate)
		{
			// Determine if we turn Enter into a \n character
			bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
			if (!is_multiline || is_gamepad_validate || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
			{
				validated = true;
				if (io.ConfigInputTextEnterKeepActive && !is_multiline)
					state->SelectAll(); // No need to scroll
				else
					clear_active_id = true;
			}
			else if (!is_readonly)
			{
				unsigned int c = '\n'; // Insert new line
				if (InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data, ImGuiInputSource_Keyboard))
					state->OnKeyPressed((int)c);
			}
		}
		else if (is_cancel)
		{
			if (flags & ImGuiInputTextFlags_EscapeClearsAll)
			{
				if (buf[0] != 0)
				{
					revert_edit = true;
				}
				else
				{
					render_cursor = render_selection = false;
					clear_active_id = true;
				}
			}
			else
			{
				clear_active_id = revert_edit = true;
				render_cursor = render_selection = false;
			}
		}
		else if (is_undo || is_redo)
		{
			state->OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
			state->ClearSelection();
		}
		else if (is_select_all)
		{
			state->SelectAll();
			state->CursorFollow = true;
		}
		else if (is_cut || is_copy)
		{
			// Cut, Copy
			if (io.SetClipboardTextFn)
			{
				const int ib = state->HasSelection() ? ImMin(state->Stb.select_start, state->Stb.select_end) : 0;
				const int ie = state->HasSelection() ? ImMax(state->Stb.select_start, state->Stb.select_end) : state->CurLenW;
				const int clipboard_data_len = ImTextCountUtf8BytesFromStr(state->TextW.Data + ib, state->TextW.Data + ie) + 1;
				char* clipboard_data = (char*)IM_ALLOC(clipboard_data_len * sizeof(char));
				ImTextStrToUtf8(clipboard_data, clipboard_data_len, state->TextW.Data + ib, state->TextW.Data + ie);
				ImGui::SetClipboardText(clipboard_data);
				ImGui::MemFree(clipboard_data);
			}
			if (is_cut)
			{
				if (!state->HasSelection())
					state->SelectAll();
				state->CursorFollow = true;
				stb_textedit_cut(state, &state->Stb);
			}
		}
		else if (is_paste)
		{
			if (const char* clipboard = ImGui::GetClipboardText())
			{
				// Filter pasted buffer
				const int clipboard_len = (int)strlen(clipboard);
				ImWchar* clipboard_filtered = (ImWchar*)IM_ALLOC((clipboard_len + 1) * sizeof(ImWchar));
				int clipboard_filtered_len = 0;
				for (const char* s = clipboard; *s != 0; )
				{
					unsigned int c;
					s += ImTextCharFromUtf8(&c, s, NULL);
					if (!InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data, ImGuiInputSource_Clipboard))
						continue;
					clipboard_filtered[clipboard_filtered_len++] = (ImWchar)c;
				}
				clipboard_filtered[clipboard_filtered_len] = 0;
				if (clipboard_filtered_len > 0) // If everything was filtered, ignore the pasting operation
				{
					stb_textedit_paste(state, &state->Stb, clipboard_filtered, clipboard_filtered_len);
					state->CursorFollow = true;
				}
				ImGui::MemFree(clipboard_filtered);
			}
		}

		// Update render selection flag after events have been handled, so selection highlight can be displayed during the same frame.
		render_selection |= state->HasSelection() && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
	}

	// Process callbacks and apply result back to user's buffer.
	const char* apply_new_text = NULL;
	int apply_new_text_length = 0;
	if (g.ActiveId == id)
	{
		IM_ASSERT(state != NULL);
		if (revert_edit && !is_readonly)
		{
			if (flags & ImGuiInputTextFlags_EscapeClearsAll)
			{
				// Clear input
				IM_ASSERT(buf[0] != 0);
				apply_new_text = "";
				apply_new_text_length = 0;
				value_changed = true;
				STB_TEXTEDIT_CHARTYPE empty_string;
				stb_textedit_replace(state, &state->Stb, &empty_string, 0);
			}
			else if (strcmp(buf, state->InitialTextA.Data) != 0)
			{
				// Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
				// Push records into the undo stack so we can CTRL+Z the revert operation itself
				apply_new_text = state->InitialTextA.Data;
				apply_new_text_length = state->InitialTextA.Size - 1;
				value_changed = true;
				ImVector<ImWchar> w_text;
				if (apply_new_text_length > 0)
				{
					w_text.resize(ImTextCountCharsFromUtf8(apply_new_text, apply_new_text + apply_new_text_length) + 1);
					ImTextStrFromUtf8(w_text.Data, w_text.Size, apply_new_text, apply_new_text + apply_new_text_length);
				}
				stb_textedit_replace(state, &state->Stb, w_text.Data, (apply_new_text_length > 0) ? (w_text.Size - 1) : 0);
			}
		}

		// Apply ASCII value
		if (!is_readonly)
		{
			state->TextAIsValid = true;
			state->TextA.resize(state->TextW.Size * 4 + 1);
			ImTextStrToUtf8(state->TextA.Data, state->TextA.Size, state->TextW.Data, NULL);
		}

		// When using 'ImGuiInputTextFlags_EnterReturnsTrue' as a special case we reapply the live buffer back to the input buffer
		// before clearing ActiveId, even though strictly speaking it wasn't modified on this frame.
		// If we didn't do that, code like InputInt() with ImGuiInputTextFlags_EnterReturnsTrue would fail.
		// This also allows the user to use InputText() with ImGuiInputTextFlags_EnterReturnsTrue without maintaining any user-side storage
		// (please note that if you use this property along ImGuiInputTextFlags_CallbackResize you can end up with your temporary string object
		// unnecessarily allocating once a frame, either store your string data, either if you don't then don't use ImGuiInputTextFlags_CallbackResize).
		const bool apply_edit_back_to_user_buffer = !revert_edit || (validated && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
		if (apply_edit_back_to_user_buffer)
		{
			// Apply new value immediately - copy modified buffer back
			// Note that as soon as the input box is active, the in-widget value gets priority over any underlying modification of the input buffer
			// FIXME: We actually always render 'buf' when calling DrawList->AddText, making the comment above incorrect.
			// FIXME-OPT: CPU waste to do this every time the widget is active, should mark dirty state from the stb_textedit callbacks.

			// User callback
			if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackAlways)) != 0)
			{
				IM_ASSERT(callback != NULL);

				// The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
				ImGuiInputTextFlags event_flag = 0;
				ImGuiKey event_key = ImGuiKey_None;
				if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && ImGui::Shortcut(ImGuiKey_Tab, id))
				{
					event_flag = ImGuiInputTextFlags_CallbackCompletion;
					event_key = ImGuiKey_Tab;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
				{
					event_flag = ImGuiInputTextFlags_CallbackHistory;
					event_key = ImGuiKey_UpArrow;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
				{
					event_flag = ImGuiInputTextFlags_CallbackHistory;
					event_key = ImGuiKey_DownArrow;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackEdit) && state->Edited)
				{
					event_flag = ImGuiInputTextFlags_CallbackEdit;
				}
				else if (flags & ImGuiInputTextFlags_CallbackAlways)
				{
					event_flag = ImGuiInputTextFlags_CallbackAlways;
				}

				if (event_flag)
				{
					ImGuiInputTextCallbackData callback_data;
					callback_data.Ctx = &g;
					callback_data.EventFlag = event_flag;
					callback_data.Flags = flags;
					callback_data.UserData = callback_user_data;

					char* callback_buf = is_readonly ? buf : state->TextA.Data;
					callback_data.EventKey = event_key;
					callback_data.Buf = callback_buf;
					callback_data.BufTextLen = state->CurLenA;
					callback_data.BufSize = state->BufCapacityA;
					callback_data.BufDirty = false;

					// We have to convert from wchar-positions to UTF-8-positions, which can be pretty slow (an incentive to ditch the ImWchar buffer, see https://github.com/nothings/stb/issues/188)
					ImWchar* text = state->TextW.Data;
					const int utf8_cursor_pos = callback_data.CursorPos = ImTextCountUtf8BytesFromStr(text, text + state->Stb.cursor);
					const int utf8_selection_start = callback_data.SelectionStart = ImTextCountUtf8BytesFromStr(text, text + state->Stb.select_start);
					const int utf8_selection_end = callback_data.SelectionEnd = ImTextCountUtf8BytesFromStr(text, text + state->Stb.select_end);

					// Call user code
					callback(&callback_data);

					// Read back what user may have modified
					callback_buf = is_readonly ? buf : state->TextA.Data; // Pointer may have been invalidated by a resize callback
					IM_ASSERT(callback_data.Buf == callback_buf);         // Invalid to modify those fields
					IM_ASSERT(callback_data.BufSize == state->BufCapacityA);
					IM_ASSERT(callback_data.Flags == flags);
					const bool buf_dirty = callback_data.BufDirty;
					if (callback_data.CursorPos != utf8_cursor_pos || buf_dirty) { state->Stb.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos); state->CursorFollow = true; }
					if (callback_data.SelectionStart != utf8_selection_start || buf_dirty) { state->Stb.select_start = (callback_data.SelectionStart == callback_data.CursorPos) ? state->Stb.cursor : ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart); }
					if (callback_data.SelectionEnd != utf8_selection_end || buf_dirty) { state->Stb.select_end = (callback_data.SelectionEnd == callback_data.SelectionStart) ? state->Stb.select_start : ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd); }
					if (buf_dirty)
					{
						IM_ASSERT((flags & ImGuiInputTextFlags_ReadOnly) == 0);
						IM_ASSERT(callback_data.BufTextLen == (int)strlen(callback_data.Buf)); // You need to maintain BufTextLen if you change the text!
						InputTextReconcileUndoStateAfterUserCallback(state, callback_data.Buf, callback_data.BufTextLen); // FIXME: Move the rest of this block inside function and rename to InputTextReconcileStateAfterUserCallback() ?
						if (callback_data.BufTextLen > backup_current_text_length && is_resizable)
							state->TextW.resize(state->TextW.Size + (callback_data.BufTextLen - backup_current_text_length)); // Worse case scenario resize
						state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, callback_data.Buf, NULL);
						state->CurLenA = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
						state->CursorAnimReset();
					}
				}
			}

			// Will copy result string if modified
			if (!is_readonly && strcmp(state->TextA.Data, buf) != 0)
			{
				apply_new_text = state->TextA.Data;
				apply_new_text_length = state->CurLenA;
				value_changed = true;
			}
		}
	}

	// Handle reapplying final data on deactivation (see InputTextDeactivateHook() for details)
	if (g.InputTextDeactivatedState.ID == id)
	{
		if (g.ActiveId != id && ImGui::IsItemDeactivatedAfterEdit() && !is_readonly && strcmp(g.InputTextDeactivatedState.TextA.Data, buf) != 0)
		{
			apply_new_text = g.InputTextDeactivatedState.TextA.Data;
			apply_new_text_length = g.InputTextDeactivatedState.TextA.Size - 1;
			value_changed = true;
			//IMGUI_DEBUG_LOG("InputText(): apply Deactivated data for 0x%08X: \"%.*s\".\n", id, apply_new_text_length, apply_new_text);
		}
		g.InputTextDeactivatedState.ID = 0;
	}

	// Copy result to user buffer. This can currently only happen when (g.ActiveId == id)
	if (apply_new_text != NULL)
	{
		// We cannot test for 'backup_current_text_length != apply_new_text_length' here because we have no guarantee that the size
		// of our owned buffer matches the size of the string object held by the user, and by design we allow InputText() to be used
		// without any storage on user's side.
		IM_ASSERT(apply_new_text_length >= 0);
		if (is_resizable)
		{
			ImGuiInputTextCallbackData callback_data;
			callback_data.Ctx = &g;
			callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
			callback_data.Flags = flags;
			callback_data.Buf = buf;
			callback_data.BufTextLen = apply_new_text_length;
			callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
			callback_data.UserData = callback_user_data;
			callback(&callback_data);
			buf = callback_data.Buf;
			buf_size = callback_data.BufSize;
			apply_new_text_length = ImMin(callback_data.BufTextLen, buf_size - 1);
			IM_ASSERT(apply_new_text_length <= buf_size);
		}
		//IMGUI_DEBUG_PRINT("InputText(\"%s\"): apply_new_text length %d\n", label, apply_new_text_length);

		// If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
		ImStrncpy(buf, apply_new_text, ImMin(apply_new_text_length + 1, buf_size));
	}

	// Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
	// Otherwise request text input ahead for next frame.
	if (g.ActiveId == id && clear_active_id)
		ImGui::ClearActiveID();
	else if (g.ActiveId == id)
		g.WantTextInputNextFrame = 1;

	// Render frame
	if (!is_multiline)
	{
		ImGui::RenderNavHighlight(frame_bb, id);
		ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true,3.f);
	}

	const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + inner_size.x, frame_bb.Min.y + inner_size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 draw_pos = is_multiline ? draw_window->DC.CursorPos : ImVec2{frame_bb.Min.x + style.FramePadding.x, frame_bb.Min.y + style.FramePadding.y};
	ImVec2 text_size(0.0f, 0.0f);

	// Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
	// without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
	// Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
	const int buf_display_max_length = 2 * 1024 * 1024;
	const char* buf_display = buf_display_from_state ? state->TextA.Data : buf; //-V595
	const char* buf_display_end = NULL; // We have specialized paths below for setting the length
	if (is_displaying_hint)
	{
		buf_display = hint;
		buf_display_end = hint + strlen(hint);
	}

	// Render text. We currently only render selection when the widget is active or while scrolling.
	// FIXME: We could remove the '&& render_cursor' to keep rendering selection when inactive.
	if (render_cursor || render_selection)
	{
		IM_ASSERT(state != NULL);
		if (!is_displaying_hint)
			buf_display_end = buf_display + state->CurLenA;

		// Render text (with cursor and selection)
		// This is going to be messy. We need to:
		// - Display the text (this alone can be more easily clipped)
		// - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
		// - Measure text height (for scrollbar)
		// We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
		// FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
		const ImWchar* text_begin = state->TextW.Data;
		ImVec2 cursor_offset, select_start_offset;

		{
			// Find lines numbers straddling 'cursor' (slot 0) and 'select_start' (slot 1) positions.
			const ImWchar* searches_input_ptr[2] = { NULL, NULL };
			int searches_result_line_no[2] = { -1000, -1000 };
			int searches_remaining = 0;
			if (render_cursor)
			{
				searches_input_ptr[0] = text_begin + state->Stb.cursor;
				searches_result_line_no[0] = -1;
				searches_remaining++;
			}
			if (render_selection)
			{
				searches_input_ptr[1] = text_begin + ImMin(state->Stb.select_start, state->Stb.select_end);
				searches_result_line_no[1] = -1;
				searches_remaining++;
			}

			// Iterate all lines to find our line numbers
			// In multi-line mode, we never exit the loop until all lines are counted, so add one extra to the searches_remaining counter.
			searches_remaining += is_multiline ? 1 : 0;
			int line_count = 0;
			//for (const ImWchar* s = text_begin; (s = (const ImWchar*)wcschr((const wchar_t*)s, (wchar_t)'\n')) != NULL; s++)  // FIXME-OPT: Could use this when wchar_t are 16-bit
			for (const ImWchar* s = text_begin; *s != 0; s++)
				if (*s == '\n')
				{
					line_count++;
					if (searches_result_line_no[0] == -1 && s >= searches_input_ptr[0]) { searches_result_line_no[0] = line_count; if (--searches_remaining <= 0) break; }
					if (searches_result_line_no[1] == -1 && s >= searches_input_ptr[1]) { searches_result_line_no[1] = line_count; if (--searches_remaining <= 0) break; }
				}
			line_count++;
			if (searches_result_line_no[0] == -1)
				searches_result_line_no[0] = line_count;
			if (searches_result_line_no[1] == -1)
				searches_result_line_no[1] = line_count;

			// Calculate 2d position by finding the beginning of the line and measuring distance
			cursor_offset.x = InputTextCalcTextSizeW(&g, ImStrbolW(searches_input_ptr[0], text_begin), searches_input_ptr[0]).x;
			cursor_offset.y = searches_result_line_no[0] * g.FontSize;
			if (searches_result_line_no[1] >= 0)
			{
				select_start_offset.x = InputTextCalcTextSizeW(&g, ImStrbolW(searches_input_ptr[1], text_begin), searches_input_ptr[1]).x;
				select_start_offset.y = searches_result_line_no[1] * g.FontSize;
			}

			// Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
			if (is_multiline)
				text_size = ImVec2(inner_size.x, line_count * g.FontSize);
		}

		// Scroll
		if (render_cursor && state->CursorFollow)
		{
			// Horizontal scroll in chunks of quarter width
			if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
			{
				const float scroll_increment_x = inner_size.x * 0.25f;
				const float visible_width = inner_size.x - style.FramePadding.x;
				if (cursor_offset.x < state->ScrollX)
					state->ScrollX = IM_FLOOR(ImMax(0.0f, cursor_offset.x - scroll_increment_x));
				else if (cursor_offset.x - visible_width >= state->ScrollX)
					state->ScrollX = IM_FLOOR(cursor_offset.x - visible_width + scroll_increment_x);
			}
			else
			{
				state->ScrollX = 0.0f;
			}

			// Vertical scroll
			if (is_multiline)
			{
				// Test if cursor is vertically visible
				if (cursor_offset.y - g.FontSize < scroll_y)
					scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
				else if (cursor_offset.y - (inner_size.y - style.FramePadding.y * 2.0f) >= scroll_y)
					scroll_y = cursor_offset.y - inner_size.y + style.FramePadding.y * 2.0f;
				const float scroll_max_y = ImMax((text_size.y + style.FramePadding.y * 2.0f) - inner_size.y, 0.0f);
				scroll_y = ImClamp(scroll_y, 0.0f, scroll_max_y);
				draw_pos.y += (draw_window->Scroll.y - scroll_y);   // Manipulate cursor pos immediately avoid a frame of lag
				draw_window->Scroll.y = scroll_y;
			}

			state->CursorFollow = false;
		}

		// Draw selection
		const ImVec2 draw_scroll = ImVec2(state->ScrollX, 0.0f);
		if (render_selection)
		{
			const ImWchar* text_selected_begin = text_begin + ImMin(state->Stb.select_start, state->Stb.select_end);
			const ImWchar* text_selected_end = text_begin + ImMax(state->Stb.select_start, state->Stb.select_end);

			ImU32 bg_color = ImGui::GetColorU32(ImGuiCol_TextSelectedBg, render_cursor ? 1.0f : 0.6f); // FIXME: current code flow mandate that render_cursor is always true here, we are leaving the transparent one for tests.
			float bg_offy_up = is_multiline ? 0.0f : -1.0f;    // FIXME: those offsets should be part of the style? they don't play so well with multi-line selection.
			float bg_offy_dn = is_multiline ? 0.0f : 2.0f;

			ImVec2 detal_scroll = { select_start_offset.x - draw_scroll.x,select_start_offset.y - draw_scroll.y };
			ImVec2 rect_pos = { draw_pos.x + detal_scroll.x,draw_pos.y + detal_scroll .y};
			for (const ImWchar* p = text_selected_begin; p < text_selected_end; )
			{
				if (rect_pos.y > clip_rect.w + g.FontSize)
					break;
				if (rect_pos.y < clip_rect.y)
				{
					//p = (const ImWchar*)wmemchr((const wchar_t*)p, '\n', text_selected_end - p);  // FIXME-OPT: Could use this when wchar_t are 16-bit
					//p = p ? p + 1 : text_selected_end;
					while (p < text_selected_end)
						if (*p++ == '\n')
							break;
				}
				else
				{
					ImVec2 rect_size = InputTextCalcTextSizeW(&g, p, text_selected_end, &p, NULL, true);
					if (rect_size.x <= 0.0f) rect_size.x = IM_FLOOR(g.Font->GetCharAdvance((ImWchar)' ') * 0.50f); // So we can see selected empty lines
					ImRect rect(  ImVec2(rect_pos.x, rect_pos.x+bg_offy_up - g.FontSize),  ImVec2(rect_pos.x+rect_size.x, rect_pos.y+ bg_offy_dn));
					rect.ClipWith(clip_rect);
					if (rect.Overlaps(clip_rect))
						draw_window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color);
				}
				rect_pos.x = draw_pos.x - draw_scroll.x;
				rect_pos.y += g.FontSize;
			}
		}

		// We test for 'buf_display_max_length' as a way to avoid some pathological cases (e.g. single-line 1 MB string) which would make ImDrawList crash.
		if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
		{
			ImU32 col = ImGui::GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
			draw_window->DrawList->AddText(g.Font, g.FontSize, { draw_pos.x - draw_scroll.x ,draw_pos.y - draw_scroll .y}, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
		}

		// Draw blinking cursor
		if (render_cursor)
		{
			state->CursorAnim += io.DeltaTime;
			bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || ImFmod(state->CursorAnim, 1.20f) <= 0.80f;
			ImVec2 detal_scroll = { cursor_offset.x - draw_scroll.x ,cursor_offset.y - draw_scroll .y};
			ImVec2 cursor_screen_pos = ImFloor({ draw_pos.x + detal_scroll.x ,draw_pos.y + detal_scroll .y});
			ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
			if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
				draw_window->DrawList->AddLine(cursor_screen_rect.Min, cursor_screen_rect.GetBL(), ImGui::GetColorU32(ImGuiCol_Text));

			// Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
			if (!is_readonly)
			{
				g.PlatformImeData.WantVisible = true;
				g.PlatformImeData.InputPos = ImVec2(cursor_screen_pos.x - 1.0f, cursor_screen_pos.y - g.FontSize);
				g.PlatformImeData.InputLineHeight = g.FontSize;
			}
		}
	}
	else
	{
		// Render text only (no selection, no cursor)
		if (is_multiline)
			text_size = ImVec2(inner_size.x, InputTextCalcTextLenAndLineCount(buf_display, &buf_display_end) * g.FontSize); // We don't need width
		else if (!is_displaying_hint && g.ActiveId == id)
			buf_display_end = buf_display + state->CurLenA;
		else if (!is_displaying_hint)
			buf_display_end = buf_display + strlen(buf_display);

		if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
		{
			ImU32 col = ImGui::GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
			draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
		}
	}

	if (is_password && !is_displaying_hint)
		ImGui::PopFont();

	if (is_multiline)
	{
		// For focus requests to work on our multiline we need to ensure our child ItemAdd() call specifies the ImGuiItemFlags_Inputable (ref issue #4761)...
		ImGui::Dummy(ImVec2(text_size.x, text_size.y + style.FramePadding.y));
		g.NextItemData.ItemFlags |= ImGuiItemFlags_Inputable | ImGuiItemFlags_NoTabStop;
		ImGui::EndChild();
		item_data_backup.StatusFlags |= (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredWindow);

		// ...and then we need to undo the group overriding last item data, which gets a bit messy as EndGroup() tries to forward scrollbar being active...
		// FIXME: This quite messy/tricky, should attempt to get rid of the child window.
		ImGui::EndGroup();
		if (g.LastItemData.ID == 0)
		{
			g.LastItemData.ID = id;
			g.LastItemData.InFlags = item_data_backup.InFlags;
			g.LastItemData.StatusFlags = item_data_backup.StatusFlags;
		}
	}

	// Log as text
	if (g.LogEnabled && (!is_password || is_displaying_hint))
	{
		ImGui::LogSetNextTextDecoration("{", "}");
		ImGui::LogRenderedText(&draw_pos, buf_display, buf_display_end);
	}

	if (label_size.x > 0)
		ImGui::RenderText(ImVec2(total_bb.Min.x + 10 , frame_bb.Min.y + style.FramePadding.y), label);

	if (value_changed && !(flags & ImGuiInputTextFlags_NoMarkEdited))
		ImGui::MarkItemEdited(id);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Inputable);
	if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
		return validated;
	else
		return value_changed;
}

void draw::ProgressBarEx(const char* label, float curt, float max, const ImVec2& size_arg)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
	ImRect bb(pos, { pos.x + size.x ,pos.y + size .y});
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, 0))
		return;

	// Render
	//fraction = ImGui::ImSaturate(fraction);
	//ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
	bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
	//const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
	//ImGui::RenderRectFilledRangeH(window->DrawList, bb, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 0.0f, fraction, style.FrameRounding);
	window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32(211, 211, 211, 50));

	float w = (curt / max)*( bb.Max.x - bb.Min.x);
	window->DrawList->AddRectFilled(bb.Min, { bb.Min.x +w,bb.Max.y}, IM_COL32(0, 255, 0, 50));
	

	if (label_size.x > 0.0f)
		ImGui::RenderText({ bb.Min.x+w+5,bb.Max.y- label_size.y-5 }, label);
}





