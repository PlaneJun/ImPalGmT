#pragma once
#include <imgui.h>

class MainWidget
{
public:
    void OnPaint();
    
    void SetWindowSize(float w, float h);

    ImVec2 GetWindowSize() { return size_; }
private:

    ImVec2 size_;

};