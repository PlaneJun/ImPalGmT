
#include <iostream>
#include "render/render.h"
#include "widget/MainWidget.h"

MainWidget g_main_widget;


void render_callback(float w, float h)
{
	g_main_widget.SetWindowSize(w, h);
	g_main_widget.OnPaint();
}

int main()
{
	render::get_instasnce()->CreatGui(L"ImPalGmt", L"PalWorld Server Gm Tool", 800, 600, render_callback);
}
