#include <memory>
#include "DreamForge.hpp"
#include <iostream>

class EditorApp : public JsquaredApp
{
public:
    EditorApp()=default;
    ~EditorApp()=default;
};

int main(int argumentCount, char** argumentVector)
{
    EditorApp app;
    DFLog::get().stdoutInfo("hello from the editor app\n");
    app.runApp();
}