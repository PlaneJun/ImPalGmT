#include <string>




class draw
{
public:
	static draw* get_instance();

	std::string string_To_UTF8(const std::string& str);
	void draw_string_center(float x, float y, uint32_t color, const char* message, ...);
	void draw_string(float x, float y, uint32_t color, const char* message, ...);
	void draw_line(float x1, float y1, float x2, float y2, uint32_t clr, float thickness = 1.0f);
	void draw_rect(float x, float y, float w, float h, uint32_t clr, float width);
	bool draw_filled_rect(const char *name, int w, int h, uint32_t color);
	void draw_triangle_filled(float x, float y, float size, uint32_t col);
	void draw_circle(float x, float y, float radius, uint32_t clr, float numseg, float thickness);

private:
	static draw* s_instance_;
};



